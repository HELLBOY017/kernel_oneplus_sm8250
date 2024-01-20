// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#include <linux/version.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <trace/hooks/dtask.h>
#endif

#include <linux/sched/clock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/sched_assist/sched_assist_common.h>

#include "locking_main.h"

#ifdef CONFIG_OPLUS_LOCKING_OSQ
#include "rwsem.h"
#endif
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
#include "kern_lock_stat.h"
#endif

enum rwsem_waiter_type {
	RWSEM_WAITING_FOR_WRITE,
	RWSEM_WAITING_FOR_READ
};

struct rwsem_waiter {
	struct list_head list;
	struct task_struct *task;
	enum rwsem_waiter_type type;
};

#define RWSEM_READER_OWNED	((struct task_struct *)1UL)

static inline bool rwsem_owner_is_writer(struct task_struct *owner)
{
	return owner && owner != RWSEM_READER_OWNED;
}

static void rwsem_list_add_ux(struct list_head *entry, struct list_head *head)
{
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct rwsem_waiter *waiter = NULL;
	list_for_each_safe(pos, n, head) {
		waiter = list_entry(pos, struct rwsem_waiter, list);
#ifdef CONFIG_RWSEM_PRIO_AWARE
		if (!test_task_ux(waiter->task) && waiter->task->prio > MAX_RT_PRIO) {
#else
		if (!test_task_ux(waiter->task)) {
#endif
			list_add(entry, waiter->list.prev);
			return;
		}
	}
	if (pos == head) {
		list_add_tail(entry, head);
	}
}

#ifndef CONFIG_MTK_TASK_TURBO
#ifdef CONFIG_RWSEM_PRIO_AWARE
bool rwsem_list_add(struct task_struct *tsk, struct list_head *entry, struct list_head *head, struct rw_semaphore *sem)
{
	bool is_ux = test_task_ux(tsk);

	if (!entry || !head) {
		return false;
	}

	if (is_ux) {
		rwsem_list_add_ux(entry, head);
		sem->m_count++;
		return true;
	}

	return false;
}
#else /* CONFIG_RWSEM_PRIO_AWARE */
bool rwsem_list_add(struct task_struct *tsk, struct list_head *entry, struct list_head *head)
{
	bool is_ux = test_task_ux(tsk);

	if (!entry || !head) {
		return false;
	}

	if (is_ux) {
		rwsem_list_add_ux(entry, head);
		return true;
	}

	return false;
}
#endif /* CONFIG_RWSEM_PRIO_AWARE */
#endif /* CONFIG_MTK_TASK_TURBO */

void rwsem_set_inherit_ux(struct task_struct *tsk, struct task_struct *waiter_task, struct task_struct *owner, struct rw_semaphore *sem)
{
	bool is_ux = test_set_inherit_ux(tsk);
	if (waiter_task && is_ux) {
		if (rwsem_owner_is_writer(owner) && sem && !sem->ux_dep_task) {
			int type = get_ux_state_type(owner);
			if ((UX_STATE_NONE == type) || (UX_STATE_INHERIT == type)) {
				set_inherit_ux(owner, INHERIT_UX_RWSEM, tsk->ux_depth, tsk->ux_state);
				sem->ux_dep_task = owner;
			}
		}
	}
}

void rwsem_unset_inherit_ux(struct rw_semaphore *sem, struct task_struct *tsk)
{
	if (tsk && sem && sem->ux_dep_task == tsk) {
		unset_inherit_ux(tsk, INHERIT_UX_RWSEM);
		sem->ux_dep_task = NULL;
	}
}

#ifdef CONFIG_OPLUS_LOCKING_OSQ
static void rwsem_update_ux_cnt_when_add(struct rw_semaphore *sem)
{
	/* struct oplus_task_struct *ots = get_oplus_task_struct(current); */
	struct oplus_rw_semaphore *osem = get_oplus_rw_semaphore(sem);

	if (unlikely(!current))
		return;

	/* Record the ux flag when task is added to wait list */
	current->lkinfo.is_block_ux = (current->ux_state & 0xf) || (current->prio < MAX_RT_PRIO);
	if (current->lkinfo.is_block_ux)
		atomic_long_inc(&osem->count);
}

static void rwsem_update_ux_cnt_when_remove(struct rw_semaphore *sem)
{
	/* struct oplus_task_struct *ots = get_oplus_task_struct(current); */
	struct oplus_rw_semaphore *osem = get_oplus_rw_semaphore(sem);

	if (unlikely(!current))
		return;

	/*
	 * Update the ux tasks when tasks exit the waiter list.
	 * We only care about the recorded ux flag, but don't
	 * care about the current ux flag. This avoids the corruption
	 * of the number caused by ux change when in waiter list.
	 */
	if (current->lkinfo.is_block_ux) {
		/* Count == 0! Something is wrong! */
		WARN_ON_ONCE(!atomic_long_read(&osem->count));
		atomic_long_add(-1, &osem->count);
		current->lkinfo.is_block_ux = false;
	}
}
#endif

void locking_vh_alter_rwsem_list_add(struct rwsem_waiter *waiter,
			struct rw_semaphore *sem, bool *already_on_list)
{
#ifdef CONFIG_OPLUS_LOCKING_OSQ
	if (likely(locking_opt_enable(LK_OSQ_ENABLE)))
		rwsem_update_ux_cnt_when_add(sem);
#endif
}

void locking_vh_rwsem_read_wait_start(struct rw_semaphore *sem)
{
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
	monitor_rwsem_read_wait_start(sem);
#endif
}

void locking_vh_rwsem_read_wait_finish(struct rw_semaphore *sem)
{
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
	monitor_rwsem_read_wait_finish(sem);
#endif
#ifdef CONFIG_OPLUS_LOCKING_OSQ
	rwsem_update_ux_cnt_when_remove(sem);
#endif
}

void locking_vh_rwsem_write_wait_start(struct rw_semaphore *sem)
{
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
	monitor_rwsem_write_wait_start(sem);
#endif
}

void locking_vh_rwsem_write_wait_finish(struct rw_semaphore *sem)
{
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
	monitor_rwsem_write_wait_finish(sem);
#endif
#ifdef CONFIG_OPLUS_LOCKING_OSQ
	rwsem_update_ux_cnt_when_remove(sem);
#endif
}

#ifdef CONFIG_OPLUS_LOCKING_OSQ
static int rwsem_opt_spin_time_threshold = NSEC_PER_USEC * 15;
static int rwsem_ux_opt_spin_time_threshold = NSEC_PER_USEC * 40;
static int rwsem_opt_spin_total_cnt = 0;
static int rwsem_opt_spin_timeout_exit_cnt = 0;
module_param(rwsem_opt_spin_time_threshold, int, 0644);
module_param(rwsem_ux_opt_spin_time_threshold, int, 0644);
module_param(rwsem_opt_spin_total_cnt, int, 0444);
module_param(rwsem_opt_spin_timeout_exit_cnt, int, 0444);

void locking_vh_rwsem_opt_spin_start(struct rw_semaphore *sem, bool *time_out, int *cnt, bool chk_only)
{
	/* struct oplus_task_struct *ots = get_oplus_task_struct(current); */
	u64 delta;

	if (unlikely(IS_ERR_OR_NULL(current)))
		return;

	if (!current->lkinfo.opt_spin_start_time) {
		if (chk_only)
			return;

		rwsem_opt_spin_total_cnt++;
		current->lkinfo.opt_spin_start_time = sched_clock();
		return;
	}

	if (unlikely(!locking_opt_enable(LK_OSQ_ENABLE)))
		return;

	++(*cnt);
	/* Check whether time is out every 16 times in order to reduce the cost of calling sched_clock() */
	if ((*cnt) & 0xf)
		return;

	delta = sched_clock() - current->lkinfo.opt_spin_start_time;
	if (((current->ux_state & 0xf) && delta > rwsem_ux_opt_spin_time_threshold) ||
	    (!(current->ux_state & 0xf) && delta > rwsem_opt_spin_time_threshold)) {
		rwsem_opt_spin_timeout_exit_cnt++;
		*time_out = true;
	}
}

void locking_vh_rwsem_opt_spin_finish(struct rw_semaphore *sem, bool taken, bool wlock)
{
	/* struct oplus_task_struct *ots = get_oplus_task_struct(current); */
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	u64 delta;
#endif

	if (unlikely(IS_ERR_OR_NULL(current)))
		return;

	if (likely(current->lkinfo.opt_spin_start_time)) {
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
		delta = sched_clock() - current->lkinfo.opt_spin_start_time;
		if (wlock)
			handle_wait_stats(OSQ_RWSEM_WRITE, delta);
		else
			handle_wait_stats(OSQ_RWSEM_READ, delta);
#endif
		current->lkinfo.opt_spin_start_time = 0;
	}
}

#define MAX_VALID_COUNT (1000)
void locking_vh_rwsem_can_spin_on_owner(struct rw_semaphore *sem, bool *ret, bool wlock)
{
	/* struct oplus_task_struct *ots = get_oplus_task_struct(current); */
	struct oplus_rw_semaphore *osem = get_oplus_rw_semaphore(sem);
	long block_ux_cnt;

	if (unlikely(!locking_opt_enable(LK_OSQ_ENABLE)))
		return;

	/* NOTE:kernel-5.15, this will always be writer, should just go? */
	/* ux or rt task just go */
	if (wlock || (current->ux_state & 0xf) || (current->prio < MAX_RT_PRIO))
		return;

	block_ux_cnt = atomic_long_read(&osem->count);
	/* If some ux task is in the waiter list, non-ux can't optimistic spin. */
	if (block_ux_cnt > 0 && block_ux_cnt < MAX_VALID_COUNT)
		*ret = false;
}
#endif
