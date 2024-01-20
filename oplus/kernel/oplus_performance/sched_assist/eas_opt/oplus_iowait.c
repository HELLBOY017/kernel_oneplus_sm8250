// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/uaccess.h>
#include <linux/reciprocal_div.h>
#include <linux/cpumask.h>
#include <linux/arch_topology.h>
#include <../kernel/sched/sched.h>
#include "oplus_iowait.h"

#define LARGE_BUFFER_SIZE 250
#define MIDDLE_BUFFER_SIZE 50
#define L_BUFFER_SIZE 10

unsigned int sysctl_iowait_reset_ticks = 1;
unsigned int sysctl_iowait_apply_ticks = 0;
unsigned int sysctl_oplus_iowait_boost_enabled = 0;
unsigned int sysctl_oplus_iowait_skip_min_enabled = 1;
EXPORT_SYMBOL(sysctl_iowait_reset_ticks);
EXPORT_SYMBOL(sysctl_iowait_apply_ticks);
EXPORT_SYMBOL(sysctl_oplus_iowait_boost_enabled);
EXPORT_SYMBOL(sysctl_oplus_iowait_skip_min_enabled);
static ssize_t proc_iowait_boost_enabled_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[L_BUFFER_SIZE];
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

	sysctl_oplus_iowait_boost_enabled = val;

	return count;
}

static ssize_t proc_iowait_boost_enabled_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "iowait_boost_freq_enabled=%d\n",
			sysctl_oplus_iowait_boost_enabled);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t proc_iowait_skip_min_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[L_BUFFER_SIZE];
	int err;
	unsigned int val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtouint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	sysctl_oplus_iowait_skip_min_enabled = val;

	return count;
}

static ssize_t proc_iowait_skip_min_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "iowait_skip_min_enabled=%u\n", sysctl_oplus_iowait_skip_min_enabled);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}
static ssize_t proc_iowait_reset_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[L_BUFFER_SIZE];
	int err;
	unsigned int val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtouint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	sysctl_iowait_reset_ticks = val;

	return count;
}

static ssize_t proc_iowait_reset_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "iowait_reset_ticks=%u\n", sysctl_iowait_reset_ticks);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t proc_iowait_apply_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[L_BUFFER_SIZE];
	int err;
	unsigned int val;

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	err = kstrtouint(strstrip(buffer), 10, &val);
	if (err)
		return err;

	sysctl_iowait_apply_ticks = val;

	return count;
}

static ssize_t proc_iowait_apply_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "iowait_apply_ticks=%u\n", sysctl_iowait_apply_ticks);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}
static const struct file_operations proc_iowait_reset_fops = {
	.write = proc_iowait_reset_write,
	.read = proc_iowait_reset_read,
	.llseek = default_llseek,
};

static const struct file_operations proc_iowait_apply_fops = {
	.write = proc_iowait_apply_write,
	.read = proc_iowait_apply_read,
	.llseek = default_llseek,
};

static const struct file_operations proc_iowait_boost_enabled_fops = {
	.write = proc_iowait_boost_enabled_write,
	.read = proc_iowait_boost_enabled_read,
	.llseek = default_llseek,
};

static const struct file_operations proc_iowait_skip_min_fops = {
	.write = proc_iowait_skip_min_write,
	.read = proc_iowait_skip_min_read,
	.llseek = default_llseek,
};

struct proc_dir_entry *iowait_dir_parent = NULL;
struct proc_dir_entry *iowait_dir = NULL;
int oplus_sched_assist_iowait_proc_init(struct proc_dir_entry *dir)
{
	struct proc_dir_entry *proc_node;

	iowait_dir_parent = dir;
	iowait_dir = proc_mkdir("iowait_config", iowait_dir_parent);
	if (!iowait_dir) {
		pr_err("failed to create proc dir iowait_config\n");
		goto err_create_d_iowait;
	}

	proc_node = proc_create("iowait_reset_ticks", 0666, iowait_dir, &proc_iowait_reset_fops);
	if (!proc_node) {
		pr_err("failed to create proc node iowait_reset_ticks\n");
		goto err_create_iowait_reset;
	}

	proc_node = proc_create("iowait_apply_ticks", 0666, iowait_dir, &proc_iowait_apply_fops);
	if (!proc_node) {
		pr_err("failed to create proc node iowait_apply_ticks\n");
		goto err_create_iowait_apply;
	}

	proc_node = proc_create("oplus_iowait_boost_enabled", 0666, iowait_dir, &proc_iowait_boost_enabled_fops);
	if (!proc_node) {
		pr_err("failed to create proc node iowait_boost_freq_enabled\n");
		goto err_create_iowait_boost_enabled;
	}

	proc_node = proc_create("oplus_iowait_skip_min_enabled", 0666, iowait_dir, &proc_iowait_skip_min_fops);
	if (!proc_node) {
		pr_err("failed to create proc node iowait_skip_min_fops\n");
		goto err_create_iowait_skip_min_enabled;
	}

	return 0;

err_create_iowait_skip_min_enabled:
	remove_proc_entry("oplus_iowait_boost_enabled", iowait_dir);
err_create_iowait_boost_enabled:
	remove_proc_entry("iowait_apply_ticks", iowait_dir);
err_create_iowait_apply:
	remove_proc_entry("iowait_reset_ticks", iowait_dir);
err_create_iowait_reset:
	remove_proc_entry("iowait_config", iowait_dir_parent);
err_create_d_iowait:
	return -ENOENT;
}

void oplus_sched_assist_iowait_proc_remove(struct proc_dir_entry *dir)
{
	remove_proc_entry("oplus_iowait_skip_min_enabled", iowait_dir);
	remove_proc_entry("oplus_iowait_boost_enabled", iowait_dir);
	remove_proc_entry("iowait_reset_ticks", iowait_dir);
	remove_proc_entry("iowait_apply_ticks", iowait_dir);
	remove_proc_entry("iowait_config", iowait_dir_parent);
}
