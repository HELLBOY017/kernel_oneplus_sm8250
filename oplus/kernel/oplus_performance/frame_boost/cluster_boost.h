// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */
#ifndef _CLUSTER_BOOST_H
#define _CLUSTER_BOOST_H
int fbg_set_task_preferred_cluster(void __user *uarg);
bool fbg_cluster_boost(struct task_struct *p, int *target_cpu);
#endif /* _CLUSTER_BOOST_H */
