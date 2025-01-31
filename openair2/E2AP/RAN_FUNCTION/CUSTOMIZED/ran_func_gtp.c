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

#include "ran_func_gtp.h"

#include <assert.h>

#include "common/ran_context.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"

#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

#include "../surrey_log.h"

#if defined(NGRAN_GNB_CUCP)
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#endif

bool read_gtp_sm(void* data)
{
  assert(data != NULL);

  gtp_ind_data_t* gtp = (gtp_ind_data_t*)(data);
  // fill_gtp_ind_data(gtp);

  gtp->msg.tstamp = time_now_us();

  uint64_t ue_id_list[MAX_MOBILES_PER_GNB];
  size_t num_ues = nr_pdcp_get_num_ues(ue_id_list, MAX_MOBILES_PER_GNB);

  gtp->msg.len = num_ues;
  if (gtp->msg.len > 0) {
    gtp->msg.ngut = calloc(gtp->msg.len, sizeof(gtp_ngu_t_stats_t));
    assert(gtp->msg.ngut != NULL);
  } else {
    return false;
  }

  // Get handover information if available
  rrc_gNB_ue_context_t* ue_context = NULL;
  RB_FOREACH (ue_context, rrc_nr_ue_tree_s, &RC.nrrrc[0]->rrc_ue_head) {
    gNB_RRC_UE_t* UE = &ue_context->ue_context;
    if (UE && UE->ho_context) {
      gtp->msg.ho_info.ue_id = UE->rrc_ue_id; // or gNB_CU_ue_id
      gtp->msg.ho_info.source_du = UE->ho_context->source->du_ue_id;
      gtp->msg.ho_info.target_du = UE->ho_context->target->du_ue_id;
      gtp->msg.ho_info.ho_complete = true; // Since this is completion indication
      break; // Take the first UE in handover state
    } else {
      gtp->msg.ho_info.ue_id = UE->rrc_ue_id; // or gNB_CU_ue_id
      gtp->msg.ho_info.source_du = 11;
      gtp->msg.ho_info.target_du = 12;
      gtp->msg.ho_info.ho_complete = false; // Since this is completion indication
    }
    // Fill handover information
    LOG_SURREY_SM("Fill handover information : DU Handover ---> %s\n", gtp->msg.ho_info.ho_complete ? "TRUE" : "FALSE");
  }

#if defined(NGRAN_GNB_CUCP) && defined(NGRAN_GNB_CUUP)
  if (RC.nrrrc[0]->node_type == ngran_gNB_DU || RC.nrrrc[0]->node_type == ngran_gNB_CUCP)
    return false;
  assert((RC.nrrrc[0]->node_type == ngran_gNB_CU || RC.nrrrc[0]->node_type == ngran_gNB) && "Expected node types: CU or gNB-mono");

  for (size_t i = 0; i < num_ues; i++) {
    rrc_gNB_ue_context_t* ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[0], ue_id_list[i]);

    gtp->msg.ngut[i].rnti = ue_id_list[i];
    int nb_pdu_session = ue_context_p->ue_context.nb_of_pdusessions;
    if (nb_pdu_session > 0) {
      int nb_pdu_idx = nb_pdu_session - 1;
      gtp->msg.ngut[i].teidgnb = ue_context_p->ue_context.pduSession[nb_pdu_idx].param.gNB_teid_N3;
      gtp->msg.ngut[i].teidupf = ue_context_p->ue_context.pduSession[nb_pdu_idx].param.UPF_teid_N3;
      // TODO: one PDU session has multiple QoS Flow
      int nb_qos_flow = ue_context_p->ue_context.pduSession[nb_pdu_idx].param.nb_qos;
      if (nb_qos_flow > 0) {
        gtp->msg.ngut[i].qfi = ue_context_p->ue_context.pduSession[nb_pdu_idx].param.qos[nb_qos_flow - 1].qfi;
      }
    }
  }

  return true;

#elif defined(NGRAN_GNB_CUUP)
  // For the moment, CU-UP doesn't store PDU session information
  printf("GTP SM not yet implemented in CU-UP\n");
  return false;
#endif
}

void read_gtp_setup_sm(void* data)
{
  assert(data != NULL);
  assert(0 != 0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_gtp_sm(void const* src)
{
  assert(src != NULL);
  assert(0 != 0 && "Not supported");
}
