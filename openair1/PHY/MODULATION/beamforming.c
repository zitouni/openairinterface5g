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

/*! \file PHY/MODULATION/beamforming.c
 * \brief 
 * \author X. JIANG, F. Kaltenberger, R. KNOPP
 * \date 2016
 * \version 0.1
 * \company Eurecom
 * \email: xiwen.jiang@eurecom.fr,florian.kaltenberger@eurecom.fr,raymond.knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/defs_common.h"
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"
#include "modulation_eNB.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

int beam_precoding(int32_t **txdataF,
	           int32_t **txdataF_BF,
                   LTE_DL_FRAME_PARMS *frame_parms,
	           int32_t ***beam_weights,
                   int slot,
                   int symbol,
                   int aa)
{
  uint8_t p;
  //uint16_t re=0;
  int slot_offset_F;
  
  slot_offset_F = slot*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);

  // clear txdata_BF[aa][re] for each call of ue_spec_beamforming
  memset(txdataF_BF[aa],0,sizeof(int32_t)*(frame_parms->ofdm_symbol_size));

  for (p=0; p<NB_ANTENNA_PORTS_ENB; p++) {
    if (p<frame_parms->nb_antenna_ports_eNB || p==5) {
      multadd_cpx_vector((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size],
			 (int16_t*)beam_weights[p][aa], 
			 (int16_t*)&txdataF_BF[aa][symbol*frame_parms->ofdm_symbol_size], 
			 0, 
			 frame_parms->ofdm_symbol_size, 
			 15);
      //mult_cpx_conj_vector((int16_t*)beam_weights[p][aa], (int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size], (int16_t*)txdataF_BF[aa], frame_parms->ofdm_symbol_size, 15, 1);

      // if check version
      /*for (re=0;re<frame_parms->ofdm_symbol_size;re++) {
        if (txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) {
          ((int16_t*)&txdataF_BF[aa][re])[0] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);

            printf("beamforming.c:txdataF[%d][%d]=%d+j%d, beam_weights[%d][%d][%d]=%d+j%d,txdata_BF[%d][%d]=%d+j%d\n",
                   p,slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re,
                   ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0],
                   ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1],
                   p,aa,re,
                   ((int16_t*)&beam_weights[p][aa][re])[0],((int16_t*)&beam_weights[p][aa][re])[1],
                   aa,re,
                   ((int16_t*)&txdataF_BF[aa][re])[0],
                   ((int16_t*)&txdataF_BF[aa][re])[1]);
        }
      }*/
    }
  }
  return 0;
}

// fdragon
int beam_precoding_one_eNB(int32_t **txdataF,
                           int32_t **txdataF_BF,
						   int32_t ***beam_weights,
						   int subframe,
						   int nb_antenna_ports,
						   int nb_tx, // total physical antenna
						   LTE_DL_FRAME_PARMS *frame_parms
						   )
{
  int p, re, symbol, aa; // loop index
  int re_offset;
  int ofdm_symbol_size     = frame_parms->ofdm_symbol_size;
  int symbols_per_tti      = frame_parms->symbols_per_tti;
  int nb_antenna_ports_eNB = frame_parms->nb_antenna_ports_eNB; // logic antenna ports
  
  
  re_offset = ofdm_symbol_size*symbols_per_tti*subframe;

  
  // txdataF_BF[aa][re] = sum(beam_weghts[p][aa][re]*txdataF[p][re]), p=0~nb_antenna_ports-1
  // real part and image part need to compute separately
  
  for (aa=0; aa<nb_tx; aa++) {
	  memset(txdataF_BF[aa],0,sizeof(int32_t)*(ofdm_symbol_size*symbols_per_tti));
	  for(p=0;p<nb_antenna_ports;p++){
		  if (p<nb_antenna_ports_eNB || p==5){
			  for (symbol=0; symbol<symbols_per_tti; symbol++){
				  
				  multadd_cpx_vector((int16_t*)&txdataF[p][symbol*ofdm_symbol_size+re_offset],
				                     (int16_t*)beam_weights[p][aa], 
									 (int16_t*)&txdataF_BF[aa][symbol*ofdm_symbol_size], 
									 0, 
									 ofdm_symbol_size, 
									 15);
				  
				  
				  /*
				  for (re=0; re<ofdm_symbol_size; re++){
					  // direct
					  ((int16_t*)&txdataF_BF[aa][re])[0] += (int16_t)((((int16_t*)&txdataF[p][re+symbol*ofdm_symbol_size+re_offset])[0]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
					  ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p][re+symbol*ofdm_symbol_size+re_offset])[1]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
					  ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][re+symbol*ofdm_symbol_size+re_offset])[0]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
					  ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][re+symbol*ofdm_symbol_size+re_offset])[1]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
				  }
				  */
			  }
		  }
	  }
  }

  
	  
	  
	  
		  

  return 0;
}