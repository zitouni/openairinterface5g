#!/bin/bash
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

function run_test_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- xenial image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool test [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
    echo ""
    echo "    --workspace #### OR -ws ####"
    echo "    Specify the workspace."
    echo ""
    variant_usage
    echo "    Specify the variant to build."
    echo ""
    echo "    --keep-vm-alive OR -k"
    echo "    Keep the VM alive after the build."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

function start_basic_sim_enb {
    local LOC_VM_IP_ADDR=$2
    local LOC_EPC_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"export ENODEB=1\"" >> $1
    echo "export ENODEB=1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/enb -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    sleep 10
    rm $1
}

function start_basic_sim_ue {
    local LOC_UE_LOG_FILE=$3
    local LOC_NB_RBS=$4
    local LOC_FREQUENCY=$5
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue\"" > $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator/ue" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue" >> $1
    echo "echo \"./lte-uesoftmodem -C ${LOC_FREQUENCY}000000 -r $LOC_NB_RBS --ue-rxgain 140\" > ./my-lte-uesoftmodem-run.sh" >> $1
    echo "chmod 775 ./my-lte-uesoftmodem-run.sh" >> $1
    echo "cat ./my-lte-uesoftmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/ue -o /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ./my-lte-uesoftmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1

    local i="0"
    echo "ifconfig oip1 | egrep -c \"inet addr\"" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1`
        if [ $CONNECTED -eq 1 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    rm $1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
    else
        UE_SYNC=1
    fi
}

function get_ue_ip_addr {
    echo "ifconfig oip1 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    UE_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "UE IP Address for EPC is : $UE_IP_ADDR"
    rm $1
}

function ping_ue_ip_addr {
    echo "echo \"ping -c 20 $3\"" > $1
    echo "echo \"COMMAND IS: ping -c 20 $3\" > $4" > $1
    echo "ping -c 20 $UE_IP_ADDR | tee -a $4" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function check_ping_result {
    local LOC_PING_FILE=$1
    local LOC_NB_PINGS=$2
    if [ -f $LOC_PING_FILE ]
    then
        local FILE_COMPLETE=`egrep -c "ping statistics" $LOC_PING_FILE`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            PING_STATUS=-1
        else
            local ALL_PACKET_RECEIVED=`egrep -c "$LOC_NB_PINGS received" $LOC_PING_FILE`
            if [ $ALL_PACKET_RECEIVED -eq 1 ]
            then
                echo "got all ping packets"
            else
                PING_STATUS=-1
            fi
        fi
    else
        PING_STATUS=-1
    fi
}

function iperf_dl {
    local REQ_BANDWIDTH=$5
    local BASE_LOG_FILE=$6
    echo "echo \"iperf -u -s -i 1\"" > $1
    echo "echo \"COMMAND IS: iperf -u -s -i 1\" > tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt" > $1
    echo "nohup iperf -u -s -i 1 >> tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt &" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1

    echo "echo \"iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\"" > $3
    echo "echo \"COMMAND IS: iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\" > ${BASE_LOG_FILE}_client.txt" > $3
    echo "iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 | tee -a ${BASE_LOG_FILE}_client.txt" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    rm -f $3

    echo "killall --signal SIGKILL iperf" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
}

function iperf_ul {
    local REQ_BANDWIDTH=$5
    local BASE_LOG_FILE=$6
    echo "echo \"iperf -u -s -i 1\"" > $3
    echo "echo \"COMMAND IS: iperf -u -s -i 1\" > ${BASE_LOG_FILE}_server.txt" > $3
    echo "nohup iperf -u -s -i 1 >> ${BASE_LOG_FILE}_server.txt &" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    rm $3

    echo "echo \"iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\"" > $1
    echo "echo \"COMMAND IS: iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\" > /home/ubuntu/tmp/cmake_targets/log/${BASE_LOG_FILE}_client.txt" > $1
    echo "iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 | tee -a /home/ubuntu/tmp/cmake_targets/log/${BASE_LOG_FILE}_client.txt" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1

    echo "killall --signal SIGKILL iperf" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    rm $3
}

function check_iperf {
    local LOC_BASE_LOG=$1
    local LOC_REQ_BW=$2
    local LOC_REQ_BW_MINUS_ONE=`echo "$LOC_REQ_BW - 1" | bc -l`
    if [ -f ${LOC_BASE_LOG}_client.txt ]
    then
        local FILE_COMPLETE=`egrep -c "Server Report" ${LOC_BASE_LOG}_client.txt`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            IPERF_STATUS=-1
        else
            local EFFECTIVE_BANDWIDTH=`tail -n3 ${LOC_BASE_LOG}_client.txt | egrep "Mbits/sec" | sed -e "s#^.*MBytes *##" -e "s#sec.*#sec#"`
            if [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW}.*Mbits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_ONE}.*Mbits.* ]]
            then
                echo "got requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
            else
                IPERF_STATUS=-1
            fi
        fi
    else
        IPERF_STATUS=-1
    fi
}

function terminate_enb_ue_basic_sim {
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" > $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=enb_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=enb_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=ue_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=ue_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGINT lte-softmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGINT lte-softmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-softmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-softmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-uesoftmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-uesoftmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function recover_core_dump {
    local IS_SEG_FAULT=`egrep -ic "segmentation fault" $3`
    if [ $IS_SEG_FAULT -ne 0 ]
    then
        local TC=`echo $3 | sed -e "s#^.*enb_##" -e "s#Hz.*#Hz#"`
        echo "Segmentation fault detected on enb -> recovering core dump"
        echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb" > $1
        echo "sync" >> $1
        echo "sudo tar -cjhf basic-simulator-enb-core-${TC}.bz2 core lte-softmodem *.so ci-lte-basic-sim.conf my-lte-softmodem-run.sh" >> $1
        echo "sudo rm core" >> $1
        echo "rm ci-lte-basic-sim.conf" >> $1
        echo "sync" >> $1
        ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/basic_simulator/enb/basic-simulator-enb-core-${TC}.bz2 $4
        rm -f $1
    fi
}

function terminate_ltebox_epc {
    echo "echo \"cd /opt/ltebox/tools\"" > $1
    echo "cd /opt/ltebox/tools" >> $1
    echo "echo \"sudo ./stop_ltebox\"" >> $1
    echo "sudo ./stop_ltebox" >> $1
    echo "echo \"sudo daemon --name=simulated_hss --stop\"" >> $1
    echo "sudo daemon --name=simulated_hss --stop" >> $1
    echo "echo \"sudo killall --signal SIGKILL hss_sim\"" >> $1
    echo "sudo killall --signal SIGKILL hss_sim" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
}

function run_test_on_vm {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"
    echo "VM_NAME             = $VM_NAME"
    echo "VM_CMD_FILE         = $VM_CMDS"
    echo "JENKINS_WKSP        = $JENKINS_WKSP"
    echo "ARCHIVES_LOC        = $ARCHIVES_LOC"

    echo "############################################################"
    echo "Waiting for VM to be started"
    echo "############################################################"
    uvt-kvm wait $VM_NAME --insecure

    VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
    echo "$VM_NAME has for IP addr = $VM_IP_ADDR"

    if [ "$RUN_OPTIONS" == "none" ]
    then
        echo "No run on VM testing for this variant currently"
        exit $STATUS
    fi

    if [[ $RUN_OPTIONS =~ .*run_exec_autotests.* ]]
    then
        echo "############################################################"
        echo "Running test script on VM ($VM_NAME)"
        echo "############################################################"
        echo "echo \"sudo apt-get --yes --quiet install bc \"" > $VM_CMDS
        echo "sudo apt-get update > bc-install.txt 2>&1" >> $VM_CMDS
        echo "sudo apt-get --yes install bc >> bc-install.txt 2>&1" >> $VM_CMDS
        echo "cd tmp" >> $VM_CMDS
        echo "echo \"source oaienv\"" >> $VM_CMDS
        echo "source oaienv" >> $VM_CMDS
        echo "echo \"cd cmake_targets/autotests\"" >> $VM_CMDS
        echo "cd cmake_targets/autotests" >> $VM_CMDS
        echo "echo \"rm -Rf log\"" >> $VM_CMDS
        echo "rm -Rf log" >> $VM_CMDS
        echo "$RUN_OPTIONS" | sed -e 's@"@\\"@g' -e 's@^@echo "@' -e 's@$@"@' >> $VM_CMDS
        echo "$RUN_OPTIONS" >> $VM_CMDS
        echo "cp /home/ubuntu/bc-install.txt log" >> $VM_CMDS
        echo "cd log" >> $VM_CMDS
        echo "zip -r -qq tmp.zip *.* 0*" >> $VM_CMDS

        ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

        echo "############################################################"
        echo "Creating a tmp folder to store results and artifacts"
        echo "############################################################"

        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/autotests/log/tmp.zip $ARCHIVES_LOC
        pushd $ARCHIVES_LOC
        unzip -qq -DD tmp.zip
        rm tmp.zip
        if [ -f results_autotests.xml ]
        then
            FUNCTION=`echo $VM_NAME | sed -e "s@$VM_TEMPLATE@@"`
            NEW_NAME=`echo "results_autotests.xml" | sed -e "s@results_autotests@results_autotests-$FUNCTION@"`
            echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > $NEW_NAME
            echo "<?xml-stylesheet type=\"text/xsl\" href=\"$FUNCTION.xsl\" ?>" >> $NEW_NAME
            cat results_autotests.xml >> $NEW_NAME
            sed -e "s@TEMPLATE@$FUNCTION@" $JENKINS_WKSP/ci-scripts/template.xsl > $FUNCTION.xsl
            #mv results_autotests.xml $NEW_NAME
            rm results_autotests.xml
        fi
        popd

        if [ $KEEP_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Destroying VM"
            echo "############################################################"
            uvt-kvm destroy $VM_NAME
            ssh-keygen -R $VM_IP_ADDR
        fi
        rm -f $VM_CMDS

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        LOG_FILES=`ls $ARCHIVES_LOC/results_autotests*.xml`
        NB_FOUND_FILES=0
        NB_RUNS=0
        NB_FAILURES=0

        for FULLFILE in $LOG_FILES
        do
            TESTSUITES=`egrep "testsuite errors" $FULLFILE`
            for TESTSUITE in $TESTSUITES
            do
                if [[ "$TESTSUITE" == *"tests="* ]]
                then
                    RUNS=`echo $TESTSUITE | awk 'BEGIN{FS="="}{print $2}END{}' | sed -e "s@'@@g" `
                    NB_RUNS=$((NB_RUNS + RUNS))
                fi
                if [[ "$TESTSUITE" == *"failures="* ]]
                then
                    FAILS=`echo $TESTSUITE | awk 'BEGIN{FS="="}{print $2}END{}' | sed -e "s@'@@g" `
                    NB_FAILURES=$((NB_FAILURES + FAILS))
                fi
            done
            NB_FOUND_FILES=$((NB_FOUND_FILES + 1))
        done

        echo "NB_FOUND_FILES = $NB_FOUND_FILES"
        echo "NB_RUNS        = $NB_RUNS"
        echo "NB_FAILURES    = $NB_FAILURES"

        if [ $NB_FOUND_FILES -eq 0 ]; then STATUS=-1; fi
        if [ $NB_RUNS -eq 0 ]; then STATUS=-1; fi
        if [ $NB_FAILURES -ne 0 ]; then STATUS=-1; fi

    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-basic-sim.* ]]
    then
        PING_STATUS=0
        IPERF_STATUS=0
        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        EPC_VM_NAME=`echo $VM_NAME | sed -e "s#basic-sim#epc#"`
        LTEBOX=0
        if [ -d /opt/ltebox-archives/ ]
        then
            # Checking if all ltebox archives are available to run ltebx epc on a brand new VM
            if [ -f /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ] && [ -f /opt/ltebox-archives/etc-conf.zip ] && [ -f /opt/ltebox-archives/hss-sim.zip ]
            then
                echo "############################################################"
                echo "Test EPC on VM ($EPC_VM_NAME) will be using ltebox"
                echo "############################################################"
                LTEBOX=1
            fi
        fi
        # Here we could have other types of EPC detection

        # Do we need to start the EPC VM
        EPC_VM_CMDS=`echo $VM_CMDS | sed -e "s#cmds#epc-cmds#"`
        echo "EPC_VM_CMD_FILE     = $EPC_VM_CMDS"
        IS_EPC_VM_ALIVE=`uvt-kvm list | grep -c $EPC_VM_NAME`
        if [ $IS_EPC_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Creating test EPC VM ($EPC_VM_NAME) on Ubuntu Cloud Image base"
            echo "############################################################"
            uvt-kvm create $EPC_VM_NAME release=xenial --unsafe-caching
        fi

        uvt-kvm wait $EPC_VM_NAME --insecure
        EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`
        echo "$EPC_VM_NAME has for IP addr = $EPC_VM_IP_ADDR"
        scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu

        # ltebox specific actions (install and start)
        LTE_BOX_TO_INSTALL=1
        if [ $LTEBOX -eq 1 ]
        then
            echo "ls -ls /opt/ltebox/tools/start_ltebox" > $EPC_VM_CMDS
            RESPONSE=`ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS`
            NB_EXES=`echo $RESPONSE | grep -c ltebox`
            if [ $NB_EXES -eq 1 ]; then LTE_BOX_TO_INSTALL=0; fi
        fi

        if [ $LTEBOX -eq 1 ] && [ $LTE_BOX_TO_INSTALL -eq 1 ]
        then
            echo "############################################################"
            echo "Copying ltebox archives into EPC VM ($EPC_VM_NAME)" 
            echo "############################################################"
            scp -o StrictHostKeyChecking=no /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu
            scp -o StrictHostKeyChecking=no /opt/ltebox-archives/etc-conf.zip ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu
            scp -o StrictHostKeyChecking=no /opt/ltebox-archives/hss-sim.zip ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu

            echo "############################################################"
            echo "Install EPC on EPC VM ($EPC_VM_NAME)"
            echo "############################################################"
            echo "sudo cp 01proxy /etc/apt/apt.conf.d/" > $EPC_VM_CMDS
            echo "touch /home/ubuntu/.hushlogin" >> $EPC_VM_CMDS
            echo "echo \"sudo apt-get --yes --quiet install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf\"" >> $EPC_VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $EPC_VM_CMDS
            echo "sudo apt-get --yes install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf >> zip-install.txt 2>&1" >> $EPC_VM_CMDS

            # Installing HSS
            echo "echo \"cd /opt\"" >> $EPC_VM_CMDS
            echo "cd /opt" >> $EPC_VM_CMDS
            echo "echo \"sudo unzip -qq /home/ubuntu/hss-sim.zip\"" >> $EPC_VM_CMDS
            echo "sudo unzip -qq /home/ubuntu/hss-sim.zip" >> $EPC_VM_CMDS
            echo "echo \"cd /opt/hss_sim0609\"" >> $EPC_VM_CMDS
            echo "cd /opt/hss_sim0609" >> $EPC_VM_CMDS

            # Installing ltebox
            echo "echo \"cd /home/ubuntu\"" >> $EPC_VM_CMDS
            echo "cd /home/ubuntu" >> $EPC_VM_CMDS
            echo "echo \"sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb \"" >> $EPC_VM_CMDS
            echo "sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb >> zip-install.txt 2>&1" >> $EPC_VM_CMDS

            echo "echo \"cd /opt/ltebox/etc/\"" >> $EPC_VM_CMDS
            echo "cd /opt/ltebox/etc/" >> $EPC_VM_CMDS
            echo "echo \"sudo unzip -qq -o /home/ubuntu/etc-conf.zip\"" >> $EPC_VM_CMDS
            echo "sudo unzip -qq -o /home/ubuntu/etc-conf.zip" >> $EPC_VM_CMDS
            echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$EPC_VM_IP_ADDR#' gw.conf" >> $EPC_VM_CMDS
            echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$EPC_VM_IP_ADDR#' mme.conf" >> $EPC_VM_CMDS
        fi

        # Starting EPC
        if [ $LTEBOX -eq 1 ]
        then
            echo "############################################################"
            echo "Start EPC on EPC VM ($EPC_VM_NAME)"
            echo "############################################################"
            echo "echo \"cd /opt/hss_sim0609\"" >> $EPC_VM_CMDS
            echo "cd /opt/hss_sim0609" >> $EPC_VM_CMDS
            echo "echo \"sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real\"" >> $EPC_VM_CMDS
            echo "sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real" >> $EPC_VM_CMDS

            echo "echo \"cd /opt/ltebox/tools/\"" >> $EPC_VM_CMDS
            echo "cd /opt/ltebox/tools/" >> $EPC_VM_CMDS
            echo "echo \"sudo ./start_ltebox\"" >> $EPC_VM_CMDS
            echo "nohup sudo ./start_ltebox > /home/ubuntu/ltebox.txt" >> $EPC_VM_CMDS

            ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
            rm -f $EPC_VM_CMDS

            # We may have some adaptation to do
            if [ -f /opt/ltebox-archives/adapt_ue_sim.txt ]
            then
                echo "############################################################"
                echo "Doing some adaptation on UE side"
                echo "############################################################"
                ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < /opt/ltebox-archives/adapt_ue_sim.txt
            fi

            i="0"
            echo "ifconfig tun5 | egrep -c \"inet addr\"" > $EPC_VM_CMDS
            while [ $i -lt 10 ]
            do
                sleep 2
                CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS`
                if [ $CONNECTED -eq 1 ]
                then
                    i="100"
                else
                    i=$[$i+1]
                fi
            done
            rm $EPC_VM_CMDS
            if [ $i -lt 50 ]
            then
                echo "Problem w/ starting ltebox EPC"
                exit -1
            fi
        fi

        # HERE ADD ANY INSTALL ACTIONS FOR ANOTHER EPC

        # Retrieve EPC real IP address
        if [ $LTEBOX -eq 1 ]
        then
            # in our configuration file, we are using pool 5
            echo "ifconfig tun5 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $EPC_VM_CMDS
            REAL_EPC_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS`
            echo "EPC IP Address     is : $REAL_EPC_IP_ADDR"
            rm $EPC_VM_CMDS
        fi

        echo "############################################################"
        echo "Starting the eNB in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_05MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 lte-fdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_05MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_05MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Iperf UL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_ul
        iperf_ul $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 2 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 2

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in FDD-10MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_10MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 50 lte-fdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in FDD-10MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_10MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 50 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_10MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_10MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Iperf UL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_10MHz_iperf_ul
        iperf_ul $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 2 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 2

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in FDD-20MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_20MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 100 lte-fdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in FDD-20MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_20MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 100 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_20MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_20MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in TDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=tdd_05MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 lte-tdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in TDD-5MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=tdd_05MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 2350
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=tdd_05MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

#        Bug in TDD 5Mhz --- not running it
#        echo "############################################################"
#        echo "Iperf DL"
#        echo "############################################################"
#        CURR_IPERF_LOG_BASE=tdd_05MHz_iperf_dl
#        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
#        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
#        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
#        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in TDD-10MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=tdd_10MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 50 lte-tdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in TDD-10MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=tdd_10MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 50 2350
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=tdd_10MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=tdd_10MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in TDD-20MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=tdd_20MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 100 lte-tdd-basic-sim.conf

        echo "############################################################"
        echo "Starting the UE in TDD-20MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=tdd_20MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 100 2350
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=tdd_20MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=tdd_20MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Terminate EPC"
        echo "############################################################"

        if [ $LTEBOX -eq 1 ]
        then
            terminate_ltebox_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
        fi

        if [ $KEEP_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Destroying VMs"
            echo "############################################################"
            uvt-kvm destroy $VM_NAME
            ssh-keygen -R $VM_IP_ADDR
            uvt-kvm destroy $EPC_VM_NAME
            ssh-keygen -R $EPC_VM_IP_ADDR
        fi

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $IPERF_STATUS -ne 0 ]; then STATUS=-1; fi

    fi
}
