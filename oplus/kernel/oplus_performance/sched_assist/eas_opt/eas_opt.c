// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#ifdef CONFIG_OPLUS_FEATURE_VT_CAP
#include "oplus_cap.h"
#endif

#ifdef CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT
#include "oplus_iowait.h"
#endif

#define LARGE_BUFFER_SIZE 250
#define MIDDLE_BUFFER_SIZE 50
#define L_BUFFER_SIZE 10

int eas_opt_debug_enable = 0;
EXPORT_SYMBOL(eas_opt_debug_enable);
int eas_opt_enable = 0;
EXPORT_SYMBOL(eas_opt_enable);
int force_apply_ocap_enable = 0;
EXPORT_SYMBOL(force_apply_ocap_enable);

#define PARAM_NUM 2
static ssize_t proc_eas_opt_enabled_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
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

	switch (val) {
	case 0:
		eas_opt_enable = 0;
		eas_opt_debug_enable = 0;
		force_apply_ocap_enable = 0;
		break;
	case 1:
		eas_opt_enable = 1;
		eas_opt_debug_enable = 0;
		force_apply_ocap_enable = 0;
		break;
	case 2:
		eas_opt_enable = 1;
		eas_opt_debug_enable = 0;
		force_apply_ocap_enable = 1;
		break;
	case 3:
		eas_opt_enable = 1;
		eas_opt_debug_enable = 1;
		force_apply_ocap_enable = 0;
		break;
	case 4:
		eas_opt_enable = 1;
		eas_opt_debug_enable = 1;
		force_apply_ocap_enable = 1;
		break;
	default:
		break;
	}
	return count;
}

static ssize_t proc_eas_opt_enabled_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[MIDDLE_BUFFER_SIZE];
	size_t len = 0;

	len = snprintf(buffer, sizeof(buffer), "eas_enable=%d eas_debug_enable=%d force_apply=%d\n", eas_opt_enable, eas_opt_debug_enable, force_apply_ocap_enable);

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct file_operations proc_eas_opt_enabled_fops = {
	.write		= proc_eas_opt_enabled_write,
	.read		= proc_eas_opt_enabled_read,
};

struct proc_dir_entry *eas_opt_dir = NULL;
static int __init oplus_eas_opt_init(void)
{
	int ret = 0;
	struct proc_dir_entry *proc_node;

	eas_opt_dir = proc_mkdir("eas_opt", NULL);
	if (!eas_opt_dir) {
		pr_err("fail to mkdir /proc/eas_opt\n");
		return -ENOMEM;
	}

	proc_node = proc_create("eas_opt_enable", 0666, eas_opt_dir, &proc_eas_opt_enabled_fops);

#ifdef CONFIG_OPLUS_FEATURE_VT_CAP
	ret = oplus_cap_init(eas_opt_dir);
	if (ret != 0)
		return ret;

	pr_info("[eas_opt]: oplus_cap init success\n");
#endif

#ifdef CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT
	ret = oplus_sched_assist_iowait_proc_init(eas_opt_dir);
	if (ret != 0)
		return ret;

	pr_info("[eas_opt]: iowait_config init success\n");
#endif

	return ret;
}

module_init(oplus_eas_opt_init);
MODULE_DESCRIPTION("Oplus Eas Opt Moduler");
MODULE_LICENSE("GPL v2");
