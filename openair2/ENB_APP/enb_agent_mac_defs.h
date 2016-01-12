/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2016 Eurecom

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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file enb_agent_mac_defs.h
 * \brief enb agent - mac interface primitives
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 * \mail x.foukas@sms.ed.ac.uk
 */

#ifndef __ENB_AGENT_MAC_PRIMITIVES_H__
#define __ENB_AGENT_MAC_PRIMITIVES_H__

#include "enb_agent_defs.h"

/* ENB AGENT-MAC Interface */
typedef struct {
  msg_context_t *agent_ctxt;

  /// Inform the controller about the scheduling requests received during the subframe
  void (*enb_agent_send_sr_info)(mid_t mod_id, msg_context_t *context);

  /*TODO: Fill in with the rest of the MAC layer technology specific callbacks (UL/DL scheduling, RACH info etc)*/

} AGENT_MAC_xface;

#endif
