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

extern int ufcs_check_refuse_msg(struct ufcs_class *class, struct ufcs_msg *msg,
	enum ufcs_msg_type type, u8 cmd);

int ufcs_pe_oplus_get_emark_info(struct ufcs_class *class)
{
	struct ufcs_msg *msg;
	struct ufcs_event *event = NULL;
	bool soft_reset = false;
	int rc;

retry:
	rc = ufcs_send_oplus_ctrl_msg_get_emark_info(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset) {
				rc = -EIO;
				goto out;
			}
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send oplus get emark info msg error, rc=%d\n", rc);
		goto out;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		return 1;
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
		rc = ufcs_check_refuse_msg(class, msg, UFCS_VENDOR_MSG, 0);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto out;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			ufcs_free_event(class, &event);
			goto retry;
		}
		fallthrough;
	case UFCS_EVENT_TIMEOUT:
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return -EPROTO;
	case UFCS_EVENT_OPLUS_RECV_EMARK_INFO_MSG:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		class->emark_info = msg->vendor_msg.vnd_msg.data_msg.emark_info.data;
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return -EPROTO;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg) {
			rc = -ENOTSUPP;
			goto out;
		}
		ufcs_free_event(class, &event);
		goto re_recv;
	}

out:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	return rc;
}

int ufcs_pe_oplus_get_power_info(struct ufcs_class *class)
{
	struct ufcs_msg *msg;
	struct ufcs_event *event = NULL;
	int i;
	bool soft_reset = false;
	int rc;

retry:
	rc = ufcs_send_oplus_ctrl_msg_get_power_info_ext(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset) {
				rc = -EIO;
				goto out;
			}
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send oplus get emark info msg error, rc=%d\n", rc);
		goto out;
	}
	start_sender_response_timer(class);

re_recv:
	event = ufcs_get_next_event(class);
	if (event == NULL) {
		if (!class->event.exit) {
			ufcs_err("can't get ufcs event\n");
			goto re_recv;
		}
		return 1;
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
		rc = ufcs_check_refuse_msg(class, msg, UFCS_VENDOR_MSG, 0);
		if (rc >= 0) {
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		if (rc == -EAGAIN) {
			if (soft_reset)
				goto out;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			ufcs_free_event(class, &event);
			goto retry;
		}
		fallthrough;
	case UFCS_EVENT_TIMEOUT:
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return -EPROTO;
	case UFCS_EVENT_OPLUS_RECV_POWER_INFO_MSG:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		for (i = 0; i < UFCS_OPLUS_VND_POWER_INFO_MAX; i++) {
			if (msg->vendor_msg.vnd_msg.data_msg.power_info.data[i] == 0)
				break;
			class->pie.data[i] = msg->vendor_msg.vnd_msg.data_msg.power_info.data[i];
		}
		class->pie.num = i;
		if (class->pie.num == 0)
			ufcs_err("pie num is 0\n");
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		class->state.curr = PE_STATE_SOFT_RESET;
		return -EPROTO;
	default:
		ufcs_err("unsupported event type, type=%s\n", ufcs_get_event_name(event));
		if (event->msg) {
			rc = -ENOTSUPP;
			goto out;
		}
		ufcs_free_event(class, &event);
		goto re_recv;
	}

out:
	stop_sender_response_timer(class);
	ufcs_free_event(class, &event);
	return rc;
}
