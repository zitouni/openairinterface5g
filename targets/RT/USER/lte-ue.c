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

/*! \file lte-ue.c
 * \brief threads and support functions for real-time LTE UE target
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <syscall.h>

#include "rt_wrapper.h"
#include "assertions.h"
#include "PHY/types.h"

#include "PHY/defs.h"
#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "RRC/LITE/extern.h"
#endif
#include "PHY_INTERFACE/extern.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#ifdef EXMIMO
#include "openair0_lib.h"
#else
#include "../../ARCH/COMMON/common_lib.h"
#endif

#include "PHY/extern.h"
#include "MAC_INTERFACE/extern.h"
//#include "SCHED/defs.h"
#include "SCHED/extern.h"
#ifdef OPENAIR2
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#endif

#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL

typedef enum {
  pss=0,
  pbch=1,
  si=2
} sync_mode_t;

int init_dlsch_threads(void);
void cleanup_dlsch_threads(void);
int32_t init_rx_pdsch_thread(void);
void cleanup_rx_pdsch_thread(void);

pthread_attr_t                  attr_UE_init_synch;
pthread_attr_t                  attr_UE_thread_tx;
pthread_attr_t                  attr_UE_thread_rx;
struct sched_param              sched_param_UE_init_synch;
struct sched_param              sched_param_UE_thread_tx;
struct sched_param              sched_param_UE_thread_rx;

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4]; 
extern openair0_rf_map rf_map[MAX_NUM_CCs];

extern openair0_device openair0;
extern int oai_exit;

extern int32_t **rxdata;
extern int32_t **txdata;

extern unsigned int samples_per_frame;
extern unsigned int tx_forward_nsamps;
extern int tx_delay;

extern int rx_input_level_dBm;
extern uint8_t exit_missed_slots;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern void exit_fun(const char* s);

#ifdef EXMIMO

extern unsigned int             rxg_max[4];
extern unsigned int             rxg_med[4];
extern unsigned int             rxg_byp[4];
extern unsigned int             nf_max[4];
extern unsigned int             nf_med[4];
extern unsigned int             nf_byp[4];
extern rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4];

extern double tx_gain[MAX_NUM_CCs][4];
extern double rx_gain[MAX_NUM_CCs][4];
#endif
#define KHz (1000UL)
#define MHz (1000 * KHz)

typedef struct eutra_band_s {
  int16_t band;
  uint32_t ul_min;
  uint32_t ul_max;
  uint32_t dl_min;
  uint32_t dl_max;
  lte_frame_type_t frame_type;
} eutra_band_t;

typedef struct band_info_s {
  int nbands;
  eutra_band_t band_info[100];
} band_info_t;

band_info_t bands_to_scan;

static const eutra_band_t eutra_bands[] =
  {
    { 1, 1920    * MHz, 1980    * MHz, 2110    * MHz, 2170    * MHz, FDD},
    { 2, 1850    * MHz, 1910    * MHz, 1930    * MHz, 1990    * MHz, FDD},
    { 3, 1710    * MHz, 1785    * MHz, 1805    * MHz, 1880    * MHz, FDD},
    { 4, 1710    * MHz, 1755    * MHz, 2110    * MHz, 2155    * MHz, FDD},
    { 5,  824    * MHz,  849    * MHz,  869    * MHz,  894    * MHz, FDD},
    { 6,  830    * MHz,  840    * MHz,  875    * MHz,  885    * MHz, FDD},
    { 7, 2500    * MHz, 2570    * MHz, 2620    * MHz, 2690    * MHz, FDD},
    { 8,  880    * MHz,  915    * MHz,  925    * MHz,  960    * MHz, FDD},
    { 9, 1749900 * KHz, 1784900 * KHz, 1844900 * KHz, 1879900 * KHz, FDD},
    {10, 1710    * MHz, 1770    * MHz, 2110    * MHz, 2170    * MHz, FDD},
    {11, 1427900 * KHz, 1452900 * KHz, 1475900 * KHz, 1500900 * KHz, FDD},
    {12,  698    * MHz,  716    * MHz,  728    * MHz,  746    * MHz, FDD},
    {13,  777    * MHz,  787    * MHz,  746    * MHz,  756    * MHz, FDD},
    {14,  788    * MHz,  798    * MHz,  758    * MHz,  768    * MHz, FDD},

    {17,  704    * MHz,  716    * MHz,  734    * MHz,  746    * MHz, FDD},
    {20,  832    * MHz,  862    * MHz,  791    * MHz,  821    * MHz, FDD},
    {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
    {34, 2010    * MHz, 2025    * MHz, 2010    * MHz, 2025    * MHz, TDD},
    {35, 1850    * MHz, 1910    * MHz, 1850    * MHz, 1910    * MHz, TDD},
    {36, 1930    * MHz, 1990    * MHz, 1930    * MHz, 1990    * MHz, TDD},
    {37, 1910    * MHz, 1930    * MHz, 1910    * MHz, 1930    * MHz, TDD},
    {38, 2570    * MHz, 2620    * MHz, 2570    * MHz, 2630    * MHz, TDD},
    {39, 1880    * MHz, 1920    * MHz, 1880    * MHz, 1920    * MHz, TDD},
    {40, 2300    * MHz, 2400    * MHz, 2300    * MHz, 2400    * MHz, TDD},
    {41, 2496    * MHz, 2690    * MHz, 2496    * MHz, 2690    * MHz, TDD},
    {42, 3400    * MHz, 3600    * MHz, 3400    * MHz, 3600    * MHz, TDD},
    {43, 3600    * MHz, 3800    * MHz, 3600    * MHz, 3800    * MHz, TDD},
    {44, 703    * MHz, 803    * MHz, 703    * MHz, 803    * MHz, TDD},
  };

static void *UE_thread_synch(void *arg) {

  int i,hw_slot_offset;
  PHY_VARS_UE *UE = arg;
  int current_band = 0;
  int current_offset = 0;
  sync_mode_t sync_mode = pss;
  int card;
  int ind;
  int CC_id;
  int k;
  int found;

  UE->is_synchronized = 0;
  printf("UE_thread_sync in with PHY_vars_UE %p\n",arg);
  printf("waiting for sync (UE_thread_synch) \n");


  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_sync_thread)\n");
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex (UE_sync_thread)\n");

  printf("starting UE synch thread\n");
  ind = 0;
  found = 0;
  current_band = eutra_bands[ind].band; 
  do  {
    printf("Scanning band %d, dl_min %u\n",current_band,eutra_bands[ind].dl_min);
    if ((eutra_bands[ind].dl_min <= downlink_frequency[0][0]) && (eutra_bands[ind].dl_max>= downlink_frequency[0][0])) {
      for (card=0;card<MAX_NUM_CCs;card++)
	for (i=0; i<4; i++) 
	  uplink_frequency_offset[card][i] = eutra_bands[ind].ul_min - eutra_bands[ind].dl_min;
      found = 1;
      break;
    }
    ind++;
    current_band = eutra_bands[ind].band; 
  } while (current_band < 44);
    
  if (found == 0) {
    exit_fun("Can't find EUTRA band for frequency");
    oai_exit=1;
  }


  if  (UE->UE_scan == 1) {
    for (card=0;card<MAX_CARDS;card++) {
      for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
	downlink_frequency[card][i] = bands_to_scan.band_info[0].dl_min;
	uplink_frequency_offset[card][i] = bands_to_scan.band_info[0].ul_min-bands_to_scan.band_info[0].dl_min;
    
	openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i];
	openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i];
#ifdef USRP
	openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB-USRP_GAIN_OFFSET;  
	switch(UE->lte_frame_parms.N_RB_DL) {
	case 6:
	  openair0_cfg[card].rx_gain[i] -= 12;
	  break;
	case 25:
	  openair0_cfg[card].rx_gain[i] -= 6;
	  break;
	case 50:
	  openair0_cfg[card].rx_gain[i] -= 3;
	  break;
	default:
	  printf("Unknown number of RBs %d\n",UE->lte_frame_parms.N_RB_DL);
	  break;
	}
	printf("UE synch: setting RX gain (%d,%d) to %f\n",card,i,openair0_cfg[card].rx_gain[i]);
#endif
      }
#ifdef EXMIMO
	//openair0_config(&openair0_cfg[card],1);
#endif
      }
#ifdef USRP
#ifndef USRP_DEBUG
	openair0_set_rx_frequencies(&openair0,&openair0_cfg[0]);
	openair0_set_gains(&openair0,&openair0_cfg[0]);
#endif
#endif
	LOG_D(PHY,"[SCHED][UE] Scanning band %d, freq %u\n",bands_to_scan.band_info[0].band, bands_to_scan.band_info[0].dl_min);
      }
  else {
    LOG_D(PHY,"[SCHED][UE] Check absolute frequency %u (oai_exit %d)\n",downlink_frequency[0][0],oai_exit);

    sync_mode=pbch;
      }

  while (oai_exit==0) {
    
    if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
      LOG_E(PHY,"[SCHED][UE] error locking mutex for UE initial synch thread\n");
      exit_fun("noting to add");
    }
    else {
      while (UE->instance_cnt_synch < 0) {
	pthread_cond_wait(&UE->cond_synch,&UE->mutex_synch);
      }
      if (pthread_mutex_unlock(&UE->mutex_synch) != 0) {	
	LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for UE Initial Synch thread\n");
	exit_fun("nothing to add");
      }

    }  // mutex_lock      

    vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH,1); 
    //printf("Sync_mode %d\n",sync_mode);
    switch (sync_mode) {
      case pss:
	
	
	
	current_offset += 20000000; // increase by 20 MHz
	if (current_offset > bands_to_scan.band_info[current_band].dl_max-bands_to_scan.band_info[current_band].dl_min) {
	current_band++;
	current_offset=0;
	}
      if (current_band==bands_to_scan.nbands) {
	current_band=0;
	oai_exit=1; 
      }
      
      for (card=0;card<MAX_CARDS;card++) {
	for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
	  downlink_frequency[card][i] = bands_to_scan.band_info[current_band].dl_min+current_offset;
	  uplink_frequency_offset[card][i] = bands_to_scan.band_info[current_band].ul_min-bands_to_scan.band_info[0].dl_min + current_offset;
	  
	  
	  openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i]+openair_daq_vars.freq_offset;
	  openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i]+openair_daq_vars.freq_offset;
#ifdef USRP
	  openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB-USRP_GAIN_OFFSET;  // 65 calibrated for USRP B210 @ 2.6 GHz
	  switch(UE->lte_frame_parms.N_RB_DL) {
	  case 6:
	    openair0_cfg[card].rx_gain[i] -= 12;
	    break;
	  case 25:
	    openair0_cfg[card].rx_gain[i] -= 6;
	    break;
	  case 50:
	    openair0_cfg[card].rx_gain[i] -= 3;
	    break;
	  default:
	    printf("Unknown number of RBs %d\n",UE->lte_frame_parms.N_RB_DL);
	    break;
	  }
	  printf("UE synch: setting RX gain (%d,%d) to %f\n",card,i,openair0_cfg[card].rx_gain[i]);
#endif
	
      }
#ifdef EXMIMO
	  //openair0_config(&openair0_cfg[card],1);	
#endif
    }	
#ifdef USRP
#ifndef USRP_DEBUG
      openair0_set_rx_frequencies(&openair0,&openair0_cfg[0]);
	  //	openair0_set_gains(&openair0,&openair0_cfg[0]);
#endif
#endif
      break;
    case pbch:
      //      printf("synch: Running initial sync\n");
      // This is a hack to fix a bug when using USRP
      memset(PHY_vars_UE_g[0][0]->lte_ue_common_vars.rxdata[0],0,1024);
      if (initial_sync(UE,UE->mode)==0) {
	/*
	  lte_adjust_synch(&PHY_vars_UE_g[0]->lte_frame_parms,
	  PHY_vars_UE_g[0],
	  0,
	  1,
	  16384);
	*/
	//for better visualization afterwards
	/*
	  for (aa=0; aa<PHY_vars_UE_g[0]->lte_frame_parms.nb_antennas_rx; aa++)
	  memset(PHY_vars_UE_g[0]->lte_ue_common_vars.rxdata[aa],0,
	  PHY_vars_UE_g[0]->lte_frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(int));
	*/
	
	
	UE->is_synchronized = 1;
#ifndef EXMIMO
	UE->slot_rx = 0;
	UE->slot_tx = 4;
#else
	UE->slot_rx = 18;
	UE->slot_tx = 2;
#endif
	hw_slot_offset = (UE->rx_offset<<1) / UE->lte_frame_parms.samples_per_tti;
	LOG_I(HW,"Got synch: hw_slot_offset %d\n",hw_slot_offset);
	  
      }
      else {  // intial_synch
	  
	if (openair_daq_vars.freq_offset >= 0) {
	  openair_daq_vars.freq_offset += 100;
	  openair_daq_vars.freq_offset *= -1;
	}
	else {
	  openair_daq_vars.freq_offset *= -1;
	}

	if (abs(openair_daq_vars.freq_offset) > 7500) {
	  LOG_I(PHY,"[initial_sync] No cell synchronization found, abandoning\n");
	  mac_xface->macphy_exit("No cell synchronization found, abandoning");
          return 0; // not reached
	}
	  LOG_I(PHY,"[initial_sync] trying carrier off %d Hz, rxgain %d (DL %u, UL %u)\n",openair_daq_vars.freq_offset,
		UE->rx_total_gain_dB,
		downlink_frequency[0][0]+openair_daq_vars.freq_offset,
		downlink_frequency[0][0]+uplink_frequency_offset[0][0]+openair_daq_vars.freq_offset);
	  for (card=0;card<MAX_CARDS;card++) {
	    for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
	      openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i]+openair_daq_vars.freq_offset;
	      openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i]+openair_daq_vars.freq_offset;
#ifdef USRP
	      openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB-USRP_GAIN_OFFSET;  // 65 calibrated for USRP B210 @ 2.6 GHz

	      switch(UE->lte_frame_parms.N_RB_DL) {
	      case 6:
		openair0_cfg[card].rx_gain[i] -= 12;
		break;
	      case 25:
		openair0_cfg[card].rx_gain[i] -= 6;
		break;
	      case 50:
		openair0_cfg[card].rx_gain[i] -= 3;
		break;
	      default:
		printf("Unknown number of RBs %d\n",UE->lte_frame_parms.N_RB_DL);
		break;
	      }
	      //	      printf("UE synch: setting RX gain (%d,%d) to %d\n",card,i,openair0_cfg[card].rx_gain[i]);
#endif
	    }
#ifdef EXMIMO
	      //openair0_config(&openair0_cfg[card],1);
	      //rt_sleep_ns(FRAME_PERIOD);
#endif
	  }
#ifdef USRP
#ifndef USRP_DEBUG
	      openair0_set_frequencies(&openair0,&openair0_cfg[0]);
	      //	    openair0_set_gains(&openair0,&openair0_cfg[0]);
#endif

#endif
	    
	  

	      //	      openair0_dump_config(&openair0_cfg[0],UE_flag);
	    
	      //	      rt_sleep_ns(FRAME_PERIOD);

      } // initial_sync=0
      break;
    case si:
    default:
      break;
    }
    vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH,0);  

    if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
      printf("[openair][SCHED][eNB] error locking mutex for UE synch\n");
    }
    else {
      UE->instance_cnt_synch--;
      
      if (pthread_mutex_unlock(&UE->mutex_synch) != 0) {	
	printf("[openair][SCHED][eNB] error unlocking mutex for UE synch\n");
      }
    }
    vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH,0); 
  }  // while !oai_exit
  return(0);
}

static void *UE_thread_tx(void *arg) {

#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;
#endif
#ifdef RTAI
  RT_TASK *task;
#endif
  int ret;

  PHY_VARS_UE *UE = (PHY_VARS_UE*)arg;

  UE->instance_cnt_tx=-1;

#ifdef RTAI
  task = rt_task_init_schmod(nam2num("UE TX Thread"), 0, 0, 0, SCHED_FIFO, 0xF);
  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE TX thread!!!!\n");
    return 0;
  }
  LOG_D(HW,"Started UE TX thread (id %p)\n",task);
#else

#ifdef LOWLATENCY
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  /* This creates a 1ms reservation every 10ms period*/
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime = 1 * 500000;  // each tx thread requires .5ms to finish its job
  attr.sched_deadline =1 * 1000000; // each tx thread will finish within 1ms
  attr.sched_period = 1 * 1000000; // each tx thread has a period of 1ms from the starting point

  
  if (sched_setattr(0, &attr, flags) < 0 ){
    perror("[SCHED] eNB tx thread: sched_setattr failed\n");
    exit(-1);
  }
#endif
#endif
  printf("waiting for sync (UE_thread_tx)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread_tx)\n");
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread_tx)\n");

  printf("Starting UE TX thread\n");

  mlockall(MCL_CURRENT | MCL_FUTURE);

  while (!oai_exit) {

    if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
      LOG_E(PHY,"[SCHED][eNB] error locking mutex for UE TX\n");
      exit_fun("nothing to add");
    }
    else {
      
      while (UE->instance_cnt_tx < 0) {
	pthread_cond_wait(&UE->cond_tx,&UE->mutex_tx);
      }
      if (pthread_mutex_unlock(&UE->mutex_tx) != 0) {	
	LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for UE TX\n");
	exit_fun("nothing to add");
      }
    }
    

    if ((subframe_select(&UE->lte_frame_parms,UE->slot_tx>>1)==SF_UL)||
	(UE->lte_frame_parms.frame_type == FDD)){
      phy_procedures_UE_TX(UE,0,0,UE->mode,no_relay);
    }
    if ((subframe_select(&UE->lte_frame_parms,UE->slot_tx>>1)==SF_S) &&
	((UE->slot_tx&1)==1)) {
      phy_procedures_UE_S_TX(UE,0,0,no_relay);
    }
    
 #ifdef OPENAIR2   
    if (UE->lte_frame_parms.frame_type == TDD) {
      
      ret = mac_xface->ue_scheduler(UE->Mod_id, 
				    UE->frame_tx,
				    UE->slot_rx>>1, 
				    subframe_select(&UE->lte_frame_parms,UE->slot_tx>>1),
                    0,
                    0/*FIXME CC_id*/);
      
      if (ret == CONNECTION_LOST) {
	LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, returning to PRACH\n",UE->Mod_id,
	      UE->frame_rx,UE->slot_tx>>1);
	UE->UE_mode[0] = PRACH;
	//      mac_xface->macphy_exit("Connection lost");
      }
      else if (ret == PHY_RESYNCH) {
	LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, trying to resynch\n",
	      UE->Mod_id,
	      UE->frame_rx,UE->slot_tx>>1);
	UE->UE_mode[0] = RESYNCH;
	//     mac_xface->macphy_exit("Connection lost");
	//exit(-1);
      } else if (ret == PHY_HO_PRACH) {
	LOG_I(PHY,"[UE %d] Frame %d, subframe %d, return to PRACH and perform a contention-free access\n",
	      UE->Mod_id,UE->frame_rx,UE->slot_tx>>1);
	UE->UE_mode[0] = PRACH;
      }
    }

#endif
    
    
    if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
      printf("[openair][SCHED][eNB] error locking mutex for UE TX thread\n");
    }
    else {
      UE->instance_cnt_tx--;
      
      if (pthread_mutex_unlock(&UE->mutex_tx) != 0) {	
	printf("[openair][SCHED][eNB] error unlocking mutex for UE\n");
      }
    }
    
    UE->slot_tx+=2;
    if (UE->slot_tx>=20) {
      UE->slot_tx-=20;
      UE->frame_tx++;
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX_UE, UE->frame_tx);
    }
    vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX_UE, UE->slot_tx>>1);
  }
  return(0);
}

//! \brief .
//! This is a pthread.
//! \param arg expects a pointer to \ref PHY_VARS_UE.
static void *UE_thread_rx(void *arg) {
  
  PHY_VARS_UE *UE = (PHY_VARS_UE*)arg;
  int i;
  int ret;
  
#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;
#endif
#ifdef RTAI
  RT_TASK *task;
#endif
  
  UE->instance_cnt_rx=-1;

#ifdef RTAI
  task = rt_task_init_schmod(nam2num("UE RX Thread"), 0, 0, 0, SCHED_FIFO, 0xF);
  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE RX thread!!!!\n");
    return 0;
  }
  LOG_D(HW,"Started UE RX thread (id %p)\n",task);
#else
  
#ifdef LOWLATENCY
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  // This creates a 1ms reservation every 10ms period
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime = 1 * 500000;  // each rx thread requires 1ms to finish its job
  attr.sched_deadline =1 * 1000000; // each rx thread will finish within 1ms
  attr.sched_period = 1 * 1000000; // each rx thread has a period of 1ms from the starting point
  
  if (sched_setattr(0, &attr, flags) < 0 ){
    perror("[SCHED] eNB tx thread: sched_setattr failed\n");
    exit(-1);
  }  
#endif
#endif
  
  mlockall(MCL_CURRENT | MCL_FUTURE);
  
  printf("waiting for sync (UE_thread_rx)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread_rx)\n");
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread_rx)\n");
  
  printf("Starting UE RX thread\n");
  
  while (!oai_exit) { 
    //   printf("UE_thread_rx: locking UE RX mutex\n");
    if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
      LOG_E(PHY,"[SCHED][eNB] error locking mutex for UE RX\n");
      exit_fun("nothing to add");
      break;
    }
      
      while (UE->instance_cnt_rx < 0) {
	pthread_cond_wait(&UE->cond_rx,&UE->mutex_rx);
      }

      if (pthread_mutex_unlock(&UE->mutex_rx) != 0) {	
	LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for UE RX\n");
	exit_fun("nothing to add");
        break;
      }
      
      for (i=0;i<2;i++) {
	if ((subframe_select(&UE->lte_frame_parms,UE->slot_rx>>1)==SF_DL) |
            (UE->lte_frame_parms.frame_type == FDD)) {
	  phy_procedures_UE_RX(UE,0,0,UE->mode,no_relay,NULL);
	}
	if ((subframe_select(&UE->lte_frame_parms,UE->slot_rx>>1)==SF_S) &&
	    ((UE->slot_rx&1)==0)) {
	  phy_procedures_UE_RX(UE,0,0,UE->mode,no_relay,NULL);
	}

#ifdef OPENAIR2      	
	if (i==0) {
	  ret = mac_xface->ue_scheduler(UE->Mod_id, 
					UE->frame_tx,
					UE->slot_rx>>1,
					subframe_select(&UE->lte_frame_parms,UE->slot_tx>>1),
                    0,
                    0/*FIXME CC_id*/);
	  
	  if (ret == CONNECTION_LOST) {
	    LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, returning to PRACH\n",UE->Mod_id,
		  UE->frame_rx,UE->slot_tx>>1);
	    UE->UE_mode[0] = PRACH;
	    //      mac_xface->macphy_exit("Connection lost");
	  }
	  else if (ret == PHY_RESYNCH) {
	    LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, trying to resynch\n",
		  UE->Mod_id,
		  UE->frame_rx,UE->slot_tx>>1);
	    UE->UE_mode[0] = RESYNCH;
	    //     mac_xface->macphy_exit("Connection lost");
	  } 
	  else if (ret == PHY_HO_PRACH) {
	    LOG_I(PHY,"[UE %d] Frame %d, subframe %d, return to PRACH and perform a contention-free access\n",
		  UE->Mod_id,UE->frame_rx,UE->slot_tx>>1);
	    UE->UE_mode[0] = PRACH;
	  }
        }	  
#endif	
	UE->slot_rx++;
	
	if (UE->slot_rx==20) {
	UE->slot_rx=0;
	UE->frame_rx++;
	vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX_UE, UE->frame_rx);
      } 
	
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX_UE, UE->slot_rx>>1);
      
	}
	
	if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
	printf("[openair][SCHED][eNB] error locking mutex for UE RX\n");
      }
	else {
	  UE->instance_cnt_rx--;
	  
	  if (pthread_mutex_unlock(&UE->mutex_rx) != 0) {	
	    printf("[openair][SCHED][eNB] error unlocking mutex for UE RX\n");
	  }
	}
	//    printf("UE_thread_rx done\n");
  }

  // thread finished
  return 0;
}
    



#ifndef EXMIMO
#define RX_OFF_MAX 10
#define RX_OFF_MIN 5
#define RX_OFF_MID ((RX_OFF_MAX+RX_OFF_MIN)/2)

void *UE_thread(void *arg) {

  PHY_VARS_UE *UE=PHY_vars_UE_g[0][0];
  LTE_DL_FRAME_PARMS *frame_parms=&UE->lte_frame_parms;
  int spp = openair0_cfg[0].samples_per_packet;

  int slot=1,frame=0,hw_subframe=0,rxpos=0,txpos=0;
  // unsigned int aa;
  int dummy[2][spp];
  int dummy_dump = 0;
  int tx_enabled=0;
  int start_rx_stream=0;
  int rx_off_diff = 0;
  int rx_correction_timer = 0;
  int i;
  int first_rx=0;
  RTIME T0;
#ifdef RTAI
  RT_TASK *task;
#endif

  unsigned int rxs;
  void *rxp[2],*txp[2];
  openair0_timestamp timestamp;

  
#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;
#endif
  


#ifdef RTAI
  task = rt_task_init_schmod(nam2num("UE thread"), 0, 0, 0, SCHED_FIFO, 0xF);
  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE thread!!!!\n");
    return 0;
  }
#else
  
#ifdef LOWLATENCY
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  // This creates a .5 ms  reservation
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = 0.25 * 1000000;
  attr.sched_deadline = 0.25 * 1000000;
  attr.sched_period   = 0.5 * 1000000;
    
  // pin the UE main thread to CPU0
  // if (pthread_setaffinity_np(pthread_self(), sizeof(mask),&mask) <0) {
  //   perror("[MAIN_ENB_THREAD] pthread_setaffinity_np failed\n");
  //   }
  
  if (sched_setattr(0, &attr, flags) < 0 ){
    perror("[SCHED] main eNB thread: sched_setattr failed\n");
    exit_fun("Nothing to add");
  } else {
    LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %ld started on CPU %d\n",
	  gettid(),sched_getcpu());
  }
#endif
#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread)\n");
  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread)\n");
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread)\n");

  printf("starting UE thread\n");

  


  T0 = rt_get_time_ns();
  first_rx = 1;
  rxpos=0;
  while (!oai_exit) {
    vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, hw_subframe);
    vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame);

    
    while (rxpos < (1+hw_subframe)*UE->lte_frame_parms.samples_per_tti) {
       vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ,1);
  
#ifndef USRP_DEBUG

      for (i=0;i<UE->lte_frame_parms.nb_antennas_rx;i++)
	rxp[i] = (dummy_dump==0) ? (void*)&rxdata[i][rxpos] : (void*)dummy[i];
      rxs = openair0.trx_read_func(&openair0,
				   &timestamp,
				   rxp,
				   spp - ((first_rx==1) ? rx_off_diff : 0),
				   UE->lte_frame_parms.nb_antennas_rx);
      if (rxs != (spp- ((first_rx==1) ? rx_off_diff : 0)))
	exit_fun("problem in rx");

      rx_off_diff = 0;
      vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ,0);
      
      // Transmit TX buffer based on timestamp from RX
      if (tx_enabled) {
	vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE,1);
	for (i=0;i<UE->lte_frame_parms.nb_antennas_tx;i++)
	  txp[i] = (void*)&txdata[i][txpos];
	openair0.trx_write_func(&openair0,
				(timestamp+spp*tx_delay-tx_forward_nsamps),
				txp,
				spp,
				UE->lte_frame_parms.nb_antennas_tx,
				1);
				
	vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE,0);
      }
#else
      rt_sleep_ns(1000000);
#endif
      rxpos+=spp;
      txpos+=spp;
      
      if(txpos >= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti)
	txpos -= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti;
    }
    
    if(rxpos >= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti)
      rxpos -= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti;

    if (UE->is_synchronized==1)  {
      LOG_D(HW,"UE_thread: hw_frame %d, hw_subframe %d (time %llu)\n",frame,hw_subframe,rt_get_time_ns()-T0);

      if (start_rx_stream==1) {
	//	printf("UE_thread: locking UE mutex_rx\n");
	if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
	  LOG_E(PHY,"[SCHED][UE] error locking mutex for UE RX thread\n");
	  exit_fun("nothing to add");
	}
	else {
 	  
	  UE->instance_cnt_rx++;
	  //	  printf("UE_thread: Unlocking UE mutex_rx\n");
	  pthread_mutex_unlock(&UE->mutex_rx);
	  if (UE->instance_cnt_rx == 0) {
	    // LOG_D(HW,"Scheduling UE RX for frame %d (hw frame %d), subframe %d (%d), mode %d\n",UE->frame_rx,frame,hw_subframe,UE->slot_rx>>1,mode);
	    if (pthread_cond_signal(&UE->cond_rx) != 0) {
	      LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE RX thread\n");
	      exit_fun("nothing to add");
	    }
	    else {
	      //	      printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
	    }
	    if (UE->mode == rx_calib_ue) {
	      if (frame == 10) {
		LOG_D(PHY,"[SCHED][UE] Found cell with N_RB_DL %d, PHICH CONFIG (%d,%d), Nid_cell %d, NB_ANTENNAS_TX %d, initial frequency offset %d Hz, frequency offset %d Hz, RSSI (digital) %d dB, measured Gain %d dB, total_rx_gain %d dB, USRP rx gain %f dB\n",
		      UE->lte_frame_parms.N_RB_DL,
		      UE->lte_frame_parms.phich_config_common.phich_duration,
		      UE->lte_frame_parms.phich_config_common.phich_resource,
		      UE->lte_frame_parms.Nid_cell,
		      UE->lte_frame_parms.nb_antennas_tx_eNB,
		      openair_daq_vars.freq_offset,
		      UE->lte_ue_common_vars.freq_offset,
		      UE->PHY_measurements.rx_power_avg_dB[0],
		      UE->PHY_measurements.rx_power_avg_dB[0] - rx_input_level_dBm,
		      UE->rx_total_gain_dB,
		      openair0_cfg[0].rx_gain[0]
		      );
		exit_fun("[HW][UE] UE in RX calibration mode, exiting");
	      }
	    }
	  }
	  else {
	    LOG_E(PHY,"[SCHED][UE] UE RX thread busy!!\n");
	    exit_fun("nothing to add");
	  }
	}

	if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
	  LOG_E(PHY,"[SCHED][UE] error locking mutex for UE TX thread\n");
	  exit_fun("nothing to add");
	}
	else {

	  if (tx_enabled == 1) {
	    UE->instance_cnt_tx++;
	    //printf("UE_thread: Unlocking UE mutex_rx\n");
	    pthread_mutex_unlock(&UE->mutex_tx);
	    if (UE->instance_cnt_tx == 0) {
	      if (pthread_cond_signal(&UE->cond_tx) != 0) {
		LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE TX thread\n");
		exit_fun("nothing to add");
	      }
	      else {
		//	      printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
	      }
	    }
	    else {
	      LOG_E(PHY,"[SCHED][UE] UE TX thread busy!!\n");
	      exit_fun("nothing to add");
	    }
	  }
	}
      }
    }
    else {  // we are not yet synchronized
      if ((hw_subframe == 9)&&(dummy_dump == 0)) {
	// Wake up initial synch thread
	if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
	  LOG_E(PHY,"[SCHED][UE] error locking mutex for UE initial synch thread\n");
	  exit_fun("nothing to add");
	}
	else {
	  
	  UE->instance_cnt_synch++;
	  pthread_mutex_unlock(&UE->mutex_synch);
	  dummy_dump = 1;
	  if (UE->instance_cnt_synch == 0) {
	    if (pthread_cond_signal(&UE->cond_synch) != 0) {
	      LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE sync thread\n");
	      exit_fun("nothing to add");
	    }
	  }
	  else {
	    LOG_E(PHY,"[SCHED][UE] UE sync thread busy!!\n");
	    exit_fun("nothing to add");
	  }
	}
      }
    }
    
    hw_subframe++;
    slot+=2;
    if(hw_subframe==10) {
      hw_subframe = 0;
      first_rx = 1;
      frame++;
      slot = 1;
      if (UE->instance_cnt_synch < 0) {
	if (UE->is_synchronized == 1) {
	  //	  openair0_set_gains(&openair0,&openair0_cfg[0]);
	  rx_off_diff = 0;
	  //	  LOG_D(PHY,"HW RESYNC: hw_frame %d: rx_offset = %d\n",frame,UE->rx_offset);
	  if ((UE->rx_offset > RX_OFF_MAX)&&(start_rx_stream==0)) {
	    start_rx_stream=1;
	    //LOG_D(PHY,"HW RESYNC: hw_frame %d: Resynchronizing sample stream\n");
	    frame=0;
	    // dump ahead in time to start of frame

#ifndef USRP_DEBUG
	    rxs = openair0.trx_read_func(&openair0,
					 &timestamp,
					 (void**)rxdata,
					 UE->rx_offset,
					 UE->lte_frame_parms.nb_antennas_rx);
#else
	    rt_sleep_ns(10000000);
#endif
	    UE->rx_offset=0;
	    tx_enabled=1;
	  }
	  else if ((UE->rx_offset < RX_OFF_MIN)&&(start_rx_stream==1) && (rx_correction_timer == 0)) {
	    rx_off_diff = -UE->rx_offset + RX_OFF_MIN;
	    rx_correction_timer = 5;
	  }
	  else if ((UE->rx_offset > (FRAME_LENGTH_COMPLEX_SAMPLES-RX_OFF_MAX)) &&(start_rx_stream==1) && (rx_correction_timer == 0)) {
	    rx_off_diff = FRAME_LENGTH_COMPLEX_SAMPLES-UE->rx_offset;
	    rx_correction_timer = 5;
	  }
	  if (rx_correction_timer>0)
	    rx_correction_timer--;
	  //	  LOG_D(PHY,"HW RESYNC: hw_frame %d: (rx_offset %d) Correction: rx_off_diff %d (timer %d)\n",frame,UE->rx_offset,rx_off_diff,rx_correction_timer);
	}
	dummy_dump=0;
      }
    }
    
#if defined(ENABLE_ITTI)
    itti_update_lte_time(frame, slot);
#endif
  }
  return(0);
}
#endif



#ifdef EXMIMO
/* This is the main UE thread. Initially it is doing a periodic get_frame. One synchronized it gets woken up by the kernel driver using the RTAI message mechanism (rt_send and rt_receive). */
void *UE_thread(void *arg) {
  PHY_VARS_UE *UE=PHY_vars_UE_g[0][0];
#ifdef RTAI
  RT_TASK *task;
#endif
  // RTIME in, out, diff;
  int slot=0,frame=0,hw_slot,last_slot,next_slot;
  // unsigned int aa;
  int delay_cnt;
  RTIME time_in;
  int hw_slot_offset=0,rx_offset_mbox=0,mbox_target=0,mbox_current=0;
  int diff2;
  int i, ret;
  int CC_id,card;
  volatile unsigned int *DAQ_MBOX = openair0_daq_cnt();

  int wait_sync_cnt = 0;
  int first_synch = 1;
#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;
  unsigned long mask = 1; // processor 0 
#endif



#ifdef RTAI
  task = rt_task_init_schmod(nam2num("UE thread"), 0, 0, 0, SCHED_FIFO, 0xF);
  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE thread!!!!\n");
    return 0;
  }
#endif


#ifdef HARD_RT
  rt_make_hard_real_time();
#endif


#ifdef LOWLATENCY
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  // This creates a .25 ms  reservation
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = 0.1 * 1000000;
  attr.sched_deadline = 0.25 * 1000000;
  attr.sched_period   = 0.5 * 1000000;
  
  // pin the UE main thread to CPU0
  // if (pthread_setaffinity_np(pthread_self(), sizeof(mask),&mask) <0) {
  //   perror("[MAIN_ENB_THREAD] pthread_setaffinity_np failed\n");
  //   }
   
  if (sched_setattr(0, &attr, flags) < 0 ){
    perror("[SCHED] main UE thread: sched_setattr failed\n");
    exit_fun("Nothing to add");
  } else {
    LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %ld started on CPU %d\n",
	  gettid(),sched_getcpu());
  }
#endif
  

  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread)\n");
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread)\n");

  printf("starting UE thread\n");

  openair_daq_vars.freq_offset = 0; //-7500;

  first_synch = 1;

  while (!oai_exit)  {

    hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15; //the slot the hw is about to store

     
    if (UE->is_synchronized) {

      if (first_synch == 1) {
	first_synch = 0;
	for (card=0;card<openair0_num_detected_cards;card++)
          openair0_start_rt_acquisition(card);
	rt_sleep_ns(FRAME_PERIOD/10);
      }

      //this is the mbox counter that indicates the start of the frame
      rx_offset_mbox = (UE->rx_offset * 150) / (10*UE->lte_frame_parms.samples_per_tti);
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_UE_RX_OFFSET, UE->rx_offset);
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_UE_OFFSET_MBOX, rx_offset_mbox);
      //this is the mbox counter where we should be
      mbox_target = (((((slot+1)%20)*15+1)>>1) + rx_offset_mbox + 1)%150;
      // round up to the next multiple of two (mbox counter from express MIMO gives only even numbers)
      mbox_target = ((mbox_target+1)-((mbox_target-1)%2))%150;
      //this is the mbox counter where we are
      mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];
      //this is the time we need to sleep in order to synchronize with the hw (in multiples of DAQ_PERIOD)
      if ((mbox_current>=120) && (mbox_target<30)) //handle the frame wrap-arround
	diff2 = 150-mbox_current+mbox_target;
      else if ((mbox_current<30) && (mbox_target>=120))
	diff2 = -150+mbox_target-mbox_current;
      else
	diff2 = mbox_target - mbox_current;
	
      if (diff2 <(-7)) {
	LOG_D(HW,"UE Frame %d: missed slot, proceeding with next one (slot %d, hw_slot %d, diff %d)\n",frame, slot, hw_slot, diff2);
	if (frame>0){
	  if (exit_missed_slots==1)
	    exit_fun("[HW][UE] missed slot");
	  else{
	    num_missed_slots++;
	    LOG_W(HW,"[UE] just missed slot (total missed slots %ld)\n", num_missed_slots);
	  }
	}
        slot++;
        if (slot==20) {
          slot=0;
          frame++;
        }
	// update thread slot/frame counters because of skipped slot
	UE->slot_rx++;
	UE->slot_tx++;
	
	if (UE->slot_rx == 20) {
	  UE->slot_rx = 0;
	  UE->frame_rx++;
	}
	if (UE->slot_tx == 20) {
	  UE->slot_tx = 0;
	  UE->frame_tx++;
	}
	continue;
      }
      if (diff2>8)
	LOG_D(HW,"UE Frame %d: skipped slot, waiting for hw to catch up (slot %d, hw_slot %d, mbox_current %d, mbox_target %d, diff %d)\n",frame, slot, hw_slot, mbox_current, mbox_target, diff2);
	
	
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
      vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff2);
	


      // This loop implements the delay of 1 slot to allow for processing
      delay_cnt = 0;
      while ((diff2>0) && (!oai_exit) )  {
        time_in = rt_get_time_ns();
        //LOG_D(HW,"eNB Frame %d delaycnt %d : hw_slot %d (%d), slot %d (%d), diff %d, time %llu\n",frame,delay_cnt,hw_slot,((volatile unsigned int *)DAQ_MBOX)[0],slot,mbox_target,diff2,time_in);
        vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
        vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,1);
        ret = rt_sleep_ns(diff2*DAQ_PERIOD);
        vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,0);
        vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
        if (ret)
          LOG_D(HW,"eNB Frame %d, time %llu: rt_sleep_ns returned %d\n",frame, time_in);
          
        hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
        //LOG_D(HW,"eNB Frame %d : hw_slot %d, time %llu\n",frame,hw_slot,rt_get_time_ns());
        delay_cnt++;
        if (delay_cnt == 30)  {
          LOG_D(HW,"UE frame %d: HW stopped ... \n",frame);
          exit_fun("[HW][UE] HW stopped");
        }
        mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];
        if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
          diff2 = 150-mbox_current+mbox_target;
        else if ((mbox_current<15) && (mbox_target>=135))
          diff2 = -150+mbox_target-mbox_current;
        else
          diff2 = mbox_target - mbox_current;
          
        vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
        vcd_signal_dumper_dump_variable_by_name(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff2);
      }

      // on even slots, schedule processing of entire subframe
      if ((slot&1) == 0)  {
	
	if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
	  LOG_E(PHY,"[SCHED][UE] error locking mutex for UE RX thread\n");
	  exit_fun("nothing to add");
	}
	else {
	  
	  UE->instance_cnt_rx++;
	  //printf("UE_thread: Unlocking UE mutex_rx\n");
	  pthread_mutex_unlock(&UE->mutex_rx);
	  if (UE->instance_cnt_rx == 0) {
	    LOG_D(HW,"Scheduling UE RX for frame %d (hw frame %d), subframe %d (%d), mode %d\n",UE->frame_rx,frame,slot>>1,UE->slot_rx>>1,UE->mode);
	    if (pthread_cond_signal(&UE->cond_rx) != 0) {
	      LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE RX thread\n");
	      exit_fun("nothing to add");
	    }
	    else {
	      //	      printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
	    }
	    if (UE->mode == rx_calib_ue) {
	      if (frame == 10) {
		LOG_D(PHY,"[SCHED][UE] Found cell with N_RB_DL %d, PHICH CONFIG (%d,%d), Nid_cell %d, NB_ANTENNAS_TX %d, initial frequency offset %d Hz, frequency offset %d Hz, RSSI (digital) %d dB, measured Gain %d dB, total_rx_gain %d dB, USRP rx gain %f dB\n",
		      UE->lte_frame_parms.N_RB_DL,
		      UE->lte_frame_parms.phich_config_common.phich_duration,
		      UE->lte_frame_parms.phich_config_common.phich_resource,
		      UE->lte_frame_parms.Nid_cell,
		      UE->lte_frame_parms.nb_antennas_tx_eNB,
		      openair_daq_vars.freq_offset,
		      UE->lte_ue_common_vars.freq_offset,
		      UE->PHY_measurements.rx_power_avg_dB[0],
		      UE->PHY_measurements.rx_power_avg_dB[0] - rx_input_level_dBm,
		      UE->rx_total_gain_dB,
		      openair0_cfg[0].rx_gain[0]
		      );
		exit_fun("[HW][UE] UE in RX calibration mode, exiting");
	      }
	    }
	  }
	  else {
	    LOG_E(PHY,"[SCHED][UE] UE RX thread busy!!\n");
	    exit_fun("nothing to add");
	  }
	}

	if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
	  LOG_E(PHY,"[SCHED][UE] error locking mutex for UE TX thread\n");
	  exit_fun("nothing to add");
	}
	else {
	  
	  UE->instance_cnt_tx++;
	  //printf("UE_thread: Unlocking UE mutex_rx\n");
	  pthread_mutex_unlock(&UE->mutex_tx);
	  if (UE->instance_cnt_tx == 0) {
	    LOG_D(HW,"Scheduling UE TX for frame %d (hw frame %d), subframe %d (%d), mode %d\n",UE->frame_tx,frame,slot>>1,UE->slot_tx>>1,UE->mode);
	    if (pthread_cond_signal(&UE->cond_tx) != 0) {
	      LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE TX thread\n");
	      exit_fun("nothing to add");
	    }
	    else {
	      //	      printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
	    }
	  }
	  else {
	    LOG_E(PHY,"[SCHED][UE] UE TX thread busy!!\n");
	    exit_fun("nothing to add");
	  }
	}
      }

      /*
	if ((slot%2000)<10)
	LOG_D(HW,"fun0: doing very hard work\n");
      */
      // now increment slot and frame counters
      slot++;
      if (slot==20) {
	slot=0;
	frame++;
      }
    }	
    else if (UE->is_synchronized == 0) {  // we are not yet synchronized
      hw_slot_offset = 0;
      first_synch = 1;
      slot = 0;


      // wait until we can lock mutex_synch
      //printf("Locking mutex_synch (UE_thread)\n");
      if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
	LOG_E(PHY,"[SCHED][UE] error locking mutex for UE initial synch thread\n");
	exit_fun("noting to add");
      }
      else {
	//printf("Before getting frame IC %d (UE_thread)\n",UE->instance_cnt_synch);
	if (UE->instance_cnt_synch < 0) {

	  wait_sync_cnt=0;

	  openair0_get_frame(0);
	  rt_sleep_ns(FRAME_PERIOD);
	  // increment instance count for sync thread
	  UE->instance_cnt_synch++;
	  pthread_mutex_unlock(&UE->mutex_synch);
	  if (pthread_cond_signal(&UE->cond_synch) != 0) {
	    LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE sync thread\n");
	    exit_fun("nothing to add");
	  }
	}
	else {
	  wait_sync_cnt++;
	  pthread_mutex_unlock(&UE->mutex_synch);

	  if (wait_sync_cnt>1000)
	    exit_fun("waiting to long for synch thread");
	  else
	    rt_sleep_ns(FRAME_PERIOD);
	}
      }	  

      
      /*            
		    if (initial_sync(UE,mode)==0) {

		    if (mode == rx_calib_ue) {
		    exit_fun("[HW][UE] UE in RX calibration mode");
		    }
		    else {
		    is_synchronized = 1;
		    //start the streaming DMA transfers
		    for (card=0;card<openair0_num_detected_cards;card++)
		    openair0_start_rt_acquisition(card);
	  
		    hw_slot_offset = (UE->rx_offset<<1) / UE->lte_frame_parms.samples_per_tti;
		    }
		    }
		    else {
		    if (openair_daq_vars.freq_offset >= 0) {
		    openair_daq_vars.freq_offset += 100;
		    openair_daq_vars.freq_offset *= -1;
		    }
		    else {
		    openair_daq_vars.freq_offset *= -1;
		    }
		    if (abs(openair_daq_vars.freq_offset) > 7500) {
		    LOG_I(PHY,"[initial_sync] No cell synchronization found, abondoning\n");
		    mac_xface->macphy_exit("No cell synchronization found, abondoning");
		    }
		    else {
		    //	  LOG_I(PHY,"[initial_sync] trying carrier off %d Hz\n",openair_daq_vars.freq_offset);
		    #ifndef USRP
		    for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
		    for (i=0; i<openair0_cfg[rf_map[CC_id].card].rx_num_channels; i++) 
		    openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+openair_daq_vars.freq_offset;
		    for (i=0; i<openair0_cfg[rf_map[CC_id].card].tx_num_channels; i++) 
		    openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+openair_daq_vars.freq_offset;
		    }
		    openair0_config(&openair0_cfg[0],UE_flag);
		    #endif
		    rt_sleep_ns(FRAME_PERIOD);
		    }
		    }
      */
    }
  }

  LOG_D(HW,"UE_thread: finished, ran %d times.\n",frame);
  
#ifdef HARD_RT
  rt_make_soft_real_time();
#endif
  
  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif
  LOG_D(HW,"Task deleted. returning\n");
  return 0;
}

#else  // This is for USRP or ETHERNET targets

#endif


void init_UE_threads(void) {
  
  PHY_VARS_UE *UE=PHY_vars_UE_g[0][0];

  pthread_attr_init(&attr_UE_thread_tx);
  pthread_attr_setstacksize(&attr_UE_thread_tx,16*PTHREAD_STACK_MIN);
  sched_param_UE_thread_tx.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  pthread_attr_setschedparam  (&attr_UE_thread_tx, &sched_param_UE_thread_tx);
  pthread_attr_setschedpolicy (&attr_UE_thread_tx, SCHED_FIFO);

  pthread_attr_init(&attr_UE_thread_rx);
  pthread_attr_setstacksize(&attr_UE_thread_rx,8*PTHREAD_STACK_MIN);
  sched_param_UE_thread_rx.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  pthread_attr_setschedparam  (&attr_UE_thread_rx, &sched_param_UE_thread_rx);
  pthread_attr_setschedpolicy (&attr_UE_thread_rx, SCHED_FIFO);

  pthread_attr_init (&attr_UE_init_synch);
  pthread_attr_setstacksize(&attr_UE_init_synch,8*PTHREAD_STACK_MIN);
  sched_param_UE_init_synch.sched_priority = sched_get_priority_max(SCHED_FIFO); //OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_UE_init_synch, &sched_param_UE_init_synch);
  pthread_attr_setschedpolicy (&attr_UE_init_synch, SCHED_FIFO);

  UE->instance_cnt_tx=-1;
  UE->instance_cnt_rx=-1;
  UE->instance_cnt_synch=-1;
  pthread_mutex_init(&UE->mutex_tx,NULL);
  pthread_mutex_init(&UE->mutex_rx,NULL);
  pthread_mutex_init(&UE->mutex_synch,NULL);
  pthread_cond_init(&UE->cond_tx,NULL);
  pthread_cond_init(&UE->cond_rx,NULL);
  pthread_cond_init(&UE->cond_synch,NULL);
  pthread_create(&UE->thread_tx,NULL,UE_thread_tx,(void*)UE);
  pthread_create(&UE->thread_rx,NULL,UE_thread_rx,(void*)UE);
  pthread_create(&UE->thread_rx,NULL,UE_thread_synch,(void*)UE);
  UE->frame_tx = 0;
  UE->frame_rx = 0;

  
}


#ifdef OPENAIR2
void fill_ue_band_info(void) {

  UE_EUTRA_Capability_t *UE_EUTRA_Capability = UE_rrc_inst[0].UECap->UE_EUTRA_Capability;
  int i,j;

  bands_to_scan.nbands = UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.count;
  
  for (i=0;i<bands_to_scan.nbands;i++) {

    for (j=0;j<sizeof (eutra_bands) / sizeof (eutra_bands[0]);j++)
      if (eutra_bands[j].band == UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.array[i]->bandEUTRA) {
	memcpy(&bands_to_scan.band_info[i],
	       &eutra_bands[j],
	       sizeof(eutra_band_t));
	
	printf("Band %d (%lu) : DL %u..%u Hz, UL %u..%u Hz, Duplex %s \n",
	       bands_to_scan.band_info[i].band,
	       UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.array[i]->bandEUTRA,
	       bands_to_scan.band_info[i].dl_min,
	       bands_to_scan.band_info[i].dl_max,
	       bands_to_scan.band_info[i].ul_min,
	       bands_to_scan.band_info[i].ul_max,
	       (bands_to_scan.band_info[i].frame_type==FDD) ? "FDD" : "TDD");
	break;
      }
  }
}
#endif

int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg, openair0_rf_map rf_map[MAX_NUM_CCs])
{

#ifndef EXMIMO
  uint16_t N_TA_offset = 0;
#endif

  int i, CC_id;
  LTE_DL_FRAME_PARMS *frame_parms;
  for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
    if (phy_vars_ue[CC_id]) {
      frame_parms = &(phy_vars_ue[CC_id]->lte_frame_parms); 
    }
    else {
      printf("phy_vars_eNB[%d] not initialized\n", CC_id);
      return(-1);
    }


#ifndef EXMIMO
    if (frame_parms->frame_type == TDD) {
      if (frame_parms->N_RB_DL == 100)
	N_TA_offset = 624;
      else if (frame_parms->N_RB_DL == 50)
	N_TA_offset = 624/2;
      else if (frame_parms->N_RB_DL == 25)
	N_TA_offset = 624/4;
    }
#endif
   
#ifdef EXMIMO
    openair0_cfg[CC_id].tx_num_channels = 0;
    openair0_cfg[CC_id].rx_num_channels = 0;

    // replace RX signal buffers with mmaped HW versions
    for (i=0;i<frame_parms->nb_antennas_rx;i++) {
      printf("Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i]);
      phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].adc_head[rf_map[CC_id].chain+i];
      if (openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i]) {
	printf("Error with rf_map! A channel has already been allocated!\n");
	return(-1);
      }
      else {
	openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i];
	openair0_cfg[rf_map[CC_id].card].rx_gain[rf_map[CC_id].chain+i] = rx_gain[CC_id][i];
	openair0_cfg[rf_map[CC_id].card].rxg_mode[rf_map[CC_id].chain+i] = rx_gain_mode[CC_id][i];
	openair0_cfg[rf_map[CC_id].card].rx_num_channels++;
      }

      printf("rxdata[%d] @ %p\n",i,phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i]);
    }
    for (i=0;i<frame_parms->nb_antennas_tx;i++) {
      printf("Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i]);
      phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].dac_head[rf_map[CC_id].chain+i];
      if (openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i]) {
	printf("Error with rf_map! A channel has already been allocated!\n");
	return(-1);
      }
      else {
	openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
	openair0_cfg[rf_map[CC_id].card].tx_gain[rf_map[CC_id].chain+i] = tx_gain[CC_id][i];
	openair0_cfg[rf_map[CC_id].card].tx_num_channels++;
      }

      printf("txdata[%d] @ %p\n",i,phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i]);
    }
  
#else
    // replace RX signal buffers with mmaped HW versions
    rxdata = (int32_t**)malloc16(frame_parms->nb_antennas_rx*sizeof(int32_t*));
    txdata = (int32_t**)malloc16(frame_parms->nb_antennas_tx*sizeof(int32_t*));
    for (i=0;i<frame_parms->nb_antennas_rx;i++) {
      printf("Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i]);
      rxdata[i] = (int32_t*)malloc16(samples_per_frame*sizeof(int32_t));
      phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i] = rxdata[i]-N_TA_offset; // N_TA offset for TDD
    }
    for (i=0;i<frame_parms->nb_antennas_tx;i++) {
      printf("Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i]);
      txdata[i] = (int32_t*)malloc16(samples_per_frame*sizeof(int32_t));
      phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i] = txdata[i];
      memset(txdata[i], 0, samples_per_frame*sizeof(int32_t));
    }
    
#endif
    
  }
  return(0);

}
