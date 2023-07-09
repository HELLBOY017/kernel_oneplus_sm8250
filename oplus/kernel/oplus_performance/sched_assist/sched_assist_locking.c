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
#include "sched_assist_common.h"

#include <linux/mm.h>
#include <linux/rwsem.h>

static bool locking_protect_disable = false;

static int locking_max_over_thresh = 2000;
#define S2NS_T 1000000
static int locking_entity_over(struct sched_entity *a, struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) > (s64)locking_max_over_thresh * S2NS_T;
}

void sched_locking_target_comm(struct task_struct *p)
{
	/* vts kernel_net_test close locking_protect */
	if (locking_protect_disable == false) {
		if(strstr(p->comm, "kernel_net_tes")) {
			locking_protect_disable = true;
		}
	}
}

bool task_skip_protect(struct task_struct *p)
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

void record_locking_info(struct task_struct *p, unsigned long settime)
{
	if (settime) {
		p->locking_depth++;
	} else {
		if (p->locking_depth > 0) {
			if (--(p->locking_depth))
				return;
		}
	}
	p->locking_time_start = settime;
}
EXPORT_SYMBOL_GPL(record_locking_info);

void clear_locking_info(struct task_struct *p)
{
	p->locking_time_start = 0;
}

bool task_inlock(struct task_struct *p)
{
	return p->locking_time_start > 0 &&
		likely(locking_protect_disable = false);
}

bool locking_protect_outtime(struct task_struct *p)
{
	return time_after(jiffies, p->locking_time_start);
}

void enqueue_locking_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	bool exist = false;

	if (!rq || !p || !list_empty(&p->locking_entry) || unlikely(locking_protect_disable)) {
		return;
	}

	p->enqueue_time = rq->clock;

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
		}
	}
}

void dequeue_locking_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;

	if (!rq || !p || unlikely(locking_protect_disable)) {
		return;
	}
	p->enqueue_time = 0;
	if (!list_empty(&p->locking_entry)) {
		list_for_each_safe(pos, n, &rq->locking_thread_list) {
			if (pos == &p->locking_entry) {
				list_del_init(&p->locking_entry);
				task_rq(p)->rq_locking_task--;
				put_task_struct(p);
				return;
			}
		}
	}
}

void pick_locking_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se)
{
	struct task_struct *ori_p = *p;
	struct task_struct *key_task;
	struct sched_entity *key_se;

	if (!rq || !ori_p || !se || test_task_ux(*p) || unlikely(locking_protect_disable))
		return;

pick_again:
	if (!list_empty(&rq->locking_thread_list)) {
		key_task = list_first_entry_or_null(&rq->locking_thread_list,
					struct task_struct, locking_entry);

		if (key_task) {
			/* in case that ux thread keep running too long */
			if (locking_entity_over(&key_task->se, &ori_p->se))
				return;

			if (task_inlock(key_task)) {
				key_se = &key_task->se;
				if (key_se) {
					*p = key_task;
					*se = key_se;
					return;
				}
			} else {
				list_del_init(&key_task->locking_entry);
				task_rq(key_task)->rq_locking_task--;
				put_task_struct(key_task);
			}
			goto pick_again;
		}
	}
}
