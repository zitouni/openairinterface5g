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
#include <string.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "VoiceDomainPreferenceAndUeUsageSetting.h"

int decode_voice_domain_preference_and_ue_usage_setting(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  memset(voicedomainpreferenceandueusagesetting, 0, sizeof(VoiceDomainPreferenceAndUeUsageSetting));
  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);

  voicedomainpreferenceandueusagesetting->ue_usage_setting        = (*(buffer + decoded) >> 2) & 0x1;
  voicedomainpreferenceandueusagesetting->voice_domain_for_eutran = *(buffer + decoded) & 0x3;
  decoded++;

#if defined (NAS_DEBUG)
  dump_voice_domain_preference_and_ue_usage_setting_xml(voicedomainpreferenceandueusagesetting, iei);
#endif
  return decoded;
}
int encode_voice_domain_preference_and_ue_usage_setting(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_voice_domain_preference_and_ue_usage_setting_xml(voicedomainpreferenceandueusagesetting, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = 0x00 |
                        (voicedomainpreferenceandueusagesetting->ue_usage_setting << 2) |
                        voicedomainpreferenceandueusagesetting->voice_domain_for_eutran;
  encoded++;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_voice_domain_preference_and_ue_usage_setting_xml(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei)
{
  printf("<Voice domain preference and UE usage setting>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <UE_USAGE_SETTING>%u</UE_USAGE_SETTING>\n", voicedomainpreferenceandueusagesetting->ue_usage_setting);
  printf("    <VOICE_DOMAIN_FOR_EUTRAN>%u</VOICE_DOMAIN_FOR_EUTRAN>\n", voicedomainpreferenceandueusagesetting->voice_domain_for_eutran);
  printf("</Voice domain preference and UE usage setting>\n");
}

