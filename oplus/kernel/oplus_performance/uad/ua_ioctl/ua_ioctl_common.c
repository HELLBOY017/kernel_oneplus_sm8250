// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/compat.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../fs/proc/internal.h>
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
#ifdef CONFIG_ARCH_MEDIATEK
#include <../kernel/oplus_perf_sched/frame_boost/frame_boost.h>
#else
#include <../kernel/tuning/frame_boost.h>
#endif /* CONFIG_ARCH_MEDIATEK */
#endif

#include "frame_ioctl.h"
#include "touch_ioctl.h"
#include "ua_ioctl_common.h"

#define UA_PROC_NODE "oplus_cpu"

static struct proc_dir_entry *ua_common_proc;
unsigned long g_ua_status;


static int set_ua_status(void __user *uarg)
{
	struct ua_info_data info;
	int ret = 0;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&info, uarg, sizeof(struct ua_info_data))) {
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	/* TODO: save different states in unsigned long value, one bit for one state */
	pr_info("[xieliujie] set_ua_status: status=%d\n", info.status);

	if (info.status >=0 && info.status < UA_MAX_STATUS)
		set_bit(info.status, &g_ua_status);

	return ret;
}

static int get_util_info(void __user *uarg)
{
	struct ua_info_data info;
	int ret = 0;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&info, uarg, sizeof(struct ua_info_data))) {
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	pr_err("[xieliujie] get_util_info: info.status=%d info.prev=%lu info.curr=%lu\n",
		info.status, info.frame_prev_util_scale, info.frame_curr_util_scale);

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	fbg_get_prev_util(&info.frame_prev_util_scale);
	fbg_get_curr_util(&info.frame_curr_util_scale);
#endif /* CONFIG_OPLUS_FEATURE_FRAME_BOOST */

	if (copy_to_user(uarg, &info, sizeof(struct ua_info_data))) {
		pr_err("%s: copy_to_user fail\n", __func__);
		ret = -EFAULT;
	}

	return ret;
}

static long cpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)(uintptr_t)arg;
	long ret;

	if (_IOC_TYPE(cmd) != CPU_CTRL_MAGIC)
		return -EINVAL;

	if (_IOC_NR(cmd) >= CPU_CTRL_CMD_MAX)
		return -EINVAL;

	switch (cmd) {
	case CPU_CTRL_SET_UA_STATUS:
		return set_ua_status(uarg);

	case CPU_CTRL_GET_UTIL_INFO:
		return get_util_info(uarg);

	default:
		pr_err("error cmd=%u\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long cpu_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return cpu_ioctl(file, cmd, (unsigned long)(compat_ptr(arg)));
}
#endif

static int cpu_ctrl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cpu_ctrl_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cpu_ioctrl_fops = {
	.unlocked_ioctl	= cpu_ioctl,
	.open	= cpu_ctrl_open,
	.release	= cpu_ctrl_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= cpu_compat_ioctl,
#endif
	.llseek = default_llseek,
};

static int __init ioctl_common_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pentry;

	/* Create UA common proc node */
	ua_common_proc = proc_mkdir(UA_PROC_NODE, NULL);

	pentry = proc_create("ua_ctrl", S_IRWXUGO, ua_common_proc, &cpu_ioctrl_fops);
	if (!pentry)
		return -ENOENT;

	/* Create frame boost proc node */
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	ret = frame_ioctl_init();
	if (ret)
		return ret;
#endif

	ret = touch_ioctl_init();
	if (ret)
		return ret;

	return ret;
}

static void __exit ioctl_common_exit(void)
{
	if (ua_common_proc != NULL)
		remove_proc_entry(UA_PROC_NODE, NULL);

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FRAME_BOOST)
	frame_ioctl_exit();
#endif /* CONFIG_OPLUS_FEATURE_FRAME_BOOST */

	touch_ioctl_exit();
}

module_init(ioctl_common_init);
module_exit(ioctl_common_exit);

MODULE_LICENSE("GPL v2");
