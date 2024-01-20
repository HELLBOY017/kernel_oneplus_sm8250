// SPDX-License-Identifier: GPL-2.0
/*
 * CPUFreq governor based on scheduler-provided CPU utilization data.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#ifdef CONFIG_ARCH_MEDIATEK
#include <../kernel/sched/sched.h>
#include <trace/hooks/cpuidle.h>
#include "../misc/mediatek/sched/common.h"
#else
#include <../../../kernel/sched/walt/walt.h>
#endif /* CONFIG_ARCH_MEDIATEK */
#include <linux/sched/cpufreq.h>
#include <trace/events/power.h>
#include <trace/events/sched.h>

#include "cpufreq_uag.h"
#include "nonlinear_opp_cap.h"

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
#include "stall_util_cal.h"
#endif

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
#include "../frame_boost/frame_group.h"
#endif
#include <linux/version.h>
#include "../../power/cpufreq_health/cpufreq_health.h"
#include <linux/kernel.h>
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
#include "../sched_assist/eas_opt/oplus_iowait.h"
#endif

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
#include "../sched_assist/eas_opt/oplus_cap.h"
#endif

#ifdef CONFIG_OPLUS_MULTI_LV_TL
#define DEFAULT_MULTI_TL_SYS 95
static unsigned int default_target_loads_sys[] = {DEFAULT_MULTI_TL_SYS};
#endif

#define IOWAIT_BOOST_MIN        (SCHED_CAPACITY_SCALE / 8)

#ifdef CONFIG_OPLUS_UAG_USE_TL
/* Target load.  Lower values result in higher CPU speeds. */
#define DEFAULT_TARGET_LOAD 80
static unsigned int default_target_loads[] = {DEFAULT_TARGET_LOAD};
#endif

#ifdef CONFIG_OPLUS_UAG_SOFT_LIMIT
#define DEFAULT_BREAK_FREQ_MARGIN 20
#endif

struct uag_gov_cpu {
#ifdef CONFIG_ARCH_MEDIATEK
	struct update_util_data	update_util;
	unsigned int		prev_frame_loading;	/* prev frame idle time percentage */
	u64			enter_idle;			/* cpu latest enter idle timestamp */
	u64			prev_enter_ilde; 	/* cpu last time enter idle timestamp */
	u64			exit_idle; 			/* cpu latest exit idle timestamp */
	u64			prev_exit_idle;		/* cpu last time exit idle timestamp */
	u64			idle_duration;		/* cpu current frame window in idle state time */
	u64			prev_idle_duration;	/* cpu prev frame window in idle state time */
#else
	struct update_util_data	update_util;
	struct walt_cpu_load	walt_load;
#endif /* CONFIG_ARCH_MEDIATEK */
	struct uag_gov_policy	*sg_policy;
	unsigned int		cpu;

	bool			iowait_boost_pending;
	unsigned int		iowait_boost;
	u64			last_update;

	unsigned long		bw_dl;
	unsigned long		max;
	unsigned long		util;
	unsigned int		flags;
};

static DEFINE_PER_CPU(struct uag_gov_cpu, uag_gov_cpu);
static DEFINE_PER_CPU(struct uag_gov_tunables *, cached_tunables);

/************************ Governor internals ***********************/

static bool uag_gov_should_update_freq(struct uag_gov_policy *sg_policy, u64 time)
{
	s64 delta_ns;

	/*
	 * Since cpufreq_update_util() is called with rq->lock held for
	 * the @target_cpu, our per-CPU data is fully serialized.
	 *
	 * However, drivers cannot in general deal with cross-CPU
	 * requests, so while get_next_freq() will work, our
	 * uag_gov_update_commit() call may not for the fast switching platforms.
	 *
	 * Hence stop here for remote requests if they aren't supported
	 * by the hardware, as calculating the frequency is pointless if
	 * we cannot in fact act on it.
	 *
	 * This is needed on the slow switching platforms too to prevent CPUs
	 * going offline from leaving stale IRQ work items behind.
	 */
#ifdef CONFIG_ARCH_MEDIATEK
	if (!cpufreq_this_cpu_can_update(sg_policy->policy))
		return false;
#endif /* CONFIG_ARCH_MEDIATEK */

	if (unlikely(sg_policy->limits_changed)) {
		sg_policy->limits_changed = false;
		sg_policy->need_freq_update = true;
		return true;
	}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	if (sg_policy->flags & SCHED_CPUFREQ_DEF_FRAMEBOOST)
		return true;
#endif

	delta_ns = time - sg_policy->last_freq_update_time;

	return delta_ns >= sg_policy->freq_update_delay_ns;
}

static bool uag_gov_update_next_freq(struct uag_gov_policy *sg_policy, u64 time,
				   unsigned int next_freq)
{
	if (!sg_policy->need_freq_update) {
		if (sg_policy->next_freq == next_freq)
			return false;
	}

	sg_policy->next_freq = next_freq;
	sg_policy->last_freq_update_time = time;

	return true;
}

#ifdef CONFIG_ARCH_QCOM
#define KHZ 1000
static void uag_track_cycles(struct uag_gov_policy *sg_policy,
				unsigned int prev_freq,
				u64 upto)
{
	u64 delta_ns, cycles;
	u64 next_ws = sg_policy->last_ws + sched_ravg_window;

	upto = min(upto, next_ws);
	/* Track cycles in current window */
	delta_ns = upto - sg_policy->last_cyc_update_time;
	delta_ns *= prev_freq;
	do_div(delta_ns, (NSEC_PER_SEC / KHZ));
	cycles = delta_ns;
	sg_policy->curr_cycles += cycles;
	sg_policy->last_cyc_update_time = upto;
}
#endif

static void uag_gov_fast_switch(struct uag_gov_policy *sg_policy, u64 time,
			      unsigned int next_freq)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	int cpu;

	if (!uag_gov_update_next_freq(sg_policy, time, next_freq))
		return;

	uag_track_cycles(sg_policy, sg_policy->policy->cur, time);
	next_freq = cpufreq_driver_fast_switch(sg_policy->policy, next_freq);
	if (!next_freq)
		return;

	policy->cur = next_freq;

	if (trace_cpu_frequency_enabled()) {
		for_each_cpu(cpu, policy->cpus)
			trace_cpu_frequency(next_freq, cpu);
	}
}

static void uag_gov_deferred_update(struct uag_gov_policy *sg_policy, u64 time,
				  unsigned int next_freq)
{
	if (!uag_gov_update_next_freq(sg_policy, time, next_freq))
		return;

	if (!sg_policy->work_in_progress) {
		sg_policy->work_in_progress = true;
		irq_work_queue(&sg_policy->irq_work);
	}
}

#ifdef CONFIG_OPLUS_UAG_USE_TL
#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
static inline unsigned int get_opp_capacity(struct cpufreq_policy *policy,
					  int row)
{
	return (int)pd_get_opp_capacity(policy->cpu, row);
}
#else
static inline unsigned int get_opp_capacity(struct cpufreq_policy *policy,
					  int row)
{
	unsigned int cap, orig_cap;
	unsigned long freq, max_freq;

	max_freq = policy->cpuinfo.max_freq;
	orig_cap = capacity_orig_of(policy->cpu);

	freq = policy->freq_table[row].frequency;
	cap = orig_cap * freq / max_freq;

	return cap;
}

static inline int get_opp_count(struct cpufreq_policy *policy)
{
	int opp_nr;
	struct cpufreq_frequency_table *freq_pos;

	cpufreq_for_each_entry_idx(freq_pos, policy->freq_table, opp_nr);
	return opp_nr;
}
#endif /* CONFIG_UAG_NONLINEAR_FREQ_CTL */

#ifdef CONFIG_OPLUS_MULTI_LV_TL
static inline unsigned long uag_has_ua_util(struct uag_gov_policy *sg_policy)
{
	return sg_policy->multi_util[UA_UTIL_FBG];
}

static inline unsigned int uag_select_multi_tl(
		struct uag_gov_policy *sg_policy, unsigned int util)
{
	struct uag_gov_tunables *tunables = sg_policy->tunables;
	struct multi_target_loads *multi_tl = &tunables->multi_tl[sg_policy->multi_util_type];
	unsigned int tl = 0;
	int i;

	if (!tunables->multi_tl_enable || !uag_has_ua_util(sg_policy))
		return tl;

	for (i = 0; i < multi_tl->ntarget_loads - 1 &&
			 util >= multi_tl->util_loads[i+1]; i += 2) {
	}

	tl = multi_tl->util_loads[i];

	trace_uag_select_multi_tl(sg_policy, util, tl);

	return tl;
}
#endif

static unsigned int util_to_targetload(
	struct uag_gov_policy *sg_policy, unsigned int util)
{
	struct uag_gov_tunables *tunables = sg_policy->tunables;
	int i;
	unsigned int ret;
	unsigned long flags;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

#ifdef CONFIG_OPLUS_MULTI_LV_TL
	ret = uag_select_multi_tl(sg_policy, util);
	if (ret) {
		spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
		return ret;
	}
#endif

	for (i = 0; i < tunables->ntarget_loads - 1 &&
		     util >= tunables->util_loads[i+1]; i += 2) {
	}

	ret = tunables->util_loads[i];
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}
#ifdef CONFIG_ARCH_MEDIATEK
unsigned int find_util_l(struct uag_gov_policy *sg_policy, unsigned int util)
{
	unsigned int capacity;
	int idx;

	for (idx = sg_policy->len-1; idx >= 0; idx--) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
		if (capacity >= util){
			return capacity;
		}
	}
	return get_opp_capacity(sg_policy->policy, 0);
}

unsigned int find_util_h(struct uag_gov_policy *sg_policy, unsigned int util)
{
	unsigned int capacity;
	int idx;
	int target_idx = -1;

	for (idx = sg_policy->len-1; idx >= 0; idx--) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
		if (capacity ==  util) {
			return util;
		}
		if (capacity < util) {
			target_idx = idx;
			continue;
		}
		if (target_idx == -1)
			return capacity;
		return get_opp_capacity(sg_policy->policy, target_idx);
	}
	return get_opp_capacity(sg_policy->policy, target_idx);
}
#else
unsigned int find_util_l(struct uag_gov_policy *sg_policy, unsigned int util)
{
	unsigned int capacity;
	int idx;

	for (idx = 0; idx < sg_policy->len; idx++) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
		if (capacity >= util) {
			return capacity;
		}
	}
	return get_opp_capacity(sg_policy->policy, sg_policy->len-1);
}

unsigned int find_util_h(struct uag_gov_policy *sg_policy, unsigned int util)
{
	unsigned int capacity;
	int idx;
	int target_idx = -1;

	for (idx = 0; idx < sg_policy->len; idx++) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
		if (capacity ==  util) {
			return util;
		}
		if (capacity < util) {
			target_idx = idx;
			continue;
		}
		if (target_idx == sg_policy->len)
			return capacity;
		return get_opp_capacity(sg_policy->policy, target_idx);
	}
	return get_opp_capacity(sg_policy->policy, target_idx);
}
#endif /* CONFIG_ARCH_MEDIATEK */
unsigned int find_closest_util(struct uag_gov_policy *sg_policy, unsigned int util,
				  unsigned int policy)
{
	switch (policy) {
	case CPUFREQ_RELATION_L:
		return find_util_l(sg_policy, util);
	case CPUFREQ_RELATION_H:
		return find_util_h(sg_policy, util);
	default:
		return util;
	}
}

unsigned int choose_util(struct uag_gov_policy *sg_policy,
		unsigned int util)
{
	unsigned int prevutil, utilmin, utilmax;
	unsigned int tl;
	unsigned long orig_util = util;

	if (!sg_policy) {
		pr_err("sg_policy is null\n");
		return -EINVAL;
	}

	utilmin = 0;
	utilmax = UINT_MAX;

	do {
		prevutil = util;
		tl = util_to_targetload(sg_policy, util);
		/*
		 * Find the lowest frequency where the computed load is less
		 * than or equal to the target load.
		 */

		util = find_closest_util(sg_policy, (orig_util * 100 / tl), CPUFREQ_RELATION_L);
		trace_choose_util(sg_policy->policy->cpu, util, prevutil, utilmax, utilmin, tl);
		if (util > prevutil) {
			/* The previous frequency is too low. */
			utilmin = prevutil;

			if (util >= utilmax) {
				/*
				 * Find the highest frequency that is less
				 * than freqmax.
				 */
				util = find_closest_util(sg_policy, utilmax - 1,CPUFREQ_RELATION_H);
				if (util == utilmin) {
					/*
					 * The first frequency below freqmax
					 * has already been found to be too
					 * low.  freqmax is the lowest speed
					 * we found that is fast enough.
					 */
					util = utilmax;
					break;
				}
			}
		} else if (util < prevutil) {
			/* The previous frequency is high enough. */
			utilmax = prevutil;

			if (util <= utilmin) {
				/*
				 * Find the lowest frequency that is higher
				 * than freqmin.
				 */
				util = find_closest_util(sg_policy, utilmin + 1, CPUFREQ_RELATION_L);
				/*
				 * If freqmax is the first frequency above
				 * freqmin then we have already found that
				 * this speed is fast enough.
				 */
				if (util == utilmax)
					break;
			}
		}

		/* If same frequency chosen as previous then done. */
	} while (util != prevutil);

	return util;
}
#endif /* CONFIG_OPLUS_UAG_USE_TL */

/**
 * get_next_freq - Compute a new frequency for a given cpufreq policy.
 * @sg_policy: uag policy object to compute the new frequency for.
 * @util: Current CPU utilization.
 * @max: CPU capacity.
 *
 * If the utilization is frequency-invariant, choose the new frequency to be
 * proportional to it, that is
 *
 * next_freq = C * max_freq * util / max
 *
 * Otherwise, approximate the would-be frequency-invariant utilization by
 * util_raw * (curr_freq / max_freq) which leads to
 *
 * next_freq = C * curr_freq * util_raw / max
 *
 * Take C = 1.25 for the frequency tipping point at (util / max) = 0.8.
 *
 * The lowest driver-supported frequency which is equal or greater than the raw
 * next_freq (as calculated above) is returned, subject to policy min/max and
 * cpufreq driver limitations.
 */
static unsigned int get_next_freq(struct uag_gov_policy *sg_policy,
				  unsigned long util, unsigned long max)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned int freq = policy->cpuinfo.max_freq;
	unsigned long next_freq = 0;

#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
	nlopp_map_util_freq((void *)sg_policy, util, freq, policy->related_cpus, &next_freq);
#endif

	if (next_freq) {
		freq = next_freq;
	} else {
		freq = map_util_freq(util, freq, max);

		if (freq == sg_policy->cached_raw_freq && !sg_policy->need_freq_update)
			return sg_policy->next_freq;

		sg_policy->cached_raw_freq = freq;
		freq = cpufreq_driver_resolve_freq(policy, freq);
	}

	return freq;
}

/*
 * This function computes an effective utilization for the given CPU, to be
 * used for frequency selection given the linear relation: f = u * f_max.
 *
 * The scheduler tracks the following metrics:
 *
 *   cpu_util_{cfs,rt,dl,irq}()
 *   cpu_bw_dl()
 *
 * Where the cfs,rt and dl util numbers are tracked with the same metric and
 * synchronized windows and are thus directly comparable.
 *
 * The cfs,rt,dl utilization are the running times measured with rq->clock_task
 * which excludes things like IRQ and steal-time. These latter are then accrued
 * in the irq utilization.
 *
 * The DL bandwidth number otoh is not a measured metric but a value computed
 * based on the task model parameters and gives the minimal utilization
 * required to meet deadlines.
 */
unsigned long uag_cpu_util(int cpu, unsigned long util_cfs,
				 unsigned long max, enum schedutil_type type,
				 struct task_struct *p)
{
	unsigned long dl_util, util, irq;
	struct rq *rq = cpu_rq(cpu);

	if (!uclamp_is_used() &&
	    type == FREQUENCY_UTIL && rt_rq_is_runnable(&rq->rt)) {
		return max;
	}

	/*
	 * Early check to see if IRQ/steal time saturates the CPU, can be
	 * because of inaccuracies in how we track these -- see
	 * update_irq_load_avg().
	 */
	irq = cpu_util_irq(rq);
	if (unlikely(irq >= max))
		return max;

	/*
	 * Because the time spend on RT/DL tasks is visible as 'lost' time to
	 * CFS tasks and we use the same metric to track the effective
	 * utilization (PELT windows are synchronized) we can directly add them
	 * to obtain the CPU's actual utilization.
	 *
	 * CFS and RT utilization can be boosted or capped, depending on
	 * utilization clamp constraints requested by currently RUNNABLE
	 * tasks.
	 * When there are no CFS RUNNABLE tasks, clamps are released and
	 * frequency will be gracefully reduced with the utilization decay.
	 */
	util = util_cfs + cpu_util_rt(rq);
	if (type == FREQUENCY_UTIL)
		util = uclamp_rq_util_with(rq, util, p);

	dl_util = cpu_util_dl(rq);

	/*
	 * For frequency selection we do not make cpu_util_dl() a permanent part
	 * of this sum because we want to use cpu_bw_dl() later on, but we need
	 * to check if the CFS+RT+DL sum is saturated (ie. no idle time) such
	 * that we select f_max when there is no idle time.
	 *
	 * NOTE: numerical errors or stop class might cause us to not quite hit
	 * saturation when we should -- something for later.
	 */
	if (util + dl_util >= max)
		return max;

	/*
	 * OTOH, for energy computation we need the estimated running time, so
	 * include util_dl and ignore dl_bw.
	 */
	if (type == ENERGY_UTIL)
		util += dl_util;

	/*
	 * There is still idle time; further improve the number by using the
	 * irq metric. Because IRQ/steal time is hidden from the task clock we
	 * need to scale the task numbers:
	 *
	 *              max - irq
	 *   U' = irq + --------- * U
	 *                 max
	 */
	util = scale_irq_capacity(util, irq, max);
	util += irq;

	/*
	 * Bandwidth required by DEADLINE must always be granted while, for
	 * FAIR and RT, we use blocked utilization of IDLE CPUs as a mechanism
	 * to gracefully reduce the frequency when no tasks show up for longer
	 * periods of time.
	 *
	 * Ideally we would like to set bw_dl as min/guaranteed freq and util +
	 * bw_dl as requested freq. However, cpufreq is not yet ready for such
	 * an interface. So, we only do the latter for now.
	 */
	if (type == FREQUENCY_UTIL)
		util += cpu_bw_dl(rq);

	return min(max, util);
}

#ifdef CONFIG_ARCH_QCOM
static unsigned long uag_gov_get_util(struct uag_gov_cpu *sg_cpu)
{
	struct rq *rq = cpu_rq(sg_cpu->cpu);
	unsigned long max = arch_scale_cpu_capacity(sg_cpu->cpu);
	unsigned long util;

	sg_cpu->max = max;
	util = cpu_util_freq_walt(sg_cpu->cpu, &sg_cpu->walt_load);
	sg_cpu->bw_dl = cpu_bw_dl(rq);

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	uag_adjust_util(sg_cpu->cpu, &util, sg_cpu->sg_policy);
#endif

	return uclamp_rq_util_with(rq, util, NULL);
}
#else
static unsigned long uag_gov_get_util(struct uag_gov_cpu *sg_cpu)
{
	struct rq *rq = cpu_rq(sg_cpu->cpu);
	unsigned long util = cpu_util_cfs(rq);
	unsigned long max = arch_scale_cpu_capacity(sg_cpu->cpu);

	sg_cpu->max = max;
	sg_cpu->bw_dl = cpu_bw_dl(rq);

	spin_lock(&per_cpu(cpufreq_idle_cpu_lock, sg_cpu->cpu));
	if (per_cpu(cpufreq_idle_cpu, sg_cpu->cpu)) {
		spin_unlock(&per_cpu(cpufreq_idle_cpu_lock, sg_cpu->cpu));
		return 0;
	}
	spin_unlock(&per_cpu(cpufreq_idle_cpu_lock, sg_cpu->cpu));

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	uag_adjust_util(sg_cpu->cpu, &util, sg_cpu->sg_policy);
#endif
	return uag_cpu_util(sg_cpu->cpu, util, max, FREQUENCY_UTIL, NULL);
}
#endif /* CONFIG_ARCH_QCOM */

static unsigned long freq_to_util(struct uag_gov_policy *sg_policy,
				  unsigned int freq)
{
	int opp;

	if (freq == 0)
#ifdef CONFIG_ARCH_MEDIATEK
		opp = sg_policy->len - 1;
#else
		opp = 0;
#endif
	else {
		opp = cpufreq_frequency_table_get_index(sg_policy->policy, freq);
		if (opp < 0) {
			pr_err("get opp idx failed! cpu = %d freq = %d", sg_policy->policy->cpu, freq);
			return 0;
		}
	}
	trace_printk("opp = %lu\n", opp);

	return pd_get_opp_capacity(sg_policy->policy->cpu, opp);
}

static inline unsigned long target_util(struct uag_gov_policy *sg_policy,
				  unsigned int freq)
{
	unsigned long util;
	util = freq_to_util(sg_policy, freq);
	trace_printk("util = %lu\n", util);
	return util;
}

#ifdef CONFIG_ARCH_MEDIATEK
static unsigned int freq2util(struct uag_gov_policy *sg_policy, unsigned int freq)
{
	int idx;
	unsigned int capacity, opp_freq;

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int cpu = sg_policy->policy->cpu;
	int cid = arch_get_cluster_id(cpu);

	freq = mt_cpufreq_find_close_freq(cid, freq);
#endif

	for (idx = sg_policy->len-1; idx >= 0; idx--) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
		opp_freq = pd_get_cpu_freq(sg_policy->policy->cpu, idx);
#else
		opp_freq = sg_policy->policy->freq_table[idx].frequency;
#endif

		if (freq <= opp_freq) {
			return capacity;
		}
	}
	return get_opp_capacity(sg_policy->policy, 0);
}
#else
static unsigned int freq2util(struct uag_gov_policy *sg_policy, unsigned int freq)
{
	int idx;
	unsigned int capacity, opp_freq;

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int cpu = sg_policy->policy->cpu;
	int cid = arch_get_cluster_id(cpu);

	freq = mt_cpufreq_find_close_freq(cid, freq);
#endif

	for (idx = 0; idx < sg_policy->len; idx++) {
		capacity = get_opp_capacity(sg_policy->policy, idx);
#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
		opp_freq = pd_get_cpu_freq(sg_policy->policy->cpu, idx);
#else
		opp_freq = sg_policy->policy->freq_table[idx].frequency;
#endif

		if (freq <= opp_freq) {
			return capacity;
		}
	}
	return get_opp_capacity(sg_policy->policy, sg_policy->len-1);
}
#endif /* CONFIG_ARCH_MEDIATEK */

#ifdef CONFIG_ARCH_QCOM
static void uag_calc_avg_cap(struct uag_gov_policy *sg_policy, u64 curr_ws,
				unsigned int prev_freq)
{
	u64 last_ws = sg_policy->last_ws;
	unsigned int avg_freq;

	BUG_ON(curr_ws < last_ws);
	if (curr_ws <= last_ws)
		return;

	/* If we skipped some windows */
	if (curr_ws > (last_ws + sched_ravg_window)) {
		avg_freq = prev_freq;
		/* Reset tracking history */
		sg_policy->last_cyc_update_time = curr_ws;
	} else {
		uag_track_cycles(sg_policy, prev_freq, curr_ws);
		avg_freq = sg_policy->curr_cycles;
		avg_freq /= sched_ravg_window / (NSEC_PER_SEC / KHZ);
	}
	sg_policy->avg_cap = freq2util(sg_policy, avg_freq);
	sg_policy->curr_cycles = 0;
	sg_policy->last_ws = curr_ws;
}

#define NL_RATIO 75
static void uag_gov_util_adjust(struct uag_gov_cpu *sg_cpu, unsigned long cpu_util,
				unsigned long nl, unsigned long *util, unsigned long *max)
{
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
	bool is_migration = sg_policy->flags &  SCHED_CPUFREQ_INTERCLUSTER_MIG;
	bool is_hiload;

	is_hiload = (cpu_util >= mult_frac(sg_policy->avg_cap,
					   sg_policy->tunables->hispeed_load,
					   100));

	if (is_hiload && !is_migration)
		*util = max(*util, sg_policy->hispeed_util);

	if (is_hiload && nl >= mult_frac(cpu_util, NL_RATIO, 100))
		*util = *max;

	trace_printk("cpu_util = %lu avg_cap = %lu, hispeed_util = %lu, is_hiload = %d, is_migration = %d, nl = %lu, max = %lu, util = %lu\n",
		cpu_util, sg_policy->avg_cap, sg_policy->hispeed_util, is_hiload, is_migration, nl, *max, *util);
}
#else
static void uag_gov_util_adjust(struct uag_gov_cpu *sg_cpu, unsigned long cpu_util,
				unsigned long *util, unsigned long *max)
{
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
	bool is_hiload;

	/*add judge of hispeed*/
	is_hiload = (sg_cpu->prev_frame_loading >= sg_policy->tunables->hispeed_load);
	if (is_hiload)
		*util = max(*util, sg_policy->hispeed_util);
	trace_printk("hispeed util = %lu, prev_frame_loading = %u, util = %lu\n",
		sg_policy->hispeed_util, sg_cpu->prev_frame_loading, *util);
	return;
}
#endif /* CONFIG_ARCH_QCOM */

/**
 * uag_gov_iowait_reset() - Reset the IO boost status of a CPU.
 * @sg_cpu: the uag_gov data for the CPU to boost
 * @time: the update time from the caller
 * @set_iowait_boost: true if an IO boost has been requested
 *
 * The IO wait boost of a task is disabled after a tick since the last update
 * of a CPU. If a new IO wait boost is requested after more then a tick, then
 * we enable the boost starting from IOWAIT_BOOST_MIN, which improves energy
 * efficiency by ignoring sporadic wakeups from IO.
 */
static bool uag_gov_iowait_reset(struct uag_gov_cpu *sg_cpu, u64 time,
			       bool set_iowait_boost)
{
	s64 delta_ns = time - sg_cpu->last_update;
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
	unsigned int ticks = TICK_NSEC;

	if (sysctl_oplus_iowait_boost_enabled && sysctl_iowait_reset_ticks)
		ticks = sysctl_iowait_reset_ticks * TICK_NSEC;

	if (delta_ns <= ticks)
		return false;
#else
	/* Reset boost only if a tick has elapsed since last request */
	if (delta_ns <= TICK_NSEC)
		return false;
#endif

	sg_cpu->iowait_boost = set_iowait_boost ? IOWAIT_BOOST_MIN : 0;
	sg_cpu->iowait_boost_pending = set_iowait_boost;

	return true;
}

/**
 * uag_gov_iowait_boost() - Updates the IO boost status of a CPU.
 * @sg_cpu: the uag_gov data for the CPU to boost
 * @time: the update time from the caller
 * @flags: SCHED_CPUFREQ_IOWAIT if the task is waking up after an IO wait
 *
 * Each time a task wakes up after an IO operation, the CPU utilization can be
 * boosted to a certain utilization which doubles at each "frequent and
 * successive" wakeup from IO, ranging from IOWAIT_BOOST_MIN to the utilization
 * of the maximum OPP.
 *
 * To keep doubling, an IO boost has to be requested at least once per tick,
 * otherwise we restart from the utilization of the minimum OPP.
 */
static void uag_gov_iowait_boost(struct uag_gov_cpu *sg_cpu, u64 time,
			       unsigned int flags)
{
	bool set_iowait_boost = flags & SCHED_CPUFREQ_IOWAIT;

	/* Reset boost if the CPU appears to have been idle enough */
	if (sg_cpu->iowait_boost &&
	    uag_gov_iowait_reset(sg_cpu, time, set_iowait_boost))
		return;

	/* Boost only tasks waking up after IO */
	if (!set_iowait_boost)
		return;

	/* Ensure boost doubles only one time at each request */
	if (sg_cpu->iowait_boost_pending)
		return;
	sg_cpu->iowait_boost_pending = true;

	/* Double the boost at each request */
	if (sg_cpu->iowait_boost) {
		sg_cpu->iowait_boost =
			min_t(unsigned int, sg_cpu->iowait_boost << 1, SCHED_CAPACITY_SCALE);
		return;
	}

	/* First wakeup after IO: start with minimum boost */
	sg_cpu->iowait_boost = IOWAIT_BOOST_MIN;
}

/**
 * uag_gov_iowait_apply() - Apply the IO boost to a CPU.
 * @sg_cpu: the uag_gov data for the cpu to boost
 * @time: the update time from the caller
 * @util: the utilization to (eventually) boost
 * @max: the maximum value the utilization can be boosted to
 *
 * A CPU running a task which woken up after an IO operation can have its
 * utilization boosted to speed up the completion of those IO operations.
 * The IO boost value is increased each time a task wakes up from IO, in
 * uag_gov_iowait_apply(), and it's instead decreased by this function,
 * each time an increase has not been requested (!iowait_boost_pending).
 *
 * A CPU which also appears to have been idle for at least one tick has also
 * its IO boost utilization reset.
 *
 * This mechanism is designed to boost high frequently IO waiting tasks, while
 * being more conservative on tasks which does sporadic IO operations.
 */
static unsigned long uag_gov_iowait_apply(struct uag_gov_cpu *sg_cpu, u64 time,
					unsigned long util, unsigned long max)
{
	unsigned long boost;
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
#endif

	/* No boost currently required */
	if (!sg_cpu->iowait_boost)
		return util;

	/* Reset boost if the CPU appears to have been idle enough */
	if (uag_gov_iowait_reset(sg_cpu, time, false))
		return util;

#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
	if (!sg_cpu->iowait_boost_pending &&
			(!sysctl_oplus_iowait_boost_enabled || !sysctl_iowait_apply_ticks ||
			 (time - sg_policy->last_update > (sysctl_iowait_apply_ticks * TICK_NSEC)))) {
#else
	if (!sg_cpu->iowait_boost_pending) {
#endif
		/*
		 * No boost pending; reduce the boost value.
		 */
		sg_cpu->iowait_boost >>= 1;
		if (sg_cpu->iowait_boost < IOWAIT_BOOST_MIN) {
			sg_cpu->iowait_boost = 0;
			return util;
		}
	}

	sg_cpu->iowait_boost_pending = false;

	/*
	 * @util is already in capacity scale; convert iowait_boost
	 * into the same scale so we can compare.
	 */
	boost = (sg_cpu->iowait_boost * max) >> SCHED_CAPACITY_SHIFT;
	return max(boost, util);
}

/*
 * Make uag_gov_should_update_freq() ignore the rate limit when DL
 * has increased the utilization.
 */
static inline void ignore_dl_rate_limit(struct uag_gov_cpu *sg_cpu, struct uag_gov_policy *sg_policy)
{
	if (cpu_bw_dl(cpu_rq(sg_cpu->cpu)) > sg_cpu->bw_dl)
		sg_policy->limits_changed = true;
}

/*static bool is_cobuck_platform()
{
	return true;
}*/

static unsigned int uag_gov_get_final_freq(struct uag_gov_cpu *sg_cpu, struct uag_gov_policy *sg_policy,
					unsigned int next_freq)
{
	struct cpufreq_policy cobuck_policy;
	struct uag_gov_policy *cobuck_sg_policy;
	unsigned int cobuck_volt, cobuck_cid, cobuck_first_cpu;
	unsigned int curr_first_cpu, curr_cid, curr_volt, opp_freq, opp_volt;
	unsigned int lowest_volt_diff = 0;
	unsigned int final_freq = next_freq;
	int cobuck_cpu, opp;

	if (!sg_policy->tunables->cobuck_enable)
		return next_freq;

	if (sg_policy->policy->cpu == 7)
		cobuck_cpu = 4;
	else if (sg_policy->policy->cpu == 4)
		cobuck_cpu = 7;
	else
		cobuck_cpu = -1;
	trace_printk("cobuck cpu = %d cpu = %d\n", cobuck_cpu, sg_policy->policy->cpu);

	if (cobuck_cpu == -1)
		return next_freq;

	/* get cobuck cpu current freq and volt*/
	if (cpufreq_get_policy(&cobuck_policy, cobuck_cpu))
		return next_freq;

	if (strcmp(cobuck_policy.governor->name, "uag"))
		return next_freq;

	cobuck_sg_policy = cobuck_policy.governor_data;

	if (!cobuck_sg_policy)
		return next_freq;

	cobuck_first_cpu = cpumask_first(cobuck_policy.related_cpus);
	cobuck_cid = topology_physical_package_id(cobuck_first_cpu);
	cobuck_volt = freq_to_voltage(cobuck_cid, cobuck_policy.cur);
	trace_printk("cobuck cluster curr freq = %ld\n", cobuck_policy.cur);

	curr_first_cpu = cpumask_first(sg_policy->policy->related_cpus);
	curr_cid = topology_physical_package_id(curr_first_cpu);
	curr_volt = freq_to_voltage(curr_cid, next_freq);

	lowest_volt_diff = abs(curr_volt - cobuck_volt);
	opp = cpufreq_frequency_table_get_index(sg_policy->policy, next_freq);
	trace_printk("curr cluster opp = %d\n", opp);
	if (opp < 0)
		return next_freq;
#ifdef CONFIG_ARCH_MEDIATEK
	for (; opp >= 0; opp--) {
#else
	for (; opp <= sg_policy->len - 1; opp++) {
#endif
#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
		opp_freq = pd_get_cpu_freq(sg_policy->policy->cpu, opp);
#else
		opp_freq = sg_policy->policy->freq_table[opp].frequency;
#endif
		opp_volt = freq_to_voltage(curr_cid, opp_freq);
		if (lowest_volt_diff >= abs(opp_volt - cobuck_volt)) {
			lowest_volt_diff = abs(opp_volt - cobuck_volt);
			final_freq = opp_freq;
		}
	}
	if (final_freq <= next_freq) {
		sg_policy->cobuck_boosted = 0;
		return next_freq;
	} else if (cobuck_sg_policy->cobuck_boosted) {
		sg_policy->cobuck_boosted = 0;
		return next_freq;
	} else
		sg_policy->cobuck_boosted = 1;

	trace_printk("final freq = %u\n", final_freq);
	return final_freq;
}

#ifdef CONFIG_OPLUS_MULTI_LV_TL
static inline unsigned long uag_multi_util_max(struct uag_gov_policy *sg_policy)
{
	unsigned long max = 0;
	int i;

	for (i = 0; i < UA_UTIL_SIZE; i++) {
		if (max < sg_policy->multi_util[i])
			max = sg_policy->multi_util[i];
	}

	return max;
}
#endif

static void uag_gov_update_single(struct update_util_data *cb, u64 time,
				unsigned int flags)
{
	struct uag_gov_cpu *sg_cpu = container_of(cb, struct uag_gov_cpu, update_util);
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
	unsigned long util, max;
	unsigned int next_f;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
	struct cpufreq_policy *policy = sg_policy->policy;
	int util_thresh = 0;
	unsigned int avg_nr_running = 1;
	int cluster_id = topology_physical_package_id(cpumask_first(policy->cpus));
	unsigned long util_orig;
#endif
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
	unsigned long util_bak;
#endif
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
	struct rq *rq = cpu_rq(sg_cpu->cpu);
#endif

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	unsigned long irq_flags = 0;
#endif
#ifdef CONFIG_ARCH_QCOM
	unsigned long nl;
#endif /* CONFIG_ARCH_QCOM */

	raw_spin_lock_irqsave(&sg_policy->update_lock, irq_flags);
	uag_gov_iowait_boost(sg_cpu, time, flags);
	sg_cpu->last_update = time;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	sg_policy->flags = flags;
#endif

	ignore_dl_rate_limit(sg_cpu, sg_policy);

	if (!uag_gov_should_update_freq(sg_policy, time)) {
		raw_spin_unlock_irqrestore(&sg_policy->update_lock, irq_flags);
		return;
	}

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	uag_update_counter(sg_policy);
#endif
	util = uag_gov_get_util(sg_cpu);
	max = sg_cpu->max;

#ifdef CONFIG_ARCH_QCOM
	uag_calc_avg_cap(sg_policy, sg_cpu->walt_load.ws,
				   sg_policy->policy->cur); // 通过cpu的cpu cycles来更新当前的平均频率以及平均cap，

	nl = sg_cpu->walt_load.nl;
	uag_gov_util_adjust(sg_cpu, util, nl, &util, &max);
#else
	uag_gov_util_adjust(sg_cpu, util, &util, &max);
#endif /* CONFIG_ARCH_QCOM */

#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
	util_bak = util;
	util = uag_gov_iowait_apply(sg_cpu, time, util, max);
	if (unlikely(eas_opt_debug_enable))
		trace_printk("[eas_opt]: enable_iowait_boost=%d, cpu:%d, max:%d, cpu->util:%d,iowait_util:%d\n",
				sysctl_oplus_iowait_boost_enabled, sg_cpu->cpu, max, util_bak, util);
#else
	util = uag_gov_iowait_apply(sg_cpu, time, util, max);
#endif
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
#ifdef CONFIG_OPLUS_MULTI_LV_TL
	sg_policy->multi_util[UA_UTIL_SYS] = util;
	if (sg_policy->tunables->multi_tl_enable) {
		fbg_freq_policy_util(sg_cpu->sg_policy->flags, sg_policy->policy->cpus, &sg_policy->multi_util[UA_UTIL_FBG]);
		util = uag_multi_util_max(sg_policy);
	} else
		fbg_freq_policy_util(sg_cpu->sg_policy->flags, sg_policy->policy->cpus, &util);
	trace_uag_update_multi_util(sg_policy);
#else
	/* Adjust &util with frame boost policy, avoiding frame misssing */
	fbg_freq_policy_util(sg_cpu->sg_policy->flags, sg_policy->policy->cpus, &util);
#endif
#endif
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
	if (eas_opt_enable && (util_thresh_percent[cluster_id] != 100)) {
		util_thresh = max * util_thresh_cvt[cluster_id] >> SCHED_CAPACITY_SHIFT;
		avg_nr_running = rq->nr_running;
		util_orig = util;
		util = (util_thresh < util) ?
			(util_thresh + ((avg_nr_running * (util-util_thresh) * nr_oplus_cap_multiple[cluster_id]) >> SCHED_CAPACITY_SHIFT)) : util;
#ifdef CONFIG_OPLUS_MULTI_LV_TL
		sg_policy->multi_util[UA_UTIL_FBG] = min_t(int, sg_policy->multi_util[UA_UTIL_FBG], util);
		sg_policy->multi_util[UA_UTIL_SYS] = min_t(int, sg_policy->multi_util[UA_UTIL_SYS], util);
#endif
		if (unlikely(eas_opt_debug_enable))
			trace_printk("[eas_opt]: cluster_id: %d, capacity: %d, util_thresh: %d, util_orig: %d, "
					"util: %d, avg_nr_running: %d, oplus_cap_multiple: %d,nr_oplus_cap_multiple: %d, util_thresh: %d\n",
					cluster_id, max, util_thresh, util_orig, util, avg_nr_running,
					oplus_cap_multiple[cluster_id], nr_oplus_cap_multiple[cluster_id], util_thresh_percent[cluster_id]);
	}
#endif
	next_f = get_next_freq(sg_policy, util, max);

	/*cobuck check*/
	next_f = uag_gov_get_final_freq(sg_cpu, sg_policy, next_f); 

#ifdef CONFIG_ARCH_QCOM
	next_f = cpufreq_driver_resolve_freq(sg_policy->policy, next_f);
#endif
	/*
	 * This code runs under rq->lock for the target CPU, so it won't run
	 * concurrently on two different CPUs for the same target and it is not
	 * necessary to acquire the lock in the fast switch case.
	 */
	if (sg_policy->policy->fast_switch_enabled) {
		uag_gov_fast_switch(sg_policy, time, next_f);
	} else
		uag_gov_deferred_update(sg_policy, time, next_f);

	raw_spin_unlock_irqrestore(&sg_policy->update_lock, irq_flags);
}

static unsigned int uag_gov_next_freq_shared(struct uag_gov_cpu *sg_cpu, u64 time)
{
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned long util = 0, max = 1;
	unsigned int j;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
	int util_thresh = 0;
	unsigned int avg_nr_running = 1;
	unsigned int count_cpu = 0;
	int cluster_id = topology_physical_package_id(cpumask_first(policy->cpus));
	unsigned long util_orig;
#endif

	for_each_cpu(j, policy->cpus) {
		struct uag_gov_cpu *j_sg_cpu = &per_cpu(uag_gov_cpu, j);
		unsigned long j_util, j_max;
#ifdef CONFIG_ARCH_QCOM
		unsigned long j_nl;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
		struct rq *rq = cpu_rq(j);
#endif
#endif /* CONFIG_ARCH_QCOM */
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
		avg_nr_running += rq->nr_running;
		count_cpu++;
#endif

		j_util = j_sg_cpu->util;
		j_max = j_sg_cpu->max;
		j_util = uag_gov_iowait_apply(j_sg_cpu, time, j_util, j_max);
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
		if (unlikely(eas_opt_debug_enable))
			trace_printk("[eas_opt]: enable_iowait_boost=%d, cpu:%d, max:%d, cpu->util:%d,iowait_util:%d\n",
					sysctl_oplus_iowait_boost_enabled, j_sg_cpu->cpu, j_sg_cpu->max, j_sg_cpu->util, j_util);
#endif
		if (j_util * max > j_max * util) {
			util = j_util;
			max = j_max;
		}

#ifdef CONFIG_ARCH_QCOM
		j_nl = j_sg_cpu->walt_load.nl;
		uag_gov_util_adjust(j_sg_cpu, j_util, j_nl, &util, &max);
#else
		uag_gov_util_adjust(j_sg_cpu, j_util, &util, &max); // 是否已经达到了门槛值
#endif /* CONFIG_ARCH_QCOM */
	}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
#ifdef CONFIG_OPLUS_MULTI_LV_TL
	sg_policy->multi_util[UA_UTIL_SYS] = util;
	if (sg_policy->tunables->multi_tl_enable) {
		fbg_freq_policy_util(sg_policy->flags, policy->cpus, &sg_policy->multi_util[UA_UTIL_FBG]);
		util = uag_multi_util_max(sg_policy);
	} else
		fbg_freq_policy_util(sg_policy->flags, policy->cpus, &util);
	trace_uag_update_multi_util(sg_policy);
#else
	/* Adjust &util with frame boost policy, avoiding frame misssing */
	fbg_freq_policy_util(sg_policy->flags, policy->cpus, &util);
#endif
#endif
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
	if (eas_opt_enable && (util_thresh_percent[cluster_id] != 100) && count_cpu) {
		util_thresh = max * util_thresh_cvt[cluster_id] >> SCHED_CAPACITY_SHIFT;
		avg_nr_running = mult_frac(avg_nr_running, 1, count_cpu);
		util_orig = util;
		util = (util_thresh < util) ?
			(util_thresh + ((avg_nr_running * (util-util_thresh) * nr_oplus_cap_multiple[cluster_id]) >> SCHED_CAPACITY_SHIFT)) : util;
#ifdef CONFIG_OPLUS_MULTI_LV_TL
		sg_policy->multi_util[UA_UTIL_FBG] = min_t(int, sg_policy->multi_util[UA_UTIL_FBG], util);
		sg_policy->multi_util[UA_UTIL_SYS] = min_t(int, sg_policy->multi_util[UA_UTIL_SYS], util);
#endif
		if (unlikely(eas_opt_debug_enable))
			trace_printk("[eas_opt]: cluster_id: %d, util_orig: %d, util: %d, avg_nr_running: %d, oplus_cap_multiple: %d,nr_oplus_cap_multiple: %d, util_thresh: %d\n",
					cluster_id, util_orig, util, avg_nr_running, oplus_cap_multiple[cluster_id], nr_oplus_cap_multiple[cluster_id], util_thresh_percent[cluster_id]);
	}
#endif

	return get_next_freq(sg_policy, util, max);
}

static void
uag_gov_update_shared(struct update_util_data *cb, u64 time, unsigned int flags)
{
	struct uag_gov_cpu *sg_cpu = container_of(cb, struct uag_gov_cpu, update_util);
	struct uag_gov_policy *sg_policy = sg_cpu->sg_policy;
	unsigned int next_f;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	unsigned long irq_flags;
#endif
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	raw_spin_lock_irqsave(&sg_policy->update_lock, irq_flags);
#else
	raw_spin_lock(&sg_policy->update_lock);
#endif

	uag_gov_iowait_boost(sg_cpu, time, flags);
	sg_cpu->last_update = time;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	sg_policy->flags = flags;
#endif
	sg_cpu->util = uag_gov_get_util(sg_cpu);
	sg_cpu->flags = flags;

	ignore_dl_rate_limit(sg_cpu, sg_policy);

#ifdef CONFIG_ARCH_QCOM
	uag_calc_avg_cap(sg_policy, sg_cpu->walt_load.ws,
					   sg_policy->policy->cur);
#endif /* CONFIG_ARCH_QCOM */

	if (uag_gov_should_update_freq(sg_policy, time)) {
#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
		uag_update_counter(sg_policy);
#endif
		next_f = uag_gov_next_freq_shared(sg_cpu, time);

		/*cobuck check*/
		next_f = uag_gov_get_final_freq(sg_cpu, sg_policy, next_f);

#ifdef CONFIG_ARCH_QCOM
		next_f = cpufreq_driver_resolve_freq(sg_policy->policy, next_f);
#endif
#if IS_ENABLED(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT)
		sg_policy->last_update = time;
#endif

		if (sg_policy->policy->fast_switch_enabled)
			uag_gov_fast_switch(sg_policy, time, next_f);
		else
			uag_gov_deferred_update(sg_policy, time, next_f);
	}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	raw_spin_unlock_irqrestore(&sg_policy->update_lock, irq_flags);
#else
	raw_spin_unlock(&sg_policy->update_lock);
#endif
}

static void uag_gov_work(struct kthread_work *work)
{
	struct uag_gov_policy *sg_policy = container_of(work, struct uag_gov_policy, work);
	unsigned int freq;
	unsigned long flags;

	/*
	 * Hold sg_policy->update_lock shortly to handle the case where:
	 * incase sg_policy->next_freq is read here, and then updated by
	 * uag_gov_deferred_update() just before work_in_progress is set to false
	 * here, we may miss queueing the new update.
	 *
	 * Note: If a work was queued after the update_lock is released,
	 * uag_gov_work() will just be called again by kthread_work code; and the
	 * request will be proceed before the uag_gov thread sleeps.
	 */
	raw_spin_lock_irqsave(&sg_policy->update_lock, flags);
	freq = sg_policy->next_freq;
	uag_track_cycles(sg_policy, sg_policy->policy->cur,
			   ktime_get_ns());
	sg_policy->work_in_progress = false;
	raw_spin_unlock_irqrestore(&sg_policy->update_lock, flags);

	mutex_lock(&sg_policy->work_lock);
	__cpufreq_driver_target(sg_policy->policy, freq, CPUFREQ_RELATION_L);
	mutex_unlock(&sg_policy->work_lock);
}

static void uag_gov_irq_work(struct irq_work *irq_work)
{
	struct uag_gov_policy *sg_policy;

	sg_policy = container_of(irq_work, struct uag_gov_policy, irq_work);

	kthread_queue_work(&sg_policy->worker, &sg_policy->work);
}

/************************** sysfs interface ************************/

static struct uag_gov_tunables *global_tunables;
static DEFINE_MUTEX(global_tunables_lock);

static inline struct uag_gov_tunables *to_uag_gov_tunables(struct gov_attr_set *attr_set)
{
	return container_of(attr_set, struct uag_gov_tunables, attr_set);
}

static ssize_t rate_limit_us_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return sprintf(buf, "%u\n", tunables->rate_limit_us);
}

static ssize_t
rate_limit_us_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct uag_gov_policy *sg_policy;
	unsigned int rate_limit_us;

	if (kstrtouint(buf, 10, &rate_limit_us))
		return -EINVAL;

	tunables->rate_limit_us = rate_limit_us;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook)
		sg_policy->freq_update_delay_ns = rate_limit_us * NSEC_PER_USEC;

	return count;
}

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
static ssize_t stall_aware_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return sprintf(buf, "%d\n", tunables->stall_aware);
}

static ssize_t
stall_aware_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	unsigned int stall_aware;

	if (kstrtouint(buf, 10, &stall_aware))
		return -EINVAL;

	tunables->stall_aware = stall_aware;
	return count;
}

static ssize_t stall_reduce_pct_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return sprintf(buf, "%llu\n", tunables->stall_reduce_pct);
}

static ssize_t
stall_reduce_pct_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	int stall_reduce_pct;

	if (kstrtouint(buf, 10, &stall_reduce_pct))
		return -EINVAL;

	tunables->stall_reduce_pct = stall_reduce_pct;
	return count;
}

static ssize_t report_policy_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return sprintf(buf, "%d\n", tunables->report_policy);
}

static ssize_t
report_policy_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	int report_policy;

	if (kstrtoint(buf, 10, &report_policy))
		return -EINVAL;

	tunables->report_policy = report_policy;
	return count;
}
#endif /* CONFIG_OPLUS_UAG_AMU_AWARE */

#ifdef CONFIG_OPLUS_UAG_USE_TL
static ssize_t target_loads_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	for (i = 0; i < tunables->ntarget_loads; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret - 1, "%u%s", tunables->target_loads[i],
			i & 0x1 ? ":" : " ");

	snprintf(buf + ret - 1, PAGE_SIZE - ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;

	for (i = 0; i < ntokens; i += 2) {
		if(tokenized_data[i] < 80 || tokenized_data[i] > 100)
			goto err_kfree;
	}

	return tokenized_data;
err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

#ifdef CONFIG_OPLUS_MULTI_LV_TL
static inline void uag_reg_multi_tl_fbg(struct uag_gov_tunables *tunables)
{
	struct multi_target_loads *multi_tl = &tunables->multi_tl[UA_UTIL_FBG];

	multi_tl->target_loads = tunables->target_loads;
	multi_tl->util_loads = tunables->util_loads;
	multi_tl->ntarget_loads = tunables->ntarget_loads;
}
#endif

static ssize_t target_loads_store(struct gov_attr_set *attr_set, const char *buf,
					size_t count)
{
	int ntokens, i;
	unsigned int *new_target_loads = NULL;
	unsigned long flags;
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct uag_gov_policy *sg_policy;
	unsigned int *new_util_loads = NULL;


	//get the first policy if this tunnables have mutil policies
	sg_policy = list_first_entry(&attr_set->policy_list, struct uag_gov_policy, tunables_hook);
	if (!sg_policy) {
		pr_err("sg_policy is null\n");
		return count;
	}

	new_target_loads = get_tokenized_data(buf, &ntokens);

	if (IS_ERR(new_target_loads))
		return PTR_ERR(new_target_loads);

	new_util_loads = kzalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!new_util_loads)
		return -ENOMEM;

	memcpy(new_util_loads, new_target_loads, sizeof(unsigned int) * ntokens);

	for (i = 0; i < ntokens - 1; i += 2) {
		new_util_loads[i+1] = freq2util(sg_policy, new_target_loads[i+1]);
	}

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	if (tunables->target_loads != default_target_loads)
		kfree(tunables->target_loads);
	if (tunables->util_loads != default_target_loads)
		kfree(tunables->util_loads);

	tunables->target_loads = new_target_loads;
	tunables->ntarget_loads = ntokens;
	tunables->util_loads = new_util_loads;

#ifdef CONFIG_OPLUS_MULTI_LV_TL
	uag_reg_multi_tl_fbg(tunables);
#endif
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return count;
}

ssize_t set_sugov_tl_uag(unsigned int cpu, char *buf)
{
	struct cpufreq_policy *policy;
	struct uag_gov_policy *sg_policy;
	struct uag_gov_tunables *tunables;
	struct gov_attr_set *attr_set;
	size_t count;

	if (!buf)
		return -EFAULT;

	policy = cpufreq_cpu_get(cpu);
	if (!policy)
		return -ENODEV;

	sg_policy = policy->governor_data;
	if (!sg_policy)
		return -EINVAL;

	tunables = sg_policy->tunables;
	if (!tunables)
		return -ENOMEM;

	attr_set = &tunables->attr_set;
	count = strlen(buf);

	return target_loads_store(attr_set, buf, count);
}
EXPORT_SYMBOL_GPL(set_sugov_tl_uag);
#endif

static ssize_t hispeed_load_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->hispeed_load);
}

static ssize_t hispeed_load_store(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	if (kstrtouint(buf, 10, &tunables->hispeed_load))
		return -EINVAL;

	tunables->hispeed_load = min(100U, tunables->hispeed_load);

	return count;
}

static ssize_t hispeed_freq_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->hispeed_freq);
}

static ssize_t hispeed_freq_store(struct gov_attr_set *attr_set,
					const char *buf, size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	unsigned int val;
	struct uag_gov_policy *sg_policy;
	unsigned long hs_util;
	unsigned long flags;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->hispeed_freq = val;
	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		raw_spin_lock_irqsave(&sg_policy->update_lock, flags);
		hs_util = target_util(sg_policy,
					sg_policy->tunables->hispeed_freq);
		sg_policy->hispeed_util = hs_util;
		raw_spin_unlock_irqrestore(&sg_policy->update_lock, flags);
	}

	return count;
}

#ifdef CONFIG_OPLUS_UAG_SOFT_LIMIT
static ssize_t soft_limit_freq_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(&tunables->soft_limit_freq_lock, flags);
	ret = sprintf(buf, "%lu\n", tunables->soft_limit_freq);
	spin_unlock_irqrestore(&tunables->soft_limit_freq_lock, flags);

	return ret;
}

static ssize_t soft_limit_freq_store(struct gov_attr_set *attr_set, const char *buf,
					size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct uag_gov_policy *sg_policy;
	struct cpufreq_policy *policy;
	unsigned int soft_freq;
	unsigned long flags;
	struct cpumask *cpus;
	unsigned long max_capacity, capacity;
	int cpu;
	struct rq *rq;

	sg_policy = list_first_entry(&attr_set->policy_list, struct uag_gov_policy, tunables_hook);
	if (!sg_policy) {
		pr_err("sg_policy is null\n");
		return count;
	}

	if (kstrtouint(buf, 10, &soft_freq))
		return -EINVAL;

	spin_lock_irqsave(&tunables->soft_limit_freq_lock, flags);
	tunables->soft_limit_freq = soft_freq;
	tunables->soft_limit_util = freq2util(sg_policy, soft_freq);
	spin_unlock_irqrestore(&tunables->soft_limit_freq_lock, flags);

	policy = sg_policy->policy;
	cpus = policy->related_cpus;
	max_capacity = arch_scale_cpu_capacity(cpumask_first(cpus));
	capacity = soft_freq * max_capacity;
	capacity /= policy->cpuinfo.max_freq;

	if (policy->max != policy->cpuinfo.max_freq)
		max_capacity = mult_frac(max_capacity, policy->max,
				policy->cpuinfo.max_freq);

	for_each_cpu(cpu, cpus) {
		rq = cpu_rq(cpu);
		rq->cpu_capacity_orig = min(max_capacity, capacity);
	}

	return count;
}

static ssize_t break_freq_margin_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->break_freq_margin);
}

static ssize_t break_freq_margin_store(struct gov_attr_set *attr_set, const char *buf,
					size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	if (kstrtouint(buf, 10, &tunables->break_freq_margin))
		return -EINVAL;

	return count;
}

static ssize_t soft_limit_enable_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->soft_limit_enable);
}

static ssize_t soft_limit_enable_store(struct gov_attr_set *attr_set, const char *buf,
				   size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	if (kstrtobool(buf, &tunables->soft_limit_enable))
		return -EINVAL;

	return count;
}
#endif

static ssize_t cobuck_enable_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->cobuck_enable);
}

static ssize_t cobuck_enable_store(struct gov_attr_set *attr_set, const char *buf,
				   size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	if (kstrtobool(buf, &tunables->cobuck_enable))
		return -EINVAL;

	return count;
}

#ifdef CONFIG_OPLUS_MULTI_LV_TL
static ssize_t multi_tl_enable_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->multi_tl_enable);
}

static ssize_t multi_tl_enable_store(struct gov_attr_set *attr_set, const char *buf,
				   size_t count)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct uag_gov_policy *sg_policy;

	if (kstrtobool(buf, &tunables->multi_tl_enable))
		return -EINVAL;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		if (!tunables->multi_tl_enable) {
			sg_policy->fbg_gt_sys_cnt = 0;
			sg_policy->total_cnt = 0;
		}
	}

	return count;
}

static ssize_t multi_tl_sys_show(struct gov_attr_set *attr_set, char *buf)
{
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct multi_target_loads *multi_tl = &tunables->multi_tl[UA_UTIL_SYS];
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	for (i = 0; i < multi_tl->ntarget_loads; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret - 1, "%u%s", multi_tl->target_loads[i],
			i & 0x1 ? ":" : " ");

	snprintf(buf + ret - 1, PAGE_SIZE - ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

static ssize_t multi_tl_sys_store(struct gov_attr_set *attr_set,
				const char *buf, size_t count)
{
	int ntokens, i;
	unsigned int *new_target_loads = NULL;
	unsigned long flags;
	struct uag_gov_tunables *tunables = to_uag_gov_tunables(attr_set);
	struct multi_target_loads *multi_tl = &tunables->multi_tl[UA_UTIL_SYS];
	struct uag_gov_policy *sg_policy;
	unsigned int *new_util_loads = NULL;


	//get the first policy if this tunnables have mutil policies
	sg_policy = list_first_entry(&attr_set->policy_list, struct uag_gov_policy, tunables_hook);
	if (!sg_policy) {
		pr_err("sg_policy is null\n");
		return count;
	}

	new_target_loads = get_tokenized_data(buf, &ntokens);

	if (IS_ERR(new_target_loads))
		return PTR_ERR(new_target_loads);

	new_util_loads = kzalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!new_util_loads)
		return -ENOMEM;

	memcpy(new_util_loads, new_target_loads, sizeof(unsigned int) * ntokens);
	for (i = 0; i < ntokens - 1; i += 2) {
		new_util_loads[i+1] = freq2util(sg_policy, new_target_loads[i+1]);
	}

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	if (multi_tl->target_loads != default_target_loads_sys)
		kfree(multi_tl->target_loads);
	if (multi_tl->util_loads != default_target_loads_sys)
		kfree(multi_tl->util_loads);

	multi_tl->target_loads = new_target_loads;
	multi_tl->ntarget_loads = ntokens;
	multi_tl->util_loads = new_util_loads;
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return count;
}

static struct governor_attr multi_tl_enable = __ATTR_RW(multi_tl_enable);
static struct governor_attr multi_tl_sys =
    __ATTR(multi_tl_sys, 0664, multi_tl_sys_show, multi_tl_sys_store);
#endif

static struct governor_attr rate_limit_us = __ATTR_RW(rate_limit_us);
static struct governor_attr hispeed_load = __ATTR_RW(hispeed_load);
static struct governor_attr hispeed_freq = __ATTR_RW(hispeed_freq);
static struct governor_attr cobuck_enable = __ATTR_RW(cobuck_enable);


#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
static struct governor_attr stall_aware = __ATTR_RW(stall_aware);
static struct governor_attr stall_reduce_pct = __ATTR_RW(stall_reduce_pct);
static struct governor_attr report_policy = __ATTR_RW(report_policy);
#endif

#ifdef CONFIG_OPLUS_UAG_USE_TL
static struct governor_attr target_loads =
	__ATTR(target_loads, 0664, target_loads_show, target_loads_store);
#endif

#ifdef CONFIG_OPLUS_UAG_SOFT_LIMIT
static struct governor_attr soft_limit_freq =
	__ATTR(soft_limit_freq, 0664, soft_limit_freq_show, soft_limit_freq_store);
static struct governor_attr break_freq_margin =
	__ATTR(break_freq_margin, 0664, break_freq_margin_show, break_freq_margin_store);
static struct governor_attr soft_limit_enable =
	__ATTR(soft_limit_enable, 0664, soft_limit_enable_show, soft_limit_enable_store);
#endif

static struct attribute *uag_gov_attrs[] = {
	&rate_limit_us.attr,
	&hispeed_load.attr,
	&hispeed_freq.attr,
	&cobuck_enable.attr,
#ifdef CONFIG_OPLUS_UAG_USE_TL
	&target_loads.attr,
#endif

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	&stall_aware.attr,
	&stall_reduce_pct.attr,
	&report_policy.attr,
#endif

#ifdef CONFIG_OPLUS_MULTI_LV_TL
	&multi_tl_sys.attr,
	&multi_tl_enable.attr,
#endif

#ifdef CONFIG_OPLUS_UAG_SOFT_LIMIT
	&soft_limit_freq.attr,
	&break_freq_margin.attr,
	&soft_limit_enable.attr,
#endif
	NULL
};
ATTRIBUTE_GROUPS(uag_gov);

static struct kobj_type uag_gov_tunables_ktype = {
	.default_groups = uag_gov_groups,
	.sysfs_ops = &governor_sysfs_ops,
};

/********************** cpufreq governor interface *********************/

struct cpufreq_governor cpufreq_uag_gov;

static struct uag_gov_policy *uag_gov_policy_alloc(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy;

	sg_policy = kzalloc(sizeof(*sg_policy), GFP_KERNEL);
	if (!sg_policy)
		return NULL;

	sg_policy->policy = policy;
	raw_spin_lock_init(&sg_policy->update_lock);
	return sg_policy;
}

static inline void uag_gov_cpu_reset(struct uag_gov_policy *sg_policy)
{
	unsigned int cpu;

	for_each_cpu(cpu, sg_policy->policy->cpus) {
		struct uag_gov_cpu *sg_cpu = &per_cpu(uag_gov_cpu, cpu);
		sg_cpu->sg_policy = NULL;
	}
}

static void uag_gov_policy_free(struct uag_gov_policy *sg_policy)
{
	kfree(sg_policy);
}

static int uag_gov_kthread_create(struct uag_gov_policy *sg_policy)
{
	struct task_struct *thread;
	struct sched_attr attr = {
		.size		= sizeof(struct sched_attr),
		.sched_policy	= SCHED_DEADLINE,
		.sched_flags	= SCHED_FLAG_SUGOV,
		.sched_nice	= 0,
		.sched_priority	= 0,
		/*
		 * Fake (unused) bandwidth; workaround to "fix"
		 * priority inheritance.
		 */
		.sched_runtime	=  1000000,
		.sched_deadline = 10000000,
		.sched_period	= 10000000,
	};
	struct cpufreq_policy *policy = sg_policy->policy;
	int ret;

	/* kthread only required for slow path */
	if (policy->fast_switch_enabled)
		return 0;

	kthread_init_work(&sg_policy->work, uag_gov_work);
	kthread_init_worker(&sg_policy->worker);
	thread = kthread_create(kthread_worker_fn, &sg_policy->worker,
				"uag_gov:%d",
				cpumask_first(policy->related_cpus));
	if (IS_ERR(thread)) {
		pr_err("failed to create uag_gov thread: %ld\n", PTR_ERR(thread));
		return PTR_ERR(thread);
	}

	ret = sched_setattr_nocheck(thread, &attr);
	if (ret) {
		kthread_stop(thread);
		pr_warn("%s: failed to set SCHED_DEADLINE\n", __func__);
		return ret;
	}

	sg_policy->thread = thread;
	kthread_bind_mask(thread, policy->related_cpus);
	init_irq_work(&sg_policy->irq_work, uag_gov_irq_work);
	mutex_init(&sg_policy->work_lock);

	wake_up_process(thread);

	return 0;
}

static void uag_gov_kthread_stop(struct uag_gov_policy *sg_policy)
{
	/* kthread only required for slow path */
	if (sg_policy->policy->fast_switch_enabled)
		return;

	kthread_flush_worker(&sg_policy->worker);
	kthread_stop(sg_policy->thread);
	mutex_destroy(&sg_policy->work_lock);
}

static struct uag_gov_tunables *uag_gov_tunables_alloc(struct uag_gov_policy *sg_policy)
{
	struct uag_gov_tunables *tunables;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (tunables) {
		gov_attr_set_init(&tunables->attr_set, &sg_policy->tunables_hook);
		if (!have_governor_per_policy())
			global_tunables = tunables;
	}
	return tunables;
}

static void uag_gov_tunables_free(struct uag_gov_tunables *tunables)
{
	if (!have_governor_per_policy())
		global_tunables = NULL;

	kfree(tunables);
}

#define DEFAULT_HISPEED_LOAD 90
static void uag_gov_tunables_save(struct cpufreq_policy *policy,
		struct uag_gov_tunables *tunables)
{
	int cpu;
	struct uag_gov_tunables *cached = per_cpu(cached_tunables, policy->cpu);

	if (!cached) {
		cached = kzalloc(sizeof(*tunables), GFP_KERNEL);
		if (!cached)
			return;

		for_each_cpu(cpu, policy->related_cpus)
			per_cpu(cached_tunables, cpu) = cached;
	}

	cached->hispeed_load = tunables->hispeed_load;
	cached->hispeed_freq = tunables->hispeed_freq;
}

static void uag_gov_tunables_restore(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy = policy->governor_data;
	struct uag_gov_tunables *tunables = sg_policy->tunables;
	struct uag_gov_tunables *cached = per_cpu(cached_tunables, policy->cpu);

	if (!cached)
		return;

	tunables->hispeed_load = cached->hispeed_load;
	tunables->hispeed_freq = cached->hispeed_freq;
}

#ifdef CONFIG_ARCH_MEDIATEK
/*cpu enter idle vendor hook
*curr_frame_start: current frame start timestamp
*prev_frame_start: prev frame start timestamp
*/
static void android_vh_cpu_idle_enter_handler(void *unused, int *index, struct cpuidle_device *dev)
{
	struct uag_gov_cpu *ug_cpu = &per_cpu(uag_gov_cpu, dev->cpu);
	u64 now = ktime_get_ns();

	/*if system don't refresh for more than 3 frames time, set prev frame loading 0*/
	if ((now - curr_frame_start) >= 3 * (curr_frame_start - prev_frame_start)) {
		ug_cpu->prev_frame_loading = 0;
		return;
	}

	ug_cpu->prev_enter_ilde = ug_cpu->enter_idle;
	ug_cpu->enter_idle = now;

	/*if current frame start too late to update*/
	if ((ug_cpu->prev_enter_ilde <= curr_frame_start) && (ug_cpu->exit_idle >= curr_frame_start)
		&& (ug_cpu->idle_duration != ug_cpu->exit_idle - curr_frame_start)) {
		if (ug_cpu->prev_enter_ilde <= prev_frame_start) {
			ug_cpu->prev_idle_duration = curr_frame_start - prev_frame_start;
		} else
			ug_cpu->prev_idle_duration = ug_cpu->idle_duration - (ug_cpu->exit_idle - curr_frame_start);
		ug_cpu->idle_duration = ug_cpu->exit_idle - curr_frame_start;
	}

    /*cpu exit idle before current frame
    *       prev_frame           curr_frame
    *         |      _____exit idle   |  ___enter idle
    *eg:______|_____|_____|___________|_|___
    *		  prev_frame_duration
    */
	if ((ug_cpu->enter_idle >= curr_frame_start) && (ug_cpu->exit_idle <= curr_frame_start)) {
		ug_cpu->prev_idle_duration = ug_cpu->idle_duration;
		ug_cpu->idle_duration = 0;
	}

	/* some exception 2 frames caculate idle together */
	if (ug_cpu->prev_idle_duration > curr_frame_start - prev_frame_start)
		ug_cpu->prev_idle_duration /= 2;
	ug_cpu->prev_frame_loading =
		100 - ((ug_cpu->prev_idle_duration * 100) / (curr_frame_start - prev_frame_start));

	/* dicard frame loading when other exception */
	if (ug_cpu->prev_frame_loading > 100)
		ug_cpu->prev_frame_loading = 0;

	trace_printk("prev_frame_loading = %d\n", ug_cpu->prev_frame_loading);
}

/*cpu exit idle vendor hook*/
static void android_vh_cpu_idle_exit_handler(void *unused, int index, struct cpuidle_device *dev)
{
	struct uag_gov_cpu *ug_cpu = &per_cpu(uag_gov_cpu, dev->cpu);
	u64 now = ktime_get_ns();

	/*if system don't refresh for more than 3 frames time, set prev frame loading 0*/
	if ((now - curr_frame_start) >= 3 * (curr_frame_start - prev_frame_start)) {
		ug_cpu->prev_frame_loading = 0;
		return;
	}

	ug_cpu->prev_exit_idle = ug_cpu->exit_idle;
	ug_cpu->exit_idle = now;
	trace_printk("prev_exit_idle = %lu, exit_idle =%lu, curr_frame_start = %lu, prev_frame_start = %lu\n",
		ug_cpu->prev_exit_idle, ug_cpu->exit_idle, curr_frame_start, prev_frame_start);
    /*if cpu enter idle before prev frame start, then
    *prev frame duration equals to prev frame length.
    *           prev frame   curr frame
    *    enter idle _|___________|__exit idle
    *eg:___________|_|___________|__|_______
    */
	if (ug_cpu->enter_idle <= prev_frame_start) {
		ug_cpu->prev_idle_duration = curr_frame_start - prev_frame_start;
	} else if (ug_cpu->enter_idle <= curr_frame_start) {
	    /*if cpu enter idle before current frame
        *      prev_frame   curr_frame
        *         |  enter dile___|____exit idle
        *eg:______|___________|___|____|___
	    *		  prev_frame_duration
	    */
		ug_cpu->prev_idle_duration =
			ug_cpu->idle_duration + (curr_frame_start - ug_cpu->enter_idle);
		if (now >= curr_frame_start)
			ug_cpu->idle_duration = now - curr_frame_start;
		else
			ug_cpu->idle_duration = 0;
	} else {
		/*caculate current frame duration*/
		if ((ug_cpu->prev_exit_idle < curr_frame_start) &&
			(ug_cpu->exit_idle > curr_frame_start) &&
			(ug_cpu->idle_duration != 0)) {
			/*if current frame start too late to update*/
			ug_cpu->prev_idle_duration = ug_cpu->idle_duration;
			ug_cpu->idle_duration = ug_cpu->exit_idle - ug_cpu->enter_idle;
		} else
			ug_cpu->idle_duration += now - ug_cpu->enter_idle;
	}
	/* some exception 2 frames caculate idle together */
	if (ug_cpu->prev_idle_duration > curr_frame_start - prev_frame_start)
		ug_cpu->prev_idle_duration /= 2;
	ug_cpu->prev_frame_loading =
		100 - ((ug_cpu->prev_idle_duration * 100) / (curr_frame_start - prev_frame_start));

	/* dicard frame loading when other exception */
	if (ug_cpu->prev_frame_loading > 100)
		ug_cpu->prev_frame_loading = 0;
	trace_printk("prev_frame_loading = %d\n", ug_cpu->prev_frame_loading);
}
#endif /* CONFIG_ARCH_MEDIATEK */

static int uag_gov_init(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy;
	struct uag_gov_tunables *tunables;
	int ret = 0;
	int cluster_id, first_cpu;

	/* State should be equivalent to EXIT */
	if (policy->governor_data)
		return -EBUSY;

	cpufreq_enable_fast_switch(policy);

	sg_policy = uag_gov_policy_alloc(policy);
	if (!sg_policy) {
		ret = -ENOMEM;
		goto disable_fast_switch;
	}

	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
#ifdef CONFIG_OPLUS_UAG_USE_TL
#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
	sg_policy->len = pd_get_nr_caps(cluster_id);
#else
	sg_policy->len = get_opp_count(policy);
#endif
#endif

	ret = uag_gov_kthread_create(sg_policy);// 创建一个慢速调频线程
	if (ret)
		goto free_sg_policy;

	mutex_lock(&global_tunables_lock);

	if (global_tunables) {
		if (WARN_ON(have_governor_per_policy())) {
			ret = -EINVAL;
			goto stop_kthread;
		}
		policy->governor_data = sg_policy;
		sg_policy->tunables = global_tunables;

		gov_attr_set_get(&global_tunables->attr_set, &sg_policy->tunables_hook);//将sg_policy挂在共享tunables的链表上
		goto out;
	}

	tunables = uag_gov_tunables_alloc(sg_policy);
	if (!tunables) {
		ret = -ENOMEM;
		goto stop_kthread;
	}

	tunables->rate_limit_us = cpufreq_policy_transition_delay_us(policy);
	tunables->hispeed_load = DEFAULT_HISPEED_LOAD;
	tunables->hispeed_freq = 0;

	if (policy->cpu == 0)
		tunables->cobuck_enable = 0;
	else
		tunables->cobuck_enable = 0;

	sg_policy->hispeed_util = target_util(sg_policy, tunables->hispeed_freq);
#ifdef CONFIG_OPLUS_UAG_USE_TL
	tunables->target_loads = default_target_loads;
	tunables->ntarget_loads = ARRAY_SIZE(default_target_loads);
	//same with target_loads by default
	tunables->util_loads = default_target_loads;
	spin_lock_init(&tunables->target_loads_lock);
#endif
#ifdef CONFIG_OPLUS_MULTI_LV_TL
	tunables->multi_tl[UA_UTIL_SYS].target_loads = default_target_loads_sys;
	tunables->multi_tl[UA_UTIL_SYS].ntarget_loads = ARRAY_SIZE(default_target_loads_sys); //默认的sys的tl是95， 而设进tunables的才是fbg的target
	tunables->multi_tl[UA_UTIL_SYS].util_loads = default_target_loads_sys;
	uag_reg_multi_tl_fbg(tunables);
	tunables->multi_tl_enable = 1;
#endif

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	/* init stall cal */
	tunables->stall_aware = 1;
	tunables->stall_reduce_pct = 70;
	tunables->report_policy = REPORT_REDUCE_STALL;
#endif

#ifdef CONFIG_OPLUS_UAG_SOFT_LIMIT
	tunables->soft_limit_freq = policy->cpuinfo.max_freq;
	tunables->soft_limit_util = freq2util(sg_policy, policy->cpuinfo.max_freq);
	tunables->soft_limit_enable = 1;
	tunables->break_freq_margin = DEFAULT_BREAK_FREQ_MARGIN;
	spin_lock_init(&tunables->soft_limit_freq_lock);
#endif

	policy->governor_data = sg_policy;
	sg_policy->tunables = tunables;
	uag_gov_tunables_restore(policy);

	ret = kobject_init_and_add(&tunables->attr_set.kobj, &uag_gov_tunables_ktype,
				   get_governor_parent_kobj(policy), "%s",
				   cpufreq_uag_gov.name);// 在policy下创建tunables的节点   policy*/uag/
	if (ret)
		goto fail;

	policy->dvfs_possible_from_any_cpu = 1;

	if (cluster_id < MAX_CLUSTERS)
		init_flag[cluster_id] = 1;

out:
	mutex_unlock(&global_tunables_lock);
	return 0;

fail:
	kobject_put(&tunables->attr_set.kobj);
	policy->governor_data = NULL;
	uag_gov_tunables_free(tunables);

stop_kthread:
	uag_gov_kthread_stop(sg_policy);
	mutex_unlock(&global_tunables_lock);

free_sg_policy:
	uag_gov_policy_free(sg_policy);

disable_fast_switch:
	cpufreq_disable_fast_switch(policy);

	pr_err("initialization failed (error %d)\n", ret);
	return ret;
}

static void uag_gov_exit(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy = policy->governor_data;
	struct uag_gov_tunables *tunables = sg_policy->tunables;
	unsigned int count;
	unsigned int first_cpu;
	int cluster_id;

	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
	if (cluster_id < MAX_CLUSTERS)
		init_flag[cluster_id] = 0;

	mutex_lock(&global_tunables_lock);

	count = gov_attr_set_put(&tunables->attr_set, &sg_policy->tunables_hook);
	policy->governor_data = NULL;
	if (!count) {
		uag_gov_tunables_save(policy, tunables);
		uag_gov_tunables_free(tunables);
	}

	mutex_unlock(&global_tunables_lock);

	uag_gov_kthread_stop(sg_policy);
	uag_gov_cpu_reset(sg_policy);
	uag_gov_policy_free(sg_policy);
	cpufreq_disable_fast_switch(policy);
}

static int uag_gov_start(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;
#if defined(CONFIG_OPLUS_UAG_USE_TL) && defined(CONFIG_ARCH_QCOM)
	unsigned int first_cpu;
	int cluster_id;
#endif
#if defined(CONFIG_UAG_NONLINEAR_FREQ_CTL) && defined(CONFIG_ARCH_MEDIATEK)
	unsigned int first_cpu;
	int cluster_id;
#endif

	sg_policy->freq_update_delay_ns	= sg_policy->tunables->rate_limit_us * NSEC_PER_USEC;
	sg_policy->last_freq_update_time	= 0;
	sg_policy->next_freq			= 0;
	sg_policy->work_in_progress		= false;
	sg_policy->limits_changed		= false;
	sg_policy->cached_raw_freq		= 0;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	sg_policy->flags = 0;
#endif

	sg_policy->need_freq_update = false;

	for_each_cpu(cpu, policy->cpus) {
		struct uag_gov_cpu *sg_cpu = &per_cpu(uag_gov_cpu, cpu);

		memset(sg_cpu, 0, sizeof(*sg_cpu));
		sg_cpu->cpu			= cpu;
		sg_cpu->sg_policy		= sg_policy;
	}

#if defined(CONFIG_OPLUS_UAG_USE_TL) && defined(CONFIG_ARCH_QCOM)
	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
	g_em_map_util_freq.cem_map_util_freq[cluster_id].pgov_map_func = uag_update_util;
	g_em_map_util_freq.cem_map_util_freq[cluster_id].gov_id = 2;
	//register_trace_android_vh_map_util_freq(uag_update_util, NULL);// 该函数在get_next_freq中调用，但是uag没有使用
#endif

#if defined(CONFIG_UAG_NONLINEAR_FREQ_CTL) && defined(CONFIG_ARCH_MEDIATEK)
	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
	g_em_map_util_freq.cem_map_util_freq[cluster_id].pgov_map_func = nonlinear_map_util_freq;
	g_em_map_util_freq.cem_map_util_freq[cluster_id].gov_id = 2;
	//register_trace_android_vh_map_util_freq(nonlinear_map_util_freq, NULL);
#endif

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	uag_register_stall_update(); //回调在trace_sched_stat_runtime
#endif

	for_each_cpu(cpu, policy->cpus) {
		struct uag_gov_cpu *sg_cpu = &per_cpu(uag_gov_cpu, cpu);
#ifdef CONFIG_ARCH_MEDIATEK
		cpufreq_add_update_util_hook(cpu, &sg_cpu->update_util,
#else
		cpufreq_add_update_util_hook(cpu, &sg_cpu->update_util,
#endif
					     policy_is_shared(policy) ?
							uag_gov_update_shared :
							uag_gov_update_single);
	}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
#ifdef CONFIG_ARCH_MEDIATEK
	fbg_add_update_freq_hook(cpufreq_update_util);
#else
	fbg_add_update_freq_hook(cpufreq_update_util);
#endif /* CONFIG_ARCH_MEDIATEK */
#endif
	return 0;
}

static void uag_gov_stop(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;
#if defined(CONFIG_OPLUS_UAG_USE_TL) && defined(CONFIG_ARCH_QCOM)
	unsigned int first_cpu;
	int cluster_id;
#endif
#if defined(CONFIG_UAG_NONLINEAR_FREQ_CTL) && defined(CONFIG_ARCH_MEDIATEK)
	unsigned int first_cpu;
	int cluster_id;
#endif

	for_each_cpu(cpu, policy->cpus)
#ifdef CONFIG_ARCH_MEDIATEK
		cpufreq_remove_update_util_hook(cpu);
#else
		cpufreq_remove_update_util_hook(cpu);
#endif /* CONFIG_ARCH_MEDIATEK */

#if defined(CONFIG_OPLUS_UAG_USE_TL) && defined(CONFIG_ARCH_QCOM)
	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
	g_em_map_util_freq.cem_map_util_freq[cluster_id].pgov_map_func = default_em_map_util_freq;
	g_em_map_util_freq.cem_map_util_freq[cluster_id].gov_id = 0;
	//unregister_trace_android_vh_map_util_freq(uag_update_util, NULL);
#endif


#if defined(CONFIG_UAG_NONLINEAR_FREQ_CTL) && defined(CONFIG_ARCH_MEDIATEK)
	first_cpu = cpumask_first(policy->related_cpus);
	cluster_id = topology_physical_package_id(first_cpu);
	g_em_map_util_freq.cem_map_util_freq[cluster_id].pgov_map_func = default_em_map_util_freq;
	g_em_map_util_freq.cem_map_util_freq[cluster_id].gov_id = 0;
	//unregister_trace_android_vh_map_util_freq(nonlinear_map_util_freq, NULL);
#endif

#ifdef CONFIG_OPLUS_UAG_AMU_AWARE
	uag_unregister_stall_update();
#endif

	synchronize_rcu();

	if (!policy->fast_switch_enabled) {
		irq_work_sync(&sg_policy->irq_work);
		kthread_cancel_work_sync(&sg_policy->work);
	}
}

static void uag_gov_limits(struct cpufreq_policy *policy)
{
	struct uag_gov_policy *sg_policy = policy->governor_data;
	unsigned long flags, now;
	unsigned int freq, final_freq;

	if (!policy->fast_switch_enabled) {
		mutex_lock(&sg_policy->work_lock);
		raw_spin_lock_irqsave(&sg_policy->update_lock, flags);
		uag_track_cycles(sg_policy, sg_policy->policy->cur,
				   ktime_get_ns());
		raw_spin_unlock_irqrestore(&sg_policy->update_lock, flags);
		cpufreq_policy_apply_limits(policy);
		mutex_unlock(&sg_policy->work_lock);
	} else {
		raw_spin_lock_irqsave(&sg_policy->update_lock, flags);
		freq = policy->cur;
		now = sched_ktime_clock();

		/*
		 * cpufreq_driver_resolve_freq() has a clamp, so we do not need
		 * to do any sort of additional validation here.
		 */
		final_freq = cpufreq_driver_resolve_freq(policy, freq);

		if (uag_gov_update_next_freq(sg_policy, now, final_freq)) {
			uag_gov_fast_switch(sg_policy, now, final_freq);
		}
		raw_spin_unlock_irqrestore(&sg_policy->update_lock, flags);
	}

	sg_policy->limits_changed = true;
}

struct cpufreq_governor cpufreq_uag_gov = {
	.name			= "uag",
	.owner			= THIS_MODULE,
	.dynamic_switching	= true,
	.init			= uag_gov_init,
	.exit			= uag_gov_exit,
	.start			= uag_gov_start,
	.stop			= uag_gov_stop,
	.limits			= uag_gov_limits,
};

#ifdef CONFIG_ARCH_MEDIATEK
static void register_uag_hooks(void)
{
	pr_err("uag register vendor hook!!!\n");
	/* register vendor hook in kernel/drivers/cpuidle/cpuidle.c */
	register_trace_android_vh_cpu_idle_enter(android_vh_cpu_idle_enter_handler, NULL);
	register_trace_android_vh_cpu_idle_exit(android_vh_cpu_idle_exit_handler, NULL);
}
#endif /* CONFIG_ARCH_MEDIATEK */

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_UAG
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_uag_gov;
}
#endif

#ifdef CONFIG_OPLUS_MULTI_LV_TL
static int multi_tl_stat_show(struct seq_file *s, void *data)
{
	unsigned int cpu;
	struct uag_gov_policy *sg_policy = NULL;

	for_each_possible_cpu(cpu) {
		struct uag_gov_cpu *sg_cpu = &per_cpu(uag_gov_cpu, cpu);

		if (!sg_cpu->sg_policy || sg_policy == sg_cpu->sg_policy)
			continue;

		sg_policy = sg_cpu->sg_policy;

		seq_printf(s, "policy%u\nfbg_gt_sys_cnt %u, total_cnt %u\n",
				cpu, sg_policy->fbg_gt_sys_cnt,
				sg_policy->total_cnt);
		sg_policy->fbg_gt_sys_cnt = 0;
		sg_policy->total_cnt = 0;
	}

	return 0;
}

static int multi_tl_stat_open_proc(struct inode *inode, struct file *file)
{
	return single_open(file, multi_tl_stat_show, NULL);
}

static const struct file_operations multi_tl_stat_ops = {
	.open	= multi_tl_stat_open_proc,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

static inline void multi_tl_proc_create(struct proc_dir_entry *dir)
{
	if (!proc_create("multi_tl_stat", 0644, dir, &multi_tl_stat_ops))
		pr_warn("proc/uag/multi_tl_stat create failed\n");
}
#endif

static int __init cpufreq_uag_init(void)
{
	int ret = 0;
	struct proc_dir_entry *dir = NULL;

	dir = proc_mkdir("uag", NULL);

#ifdef CONFIG_OPLUS_MULTI_LV_TL
	if (dir)
		multi_tl_proc_create(dir);
#endif

#ifdef CONFIG_ARCH_MEDIATEK
	register_uag_hooks();
#endif /* CONFIG_ARCH_MEDIATEK */

#ifdef CONFIG_UAG_NONLINEAR_FREQ_CTL
	ret = init_opp_cap_info(dir);
	if (ret) {
		pr_err("moduler init failed (error %d)\n", ret);
		return ret;
	}
#endif

	ret = cpufreq_register_governor(&cpufreq_uag_gov);
	return ret;
}

static void __exit cpufreq_uag_exit(void)
{
	clear_opp_cap_info();
	cpufreq_unregister_governor(&cpufreq_uag_gov);
}

module_init(cpufreq_uag_init);
module_exit(cpufreq_uag_exit);

MODULE_LICENSE("GPL v2");
