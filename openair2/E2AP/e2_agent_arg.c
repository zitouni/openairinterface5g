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
#include "e2_agent_paramdef.h"
#include "e2_agent_arg.h"
#include "common/config/config_userapi.h"

/* Helper function to trim whitespace from string */
static char* trim_whitespace(char* str)
{
  /* Skip leading whitespace */
  while (*str == ' ' || *str == '\t')
    str++;

  if (*str == 0)
    return str;

  /* Trim trailing whitespace */
  char* end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t'))
    end--;
  *(end + 1) = '\0';

  return str;
}

/* Function to validate required parameters */
static bool validate_required_params(paramdef_t* params)
{
  return config_isparamset(params, E2AGENT_CONFIG_SMDIR_IDX) && config_isparamset(params, E2AGENT_CONFIG_IP_IDX)
         && config_isparamset(params, E2AGENT_CLIENT_IP_IDX);
}

/* Function to copy basic configuration parameters */
static void copy_basic_config(e2_agent_args_t* dst, paramdef_t* params)
{
  if (params[E2AGENT_CONFIG_SMDIR_IDX].strptr != NULL) {
    dst->sm_dir = *params[E2AGENT_CONFIG_SMDIR_IDX].strptr;
  }

  if (params[E2AGENT_CLIENT_IP_IDX].strptr != NULL) {
    dst->client_ip = *params[E2AGENT_CLIENT_IP_IDX].strptr;
  }
}

/* Function to store a single IP address */
static void store_ip_address(e2_agent_args_t* dst, const char* ip)
{
  if (dst->ric_ip_list.num_ric_addresses >= MAX_RIC_IP_ADDRESSES) {
    printf("Warning: Maximum number of RIC IP addresses reached (%d)\n", MAX_RIC_IP_ADDRESSES);
    return;
  }

  dst->ric_ip_list.ric_ip_addresses[dst->ric_ip_list.num_ric_addresses] = strdup(ip);
  printf("RIC IP %d: %s\n",
         dst->ric_ip_list.num_ric_addresses + 1,
         dst->ric_ip_list.ric_ip_addresses[dst->ric_ip_list.num_ric_addresses]);
  dst->ric_ip_list.num_ric_addresses++;
}

/* Function to process IP list string */
static void process_ip_list(e2_agent_args_t* dst, const char* ip_list_str)
{
  char* ip_str = strdup(ip_list_str);
  char* saveptr = NULL;
  char* token = strtok_r(ip_str, ",;", &saveptr);

  while (token != NULL && dst->ric_ip_list.num_ric_addresses < MAX_RIC_IP_ADDRESSES) {
    char* trimmed_ip = trim_whitespace(token);
    if (strlen(trimmed_ip) > 0) {
      store_ip_address(dst, trimmed_ip);
    }
    token = strtok_r(NULL, ",;", &saveptr);
  }

  free(ip_str);
}

/* Function to validate final configuration */
static bool validate_configuration(e2_agent_args_t* dst)
{
  bool valid = true;

  if (dst->ric_ip_list.num_ric_addresses == 0) {
    printf("Warning: No RIC IP addresses configured\n");
    valid = false;
  } else {
    printf("Successfully configured %d RIC IP address(es)\n", dst->ric_ip_list.num_ric_addresses);
  }

  if (dst->sm_dir == NULL) {
    printf("Warning: No SM directory configured\n");
    valid = false;
  }

  if (dst->client_ip == NULL) {
    printf("Warning: No client IP configured\n");
    valid = false;
  }

  return valid;
}

/* Main configuration function */
e2_agent_args_t RCconfig_NR_E2agent(void)
{
  printf("[E2 AGENT] Reading configuration...\n");

  /* Initialize configuration parameters array with predefined descriptions */
  paramdef_t e2agent_params[] = E2AGENT_PARAMS_DESC;

  /* Initialize return structure */
  e2_agent_args_t dst = {.enabled = false, .ric_ip_list = {.num_ric_addresses = 0}};

  /* Attempt to read configuration from the config file */
  int ret = config_get(config_get_if(), e2agent_params, sizeofArray(e2agent_params), CONFIG_STRING_E2AGENT);
  if (ret < 0) {
    printf(
        "[E2 AGENT] Configuration file does not contain a \"%s\" section, "
        "applying default parameters\n",
        CONFIG_STRING_E2AGENT);
    return dst;
  }

  /* Validate required parameters */
  dst.enabled = validate_required_params(e2agent_params);
  if (!dst.enabled) {
    printf("[E2 AGENT] DISABLED (missing required parameters: .%s.{%s,%s,%s})\n",
           CONFIG_STRING_E2AGENT,
           E2AGENT_CONFIG_IP,
           E2AGENT_CLIENT_IP,
           E2AGENT_CONFIG_SMDIR);
    return dst;
  }

  /* Copy basic configuration */
  copy_basic_config(&dst, e2agent_params);

  /* Process RIC IP addresses if specified */
  if (e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr != NULL) {
    process_ip_list(&dst, *e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr);
  }

  /* Perform final validation */
  dst.enabled = validate_configuration(&dst);

  printf("[E2 AGENT] Configuration %s\n", dst.enabled ? "complete" : "failed");
  return dst;
}

/* Function to clean up allocated memory in e2_agent_args_t structure */
void cleanup_e2_agent_args(e2_agent_args_t* args)
{
  if (args) {
    /* Free each allocated IP address string */
    for (int i = 0; i < args->ric_ip_list.num_ric_addresses; i++) {
      free(args->ric_ip_list.ric_ip_addresses[i]);
    }

    /* Reset the counter */
    args->ric_ip_list.num_ric_addresses = 0;

    /* Note: client_ip and sm_dir are not freed as they typically point to static config data */
  }
}