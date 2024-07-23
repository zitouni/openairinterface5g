
COMPILATION 
./build_oai --gNB --nrUE -w SIMU -w USRP --build-lib nrscope --build-e2 --ninja -C

simulator:
CU:  sudo RFSIMULATOR=server ./nr-softmodem -E --rfsim --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/cu_IcsCore_2DU.conf
du1:  sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/du_Nokia_cell1OnCU.conf
du2: sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/du_Nokia_cell2OnDU2.conf

ue: sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --rfsimulator.serveraddr 127.0.0.1 --sa --nokrnmod -O  ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue_ics_core.conf




