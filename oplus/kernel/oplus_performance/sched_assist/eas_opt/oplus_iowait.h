// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef __OPLUS_IOWAIT__
#define __OPLUS_IOWAIT__

extern int eas_opt_enable;
extern int eas_opt_debug_enable;
extern unsigned int sysctl_iowait_reset_ticks;
extern unsigned int sysctl_iowait_apply_ticks;
extern unsigned int sysctl_oplus_iowait_boost_enabled;
extern unsigned int sysctl_oplus_iowait_skip_min_enabled;
struct proc_dir_entry;
extern int oplus_sched_assist_iowait_proc_init(struct proc_dir_entry *dir);
extern void oplus_sched_assist_iowait_proc_remove(struct proc_dir_entry *dir);

#endif /* __OPLUS_IOWAIT__ */
