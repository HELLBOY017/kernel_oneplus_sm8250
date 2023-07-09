// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/string.h>
#include <linux/kernel.h>

static noinline int tracing_mark_write(const char *buf)
{
	trace_printk(buf);
	return 0;
}


void val_systrace_c(unsigned long val, char *msg)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "C|10000|%s|%lu\n", msg, val);
	tracing_mark_write(buf);
}

void frame_min_util_systrace_c(int util)
{
	static int prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "frame_min_util");
		prev_util = util;
	}
}

void cfs_policy_util_systrace_c(unsigned long util)
{
	static unsigned long prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "cfs_policy_util");
		prev_util = util;
	}
}

void cfs_curr_util_systrace_c(unsigned long util)
{
	static unsigned long prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "cfs_curr_util");
		prev_util = util;
	}
}

void rt_policy_util_systrace_c(unsigned long util)
{
	static unsigned long prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "rt_policy_util");
		prev_util = util;
	}
}

void rt_curr_util_systrace_c(unsigned long util)
{
	static unsigned long prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "rt_curr_util");
		prev_util = util;
	}
}

void raw_util_systrace_c(unsigned long util)
{
	static unsigned long prev_util = 0;

	if (prev_util != util) {
		val_systrace_c(util, "raw_util");
		prev_util = util;
	}
}


void pref_cpus_systrace_c(unsigned int cpu)
{
	static unsigned int prev_cpu = 0;

	if (prev_cpu != cpu) {
		val_systrace_c(cpu, "pref_cpus");
		prev_cpu = cpu;
	}
}

void avai_cpus_systrace_c(unsigned int cpu)
{
	static unsigned int prev_cpu = 0;

	if (prev_cpu != cpu) {
		val_systrace_c(cpu, "avai_cpus");
		prev_cpu = cpu;
	}
}

void query_cpus_systrace_c(unsigned int cpu)
{
	static unsigned int prev_cpu = 0;

	if (prev_cpu != cpu) {
		val_systrace_c(cpu, "query_cpus");
		prev_cpu = cpu;
	}
}


void sf_zone_systrace_c(unsigned int zone)
{
	static unsigned int prev_zone = 0;

	if (prev_zone != zone) {
		val_systrace_c(zone, "frame_zone_sf");
		prev_zone = zone;
	}
}

void def_zone_systrace_c(unsigned int zone)
{
	static unsigned int prev_zone = 0;

	if (prev_zone != zone) {
		val_systrace_c(zone, "frame_zone_def");
		prev_zone = zone;
	}
}

void cpu_util_systrace_c(unsigned long util, unsigned int cpu, char *msg)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "C|10000|%s_cpu%d|%lu\n", msg, cpu, util);
	tracing_mark_write(buf);
}
