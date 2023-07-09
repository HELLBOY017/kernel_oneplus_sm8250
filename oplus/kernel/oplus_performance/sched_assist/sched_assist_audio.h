/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SA_AUDIO_H_
#define _OPLUS_SA_AUDIO_H_


extern int oplus_sched_assist_audio_proc_init(struct proc_dir_entry *dir);
extern void oplus_sched_assist_audio_proc_remove(struct proc_dir_entry *dir);

extern void oplus_sched_assist_audio_perf_set_status(int status);

extern void oplus_sched_assist_audio_latency_sensitive(struct task_struct *t, bool *latency_sensitive);
extern void oplus_sched_assist_audio_time_slack(struct task_struct *task);
extern bool oplus_sched_assist_audio_idle_balance(struct rq *this_rq);
extern void oplus_sched_assist_audio_perf_addIm(struct task_struct *task, int im_flag);
extern bool oplus_sched_assist_audio_perf_check_exit_latency(struct task_struct *task, int cpu);
extern void oplus_sched_assist_audio_enqueue_hook(struct task_struct *task);
#endif /* _OPLUS_SA_AUDIO_H_ */
