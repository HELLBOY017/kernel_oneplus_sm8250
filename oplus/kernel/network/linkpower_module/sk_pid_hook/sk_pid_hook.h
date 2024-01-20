/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _SK_PID_HOOK_H_
#define _SK_PID_HOOK_H_

#include <net/sock.h>

typedef struct
{
	uint16_t sport;
	uint16_t pid;
} netlink_ul_sport_pid_struct;

typedef struct
{
	uint32_t uid;
} netlink_dl_uid_struct;

typedef struct
{
	uint32_t uid;
	uint32_t pid;
} netlink_dl_proc_struct;

typedef struct
{
	uint32_t uid;
	uint32_t pid;
	uint32_t count;
} netlink_ul_close_sk_struct;

typedef struct
{
	uint32_t uid;
	uint32_t beat_msg_len;
	uint32_t beat_feature_len;
	uint8_t beat_feature[8];
	uint32_t push_feature_len;
	uint8_t push_feature[8];
} netlink_dl_start_monitor_push_sk_struct;

typedef struct
{
	uint32_t uid;
	uint32_t pid;
	uint32_t type;
	uint16_t beat_count;
	uint16_t push_count;
} netlink_unsol_monitor_push_sk_struct;

typedef struct
{
	netlink_dl_proc_struct proc;
	uint16_t connect_count;
	uint16_t syn_snd_rst_count;
	uint16_t syn_rcv_rst_count;
	uint16_t syn_retrans_count;
} netlink_ul_sk_connect_info_struct;

typedef struct
{
	uint32_t v6_saddr32[4];
	uint32_t v6_daddr32[4];
	uint16_t is_ipv6;
	uint16_t protocol;
	uint16_t sport;
	uint16_t dport;
} sk_info_struct;

typedef struct
{
	netlink_dl_start_monitor_push_sk_struct monitor;
	struct sock *sk;
	uint32_t pid;
	uint32_t type;
	uint16_t beat_count;
	uint16_t push_count;
} monitor_push_sk_struct;

#endif /*_SK_PID_HOOK_H_*/
