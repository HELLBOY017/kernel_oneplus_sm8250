// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <../kernel/sched/sched.h>

#include <trace/events/sched.h>

#include "cpufreq_uag.h"
#include "stall_util_cal.h"

static int uag_amu_enable;

DEFINE_PER_CPU(struct amu_data, amu_cntr);
DEFINE_PER_CPU(struct amu_data, amu_prev_cntr);
DEFINE_PER_CPU(struct amu_data, amu_delta);

DEFINE_PER_CPU(u64, amu_update_delta_time);
DEFINE_PER_CPU(u64, amu_last_update_time);
DEFINE_PER_CPU(u64, amu_normal_util);
DEFINE_PER_CPU(u64, amu_stall_util);

void uag_adjust_util(int cpu, unsigned long *util, void *ptr)
{
	struct uag_gov_policy *sg_pol = (struct uag_gov_policy *) ptr;
	struct uag_gov_tunables *tunables = sg_pol->tunables;
	unsigned long orig = *util;
	u64 normal, stall, reduce_pct;
	s64 result, result_2;

	if (unlikely(!uag_amu_enable))
		return;

	if (!tunables->stall_aware)
		return;

	reduce_pct = tunables->stall_reduce_pct;

	normal = min(per_cpu(amu_normal_util, cpu), 1024LLU);
	stall = min(per_cpu(amu_stall_util, cpu), 1024LLU);
	result = orig - (stall * reduce_pct / 100);
	result_2 = normal - (stall * reduce_pct / 100);

	/* adjust policy */
	switch (tunables->report_policy) {
	case REPORT_NONE:
		break;
	case REPORT_MAX_UTIL:
		*util = max_t(s64, normal, orig);
		break;
	case REPORT_MIN_UTIL:
		*util = 0;
		break;
	case REPORT_DIRECT:
		*util = normal;
		break;
	case REPORT_REDUCE_STALL:
		*util = max_t(s64, result, 0);
		break;
	case REPORT_NORMAL_REDUCE_STALL:
		*util = max_t(s64, result_2, 0);
		break;
	}

	trace_uag_amu_adjust_util(
		cpu, orig, normal, stall, reduce_pct,
		result, *util, tunables->report_policy);
}

static inline u64 get_max_freq(void)
{
	struct cpufreq_policy *pol;
	int cpu = num_possible_cpus() - 1, i;
	u64 freq = 0;

	for (i = cpu; i >= 0 && !freq; --i) {
		pol = cpufreq_cpu_get(i);
		if (likely(pol)) {
			freq = pol->max;
			cpufreq_cpu_put(pol);
		}
	}
	return freq;
}

void uag_update_counter(void *ptr)
{
	int cpu, i;
	struct uag_gov_policy *sg_pol = (struct uag_gov_policy *) ptr;
	struct cpufreq_policy *pol = sg_pol->policy;
	struct uag_gov_tunables *tunables = sg_pol->tunables;
	u64 max_freq = get_max_freq();
	u64 time = ktime_get();

	if (unlikely(!uag_amu_enable))
		return;

	if (!tunables->stall_aware)
		return;

	if (!max_freq)
		return;

	for_each_cpu(cpu, pol->cpus) {
		struct amu_data data;
		u64 delta_cycle, delta_stall_cycle, delta_time;

		for (i = 0; i < SYS_AMU_MAX; ++i) {
			data.val[i] = per_cpu(amu_cntr, cpu).val[i];
			per_cpu(amu_delta, cpu).val[i] = data.val[i] - per_cpu(amu_prev_cntr, cpu).val[i];
			per_cpu(amu_prev_cntr, cpu).val[i] = data.val[i];
		}
		per_cpu(amu_update_delta_time, cpu) = delta_time = ktime_sub(time, per_cpu(amu_last_update_time, cpu));
		per_cpu(amu_last_update_time, cpu) = time;

		delta_cycle = per_cpu(amu_delta, cpu).val[SYS_AMU_CORE_CYC];
		delta_stall_cycle = per_cpu(amu_delta, cpu).val[SYS_AMU_STALL_MEM];

		if (delta_time) {
			/* delta_time: ns */
			per_cpu(amu_normal_util, cpu) = delta_cycle ?
				((delta_cycle * 1000000LLU * 1024LLU) / delta_time) / max_freq : 0;

			per_cpu(amu_stall_util, cpu) = delta_stall_cycle ?
				((delta_stall_cycle * 1000000LLU * 1024LLU) / delta_time) / max_freq : 0;

		} else {
			per_cpu(amu_normal_util, cpu) = 0;
			per_cpu(amu_stall_util, cpu) = 0;
		}

		trace_uag_update_amu_counter(cpu, time);
	}
}

static void uag_single_update(void *unused, struct task_struct *tsk,
	u64 runtime, u64 vruntime)
{
	int cpu = smp_processor_id();
	int i;

	/* FIXME debug for MT6895 */
	return;

	for (i = 0; i < SYS_AMU_MAX; ++i) {
		switch (i) {
		case SYS_AMU_CONST_CYC:
			per_cpu(amu_cntr, cpu).val[i] = read_sysreg_s(SYS_AMEVCNTR0_CONST_EL0);
			break;
		case SYS_AMU_CORE_CYC:
			per_cpu(amu_cntr, cpu).val[i] = read_sysreg_s(SYS_AMEVCNTR0_CORE_EL0);
			break;
		case SYS_AMU_INST_RET:
			per_cpu(amu_cntr, cpu).val[i] = read_sysreg_s(SYS_AMEVCNTR0_INST_RET_EL0);
			break;
		case SYS_AMU_STALL_MEM:
			per_cpu(amu_cntr, cpu).val[i] = read_sysreg_s(SYS_AMEVCNTR0_MEM_STALL);
			break;
		}
	}
}

void uag_register_stall_update(void)
{
	if (IS_ENABLED(CONFIG_ARM64_AMU_EXTN)) {
		register_trace_sched_stat_runtime(uag_single_update, NULL);
#ifdef CONFIG_ARCH_QCOM
		uag_amu_enable = 1;
#else
		uag_amu_enable = 0;
#endif
	}
}

void uag_unregister_stall_update(void)
{
	if (IS_ENABLED(CONFIG_ARM64_AMU_EXTN)) {
		uag_amu_enable = 0;
		unregister_trace_sched_stat_runtime(uag_single_update, NULL);
	}
}
