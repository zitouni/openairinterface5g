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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/CODING/defs.h"
#include "PHY/CODING/extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/LTE_TRANSPORT/defs.h"
#include "defs.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

int beam_precoding(int32_t **txdataF,
	           int32_t **txdataF_BF,
                   LTE_DL_FRAME_PARMS *frame_parms,
	           int32_t ***beam_weights,
                   int slot,
                   int symbol)
{
  uint8_t p,aa;
  uint16_t re=0;
  int slot_offset_F;
  
  slot_offset_F = slot*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);

  // clear txdata_BF[aa][re] for each call of ue_spec_beamforming
  for(aa=0;aa<frame_parms->nb_antennas_tx;aa++)
    memset(txdataF_BF[aa],0,sizeof(int32_t)*(frame_parms->ofdm_symbol_size));

  for (re=0;re<frame_parms->ofdm_symbol_size;re++) {
    for (p=0; p<2; p++) {
      if (txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) {
        for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
          ((int16_t*)&txdataF_BF[aa][re])[0] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
          ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);

	  /*
          printf("beamforming.c:txdataF[%d][%d]=%d+j%d, beam_weights[%d][%d][%d]=%d+j%d,txdata_BF[%d][%d]=%d+j%d\n",
                 p,slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re,
                 ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0],
                 ((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1],
                 p,aa,re,
                 ((int16_t*)&beam_weights[p][aa][re])[0],((int16_t*)&beam_weights[p][aa][re])[1],
                 aa,re,
                 ((int16_t*)&txdataF_BF[aa][re])[0],
                 ((int16_t*)&txdataF_BF[aa][re])[1]); 
	  */
        }
      
      } 
    }
    if (txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually using TM7 
      for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
        ((int16_t*)&txdataF_BF[aa][re])[0] = (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[5][aa][re])[0])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[5][aa][re])[1])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[1] = (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[5][aa][re])[1])>>15);
        ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[5][aa][re])[0])>>15);      
        
       /* printf("beamforming.c:txdata[5][%d]=%d+j%d, beam_weights[5][%d][%d]=%d+j%d,txdata_BF[%d][%d]=%d+j%d\n",
               slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re,
               ((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0],
               ((int16_t*)&txdataF[5][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1],
               aa,re,
               ((int16_t*)&beam_weights[5][aa][re])[0],
               ((int16_t*)&beam_weights[5][aa][re])[1],
               aa,re,
               ((int16_t*)&txdataF_BF[aa][re])[0],
               ((int16_t*)&txdataF_BF[aa][re])[1]); */
       
      }
    }

    if (txdataF[7][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually using TM8-10
      for (p=7;p<15;p++) {
        if (txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re]!=0) { //that means this RE is actually allocated
          for (aa=0;aa<frame_parms->nb_antennas_tx;aa++) {
            ((int16_t*)&txdataF_BF[aa][re])[0] = (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[0] -= (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[1] = (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[0]*((int16_t*)&beam_weights[p][aa][re])[1])>>15);
            ((int16_t*)&txdataF_BF[aa][re])[1] += (int16_t)((((int16_t*)&txdataF[p][slot_offset_F+symbol*frame_parms->ofdm_symbol_size+re])[1]*((int16_t*)&beam_weights[p][aa][re])[0])>>15);
          }
        }
      }
    }  
  }

return 0;

}
