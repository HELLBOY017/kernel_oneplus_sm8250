// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[VENDOR]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>

#include "ufcs_core.h"
#include "ufcs_class.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"

#define INIT_VND_MSG_HEAD(__head, __index)	\
do {						\
	(__head)->addr = UFCS_DEVICE_SOURCE;	\
	(__head)->version = UFCS_VER_CURR;	\
	(__head)->type = UFCS_VENDOR_MSG;	\
	(__head)->index = __index;		\
} while (0)

static const char *const oplus_msg_name[] = {
	[OPLUS_MSG_GET_EMARK_INFO]		= "get_emark_info",
	[OPLUS_MSG_RESP_EMARK_INFO]		= "resp_emark_info",
	[OPLUS_MSG_GET_POWER_INFO_EXT]		= "get_power_info_ext",
	[OPLUS_MSG_RESP_POWER_INFO_EXT]		= "resp_power_info_ext",
};

static const char *const oplus_emark_info_str[] = {
	[UFCS_OPLUS_EMARK_INFO_ERR]		= "error",
	[UFCS_OPLUS_EMARK_INFO_OTHER_CABLE]	= "other",
	[UFCS_OPLUS_EMARK_INFO_OPLUS_CABLE]	= "oplus",
	[UFCS_OPLUS_EMARK_INFO_OTHER]		= "unknown",
};

static const char *const oplus_power_info_type_str[] = {
	[UFCS_OPLUS_POWER_INFO_UFCS]		= "UFCS",
	[UFCS_OPLUS_POWER_INFO_PPS]		= "PPS",
	[UFCS_OPLUS_POWER_INFO_SVOOC]		= "SVOOC",
};

const char *ufcs_get_oplus_msg_name(enum ufcs_oplus_vnd_msg_type type)
{
	switch (type) {
	case OPLUS_MSG_GET_EMARK_INFO:
	case OPLUS_MSG_RESP_EMARK_INFO:
	case OPLUS_MSG_GET_POWER_INFO_EXT:
	case OPLUS_MSG_RESP_POWER_INFO_EXT:
		break;
	default:
		return "invalid";
	}
	return oplus_msg_name[type];
}

const char *ufcs_get_oplus_emark_info_str(enum ufcs_oplus_vnd_emark_info info)
{
	switch (info) {
	case UFCS_OPLUS_EMARK_INFO_ERR:
	case UFCS_OPLUS_EMARK_INFO_OTHER_CABLE:
	case UFCS_OPLUS_EMARK_INFO_OPLUS_CABLE:
	case UFCS_OPLUS_EMARK_INFO_OTHER:
		break;
	default:
		return "invalid";
	}
	return oplus_emark_info_str[info];
}
EXPORT_SYMBOL(ufcs_get_oplus_emark_info_str);

const char *ufcs_get_oplus_power_info_type_str(enum ufcs_oplus_vnd_power_info_type type)
{
	switch (type) {
	case UFCS_OPLUS_POWER_INFO_UFCS:
	case UFCS_OPLUS_POWER_INFO_PPS:
	case UFCS_OPLUS_POWER_INFO_SVOOC:
		break;
	default:
		return "invalid";
	}
	return oplus_power_info_type_str[type];
}
EXPORT_SYMBOL(ufcs_get_oplus_power_info_type_str);

int ufcs_oplus_send_ctrl_msg(struct ufcs_class *class, enum ufcs_oplus_vnd_msg_type type)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs oplus %s ctrl msg buf error\n", ufcs_get_oplus_msg_name(type));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_VND_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->vendor_msg.vnd_id = UFCS_OPLUS_DEV_ID;
	msg->vendor_msg.length = 1;
	msg->vendor_msg.vnd_msg.type = type;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send oplus %s ctrl msg error, rc=%d\n",
			 ufcs_get_oplus_msg_name(type), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

bool ufcs_is_supported_oplus_ctrl_msg(struct ufcs_vendor_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return false;
	}

	switch (msg->vnd_msg.type) {
	case OPLUS_MSG_GET_EMARK_INFO:
	case OPLUS_MSG_GET_POWER_INFO_EXT:
		return true;
	default:
		return false;
	}

	return false;
}

bool ufcs_is_supported_oplus_data_msg(struct ufcs_vendor_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return false;
	}

	switch (msg->vnd_msg.type) {
	case OPLUS_MSG_RESP_EMARK_INFO:
	case OPLUS_MSG_RESP_POWER_INFO_EXT:
		return true;
	default:
		return false;
	}

	return false;
}

static void ufcs_recv_oplus_data_msg_emark_info(struct ufcs_oplus_data_msg_resp_emark_info *emark_info)
{
	ufcs_err("emark_info=%llx\n", emark_info->data);
	emark_info->data = be64_to_cpu(emark_info->data);
	ufcs_err("emark_info=%llx\n", emark_info->data);
}

static void ufcs_recv_data_msg_power_info_ext(struct ufcs_oplus_data_msg_resp_power_info_ext *power_info)
{
	int i;

	for (i = 0; i < UFCS_OPLUS_VND_POWER_INFO_MAX; i++)
		power_info->data[i] = be64_to_cpu(power_info->data[i]);
}

int ufcs_oplus_data_msg_init(struct ufcs_vendor_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return -EINVAL;
	}

	switch (msg->vnd_msg.type) {
	case OPLUS_MSG_RESP_EMARK_INFO:
		ufcs_recv_oplus_data_msg_emark_info(&msg->vnd_msg.data_msg.emark_info);
		break;
	case OPLUS_MSG_RESP_POWER_INFO_EXT:
		ufcs_recv_data_msg_power_info_ext(&msg->vnd_msg.data_msg.power_info);
		break;
	default:
		break;
	}

	return 0;
}
