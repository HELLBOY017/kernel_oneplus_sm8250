/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#ifndef __OPLUS_CPU_JANK_HOTTHREAD_H__
#define __OPLUS_CPU_JANK_HOTTHREAD_H__

#include "osi_base.h"

struct task_track {
	u64 timestamp;
	pid_t pid;
	pid_t tgid;
	uid_t uid;
	char comm[TASK_COMM_LEN];
	char leader_comm[TASK_COMM_LEN];
	struct task_record record;
};

struct task_track_cpu {
	struct task_track track[JANK_WIN_CNT];
};

struct hot_thread_struct {
	u64 timestamp;
	pid_t pid;
	pid_t tgid;
	uid_t uid;
	char comm[TASK_COMM_LEN];
	char leader_comm[TASK_COMM_LEN];
	u8 top_app_cnt;
	u8 non_topapp_cnt;
	u8 total_cnt;
} ____cacheline_aligned;

#define TOP_THREAD_CNT     (5)
#define MAX_CLUSTER        (3)
#define TICK_PER_WIN       (32)
extern struct hot_thread_struct  hot_thread_top[JANK_WIN_CNT][TOP_THREAD_CNT];

void jank_hotthread_update_tick(struct task_struct *p,
				u64 now);
void jank_hotthread_show(struct seq_file *m, u32 win_idx,
				u64 now);
void hotthread_show(struct seq_file *m, u32 win_idx,
				u64 now);
int osi_hotthread_proc_init(struct proc_dir_entry *pde);
void osi_hotthread_proc_deinit(struct proc_dir_entry *pde);
#endif  /* endif */
