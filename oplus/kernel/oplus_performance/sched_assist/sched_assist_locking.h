/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#ifndef _OPLUS_LOCKING_ASSIST_H_
#define _OPLUS_LOCKING_ASSIST_H_

void locking_init_rq_data(struct rq *rq);
void pick_locking_thread(struct rq *rq, struct task_struct **p,
					struct sched_entity **se);
void dequeue_locking_thread(struct rq *rq, struct task_struct *p);
void enqueue_locking_thread(struct rq *rq, struct task_struct *p);

void record_locking_info(struct task_struct *p, unsigned long settime);
void clear_locking_info(struct task_struct *p);
bool task_inlock(struct task_struct *p);
bool locking_protect_outtime(struct task_struct *p);
bool task_skip_protect(struct task_struct *p);
#endif /* _OPLUS_LOCKING_ASSIST_H_  */
