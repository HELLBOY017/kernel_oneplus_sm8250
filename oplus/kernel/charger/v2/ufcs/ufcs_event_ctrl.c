// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[CTRL]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>

#include "ufcs_core.h"
#include "ufcs_class.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"

#define INIT_CTRL_MSG_HEAD(__head, __index)	\
do {						\
	(__head)->addr = UFCS_DEVICE_SOURCE;	\
	(__head)->version = UFCS_VER_CURR;	\
	(__head)->type = UFCS_CTRL_MSG;		\
	(__head)->index = __index;		\
} while (0)

static const char *const ctrl_msg_name[] = {
	[CTRL_MSG_PING]				= "ping",
	[CTRL_MSG_ACK]				= "ack",
	[CTRL_MSG_NCK]				= "nck",
	[CTRL_MSG_ACCEPT]			= "accept",
	[CTRL_MSG_SOFT_RESET]			= "soft reset",
	[CTRL_MSG_POWER_READY]			= "power ready",
	[CTRL_MSG_GET_OUTPUT_CAPABILITIES]	= "get output capabilities",
	[CTRL_MSG_GET_SOURCE_INFO]		= "get source info",
	[CTRL_MSG_GET_SINK_INFO]		= "get sink info",
	[CTRL_MSG_GET_CABLE_INFO]		= "get cable info",
	[CTRL_MSG_GET_DEVICE_INFO]		= "get device info",
	[CTRL_MSG_GET_ERROR_INFO]		= "get error info",
	[CTRL_MSG_DETECT_CABLE_INFO]		= "detect cable info",
	[CTRL_MSG_START_CABLE_DETECT]		= "start cable detect",
	[CTRL_MSG_END_CABLE_DETECT]		= "end cable detect",
	[CTRL_MSG_EXIT_UFCS_MODE]		= "exit ufcs mode",
};

const char *ufcs_get_ctrl_msg_name(enum ufcs_ctrl_msg_type type)
{
	if (type < 0 || type > CTRL_MSG_EXIT_UFCS_MODE)
		return "invalid";
	return ctrl_msg_name[type];
}

int ufcs_send_ctrl_msg(struct ufcs_class *class, enum ufcs_ctrl_msg_type type)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s ctrl msg buf error\n", ufcs_get_ctrl_msg_name(type));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_CTRL_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->ctrl_msg.command = type;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s ctrl msg error, rc=%d\n",
			 ufcs_get_ctrl_msg_name(type), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

bool ufcs_is_supported_ctrl_msg(struct ufcs_ctrl_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return false;
	}

	switch (msg->command) {
	case CTRL_MSG_PING:
	case CTRL_MSG_ACK:
	case CTRL_MSG_NCK:
	case CTRL_MSG_ACCEPT:
	case CTRL_MSG_SOFT_RESET:
	case CTRL_MSG_POWER_READY:
	case CTRL_MSG_GET_SINK_INFO:
	case CTRL_MSG_GET_DEVICE_INFO:
	case CTRL_MSG_GET_ERROR_INFO:
	case CTRL_MSG_DETECT_CABLE_INFO:
	case CTRL_MSG_START_CABLE_DETECT:
	case CTRL_MSG_END_CABLE_DETECT:
	case CTRL_MSG_EXIT_UFCS_MODE:
		return true;
	default:
		return false;
	}

	return false;
}

bool ufcs_is_ack_nck_msg(struct ufcs_ctrl_msg *msg)
{
	if (msg->command == CTRL_MSG_ACK || msg->command == CTRL_MSG_NCK)
		return true;
	return false;
}

bool ufcs_recv_ack_nck_msg(struct ufcs_class *class, struct ufcs_ctrl_msg *msg)
{
	struct ufcs_msg_sender *sender;

	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return false;
	}
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return false;
	}
	if (msg->command != CTRL_MSG_ACK && msg->command != CTRL_MSG_NCK)
		return false;

	sender = &class->sender;
	stop_ack_receive_timer(class);
	if (sender->status == MSG_WAIT_ACK) {
		if (msg->command == CTRL_MSG_ACK) {
			sender->status = MSG_SEND_OK;
			complete(&sender->ack);
		} else {
			sender->status = MSG_NCK;
			ufcs_err("recv nck");
			kthread_queue_work(class->worker, &class->msg_send_work);
		}
		return true;
	}

	return false;
}
