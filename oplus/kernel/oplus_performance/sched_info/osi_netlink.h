/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#ifndef CPU_NETLINK_H
#define CPU_NETLINK_H



#include <linux/version.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/genetlink.h>


#define USE_GEN_NETLINK

/*******************************
 * FIXME: BYHP
 * You need to add a number to the following header file
 * <include\uapi\linux\netlink.h>
 *******************************/
#define NETLINK_OPLUS_CPU 31

#define SEND_DATA_LEN 2

enum sock_no {
	CPU_HIGH_LOAD = 1,
	NOTIFY_CPU_RQ = 2,
	PROC_COMM = 3,
	PROC_AUX_COMM = 4,
	PROC_LOAD = 5,
	PROC_AUX_COMM_FORK = 6,
	PROC_AUX_COMM_REMOVE = 7,
	BG_PROCS_DSTATE = 8,
	HISTORY_TOTAL_LOADS = 9,
	PERSEC_HIGH_LOAD = 10,
	ALL_GROUP_BACK_TO_LOW = 11
};


#define OPLUS_CPULOAD_FAMILY_NAME			"oplus_cpu_nl"
#define OPLUS_CPULOAD_FAMILY_VER			1
#define OPLUS_CPULOAD_GENL_ID_GENERATE		0

/* attribute type */
enum {
	OPLUS_CPULOAD_ATTR_MSG_UNDEFINE = 0,
	OPLUS_CPULOAD_ATTR_MSG_GENL,
	__OPLUS_CPULOAD_ATTR_MSG_MAX
};
#define OPLUS_CPULOAD_ATTR_MSG_MAX (__OPLUS_CPULOAD_ATTR_MSG_MAX - 1)

/* cmd type */
enum {
	OPLUS_CPULOAD_CMD_UNDEFINE = 0,
	OPLUS_CPULOAD_CMD_RECV,				/* recv_from_user */
	OPLUS_CPULOAD_CMD_SEND,				/* send_to_user */
	__OPLUS_CPULOAD_CMD_MAX,
};
#define OPLUS_CPULOAD_CMD_MAX (__OPLUS_CPULOAD_CMD_MAX - 1)


int send_to_user(int sock_no, size_t len, const int *data);
void create_cpu_netlink(int socket_no);
void destroy_cpu_netlink(void);

#endif	/* CPU_NETLINK_H */
