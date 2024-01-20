/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _HEARTBEAT_PROXY_MTK_H_
#define _HEARTBEAT_PROXY_MTK_H_

typedef enum
{
	/* process ok */
	HBA_INT_PROCESS_OK = 0,

	/* ipc errors */
	HBA_INT_ERR_IPC_FW = 1,
	HBA_INT_ERR_IPC_KERNEL,
	HBA_INT_ERR_IPC_MD,
	HBA_INT_ERR_IPC_TIMEOUT,

	/* invalid argument errors */
	HBA_INT_ERR_IPV6_NOT_SUPPORT = 10,
	HBA_INT_ERR_PROXY_ALREADY_EXIST,
	HBA_INT_ERR_NULL_PROXY,
	HBA_INT_ERR_NULL_SETTINGS,
	HBA_INT_ERR_INVALID_PROXY,
	HBA_INT_ERR_INVALID_SOCKET,
	HBA_INT_ERR_INVALID_PAYLOAD,

	/* resource errors */
	HBA_INT_ERR_DATA_NETWORK_UNAVAILABLE = 20,
	HBA_INT_ERR_NUM_ALREADY_MAX,
	HBA_INT_ERR_HW_FILTER_EXHAUST,
	HBA_INT_ERR_MD_INTERNAL_ABNORMAL,

	/* runtime errors */
	HBA_INT_ERR_REPEAT_REQUEST = 30,
	HBA_INT_ERR_PROXY_NOT_EXIST,
	HBA_INT_ERR_STATE_NOT_MATCH,
	HBA_INT_ERR_SEQ_SYNC_UNKNOWN,
	HBA_INT_ERR_SEQ_SYNC_SK_NULL,
	HBA_INT_ERR_SEQ_SYNC_SK_OWNED,
	HBA_INT_ERR_SEQ_SYNC_QUEUE_OVERFLOW,
	HBA_INT_ERR_SEQ_SYNC_SKB_INVALID,
	HBA_INT_ERR_SEQ_SYNC_NOT_MATCH,
	HBA_INT_ERR_MD_PASUE,
	HBA_INT_ERR_MD_RESUME,
	HBA_INT_ERR_MD_STOP,
	HBA_INT_ERR_MD_SEDN_TIME_OUT,
	HBA_INT_ERR_MD_ACK_TIME_OUT,
	HBA_INT_ERR_MD_UNKNOWN,
} hba_result_enum;

typedef struct
{
	uint32_t send_count;
	uint32_t recv_count;
	uint32_t hitchhike_count;
} hba_statistics_msg_struct;

typedef struct
{
	char proxy_key[128];
	uint32_t saddr;
	uint32_t daddr;
	uint32_t v6_saddr32[4];
	uint32_t v6_daddr32[4];
	uint16_t is_ipv6;
	uint16_t sport;
	uint16_t dport;
	uint16_t interface_id;
	uint16_t default_phone_id;
	uint16_t is_allow_dynamic_cycle;
	uint16_t cycle;
	uint16_t max_cycle;
	uint16_t cycle_step;
	uint16_t cycle_step_success_num;
	uint16_t tcp_retries2;
	uint16_t send_payload_length;
	uint16_t reply_payload_length;
	uint16_t hitchhike_interval;
	uint8_t send_payload[128];
	uint8_t reply_payload[128];
} netlink_dl_hba_establish_msg_st;

typedef struct
{
	char proxy_key[128];
} netlink_dl_hba_ctrl_msg_st;

typedef struct
{
	char proxy_key[128];
	uint16_t err;
	uint16_t curr_cycle;
	hba_statistics_msg_struct statistics;
} netlink_ul_hba_response_msg_st;

typedef enum
{
	HBA_STATE_NONE,
	HBA_STATE_ESTABLISH_REQUEST,
	HBA_STATE_ESTABLISH_OK,
	HBA_STATE_PAUSE_REQUEST,
	HBA_STATE_PAUSE_OK,
	HBA_STATE_RESUME_REQUEST,
	HBA_STATE_STOP_REQUEST,
	HBA_STATE_ERROR,
} hba_state_enum;

typedef enum
{
	HBA_HW_FILTER_AVALIABLE = 0,
	HBA_HW_FILTER_EXHAUST = 1,
} hba_hwfltr_st_enum;

typedef struct
{
	uint8_t saddr[4];
	uint8_t daddr[4];
	uint8_t v6_saddr8[16];
	uint8_t v6_daddr8[16];
	uint16_t is_ipv6;
	uint16_t protocol;
	uint16_t sport;
	uint16_t dport;
	uint32_t tcp_retries2;
} hba_netinfo_ctrl_msg_struct;

typedef struct
{
	uint8_t ipv4_tos;
	uint8_t ipv4_ttl;
	uint16_t ipv4_id;
	uint32_t ipv6_flow_label;
	uint8_t ipv6_hop_limit;
	uint8_t ipv6_tclass;
	uint8_t tcp_rcv_wscale;
	uint8_t tcp_snd_wscale;
	uint32_t tcp_wscale_ok;
	uint16_t tcp_snd_wnd;
	uint16_t tcp_rcv_wnd;
	uint32_t tcp_snd_nxt;
	uint32_t tcp_rcv_nxt;
} hba_seq_sync_msg_struct;

typedef struct
{
	char proxy_key[128];
	uint16_t interface_id;
	uint16_t default_phone_id;
	uint32_t hb_len;
	uint8_t hb_pattern[128];
	uint32_t hback_len;
	uint8_t hback_pattern[128];
	uint16_t hitchhike_interval;
	uint16_t is_allow_dynamic_cycle;
	uint16_t cycle;
	uint16_t max_cycle;
	uint16_t cycle_step;
	uint16_t cycle_step_success_num;
	hba_seq_sync_msg_struct seq_sync;
	hba_netinfo_ctrl_msg_struct netinfo_ctrl;
} hba_estblish_req_struct;

typedef struct
{
	char proxy_key[128];
	hba_result_enum result; /*0: success, >0 fail, result_enum will be added to illustrate the detail cause */
} hba_estblish_cnf_struct;

typedef struct
{
	char proxy_key[128];
	uint16_t default_phone_id;
	hba_seq_sync_msg_struct seq_sync;
} hba_resume_req_struct;

typedef struct
{
	char proxy_key[128];
	hba_result_enum result; /*0: success, >0 fail*/
} hba_resume_cnf_struct;

typedef enum
{
	HBA_CTRL_MODE_UNKNOWN,
	HBA_CTRL_MODE_PAUSE,
	HBA_CTRL_MODE_STOP,
} hba_ctrl_mode_enum;

/* use for pause or stop */
typedef struct
{
	char proxy_key[128];
	uint16_t default_phone_id;
	hba_ctrl_mode_enum mode; /*0: pause, 1: stop*/
} hba_ctrl_req_struct;

typedef struct
{
	char proxy_key[128];
	hba_ctrl_mode_enum mode; /*0: pause, 1: stop*/
	hba_result_enum result;	 /*0: success, >0 fail*/
	uint16_t curr_cycle;
	hba_seq_sync_msg_struct seq_sync;
	hba_statistics_msg_struct statistics;
} hba_ctrl_cnf_struct;

typedef struct
{
	char proxy_key[128];
	uint16_t default_phone_id;
} hba_send_now_req_struct;

typedef struct
{
	char proxy_key[128];
	hba_result_enum result;
} hba_send_now_cnf_struct;

typedef struct
{
	char proxy_key[128];
	hba_result_enum result;
	uint16_t curr_cycle;
	hba_seq_sync_msg_struct seq_sync;
	hba_statistics_msg_struct statistics;
} hba_timeout_ind_struct;

typedef struct
{
	hba_hwfltr_st_enum hwfltr_st; /* hard ware filter source status, 0 - avaliabe (recovery from exhaust); 1 - exhaust*/
} hba_hwfltr_st_ind_struct;

#endif /*_HEARTBEAT_PROXY_MTK_H_*/
