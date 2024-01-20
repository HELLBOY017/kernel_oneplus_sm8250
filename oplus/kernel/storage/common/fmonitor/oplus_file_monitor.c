// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/limits.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/fcntl.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/atomic.h>
#include <linux/f2fs_fs.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/seq_file.h>
#include <linux/compiler.h>
#include "fs/f2fs/f2fs.h"
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include "trace/events/android_fs.h"
#else
#include <linux/fs.h>
#define MAX_TRACE_PATHBUF_LEN	256
static inline char *
android_fstrace_get_pathname(char *buf, int buflen, struct inode *inode)
{
	char *path;
	struct dentry *d;
	/*
	 * d_obtain_alias() will either iput() if it locates an existing
	 * dentry or transfer the reference to the new dentry created.
	 * So get an extra reference here.
	 */
	ihold(inode);
	d = d_obtain_alias(inode);
	if (likely(!IS_ERR(d))) {
		path = dentry_path_raw(d, buf, buflen);
		if (unlikely(IS_ERR(path))) {
			strcpy(buf, "ERROR");
			path = buf;
		}
		dput(d);
	} else {
		strcpy(buf, "ERROR");
		path = buf;
	}
	return path;
}
#endif
#include <trace/events/f2fs.h>
static struct proc_dir_entry *file_monitor_procfs = NULL;
static struct proc_dir_entry *path_filter_file = NULL;
static struct proc_dir_entry *remove_event_file = NULL;
static struct proc_dir_entry *unlink_switch_file = NULL;
static char pfliter_buf[1024] = {0};
static char pfliter_front[512] = {0};
static char pfliter_back[512] = {0};
static char remove_buf[1024] = {0};
static int g_unlink_switch = 0;
static int rlen = 0;
#define FILE_MONITOR_LOG_TAG "[file_monitor]"

static ssize_t remove_proc_write(struct file *file, const char __user *buf,
		size_t count, loff_t *off)
{
	memset(remove_buf, 0, sizeof(remove_buf));

	if (count > sizeof(remove_buf) - 1) {
		count = sizeof(remove_buf) - 1;
	}

	if (copy_from_user(remove_buf, buf, count)) {
		pr_err(FILE_MONITOR_LOG_TAG "%s: read proc input error.\n", __func__);
		return count;
	}

	if (count > 0) {
		remove_buf[count-1] = '\0';
	}

	return count;
}

static ssize_t remove_proc_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(buf, count, ppos, remove_buf, strlen(remove_buf));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static struct proc_ops remove_event_fops = {
	.proc_read = remove_proc_read,
	.proc_write = remove_proc_write,
	.proc_lseek = default_llseek,
};
#else
static struct file_operations remove_event_fops = {
	.read = remove_proc_read,
	.write = remove_proc_write,
	.llseek = default_llseek,
};
#endif

static ssize_t pfliter_proc_write(struct file *file, const char __user *buf,
		size_t count, loff_t *off)
{
	char *ptr;
	memset(pfliter_buf, 0, sizeof(pfliter_buf));

	if (count > sizeof(pfliter_buf) - 1) {
		count = sizeof(pfliter_buf) - 1;
	}

	if (copy_from_user(pfliter_buf, buf, count)) {
		pr_err(FILE_MONITOR_LOG_TAG "%s: read proc input error.\n", __func__);
		return count;
	}

	if (count > 0) {
		pfliter_buf[count-1] = '\0';
	}

	ptr = strstr(pfliter_buf, "*");
	if (ptr) {
		size_t len1 = ptr - pfliter_buf;
		size_t len2 = count - len1 - 2;

		strncpy(pfliter_front, pfliter_buf, len1);
		strncpy(pfliter_back, ptr + 1, len2);

		pfliter_front[len1] = '\0';
		pfliter_back[len2] = '\0';
	}

	return count;
}

static ssize_t pfliter_proc_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char bpath[100];
	size_t len = 0;

	len = snprintf(bpath, sizeof(bpath), "%s", pfliter_buf);
	return simple_read_from_buffer(buf, count, ppos, bpath, len);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static struct proc_ops path_filter_fops = {
	.proc_read = pfliter_proc_read,
	.proc_write = pfliter_proc_write,
	.proc_lseek = default_llseek,
};
#else
static struct file_operations path_filter_fops = {
	.read = pfliter_proc_read,
	.write = pfliter_proc_write,
	.llseek = default_llseek,
};
#endif

static ssize_t uswitch_proc_write(struct file *file, const char __user *buf,
		size_t count, loff_t *off)
{
	char str[3] = {0};

	if (count > 2 || count < 1) {
		pr_err(FILE_MONITOR_LOG_TAG "unlink switch invalid parameter\n");
		return -EINVAL;
	}

	if (copy_from_user(str, buf, count)) {
		pr_err(FILE_MONITOR_LOG_TAG "copy_from_user failed\n");
		return -EFAULT;
	}

	if (unlikely(!strncmp(str, "1", 1))) {
		pr_info(FILE_MONITOR_LOG_TAG "unlink switch enabled\n");
		g_unlink_switch = 1;
	} else {
		pr_info(FILE_MONITOR_LOG_TAG "unlink switch disabled\n");
		g_unlink_switch = 0;
	}

	return (ssize_t)count;
}

static int unlink_switch_show(struct seq_file *s, void *data)
{
	if (g_unlink_switch == 1)
		seq_printf(s, "%d\n", 1);
	else
		seq_printf(s, "%d\n", 0);

	return 0;
}

static int uswitch_proc_open(struct inode *inodp, struct file *filp)
{
	return single_open(filp, unlink_switch_show, inodp);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static struct proc_ops unlink_switch_fops = {
	.proc_open = uswitch_proc_open,
	.proc_read = seq_read,
	.proc_write = uswitch_proc_write,
	.proc_lseek = default_llseek,
};
#else
static struct file_operations unlink_switch_fops = {
	.open = uswitch_proc_open,
	.read = seq_read,
	.write = uswitch_proc_write,
	.llseek = default_llseek,
};
#endif

static void cb_f2fs_unlink_enter(void *ignore, struct inode *dir,
                                          struct dentry *dentry)
{
	int written = 0;
	char *file;
	struct inode *inode = d_inode(dentry);
	char *path, pathbuf[MAX_TRACE_PATHBUF_LEN];

	if (g_unlink_switch == 0) {
		return;
	}

	path = android_fstrace_get_pathname(pathbuf,
										MAX_TRACE_PATHBUF_LEN,
										inode);
	if(strlen(pfliter_front)) {
		if(strstr(path, pfliter_front) && strstr(path, pfliter_back)) {
			rlen = strlen(remove_buf);
			if (rlen >= sizeof(remove_buf) - 1) {
				memset(remove_buf, 0, sizeof(remove_buf));
				rlen = 0;
			}
			file = strrchr(path, '/');
			written = snprintf(remove_buf + rlen, sizeof(remove_buf) - rlen -1, "F:%s T:%s \n", file, current->comm);
			rlen += written;
		}
	} else {
		if(strstr(path, pfliter_buf)) {
			rlen = strlen(remove_buf);
			if (rlen >= sizeof(remove_buf) - 1) {
				memset(remove_buf, 0, sizeof(remove_buf));
				rlen = 0;
			}
			file = strrchr(path, '/');
			written = snprintf(remove_buf + rlen, sizeof(remove_buf) - rlen -1, "F:%s T:%s \n", file, current->comm);
			rlen += written;
		}
	}
};


static void file_monitor_register_tracepoints(void)
{
	int ret;

	ret = register_trace_f2fs_unlink_enter(cb_f2fs_unlink_enter, NULL);

	return;
}

static void f2fs_unregister_tracepoint_probes(void)
{
	unregister_trace_f2fs_unlink_enter(cb_f2fs_unlink_enter, NULL);
}

int __init file_monitor_init(void)
{
	file_monitor_procfs = proc_mkdir("oplus_storage/file_monitor", NULL);
	if (file_monitor_procfs == NULL) {
		pr_err(FILE_MONITOR_LOG_TAG" Failed to create file_monitor procfs\n");
		return -EFAULT;
	}

	path_filter_file = proc_create("path_filter", 0644, file_monitor_procfs, &path_filter_fops);
	if (path_filter_file == NULL) {
		pr_err(FILE_MONITOR_LOG_TAG" Failed to create file oplus_storage/file_monitor/path_filter\n");
		return -EFAULT;
	}

	remove_event_file = proc_create("remove_event", 0644, file_monitor_procfs, &remove_event_fops);
	if (remove_event_file == NULL) {
		pr_err(FILE_MONITOR_LOG_TAG" Failed to create file oplus_storage/file_monitor/remove_event\n");
		return -EFAULT;
	}

	unlink_switch_file = proc_create("unlink_switch", 0644, file_monitor_procfs, &unlink_switch_fops);
	if (unlink_switch_file == NULL) {
		pr_err(FILE_MONITOR_LOG_TAG" Failed to create file oplus_storage/file_monitor/unlink_switch\n");
		return -EFAULT;
	}

	memset(pfliter_buf, 0, sizeof(pfliter_buf));
	memset(remove_buf, 0, sizeof(remove_buf));
	strcpy(pfliter_buf, "file_monitor");
	file_monitor_register_tracepoints();

	return 0;
}

void __exit file_monitor_exit(void)
{
	remove_proc_entry("path_filter", file_monitor_procfs);
	remove_proc_entry("remove_event", file_monitor_procfs);
	remove_proc_entry("unlink_switch", file_monitor_procfs);

	f2fs_unregister_tracepoint_probes();
}

module_init(file_monitor_init);
module_exit(file_monitor_exit);

MODULE_DESCRIPTION("oplus file monitor");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");

