#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import glob
import re
import yaml
import os
import sys


def main():
  #read yaml input parameters
  f = open(f'{sys.argv[1]}',)
  data = yaml.full_load(f)
  initial_path = f'{data[0]["paths"]["source_dir"]}'
  dir = glob.glob(initial_path + '/**/*.conf', recursive=True)

  #identify configs, read and replace corresponding values
  for config in data[1]["configurations"]:
    filePrefix = config["filePrefix"]
    outputfilename = config["outputfilename"]
    print('================================================')
    print('filePrefix = ' + filePrefix)
    print('outputfilename = ' + outputfilename)
    found = False
    for inputfile in dir:
      if found:
        continue
      if inputfile.find(filePrefix) >=0:
        prefix_outputfile = {"cu.band7.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}', 
                             "du.band7.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rru.fdd": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rru.tdd": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "enb.band7.tm1.fr1.25PRB.usrpb210.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "enb.band40.tm1.25PRB.FairScheduler.usrpb210": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band7.tm1.nfapi": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band7.tm1.if4p5.lo.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band40.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.band78.tm1.fr1.106PRB.usrpb210.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.band78.sa.fr1.106PRB.usrpn310.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.sa.band78.fr1.51PRB.usrpb210.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.sa.band66.fr1.106PRB.usrpn300.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gNB_SA_CU.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gNB_SA_DU.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "proxy_gnb.band78.sa.fr1.106PRB.usrpn310.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "proxy_rcc.band7.tm1.nfapi.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "proxy_nr-ue.nfapi.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "ue.nfapi": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "ue_sim_ci": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}'
                             }
        print('inputfile = ' + inputfile)
        found = True
        if filePrefix in prefix_outputfile:
          outputfile1 = prefix_outputfile[filePrefix]  

        directory = f'{data[0]["paths"]["dest_dir"]}'
        if not os.path.exists(directory):
          os.makedirs(directory, exist_ok=True)

        with open(f'{inputfile}', mode='r') as inputfile, \
             open(outputfile1, mode='w') as outputfile:
          for line in inputfile:
            if re.search(r'EHPLMN_LIST', line):
              outputfile.write(line)
              continue
            templine = line
            for key in config["config"]:
              if templine.find(key["key"]) >= 0:
                if re.search(r'preference', templine): # false positive
                  continue
                if key["key"] != 'sdr_addrs' and re.search(r'sdr_addrs', templine): # false positive
                  continue
                elif re.search('downlink_frequency', line):
                  templine = re.sub(r'[0-9]+', key["env"], line)
                elif re.search('uplink_frequency_offset', line):
                  templine = re.sub(r'[0-9]+', key["env"], line)
                # next: matches key = ( "SOMETHING" ) or key = [ "SOMETHING" ]
                elif re.search(key["key"] + "\s*=\s*[\(\[]\s*\"[0-9.a-zA-Z:_-]+\"\s*[\)\]]", templine):
                  templine = re.sub("(" + key["key"] + "\s*=\s*[\(\[]\s*\")[0-9.a-zA-Z:_-]+(\"[\)\]])",
                                    r'\1' + key["env"] + r"\2", templine)
                # next: matches key = "SOMETHING"  or key = [SOMETHING],
                elif re.search(key["key"] + "\s*=\s*[\"\[][0-9.a-zA-Z:_/-]+[\"\]]", templine):
                  templine = re.sub("(" + key["key"] + "\s*=\s*[\"\[])[0-9.a-zA-Z:_/-]+([\"\]])",
                                    r'\1' + key["env"] + r"\2", templine)
                # next: matches key = NUMBER
                elif re.search(key["key"] + "\s*=\s*[x0-9]+", templine): # x for "0x" hex start
                  templine = re.sub("(" + key["key"] + "\s*=\s*(?:0x)?)[x0-9a-fA-F]+", r"\1" + key["env"], templine)
                # next: special case for sdr_addrs
                elif key["key"] == 'sdr_addrs' and re.search(key["key"] + "\s*=\s*", templine):
                  templine = re.sub("(" + key["key"] + "\s*=\s*.*$)", key["key"] + " = \"" + key["env"] + "\"", templine)
            outputfile.write(templine)

if __name__ == "__main__":
    main()
