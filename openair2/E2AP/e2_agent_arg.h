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

#ifndef E2_AGENT_ARGS_H
#define E2_AGENT_ARGS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "e2_agent_paramdef.h" // Include this to use ric_ip_config_t
// Wrapper for OAI
typedef struct {
  ric_ip_config_t ric_ip_list;
  char *client_ip;
  char *sm_dir;
  bool enabled;
} e2_agent_args_t;

// typedef struct{
//   // const char *ip;
//   const char *server_ip;
//   const char *client_ip;
//   const char *sm_dir;
//   const bool enabled;
// } e2_agent_args_t;

e2_agent_args_t RCconfig_NR_E2agent(void);
void cleanup_e2_agent_args(e2_agent_args_t *args);

#endif
