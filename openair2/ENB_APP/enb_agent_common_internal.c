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

/*! \file enb_agent_common_internal.c
 * \brief internal functions for common message primitves and utilities 
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include <stdlib.h>
#include <string.h>

#include "enb_agent_common_internal.h"
#include "enb_agent_mac_internal.h"

int apply_reconfiguration_policy(mid_t mod_id, const char *policy, size_t policy_length) {

  yaml_parser_t parser;
  yaml_event_t event;

  int done = 0;
  int mapping_started = 0;

  LOG_I(ENB_APP, "Time to apply a new policy \n");

  yaml_parser_initialize(&parser);

  yaml_parser_set_input_string(&parser, policy, strlen(policy));

  while (!done) {
    if (!yaml_parser_parse(&parser, &event))
      goto error;
 
    switch (event.type) {
    case YAML_STREAM_START_EVENT:
    case YAML_STREAM_END_EVENT:
    case YAML_DOCUMENT_START_EVENT:
    case YAML_DOCUMENT_END_EVENT:
      break;
    case YAML_MAPPING_START_EVENT:
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      // Check the system name and call the proper handler
      if (strcmp(event.data.scalar.value, "mac") == 0) {
	LOG_D(ENB_APP, "This is intended for the mac system\n");
	// Call the mac handler
	if (parse_mac_config(mod_id, &parser) == -1) {
	  goto error;
	}
      } else if (strcmp(event.data.scalar.value, "rlc") == 0) {
	// Call the RLC handler
	LOG_D(ENB_APP, "This is intended for the rlc system\n");
	// TODO : Just skip it for now
	if (skip_system_section(&parser) == -1) {
	  goto error;
	}
      } else if (strcmp(event.data.scalar.value, "pdcp") == 0) {
	// Call the PDCP handler
	LOG_D(ENB_APP, "This is intended for the pdcp system\n");
	// TODO : Just skip it for now
	if (skip_system_section(&parser) == -1) {
	  goto error;
	}
      } else if (strcmp(event.data.scalar.value, "rrc") == 0) {
	// Call the RRC handler
	LOG_D(ENB_APP, "This is intended for the rrc system\n");
	// TODO : Just skip it for now
	if (skip_system_section(&parser) == -1) {
	  goto error;
	}
      } else {
	goto error;
      }
      break;
    default:
      // We are not expecting to find any other type of event at this level
      // of the hierarchy
      yaml_event_delete(&event);
      goto error;
    }
    
    done = (event.type == YAML_STREAM_END_EVENT);

    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_parser_delete(&parser);
  return -1;

}

int skip_system_section(yaml_parser_t *parser) {
  yaml_event_t event;
  
  int done = 0;

  int sequence_started = 0;
  int mapping_started = 0;

  while (!done) {
    if (!yaml_parser_parse(parser, &event))
      goto error;
   
    switch (event.type) {
      
      case YAML_SEQUENCE_START_EVENT:
      LOG_D(ENB_APP, "A sequence just started as expected\n");
      sequence_started = 1;
      break;
    case YAML_SEQUENCE_END_EVENT:
      LOG_D(ENB_APP, "A sequence ended\n");
      sequence_started = 0;
      break;
    case YAML_MAPPING_START_EVENT:
      if (!sequence_started) {
	goto error;
      }
      LOG_D(ENB_APP, "A mapping started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      if (!mapping_started) {
	goto error;
      }
      LOG_D(ENB_APP, "A mapping ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Entered a subsystem section. Let's skip it
      if (skip_subsystem_section(parser) == -1) {
	goto error;
      }
    }
    
    done = (event.type == YAML_SEQUENCE_END_EVENT);

    yaml_event_delete(&event);
    
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
}

int skip_subsystem_section(yaml_parser_t *parser) {
  
  yaml_event_t event;

  int done = 0;
  int mapping_started = 0;

  while (!done) {
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Check what key needs to be set
      if (strcmp(event.data.scalar.value, "behavior") == 0) {
	LOG_D(ENB_APP, "Skipping the behavior attribute\n");
	yaml_event_delete(&event);
	if (!yaml_parser_parse(parser, &event)) {
	  goto error;
	}
	if (event.type == YAML_SCALAR_EVENT) {
	  break;
	} else {
	  goto error;
	}
      } else if (strcmp(event.data.scalar.value, "parameters") == 0) {
	LOG_D(ENB_APP, "Skipping the parameters for this subsystem\n");
	if (skip_subsystem_parameters_config(parser) == -1) {
	  goto error;
	}
      }
      break;
    default:
      goto error;
    }
    
    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
}

int skip_subsystem_parameters_config(yaml_parser_t *parser) {
  yaml_event_t event;
  
  void *param;
  
  int done = 0;
  int mapping_started = 0;

  while (!done) {
    
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
      // We are expecting a mapping of parameters
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Check what key needs to be set
      LOG_D(ENB_APP, "Skipping parameter %s\n", event.data.scalar.value);
      if (skip_parameter_modification(parser) == -1 ) {
	goto error;
      }
      break;
    default:
      goto error;
    }

    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;
  
 error:
  yaml_event_delete(&event);
  return -1;
}


int skip_parameter_modification(yaml_parser_t *parser) {
  yaml_event_t event;
  
  int done = 0;
  int is_array = 0;
  int i = 0;

  while (!done) {
    if (!yaml_parser_parse(parser, &event)) {
      goto error;
    }
    
    // Expecting either a scalar or a sequence
    switch (event.type) {
    case YAML_SEQUENCE_START_EVENT:
      is_array = 1;
      break;
    case YAML_SCALAR_EVENT:
      if ((strcmp(event.data.scalar.tag, YAML_INT_TAG) == 0) ||
	  (strcmp(event.data.scalar.tag, YAML_FLOAT_TAG) == 0) ||
	  (strcmp(event.data.scalar.tag, YAML_STR_TAG) == 0) ||
	  (strcmp(event.data.scalar.tag, YAML_BOOL_TAG) == 0)) {
	// Do nothing
      } else {
	// No other type is supported at the moment, so it should be considered an error
	// if we reach here
	goto error;
      }
      if (is_array) {
	i++;
      } else {
	done = 1;
      }
      break;
    case YAML_SEQUENCE_END_EVENT:
      done = 1;
      break;
    default:
      goto error;
    }
    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
}

int apply_parameter_modification(void *parameter, yaml_parser_t *parser) {
  yaml_event_t event;
  
  int done = 0;
  int is_array = 0;
  int i = 0;

  char *endptr;

  while (!done) {
    if (!yaml_parser_parse(parser, &event)) {
      goto error;
    }
    
    // Expecting either a scalar or a sequence
    switch (event.type) {
    case YAML_SEQUENCE_START_EVENT:
      is_array = 1;
      break;
    case YAML_SCALAR_EVENT:
      if (strcmp(event.data.scalar.tag, YAML_INT_TAG) == 0) {
	((int *) parameter)[i] = strtol(event.data.scalar.value, &endptr, 10);
      } else if (strcmp(event.data.scalar.tag, YAML_FLOAT_TAG) == 0) {
	((float *) parameter)[i] = strtof(event.data.scalar.value, &endptr);
      } else if (strcmp(event.data.scalar.tag, YAML_STR_TAG) == 0) {
	strncpy(&((char *) parameter)[i], event.data.scalar.value, event.data.scalar.length);
      } else if (strcmp(event.data.scalar.tag, YAML_BOOL_TAG) == 0) {
	if (strcmp(event.data.scalar.value, "true") == 0) {
	  ((int *) parameter)[i] = 1;
	} else {
	  ((int *) parameter)[i] = 0;
	}
      } else {
	// No other type is supported at the moment, so it should be considered an error
	// if we reach here
	goto error;
      }
      if (is_array) {
	i++;
      } else {
	done = 1;
      }
      break;
    case YAML_SEQUENCE_END_EVENT:
      done = 1;
      break;
    default:
      goto error;
    }
    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
  
}
