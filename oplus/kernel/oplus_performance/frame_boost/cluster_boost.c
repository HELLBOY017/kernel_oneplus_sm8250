// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/uaccess.h>
#include <../kernel/sched/sched.h>
#include "frame_ioctl.h"
#include "frame_group.h"

extern unsigned long cpu_util_without(int cpu, struct task_struct *p);

static DEFINE_RAW_SPINLOCK(preferred_cluster_id_lock);
/*
 * Only interested in these threads their tgid euqal to user_interested_tgid.
 * the purpose of adding this condition is to exclude these threads created
 * before FBG module init, the preferred_cluster_id of their oplus_task_struct is 0,
 * they will be preferred to run cluster 0 incorrectly.
 */
static atomic_t user_interested_tgid = ATOMIC_INIT(-1);

int fbg_set_task_preferred_cluster(void __user *uarg)
{
	struct ofb_ctrl_cluster data;
	struct task_struct *task = NULL;
	unsigned long flags;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&data, uarg, sizeof(data)))
		return -EFAULT;

	rcu_read_lock();
	task = find_task_by_vpid(data.tid);
	if (task)
		get_task_struct(task);
	rcu_read_unlock();

	if (task) {
		atomic_set(&user_interested_tgid, task->tgid);
		raw_spin_lock_irqsave(&preferred_cluster_id_lock, flags);
		if ((data.cluster_id >= 0) && (data.cluster_id < fbg_num_sched_clusters)) {
			task->preferred_cluster_id = data.cluster_id;
		} else {
			task->preferred_cluster_id = -1;
		}
		raw_spin_unlock_irqrestore(&preferred_cluster_id_lock, flags);
		put_task_struct(task);
	}

	return 0;
}
bool fbg_cluster_boost(struct task_struct *p, int *target_cpu)
{
	unsigned long flags;
	int preferred_cluster_id;
	struct oplus_sched_cluster *preferred_cluster;
	struct cpumask *preferred_cpus;
	cpumask_t search_cpus = CPU_MASK_NONE;
	int iter_cpu;
	int active_cpu = -1;
	int max_spare_cap_cpu = -1;
	unsigned long spare_cap = 0, max_spare_cap = 0;

	if (likely(atomic_read(&user_interested_tgid) != p->tgid))
		return false;

	raw_spin_lock_irqsave(&preferred_cluster_id_lock, flags);
	preferred_cluster_id = p->preferred_cluster_id;
	raw_spin_unlock_irqrestore(&preferred_cluster_id_lock, flags);

	if (preferred_cluster_id < 0 || preferred_cluster_id >= fbg_num_sched_clusters)
		return false;

	preferred_cluster = fb_cluster[preferred_cluster_id];
	preferred_cpus = &preferred_cluster->cpus;
	cpumask_and(&search_cpus, &p->cpus_mask, cpu_active_mask);
	cpumask_and(&search_cpus, &search_cpus, preferred_cpus);

	for_each_cpu(iter_cpu, &search_cpus) {
		if (active_cpu == -1)
			active_cpu = iter_cpu;

		if (available_idle_cpu(iter_cpu) || (iter_cpu == task_cpu(p) &&
			p->state == TASK_RUNNING)) {
			max_spare_cap_cpu = iter_cpu;
			break;
		}

		spare_cap = max_t(long, capacity_of(iter_cpu) - cpu_util_without(iter_cpu, p), 0);
		if (spare_cap > max_spare_cap) {
			max_spare_cap = spare_cap;
			max_spare_cap_cpu = iter_cpu;
		}
	}

	if (max_spare_cap_cpu == -1)
		max_spare_cap_cpu = active_cpu;

	if ((max_spare_cap_cpu == -1)
		|| ((cpumask_weight(&search_cpus) == 1) &&
		(!available_idle_cpu(max_spare_cap_cpu)))) {
		return false;
	}

	*target_cpu = max_spare_cap_cpu;

	return true;
}
