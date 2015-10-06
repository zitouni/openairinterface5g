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

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "KsiAndSequenceNumber.h"
#include "ShortMac.h"

#ifndef SERVICE_REQUEST_H_
#define SERVICE_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SERVICE_REQUEST_MINIMUM_LENGTH ( \
    KSI_AND_SEQUENCE_NUMBER_MINIMUM_LENGTH + \
    SHORT_MAC_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SERVICE_REQUEST_MAXIMUM_LENGTH ( \
    KSI_AND_SEQUENCE_NUMBER_MAXIMUM_LENGTH + \
    SHORT_MAC_MAXIMUM_LENGTH )


/*
 * Message name: Service request
 * Description: This message is sent by the UE to the network to request the establishment of a NAS signalling connection and of the radio and S1 bearers. Its structure does not follow the structure of a standard layer 3 message. See table 8.2.25.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct service_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator     protocoldiscriminator:4;
  SecurityHeaderType        securityheadertype:4;
  KsiAndSequenceNumber      ksiandsequencenumber;
  ShortMac                  messageauthenticationcode;
} service_request_msg;

int decode_service_request(service_request_msg *servicerequest, uint8_t *buffer, uint32_t len);

int encode_service_request(service_request_msg *servicerequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SERVICE_REQUEST_H_) */

