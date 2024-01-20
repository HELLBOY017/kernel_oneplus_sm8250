// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_TIMER_H__
#define __OPLUS_UFCS_TIMER_H__

#include <linux/hrtimer.h>

enum ufcs_timer {
	ACK_RECEIVE_TIMER = 0,
	SENDER_RESPONSE_TIMER,
	POWER_SUPPLY_TIMER,
	CABLE_INFO_TIMER,
	RESTART_TRANS_TIMER,
	CABLE_TRANS_TIMER,
	MSG_TRANS_DELAY_TIMER,
	HANDSHAKE_TIMER,
	WAIT_MSG_TIMER,
	TIMER_MAX,
};

/* ufcs spec 7.3.8 */
#define T_ACK_RECEIVE_MS		10
#define T_SENDER_RESPONSE_MS		50
#define T_POWER_SUPPLY_MS		550
#define T_CABLE_INFO_RESPONSE_MS	1200
#define T_RESTART_TRANS_MS		1100
#define T_CABLE_TRANS_MS		1000
#define T_MSG_TRANS_DELAY_MS		10   /* oplus requires 5-40ms */
#define T_ACK_TRANSMIT_US		100
#define T_RETRY_US			500
#define T_RECEIVER_RESPONSE_MS		40

#define T_ACK_RECEIVE_NO_RESEND_MS	50

/* customize */
#define T_HANDSHAKE_TIMEOUT_MS		300
#define T_WAIT_MSG_TIMEOUT_MS		300

struct ufcs_class;

int ufcs_timer_init(struct ufcs_class *class);
void start_ack_receive_timer(struct ufcs_class *class);
void stop_ack_receive_timer(struct ufcs_class *class);
void start_sender_response_timer(struct ufcs_class *class);
void stop_sender_response_timer(struct ufcs_class *class);
void start_power_supply_timer(struct ufcs_class *class);
void stop_power_supply_timer(struct ufcs_class *class);
void start_cable_info_timer(struct ufcs_class *class);
void stop_cable_info_timer(struct ufcs_class *class);
void start_restart_trans_timer(struct ufcs_class *class);
void stop_restart_trans_timer(struct ufcs_class *class);
void start_cable_trans_timer(struct ufcs_class *class);
void stop_cable_trans_timer(struct ufcs_class *class);
void start_msg_trans_delay_timer(struct ufcs_class *class);
void stop_msg_trans_delay_timer(struct ufcs_class *class);
void start_handshake_timer(struct ufcs_class *class);
void stop_handshake_timer(struct ufcs_class *class);
void start_wait_msg_timer(struct ufcs_class *class);
void stop_wait_msg_timer(struct ufcs_class *class);

#endif /* __OPLUS_UFCS_TIMER_H__ */
