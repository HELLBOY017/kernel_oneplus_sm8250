/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: tmgp_sgame_stats.h
** Description: wangzherongyao game kernel stats
**
** Version: 1.0
** Date : 2022/6/24
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/7/7 1.0 build this module
****************************************************************/

#ifndef __TMGP_SGAME_STATS_H__
#define __TMGP_SGAME_STATS_H__

#include "../include/ring_data.h"

#define MAX_DOWN_DELAY_COUNT 5

enum wzry_event_type {
	WZRY_ENTRY_UNSPEC,
	WZRY_ENTRY_RX_DEV,
	WZRY_ENTRY_RX_IP,
	WZRY_ENTRY_RX_UDP,
	WZRY_ENTRY_RX_USER,
	WZRY_ENTRY_TX_USER,
	WZRY_ENTRY_TX_UDP,
	WZRY_ENTRY_TX_IP,
	WZRY_ENTRY_TX_DEV,
	WZRY_ENTRY_MAX,
};


typedef struct {
	u64 timestamp;
	u64 rx_ip_delay;
	u64 rx_udp_delay;
	u64 rx_user_delay;
	u64 tx_udp_delay;
	u64 tx_ip_delay;
	u64 tx_sched_delay;
	u32 down_packets;
	u32 up_packets;
	u64 down_packet_delay[MAX_DOWN_DELAY_COUNT];
} wzry_delay_item;


typedef struct {
	u64 rx_dev_time;
	u64 rx_ip_time;
	u64 rx_udp_time;
	u64 rx_user_time;
	u64 tx_user_time;
	u64 tx_udp_time;
	u64 tx_ip_time;
	u64 tx_dev_time;
	u32 down_packets;
	u32 up_packets;
	u64 last_down_time;
	u64 down_packet_delay[MAX_DOWN_DELAY_COUNT];
} wzry_delay_save;





#endif  /* __TMGP_SGAME_STATS_H__ */
