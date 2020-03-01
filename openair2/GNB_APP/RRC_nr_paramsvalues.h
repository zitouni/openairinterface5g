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

/*! \file openair2/GNB_APP/RRC_nr_paramsvalues.h
 * \brief macro definitions for RRC authorized and asn1 parameters values, to be used in paramdef_t/chechedparam_t structure initializations 
 * \author Francois TABURET, WEI-TAI CHEN, Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France, NTUST, EURECOM
 * \email: francois.taburet@nokia-bell-labs.com, kroempa@gmail.com, turker.yilmaz@eurecom.fr
 * \note
 * \warning
 */

#ifndef __NR_RRC_PARAMSVALUES__H__
#define __NR_RRC_PARAMSVALUES__H__
/*    cell configuration section name */
#define GNB_CONFIG_STRING_GNB_LIST                              "gNBs"
/* component carriers configuration section name */   
#define GNB_CONFIG_STRING_COMPONENT_CARRIERS                    "component_carriers"     

#define GNB_CONFIG_STRING_FRAME_TYPE                            "frame_type"
#define GNB_CONFIG_STRING_DL_PREFIX_TYPE                        "DL_prefix_type"
#define GNB_CONFIG_STRING_UL_PREFIX_TYPE                        "UL_prefix_type"
#define GNB_CONFIG_STRING_NR_BAND                               "nr_band"
#define GNB_CONFIG_STRING_DOWNLINK_FREQUENCY                    "downlink_frequency"
#define GNB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET               "uplink_frequency_offset"
#define GNB_CONFIG_STRING_NID_CELL                              "Nid_cell"
#define GNB_CONFIG_STRING_N_RB_DL                               "N_RB_DL"

#define FRAMETYPE_OKVALUES                                      {"FDD","TDD"}
#define FRAMETYPE_MODVALUES                                     { FDD, TDD} 

#define TDDCFG(A)                                               TDD_Config__subframeAssignment_ ## A
#define TDDCONFIG_OKRANGE                                       { TDDCFG(sa0), TDDCFG(sa6)}   

#define TDDCFGS(A)                                              TDD_Config__specialSubframePatterns_ ## A
#define TDDCONFIGS_OKRANGE                                      { TDDCFGS(ssp0), TDDCFGS(ssp8)}   

#define PREFIX_OKVALUES                                         {"NORMAL","EXTENDED"}
#define PREFIX_MODVALUES                                        { NORMAL, EXTENDED} 

#define PREFIXUL_OKVALUES                                       {"NORMAL","EXTENDED"}
#define PREFIXUL_MODVALUES                                      { NORMAL, EXTENDED} 

#define NRBDL_OKVALUES                                          {6,15,25,50,75,100}

#define NRUETIMER_T300_OKVALUES                                 {100,200,300,400,600,1000,1500,2000}
#define NRUETT300(A)                                            NR_UE_TimersAndConstants__t300_ ## A
#define NRUETIMER_T300_MODVALUES                                { NRUETT300(ms100), NRUETT300(ms200), NRUETT300(ms300), NRUETT300(ms400), NRUETT300(ms600), NRUETT300(ms1000), NRUETT300(ms1500), NRUETT300(ms2000)}

#define NRUETIMER_T301_OKVALUES                                 {100,200,300,400,600,1000,1500,2000}
#define NRUETT301(A)                                            NR_UE_TimersAndConstants__t301_ ## A
#define NRUETIMER_T301_MODVALUES                                { NRUETT301(ms100), NRUETT301(ms200), NRUETT301(ms300), NRUETT301(ms400), NRUETT301(ms600), NRUETT301(ms1000), NRUETT301(ms1500), NRUETT301(ms2000)}

#define NRUETIMER_T310_OKVALUES                                 {0,50,100,200,500,1000,2000}
#define NRUETT310(A)                                            NR_UE_TimersAndConstants__t310_ ## A
#define NRUETIMER_T310_MODVALUES                                { NRUETT310(ms0), NRUETT310(ms50), NRUETT310(ms100), NRUETT310(ms200), NRUETT310(ms500), NRUETT310(ms1000), NRUETT310(ms2000)}

#define NRUETIMER_T311_OKVALUES                                 {1000,3000,5000,10000,15000,20000,30000}
#define NRUETT311(A)                                            NR_UE_TimersAndConstants__t311_ ## A
#define NRUETIMER_T311_MODVALUES                                { NRUETT311(ms1000), NRUETT311(ms3000), NRUETT311(ms5000), NRUETT311(ms10000), NRUETT311(ms15000), NRUETT311(ms20000), NRUETT311(ms30000)}

#define NRUETIMER_T319_OKVALUES                                 {100,200,300,400,600,1000,1500,2000}
#define NRUETT319(A)                                            NR_UE_TimersAndConstants__t319_ ## A
#define NRUETIMER_T319_MODVALUES                                { NRUETT319(ms100), NRUETT319(ms200), NRUETT319(ms300), NRUETT319(ms400), NRUETT319(ms600), NRUETT319(ms1000), NRUETT319(ms1500), NRUETT319(ms2000}

#define NRUETIMER_N310_OKVALUES                                 {1,2,3,4,6,8,10,20}
#define NRUETN310(A)                                            NR_UE_TimersAndConstants__n310_ ## A
#define NRUETIMER_N310_MODVALUES                                { NRUETN310(n1), NRUETN310(n2), NRUETN310(n3), NRUETN310(n4), NRUETN310(n6), NRUETN310(n8), NRUETN310(n10), NRUETN310(n20)}

#define NRUETIMER_N311_OKVALUES                                 {1,2,3,4,5,6,8,10}
#define NRUETN311(A)                                            NR_UE_TimersAndConstants__n311_ ## A
#define NRUETIMER_N311_MODVALUES                                { NRUETN311(n1), NRUETN311(n2), NRUETN311(n3), NRUETN311(n4), NRUETN311(n5), NRUETN311(n6), NRUETN311(n8), NRUETN311(n10)}

#endif
