// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2030 Oplus. All rights reserved.
 */
#include <linux/types.h>
#include <linux/string.h>      /* for strncmp */
#include <linux/workqueue.h>   /* for struct work_struct, INIT_WORK, schedule_work, cancel_work_sync etc */
#include <linux/timer.h>       /* for struct timer_list, timer_setup, mod_timer, del_timer etc */
#include <linux/signal_types.h> /* for struct kernel_siginfo */
#include <linux/sched/signal.h> /* for send_sig_info */
#include <linux/sched.h>       /* for struct task_struct, find_task_by_vpid, PF_FROZEN, TASK_UNINTERRUPTIBLE etc */
#include <linux/err.h>         /* for IS_ERR_OR_NULL */
#include <linux/rcupdate.h>    /* for rcu_read_lock, rcu_read_unlock */
#include <linux/printk.h>      /* for pr_err, pr_info etc */
#include <linux/sched/debug.h> /* for sched_show_task */
#include <linux/uaccess.h>     /* for copy_from_user */
#include <linux/errno.h>       /* for EINVAL, EFAULT */
#include <linux/compiler.h>    /* for likely, unlikely */
#include <linux/proc_fs.h>     /* for struct proc_ops, proc_create, proc_remove etc */
#include <linux/seq_file.h>    /* for single_open, seq_lseek, seq_release etc */
#include <linux/slab.h>        /* for kzalloc, kfree */
#include <linux/module.h>      /* for late_initcall, module_exit etc */
#include <linux/jiffies.h>     /* for jiffies, include asm/param.h for HZ */
#include <linux/kernel.h>      /* for current ,snprintf */
#include <linux/version.h>
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_THEIA)
#include "../include/theia_send_event.h" /* for theia_send_event etc */
#endif

#define INIT_WATCHDOG_LOG_TAG "[init_watchdog]"
#define BUF_STR_LEN 10
#define INIT_WATCHDOG_CHECK_INTERVAL 30
#define SIG_WAKEUP_INIT (SIGRTMIN + 0x16)
#define EXTRA_INFO_BUF_LEN 128

typedef enum {
	STATE_DISABLE,
	STATE_ENABLE,
	STATE_CHECKED,
} INIT_WATCHDOG_STATE;

typedef enum  {
	STATE_NOT_KICK,
	STATE_KICKED,
} INIT_WATCHDOG_KICK_STATE;

struct init_watchdog_data {
	struct work_struct work;
	struct timer_list timer;
	INIT_WATCHDOG_STATE state;
	INIT_WATCHDOG_KICK_STATE kick_state;
	unsigned int block_count;
};

static struct init_watchdog_data *g_init_watchdog_data;
static int g_init_watchdog_debug_enabled;
static struct proc_dir_entry *g_proc_oplus_init_watchdog;

static void init_check_work_handler(struct work_struct *work)
{
	struct task_struct *taskp = NULL;
	char extra_info[EXTRA_INFO_BUF_LEN] = {0};

	/* init not ready for check, don't check this loop */
	if (g_init_watchdog_data->state != STATE_CHECKED) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init check is not ready or already blocked, just return\n");
		return;
	}

	/* if kicked, init process work normal, clear block count, exit to next loop */
	if (g_init_watchdog_data->kick_state == STATE_KICKED) {
		g_init_watchdog_data->kick_state = STATE_NOT_KICK;
		g_init_watchdog_data->block_count = 0;
		pr_info(INIT_WATCHDOG_LOG_TAG "init process is alive\n");
		return;
	}

	rcu_read_lock();
	taskp = find_task_by_vpid(1);
	rcu_read_unlock();

	if (IS_ERR_OR_NULL(taskp)) {
		pr_err(INIT_WATCHDOG_LOG_TAG "can't find init task_struct\n");
		return;
	}

	if (taskp->flags & PF_FROZEN) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init process is frozen, normal state\n");
		g_init_watchdog_data->block_count = 0;
		return;
	}

	g_init_watchdog_data->block_count++;

	/* D-state, unrecoverable, just dump debug message */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	if (READ_ONCE(taskp->__state) == TASK_UNINTERRUPTIBLE) {
#else
	if (taskp->state == TASK_UNINTERRUPTIBLE) {
#endif
		pr_err(INIT_WATCHDOG_LOG_TAG "init process block in D-state more than 30s\n");

		snprintf(extra_info, EXTRA_INFO_BUF_LEN, "init process block in D-state "
			"more than 30s, block_count is %u", g_init_watchdog_data->block_count);
	} else {
		pr_err(INIT_WATCHDOG_LOG_TAG "init process blocked for more than 30s\n");
		/* will enable wakeup init when detected stuck in sleep state in the future */

		snprintf(extra_info, EXTRA_INFO_BUF_LEN, "init process block for more "
			"than 30s, block_count is %u", g_init_watchdog_data->block_count);
	}
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_THEIA)
	theia_send_event(THEIA_EVENT_INIT_HANG, THEIA_LOGINFO_KERNEL_LOG,
		current->pid, extra_info);
#endif
	pr_err(INIT_WATCHDOG_LOG_TAG "block_count is %u\n",
		g_init_watchdog_data->block_count);
	pr_err(INIT_WATCHDOG_LOG_TAG "dump init process kernel backtrace\n");
	sched_show_task(taskp);

	/* disable check to avoid continue report, util init recover and kick */
	g_init_watchdog_data->state = STATE_DISABLE;
	/* dump all d-state process stacktrace if necessary */
}

static ssize_t kick_watchdog_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *off)
{
	char str[BUF_STR_LEN] = {0};

	if (count < 4 || count > 9) {
		pr_err(INIT_WATCHDOG_LOG_TAG "proc_write invalid parameter\n");
		return -EINVAL;
	}

	if (copy_from_user(str, buf, count)) {
		pr_err(INIT_WATCHDOG_LOG_TAG "copy_from_user failed\n");
		return -EFAULT;
	}

	if (likely(!strncmp(str, "kick", 4))) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init process kicked\n");
		g_init_watchdog_data->state = STATE_CHECKED;
		g_init_watchdog_data->kick_state = STATE_KICKED;
	} else if (!strncmp(str, "enable", 6)) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init watchdog is enabled\n");
		g_init_watchdog_data->state = STATE_ENABLE;
	} else if (unlikely(!strncmp(str, "disable", 7))) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init watchdog is disabled\n");
		g_init_watchdog_data->state = STATE_DISABLE;
	} else {
		pr_err(INIT_WATCHDOG_LOG_TAG "invalid set value, ignore\n");
	}

	return (ssize_t)count;
}

static int kick_watchdog_show(struct seq_file *s, void *data)
{
	if (g_init_watchdog_data->kick_state == STATE_KICKED)
		seq_printf(s, "%s\n", "kicked");
	else if (g_init_watchdog_data->state == STATE_CHECKED)
		seq_printf(s, "%s\n", "checked");
	else if (g_init_watchdog_data->state == STATE_ENABLE)
		seq_printf(s, "%s\n", "enabled");
	else
		seq_printf(s, "%s\n", "disabled");

	return 0;
}

static int kick_watchdog_open(struct inode *inodp, struct file *filp)
{
	return single_open(filp, kick_watchdog_show, inodp);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static struct proc_ops kick_watchdog_fops = {
	.proc_open = kick_watchdog_open,
	.proc_read = seq_read,
	.proc_write = kick_watchdog_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static struct file_operations kick_watchdog_fops = {
	.open = kick_watchdog_open,
	.read = seq_read,
	.write = kick_watchdog_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static ssize_t debug_watchdog_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *off)
{
	char str[3] = {0};

	if (count > 2 || count < 1) {
		pr_err(INIT_WATCHDOG_LOG_TAG "debug_watchdog_write invalid parameter\n");
		return -EINVAL;
	}

	if (copy_from_user(str, buf, count)) {
		pr_err(INIT_WATCHDOG_LOG_TAG "copy_from_user failed\n");
		return -EFAULT;
	}

	if (likely(!strncmp(str, "1", 1))) {
		pr_info(INIT_WATCHDOG_LOG_TAG "init watchdog debug enabled\n");
		g_init_watchdog_debug_enabled = 1;
	} else {
		pr_info(INIT_WATCHDOG_LOG_TAG "init watchdog debug disabled\n");
		g_init_watchdog_debug_enabled = 0;
	}

	return (ssize_t)count;
}

static int debug_watchdog_show(struct seq_file *s, void *data)
{
	if (g_init_watchdog_debug_enabled == 1)
		seq_printf(s, "%d\n", 1);
	else
		seq_printf(s, "%d\n", 0);

	return 0;
}

static int debug_watchdog_open(struct inode *inodp, struct file *filp)
{
	return single_open(filp, debug_watchdog_show, inodp);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static struct proc_ops debug_watchdog_fops = {
	.proc_open = debug_watchdog_open,
	.proc_read = seq_read,
	.proc_write = debug_watchdog_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static struct file_operations debug_watchdog_fops = {
	.open = debug_watchdog_open,
	.read = seq_read,
	.write = debug_watchdog_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static void init_check_timer_func(struct timer_list *t)
{
	mod_timer(t, jiffies + HZ * INIT_WATCHDOG_CHECK_INTERVAL);
	schedule_work(&g_init_watchdog_data->work);
}

static int __init init_watchdog_init(void)
{
	struct proc_dir_entry *p_entry;

	/* vmalloc init_watchdog_data */
	g_init_watchdog_data = kzalloc(sizeof(*g_init_watchdog_data), GFP_KERNEL);
	if (g_init_watchdog_data == NULL) {
		pr_err(INIT_WATCHDOG_LOG_TAG "kzalloc init_watchdog_data failed\n");
		return -1;
	}

	g_proc_oplus_init_watchdog = proc_mkdir("oplus_init_watchdog", NULL);
	if (!g_proc_oplus_init_watchdog)
		goto proc_create_err;

	p_entry = proc_create_data("kick", 0640, g_proc_oplus_init_watchdog, &kick_watchdog_fops, NULL);
	if (!p_entry)
		goto proc_create_err;

	p_entry = proc_create_data("debug", 0640, g_proc_oplus_init_watchdog, &debug_watchdog_fops, NULL);
	if (!p_entry)
		goto proc_create_err;

	g_init_watchdog_debug_enabled = 0;
	g_init_watchdog_data->state = STATE_DISABLE;
	g_init_watchdog_data->kick_state = STATE_NOT_KICK;
	g_init_watchdog_data->block_count = 0;

	/* create work, init timer */
	INIT_WORK(&g_init_watchdog_data->work, init_check_work_handler);

	timer_setup(&g_init_watchdog_data->timer, init_check_timer_func,
		TIMER_DEFERRABLE);
	mod_timer(&g_init_watchdog_data->timer,
		jiffies + HZ * INIT_WATCHDOG_CHECK_INTERVAL);

	return 0;

proc_create_err:
	pr_err(INIT_WATCHDOG_LOG_TAG "proc node create failed\n");
	remove_proc_entry("oplus_init_watchdog", NULL);
	kfree(g_init_watchdog_data);
	return -ENOENT;
}

static void __exit init_watchdog_exit(void)
{
	del_timer_sync(&g_init_watchdog_data->timer);
	cancel_work_sync(&g_init_watchdog_data->work);
	remove_proc_entry("oplus_init_watchdog", NULL);
	kfree(g_init_watchdog_data);
	g_init_watchdog_data = NULL;
}

module_init(init_watchdog_init);
module_exit(init_watchdog_exit);

MODULE_DESCRIPTION("oplus_init_watchdog");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
