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

#ifndef E2_AGENT_PARAMDEF_H_
#define E2_AGENT_PARAMDEF_H_

/* E2 Agent configuration */
#define CONFIG_STRING_E2AGENT "e2_agent"

#define E2AGENT_CONFIG_IP "near_ric_ip_addr"
#define E2AGENT_CLIENT_IP "local_ip_addr"
#define E2AGENT_CONFIG_SMDIR "sm_dir"
#define MAX_RIC_IP_ADDRESSES 20 // Maximum number of RIC IP addresses supported

// Structure to hold multiple RIC IP addresses
typedef struct {
  char* ric_ip_addresses[MAX_RIC_IP_ADDRESSES];
  int num_ric_addresses;
} ric_ip_config_t;

static const char* const e2agent_config_ip_default = "127.0.0.1";
static const char* const e2agent_client_ip_default = "127.0.0.1";
static const char* const e2agent_config_smdir_default = ".";

#define E2AGENT_PARAMS_DESC                                                                                                  \
  {                                                                                                                          \
      {E2AGENT_CONFIG_IP, "RIC IP address", 0, strptr : NULL, defstrval : (char*)e2agent_config_ip_default, TYPE_STRING, 0}, \
      {                                                                                                                      \
        E2AGENT_CLIENT_IP,                                                                                                   \
        "RIC Client IP address",                                                                                             \
        0,                                                                                                                   \
        strptr : NULL,                                                                                                       \
        defstrval : (char*)e2agent_client_ip_default,                                                                        \
        TYPE_STRING,                                                                                                         \
        0                                                                                                                    \
      },                                                                                                                     \
      {                                                                                                                      \
        E2AGENT_CONFIG_SMDIR,                                                                                                \
        "Directory with SMs to load",                                                                                        \
        0,                                                                                                                   \
        strptr : NULL,                                                                                                       \
        defstrval : (char*)e2agent_config_smdir_default,                                                                     \
        TYPE_STRING,                                                                                                         \
        0                                                                                                                    \
      },                                                                                                                     \
  }

#define E2AGENT_CONFIG_IP_IDX 0
#define E2AGENT_CLIENT_IP_IDX 1
#define E2AGENT_CONFIG_SMDIR_IDX 2

#endif
