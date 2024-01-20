/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: netlink_api.h
** Description: Add Netlink interface
**
** Version: 1.0
** Date : 2022/6/23
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/6/23 1.0 build this module
****************************************************************/


#ifndef __NETLINK_API_H__
#define __NETLINK_API_H__


#include <linux/types.h>
#include <linux/list.h>

#include "../proto-src/netlink_msg.pb-c.h"


enum comm_netlink_errcode_e {
	COMM_NETLINK_SUCC = 0,
	COMM_NETLINK_FAIL = 1,
	/* need to add errcode here */
	COMM_NETLINK_ERR_NONE_RECIEVER = 2,
	COMM_NETLINK_ERR_PARAM = 3,
	COMM_NETLINK_ERR_MEMORY = 4,
};


enum comm_netlink_event_id_e {
	COMM_NETLINK_EVENT_UNSPEC = 0,

	/* dpi event 11-50 */
	COMM_NETLINK_EVENT_SET_DPI_UID = 11,
	COMM_NETLINK_EVENT_GET_DPI_STREAM_SPEED = 12,
	COMM_NETLINK_EVENT_GET_ALL_UID_DPI_SPEED = 13,
	COMM_NETLINK_EVENT_SET_DPI_MATCH_ALL_UID = 14,

	/* game tpgmp 51-100 */
	COMM_NETLINK_EVENT_GET_WZRY_SERVER_IP = 51,
	COMM_NETLINK_EVENT_NOTIFY_WZRY_SERVER_IP = 52,
	COMM_NETLINK_EVENT_GET_WZRY_DATA = 53,
	COMM_NETLINK_EVENT_GET_WZRY_DELAY = 54,

	/* log stream identify*/
	COMM_NETLINK_EVENT_SET_LOG_STREAM_IPADDR = 101,

	COMM_NETLINK_EVENT_MAX,
};


enum comm_netlink_type_e {
	COMM_NETLINK_TYPE_UNSPEC,
	COMM_NETLINK_TYPE_REQUEST,
	COMM_NETLINK_TYPE_INFORM,
};

typedef void (*rsp_data_free)(void *data);
/* return :enum comm_netlink_errcode_e */
typedef int (*event_recv_requset)(u32 eventid, Netlink__Proto__RequestMessage *requestMsg, char **rsp_data, u32 *rsp_len);
typedef void (*event_recv_inform)(u32 eventid, Netlink__Proto__InformMessage *infoMsg);


/* request just allow to set once */
int register_netlink_request(u32 eventid, event_recv_requset request_cb, rsp_data_free free_cb);
int unregister_netlink_request(u32 eventid);

int register_netlink_inform(u32 eventid, event_recv_inform inform_cb);
int unregister_netlink_inform(u32 eventid, event_recv_inform inform_cb);


int notify_netlink_event(char *data, u32 len);


#define NETLINK_RSP_DATA_DECLARE(rsp_name, request_id, event_id, err_code) \
	Netlink__Proto__ResponseMessage rsp_name = NETLINK__PROTO__RESPONSE_MESSAGE__INIT; \
	Netlink__Proto__MessageHeader header = NETLINK__PROTO__MESSAGE_HEADER__INIT; \
	header.requestid = request_id; \
	header.eventid = event_id; \
	header.retcode = err_code; \
	rsp_name.header = &header



#endif  /* __NETLINK_API_H__ */
