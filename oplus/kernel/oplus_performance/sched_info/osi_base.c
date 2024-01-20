// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>

#include "osi_base.h"

#ifdef JANK_SYSTRACE_DEBUG
#ifdef JANK_DEBUG
u32 systrace_enable = 1;
#else
u32 systrace_enable;
#endif
module_param(systrace_enable, int, 0644);
MODULE_PARM_DESC(systrace_enable,
		"This variable controls whether systrace information is output");

static void __maybe_unused tracing_mark_write(char *buf)
{
	preempt_disable();
	trace_printk(buf);
	preempt_enable();
}

#define JANK_TRACE_LEN			256
void __maybe_unused jank_systrace_print(u32 pid, char *tag, u64 val)
{
	char buf[JANK_TRACE_LEN];

	if (likely(!systrace_enable))
		return;

	snprintf(buf, sizeof(buf), "C|%d|%s|%llu\n", pid, tag, val);
	tracing_mark_write(buf);
}

void __maybe_unused jank_systrace_print_idx(u32 pid,
		char *tag, u32 idx, u64 val)
{
	char buf[JANK_TRACE_LEN];

	if (likely(!systrace_enable))
		return;

	snprintf(buf, sizeof(buf), "C|%d|%s$%d|%llu\n", pid, tag, idx, val);
	tracing_mark_write(buf);
}
#endif

#if defined(CONFIG_CGROUP_SCHED)
#if defined(CONFIG_SCHED_TUNE)
int task_cgroup_id(struct task_struct *p)
{
	struct cgroup_subsys_state *css = task_css(p, schedtune_cgrp_id);

	return css ? css->id : -EFAULT;
}
#else
int task_cgroup_id(struct task_struct *task)
{
	struct cgroup_subsys_state *css = task_css(task, cpu_cgrp_id);

	return css ? css->id : -EFAULT;
}
#endif
#else
int task_cgroup_id(struct task_struct *p)
{
	return -EFAULT;
}
#endif

bool is_fg_or_topapp(struct task_struct *p)
{
	int class;

	if (!p)
		return false;

	class = task_cgroup_id(p);
	if (class == CGROUP_FOREGROUND || class == CGROUP_TOP_APP)
		return true;

	return false;
}

bool is_default(struct task_struct *p)
{
	if (!p)
		return false;

	if (task_cgroup_id(p) == CGROUP_DEFAULT)
		return true;

	return false;
}

bool is_foreground(struct task_struct *p)
{
	if (!p)
		return false;

	if (task_cgroup_id(p) == CGROUP_FOREGROUND)
		return true;

	return false;
}

bool is_background(struct task_struct *p)
{
	if (!p)
		return false;

	if (task_cgroup_id(p) == CGROUP_BACKGROUND)
		return true;

	return false;
}

bool is_topapp(struct task_struct *p)
{
	if (!p)
		return false;

	if (task_cgroup_id(p) == CGROUP_TOP_APP)
		return true;

	return false;
}

bool is_same_idx(u64 timestamp, u64 now)
{
	return time2idx(now) == time2idx(timestamp);
}

bool is_same_window(u64 timestamp, u64 now)
{
	return time2winidx(now) == time2winidx(timestamp);
}

/*
 * Determine if the data is valid
 * Invalid when data exceeds 16 window cycles
 */
bool timestamp_is_valid(u64 timestamp, u64 now)
{
	u64 win_cnt;

	if (now < timestamp)
		return false;

	win_cnt = time2idx(now) - time2idx(timestamp);

	if (win_cnt >= JANK_WIN_CNT)
		return false;
	else
		return true;
}

u32 split_window(u64 now, u64 delta,
		u64 *win_a, u64 *win_cnt, u64 *win_b)
{
	if (!win_a || !win_cnt || !win_b)
		return 0;

	if (!delta) {
		*win_a = *win_cnt = *win_b = 0;
		return 0;
	}

	*win_b = min_t(u64, delta, (now & JANK_WIN_SIZE_IN_NS_MASK));
	*win_cnt = (delta - *win_b) >> JANK_WIN_SIZE_SHIFT_IN_NS;
	*win_a = delta - *win_b - (*win_cnt << JANK_WIN_SIZE_SHIFT_IN_NS);

	return *win_cnt + !!(*win_b) + !!(*win_a);
}

int g_osi_debug;

static ssize_t proc_osi_debug_read(struct file *file,
		char __user *buf, size_t count, loff_t *ppos)
{
	char buffer[PROC_NUMBUF];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "%d\n", g_osi_debug);
	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t proc_osi_debug_write(struct file *file,
			const char __user *buf, size_t count, loff_t *ppos)
{
	char buffer[PROC_NUMBUF];
	int err, data;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	err = kstrtouint(strstrip(buffer), 0, &data);
	if (err)
		return err;
	g_osi_debug = data;

	return count;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations proc_osi_debug_operations = {
	.read = proc_osi_debug_read,
	.write = proc_osi_debug_write,
	.llseek	= default_llseek,
};
#else
static const struct proc_ops proc_osi_debug_operations = {
	.proc_read = proc_osi_debug_read,
	.proc_write = proc_osi_debug_write,
	.proc_lseek	= default_llseek,
};
#endif
void osi_base_proc_init(struct proc_dir_entry *pde)
{
	struct proc_dir_entry *entry = NULL;
	entry = proc_create("osi_debug", S_IRUGO,
				pde, &proc_osi_debug_operations);
	if (!entry) {
		osi_err("create osi_debug fail\n");
		return;
	}
}

void osi_base_proc_deinit(struct proc_dir_entry *pde)
{
	remove_proc_entry("osi_debug", pde);
}
