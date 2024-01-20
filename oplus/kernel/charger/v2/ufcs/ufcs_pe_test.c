// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[PE]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/list.h>

#include "ufcs_policy_engine.h"
#include "ufcs_core.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"
#include "ufcs_intf.h"

static int ufcs_pe_test_ctrl_request_handle(struct ufcs_class *class, enum ufcs_ctrl_msg_type type)
{
	u64 pdo[UFCS_OUTPUT_MODE_MAX];
	u64 src_info;
	int rc;

	switch (type) {
	case CTRL_MSG_GET_OUTPUT_CAPABILITIES:
		rc = ufcs_get_pdo_info(class, pdo, UFCS_OUTPUT_MODE_MAX);
		if (rc < 0)
			ufcs_err("test mode get pdo info error, rc=%d\n", rc);
		break;
	case CTRL_MSG_GET_SOURCE_INFO:
		rc = ufcs_get_source_info(class, &src_info);
		if (rc < 0)
			ufcs_err("test mode get src info error, rc=%d\n", rc);
		break;
	default:
		ufcs_err("Unsupported test mode ctrl msg, type=%u\n", type);
		return -EINVAL;
	}

	return rc;
}

static int ufcs_pe_test_data_request_handle(struct ufcs_class *class, enum ufcs_data_msg_type type)
{
	int rc;

	switch (type) {
	case DATA_MSG_REQUEST:
		rc = ufcs_pdo_set(class, UFCS_TEST_MODE_POWER_VOLT, UFCS_TEST_MODE_POWER_CURR);
		if (rc < 0)
			ufcs_err("test mode set pdo error, rc=%d\n", rc);
		break;
	case DATA_MSG_CONFIG_WATCHDOG:
		rc = ufcs_config_watchdog(class, UFCS_TEST_MODE_WATCHDOG);
		if (rc < 0)
			ufcs_err("test mode config watchdog error, rc=%d\n", rc);
		break;
	default:
		ufcs_err("Unsupported test mode data msg, type=%u\n", type);
		return -EINVAL;
	}

	return rc;
}

void ufcs_pe_test_handle_work(struct kthread_work *work)
{
	struct ufcs_class *class = container_of(work, struct ufcs_class, test_handle_work);
	u16 request;
	bool enable_test_mode, enable_vol_acc_test;
	const char *cmd_name;

	request = class->test_request;
	enable_test_mode = (bool)UFCS_TEST_REQUEST_ENABLE_TEST_MODE(request);
	enable_vol_acc_test = (bool)UFCS_TEST_REQUEST_ENABLE_VOL_ACC_TEST_MODE(request);

	if (class->test_mode != enable_test_mode) {
		class->test_mode = enable_test_mode;
		ufcs_send_state(UFCS_NOTIFY_TEST_MODE_CHANGED, NULL);
		ufcs_info("%s test mode\n", enable_test_mode ? "enable" : "disable");
	}
	if (class->vol_acc_test != enable_vol_acc_test) {
		class->vol_acc_test = enable_vol_acc_test;
		ufcs_send_state(UFCS_NOTIFY_VOL_ACC_TEST_MODE_CHANGED, NULL);
		ufcs_info("%s voltage acc test mode\n", enable_vol_acc_test ? "enable" : "disable");
	}

	if (UFCS_IS_TEST_CTRL_MSG(request))
		return;
	if (!class->test_mode)
		return;

	switch (UFCS_TEST_REQUEST_MSG_TYPE(request)) {
	case UFCS_CTRL_MSG:
		cmd_name = ufcs_get_ctrl_msg_name(UFCS_TEST_REQUEST_MSG_CMD(request));
		break;
	case UFCS_DATA_MSG:
		cmd_name = ufcs_get_data_msg_name(UFCS_TEST_REQUEST_MSG_CMD(request));
		break;
	default:
		cmd_name = "Invalid";
		break;
	}

	ufcs_info("test_request: test:%u, vol_acc:%u, addr:%s, msg_type:%s, cmd_type:%s\n",
		UFCS_TEST_REQUEST_ENABLE_TEST_MODE(request),
		UFCS_TEST_REQUEST_ENABLE_VOL_ACC_TEST_MODE(request),
		ufcs_get_msg_addr_name(UFCS_TEST_REQUEST_DEV_ADDR(request)),
		ufcs_get_msg_type_name(UFCS_TEST_REQUEST_MSG_TYPE(request)),
		cmd_name);

	switch (UFCS_TEST_REQUEST_MSG_TYPE(request)) {
	case UFCS_CTRL_MSG:
		(void)ufcs_pe_test_ctrl_request_handle(class, UFCS_TEST_REQUEST_MSG_CMD(request));
		break;
	case UFCS_DATA_MSG:
		(void)ufcs_pe_test_data_request_handle(class, UFCS_TEST_REQUEST_MSG_CMD(request));
		break;
	default:
		ufcs_err("Unsupported msg type(=%u)\n", UFCS_TEST_REQUEST_MSG_TYPE(request));
		break;
	}
}

void ufcs_pe_test_request_handle(struct ufcs_class *class, u16 request)
{
	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return;
	}

	class->test_request = request;
	kthread_queue_work(class->worker, &class->test_handle_work);
}
