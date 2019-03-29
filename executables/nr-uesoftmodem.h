#ifndef NR_UESOFTMODEM_H
#define NR_UESOFTMODEM_H

#define _GNU_SOURCE
#include <execinfo.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/sched.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/sysinfo.h>
#include "../../ARCH/COMMON/common_lib.h"
#undef MALLOC
#include "assertions.h"
#include "PHY/types.h"
#include "PHY/defs_nr_UE.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include <threadPool/thread-pool.h>


#if defined(ENABLE_ITTI)
  #if defined(ENABLE_USE_MME)
    #include "s1ap_eNB.h"
    #ifdef PDCP_USE_NETLINK
      #include "SIMULATION/ETH_TRANSPORT/proto.h"
    #endif
  #endif
#endif

/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_RFCFGF        "Configuration file for front-end (e.g. LMS7002M)\n"
#define CONFIG_HLP_ULMAXE        "set the eNodeB max ULSCH erros\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"
#define CONFIG_HLP_UERXG         "set UE RX gain\n"
#define CONFIG_HLP_UERXGOFF      "external UE amplifier offset\n"
#define CONFIG_HLP_UETXG         "set UE TX gain\n"
#define CONFIG_HLP_UENANTR       "set UE number of rx antennas\n"
#define CONFIG_HLP_UENANTT       "set UE number of tx antennas\n"
#define CONFIG_HLP_UESCAN        "set UE to scan around carrier\n"
#define CONFIG_HLP_UEFO          "set UE to enable estimation and compensation of frequency offset\n"
#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_DLSHIFT       "dynamic shift for LLR compuation for TM3/4 (default 0)\n"
#define CONFIG_HLP_UELOOP        "get softmodem (UE) to loop through memory instead of acquiring from HW\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DMAMAP        "sets flag for improved EXMIMO UE performance\n"
#define CONFIG_HLP_EXCCLK        "tells hardware to use an external clock reference\n"
#define CONFIG_HLP_USIM          "use XOR autentication algo in case of test usim mode\n"
#define CONFIG_HLP_NOSNGLT       "Disables single-thread mode in lte-softmodem\n"
#define CONFIG_HLP_TADV          "Set timing_advance\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_EXMCAL        "Calibrate the EXMIMO borad, available files: exmimo2_2arxg.lime exmimo2_2brxg.lime \n"
#define CONFIG_HLP_ITTIL         "Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n"
#define CONFIG_HLP_DLMCS         "Set the maximum downlink MCS\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_PRB           "Set the PRB, valid values: 6, 25, 50, 100  \n"
#define CONFIG_HLP_MSLOTS        "Skip the missed slots/subframes \n"
#define CONFIG_HLP_ULMCS         "Set the maximum uplink MCS\n"
#define CONFIG_HLP_TDD           "Set hardware to TDD mode (default: FDD). Used only with -U (otherwise set in config file).\n"
#define CONFIG_HLP_UE            "Set the lte softmodem as a UE\n"
#define CONFIG_HLP_L2MONW        "Enable L2 wireshark messages on localhost \n"
#define CONFIG_HLP_L2MONP        "Enable L2 pcap  messages on localhost \n"
#define CONFIG_HLP_VCD           "Enable VCD (generated file will is named openair_dump_eNB.vcd, read it with target/RT/USER/eNB.gtkw\n"
#define CONFIG_HLP_TQFS          "Apply three-quarter of sampling frequency, 23.04 Msps to reduce the data rate on USB/PCIe transfers (only valid for 20 MHz)\n"
#define CONFIG_HLP_TPORT         "tracer port\n"
#define CONFIG_HLP_NOTWAIT       "don't wait for tracer, start immediately\n"
#define CONFIG_HLP_TNOFORK       "to ease debugging with gdb\n"

#define CONFIG_HLP_NUMEROLOGY    "adding numerology for 5G\n"
#define CONFIG_HLP_EMULATE_RF    "Emulated RF enabled(disable by defult)\n"
#define CONFIG_HLP_PARALLEL_CMD  "three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'\n"
#define CONFIG_HLP_WORKER_CMD    "two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'\n"

#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"

#define CONFIG_HLP_USRP_ARGS                "set the arguments to identify USRP (same syntax as in UHD)\n"
#define CONFIG_HLP_USRP_CLK_SRC              "USRP clock source: 'internal' or 'external'\n"

/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags                      XXXptr        defXXXval         type       numelt   */
/*------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UEMODEPARAMS_DESC {  \
    {"calib-ue-rx",                 CONFIG_HLP_CALUER,     0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"calib-ue-rx-med",             CONFIG_HLP_CALUERM,    0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"calib-ue-rx-byp",             CONFIG_HLP_CALUERB,    0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"debug-ue-prach",              CONFIG_HLP_DBGUEPR,    PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"no-L2-connect",               CONFIG_HLP_NOL2CN,     PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"calib-prach-tx",              CONFIG_HLP_CALPRACH,   PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"loop-memory",                 CONFIG_HLP_UELOOP,     0,    strptr:&loopfile,       defstrval:"iqs.in", TYPE_STRING,0},    \
    {"ue-dump-frame",               CONFIG_HLP_DUMPFRAME,  PARAMFLAG_BOOL,   iptr:&dumpframe,      defintval:0,  TYPE_INT,   0},    \
  }
#define CMDLINE_CALIBUERX_IDX                   0
#define CMDLINE_CALIBUERXMED_IDX                1
#define CMDLINE_CALIBUERXBYP_IDX                2
#define CMDLINE_DEBUGUEPRACH_IDX                3
#define CMDLINE_NOL2CONNECT_IDX                 4
#define CMDLINE_CALIBPRACHTX_IDX                5
#define CMDLINE_MEMLOOP_IDX                     6
#define CMDLINE_DUMPMEMORY_IDX                  7
/*------------------------------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE                                                                */
/*   optname                     helpstr             paramflags                      XXXptr                  defXXXval       type          numelt   */
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UEPARAMS_DESC {  \
    {"ue-rxgain",                CONFIG_HLP_UERXG,      0,    dblptr:&(rx_gain[0][0]),      defdblval:0,    TYPE_DOUBLE,   0},     \
    {"ue-rxgain-off",            CONFIG_HLP_UERXGOFF,   0,    dblptr:&rx_gain_off,        defdblval:0,    TYPE_DOUBLE,   0},     \
    {"ue-txgain",                CONFIG_HLP_UETXG,      0,    dblptr:&(tx_gain[0][0]),      defdblval:0,    TYPE_DOUBLE,   0},     \
    {"ue-nb-ant-rx",             CONFIG_HLP_UENANTR,    0,    u8ptr:&nb_antenna_rx,       defuintval:1,   TYPE_UINT8,    0},     \
    {"ue-nb-ant-tx",             CONFIG_HLP_UENANTT,    0,    u8ptr:&nb_antenna_tx,       defuintval:1,   TYPE_UINT8,    0},     \
    {"ue-scan-carrier",          CONFIG_HLP_UESCAN,     PARAMFLAG_BOOL, iptr:&UE_scan_carrier,        defintval:0,    TYPE_INT,    0},     \
    {"ue-fo-compensation",         CONFIG_HLP_UEFO,       PARAMFLAG_BOOL, iptr:&UE_fo_compensation,       defintval:0,    TYPE_INT,    0},     \
    {"ue-max-power",             NULL,          0,    iptr:&(tx_max_power[0]),      defintval:90,   TYPE_INT,    0},     \
    {"r"  ,                        CONFIG_HLP_PRB,        0,                iptr:&(frame_parms[0]->N_RB_DL),   defintval:25,   TYPE_UINT,    0},     \
    {"dlsch-demod-shift",        CONFIG_HLP_DLSHIFT,  0,      iptr:(int32_t *)&dlsch_demod_shift, defintval:0,         TYPE_INT,    0},        \
    {"usrp-args",               CONFIG_HLP_USRP_ARGS,   0,                      strptr:(char **)&usrp_args,         defstrval:"type=b200",          TYPE_STRING,    0},                     \
    {"usrp-clksrc",             CONFIG_HLP_USRP_CLK_SRC,0,                      strptr:(char **)&usrp_clksrc,       defstrval:"internal",           TYPE_STRING,    0}                     \
  }

#define DEFAULT_DLF 2680000000

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                                                */
/*   optname                     helpstr                paramflags                      XXXptr                  defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_PARAMS_DESC {  \
    {"rf-config-file",          CONFIG_HLP_RFCFGF,      0,                      strptr:(char **)&rf_config_file,    defstrval:NULL,                 TYPE_STRING,    sizeof(rf_config_file)}, \
    {"ulsch-max-errors",        CONFIG_HLP_ULMAXE,      0,                      uptr:&ULSCH_max_consecutive_errors, defuintval:0,                   TYPE_UINT,      0},                     \
    {"phy-test",                CONFIG_HLP_PHYTST,      PARAMFLAG_BOOL,         iptr:&phy_test,                     defintval:0,                    TYPE_INT,       0},                     \
    {"usim-test",               CONFIG_HLP_USIM,        PARAMFLAG_BOOL,         u8ptr:&usim_test,                   defintval:0,                    TYPE_UINT8,     0},                     \
    {"external-clock",          CONFIG_HLP_EXCCLK,      PARAMFLAG_BOOL,         uptr:&clock_source,                 defintval:0,                    TYPE_INT,       0},                     \
    {"single-thread-disable",   CONFIG_HLP_NOSNGLT,     PARAMFLAG_BOOL,         iptr:&single_thread_flag,           defintval:1,                    TYPE_INT,       0},                     \
    {"nr-dlsch-demod-shift",    CONFIG_HLP_DLSHIFT,     0,                      iptr:(int32_t *)&nr_dlsch_demod_shift, defintval:0,                     TYPE_INT,       0},                     \
    {"A" ,                      CONFIG_HLP_TADV,        0,                      uptr:&timing_advance,               defintval:0,                    TYPE_UINT,      0},                     \
    {"C" ,                      CONFIG_HLP_DLF,         0,                      uptr:&(downlink_frequency[0][0]),   defuintval:2680000000,          TYPE_UINT,      0},                     \
    {"a" ,                      CONFIG_HLP_CHOFF,       0,                      iptr:&chain_offset,                 defintval:0,                    TYPE_INT,       0},                     \
    {"d" ,                      CONFIG_HLP_SOFTS,       PARAMFLAG_BOOL,         iptr:&do_forms,                    defintval:0,                     TYPE_INT,      0},                     \
    {"E" ,                      CONFIG_HLP_TQFS,        PARAMFLAG_BOOL,         iptr:&threequarter_fs,             defintval:0,                     TYPE_INT,      0},                     \
    {"m" ,                      CONFIG_HLP_DLMCS,       0,                      uptr:&target_dl_mcs,                defintval:0,                    TYPE_UINT,      0},                     \
    {"t" ,                      CONFIG_HLP_ULMCS,       0,                      uptr:&target_ul_mcs,                defintval:0,                    TYPE_UINT,      0},                     \
    {"W" ,                      CONFIG_HLP_L2MONW,      0,                      strptr:(char **)&in_ip,             defstrval:"127.0.0.1",          TYPE_STRING,    sizeof(in_ip)},         \
    {"P" ,                      CONFIG_HLP_L2MONP,      0,                      strptr:(char **)&in_path,           defstrval:"/tmp/oai_opt.pcap",  TYPE_STRING,    sizeof(in_path)},       \
    {"q" ,                      CONFIG_HLP_STMON,       PARAMFLAG_BOOL,         iptr:&opp_enabled,                  defintval:0,                    TYPE_INT,       0},                     \
    {"S" ,                      CONFIG_HLP_MSLOTS,      PARAMFLAG_BOOL,         u8ptr:&exit_missed_slots,           defintval:1,                    TYPE_UINT8,     0},                     \
    {"T" ,                      CONFIG_HLP_TDD,         PARAMFLAG_BOOL,         iptr:&tddflag,                      defintval:0,                    TYPE_INT,       0},                     \
    {"numerology" ,             CONFIG_HLP_NUMEROLOGY,  PARAMFLAG_BOOL,         iptr:&numerology,                   defintval:0,                    TYPE_INT,       0},                     \
    {"emulate-rf" ,             CONFIG_HLP_EMULATE_RF,  PARAMFLAG_BOOL,         iptr:&emulate_rf,                   defintval:0,                    TYPE_INT,       0},                     \
    {"parallel-config",         CONFIG_HLP_PARALLEL_CMD,0,                      strptr:(char **)&parallel_config,   defstrval:NULL,                 TYPE_STRING,    0},                     \
    {"worker-config",           CONFIG_HLP_WORKER_CMD,  0,                      strptr:(char **)&worker_config,     defstrval:NULL,                 TYPE_STRING,    0},                     \
    {"nbiot-disable",           CONFIG_HLP_DISABLNBIOT, PARAMFLAG_BOOL,         iptr:&nonbiotflag,      defintval:0,      TYPE_INT, 0} \
  }

#define CONFIG_HLP_FLOG          "Enable online log \n"
#define CONFIG_HLP_LOGL          "Set the global log level, valide options: (9:trace, 8/7:debug, 6:info, 4:warn, 3:error)\n"
#define CONFIG_HLP_LOGV          "Set the global log verbosity \n"
#define CONFIG_HLP_TELN          "Start embedded telnet server \n"
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for LOG utility                                                                                        */
/*   optname                     helpstr                paramflags                      XXXptr                  defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_LOGPARAMS_DESC {  \
    {"R" ,           CONFIG_HLP_FLOG, 0,                uptr:&online_log_messages,    defintval:1,         TYPE_INT,    0},        \
    {"g" ,           CONFIG_HLP_LOGL, 0,      uptr:&glog_level,     defintval:0,         TYPE_UINT,     0},        \
    {"G" ,                           CONFIG_HLP_LOGV, 0,      uptr:&glog_verbosity,           defintval:0,         TYPE_UINT16,   0},        \
    {"telnetsrv",        CONFIG_HLP_TELN, PARAMFLAG_BOOL,   uptr:&start_telnetsrv,    defintval:0,         TYPE_UINT,     0},        \
  }
#define CMDLINE_ONLINELOG_IDX     0
#define CMDLINE_GLOGLEVEL_IDX     1
#define CMDLINE_GLOGVERBO_IDX     2
#define CMDLINE_STARTTELN_IDX     3


extern int T_port;
extern int T_nowait;
extern int T_dont_fork;



/***************************************************************************************************************************************/

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];

extern int rx_input_level_dBm;
extern uint8_t exit_missed_slots;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern int oaisim_flag;
extern volatile int  oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;
extern int transmission_mode;
extern double cpuf;

#if defined(ENABLE_ITTI)
  extern volatile int             start_eNB;
  extern volatile int             start_UE;
#endif

// In nr-ue.c
extern int setup_ue_buffers(PHY_VARS_NR_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);
extern void init_UE(int);
extern void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, cpu_set_t *cpuset, char *name);
extern void reset_opp_meas(void);
extern void print_opp_meas(void);
void *UE_thread(void *arg);
PHY_VARS_NR_UE *init_nr_ue_vars(NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t UE_id,
                                uint8_t abstraction_flag);
extern tpool_t *Tpool;
#endif
