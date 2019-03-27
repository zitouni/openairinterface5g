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

/*! \file PHY/NR_TRANSPORT/nr_ulsch_decoding.c
* \brief Top-level routines for decoding  LDPC (ULSCH) transport channels from 38.212, V15.4.0 2018-12
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/


// [from gNB coding]
#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR/sched_nr.h"
#include "defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>
//#define DEBUG_ULSCH_DECODING

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384

static uint64_t nb_total_decod =0;
static uint64_t nb_error_decod =0;

//extern double cpuf;

void free_gNB_ulsch(NR_gNB_ULSCH_t *ulsch)
{

  int i,r;

  if (ulsch) {
    for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {

      if (ulsch->harq_processes[i]) {
        if (ulsch->harq_processes[i]->b) {
          free16(ulsch->harq_processes[i]->b,MAX_NR_ULSCH_PAYLOAD_BYTES);
          ulsch->harq_processes[i]->b = NULL;
        }
        for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS; r++) {
          free16(ulsch->harq_processes[i]->c[r],(8448)*sizeof(uint8_t));
          ulsch->harq_processes[i]->c[r] = NULL;
        }
        for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS; r++) {
          if (ulsch->harq_processes[i]->d[r]) {
            free16(ulsch->harq_processes[i]->d[r],(68*384)*sizeof(int16_t));
            ulsch->harq_processes[i]->d[r] = NULL;
          }
        }
        for (r=0; r<(MAX_NUM_NR_ULSCH_SEGMENTS); r++) {
          if (ulsch->harq_processes[i]->p_nrLDPC_procBuf[r]){
            nrLDPC_free_mem(ulsch->harq_processes[i]->p_nrLDPC_procBuf[r]);
            ulsch->harq_processes[i]->p_nrLDPC_procBuf[r] = NULL;
          }
        }
        free16(ulsch->harq_processes[i],sizeof(NR_UL_gNB_HARQ_t));
        ulsch->harq_processes[i] = NULL;
      }
    }
    free16(ulsch,sizeof(NR_gNB_ULSCH_t));
    ulsch = NULL;
  }
}


NR_gNB_ULSCH_t *new_gNB_ulsch(uint8_t max_ldpc_iterations,uint8_t N_RB_UL, uint8_t abstraction_flag)
{

  NR_gNB_ULSCH_t *ulsch;
  uint8_t exit_flag = 0,i,r;
  unsigned char bw_scaling =1;

  switch (N_RB_UL) {
    case 106:
      bw_scaling =2;
    break;

    default:
      bw_scaling =1;
    break;
  }

  ulsch = (NR_gNB_ULSCH_t *)malloc16(sizeof(NR_gNB_ULSCH_t));

  if (ulsch) {

    memset(ulsch,0,sizeof(NR_gNB_ULSCH_t));

    ulsch->max_ldpc_iterations = max_ldpc_iterations;
    ulsch->Mlimit = 4;

    for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {

      ulsch->harq_processes[i] = (NR_UL_gNB_HARQ_t *)malloc16(sizeof(NR_UL_gNB_HARQ_t));

      if (ulsch->harq_processes[i]) {

        memset(ulsch->harq_processes[i],0,sizeof(NR_UL_gNB_HARQ_t));

        ulsch->harq_processes[i]->b = (uint8_t*)malloc16(MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);

        if (ulsch->harq_processes[i]->b)
          memset(ulsch->harq_processes[i]->b,0,MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);
        else
          exit_flag=3;

        if (abstraction_flag == 0) {
          for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS/bw_scaling; r++) {

            ulsch->harq_processes[i]->p_nrLDPC_procBuf[r] = nrLDPC_init_mem();

            ulsch->harq_processes[i]->c[r] = (uint8_t*)malloc16(8448*sizeof(uint8_t));

            if (ulsch->harq_processes[i]->c[r])
              memset(ulsch->harq_processes[i]->c[r],0,8448*sizeof(uint8_t));
            else
              exit_flag=2;

            ulsch->harq_processes[i]->d[r] = (int16_t*)malloc16((68*384)*sizeof(int16_t));

            if (ulsch->harq_processes[i]->d[r])
              memset(ulsch->harq_processes[i]->d[r],0,(68*384)*sizeof(int16_t));
            else
              exit_flag=2;
          }
        }
      } else {
        exit_flag=1;
      }
    }

    if (exit_flag==0)
      return(ulsch);
  }

  printf("new_gNB_ulsch with size %zu: exit_flag = %u\n",sizeof(NR_UL_gNB_HARQ_t), exit_flag);
  free_gNB_ulsch(ulsch);
  
  return(NULL);
}


uint32_t nr_ulsch_decoding(PHY_VARS_gNB *phy_vars_gNB,
                           uint8_t UE_id,
                           short *ulsch_llr,
                           NR_DL_FRAME_PARMS *frame_parms,
                           uint32_t frame,
                           uint16_t nb_symb_sch,
                           uint8_t nr_tti_rx,
                           uint8_t harq_pid,
                           uint8_t is_crnti,
                           uint8_t llr8_flag)
{

  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  int32_t no_iteration_ldpc, length_dec;
  uint32_t r,r_offset=0,Kr=8424,Kr_bytes,K_bytes_F,err_flag=0;
  uint8_t crc_type;
  int8_t llrProcBuf[OAI_UL_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));
  

  NR_gNB_ULSCH_t                       *ulsch                 = phy_vars_gNB->ulsch[UE_id][0];
  NR_UL_gNB_HARQ_t                     *harq_process          = ulsch->harq_processes[harq_pid];
  nfapi_nr_ul_config_ulsch_pdu_rel15_t *nfapi_ulsch_pdu_rel15 = &harq_process->ulsch_pdu.ulsch_pdu_rel15;
  
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params* p_decParams    = &decParams;
  t_nrLDPC_time_stats procTime;
  t_nrLDPC_time_stats* p_procTime     = &procTime ;
  t_nrLDPC_procBuf** p_nrLDPC_procBuf = harq_process->p_nrLDPC_procBuf;

  int16_t z [68*384];
  int8_t l [68*384];
  int16_t inv_d [68*384];
  uint8_t kc;
  uint8_t Ilbrm        = 0;
  uint32_t Tbslbrm     = 950984;
  uint16_t nb_rb       = 30; //to update
  uint8_t nb_re_dmrs   = 6;
  uint16_t length_dmrs = 1;
  double Coderate = 0.0;

  uint32_t i,j;

  __m128i *pv = (__m128i*)&z;
  __m128i *pl = (__m128i*)&l;
  
  
   if (!ulsch_llr) {
    printf("ulsch_decoding.c: NULL ulsch_llr pointer\n");
    return(ulsch->max_ldpc_iterations);
  }

  if (!harq_process) {
    printf("ulsch_decoding.c: NULL harq_process pointer\n");
    return(ulsch->max_ldpc_iterations);
  }

  if (!frame_parms) {
    printf("ulsch_decoding.c: NULL frame_parms pointer\n");
    return(ulsch->max_ldpc_iterations);
  }

  nb_rb = nfapi_ulsch_pdu_rel15->number_rbs;

  // harq_process->trials[nfapi_ulsch_pdu_rel15->round]++;

  harq_process->TBS = nr_compute_tbs(nfapi_ulsch_pdu_rel15->mcs, nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, nfapi_ulsch_pdu_rel15->n_layers);

  A   = harq_process->TBS;
  ret = ulsch->max_ldpc_iterations;

  harq_process->G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, nfapi_ulsch_pdu_rel15->Qm,nfapi_ulsch_pdu_rel15->n_layers);
  G = harq_process->G;

  LOG_I(PHY,"ULSCH Decoding, harq_pid %d TBS %d G %d mcs %d Nl %d nb_symb_sch %d nb_rb %d\n",harq_pid,A,G, nfapi_ulsch_pdu_rel15->mcs, nfapi_ulsch_pdu_rel15->n_layers, nb_symb_sch,nb_rb);

  if (harq_process->round == 0) {

    // This is a new packet, so compute quantities regarding segmentation
    harq_process->B = A+24;

    // [hna] Perform nr_segmenation with input and output set to NULL to calculate only (B, C, K, Z, F)
    nr_segmentation(NULL,
                    NULL,
                    harq_process->B,
                    &harq_process->C,
                    &harq_process->K,
                    &harq_process->Z, // [hna] Z is Zc
                    &harq_process->F);

    p_decParams->Z = harq_process->Z;

#ifdef DEBUG_ULSCH_DECODING
    printf("ulsch decoding nr segmentation Z %d\n", p_decParams->Z);
    if (!frame%100)
      printf("K %d C %d Z %d nl %d \n", harq_process->K, harq_process->C, p_decParams->Z, harq_process->Nl);
#endif
  }

  Coderate = (float) A /(float) G;

  if ((A <=292) || ((A<=3824) && (Coderate <= 0.6667)) || Coderate <= 0.25){
    p_decParams->BG = 2;
    if (Coderate < 0.3333) {
      p_decParams->R = 15;
      kc = 52;
    }
    else if (Coderate <0.6667) {
      p_decParams->R = 13;
      kc = 32;
    }
    else {
      p_decParams->R = 23;
      kc = 17;
    }
  } else {
    p_decParams->BG = 1;
    if (Coderate < 0.6667) {
      p_decParams->R = 13;
      kc = 68;
    }
    else if (Coderate <0.8889) {
      p_decParams->R = 23;
      kc = 35;
    }
    else {
      p_decParams->R = 89;
      kc = 27;
    }
  }

  //printf("coderate %f kc %d \n", Coderate, kc);



  p_decParams->numMaxIter = ulsch->max_ldpc_iterations;
  p_decParams->outMode= 0;

  err_flag = 0;
  r_offset = 0;

  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_UL) {

    case 106:
      bw_scaling =2;
      break;

    default:
      bw_scaling =1;
      break;
  }

  if (harq_process->C > MAX_NUM_NR_ULSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_NR_ULSCH_SEGMENTS/bw_scaling);
    return((1+ulsch->max_ldpc_iterations));
  }
#ifdef DEBUG_ULSCH_DECODING
  printf("Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);
#endif

  opp_enabled=1;

  Kr = harq_process->K; // [hna] overwrites this line "Kr = p_decParams->Z*kb"
  Kr_bytes = Kr>>3;

  K_bytes_F = Kr_bytes-(harq_process->F>>3);

  Tbslbrm = nr_compute_tbs(28,nb_rb,frame_parms->symbols_per_slot,0,0, nfapi_ulsch_pdu_rel15->n_layers);

  for (r=0; r<harq_process->C; r++) {

    E = nr_get_E(G, harq_process->C, nfapi_ulsch_pdu_rel15->Qm, nfapi_ulsch_pdu_rel15->n_layers, r);

#if gNB_TIMING_TRACE
    start_meas(ulsch_deinterleaving_stats);
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////// nr_deinterleaving_ldpc ///////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////// ulsch_llr =====> harq_process->e //////////////////////////////

    nr_deinterleaving_ldpc(E,
                           nfapi_ulsch_pdu_rel15->Qm,
                           harq_process->e[r],
                           ulsch_llr+r_offset);

    //for (int i =0; i<16; i++)
    //          printf("rx output deinterleaving w[%d]= %d r_offset %d\n", i,harq_process->w[r][i], r_offset);

#if gNB_TIMING_TRACE
    stop_meas(ulsch_deinterleaving_stats);
#endif

#if gNB_TIMING_TRACE
    start_meas(ulsch_rate_unmatching_stats);
#endif

#ifdef DEBUG_ULSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          nfapi_ulsch_pdu_rel15->Qm,
          nfapi_ulsch_pdu_rel15->number_rbs,
          nfapi_ulsch_pdu_rel15->n_layers,
          nfapi_ulsch_pdu_rel15->rv,
          harq_process->round);
#endif
    //////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////// nr_rate_matching_ldpc_rx ////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////// harq_process->e =====> harq_process->d /////////////////////////

    if (nr_rate_matching_ldpc_rx(Ilbrm,
                                 Tbslbrm,
                                 p_decParams->BG,
                                 p_decParams->Z,
                                 harq_process->d[r],
                                 harq_process->e[r],
                                 harq_process->C,
                                 nfapi_ulsch_pdu_rel15->rv,
                                 (harq_process->round==0)?1:0,
                                 E)==-1) {
#if gNB_TIMING_TRACE
      stop_meas(ulsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"ulsch_decoding.c: Problem in rate_matching\n");
      return(ulsch->max_ldpc_iterations);
    } else {
#if gNB_TIMING_TRACE
      stop_meas(ulsch_rate_unmatching_stats);
#endif
    }

    r_offset += E;

#ifdef DEBUG_ULSCH_DECODING
    if (r==0) {
      write_output("decoder_llr.m","decllr",ulsch_llr,G,1,0);
      write_output("decoder_in.m","dec",&harq_process->d[0][0],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; 
    for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][i]);
    printf("\n");
#endif


    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1) {
      crc_type = CRC24_A;
      length_dec = harq_process->B;
    }
    else {
      crc_type = CRC24_B;
      length_dec = (harq_process->B+24*harq_process->C)/harq_process->C;
    }

    if (err_flag == 0) {

#if gNB_TIMING_TRACE
      start_meas(ulsch_turbo_decoding_stats);
#endif

      //LOG_E(PHY,"AbsSubframe %d.%d Start LDPC segment %d/%d A %d ",frame%1024,nr_tti_rx,r,harq_process->C-1, A);

      
      memset(pv,0,2*harq_process->Z*sizeof(int16_t));
      memset((pv+K_bytes_F),127,harq_process->F*sizeof(int16_t));

      for (i=((2*p_decParams->Z)>>3), j = 0; i < K_bytes_F; i++, j++) {
        pv[i]= _mm_loadu_si128((__m128i*)(&harq_process->d[r][8*j]));
      }

      for (i=Kr_bytes,j=K_bytes_F-((2*p_decParams->Z)>>3); i < ((kc*p_decParams->Z)>>3); i++, j++) {
        pv[i]= _mm_loadu_si128((__m128i*)(&harq_process->d[r][8*j]));
      }
    
      for (i=0, j=0; j < ((kc*p_decParams->Z)>>4);  i+=2, j++) {
        pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      }

      //////////////////////////////////////////////////////////////////////////////////////////


      //////////////////////////////////////////////////////////////////////////////////////////
      ///////////////////////////////////// nrLDPC_decoder /////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////

      ////////////////////////////////// pl =====> llrProcBuf //////////////////////////////////

      no_iteration_ldpc = nrLDPC_decoder(p_decParams,
                                         (int8_t*)&pl[0],
                                         llrProcBuf,
                                         p_nrLDPC_procBuf[r],
                                         p_procTime);

      if (check_crc((uint8_t*)llrProcBuf,length_dec,harq_process->F,crc_type)) {
        printf("Segment %d CRC OK\n",r);
        ret = 2;
      } else {
        printf("CRC NOK\n");
        ret = 1+ulsch->max_ldpc_iterations;
      }
    
      nb_total_decod++;
      if (no_iteration_ldpc > ulsch->max_ldpc_iterations){
        nb_error_decod++;
      }

      ret=no_iteration_ldpc;

      for (int m=0; m < Kr>>3; m ++) {
        harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
      }

#ifdef DEBUG_ULSCH_DECODING
      //printf("output decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);
      for (int k=0;k<A>>3;k++)
       printf("output decoder [%d] =  0x%02x \n", k, harq_process->c[r][k]);
      printf("no_iterations_ldpc %d (ret %d)\n",no_iteration_ldpc,ret);
      //write_output("dec_output.m","dec0",harq_process->c[0],Kr_bytes,1,4);
#endif

#if gNB_TIMING_TRACE
      stop_meas(ulsch_turbo_decoding_stats);
#endif
    }

    if ((err_flag == 0) && (ret>=(1+ulsch->max_ldpc_iterations))) {
      // a Code segment is in error so break;
      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,nr_tti_rx,r,harq_process->C-1);
      err_flag = 1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////
  }

  int32_t frame_rx_prev = frame;
  int32_t tti_rx_prev = nr_tti_rx - 1;
  if (tti_rx_prev < 0) {
    frame_rx_prev--;
    tti_rx_prev += 10*frame_parms->ttis_per_subframe;
  }
  frame_rx_prev = frame_rx_prev%1024;

  if (err_flag == 1) {

#if gNB_DEBUG_TRACE
    LOG_I(PHY,"[gNB %d] ULSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
          phy_vars_gNB->Mod_id, frame, nr_tti_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
#endif

    // harq_process->harq_ack.ack = 0;
    // harq_process->harq_ack.harq_id = harq_pid;
    // harq_process->harq_ack.send_harq_status = 1;
    // harq_process->errors[harq_process->round]++;
    harq_process->round++;

    if (harq_process->round >= ulsch->Mlimit) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
      harq_process->handled  = 0;
      ulsch->harq_mask &= ~(1 << harq_pid);
    }

    if(is_crnti) {
      LOG_D(PHY,"[gNB %d] ULSCH: Setting NACK for nr_tti_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
            phy_vars_gNB->Mod_id,nr_tti_rx,harq_pid,harq_process->status,harq_process->round,ulsch->Mlimit,harq_process->TBS);
    }

    harq_process->handled  = 1;
    return((1+ulsch->max_ldpc_iterations));

  } else {

#if gNB_DEBUG_TRACE
    LOG_I(PHY,"[gNB %d] ULSCH: Setting ACK for nr_tti_rx %d TBS %d mcs %d nb_rb %d harq_process->round %d\n",
          phy_vars_gNB->Mod_id,nr_tti_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb, harq_process->round);
#endif

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    // harq_process->handled  = 0;
    ulsch->harq_mask  &= ~(1 << harq_pid);
    // harq_process->harq_ack.ack = 1;
    // harq_process->harq_ack.harq_id = harq_pid;
    // harq_process->harq_ack.send_harq_status = 1;

    if(is_crnti)
    {
      LOG_D(PHY,"[gNB %d] ULSCH: Setting ACK for nr_tti_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_gNB->Mod_id,nr_tti_rx,harq_pid,harq_process->round,harq_process->TBS);
    }
  }

  // Reassembly of Transport block here
  offset = 0;
  Kr = harq_process->K;
  Kr_bytes = Kr>>3;

  for (r=0; r<harq_process->C; r++) {

    memcpy(harq_process->b+offset,
           harq_process->c[r],
           Kr_bytes- - (harq_process->F>>3) -((harq_process->C>1)?3:0));

    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));

#ifdef DEBUG_ULSCH_DECODING
    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    printf("copied %d bytes to b sequence (harq_pid %d)\n",
           (Kr_bytes - (harq_process->F>>3)-((harq_process->C>1)?3:0)),harq_pid);
    printf("b[0] = %x,c[%d] = %x\n",
           harq_process->b[offset],
           harq_process->F>>3,
           harq_process->c[r]);
#endif

  }

  ulsch->last_iteration_cnt = ret;

  return(ret);
}