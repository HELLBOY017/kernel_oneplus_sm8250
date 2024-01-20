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
#include <linux/kthread.h>
#include <linux/version.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched.h>
#include <linux/power_supply.h>

#include "ufcs_policy_engine.h"
#include "ufcs_core.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"
#include "ufcs_sha256.h"

#define HANDSHAKE_RETRY_MAX	3

struct ufcs_state_handler {
	const char *name;
	void (*handle_state)(struct ufcs_class *class, struct ufcs_event *event);
};

static const char * const ufcs_pe_state_name[] = {
	[PE_STATE_IDEL]			= "idel",
	[PE_STATE_SOFT_RESET]		= "software_reset",
	[PE_STATE_HW_RESET]		= "hardware_reset",
	[PE_STATE_SEND_EXIT]		= "send_exit",
	[PE_STATE_RECV_EXIT]		= "recv_exit",
	[PE_STATE_HANDSHAKE]		= "handshake",
	[PE_STATE_PING]			= "ping",
	[PE_STATE_OUTPUT_CAP]		= "output_cap",
	[PE_STATE_REQUEST]		= "request",
	[PE_STATE_SEND_SINK_INFO]	= "send_sink_info",
	[PE_STATE_GET_SOURCE_INFO]	= "get_source_info",
	[PE_STATE_GET_CABLE_INFO]	= "get_cable_info",
	[PE_STATE_GET_DEVICE_INFO]	= "get_device_info",
	[PE_STATE_GET_ERROR_INFO]	= "get_error_info",
	[PE_STATE_SEND_DEVICE_INFO]	= "send_device_info",
	[PE_STATE_SEND_ERROR_INFO]	= "send_error_info",
	[PE_STATE_CONFIG_WD]		= "config_watch_dog",
	[PE_STATE_VERIFY_REQUEST]	= "verify_request",
	[PE_STATE_VERIFY_RESPONSE]	= "verify_response",
	[PE_STATE_POWER_CHANGE]		= "power_change",
	[PE_STATE_GET_EMARK_INFO]	= "get_emark_info",
	[PE_STATE_GET_POWER_INFO]	= "get_power_info",
	[PE_STATE_TEST_REQUEST]		= "test_request",
};

#define PE_HANDLER(type, func)	\
[type] = {		\
	.handle_state = func##_handle,		\
}

void ufcs_exit_sm_work(struct ufcs_class *class)
{
	ufcs_free_all_event(class);
	class->handshake_success = false;
	class->start_cable_detect = false;
	class->state.curr = PE_STATE_IDEL;
	class->sender.msg_number_counter = 0;
	class->state.err = -EIO;
	complete_all(&class->request_ack);
	class->sender.status = MSG_RETRY_OVER;
	complete_all(&class->sender.ack);
}

void ufcs_reset_sm_work(struct ufcs_class *class)
{
	ufcs_free_all_event(class);
	class->state.curr = PE_STATE_IDEL;
	class->sender.msg_number_counter = 0;
	class->state.err = -EAGAIN;
	complete_all(&class->request_ack);
}

int ufcs_check_refuse_msg(struct ufcs_class *class, struct ufcs_msg *msg,
	enum ufcs_msg_type type, u8 cmd)
{
	enum ufcs_refuse_reason reason;

	if (UFCS_REFUSE_INFO_MSG_TYPE(msg->data_msg.refuse.data) != type ||
	    (type != UFCS_VENDOR_MSG && UFCS_REFUSE_INFO_MSG_CMD(msg->data_msg.refuse.data) != cmd)) {
		ufcs_err("unexpected refuse msg, type=%u, cmd=%u\n",
			 UFCS_REFUSE_INFO_MSG_TYPE(msg->data_msg.refuse.data),
			 UFCS_REFUSE_INFO_MSG_CMD(msg->data_msg.refuse.data));
		return 0;
	}
	reason = UFCS_REFUSE_INFO_REASON(msg->data_msg.refuse.data);
	switch (type) {
	case UFCS_CTRL_MSG:
		ufcs_info("refuse %s ctrl msg, reason=\"%s\"\n",
			  ufcs_get_ctrl_msg_name(cmd),
			  ufcs_get_refuse_reason_name(reason));
		break;
	case UFCS_DATA_MSG:
		ufcs_info("refuse %s data msg, reason=\"%s\"\n",
			  ufcs_get_data_msg_name(cmd),
			  ufcs_get_refuse_reason_name(reason));
		break;
	case UFCS_VENDOR_MSG:
		ufcs_info("refuse vendor msg, reason=\"%s\"\n",
			  ufcs_get_refuse_reason_name(reason));
		break;
	default:
		ufcs_err("refuse unknown(type=%d) msg, reason=\"%s\"\n",
			 type, ufcs_get_refuse_reason_name(reason));
		break;
	}
	if (reason == REFUSE_DEVICE_BUSY)
		return -EAGAIN;
	return -EIO;
}

static void ufcs_state_idel_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;
	bool soft_reset;

	mutex_lock(&class->pe_lock);
	class->state.err = 0;

	switch (event->type) {
	case UFCS_EVENT_RECV_HW_RESET:
		class->state.curr = PE_STATE_HW_RESET;
		break;
	case UFCS_EVENT_RECV_PING:
		ufcs_info("recv ping\n");
		ufcs_free_event(class, &event);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		class->state.curr = PE_STATE_SOFT_RESET;
		break;
	case UFCS_EVENT_RECV_GET_SINK_INFO:
		class->state.curr = PE_STATE_SEND_SINK_INFO;
		break;
	case UFCS_EVENT_RECV_GET_DEVICE_INFO:
		class->state.curr = PE_STATE_SEND_DEVICE_INFO;
		break;
	case UFCS_EVENT_RECV_GET_ERROR_INFO:
		class->state.curr = PE_STATE_SEND_ERROR_INFO;
		break;
	case UFCS_EVENT_SEND_GET_DEVICE_INFO:
		class->state.curr = PE_STATE_GET_DEVICE_INFO;
		break;
	case UFCS_EVENT_SEND_GET_ERROR_INFO:
		class->state.curr = PE_STATE_GET_ERROR_INFO;
		break;
	case UFCS_EVENT_SEND_GET_SOURCE_INFO:
		class->state.curr = PE_STATE_GET_SOURCE_INFO;
		break;
	case UFCS_EVENT_SEND_REQUEST:
		class->state.curr = PE_STATE_REQUEST;
		break;
	case UFCS_EVENT_SEND_GET_OUTPUT_CAP:
		class->state.curr = PE_STATE_OUTPUT_CAP;
		break;
	case UFCS_EVENT_RECV_EXIT_UFCS_MODE:
		class->state.curr = PE_STATE_RECV_EXIT;
		break;
	case UFCS_EVENT_SEND_EXIT_UFCS_MODE:
		class->state.curr = PE_STATE_SEND_EXIT;
		break;
	case UFCS_EVENT_HANDSHAKE:
		class->state.curr = PE_STATE_HANDSHAKE;
		break;
	case UFCS_EVENT_SEND_VERIFY_REQUEST:
		class->state.curr = PE_STATE_VERIFY_REQUEST;
		break;
	case UFCS_EVENT_RECV_POWER_CHANGE:
		class->state.curr = PE_STATE_POWER_CHANGE;
		break;
	case UFCS_EVENT_SEND_CONFIG_WD:
		class->state.curr = PE_STATE_CONFIG_WD;
		break;
	case UFCS_EVENT_SEND_START_CABLE_DETECT:
		class->state.curr = PE_STATE_GET_CABLE_INFO;
		break;
	case UFCS_EVENT_OPLUS_SEND_GET_EMARK_INFO_MSG:
		class->state.curr = PE_STATE_GET_EMARK_INFO;
		break;
	case UFCS_EVENT_OPLUS_SEND_GET_POWER_INFO_MSG:
		class->state.curr = PE_STATE_GET_POWER_INFO;
		break;
	case UFCS_EVENT_RECV_TEST_REQUEST:
		class->state.curr = PE_STATE_TEST_REQUEST;
		break;
	default:
		ufcs_err("not support %s event\n", ufcs_get_event_name(event));
		if (event->msg) {
			soft_reset = false;
retry:
			rc = ufcs_send_data_msg_refuse(class, event->msg, REFUSE_NOT_SUPPORT_CMD);
			if (rc < 0) {
				if (rc == -EAGAIN) {
					if (soft_reset) {
						ufcs_free_event(class, &event);
						ufcs_source_hard_reset(class->ufcs);
						return;
					}
					ufcs_send_ctrl_msg_soft_reset(class);
					soft_reset = true;
					goto retry;
				}
				ufcs_free_event(class, &event);
				ufcs_err("send refuse msg error, rc=%d\n", rc);
				goto err;
			}
		}
		ufcs_free_event(class, &event);
		break;
	}

	return;
err:
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_soft_reset_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	ufcs_free_event(class, &event);
	ufcs_reset_sm_work(class);
}

static void ufcs_state_hw_reset_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	ufcs_free_event(class, &event);
	ufcs_exit_sm_work(class);
	ufcs_send_state(UFCS_NOTIFY_SOURCE_HW_RESET, NULL);
}

static void ufcs_state_send_exit_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	bool soft_reset = false;
	int rc;

	ufcs_free_event(class, &event);
retry:
	rc = ufcs_send_ctrl_msg_exit_ufcs_mode(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset) {
				rc = -EIO;
				goto err;
			}
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send exit ufcs mode msg error, rc=%d\n", rc);
		goto err;
	}

	complete(&class->request_ack);
	return;
err:
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_recv_exit_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	ufcs_free_event(class, &event);
	class->handshake_success = false;
	ufcs_send_state(UFCS_NOTIFY_EXIT, NULL);
}

static void ufcs_state_handshake_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;
	int retry_count = 1;
	enum ufcs_baud_rate baud_rate = UFCS_BAUD_115200;

	ufcs_free_event(class, &event);
retry:
	class->ufcs->handshake_state = UFCS_HS_WAIT;
	class->ufcs->ops->handshake(class->ufcs);
	start_handshake_timer(class);
re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		class->handshake_success = false;
		ufcs_exit_sm_work(class);
		return;
	}

	switch (event->type) {
	case UFCS_EVENT_RECV_ACK:
		stop_handshake_timer(class);
		do {
			ufcs_info("set baud rate to %d\n", baud_rate);
			class->ufcs->ops->set_baud_rate(class->ufcs, baud_rate);
			rc = ufcs_send_ctrl_msg_ping(class);
			if (rc < 0 && rc != -EBUSY) {
				ufcs_err("ufcs ping error\n");
				goto err;
			} else if (rc >= 0) {
				break;
			}
			baud_rate++;
		} while (baud_rate < UFCS_BAUD_MAX);
		if (baud_rate >= UFCS_BAUD_MAX) {
			ufcs_err("no baud rate available");
			goto err;
		}
		/* ufcs success */
		class->handshake_success = true;
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_NCK:
		stop_handshake_timer(class);
		fallthrough;
	case UFCS_EVENT_TIMEOUT:
		if (retry_count < HANDSHAKE_RETRY_MAX &&
		    !class->config.handshake_hard_retry) {
			retry_count++;
			ufcs_free_event(class, &event);
			ufcs_err("handshake timeout, retry\n");
			goto retry;
		}
		goto err;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_handshake_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;

err:
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_output_cap_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	int i;
	int rc;

	ufcs_free_event(class, &event);
retry:
	rc = ufcs_send_ctrl_msg_get_output_capabilities(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send get_output_capabilities msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	msg = event->msg;
	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_GET_OUTPUT_CAPABILITIES);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		goto exit;
	case UFCS_EVENT_RECV_OUTPUT_CAP:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		for (i = 0; i < UFCS_OUTPUT_MODE_MAX; i++) {
			if (msg->data_msg.output_cap.output_mode[i] == 0)
				break;
			class->pdo.data[i] = msg->data_msg.output_cap.output_mode[i];
		}
		class->pdo.num = i;
		if (class->pdo.num == 0)
			ufcs_err("pdo num is 0\n");
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_state_request_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	bool wait_power_ready = false;
	struct ufcs_data_msg_request *request;
	u8 pdo_index;
	u32 pdo_vol, pdo_curr;
	int rc;

	request = (struct ufcs_data_msg_request *)event->data;
	if (request == NULL) {
		ufcs_err("event data is NULL\n");
		class->state.err = -EINVAL;
		complete(&class->request_ack);
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return;
	}
	pdo_index = UFCS_REQUEST_OUTPUT_MODE_INDEX(request->request);
	pdo_vol = UFCS_REQUEST_VOL(request->request);
	pdo_curr = UFCS_REQUEST_CURR(request->request);
	ufcs_info("request: index=%u, vol=%u mV, curr=%u mA\n", pdo_index, pdo_vol, pdo_curr);
	ufcs_free_event(class, &event);

retry:
	rc = ufcs_send_data_msg_request(class, pdo_index, pdo_vol, pdo_curr);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send request msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("no power ready msg, maybe timeout\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_DATA_MSG, DATA_MSG_REQUEST);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		if (wait_power_ready)
			ufcs_err("power ready timeout\n");
		goto exit;
	case UFCS_EVENT_RECV_ACCEPT:
		stop_sender_response_timer(class);
		wait_power_ready = true;
		start_power_supply_timer(class);
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_POWER_READY:
		if (!wait_power_ready)
			ufcs_err("The accept message is missing\n");
		stop_power_supply_timer(class);
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		stop_power_supply_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	stop_power_supply_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	stop_power_supply_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_state_get_src_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	int rc;

	ufcs_free_event(class, &event);
retry:
	rc = ufcs_send_ctrl_msg_get_source_info(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send get source info msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	msg = event->msg;
	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_GET_SOURCE_INFO);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		goto exit;
	case UFCS_EVENT_RECV_SOURCE_INFO:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		class->src_info = msg->data_msg.src_info.info;
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_state_send_sink_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_data_msg_sink_info sink_info;
	struct power_supply *psy;
	union power_supply_propval pval = { 0 };
	int batt_temp, usb_temp, vol, curr;
	int rc;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		ufcs_err("battery psy not found\n");
		batt_temp = UFCS_SINK_INFO_INVALID_TEMP;
		usb_temp = UFCS_SINK_INFO_INVALID_TEMP;
		vol = 0;
		curr = 0;
	} else {
		usb_temp = UFCS_SINK_INFO_INVALID_TEMP;
		rc = power_supply_get_property(psy, POWER_SUPPLY_PROP_TEMP, &pval);
		if (rc < 0) {
			ufcs_err("can't get battery temp, rc=%d\n", rc);
			batt_temp = UFCS_SINK_INFO_INVALID_TEMP;
		} else {
			batt_temp = pval.intval / 10;
		}
		rc = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &pval);
		if (rc < 0) {
			ufcs_err("can't get battery voltage, rc=%d\n", rc);
			vol = 0;
		} else {
			vol = pval.intval / 1000;
		}
		rc = power_supply_get_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW, &pval);
		if (rc < 0) {
			ufcs_err("can't get current voltage, rc=%d\n", rc);
			curr = 0;
		} else {
			curr = abs(pval.intval / 1000);
		}
		power_supply_put(psy);
	}

	sink_info.info = UFCS_SINK_INFO_DATA(batt_temp, usb_temp, vol, curr);
	rc = ufcs_send_data_msg_sink_info(class, &sink_info);
	if (rc < 0)
		ufcs_err("send sink info msg error, rc=%d\n", rc);

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
}

static void ufcs_state_get_cable_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;

	ufcs_free_event(class, &event);

	rc = ufcs_pe_start_cable_detect(class);
	if (rc > 0) {
		class->start_cable_detect = false;
		ufcs_exit_sm_work(class);
		return;
	} else if (rc < 0) {
		class->start_cable_detect = false;
		if (rc == -EPROTO) {
			class->state.err = rc;
			complete(&class->request_ack);
			return;
		}
		goto err;
	}

	rc = ufcs_pe_get_cable_info(class);
	if (rc > 0) {
		class->start_cable_detect = false;
		ufcs_exit_sm_work(class);
		return;
	} else if (rc == 0) {
		complete(&class->request_ack);
		ufcs_pe_end_cable_detect(class);
		class->start_cable_detect = false;
		class->state.curr = PE_STATE_IDEL;
		return;
	} else if (rc == -EPROTO) {
		class->state.err = rc;
		class->start_cable_detect = false;
		complete(&class->request_ack);
		return;
	}
	ufcs_pe_end_cable_detect(class);
	class->start_cable_detect = false;

	rc = ufcs_pe_detect_cable_info(class);
	if (rc > 0) {
		ufcs_exit_sm_work(class);
		return;
	} else if (rc < 0) {
		if (rc == -EPROTO) {
			class->state.err = rc;
			complete(&class->request_ack);
			return;
		}
		goto err;
	}

	class->state.curr = PE_STATE_IDEL;
	complete(&class->request_ack);
	return;

err:
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_get_dev_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	int rc;

	ufcs_free_event(class, &event);
retry:
	rc = ufcs_send_ctrl_msg_get_device_info(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send get device info msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	msg = event->msg;
	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_GET_DEVICE_INFO);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		goto exit;
	case UFCS_EVENT_RECV_DEVICE_INFO:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		class->dev_info = msg->data_msg.dev_info.info;
		if ((UFCS_DEVICE_INFO_DEV_VENDOR(class->dev_info) == UFCS_TEST_MODE_DEV_VND) &&
		    (UFCS_DEVICE_INFO_IC_VENDOR(class->dev_info) == UFCS_TEST_MODE_IC_VND)) {
			ufcs_info("enable test mode\n");
			class->test_mode = true;
			ufcs_send_state(UFCS_NOTIFY_TEST_MODE_CHANGED, NULL);
		}
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_state_get_error_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	int rc;

	ufcs_free_event(class, &event);
retry:
	rc = ufcs_send_ctrl_msg_get_error_info(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send get error info msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	msg = event->msg;
	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_GET_ERROR_INFO);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		goto exit;
	case UFCS_EVENT_RECV_ERROR_INFO:
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		class->err_info = msg->data_msg.err_info.info;
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static int ufcs_send_dev_info(struct ufcs_class *class)
{
	struct ufcs_data_msg_device_info dev_info;

	dev_info.info = UFCS_DEVICE_INFO_DATA(UFCS_VND_DEV_ID,
		class->config.ic_vendor_id, UFCS_HW_VERSION, UFCS_SW_VERSION);
	return ufcs_send_data_msg_dev_info(class, &dev_info);
}

static void ufcs_state_send_dev_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;

	rc = ufcs_send_dev_info(class);
	if (rc < 0)
		ufcs_err("send device info msg error, rc=%d\n", rc);

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
}

static void ufcs_state_send_error_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_data_msg_error_info err_info = { 0 };
	int rc;

	/* TODO: error info */
	rc = ufcs_send_data_msg_err_info(class, &err_info);
	if (rc < 0)
		ufcs_err("send error info msg error, rc=%d\n", rc);

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
}

static void ufcs_state_config_wd_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	struct ufcs_data_msg_config_watchdog config_wd;
	int rc;

	if (event->data == NULL) {
		ufcs_err("event data is NULL\n");
		class->state.err = -EINVAL;
		complete(&class->request_ack);
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return;
	}
	memcpy(&config_wd, event->data, sizeof(struct ufcs_data_msg_config_watchdog));
	ufcs_info("set watchdog to %u ms\n", config_wd.over_time_ms);
	ufcs_free_event(class, &event);

retry:
	rc = ufcs_send_data_msg_config_wd(class, &config_wd);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send config watchdog msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_DATA_MSG, DATA_MSG_CONFIG_WATCHDOG);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		goto exit;
	case UFCS_EVENT_RECV_ACCEPT:
		stop_sender_response_timer(class);
		complete(&class->request_ack);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_check_verify_response_msg(struct ufcs_class *class, struct ufcs_msg *msg)
{
	int rc;
	u8 encrypt_data[UFCS_VERIFY_ENCRYPTED_DATA_SIZE] = { 0 };
	bool pass = false;

	rc = ufcs_get_encrypt_data(msg->data_msg.verify_response.random_data,
		class->verify_info.random_data, class->verify_info.auth_data,
		encrypt_data);
	if (rc == 0) {
		rc = memcmp(msg->data_msg.verify_response.encrypted_data,
			encrypt_data, UFCS_VERIFY_ENCRYPTED_DATA_SIZE);
		if (rc == 0)
			pass = true;
	} else {
		ufcs_err("ufcs get encrypt data error, rc=%d\n", rc);
		class->state.err = rc;
		goto out;
	}
	if (pass) {
		ufcs_info("ufcs verify pass\n");
		class->verify_pass = true;
	} else {
		ufcs_info("ufcs verify faile\n");
		class->verify_pass = false;
	}
	class->state.err = 0;
out:
	complete(&class->request_ack);
}

static void ufcs_state_verify_request_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	bool soft_reset = false;
	bool wait_get_dev_info = false;
	bool wait_verify_response = false;
	struct ufcs_data_msg_verify_request verify_request;
	int rc;

	if (event->data == NULL) {
		ufcs_err("event data is NULL\n");
		class->state.err = -EINVAL;
		complete(&class->request_ack);
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return;
	}
	memcpy(&verify_request, event->data, sizeof(struct ufcs_data_msg_verify_request));
	memcpy(class->verify_info.random_data, verify_request.random_data,
	       UFCS_VERIFY_RANDOM_DATA_SIZE);
	ufcs_free_event(class, &event);

retry:
	rc = ufcs_send_data_msg_verify_request(class, &verify_request);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto err;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send verify request msg error, rc=%d\n", rc);
		goto err;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		ufcs_exit_sm_work(class);
		return;
	}

	switch (event->type) {
	case UFCS_EVENT_RECV_REFUSE:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = ufcs_check_refuse_msg(class, msg, UFCS_DATA_MSG, DATA_MSG_VERIFY_REQUEST);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc != -EAGAIN)
			goto exit;
		if (soft_reset)
			goto err;
		ufcs_send_ctrl_msg_soft_reset(class);
		soft_reset = true;
		ufcs_free_event(class, &event);
		goto retry;
	case UFCS_EVENT_TIMEOUT:
		if (wait_get_dev_info)
			ufcs_err("wait get device info msg timeout\n");
		if (wait_verify_response)
			ufcs_err("wait verify response msg timeout\n");
		goto exit;
	case UFCS_EVENT_RECV_ACCEPT:
		stop_sender_response_timer(class);
		wait_get_dev_info = true;
		wait_verify_response = false;
		start_wait_msg_timer(class);
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_GET_DEVICE_INFO:
		stop_wait_msg_timer(class);
		rc = ufcs_send_dev_info(class);
		if (rc < 0) {
			ufcs_err("send device info msg error, rc=%d\n", rc);
			goto err;
		}
		wait_get_dev_info = false;
		wait_verify_response = true;
		start_wait_msg_timer(class);
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_VERIFY_RESPONSE:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_wait_msg_timer(class);
		ufcs_check_verify_response_msg(class, msg);
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg)
			goto err;
		ufcs_free_event(class, &event);
		goto re_recv;
	}

	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
	return;
err:
	stop_sender_response_timer(class);
	stop_wait_msg_timer(class);
	ufcs_free_event(class, &event);
	ufcs_source_hard_reset(class->ufcs);
	return;
exit:
	stop_sender_response_timer(class);
	stop_wait_msg_timer(class);
	ufcs_free_event(class, &event);
	class->state.curr = PE_STATE_IDEL;
	class->state.err = -EAGAIN;
	complete(&class->request_ack);
}

static void ufcs_state_power_change_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	int i;

	msg = event->msg;
	if (msg == NULL) {
		ufcs_err("msg is NULL\n");
		goto out;
	}

	for (i = 0; i < UFCS_OUTPUT_MODE_MAX; i++)
		class->pwr_change_info[i] = msg->data_msg.power_change.data[i];
	class->power_changed = true;
	ufcs_send_state(UFCS_NOTIFY_POWER_CHANGE, NULL);
out:
	class->state.curr = PE_STATE_IDEL;
	ufcs_free_event(class, &event);
}

static void ufcs_state_get_emark_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;

	ufcs_free_event(class, &event);
	rc = ufcs_pe_oplus_get_emark_info(class);
	if (rc > 0) {
		ufcs_exit_sm_work(class);
		return;
	} else if (rc < 0) {
		if (rc == -EPROTO) {
			class->state.err = rc;
			complete(&class->request_ack);
			return;
		}
		goto err;
	}

	class->state.curr = PE_STATE_IDEL;
	complete(&class->request_ack);
	return;

err:
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_get_power_info_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	int rc;

	ufcs_free_event(class, &event);
	rc = ufcs_pe_oplus_get_power_info(class);
	if (rc > 0) {
		ufcs_exit_sm_work(class);
		return;
	} else if (rc < 0) {
		if (rc == -EPROTO) {
			class->state.err = rc;
			complete(&class->request_ack);
			return;
		}
		goto err;
	}

	class->state.curr = PE_STATE_IDEL;
	complete(&class->request_ack);
	return;

err:
	ufcs_source_hard_reset(class->ufcs);
}

static void ufcs_state_test_request_handle(struct ufcs_class *class, struct ufcs_event *event)
{
	u16 request;

	request = event->msg->data_msg.test_request.data;
	ufcs_free_event(class, &event);

	ufcs_pe_test_request_handle(class, request);
	class->state.curr = PE_STATE_IDEL;

	return;
}

struct ufcs_state_handler g_pe_handler[] = {
	PE_HANDLER(PE_STATE_IDEL, ufcs_state_idel),
	PE_HANDLER(PE_STATE_SOFT_RESET, ufcs_state_soft_reset),
	PE_HANDLER(PE_STATE_HW_RESET, ufcs_state_hw_reset),
	PE_HANDLER(PE_STATE_SEND_EXIT, ufcs_state_send_exit),
	PE_HANDLER(PE_STATE_RECV_EXIT, ufcs_state_recv_exit),
	PE_HANDLER(PE_STATE_HANDSHAKE, ufcs_state_handshake),
	PE_HANDLER(PE_STATE_OUTPUT_CAP, ufcs_state_output_cap),
	PE_HANDLER(PE_STATE_REQUEST, ufcs_state_request),
	PE_HANDLER(PE_STATE_GET_SOURCE_INFO, ufcs_state_get_src_info),
	PE_HANDLER(PE_STATE_SEND_SINK_INFO, ufcs_state_send_sink_info),
	PE_HANDLER(PE_STATE_GET_CABLE_INFO, ufcs_state_get_cable_info),
	PE_HANDLER(PE_STATE_GET_DEVICE_INFO, ufcs_state_get_dev_info),
	PE_HANDLER(PE_STATE_GET_ERROR_INFO, ufcs_state_get_error_info),
	PE_HANDLER(PE_STATE_SEND_DEVICE_INFO, ufcs_state_send_dev_info),
	PE_HANDLER(PE_STATE_SEND_ERROR_INFO, ufcs_state_send_error_info),
	PE_HANDLER(PE_STATE_CONFIG_WD, ufcs_state_config_wd),
	PE_HANDLER(PE_STATE_VERIFY_REQUEST, ufcs_state_verify_request),
	PE_HANDLER(PE_STATE_POWER_CHANGE, ufcs_state_power_change),
	PE_HANDLER(PE_STATE_GET_EMARK_INFO, ufcs_state_get_emark_info),
	PE_HANDLER(PE_STATE_GET_POWER_INFO, ufcs_state_get_power_info),
	PE_HANDLER(PE_STATE_TEST_REQUEST, ufcs_state_test_request),
};

static int ufcs_sm_task(void *arg)
{
	struct ufcs_class *class = arg;
	struct ufcs_event *event = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct sched_param sp = {.sched_priority = MAX_RT_PRIO / 2 };
#endif

	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return -ENODEV;
	}
	ufcs_info("ufcs sm work start\n");

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	sched_setscheduler(class->sm_task, SCHED_FIFO, &sp);
#else
	sched_set_fifo(class->sm_task);
#endif

	while (!kthread_should_stop()) {
		wait_event(class->sm_wq, class->sm_task_wakeup);
		class->sm_task_wakeup = false;
retry:
		event = ufcs_get_next_event(class);
		if (event == NULL) {
			if (!class->event.exit) {
				ufcs_err("can't get ufcs event\n");
				goto retry;
			}
			ufcs_exit_sm_work(class);
			if (mutex_is_locked(&class->pe_lock))
				mutex_unlock(&class->pe_lock);
			continue;
		}

		ufcs_info("curr_state=%s, event=%s\n",
			  ufcs_pe_state_name[class->state.curr],
			  ufcs_get_event_name(event));
		g_pe_handler[class->state.curr].handle_state(class, event);

		if ((class->state.curr == PE_STATE_IDEL) &&
		    mutex_is_locked(&class->pe_lock))
			mutex_unlock(&class->pe_lock);

		if (kthread_should_stop())
			break;

		if (!class->event.exit)
			goto retry;
	}
	ufcs_info("ufcs sm work exit\n");

	return 0;
}

int ufcs_policy_engine_init(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}

	kthread_init_work(&class->test_handle_work, ufcs_pe_test_handle_work);
	class->sm_task_wakeup = false;
	class->sm_task = kthread_create(ufcs_sm_task, class, "ufcs_sm");
	if (IS_ERR(class->sm_task)) {
		ufcs_err("creat ufcs_sm task error, rc=%ld\n", PTR_ERR(class->sm_task));
		return PTR_ERR(class->sm_task);
	}
	init_waitqueue_head(&class->sm_wq);
	wake_up_process(class->sm_task);

	return 0;
}
