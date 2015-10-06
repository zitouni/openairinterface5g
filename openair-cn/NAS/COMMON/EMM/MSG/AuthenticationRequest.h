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
#include "MessageType.h"
#include "NasKeySetIdentifier.h"
#include "AuthenticationParameterRand.h"
#include "AuthenticationParameterAutn.h"

#ifndef AUTHENTICATION_REQUEST_H_
#define AUTHENTICATION_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define AUTHENTICATION_REQUEST_MINIMUM_LENGTH ( \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_RAND_MINIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_AUTN_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define AUTHENTICATION_REQUEST_MAXIMUM_LENGTH ( \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_RAND_MAXIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_AUTN_MAXIMUM_LENGTH )


/*
 * Message name: Authentication request
 * Description: This message is sent by the network to the UE to initiate authentication of the UE identity. See table 8.2.7.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct authentication_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator            protocoldiscriminator:4;
  SecurityHeaderType               securityheadertype:4;
  MessageType                      messagetype;
  NasKeySetIdentifier              naskeysetidentifierasme;
  AuthenticationParameterRand      authenticationparameterrand;
  AuthenticationParameterAutn      authenticationparameterautn;
} authentication_request_msg;

int decode_authentication_request(authentication_request_msg *authenticationrequest, uint8_t *buffer, uint32_t len);

int encode_authentication_request(authentication_request_msg *authenticationrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(AUTHENTICATION_REQUEST_H_) */

