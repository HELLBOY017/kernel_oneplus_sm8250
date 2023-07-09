// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/ioctl.h>
#include <linux/compat.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../fs/proc/internal.h>

#include "frame_boost.h"
#include "frame_debug.h"
#include "cluster_boost.h"

#define IM_FLAG_SURFACEFLINGER (1)
#define IM_FLAG_RENDERENGINE (3)

static struct proc_dir_entry *frame_boost_proc;
int stune_boost[BOOST_MAX_TYPE];
#ifdef CONFIG_OPLUS_FEATURE_IM
static inline int oplus_get_im_flag(struct task_struct *task)
{
	return task->im_flag;
}
#else
static inline int oplus_get_im_flag(struct task_struct *task)
{
	return 0;
}
#endif

static void crtl_update_refresh_rate(int pid, unsigned int vsyncNs)
{
	unsigned int frame_rate =  NSEC_PER_SEC / (unsigned int)(vsyncNs);
	bool is_sf = false;

	is_sf = (oplus_get_im_flag(current) == IM_FLAG_SURFACEFLINGER);

	if (is_sf) {
		set_max_frame_rate(frame_rate);
		set_frame_group_window_size(vsyncNs);
		return;
	}

	if ((pid != current->pid) || (pid != get_frame_group_ui()))
		return;

	if (set_frame_rate(frame_rate))
		set_frame_group_window_size(vsyncNs);
}

/*********************************
 * frame boost ioctl proc
 *********************************/
static long ofb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct ofb_ctrl_data data;
	void __user *uarg = (void __user *)arg;

	if (_IOC_TYPE(cmd) != OFB_MAGIC)
		return -EINVAL;

	if (_IOC_NR(cmd) >= CMD_ID_MAX)
		return -EINVAL;

	if (copy_from_user(&data, uarg, sizeof(data))) {
		ofb_err("invalid address");
		return -EFAULT;
	}

	switch (cmd) {
	case CMD_ID_SET_FPS:
		if (data.vsyncNs <= 0)
			return -EINVAL;
		crtl_update_refresh_rate(data.pid, (unsigned int)data.vsyncNs);
		break;
	case CMD_ID_BOOST_HIT:
		/* App which is not our frame boost target may request frame vsync(like systemui),
		 * just ignore hint from them! Return zero to avoid too many androd error log
		 */
		if ((data.pid != current->pid) || (data.pid != get_frame_group_ui()))
			return ret;

		if (data.stage == BOOST_FRAME_START) {
			set_frame_state(FRAME_START);
			rollover_frame_group_window(DEFAULT_FRAME_GROUP_ID);
			default_group_update_cpufreq();
		}

		if (data.stage == BOOST_OBTAIN_VIEW) {
			if (get_frame_rate() >= stune_boost[BOOST_UTIL_FRAME_RATE]) {
				set_frame_util_min(stune_boost[BOOST_UTIL_MIN_OBTAIN_VIEW], true);
				default_group_update_cpufreq();
			}
		}

		if (data.stage == BOOST_SET_RENDER_THREAD)
			set_render_thread(data.pid, data.tid);

		if (data.stage == BOOST_FRAME_TIMEOUT) {
			if ((get_frame_rate() >= stune_boost[BOOST_UTIL_FRAME_RATE]) && check_putil_over_thresh(stune_boost[BOOST_UTIL_MIN_THRESHOLD])) {
				set_frame_util_min(stune_boost[BOOST_UTIL_MIN_TIMEOUT], true);
				default_group_update_cpufreq();
			}
		}

		break;
	case CMD_ID_END_FRAME:
		/* ofb_debug("CMD_ID_END_FRAME pid:%d tid:%d", data.pid, data.tid); */
		break;
	case CMD_ID_SF_FRAME_MISSED:
		/* ofb_debug("CMD_ID_END_FRAME pid:%d tid:%d", data.pid, data.tid); */
		break;
	case CMD_ID_SF_COMPOSE_HINT:
		/* ofb_debug("CMD_ID_END_FRAME pid:%d tid:%d", data.pid, data.tid); */
		break;
	case CMD_ID_IS_HWUI_RT:
		/* ofb_debug("CMD_ID_END_FRAME pid:%d tid:%d", data.pid, data.tid); */
		break;
	case CMD_ID_SET_TASK_TAGGING:
		/* ofb_debug("CMD_ID_END_FRAME pid:%d tid:%d", data.pid, data.tid); */
		break;
	default:
		/* ret = -EINVAL; */
		break;
	}

	return ret;
}

static long fbg_add_task_to_group(void __user *uarg)
{
	struct ofb_key_thread_info info;
	unsigned int i;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&info, uarg, sizeof(struct ofb_key_thread_info))) {
		ofb_debug("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	for (i = 0; i < info.thread_num; i++)
		add_task_to_game_frame_group(info.tid[i], info.add);

	return 0;
}

static void get_frame_util_info(struct ofb_frame_util_info *info)
{
	memset(info, 0, sizeof(struct ofb_frame_util_info));
	fbg_get_frame_scale(&info->frame_scale);
	fbg_get_frame_busy(&info->frame_busy);
	if (unlikely(sysctl_frame_boost_debug)) {
		ofb_debug("%s: frame_scale=%ld, frame_busy=%ld\n",
			__func__, info->frame_scale, info->frame_busy);
	}
}

static long fbg_notify_frame_start(void __user *uarg)
{
	struct ofb_frame_util_info info;

	if (uarg == NULL)
		return -EINVAL;

	set_frame_state(FRAME_START);
	rollover_frame_group_window(GAME_FRAME_GROUP_ID);

	get_frame_util_info(&info);
	if (copy_to_user(uarg, &info, sizeof(struct ofb_frame_util_info))) {
		ofb_debug("%s: copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static bool is_ofb_extra_cmd(unsigned int cmd)
{
	return _IOC_TYPE(cmd) == OFB_EXTRA_MAGIC;
}

static long handle_ofb_extra_cmd(unsigned int cmd, void __user *uarg)
{
	switch (cmd) {
	case CMD_ID_SET_TASK_PREFERED_CLUSTER:
		return fbg_set_task_preferred_cluster(uarg);
	case CMD_ID_ADD_TASK_TO_GROUP:
		return fbg_add_task_to_group(uarg);
	case CMD_ID_NOTIFY_FRAME_START:
		return fbg_notify_frame_start(uarg);
	default:
		return -ENOTTY;
	}

	return 0;
}

static long ofb_sys_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct ofb_ctrl_data data;
	struct ofb_stune_data stune_data;
	void __user *uarg = (void __user *)arg;

	if (is_ofb_extra_cmd(cmd)) {
		return handle_ofb_extra_cmd(cmd, uarg);
	}

	if (_IOC_TYPE(cmd) != OFB_MAGIC)
		return -EINVAL;

	if (_IOC_NR(cmd) >= CMD_ID_MAX)
		return -EINVAL;


	switch (cmd) {
	case CMD_ID_BOOST_HIT:
		if (copy_from_user(&data, uarg, sizeof(data))) {
			ofb_debug("invalid address");
			return -EFAULT;
		}

		if (data.stage == BOOST_MOVE_FG) {
			set_ui_thread(data.pid, data.tid);
			set_render_thread(data.pid, data.tid);
			set_frame_state(FRAME_END);
			rollover_frame_group_window(DEFAULT_FRAME_GROUP_ID);
		}

		if (data.stage == BOOST_ADD_FRAME_TASK)
			add_rm_related_frame_task(data.pid, data.tid, data.capacity_need,
				data.related_depth, data.related_width);

		break;
	case CMD_ID_SET_SF_MSG_TRANS:
		if (copy_from_user(&data, uarg, sizeof(data))) {
			ofb_debug("invalid address");
			return -EFAULT;
		}

		if (data.stage == BOOST_MSG_TRANS_START)
			rollover_frame_group_window(SF_FRAME_GROUP_ID);

		if (data.stage == BOOST_SF_EXECUTE) {
			if (data.pid == data.tid) {
				set_sf_thread(data.pid, data.tid);
			} else {
				set_renderengine_thread(data.pid, data.tid);
			}
		}

		break;
	case CMD_ID_BOOST_STUNE:
		if (copy_from_user(&stune_data, uarg, sizeof(stune_data))) {
			ofb_debug("invalid address");
			return -EFAULT;
		}
		if ((stune_data.boost_freq >= 0) && (stune_data.boost_freq <= 100))
			stune_boost[BOOST_DEF_FREQ] = stune_data.boost_freq;

		if ((stune_data.boost_migr >= 0) && (stune_data.boost_migr <= 100))
			stune_boost[BOOST_DEF_MIGR] = stune_data.boost_migr;

		if ((stune_data.util_frame_rate >= 0) && (stune_data.util_frame_rate <= 240))
			stune_boost[BOOST_UTIL_FRAME_RATE] = stune_data.util_frame_rate;

		if ((stune_data.util_min_threshold >= 0) && (stune_data.util_min_threshold <= 1024))
			stune_boost[BOOST_UTIL_MIN_THRESHOLD] = stune_data.util_min_threshold;

		if ((stune_data.util_min_obtain_view >= 0) && (stune_data.util_min_obtain_view <= 1024))
			stune_boost[BOOST_UTIL_MIN_OBTAIN_VIEW] = stune_data.util_min_obtain_view;

		if ((stune_data.util_min_timeout >= 0) && (stune_data.util_min_timeout <= 1024))
			stune_boost[BOOST_UTIL_MIN_TIMEOUT] = stune_data.util_min_timeout;

		if ((stune_data.vutil_margin >= -16) && (stune_data.vutil_margin <= 16))
			set_frame_margin(stune_data.vutil_margin);

		break;
	case CMD_ID_BOOST_STUNE_GPU: {
		bool boost_allow = true;

		if (copy_from_user(&stune_data, uarg, sizeof(stune_data))) {
			ofb_debug("invalid address");
			return -EFAULT;
		}

		/* This frame is not using client composition if data.level is zero.
		* But we still keep client composition setting with one frame extension.
		*/
		if (check_last_compose_time(stune_data.level) && !stune_data.level)
			boost_allow = false;

		if (boost_allow && stune_data.boost_migr != INVALID_VAL)
			stune_boost[BOOST_SF_MIGR] = stune_data.boost_migr;

		if (boost_allow && stune_data.boost_freq != INVALID_VAL)
			stune_boost[BOOST_SF_FREQ] = stune_data.boost_freq;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long ofb_ctrl_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return ofb_ioctl(file, cmd, (unsigned long)(compat_ptr(arg)));
}

static long ofb_sys_ctrl_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return ofb_sys_ioctl(file, cmd, (unsigned long)(compat_ptr(arg)));
}
#endif

static int ofb_ctrl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ofb_ctrl_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations ofb_ctrl_fops = {
	.unlocked_ioctl	= ofb_ioctl,
	.open	= ofb_ctrl_open,
	.release	= ofb_ctrl_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ofb_ctrl_compat_ioctl,
#endif
};

static const struct file_operations ofb_sys_ctrl_fops = {
	.unlocked_ioctl	= ofb_sys_ioctl,
	.open	= ofb_ctrl_open,
	.release	= ofb_ctrl_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ofb_sys_ctrl_compat_ioctl,
#endif
};

static ssize_t proc_stune_boost_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	int i = 0, err;
	char buffer[256];
	char *temp_str, *token;
	char str_val[BOOST_MAX_TYPE][8];

	memset(buffer, 0, sizeof(buffer));

	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	temp_str = strstrip(buffer);
	while ((token = strsep(&temp_str, " ")) && *token && (i < BOOST_MAX_TYPE)) {
		int boost_val = 0;

		strlcpy(str_val[i], token, sizeof(str_val[i]));
		err = kstrtoint(strstrip(str_val[i]), 10, &boost_val);
		if (err)
			ofb_err("failed to write boost param (i=%d str=%s)\n", i, str_val[i]);

		if (boost_val >= 0 && i < BOOST_MAX_TYPE) {
			if (i == BOOST_UTIL_FRAME_RATE) {
			    stune_boost[i] = max(0, min(boost_val, 240));
			} else if ((i == BOOST_UTIL_MIN_THRESHOLD) || (i == BOOST_UTIL_MIN_OBTAIN_VIEW) || (i == BOOST_UTIL_MIN_TIMEOUT)) {
			    stune_boost[i] = max(0, min(boost_val, 1024));
			} else {
			    stune_boost[i] = min(boost_val, 100);
			}
			ofb_debug("write boost param=%d, i=%d\n", boost_val, i);
		}

		i++;
	}

	return count;
}

static ssize_t proc_stune_boost_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[256];
	int i;
	size_t len = 0;

	for (i = 0; i < BOOST_MAX_TYPE; ++i)
		len += snprintf(buffer + len, sizeof(buffer) - len, "%d ", stune_boost[i]);

	len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct file_operations ofb_stune_boost_fops = {
	.write		= proc_stune_boost_write,
	.read		= proc_stune_boost_read,
};

static int info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, info_show, NULL);
}

static const struct file_operations ofb_frame_group_info_fops = {
	.open	= info_proc_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

#define GLOBAL_SYSTEM_UID KUIDT_INIT(1000)
#define GLOBAL_SYSTEM_GID KGIDT_INIT(1000)
int frame_ioctl_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pentry;

	frame_boost_proc = proc_mkdir(FRAMEBOOST_PROC_NODE, NULL);

	pentry = proc_create("ctrl", S_IRWXUGO, frame_boost_proc, &ofb_ctrl_fops);
	if (!pentry)
		goto ERROR_INIT;

	pentry = proc_create("sys_ctrl", (S_IRWXU|S_IRWXG), frame_boost_proc, &ofb_sys_ctrl_fops);
	if (!pentry) {
		goto ERROR_INIT;
	} else {
		pentry->uid = GLOBAL_SYSTEM_UID;
		pentry->gid = GLOBAL_SYSTEM_GID;
	}

	pentry = proc_create("stune_boost", (S_IRUGO|S_IWUSR|S_IWGRP), frame_boost_proc, &ofb_stune_boost_fops);
	if (!pentry)
		goto ERROR_INIT;

	pentry = proc_create("info", S_IRUGO, frame_boost_proc, &ofb_frame_group_info_fops);
	if (!pentry)
		goto ERROR_INIT;

	return ret;

ERROR_INIT:
	remove_proc_entry(FRAMEBOOST_PROC_NODE, NULL);
	return -ENOENT;
}
