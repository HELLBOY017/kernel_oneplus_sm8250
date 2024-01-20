// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[DATA]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>

#include "ufcs_core.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"

#define INIT_DATA_MSG_HEAD(__head, __index)	\
do {						\
	(__head)->addr = UFCS_DEVICE_SOURCE;	\
	(__head)->version = UFCS_VER_CURR;	\
	(__head)->type = UFCS_DATA_MSG;		\
	(__head)->index = __index;		\
} while (0)

static const char *const data_msg_name[] = {
	[DATA_MSG_OUTPUT_CAPABILITIES]	= "output_capabilities",
	[DATA_MSG_REQUEST]		= "request",
	[DATA_MSG_SOURCE_INFO]		= "source_info",
	[DATA_MSG_SINK_INFO]		= "sink_info",
	[DATA_MSG_CABLE_INFO]		= "cable_info",
	[DATA_MSG_DEVICE_INFO]		= "device_info",
	[DATA_MSG_ERROR_INFO]		= "error_info",
	[DATA_MSG_CONFIG_WATCHDOG]	= "config_watchdog",
	[DATA_MSG_REFUSE]		= "refuse",
	[DATA_MSG_VERIFY_REQUEST]	= "verify_request",
	[DATA_MSG_VERIFY_RESPONSE]	= "verify_response",
	[DATA_MSG_POWER_CHANGE]		= "power_change",
	[DATA_MSG_TEST_REQUEST]		= "test_request",
};

static const char *const refuse_reason_name[] = {
	[REFUSE_UNKNOWN_CMD]		= "unknown command",
	[REFUSE_NOT_SUPPORT_CMD]	= "not supported command",
	[REFUSE_DEVICE_BUSY]		= "device busy",
	[REFUSE_OUT_OF_RANGE]		= "vol/curr/power out of range",
	[REFUSE_OTHER]			= "other",
};

const char *ufcs_get_data_msg_name(enum ufcs_data_msg_type type)
{
	if (type < 0 || type > DATA_MSG_TEST_REQUEST)
		return "invalid";
	return data_msg_name[type];
}

const char *ufcs_get_refuse_reason_name(enum ufcs_refuse_reason reason)
{
	if (reason < REFUSE_UNKNOWN_CMD || reason > REFUSE_OTHER)
		return "invalid";
	return refuse_reason_name[reason];
}

int ufcs_send_data_msg_output_cap(struct ufcs_class *class,
	struct ufcs_data_msg_output_capabilities *output_cap)
{
	ufcs_err("sink device not support send %s msg\n",
		 ufcs_get_data_msg_name(DATA_MSG_OUTPUT_CAPABILITIES));
	return -ENOTSUPP;
}

int __ufcs_send_data_msg_request(struct ufcs_class *class,
	struct ufcs_data_msg_request *request)
{
	struct ufcs_msg *msg;
	int rc;

	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return -EINVAL;
	}
	if (request == NULL) {
		ufcs_err("request is NULL\n");
		return -EINVAL;
	}

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_REQUEST));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_REQUEST;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_request);
	msg->data_msg.request.request = request->request;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_REQUEST), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_request(struct ufcs_class *class, u8 index, u32 vol, u32 curr)
{
	struct ufcs_data_msg_request request;

	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return -EINVAL;
	}

	request.request = UFCS_REQUEST_DATA(index, vol, curr);
	return __ufcs_send_data_msg_request(class, &request);
}

int ufcs_send_data_msg_src_info(struct ufcs_class *class,
	struct ufcs_data_msg_source_info *src_info)
{
	ufcs_err("sink device not support send %s msg\n",
		 ufcs_get_data_msg_name(DATA_MSG_SOURCE_INFO));
	return -ENOTSUPP;
}

int ufcs_send_data_msg_sink_info(struct ufcs_class *class,
	struct ufcs_data_msg_sink_info *sink_info)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_SINK_INFO));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_SINK_INFO;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_sink_info);
	msg->data_msg.sink_info.info = sink_info->info;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_SINK_INFO), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_cable_info(struct ufcs_class *class,
	struct ufcs_data_msg_cable_info *cable_info)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_CABLE_INFO));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_CABLE_INFO;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_cable_info);
	msg->data_msg.cable_info.info = cable_info->info;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_CABLE_INFO), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_dev_info(struct ufcs_class *class,
	struct ufcs_data_msg_device_info *dev_info)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_DEVICE_INFO));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_DEVICE_INFO;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_device_info);
	msg->data_msg.dev_info.info = dev_info->info;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_DEVICE_INFO), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_err_info(struct ufcs_class *class,
	struct ufcs_data_msg_error_info *err_info)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_ERROR_INFO));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_ERROR_INFO;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_error_info);
	msg->data_msg.err_info.info = err_info->info;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_ERROR_INFO), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_config_wd(struct ufcs_class *class,
	struct ufcs_data_msg_config_watchdog *config_wd)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_CONFIG_WATCHDOG));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_CONFIG_WATCHDOG;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_config_watchdog);
	msg->data_msg.config_wd.over_time_ms = config_wd->over_time_ms;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_CONFIG_WATCHDOG), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

static int __ufcs_send_data_msg_refuse(struct ufcs_class *class,
	struct ufcs_data_msg_refuse *refuse)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_REFUSE));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_REFUSE;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_refuse);
	msg->data_msg.refuse.data = refuse->data;

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_REFUSE), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_refuse(struct ufcs_class *class, struct ufcs_msg *msg,
			      enum ufcs_refuse_reason reason)
{
	struct ufcs_data_msg_refuse refuse;
	u8 cmd;

	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		cmd = msg->ctrl_msg.command;
		break;
	case UFCS_DATA_MSG:
		cmd = msg->data_msg.command;
		break;
	case UFCS_VENDOR_MSG:
		cmd = 0;
		break;
	default:
		cmd = 0;
		reason = REFUSE_UNKNOWN_CMD;
		break;
	}

	refuse.data = UFCS_REFUSE_INFO_DATA(msg->head.index, msg->head.type, cmd, reason);

	return __ufcs_send_data_msg_refuse(class, &refuse);
}

int ufcs_send_data_msg_verify_request(struct ufcs_class *class,
	struct ufcs_data_msg_verify_request *verify_request)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_VERIFY_REQUEST));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_VERIFY_REQUEST;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_verify_request);
	msg->data_msg.verify_request.index = verify_request->index;
	memcpy(msg->data_msg.verify_request.random_data, verify_request->random_data,
	       UFCS_VERIFY_RANDOM_DATA_SIZE);

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_VERIFY_REQUEST), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_verify_response(struct ufcs_class *class,
	struct ufcs_data_msg_verify_response *verify_response)
{
	struct ufcs_msg *msg;
	int rc;

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs %s data msg buf error\n",
			 ufcs_get_data_msg_name(DATA_MSG_VERIFY_RESPONSE));
		return -ENOMEM;
	}

	msg->magic = UFCS_MSG_MAGIC;
	INIT_DATA_MSG_HEAD(&msg->head, class->sender.msg_number_counter);
	msg->data_msg.command = DATA_MSG_VERIFY_RESPONSE;
	msg->data_msg.length = sizeof(struct ufcs_data_msg_verify_response);
	memcpy(msg->data_msg.verify_response.encrypted_data, verify_response->encrypted_data,
	       UFCS_VERIFY_ENCRYPTED_DATA_SIZE);
	memcpy(msg->data_msg.verify_response.random_data, verify_response->random_data,
	       UFCS_VERIFY_RANDOM_DATA_SIZE);

	rc = ufcs_send_msg(class, msg);
	if (rc < 0)
		ufcs_err("send %s data msg error, rc=%d\n",
			 ufcs_get_data_msg_name(DATA_MSG_VERIFY_RESPONSE), rc);
	devm_kfree(&class->ufcs->dev, msg);

	return rc;
}

int ufcs_send_data_msg_power_change(struct ufcs_class *class,
	struct ufcs_data_msg_power_change *power_change)
{
	ufcs_err("sink device not support send %s msg\n",
		 ufcs_get_data_msg_name(DATA_MSG_POWER_CHANGE));
	return -ENOTSUPP;
}

int ufcs_send_data_msg_test_request(struct ufcs_class *class,
	struct ufcs_data_msg_test_request *test_request)
{
	ufcs_err("sink device not support send %s msg\n",
		 ufcs_get_data_msg_name(DATA_MSG_TEST_REQUEST));
	return -ENOTSUPP;
}

static void ufcs_recv_data_msg_output_cap(struct ufcs_data_msg_output_capabilities *output_cap)
{
	int i;

	for (i = 0; i < UFCS_OUTPUT_MODE_MAX; i++)
		output_cap->output_mode[i] = be64_to_cpu(output_cap->output_mode[i]);
}

static void ufcs_recv_data_msg_request(struct ufcs_data_msg_request *request)
{
	request->request = be64_to_cpu(request->request);
}

static void ufcs_recv_data_msg_src_info(struct ufcs_data_msg_source_info *src_info)
{
	src_info->info = be64_to_cpu(src_info->info);
}

static void ufcs_recv_data_msg_sink_info(struct ufcs_data_msg_sink_info *sink_info)
{
	sink_info->info = be64_to_cpu(sink_info->info);
}

static void ufcs_recv_data_msg_cable_info(struct ufcs_data_msg_cable_info *cable_info)
{
	cable_info->info = be64_to_cpu(cable_info->info);
}

static void ufcs_recv_data_msg_dev_info(struct ufcs_data_msg_device_info *dev_info)
{
	dev_info->info = be64_to_cpu(dev_info->info);
}

static void ufcs_recv_data_msg_err_info(struct ufcs_data_msg_error_info *err_info)
{
	err_info->info = be32_to_cpu(err_info->info);
}

static void ufcs_recv_data_msg_config_wd(struct ufcs_data_msg_config_watchdog *config_wd)
{
	config_wd->over_time_ms = be16_to_cpu(config_wd->over_time_ms);
}

static void ufcs_recv_data_msg_refuse(struct ufcs_data_msg_refuse *refuse)
{
	refuse->data = be32_to_cpu(refuse->data);
}

static void ufcs_recv_or_pack_data_msg_verify_request(struct ufcs_data_msg_verify_request *verify_request)
{
	int i;
	u8 buf[UFCS_VERIFY_RANDOM_DATA_SIZE];

	memcpy(buf, verify_request->random_data, UFCS_VERIFY_RANDOM_DATA_SIZE);
	for (i = 0; i < UFCS_VERIFY_RANDOM_DATA_SIZE; i++)
		verify_request->random_data[i] = buf[UFCS_VERIFY_RANDOM_DATA_SIZE - 1 - i];
}

static void ufcs_recv_or_pack_data_msg_verify_response(struct ufcs_data_msg_verify_response *verify_response)
{
	int i;
	u8 encrypted_data_buf[UFCS_VERIFY_ENCRYPTED_DATA_SIZE];
	u8 random_data_buf[UFCS_VERIFY_RANDOM_DATA_SIZE];

	memcpy(encrypted_data_buf, verify_response->encrypted_data, UFCS_VERIFY_ENCRYPTED_DATA_SIZE);
	for (i = 0; i < UFCS_VERIFY_ENCRYPTED_DATA_SIZE; i++)
		verify_response->encrypted_data[i] = encrypted_data_buf[UFCS_VERIFY_ENCRYPTED_DATA_SIZE - 1 - i];
	memcpy(random_data_buf, verify_response->random_data, UFCS_VERIFY_RANDOM_DATA_SIZE);
	for (i = 0; i < UFCS_VERIFY_RANDOM_DATA_SIZE; i++)
		verify_response->random_data[i] = random_data_buf[UFCS_VERIFY_RANDOM_DATA_SIZE - 1 - i];
}

static void ufcs_recv_data_msg_power_change(struct ufcs_data_msg_power_change *power_change)
{
	u32 data;
	u8 *buf = (u8 *)power_change->data;
	int i;

	for (i = UFCS_OUTPUT_MODE_MAX - 1; i > 0; i--) {
		data = (((u32)(buf[i])) << 16) + (((u32)(buf[i + 1])) << 8) + buf[i + 2];
		power_change->data[i] = data;
	}
}

static void ufcs_recv_data_msg_test_request(struct ufcs_data_msg_test_request *test_request)
{
	test_request->data = be16_to_cpu(test_request->data);
}

static void ufcs_pack_data_msg_output_cap(struct ufcs_data_msg_output_capabilities *output_cap)
{
	int i;

	for (i = 0; i < UFCS_OUTPUT_MODE_MAX; i++)
		output_cap->output_mode[i] = cpu_to_be64(output_cap->output_mode[i]);
}

static void ufcs_pack_data_msg_request(struct ufcs_data_msg_request *request)
{
	request->request = cpu_to_be64(request->request);
}

static void ufcs_pack_data_msg_src_info(struct ufcs_data_msg_source_info *src_info)
{
	src_info->info = cpu_to_be64(src_info->info);
}

static void ufcs_pack_data_msg_sink_info(struct ufcs_data_msg_sink_info *sink_info)
{
	sink_info->info = cpu_to_be64(sink_info->info);
}

static void ufcs_pack_data_msg_cable_info(struct ufcs_data_msg_cable_info *cable_info)
{
	cable_info->info = cpu_to_be64(cable_info->info);
}

static void ufcs_pack_data_msg_dev_info(struct ufcs_data_msg_device_info *dev_info)
{
	dev_info->info = cpu_to_be64(dev_info->info);
}

static void ufcs_pack_data_msg_err_info(struct ufcs_data_msg_error_info *err_info)
{
	err_info->info = cpu_to_be32(err_info->info);
}

static void ufcs_pack_data_msg_config_wd(struct ufcs_data_msg_config_watchdog *config_wd)
{
	config_wd->over_time_ms = cpu_to_be16(config_wd->over_time_ms);
}

static void ufcs_pack_data_msg_refuse(struct ufcs_data_msg_refuse *refuse)
{
	refuse->data = cpu_to_be32(refuse->data);
}

static void ufcs_pack_data_msg_power_change(struct ufcs_data_msg_power_change *power_change)
{
	u32 data;
	u8 *buf = (u8 *)power_change->data;
	int i;

	for (i = 0; i < UFCS_OUTPUT_MODE_MAX; i++) {
		data = power_change->data[i];
		buf[i] = (data >> 16) & 0xff;
		buf[i + 1] = (data >> 8) & 0xff;
		buf[i + 2] = data & 0xff;
	}
}

static void ufcs_pack_data_msg_test_request(struct ufcs_data_msg_test_request *test_request)
{
	test_request->data = cpu_to_be16(test_request->data);
}

bool ufcs_is_supported_data_msg(struct ufcs_data_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return false;
	}

	switch (msg->command) {
	case DATA_MSG_OUTPUT_CAPABILITIES:
	case DATA_MSG_SOURCE_INFO:
	case DATA_MSG_CABLE_INFO:
	case DATA_MSG_DEVICE_INFO:
	case DATA_MSG_ERROR_INFO:
	case DATA_MSG_REFUSE:
	case DATA_MSG_VERIFY_REQUEST:
	case DATA_MSG_VERIFY_RESPONSE:
	case DATA_MSG_POWER_CHANGE:
	case DATA_MSG_TEST_REQUEST:
		return true;
	default:
		return false;
	}

	return false;
}

int ufcs_data_msg_init(struct ufcs_data_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return -EINVAL;
	}

	switch (msg->command) {
	case DATA_MSG_OUTPUT_CAPABILITIES:
		ufcs_recv_data_msg_output_cap(&msg->output_cap);
		break;
	case DATA_MSG_REQUEST:
		ufcs_recv_data_msg_request(&msg->request);
		break;
	case DATA_MSG_SOURCE_INFO:
		ufcs_recv_data_msg_src_info(&msg->src_info);
		break;
	case DATA_MSG_SINK_INFO:
		ufcs_recv_data_msg_sink_info(&msg->sink_info);
		break;
	case DATA_MSG_CABLE_INFO:
		ufcs_recv_data_msg_cable_info(&msg->cable_info);
		break;
	case DATA_MSG_DEVICE_INFO:
		ufcs_recv_data_msg_dev_info(&msg->dev_info);
		break;
	case DATA_MSG_ERROR_INFO:
		ufcs_recv_data_msg_err_info(&msg->err_info);
		break;
	case DATA_MSG_CONFIG_WATCHDOG:
		ufcs_recv_data_msg_config_wd(&msg->config_wd);
		break;
	case DATA_MSG_REFUSE:
		ufcs_recv_data_msg_refuse(&msg->refuse);
		break;
	case DATA_MSG_VERIFY_REQUEST:
		ufcs_recv_or_pack_data_msg_verify_request(&msg->verify_request);
		break;
	case DATA_MSG_VERIFY_RESPONSE:
		ufcs_recv_or_pack_data_msg_verify_response(&msg->verify_response);
		break;
	case DATA_MSG_POWER_CHANGE:
		ufcs_recv_data_msg_power_change(&msg->power_change);
		break;
	case DATA_MSG_TEST_REQUEST:
		ufcs_recv_data_msg_test_request(&msg->test_request);
		break;
	default:
		break;
	}

	return 0;
}

int ufcs_data_msg_pack(struct ufcs_data_msg *msg)
{
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		return -EINVAL;
	}

	switch (msg->command) {
	case DATA_MSG_OUTPUT_CAPABILITIES:
		ufcs_pack_data_msg_output_cap(&msg->output_cap);
		break;
	case DATA_MSG_REQUEST:
		ufcs_pack_data_msg_request(&msg->request);
		break;
	case DATA_MSG_SOURCE_INFO:
		ufcs_pack_data_msg_src_info(&msg->src_info);
		break;
	case DATA_MSG_SINK_INFO:
		ufcs_pack_data_msg_sink_info(&msg->sink_info);
		break;
	case DATA_MSG_CABLE_INFO:
		ufcs_pack_data_msg_cable_info(&msg->cable_info);
		break;
	case DATA_MSG_DEVICE_INFO:
		ufcs_pack_data_msg_dev_info(&msg->dev_info);
		break;
	case DATA_MSG_ERROR_INFO:
		ufcs_pack_data_msg_err_info(&msg->err_info);
		break;
	case DATA_MSG_CONFIG_WATCHDOG:
		ufcs_pack_data_msg_config_wd(&msg->config_wd);
		break;
	case DATA_MSG_REFUSE:
		ufcs_pack_data_msg_refuse(&msg->refuse);
		break;
	case DATA_MSG_VERIFY_REQUEST:
		ufcs_recv_or_pack_data_msg_verify_request(&msg->verify_request);
		break;
	case DATA_MSG_VERIFY_RESPONSE:
		ufcs_recv_or_pack_data_msg_verify_response(&msg->verify_response);
		break;
	case DATA_MSG_POWER_CHANGE:
		ufcs_pack_data_msg_power_change(&msg->power_change);
		break;
	case DATA_MSG_TEST_REQUEST:
		ufcs_pack_data_msg_test_request(&msg->test_request);
		break;
	default:
		break;
	}

	return 0;
}
