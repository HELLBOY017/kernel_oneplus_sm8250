// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <../kernel/sched/sched.h>

#include "sched_assist_common.h"

#define AUDIO_RT_PULL_THRESHOLD_NS 250000
#define AUDIO_TASK_IDLE_EXIT_LATENCY 60

int sa_audio_perf_enable = 1;
int sa_audio_perf_status = 0;
int sa_audio_debug_enable = 0;

u64 perf_timer_slack_ns = 50000;
struct proc_dir_entry *audio_dir;
struct proc_dir_entry *audio_dir_parent;
struct list_head *debug_pids = NULL;
DEFINE_MUTEX(debug_pids_mutex);

void oplus_sched_assist_audio_latency_sensitive(struct task_struct *t, bool *latency_sensitive);
int oplus_sched_assist_audio_proc_init(struct proc_dir_entry *dir);
void oplus_sched_assist_audio_proc_remove(struct proc_dir_entry *dir);
void oplus_sched_assist_audio_perf_set_status(int status);
void oplus_sched_assist_audio_time_slack(struct task_struct *task);
bool oplus_sched_assist_audio_idle_balance(struct rq *this_rq);
void oplus_sched_assist_audio_perf_addIm(struct task_struct *task, int im_flag);
void oplus_sched_assist_audio_enqueue_hook(struct task_struct *task);

extern int tracing_mark_write(const char *buf);

static void debug_systrace_c(u64 value, const char *tag)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "C|9999|Ux_audio_%s|%llu\n", tag, value);
	tracing_mark_write(buf);
}

static inline void debug_log(struct task_struct *task, u64 value, const char *tag)
{
	if (!task)
		return;

	if (unlikely(global_debug_enabled & DEBUG_SYSTRACE))
		debug_systrace_c(task->pid, tag);
}

static bool is_audio_perf_enable()
{
	if (unlikely(!sysctl_sched_assist_enabled))
		return false;

	if (unlikely(!sa_audio_perf_enable))
		return false;

	return true;
}

static bool is_audio_perf_status_on()
{
	return is_audio_perf_enable() && sa_audio_perf_status;
}

static bool is_audio_flaged(struct task_struct *t)
{
	if (!t)
		return false;
	if (t->ux_im_flag != IM_FLAG_AUDIO)
		return false;

	return true;
}

struct pid_node {
	int pid;
	struct list_head list;
};

static void add_pid(int pid)
{
	struct pid_node *pnode, *pos;

	mutex_lock(&debug_pids_mutex);
	list_for_each_entry(pos, debug_pids, list) {
		if (pos->pid == pid) {
			mutex_unlock(&debug_pids_mutex);
			return;
		}
	}

	pnode = kzalloc(sizeof(struct pid_node), GFP_KERNEL);
	if (!pnode) {
		ux_err("OOM when allocating audio pid node\n");
		mutex_unlock(&debug_pids_mutex);
		return;
	}
	pnode->pid = pid;
	INIT_LIST_HEAD(&pnode->list);
	list_add_tail(&pnode->list, debug_pids);
	mutex_unlock(&debug_pids_mutex);
}

static void remove_pid(int pid)
{
	struct pid_node *pos, *tmp;

	mutex_lock(&debug_pids_mutex);
	list_for_each_entry_safe(pos, tmp, debug_pids, list) {
		if (pid == pos->pid) {
			list_del(&pos->list);
			kfree(pos);
			mutex_unlock(&debug_pids_mutex);
			return;
		}
	}
	mutex_unlock(&debug_pids_mutex);
}

static void pids_release()
{
	struct pid_node *pos, *tmp;

	if (!debug_pids)
		return;

	mutex_lock(&debug_pids_mutex);
	list_for_each_entry_safe(pos, tmp, debug_pids, list) {
		list_del(&pos->list);
		kfree(pos);
	}
	kfree(debug_pids);
	debug_pids = NULL;
	mutex_unlock(&debug_pids_mutex);
}

static bool pids_init()
{
	if (debug_pids)
		return true;

	mutex_lock(&debug_pids_mutex);
	debug_pids = kzalloc(sizeof(struct list_head), GFP_KERNEL);
	if (!debug_pids) {
		ux_err("OOM when allocating audio debug_pids\n");
		mutex_unlock(&debug_pids_mutex);
		return false;
	}
	INIT_LIST_HEAD(debug_pids);
	mutex_unlock(&debug_pids_mutex);
	return true;
}

static int pids_clean_and_show(char *buffer, int size)
{
	int len = 0;
	struct task_struct *task = NULL;
	struct pid_node *pos, *tmp;

	mutex_lock(&debug_pids_mutex);
	list_for_each_entry_safe(pos, tmp, debug_pids, list) {
		task = find_task_by_vpid(pos->pid);
		if (task) {
			get_task_struct(task);
			if (!is_audio_flaged(task)) {
				list_del(&pos->list);
				kfree(pos);
			} else {
				len += snprintf(buffer + len, size - len, "pid=%d comm=%s tgid=%d\n",
					task->pid, task->comm, task->tgid);
			}

			put_task_struct(task);

			if (len >= size) {
				len = size;
				buffer[len - 1] = '\0';
				break;
			}
		}
	}
	mutex_unlock(&debug_pids_mutex);
	return len;
}

static ssize_t proc_enabled_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[8];
	int err, val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtoint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	sa_audio_perf_enable = val;

	return count;
}

static ssize_t proc_enabled_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[20];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "enabled=%d\n", sa_audio_perf_enable);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct file_operations proc_enable_fops = {
	.write		= proc_enabled_write,
	.read		= proc_enabled_read,
};

static ssize_t proc_debug_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[8];
	int err, val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtoint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	if (sa_audio_debug_enable == val)
		return count;

	sa_audio_debug_enable = val;
	if (sa_audio_debug_enable) {
		if (!pids_init())
			sa_audio_debug_enable = false;
	} else {
		pids_release();
	}

	return count;
}

static ssize_t proc_debug_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[1024];
	size_t len = 0;

	if (!sa_audio_debug_enable) {
		len = snprintf(buffer, sizeof(buffer), "debug=%d\n", sa_audio_debug_enable);
	} else {
		len = pids_clean_and_show(buffer, sizeof(buffer));
	}

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct file_operations proc_debug_fops = {
	.write		= proc_debug_write,
	.read		= proc_debug_read,
};

static ssize_t proc_status_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[8];
	int err, val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtoint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	if (sa_audio_perf_status == val)
		return count;

	oplus_sched_assist_audio_perf_set_status(val);
	return count;
}

static ssize_t proc_status_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[20];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "status=%d\n", sa_audio_perf_status);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct file_operations proc_status_fops = {
	.write		= proc_status_write,
	.read		= proc_status_read,
};

int oplus_sched_assist_audio_proc_init(struct proc_dir_entry *dir)
{
	struct proc_dir_entry *proc_node;

	audio_dir_parent = dir;
	audio_dir = proc_mkdir("audio", audio_dir_parent);
	if (!audio_dir) {
		ux_err("failed to create proc dir audio\n");
		goto err_creat_d_audio;
	}

	proc_node = proc_create("enable", 0666, audio_dir, &proc_enable_fops);
	if (!proc_node) {
		ux_err("failed to create proc node enable\n");
		goto err_creat_audio_enable;
	}

	proc_node = proc_create("debug", 0666, audio_dir, &proc_debug_fops);
	if (!proc_node) {
		ux_err("failed to create proc node debug\n");
		goto err_creat_audio_debug;
	}
	if (sa_audio_debug_enable &&!pids_init()) {
		goto err_creat_audio_debug;
	}

	proc_node = proc_create("status", 0666, audio_dir, &proc_status_fops);
	if (!proc_node) {
		ux_err("failed to create proc node debug\n");
		goto err_creat_audio_status;
	}

	return 0;
err_creat_audio_status:
	remove_proc_entry("debug", audio_dir);
err_creat_audio_debug:
	remove_proc_entry("enable", audio_dir);
err_creat_audio_enable:
	remove_proc_entry("audio", audio_dir_parent);
err_creat_d_audio:
	return -ENOENT;
}

void oplus_sched_assist_audio_proc_remove(struct proc_dir_entry *dir)
{
	remove_proc_entry("enable", audio_dir);
	remove_proc_entry("audio", audio_dir_parent);
}

static void set_sched_boost(struct task_struct *p, bool enable)
{
	int ux_state = p->ux_state;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_SCHED_UX_PRIORITY)
	if (enable) {
		ux_state = (ux_state | SA_TYPE_LIGHT | UX_PRIORITY_AUDIO);
	} else {
		ux_state = (ux_state & ~(SA_TYPE_LIGHT | SCHED_ASSIST_UX_PRIORITY_MASK));
	}
#else
	if (enable) {
		p->ux_state = (ux_state | SA_TYPE_LIGHT);
	} else {
		p->ux_state = (ux_state & ~SA_TYPE_LIGHT);
	}
#endif
	oplus_set_ux_state_lock(p, ux_state, true);
#if IS_ENABLED(CONFIG_SCHED_WALT)
	sched_set_wake_up_idle(p, enable);
#endif

	if (unlikely(sa_audio_debug_enable)) {
		if (enable) {
			add_pid(p->pid);
		} else {
			remove_pid(p->pid);
		}
	}
}

void oplus_sched_assist_audio_perf_addIm(struct task_struct *task, int im_flag)
{
	if (!is_audio_perf_enable())
		return;

	if (is_audio_flaged(task) && im_flag == IM_FLAG_NONE) {
		set_sched_boost(task, false);
	} else if (im_flag == IM_FLAG_AUDIO) {
		set_sched_boost(task, true);
	}
}

void oplus_sched_assist_audio_latency_sensitive(struct task_struct *task, bool *latency_sensitive)
{
	if (!is_audio_perf_status_on())
		return;

	if (*latency_sensitive || !is_audio_flaged(task))
		return;

	*latency_sensitive = true;

	debug_log(task, *latency_sensitive, "latency_sens");
}
EXPORT_SYMBOL(oplus_sched_assist_audio_latency_sensitive);

void oplus_sched_assist_audio_time_slack(struct task_struct *task)
{
	if (!is_audio_perf_status_on())
		return;

	if (!is_audio_flaged(task))
		return;

	if (task->timer_slack_ns > perf_timer_slack_ns) {
		task->timer_slack_ns = perf_timer_slack_ns;
		debug_log(task, task->timer_slack_ns, "timer_slack_ns");
	}
}
EXPORT_SYMBOL(oplus_sched_assist_audio_time_slack);

void oplus_sched_assist_audio_enqueue_hook(struct task_struct *task)
{
	if (!is_audio_perf_status_on())
		return;

	if (!is_audio_flaged(task))
		return;

	oplus_sched_assist_audio_time_slack(task);

	if (unlikely(global_debug_enabled & DEBUG_SYSTRACE)) {
		char buf[32];
		snprintf(buf, sizeof(buf), "C|9999|Ux_Audio|%d\n", task->pid);
		tracing_mark_write(buf);
	}
}
EXPORT_SYMBOL(oplus_sched_assist_audio_enqueue_hook);

bool oplus_sched_assist_audio_idle_balance(struct rq *this_rq)
{
	int this_cpu = this_rq->cpu, cpu;
	struct rq *src_rq;
	struct task_struct *p;
	bool pulled = false;

	if (!is_audio_perf_status_on())
		return false;

	/* can't help if this has a runnable RT */
	if (this_rq->rt.rt_queued > 0)
		return false;

	for_each_cpu(cpu, this_rq->rd->rto_mask) {
		if (this_cpu == cpu)
			continue;

		src_rq = cpu_rq(cpu);

		/*
		 * We can potentially drop this_rq's lock in
		 * double_lock_balance, and another CPU could
		 * alter this_rq
		 */
		double_lock_balance(this_rq, src_rq);

		/*
		 * We can pull only a task, which is pushable
		 * on its rq, and no others.
		 */
		p = pick_highest_pushable_task(src_rq, this_cpu);

		if (!p)
			goto skip;

		/* we only allow audio-app group task doing this work */
		if (!is_audio_flaged(p))
			goto skip;

		if (src_rq->clock - p->enqueue_time < AUDIO_RT_PULL_THRESHOLD_NS)
			goto skip;

		pulled = true;
		deactivate_task(src_rq, p, 0);
		set_task_cpu(p, this_cpu);
		activate_task(this_rq, p , 0);

		debug_log(p, cpu, "idle_balance");
skip:
		double_unlock_balance(this_rq, src_rq);
	}

	return pulled;
}
EXPORT_SYMBOL(oplus_sched_assist_audio_idle_balance);

bool oplus_sched_assist_audio_perf_check_exit_latency(struct task_struct *task, int cpu)
{
	struct cpuidle_state *idle;
	if (!is_audio_perf_status_on())
		return false;

	if (!is_audio_flaged(task))
		return false;

	idle = idle_get_state(cpu_rq(cpu));
	if (idle && idle->exit_latency > AUDIO_TASK_IDLE_EXIT_LATENCY) {
		debug_log(task, cpu, "skip_deep_idle");
		return true;
	}

	return false;
}
EXPORT_SYMBOL(oplus_sched_assist_audio_perf_check_exit_latency);

void oplus_sched_assist_audio_perf_set_status(int status)
{
	sa_audio_perf_status = !!status;

	if (unlikely(global_debug_enabled & DEBUG_SYSTRACE))
		debug_systrace_c(sa_audio_perf_status, "status");
}
EXPORT_SYMBOL(oplus_sched_assist_audio_perf_set_status);
