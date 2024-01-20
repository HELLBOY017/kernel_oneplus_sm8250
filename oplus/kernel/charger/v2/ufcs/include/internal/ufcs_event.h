// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_EVENT_H__
#define __OPLUS_UFCS_EVENT_H__

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

#include "ufcs_msg.h"

enum msg_send_status {
	MSG_ACK_TIMEOUT,
	MSG_RETRY_OVER,
	MSG_NCK,
	MSG_SEND_OK,
	MSG_SEND_PENDIGN,
	MSG_SEND_FAIL,
	MSG_SEND_START,
	MSG_WAIT_ACK,
};

struct ufcs_msg_sender {
	struct ufcs_msg *msg;
	struct completion ack;
	enum msg_send_status status;
	u8 msg_number_counter;
	u8 msg_retry_count;
	struct mutex lock;
};

enum ufcs_event_type {
	/* ctrl msg */
	UFCS_EVENT_SEND_HW_RESET = 0,
	UFCS_EVENT_SEND_PING,
	UFCS_EVENT_SEND_ACK,
	UFCS_EVENT_SEND_NCK,
	UFCS_EVENT_SEND_ACCEPT,
	UFCS_EVENT_SEND_SOFT_RESET,
	UFCS_EVENT_SEND_POWER_READY,
	UFCS_EVENT_SEND_GET_OUTPUT_CAP,
	UFCS_EVENT_SEND_GET_SOURCE_INFO,
	UFCS_EVENT_SEND_GET_SINK_INFO,
	UFCS_EVENT_SEND_GET_DEVICE_INFO,
	UFCS_EVENT_SEND_GET_ERROR_INFO,
	UFCS_EVENT_SEND_DETECT_CABLE_INFO,
	UFCS_EVENT_SEND_START_CABLE_DETECT,
	UFCS_EVENT_SEND_END_CABLE_DETECT,
	UFCS_EVENT_SEND_EXIT_UFCS_MODE,

	UFCS_EVENT_RECV_HW_RESET,
	UFCS_EVENT_RECV_PING,
	UFCS_EVENT_RECV_ACK,
	UFCS_EVENT_RECV_NCK,
	UFCS_EVENT_RECV_ACCEPT,
	UFCS_EVENT_RECV_SOFT_RESET,
	UFCS_EVENT_RECV_POWER_READY,
	UFCS_EVENT_RECV_GET_OUTPUT_CAP,
	UFCS_EVENT_RECV_GET_SOURCE_INFO,
	UFCS_EVENT_RECV_GET_SINK_INFO,
	UFCS_EVENT_RECV_GET_DEVICE_INFO,
	UFCS_EVENT_RECV_GET_ERROR_INFO,
	UFCS_EVENT_RECV_DETECT_CABLE_INFO,
	UFCS_EVENT_RECV_START_CABLE_DETECT,
	UFCS_EVENT_RECV_END_CABLE_DETECT,
	UFCS_EVENT_RECV_EXIT_UFCS_MODE,

	/* data msg */
	UFCS_EVENT_RECV_VERIFY_REQUEST,
	UFCS_EVENT_RECV_VERIFY_RESPONSE,
	UFCS_EVENT_RECV_OUTPUT_CAP,
	UFCS_EVENT_RECV_REQUEST,
	UFCS_EVENT_RECV_SOURCE_INFO,
	UFCS_EVENT_RECV_SINK_INFO,
	UFCS_EVENT_RECV_DEVICE_INFO,
	UFCS_EVENT_RECV_CABLE_INFO,
	UFCS_EVENT_RECV_ERROR_INFO,
	UFCS_EVENT_RECV_CONFIG_WD,
	UFCS_EVENT_RECV_REFUSE,
	UFCS_EVENT_RECV_POWER_CHANGE,
	UFCS_EVENT_RECV_TEST_REQUEST,

	UFCS_EVENT_SEND_VERIFY_REQUEST,
	UFCS_EVENT_SEND_VERIFY_RESPONSE,
	UFCS_EVENT_SEND_OUTPUT_CAP,
	UFCS_EVENT_SEND_REQUEST,
	UFCS_EVENT_SEND_SOURCE_INFO,
	UFCS_EVENT_SEND_SINK_INFO,
	UFCS_EVENT_SEND_DEVICE_INFO,
	UFCS_EVENT_SEND_CABLE_INFO,
	UFCS_EVENT_SEND_ERROR_INFO,
	UFCS_EVENT_SEND_CONFIG_WD,
	UFCS_EVENT_SEND_REFUSE,
	UFCS_EVENT_SEND_POWER_CHANGE,
	UFCS_EVENT_SEND_TEST_REQUEST,

	/* vendor msg */
	UFCS_EVENT_OPLUS_SEND_GET_EMARK_INFO_MSG,
	UFCS_EVENT_OPLUS_SEND_GET_POWER_INFO_MSG,

	UFCS_EVENT_OPLUS_RECV_EMARK_INFO_MSG,
	UFCS_EVENT_OPLUS_RECV_POWER_INFO_MSG,

	/* other */
	UFCS_EVENT_HANDSHAKE,
	UFCS_EVENT_TIMEOUT,

	UFCS_EVENT_MAX,
};

struct ufcs_event {
	enum ufcs_event_type type;
	struct ufcs_msg *msg;
	struct list_head list;
	void *data;
};

struct ufcs_event_info {
	struct list_head event_list;
	bool exit;
	spinlock_t event_list_lock;
	struct completion ack;
};

struct ufcs_class;

/* common api */
const char *ufcs_get_msg_addr_name(enum ufcs_device_addr addr);
const char *ufcs_get_msg_type_name(enum ufcs_msg_type type);
const char *ufcs_get_msg_version_name(enum ufcs_version ver);
const char *ufcs_get_event_name(struct ufcs_event *event);
int ufcs_event_init(struct ufcs_class *class);
int ufcs_event_reset(struct ufcs_class *class);
int ufcs_push_event(struct ufcs_class *class, struct ufcs_event *event);
int ufcs_send_msg(struct ufcs_class *class, struct ufcs_msg *msg);
void ufcs_free_event(struct ufcs_class *class, struct ufcs_event **event);
void ufcs_free_all_event(struct ufcs_class *class);
struct ufcs_event *ufcs_get_next_event(struct ufcs_class *class);

/* ctrl msg api */
const char *ufcs_get_ctrl_msg_name(enum ufcs_ctrl_msg_type type);
int ufcs_send_ctrl_msg(struct ufcs_class *class, enum ufcs_ctrl_msg_type type);
bool ufcs_is_supported_ctrl_msg(struct ufcs_ctrl_msg *msg);
bool ufcs_is_ack_nck_msg(struct ufcs_ctrl_msg *msg);
bool ufcs_recv_ack_nck_msg(struct ufcs_class *class, struct ufcs_ctrl_msg *msg);

static inline int ufcs_send_ctrl_msg_ack(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_ACK);
}

static inline int ufcs_send_ctrl_msg_nck(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_NCK);
}

static inline int ufcs_send_ctrl_msg_ping(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_PING);
}

static inline int ufcs_send_ctrl_msg_accept(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_ACCEPT);
}

static inline int ufcs_send_ctrl_msg_soft_reset(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_SOFT_RESET);
}

static inline int ufcs_send_ctrl_msg_power_ready(struct ufcs_class *class)
{
	return -ENOTSUPP;
}

static inline int ufcs_send_ctrl_msg_get_output_capabilities(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_GET_OUTPUT_CAPABILITIES);
}

static inline int ufcs_send_ctrl_msg_get_source_info(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_GET_SOURCE_INFO);
}

static inline int ufcs_send_ctrl_msg_get_sink_info(struct ufcs_class *class)
{
	return -ENOTSUPP;
}

static inline int ufcs_send_ctrl_msg_get_cable_info(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_GET_CABLE_INFO);
}

static inline int ufcs_send_ctrl_msg_get_device_info(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_GET_DEVICE_INFO);
}

static inline int ufcs_send_ctrl_msg_get_error_info(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_GET_ERROR_INFO);
}

static inline int ufcs_send_ctrl_msg_detect_cable_info(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_DETECT_CABLE_INFO);
}

static inline int ufcs_send_ctrl_msg_start_cable_detect(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_START_CABLE_DETECT);
}

static inline int ufcs_send_ctrl_msg_end_cable_detect(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_END_CABLE_DETECT);
}

static inline int ufcs_send_ctrl_msg_exit_ufcs_mode(struct ufcs_class *class)
{
	return ufcs_send_ctrl_msg(class, CTRL_MSG_EXIT_UFCS_MODE);
}

/* data msg api */
const char *ufcs_get_data_msg_name(enum ufcs_data_msg_type type);
const char *ufcs_get_refuse_reason_name(enum ufcs_refuse_reason reason);
bool ufcs_is_supported_data_msg(struct ufcs_data_msg *msg);
int ufcs_data_msg_init(struct ufcs_data_msg *msg);
int ufcs_data_msg_pack(struct ufcs_data_msg *msg);
int ufcs_send_data_msg_output_cap(struct ufcs_class *class,
	struct ufcs_data_msg_output_capabilities *output_cap);
	int __ufcs_send_data_msg_request(struct ufcs_class *class,
	struct ufcs_data_msg_request *request);
int ufcs_send_data_msg_request(struct ufcs_class *class, u8 index, u32 vol, u32 curr);
int ufcs_send_data_msg_src_info(struct ufcs_class *class,
	struct ufcs_data_msg_source_info *src_info);
int ufcs_send_data_msg_sink_info(struct ufcs_class *class,
	struct ufcs_data_msg_sink_info *sink_info);
int ufcs_send_data_msg_cable_info(struct ufcs_class *class,
	struct ufcs_data_msg_cable_info *cable_info);
int ufcs_send_data_msg_dev_info(struct ufcs_class *class,
	struct ufcs_data_msg_device_info *dev_info);
int ufcs_send_data_msg_err_info(struct ufcs_class *class,
	struct ufcs_data_msg_error_info *err_info);
int ufcs_send_data_msg_config_wd(struct ufcs_class *class,
	struct ufcs_data_msg_config_watchdog *config_wd);
int ufcs_send_data_msg_refuse(struct ufcs_class *class, struct ufcs_msg *msg,
			      enum ufcs_refuse_reason reason);
int ufcs_send_data_msg_verify_request(struct ufcs_class *class,
	struct ufcs_data_msg_verify_request *verify_request);
int ufcs_send_data_msg_verify_response(struct ufcs_class *class,
	struct ufcs_data_msg_verify_response *verify_response);
int ufcs_send_data_msg_power_change(struct ufcs_class *class,
	struct ufcs_data_msg_power_change *power_change);
int ufcs_send_data_msg_test_request(struct ufcs_class *class,
	struct ufcs_data_msg_test_request *test_request);

/* vendor msg */
const char *ufcs_get_oplus_msg_name(enum ufcs_oplus_vnd_msg_type type);
bool ufcs_is_supported_oplus_ctrl_msg(struct ufcs_vendor_msg *msg);
bool ufcs_is_supported_oplus_data_msg(struct ufcs_vendor_msg *msg);
int ufcs_oplus_send_ctrl_msg(struct ufcs_class *class, enum ufcs_oplus_vnd_msg_type type);
int ufcs_oplus_data_msg_init(struct ufcs_vendor_msg *msg);

static inline int ufcs_send_oplus_ctrl_msg_get_emark_info(struct ufcs_class *class)
{
	return ufcs_oplus_send_ctrl_msg(class, OPLUS_MSG_GET_EMARK_INFO);
}

static inline int ufcs_send_oplus_ctrl_msg_get_power_info_ext(struct ufcs_class *class)
{
	return ufcs_oplus_send_ctrl_msg(class, OPLUS_MSG_GET_POWER_INFO_EXT);
}

#endif /* __OPLUS_UFCS_EVENT_H__ */
