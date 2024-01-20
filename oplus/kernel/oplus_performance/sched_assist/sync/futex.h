/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SCHED_FUTEX_H_
#define _OPLUS_SCHED_FUTEX_H_

#include <linux/futex.h>

struct futex_uinfo {
	u32 cmd;
	u32 owner_tid;
	u32 type;
	u64 inform_user;
};

void locking_vh_do_futex(int cmd, unsigned int *flags, u32 __user *uaddr2);
void locking_vh_futex_wait_start(unsigned int flags, u32 bitset);
void locking_vh_futex_wait_end(unsigned int flags, u32 bitset);
void locking_vh_alter_futex_plist_add(struct plist_node *node, struct plist_head *head, bool *already_on_hb);
void locking_vh_futex_sleep_start(struct task_struct *p);
void locking_vh_futex_wake_traverse_plist(struct plist_head *chain, int *target_nr, union futex_key key, u32 bitset);
void locking_vh_futex_wake_this(int ret, int nr_wake, int target_nr, struct task_struct *p);
void locking_vh_futex_wake_up_q_finish(int nr_wake, int target_nr);

#endif /* _OPLUS_SCHED_FUTEX_H_ */
