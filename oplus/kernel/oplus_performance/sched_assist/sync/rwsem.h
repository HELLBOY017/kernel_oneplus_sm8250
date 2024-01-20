/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SCHED_RWSEM_H_
#define _OPLUS_SCHED_RWSEM_H_

#include <linux/rwsem.h>

struct rwsem_waiter;
struct oplus_rw_semaphore {
	/* number of ux or rt tasks in wait list */
	atomic_long_t count;
};

static inline struct oplus_rw_semaphore *get_oplus_rw_semaphore(struct rw_semaphore *sem)
{
	return (struct oplus_rw_semaphore *)(sem->android_oem_data1);
};

#ifndef CONFIG_MTK_TASK_TURBO
#ifdef CONFIG_RWSEM_PRIO_AWARE
bool rwsem_list_add(struct task_struct *tsk, struct list_head *entry, struct list_head *head, struct rw_semaphore *sem);
#else /* CONFIG_RWSEM_PRIO_AWARE */
bool rwsem_list_add(struct task_struct *tsk, struct list_head *entry, struct list_head *head);
#endif /* CONFIG_RWSEM_PRIO_AWARE */
#endif /* CONFIG_MTK_TASK_TURBO */

void rwsem_set_inherit_ux(struct task_struct *tsk, struct task_struct *waiter_task,
		struct task_struct *owner, struct rw_semaphore *sem);
void rwsem_unset_inherit_ux(struct rw_semaphore *sem, struct task_struct *tsk);
#ifdef CONFIG_OPLUS_LOCKING_OSQ
void locking_vh_rwsem_opt_spin_start(struct rw_semaphore *sem, bool *time_out, int *cnt, bool chk_only);
void locking_vh_rwsem_opt_spin_finish(struct rw_semaphore *sem, bool taken, bool wlock);
void locking_vh_rwsem_can_spin_on_owner(struct rw_semaphore *sem, bool *ret, bool wlock);
#endif
void locking_vh_rwsem_read_wait_start(struct rw_semaphore *sem);
void locking_vh_rwsem_read_wait_finish(struct rw_semaphore *sem);
void locking_vh_rwsem_write_wait_start(struct rw_semaphore *sem);
void locking_vh_rwsem_write_wait_finish(struct rw_semaphore *sem);
void locking_vh_alter_rwsem_list_add(struct rwsem_waiter *waiter,
		struct rw_semaphore *sem, bool *already_on_list);
#endif /* _OPLUS_SCHED_RWSEM_H_ */
