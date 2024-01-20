// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2030 Oplus. All rights reserved.
 * Description : assist driver for sigkill diagnosis, help userspace athena to
 * record kill reason category for sigkill
 * Version : 1.0
 */
#ifndef _OPLUS_SIGKILL_DIAGNOSIS_H_
#define _OPLUS_SIGKILL_DIAGNOSIS_H_

#include <linux/sched.h>


extern void record_sigkill_reason(void *unused, int sig, struct task_struct *curr, struct task_struct *p);

#endif /* _OPLUS_SIGKILL_DIAGNOSIS_H_ */
