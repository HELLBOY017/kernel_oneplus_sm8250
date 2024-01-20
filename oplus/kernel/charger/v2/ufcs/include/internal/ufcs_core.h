// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_CORE_H__
#define __OPLUS_UFCS_CORE_H__

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/completion.h>
#include <linux/wait.h>

#include "ufcs_class.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"
#include "ufcs_timer.h"
#include "ufcs_policy_engine.h"

extern int ufcs_log_level;

enum {
	LOG_LEVEL_OFF = 0,
	LOG_LEVEL_ERR,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_ALL_INFO,
	LOG_LEVEL_ALL_ERR,
};

#define ufcs_err(fmt, ...)                                                      \
	({                                                                     \
		if (ufcs_log_level >= LOG_LEVEL_ERR)                          \
			printk(KERN_ERR "[ERROR]: UFCS" pr_fmt(fmt),      \
			       ##__VA_ARGS__);                                 \
	})
#define ufcs_info(fmt, ...)                                                     \
	({                                                                     \
		if (ufcs_log_level >= LOG_LEVEL_ALL_ERR)                      \
			printk(KERN_ERR "[INFO]: UFCS" pr_fmt(fmt),       \
			       ##__VA_ARGS__);                                 \
		else if (ufcs_log_level >= LOG_LEVEL_INFO)                    \
			printk(KERN_INFO "[INFO]: UFCS" pr_fmt(fmt),      \
			       ##__VA_ARGS__);                                 \
	})
#define ufcs_debug(fmt, ...)                                                    \
	({                                                                     \
		if (ufcs_log_level >= LOG_LEVEL_ALL_ERR)                      \
			printk(KERN_ERR "[DEBUG]: UFCS" pr_fmt(fmt),      \
			       ##__VA_ARGS__);                                 \
		else if (ufcs_log_level >= LOG_LEVEL_ALL_INFO)                \
			printk(KERN_INFO "[DEBUG]: UFCS" pr_fmt(fmt),     \
			       ##__VA_ARGS__);                                 \
		else if (ufcs_log_level >= LOG_LEVEL_DEBUG)                   \
			printk(KERN_NOTICE "[DEBUG]: UFCS" pr_fmt(fmt),   \
			       ##__VA_ARGS__);                                 \
	})

struct ufcs_pdo_info {
	u8 num;
	u64 data[UFCS_OUTPUT_MODE_MAX];
};

struct ufcs_power_info_ext {
	u8 num;
	u64 data[UFCS_OPLUS_VND_POWER_INFO_MAX];
};

struct ufcs_verify_info {
	u8 random_data[UFCS_VERIFY_RANDOM_DATA_SIZE];
	u8 auth_data[UFCS_VERIFY_AUTH_DATA_SIZE];
};

struct ufcs_class {
	struct ufcs_dev *ufcs;
	struct kthread_worker *worker;
	struct kthread_work event_work;
	struct kthread_work msg_send_work;
	struct kthread_work recv_work;
	struct kthread_work test_handle_work;
	struct work_struct ack_nck_work;
	struct task_struct *sm_task;
	wait_queue_head_t sm_wq;

	struct hrtimer timer[TIMER_MAX];
	struct ufcs_event *timeout_event[TIMER_MAX];
	struct ufcs_msg_sender sender;
	struct ufcs_msg *recv_msg;

	struct ufcs_pdo_info pdo;
	struct ufcs_event_info event;
	struct ufcs_state_info state;
	struct ufcs_config config;
	struct ufcs_verify_info verify_info;

	u64 src_info;
	u64 cable_info;
	u64 dev_info;
	u32 err_info;
	u32 pwr_change_info[UFCS_OUTPUT_MODE_MAX];
	u16 test_request;
	bool power_changed;
	bool test_mode;
	bool vol_acc_test;
	bool verify_pass;

	u64 emark_info;
	struct ufcs_power_info_ext pie;

	bool handshake_success;
	bool sm_task_wakeup;
	bool start_cable_detect;

	struct mutex pe_lock;
	struct mutex handshake_lock;
	struct mutex ext_req_lock;
	struct completion request_ack;
};

void ufcs_clean_process_info(struct ufcs_class *class);

#endif /* __OPLUS_UFCS_CORE_H__ */
