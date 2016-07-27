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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file PHY/LTE_TRANSPORT/if5_tools.c
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include "PHY/defs.h"

#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"


void send_IF5(PHY_VARS_eNB *eNB, openair0_timestamp proc_timestamp, int subframe, uint8_t *seqno, uint16_t packet_type) {      
  
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 
  int32_t *tx_buffer=NULL;

  uint16_t packet_id=0, i=0;

  uint32_t spp_eth  = (uint32_t) eNB->ifdevice.openair0_cfg->samples_per_packet;
  uint32_t spsf     = (uint32_t) eNB->ifdevice.openair0_cfg->samples_per_frame/10;
  
  if (packet_type == IF5_RRH_GW_DL) {    

    for (i=0; i < fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {
          
      eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                   (proc_timestamp + packet_id*spp_eth),
                                   txp,
                                   spp_eth,
                                   fp->nb_antennas_tx,
                                   0);

      for (i=0; i < fp->nb_antennas_tx; i++)
        txp[i] += spp_eth;

    }
    
  } else if (packet_type == IF5_RRH_GW_UL) {
        
    for (i=0; i < fp->nb_antennas_rx; i++)
      rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                   (proc_timestamp + packet_id*spp_eth),
                                   rxp,
                                   spp_eth,
                                   fp->nb_antennas_rx,
                                   0);

      for (i=0; i < fp->nb_antennas_rx; i++)
        rxp[i] += spp_eth;

    }    
    
  } else if (packet_type == IF5_MOBIPASS) {    
    uint16_t db_fulllength=640;
    
    __m128i *data_block=NULL, *data_block_head=NULL;

    __m128i *txp128;
    __m128i t0, t1;

    tx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)(tx_buffer + MAC_HEADER_SIZE_BYTES);
    data_block_head = (__m128i *)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + 4);
    
    header->flags = 0;
    header->fifo_status = 0;  
    header->seqno = *seqno;
    header->ack = 0;
    header->word0 = 0;  
    
    txp[0] = (void*)&eNB->common_vars.txdata[0][0][subframe*eNB->frame_parms.samples_per_tti];
    txp128 = (__m128i *) txp[0];
              
    for (packet_id=0; packet_id<(fp->samples_per_tti*2)/db_fulllength; packet_id++) {
      header->time_stamp = proc_timestamp + packet_id*db_fulllength; 
      data_block = data_block_head; 
    
      for (i=0; i<db_fulllength>>3; i+=2) {
        t0 = _mm_srai_epi16(*txp128++, 4);
        t1 = _mm_srai_epi16(*txp128++, 4);   
        
        *data_block++ = _mm_packs_epi16(t0, t1);     
      }
      
      // Write the packet to the fronthaul
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        packet_id,
                                        &tx_buffer,
                                        db_fulllength,
                                        1,
                                        IF5_MOBIPASS)) < 0) {
        perror("ETHERNET write for IF5_MOBIPASS\n");
      }
    
      header->seqno += 1;    
    }  
    *seqno = header->seqno;
    
  } else {    
    AssertFatal(1==0, "send_IF5 - Unknown packet_type %x", packet_type);     
  }  
  
  free(tx_buffer);
  return;  		    
}


void recv_IF5(PHY_VARS_eNB *eNB, openair0_timestamp *proc_timestamp, int subframe, uint16_t packet_type) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 

  uint16_t packet_id=0, i=0;

  int32_t spp_eth  = (int32_t) eNB->ifdevice.openair0_cfg->samples_per_packet;
  int32_t spsf     = (int32_t) eNB->ifdevice.openair0_cfg->samples_per_frame/10;

  openair0_timestamp timestamp[spsf / spp_eth];
  
  if (packet_type == IF5_RRH_GW_DL) {
        
    for (i=0; i < fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  &timestamp[packet_id],
                                  txp,
                                  spp_eth,
                                  fp->nb_antennas_tx);

      for (i=0; i < fp->nb_antennas_tx; i++)
        txp[i] += spp_eth;

    }
    
    *proc_timestamp = timestamp[0];
    
  } else if (packet_type == IF5_RRH_GW_UL) { 
    
    for (i=0; i < fp->nb_antennas_rx; i++)
      rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  &timestamp[packet_id],
                                  rxp,
                                  spp_eth,
                                  fp->nb_antennas_rx);

      for (i=0; i < fp->nb_antennas_rx; i++)
        rxp[i] += spp_eth;

    }

    *proc_timestamp = timestamp[0];
      
  } else if (packet_type == IF5_MOBIPASS) {
    
    
  } else {
    AssertFatal(1==0, "recv_IF5 - Unknown packet_type %x", packet_type);     
  }  
  
  return;  
}
