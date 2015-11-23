/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MS_NETWORK_FEATURE_SUPPORT_H_
#define MS_NETWORK_FEATURE_SUPPORT_H_

#define MS_NETWORK_FEATURE_SUPPORT_MINIMUM_LENGTH 3
#define MS_NETWORK_FEATURE_SUPPORT_MAXIMUM_LENGTH 10

typedef struct MsNetworkFeatureSupport_tag {
  uint8_t spare_bits:3;
  uint8_t extended_periodic_timers:1;
} MsNetworkFeatureSupport;

int encode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ms_network_feature_support_xml(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei);

#endif /* MS NETWORK CAPABILITY_H_ */

