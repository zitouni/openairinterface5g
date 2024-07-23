

Simulator mode:
mono gNB:  sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/x310_tm500.conf --gNBs.[0].min_rxtxtime 6 --rfsim --sa
CU:  sudo RFSIMULATOR=server ./nr-softmodem -E --rfsim --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/cu.conf
DU:  sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/CONF_ICS/du_tm500.conf
UE:  

sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3319320000 --ssb 192 --rfsim --rfsimulator.serveraddr 127.0.0.1 --sa --nokrnmod -O  ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/ue_ics_core.conf




with USRP:
mono gNB TM500:
  sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/gnb_tm500.conf --gNBs.[0].min_rxtxtime 6 --sa --usrp-tx-thread-config 1  --continuous-tx 1  -E 


monogNB NOKIA:
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/gnb_Nokia7.conf --gNBs.[0].min_rxtxtime 6 --sa --usrp-tx-thread-config 1  --continuous-tx 1  -E


CU :
sudo RFSIMULATOR=server ./nr-softmodem -E --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/cu.conf   --usrp-tx-thread-config 1  --continuous-tx 1

DU TM500:
sudo RFSIMULATOR=server ./nr-softmodem --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/du_tm500.conf --usrp-tx-thread-config 1  --continuous-tx 1  -E 

DU NOKIA:
sudo RFSIMULATOR=server ./nr-softmodem --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF_ICS/du_Nokia.conf --usrp-tx-thread-config 1  --continuous-tx 1  -E 
