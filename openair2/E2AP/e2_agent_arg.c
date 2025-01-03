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

e2_agent_args_t RCconfig_NR_E2agent(void)
{
  /* Initialize configuration parameters array with predefined descriptions */
  paramdef_t e2agent_params[] = E2AGENT_PARAMS_DESC;
  /* Attempt to read configuration from the config file */
  int ret = config_get(config_get_if(), e2agent_params, sizeofArray(e2agent_params), CONFIG_STRING_E2AGENT);
  if (ret < 0) {
    printf("configuration file does not contain a \"%s\" section, applying default parameters from FlexRIC\n",
           CONFIG_STRING_E2AGENT);
    /* Return zero-initialized structure if config file reading fails */
    return (e2_agent_args_t){0};
  }
  /* Check if all required parameters are set in the configuration */
  bool enabled = config_isparamset(e2agent_params, E2AGENT_CONFIG_SMDIR_IDX)
                 && config_isparamset(e2agent_params, E2AGENT_CONFIG_IP_IDX)
                 && config_isparamset(e2agent_params, E2AGENT_CLIENT_IP_IDX);
  /* Initialize the destination structure with default values */
  e2_agent_args_t dst = {.enabled = enabled, .ric_ip_list = {.num_ric_addresses = 0}};
  // e2_agent_args_t dst = {.enabled = enabled};
  if (!enabled) {
    printf("E2 agent is DISABLED (for activation, define .%s.{%s,%s,%s} parameters)\n",
           CONFIG_STRING_E2AGENT,
           E2AGENT_CONFIG_IP,
           E2AGENT_CLIENT_IP,
           E2AGENT_CONFIG_SMDIR);
    return dst;
  }
  /* Copy SM directory path if specified */
  if (e2agent_params[E2AGENT_CONFIG_SMDIR_IDX].strptr != NULL)
    dst.sm_dir = *e2agent_params[E2AGENT_CONFIG_SMDIR_IDX].strptr;

  /* Copy client IP address if specified */
  if (e2agent_params[E2AGENT_CLIENT_IP_IDX].strptr != NULL)
    dst.client_ip = *e2agent_params[E2AGENT_CLIENT_IP_IDX].strptr;

  // if (e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr != NULL)
  //   dst.server_ip = *e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr;

  /* Process RIC IP addresses */
  if (e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr != NULL) {
    /* Create a copy of the IP string for tokenization */
    char *ip_str = strdup(*e2agent_params[E2AGENT_CONFIG_IP_IDX].strptr);
    char *saveptr = NULL;

    /* Start tokenizing the IP string using comma or semicolon as delimiters */
    char *token = strtok_r(ip_str, ",;", &saveptr);

    /* Process each IP address token */
    while (token != NULL && dst.ric_ip_list.num_ric_addresses < MAX_RIC_IP_ADDRESSES) {
      /* Remove leading whitespace characters */
      while (*token == ' ' || *token == '\t')
        token++;

      /* Remove trailing whitespace characters */
      char *end = token + strlen(token) - 1;
      while (end > token && (*end == ' ' || *end == '\t'))
        end--;
      *(end + 1) = '\0';

      /* Store valid IP addresses */
      if (strlen(token) > 0) {
        /* Allocate memory and store the IP address */
        dst.ric_ip_list.ric_ip_addresses[dst.ric_ip_list.num_ric_addresses] = strdup(token);

        /* Log the stored IP address */
        printf("RIC IP %d: %s\n",
               dst.ric_ip_list.num_ric_addresses + 1,
               dst.ric_ip_list.ric_ip_addresses[dst.ric_ip_list.num_ric_addresses]);

        /* Increment the counter of stored IP addresses */
        dst.ric_ip_list.num_ric_addresses++;
      }

      /* Get next token */
      token = strtok_r(NULL, ",;", &saveptr);
    }

    /* Free the temporary IP string */
    free(ip_str);

    /* Validate that at least one IP address was found */
    if (dst.ric_ip_list.num_ric_addresses == 0) {
      printf("Warning: No valid RIC IP addresses found in configuration\n");
      dst.enabled = false;
    } else {
      printf("Successfully configured %d RIC IP address(es)\n", dst.ric_ip_list.num_ric_addresses);
    }
  }

  /* Perform final validation checks */
  if (dst.ric_ip_list.num_ric_addresses == 0) {
    printf("Warning: No RIC IP addresses configured\n");
    dst.enabled = false;
  }

  if (dst.sm_dir == NULL) {
    printf("Warning: No SM directory configured\n");
    dst.enabled = false;
  }

  if (dst.client_ip == NULL) {
    printf("Warning: No client IP configured\n");
    dst.enabled = false;
  }

  /* Return the configured structure */
  return dst;
}

/* Function to clean up allocated memory in e2_agent_args_t structure */
void cleanup_e2_agent_args(e2_agent_args_t *args)
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