This tutorial explains how to perform handovers.

# Considered setup

# Steps to reproduce the demo

## Set up

- Make sure each individual CU-DU combination works individually
- Start CU and connect all relevant DUs
- `print nrRRC_stats.log`
- fill in `neighbour_config.conf`, and `@include` in CU file
- Start the CU
- Start all DUs

## Verify the UE sees?



## Trigger handover manually

Build the CU with support for telnet:
```bash
./build_oai --ninja --gNB --build-lib telnetsrv
```

Start the CU with telnet support:
```bash
sudo ./nr-softmodem --sa -O cu.conf --telnetsrv --telnetsrv.shrmod ci
```

Connect DUs, and connect the phone as usual. You can trigger a handover
manually like so:
```bash
echo ci trigger_f1_ho [cu-ue-id] | nc 127.0.0.1 9090 && echo
```

This triggers a handover (in a round-robin fashion across all DUs) for a UE
`cu-ue-id`. This parameter is optional, and a handover will be triggered if
only one UE context is present; if there is more than one or less than one UE,
the handover will be refused.
