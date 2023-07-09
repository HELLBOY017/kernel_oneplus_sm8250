/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _FRAME_DEBUG_H
#define _FRAME_DEBUG_H

void val_systrace_c(unsigned long val, char *msg);
void frame_min_util_systrace_c(int util);
void cfs_policy_util_systrace_c(unsigned long util);
void cfs_curr_util_systrace_c(unsigned long util);
void rt_policy_util_systrace_c(unsigned long util);
void rt_curr_util_systrace_c(unsigned long util);
void raw_util_systrace_c(unsigned long util);
void pref_cpus_systrace_c(unsigned int cpu);
void avai_cpus_systrace_c(unsigned int cpu);
void query_cpus_systrace_c(unsigned int cpu);
void sf_zone_systrace_c(unsigned int zone);
void def_zone_systrace_c(unsigned int zone);
void cpu_util_systrace_c(unsigned long util, unsigned int cpu, char *msg);

#endif /* _FRAME_DEBUG_H */
