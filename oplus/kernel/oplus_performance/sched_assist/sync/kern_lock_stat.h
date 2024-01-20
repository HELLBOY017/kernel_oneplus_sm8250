/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#ifndef _KERN_LOCK_STAT_H_
#define _KERN_LOCK_STAT_H_

/* #define INCLUDE_UNUSE */

enum lock_type {
#ifdef INCLUDE_UNUSE
	SPINLOCK,
#endif
	MUTEX,
	RWSEM_READ,
	RWSEM_WRITE,
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	OSQ_MUTEX,
	OSQ_RWSEM_READ,
	OSQ_RWSEM_WRITE,
#endif
	FUTEX_ART,
#ifdef INCLUDE_UNUSE
	FUTEX_JUC,
#endif

	LOCK_TYPES,
};


enum watermark {
	LOW_THRES,
	HIGH_THRES,
	FATAL_THRES,

	MAX_THRES,
};


enum group_type {
	GRP_UX,
	GRP_RT,
	GRP_OT,
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	GRP_BG,
	GRP_FG,
	GRP_TA,
#endif
	GRP_TYPES,
};

void handle_wait_stats(int type, u64 time);
void monitor_futex_wait_start(unsigned int flags, u32 bitset);
void monitor_futex_wait_end(unsigned int flags, u32 bitset);
void monitor_mutex_wait_start(struct mutex *lock);
void monitor_mutex_wait_finish(struct mutex *lock);
void monitor_rwsem_read_wait_start(struct rw_semaphore *sem);
void monitor_rwsem_read_wait_finish(struct rw_semaphore *sem);
void monitor_rwsem_write_wait_start(struct rw_semaphore *sem);
void monitor_rwsem_write_wait_finish(struct rw_semaphore *sem);
#endif /* _KERN_LOCK_STAT_H_ */
