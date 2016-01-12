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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file enb_agent_handler.c
 * \brief enb agent tx and rx message handler 
 * \author Navid Nikaein and Xenofon Foukas 
 * \date 2016
 * \version 0.1
 */


#include "enb_agent_common.h"
#include "enb_agent_mac.h"
#include "log.h"

#include "assertions.h"

enb_agent_message_decoded_callback messages_callback[][3] = {
  {enb_agent_hello, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG*/
  {enb_agent_echo_reply, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG*/ //Must add handler when receiving echo reply
  {enb_agent_mac_handle_stats, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REQUEST_MSG*/
  {0,0,0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG*/

};

enb_agent_message_destruction_callback message_destruction_callback[] = {
  enb_agent_destroy_hello,
  enb_agent_destroy_echo_request,
  enb_agent_destroy_echo_reply,
  enb_agent_mac_destroy_stats_request,
  enb_agent_mac_destroy_stats_reply,
  enb_agent_mac_destroy_sr_info,
};

static const char *enb_agent_direction2String[] = {
  "", /* not_set  */
  "originating message", /* originating message */
  "successfull outcome", /* successfull outcome */
  "unsuccessfull outcome", /* unsuccessfull outcome */
};


Protocol__ProgranMessage* enb_agent_handle_message (mid_t mod_id,
						    uint8_t *data, 
						    uint32_t size){
  
  Protocol__ProgranMessage *decoded_message, *reply_message;
  err_code_t err_code;
  DevAssert(data != NULL);

  if (enb_agent_deserialize_message(data, size, &decoded_message) < 0) {
    err_code= PROTOCOL__PROGRAN_ERR__MSG_DECODING;
    goto error; 
  }
  
  if ((decoded_message->msg_case > sizeof(messages_callback) / (3*sizeof(enb_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__PROGRAN_DIRECTION__UNSUCCESSFUL_OUTCOME)){
    err_code= PROTOCOL__PROGRAN_ERR__MSG_NOT_HANDLED;
      goto error;
  }
    
  if (messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1] == NULL) {
    err_code= PROTOCOL__PROGRAN_ERR__MSG_NOT_SUPPORTED;
    goto error;

  }

  err_code = ((*messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));
  if ( err_code < 0 ){
    goto error;
  }
  
  protocol__progran_message__free_unpacked(decoded_message, NULL);

  return reply_message;
  
error:
  LOG_E(ENB_AGENT,"errno %d occured\n",err_code);
  return NULL;

}



void * enb_agent_pack_message(Protocol__ProgranMessage *msg, 
			      uint32_t * size){

  void * buffer;
  err_code_t err_code = PROTOCOL__PROGRAN_ERR__NO_ERR;
  
  if (enb_agent_serialize_message(msg, &buffer, size) < 0 ) {
    err_code = PROTOCOL__PROGRAN_ERR__MSG_ENCODING;
    goto error;
  }
  
  // free the msg --> later keep this in the data struct and just update the values
  //TODO call proper destroy function
  err_code = ((*message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(ENB_AGENT,"Serilized the enb mac stats reply (size %d)\n", *size);
  
  return buffer;
  
 error : 
  LOG_E(ENB_AGENT,"errno %d occured\n",err_code);
  
  return NULL;   
}

Protocol__ProgranMessage *enb_agent_handle_timed_task(void *args) {
  err_code_t err_code;
  enb_agent_timer_args_t *timer_args = (enb_agent_timer_args_t *) args;

  Protocol__ProgranMessage *timed_task, *reply_message;
  timed_task = timer_args->msg;
   err_code = ((*messages_callback[timed_task->msg_case-1][timed_task->msg_dir-1])(timer_args->mod_id, (void *) timed_task, &reply_message));
  if ( err_code < 0 ){
    goto error;
  }

  return reply_message;
  
 error:
  LOG_E(ENB_AGENT,"errno %d occured\n",err_code);
  return NULL;
}

Protocol__ProgranMessage* enb_agent_process_timeout(long timer_id, void* timer_args){
    
  struct enb_agent_timer_element_s *found = get_timer_entry(timer_id);
 
  if (found == NULL ) goto error;
  LOG_I(ENB_AGENT, "Found the entry (%p): timer_id is 0x%lx  0x%lx\n", found, timer_id, found->timer_id);
  
  if (timer_args == NULL)
    LOG_W(ENB_AGENT,"null timer args\n");
  
  return found->cb(timer_args);

 error:
  LOG_E(ENB_AGENT, "can't get the timer element\n");
  return TIMER_ELEMENT_NOT_FOUND;
}

err_code_t enb_agent_destroy_progran_message(Protocol__ProgranMessage *msg) {
  return ((*message_destruction_callback[msg->msg_case-1])(msg));
}
