// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[TIMER]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/hrtimer.h>

#include "ufcs_core.h"
#include "ufcs_timer.h"
#include "ufcs_event.h"

static enum hrtimer_restart ack_receive_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[ACK_RECEIVE_TIMER]);

	ufcs_err("ack receive timeout\n");
	class->sender.status = MSG_ACK_TIMEOUT;
	kthread_queue_work(class->worker, &class->msg_send_work);

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart sender_response_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[SENDER_RESPONSE_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("sender response timeout\n");
	event = class->timeout_event[SENDER_RESPONSE_TIMER];
	class->timeout_event[SENDER_RESPONSE_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart power_supply_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[POWER_SUPPLY_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("power supply timeout\n");
	event = class->timeout_event[POWER_SUPPLY_TIMER];
	class->timeout_event[POWER_SUPPLY_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart cable_info_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[CABLE_INFO_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("cable info timeout\n");
	event = class->timeout_event[CABLE_INFO_TIMER];
	class->timeout_event[CABLE_INFO_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart restart_trans_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[RESTART_TRANS_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("restart trans timeout\n");
	event = class->timeout_event[RESTART_TRANS_TIMER];
	class->timeout_event[RESTART_TRANS_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart cable_trans_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[CABLE_TRANS_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("cable trans timeout\n");
	event = class->timeout_event[CABLE_TRANS_TIMER];
	class->timeout_event[CABLE_TRANS_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart msg_trans_delay_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[MSG_TRANS_DELAY_TIMER]);

	if (class->sender.status != MSG_SEND_PENDIGN) {
		ufcs_err("sender status expected %d, but actually got %d\n",
		MSG_SEND_PENDIGN, class->sender.status);
	} else {
		class->sender.status = MSG_SEND_START;
		if (class->sender.msg)
			kthread_queue_work(class->worker, &class->msg_send_work);
	}
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart handshake_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[HANDSHAKE_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("handshake timeout\n");
	event = class->timeout_event[HANDSHAKE_TIMER];
	class->timeout_event[HANDSHAKE_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart wait_msg_timeout(struct hrtimer *timer)
{
	struct ufcs_class *class = container_of(timer, struct ufcs_class, timer[WAIT_MSG_TIMER]);
	struct ufcs_event *event;
	int rc;

	ufcs_err("wait msg timeout\n");
	event = class->timeout_event[WAIT_MSG_TIMER];
	class->timeout_event[WAIT_MSG_TIMER] = NULL;
	if (event == NULL) {
		ufcs_err("event buf is NULL\n");
		return HRTIMER_NORESTART;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_TIMEOUT;
	INIT_LIST_HEAD(&event->list);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push timeout event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return HRTIMER_NORESTART;
}

int ufcs_timer_init(struct ufcs_class *class)
{
	int i;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}

	for (i = 0; i < TIMER_MAX; i++) {
		hrtimer_init(&class->timer[i], CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		class->timeout_event[i] = NULL;
	}

	class->timer[ACK_RECEIVE_TIMER].function = ack_receive_timeout;
	class->timer[SENDER_RESPONSE_TIMER].function = sender_response_timeout;
	class->timer[POWER_SUPPLY_TIMER].function = power_supply_timeout;
	class->timer[CABLE_INFO_TIMER].function = cable_info_timeout;
	class->timer[RESTART_TRANS_TIMER].function = restart_trans_timeout;
	class->timer[CABLE_TRANS_TIMER].function = cable_trans_timeout;
	class->timer[MSG_TRANS_DELAY_TIMER].function = msg_trans_delay_timeout;
	class->timer[HANDSHAKE_TIMER].function = handshake_timeout;
	class->timer[WAIT_MSG_TIMER].function = wait_msg_timeout;

	return 0;
}

void start_ack_receive_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->config.msg_resend)
		hrtimer_start(&class->timer[ACK_RECEIVE_TIMER],
			ms_to_ktime(T_ACK_RECEIVE_MS), HRTIMER_MODE_REL);
	else
		hrtimer_start(&class->timer[ACK_RECEIVE_TIMER],
			ms_to_ktime(T_ACK_RECEIVE_NO_RESEND_MS), HRTIMER_MODE_REL);
}

void stop_ack_receive_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[ACK_RECEIVE_TIMER]);
}

void start_sender_response_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[SENDER_RESPONSE_TIMER] == NULL) {
		class->timeout_event[SENDER_RESPONSE_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[SENDER_RESPONSE_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[SENDER_RESPONSE_TIMER],
		ms_to_ktime(T_SENDER_RESPONSE_MS), HRTIMER_MODE_REL);
}

void stop_sender_response_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[SENDER_RESPONSE_TIMER]);
	if (class->timeout_event[SENDER_RESPONSE_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[SENDER_RESPONSE_TIMER]);
		class->timeout_event[SENDER_RESPONSE_TIMER] = NULL;
	}
}

void start_power_supply_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[POWER_SUPPLY_TIMER] == NULL) {
		class->timeout_event[POWER_SUPPLY_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[POWER_SUPPLY_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[POWER_SUPPLY_TIMER],
		ms_to_ktime(T_POWER_SUPPLY_MS), HRTIMER_MODE_REL);
}

void stop_power_supply_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[POWER_SUPPLY_TIMER]);
	if (class->timeout_event[POWER_SUPPLY_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[POWER_SUPPLY_TIMER]);
		class->timeout_event[POWER_SUPPLY_TIMER] = NULL;
	}
}

void start_cable_info_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[CABLE_INFO_TIMER] == NULL) {
		class->timeout_event[CABLE_INFO_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[CABLE_INFO_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[CABLE_INFO_TIMER],
		ms_to_ktime(T_CABLE_INFO_RESPONSE_MS), HRTIMER_MODE_REL);
}

void stop_cable_info_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[CABLE_INFO_TIMER]);
	if (class->timeout_event[CABLE_INFO_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[CABLE_INFO_TIMER]);
		class->timeout_event[CABLE_INFO_TIMER] = NULL;
	}
}

void start_restart_trans_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[RESTART_TRANS_TIMER] == NULL) {
		class->timeout_event[RESTART_TRANS_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[RESTART_TRANS_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[RESTART_TRANS_TIMER],
		ms_to_ktime(T_RESTART_TRANS_MS), HRTIMER_MODE_REL);
}

void stop_restart_trans_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[RESTART_TRANS_TIMER]);
	if (class->timeout_event[RESTART_TRANS_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[RESTART_TRANS_TIMER]);
		class->timeout_event[RESTART_TRANS_TIMER] = NULL;
	}
}

void start_cable_trans_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[CABLE_TRANS_TIMER] == NULL) {
		class->timeout_event[CABLE_TRANS_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[CABLE_TRANS_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[CABLE_TRANS_TIMER],
		ms_to_ktime(T_CABLE_TRANS_MS), HRTIMER_MODE_REL);
}

void stop_cable_trans_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[CABLE_TRANS_TIMER]);
	if (class->timeout_event[CABLE_TRANS_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[CABLE_TRANS_TIMER]);
		class->timeout_event[CABLE_TRANS_TIMER] = NULL;
	}
}

void start_msg_trans_delay_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_start(&class->timer[MSG_TRANS_DELAY_TIMER],
		ms_to_ktime(T_MSG_TRANS_DELAY_MS), HRTIMER_MODE_REL);
}

void stop_msg_trans_delay_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[MSG_TRANS_DELAY_TIMER]);
}

void start_handshake_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[HANDSHAKE_TIMER] == NULL) {
		class->timeout_event[HANDSHAKE_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[HANDSHAKE_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[HANDSHAKE_TIMER],
		ms_to_ktime(T_HANDSHAKE_TIMEOUT_MS), HRTIMER_MODE_REL);
}

void stop_handshake_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[HANDSHAKE_TIMER]);
	if (class->timeout_event[HANDSHAKE_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[HANDSHAKE_TIMER]);
		class->timeout_event[HANDSHAKE_TIMER] = NULL;
	}
}

void start_wait_msg_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	if (class->timeout_event[WAIT_MSG_TIMER] == NULL) {
		class->timeout_event[WAIT_MSG_TIMER] =
			devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
		if (class->timeout_event[WAIT_MSG_TIMER] == NULL) {
			ufcs_err("alloc event buf error\n");
			return;
		}
	}

	hrtimer_start(&class->timer[WAIT_MSG_TIMER],
		ms_to_ktime(T_WAIT_MSG_TIMEOUT_MS), HRTIMER_MODE_REL);
}

void stop_wait_msg_timer(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return;
	}

	hrtimer_cancel(&class->timer[WAIT_MSG_TIMER]);
	if (class->timeout_event[WAIT_MSG_TIMER] != NULL) {
		devm_kfree(&class->ufcs->dev, class->timeout_event[WAIT_MSG_TIMER]);
		class->timeout_event[WAIT_MSG_TIMER] = NULL;
	}
}
