/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _FRAME_GROUP_H
#define _FRAME_GROUP_H

#define DEFAULT_FRAME_GROUP_ID (1)
#define SF_FRAME_GROUP_ID (2)
#define GAME_FRAME_GROUP_ID (3)

enum DYNAMIC_TRANS_TYPE {
	DYNAMIC_TRANS_BINDER = 0,
	DYNAMIC_TRANS_TYPE_MAX,
};

enum freq_update_flags {
	FRAME_FORCE_UPDATE = (1 << 0),
	FRAME_NORMAL_UPDATE = (1 << 1),
};

#define FRAME_ZONE       (1 << 0)
#define USER_ZONE        (1 << 1)

#define SCHED_CPUFREQ_DEF_FRAMEBOOST    (1U << 6)
#define SCHED_CPUFREQ_SF_FRAMEBOOST     (1U << 7)

struct oplus_sched_cluster {
	struct list_head	list;
	struct cpumask	cpus;
	int id;
};

extern int fbg_num_sched_clusters;
struct binder_transaction;
struct binder_thread;
struct binder_proc;
/* FIXME */
#define MAX_CLS_NUM 5
extern struct oplus_sched_cluster *fb_cluster[MAX_CLS_NUM];

void task_rename_hook(void *unused, struct task_struct *p, const char *buf);
void fbg_binder_wakeup_hook(void *unused, struct task_struct *caller_task,
		struct task_struct *binder_proc_task, struct task_struct *binder_th_task,
		unsigned int code, bool pending_async, bool sync);
void fbg_binder_restore_priority_hook(void *unused, struct binder_transaction *t,
			struct task_struct *task);
void fbg_binder_wait_for_work_hook(void *unused, bool do_proc_work,
			struct binder_thread *tsk, struct binder_proc *proc);
void fbg_sync_txn_recvd_hook(void *unused, struct task_struct *tsk, struct task_struct *from);
void fbg_update_cfs_util_hook(void *unused, struct task_struct *tsk,
			u64 runtime, u64 vruntime);
void fbg_update_rt_util_hook(void *unused, struct task_struct *tsk, u64 runtime);
void fbg_flush_task_hook(void *unused, struct task_struct *tsk);
void fbg_sched_fork_hook(void *unused, struct task_struct *tsk);

bool frame_boost_enabled(void);
bool is_fbg_task(struct task_struct *p);
int frame_group_init(void);
u64 fbg_ktime_get_ns(void);
void fbg_add_update_freq_hook(void (*func)(struct rq *rq, unsigned int flags));
void register_frame_group_vendor_hooks(void);
int rollover_frame_group_window(int group_id);
void set_frame_group_window_size(unsigned int window);
void set_ui_thread(int pid, int tid);
void set_render_thread(int pid, int tid);
void set_sf_thread(int pid, int tid);
void set_renderengine_thread(int pid, int tid);
bool add_rm_related_frame_task(int pid, int tid, int add, int r_depth, int r_width);
bool add_task_to_game_frame_group(int tid, int add);
bool default_group_update_cpufreq(void);
int get_frame_group_ui(void);

bool fbg_freq_policy_util(unsigned int policy_flags, const struct cpumask *query_cpus,
	unsigned long *util);
bool set_frame_group_task_to_perfer_cpu(struct task_struct *p, int *target_cpu);
bool fbg_need_up_migration(struct task_struct *p, struct rq *rq);
bool fbg_skip_migration(struct task_struct *tsk, int src_cpu, int dst_cpu);
bool fbg_skip_rt_sync(struct rq *rq, struct task_struct *p, bool *sync);
bool check_putil_over_thresh(unsigned long thresh);
bool fbg_rt_task_fits_capacity(struct task_struct *tsk, int cpu);

void fbg_android_rvh_schedule_handler(struct task_struct *prev,
	struct task_struct *next, struct rq *rq);

void fbg_get_frame_scale(unsigned long *frame_scale);
void fbg_get_frame_busy(unsigned int *frame_busy);

int info_show(struct seq_file *m, void *v);
#endif /* _FRAME_GROUP_H */
