/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/NR_TRANSPORT/nr_ulsch.h
* \brief functions used for PUSCH/ULSCH physical and transport channels for gNB
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/

#include "PHY/defs_gNB.h"

void free_gNB_ulsch(NR_gNB_ULSCH_t *ulsch);

NR_gNB_ULSCH_t *new_gNB_ulsch(uint8_t max_ldpc_iterations,uint8_t N_RB_UL, uint8_t abstraction_flag);

uint32_t nr_ulsch_decoding(PHY_VARS_gNB *phy_vars_gNB,
                           uint8_t UE_id,
                           short *ulsch_llr,
                           NR_DL_FRAME_PARMS *frame_parms,
                           uint32_t frame,
                           uint16_t nb_symb_sch,
                           uint8_t nr_tti_rx,
                           uint8_t harq_pid,
                           uint8_t is_crnti,
                           uint8_t llr8_flag);

