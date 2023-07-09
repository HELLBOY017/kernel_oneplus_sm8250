/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: comm_netlink.h
** Description: Add Common netlink
**
** Version: 1.0
** Date : 2022/6/23
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/6/23 1.0 build this module
****************************************************************/

#ifndef __COMM_NETLINK_H__
#define __COMM_NETLINK_H__

#include "../include/netlink_api.h"

#define OPLUS_NLA_DATA(na) ((char *)((char *)(na) + NLA_HDRLEN))


typedef struct {
	struct hlist_node node;
	u32 id;
	event_recv_requset cb;
	rsp_data_free free_cb;
} requset_node;

typedef struct {
	struct hlist_node node;
	u32 id;
	event_recv_inform cb;
} inform_node;

typedef struct {
	struct hlist_node node;
	u32 notify_pid;
} notify_node;


#endif  /* __COMM_NETLINK_H__ */
