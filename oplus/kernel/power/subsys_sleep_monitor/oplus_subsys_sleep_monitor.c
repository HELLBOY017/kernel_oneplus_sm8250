#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/soc/qcom/smem.h>
#include <clocksource/arm_arch_timer.h>
#include <linux/soc/qcom/smem.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include <oplus_subsys_sleep_monitor.h>


static struct proc_dir_entry *subsys_proc = NULL;

static bool adsp_smem_init_done = false;
static bool cdsp_smem_init_done = false;
static bool slpi_smem_init_done = false;

static subsys_lprinfo *g_adsp_lprinfo = NULL;
static subsys_sleepmon *g_adsp_sleepmon = NULL;

static subsys_lprinfo *g_cdsp_lprinfo = NULL;
static subsys_sleepmon *g_cdsp_sleepmon = NULL;

static subsys_lprinfo *g_slpi_lprinfo = NULL;
static subsys_sleepmon *g_slpi_sleepmon = NULL;


static u64 ticks_to_us(u64 ticks){
	if((ticks != UINT32_MAX) && (ticks != UINT64_MAX)) {
		return ((ticks*10)/192);
	}
	return ticks;
}

static int check_sleep_mode_chosen(subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon)
{
	int i,j;
	int reason = 0;
	u8 synth_modes_cnt = g_subsys_lprinfo->num_synth_modes;

	/*
	if(!smem_init_done)
		return 0;
	*/

	if(g_subsys_sleepmon->active_thread.tid != 0) {    //active thread check
		reason |= SUBSYSTEM_MODE_REASON_ACTIVE_THREAD;
	}

	for(i = 0; i < g_subsys_lprinfo->num_lpr; i++) {
		for(j = 0; j < g_subsys_lprinfo->lprm[i].num_lprm; j++) {
			if((g_subsys_sleepmon->synth_mode_voters.lprmodes[i].lprmModeEnabled[j] == false) &&
					(g_subsys_lprinfo->lprm[i].lprmModeSupt[j] == true)) {
				reason |= SUBSYSTEM_MODE_REASON_LPRM;
			}
		}
	}

	for(i = 0; i < synth_modes_cnt; i++) {
		if(g_subsys_sleepmon->duration_voters.sleepDuration <
				g_subsys_sleepmon->duration_voters.modeDuration[i]) {    //sleep duration check
			reason |= SUBSYSTEM_MODE_REASON_SLEEP_DURATION;
			break;
		}
	}

	for(i = 0; i < synth_modes_cnt; i++) {
		if(g_subsys_sleepmon->latency_voters.latencyBudget <
				g_subsys_sleepmon->latency_voters.modeLatency[i]) {    //latency check
			reason |= SUBSYSTEM_MODE_REASON_LATENCY;
			break;
		}
	}

	for(i = 0; i < XO_VOTERS_NUM; i++) {
		if(strlen(g_subsys_sleepmon->xo_voters.xoclks[i]) != 0) {
			reason |= SUBSYSTEM_MODE_REASON_CLOCK;
			break;
		}
	}

	return reason;
}


void print_xo_voters(struct seq_file *seq, subsys_sleepmon *g_subsys_sleepmon, bool details)
{
	int i;
	bool has_xo_voters = false;

	if(details){
		seq_printf(seq, "[----XO Voters----]\n");
		seq_printf(seq, "Clocks:\n");
		for(i = 0; i < XO_VOTERS_NUM; i++) {
			if(strlen(g_subsys_sleepmon->xo_voters.xoclks[i]) != 0) {
				has_xo_voters = true;
				seq_printf(seq, "    %s\n", g_subsys_sleepmon->xo_voters.xoclks[i]);
			}
		}
		if(!has_xo_voters) {
			seq_printf(seq, "    No clocks vote XO\n");
		}
	} else {
		seq_printf(seq, "clocks=");
		for(i = 0; i < XO_VOTERS_NUM; i++) {
			if(strlen(g_subsys_sleepmon->xo_voters.xoclks[i]) != 0) {
				seq_printf(seq, "%s|", g_subsys_sleepmon->xo_voters.xoclks[i]);
			}
		}
	}
	seq_printf(seq, "\n");
}

void print_latency_voters(struct seq_file *seq, subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon, bool details)
{
	int i;
	u8 synth_modes_cnt = g_subsys_lprinfo->num_synth_modes;

	if(details){
		seq_printf(seq, "[----Latency vote----]\n");
		seq_printf(seq, "mode      |");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%16d|", i);
		}
		seq_printf(seq, "\n");

		seq_printf(seq, "latency   |");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%16d|",
					ticks_to_us(g_subsys_sleepmon->latency_voters.modeLatency[i]));
		}
		seq_printf(seq, "\n");

		seq_printf(seq, "Normal latency: %d\n", g_subsys_sleepmon->latency_voters.normalLatency);
		seq_printf(seq, "    voter: %s\n", g_subsys_sleepmon->latency_voters.normalLatencyReqClient);
		seq_printf(seq, "Island latency: %d\n", ticks_to_us(g_subsys_sleepmon->latency_voters.islandLatency));
		seq_printf(seq, "    islands: %d\n", g_subsys_sleepmon->latency_voters.islands);

		seq_printf(seq, "Sleep latency budget: %d\n", ticks_to_us(g_subsys_sleepmon->latency_voters.latencyBudget));
		seq_printf(seq, "\n");
	} else {
		seq_printf(seq, "latency_mode=");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%d|",
					ticks_to_us(g_subsys_sleepmon->latency_voters.modeLatency[i]));
		}
		seq_printf(seq, "\n");

		if((g_subsys_sleepmon->latency_voters.islandLatency == UINT32_MAX) ||
			(g_subsys_sleepmon->latency_voters.islandLatency == 0) ||
			(g_subsys_sleepmon->latency_voters.normalLatency <= g_subsys_sleepmon->latency_voters.islandLatency)) {
			seq_printf(seq, "latency_normal=%d(%s)\n", g_subsys_sleepmon->latency_voters.normalLatency,
					g_subsys_sleepmon->latency_voters.normalLatencyReqClient);
		} else {
			seq_printf(seq, "latency_island=%d(%d)\n", ticks_to_us(g_subsys_sleepmon->latency_voters.islandLatency),
					g_subsys_sleepmon->latency_voters.islands);
		}
	}
}

void print_sleep_duration_voters(struct seq_file *seq, subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon,bool details)
{
	int i;
	u8 synth_modes_cnt = g_subsys_lprinfo->num_synth_modes;

	if(details){
		seq_printf(seq, "[----Sleep duration vote----]\n");
		seq_printf(seq, "mode      |");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%16d|", i);
		}
		seq_printf(seq, "\n");

		seq_printf(seq, "duration  |");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%16d|",
					ticks_to_us(g_subsys_sleepmon->duration_voters.modeDuration[i]));
		}
		seq_printf(seq, "\n");

		//if no non-deferrable timer present, normal timer equal TIMER_MAX_EXPIRY
		if(g_subsys_sleepmon->duration_voters.normalTimerExpiryAbs < TIMER_MAX_EXPIRY) {
			seq_printf(seq, "Normal timer: relative = %lld, abs = %lld\n",
				ticks_to_us(g_subsys_sleepmon->duration_voters.normalTimerExpiry),
				ticks_to_us(g_subsys_sleepmon->duration_voters.normalTimerExpiryAbs));
			seq_printf(seq, "    voter: %u(%s)\n", g_subsys_sleepmon->duration_voters.normalTimerTid,
					g_subsys_sleepmon->duration_voters.normalTimerTName);
		} else {
			seq_printf(seq, "Normal timer: relative = %lld, abs = %lld\n", UINT64_MAX, UINT64_MAX);
			seq_printf(seq, "    voter: %u(%s)\n", 0, "NA");
		}

		if(g_subsys_sleepmon->duration_voters.islandTimerExpiryAbs < TIMER_MAX_EXPIRY) {
			seq_printf(seq, "Island timer: relative = %lld, abs = %lld\n",
					ticks_to_us(g_subsys_sleepmon->duration_voters.islandTimerExpiry),
					ticks_to_us(g_subsys_sleepmon->duration_voters.islandTimerExpiryAbs));
			seq_printf(seq, "    island id: %d (refer uSleep_islands.h)\n", g_subsys_sleepmon->duration_voters.islandId);
		} else {
			seq_printf(seq, "Island timer: relative = %lld, abs = %lld\n", UINT64_MAX, UINT64_MAX);
			seq_printf(seq, "    island id: %d (refer uSleep_islands.h)\n", UINT32_MAX);
		}
		
		if(g_subsys_sleepmon->duration_voters.minTimerExpiry >= UINT64_MAX) {
			seq_printf(seq, "min timer: %lld\n", UINT64_MAX);
		} else {
			seq_printf(seq, "min timer: %lld\n", ticks_to_us(g_subsys_sleepmon->duration_voters.minTimerExpiry));
		}
		seq_printf(seq, "Soft duration: %d\n", ticks_to_us(g_subsys_sleepmon->duration_voters.softDuration));

		/*Sleep Duration is uint64 type value, Soft duration is uint32 type value
		 *most time Soft duration is -1, when Sleep duration equal Soft duration, will print 4294967295
		 *we hope Sleep duration also print -1 to easy understand
		 *
		 *Soft duration: -1
		 *Sleep Duration: 4294967295
		 *
		 *change to
		 *Soft duration: -1
		 *Sleep Duration: -1
		 */
		if(g_subsys_sleepmon->duration_voters.sleepDuration <= UINT32_MAX) {
			seq_printf(seq, "Sleep Duration: %d\n", ticks_to_us(g_subsys_sleepmon->duration_voters.sleepDuration));
		} else {
			seq_printf(seq, "Sleep Duration: %lld\n", ticks_to_us(g_subsys_sleepmon->duration_voters.sleepDuration));
		}
		seq_printf(seq, "\n");
	} else {
		seq_printf(seq, "duration_mode=");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%d|", ticks_to_us(g_subsys_sleepmon->duration_voters.modeDuration[i]));
		}
		seq_printf(seq, "\n");

		if(g_subsys_sleepmon->duration_voters.sleepDuration ==
			g_subsys_sleepmon->duration_voters.normalTimerExpiry) {
			seq_printf(seq, "duration_normal=%lld(%s)\n", ticks_to_us(g_subsys_sleepmon->duration_voters.normalTimerExpiry),
					g_subsys_sleepmon->duration_voters.normalTimerTName);

		} else if(g_subsys_sleepmon->duration_voters.sleepDuration ==
				g_subsys_sleepmon->duration_voters.islandTimerExpiry) {
			seq_printf(seq, "duration_island=%lld(%d)\n", ticks_to_us(g_subsys_sleepmon->duration_voters.islandTimerExpiry),
				g_subsys_sleepmon->duration_voters.islandId);
		//} else if((g_subsys_sleepmon->duration_voters.softDuration != UINT32_MAX) &&
		//	(g_subsys_sleepmon->duration_voters.minTimerExpiry >
		//		g_subsys_sleepmon->duration_voters.softDuration)) {
		} else if(g_subsys_sleepmon->duration_voters.minTimerExpiry >
				g_subsys_sleepmon->duration_voters.softDuration) {
			seq_printf(seq, "duration_soft=%d\n", ticks_to_us(g_subsys_sleepmon->duration_voters.softDuration));
		}
	}
}

void print_LPRM_voters(struct seq_file *seq, subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon, bool details)
{
	int i,j,m,n;
	bool has_lprm_disabled = false;
	bool lprm_disabled = false;

	if(details){
		seq_printf(seq, "[----LPRM vote----]\n");
		for(i = 0; i < g_subsys_lprinfo->num_lpr; i++) {
			lprm_disabled = false;
			seq_printf(seq, "LPR: %-16s\n", g_subsys_lprinfo->lpr[i]);
			seq_printf(seq, "    LPRM:   ");
			for(j = 0; j < g_subsys_lprinfo->lprm[i].num_lprm; j++) {
				if(g_subsys_lprinfo->lprm[i].lprmModeSupt[j] == true) {
					seq_printf(seq, "     Mode%d|", j);
				} else {
					seq_printf(seq, "     NSupt|");
				}
			}
			seq_printf(seq, "\n");

			seq_printf(seq, "    Enabled:");
			for(j = 0; j < g_subsys_lprinfo->lprm[i].num_lprm; j++) {
				seq_printf(seq, "%10d|", g_subsys_sleepmon->synth_mode_voters.lprmodes[i].lprmModeEnabled[j]);
				if((g_subsys_lprinfo->lprm[i].lprmModeSupt[j] == true) &&
						g_subsys_sleepmon->synth_mode_voters.lprmodes[i].lprmModeEnabled[j] == 0) {
					lprm_disabled = true;
				}
			}
			seq_printf(seq, "\n");

			if(lprm_disabled) {
				for(n = 0; n < NUM_NPA_NODES; n++) {
					if((g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].lpr_index - 1) == i) {
						seq_printf(seq, "    resource: %16s\n",
								g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].resourceName);
						for(m = 0; m < NUM_CLIENT_NODES; m++) {
							seq_printf(seq, "        client: %16s\n",
									g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].clientName[m]);
						}
					}
				}
			g_subsys_sleepmon->duration_voters.normalTimerTid = 0;
			}
			seq_printf(seq, "\n");
		}
		seq_printf(seq, "\n");
	} else {
		for(i = 0; i < g_subsys_lprinfo->num_lpr; i++) {
			lprm_disabled = false;
			for(j = 0; j < g_subsys_lprinfo->lprm[i].num_lprm; j++) {
				if((g_subsys_lprinfo->lprm[i].lprmModeSupt[j] == true) &&
						g_subsys_sleepmon->synth_mode_voters.lprmodes[i].lprmModeEnabled[j] == 0) {
					lprm_disabled = true;
					has_lprm_disabled = true;
				}
			}

			if(lprm_disabled) {
				for(n = 0; n < NUM_NPA_NODES; n++) {
					if((g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].lpr_index - 1) == i) {
						seq_printf(seq, "resource=%s(",
								g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].resourceName);
						for(m = 0; m < NUM_CLIENT_NODES; m++) {
							if(strlen(g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].clientName[m]) != 0) {
								seq_printf(seq, "%s|",
										g_subsys_sleepmon->synth_mode_voters.lprnpavoters[n].clientName[m]);
							}
						}
						seq_printf(seq, ")");
						seq_printf(seq, "\n");
					}
				}
			}
		}
		seq_printf(seq, "\n");
		if(!has_lprm_disabled) {
			seq_printf(seq, "resource=null\n");
		}
	}
}

void print_active_thread(struct seq_file *seq, subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon, bool details)
{
	u8 synth_modes_cnt = g_subsys_lprinfo->num_synth_modes;

	if(details){
		seq_printf(seq, "[----Active thread----]\n");
		seq_printf(seq, "Processor is %s in sleep context\n",
				g_subsys_sleepmon->active_thread.stm ? "running":"not running");
		if(!g_subsys_sleepmon->active_thread.stm &&
			(g_subsys_sleepmon->synth_mode_voters.modeChosen < synth_modes_cnt)) {
			seq_printf(seq, "Process: %d(%s)\n", g_subsys_sleepmon->active_thread.pid,
					g_subsys_sleepmon->active_thread.pname);
			seq_printf(seq, "    Active Thread: %d(%s)\n", g_subsys_sleepmon->active_thread.tid,
					g_subsys_sleepmon->active_thread.tname);
		} else if((g_subsys_sleepmon->synth_mode_voters.modeChosen >=  synth_modes_cnt) &&
			(g_subsys_sleepmon->modeskip_reason[synth_modes_cnt].reason != MODE_SKIP_REASON_REGISTRY_DISABLED)) {
			seq_printf(seq, "    No mode chosen, leaving idle mode\n");
		}
		seq_printf(seq, "\n");
	} else {
		seq_printf(seq, "active_thread=");
		seq_printf(seq, "%s|%s\n", g_subsys_sleepmon->active_thread.pname,
				g_subsys_sleepmon->active_thread.tname);
	}
}

void print_mode_chosen(struct seq_file *seq, subsys_lprinfo *g_subsys_lprinfo, subsys_sleepmon *g_subsys_sleepmon, bool details)
{
	int i;
	u8 synth_modes_cnt = g_subsys_lprinfo->num_synth_modes;
	int reason_id = 0;

	if(details){
		seq_printf(seq, "Sleep mode chosen: %d from 0 - %d. (0 is deepest sleep level)\n",
				g_subsys_sleepmon->synth_mode_voters.modeChosen, (synth_modes_cnt - 1));
		reason_id = check_sleep_mode_chosen(g_adsp_lprinfo, g_adsp_sleepmon);
		seq_printf(seq, "| active thread  |      LPRM      | sleep duration |    latency     |\n");
		seq_printf(seq, "---------------------------------------------------------------------\n");
		seq_printf(seq, "|");
		for(i = 0; i < MODE_CHECK_REASON_NUM; i++) {
			if(!(reason_id & (1 << i))) {
				seq_printf(seq, "%s", mode_check_result[0]);
			} else {
				seq_printf(seq, "%s", mode_check_result[1]);
			}
		}
		seq_printf(seq, "\n");
		seq_printf(seq, "\n");

		seq_printf(seq, "mode        |");
		for(i = 0; i < synth_modes_cnt; i++) {
			seq_printf(seq, "%16d|", i);
		}
		seq_printf(seq, "\n");

		seq_printf(seq, "skip reason |");
		for(i = 0; i < synth_modes_cnt; i++) {
			if(g_subsys_sleepmon->modeskip_reason[synth_modes_cnt].reason == MODE_SKIP_REASON_REGISTRY_DISABLED) {
				seq_printf(seq, "%16s|", "REGISTRY_Dis");
			} else if(g_subsys_sleepmon->modeskip_reason[i].reason == MODE_SKIP_REASON_LPRM_DISABLED) {
				seq_printf(seq, "%16s|", "LPRM Disabled");
			} else if(g_subsys_sleepmon->modeskip_reason[i].reason == MODE_SKIP_REASON_SLEEP_DURATION_ISFC) {
				seq_printf(seq, "%16s|", "Sleep Duration");
			} else if(g_subsys_sleepmon->modeskip_reason[i].reason == MODE_SKIP_REASON_LPRM_LATENCY_RSTR) {
				seq_printf(seq, "%16s|", "Latency");
			} else {
				seq_printf(seq, "%16s|", "None");
			}
		}
		seq_printf(seq, "\n");
		seq_printf(seq, "\n");
	} else {
		seq_printf(seq, "sleep_mode_chosen=%d|%d\n",g_subsys_sleepmon->synth_mode_voters.modeChosen,
				(synth_modes_cnt - 1));

		seq_printf(seq, "sleep_mode_reason=");
		reason_id = check_sleep_mode_chosen(g_adsp_lprinfo, g_adsp_sleepmon);
		for(i = 0; i < MODE_CHECK_REASON_NUM; i++) {
			if(reason_id & (1 << i)) {
				seq_printf(seq, "%s|", modechosen_reason[i+1]);
			}
		}
		seq_printf(seq, "\n");
	}
}


/*----adsp subsystem sleep info node begin----*/
static int adsp_sleepmon_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_adsp_lprinfo, g_adsp_sleepmon, true);
	seq_printf(seq, "Details below:\n");
	seq_printf(seq, "\n");
	print_xo_voters(seq, g_adsp_sleepmon, true);
	print_active_thread(seq, g_adsp_lprinfo, g_adsp_sleepmon, true);
	print_latency_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, true);
	print_sleep_duration_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, true);
	print_LPRM_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, true);

	return 0;
}

static int adsp_sleepmon_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!adsp_smem_init_done) {

		g_adsp_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_ADSP_PID,
				LPRINFO_SMEM_ID,
				NULL);

		g_adsp_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_ADSP_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_adsp_lprinfo) ||
				IS_ERR_OR_NULL(g_adsp_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			adsp_smem_init_done = true;
		}
	}

	ret = single_open(file, adsp_sleepmon_show, NULL);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops adsp_sleepmon_fops = {
	.proc_open		= adsp_sleepmon_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations adsp_sleepmon_fops = {
	.open		= adsp_sleepmon_open,
	.read		= seq_read,
};
#endif

static int adsp_sleepmon_compact_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_adsp_lprinfo, g_adsp_sleepmon, false);
	print_xo_voters(seq, g_adsp_sleepmon, false);

	print_active_thread(seq, g_adsp_lprinfo, g_adsp_sleepmon, false);
	print_latency_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, false);
	print_sleep_duration_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, false);
	print_LPRM_voters(seq, g_adsp_lprinfo, g_adsp_sleepmon, false);

	return 0;
}

static int adsp_sleepmon_compact_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!adsp_smem_init_done) {

		g_adsp_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_ADSP_PID,
				LPRINFO_SMEM_ID,
				NULL);


		g_adsp_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_ADSP_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_adsp_lprinfo) ||
				IS_ERR_OR_NULL(g_adsp_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			adsp_smem_init_done = true;
		}
	}

	ret = single_open(file, adsp_sleepmon_compact_show, NULL);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops adsp_sleepmon_compact_fops = {
	.proc_open		= adsp_sleepmon_compact_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations adsp_sleepmon_compact_fops = {
	.open		= adsp_sleepmon_compact_open,
	.read		= seq_read,
};
#endif
/*----adsp subsystem sleep info node end----*/


/*----cdsp subsystem sleep info node begin----*/
static int cdsp_sleepmon_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, true);
	seq_printf(seq, "Details below:\n");
	seq_printf(seq, "\n");
	print_xo_voters(seq, g_cdsp_sleepmon, true);
	print_active_thread(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, true);
	print_latency_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, true);
	print_sleep_duration_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, true);
	print_LPRM_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, true);

	return 0;
}

static int cdsp_sleepmon_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!cdsp_smem_init_done) {

		g_cdsp_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_CDSP_PID,
				LPRINFO_SMEM_ID,
				NULL);

		g_cdsp_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_CDSP_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_cdsp_lprinfo) ||
				IS_ERR_OR_NULL(g_cdsp_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			cdsp_smem_init_done = true;
		}
	}

	ret = single_open(file, cdsp_sleepmon_show, NULL);

	return ret;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops cdsp_sleepmon_fops = {
	.proc_open		= cdsp_sleepmon_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations cdsp_sleepmon_fops = {
	.open		= cdsp_sleepmon_open,
	.read		= seq_read,
};
#endif

static int cdsp_sleepmon_compact_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, false);
	print_xo_voters(seq, g_cdsp_sleepmon, false);

	print_active_thread(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, false);
	print_latency_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, false);
	print_sleep_duration_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, false);
	print_LPRM_voters(seq, g_cdsp_lprinfo, g_cdsp_sleepmon, false);

	return 0;
}

static int cdsp_sleepmon_compact_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!cdsp_smem_init_done) {

		g_cdsp_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_CDSP_PID,
				LPRINFO_SMEM_ID,
				NULL);


		g_cdsp_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_CDSP_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_cdsp_lprinfo) ||
				IS_ERR_OR_NULL(g_cdsp_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			cdsp_smem_init_done = true;
		}
	}

	ret = single_open(file, cdsp_sleepmon_compact_show, NULL);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops cdsp_sleepmon_compact_fops = {
	.proc_open		= cdsp_sleepmon_compact_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations cdsp_sleepmon_compact_fops = {
	.open		= cdsp_sleepmon_compact_open,
	.read		= seq_read,
};
#endif
/*----cdsp subsystem sleep info node end----*/


/*----slpi subsystem sleep info node begin----*/
static int slpi_sleepmon_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_slpi_lprinfo, g_slpi_sleepmon, true);
	seq_printf(seq, "Details below:\n");
	seq_printf(seq, "\n");
	print_xo_voters(seq, g_slpi_sleepmon, true);
	print_active_thread(seq, g_slpi_lprinfo, g_slpi_sleepmon, true);
	print_latency_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, true);
	print_sleep_duration_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, true);
	print_LPRM_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, true);

	return 0;
}

static int slpi_sleepmon_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!slpi_smem_init_done) {

		g_slpi_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_SSC_PID,
				LPRINFO_SMEM_ID,
				NULL);

		g_slpi_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_SSC_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_slpi_lprinfo) ||
				IS_ERR_OR_NULL(g_slpi_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			slpi_smem_init_done = true;
		}
	}

	ret = single_open(file, slpi_sleepmon_show, NULL);

	return ret;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops slpi_sleepmon_fops = {
	.proc_open		= slpi_sleepmon_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations slpi_sleepmon_fops = {
	.open		= slpi_sleepmon_open,
	.read		= seq_read,
};
#endif

static int slpi_sleepmon_compact_show(struct seq_file *seq, void *v)
{
	print_mode_chosen(seq, g_slpi_lprinfo, g_slpi_sleepmon, false);
	print_xo_voters(seq, g_slpi_sleepmon, false);

	print_active_thread(seq, g_slpi_lprinfo, g_slpi_sleepmon, false);
	print_latency_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, false);
	print_sleep_duration_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, false);
	print_LPRM_voters(seq, g_slpi_lprinfo, g_slpi_sleepmon, false);

	return 0;
}

static int slpi_sleepmon_compact_open(struct inode *inode, struct file *file)
{
	int ret;

	if (!slpi_smem_init_done) {

		g_slpi_lprinfo = qcom_smem_get(
				SLEEPMONITOR_SMEM_SSC_PID,
				LPRINFO_SMEM_ID,
				NULL);


		g_slpi_sleepmon = qcom_smem_get(
				SLEEPMONITOR_SMEM_SSC_PID,
				SLEEPVOTERS_SMEM_ID,
				NULL);

		if (IS_ERR_OR_NULL(g_slpi_lprinfo) ||
				IS_ERR_OR_NULL(g_slpi_sleepmon)) {
			pr_err("smem get failed !!!\n");
			return -ENOMEM;
		} else {
			slpi_smem_init_done = true;
		}
	}

	ret = single_open(file, slpi_sleepmon_compact_show, NULL);

	return ret;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops slpi_sleepmon_compact_fops = {
	.proc_open		= slpi_sleepmon_compact_open,
	.proc_read		= seq_read,
};
#else
static const struct file_operations slpi_sleepmon_compact_fops = {
	.open		= slpi_sleepmon_compact_open,
	.read		= seq_read,
};
#endif
/*----slpi subsystem sleep info node end----*/


static int __init subsys_sleepmon_init(void)
{
	subsys_proc = proc_mkdir("subsys_sleepmon", NULL);

	proc_create("adsp", 0444, subsys_proc, &adsp_sleepmon_fops);
	proc_create("adsp_compact", 0444, subsys_proc, &adsp_sleepmon_compact_fops);

	proc_create("cdsp", 0444, subsys_proc, &cdsp_sleepmon_fops);
	proc_create("cdsp_compact", 0444, subsys_proc, &cdsp_sleepmon_compact_fops);

	proc_create("slpi", 0444, subsys_proc, &slpi_sleepmon_fops);
	proc_create("slpi_compact", 0444, subsys_proc, &slpi_sleepmon_compact_fops);

	return 0;
}
module_init(subsys_sleepmon_init);

static void __exit subsys_sleepmon_exit(void)
{
	remove_proc_entry("adsp", subsys_proc);
	remove_proc_entry("adsp_compact", subsys_proc);

	remove_proc_entry("cdsp", subsys_proc);
	remove_proc_entry("cdsp_compact", subsys_proc);

	remove_proc_entry("slpi", subsys_proc);
	remove_proc_entry("slpi_compact", subsys_proc);

	remove_proc_entry("subsys_sleepmon", NULL);
}
module_exit(subsys_sleepmon_exit);

MODULE_AUTHOR("Colin.Liu");
MODULE_DESCRIPTION("Subsys sleep monitor module");
MODULE_LICENSE("GPL v2");
