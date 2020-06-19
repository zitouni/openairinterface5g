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

/*!\file nrLDPC_decoder.c
 * \brief Defines thenrLDPC decoder
*/

#include <stdint.h>
#include <immintrin.h>
#include "nrLDPCdecoder_defs.h"
#include "nrLDPC_types.h"
#include "nrLDPC_init.h"
#include "nrLDPC_mPass.h"
#include "nrLDPC_cnProc.h"
#include "nrLDPC_bnProc.h"


  /*----------------------------------------------------------------------/
 /                 cn processing files -->AVX512                         /
/----------------------------------------------------------------------*/

//BG1-------------------------------------------------------------------
#ifdef __AVX512BW__
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z384_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z352_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z320_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z288_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z256_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z240_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z224_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z208_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z192_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z176_R13_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z160_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z144_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z128_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z120_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z112_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z104_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z96_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z88_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z80_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z72_R13_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z64_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z60_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z56_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z52_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z48_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z44_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z40_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z36_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z32_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z30_R13_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z28_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z26_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z24_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z22_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z20_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z18_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z16_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z15_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z14_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z13_R13_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z12_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z11_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z10_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z9_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z8_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z7_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z6_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z5_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z4_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z3_R13_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG1_Z2_R13_AVX512.c"

//BG2------------------------------------------------------------------
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z384_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z352_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z320_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z288_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z256_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z240_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z224_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z208_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z192_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z176_R15_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z160_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z144_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z128_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z120_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z112_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z104_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z96_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z88_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z80_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z72_R15_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z64_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z60_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z56_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z52_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z48_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z44_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z40_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z36_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z32_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z30_R15_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z28_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z26_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z24_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z22_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z20_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z18_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z16_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z15_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z14_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z13_R15_AVX512.c"

#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z12_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z11_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z10_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z9_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z8_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z7_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z6_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z5_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z4_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z3_R15_AVX512.c"
#include "nrLDPC_tools/ldpc_gen_files/nrLDPC_cnProc_BG2_Z2_R15_AVX512.c"

#else

  /*----------------------------------------------------------------------/
 /                  cn Processing files -->AVX2                           /
/----------------------------------------------------------------------*/

//BG1------------------------------------------------------------------
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z384_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z352_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z320_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z288_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z256_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z240_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z224_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z208_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z192_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z176_R13_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z160_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z144_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z128_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z120_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z112_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z104_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z96_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z88_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z80_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z72_R13_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z64_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z60_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z56_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z52_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z48_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z44_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z40_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z36_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z32_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z30_R13_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z28_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z26_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z24_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z22_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z20_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z18_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z16_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z15_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z14_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z13_R13_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z12_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z11_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z10_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z9_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z8_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z7_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z6_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z5_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z4_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z3_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG1_Z2_R13_AVX2.c"

//BG2 --------------------------------------------------------------------
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z384_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z352_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z320_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z288_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z256_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z240_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z224_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z208_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z192_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z176_R15_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z160_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z144_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z128_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z120_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z112_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z104_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z96_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z88_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z80_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z72_R15_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z64_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z60_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z56_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z52_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z48_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z44_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z40_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z36_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z32_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z30_R15_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z28_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z26_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z24_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z22_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z20_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z18_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z16_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z15_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z14_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z13_R15_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z12_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z11_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z10_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z9_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z8_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z7_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z6_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z5_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z4_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z3_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/cnProc/nrLDPC_cnProc_BG2_Z2_R15_AVX2.c"
#endif
//Bn Processing  files
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z384_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z256_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z192_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z128_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z96_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z64_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z48_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z32_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z24_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z16_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z12_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z8_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z6_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z4_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z3_R13_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG1_Z2_R13_AVX2.c"

#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z384_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z256_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z192_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z128_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z96_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z64_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z48_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z32_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z24_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z16_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z12_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z8_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z6_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z4_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z3_R15_AVX2.c"
#include "nrLDPC_tools/ldpc_gen_files/bnProcPc/nrLDPC_bnProcPc_BG2_Z2_R15_AVX2.c"

///#include "nrLDPC_tools/ldpc_gen_files/bnProc/nrLDPC_bnProc_BG1_Z384_R13_AVX2.c"
//#include "nrLDPC_tools/ldpc_gen_files/bnProc/nrLDPC_bnProc_BG2_Z384_R15_AVX2.c"




#define NR_LDPC_ENABLE_PARITY_CHECK
#define NR_LDPC_PROFILER_DETAIL

#ifdef NR_LDPC_DEBUG_MODE
#include "nrLDPC_tools/nrLDPC_debug.h"
#endif

static inline uint32_t nrLDPC_decoder_core(int8_t* p_llr, int8_t* p_out, t_nrLDPC_procBuf* p_procBuf, uint32_t numLLR, t_nrLDPC_lut* p_lut, t_nrLDPC_dec_params* p_decParams, t_nrLDPC_time_stats* p_profiler);

int32_t nrLDPC_decod(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, t_nrLDPC_procBuf* p_procBuf, t_nrLDPC_time_stats* p_profiler)
{
    uint32_t numLLR;
    uint32_t numIter = 0;
    t_nrLDPC_lut lut;
    t_nrLDPC_lut* p_lut = &lut;

    //printf("p_procBuf->cnProcBuf = %p\n", p_procBuf->cnProcBuf);

    // Initialize decoder core(s) with correct LUTs
    numLLR = nrLDPC_init(p_decParams, p_lut);

    // LaunchnrLDPC decoder core for one segment
    numIter = nrLDPC_decoder_core(p_llr, p_out, p_procBuf, numLLR, p_lut, p_decParams, p_profiler);

    return numIter;
}

/**
   \brief PerformsnrLDPC decoding of one code block
   \param p_llr Input LLRs
   \param p_out Output vector
   \param numLLR Number of LLRs
   \param p_lut Pointer to decoder LUTs
   \param p_decParamsnrLDPC decoder parameters
   \param p_profilernrLDPC profiler statistics
*/
static inline uint32_t nrLDPC_decoder_core(int8_t* p_llr, int8_t* p_out, t_nrLDPC_procBuf* p_procBuf, uint32_t numLLR, t_nrLDPC_lut* p_lut, t_nrLDPC_dec_params* p_decParams, t_nrLDPC_time_stats* p_profiler)
{
    uint16_t Z          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
    uint8_t  numMaxIter = p_decParams->numMaxIter;
    e_nrLDPC_outMode outMode = p_decParams->outMode;
   // int8_t* cnProcBuf= p_procBuf->cnProcBuf;
   // int8_t* cnProcBufRes=p_procBuf->cnProcBufRes;

    // Minimum number of iterations is 1
    // 0 iterations means hard-decision on input LLRs
    uint32_t i = 1;
    // Initialize with parity check fail != 0
    int32_t pcRes = 1;
    int8_t* p_llrOut;

    if (outMode == nrLDPC_outMode_LLRINT8)
    {
        p_llrOut = p_out;
    }
    else
    {
        // Use LLR processing buffer as temporary output buffer
        p_llrOut = p_procBuf->llrProcBuf;
        // Clear llrProcBuf
        memset(p_llrOut,0, NR_LDPC_MAX_NUM_LLR*sizeof(int8_t));
    }


    // Initialization
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llr2llrProcBuf);
#endif
    nrLDPC_llr2llrProcBuf(p_lut, p_llr, p_procBuf, Z, BG);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2llrProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_LLR_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_PROC, p_procBuf);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llr2CnProcBuf);
#endif
    if (BG == 1)
    {
        nrLDPC_llr2CnProcBuf_BG1(p_lut, p_llr, p_procBuf, Z);
    }
    else
    {
        nrLDPC_llr2CnProcBuf_BG2(p_lut, p_llr, p_procBuf, Z);
    }
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2CnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_CN_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, p_procBuf);
#endif

    // First iteration

    // CN processing
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->cnProc);
#endif
    if (BG==1)
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
             	nrLDPC_cnProc_BG1_Z384_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
               	#else
                nrLDPC_cnProc_BG1_Z384_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
               	#endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z352_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z352_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z320_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z320_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z288_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z288_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z256_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z256_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z240_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z240_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z224_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z224_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z208_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z208_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z192_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z192_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z144_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z144_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z128_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z128_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z120_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG1_Z120_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z112_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z112_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z104_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z104_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z96_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z96_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z88_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z88_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z80_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z80_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z72_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z72_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z64_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z64_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z60_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z60_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z56_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z56_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z52_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z52_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z48_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z48_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z44_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z44_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z40_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z40_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z36_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z36_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z32_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z32_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z30_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z30_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z28_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z28_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z26_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z26_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z24_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z24_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z22_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z22_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z20_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z20_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z18_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z18_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z16_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z16_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z15_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z15_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z14_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z14_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z13_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z13_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z12_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z12_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z11_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z11_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z10_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z10_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z9_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z9_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z8_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z8_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z7_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z7_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z6_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z6_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z5_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z5_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z4_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z4_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z3_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z3_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z2_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z2_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    } //end if
    else
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z384_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z384_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z352_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z352_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z320_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z320_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z288_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z288_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z256_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z256_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z240_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z240_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z224_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z224_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z208_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z208_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z192_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z192_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z144_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z144_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z128_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                 nrLDPC_cnProc_BG2_Z128_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z120_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z120_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z112_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z112_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z104_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z104_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z96_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z96_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z88_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z88_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z80_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z80_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z72_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z72_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z64_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z64_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z60_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z60_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z56_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z56_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z52_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z52_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z48_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z48_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z44_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z44_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z40_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z40_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z36_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z36_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z32_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z32_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z30_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z30_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z28_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z28_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z26_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z26_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z24_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z24_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z22_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z22_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z20_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z20_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z18_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z18_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z16_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z16_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z15_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z15_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z14_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z14_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z13_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z13_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z12_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z12_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z11_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z11_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z10_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z10_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z9_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z9_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z8_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z8_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z7_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z7_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z6_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z6_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z5_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z5_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z4_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z4_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z3_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z3_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z2_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z2_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    }//end of else
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->cnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_CN_PROC_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC_RES, p_procBuf);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->cn2bnProcBuf);
#endif
    if (BG == 1)
    {
        nrLDPC_cn2bnProcBuf_BG1(p_lut, p_procBuf, Z);
    }
    else
    {
        nrLDPC_cn2bnProcBuf_BG2(p_lut, p_procBuf, Z);
    }
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->cn2bnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_BN_PROC);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC, p_procBuf);
#endif

    // BN processing
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bnProcPc);
#endif
         if(BG==1){

            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG1_Z384_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG1_Z256_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG1_Z192_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG1_Z128_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG1_Z96_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG1_Z64_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 48:
                {
                    nrLDPC_bnProcPc_BG1_Z48_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 32:
                {
                    nrLDPC_bnProcPc_BG1_Z32_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        else{
            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG2_Z384_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG2_Z256_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG2_Z192_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG2_Z128_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG2_Z96_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG2_Z64_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 48:
                {
                    nrLDPC_bnProcPc_BG2_Z48_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 32:
                {
                    nrLDPC_bnProcPc_BG2_Z32_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bnProcPc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_LLR_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_RES, p_procBuf);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bnProc);
#endif
    nrLDPC_bnProc(p_lut, p_procBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_initBuffer2File(nrLDPC_buffers_BN_PROC_RES);
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC_RES, p_procBuf);
#endif

    // BN results to CN processing buffer
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->bn2cnProcBuf);
#endif
    if (BG == 1)
    {
        nrLDPC_bn2cnProcBuf_BG1(p_lut, p_procBuf, Z);
    }
    else
    {
        nrLDPC_bn2cnProcBuf_BG2(p_lut, p_procBuf, Z);
    }
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->bn2cnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
    nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, p_procBuf);
#endif

    // Parity Check not necessary here since it will fail
    // because first 2 cols/BNs in BG are punctured and cannot be
    // estimated after only one iteration

    // First iteration finished

    while ( (i < (numMaxIter-1)) && (pcRes != 0) )
    {
        // Increase iteration counter
        i++;

        // CN processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cnProc);
#endif
    if (BG==1)
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z384_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z384_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z352_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z352_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z320_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z320_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z288_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z288_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z256_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z256_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z240_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z240_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z224_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z224_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z208_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z208_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z192_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z192_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z144_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z144_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z128_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z128_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z120_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG1_Z120_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z112_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z112_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z104_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z104_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z96_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z96_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z88_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z88_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z80_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z80_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z72_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z72_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z64_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z64_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z60_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z60_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z56_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z56_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z52_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z52_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z48_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z48_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z44_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z44_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z40_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z40_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z36_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z36_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z32_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z32_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z30_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z30_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z28_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z28_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z26_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z26_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z24_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z24_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z22_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z22_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z20_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z20_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z18_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z18_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z16_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z16_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z15_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z15_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z14_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z14_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z13_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z13_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z12_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z12_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z11_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z11_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z10_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z10_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z9_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z9_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z8_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z8_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z7_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z7_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z6_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z6_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z5_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z5_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z4_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z4_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z3_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z3_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z2_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z2_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    } //end if
    else
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z384_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z384_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z352_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z352_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z320_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z320_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z288_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z288_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z256_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z256_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z240_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z240_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z224_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z224_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z208_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z208_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z192_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z192_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z144_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z144_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z128_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                 nrLDPC_cnProc_BG2_Z128_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z120_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z120_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z112_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z112_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z104_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z104_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z96_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z96_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z88_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z88_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z80_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z80_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z72_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z72_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z64_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z64_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z60_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z60_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z56_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z56_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z52_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z52_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z48_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z48_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z44_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z44_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z40_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z40_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z36_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z36_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z32_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z32_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z30_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z30_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z28_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z28_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z26_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z26_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z24_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z24_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z22_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z22_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z20_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z20_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z18_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z18_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z16_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z16_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z15_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z15_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z14_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z14_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z13_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z13_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z12_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z12_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z11_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z11_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z10_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z10_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z9_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z9_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z8_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z8_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z7_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z7_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z6_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z6_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z5_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z5_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z4_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z4_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z3_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z3_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z2_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z2_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    }//end of else
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC_RES, p_procBuf);
#endif

        // Send CN results back to BNs
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cn2bnProcBuf);
#endif
        if (BG == 1)
        {
            nrLDPC_cn2bnProcBuf_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            nrLDPC_cn2bnProcBuf_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cn2bnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC, p_procBuf);
#endif

        // BN Processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProcPc);
#endif
            if(BG==1){

            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG1_Z384_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG1_Z256_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG1_Z192_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG1_Z128_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG1_Z96_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG1_Z64_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 48:
                {
                    nrLDPC_bnProcPc_BG1_Z48_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 32:
                {
                    nrLDPC_bnProcPc_BG1_Z32_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        else{
            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG2_Z384_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG2_Z256_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG2_Z192_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG2_Z128_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG2_Z96_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG2_Z64_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 48:
                {
                    nrLDPC_bnProcPc_BG2_Z48_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 32:
                {
                    nrLDPC_bnProcPc_BG2_Z32_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProcPc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_RES, p_procBuf);
#endif

#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProc);
#endif
        nrLDPC_bnProc(p_lut, p_procBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC_RES, p_procBuf);
#endif

        // BN results to CN processing buffer
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bn2cnProcBuf);
#endif
        if (BG == 1)
        {
            nrLDPC_bn2cnProcBuf_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            nrLDPC_bn2cnProcBuf_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bn2cnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, p_procBuf);
#endif

        // Parity Check
#ifdef NR_LDPC_ENABLE_PARITY_CHECK
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cnProcPc);
#endif
        if (BG == 1)
        {
            pcRes = nrLDPC_cnProcPc_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            pcRes = nrLDPC_cnProcPc_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cnProcPc);
#endif
#endif
    }

    // Last iteration
    if ( (i < numMaxIter) && (pcRes != 0) )
    {
        // Increase iteration counter
        i++;

        // CN processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cnProc);
#endif
    if (BG==1)
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z384_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z384_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z352_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z352_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z320_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z320_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z288_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z288_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z256_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z256_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z240_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z240_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z224_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z224_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z208_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z208_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z192_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z192_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z176_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z176_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z144_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z144_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z128_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z128_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z120_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG1_Z120_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z112_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z112_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z104_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z104_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z96_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z96_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z88_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z88_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z80_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z80_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z72_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z72_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z64_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z64_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z60_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z60_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z56_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z56_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z52_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z52_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z48_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z48_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z44_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z44_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z40_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z40_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z36_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z36_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z32_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z32_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z30_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z30_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z28_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z28_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z26_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z26_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z24_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z24_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z22_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z22_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z20_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z20_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z18_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z18_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z16_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z16_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z15_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z15_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z14_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z14_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z13_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z13_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z12_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z12_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z11_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z11_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z10_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z10_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z9_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z9_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z8_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z8_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z7_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z7_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z6_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z6_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z5_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z5_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z4_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z4_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z3_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z3_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG1_Z2_R13_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG1_Z2_R13_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    } //end if
    else
    {
        switch (Z)
        {
            case 384:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z384_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z384_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 352:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z352_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z352_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 320:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z320_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z320_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 288:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z288_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z288_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 256:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z256_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z256_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 240:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z240_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z240_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 224:
            {

                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z224_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z224_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 208:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z208_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z208_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 192:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z192_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z192_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 176:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 160:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z176_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z176_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 144:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z144_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z144_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 128:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z128_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                 nrLDPC_cnProc_BG2_Z128_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 120:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z120_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
            #else
             nrLDPC_cnProc_BG2_Z120_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 112:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z112_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z112_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }


            case 104:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z104_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z104_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 96:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z96_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z96_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 88:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z88_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z88_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 80:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z80_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z80_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 72:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z72_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z72_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 64:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z64_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z64_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 60:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z60_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z60_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 56:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z56_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z56_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 52:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z52_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z52_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 48:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z48_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z48_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 44:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z44_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z44_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 40:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z40_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z40_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 36:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z36_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z36_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 32:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z32_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z32_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 30:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z30_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z30_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 28:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z28_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z28_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 26:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z26_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z26_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 24:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z24_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z24_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 22:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z22_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z22_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 20:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z20_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z20_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 18:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z18_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z18_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 16:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z16_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z16_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 15:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z15_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z15_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 14:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z14_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z14_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 13:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z13_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z13_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 12:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z12_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z12_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 11:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z11_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z11_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 10:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z10_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z10_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 9:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z9_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z9_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 8:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z8_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z8_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 7:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z7_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z7_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 6:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z6_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z6_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 5:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z5_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z5_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 4:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z4_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z4_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }

            case 3:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z3_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z3_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
            case 2:
            {
                #ifdef __AVX512BW__
                nrLDPC_cnProc_BG2_Z2_R15_AVX512(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #else
                nrLDPC_cnProc_BG2_Z2_R15_AVX2(p_procBuf->cnProcBuf,p_procBuf->cnProcBufRes);
                #endif                                                                                                                                                                        
                break;
            }
        }
    }//end of else

#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC_RES, p_procBuf);
#endif

        // Send CN results back to BNs
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cn2bnProcBuf);
#endif
        if (BG == 1)
        {
            nrLDPC_cn2bnProcBuf_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            nrLDPC_cn2bnProcBuf_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cn2bnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC, p_procBuf);
#endif

        // BN Processing
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProcPc);
#endif
        nrLDPC_bnProcPc(p_lut, p_procBuf, Z);
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProcPc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_LLR_RES, p_procBuf);
#endif

        // If parity check not enabled, no need to send the BN proc results
        // back to CNs
#ifdef NR_LDPC_ENABLE_PARITY_CHECK
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bnProc);
#endif

        if(BG==1){

            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG1_Z384_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG1_Z256_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG1_Z192_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG1_Z128_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG1_Z96_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG1_Z64_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 48:
                {
                    nrLDPC_bnProcPc_BG1_Z48_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 32:
                {
                    nrLDPC_bnProcPc_BG1_Z32_R13_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        else{
            switch (Z)
            {
                case 384:
                {
                    nrLDPC_bnProcPc_BG2_Z384_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                case 256:
                {
                    nrLDPC_bnProcPc_BG2_Z256_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 192:
                {
                    nrLDPC_bnProcPc_BG2_Z192_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 128:
                {
                    nrLDPC_bnProcPc_BG2_Z128_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 96:
                {
                    nrLDPC_bnProcPc_BG2_Z96_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 64:
                {
                    nrLDPC_bnProcPc_BG2_Z64_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 48:
                {
                    nrLDPC_bnProcPc_BG2_Z48_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }

                case 32:
                {
                    nrLDPC_bnProcPc_BG2_Z32_R15_AVX2(p_procBuf->bnProcBuf,p_procBuf->bnProcBufRes,  p_procBuf->llrRes , p_procBuf->llrProcBuf );                                                                                                                                                                       
                    break;
                }
                default :
                {
                    nrLDPC_bnProcPc(p_lut, p_procBuf, Z);                                                                                                                                                                       
                    break;
                }
               
            }
        }
        

#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bnProc);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_BN_PROC_RES, p_procBuf);
#endif

        // BN results to CN processing buffer
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->bn2cnProcBuf);
#endif
        if (BG == 1)
        {
            nrLDPC_bn2cnProcBuf_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            nrLDPC_bn2cnProcBuf_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->bn2cnProcBuf);
#endif

#ifdef NR_LDPC_DEBUG_MODE
        nrLDPC_debug_writeBuffer2File(nrLDPC_buffers_CN_PROC, p_procBuf);
#endif

        // Parity Check
#ifdef NR_LDPC_PROFILER_DETAIL
        start_meas(&p_profiler->cnProcPc);
#endif
        if (BG == 1)
        {
            pcRes = nrLDPC_cnProcPc_BG1(p_lut, p_procBuf, Z);
        }
        else
        {
            pcRes = nrLDPC_cnProcPc_BG2(p_lut, p_procBuf, Z);
        }
#ifdef NR_LDPC_PROFILER_DETAIL
        stop_meas(&p_profiler->cnProcPc);
#endif
#endif
    }

    // If maximum number of iterations reached an PC still fails increase number of iterations
    // Thus, i > numMaxIter indicates that PC has failed

#ifdef NR_LDPC_ENABLE_PARITY_CHECK
    if (pcRes != 0)
    {
        i++;
    }
#endif

    // Assign results from processing buffer to output
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llrRes2llrOut);
#endif
    nrLDPC_llrRes2llrOut(p_lut, p_llrOut, p_procBuf, Z, BG);
#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llrRes2llrOut);
#endif

    // Hard-decision
#ifdef NR_LDPC_PROFILER_DETAIL
    start_meas(&p_profiler->llr2bit);
#endif
    if (outMode == nrLDPC_outMode_BIT)
    {
        nrLDPC_llr2bitPacked(p_out, p_llrOut, numLLR);
    }
    else if (outMode == nrLDPC_outMode_BITINT8)
    {
        nrLDPC_llr2bit(p_out, p_llrOut, numLLR);
    }

#ifdef NR_LDPC_PROFILER_DETAIL
    stop_meas(&p_profiler->llr2bit);
#endif

    return i;
}





