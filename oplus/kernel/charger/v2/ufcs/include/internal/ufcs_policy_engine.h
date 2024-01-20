// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_POLICY_ENGINE_H__
#define __OPLUS_UFCS_POLICY_ENGINE_H__

#include <linux/kthread.h>

enum ufcs_pe_state {
	PE_STATE_IDEL = 0,
	PE_STATE_SOFT_RESET,
	PE_STATE_HW_RESET,
	PE_STATE_SEND_EXIT,
	PE_STATE_RECV_EXIT,
	PE_STATE_HANDSHAKE,
	PE_STATE_PING,
	PE_STATE_OUTPUT_CAP,
	PE_STATE_REQUEST,
	PE_STATE_SEND_SINK_INFO,
	PE_STATE_GET_SOURCE_INFO,
	PE_STATE_GET_CABLE_INFO,
	PE_STATE_GET_DEVICE_INFO,
	PE_STATE_GET_ERROR_INFO,
	PE_STATE_SEND_DEVICE_INFO,
	PE_STATE_SEND_ERROR_INFO,
	PE_STATE_CONFIG_WD,
	PE_STATE_VERIFY_REQUEST,
	PE_STATE_VERIFY_RESPONSE,
	PE_STATE_POWER_CHANGE,
	PE_STATE_GET_EMARK_INFO,
	PE_STATE_GET_POWER_INFO,
	PE_STATE_TEST_REQUEST,
};

struct ufcs_state_info {
	enum ufcs_pe_state curr;
	enum ufcs_pe_state next;
	enum ufcs_pe_state target;
	int err;
};

struct ufcs_class;

int ufcs_policy_engine_init(struct ufcs_class *class);
int ufcs_pe_start_cable_detect(struct ufcs_class *class);
int ufcs_pe_get_cable_info(struct ufcs_class *class);
int ufcs_pe_end_cable_detect(struct ufcs_class *class);
int ufcs_pe_detect_cable_info(struct ufcs_class *class);
void ufcs_exit_sm_work(struct ufcs_class *class);
void ufcs_reset_sm_work(struct ufcs_class *class);
int ufcs_pe_oplus_get_emark_info(struct ufcs_class *class);
int ufcs_pe_oplus_get_power_info(struct ufcs_class *class);
void ufcs_pe_test_handle_work(struct kthread_work *work);
void ufcs_pe_test_request_handle(struct ufcs_class *class, u16 request);

#endif /* __OPLUS_UFCS_POLICY_ENGINE_H__ */
