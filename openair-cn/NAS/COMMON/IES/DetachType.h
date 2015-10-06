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

#ifndef DETACH_TYPE_H_
#define DETACH_TYPE_H_

#define DETACH_TYPE_MINIMUM_LENGTH 1
#define DETACH_TYPE_MAXIMUM_LENGTH 1

typedef struct DetachType_tag {
#define DETACH_TYPE_NORMAL_DETACH 0
#define DETACH_TYPE_SWITCH_OFF    1
  uint8_t  switchoff:1;
#define DETACH_TYPE_EPS     0b001
#define DETACH_TYPE_IMSI    0b010
#define DETACH_TYPE_EPS_IMSI    0b011
#define DETACH_TYPE_RESERVED_1    0b110
#define DETACH_TYPE_RESERVED_2    0b111
  uint8_t  typeofdetach:3;
} DetachType;

int encode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_detach_type_xml(DetachType *detachtype, uint8_t iei);

uint8_t encode_u8_detach_type(DetachType *detachtype);

int decode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_detach_type(DetachType *detachtype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* DETACH TYPE_H_ */

