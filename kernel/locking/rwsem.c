// SPDX-License-Identifier: GPL-2.0
/* kernel/rwsem.c: R/W semaphores, public implementation
 *
 * Written by David Howells (dhowells@redhat.com).
 * Derived from asm-i386/semaphore.h
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/export.h>
#include <linux/rwsem.h>
#include <linux/atomic.h>

#if IS_ENABLED(CONFIG_OPLUS_LOCKING_STRATEGY)
#include <linux/sched_assist/sync/rwsem.h>
#endif
#include "rwsem.h"

#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
#include <linux/mm.h>
#include <linux/sched.h>
#include <../sched/sched.h>
#include <linux/sched/clock.h>
#endif
#ifdef CONFIG_LOCKING_PROTECT
#include <linux/sched_assist/sched_assist_locking.h>
#include <linux/sched.h>
#include <../sched/sched.h>
#endif

/*
 * lock for reading
 */
void __sched down_read(struct rw_semaphore *sem)
{
	might_sleep();
	rwsem_acquire_read(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_reader_owned(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(down_read);

int __sched down_read_killable(struct rw_semaphore *sem)
{
	might_sleep();
	rwsem_acquire_read(&sem->dep_map, 0, 0, _RET_IP_);

	if (LOCK_CONTENDED_RETURN(sem, __down_read_trylock, __down_read_killable)) {
		rwsem_release(&sem->dep_map, 1, _RET_IP_);
		return -EINTR;
	}

	rwsem_set_reader_owned(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif

	return 0;
}

EXPORT_SYMBOL(down_read_killable);

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
int down_read_trylock(struct rw_semaphore *sem)
{
	int ret = __down_read_trylock(sem);

	if (ret == 1) {
		rwsem_acquire_read(&sem->dep_map, 0, 1, _RET_IP_);
		rwsem_set_reader_owned(sem);
		#ifdef CONFIG_LOCKING_PROTECT
		record_locking_info(current, jiffies);
		#endif
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
		uxchain_rwsem_down(sem);
#endif
	}
	return ret;
}

EXPORT_SYMBOL(down_read_trylock);

/*
 * lock for writing
 */
void __sched down_write(struct rw_semaphore *sem)
{
	might_sleep();
	rwsem_acquire(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_owner(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(down_write);

/*
 * lock for writing
 */
int __sched down_write_killable(struct rw_semaphore *sem)
{
	might_sleep();
	rwsem_acquire(&sem->dep_map, 0, 0, _RET_IP_);

	if (LOCK_CONTENDED_RETURN(sem, __down_write_trylock, __down_write_killable)) {
		rwsem_release(&sem->dep_map, 1, _RET_IP_);
		return -EINTR;
	}

#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_owner(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
	return 0;
}

EXPORT_SYMBOL(down_write_killable);

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
int down_write_trylock(struct rw_semaphore *sem)
{
	int ret = __down_write_trylock(sem);

	if (ret == 1) {
		rwsem_acquire(&sem->dep_map, 0, 1, _RET_IP_);
		rwsem_set_owner(sem);
		#ifdef CONFIG_LOCKING_PROTECT
		record_locking_info(current, jiffies);
		#endif
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
		uxchain_rwsem_down(sem);
#endif
	}

	return ret;
}

EXPORT_SYMBOL(down_write_trylock);

/*
 * release a read lock
 */
void up_read(struct rw_semaphore *sem)
{
	rwsem_release(&sem->dep_map, 1, _RET_IP_);
	DEBUG_RWSEMS_WARN_ON(sem->owner != RWSEM_READER_OWNED);

	__up_read(sem);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, 0);
#endif
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_up(sem);
#endif
}

EXPORT_SYMBOL(up_read);

/*
 * release a write lock
 */
void up_write(struct rw_semaphore *sem)
{
	rwsem_release(&sem->dep_map, 1, _RET_IP_);
	DEBUG_RWSEMS_WARN_ON(sem->owner != current);

	rwsem_clear_owner(sem);
	__up_write(sem);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, 0);
#endif
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_up(sem);
#endif
}

EXPORT_SYMBOL(up_write);

/*
 * downgrade write lock to read lock
 */
void downgrade_write(struct rw_semaphore *sem)
{
	lock_downgrade(&sem->dep_map, _RET_IP_);
	DEBUG_RWSEMS_WARN_ON(sem->owner != current);

	rwsem_set_reader_owned(sem);
	__downgrade_write(sem);
}

EXPORT_SYMBOL(downgrade_write);

#ifdef CONFIG_DEBUG_LOCK_ALLOC

void down_read_nested(struct rw_semaphore *sem, int subclass)
{
	might_sleep();
	rwsem_acquire_read(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_reader_owned(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(down_read_nested);

void _down_write_nest_lock(struct rw_semaphore *sem, struct lockdep_map *nest)
{
	might_sleep();
	rwsem_acquire_nest(&sem->dep_map, 0, 0, nest, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_owner(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(_down_write_nest_lock);

void down_read_non_owner(struct rw_semaphore *sem)
{
	might_sleep();

	__down_read(sem);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_reader_owned(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(down_read_non_owner);

void down_write_nested(struct rw_semaphore *sem, int subclass)
{
	might_sleep();
	rwsem_acquire(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_owner(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
}

EXPORT_SYMBOL(down_write_nested);

int __sched down_write_killable_nested(struct rw_semaphore *sem, int subclass)
{
	might_sleep();
	rwsem_acquire(&sem->dep_map, subclass, 0, _RET_IP_);

	if (LOCK_CONTENDED_RETURN(sem, __down_write_trylock, __down_write_killable)) {
		rwsem_release(&sem->dep_map, 1, _RET_IP_);
		return -EINTR;
	}

#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, jiffies);
#endif
	rwsem_set_owner(sem);
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_down(sem);
#endif
	return 0;
}

EXPORT_SYMBOL(down_write_killable_nested);

void up_read_non_owner(struct rw_semaphore *sem)
{
	DEBUG_RWSEMS_WARN_ON(sem->owner != RWSEM_READER_OWNED);
	__up_read(sem);
#ifdef CONFIG_LOCKING_PROTECT
	record_locking_info(current, 0);
#endif
#ifdef OPLUS_FEATURE_SCHED_ASSIST
//#ifdef CONFIG_UXCHAIN_V2
	uxchain_rwsem_up(sem);
#endif
}

EXPORT_SYMBOL(up_read_non_owner);

#endif
