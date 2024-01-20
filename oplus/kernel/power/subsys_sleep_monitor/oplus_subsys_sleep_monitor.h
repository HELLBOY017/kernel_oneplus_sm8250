#ifndef _OPLUS_SUBSYS_SLEEP_MONITOR_H
#define _OPLUS_SUBSYS_SLEEP_MONITOR_H

#define SLEEPMONITOR_SMEM_MODEM_PID  1
#define SLEEPMONITOR_SMEM_ADSP_PID   2
#define SLEEPMONITOR_SMEM_SSC_PID    3
#define SLEEPMONITOR_SMEM_CDSP_PID   5

#define LPRINFO_SMEM_ID      590
#define SLEEPVOTERS_SMEM_ID  591

#define NUM_COMPONENT_MODES     15
#define NUM_LPRM_MAX            5
#define NUM_SYNTH_MODES         6
#define XO_VOTERS_NUM           4
#define MODE_CHECK_REASON_NUM   5
#define NUM_CLIENT_NODES        2
#define NUM_NPA_NODES           3

#define MODE_SKIP_REASON_LPRM_DISABLED          0x2
#define MODE_SKIP_REASON_SLEEP_DURATION_ISFC    0x4
#define MODE_SKIP_REASON_LPRM_LATENCY_RSTR      0x8
#define MODE_SKIP_REASON_REGISTRY_DISABLED      0x10
#define MODE_SKIP_REASON_DEADLINE_IN_PAST       0x20

#if !defined(UINT32_MAX)
#define UINT32_MAX 4294967295U
#endif

#if !defined(UINT64_MAX)
#define UINT64_MAX 18446744073709551615ULL
#endif

#ifndef US_PER_SEC
#define US_PER_SEC 1000000ULL
#endif

#define TIMER_MAX_EXPIRY (1042499uLL * 3600uLL * 19200000uLL)


typedef struct xo_voters_t
{
	char xoclks[XO_VOTERS_NUM][32];
}sleep_xovoters;

typedef struct latency_voters_t
{
	u32 normalLatency;			//us
	char normalLatencyReqClient[32];
	u32 islandLatency;			//ticks
	u32 islands;
	u32 latencyBudget;			//ticks
	u32 modeLatency[NUM_SYNTH_MODES];	//ticks
}sleep_latencyvoters;


typedef struct sleep_duration_voters_t
{
	u64 normalTimerExpiryAbs;
	u64 normalTimerExpiry;
	u32 normalTimerTid;
	char normalTimerTName[32];
	u64 islandTimerExpiryAbs;
	u64 islandTimerExpiry;
	u32 islandId;
	u64 minTimerExpiry;
	u64 softDuration;
	u64 sleepDuration;
	u32 modeDuration[NUM_SYNTH_MODES];
}sleep_durationvoters;


typedef struct lprm_t
{
	u8 num_lprm;
	bool lprmModeSupt[NUM_LPRM_MAX];
}sleep_lprm_info;

typedef struct lpr_t
{
	u8 num_synth_modes;
	u8 num_lpr;
	char lpr[NUM_COMPONENT_MODES][16];
	sleep_lprm_info lprm[NUM_COMPONENT_MODES];
}subsys_lprinfo;

typedef struct npa_voters_t
{
	u8 lpr_index;
	char resourceName[32];
	char clientName[NUM_CLIENT_NODES][32];
}sleep_npavoters;

typedef struct lpr_voters_t
{
	bool lprmModeEnabled[NUM_LPRM_MAX];
}sleep_lprmvoters;

typedef struct synth_voters_t
{
	u8 modeChosen;
	sleep_lprmvoters lprmodes[NUM_COMPONENT_MODES];
	sleep_npavoters lprnpavoters[NUM_NPA_NODES];
}sleep_synthvoters;

typedef struct active_thread_t
{
	bool stm;
	u32 pid,tid;
	char pname[32];
	char tname[32];
}sleep_activethread;

typedef struct modeskip_reason_t
{
	u32 reason;
}sleep_modereason;

typedef struct sleep_voters_t
{
	sleep_xovoters        xo_voters;
	sleep_latencyvoters   latency_voters;
	sleep_durationvoters  duration_voters;
	sleep_synthvoters     synth_mode_voters;
	sleep_activethread    active_thread;
	sleep_modereason      modeskip_reason[NUM_SYNTH_MODES];
}subsys_sleepmon;


#define SUBSYSTEM_MODE_REASON_ACTIVE_THREAD  0x1
#define SUBSYSTEM_MODE_REASON_LPRM  0x2
#define SUBSYSTEM_MODE_REASON_SLEEP_DURATION  0x4
#define SUBSYSTEM_MODE_REASON_LATENCY  0x8
#define SUBSYSTEM_MODE_REASON_CLOCK  0x10

char *modechosen_reason[] = {
	"None",
	"Active_thread",
	"LPRM_Disabled",
	"Duration_ISFC",
	"Latency_RSTR",
	"Clocks_Vote_XO"
};

char *mode_check_result[] = {
	"       OK       |",
	"      FAIL      |"
};


#endif
