/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/* \file NR_IF_Module.c
 * \brief functions for NR UE FAPI-like interface
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_nr_UE.h"
#include "NR_IF_Module.h"
#include "NR_MAC_UE/mac_proto.h"
#include "assertions.h"
#include "NR_MAC_UE/mac_extern.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "executables/softmodem-common.h"
#include "openair2/RRC/NR_UE/rrc_proto.h"
#include "openair2/GNB_APP/L1_nr_paramdef.h"
#include "openair2/GNB_APP/gnb_paramdef.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include <stdio.h>

#define MAX_IF_MODULES 100

UL_IND_t *UL_INFO = NULL;

static eth_params_t         stub_eth_params;
static nr_ue_if_module_t *nr_ue_if_module_inst[MAX_IF_MODULES];
static int ue_tx_sock_descriptor = -1;
static int ue_rx_sock_descriptor = -1;
static int g_harq_pid;
sem_t sfn_slot_semaphore;

queue_t nr_sfn_slot_queue;
queue_t nr_chan_param_queue;
queue_t nr_dl_tti_req_queue;
queue_t nr_tx_req_queue;
queue_t nr_ul_dci_req_queue;
queue_t nr_ul_tti_req_queue;
queue_t nr_wait_ul_tti_req_queue;

void nrue_init_standalone_socket(int tx_port, int rx_port)
{
  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(tx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (inet_pton(server_address.sin_family, stub_eth_params.remote_addr, &server_address.sin_addr) <= 0)
    {
      LOG_E(MAC, "Invalid standalone PNF Address\n");
      close(sd);
      return;
    }

    // Using connect to use send() instead of sendto()
    if (connect(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_tx_sock_descriptor == -1);
    ue_tx_sock_descriptor = sd;
    LOG_D(NR_RRC, "Successfully set up tx_socket in %s.\n", __FUNCTION__);
  }

  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(rx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (bind(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_rx_sock_descriptor == -1);
    ue_rx_sock_descriptor = sd;
    LOG_D(NR_RRC, "Successfully set up rx_socket in %s.\n", __FUNCTION__);
  }
  LOG_I(NR_RRC, "NRUE standalone socket info: tx_port %d  rx_port %d on %s.\n",
        tx_port, rx_port, stub_eth_params.remote_addr);
}

void send_nsa_standalone_msg(NR_UL_IND_t *UL_INFO, uint16_t msg_id)
{
  switch(msg_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_D(NR_MAC, "RACH header id :%d", UL_INFO->rach_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rach_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_RACH_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->rach_ind.sfn, UL_INFO->rach_ind.slot, UL_INFO->rach_ind.number_of_pdus);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_D(NR_MAC, "RX header id :%d", UL_INFO->rx_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rx_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_RX_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->rx_ind.sfn, UL_INFO->rx_ind.slot, UL_INFO->rx_ind.number_of_pdus);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_D(NR_MAC, "CRC header id :%d", UL_INFO->crc_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->crc_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_CRC_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->crc_ind.sfn, UL_INFO->crc_ind.slot, UL_INFO->crc_ind.number_crcs);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_I(NR_MAC, "UCI header id :%d", UL_INFO->uci_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->uci_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_UCI_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->uci_ind.sfn, UL_INFO->uci_ind.slot, UL_INFO->uci_ind.num_ucis);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
    break;
    default:
    break;
  }
}

static void fill_dl_info_with_pdcch(fapi_nr_dci_indication_t *dci, nfapi_nr_dl_dci_pdu_t *rx_dci, int idx)
{
    int num_bytes = (rx_dci->PayloadSizeBits + 7) / 8;
    LOG_I(NR_PHY, "[%d, %d] PDCCH DCI (Payload) for rnti %x with PayloadSizeBits %d, num_bytes %d\n",
          dci->SFN, dci->slot, rx_dci->RNTI, rx_dci->PayloadSizeBits, num_bytes);
    for (int k = 0; k < num_bytes; k++)
    {
        LOG_I(NR_MAC, "PDCCH DCI PDU payload[%d] = %d\n", k, rx_dci->Payload[k]);
        dci->dci_list[idx].payloadBits[k] = rx_dci->Payload[k];
    }
    dci->dci_list[idx].payloadSize = rx_dci->PayloadSizeBits;
    dci->dci_list[idx].rnti = rx_dci->RNTI;
    dci->number_of_dcis = idx + 1;
}

static void fill_mib_in_rx_ind(nfapi_nr_dl_tti_request_pdu_t *pdu_list, fapi_nr_rx_indication_t *rx_ind, int pdu_idx, int pdu_type)
{
    AssertFatal(pdu_idx < sizeof(rx_ind->rx_indication_body) / sizeof(rx_ind->rx_indication_body[0]),
                "pdu_index (%d) is greater than rx_indication_body size!\n", pdu_idx);
    AssertFatal(pdu_idx == rx_ind->number_pdus,  "Invalid pdu_idx %d!\n", pdu_idx);

    LOG_D(NR_MAC, "Recevied an SSB and are filling rx_ind with the MIB!\n");

    nfapi_nr_dl_tti_ssb_pdu_rel15_t *ssb_pdu = &pdu_list->ssb_pdu.ssb_pdu_rel15;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.cell_id = ssb_pdu->PhysCellId;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu = MALLOC(3 * sizeof(*rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu));
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[0] = (ssb_pdu->bchPayload) & 0xff;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[1] = (ssb_pdu->bchPayload >> 8) & 0xff;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[2] = (ssb_pdu->bchPayload >> 16) & 0xff;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.rsrp_dBm = ssb_pdu->ssbRsrp;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_index = ssb_pdu->SsbBlockIndex;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_length = pdu_list->PDUSize;
    rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_start_subcarrier = ssb_pdu->SsbSubcarrierOffset;
    rx_ind->rx_indication_body[pdu_idx].pdu_type = pdu_type;
    rx_ind->number_pdus = pdu_idx + 1;

}

static bool is_my_dci(NR_UE_MAC_INST_t *mac, nfapi_nr_dl_dci_pdu_t *received_pdu)
{
    /* For multiple UEs, we need to be able to filter the rx'd messages by
       the RNTI. The filtering is different between NSA mode and SA mode.
       NSA mode has a two step CFRA procedure and SA has a 4 step procedure.
       We only need to check if the rx'd RNTI doesnt match the CRNTI if the RAR
       has been processed already, in NSA mode.
       In SA, depending on the RA state, we can have a SIB (0xffff), RAR (0x10b),
       Msg3 (TC_RNTI) or an actual DCI message (CRNTI). When we get Msg3, the
       MAC instance of the UE still has a CRNTI = 0. We should only check if the
       CRNTI doesnt match the received RNTI in SA mode if Msg3 has been processed
       already. Only once the RA procedure succeeds is the CRNTI value updated
       to the TC_RNTI. */
    if (get_softmodem_params()->nsa)
    {
        if (received_pdu->RNTI != mac->crnti &&
            (received_pdu->RNTI != mac->ra.ra_rnti || mac->ra.RA_RAPID_found))
            return false;
    }
    if (get_softmodem_params()->sa)
    {
        if (received_pdu->RNTI != mac->crnti && mac->ra.ra_state == RA_SUCCEEDED)
            return false;
        if (received_pdu->RNTI != mac->ra.t_crnti && mac->ra.ra_state == WAIT_CONTENTION_RESOLUTION)
            return false;
        if (received_pdu->RNTI != 0x10b && mac->ra.ra_state == WAIT_RAR)
            return false;
        if (received_pdu->RNTI != 0xFFFF && mac->ra.ra_state <= GENERATE_PREAMBLE)
            return false;
    }
    return true;
}

static void copy_dl_tti_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    mac->nr_ue_emul_l1.expected_sib = false;
    memset(mac->nr_ue_emul_l1.index_has_sib, 0, sizeof(*mac->nr_ue_emul_l1.index_has_sib));
    mac->nr_ue_emul_l1.expected_rar = false;
    memset(mac->nr_ue_emul_l1.index_has_rar, 0, sizeof(*mac->nr_ue_emul_l1.index_has_rar));
    mac->nr_ue_emul_l1.expected_dci = false;
    memset(mac->nr_ue_emul_l1.index_has_dci, 0, sizeof(*mac->nr_ue_emul_l1.index_has_dci));
    int pdu_idx = 0;

    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    AssertFatal(num_pdus >= 0, "Invalid dl_tti_request number of PDUS\n");

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE)
        {
            LOG_I(NR_PHY, "[%d, %d] PDSCH PDU for rnti %x\n",
                dl_tti_request->SFN, dl_tti_request->Slot, pdu_list->pdsch_pdu.pdsch_pdu_rel15.rnti);
        }

        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE)
        {
            LOG_I(NR_PHY, "[%d, %d] PDCCH DCI PDU (Format for incoming PDSCH PDU)\n",
                dl_tti_request->SFN, dl_tti_request->Slot);
            uint16_t num_dcis = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
            if (num_dcis > 0)
            {
                if (!dl_info->dci_ind)
                {
                    dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
                }
                dl_info->dci_ind->SFN = dl_tti_request->SFN;
                dl_info->dci_ind->slot = dl_tti_request->Slot;
                AssertFatal(num_dcis <= sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]),
                            "The number of DCIs is greater than dci_list");
                for (int j = 0; j < num_dcis; j++)
                {
                    nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                    if (!is_my_dci(mac, dci_pdu_list))
                    {
                        continue;
                    }
                    fill_dl_info_with_pdcch(dl_info->dci_ind, dci_pdu_list, pdu_idx);
                    if (dci_pdu_list->RNTI == 0xffff)
                    {
                        mac->nr_ue_emul_l1.expected_sib = true;
                        mac->nr_ue_emul_l1.index_has_sib[j] = true;
                        LOG_D(NR_MAC, "Setting index_has_sib[%d] = true\n", j);
                    }
                    else if (dci_pdu_list->RNTI == mac->ra.ra_rnti)
                    {
                        mac->nr_ue_emul_l1.expected_rar = true;
                        mac->nr_ue_emul_l1.index_has_rar[j] = true;
                        LOG_D(NR_MAC, "Setting index_has_rar[%d] = true\n", j);
                    }
                    else
                    {
                        mac->nr_ue_emul_l1.expected_dci = true;
                        mac->nr_ue_emul_l1.index_has_dci[j] = true;
                        LOG_D(NR_MAC, "Setting index_has_dci[%d] = true\n", j);
                    }
                    pdu_idx++;
                }
            }
        }
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
        {
            /* If we get a MIB, we want to handle it right away and then come back.
               The MIB and SIB come in the same dl_tti_req but the MIB should be
               processed first and then the DCI and payload of the SIB1 can be
               processed. The MIB should be handled first and then the rx_ind
               will be freed after handling. This is why the PDU index will
               always be zero for the RX_IND becasue we should not have more than
               one MIB. */
            if (!dl_info->rx_ind)
            {
                dl_info->rx_ind = CALLOC(1, sizeof(*dl_info->rx_ind));
            }
            fapi_nr_rx_indication_t *rx_ind = dl_info->rx_ind;
            rx_ind->sfn = dl_tti_request->SFN;
            rx_ind->slot = dl_tti_request->Slot;
            fill_mib_in_rx_ind(pdu_list, rx_ind, 0, FAPI_NR_RX_PDU_TYPE_SSB);
            NR_UL_TIME_ALIGNMENT_t ul_time_alignment;
            memset(&ul_time_alignment, 0, sizeof(ul_time_alignment));
            nr_ue_dl_indication(&mac->dl_info, &ul_time_alignment);
        }
    }
    dl_info->slot = dl_tti_request->Slot;
    dl_info->frame = dl_tti_request->SFN;
}

static void fill_rx_ind(nfapi_nr_pdu_t *pdu_list, fapi_nr_rx_indication_t *rx_ind, int pdu_idx, int pdu_type)
{
    AssertFatal(pdu_list->num_TLV < sizeof(pdu_list->TLVs) / sizeof(pdu_list->TLVs[0]), "Num TLVs exceeds TLV array size");
    int length = 0;
    for (int j = 0; j < pdu_list->num_TLV; j++)
    {
        length += pdu_list->TLVs[j].length;
    }
    LOG_I(NR_PHY, "%s: num_tlv %d and length %d, pdu index %d\n",
        __FUNCTION__, pdu_list->num_TLV, length, pdu_idx);
    uint8_t *pdu = malloc(length);
    AssertFatal(pdu != NULL, "%s: Out of memory in malloc", __FUNCTION__);
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.pdu = pdu;
    for (int j = 0; j < pdu_list->num_TLV; j++)
    {
        const uint32_t *ptr;
        if (pdu_list->TLVs[j].tag)
            ptr = pdu_list->TLVs[j].value.ptr;
        else
            ptr = pdu_list->TLVs[j].value.direct;
        memcpy(pdu, ptr, pdu_list->TLVs[j].length);
        pdu += pdu_list->TLVs[j].length;
    }
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.ack_nack = 1;
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.pdu_length = length;
    rx_ind->rx_indication_body[pdu_idx].pdu_type = pdu_type;

}


static void copy_tx_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_tx_data_request_t *tx_data_request)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    int num_pdus = tx_data_request->Number_of_PDUs;
    AssertFatal(num_pdus >= 0, "Invalid tx_data_request number of PDUS\n");

    if (!dl_info->rx_ind)
    {
        dl_info->rx_ind = CALLOC(1, sizeof(fapi_nr_rx_indication_t));
    }
    AssertFatal(dl_info->rx_ind != NULL, "%s: Out of memory in calloc", __FUNCTION__);
    fapi_nr_rx_indication_t *rx_ind = dl_info->rx_ind;
    rx_ind->sfn = tx_data_request->SFN;
    rx_ind->slot = tx_data_request->Slot;

    int pdu_idx = 0;

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_pdu_t *pdu_list = &tx_data_request->pdu_list[i];
        if (mac->nr_ue_emul_l1.index_has_sib[i])
        {
            AssertFatal(!get_softmodem_params()->nsa,
                        "Should not be processing SIB in NSA mode, something bad happened\n");
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_SIB);
            pdu_idx++;
        }
        else if (mac->nr_ue_emul_l1.index_has_rar[i])
        {
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_RAR);
            pdu_idx++;
        }
        else if (mac->nr_ue_emul_l1.index_has_dci[i])
        {
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_DLSCH);
            pdu_idx++;
        }
        else
        {
            LOG_D(NR_MAC, "mac->nr_ue_emul_l1.index_has_dci[%d] = 0, so this index contained a DCI for a different UE\n", i);
        }

    }
    dl_info->slot = tx_data_request->Slot;
    dl_info->frame = tx_data_request->SFN;
    dl_info->rx_ind->number_pdus = pdu_idx;
}

static void copy_ul_dci_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_dci_request_t *ul_dci_req)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    int pdu_idx = 0;

    int num_pdus = ul_dci_req->numPdus;
    AssertFatal(num_pdus >= 0, "Invalid ul_dci_request number of PDUS\n");

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_ul_dci_request_pdus_t *pdu_list = &ul_dci_req->ul_dci_pdu_list[i];
        AssertFatal(pdu_list->PDUType == 0, "ul_dci_req pdu type != PUCCH");
        LOG_I(NR_PHY, "[%d %d] PUCCH PDU in ul_dci for rnti %x and numDLDCI = %d\n",
             ul_dci_req->SFN, ul_dci_req->Slot, pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu->RNTI,
             pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci);
        uint16_t num_dci = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
        if (num_dci > 0)
        {
            if (!dl_info->dci_ind)
            {
                dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
            }
            AssertFatal(dl_info->dci_ind != NULL, "%s: Out of memory in calloc", __FUNCTION__);
            dl_info->dci_ind->SFN = ul_dci_req->SFN;
            dl_info->dci_ind->slot = ul_dci_req->Slot;
            AssertFatal(num_dci < sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]), "The number of DCIs is greater than dci_list");
            for (int j = 0; j < num_dci; j++)
            {
                nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                if (dci_pdu_list->RNTI != mac->crnti)
                {
                  LOG_D(NR_MAC, "dci_pdu_list->RNTI (%x) != mac->crnti (%x)\n", dci_pdu_list->RNTI, mac->crnti);
                  continue;
                }
                fill_dl_info_with_pdcch(dl_info->dci_ind, dci_pdu_list, pdu_idx);
                pdu_idx++;
            }
        }
    }
    dl_info->frame = ul_dci_req->SFN;
    dl_info->slot = ul_dci_req->Slot;
}

static nfapi_nr_uci_indication_t *multiplex_uci_ind(NR_UE_MAC_INST_t *mac, int num_active_harqs)
{
    AssertFatal(num_active_harqs >= 0, "Invalid value for num_active_harqs %d\n", num_active_harqs);
    if (num_active_harqs == 0)
    {
        return NULL;
    }
    if (num_active_harqs != nr_uci_ind_queue.num_items)
    {
        LOG_I(NR_MAC, "The number of active harqs %d doesn't match the number of UCIs in the queue %lu\n",
                       num_active_harqs, nr_uci_ind_queue.num_items);
        return NULL;
    }

    nfapi_nr_uci_indication_t *uci_ind = MALLOC(sizeof(*uci_ind));
    uci_ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION;
    uci_ind->sfn = NFAPI_SFNSLOT2SFN(mac->nr_ue_emul_l1.active_harq_sfn_slot);
    uci_ind->slot = NFAPI_SFNSLOT2SLOT(mac->nr_ue_emul_l1.active_harq_sfn_slot);
    uci_ind->num_ucis = num_active_harqs;
    uci_ind->uci_list = CALLOC(uci_ind->num_ucis, sizeof(*uci_ind->uci_list));
    for (int i = 0; i < num_active_harqs; i++)
    {
        nfapi_nr_uci_indication_t *queued_uci_ind = get_queue(&nr_uci_ind_queue);
        AssertFatal(queued_uci_ind, "There was not a UCI in the queue!\n");
        nfapi_nr_uci_pucch_pdu_format_0_1_t *pdu_0_1 = &uci_ind->uci_list[i].pucch_pdu_format_0_1;

        /* In openair1/SCHED_NR_UE/fapi_nr_ue_l1.c nr_ue_schedule_response_stub(), the
        number of UCIs is hard coded to 1. This is why we always use index 0 of the
        queued UCI indication to fill the new multiplexed UCI indication */
        AssertFatal(queued_uci_ind->num_ucis == 1, "The number of UCIs from de-queueud UCI is not 1, its %d\n",
                    queued_uci_ind->num_ucis);
        uci_ind->uci_list[i].pdu_type = queued_uci_ind->uci_list[0].pdu_type;
        uci_ind->uci_list[i].pdu_size = queued_uci_ind->uci_list[0].pdu_size;

        nfapi_nr_uci_pucch_pdu_format_0_1_t *queued_pdu_0_1 = &queued_uci_ind->uci_list[0].pucch_pdu_format_0_1;
        pdu_0_1->handle = queued_pdu_0_1->handle;
        pdu_0_1->rnti = queued_pdu_0_1->rnti;
        pdu_0_1->pucch_format = queued_pdu_0_1->pucch_format;
        pdu_0_1->ul_cqi = queued_pdu_0_1->ul_cqi;
        pdu_0_1->timing_advance = queued_pdu_0_1->timing_advance;
        pdu_0_1->rssi = queued_pdu_0_1->rssi;
        free(queued_uci_ind->uci_list);
        queued_uci_ind->uci_list = NULL;
        free(queued_uci_ind);
        queued_uci_ind = NULL;
    }
    return uci_ind;
}

static void copy_ul_tti_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_tti_request_t *ul_tti_req)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    int num_pdus = ul_tti_req->n_pdus;
    AssertFatal(num_pdus >= 0, "Invalid ul_tti_request number of PDUS\n");
    AssertFatal(num_pdus <= sizeof(ul_tti_req->pdus_list) / sizeof(ul_tti_req->pdus_list[0]),
                "Too many pdus %d in ul_tti_req\n", num_pdus);

    bool sent_uci = false;
    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_ul_tti_request_number_of_pdus_t *pdu_list = &ul_tti_req->pdus_list[i];
        LOG_D(NR_PHY, "This is the pdu type %d and rnti %x and SR flag %d and harq_pdu_len %d in in ul_tti_req\n",
              pdu_list->pdu_type, ul_tti_req->pdus_list[i].pucch_pdu.rnti, pdu_list->pucch_pdu.sr_flag, pdu_list->pucch_pdu.bit_len_harq);
        if (pdu_list->pdu_type == NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE && pdu_list->pucch_pdu.rnti == mac->crnti)
        {
            AssertFatal(nr_uci_ind_queue.num_items >= 0, "Invalid num_items in UCI_ind queue %lu\n",
                        nr_uci_ind_queue.num_items);
            int num_active_harqs = pdu_list->pucch_pdu.bit_len_harq;
            LOG_I(NR_MAC, "The number of active harqs %d from ul_tti_req\n", num_active_harqs);
            nfapi_nr_uci_indication_t *uci_ind = multiplex_uci_ind(mac, num_active_harqs);

            if (uci_ind && uci_ind->num_ucis > 0)
            {
                LOG_D(NR_MAC, "This is the SFN/SF [%d, %d] and RNTI %x of the UCI ind. ul_tti_req.pdu[%d]->rnti = %x \n",
                        uci_ind->sfn, uci_ind->slot, uci_ind->uci_list[0].pucch_pdu_format_0_1.rnti, i, ul_tti_req->pdus_list[i].pucch_pdu.rnti);
                uci_ind->sfn = ul_tti_req->SFN;
                uci_ind->slot = ul_tti_req->Slot;
                for (int j = 0; j < uci_ind->num_ucis; j++)
                {
                    nfapi_nr_uci_pucch_pdu_format_0_1_t *pdu_0_1 = &uci_ind->uci_list[j].pucch_pdu_format_0_1;
                    if (pdu_list->pucch_pdu.sr_flag)
                    {
                        LOG_D(NR_MAC, "We have the SR flag in pdu i %d\n", i);
                        pdu_0_1->pduBitmap = 1; // (value->pduBitmap >> 1) & 0x01) == HARQ and (value->pduBitmap) & 0x01) == SR
                        pdu_0_1->sr = CALLOC(1, sizeof(*pdu_0_1->sr));
                        pdu_0_1->sr->sr_confidence_level = 0;
                        pdu_0_1->sr->sr_indication = 1;
                    }
                    if (pdu_list->pucch_pdu.bit_len_harq > 0)
                    {
                        LOG_D(NR_MAC, "We have the Harq len bits %d\n", pdu_list->pucch_pdu.bit_len_harq);
                        pdu_0_1->pduBitmap = 2; // (value->pduBitmap >> 1) & 0x01) == HARQ and (value->pduBitmap) & 0x01) == SR
                        pdu_0_1->harq = CALLOC(1, sizeof(*pdu_0_1->harq));
                        pdu_0_1->harq->num_harq = 1;
                        pdu_0_1->harq->harq_confidence_level = 0;
                        pdu_0_1->harq->harq_list = CALLOC(pdu_0_1->harq->num_harq, sizeof(*pdu_0_1->harq->harq_list));
                        for (int k = 0; k < pdu_0_1->harq->num_harq; k++)
                        {
                            pdu_0_1->harq->harq_list[k].harq_value = 0;
                        }
                    }
                }
                LOG_I(NR_MAC, "We have dequeued the previously filled uci_ind and updated the snf/slot to %d/%d.\n",
                      uci_ind->sfn, uci_ind->slot);
                NR_UL_IND_t UL_INFO = {
                    .uci_ind = *uci_ind,
                };
                send_nsa_standalone_msg(&UL_INFO, uci_ind->header.message_id);
                sent_uci = true;

                for (int k = 0; k < uci_ind->num_ucis; k++)
                {
                    nfapi_nr_uci_pucch_pdu_format_0_1_t *pdu_0_1 = &uci_ind->uci_list[k].pucch_pdu_format_0_1;
                    if (pdu_list->pucch_pdu.sr_flag)
                    {
                        free(pdu_0_1->sr);
                        pdu_0_1->sr = NULL;
                    }
                    if (pdu_list->pucch_pdu.bit_len_harq > 1)
                    {
                        free(pdu_0_1->harq->harq_list);
                        pdu_0_1->harq->harq_list = NULL;
                        free(pdu_0_1->harq);
                        pdu_0_1->harq = NULL;
                    }
                }
                free(uci_ind->uci_list);
                uci_ind->uci_list = NULL;
                free(uci_ind);
                uci_ind = NULL;
            }
        }
    }
    if (!sent_uci)
    {
        LOG_E(NR_MAC, "UCI ind not sent\n");
        if (!put_queue(&nr_ul_tti_req_queue, ul_tti_req))
        {
            LOG_E(NR_PHY, "put_queue failed for ul_tti_req.\n");
            free(ul_tti_req);
            ul_tti_req = NULL;
        }
    }
}

static void fill_dci_from_dl_config(nr_downlink_indication_t*dl_ind, fapi_nr_dl_config_request_t *dl_config)
{
  if (!dl_ind->dci_ind)
  {
    return;
  }

  AssertFatal(dl_config->number_pdus < sizeof(dl_config->dl_config_list) / sizeof(dl_config->dl_config_list[0]),
              "Too many dl_config pdus %d", dl_config->number_pdus);
  for (int i = 0; i < dl_config->number_pdus; i++)
  {
    LOG_I(PHY, "In %s: filling DCI with a total of %d total DL PDUs (dl_config %p) \n",
          __FUNCTION__, dl_config->number_pdus, dl_config);
    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15_dci = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
    int num_dci_options = rel15_dci->num_dci_options;
    if (num_dci_options <= 0)
    {
      LOG_I(NR_MAC, "num_dci_opts = %d for pdu[%d] in dl_config_list\n", rel15_dci->num_dci_options, i);
    }
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_length_options) / sizeof(rel15_dci->dci_length_options[0]),
                "num_dci_options %d > dci_length_options array\n", num_dci_options);
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_format_options) / sizeof(rel15_dci->dci_format_options[0]),
                "num_dci_options %d > dci_format_options array\n", num_dci_options);

    for (int j = 0; j < num_dci_options; j++)
    {
      int num_dcis = dl_ind->dci_ind->number_of_dcis;
      AssertFatal(num_dcis <= sizeof(dl_ind->dci_ind->dci_list) / sizeof(dl_ind->dci_ind->dci_list[0]),
                  "dl_config->number_pdus %d > dci_ind->dci_list array\n", num_dcis);
      for (int k = 0; k < num_dcis; k++)
      {
        LOG_I(NR_PHY, "Received len %d, length options[%d] %d, format assigned %d, format options[%d] %d\n",
                  dl_ind->dci_ind->dci_list[k].payloadSize, j, rel15_dci->dci_length_options[j],
                  dl_ind->dci_ind->dci_list[k].dci_format, j, rel15_dci->dci_format_options[j]);
        if (rel15_dci->dci_length_options[j] == dl_ind->dci_ind->dci_list[k].payloadSize)
        {
            dl_ind->dci_ind->dci_list[k].dci_format = rel15_dci->dci_format_options[j];
        }
        int CCEind = rel15_dci->CCE[j];
        int L = rel15_dci->L[j];
        dl_ind->dci_ind->dci_list[k].n_CCE = CCEind;
        dl_ind->dci_ind->dci_list[k].N_CCE = L;
      }
    }
  }
}

void check_and_process_dci(nfapi_nr_dl_tti_request_t *dl_tti_request,
                           nfapi_nr_tx_data_request_t *tx_data_request,
                           nfapi_nr_ul_dci_request_t *ul_dci_request,
                           nfapi_nr_ul_tti_request_t *ul_tti_request)
{
    frame_t frame = 0;
    int slot = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);

    if (pthread_mutex_lock(&mac->mutex_dl_info)) abort();

    if (dl_tti_request)
    {
        frame = dl_tti_request->SFN;
        slot = dl_tti_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] dl_tti_request\n", frame, slot);
        copy_dl_tti_req_to_dl_info(&mac->dl_info, dl_tti_request);
    }
    /* This checks if the previously recevied DCI matches our current RNTI
       value. The assumption is that if the DCI matches our RNTI, then the
       incoming tx_data_request is also destined for the current UE. If the
       RAR hasn't been processed yet, we do not want to be filtering the
       tx_data_requests. */
    if (tx_data_request && (mac->nr_ue_emul_l1.expected_sib ||
                            mac->nr_ue_emul_l1.expected_rar ||
                            mac->nr_ue_emul_l1.expected_dci))
    {
        frame = tx_data_request->SFN;
        slot = tx_data_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] PDSCH in tx_request\n", frame, slot);
        copy_tx_data_req_to_dl_info(&mac->dl_info, tx_data_request);
    }
    else if (ul_dci_request)
    {
        frame = ul_dci_request->SFN;
        slot = ul_dci_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] ul_dci_request\n", frame, slot);
        copy_ul_dci_data_req_to_dl_info(&mac->dl_info, ul_dci_request);
    }
    else if (ul_tti_request)
    {
        frame = ul_tti_request->SFN;
        slot = ul_tti_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] ul_tti_request\n", frame, slot);
        copy_ul_tti_data_req_to_dl_info(&mac->dl_info, ul_tti_request);
    }
    else
    {
        if (pthread_mutex_unlock(&mac->mutex_dl_info)) abort();
        LOG_E(NR_MAC, "Error! All indications were NULL\n");
        return;
    }


    NR_UL_TIME_ALIGNMENT_t ul_time_alignment;
    memset(&ul_time_alignment, 0, sizeof(ul_time_alignment));
    fill_dci_from_dl_config(&mac->dl_info, &mac->dl_config_request);
    nr_ue_dl_indication(&mac->dl_info, &ul_time_alignment);

    if (pthread_mutex_unlock(&mac->mutex_dl_info)) abort();

    // If we filled dl_info AFTER we got the slot indication, we want to check if we should fill tx_req:
    nr_uplink_indication_t ul_info;
    memset(&ul_info, 0, sizeof(ul_info));
    int slots_per_frame = 20; //30 kHZ subcarrier spacing
    int slot_ahead = 2; // TODO: Make this dynamic
    ul_info.frame_rx = frame;
    ul_info.slot_rx = slot;
    ul_info.slot_tx = (slot + slot_ahead) % slots_per_frame;
    ul_info.frame_tx = (ul_info.slot_rx + slot_ahead >= slots_per_frame) ? ul_info.frame_rx + 1 : ul_info.frame_rx;
    ul_info.ue_sched_mode = SCHED_ALL;
    if (mac->scc || mac->scc_SIB)
    {
        if (is_nr_UL_slot(mac->scc ?
                          mac->scc->tdd_UL_DL_ConfigurationCommon :
                          mac->scc_SIB->tdd_UL_DL_ConfigurationCommon,
                          ul_info.slot_tx,
                          mac->frame_type))
            nr_ue_ul_indication(&ul_info);
    }
}

void save_nr_measurement_info(nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    char buffer[MAX_MESSAGE_SIZE];
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: dl_tti_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }
    LOG_D(NR_PHY, "%s: dl_tti_request number of PDUS: %d\n", __FUNCTION__, num_pdus);
    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
        {
            LOG_D(NR_PHY, "Cell_id: %d, the ssb_block_idx %d, sc_offset: %d and payload %d\n",
                pdu_list->ssb_pdu.ssb_pdu_rel15.PhysCellId,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbBlockIndex,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset,
                pdu_list->ssb_pdu.ssb_pdu_rel15.bchPayload);
            pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp = 60;
            LOG_D(NR_RRC, "Setting pdulist[%d].ssbRsrp to %d\n", i, pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp);
        }
    }

    size_t pack_len = nfapi_nr_p7_message_pack((void *)dl_tti_request,
                                    buffer,
                                    sizeof(buffer),
                                    NULL);
    if (pack_len < 0)
    {
        LOG_E(NR_PHY, "%s: Error packing nr p7 message.\n", __FUNCTION__);
    }
    nsa_sendmsg_to_lte_ue(buffer, pack_len, NR_UE_RRC_MEASUREMENT);
    LOG_A(NR_RRC, "Populated NR_UE_RRC_MEASUREMENT information and sent to LTE UE\n");
}

static void enqueue_nr_nfapi_msg(void *buffer, ssize_t len, nfapi_p7_message_header_t header)
{

    switch (header.message_id)
    {
        case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
        {
            nfapi_nr_dl_tti_request_t *dl_tti_request = MALLOC(sizeof(*dl_tti_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, dl_tti_request,
                                            sizeof(*dl_tti_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message dl_tti_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST message in sfn/slot %d %d. \n",
                    dl_tti_request->SFN, dl_tti_request->Slot);
            if (!put_queue(&nr_dl_tti_req_queue, dl_tti_request))
            {
                LOG_E(NR_PHY, "put_queue failed for dl_tti_request.\n");
                free(dl_tti_request);
                dl_tti_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
        {
            nfapi_nr_tx_data_request_t *tx_data_request = MALLOC(sizeof(*tx_data_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, tx_data_request,
                                        sizeof(*tx_data_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message tx_data_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST message in SFN/slot %d %d. \n",
                    tx_data_request->SFN, tx_data_request->Slot);
            if (!put_queue(&nr_tx_req_queue, tx_data_request))
            {
                LOG_E(NR_PHY, "put_queue failed for tx_request.\n");
                free(tx_data_request);
                tx_data_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
        {
            nfapi_nr_ul_dci_request_t *ul_dci_request = MALLOC(sizeof(*ul_dci_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, ul_dci_request,
                                            sizeof(*ul_dci_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_dci_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST message in SFN/slot %d %d. \n",
                    ul_dci_request->SFN, ul_dci_request->Slot);
            if (!put_queue(&nr_ul_dci_req_queue, ul_dci_request))
            {
                LOG_E(NR_PHY, "put_queue failed for ul_dci_request.\n");
                free(ul_dci_request);
                ul_dci_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
        {
            nfapi_nr_ul_tti_request_t *ul_tti_request = MALLOC(sizeof(*ul_tti_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, ul_tti_request,
                                           sizeof(*ul_tti_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_tti_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST message in SFN/slot %d %d.\n",
                  ul_tti_request->SFN, ul_tti_request->Slot);
            if (nr_uci_ind_queue.num_items > 0) //TODO: In the future UL_TTIs can be for ULSCH and SRS.
            {
                LOG_D(NR_MAC, "We added UL_TTI_REQ to queue for sfn slot %d %d\n",
                      ul_tti_request->SFN, ul_tti_request->Slot);
                if (!put_queue(&nr_ul_tti_req_queue, ul_tti_request))
                {
                    LOG_E(NR_PHY, "put_queue failed for ul_tti_request.\n");
                    free(ul_tti_request);
                    ul_tti_request = NULL;
                }
            }
            /* TODO: This indicates that dl_tti_req was late or never arrived. If there are
               not any prepared uci indications, the NRUE likely never had time to
               populate the message is the dl_tti_req came in late and we received a
               ul_tti_req immediately after the dl_tti_request. This is an attempt to
               mitigate proxy timing issues. */
            else if (nr_uci_ind_queue.num_items == 0)
            {
                LOG_D(NR_MAC, "We added UL_TTI_REQ to queue for sfn slot %d %d\n",
                      ul_tti_request->SFN, ul_tti_request->Slot);
                nfapi_nr_ul_tti_request_t *evicted_ul_tti_req = put_queue_replace(&nr_wait_ul_tti_req_queue, ul_tti_request);
                free(evicted_ul_tti_req);
            }
            break;
        }

        default:
            LOG_E(NR_PHY, "Invalid nFAPI message. Header ID %d\n",
                  header.message_id);
            break;
    }
    return;
}

void *nrue_standalone_pnf_task(void *context)
{
  struct sockaddr_in server_address;
  socklen_t addr_len = sizeof(server_address);
  int sd = ue_rx_sock_descriptor;
  assert(sd > 0);

  char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];

  LOG_I(NR_RRC, "Successfully started %s.\n", __FUNCTION__);

  while (true)
  {
    ssize_t len = recvfrom(sd, buffer, sizeof(buffer), MSG_TRUNC, (struct sockaddr *)&server_address, &addr_len);
    if (len == -1)
    {
      LOG_E(NR_PHY, "reading from standalone pnf sctp socket failed \n");
      continue;
    }
    if (len > sizeof(buffer))
    {
      LOG_E(NR_PHY, "%s(%d). Message truncated. %zd\n", __FUNCTION__, __LINE__, len);
      continue;
    }
    if (len == sizeof(uint16_t))
    {
      uint16_t *sfn_slot = CALLOC(1, sizeof(*sfn_slot));
      memcpy(sfn_slot, buffer, sizeof(*sfn_slot));

      LOG_I(NR_PHY, "Received from proxy sfn %d slot %d\n",
            NFAPI_SFNSLOT2SFN(*sfn_slot), NFAPI_SFNSLOT2SLOT(*sfn_slot));

      if (!put_queue(&nr_sfn_slot_queue, sfn_slot))
      {
        LOG_E(NR_PHY, "put_queue failed for sfn slot.\n");
      }

      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(NR_PHY, "sem_post() error\n");
        abort();
      }
    }
    else if (len == sizeof(nr_phy_channel_params_t))
    {
      nr_phy_channel_params_t *ch_info = CALLOC(1, sizeof(*ch_info));
      memcpy(ch_info, buffer, sizeof(*ch_info));

      LOG_D(NR_PHY, "Received_SINR = %f, sfn:slot %d:%d\n",
            ch_info->sinr, NFAPI_SFNSLOT2SFN(ch_info->sfn_slot), NFAPI_SFNSLOT2SLOT(ch_info->sfn_slot));

      if (!put_queue(&nr_chan_param_queue, ch_info))
      {
        LOG_E(NR_PHY, "put_queue failed for sfn slot.\n");
      }

      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(MAC, "sem_post() error\n");
        abort();
      }
    }
    else
    {
      nfapi_p7_message_header_t header;
      if (nfapi_p7_message_header_unpack(buffer, len, &header, sizeof(header), NULL) < 0)
      {
        LOG_E(NR_PHY, "Header unpack failed for nrue_standalone pnf\n");
        continue;
      }
      enqueue_nr_nfapi_msg(buffer, len, header);
    }
  } //while(true)
}

//  L2 Abstraction Layer
int handle_bcch_bch(module_id_t module_id, int cc_id,
                    unsigned int gNB_index, uint8_t *pduP,
                    unsigned int additional_bits,
                    uint32_t ssb_index, uint32_t ssb_length,
                    uint16_t ssb_start_subcarrier, uint16_t cell_id){

  return nr_ue_decode_mib(module_id,
			  cc_id,
			  gNB_index,
			  additional_bits,
			  ssb_length,  //  Lssb = 64 is not support    
			  ssb_index,
			  pduP,
			  ssb_start_subcarrier,
			  cell_id);

}

//  L2 Abstraction Layer
int handle_bcch_dlsch(module_id_t module_id, int cc_id, unsigned int gNB_index, uint8_t ack_nack, uint8_t *pduP, uint32_t pdu_len){
  return nr_ue_decode_BCCH_DL_SCH(module_id, cc_id, gNB_index, ack_nack, pduP, pdu_len);
}

//  L2 Abstraction Layer
int handle_dci(module_id_t module_id, int cc_id, unsigned int gNB_index, frame_t frame, int slot, fapi_nr_dci_indication_pdu_t *dci){

  return nr_ue_process_dci_indication_pdu(module_id, cc_id, gNB_index, frame, slot, dci);

}

// L2 Abstraction Layer
// Note: sdu should always be processed because data and timing advance updates are transmitted by the UE
int8_t handle_dlsch(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.harq_pid = g_harq_pid;

  update_harq_status(dl_info->module_id,
                     dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.harq_pid,
                     dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack);
  if(dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack)
    nr_ue_send_sdu(dl_info, ul_time_alignment, pdu_id);

  return 0;
}

void update_harq_status(module_id_t module_id, uint8_t harq_pid, uint8_t ack_nack) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  NR_UE_HARQ_STATUS_t *current_harq = &mac->dl_harq_info[harq_pid];

  if (current_harq->active) {
    current_harq->ack = ack_nack;
    current_harq->ack_received = true;
    LOG_D(PHY,"Updating harq_status for harq_id %d,ack/nak %d\n",harq_pid,current_harq->ack);
  }
  else {
    //shouldn't get here
    LOG_E(MAC, "Trying to process acknack for an inactive harq process (%d)\n", harq_pid);
  }
}


int nr_ue_ul_indication(nr_uplink_indication_t *ul_info){

  NR_UE_L2_STATE_t ret;
  module_id_t module_id = ul_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  if (ul_info->ue_sched_mode == ONLY_PUSCH) {
    ret = nr_ue_scheduler(NULL, ul_info);
    return 0;
  }
  if (ul_info->ue_sched_mode == SCHED_ALL) {
    ret = nr_ue_scheduler(NULL, ul_info);
  }
  else
    LOG_D(NR_MAC, "In %s():%d not calling scheduler. sched mode = %d and mac->ra.ra_state = %d\n",
        __FUNCTION__, __LINE__, ul_info->ue_sched_mode, mac->ra.ra_state);

  NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon = mac->scc != NULL ? mac->scc->tdd_UL_DL_ConfigurationCommon : mac->scc_SIB->tdd_UL_DL_ConfigurationCommon;

  if (is_nr_UL_slot(tdd_UL_DL_ConfigurationCommon, ul_info->slot_tx, mac->frame_type) && !get_softmodem_params()->phy_test)
    nr_ue_prach_scheduler(module_id, ul_info->frame_tx, ul_info->slot_tx, ul_info->thread_id);

  if (is_nr_UL_slot(tdd_UL_DL_ConfigurationCommon, ul_info->slot_tx, mac->frame_type))
    nr_ue_pucch_scheduler(module_id, ul_info->frame_tx, ul_info->slot_tx, ul_info->thread_id);

  switch(ret){
  case UE_CONNECTION_OK:
    break;
  case UE_CONNECTION_LOST:
    break;
  case UE_PHY_RESYNCH:
    break;
  case UE_PHY_HO_PRACH:
    break;
  default:
    break;
  }

  return 0;
}

int nr_ue_dl_indication(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  uint32_t ret_mask = 0x0;
  module_id_t module_id = dl_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;

  if ((!dl_info->dci_ind && !dl_info->rx_ind)) {
    // UL indication to schedule DCI reception
    nr_ue_scheduler(dl_info, NULL);
  } else {
    // UL indication after reception of DCI or DL PDU
    if (dl_info && dl_info->dci_ind && dl_info->dci_ind->number_of_dcis) {
      LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][DCI_IND]\n");
      for (int i = 0; i < dl_info->dci_ind->number_of_dcis; i++) {
        LOG_D(MAC,">>>NR_IF_Module i=%d, dl_info->dci_ind->number_of_dcis=%d\n",i,dl_info->dci_ind->number_of_dcis);
        nr_scheduled_response_t scheduled_response;
        int8_t ret = handle_dci(dl_info->module_id,
                                dl_info->cc_id,
                                dl_info->gNB_index,
                                dl_info->frame,
                                dl_info->slot,
                                dl_info->dci_ind->dci_list+i);

        fapi_nr_dci_indication_pdu_t *dci_index = dl_info->dci_ind->dci_list+i;

        /* The check below filters out UL_DCIs (format 7) which are being processed as DL_DCIs. */
        if (dci_index->dci_format == 7 && mac->ra.ra_state == RA_SUCCEEDED) {
          LOG_D(NR_MAC, "We are filtering a UL_DCI to prevent it from being treated like a DL_DCI\n");
          break;
        }
        dci_pdu_rel15_t *def_dci_pdu_rel15 = &mac->def_dci_pdu_rel15[dci_index->dci_format];
        g_harq_pid = def_dci_pdu_rel15->harq_pid;
        LOG_D(NR_MAC, "Setting harq_pid = %d and dci_index = %d (based on format)\n", g_harq_pid, dci_index->dci_format);

        ret_mask |= (ret << FAPI_NR_DCI_IND);
        if (ret >= 0) {
          AssertFatal( nr_ue_if_module_inst[module_id] != NULL, "IF module is NULL!\n" );
          AssertFatal( nr_ue_if_module_inst[module_id]->scheduled_response != NULL, "scheduled_response is NULL!\n" );
          fill_scheduled_response(&scheduled_response, dl_config, NULL, NULL, dl_info->module_id, dl_info->cc_id, dl_info->frame, dl_info->slot, dl_info->thread_id);
          nr_ue_if_module_inst[module_id]->scheduled_response(&scheduled_response);
        }
        memset(def_dci_pdu_rel15, 0, sizeof(*def_dci_pdu_rel15));
      }
      free(dl_info->dci_ind);
      dl_info->dci_ind = NULL;
    }

    if (dl_info->rx_ind != NULL) {

      for (int i=0; i<dl_info->rx_ind->number_pdus; ++i) {

        LOG_D(MAC, "In %s sending DL indication to MAC. 1 PDU type %d of %d total number of PDUs \n",
          __FUNCTION__,
          dl_info->rx_ind->rx_indication_body[i].pdu_type,
          dl_info->rx_ind->number_pdus);

        switch(dl_info->rx_ind->rx_indication_body[i].pdu_type){
          case FAPI_NR_RX_PDU_TYPE_SSB:
            mac->ssb_rsrp_dBm = (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.rsrp_dBm;
            ret_mask |= (handle_bcch_bch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.pdu,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.additional_bits,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.ssb_index,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.ssb_length,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.ssb_start_subcarrier,
                                         (dl_info->rx_ind->rx_indication_body+i)->ssb_pdu.cell_id)) << FAPI_NR_RX_PDU_TYPE_SSB;

            break;
          case FAPI_NR_RX_PDU_TYPE_SIB:
            ret_mask |= (handle_bcch_dlsch(dl_info->module_id,
                                           dl_info->cc_id, dl_info->gNB_index,
                                           (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.ack_nack,
                                           (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu,
                                           (dl_info->rx_ind->rx_indication_body+i)->pdsch_pdu.pdu_length)) << FAPI_NR_RX_PDU_TYPE_SIB;
            break;
          case FAPI_NR_RX_PDU_TYPE_DLSCH:
            ret_mask |= (handle_dlsch(dl_info, ul_time_alignment, i)) << FAPI_NR_RX_PDU_TYPE_DLSCH;
            break;
          case FAPI_NR_RX_PDU_TYPE_RAR:
            ret_mask |= (handle_dlsch(dl_info, ul_time_alignment, i)) << FAPI_NR_RX_PDU_TYPE_RAR;
            break;
          default:
            break;
        }
      }
      free(dl_info->rx_ind);
      dl_info->rx_ind = NULL;
    }

    //clean up nr_downlink_indication_t *dl_info
    free(dl_info->dci_ind);
    dl_info->dci_ind = NULL;
    free(dl_info->rx_ind);
    dl_info->rx_ind  = NULL;

  }
  return 0;
}

nr_ue_if_module_t *nr_ue_if_module_init(uint32_t module_id){

  if (nr_ue_if_module_inst[module_id] == NULL) {
    nr_ue_if_module_inst[module_id] = (nr_ue_if_module_t *)malloc(sizeof(nr_ue_if_module_t));
    memset((void*)nr_ue_if_module_inst[module_id],0,sizeof(nr_ue_if_module_t));

    nr_ue_if_module_inst[module_id]->cc_mask=0;
    nr_ue_if_module_inst[module_id]->current_frame = 0;
    nr_ue_if_module_inst[module_id]->current_slot = 0;
    nr_ue_if_module_inst[module_id]->phy_config_request = nr_ue_phy_config_request;
    if (get_softmodem_params()->emulate_l1)
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response_stub;
    else
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response;
    nr_ue_if_module_inst[module_id]->dl_indication = nr_ue_dl_indication;
    nr_ue_if_module_inst[module_id]->ul_indication = nr_ue_ul_indication;
  }

  return nr_ue_if_module_inst[module_id];
}

int nr_ue_if_module_kill(uint32_t module_id) {

  if (nr_ue_if_module_inst[module_id] != NULL){
    free(nr_ue_if_module_inst[module_id]);
  }
  return 0;
}

int nr_ue_dcireq(nr_dcireq_t *dcireq) {

  fapi_nr_dl_config_request_t *dl_config = &dcireq->dl_config_req;
  NR_UE_MAC_INST_t *UE_mac = get_mac_inst(0);
  dl_config->sfn = UE_mac->dl_config_request.sfn;
  dl_config->slot = UE_mac->dl_config_request.slot;

  LOG_D(PHY, "Entering UE DCI configuration frame %d slot %d \n", dcireq->frame, dcireq->slot);

  ue_dci_configuration(UE_mac, dl_config, dcireq->frame, dcireq->slot);

  return 0;
}

void RCconfig_nr_ue_L1(void) {
  int j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST, NULL, 0};

  config_getlist(&L1_ParamList, L1_Params, sizeof(L1_Params) / sizeof(paramdef_t), NULL);
  if (L1_ParamList.numelt > 0) {
    for (j = 0; j < L1_ParamList.numelt; j++) {
      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        stub_eth_params.local_if_name = strdup(
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        stub_eth_params.my_addr = strdup(
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        stub_eth_params.remote_addr = strdup(
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        stub_eth_params.my_portc =
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        stub_eth_params.remote_portc =
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        stub_eth_params.my_portd =
            *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        stub_eth_params.remote_portd =
            *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        stub_eth_params.transp_preference = ETH_UDP_MODE;
      }
    }
  }
}
