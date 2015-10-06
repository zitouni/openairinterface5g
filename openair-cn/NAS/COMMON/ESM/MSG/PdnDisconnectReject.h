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
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "EsmCause.h"
#include "ProtocolConfigurationOptions.h"

#ifndef PDN_DISCONNECT_REJECT_H_
#define PDN_DISCONNECT_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define PDN_DISCONNECT_REJECT_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define PDN_DISCONNECT_REJECT_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define PDN_DISCONNECT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum pdn_disconnect_reject_iei_tag {
  PDN_DISCONNECT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} pdn_disconnect_reject_iei;

/*
 * Message name: PDN disconnect reject
 * Description: This message is sent by the network to the UE to reject release of a PDN connection. See table 8.3.21.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct pdn_disconnect_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator              protocoldiscriminator:4;
  EpsBearerIdentity                  epsbeareridentity:4;
  ProcedureTransactionIdentity       proceduretransactionidentity;
  MessageType                        messagetype;
  EsmCause                           esmcause;
  /* Optional fields */
  uint32_t                           presencemask;
  ProtocolConfigurationOptions       protocolconfigurationoptions;
} pdn_disconnect_reject_msg;

int decode_pdn_disconnect_reject(pdn_disconnect_reject_msg *pdndisconnectreject, uint8_t *buffer, uint32_t len);

int encode_pdn_disconnect_reject(pdn_disconnect_reject_msg *pdndisconnectreject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(PDN_DISCONNECT_REJECT_H_) */

