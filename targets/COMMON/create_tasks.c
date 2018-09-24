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

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "create_tasks.h"
# include "common/utils/LOG/log.h"
# include "common/ran_context.h"

# ifdef OPENAIR2
#   if defined(ENABLE_USE_MME)
#     include "s1ap_eNB.h"
#     include "nas_ue_task.h"
#     include "udp_eNB_task.h"
#     include "gtpv1u_eNB_task.h"
#   else
#     define EPC_MODE_ENABLED 0
#   endif
#   include "RRC/LTE/rrc_defs.h"
# endif
# include "sctp_eNB_task.h"
# include "f1ap_cu_task.h"
# include "f1ap_du_task.h"
# include "enb_app.h"

extern RAN_CONTEXT_t RC;
extern int emulate_rf;

int create_tasks(uint32_t enb_nb)
{
  ngran_node_t type = RC.rrc[0]->node_type;
  int rc;

  itti_wait_ready(1);

  if (enb_nb > 0) {
    LOG_I(ENB_APP, "Creating ENB_APP eNB Task\n");
    if (itti_create_task (TASK_ENB_APP, eNB_app_task, NULL) < 0) {
      LOG_E(ENB_APP, "Create task for eNB APP failed\n");
      return -1;
    }
    LOG_I(RRC,"Creating RRC eNB Task\n");
    if (itti_create_task (TASK_RRC_ENB, rrc_enb_task, NULL) < 0) {
      LOG_E(RRC, "Create task for RRC eNB failed\n");
      return -1;
    }
  }

  if (enb_nb > 0) {
    /* this task needs not be started if:
     * * there is no CU/DU split
     * * ENABLE_USE_MME is not defined
     * Since we cannot express this condition on both configuration and
     * compilation defines, we always start this task.
     */
    if (itti_create_task(TASK_SCTP, sctp_eNB_task, NULL) < 0) {
      LOG_E(SCTP, "Create task for SCTP failed\n");
      return -1;
    }
  }


  switch (type) {
  case ngran_eNB_CU:
  case ngran_ng_eNB_CU:
  case ngran_gNB_CU:
    if (enb_nb > 0) {
      rc = itti_create_task(TASK_CU_F1, F1AP_CU_task, NULL);
      AssertFatal(rc >= 0, "Create task for CU F1AP failed\n");
      //RS/BK: Fix me!
      rc = itti_create_task (TASK_L2L1, l2l1_task, NULL);
      AssertFatal(rc >= 0, "Create task for L2L1 failed\n");

    }
    /* fall through */
  case ngran_eNB:
  case ngran_ng_eNB:
  case ngran_gNB:
#if defined(ENABLE_USE_MME)
    if (enb_nb > 0) {
      rc = itti_create_task(TASK_S1AP, s1ap_eNB_task, NULL);
      AssertFatal(rc >= 0, "Create task for S1AP failed\n");
      if (!emulate_rf){
        rc = itti_create_task(TASK_UDP, udp_eNB_task, NULL);
        AssertFatal(rc >= 0, "Create task for UDP failed\n");
      }
      rc = itti_create_task(TASK_GTPV1_U, gtpv1u_eNB_task, NULL);
      AssertFatal(rc >= 0, "Create task for GTPV1U failed\n");
    }
#endif
    break;
  default:
    /* intentionally left blank */
    break;
  }
  switch (type) {
  case ngran_eNB_DU:
  case ngran_gNB_DU:
    if (enb_nb > 0) {
      rc = itti_create_task(TASK_DU_F1, F1AP_DU_task, NULL);
      AssertFatal(rc >= 0, "Create task for DU F1AP failed\n");
    }
    /* fall through */
  case ngran_eNB:
  case ngran_ng_eNB:
  case ngran_gNB:
    rc = itti_create_task (TASK_L2L1, l2l1_task, NULL);
    AssertFatal(rc >= 0, "Create task for L2L1 failed\n");
    break;
  default:
    /* intentioally left blank */
    break;
  }

  itti_wait_ready(0);
  return 0;
}
#endif
