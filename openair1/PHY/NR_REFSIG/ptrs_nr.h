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

/**********************************************************************
*
* FILENAME    :  dmrs.h
*
* MODULE      :  demodulation reference signals
*
* DESCRIPTION :  generation of dmrs sequences for NR 5G
*                3GPP TS 38.211
*
************************************************************************/

#ifndef PTRS_NR_H
#define PTRS_NR_H

#include "PHY/defs_nr_UE.h"

/************** CODE GENERATION ***********************************/

/************** DEFINE ********************************************/


/************* STRUCTURES *****************************************/


/************** VARIABLES *****************************************/

/************** FUNCTION ******************************************/

int16_t get_kRE_ref(uint8_t dmrs_antenna_port, uint8_t pusch_dmrs_type, uint8_t resourceElementOffset);

uint8_t get_K_ptrs(ptrs_UplinkConfig_t *ptrs_UplinkConfig, uint16_t N_RB);

void set_ptrs_symb_idx(uint16_t *ptrs_symbols,
                       uint8_t duration_in_symbols,
                       uint8_t start_symbol,
                       uint8_t dmrs_type,
                       uint8_t L_ptrs,
                       uint8_t pusch_maxLength,
                       uint16_t ofdm_symbol_size);

uint8_t get_L_ptrs(ptrs_UplinkConfig_t *ptrs_UplinkConfig, uint8_t I_mcs);

uint8_t is_ptrs_subcarrier(uint16_t k, uint8_t K_ptrs, uint16_t n_rnti, uint16_t N_RB, int16_t k_RE_ref, uint16_t start_sc, uint16_t ofdm_symbol_size);

uint8_t is_ptrs_symbol(uint8_t l,
                       uint16_t k,
                       uint16_t n_rnti,
                       uint16_t N_RB,
                       uint8_t duration_in_symbols,
                       uint8_t dmrs_antenna_port,
                       uint8_t K_ptrs,
                       uint16_t ptrs_symbols,
                       uint16_t start_sc,
                       uint16_t ofdm_symbol_size,
                       pusch_dmrs_type_t pusch_dmrs_type,
                       uint8_t resourceElementOffset);



#endif /* PTRS_NR_H */
