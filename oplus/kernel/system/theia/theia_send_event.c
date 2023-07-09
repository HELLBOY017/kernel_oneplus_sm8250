// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2030 Oplus. All rights reserved.
 */

#include <linux/types.h>     /* for types, pid_t */
#include <linux/kernel.h>    /* for snprintf */
#include <linux/version.h>   /* for LINUX_VERSION_CODE, KERNEL_VERSION */
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "theia_kevent_kernel.h" /* for SendTheiaKevent */
#include "../include/theia_send_event.h"

struct theia_send_kevent {
	struct work_struct work;
	THEIA_EVENT_ID id;
	unsigned int log_info;
	char *extra_info;
};

#define SEND_EVENT_DEBUG_PRINTK(a, arg...)\
	do {\
		printk("[theia_send_event]: " a, ##arg);\
	} while (0)

#define EVENT_BUF_LEN 256

#define THEIA_SEND_EVENT_TAG "CriticalLog"
#define THEIA_SEND_EVENT_ID  "Theia"

void theia_send_event(THEIA_EVENT_ID id, unsigned int log_info,
	pid_t pid, const char *extra_info)
{
	char event_buf[EVENT_BUF_LEN] = {0};

	SEND_EVENT_DEBUG_PRINTK("%s in, send event:%x to theia native daemon\n", __func__, id);
	snprintf(event_buf, EVENT_BUF_LEN, "event_id: %d. loginfo: %d. pid: %d. "
		"extrainfo: %s", id, log_info, pid, extra_info != NULL ? extra_info : "null");

	SendTheiaKevent(THEIA_ATTR_TYPE_SEND_EVENT, THEIA_SEND_EVENT_TAG,
		THEIA_SEND_EVENT_ID, event_buf);
}
EXPORT_SYMBOL_GPL(theia_send_event);

static void theia_kevent_callback(struct work_struct *work)
{
	struct theia_send_kevent *kevent = container_of(work, struct theia_send_kevent, work);
	theia_send_event(kevent->id, kevent->log_info, 0, kevent->extra_info);
	kfree(kevent->extra_info);
	kfree(kevent);
}

int theia_send_event_atomic(THEIA_EVENT_ID id, unsigned int log_info, const char *extra_info)
{
	struct theia_send_kevent *kevent;

	kevent = kzalloc(sizeof(*kevent), GFP_ATOMIC);
	if (!kevent)
		return -ENOMEM;

	kevent->id = id;
	kevent->log_info = log_info;
	kevent->extra_info = kmemdup(extra_info, strlen(extra_info) + 1, GFP_ATOMIC);
	INIT_WORK(&kevent->work, theia_kevent_callback);
	schedule_work(&kevent->work);
	return 0;
}
EXPORT_SYMBOL_GPL(theia_send_event_atomic);

void __maybe_unused theia_send_event_init(void)
{
	SEND_EVENT_DEBUG_PRINTK("%s called\n", __func__);
}

void __maybe_unused theia_send_event_exit(void)
{
	SEND_EVENT_DEBUG_PRINTK("%s called\n", __func__);
}
