/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file common/config/cmdline/config_libconfig.c
 * \brief configuration module, command line parsing implementation 
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "config_userapi.h"

int processoption(paramdef_t *cfgoptions, char *value)
{
int argok=1;
char *tmpval = value;
int ret =0;
int optisset;

     if (value == NULL) {
     	 argok=0; 
     } else if ( value[0] == '-') {
     	 argok = 0;
     }
     if ((cfgoptions->paramflags &PARAMFLAG_BOOL) == 0) { /* not a boolean, argument required */
	if (argok == 0) {
	    fprintf(stderr,"[CONFIG] command line, option %s requires an argument\n",cfgoptions->optname);
	    return 0;
	} else {        /* boolean value */
         tmpval = "1";
        }
     } 

     switch(cfgoptions->type)
       {
       	case TYPE_STRING:
           config_check_valptr(cfgoptions, (char **)(cfgoptions->strptr), sizeof(char *));
           config_check_valptr(cfgoptions, cfgoptions->strptr, strlen(tmpval+1));
           sprintf(*(cfgoptions->strptr), "%s",tmpval);
           printf_cmdl("[CONFIG] %s set to  %s from command line\n", cfgoptions->optname, tmpval);
	   optisset=1;
        break;
	
        case TYPE_STRINGLIST:
        break;
        case TYPE_UINT32:
       	case TYPE_INT32:
        case TYPE_UINT16:
       	case TYPE_INT16:
	case TYPE_UINT8:
       	case TYPE_INT8:	
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->iptr),sizeof(int32_t));
	   config_assign_int(cfgoptions,cfgoptions->optname,(int32_t)strtol(tmpval,NULL,0));  
	   optisset=1;
        break;  	
       	case TYPE_UINT64:
       	case TYPE_INT64:
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->i64ptr),sizeof(uint64_t));
	   *(cfgoptions->i64ptr)=strtoll(tmpval,NULL,0);  
           printf_cmdl("[CONFIG] %s set to  %lli from command line\n", cfgoptions->optname, (long long)*(cfgoptions->i64ptr));
	   optisset=1;
        break;        
       	case TYPE_UINTARRAY:
       	case TYPE_INTARRAY:

        break;
        case TYPE_DOUBLE:
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->dblptr),sizeof(double)); 
           *(cfgoptions->dblptr) = strtof(tmpval,NULL);  
           printf_cmdl("[CONFIG] %s set to  %lf from command line\n", cfgoptions->optname, *(cfgoptions->dblptr));
	   optisset=1; 
        break; 

       	case TYPE_IPV4ADDR:

        break;

       default:
            fprintf(stderr,"[CONFIG] command line, %s type %i  not supported\n",cfgoptions->optname, cfgoptions->type);
       break;
       } /* switch on param type */
       if (optisset == 1) {
          cfgoptions->paramflags = cfgoptions->paramflags |  PARAMFLAG_PARAMSET;
       }
       
    return argok;
}

int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix)
{
char **p = config_get_if()->argv;
int c = config_get_if()->argc;
int j;
char *cfgpath; 
 
  j = (prefix ==NULL) ? 0 : strlen(prefix); 
  cfgpath = malloc( j + MAX_OPTNAME_SIZE +1);
  if (cfgpath == NULL) {
     fprintf(stderr,"[CONFIG] %s %i malloc error,  %s\n", __FILE__, __LINE__,strerror(errno));
     return -1;
  }

  j=0;
  p++;
  c--;
    while (c >= 0 && *p != NULL) {
        if (strcmp(*p, "-h") == 0 || strcmp(*p, "--help") == 0 ) {
            config_printhelp(cfgoptions,numoptions);
        }
        for(int i=0;i<numoptions;i++) {
            if ( ( cfgoptions[i].paramflags & PARAMFLAG_DISABLECMDLINE) != 0) {
              continue;
             }
	    if (prefix != NULL) {
               sprintf(cfgpath,"%s.%s",prefix,cfgoptions[i].optname);
	    } else {
	       sprintf(cfgpath,"%s",cfgoptions[i].optname);
	    }
            if ( ((strlen(*p) == 2) && (strcmp(*p + 1,cfgpath) == 0))  || 
                 ((strlen(*p) > 2) && (strcmp(*p + 2,cfgpath ) == 0 )) ) {
               p++;
               c--;
               j = processoption(&(cfgoptions[i]), *p);
               if ( j== 0) {
                  c++;
                  p--;
               }
            }
         }   	     
   	 p++;
         c--;  
    }   /* fin du while */
  printf_cmdl("[CONFIG] %s %i options set from command line\n",((prefix == NULL) ? "":prefix),j);
  free(cfgpath);
  return j;            
}  /* parse_cmdline*/



