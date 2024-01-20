/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: dpi_core.h
** Description: Add dpi core
**
** Version: 1.0
** Date : 2022/6/24
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/6/24 1.0 build this module
****************************************************************/

#ifndef __DPI_CORE_H__
#define __DPI_CORE_H__

#include <linux/hashtable.h>


#define DEFAULT_SPEED_CALC_INTVL (1500) /* unit:ms */
#define DEFAULT_SPEED_EXPIRE (2000)    /* unit:ms */
#define DEFAULT_DPI_TIMEOUT (5 * 1000) /* unit:ms */

#define DPI_HASH_BIT   3

enum dpi_level_type_e {
	DPI_LEVEL_TYPE_UNSPEC,
	DPI_LEVEL_TYPE_UID,
	DPI_LEVEL_TYPE_APP,
	DPI_LEVEL_TYPE_FUNCTION,
	DPI_LEVEL_TYPE_STREAM,
	DPI_LEVEL_TYPE_MAX,
};

enum dpi_match_state_e {
	DPI_MATCH_STATE_START,
	DPI_MATCH_STATE_MATCHING,
	DPI_MATCH_STATE_COMPLETE,
	DPI_MATCH_STATE_MAX,
};

typedef struct {
} dpi_tmgp_sgame_data;


typedef struct {
	union {
		u32 local_ip;
		u32 local_ipv6[4];
	};
	union {
		u32 peer_ip;
		u32 peer_ipv6[4];
	};
	u16 local_port;
	u16 peer_port;
	u8 protocol;
	u8 is_ipv6;
	u16 reserved;
} dpi_tuple_t;

typedef struct {
	u64 bytes;
	u64 last_bytes;
	u64 packets;
	u64 last_packets;
	u64 speed; /* unit:bps */
	u64 byte_uptime;
	u64 speed_uptime;
} stats_dir_t;

typedef struct {
	struct hlist_node node;
	int if_idx;
	stats_dir_t rx_stats;
	stats_dir_t tx_stats;
} dpi_stats_t;

typedef struct {
	dpi_stats_t total_stats;
	int stats_count;
	DECLARE_HASHTABLE(stats_map, DPI_HASH_BIT);
} dpi_hash_stats_t;

typedef struct {
	dpi_tuple_t tuple;
	u32 if_idx;
	u32 uid;
	u64 socket_cookie;
	u64 update_time;
	enum dpi_match_state_e state;
	u64 dpi_result;
	union {
		dpi_tmgp_sgame_data tgmp_data;
	};
} dpi_match_data_t;

typedef struct dpi_result_node_s {
	struct hlist_node node;
	struct hlist_head child_list;
	int child_count;
	enum dpi_level_type_e level_type;
	struct dpi_result_node_s *parent;
	u32 uid;
	u64 update_time;
	u64 dpi_id;
	dpi_hash_stats_t hash_stats;
} dpi_result_node;


typedef struct {
	struct hlist_node tree_node;
	struct hlist_node list_node;
	dpi_result_node *result_node;
	dpi_match_data_t data;
	dpi_stats_t stats;
} dpi_socket_node;


typedef int (*dpi_match_fun)(struct sk_buff *skb, int dir, dpi_match_data_t *data);

int dpi_register_app_match(u32 uid, dpi_match_fun fun);
int dpi_unregister_app_match(u32 uid);



#endif  /* __DPI_CORE_H__ */

