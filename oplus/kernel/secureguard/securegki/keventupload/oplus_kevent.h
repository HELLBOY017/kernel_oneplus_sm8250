/* SPDX-License-Identifier: GPL-2.0-only */
/**************************************************************
* Copyright (c)  2008- 2030  OPLUS Mobile communication Corp.ltd All rights reserved.
* VENDOR_EDIT
* File       : oplus_kevent.h
* Description: For kevent action upload upload to user layer
* Version   : 1.0
* Date        : 2019-12-19
* Author    :
* TAG         :
****************************************************************/
#ifndef _OPLUS_KEVENT_H
#define _OPLUS_KEVENT_H

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <linux/version.h>



#define NETLINK_OPLUS_KEVENT 34

struct kernel_packet_info
{
    int type;	 /* 0:root,1:only string,other number represent other type */
    char log_tag[32];	/* logTag */
    char event_id[20];	  /*eventID */
    size_t payload_length;	  /* Length of packet data */
    unsigned char payload[0];	/* Optional packet data */
}__attribute__((packed));

int kevent_send_to_user(struct kernel_packet_info *userinfo);

#endif
