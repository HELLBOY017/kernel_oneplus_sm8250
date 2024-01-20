/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SCHED_MUTEX_H_
#define _OPLUS_SCHED_MUTEX_H_

#include <linux/mutex.h>

struct oplus_mutex {
	/* number of ux or rt tasks in waiter list */
	atomic_long_t count;
};

static inline struct oplus_mutex * get_oplus_mutex(struct mutex *lock)
{
	return (struct oplus_mutex *)lock->android_oem_data1;
};

void mutex_list_add(struct task_struct *task, struct list_head *entry, struct list_head *head, struct mutex *lock);
void mutex_set_inherit_ux(struct mutex *lock, struct task_struct *task);
void mutex_unset_inherit_ux(struct mutex *lock, struct task_struct *task);
void locking_vh_alter_mutex_list_add(struct mutex *lock,
			struct mutex_waiter *waiter, struct list_head *list, bool *already_on_list);
void locking_vh_mutex_wait_start(struct mutex *lock);
void locking_vh_mutex_wait_finish(struct mutex *lock);
void locking_vh_mutex_unlock_slowpath(struct mutex *lock);
#ifdef CONFIG_OPLUS_LOCKING_OSQ
void locking_vh_mutex_opt_spin_start(struct mutex *lock, bool *time_out, int *cnt);
void locking_vh_mutex_opt_spin_finish(struct mutex *lock, bool taken);
void locking_vh_mutex_can_spin_on_owner(struct mutex *lock, int *retval);
#endif
#endif /* _OPLUS_SCHED_MUTEX_H_ */
