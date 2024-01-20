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
#include <linux/input.h>
#include <linux/slab.h>

#include "ua_ioctl_common.h"

#define TOUCHBOOST_PROC_NODE "oplus_touch_boost"

static struct proc_dir_entry *touch_boost_proc;

static int cur_event;
static spinlock_t	cur_event_lock;


static void touchboost_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	unsigned long flags;

	/* Only process touch down(value=1) and up(value=0) */
	if (type == EV_KEY && code == BTN_TOUCH && (value == 1 || value == 0)) {
		spin_lock_irqsave(&cur_event_lock, flags);
		cur_event = value;
		spin_unlock_irqrestore(&cur_event_lock, flags);
	}
}

static int touchboost_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "touch_boost_cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void touchboost_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

/* Only match touchpanel */
static bool touchboost_input_match(struct input_handler *handler, struct
	input_dev *dev)
{
	const char *dev_match_name = "touchpanel";

	if(strncmp(dev_match_name, dev->name, strlen(dev_match_name)) == 0)
		return true;

	return false;
}

static const struct input_device_id touchboost_ids[] = {
	/* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y)
		},
	},
	/* touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y)
		},
	},
	/* Keypad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};

static struct input_handler touchboost_input_handler = {
	.event		= touchboost_input_event,
	.match		= touchboost_input_match,
	.connect	= touchboost_input_connect,
	.disconnect	= touchboost_input_disconnect,
	.name		= "touch_boost",
	.id_table	= touchboost_ids,
};


/*********************************
 * touch boost ioctl proc
 *********************************/
static int touch_info_show(struct seq_file *m, void *v)
{
	unsigned long flags;

	spin_lock_irqsave(&cur_event_lock, flags);
	seq_printf(m, "%d\n", cur_event);
	spin_unlock_irqrestore(&cur_event_lock, flags);

	return 0;
}

static int info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, touch_info_show, NULL);
}

static const struct file_operations otb_touch_info_fops = {
	.open	= info_proc_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

int touch_ioctl_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pentry;

	touch_boost_proc = proc_mkdir(TOUCHBOOST_PROC_NODE, NULL);

	pentry = proc_create("touch_info", S_IRUGO, touch_boost_proc, &otb_touch_info_fops);
	if (!pentry)
		goto ERROR_INIT;

	ret = input_register_handler(&touchboost_input_handler);

	spin_lock_init(&cur_event_lock);

	return 0;

ERROR_INIT:
	remove_proc_entry(TOUCHBOOST_PROC_NODE, NULL);
	return -ENOENT;
}

int touch_ioctl_exit(void)
{
	int ret = 0;

	if (touch_boost_proc != NULL)
		remove_proc_entry(TOUCHBOOST_PROC_NODE, NULL);

	return ret;
}
