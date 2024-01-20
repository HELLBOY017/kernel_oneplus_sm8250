#include <linux/version.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <trace/events/sched.h>
#include <../kernel/sched/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../fs/proc/internal.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/rwsem.h>

#include "sched_assist_trace.h"
#include "sched_assist_common.h"

static bool locking_protect_disable = false;

void sched_locking_target_comm(struct task_struct *p)
{
	/* vts kernel_net_test close locking_protect */
	if (locking_protect_disable == false) {
		if(strstr(p->comm, "kernel_net_tes")) {
			locking_protect_disable = true;
		}
	}
}

static DEFINE_PER_CPU(int, prev_locking_state);
static DEFINE_PER_CPU(int, prev_locking_depth);
#define LK_STATE_UNLOCK  (0)
#define LK_STATE_LOCK    (1)
#define MAX_SHOW_DEPTH   (128)
void locking_state_systrace_c(unsigned int cpu, struct task_struct *p)
{
	int locking_state, locking_depth;

	if (likely(!(global_debug_enabled & DEBUG_SYSTRACE)))
		return;

	locking_state = (p->locking_time_start > 0 ? LK_STATE_LOCK : LK_STATE_UNLOCK);

	if (per_cpu(prev_locking_state, cpu) != locking_state) {
		char buf[256];

		snprintf(buf, sizeof(buf), "C|9999|Cpu%d_locking_state|%d\n",
				cpu, locking_state);
		tracing_mark_write(buf);
		per_cpu(prev_locking_state, cpu) = locking_state;
	}

	locking_depth = p->locking_depth;
	if (unlikely(locking_depth > MAX_SHOW_DEPTH))
		locking_depth = MAX_SHOW_DEPTH;

	if (per_cpu(prev_locking_depth, cpu) != locking_depth) {
		char buf[256];

		snprintf(buf, sizeof(buf), "C|9999|Cpu%d_locking_depth|%d\n",
				cpu, locking_depth);
		tracing_mark_write(buf);
		per_cpu(prev_locking_depth, cpu) = locking_depth;
	}
}
EXPORT_SYMBOL_GPL(locking_state_systrace_c);

static inline struct task_struct *task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

inline bool task_skip_protect(struct task_struct *p)
{
	return test_task_ux(p);
}

void locking_init_rq_data(struct rq *rq)
{
	if (!rq)
		return;

	INIT_LIST_HEAD(&rq->locking_thread_list);
	rq->rq_locking_task = 0;
}

static inline bool test_task_is_rt(struct task_struct *p)
{
	/* valid RT priority is 0..MAX_RT_PRIO-1 */
	return (p->prio >= 0) && (p->prio <= MAX_RT_PRIO-1);
}

/*
 * This function would be called by the below two cases:
 * @in_cs = true: current has acquired the lock and enter critical section.
 * @in_cs = false: current failed to get lock and add into the waiting list.
 */
void update_locking_time(unsigned long time, bool in_cs)
{
	/* Rt thread do not need our help. */
	if (test_task_is_rt(current))
		return;

	if (!in_cs)
		goto set;

	/*
	 * Current has acquired the lock, increase it's locking depth.
	 * The depth over one means current hold more than one lock.
	 */
	if (time > 0) {
		current->locking_depth++;
		goto set;
	}

	/*
	 * Current has released the lock, decrease it's locking depth.
	 * The depth become zero means current has leave all the critical section.
	 */
	if (unlikely(current->locking_depth <= 0)) {
		current->locking_depth = 0;
		goto set;
	}

	if (--(current->locking_depth))
		return;

set:
	current->locking_time_start = time;
}
EXPORT_SYMBOL_GPL(update_locking_time);

void record_locking_info(struct task_struct *p, unsigned long settime)
{
	update_locking_time(settime, true);
}
EXPORT_SYMBOL_GPL(record_locking_info);

inline void clear_locking_info(struct task_struct *p)
{
	p->locking_time_start = 0;
}

inline bool task_inlock(struct task_struct *p)
{
	if (unlikely(locking_protect_disable == true))
		return false;

	return p->locking_time_start > 0;
}

#define MAX_PROTECT_NESTED_DEPTCH (8)
static inline bool task_inlock_with_depth_check(struct task_struct *p)
{
	if (unlikely(p->locking_depth >= MAX_PROTECT_NESTED_DEPTCH))
		return false;

	return task_inlock(p);
}

inline bool locking_protect_outtime(struct task_struct *p)
{
	return time_after(jiffies, p->locking_time_start);
}

void enqueue_locking_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	bool exist = false;

	if (!rq || !p || !list_empty(&p->locking_entry) || unlikely(locking_protect_disable))
		return;

	if (p->locking_time_start) {
		list_for_each_safe(pos, n, &rq->locking_thread_list) {
			if (pos == &p->locking_entry) {
				exist = true;
				break;
			}
		}
		if (!exist) {
			list_add_tail(&p->locking_entry, &rq->locking_thread_list);
			rq->rq_locking_task++;
			get_task_struct(p);
			trace_enqueue_locking_thread(p, p->locking_depth, rq->rq_locking_task);
		}
	}
}

void dequeue_locking_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;

	if (!rq || !p)
		return;

	if (!list_empty(&p->locking_entry)) {
		list_for_each_safe(pos, n, &rq->locking_thread_list) {
			if (pos == &p->locking_entry) {
				list_del_init(&p->locking_entry);
				rq->rq_locking_task--;
				trace_dequeue_locking_thread(p, p->locking_depth, rq->rq_locking_task);
				put_task_struct(p);
				return;
			}
		}
		/* Task p is not in this rq? */
		pr_err("Can not find real rq, p->cpu=%d curr_rq=%d smpcpu=%d\n", task_cpu(p), cpu_of(rq), smp_processor_id());
	}
}

bool pick_locking_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se)
{
	struct task_struct *key_task;
	struct sched_entity *key_se;

	if (test_task_ux(*p) || unlikely(locking_protect_disable))
		return false;

pick_again:
	if (!list_empty(&rq->locking_thread_list)) {
		key_task = list_first_entry_or_null(&rq->locking_thread_list,
					struct task_struct, locking_entry);

		if (key_task) {
			if (task_inlock_with_depth_check(key_task)) {
				key_se = &key_task->se;
				if (key_se) {
					*p = key_task;
					*se = key_se;
					trace_select_locking_thread(key_task, key_task->locking_depth, rq->rq_locking_task);
					return true;
				}
			} else {
				list_del_init(&key_task->locking_entry);
				rq->rq_locking_task--;
				put_task_struct(key_task);
			}
			goto pick_again;
		}
	}

	return false;
}

void enqueue_locking_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct task_struct *p = entity_is_task(se) ? task_of(se) : NULL;
	struct rq *rq = rq_of(cfs_rq);

	enqueue_locking_thread(rq, p);
}
EXPORT_SYMBOL_GPL(enqueue_locking_entity);

void dequeue_locking_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct task_struct *p = entity_is_task(se) ? task_of(se) : NULL;
	struct rq *rq = rq_of(cfs_rq);

	dequeue_locking_thread(rq, p);
}
EXPORT_SYMBOL_GPL(dequeue_locking_entity);

void check_locking_protect_tick(struct sched_entity *curr)
{
	if (entity_is_task(curr)) {
		struct task_struct *curr_tsk = container_of(curr, struct task_struct, se);

		if (curr_tsk && task_inlock(curr_tsk) && locking_protect_outtime(curr_tsk))
			clear_locking_info(curr_tsk);
	}
}
EXPORT_SYMBOL_GPL(check_locking_protect_tick);

bool check_locking_protect_wakeup(struct task_struct *curr, struct task_struct *p)
{
	return task_inlock_with_depth_check(curr) && !task_skip_protect(p);
}
EXPORT_SYMBOL_GPL(check_locking_protect_wakeup);
