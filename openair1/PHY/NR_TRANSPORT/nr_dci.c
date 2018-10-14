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

/*! \file PHY/NR_TRANSPORT/nr_dci.c
* \brief Implements DCI encoding/decoding and PDCCH TX/RX procedures (38.212/38.213/38.214). V15.2.0 2018-06.
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "nr_dci.h"

//#define DEBUG_PDCCH_DMRS
//#define DEBUG_DCI
//#define DEBUG_CHANNEL_CODING
#define PDCCH_TEST_POLAR_TEMP_FIX

extern short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT];

uint16_t nr_get_dci_size(nfapi_nr_dci_format_e format,
                        nfapi_nr_rnti_type_e rnti_type,
                        NR_BWP_PARMS* bwp,
                        nfapi_nr_config_request_t* config)
{
  uint16_t size = 0;
  uint16_t N_RB = bwp->N_RB;

  switch(format) {
/*Only sizes for 0_0 and 1_0 are correct at the moment*/
    case NFAPI_NR_UL_DCI_FORMAT_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 Time Domain assgnmt 4 --20
      size += 20;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment -- hopping scenario to be updated
      size += nr_get_dci_size(NFAPI_NR_DL_DCI_FORMAT_1_0, rnti_type, bwp, config) - size; // Padding to match 1_0 size
      // UL/SUL indicator assumed to be 0
      break;

    case NFAPI_NR_UL_DCI_FORMAT_0_1:
      /// fixed: Format identifier 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2, SRS request 2 --17
      size += 17;
      // Carrier indicator
      // UL/SUL indicator
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // Frequency Hopping flag
      // 1st DAI
      // 2nd DAI
      // SRS resource indicator
      // Precoding info and number of layers
      // Antenna ports
      // CSI request
      // CBGTI
      // PTRS - DMRS association
      // beta offset indicator
      // DMRS sequence init
      break;

    case NFAPI_NR_DL_DCI_FORMAT_1_0:
      /// fixed: Format identifier 1, VRB2PRB 1, MCS 5, NDI 1, RV 2, HARQ PID 4, DAI 2, PUCCH TPC 2, PUCCH RInd 3, PDSCH to HARQ TInd 3 Time Domain assgnmt 4 -- 28
      size += 28;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment

      break;

    case NFAPI_NR_DL_DCI_FORMAT_1_1:
      // Carrier indicator
      size += 1; // Format identifier
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // PRB bundling size indicator
      // Rate matching indicator
      // ZP CSI-RS trigger
        /// TB1- MCS 5, NDI 1, RV 2
      size += 8;
      // TB2
      size += 4 ;  // HARQ PID
      // DAI
      size += 2; // TPC PUCCH
      size += 3; // PUCCH resource indicator
      size += 3; // PDSCH to HARQ timing indicator
      // Antenna ports
      // Tx Config Indication
      size += 2; // SRS request
      // CBGTI
      // CBGFI
      size += 1; // DMRS sequence init
      
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_0:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_1:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_2:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_3:
      break;

  default:
    AssertFatal(1==0, "Invalid NR DCI format %d\n", format);
  }

  return size;
}

void nr_pdcch_scrambling(uint32_t *in,
                         uint8_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<16) + Nid;

  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    *out ^= (((*in)>>i)&1) ^ ((s>>i)&1);
  }

}

uint8_t nr_generate_dci_top(NR_gNB_PDCCH pdcch_vars,
							t_nrPolar_paramsPtr *nrPolar_params,
                            uint32_t **gold_pdcch_dmrs,
                            int32_t** txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS frame_parms,
                            nfapi_nr_config_request_t config)
{

  int16_t mod_dmrs[NR_MAX_CSET_DURATION][NR_MAX_PDCCH_DMRS_LENGTH>>1]; // 3 for the max coreset duration
  uint8_t idx=0;
  uint16_t a;
  int k,l,k_prime,dci_idx, dmrs_idx;
  nr_cce_t cce;
  nr_reg_t reg;
  nr_reg_t reg_mapping_list[NR_MAX_PDCCH_AGG_LEVEL*NR_NB_REG_PER_CCE];

  /*First iteration: single DCI*/
  NR_gNB_DCI_ALLOC_t dci_alloc = pdcch_vars.dci_alloc[0];
  nfapi_nr_dl_config_pdcch_parameters_rel15_t pdcch_params = dci_alloc.pdcch_params;
  uint16_t dmrs_length = dci_alloc.L*36; //2(QPSK)*3(per RB)*6(REG per CCE)
  uint16_t encoded_length = dci_alloc.L*108; //2(QPSK)*9(per RB)*6(REG per CCE)

  /*The coreset is initialised
  * in frequency: the first subcarrier is obtained by adding the first CRB overlapping the SSB and the rb_offset
  * in time: by its first slot and its first symbol*/
  uint16_t cset_start_sc = frame_parms.first_carrier_offset + ((int)floor(frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB)+pdcch_params.rb_offset)*NR_NB_SC_PER_RB;
  uint8_t cset_start_symb = pdcch_params.first_slot*frame_parms.symbols_per_slot + pdcch_params.first_symbol;
  uint8_t cset_nsymb = pdcch_params.n_symb;
  dci_idx = 0;

  /// DMRS QPSK modulation
    /*There is a need to shift from which index the pregenerated DMRS sequence is used
     * see 38211 r15.2.0 section 7.4.1.3.2: assumption is the reference point for k refers to the DMRS sequence*/
  if (pdcch_params.config_type == NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG)
    gold_pdcch_dmrs += ((int)floor(frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB)+pdcch_params.rb_offset)*3/32;

  for (int symb=cset_start_symb; symb<cset_start_symb + pdcch_params.n_symb; symb++) {
    for (int i=0; i<dmrs_length>>1; i++) {
      idx = ((((gold_pdcch_dmrs[symb][(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((gold_pdcch_dmrs[symb][((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
      mod_dmrs[symb][i<<1] = nr_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
      mod_dmrs[symb][(i<<1)+1] = nr_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
  #ifdef DEBUG_PDCCH_DMRS
    printf("symb %d i %d idx %d gold seq %u b0-b1 %d-%d mod_dmrs %d %d\n", symb, i, idx, gold_pdcch_dmrs[symb][(i<<1)>>5],
    (((gold_pdcch_dmrs[symb][(i<<1)>>5])>>((i<<1)&0x1f))&1), (((gold_pdcch_dmrs[symb][((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1),
     mod_dmrs[symb][(i<<1)], mod_dmrs[symb][(i<<1)+1]);
  #endif
    }
  }

  /// DCI payload processing
    // CRC attachment + Scrambling + Channel coding + Rate matching
  uint32_t encoder_output[NR_MAX_DCI_SIZE_DWORD];
  uint16_t n_RNTI = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? ((pdcch_params.scrambling_id)?pdcch_params.rnti:0) : 0;
  uint16_t Nid = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? pdcch_params.scrambling_id : config.sch_config.physical_cell_id.value;
#ifdef PDCCH_TEST_POLAR_TEMP_FIX
  t_nrPolar_paramsPtr currentPtr = NULL;//, polarParams = NULL;
  nr_polar_init(&currentPtr, NR_POLAR_DCI_MESSAGE_TYPE, dci_alloc.size, dci_alloc.L);
//  t_nrPolar_paramsPtr currentPtr = nr_polar_params(*nrPolar_params, NR_POLAR_DCI_MESSAGE_TYPE, dci_alloc.size, dci_alloc.L);
#else 
  nr_polar_init(nrPolar_params, NR_POLAR_DCI_MESSAGE_TYPE, dci_alloc.size, dci_alloc.L);
  t_nrPolar_paramsPtr currentPtr = nr_polar_params(*nrPolar_params, NR_POLAR_DCI_MESSAGE_TYPE, dci_alloc.size, dci_alloc.L);
#endif
  polar_encoder_dci(dci_alloc.dci_pdu, encoder_output, currentPtr, pdcch_params.rnti);

#ifdef DEBUG_CHANNEL_CODING
  printf("DCI PDU: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
    		  dci_alloc.dci_pdu[0], dci_alloc.dci_pdu[1], dci_alloc.dci_pdu[2], dci_alloc.dci_pdu[3]);
  printf("Encoded Payload: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
		  encoder_output[0], encoder_output[1], encoder_output[2], encoder_output[3]);
#endif

  /// Scrambling
  uint32_t scrambled_output[NR_MAX_DCI_SIZE_DWORD];
  nr_pdcch_scrambling(encoder_output, encoded_length, Nid, n_RNTI, scrambled_output);

    // QPSK modulation
  int16_t mod_dci[NR_MAX_DCI_SIZE>>1];
  for (int i=0; i<encoded_length>>1; i++) {
    idx = ((((scrambled_output[(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((scrambled_output[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
    mod_dci[i<<1] = nr_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
    mod_dci[(i<<1)+1] = nr_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_DCI
  printf("i %d idx %d b0-b1 %d-%d mod_dci %d %d\n", i, idx, (((encoder_output[(i<<1)>>5])>>((i<<1)&0x1f))&1),
  (((encoder_output[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), mod_dci[(i<<1)], mod_dci[(i<<1)+1]);
#endif
  }

  /// Resource mapping
  a = (config.rf_config.tx_antenna_ports.value == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;

  for (int aa = 0; aa < config.rf_config.tx_antenna_ports.value; aa++)
  {
    if (cset_start_sc >= frame_parms.ofdm_symbol_size)
      cset_start_sc -= frame_parms.ofdm_symbol_size;


    /*Reorder REG list for a freq first mapping*/
    uint8_t symb_idx[NR_MAX_CSET_DURATION] = {0,0,0};
    uint8_t nb_regs = dci_alloc.L*NR_NB_REG_PER_CCE;
    uint8_t regs_per_symb = nb_regs/cset_nsymb;
    for (int cce_idx=0; cce_idx<dci_alloc.L; cce_idx++){
      cce = dci_alloc.cce_list[cce_idx];
      for (int reg_idx=0; reg_idx<NR_NB_REG_PER_CCE; reg_idx++) {
        reg = cce.reg_list[reg_idx];
        reg_mapping_list[reg.symb_idx*regs_per_symb + symb_idx[reg.symb_idx]++] = reg;
      }
    }
#ifdef DEBUG_DCI
printf("\n Ordered REG list:\n");
for (int i=0; i<nb_regs; i++)
  printf("%d\t",reg_mapping_list[i].reg_idx );
printf("\n");
#endif

    if (pdcch_params.precoder_granularity == NFAPI_NR_CSET_ALL_CONTIGUOUS_RBS) {
    /*in this case the DMRS are mapped on all the coreset*/
      for (l=cset_start_symb; l<cset_start_symb+ cset_nsymb; l++) {
        dmrs_idx = 0;
        k = cset_start_sc + 1;
        while (dmrs_idx<3*pdcch_params.n_rb) {
          ((int16_t*)txdataF[aa])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (a * mod_dmrs[l][dmrs_idx<<1]) >> 15;
          ((int16_t*)txdataF[aa])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (a * mod_dmrs[l][(dmrs_idx<<1) + 1]) >> 15;
          k+=4;
          if (k >= frame_parms.ofdm_symbol_size)
            k -= frame_parms.ofdm_symbol_size;
          dmrs_idx++;
        }
      }    
    }

    /*Now mapping the encoded DCI based on newly constructed REG list
     * and the DMRS for the precoder granularity same as REG bundle*/
    for (int reg_idx=0; reg_idx<nb_regs; reg_idx++) {
      reg = reg_mapping_list[reg_idx];
      k = cset_start_sc + reg.start_sc_idx;
      if (k >= frame_parms.ofdm_symbol_size)
          k -= frame_parms.ofdm_symbol_size;
      l = cset_start_symb + reg.symb_idx;
      dmrs_idx = (reg.reg_idx/cset_nsymb)*3;
      k_prime = 0;
      for (int m=0; m<NR_NB_SC_PER_RB; m++) {
        if ( m == (k_prime<<2)+1) { // DMRS if not already mapped
          if (pdcch_params.precoder_granularity == NFAPI_NR_CSET_SAME_AS_REG_BUNDLE) {
            ((int16_t*)txdataF[aa])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (a * mod_dmrs[l][dmrs_idx<<1]) >> 15;
            ((int16_t*)txdataF[aa])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (a * mod_dmrs[l][(dmrs_idx<<1) + 1]) >> 15;
            k_prime++;
            dmrs_idx++;
          }
        }
        else { // DCI payload
          ((int16_t*)txdataF[aa])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (a * mod_dci[dci_idx<<1]) >> 15;
          ((int16_t*)txdataF[aa])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (a * mod_dci[(dci_idx<<1) + 1]) >> 15;
          dci_idx++;
        }
        k++;
        if (k >= frame_parms.ofdm_symbol_size)
          k -= frame_parms.ofdm_symbol_size;
      }
    }
  }

#ifdef DEBUG_DCI
  write_output("txdataF_dci.m", "txdataF_dci", txdataF[0], frame_parms.samples_per_frame_wCP>>1, 1, 1);
#endif

  return 0;
}
