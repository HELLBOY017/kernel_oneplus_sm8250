/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SCHED_COMMON_H_
#define _OPLUS_SCHED_COMMON_H_

#define ux_err(fmt, ...) \
		printk_deferred(KERN_ERR "[SCHED_ASSIST_ERR][%s]"fmt, __func__, ##__VA_ARGS__)
#define ux_warn(fmt, ...) \
		printk_deferred(KERN_WARNING "[SCHED_ASSIST_WARN][%s]"fmt, __func__, ##__VA_ARGS__)
#define ux_debug(fmt, ...) \
		printk_deferred(KERN_INFO "[SCHED_ASSIST_INFO][%s]"fmt, __func__, ##__VA_ARGS__)

#include <linux/version.h>

#define UX_MSG_LEN 64
#define UX_DEPTH_MAX 5

/* define for debug */
#define DEBUG_SYSTRACE (1 << 0)

/* define for sched assist thread type, keep same as the define in java file */
#define SA_OPT_CLEAR     (0)
#define SA_TYPE_LIGHT    (1 << 0)
#define SA_TYPE_HEAVY    (1 << 1)
#define SA_TYPE_ANIMATOR (1 << 2)
#define SA_TYPE_LISTPICK (1 << 3)
#define SA_OPT_SET       (1 << 7)
#define SA_TYPE_INHERIT  (1 << 8)
#define SA_TYPE_ONCE_UX  (1 << 9)
#define SA_TYPE_ID_CAMERA_PROVIDER  (1 << 10)
#define SA_TYPE_ID_ALLOCATOR_SER    (1 << 11)



#define SCHED_ASSIST_UX_MASK (0xFF)

/* define for sched assist scene type, keep same as the define in java file */
#define SA_SCENE_OPT_CLEAR  (0)
#define SA_LAUNCH           (1 << 0)
#define SA_SLIDE            (1 << 1)
#define SA_CAMERA           (1 << 2)
#define SA_ANIM_START       (1 << 3) /* we care about both launcher and top app */
#define SA_ANIM             (1 << 4) /* we only care about launcher */
#define SA_INPUT            (1 << 5)
#define SA_LAUNCHER_SI      (1 << 6)
#define SA_SCENE_OPT_SET    (1 << 7)
#define SA_GPU_COMPOSITION  (1 << 8)
#define SA_AUDIO            (1 << 9)

#define SF_GROUP_COUNT 2
struct ux_util_record{
        char val[64];
        u64 ux_load;
        int util;
};
extern struct ux_util_record sf_target[SF_GROUP_COUNT];
extern bool task_is_sf_group(struct task_struct *tsk);

#ifdef CONFIG_CGROUP_SCHED
#define SA_CGROUP_DEFAULT			(1)
#define SA_CGROUP_FOREGROUND			(2)
#define SA_CGROUP_BACKGROUND			(3)
#define SA_CGROUP_TOP_APP			(4)
#endif

/* define for load balance task type */
#define SA_HIGH_LOAD  	1
#define SA_LOW_LOAD  	0
#define SA_UX 	1
#define SA_TOP 	2
#define SA_FG 	3
#define SA_BG 	4

/* define for special priority */
#define ANDROID_PRIORITY_URGENT_AUDIO 101
#define ANDROID_PRIORITY_AUDIO 104

#define  FIRST_APPLICATION_UID  10000
#define  LAST_APPLICATION_UID   19999

#ifdef CONFIG_OPLUS_FEATURE_SCHED_SPREAD
DECLARE_PER_CPU(struct task_count_rq, task_lb_count);
#endif

enum UX_STATE_TYPE
{
	UX_STATE_INVALID = 0,
	UX_STATE_NONE,
	UX_STATE_SCHED_ASSIST,
	UX_STATE_INHERIT,
	MAX_UX_STATE_TYPE,
};

enum INHERIT_UX_TYPE
{
	INHERIT_UX_BINDER = 0,
	INHERIT_UX_RWSEM,
	INHERIT_UX_MUTEX,
	INHERIT_UX_SEM,
	INHERIT_UX_FUTEX,
	INHERIT_UX_MAX,
};
enum ANMATION_TYPE
{
	ANNIMATION_END = 0,
	APP_START_ANIMATION,
	APP_EXIT_ANIMATION,
	ULIMIT_PROCESS,
	LAUNCHER_SI_START,
        BACKUP,
	SYSTEMUI_SPLIT_STARTM,
};

#ifdef CONFIG_OPLUS_UX_IM_FLAG
/*
 * new flag should be add before MAX_IM_FLAG_TYPE,
 * never change the value of those existed flag type.
 */
enum IM_FLAG_TYPE {
	IM_FLAG_NONE = 0,
	IM_FLAG_SURFACEFLINGER,
	IM_FLAG_HWC,
	IM_FLAG_RENDERENGINE,
	IM_FLAG_WEBVIEW,
	IM_FLAG_CAMERA_HAL,
	IM_FLAG_AUDIO,
	IM_FLAG_HWBINDER,
	IM_FLAG_LAUNCHER,
	IM_FLAG_LAUNCHER_NON_UX_RENDER,
	IM_FLAG_SS_LOCK_OWNER,
	IM_FLAG_CAMERA_SERVER,
	IM_FLAG_SYSTEMSERVER_PID,
	IM_FLAG_FORBID_SET_CPU_AFFINITY_IN_KERNEL,
	IM_FLAG_MIDASD,
	MAX_IM_FLAG_TYPE,
	/* NOTE:
		IM_FLAG_FORBID_SET_CPU_AFFINITY must be the same value(11) as defined in TpdManager.java
		And it is duplicate with IM_FLAG_CAMERA_SERVER in kernel 5.4.
		Play a trick here, Because camera server has a system uid 1047,
		if receive IM_FLAG_FORBID_SET_CPU_AFFINITY(IM_FLAG_CAMERA_SERVER) for application task,
		reset im_flag value to IM_FLAG_FORBID_SET_CPU_AFFINITY_IN_KERNEL.
	*/
	IM_FLAG_FORBID_SET_CPU_AFFINITY = 11, /* forbid setting cpu affinity from app */
};
#endif

struct ux_sched_cluster {
        struct cpumask cpus;
        unsigned long capacity;
};

struct ux_sched_cputopo {
        int cls_nr;
        struct ux_sched_cluster sched_cls[NR_CPUS];
};

#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
extern unsigned int walt_scale_demand_divisor;
#else
extern unsigned int walt_ravg_window;
#define walt_scale_demand_divisor  (walt_ravg_window >> SCHED_CAPACITY_SHIFT)
#endif
#define scale_demand(d) ((d)/walt_scale_demand_divisor)

#ifdef CONFIG_OPLUS_FEATURE_SCHED_SPREAD
struct task_count_rq{
	int ux_high;
	int ux_low;
	int top_high;
	int top_low;
	int foreground_high;
	int foreground_low;
	int background_high;
	int background_low;
};

enum OPLUS_LB_TYPE
{
	OPLUS_LB_UX = 1,
	OPLUS_LB_TOP,
	OPLUS_LB_FG,
	OPLUS_LB_BG,
	OPLUS_LB_MAX,
};
#endif

extern int global_debug_enabled;

struct rq;
extern int sysctl_input_boost_enabled;
extern int sysctl_animation_type;
extern int sysctl_input_boost_enabled;
extern int sysctl_sched_assist_ib_duration_coedecay;
extern u64 sched_assist_input_boost_duration;
extern int ux_prefer_cpu[];
noinline int tracing_mark_write(const char *buf);
void ux_init_rq_data(struct rq *rq);
void ux_init_cpu_data(void);
#ifdef CONFIG_OPLUS_FEATURE_SCHED_SPREAD
void init_rq_cpu(int cpu);
#endif

bool test_list_pick_ux(struct task_struct *task);
void enqueue_ux_thread(struct rq *rq, struct task_struct *p);
void dequeue_ux_thread(struct rq *rq, struct task_struct *p);
void pick_ux_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se);

void inherit_ux_dequeue(struct task_struct *task, int type);
void inherit_ux_dequeue_refs(struct task_struct *task, int type, int value);
void inherit_ux_enqueue(struct task_struct *task, int type, int depth);
void inherit_ux_inc(struct task_struct *task, int type);
void inherit_ux_sub(struct task_struct *task, int type, int value);

void set_inherit_ux(struct task_struct *task, int type, int depth, int inherit_val);
void reset_inherit_ux(struct task_struct *inherit_task, struct task_struct *ux_task, int reset_type);
void unset_inherit_ux(struct task_struct *task, int type);
void unset_inherit_ux_value(struct task_struct *task, int type, int value);
void inc_inherit_ux_refs(struct task_struct *task, int type);

bool test_task_ux(struct task_struct *task);
bool test_task_ux_depth(int ux_depth);
bool test_inherit_ux(struct task_struct *task, int type);
bool test_set_inherit_ux(struct task_struct *task);
int get_ux_state_type(struct task_struct *task);

bool test_ux_task_cpu(int cpu);
bool test_ux_prefer_cpu(struct task_struct *task, int cpu);
void find_ux_task_cpu(struct task_struct *task, int *target_cpu);
void oplus_boost_kill_signal(int sig, struct task_struct *cur, struct task_struct *p);
static inline void find_slide_boost_task_cpu(struct task_struct *task, int *target_cpu) {}

bool oplus_sched_ban_setaffinity(struct task_struct *task, const struct cpumask *new_mask);

void sched_assist_systrace_pid(pid_t pid, int val, const char *fmt, ...);
#define SA_SYSTRACE_MAGIC 123
#define sched_assist_systrace(...)  sched_assist_systrace_pid(SA_SYSTRACE_MAGIC, __VA_ARGS__)

bool should_force_adjust_vruntime(struct sched_entity *se);
void place_entity_adjust_ux_task(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial);
bool should_ux_task_skip_further_check(struct sched_entity *se);
bool should_ux_preempt_wakeup(struct task_struct *wake_task, struct task_struct *curr_task);
bool should_ux_task_skip_cpu(struct task_struct *task, unsigned int cpu);
void set_ux_task_to_prefer_cpu(struct task_struct *task, int *orig_target_cpu);
int set_ux_task_cpu_common_by_prio(struct task_struct *task, int *target_cpu, bool boost, bool prefer_idle, unsigned int type);
bool ux_skip_sync_wakeup(struct task_struct *task, int *sync);
void set_ux_task_to_prefer_cpu_v1(struct task_struct *task, int *orig_target_cpu, bool *cond);
bool im_mali(struct task_struct *p);
bool cgroup_check_set_sched_assist_boost(struct task_struct *p);
int get_st_group_id(struct task_struct *task);
void cgroup_set_sched_assist_boost_task(struct task_struct *p);
#ifdef CONFIG_OPLUS_FEATURE_SCHED_SPREAD
void inc_ld_stats(struct task_struct *tsk, struct rq *rq);
void dec_ld_stats(struct task_struct *tsk, struct rq *rq);
void update_load_flag(struct task_struct *tsk, struct rq *rq);

int task_lb_sched_type(struct task_struct *tsk);
unsigned long reweight_cgroup_task(u64 slice, struct sched_entity *se, unsigned long task_weight, struct load_weight *lw);
#if !defined(CONFIG_OPLUS_SYSTEM_KERNEL_QCOM) || (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
void sched_assist_spread_tasks(struct task_struct *p, cpumask_t new_allowed_cpus,
		int start_cpu, int skip_cpu, cpumask_t *cpus, bool strict);
#else
void sched_assist_spread_tasks(struct task_struct *p, cpumask_t new_allowed_cpus,
		int order_index, int end_index, int skip_cpu, cpumask_t *cpus, bool strict);
#endif
bool should_force_spread_tasks(void);
u64 sa_calc_delta(struct sched_entity *se, u64 delta_exec, unsigned long weight, struct load_weight *lw, bool calc_fair);
void update_rq_nr_imbalance(int cpu);
#endif /* CONFIG_OPLUS_FEATURE_SCHED_SPREAD */

#ifdef CONFIG_OPLUS_FEATURE_AUDIO_OPT
void update_sa_task_stats(struct task_struct *tsk, u64 delta_ns, int stats_type);
void sched_assist_im_systrace_c(struct task_struct *tsk, int tst_type);
void sched_assist_update_record(struct task_struct *p, u64 delta_ns, int stats_type);
bool sched_assist_pick_next_entity(struct cfs_rq *cfs_rq, struct sched_entity **se);
void sched_assist_im_systrace_c(struct task_struct *tsk, int tst_type);
#endif

bool test_task_identify_ux(struct task_struct *task, int id_type_ux);
#ifdef CONFIG_SCHED_WALT
bool sched_assist_task_misfit(struct task_struct *task, int cpu, int flag);
#else
static inline bool sched_assist_task_misfit(struct task_struct *task, int cpu, int flag) { return false; }
#endif
static inline bool is_heavy_ux_task(struct task_struct *task)
{
	return task->ux_state & SA_TYPE_HEAVY;
}

static inline void set_once_ux(struct task_struct *task)
{
	task->ux_state |= SA_TYPE_ONCE_UX;
}

static inline void set_heavy_ux(struct task_struct *task)
{
	task->ux_state |= SA_TYPE_HEAVY;
}

static inline bool sched_assist_scene(unsigned int scene)
{
	if (unlikely(!sysctl_sched_assist_enabled))
		return false;

	switch (scene) {
	case SA_LAUNCH:
		return sysctl_sched_assist_scene & SA_LAUNCH;
	case SA_SLIDE:
		return sysctl_slide_boost_enabled;
	case SA_INPUT:
		return sysctl_input_boost_enabled;
	case SA_CAMERA:
		return sysctl_sched_assist_scene & SA_CAMERA;
	case SA_ANIM:
		return sysctl_sched_assist_scene & SA_ANIM;
	case SA_GPU_COMPOSITION:
		return sysctl_sched_assist_scene & SA_GPU_COMPOSITION;
	case SA_AUDIO:
		return sysctl_sched_assist_scene & SA_AUDIO;
	default:
		return false;
	}
}

#ifdef CONFIG_OPLUS_FEATURE_SCHED_SPREAD
static inline bool is_spread_task_enabled(void)
{
	return (sysctl_sched_assist_enabled >= 2) && !sched_assist_scene(SA_CAMERA);
}
static inline bool task_demand_ignore_wait_time(int event)
{
	return is_spread_task_enabled() && (event == PICK_NEXT_TASK);
}
#endif /* CONFIG_OPLUS_FEATURE_SCHED_SPREAD */

#ifdef CONFIG_OPLUS_FEATURE_AUDIO_OPT
static inline bool is_audio_task(struct task_struct *task)
{
	return (task->prio == ANDROID_PRIORITY_URGENT_AUDIO) || (task->prio == ANDROID_PRIORITY_AUDIO);
}

static inline bool is_small_task(struct task_struct *task)
{
	return task->oplus_task_info.im_small;
}
#endif /* CONFIG_OPLUS_FEATURE_AUDIO_OPT */


/* port ux prority from kernel 5.10 to 5.4 */
#define SA_OPT_SET_PRIORITY		(1 << 9)

#define SCHED_ASSIST_UX_PRIORITY_MASK	(0xFF000000)
#define SCHED_ASSIST_UX_PRIORITY_SHIFT	24

#define UX_PRIORITY_TOP_APP		0x0A000000
#define UX_PRIORITY_AUDIO		0x0A000000

#define UX_EXEC_SLICE (4000000U)
#define POSSIBLE_UX_MASK (SA_TYPE_LIGHT|SA_TYPE_HEAVY|SA_TYPE_ANIMATOR|SA_TYPE_LISTPICK|SA_TYPE_ONCE_UX)

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_SCHED_UX_PRIORITY)

void ux_priority_systrace_c(unsigned int cpu, struct task_struct *p);
unsigned int ux_task_exec_limit(struct task_struct *p);
void oplus_set_ux_state_lock(struct task_struct *t, int ux_state, bool need_lock_rq);
void enqueue_ux_thread_to_list(struct rq *rq, struct task_struct *p);
void dequeue_ux_thread_from_list(struct rq *rq, struct task_struct *p);
void android_vh_scheduler_tick_handler(struct rq *rq);
void oplus_check_preempt_wakeup_in_list(struct rq *rq, struct task_struct *wake_task, struct task_struct *curr_task, bool *preempt, bool *nopreempt);
void android_rvh_replace_next_task_fair_handler(struct rq *rq, struct task_struct **p, struct sched_entity **se, bool *repick, bool simple);

#else

static inline void oplus_set_ux_state_lock(struct task_struct *t, int ux_state, bool need_lock_rq)
{
	t->ux_state = ux_state;
}

#endif /* IS_ENABLED(CONFIG_OPLUS_FEATURE_SCHED_UX_PRIORITY) */
#endif /* _OPLUS_SCHED_COMMON_H_ */
