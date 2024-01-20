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

int ufcs_pe_start_cable_detect(struct ufcs_class *class)
{
	struct ufcs_msg *msg;
	struct ufcs_event *event = NULL;
	bool soft_reset = false;
	int rc;

	class->start_cable_detect = true;
retry:
	rc = ufcs_send_ctrl_msg_start_cable_detect(class);
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
		ufcs_err("send start cable detect msg error, rc=%d\n", rc);
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
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_START_CABLE_DETECT);
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
		stop_sender_response_timer(class);
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return -EPROTO;
	case UFCS_EVENT_RECV_ACCEPT:
		stop_sender_response_timer(class);
		rc = 0;
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

int ufcs_pe_get_cable_info(struct ufcs_class *class)
{
	struct ufcs_msg *msg;
	struct ufcs_event *event = NULL;
	bool soft_reset = false;
	int rc;

retry:
	rc = ufcs_send_ctrl_msg_get_cable_info(class);
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
		ufcs_err("send get cable info msg error, rc=%d\n", rc);
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
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_GET_CABLE_INFO);
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
		stop_sender_response_timer(class);
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return -EPROTO;
	case UFCS_EVENT_RECV_CABLE_INFO:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_sender_response_timer(class);
		rc = 0;
		class->cable_info = msg->data_msg.cable_info.info;
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
	if (event != NULL)
		ufcs_free_event(class, &event);
	return rc;
}

int ufcs_pe_end_cable_detect(struct ufcs_class *class)
{
	bool soft_reset = false;
	int rc;

retry:
	rc = ufcs_send_ctrl_msg_end_cable_detect(class);
	if (rc < 0) {
		if (rc == -EAGAIN) {
			if (soft_reset)
				return -EIO;
			ufcs_send_ctrl_msg_soft_reset(class);
			soft_reset = true;
			goto retry;
		}
		ufcs_err("send end cable detect msg error, rc=%d\n", rc);
		class->start_cable_detect = false;
		return rc;
	}
	class->start_cable_detect = false;

	return 0;
}

int ufcs_pe_detect_cable_info(struct ufcs_class *class)
{
	struct ufcs_msg *msg;
	struct ufcs_event *event = NULL;
	bool soft_reset = false;
	bool wait_start = false;
	bool wait_end = false;
	bool wait_cable_info = false;
	int rc;

retry:
	rc = ufcs_send_ctrl_msg_detect_cable_info(class);
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
		ufcs_err("send detect cable info msg error, rc=%d\n", rc);
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
		rc = ufcs_check_refuse_msg(class, msg, UFCS_CTRL_MSG, CTRL_MSG_DETECT_CABLE_INFO);
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
		if (wait_start) {
			stop_cable_info_timer(class);
			ufcs_err("wait start cable detect timeout\n");
		}
		if (wait_end) {
			stop_cable_info_timer(class);
			stop_restart_trans_timer(class);
			/* TODO: resume communication */
			ufcs_err("wait end cable detect timeout\n");
		}
		if (wait_cable_info) {
			stop_cable_info_timer(class);
			stop_restart_trans_timer(class);
			/* TODO: resume communication */
			ufcs_err("wait cable info timeout\n");
		}
		ufcs_free_event(class, &event);
		class->state.curr = PE_STATE_IDEL;
		return -EPROTO;
	case UFCS_EVENT_RECV_ACCEPT:
		stop_sender_response_timer(class);
		wait_start = true;
		start_cable_info_timer(class);
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_START_CABLE_DETECT:
		wait_start = false;
		wait_end = true;
		start_restart_trans_timer(class);
		/* TODO: stop communication */
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_END_CABLE_DETECT:
		wait_end = false;
		wait_cable_info = true;
		stop_restart_trans_timer(class);
		/* TODO: resume communication */
		ufcs_free_event(class, &event);
		goto re_recv;
	case UFCS_EVENT_RECV_CABLE_INFO:
		msg = event->msg;
		if (msg == NULL) {
			ufcs_err("msg is NULL\n");
			ufcs_free_event(class, &event);
			goto re_recv;
		}
		stop_cable_info_timer(class);
		rc = 0;
		class->cable_info = msg->data_msg.cable_info.info;
		break;
	case UFCS_EVENT_RECV_SOFT_RESET:
		stop_sender_response_timer(class);
		stop_cable_info_timer(class);
		stop_restart_trans_timer(class);
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
	stop_cable_info_timer(class);
	stop_restart_trans_timer(class);
	if (event != NULL)
		ufcs_free_event(class, &event);
	return rc;
}
