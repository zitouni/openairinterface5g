/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file enb_agent_mac.c
 * \brief enb agent message handler for MAC layer
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "enb_agent_mac.h"
#include "enb_agent_extern.h"
#include "enb_agent_common.h"
#include "enb_agent_mac_internal.h"

#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/enb_agent_mac_proto.h"

#include "log.h"


/*Flags showing if a mac agent has already been registered*/
unsigned int mac_agent_registered[NUM_MAX_ENB];

/*Array containing the Agent-MAC interfaces*/
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];

int enb_agent_mac_handle_stats(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg){

  // TODO: Must deal with sanitization of input
  // TODO: Must check if RNTIs and cell ids of the request actually exist
  // TODO: Must resolve conflicts among stats requests

  int i;
  void *buffer;
  int size;
  err_code_t err_code;
  xid_t xid;
  uint32_t usec_interval, sec_interval;

  //TODO: We do not deal with multiple CCs at the moment and eNB id is 0
  int cc_id = 0;
  int enb_id = mod_id;

  //eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  //UE_list_t *eNB_UE_list=  &eNB->UE_list;

  report_config_t report_config;

  uint32_t ue_flags = 0;
  uint32_t c_flags = 0;

  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;

  Protocol__PrpStatsRequest *stats_req = input->stats_request_msg;
  xid = (stats_req->header)->xid;

  // Check the type of request that is made
  switch(stats_req->body_case) {
  case PROTOCOL__PRP_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST: ;
    Protocol__PrpCompleteStatsRequest *comp_req = stats_req->complete_stats_request;
    if (comp_req->report_frequency == PROTOCOL__PRP_STATS_REPORT_FREQ__PRSRF_OFF) {
      /*Disable both periodic and continuous updates*/
      enb_agent_disable_cont_mac_stats_update(mod_id);
      enb_agent_destroy_timer_by_task_id(xid);
      *msg = NULL;
      return 0;
    } else { //One-off, periodical or continuous reporting
      //Set the proper flags
      ue_flags = comp_req->ue_report_flags;
      c_flags = comp_req->cell_report_flags;
      //Create a list of all eNB RNTIs and cells

      //Set the number of UEs and create list with their RNTIs stats configs
      report_config.nr_ue = get_num_ues(mod_id); //eNB_UE_list->num_UEs
      report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
      if (report_config.ue_report_type == NULL) {
	// TODO: Add appropriate error code
	err_code = -100;
	goto error;
      }
      for (i = 0; i < report_config.nr_ue; i++) {
	report_config.ue_report_type[i].ue_rnti = get_ue_crnti(enb_id, i); //eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,i)][i].crnti;
	report_config.ue_report_type[i].ue_report_flags = ue_flags;
      }
      //Set the number of CCs and create a list with the cell stats configs
      report_config.nr_cc = MAX_NUM_CCs;
      report_config.cc_report_type = (cc_report_type_t *) malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
      if (report_config.cc_report_type == NULL) {
	// TODO: Add appropriate error code
	err_code = -100;
	goto error;
      }
      for (i = 0; i < report_config.nr_cc; i++) {
	//TODO: Must fill in the proper cell ids
	report_config.cc_report_type[i].cc_id = i;
	report_config.cc_report_type[i].cc_report_flags = c_flags;
      }
      /* Check if request was periodical */
      if (comp_req->report_frequency == PROTOCOL__PRP_STATS_REPORT_FREQ__PRSRF_PERIODICAL) {
	/* Create a one off progran message as an argument for the periodical task */
	Protocol__ProgranMessage *timer_msg;
	stats_request_config_t request_config;
	request_config.report_type = PROTOCOL__PRP_STATS_TYPE__PRST_COMPLETE_STATS;
	request_config.report_frequency = PROTOCOL__PRP_STATS_REPORT_FREQ__PRSRF_ONCE;
	request_config.period = 0;
	/* Need to make sure that the ue flags are saved (Bug) */
	if (report_config.nr_ue == 0) {
	  report_config.nr_ue = 1;
	  report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t));
	   if (report_config.ue_report_type == NULL) {
	     // TODO: Add appropriate error code
	     err_code = -100;
	     goto error;
	   }
	   report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
	   report_config.ue_report_type[0].ue_report_flags = ue_flags;
	}
	request_config.config = &report_config;
	enb_agent_mac_stats_request(enb_id, xid, &request_config, &timer_msg);
	/* Create a timer */
	long timer_id = 0;
	enb_agent_timer_args_t *timer_args;
	timer_args = malloc(sizeof(enb_agent_timer_args_t));
	memset (timer_args, 0, sizeof(enb_agent_timer_args_t));
	timer_args->mod_id = enb_id;
	timer_args->msg = timer_msg;
	/*Convert subframes to usec time*/
	usec_interval = 1000*comp_req->sf;
	sec_interval = 0;
	/*add seconds if required*/
	if (usec_interval >= 1000*1000) {
	  sec_interval = usec_interval/(1000*1000);
	  usec_interval = usec_interval%(1000*1000);
	}
	enb_agent_create_timer(sec_interval, usec_interval, ENB_AGENT_DEFAULT, enb_id, ENB_AGENT_TIMER_TYPE_PERIODIC, xid, enb_agent_handle_timed_task,(void*) timer_args, &timer_id);
      } else if (comp_req->report_frequency == PROTOCOL__PRP_STATS_REPORT_FREQ__PRSRF_CONTINUOUS) {
	/*If request was for continuous updates, disable the previous configuration and
	  set up a new one*/
	enb_agent_disable_cont_mac_stats_update(mod_id);
	stats_request_config_t request_config;
	request_config.report_type = PROTOCOL__PRP_STATS_TYPE__PRST_COMPLETE_STATS;
	request_config.report_frequency = PROTOCOL__PRP_STATS_REPORT_FREQ__PRSRF_ONCE;
	request_config.period = 0;
	/* Need to make sure that the ue flags are saved (Bug) */
	if (report_config.nr_ue == 0) {
	  report_config.nr_ue = 1;
	  report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t));
	  if (report_config.ue_report_type == NULL) {
	    // TODO: Add appropriate error code
	    err_code = -100;
	    goto error;
	  }
	  report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
	  report_config.ue_report_type[0].ue_report_flags = ue_flags;
	}
	request_config.config = &report_config;
	enb_agent_enable_cont_mac_stats_update(enb_id, xid, &request_config);
      }
    }
    break;
  case PROTOCOL__PRP_STATS_REQUEST__BODY_CELL_STATS_REQUEST:;
    Protocol__PrpCellStatsRequest *cell_req = stats_req->cell_stats_request;
    // UE report config will be blank
    report_config.nr_ue = 0;
    report_config.ue_report_type = NULL;
    report_config.nr_cc = cell_req->n_cell;
    report_config.cc_report_type = (cc_report_type_t *) malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
    if (report_config.cc_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_cc; i++) {
	//TODO: Must fill in the proper cell ids
      report_config.cc_report_type[i].cc_id = cell_req->cell[i];
      report_config.cc_report_type[i].cc_report_flags = cell_req->flags;
    }
    break;
  case PROTOCOL__PRP_STATS_REQUEST__BODY_UE_STATS_REQUEST:;
    Protocol__PrpUeStatsRequest *ue_req = stats_req->ue_stats_request;
    // Cell report config will be blank
    report_config.nr_cc = 0;
    report_config.cc_report_type = NULL;
    report_config.nr_ue = ue_req->n_rnti;
    report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
    if (report_config.ue_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_ue; i++) {
      report_config.ue_report_type[i].ue_rnti = ue_req->rnti[i];
      report_config.ue_report_type[i].ue_report_flags = ue_req->flags;
    }
    break;
  default:
    //TODO: Add appropriate error code
    err_code = -100;
    goto error;
  }

  if (enb_agent_mac_stats_reply(enb_id, xid, &report_config, msg) < 0 ){
    err_code = PROTOCOL__PROGRAN_ERR__MSG_BUILD;
    goto error;
  }

  free(report_config.ue_report_type);
  free(report_config.cc_report_type);

  return 0;

 error :
  LOG_E(ENB_AGENT, "errno %d occured\n", err_code);
  return err_code;
}

int enb_agent_mac_stats_request(mid_t mod_id,
				xid_t xid,
				const stats_request_config_t *report_config,
				Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  int i;

  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_STATS_REQUEST, &header) != 0)
    goto error;

  Protocol__PrpStatsRequest *stats_request_msg;
  stats_request_msg = malloc(sizeof(Protocol__PrpStatsRequest));
  if(stats_request_msg == NULL)
    goto error;

  protocol__prp_stats_request__init(stats_request_msg);
  stats_request_msg->header = header;

  stats_request_msg->type = report_config->report_type;
  stats_request_msg->has_type = 1;

  switch (report_config->report_type) {
  case PROTOCOL__PRP_STATS_TYPE__PRST_COMPLETE_STATS:
    stats_request_msg->body_case =  PROTOCOL__PRP_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST;
    Protocol__PrpCompleteStatsRequest *complete_stats;
    complete_stats = malloc(sizeof(Protocol__PrpCompleteStatsRequest));
    if(complete_stats == NULL)
      goto error;
    protocol__prp_complete_stats_request__init(complete_stats);
    complete_stats->report_frequency = report_config->report_frequency;
    complete_stats->has_report_frequency = 1;
    complete_stats->sf = report_config->period;
    complete_stats->has_sf = 1;
    complete_stats->has_cell_report_flags = 1;
    complete_stats->has_ue_report_flags = 1;
    if (report_config->config->nr_cc > 0) {
      complete_stats->cell_report_flags = report_config->config->cc_report_type[0].cc_report_flags;
    }
    if (report_config->config->nr_ue > 0) {
      complete_stats->ue_report_flags = report_config->config->ue_report_type[0].ue_report_flags;
    }
    stats_request_msg->complete_stats_request = complete_stats;
    break;
  case  PROTOCOL__PRP_STATS_TYPE__PRST_CELL_STATS:
    stats_request_msg->body_case = PROTOCOL__PRP_STATS_REQUEST__BODY_CELL_STATS_REQUEST;
     Protocol__PrpCellStatsRequest *cell_stats;
     cell_stats = malloc(sizeof(Protocol__PrpCellStatsRequest));
    if(cell_stats == NULL)
      goto error;
    protocol__prp_cell_stats_request__init(cell_stats);
    cell_stats->n_cell = report_config->config->nr_cc;
    cell_stats->has_flags = 1;
    if (cell_stats->n_cell > 0) {
      uint32_t *cells;
      cells = (uint32_t *) malloc(sizeof(uint32_t)*cell_stats->n_cell);
      for (i = 0; i < cell_stats->n_cell; i++) {
	cells[i] = report_config->config->cc_report_type[i].cc_id;
      }
      cell_stats->cell = cells;
      cell_stats->flags = report_config->config->cc_report_type[i].cc_report_flags;
    }
    stats_request_msg->cell_stats_request = cell_stats;
    break;
  case PROTOCOL__PRP_STATS_TYPE__PRST_UE_STATS:
    stats_request_msg->body_case = PROTOCOL__PRP_STATS_REQUEST__BODY_UE_STATS_REQUEST;
     Protocol__PrpUeStatsRequest *ue_stats;
     ue_stats = malloc(sizeof(Protocol__PrpUeStatsRequest));
    if(ue_stats == NULL)
      goto error;
    protocol__prp_ue_stats_request__init(ue_stats);
    ue_stats->n_rnti = report_config->config->nr_ue;
    ue_stats->has_flags = 1;
    if (ue_stats->n_rnti > 0) {
      uint32_t *ues;
      ues = (uint32_t *) malloc(sizeof(uint32_t)*ue_stats->n_rnti);
      for (i = 0; i < ue_stats->n_rnti; i++) {
	ues[i] = report_config->config->ue_report_type[i].ue_rnti;
      }
      ue_stats->rnti = ues;
      ue_stats->flags = report_config->config->ue_report_type[i].ue_report_flags;
    }
    stats_request_msg->ue_stats_request = ue_stats;
    break;
  default:
    goto error;
  }
  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->stats_request_msg = stats_request_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (stats_request_msg != NULL)
    free(stats_request_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_destroy_stats_request(Protocol__ProgranMessage *msg) {
   if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REQUEST_MSG)
    goto error;
  free(msg->stats_request_msg->header);
  if (msg->stats_request_msg->body_case == PROTOCOL__PRP_STATS_REQUEST__BODY_CELL_STATS_REQUEST) {
    free(msg->stats_request_msg->cell_stats_request->cell);
  }
  if (msg->stats_request_msg->body_case == PROTOCOL__PRP_STATS_REQUEST__BODY_UE_STATS_REQUEST) {
    free(msg->stats_request_msg->ue_stats_request->rnti);
  }
  free(msg->stats_request_msg);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_stats_reply(mid_t mod_id,
			      xid_t xid,
			      const report_config_t *report_config,
			      Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  int i, j, k;
  int cc_id = 0;
  int enb_id = mod_id;
  //eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  //UE_list_t *eNB_UE_list=  &eNB->UE_list;


  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_STATS_REPLY, &header) != 0)
    goto error;

  Protocol__PrpStatsReply *stats_reply_msg;
  stats_reply_msg = malloc(sizeof(Protocol__PrpStatsReply));
  if (stats_reply_msg == NULL)
    goto error;
  protocol__prp_stats_reply__init(stats_reply_msg);
  stats_reply_msg->header = header;

  stats_reply_msg->n_ue_report = report_config->nr_ue;
  stats_reply_msg->n_cell_report = report_config->nr_cc;

  Protocol__PrpUeStatsReport **ue_report;
  Protocol__PrpCellStatsReport **cell_report;


  /* Allocate memory for list of UE reports */
  if (report_config->nr_ue > 0) {
    ue_report = malloc(sizeof(Protocol__PrpUeStatsReport *) * report_config->nr_ue);
    if (ue_report == NULL)
      goto error;
    for (i = 0; i < report_config->nr_ue; i++) {
      ue_report[i] = malloc(sizeof(Protocol__PrpUeStatsReport));
      protocol__prp_ue_stats_report__init(ue_report[i]);
      ue_report[i]->rnti = report_config->ue_report_type[i].ue_rnti;
      ue_report[i]->has_rnti = 1;
      ue_report[i]->flags = report_config->ue_report_type[i].ue_report_flags;
      ue_report[i]->has_flags = 1;
      /* Check the types of reports that need to be constructed based on flag values */

      /* Check flag for creation of buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_BSR) {
	//TODO: Create a report for each LCG (4 elements). See prp_ue_stats_report of
	// progRAN specifications for more details
	ue_report[i]->n_bsr = 4;
	uint32_t *elem;
	elem = (uint32_t *) malloc(sizeof(uint32_t)*ue_report[i]->n_bsr);
	if (elem == NULL)
	  goto error;
	for (j = 0; j++; j < ue_report[i]->n_bsr) {
	  // Set the actual BSR for LCG j of the current UE
	  // NN: we need to know the cc_id here, consider the first one
	  elem[j] = get_ue_bsr (enb_id, i, j); //eNB_UE_list->UE_template[UE_PCCID(enb_id,i)][i].bsr_info[j];
	}
	ue_report[i]->bsr = elem;
      }

      /* Check flag for creation of PRH report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PRH) {
	// TODO: Fill in the actual power headroom value for the RNTI
	ue_report[i]->phr = get_ue_phr (enb_id, i); // eNB_UE_list->UE_template[UE_PCCID(enb_id,i)][i].phr_info;
	ue_report[i]->has_phr = 1;
      }

      /* Check flag for creation of RLC buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_RLC_BS) {
	// TODO: Fill in the actual RLC buffer status reports
	ue_report[i]->n_rlc_report = 3; // Set this to the number of LCs for this UE
	Protocol__PrpRlcBsr ** rlc_reports;
	rlc_reports = malloc(sizeof(Protocol__PrpRlcBsr *) * ue_report[i]->n_rlc_report);
	if (rlc_reports == NULL)
	  goto error;

	// Fill the buffer status report for each logical channel of the UE
	// NN: see LAYER2/openair2_proc.c for rlc status
	for (j = 0; j < ue_report[i]->n_rlc_report; j++) {
	  rlc_reports[j] = malloc(sizeof(Protocol__PrpRlcBsr));
	  if (rlc_reports[j] == NULL)
	    goto error;
	  protocol__prp_rlc_bsr__init(rlc_reports[j]);
	  //TODO:Set logical channel id
	  rlc_reports[j]->lc_id = j+1;
	  rlc_reports[j]->has_lc_id = 1;
	  //TODO:Set tx queue size in bytes
	  rlc_reports[j]->tx_queue_size = get_tx_queue_size(enb_id,i,j+1);
	  rlc_reports[j]->has_tx_queue_size = 1;
	  //TODO:Set tx queue head of line delay in ms
	  rlc_reports[j]->tx_queue_hol_delay = 100;
	  rlc_reports[j]->has_tx_queue_hol_delay = 1;
	  //TODO:Set retransmission queue size in bytes
	  rlc_reports[j]->retransmission_queue_size = 10;
	  rlc_reports[j]->has_retransmission_queue_size = 1;
	  //TODO:Set retransmission queue head of line delay in ms
	  rlc_reports[j]->retransmission_queue_hol_delay = 100;
	  rlc_reports[j]->has_retransmission_queue_hol_delay = 1;
	  //TODO:Set current size of the pending message in bytes
	  rlc_reports[j]->status_pdu_size = 100;
	  rlc_reports[j]->has_status_pdu_size = 1;
	}
	// Add RLC buffer status reports to the full report
	if (ue_report[i]->n_rlc_report > 0)
	  ue_report[i]->rlc_report = rlc_reports;
      }

      /* Check flag for creation of MAC CE buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_MAC_CE_BS) {
	// TODO: Fill in the actual MAC CE buffer status report
	ue_report[i]->pending_mac_ces = (get_MAC_CE_bitmap_TA(enb_id,i) | (0 << 1) | (0 << 2) | (0 << 3)) & 15; /* Use as bitmap. Set one or more of the; /* Use as bitmap. Set one or more of the
					       PROTOCOL__PRP_CE_TYPE__PRPCET_ values
					       found in stats_common.pb-c.h. See
					       prp_ce_type in progRAN specification */
	ue_report[i]->has_pending_mac_ces = 1;
      }

      /* Check flag for creation of DL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI) {
	// TODO: Fill in the actual DL CQI report for the UE based on its configuration
	Protocol__PrpDlCqiReport * dl_report;
	dl_report = malloc(sizeof(Protocol__PrpDlCqiReport));
	if (dl_report == NULL)
	  goto error;
	protocol__prp_dl_cqi_report__init(dl_report);
	//TODO:Set the SFN and SF of the last report held in the agent.
	dl_report->sfn_sn = get_sfn_sf(enb_id);
	dl_report->has_sfn_sn = 1;
	//TODO:Set the number of DL CQI reports for this UE. One for each CC
	dl_report->n_csi_report = get_active_CC(enb_id,i);
	//TODO:Create the actual CSI reports.
	Protocol__PrpDlCsi **csi_reports;
	csi_reports = malloc(sizeof(Protocol__PrpDlCsi *)*dl_report->n_csi_report);
	if (csi_reports == NULL)
	  goto error;
	for (j = 0; j < dl_report->n_csi_report; j++) {
	  csi_reports[j] = malloc(sizeof(Protocol__PrpDlCsi));
	  if (csi_reports[j] == NULL)
	    goto error;
	  protocol__prp_dl_csi__init(csi_reports[j]);
	  //TODO: the servCellIndex for this report
	  csi_reports[j]->serv_cell_index = j;
	  csi_reports[j]->has_serv_cell_index = 1;
	  //TODO: the rank indicator value for this cc
	  csi_reports[j]->ri = get_current_RI(enb_id,i,j);
	  csi_reports[j]->has_ri = 1;
	  //TODO: the type of CSI report based on the configuration of the UE
	  //For this example we use type P10, which only needs a wideband value
	  //The full set of types can be found in stats_common.pb-c.h and
	  //in the progRAN specifications
    csi_reports[j]->type =  PROTOCOL__PRP_CSI_TYPE__PRCSIT_P10;
		  csi_reports[j]->has_type = 1;
		  csi_reports[j]->report_case = PROTOCOL__PRP_DL_CSI__REPORT_P10CSI;
		  if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_P10CSI){
			  Protocol__PrpCsiP10 *csi10;
			  csi10 = malloc(sizeof(Protocol__PrpCsiP10));
			  if (csi10 == NULL)
				goto error;
			  protocol__prp_csi_p10__init(csi10);
			  //TODO: set the wideband value
			  // NN: this is also depends on cc_id
			  csi10->wb_cqi = get_ue_wcqi (enb_id, i); //eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,i)][i].dl_cqi;
			  csi10->has_wb_cqi = 1;
			  //Add the type of measurements to the csi report in the proper union type
			  csi_reports[j]->p10csi = csi10;
		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_P11CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_P20CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_P21CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_A12CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_A22CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_A20CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_A30CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__PRP_DL_CSI__REPORT_A31CSI){

		  }
		}
	//Add the csi reports to the full DL CQI report
	dl_report->csi_report = csi_reports;
	//Add the DL CQI report to the stats report
	ue_report[i]->dl_cqi_report = dl_report;
      }

      /* Check flag for creation of paging buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PBS) {
	//TODO: Fill in the actual paging buffer status report. For this field to be valid, the RNTI
	//set in the report must be a P-RNTI
	Protocol__PrpPagingBufferReport *paging_report;
	paging_report = malloc(sizeof(Protocol__PrpPagingBufferReport));
	if (paging_report == NULL)
	  goto error;
	protocol__prp_paging_buffer_report__init(paging_report);
	//Set the number of pending paging messages
	paging_report->n_paging_info = 1;
	//Provide a report for each pending paging message
	Protocol__PrpPagingInfo **p_info;
	p_info = malloc(sizeof(Protocol__PrpPagingInfo *) * paging_report->n_paging_info);
	if (p_info == NULL)
	  goto error;
	for (j = 0; j < paging_report->n_paging_info; j++) {
	  p_info[j] = malloc(sizeof(Protocol__PrpPagingInfo));
	  if(p_info[j] == NULL)
	    goto error;
	  protocol__prp_paging_info__init(p_info[j]);
	  //TODO: Set paging index. This index is the same that will be used for the scheduling of the
	  //paging message by the controller
	  p_info[j]->paging_index = 10;
	  p_info[j]->has_paging_index = 1;
	  //TODO:Set the paging message size
	  p_info[j]->paging_message_size = 100;
	  p_info[j]->has_paging_message_size = 1;
	  //TODO: Set the paging subframe
	  p_info[j]->paging_subframe = 10;
	  p_info[j]->has_paging_subframe = 1;
	  //TODO: Set the carrier index for the pending paging message
	  p_info[j]->carrier_index = 0;
	  p_info[j]->has_carrier_index = 1;
	}
	//Add all paging info to the paging buffer rerport
	paging_report->paging_info = p_info;
	//Add the paging report to the UE report
	ue_report[i]->pbr = paging_report;
      }

      /* Check flag for creation of UL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_UL_CQI) {
	//Fill in the full UL CQI report of the UE
	Protocol__PrpUlCqiReport *full_ul_report;
	full_ul_report = malloc(sizeof(Protocol__PrpUlCqiReport));
	if(full_ul_report == NULL)
	  goto error;
	protocol__prp_ul_cqi_report__init(full_ul_report);
	//TODO:Set the SFN and SF of the generated report
	full_ul_report->sfn_sn = get_sfn_sf(enb_id);
	full_ul_report->has_sfn_sn = 1;
	//TODO:Set the number of UL measurement reports based on the types of measurements
	//configured for this UE and on the servCellIndex
	full_ul_report->n_cqi_meas = 1;
	Protocol__PrpUlCqi **ul_report;
	ul_report = malloc(sizeof(Protocol__PrpUlCqi *) * full_ul_report->n_cqi_meas);
	if(ul_report == NULL)
	  goto error;
	//Fill each UL report of the UE for each of the configured report types
	for(j = 0; j++; j < full_ul_report->n_cqi_meas) {
	  ul_report[j] = malloc(sizeof(Protocol__PrpUlCqi));
	  if(ul_report[j] == NULL)
	  goto error;
	  protocol__prp_ul_cqi__init(ul_report[j]);
	  //TODO: Set the type of the UL report. As an example set it to SRS UL report
	  // See enum prp_ul_cqi_type in progRAN specification for more details
	  ul_report[j]->type = PROTOCOL__PRP_UL_CQI_TYPE__PRUCT_SRS;
	  ul_report[j]->has_type = 1;
	  //TODO:Set the number of SINR measurements based on the report type
	  //See struct prp_ul_cqi in progRAN specification for more details
	  ul_report[j]->n_sinr = 100;
	  uint32_t *sinr_meas;
	  sinr_meas = (uint32_t *) malloc(sizeof(uint32_t) * ul_report[j]->n_sinr);
	  if (sinr_meas == NULL)
	    goto error;
	  //TODO:Set the SINR measurements for the specified type
	  for (k = 0; k < ul_report[j]->n_sinr; k++) {
	    sinr_meas[k] = 10;
	  }
	  ul_report[j]->sinr = sinr_meas;
	  //TODO: Set the servCellIndex for this report
	  ul_report[j]->serv_cell_index = 0;
	  ul_report[j]->has_serv_cell_index = 1;
	  //Set the list of UL reports of this UE to the full UL report
	  full_ul_report->cqi_meas = ul_report;
	  //Add full UL CQI report to the UE report
	  ue_report[i]->ul_cqi_report = full_ul_report;
	}
      }
    }
    /* Add list of all UE reports to the message */
    stats_reply_msg->ue_report = ue_report;
  }

  /* Allocate memory for list of cell reports */
  if (report_config->nr_cc > 0) {
    cell_report = malloc(sizeof(Protocol__PrpCellStatsReport *) * report_config->nr_cc);
    if (cell_report == NULL)
      goto error;
    // Fill in the Cell reports
    for (i = 0; i < report_config->nr_cc; i++) {
      cell_report[i] = malloc(sizeof(Protocol__PrpCellStatsReport));
      if(cell_report[i] == NULL)
	goto error;
      protocol__prp_cell_stats_report__init(cell_report[i]);
      cell_report[i]->carrier_index = report_config->cc_report_type[i].cc_id;
      cell_report[i]->has_carrier_index = 1;
      cell_report[i]->flags = report_config->cc_report_type[i].cc_report_flags;
      cell_report[i]->has_flags = 1;

      /* Check flag for creation of noise and interference report */
      if(report_config->cc_report_type[i].cc_report_flags & PROTOCOL__PRP_CELL_STATS_TYPE__PRCST_NOISE_INTERFERENCE) {
	// TODO: Fill in the actual noise and interference report for this cell
	Protocol__PrpNoiseInterferenceReport *ni_report;
	ni_report = malloc(sizeof(Protocol__PrpNoiseInterferenceReport));
	if(ni_report == NULL)
	  goto error;
	protocol__prp_noise_interference_report__init(ni_report);
	// Current frame and subframe number
	ni_report->sfn_sf = get_sfn_sf(enb_id);
	ni_report->has_sfn_sf = 1;
	// Received interference power in dbm
	ni_report->rip = 0;
	ni_report->has_rip = 1;
	// Thermal noise power in dbm
	ni_report->tnp = 0;
	ni_report->has_tnp = 1;
	cell_report[i]->noise_inter_report = ni_report;
      }
    }
    /* Add list of all cell reports to the message */
    stats_reply_msg->cell_report = cell_report;
  }

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  (*msg)->msg_dir =  PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->stats_reply_msg = stats_reply_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (stats_reply_msg != NULL)
    free(stats_reply_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_destroy_stats_reply(Protocol__ProgranMessage *msg) {
  //TODO: Need to deallocate memory for the stats reply message
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG)
    goto error;
  free(msg->stats_reply_msg->header);
  int i, j, k;

  Protocol__PrpStatsReply *reply = msg->stats_reply_msg;
  Protocol__PrpDlCqiReport *dl_report;
  Protocol__PrpUlCqiReport *ul_report;
  Protocol__PrpPagingBufferReport *paging_report;

  // Free the memory for the UE reports
  for (i = 0; i < reply->n_ue_report; i++) {
    free(reply->ue_report[i]->bsr);
    for (j = 0; j < reply->ue_report[i]->n_rlc_report; j++) {
      free(reply->ue_report[i]->rlc_report[j]);
    }
    free(reply->ue_report[i]->rlc_report);
    // If DL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI) {
      dl_report = reply->ue_report[i]->dl_cqi_report;
      // Delete all CSI reports
      for (j = 0; j < dl_report->n_csi_report; j++) {
	//Must free memory based on the type of report
	switch(dl_report->csi_report[j]->report_case) {
	case PROTOCOL__PRP_DL_CSI__REPORT_P10CSI:
	  free(dl_report->csi_report[j]->p10csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P11CSI:
	  free(dl_report->csi_report[j]->p11csi->wb_cqi);
	  free(dl_report->csi_report[j]->p11csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P20CSI:
	  free(dl_report->csi_report[j]->p20csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P21CSI:
	  free(dl_report->csi_report[j]->p21csi->wb_cqi);
	  free(dl_report->csi_report[j]->p21csi->sb_cqi);
	  free(dl_report->csi_report[j]->p21csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A12CSI:
	  free(dl_report->csi_report[j]->a12csi->wb_cqi);
	  free(dl_report->csi_report[j]->a12csi->sb_pmi);
	  free(dl_report->csi_report[j]->a12csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A22CSI:
	  free(dl_report->csi_report[j]->a22csi->wb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_list);
	  free(dl_report->csi_report[j]->a22csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A20CSI:
	  free(dl_report->csi_report[j]->a20csi->sb_list);
	  free(dl_report->csi_report[j]->a20csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A30CSI:
	  free(dl_report->csi_report[j]->a30csi->sb_cqi);
	  free(dl_report->csi_report[j]->a30csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A31CSI:
	  free(dl_report->csi_report[j]->a31csi->wb_cqi);
	  for (k = 0; k < dl_report->csi_report[j]->a31csi->n_sb_cqi; k++) {
	    free(dl_report->csi_report[j]->a31csi->sb_cqi[k]);
	  }
	  free(dl_report->csi_report[j]->a31csi->sb_cqi);
	  break;
	}

	free(dl_report->csi_report[j]);
      }
      free(dl_report->csi_report);
      free(dl_report);
    }
    // If Paging buffer report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PBS) {
      paging_report = reply->ue_report[i]->pbr;
      // Delete all paging buffer reports
      for (j = 0; j < paging_report->n_paging_info; j++) {
	free(paging_report->paging_info[j]);
      }
      free(paging_report->paging_info);
      free(paging_report);
    }
    // If UL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_UL_CQI) {
      ul_report = reply->ue_report[i]->ul_cqi_report;
      for (j = 0; j < ul_report->n_cqi_meas; j++) {
	free(ul_report->cqi_meas[j]->sinr);
	free(ul_report->cqi_meas[j]);
      }
      free(ul_report->cqi_meas);
    }
    free(reply->ue_report[i]);
  }
  free(reply->ue_report);

  // Free memory for all Cell reports
  for (i = 0; i < reply->n_cell_report; i++) {
    free(reply->cell_report[i]->noise_inter_report);
    free(reply->cell_report[i]);
  }
  free(reply->cell_report);

  free(reply);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  int i;
  const int xid = *((int *)params);
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_UL_SR_INFO, &header) != 0)
    goto error;

  Protocol__PrpUlSrInfo *ul_sr_info_msg;
  ul_sr_info_msg = malloc(sizeof(Protocol__PrpUlSrInfo));
  if (ul_sr_info_msg == NULL) {
    goto error;
  }
  protocol__prp_ul_sr_info__init(ul_sr_info_msg);

  ul_sr_info_msg->header = header;
  ul_sr_info_msg->has_sfn_sf = 1;
  ul_sr_info_msg->sfn_sf = get_sfn_sf(mod_id);
  /*TODO: Set the number of UEs that sent an SR */
  ul_sr_info_msg->n_rnti = 1;
  ul_sr_info_msg->rnti = (uint32_t *) malloc(ul_sr_info_msg->n_rnti * sizeof(uint32_t));

  if(ul_sr_info_msg->rnti == NULL) {
    goto error;
  }
  /*TODO:Set the rnti of the UEs that sent an SR */
  for (i = 0; i < ul_sr_info_msg->n_rnti; i++) {
    ul_sr_info_msg->rnti[i] = 1;
  }

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_UL_SR_INFO_MSG;
  (*msg)->msg_dir =  PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->ul_sr_info_msg = ul_sr_info_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (ul_sr_info_msg != NULL) {
    free(ul_sr_info_msg->rnti);
    free(ul_sr_info_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_destroy_sr_info(Protocol__ProgranMessage *msg) {
   if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_UL_SR_INFO_MSG)
     goto error;

   free(msg->ul_sr_info_msg->header);
   free(msg->ul_sr_info_msg->rnti);
   free(msg->ul_sr_info_msg);
   free(msg);
   return 0;

 error:
   //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
   return -1;
}

int enb_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  int i,j;
  const int xid = *((int *)params);
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_SF_TRIGGER, &header) != 0)
    goto error;

  Protocol__PrpSfTrigger *sf_trigger_msg;
  sf_trigger_msg = malloc(sizeof(Protocol__PrpSfTrigger));
  if (sf_trigger_msg == NULL) {
    goto error;
  }
  protocol__prp_sf_trigger__init(sf_trigger_msg);

  sf_trigger_msg->header = header;
  sf_trigger_msg->has_sfn_sf = 1;
  sf_trigger_msg->sfn_sf = get_sfn_sf(mod_id);

  /*TODO: Fill in the number of dl HARQ related info, based on the number of currently
   *transmitting UEs
   */
  sf_trigger_msg->n_dl_info = get_num_ues(mod_id);

  Protocol__PrpDlInfo **dl_info = NULL;

  if (sf_trigger_msg->n_dl_info > 0) {
    dl_info = malloc(sizeof(Protocol__PrpDlInfo *) * sf_trigger_msg->n_dl_info);
    if(dl_info == NULL)
      goto error;
    //Fill the status of the current HARQ process for each UE
    for(i = 0; i < sf_trigger_msg->n_dl_info; i++) {
      dl_info[i] = malloc(sizeof(Protocol__PrpDlInfo));
      if(dl_info[i] == NULL)
	goto error;
      protocol__prp_dl_info__init(dl_info[i]);
      dl_info[i]->rnti = get_ue_crnti(mod_id, i);
      dl_info[i]->has_rnti = 1;
      /*TODO: fill in the right id of this round's HARQ process for this UE*/
      int harq_id;
      int harq_status;
      get_harq(mod_id,UE_PCCID(mod_id,i),i,get_current_frame(mod_id),get_current_subframe(mod_id),&harq_id, &harq_status);
      dl_info[i]->harq_process_id = harq_id;
      dl_info[i]->has_harq_process_id = 1;
      /*TODO: fill in the status of the HARQ process (2 TBs)*/
      dl_info[i]->n_harq_status = 2;
      dl_info[i]->harq_status = malloc(sizeof(uint32_t) * dl_info[i]->n_harq_status);
      for (j = 0; j < dl_info[i]->n_harq_status; j++) {
	// TODO: This should be different per TB
	dl_info[i]->harq_status[j] = harq_status;
      }
      /*TODO: fill in the serving cell index for this UE */
      dl_info[i]->serv_cell_index = UE_PCCID(mod_id,i);
      dl_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->dl_info = dl_info;

    /*TODO: Fill in the number of UL reception status related info, based on the number of currently
   *transmitting UEs
   */
  sf_trigger_msg->n_ul_info = get_num_ues(mod_id);

  Protocol__PrpUlInfo **ul_info = NULL;

  if (sf_trigger_msg->n_ul_info > 0) {
    ul_info = malloc(sizeof(Protocol__PrpUlInfo *) * sf_trigger_msg->n_ul_info);
    if(ul_info == NULL)
      goto error;
    //Fill the reception info for each transmitting UE
    for(i = 0; i < sf_trigger_msg->n_ul_info; i++) {
      ul_info[i] = malloc(sizeof(Protocol__PrpUlInfo));
      if(ul_info[i] == NULL)
	goto error;
      protocol__prp_ul_info__init(ul_info[i]);
      ul_info[i]->rnti = get_ue_crnti(mod_id, i);
      ul_info[i]->has_rnti = 1;
      /*TODO: fill in the Tx power control command for this UE (if available)*/
      if(get_tpc(mod_id,i) != 1){
    	  ul_info[i]->tpc = get_tpc(mod_id,i);
          ul_info[i]->has_tpc = 1;
      }
      else{
    	  ul_info[i]->tpc = get_tpc(mod_id,i);
    	  ul_info[i]->has_tpc = 0;
      }
      /*TODO: fill in the amount of data in bytes in the MAC SDU received in this subframe for the
	given logical channel*/
      ul_info[i]->n_ul_reception = 11;
      ul_info[i]->ul_reception = malloc(sizeof(uint32_t) * ul_info[i]->n_ul_reception);
      for (j = 0; j < ul_info[i]->n_ul_reception; j++) {
	ul_info[i]->ul_reception[j] = 100;
      }
      /*TODO: Fill in the reception status for each UEs data*/
      ul_info[i]->reception_status = PROTOCOL__PRP_RECEPTION_STATUS__PRRS_OK;
      ul_info[i]->has_reception_status = 1;
      /*TODO: fill in the serving cell index for this UE */
      ul_info[i]->serv_cell_index = UE_PCCID(mod_id,i);
      ul_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->ul_info = ul_info;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_SF_TRIGGER_MSG;
  (*msg)->msg_dir =  PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->sf_trigger_msg = sf_trigger_msg;
  return 0;

 error:
  if (header != NULL)
    free(header);
  if (sf_trigger_msg != NULL) {
    for (i = 0; i < sf_trigger_msg->n_dl_info; i++) {
      free(sf_trigger_msg->dl_info[i]->harq_status);
    }
    free(sf_trigger_msg->dl_info);
    for (i = 0; i < sf_trigger_msg->n_ul_info; i++) {
      free(sf_trigger_msg->ul_info[i]->reception_status);
    }
    free(sf_trigger_msg->ul_info);
    free(sf_trigger_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_destroy_sf_trigger(Protocol__ProgranMessage *msg) {
  int i;
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_SF_TRIGGER_MSG)
    goto error;

  free(msg->sf_trigger_msg->header);
  for (i = 0; i < msg->sf_trigger_msg->n_dl_info; i++) {
    free(msg->sf_trigger_msg->dl_info[i]->harq_status);
    free(msg->sf_trigger_msg->dl_info[i]);
  }
  free(msg->sf_trigger_msg->dl_info);
  for (i = 0; i < msg->sf_trigger_msg->n_ul_info; i++) {
    free(msg->sf_trigger_msg->ul_info[i]->ul_reception);
    free(msg->sf_trigger_msg->ul_info[i]);
  }
  free(msg->sf_trigger_msg->ul_info);
  free(msg->sf_trigger_msg);
  free(msg);

  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__ProgranMessage **msg) {

  int xid = 0;
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_DL_MAC_CONFIG, &header) != 0)
    goto error;

  Protocol__PrpDlMacConfig *dl_mac_config_msg;
  dl_mac_config_msg = malloc(sizeof(Protocol__PrpDlMacConfig));
  if (dl_mac_config_msg == NULL) {
    goto error;
  }
  protocol__prp_dl_mac_config__init(dl_mac_config_msg);

  dl_mac_config_msg->header = header;
  dl_mac_config_msg->has_sfn_sf = 1;
  dl_mac_config_msg->sfn_sf = get_sfn_sf(mod_id);

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG;
  (*msg)->msg_dir =  PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->dl_mac_config_msg = dl_mac_config_msg;

  return 0;

 error:
  return -1;
}

int enb_agent_mac_destroy_dl_config(Protocol__ProgranMessage *msg) {
  int i,j, k;
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG)
    goto error;

  Protocol__PrpDlDci *dl_dci;

  free(msg->dl_mac_config_msg->header);
  for (i = 0; i < msg->dl_mac_config_msg->n_dl_ue_data; i++) {
    free(msg->dl_mac_config_msg->dl_ue_data[i]->ce_bitmap);
    for (j = 0; j < msg->dl_mac_config_msg->dl_ue_data[i]->n_rlc_pdu; j++) {
      for (k = 0; k <  msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->n_rlc_pdu_tb; k++) {
	free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb[k]);
      }
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb);
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]);
    }
    free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu);
    dl_dci = msg->dl_mac_config_msg->dl_ue_data[i]->dl_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_ue_data[i]);
  }
  free(msg->dl_mac_config_msg->dl_ue_data);

  for (i = 0; i <  msg->dl_mac_config_msg->n_dl_rar; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_rar[i]->rar_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_rar[i]);
  }
  free(msg->dl_mac_config_msg->dl_rar);

  for (i = 0; i < msg->dl_mac_config_msg->n_dl_broadcast; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_broadcast[i]->broad_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_broadcast[i]);
  }
  free(msg->dl_mac_config_msg->dl_broadcast);

    for ( i = 0; i < msg->dl_mac_config_msg->n_ofdm_sym; i++) {
    free(msg->dl_mac_config_msg->ofdm_sym[i]);
  }
  free(msg->dl_mac_config_msg->ofdm_sym);
  free(msg->dl_mac_config_msg);
  free(msg);

  return 0;

 error:
  return -1;
}

/***********************************************
 * eNB agent - technology mac API implementation
 ***********************************************/

void enb_agent_send_sr_info(mid_t mod_id) {
  int size;
  Protocol__ProgranMessage *msg;
  void *data;
  int priority;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = enb_agent_mac_sr_info(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=enb_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (enb_agent_msg_send(mod_id, ENB_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(ENB_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(ENB_AGENT, "Could not send sr message\n");
}

void enb_agent_send_sf_trigger(mid_t mod_id) {
  int size;
  Protocol__ProgranMessage *msg;
  void *data;
  int priority;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = enb_agent_mac_sf_trigger(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=enb_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (enb_agent_msg_send(mod_id, ENB_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(ENB_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(ENB_AGENT, "Could not send sf trigger message\n");
}

void enb_agent_send_update_mac_stats(mid_t mod_id) {

  Protocol__ProgranMessage *current_report = NULL, *msg;
  void *data;
  int size;
  err_code_t err_code;
  int priority;

  mac_stats_updates_context_t stats_context = mac_stats_context[mod_id];
  
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  if (mac_stats_context[mod_id].cont_update == 1) {
  
    /*Create a fresh report with the required flags*/
    err_code = enb_agent_mac_handle_stats(mod_id, (void *) mac_stats_context[mod_id].stats_req, &current_report);
    if (err_code < 0) {
      goto error;
    }
  }
  /* /\*TODO:Check if a previous reports exists and if yes, generate a report */
  /*  *that is the diff between the old and the new report, */
  /*  *respecting the thresholds. Otherwise send the new report*\/ */
  /* if (mac_stats_context[mod_id].prev_stats_reply != NULL) { */

  /*   msg = enb_agent_generate_diff_mac_stats_report(current_report, mac_stats_context[mod_id].prev_stats_reply); */

  /*   /\*Destroy the old stats*\/ */
  /*    enb_agent_destroy_progran_message(mac_stats_context[mod_id].prev_stats_reply); */
  /* } */
  /* /\*Use the current report for future comparissons*\/ */
  /* mac_stats_context[mod_id].prev_stats_reply = current_report; */


  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  if (current_report != NULL){
    data=enb_agent_pack_message(current_report, &size);
    /*Send any stats updates using the MAC channel of the eNB*/
    if (enb_agent_msg_send(mod_id, ENB_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(ENB_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(ENB_AGENT, "Could not send sf trigger message\n");
}

int enb_agent_register_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface) {
  if (mac_agent_registered[mod_id]) {
    LOG_E(MAC, "MAC agent for eNB %d is already registered\n", mod_id);
    return -1;
  }

  //xface->agent_ctxt = &shared_ctxt[mod_id];
  xface->enb_agent_send_sr_info = enb_agent_send_sr_info;
  xface->enb_agent_send_sf_trigger = enb_agent_send_sf_trigger;
  xface->enb_agent_send_update_mac_stats = enb_agent_send_update_mac_stats;
  xface->enb_agent_schedule_ue_spec = schedule_ue_spec_default;
  xface->enb_agent_notify_ue_state_change = enb_agent_ue_state_change;

  mac_agent_registered[mod_id] = 1;
  agent_mac_xface[mod_id] = xface;

  return 0;
}

int enb_agent_unregister_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface) {

  //xface->agent_ctxt = NULL;
  xface->enb_agent_send_sr_info = NULL;
  xface->enb_agent_send_sf_trigger = NULL;
  xface->enb_agent_send_update_mac_stats = NULL;
  xface->enb_agent_schedule_ue_spec = NULL;

  mac_agent_registered[mod_id] = 0;
  agent_mac_xface[mod_id] = NULL;

  return 0;
}


/******************************************************
 *Implementations of enb_agent_mac_internal.h functions
 ******************************************************/

err_code_t enb_agent_init_cont_mac_stats_update(mid_t mod_id) {

  /*Initialize the Mac stats update structure*/
  /*Initially the continuous update is set to false*/
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].is_initialized = 1;
  mac_stats_context[mod_id].stats_req = NULL;
  mac_stats_context[mod_id].prev_stats_reply = NULL;
  mac_stats_context[mod_id].mutex = calloc(1, sizeof(pthread_mutex_t));
  if (mac_stats_context[mod_id].mutex == NULL)
    goto error;
  if (pthread_mutex_init(mac_stats_context[mod_id].mutex, NULL))
    goto error;;

  return 0;

 error:
  return -1;
}

err_code_t enb_agent_destroy_cont_mac_stats_update(mid_t mod_id) {
  /*Disable the continuous updates for the MAC*/
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].is_initialized = 0;
  enb_agent_destroy_progran_message(mac_stats_context[mod_id].stats_req);
  enb_agent_destroy_progran_message(mac_stats_context[mod_id].prev_stats_reply);
  free(mac_stats_context[mod_id].mutex);

  mac_agent_registered[mod_id] = NULL;
  return 1;
}


err_code_t enb_agent_enable_cont_mac_stats_update(mid_t mod_id,
						  xid_t xid, stats_request_config_t *stats_req) {
  /*Enable the continuous updates for the MAC*/
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  Protocol__ProgranMessage *req_msg;

  enb_agent_mac_stats_request(mod_id, xid, stats_req, &req_msg);
  mac_stats_context[mod_id].stats_req = req_msg;
  mac_stats_context[mod_id].prev_stats_reply = NULL;

  mac_stats_context[mod_id].cont_update = 1;
  mac_stats_context[mod_id].xid = xid;

  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(ENB_AGENT, "mac_stats_context for eNB %d is not initialized\n", mod_id);
  return -1;
}

err_code_t enb_agent_disable_cont_mac_stats_update(mid_t mod_id) {
  /*Disable the continuous updates for the MAC*/
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].xid = 0;
  if (mac_stats_context[mod_id].stats_req != NULL) {
    enb_agent_destroy_progran_message(mac_stats_context[mod_id].stats_req);
  }
  if (mac_stats_context[mod_id].prev_stats_reply != NULL) {
    enb_agent_destroy_progran_message(mac_stats_context[mod_id].prev_stats_reply);
  }
  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(ENB_AGENT, "mac_stats_context for eNB %d is not initialized\n", mod_id);
  return -1;

}
