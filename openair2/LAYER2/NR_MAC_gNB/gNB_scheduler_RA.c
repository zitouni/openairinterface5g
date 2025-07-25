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

/*! \file     gNB_scheduler_RA.c
 * \brief     primitives used for random access
 * \author    Guido Casati
 * \date      2019
 * \email:    guido.casati@iis.fraunhofer.de
 * \version
 */

#include "common/platform_types.h"
#include "uper_decoder.h"

/* MAC */
#include "nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

/* Utils */
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"
#include "UTIL/OPT/opt.h"

/* rlc */
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/LAYER2/RLC/rlc.h"

#include <executables/softmodem-common.h>
extern RAN_CONTEXT_t RC;
extern const uint8_t nr_slots_per_frame[5];
extern uint16_t sl_ahead;

// forward declaration of functions used in this file
static void fill_msg3_pusch_pdu(nfapi_nr_pusch_pdu_t *pusch_pdu,
                                NR_ServingCellConfigCommon_t *scc,
                                int round,
                                int startSymbolAndLength,
                                rnti_t rnti,
                                int scs,
                                int bwp_size,
                                int bwp_start,
                                int mappingtype,
                                int fh,
                                int msg3_first_rb,
                                int msg3_nb_rb);
static void nr_fill_rar(uint8_t Mod_idP, NR_RA_t *ra, uint8_t *dlsch_buffer, nfapi_nr_pusch_pdu_t *pusch_pdu);

static const uint8_t DELTA[4] = {2, 3, 4, 6};

static const float ssb_per_rach_occasion[8] = {0.125, 0.25, 0.5, 1, 2, 4, 8};

static int16_t ssb_index_from_prach(module_id_t module_idP,
                                    frame_t frameP,
                                    sub_frame_t slotP,
                                    uint16_t preamble_index,
                                    uint8_t freq_index,
                                    uint8_t symbol)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &gNB->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  uint8_t config_index = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
  
  uint8_t total_RApreambles = MAX_NUM_NR_PRACH_PREAMBLES;
  if (rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
    total_RApreambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;
  
  float  num_ssb_per_RO = ssb_per_rach_occasion[cfg->prach_config.ssb_per_rach.value];	
  uint16_t start_symbol_index = 0;
  uint8_t mu,N_dur=0,N_t_slot=0,start_symbol = 0, temp_start_symbol = 0, N_RA_slot=0;
  uint16_t format,RA_sfn_index = -1;
  uint8_t config_period = 1;
  uint16_t prach_occasion_id = -1;
  uint8_t num_active_ssb = cc->num_active_ssb;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    mu = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  get_nr_prach_info_from_index(config_index,
			       (int)frameP,
			       (int)slotP,
			       scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
			       mu,
			       cc->frame_type,
			       &format,
			       &start_symbol,
			       &N_t_slot,
			       &N_dur,
			       &RA_sfn_index,
			       &N_RA_slot,
			       &config_period);
  uint8_t index = 0, slot_index = 0;
  for (slot_index = 0; slot_index < N_RA_slot; slot_index++) {
    if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
       if((mu == 1) || (mu == 3))
         slot_index = 1;  //For scs = 30khz and 120khz
    }
    for (int i = 0; i < N_t_slot; i++) {
      temp_start_symbol = (start_symbol + i * N_dur + 14 * slot_index) % 14;
      if(symbol == temp_start_symbol) {
        start_symbol_index = i;
        break;
      }
    }
  }
  if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
    if((mu == 1) || (mu == 3))
      slot_index = 0;  //For scs = 30khz and 120khz
  }

  //  prach_occasion_id = subframe_index * N_t_slot * N_RA_slot * fdm + N_RA_slot_index * N_t_slot * fdm + freq_index + fdm * start_symbol_index;
  prach_occasion_id = (((frameP % (cc->max_association_period * config_period)) / config_period) * cc->total_prach_occasions_per_config_period) +
                      (RA_sfn_index + slot_index) * N_t_slot * fdm + start_symbol_index * fdm + freq_index;

  //one SSB have more than one continuous RO
  if(num_ssb_per_RO <= 1)
    index = (int) (prach_occasion_id / (int)(1 / num_ssb_per_RO)) % num_active_ssb;
  //one RO is shared by one or more SSB
  else if (num_ssb_per_RO > 1) {
    index = (prach_occasion_id * (int)num_ssb_per_RO) % num_active_ssb;
    for(int j = 0; j < num_ssb_per_RO; j++) {
      if(preamble_index <  ((j + 1) * cc->cb_preambles_per_ssb))
        index = index + j;
    }
  }

  LOG_D(NR_MAC, "Frame %d, Slot %d: Prach Occasion id = %d ssb per RO = %f number of active SSB %u index = %d fdm %u symbol index %u freq_index %u total_RApreambles %u\n",
        frameP, slotP, prach_occasion_id, num_ssb_per_RO, num_active_ssb, index, fdm, start_symbol_index, freq_index, total_RApreambles);

  return index;
}

//Compute Total active SSBs and RO available
void find_SSB_and_RO_available(gNB_MAC_INST *nrmac)
{
  /* already mutex protected through nr_mac_config_scc() */
  //NR_SCHED_ENSURE_LOCKED(&nrmac->sched_lock);

  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_config_request_scf_t *cfg = &nrmac->config[0];
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  uint8_t config_index = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t mu,N_dur=0,N_t_slot=0,start_symbol=0,N_RA_slot = 0;
  uint16_t format,N_RA_sfn = 0,unused_RA_occasion,repetition = 0;
  uint8_t num_active_ssb = 0;
  uint8_t max_association_period = 1;

  struct NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB *ssb_perRACH_OccasionAndCB_PreamblesPerSSB = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB;

  switch (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present){
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneEighth:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneEighth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneFourth:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneFourth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneHalf:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneHalf + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_two:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.two + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_four:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.four;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_eight:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.eight;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_sixteen:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.sixteen;
      break;
    default:
      AssertFatal(1 == 0, "Unsupported ssb_perRACH_config %d\n", ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present);
      break;
    }

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    mu = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  // prach is scheduled according to configuration index and tables 6.3.3.2.2 to 6.3.3.2.4
  get_nr_prach_occasion_info_from_index(config_index,
                                        scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                        mu,
                                        cc->frame_type,
                                        &format,
                                        &start_symbol,
                                        &N_t_slot,
                                        &N_dur,
                                        &N_RA_slot,
                                        &N_RA_sfn,
                                        &max_association_period);

  float num_ssb_per_RO = ssb_per_rach_occasion[cfg->prach_config.ssb_per_rach.value];	
  uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
  uint64_t L_ssb = (((uint64_t) cfg->ssb_table.ssb_mask_list[0].ssb_mask.value)<<32) | cfg->ssb_table.ssb_mask_list[1].ssb_mask.value ;
  uint32_t total_RA_occasions = N_RA_sfn * N_t_slot * N_RA_slot * fdm;

  for(int i = 0; i < 64; i++) {
    if ((L_ssb >> (63 - i)) & 0x01) { // only if the bit of L_ssb at current ssb index is 1
      cc->ssb_index[num_active_ssb] = i;
      num_active_ssb++;
    }
  }

  cc->total_prach_occasions_per_config_period = total_RA_occasions;
  for(int i = 1; (1 << (i-1)) <= max_association_period; i++) {
    cc->max_association_period = (1 << (i - 1));
    total_RA_occasions = total_RA_occasions * cc->max_association_period;
    if(total_RA_occasions >= (int) (num_active_ssb / num_ssb_per_RO)) {
      repetition = (uint16_t)((total_RA_occasions * num_ssb_per_RO) / num_active_ssb);
      break;
    }
  }

  unused_RA_occasion = total_RA_occasions - (int)((num_active_ssb * repetition) / num_ssb_per_RO);
  cc->total_prach_occasions = total_RA_occasions - unused_RA_occasion;
  cc->num_active_ssb = num_active_ssb;

  LOG_D(NR_MAC,
        "Total available RO %d, num of active SSB %d: unused RO = %d association_period %u N_RA_sfn %u total_prach_occasions_per_config_period %u\n",
        cc->total_prach_occasions,
        cc->num_active_ssb,
        unused_RA_occasion,
        cc->max_association_period,
        N_RA_sfn,
        cc->total_prach_occasions_per_config_period);
}		
		
void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t slotP)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  /* already mutex protected: held in gNB_dlsch_ulsch_scheduler() */
  NR_SCHED_ENSURE_LOCKED(&gNB->sched_lock);

  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  int mu;
  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    mu = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  int index = ul_buffer_index(frameP, slotP, mu, gNB->UL_tti_req_ahead_size);
  nfapi_nr_ul_tti_request_t *UL_tti_req = &RC.nrmac[module_idP]->UL_tti_req_ahead[0][index];
  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];

  if (is_nr_UL_slot(scc->tdd_UL_DL_ConfigurationCommon, slotP, cc->frame_type)) {
    const NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &rach_ConfigCommon->rach_ConfigGeneric;
    uint8_t config_index = rach_ConfigGeneric->prach_ConfigurationIndex;
    uint8_t N_dur, N_t_slot, start_symbol = 0, N_RA_slot;
    uint16_t RA_sfn_index = -1;
    uint8_t config_period = 1;
    uint16_t format;
    int slot_index = 0;
    uint16_t prach_occasion_id = -1;

    int bwp_start = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
    // prach is scheduled according to configuration index and tables 6.3.3.2.2 to 6.3.3.2.4
    if (get_nr_prach_info_from_index(config_index,
                                     (int)frameP,
                                     (int)slotP,
                                     scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                     mu,
                                     cc->frame_type,
                                     &format,
                                     &start_symbol,
                                     &N_t_slot,
                                     &N_dur,
                                     &RA_sfn_index,
                                     &N_RA_slot,
                                     &config_period)) {

      uint16_t format0 = format & 0xff;      // first column of format from table
      uint16_t format1 = (format >> 8) & 0xff; // second column of format from table

      if (N_RA_slot > 1) { //more than 1 PRACH slot in a subframe
        if (slotP % 2 == 1)
          slot_index = 1;
        else
          slot_index = 0;
      } else if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
        slot_index = 0;
      }

      UL_tti_req->SFN = frameP;
      UL_tti_req->Slot = slotP;
      UL_tti_req->rach_present = 1;
      NR_beam_alloc_t beam = {0};
      for (int fdm_index = 0; fdm_index < fdm; fdm_index++) { // one structure per frequency domain occasion
        for (int td_index = 0; td_index < N_t_slot; td_index++) {

          prach_occasion_id = (((frameP % (cc->max_association_period * config_period))/config_period) * cc->total_prach_occasions_per_config_period) +
                              (RA_sfn_index + slot_index) * N_t_slot * fdm + td_index * fdm + fdm_index;

          if (prach_occasion_id >= cc->total_prach_occasions) // to be confirmed: unused occasion?
            continue;

          float num_ssb_per_RO = ssb_per_rach_occasion[cfg->prach_config.ssb_per_rach.value];
          if(num_ssb_per_RO <= 1) {
            int ssb_index = (int) (prach_occasion_id / (int)(1 / num_ssb_per_RO)) % cc->num_active_ssb;
            beam = beam_allocation_procedure(&gNB->beam_info, frameP, slotP, ssb_index, nr_slots_per_frame[mu]);
            AssertFatal(beam.idx >= 0, "Cannot allocate PRACH corresponding to SSB %d in any available beam\n", ssb_index);
          }
          else {
            int first_ssb_index = (prach_occasion_id * (int)num_ssb_per_RO) % cc->num_active_ssb;
            for(int j = first_ssb_index; j < first_ssb_index + num_ssb_per_RO; j++) {
              beam = beam_allocation_procedure(&gNB->beam_info, frameP, slotP, j, nr_slots_per_frame[mu]);
              AssertFatal(beam.idx >= 0, "Cannot allocate PRACH corresponding to SSB %d in any available beam\n", j);
            }
          }
          if(td_index == 0) {
            AssertFatal(UL_tti_req->n_pdus < sizeof(UL_tti_req->pdus_list) / sizeof(UL_tti_req->pdus_list[0]),
                        "Invalid UL_tti_req->n_pdus %d\n", UL_tti_req->n_pdus);

            UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE;
            UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_prach_pdu_t);
            nfapi_nr_prach_pdu_t  *prach_pdu = &UL_tti_req->pdus_list[UL_tti_req->n_pdus].prach_pdu;
            memset(prach_pdu,0,sizeof(nfapi_nr_prach_pdu_t));
            UL_tti_req->n_pdus+=1;

            // filling the prach fapi structure
            prach_pdu->phys_cell_id = *scc->physCellId;
            prach_pdu->num_prach_ocas = N_t_slot;
            prach_pdu->prach_start_symbol = start_symbol;
            prach_pdu->num_ra = fdm_index;
            prach_pdu->num_cs = get_NCS(rach_ConfigGeneric->zeroCorrelationZoneConfig,
                                        format0,
                                        rach_ConfigCommon->restrictedSetConfig);

            LOG_D(NR_MAC,
                  "Frame %d, Slot %d: Prach Occasion id = %u  fdm index = %u start symbol = %u slot index = %u subframe index = %u \n",
                  frameP,
                  slotP,
                  prach_occasion_id,
                  prach_pdu->num_ra,
                  prach_pdu->prach_start_symbol,
                  slot_index,
                  RA_sfn_index);
            // SCF PRACH PDU format field does not consider A1/B1 etc. possibilities
            // We added 9 = A1/B1 10 = A2/B2 11 A3/B3
            if (format1!=0xff) {
              switch(format0) {
                case 0xa1:
                  prach_pdu->prach_format = 11;
                  break;
                case 0xa2:
                  prach_pdu->prach_format = 12;
                  break;
                case 0xa3:
                  prach_pdu->prach_format = 13;
                  break;
              default:
                AssertFatal(1==0,"Only formats A1/B1 A2/B2 A3/B3 are valid for dual format");
              }
            }
            else{
              switch(format0) {
                case 0:
                  prach_pdu->prach_format = 0;
                  break;
                case 1:
                  prach_pdu->prach_format = 1;
                  break;
                case 2:
                  prach_pdu->prach_format = 2;
                  break;
                case 3:
                  prach_pdu->prach_format = 3;
                  break;
                case 0xa1:
                  prach_pdu->prach_format = 4;
                  break;
                case 0xa2:
                  prach_pdu->prach_format = 5;
                  break;
                case 0xa3:
                  prach_pdu->prach_format = 6;
                  break;
                case 0xb1:
                  prach_pdu->prach_format = 7;
                  break;
                case 0xb4:
                  prach_pdu->prach_format = 8;
                  break;
                case 0xc0:
                  prach_pdu->prach_format = 9;
                  break;
                case 0xc2:
                  prach_pdu->prach_format = 10;
                  break;
              default:
                AssertFatal(1==0,"Invalid PRACH format");
              }
            }
          }
        }
      }

      // block resources in vrb_map_UL
      const int mu_pusch = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
      const int16_t n_ra_rb = get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, mu_pusch);
      index = ul_buffer_index(frameP, slotP, mu, gNB->vrb_map_UL_size);
      uint16_t *vrb_map_UL = &cc->vrb_map_UL[beam.idx][index * MAX_BWP_SIZE];
      for (int i = 0; i < n_ra_rb * fdm; ++i)
        vrb_map_UL[bwp_start + rach_ConfigGeneric->msg1_FrequencyStart + i] |= SL_to_bitmap(start_symbol, N_t_slot * N_dur);
    }
  }
}

static bool ra_contains_preamble(const NR_RA_t *ra, uint16_t preamble_index)
{
  for (int j = 0; j < ra->preambles.num_preambles; j++) {
    // check if the preamble received correspond to one of the listed or configured preambles
    if (preamble_index == ra->preambles.preamble_list[j]) {
      if (ra->rnti == 0 && get_softmodem_params()->nsa)
        continue;
      return true;
    }
  }
  return false;
}

static NR_RA_t *find_free_nr_RA(NR_RA_t *ra_base, int ra_count, uint16_t preamble_index)
{
  // for posteriority: in the past we checked the beam index, but only in CFRA
  //if (ra->cfra && preamble_index != ra->preamble.preamble_list[beam_index]])
  //  // then is not right RA
  //
  // that seems strange because:
  // - we check preamble_index already
  // - why only CFRA?

  for (int i = 0; i < ra_count; ++i) {
    NR_RA_t *ra = &ra_base[i];
    if (ra->ra_state != nrRA_gNB_IDLE)
      continue;
    if (!ra_contains_preamble(ra, preamble_index))
      continue;

    return ra;
  }
  return NULL;
}

void nr_initiate_ra_proc(module_id_t module_idP,
                         int CC_id,
                         frame_t frameP,
                         sub_frame_t slotP,
                         uint16_t preamble_index,
                         uint8_t freq_index,
                         uint8_t symbol,
                         int16_t timing_offset)
{
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 1);

  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_SCHED_LOCK(&nr_mac->sched_lock);

  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];

  NR_RA_t *ra = find_free_nr_RA(cc->ra, sizeofArray(cc->ra), preamble_index);
  if (ra == NULL) {
    LOG_E(NR_MAC, "FAILURE: %4d.%2d initiating RA procedure for preamble index %d: no free RA process\n", frameP, slotP, preamble_index);
    NR_SCHED_UNLOCK(&nr_mac->sched_lock);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);
    return;
  }

  if (ra->rnti == 0) { // This condition allows for the usage of a preconfigured rnti for the CFRA
    bool rnti_found = nr_mac_get_new_rnti(&nr_mac->UE_info, cc->ra, sizeofArray(cc->ra), &ra->rnti);
    if (!rnti_found) {
      LOG_E(NR_MAC, "initialisation random access: no more available RNTIs for new UE\n");
      nr_clear_ra_proc(ra);
      NR_SCHED_UNLOCK(&nr_mac->sched_lock);
      return;
    }
  }

  ra->ra_state = nrRA_Msg2;
  ra->preamble_frame = frameP;
  ra->preamble_slot = slotP;
  ra->preamble_index = preamble_index;
  ra->timing_offset = timing_offset;
  uint8_t ul_carrier_id = 0; // 0 for NUL 1 for SUL
  ra->RA_rnti = nr_get_ra_rnti(symbol, slotP, freq_index, ul_carrier_id);
  int index = ra - cc->ra;
  LOG_I(NR_MAC, "%d.%d UE RA-RNTI %04x TC-RNTI %04x: Activating RA process index %d\n", frameP, slotP, ra->RA_rnti, ra->rnti, index);

  // Configure RA BWP
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  configure_UE_BWP(nr_mac, scc, NULL, ra, NULL, -1, -1);

  uint8_t beam_index = ssb_index_from_prach(module_idP, frameP, slotP, preamble_index, freq_index, symbol);
  ra->beam_id = cc->ssb_index[beam_index];

  NR_SCHED_UNLOCK(&nr_mac->sched_lock);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);
}

static void start_ra_contention_resolution_timer(NR_RA_t *ra, const long ra_ContentionResolutionTimer, const int K2, const int scs)
{
  // 3GPP TS 38.331 Section 6.3.2 Radio resource control information elements
  // ra-ContentionResolutionTimer ENUMERATED {sf8, sf16, sf24, sf32, sf40, sf48, sf56, sf64}
  // The initial value for the contention resolution timer.
  // Value sf8 corresponds to 8 subframes, value sf16 corresponds to 16 subframes, and so on.
  // We add 2 * K2 because the timer runs from Msg2 transmission till Msg4 ACK reception
  ra->contention_resolution_timer = ((((int)ra_ContentionResolutionTimer + 1) * 8) << scs) + 2 * K2;
  LOG_E(NR_MAC,
        "Starting RA Contention Resolution timer with %d ms + 2 * %d K2 (%d slots) duration\n",
        ((int)ra_ContentionResolutionTimer + 1) * 8,
        K2,
        ra->contention_resolution_timer);
}

static void nr_generate_Msg3_retransmission(module_id_t module_idP,
                                            int CC_id,
                                            frame_t frame,
                                            sub_frame_t slot,
                                            NR_RA_t *ra,
                                            nfapi_nr_ul_dci_request_t *ul_dci_req)
{
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_UE_UL_BWP_t *ul_bwp = &ra->UL_BWP;
  NR_UE_ServingCell_Info_t *sc_info = &ra->sc_info;

  NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = ul_bwp->tdaList_Common;
  int mu = ul_bwp->scs;
  uint16_t K2 = *pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->k2 + get_NTN_Koffset(scc);
  const int sched_frame = (frame + (slot + K2) / nr_slots_per_frame[mu]) % MAX_FRAME_NUMBER;
  const int sched_slot = (slot + K2) % nr_slots_per_frame[mu];

  if (is_xlsch_in_slot(nr_mac->ulsch_slot_bitmap[sched_slot / 64], sched_slot)) {
    const int n_slots_frame = nr_slots_per_frame[mu];
    NR_beam_alloc_t beam_ul = beam_allocation_procedure(&nr_mac->beam_info,
                                                              sched_frame,
                                                              sched_slot,
                                                              ra->beam_id,
                                                              n_slots_frame);
    if (beam_ul.idx < 0)
      return;
    NR_beam_alloc_t beam_dci = beam_allocation_procedure(&nr_mac->beam_info, frame, slot, ra->beam_id, n_slots_frame);
    if (beam_dci.idx < 0) {
      reset_beam_status(&nr_mac->beam_info, sched_frame, sched_slot, ra->beam_id, n_slots_frame, beam_ul.new_beam);
      return;
    }
    int fh = 0;
    int startSymbolAndLength = pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
    int StartSymbolIndex, NrOfSymbols;
    SLIV2SL(startSymbolAndLength, &StartSymbolIndex, &NrOfSymbols);
    int mappingtype = pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->mappingType;

    int buffer_index = ul_buffer_index(sched_frame, sched_slot, mu, nr_mac->vrb_map_UL_size);
    uint16_t *vrb_map_UL = &nr_mac->common_channels[CC_id].vrb_map_UL[beam_ul.idx][buffer_index * MAX_BWP_SIZE];

    const int BWPSize = sc_info->initial_ul_BWPSize;
    const int BWPStart = sc_info->initial_ul_BWPStart;

    int rbStart = 0;
    for (int i = 0; (i < ra->msg3_nb_rb) && (rbStart <= (BWPSize - ra->msg3_nb_rb)); i++) {
      if (vrb_map_UL[rbStart + BWPStart + i]&SL_to_bitmap(StartSymbolIndex, NrOfSymbols)) {
        rbStart += i;
        i = 0;
      }
    }
    if (rbStart > (BWPSize - ra->msg3_nb_rb)) {
      // cannot find free vrb_map for msg3 retransmission in this slot
      return;
    }

    LOG_I(NR_MAC,
          "%4d%2d: RA RNTI %04x CC_id %d Scheduling retransmission of Msg3 in (%d,%d)\n",
          frame,
          slot,
          ra->rnti,
          CC_id,
          sched_frame,
          sched_slot);

    buffer_index = ul_buffer_index(sched_frame, sched_slot, mu, nr_mac->UL_tti_req_ahead_size);
    nfapi_nr_ul_tti_request_t *future_ul_tti_req = &nr_mac->UL_tti_req_ahead[CC_id][buffer_index];
    AssertFatal(future_ul_tti_req->SFN == sched_frame
                && future_ul_tti_req->Slot == sched_slot,
                "future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
                future_ul_tti_req->SFN,
                future_ul_tti_req->Slot,
                sched_frame,
                sched_slot);
    AssertFatal(future_ul_tti_req->n_pdus <
                sizeof(future_ul_tti_req->pdus_list) / sizeof(future_ul_tti_req->pdus_list[0]),
                "Invalid future_ul_tti_req->n_pdus %d\n", future_ul_tti_req->n_pdus);
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
    nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
    memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

    fill_msg3_pusch_pdu(pusch_pdu, scc,
                        ra->msg3_round,
                        startSymbolAndLength,
                        ra->rnti, mu,
                        BWPSize, BWPStart,
                        mappingtype, fh,
                        rbStart, ra->msg3_nb_rb);
    future_ul_tti_req->n_pdus += 1;

    // generation of DCI 0_0 to schedule msg3 retransmission
    NR_SearchSpace_t *ss = ra->ra_ss;
    NR_ControlResourceSet_t *coreset = ra->coreset;
    AssertFatal(coreset, "Coreset cannot be null for RA-Msg3 retransmission\n");

    const int coresetid = coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][coresetid];
    if (!pdcch_pdu_rel15) {
      nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu = &ul_dci_req->ul_dci_pdu_list[ul_dci_req->numPdus];
      memset(ul_dci_request_pdu, 0, sizeof(nfapi_nr_ul_dci_request_pdus_t));
      ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      pdcch_pdu_rel15 = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;
      ul_dci_req->numPdus += 1;
      nr_configure_pdcch(pdcch_pdu_rel15, coreset, &ra->sched_pdcch);
      nr_mac->pdcch_pdu_idx[CC_id][coresetid] = pdcch_pdu_rel15;
    }

    uint8_t aggregation_level;
    int CCEIndex = get_cce_index(nr_mac,
                                 CC_id, slot, 0,
                                 &aggregation_level,
                                 beam_dci.idx,
                                 ss,
                                 coreset,
                                 &ra->sched_pdcch,
                                 true);
    if (CCEIndex < 0) {
      LOG_E(NR_MAC, "UE %04x cannot find free CCE!\n", ra->rnti);
      return;
    }

    // Fill PDCCH DL DCI PDU
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
    pdcch_pdu_rel15->numDlDci++;
    dci_pdu->RNTI = ra->rnti;
    dci_pdu->ScramblingId = *scc->physCellId;
    dci_pdu->ScramblingRNTI = 0;
    dci_pdu->AggregationLevel = aggregation_level;
    dci_pdu->CceIndex = CCEIndex;
    dci_pdu->beta_PDCCH_1_0 = 0;
    dci_pdu->powerControlOffsetSS = 1;

    dci_pdu_rel15_t uldci_payload={0};

    config_uldci(sc_info,
                 pusch_pdu,
                 &uldci_payload,
                 NULL,
                 ra->Msg3_tda_id,
                 ra->msg3_TPC,
                 1, // Not toggling NDI in msg3 retransmissions
                 ul_bwp);

    fill_dci_pdu_rel15(sc_info,
                       &ra->DL_BWP,
                       ul_bwp,
                       dci_pdu,
                       &uldci_payload,
                       NR_UL_DCI_FORMAT_0_0,
                       TYPE_TC_RNTI_,
                       ul_bwp->bwp_id,
                       ss,
                       coreset,
                       0, // parameter not needed for DCI 0_0
                       nr_mac->cset0_bwp_size);

    // Mark the corresponding RBs as used

    fill_pdcch_vrb_map(nr_mac,
                       CC_id,
                       &ra->sched_pdcch,
                       CCEIndex,
                       aggregation_level,
                       beam_dci.idx);

    for (int rb = 0; rb < ra->msg3_nb_rb; rb++) {
      vrb_map_UL[rbStart + BWPStart + rb] |= SL_to_bitmap(StartSymbolIndex, NrOfSymbols);
    }

    // Restart RA contention resolution timer in Msg3 retransmission slot (current slot + K2)
    // 3GPP TS 38.321 Section 5.1.5 Contention Resolution
    start_ra_contention_resolution_timer(
        ra,
        scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer,
        K2,
        ra->UL_BWP.scs);

    // reset state to wait msg3
    ra->ra_state = nrRA_WAIT_Msg3;
    ra->Msg3_frame = sched_frame;
    ra->Msg3_slot = sched_slot;

  }
}

static bool get_feasible_msg3_tda(frame_type_t frame_type,
                                  const NR_ServingCellConfigCommon_t *scc,
                                  int mu_delta,
                                  uint64_t ulsch_slot_bitmap[3],
                                  const NR_PUSCH_TimeDomainResourceAllocationList_t *tda_list,
                                  int slots_per_frame,
                                  int frame,
                                  int slot,
                                  NR_RA_t *ra,
                                  NR_beam_info_t *beam_info,
                                  const NR_TDD_UL_DL_Pattern_t *tdd)
{
  DevAssert(tda_list != NULL);

  const int NTN_gNB_Koffset = get_NTN_Koffset(scc);

  int tdd_period_slot = tdd ? slots_per_frame / get_nb_periods_per_frame(tdd->dl_UL_TransmissionPeriodicity) : slots_per_frame;
  for (int i = 0; i < tda_list->list.count; i++) {
    // check if it is UL
    long k2 = *tda_list->list.array[i]->k2 + NTN_gNB_Koffset;
    int abs_slot = slot + k2 + mu_delta;
    int temp_frame = (frame + (abs_slot / slots_per_frame)) & 1023;
    int temp_slot = abs_slot % slots_per_frame; // msg3 slot according to 8.3 in 38.213
    if ((frame_type == TDD) && !is_xlsch_in_slot(ulsch_slot_bitmap[temp_slot / 64], temp_slot))
      continue;

    // check if enough symbols in case of mixed slot
    bool is_mixed = false;
    if (frame_type == TDD) {
      bool has_mixed = tdd->nrofUplinkSymbols != 0 || tdd->nrofDownlinkSymbols != 0;
      is_mixed = has_mixed && ((temp_slot % tdd_period_slot) == tdd->nrofDownlinkSlots);
    }
    // if the mixed slot has not enough symbols, skip
    if (is_mixed && tdd->nrofUplinkSymbols < 3)
      continue;

    uint16_t slot_mask = is_mixed ? SL_to_bitmap(14 - tdd->nrofUplinkSymbols, tdd->nrofUplinkSymbols) : 0x3fff;
    long startSymbolAndLength = tda_list->list.array[i]->startSymbolAndLength;
    int start, nr;
    SLIV2SL(startSymbolAndLength, &start, &nr);
    uint16_t msg3_mask = SL_to_bitmap(start, nr);
    LOG_D(NR_MAC, "Check Msg3 TDA %d for slot %d: k2 %ld, S %d L %d\n", i, temp_slot, k2, start, nr);
    /* if this start and length of this TDA cannot be fulfilled, skip */
    if ((slot_mask & msg3_mask) != msg3_mask)
      continue;

    // check if it is possible to allocate MSG3 in a beam in this slot
    NR_beam_alloc_t beam = beam_allocation_procedure(beam_info, temp_frame, temp_slot, ra->beam_id, slots_per_frame);
    if (beam.idx < 0)
      continue;
      
    // is in mixed slot with more or equal than 3 symbols, or UL slot
    ra->Msg3_frame = temp_frame;
    ra->Msg3_slot = temp_slot;
    ra->Msg3_tda_id = i;
    ra->Msg3_beam = beam;
    return true;
  }

  return false;
}

static void nr_get_Msg3alloc(module_id_t module_id,
                             int CC_id,
                             NR_ServingCellConfigCommon_t *scc,
                             sub_frame_t current_slot,
                             frame_t current_frame,
                             NR_RA_t *ra)
{
  // msg3 is scheduled in mixed slot in the following TDD period
  DevAssert(ra->Msg3_tda_id >= 0 && ra->Msg3_tda_id < 16);

  uint16_t msg3_nb_rb = 8; // sdu has 6 or 8 bytes
  gNB_MAC_INST *mac = RC.nrmac[module_id];

  NR_UE_UL_BWP_t *ul_bwp = &ra->UL_BWP;
  NR_UE_ServingCell_Info_t *sc_info = &ra->sc_info;

  const NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = ul_bwp->tdaList_Common;
  int mu = ul_bwp->scs;

  int startSymbolAndLength = pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength, &ra->msg3_startsymb, &ra->msg3_nbSymb);

  LOG_I(NR_MAC,
        "UE %04x: Msg3 scheduled at %d.%d (%d.%d TDA %u)\n",
        ra->rnti,
        ra->Msg3_frame,
        ra->Msg3_slot,
        current_frame,
        current_slot,
        ra->Msg3_tda_id);
  const int buffer_index = ul_buffer_index(ra->Msg3_frame, ra->Msg3_slot, mu, mac->vrb_map_UL_size);
  uint16_t *vrb_map_UL = &mac->common_channels[CC_id].vrb_map_UL[ra->Msg3_beam.idx][buffer_index * MAX_BWP_SIZE];

  int bwpSize = sc_info->initial_ul_BWPSize;
  int bwpStart = sc_info->initial_ul_BWPStart;
  if (bwpSize != ul_bwp->BWPSize || bwpStart != ul_bwp->BWPStart) {
    int act_bwp_start = ul_bwp->BWPStart;
    int act_bwp_size  = ul_bwp->BWPSize;
    if (!((bwpStart >= act_bwp_start) && ((bwpStart+bwpSize) <= (act_bwp_start+act_bwp_size))))
      bwpStart = act_bwp_start;
  }

  /* search msg3_nb_rb free RBs */
  int rbSize = 0;
  int rbStart = 0;
  while (rbSize < msg3_nb_rb) {
    rbStart += rbSize; /* last iteration rbSize was not enough, skip it */
    rbSize = 0;
    while (rbStart < bwpSize && (vrb_map_UL[rbStart + bwpStart] & SL_to_bitmap(ra->msg3_startsymb, ra->msg3_nbSymb)))
      rbStart++;
    AssertFatal(rbStart < bwpSize - msg3_nb_rb, "no space to allocate Msg 3 for RA!\n");
    while (rbStart + rbSize < bwpSize
           && !(vrb_map_UL[rbStart + bwpStart + rbSize] & SL_to_bitmap(ra->msg3_startsymb, ra->msg3_nbSymb)) && rbSize < msg3_nb_rb)
      rbSize++;
  }
  ra->msg3_nb_rb = msg3_nb_rb;
  ra->msg3_first_rb = rbStart;
  ra->msg3_bwp_start = bwpStart;
}

static void fill_msg3_pusch_pdu(nfapi_nr_pusch_pdu_t *pusch_pdu,
                                NR_ServingCellConfigCommon_t *scc,
                                int round,
                                int startSymbolAndLength,
                                rnti_t rnti,
                                int scs,
                                int bwp_size,
                                int bwp_start,
                                int mappingtype,
                                int fh,
                                int msg3_first_rb,
                                int msg3_nb_rb)
{
  int start_symbol_index,nr_of_symbols;

  SLIV2SL(startSymbolAndLength, &start_symbol_index, &nr_of_symbols);
  int mcsindex = -1; // init value

  pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_pdu->rnti = rnti;
  pusch_pdu->handle = 0;
  pusch_pdu->bwp_start = bwp_start;
  pusch_pdu->bwp_size = bwp_size;
  pusch_pdu->subcarrier_spacing = scs;
  pusch_pdu->cyclic_prefix = 0;
  pusch_pdu->mcs_table = 0;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
    pusch_pdu->transform_precoding = 1; // disabled
  else {
    pusch_pdu->transform_precoding = 0; // enabled
    pusch_pdu->dfts_ofdm.low_papr_group_number = *scc->physCellId % 30;
    pusch_pdu->dfts_ofdm.low_papr_sequence_number = 0;
    if (scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding)
      AssertFatal(1==0,"Hopping mode is not supported in transform precoding\n");
  }
  pusch_pdu->data_scrambling_id = *scc->physCellId;
  pusch_pdu->nrOfLayers = 1;
  pusch_pdu->ul_dmrs_symb_pos = get_l_prime(nr_of_symbols,mappingtype,pusch_dmrs_pos2,pusch_len1,start_symbol_index, scc->dmrs_TypeA_Position);
  LOG_D(NR_MAC, "MSG3 start_sym:%d NR Symb:%d mappingtype:%d, ul_dmrs_symb_pos:%x\n", start_symbol_index, nr_of_symbols, mappingtype, pusch_pdu->ul_dmrs_symb_pos);
  pusch_pdu->dmrs_config_type = 0;
  pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
  pusch_pdu->pusch_identity = *scc->physCellId; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
  pusch_pdu->scid = 0; //DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]. Should match what is sent in DCI 0_1, otherwise set to 0.
  pusch_pdu->dmrs_ports = 1;  // 6.2.2 in 38.214 only port 0 to be used
  pusch_pdu->num_dmrs_cdm_grps_no_data = 2;  // no data in dmrs symbols as in 6.2.2 in 38.214
  pusch_pdu->resource_alloc = 1; //type 1
  memset(pusch_pdu->rb_bitmap, 0, sizeof(pusch_pdu->rb_bitmap));
  pusch_pdu->rb_start = msg3_first_rb;
  if (msg3_nb_rb > pusch_pdu->bwp_size)
    AssertFatal(1==0,"MSG3 allocated number of RBs exceed the BWP size\n");
  else
    pusch_pdu->rb_size = msg3_nb_rb;
  pusch_pdu->vrb_to_prb_mapping = 0;

  pusch_pdu->frequency_hopping = fh;
  //pusch_pdu->tx_direct_current_location;
  //The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299,
  //which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300,
  //which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  pusch_pdu->uplink_frequency_shift_7p5khz = 0;
  //Resource Allocation in time domain
  pusch_pdu->start_symbol_index = start_symbol_index;
  pusch_pdu->nr_of_symbols = nr_of_symbols;
  //Optional Data only included if indicated in pduBitmap
  pusch_pdu->pusch_data.rv_index = nr_rv_round_map[round%4];
  pusch_pdu->pusch_data.harq_process_id = 0;
  pusch_pdu->pusch_data.new_data_indicator = (round == 0) ? 1 : 0;;
  pusch_pdu->pusch_data.num_cb = 0;

  // Beamforming
  pusch_pdu->beamforming.num_prgs = 0;
  pusch_pdu->beamforming.prg_size = 0; // bwp_size;
  pusch_pdu->beamforming.dig_bf_interface = 0;
  if (pusch_pdu->beamforming.num_prgs > 0) {
    if (pusch_pdu->beamforming.prgs_list == NULL) {
      pusch_pdu->beamforming.prgs_list = calloc(pusch_pdu->beamforming.num_prgs, sizeof(*pusch_pdu->beamforming.prgs_list));
    }
    if (pusch_pdu->beamforming.dig_bf_interface > 0) {
      if (pusch_pdu->beamforming.prgs_list[0].dig_bf_interface_list == NULL) {
        pusch_pdu->beamforming.prgs_list[0].dig_bf_interface_list =
            calloc(pusch_pdu->beamforming.dig_bf_interface, sizeof(*pusch_pdu->beamforming.prgs_list[0].dig_bf_interface_list));
      }
    }
    pusch_pdu->beamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx = 0;
  }
  int num_dmrs_symb = 0;
  for(int i = start_symbol_index; i < start_symbol_index+nr_of_symbols; i++)
    num_dmrs_symb += (pusch_pdu->ul_dmrs_symb_pos >> i) & 1;
  int TBS = 0;
  while(TBS<7) {  // TBS for msg3 is 7 bytes (except for RRCResumeRequest1 currently not implemented)
    mcsindex++;
    AssertFatal(mcsindex <= 28, "Exceeding MCS limit for Msg3\n");
    int R = nr_get_code_rate_ul(mcsindex,pusch_pdu->mcs_table);
    pusch_pdu->target_code_rate = R;
    pusch_pdu->qam_mod_order = nr_get_Qm_ul(mcsindex,pusch_pdu->mcs_table);
    TBS = nr_compute_tbs(pusch_pdu->qam_mod_order,
                         R,
                         pusch_pdu->rb_size,
                         pusch_pdu->nr_of_symbols,
                         num_dmrs_symb*12, // nb dmrs set for no data in dmrs symbol
                         0, //nb_rb_oh
                         0, // to verify tb scaling
                         pusch_pdu->nrOfLayers)>>3;

    pusch_pdu->mcs_index = mcsindex;
    pusch_pdu->pusch_data.tb_size = TBS;
    pusch_pdu->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS<<3,R);
  }
}

static void nr_add_msg3(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra, uint8_t *RAR_pdu)
{
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_UE_UL_BWP_t *ul_bwp = &ra->UL_BWP;
  NR_UE_ServingCell_Info_t *sc_info = &ra->sc_info;

  if (ra->ra_state == nrRA_gNB_IDLE) {
    LOG_W(NR_MAC,"RA is not active for RA %X. skipping msg3 scheduling\n", ra->rnti);
    return;
  }

  const int scs = ul_bwp->scs;
  const uint16_t mask = SL_to_bitmap(ra->msg3_startsymb, ra->msg3_nbSymb);
  int buffer_index = ul_buffer_index(ra->Msg3_frame, ra->Msg3_slot, scs, mac->vrb_map_UL_size);
  uint16_t *vrb_map_UL = &RC.nrmac[module_idP]->common_channels[CC_id].vrb_map_UL[ra->Msg3_beam.idx][buffer_index * MAX_BWP_SIZE];
  for (int i = 0; i < ra->msg3_nb_rb; ++i) {
    AssertFatal(!(vrb_map_UL[i + ra->msg3_first_rb + ra->msg3_bwp_start] & mask),
                "RB %d in %4d.%2d is already taken, cannot allocate Msg3!\n",
                i + ra->msg3_first_rb,
                ra->Msg3_frame,
                ra->Msg3_slot);
    vrb_map_UL[i + ra->msg3_first_rb + ra->msg3_bwp_start] |= mask;
  }

  LOG_D(NR_MAC, "UE %04x: %d.%d RA is active, Msg3 in (%d,%d)\n", ra->rnti, frameP, slotP, ra->Msg3_frame, ra->Msg3_slot);
  buffer_index = ul_buffer_index(ra->Msg3_frame, ra->Msg3_slot, scs, mac->UL_tti_req_ahead_size);
  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_idP]->UL_tti_req_ahead[CC_id][buffer_index];
  AssertFatal(future_ul_tti_req->SFN == ra->Msg3_frame
              && future_ul_tti_req->Slot == ra->Msg3_slot,
              "future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
              future_ul_tti_req->SFN,
              future_ul_tti_req->Slot,
              ra->Msg3_frame,
              ra->Msg3_slot);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
  nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
  memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

  const int ibwp_size = sc_info->initial_ul_BWPSize;
  const int fh = (ul_bwp->pusch_Config && ul_bwp->pusch_Config->frequencyHopping) ? 1 : 0;
  const int startSymbolAndLength = ul_bwp->tdaList_Common->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
  const int mappingtype = ul_bwp->tdaList_Common->list.array[ra->Msg3_tda_id]->mappingType;

  LOG_D(NR_MAC,
        "UE %04x: %d.%d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d)\n",
        ra->rnti,
        frameP,
        slotP,
        ra->Msg3_frame,
        ra->Msg3_slot,
        ra->msg3_nb_rb,
        ra->msg3_first_rb,
        ra->msg3_round);

  fill_msg3_pusch_pdu(pusch_pdu,scc,
                      ra->msg3_round,
                      startSymbolAndLength,
                      ra->rnti, scs,
                      ibwp_size, ra->msg3_bwp_start,
                      mappingtype, fh,
                      ra->msg3_first_rb, ra->msg3_nb_rb);
  future_ul_tti_req->n_pdus += 1;

  // calling function to fill rar message
  nr_fill_rar(module_idP, ra, RAR_pdu, pusch_pdu);
}


static void find_monitoring_periodicity_offset_common(const NR_SearchSpace_t *ss, uint16_t *slot_period, uint16_t *offset)
{
  switch (ss->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      *slot_period = 1;
      *offset = 0;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      *slot_period = 2;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      *slot_period = 4;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      *slot_period = 5;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      *slot_period = 8;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      *slot_period = 10;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      *slot_period = 16;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      *slot_period = 20;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      *slot_period = 40;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      *slot_period = 80;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      *slot_period = 160;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      *slot_period = 320;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      *slot_period = 640;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      *slot_period = 1280;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      *slot_period = 2560;
      *offset = ss->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
    default:
      AssertFatal(1 == 0, "Invalid monitoring slot periodicity and offset value\n");
      break;
  }
}

static bool check_msg2_monitoring(const NR_RA_t *ra, int mu, int current_frame, int current_slot)
{
  int n = nr_slots_per_frame[mu];

  // check if the slot is not among the PDCCH monitored ones (38.213 10.1)
  uint16_t monitoring_slot_period, monitoring_offset;
  find_monitoring_periodicity_offset_common(ra->ra_ss, &monitoring_slot_period, &monitoring_offset);
  if ((current_frame * n + current_slot - monitoring_offset) % monitoring_slot_period != 0)
    return false;
  return true;
}

static int get_response_window(e_NR_RACH_ConfigGeneric__ra_ResponseWindow response_window)
{
  int slots;
  switch (response_window) {
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl1:
      slots = 1;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl2:
      slots = 2;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl4:
      slots = 4;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl8:
      slots = 8;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl10:
      slots = 10;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20:
      slots = 20;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl40:
      slots = 40;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl80:
      slots = 80;
      break;
    default:
      AssertFatal(false, "Invalid response window value %d\n", response_window);
  }
  return slots;
}

static bool msg2_in_response_window(int rach_frame, int rach_slot, int mu, long rrc_ra_ResponseWindow, int current_frame, int current_slot)
{
  int window_slots = get_response_window(rrc_ra_ResponseWindow);
  int n = nr_slots_per_frame[mu];

  int abs_rach = n * rach_frame + rach_slot;
  int abs_now = n * current_frame + current_slot;
  int diff = (n * 1024 + abs_now - abs_rach) % (n * 1024);

  bool in_window = diff <= window_slots;
  if (!in_window) {
    LOG_W(NR_MAC,
          "exceeded RA window: preamble at %d.%2d now %d.%d (diff %d), ra_ResponseWindow %ld/%d slots\n",
          rach_frame,
          rach_slot,
          current_frame,
          current_slot,
          diff,
          rrc_ra_ResponseWindow,
          window_slots);
  }
  return in_window;
}

static void nr_generate_Msg2(module_id_t module_idP,
                             int CC_id,
                             frame_t frameP,
                             sub_frame_t slotP,
                             NR_RA_t *ra,
                             nfapi_nr_dl_tti_request_t *DL_req,
                             nfapi_nr_tx_data_request_t *TX_req)
{
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];

  // no DL -> cannot send Msg2
  if (!is_xlsch_in_slot(nr_mac->dlsch_slot_bitmap[slotP / 64], slotP)) {
    return;
  }

  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_UE_DL_BWP_t *dl_bwp = &ra->DL_BWP;
  NR_UE_ServingCell_Info_t *sc_info = &ra->sc_info;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  long rrc_ra_ResponseWindow =
      scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow;
  if (!msg2_in_response_window(ra->preamble_frame, ra->preamble_slot, dl_bwp->scs, rrc_ra_ResponseWindow, frameP, slotP)) {
    LOG_E(NR_MAC, "UE RA-RNTI %04x TC-RNTI %04x: exceeded RA window, cannot schedule Msg2\n", ra->RA_rnti, ra->rnti);
    nr_clear_ra_proc(ra);
    return;
  }

  if (!check_msg2_monitoring(ra, dl_bwp->scs, frameP, slotP)) {
    LOG_E(NR_MAC, "UE RA-RNTI %04x TC-RNTI %04x: Msg2 not monitored by UE\n", ra->RA_rnti, ra->rnti);
    return;
  }

  const int n_slots_frame = nr_slots_per_frame[dl_bwp->scs];
  NR_beam_alloc_t beam = beam_allocation_procedure(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame);
  if (beam.idx < 0)
    return;

  const NR_UE_UL_BWP_t *ul_bwp = &ra->UL_BWP;
  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;
  bool ret = get_feasible_msg3_tda(cc->frame_type,
                                   scc,
                                   DELTA[ul_bwp->scs],
                                   nr_mac->ulsch_slot_bitmap,
                                   ul_bwp->tdaList_Common,
                                   nr_slots_per_frame[ul_bwp->scs],
                                   frameP,
                                   slotP,
                                   ra,
                                   &nr_mac->beam_info,
                                   tdd);
  if (!ret || ra->Msg3_tda_id > 15) {
    LOG_D(NR_MAC, "UE RNTI %04x %d.%d: infeasible Msg3 TDA\n", ra->rnti, frameP, slotP);
    reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
    return;
  }

  int mcsIndex = -1; // initialization value
  int rbStart = 0;
  int rbSize = 8;

  NR_SearchSpace_t *ss = ra->ra_ss;

  long BWPStart = 0;
  long BWPSize = 0;
  NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config = NULL;
  if (*ss->controlResourceSetId != 0) {
    BWPStart = dl_bwp->BWPStart;
    BWPSize = sc_info->initial_dl_BWPSize;
  } else {
    type0_PDCCH_CSS_config = &nr_mac->type0_PDCCH_CSS_config[ra->beam_id];
    BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
    BWPSize = type0_PDCCH_CSS_config->num_rbs;
  }

  NR_ControlResourceSet_t *coreset = ra->coreset;
  AssertFatal(coreset, "Coreset cannot be null for RA-Msg2\n");
  const int coresetid = coreset->controlResourceSetId;
  // Calculate number of symbols
  int time_domain_assignment = get_dl_tda(nr_mac, scc, slotP);
  int mux_pattern = type0_PDCCH_CSS_config ? type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern : 1;
  NR_tda_info_t tda_info = get_dl_tda_info(dl_bwp,
                                           ss->searchSpaceType->present,
                                           time_domain_assignment,
                                           scc->dmrs_TypeA_Position,
                                           mux_pattern,
                                           TYPE_RA_RNTI_,
                                           coresetid,
                                           false);
  if (!tda_info.valid_tda)
    return;

  uint16_t *vrb_map = cc[CC_id].vrb_map[beam.idx];
  for (int i = 0; (i < rbSize) && (rbStart <= (BWPSize - rbSize)); i++) {
    if (vrb_map[BWPStart + rbStart + i] & SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols)) {
      rbStart += i;
      i = 0;
    }
  }

  if (rbStart > (BWPSize - rbSize)) {
    LOG_W(NR_MAC, "Cannot find free vrb_map for RA RNTI %04x!\n", ra->RA_rnti);
    reset_beam_status(&nr_mac->beam_info, ra->Msg3_frame, ra->Msg3_slot, ra->beam_id, n_slots_frame, ra->Msg3_beam.new_beam);
    reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
    return;
  }

  // Checking if the DCI allocation is feasible in current subframe
  nfapi_nr_dl_tti_request_body_t *dl_req = &DL_req->dl_tti_request_body;
  if (dl_req->nPDUs > NFAPI_NR_MAX_DL_TTI_PDUS - 2) {
    LOG_W(NR_MAC, "UE %04x: %d.%d FAPI DL structure is full\n", ra->rnti, frameP, slotP);
    reset_beam_status(&nr_mac->beam_info, ra->Msg3_frame, ra->Msg3_slot, ra->beam_id, n_slots_frame, ra->Msg3_beam.new_beam);
    reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
    return;
  }

  uint8_t aggregation_level;
  int CCEIndex = get_cce_index(nr_mac, CC_id, slotP, 0, &aggregation_level, beam.idx, ss, coreset, &ra->sched_pdcch, true);

  if (CCEIndex < 0) {
    LOG_W(NR_MAC, "UE %04x: %d.%d cannot find free CCE for Msg2!\n", ra->rnti, frameP, slotP);
    reset_beam_status(&nr_mac->beam_info, ra->Msg3_frame, ra->Msg3_slot, ra->beam_id, n_slots_frame, ra->Msg3_beam.new_beam);
    reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
    return;
  }

  LOG_D(NR_MAC, "Msg2 startSymbolIndex.nrOfSymbols %d.%d\n", tda_info.startSymbolIndex, tda_info.nrOfSymbols);

  // look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not exist, create it. This is especially
  // important if we have multiple RAs, and the DLSCH has to reuse them, so we need to mark them
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][coresetid];
  if (!pdcch_pdu_rel15) {
    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2 + sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    dl_req->nPDUs += 1;
    pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nr_configure_pdcch(pdcch_pdu_rel15, coreset, &ra->sched_pdcch);
    nr_mac->pdcch_pdu_idx[CC_id][coresetid] = pdcch_pdu_rel15;
  }

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void *)dl_tti_pdsch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2 + sizeof(nfapi_nr_dl_tti_pdsch_pdu));
  dl_req->nPDUs += 1;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

  pdsch_pdu_rel15->precodingAndBeamforming.num_prgs = 0;
  pdsch_pdu_rel15->precodingAndBeamforming.prg_size = 0;
  pdsch_pdu_rel15->precodingAndBeamforming.dig_bf_interfaces = 0;
  pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].pm_idx = 0;
  pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx = 0;

  LOG_A(NR_MAC,
        "UE %04x: %d.%d Generating RA-Msg2 DCI, RA RNTI 0x%x, state %d, CoreSetType %d, RAPID %d\n",
        ra->rnti,
        frameP,
        slotP,
        ra->RA_rnti,
        ra->ra_state,
        pdcch_pdu_rel15->CoreSetType,
        ra->preamble_index);

  // SCF222: PDU index incremented for each PDSCH PDU sent in TX control message. This is used to associate control
  // information to data and is reset every slot.
  const int pduindex = nr_mac->pdu_index[CC_id]++;
  uint8_t mcsTableIdx = dl_bwp->mcsTableIdx;

  NR_pdsch_dmrs_t dmrs_parms = get_dl_dmrs_params(scc, dl_bwp, &tda_info, 1);

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = ra->RA_rnti;
  pdsch_pdu_rel15->pduIndex = pduindex;
  pdsch_pdu_rel15->BWPSize = BWPSize;
  pdsch_pdu_rel15->BWPStart = BWPStart;
  pdsch_pdu_rel15->SubcarrierSpacing = dl_bwp->scs;
  pdsch_pdu_rel15->CyclicPrefix = 0;
  pdsch_pdu_rel15->NrOfCodewords = 1;
  pdsch_pdu_rel15->mcsTable[0] = mcsTableIdx;
  pdsch_pdu_rel15->rvIndex[0] = 0;
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0;
  pdsch_pdu_rel15->dmrsConfigType = dmrs_parms.dmrsConfigType;
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = dmrs_parms.numDmrsCdmGrpsNoData;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = rbStart;
  pdsch_pdu_rel15->rbSize = rbSize;
  pdsch_pdu_rel15->VRBtoPRBMapping = 0;
  pdsch_pdu_rel15->StartSymbolIndex = tda_info.startSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols = tda_info.nrOfSymbols;
  pdsch_pdu_rel15->dlDmrsSymbPos = dmrs_parms.dl_dmrs_symb_pos;

  uint8_t tb_scaling = 0;
  int R, Qm;
  uint32_t TBS = 0;

  while (TBS < 9) { // min TBS for RAR is 9 bytes
    mcsIndex++;
    R = nr_get_code_rate_dl(mcsIndex, mcsTableIdx);
    Qm = nr_get_Qm_dl(mcsIndex, mcsTableIdx);
    TBS = nr_compute_tbs(Qm,
                         R,
                         rbSize,
                         tda_info.nrOfSymbols,
                         dmrs_parms.N_PRB_DMRS * dmrs_parms.N_DMRS_SLOT,
                         0, // overhead
                         tb_scaling, // tb scaling
                         1)
          >> 3; // layers

    pdsch_pdu_rel15->targetCodeRate[0] = R;
    pdsch_pdu_rel15->qamModOrder[0] = Qm;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->TBSize[0] = TBS;
  }

  pdsch_pdu_rel15->maintenance_parms_v3.tbSizeLbrmBytes = nr_compute_tbslbrm(mcsTableIdx, sc_info->dl_bw_tbslbrm, 1);
  pdsch_pdu_rel15->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS << 3, R);

  // Fill PDCCH DL DCI PDU
  nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
  pdcch_pdu_rel15->numDlDci++;
  dci_pdu->RNTI = ra->RA_rnti;
  dci_pdu->ScramblingId = *scc->physCellId;
  dci_pdu->ScramblingRNTI = 0;
  dci_pdu->AggregationLevel = aggregation_level;
  dci_pdu->CceIndex = CCEIndex;
  dci_pdu->beta_PDCCH_1_0 = 0;
  dci_pdu->powerControlOffsetSS = 1;

  dci_pdu_rel15_t dci_payload;
  dci_payload.frequency_domain_assignment.val =
      PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize, pdsch_pdu_rel15->rbStart, BWPSize);

  LOG_D(NR_MAC, "Msg2 rbSize.rbStart.BWPsize %d.%d.%ld\n", pdsch_pdu_rel15->rbSize, pdsch_pdu_rel15->rbStart, BWPSize);

  dci_payload.time_domain_assignment.val = time_domain_assignment;
  dci_payload.vrb_to_prb_mapping.val = 0;
  dci_payload.mcs = pdsch_pdu_rel15->mcsIndex[0];
  dci_payload.tb_scaling = tb_scaling;

  LOG_D(NR_MAC,
        "DCI type 1 payload: freq_alloc %d (%d,%d,%ld), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d \n",
        dci_payload.frequency_domain_assignment.val,
        pdsch_pdu_rel15->rbStart,
        pdsch_pdu_rel15->rbSize,
        BWPSize,
        dci_payload.time_domain_assignment.val,
        dci_payload.vrb_to_prb_mapping.val,
        dci_payload.mcs,
        dci_payload.tb_scaling);

  LOG_D(NR_MAC,
        "DCI params: rnti 0x%x, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  "
        "n_symb %d\n",
        pdcch_pdu_rel15->dci_pdu[0].RNTI,
        TYPE_RA_RNTI_,
        NR_DL_DCI_FORMAT_1_0,
        *(unsigned long long *)pdcch_pdu_rel15->FreqDomainResource,
        pdcch_pdu_rel15->StartSymbolIndex,
        pdcch_pdu_rel15->DurationSymbols);

  fill_dci_pdu_rel15(sc_info,
                     dl_bwp,
                     &ra->UL_BWP,
                     &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci - 1],
                     &dci_payload,
                     NR_DL_DCI_FORMAT_1_0,
                     TYPE_RA_RNTI_,
                     dl_bwp->bwp_id,
                     ss,
                     coreset,
                     0, // parameter not needed for DCI 1_0
                     nr_mac->cset0_bwp_size);

  // DL TX request
  nfapi_nr_pdu_t *tx_req = &TX_req->pdu_list[TX_req->Number_of_PDUs];

  // Program UL processing for Msg3
  nr_get_Msg3alloc(module_idP, CC_id, scc, slotP, frameP, ra);
  nr_add_msg3(module_idP, CC_id, frameP, slotP, ra, (uint8_t *)&tx_req->TLVs[0].value.direct[0]);

  // Start RA contention resolution timer in Msg3 transmission slot (current slot + K2)
  // 3GPP TS 38.321 Section 5.1.5 Contention Resolution
  start_ra_contention_resolution_timer(
      ra,
      scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer,
      *ra->UL_BWP.tdaList_Common->list.array[ra->Msg3_tda_id]->k2 + get_NTN_Koffset(scc),
      ra->UL_BWP.scs);

  if (ra->cfra) {
    NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[module_idP]->UE_info, ra->rnti);
    if (UE) {
      const NR_ServingCellConfig_t *servingCellConfig = UE->CellGroup ? UE->CellGroup->spCellConfig->spCellConfigDedicated : NULL;
      uint32_t delay_ms = servingCellConfig && servingCellConfig->downlinkBWP_ToAddModList
                              ? NR_RRC_SETUP_DELAY_MS + NR_RRC_BWP_SWITCHING_DELAY_MS
                              : NR_RRC_SETUP_DELAY_MS;
      int delay = nr_mac_get_reconfig_delay_slots(ra->DL_BWP.scs, delay_ms);
      /* TODO check if the UE should apply configuration before sync */
      interrupt_followup_action_t action = UE->reconfigCellGroup ? FOLLOW_INSYNC_RECONFIG : FOLLOW_INSYNC;
      nr_mac_interrupt_ue_transmission(RC.nrmac[module_idP], UE, action, delay);
    }
  }

  LOG_D(NR_MAC,
        "UE %04x: %d.%d: Setting RA-Msg3 reception (%s) for SFN.Slot %d.%d\n",
        ra->rnti,
        frameP,
        slotP,
        ra->cfra ? "CFRA" : "CBRA",
        ra->Msg3_frame,
        ra->Msg3_slot);

  T(T_GNB_MAC_DL_RAR_PDU_WITH_DATA,
    T_INT(module_idP),
    T_INT(CC_id),
    T_INT(ra->RA_rnti),
    T_INT(frameP),
    T_INT(slotP),
    T_INT(0),
    T_BUFFER(&tx_req->TLVs[0].value.direct[0], tx_req->TLVs[0].length));

  tx_req->PDU_index = pduindex;
  tx_req->num_TLV = 1;
  tx_req->TLVs[0].length = pdsch_pdu_rel15->TBSize[0];
  tx_req->PDU_length = compute_PDU_length(tx_req->num_TLV, pdsch_pdu_rel15->TBSize[0]);
  TX_req->SFN = frameP;
  TX_req->Number_of_PDUs++;
  TX_req->Slot = slotP;

  // Mark the corresponding symbols RBs as used
  fill_pdcch_vrb_map(nr_mac, CC_id, &ra->sched_pdcch, CCEIndex, aggregation_level, beam.idx);
  for (int rb = 0; rb < rbSize; rb++) {
    vrb_map[BWPStart + rb + rbStart] |= SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols);
  }

  ra->ra_state = nrRA_WAIT_Msg3;
}

static void prepare_dl_pdus(gNB_MAC_INST *nr_mac,
                            NR_RA_t *ra,
                            NR_UE_DL_BWP_t *dl_bwp,
                            nfapi_nr_dl_tti_request_body_t *dl_req,
                            NR_sched_pucch_t *pucch,
                            NR_pdsch_dmrs_t dmrs_info,
                            NR_tda_info_t tda,
                            int aggregation_level,
                            int CCEIndex,
                            int tb_size,
                            int ndi,
                            int tpc,
                            int delta_PRI,
                            int current_harq_pid,
                            int time_domain_assignment,
                            int CC_id,
                            int rnti,
                            int round,
                            int mcsIndex,
                            int tb_scaling,
                            int pduindex,
                            int rbStart,
                            int rbSize)
{
  // look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not exist, create it. This is especially
  // important if we have multiple RAs, and the DLSCH has to reuse them, so we need to mark them
  NR_ControlResourceSet_t *coreset = ra->coreset;
  const int coresetid = coreset->controlResourceSetId;
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][coresetid];
  if (!pdcch_pdu_rel15) {
    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2 + sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    dl_req->nPDUs += 1;
    pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nr_configure_pdcch(pdcch_pdu_rel15, coreset, &ra->sched_pdcch);
    nr_mac->pdcch_pdu_idx[CC_id][coresetid] = pdcch_pdu_rel15;
  }

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));
  dl_req->nPDUs+=1;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  long BWPStart = 0;
  long BWPSize = 0;
  NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config = NULL;
  NR_SearchSpace_t *ss = ra->ra_ss;
  if(*ss->controlResourceSetId!=0) {
    BWPStart = dl_bwp->BWPStart;
    BWPSize  = dl_bwp->BWPSize;
  } else {
    type0_PDCCH_CSS_config = &nr_mac->type0_PDCCH_CSS_config[ra->beam_id];
    BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
    BWPSize = type0_PDCCH_CSS_config->num_rbs;
  }

  int mcsTableIdx = dl_bwp->mcsTableIdx;

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = rnti;
  pdsch_pdu_rel15->pduIndex = pduindex;
  pdsch_pdu_rel15->BWPSize  = BWPSize;
  pdsch_pdu_rel15->BWPStart = BWPStart;
  pdsch_pdu_rel15->SubcarrierSpacing = dl_bwp->scs;
  pdsch_pdu_rel15->CyclicPrefix = 0;
  pdsch_pdu_rel15->NrOfCodewords = 1;
  int R = nr_get_code_rate_dl(mcsIndex,mcsTableIdx);
  pdsch_pdu_rel15->targetCodeRate[0] = R;
  int Qm = nr_get_Qm_dl(mcsIndex, mcsTableIdx);
  pdsch_pdu_rel15->qamModOrder[0] = Qm;
  pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
  pdsch_pdu_rel15->mcsTable[0] = mcsTableIdx;
  pdsch_pdu_rel15->rvIndex[0] = nr_rv_round_map[round % 4];
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0;
  pdsch_pdu_rel15->dmrsConfigType = dmrs_info.dmrsConfigType;
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = dmrs_info.numDmrsCdmGrpsNoData;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = rbStart;
  pdsch_pdu_rel15->rbSize = rbSize;
  pdsch_pdu_rel15->VRBtoPRBMapping = 0;
  pdsch_pdu_rel15->StartSymbolIndex = tda.startSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols = tda.nrOfSymbols;
  pdsch_pdu_rel15->dlDmrsSymbPos = dmrs_info.dl_dmrs_symb_pos;

  int x_Overhead = 0;
  nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead, pdsch_pdu_rel15->numDmrsCdmGrpsNoData, tb_scaling);

  pdsch_pdu_rel15->maintenance_parms_v3.tbSizeLbrmBytes = nr_compute_tbslbrm(mcsTableIdx, ra->sc_info.dl_bw_tbslbrm, 1);
  pdsch_pdu_rel15->maintenance_parms_v3.ldpcBaseGraph = get_BG(tb_size<<3,R);

  pdsch_pdu_rel15->precodingAndBeamforming.num_prgs=1;
  pdsch_pdu_rel15->precodingAndBeamforming.prg_size=275;
  pdsch_pdu_rel15->precodingAndBeamforming.dig_bf_interfaces=1;
  pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].pm_idx = 0;
  pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx = ra->beam_id;

  /* Fill PDCCH DL DCI PDU */
  nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
  pdcch_pdu_rel15->numDlDci++;
  dci_pdu->RNTI = rnti;
  dci_pdu->ScramblingId = *scc->physCellId;
  dci_pdu->ScramblingRNTI = 0;
  dci_pdu->AggregationLevel = aggregation_level;
  dci_pdu->CceIndex = CCEIndex;
  dci_pdu->beta_PDCCH_1_0 = 0;
  dci_pdu->powerControlOffsetSS = 1;

  dci_pdu_rel15_t dci_payload;
  dci_payload.frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
                                                                                  pdsch_pdu_rel15->rbStart,
                                                                                  BWPSize);

  dci_payload.format_indicator = 1;
  dci_payload.time_domain_assignment.val = time_domain_assignment;
  dci_payload.vrb_to_prb_mapping.val = 0;
  dci_payload.mcs = pdsch_pdu_rel15->mcsIndex[0];
  dci_payload.tb_scaling = tb_scaling;
  dci_payload.rv = pdsch_pdu_rel15->rvIndex[0];
  dci_payload.harq_pid.val = current_harq_pid;
  dci_payload.ndi = ndi;
  dci_payload.dai[0].val = pucch ? (pucch->dai_c-1) & 3 : 0;
  dci_payload.tpc = tpc; // TPC for PUCCH: table 7.2.1-1 in 38.213
  dci_payload.pucch_resource_indicator = delta_PRI; // This is delta_PRI from 9.2.1 in 38.213
  dci_payload.pdsch_to_harq_feedback_timing_indicator.val = pucch ? pucch->timing_indicator : 0;

  LOG_D(NR_MAC,
        "DCI 1_0 payload: freq_alloc %d (%d,%d,%d), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d pucchres %d harqtiming %d\n",
        dci_payload.frequency_domain_assignment.val,
        pdsch_pdu_rel15->rbStart,
        pdsch_pdu_rel15->rbSize,
        pdsch_pdu_rel15->BWPSize,
        dci_payload.time_domain_assignment.val,
        dci_payload.vrb_to_prb_mapping.val,
        dci_payload.mcs,
        dci_payload.tb_scaling,
        dci_payload.pucch_resource_indicator,
        dci_payload.pdsch_to_harq_feedback_timing_indicator.val);

  LOG_D(NR_MAC,
        "DCI params: rnti 0x%x, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  "
        "n_symb %d, BWPsize %d\n",
        pdcch_pdu_rel15->dci_pdu[0].RNTI,
        TYPE_TC_RNTI_,
        NR_DL_DCI_FORMAT_1_0,
        (unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
        pdcch_pdu_rel15->StartSymbolIndex,
        pdcch_pdu_rel15->DurationSymbols,
        pdsch_pdu_rel15->BWPSize);

  fill_dci_pdu_rel15(&ra->sc_info,
                     dl_bwp,
                     &ra->UL_BWP,
                     &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci - 1],
                     &dci_payload,
                     NR_DL_DCI_FORMAT_1_0,
                     TYPE_TC_RNTI_,
                     dl_bwp->bwp_id,
                     ss,
                     coreset,
                     0, // parameter not needed for DCI 1_0
                     nr_mac->cset0_bwp_size);

  LOG_D(NR_MAC, "BWPSize: %i\n", pdcch_pdu_rel15->BWPSize);
  LOG_D(NR_MAC, "BWPStart: %i\n", pdcch_pdu_rel15->BWPStart);
  LOG_D(NR_MAC, "SubcarrierSpacing: %i\n", pdcch_pdu_rel15->SubcarrierSpacing);
  LOG_D(NR_MAC, "CyclicPrefix: %i\n", pdcch_pdu_rel15->CyclicPrefix);
  LOG_D(NR_MAC, "StartSymbolIndex: %i\n", pdcch_pdu_rel15->StartSymbolIndex);
  LOG_D(NR_MAC, "DurationSymbols: %i\n", pdcch_pdu_rel15->DurationSymbols);
  for (int n = 0; n < 6; n++)
    LOG_D(NR_MAC, "FreqDomainResource[%i]: %x\n", n, pdcch_pdu_rel15->FreqDomainResource[n]);
  LOG_D(NR_MAC, "CceRegMappingType: %i\n", pdcch_pdu_rel15->CceRegMappingType);
  LOG_D(NR_MAC, "RegBundleSize: %i\n", pdcch_pdu_rel15->RegBundleSize);
  LOG_D(NR_MAC, "InterleaverSize: %i\n", pdcch_pdu_rel15->InterleaverSize);
  LOG_D(NR_MAC, "CoreSetType: %i\n", pdcch_pdu_rel15->CoreSetType);
  LOG_D(NR_MAC, "ShiftIndex: %i\n", pdcch_pdu_rel15->ShiftIndex);
  LOG_D(NR_MAC, "precoderGranularity: %i\n", pdcch_pdu_rel15->precoderGranularity);
  LOG_D(NR_MAC, "numDlDci: %i\n", pdcch_pdu_rel15->numDlDci);
}

static void nr_generate_Msg4(module_id_t module_idP,
                             int CC_id,
                             frame_t frameP,
                             sub_frame_t slotP,
                             NR_RA_t *ra,
                             nfapi_nr_dl_tti_request_t *DL_req,
                             nfapi_nr_tx_data_request_t *TX_req)
{
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_UE_DL_BWP_t *dl_bwp = &ra->DL_BWP;

  // if it is a DL slot, if the RA is in MSG4 state
  if (is_xlsch_in_slot(nr_mac->dlsch_slot_bitmap[slotP / 64], slotP)) {

    NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
    NR_SearchSpace_t *ss = ra->ra_ss;

    NR_ControlResourceSet_t *coreset = ra->coreset;
    AssertFatal(coreset!=NULL,"Coreset cannot be null for RA-Msg4\n");

    uint16_t mac_sdu_length = 0;

    NR_UE_info_t *UE = find_nr_UE(&nr_mac->UE_info, ra->rnti);
    if (!UE) {
      LOG_E(NR_MAC, "want to generate Msg4, but rnti %04x not in the table\n", ra->rnti);
      return;
    }

    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    /* get the PID of a HARQ process awaiting retrnasmission, or -1 otherwise */
    int current_harq_pid = sched_ctrl->retrans_dl_harq.head;

    logical_chan_id_t lcid = DL_SCH_LCID_CCCH;
    if (current_harq_pid < 0) {
      // Check for data on SRB0 (RRCSetup)
      mac_rlc_status_resp_t srb_status = mac_rlc_status_ind(module_idP, ra->rnti, module_idP, frameP, slotP, ENB_FLAG_YES, MBMS_FLAG_NO, lcid, 0, 0);

      if (srb_status.bytes_in_buffer == 0) {
        lcid = DL_SCH_LCID_DCCH;
        // Check for data on SRB1 (RRCReestablishment, RRCReconfiguration)
        srb_status = mac_rlc_status_ind(module_idP, ra->rnti, module_idP, frameP, slotP, ENB_FLAG_YES, MBMS_FLAG_NO, lcid, 0, 0);
      }

      // Need to wait until data for Msg4 is ready
      if (srb_status.bytes_in_buffer == 0)
        return;
      mac_sdu_length = srb_status.bytes_in_buffer;
    }

    const int n_slots_frame = nr_slots_per_frame[dl_bwp->scs];
    NR_beam_alloc_t beam = beam_allocation_procedure(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame);
    if (beam.idx < 0)
      return;

    long BWPStart = 0;
    long BWPSize = 0;
    NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config = NULL;
    if(*ss->controlResourceSetId!=0) {
      BWPStart = dl_bwp->BWPStart;
      BWPSize  = dl_bwp->BWPSize;
    } else {
      type0_PDCCH_CSS_config = &nr_mac->type0_PDCCH_CSS_config[ra->beam_id];
      BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
      BWPSize = type0_PDCCH_CSS_config->num_rbs;
    }

    // get CCEindex, needed also for PUCCH and then later for PDCCH
    uint8_t aggregation_level;
    int CCEIndex = get_cce_index(nr_mac,
                                 CC_id, slotP, 0,
                                 &aggregation_level,
                                 beam.idx,
                                 ss,
                                 coreset,
                                 &ra->sched_pdcch,
                                 true);

    if (CCEIndex < 0) {
      LOG_E(NR_MAC, "Cannot find free CCE for RA RNTI 0x%04x!\n", ra->rnti);
      reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
      return;
    }

    // Checking if the DCI allocation is feasible in current subframe
    nfapi_nr_dl_tti_request_body_t *dl_req = &DL_req->dl_tti_request_body;
    if (dl_req->nPDUs > NFAPI_NR_MAX_DL_TTI_PDUS - 2) {
      LOG_I(NR_MAC, "UE %04x: %d.%d FAPI DL structure is full\n", ra->rnti, frameP, slotP);
      reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
      return;
    }

    uint8_t time_domain_assignment = get_dl_tda(nr_mac, scc, slotP);
    int mux_pattern = type0_PDCCH_CSS_config ? type0_PDCCH_CSS_config->type0_pdcch_ss_mux_pattern : 1;
    NR_tda_info_t msg4_tda = get_dl_tda_info(dl_bwp,
                                             ss->searchSpaceType->present,
                                             time_domain_assignment,
                                             scc->dmrs_TypeA_Position,
                                             mux_pattern,
                                             TYPE_TC_RNTI_,
                                             coreset->controlResourceSetId,
                                             false);
    if (!msg4_tda.valid_tda)
      return;

    NR_pdsch_dmrs_t dmrs_info = get_dl_dmrs_params(scc, dl_bwp, &msg4_tda, 1);

    uint8_t mcsTableIdx = dl_bwp->mcsTableIdx;
    uint8_t mcsIndex = 0;
    int rbStart = 0;
    int rbSize = 0;
    uint8_t tb_scaling = 0;
    uint32_t tb_size = 0;
    uint16_t pdu_length;
    if(current_harq_pid >= 0) { // in case of retransmission
      NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];
      DevAssert(!harq->is_waiting);
      pdu_length = harq->tb_size;
    }
    else {
      uint8_t subheader_len = (mac_sdu_length < 256) ? sizeof(NR_MAC_SUBHEADER_SHORT) : sizeof(NR_MAC_SUBHEADER_LONG);
      pdu_length = mac_sdu_length + subheader_len + 7; //7 is contetion resolution length
    }

    // increase PRBs until we get to BWPSize or TBS is bigger than MAC PDU size
    do {
      if(rbSize < BWPSize)
        rbSize++;
      else
        mcsIndex++;
      LOG_D(NR_MAC,"Calling nr_compute_tbs with N_PRB_DMRS %d, N_DMRS_SLOT %d\n",dmrs_info.N_PRB_DMRS,dmrs_info.N_DMRS_SLOT);
      tb_size = nr_compute_tbs(nr_get_Qm_dl(mcsIndex, mcsTableIdx),
                                     nr_get_code_rate_dl(mcsIndex, mcsTableIdx),
                                     rbSize, msg4_tda.nrOfSymbols, dmrs_info.N_PRB_DMRS * dmrs_info.N_DMRS_SLOT, 0, tb_scaling,1) >> 3;
    } while (tb_size < pdu_length && mcsIndex<=28);

    AssertFatal(tb_size >= pdu_length,"Cannot allocate Msg4\n");

    int i = 0;
    uint16_t *vrb_map = cc[CC_id].vrb_map[beam.idx];
    while ((i < rbSize) && (rbStart + rbSize <= BWPSize)) {
      if (vrb_map[BWPStart + rbStart + i]&SL_to_bitmap(msg4_tda.startSymbolIndex, msg4_tda.nrOfSymbols)) {
        rbStart += i+1;
        i = 0;
      } else {
        i++;
      }
    }

    if (rbStart > (BWPSize - rbSize)) {
      LOG_E(NR_MAC, "Cannot find free vrb_map for RNTI %04x!\n", ra->rnti);
      reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
      return;
    }

    // HARQ management
    if (current_harq_pid < 0) {
      AssertFatal(sched_ctrl->available_dl_harq.head >= 0,
                  "UE context not initialized: no HARQ processes found\n");
      current_harq_pid = sched_ctrl->available_dl_harq.head;
      remove_front_nr_list(&sched_ctrl->available_dl_harq);
    }

    const int delta_PRI = 0;

    int alloc = -1;
    if (!get_FeedbackDisabled(UE->sc_info.downlinkHARQ_FeedbackDisabled_r17, current_harq_pid)) {
      int r_pucch = nr_get_pucch_resource(coreset, ra->UL_BWP.pucch_Config, CCEIndex);

      LOG_D(NR_MAC, "Msg4 r_pucch %d (CCEIndex %d, delta_PRI %d)\n", r_pucch, CCEIndex, delta_PRI);

      alloc = nr_acknack_scheduling(nr_mac, UE, frameP, slotP, ra->beam_id, r_pucch, 1);
      if (alloc < 0) {
        LOG_D(NR_MAC,"Couldn't find a pucch allocation for ack nack (msg4) in frame %d slot %d\n", frameP, slotP);
        reset_beam_status(&nr_mac->beam_info, frameP, slotP, ra->beam_id, n_slots_frame, beam.new_beam);
        return;
      }
    }

    NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];
    NR_sched_pucch_t *pucch = NULL;
    DevAssert(!harq->is_waiting);
    if (alloc < 0) {
      finish_nr_dl_harq(sched_ctrl, current_harq_pid);
    } else {
      pucch = &sched_ctrl->sched_pucch[alloc];
      add_tail_nr_list(&sched_ctrl->feedback_dl_harq, current_harq_pid);
      harq->feedback_slot = pucch->ul_slot;
      harq->feedback_frame = pucch->frame;
      harq->is_waiting = true;
    }
    ra->harq_pid = current_harq_pid;
    UE->mac_stats.dl.rounds[harq->round]++;

    harq->tb_size = tb_size;

    uint8_t *buf = (uint8_t *) harq->transportBlock;
    // Bytes to be transmitted
    if (harq->round == 0) {
      // UE Contention Resolution Identity MAC CE
      uint16_t mac_pdu_length = nr_write_ce_dlsch_pdu(module_idP, nr_mac->sched_ctrlCommon, buf, 255, ra->cont_res_id);
      LOG_D(NR_MAC,"Encoded contention resolution mac_pdu_length %d\n",mac_pdu_length);
      uint8_t buffer[CCCH_SDU_SIZE];
      uint8_t mac_subheader_len = sizeof(NR_MAC_SUBHEADER_SHORT);
      // Get RLC data on the SRB (RRCSetup, RRCReestablishment)
      mac_sdu_length = mac_rlc_data_req(module_idP,
                                        ra->rnti,
                                        module_idP,
                                        frameP,
                                        ENB_FLAG_YES,
                                        MBMS_FLAG_NO,
                                        lcid,
                                        CCCH_SDU_SIZE,
                                        (char *)buffer,
                                        0,
                                        0);

      if (mac_sdu_length < 256) {
        ((NR_MAC_SUBHEADER_SHORT *)&buf[mac_pdu_length])->R = 0;
        ((NR_MAC_SUBHEADER_SHORT *)&buf[mac_pdu_length])->F = 0;
        ((NR_MAC_SUBHEADER_SHORT *)&buf[mac_pdu_length])->LCID = lcid;
        ((NR_MAC_SUBHEADER_SHORT *)&buf[mac_pdu_length])->L = mac_sdu_length;
        ra->mac_pdu_length = mac_pdu_length + mac_sdu_length + sizeof(NR_MAC_SUBHEADER_SHORT);
      } else {
        mac_subheader_len = sizeof(NR_MAC_SUBHEADER_LONG);
        ((NR_MAC_SUBHEADER_LONG *)&buf[mac_pdu_length])->R = 0;
        ((NR_MAC_SUBHEADER_LONG *)&buf[mac_pdu_length])->F = 1;
        ((NR_MAC_SUBHEADER_LONG *)&buf[mac_pdu_length])->LCID = lcid;
        ((NR_MAC_SUBHEADER_LONG *)&buf[mac_pdu_length])->L = htons(mac_sdu_length);
        ra->mac_pdu_length = mac_pdu_length + mac_sdu_length + sizeof(NR_MAC_SUBHEADER_LONG);
      }
      memcpy(&buf[mac_pdu_length + mac_subheader_len], buffer, mac_sdu_length);
    }

    const int pduindex = nr_mac->pdu_index[CC_id]++;
    prepare_dl_pdus(nr_mac, ra, dl_bwp, dl_req, pucch, dmrs_info, msg4_tda, aggregation_level, CCEIndex, tb_size, harq->ndi, sched_ctrl->tpc1, delta_PRI,
                    current_harq_pid, time_domain_assignment, CC_id, ra->rnti, harq->round, mcsIndex, tb_scaling, pduindex, rbStart, rbSize);

    // Add padding header and zero rest out if there is space left
    if (ra->mac_pdu_length < harq->tb_size) {
      NR_MAC_SUBHEADER_FIXED *padding = (NR_MAC_SUBHEADER_FIXED *) &buf[ra->mac_pdu_length];
      padding->R = 0;
      padding->LCID = DL_SCH_LCID_PADDING;
      for(int k = ra->mac_pdu_length+1; k<harq->tb_size; k++) {
        buf[k] = 0;
      }
    }

    T(T_GNB_MAC_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(ra->rnti),
      T_INT(frameP), T_INT(slotP), T_INT(current_harq_pid), T_BUFFER(harq->transportBlock, harq->tb_size));

    // DL TX request
    nfapi_nr_pdu_t *tx_req = &TX_req->pdu_list[TX_req->Number_of_PDUs];
    memcpy(tx_req->TLVs[0].value.direct, harq->transportBlock, sizeof(uint8_t) * harq->tb_size);
    tx_req->PDU_index = pduindex;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length =  harq->tb_size;
    tx_req->PDU_length = compute_PDU_length(tx_req->num_TLV, tx_req->TLVs[0].length);
    TX_req->SFN = frameP;
    TX_req->Number_of_PDUs++;
    TX_req->Slot = slotP;

    // Mark the corresponding symbols and RBs as used
    fill_pdcch_vrb_map(nr_mac,
                       CC_id,
                       &ra->sched_pdcch,
                       CCEIndex,
                       aggregation_level,
                       beam.idx);
    for (int rb = 0; rb < rbSize; rb++) {
      vrb_map[BWPStart + rb + rbStart] |= SL_to_bitmap(msg4_tda.startSymbolIndex, msg4_tda.nrOfSymbols);
    }

    if (pucch == NULL) {
      LOG_A(NR_MAC, "(UE RNTI 0x%04x) Skipping Ack of RA-Msg4. CBRA procedure succeeded!\n", ra->rnti);
      UE->Msg4_ACKed = true;

      // Pause scheduling according to:
      // 3GPP TS 38.331 Section 12 Table 12.1-1: UE performance requirements for RRC procedures for UEs
      // Msg4 may transmit a RRCReconfiguration, for example when UE sends RRCReestablishmentComplete and MAC CE for C-RNTI in Msg3.
      // In that case, gNB will generate a RRCReconfiguration that will be transmitted in Msg4, so we need to apply CellGroup after the Ack,
      // UE->reconfigCellGroup was already set when processing RRCReestablishment message
      int delay = nr_mac_get_reconfig_delay_slots(UE->current_UL_BWP.scs, NR_RRC_RECONFIGURATION_DELAY_MS);
      nr_mac_interrupt_ue_transmission(nr_mac, UE, UE->interrupt_action, delay);

      nr_clear_ra_proc(ra);
    } else {
      ra->ra_state = nrRA_WAIT_Msg4_ACK;
      LOG_I(NR_MAC,"UE %04x Generate msg4: feedback at %4d.%2d, payload %d bytes, next state WAIT_Msg4_ACK\n", ra->rnti, pucch->frame, pucch->ul_slot, harq->tb_size);
    }
  }
}

static void nr_check_Msg4_Ack(module_id_t module_id, int CC_id, frame_t frame, sub_frame_t slot, NR_RA_t *ra)
{
  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[module_id]->UE_info, ra->rnti);
  if (!UE) {
    LOG_E(NR_MAC, "Cannot check Msg4 ACK/NACK, rnti %04x not in the table\n", ra->rnti);
    return;
  }
  const int current_harq_pid = ra->harq_pid;

  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];

  LOG_D(NR_MAC, "ue rnti 0x%04x, harq is waiting %d, round %d, frame %d %d, harq id %d\n", ra->rnti, harq->is_waiting, harq->round, frame, slot, current_harq_pid);

  if (harq->is_waiting == 0) {
    if (harq->round == 0) {
      if (UE->Msg4_ACKed) {
        LOG_A(NR_MAC, "(UE RNTI 0x%04x) Received Ack of RA-Msg4. CBRA procedure succeeded!\n", ra->rnti);
      } else {
        LOG_I(NR_MAC, "%4d.%2d UE %04x: RA Procedure failed at Msg4!\n", frame, slot, ra->rnti);
        nr_mac_trigger_ul_failure(sched_ctrl, UE->current_DL_BWP.scs);
      }

      // Pause scheduling according to:
      // 3GPP TS 38.331 Section 12 Table 12.1-1: UE performance requirements for RRC procedures for UEs
      // Msg4 may transmit a RRCReconfiguration, for example when UE sends RRCReestablishmentComplete and MAC CE for C-RNTI in Msg3.
      // In that case, gNB will generate a RRCReconfiguration that will be transmitted in Msg4, so we need to apply CellGroup after the Ack,
      // UE->reconfigCellGroup already set when processing RRCReestablishment message
      int delay = nr_mac_get_reconfig_delay_slots(UE->current_UL_BWP.scs, NR_RRC_RECONFIGURATION_DELAY_MS);
      nr_mac_interrupt_ue_transmission(RC.nrmac[module_id], UE, UE->interrupt_action, delay);

      nr_clear_ra_proc(ra);
      if (sched_ctrl->retrans_dl_harq.head >= 0) {
        remove_nr_list(&sched_ctrl->retrans_dl_harq, current_harq_pid);
      }
    } else {
      LOG_I(NR_MAC, "(UE %04x) Received Nack of RA-Msg4. Preparing retransmission!\n", ra->rnti);
      ra->ra_state = nrRA_Msg4;
    }
  }
}

void nr_clear_ra_proc(NR_RA_t *ra)
{
  /* we assume that this function is mutex-protected from outside */
  NR_SCHED_ENSURE_LOCKED(&RC.nrmac[0]->sched_lock);
  memset(ra, 0, sizeof(*ra));
  ra->ra_state = nrRA_gNB_IDLE;
  if (get_softmodem_params()->sa) { // in SA, prefill with allowed preambles
    ra->preambles.num_preambles = MAX_NUM_NR_PRACH_PREAMBLES;
    for (int i = 0; i < MAX_NUM_NR_PRACH_PREAMBLES; i++)
      ra->preambles.preamble_list[i] = i;
  }
}


/////////////////////////////////////
//    Random Access Response PDU   //
//         TS 38.213 ch 8.2        //
//        TS 38.321 ch 6.2.3       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//| E | T |       R A P I D       |//
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |//
//| R |           T A             |//
//|       T A         |  UL grant |//
//|            UL grant           |//
//|            UL grant           |//
//|            UL grant           |//
//|         T C - R N T I         |//
//|         T C - R N T I         |//
/////////////////////////////////////
//       UL grant  (27 bits)       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//|-------------------|FHF|F_alloc|//
//|        Freq allocation        |//
//|    F_alloc    |Time allocation|//
//|      MCS      |     TPC   |CSI|//
/////////////////////////////////////
// WIP
// todo:
// - handle MAC RAR BI subheader
// - sending only 1 RAR subPDU
// - UL Grant: hardcoded CSI, TPC, time alloc
// - padding
static void nr_fill_rar(uint8_t Mod_idP, NR_RA_t *ra, uint8_t *dlsch_buffer, nfapi_nr_pusch_pdu_t *pusch_pdu)
{
  LOG_D(NR_MAC, "[gNB] Generate RAR MAC PDU frame %d slot %d preamble index %u TA command %d \n", ra->Msg2_frame, ra-> Msg2_slot, ra->preamble_index, ra->timing_offset);
  NR_RA_HEADER_BI *rarbi = (NR_RA_HEADER_BI *) dlsch_buffer;
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) (dlsch_buffer + 1);
  NR_MAC_RAR *rar = (NR_MAC_RAR *) (dlsch_buffer + 2);
  unsigned char csi_req = 0, tpc_command;

  tpc_command = 3; // this is 0 dB

  /// E/T/R/R/BI subheader ///
  // E = 1, MAC PDU includes another MAC sub-PDU (RAPID)
  // T = 0, Back-off indicator subheader
  // R = 2, Reserved
  // BI = 0, 5ms
  rarbi->E = 1;
  rarbi->T = 0;
  rarbi->R = 0;
  rarbi->BI = 0;

  /// E/T/RAPID subheader ///
  // E = 0, one only RAR, first and last
  // T = 1, RAPID
  rarh->E = 0;
  rarh->T = 1;
  rarh->RAPID = ra->preamble_index;

  /// RAR MAC payload ///
  rar->R = 0;

  // TA command
  rar->TA1 = (uint8_t) (ra->timing_offset >> 5);    // 7 MSBs of timing advance
  rar->TA2 = (uint8_t) (ra->timing_offset & 0x1f);  // 5 LSBs of timing advance

  // TC-RNTI
  rar->TCRNTI_1 = (uint8_t) (ra->rnti >> 8);        // 8 MSBs of rnti
  rar->TCRNTI_2 = (uint8_t) (ra->rnti & 0xff);      // 8 LSBs of rnti

  // UL grant

  ra->msg3_TPC = tpc_command;

  if (pusch_pdu->frequency_hopping)
    AssertFatal(1==0,"PUSCH with frequency hopping currently not supported");

  int bwp_size = pusch_pdu->bwp_size;
  int prb_alloc = PRBalloc_to_locationandbandwidth0(ra->msg3_nb_rb, ra->msg3_first_rb, bwp_size);
  int valid_bits = 14;
  int f_alloc = prb_alloc & ((1 << valid_bits) - 1);

  uint32_t ul_grant = csi_req | (tpc_command << 1) | (pusch_pdu->mcs_index << 4) | (ra->Msg3_tda_id << 8) | (f_alloc << 12) | (pusch_pdu->frequency_hopping << 26);

  rar->UL_GRANT_1 = (uint8_t) (ul_grant >> 24) & 0x07;
  rar->UL_GRANT_2 = (uint8_t) (ul_grant >> 16) & 0xff;
  rar->UL_GRANT_3 = (uint8_t) (ul_grant >> 8) & 0xff;
  rar->UL_GRANT_4 = (uint8_t) ul_grant & 0xff;

#ifdef DEBUG_RAR
  LOG_I(NR_MAC, "rarh->E = 0x%x\n", rarh->E);
  LOG_I(NR_MAC, "rarh->T = 0x%x\n", rarh->T);
  LOG_I(NR_MAC, "rarh->RAPID = 0x%x (%i)\n", rarh->RAPID, rarh->RAPID);
  LOG_I(NR_MAC, "rar->R = 0x%x\n", rar->R);
  LOG_I(NR_MAC, "rar->TA1 = 0x%x\n", rar->TA1);
  LOG_I(NR_MAC, "rar->TA2 = 0x%x\n", rar->TA2);
  LOG_I(NR_MAC, "rar->UL_GRANT_1 = 0x%x\n", rar->UL_GRANT_1);
  LOG_I(NR_MAC, "rar->UL_GRANT_2 = 0x%x\n", rar->UL_GRANT_2);
  LOG_I(NR_MAC, "rar->UL_GRANT_3 = 0x%x\n", rar->UL_GRANT_3);
  LOG_I(NR_MAC, "rar->UL_GRANT_4 = 0x%x\n", rar->UL_GRANT_4);
  LOG_I(NR_MAC, "rar->TCRNTI_1 = 0x%x\n", rar->TCRNTI_1);
  LOG_I(NR_MAC, "rar->TCRNTI_2 = 0x%x\n", rar->TCRNTI_2);
#endif
  LOG_D(NR_MAC,
        "In %s: Transmitted RAR with t_alloc %d f_alloc %d ta_command %d mcs %d freq_hopping %d tpc_command %d csi_req %d t_crnti "
        "%x \n",
        __FUNCTION__,
        rar->UL_GRANT_3 & 0x0f,
        (rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12),
        rar->TA2 + (rar->TA1 << 5),
        rar->UL_GRANT_4 >> 4,
        rar->UL_GRANT_1 >> 2,
        tpc_command,
        csi_req,
        rar->TCRNTI_2 + (rar->TCRNTI_1 << 8));
}

void nr_schedule_RA(module_id_t module_idP,
                    frame_t frameP,
                    sub_frame_t slotP,
                    nfapi_nr_ul_dci_request_t *ul_dci_req,
                    nfapi_nr_dl_tti_request_t *DL_req,
                    nfapi_nr_tx_data_request_t *TX_req)
{
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  /* already mutex protected: held in gNB_dlsch_ulsch_scheduler() */
  NR_SCHED_ENSURE_LOCKED(&mac->sched_lock);

  start_meas(&mac->schedule_ra);
  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
    for (int i = 0; i < NR_NB_RA_PROC_MAX; i++) {
      NR_RA_t *ra = &cc->ra[i];
      LOG_D(NR_MAC, "RA[state:%d]\n", ra->ra_state);

      // Check RA Contention Resolution timer
      if (ra->ra_state >= nrRA_WAIT_Msg3) {
        ra->contention_resolution_timer--;
        if (ra->contention_resolution_timer < 0) {
          LOG_W(NR_MAC, "(%d.%d) RA Contention Resolution timer expired for UE 0x%04x, RA procedure failed...\n", frameP, slotP, ra->rnti);
          nr_mac_release_ue(mac, ra->rnti);
          nr_mac_trigger_release_complete(mac, ra->rnti);
          nr_clear_ra_proc(ra);
          continue;
        }
      }

      switch (ra->ra_state) {
        case nrRA_Msg2:
          nr_generate_Msg2(module_idP, CC_id, frameP, slotP, ra, DL_req, TX_req);
          break;
        case nrRA_Msg3_retransmission:
          nr_generate_Msg3_retransmission(module_idP, CC_id, frameP, slotP, ra, ul_dci_req);
          break;
        case nrRA_Msg4:
          nr_generate_Msg4(module_idP, CC_id, frameP, slotP, ra, DL_req, TX_req);
          break;
        case nrRA_WAIT_Msg4_ACK:
          nr_check_Msg4_Ack(module_idP, CC_id, frameP, slotP, ra);
          break;
        default:
          break;
      }
    }
  }
  stop_meas(&mac->schedule_ra);
}
