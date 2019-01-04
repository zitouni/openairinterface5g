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

/*! \file fapi_nr_l1.c
 * \brief functions for FAPI L1 interface
 * \author R. Knopp, WEI-TAI CHEN
 * \date 2017, 2018
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: knopp@eurecom.fr, kroempa@gmail.com
 * \note
 * \warning
 */
#include "fapi_nr_l1.h"

int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req);
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req);

extern uint8_t nfapi_mode;

void handle_nr_nfapi_bch_pdu(PHY_VARS_gNB *gNB,
                             gNB_L1_rxtx_proc_t *proc,
                             nfapi_nr_dl_config_request_pdu_t *dl_config_pdu,
                             uint8_t *sdu)
{

  AssertFatal(dl_config_pdu->bch_pdu_rel15.length == 3, "BCH PDU has length %d != 3\n",
              dl_config_pdu->bch_pdu_rel15.length);

  LOG_I(PHY,"pbch_pdu[0]: %x,pbch_pdu[1]: %x,gNB->pbch_pdu[2]: %x\n",sdu[0],sdu[1],sdu[2]);
  gNB->pbch_pdu[0] = sdu[2];
  gNB->pbch_pdu[1] = sdu[1];
  gNB->pbch_pdu[2] = sdu[0];

  // adjust transmit amplitude here based on NFAPI info
}


void handle_nfapi_nr_dci_dl_pdu(PHY_VARS_gNB *gNB,
                                int frame, int slot,
                                gNB_L1_rxtx_proc_t *proc,
                                nfapi_nr_dl_config_request_pdu_t *dl_config_pdu,
                                nfapi_nr_dl_config_request_pdu_t *dl_config_dlsch_pdu)
{
  int idx                        = slot&1;
  NR_gNB_PDCCH *pdcch_vars       = &gNB->pdcch_vars;

  LOG_I(PHY,"Frame %d, Slot %d: DCI processing - populating pdcch_vars->dci_alloc[%d] proc:slot_tx:%d idx:%d pdcch_vars->num_dci:%d\n",frame,slot, pdcch_vars->num_dci, proc->slot_tx, idx, pdcch_vars->num_dci);

  // copy dci configuration into gNB structure
  nr_fill_dci_and_dlsch(gNB,frame,slot,proc,&pdcch_vars->dci_alloc[pdcch_vars->num_dci],&dl_config_pdu->dci_dl_pdu,&dl_config_dlsch_pdu->dlsch_pdu);

  LOG_I(PHY,"Frame %d, Slot %d: DCI processing - populated pdcch_vars->dci_alloc[%d] proc:slot_tx:%d idx:%d pdcch_vars->num_dci:%d\n",proc->frame_tx,proc->slot_tx, pdcch_vars->num_dci, proc->slot_tx, idx, pdcch_vars->num_dci);
}


void nr_schedule_response(NR_Sched_Rsp_t *Sched_INFO){
  PHY_VARS_gNB *gNB;
  gNB_L1_rxtx_proc_t *proc;
  // copy data from L2 interface into L1 structures
  module_id_t                   Mod_id       = Sched_INFO->module_id;
  uint8_t                       CC_id        = Sched_INFO->CC_id;
  nfapi_nr_dl_config_request_t  *DL_req      = Sched_INFO->DL_req;
  nfapi_tx_request_t            *TX_req      = Sched_INFO->TX_req;
  frame_t                       frame        = Sched_INFO->frame;
  sub_frame_t                   slot         = Sched_INFO->slot;

  AssertFatal(RC.gNB!=NULL,"RC.gNB is null\n");
  AssertFatal(RC.gNB[Mod_id]!=NULL,"RC.gNB[%d] is null\n",Mod_id);
  AssertFatal(RC.gNB[Mod_id][CC_id]!=NULL,"RC.gNB[%d][%d] is null\n",Mod_id,CC_id);

  gNB         = RC.gNB[Mod_id][CC_id];
  proc        = &gNB->proc.L1_proc;

  uint8_t number_dl_pdu             = DL_req->dl_config_request_body.number_pdu;

  nfapi_nr_dl_config_request_pdu_t *dl_config_pdu;
  nfapi_nr_dl_config_request_pdu_t *dl_config_dlsch_pdu;
 
  int i;

  LOG_D(PHY,"NFAPI: Sched_INFO:SFN/SF:%04d%d DL_req:SFN/SF:%04d%d:dl_pdu:%d tx_req:SFN/SF:%04d%d:pdus:%d \n",
        frame,slot,
        NFAPI_SFNSF2SFN(DL_req->sfn_sf),NFAPI_SFNSF2SF(DL_req->sfn_sf),number_dl_pdu,
        NFAPI_SFNSF2SFN(TX_req->sfn_sf),NFAPI_SFNSF2SF(TX_req->sfn_sf),TX_req->tx_request_body.number_of_pdus);

  int do_oai =0;
  int dont_send =0;
  gNB->pdcch_vars.num_dci = 0;
  gNB->pdcch_vars.num_pdsch_rnti = 0;

  gNB->pdcch_vars.num_dci=0;

  for (i=0;i<number_dl_pdu;i++) {
    dl_config_pdu = &DL_req->dl_config_request_body.dl_config_pdu_list[i];
    LOG_D(PHY,"NFAPI: dl_pdu %d : type %d\n",i,dl_config_pdu->pdu_type);
    printf("NFAPI: dl_pdu %d : type %d\n",i,dl_config_pdu->pdu_type);
    switch (dl_config_pdu->pdu_type) {
      case NFAPI_NR_DL_CONFIG_BCH_PDU_TYPE:
        AssertFatal(dl_config_pdu->bch_pdu_rel15.pdu_index < TX_req->tx_request_body.number_of_pdus,
                    "bch_pdu_rel8.pdu_index>=TX_req->number_of_pdus (%d>%d)\n",
                    dl_config_pdu->bch_pdu_rel15.pdu_index,
                    TX_req->tx_request_body.number_of_pdus);
        gNB->pbch_configured=1;
        do_oai=1;

        handle_nr_nfapi_bch_pdu(gNB,
                                proc,
                                dl_config_pdu,
                                TX_req->tx_request_body.tx_pdu_list[dl_config_pdu->bch_pdu_rel15.pdu_index].segments[0].segment_data);
      break;

      case NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE:
        dl_config_dlsch_pdu = &DL_req->dl_config_request_body.dl_config_pdu_list[++i];
        handle_nfapi_nr_dci_dl_pdu(gNB,
                                   frame, slot,
                                   proc,
                                   dl_config_pdu,
                                   dl_config_dlsch_pdu);
        gNB->pdcch_vars.num_dci++;
        gNB->pdcch_vars.num_pdsch_rnti++;
        do_oai=1;
      break;
    }
  }
  
  if (nfapi_mode && do_oai && !dont_send) {
    oai_nfapi_tx_req(Sched_INFO->TX_req);

    oai_nfapi_nr_dl_config_req(Sched_INFO->DL_req); // DJP - .dl_config_request_body.dl_config_pdu_list[0]); // DJP - FIXME TODO - yuk - only copes with 1 pdu
  }

}
