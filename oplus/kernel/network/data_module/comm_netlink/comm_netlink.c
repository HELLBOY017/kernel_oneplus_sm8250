/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: comm_netlink.c
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
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netlink.h>
#include <linux/random.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <net/dst.h>
#include <net/genetlink.h>

#include "comm_netlink.h"

#include "../include/comm_def.h"
#include "../include/netlink_api.h"
#include "../proto-src/netlink_msg.pb-c.h"


#define LOG_TAG "comm_netlink"

#define logt(fmt, args...) LOG(LOG_TAG, fmt, ##args)

#define NETLINK_COMM_FAMILY_NAME "comm_netlink"
#define NETLINK_COMM_FAMILY_VERSION 1


enum comm_netlink_cmd_type_e {
	COMM_NETLINK_CMD_UNSPEC,
	COMM_NETLINK_CMD_DOWN,
	COMM_NETLINK_CMD_UP,
	COMM_NETLINK_CMD_MAX,
};

enum comm_netlink_msg_type_e {
	COMM_NETLINK_MSG_UNSPEC,
	COMM_NETLINK_MSG_REGISTER,
	COMM_NETLINK_MSG_UNREGISTER,
	COMM_NETLINK_MSG_REQUEST,
	COMM_NETLINK_MSG_RESPONSE,
	COMM_NETLINK_MSG_INFORM,
	COMM_NETLINK_MSG_NOTIFY,
	COMM_NETLINK_MSG_MAX,
};

static spinlock_t s_request_list_lock;
static spinlock_t s_inform_list_lock;
static spinlock_t s_notify_list_lock;

static struct hlist_head s_request_list;
static struct hlist_head s_inform_list;
static struct hlist_head s_notify_list;

static inline int comm_netlink_prepare_usr_msg(u8 cmd, size_t size, u32 pid, struct sk_buff **skbp);



int register_netlink_request(u32 eventid, event_recv_requset request_cb, rsp_data_free free_cb)
{
	requset_node *pos = NULL;

	spin_lock(&s_request_list_lock);

	hlist_for_each_entry(pos, &s_request_list, node) {
		if (pos->id == eventid) {
			if (pos->cb != request_cb) {
				spin_unlock(&s_request_list_lock);
				logt("register failed! already exist!");
				return -1;
			}
			spin_unlock(&s_request_list_lock);
			return 0;
		}
	}

	pos = kmalloc(sizeof(requset_node), GFP_ATOMIC);
	if (!pos) {
		spin_unlock(&s_request_list_lock);
		logt("malloc size %lu failed!", sizeof(requset_node));
		return -1;
	}
	INIT_HLIST_NODE(&pos->node);
	pos->id = eventid;
	pos->cb = request_cb;
	pos->free_cb = free_cb;
	hlist_add_head(&pos->node, &s_request_list);
	spin_unlock(&s_request_list_lock);
	return 0;
}
EXPORT_SYMBOL(register_netlink_request);


int unregister_netlink_request(u32 eventid)
{
	requset_node *pos = NULL;
	struct hlist_node *next = NULL;

	spin_lock(&s_request_list_lock);

	hlist_for_each_entry_safe(pos, next, &s_request_list, node) {
		if (pos->id == eventid) {
			hlist_del(&pos->node);
			kfree(pos);
			spin_unlock(&s_request_list_lock);
			return 0;
		}
	}
	spin_unlock(&s_request_list_lock);
	return 0;
}
EXPORT_SYMBOL(unregister_netlink_request);


int register_netlink_inform(u32 eventid, event_recv_inform inform_cb)
{
	inform_node *pos = NULL;

	spin_lock(&s_inform_list_lock);

	hlist_for_each_entry(pos, &s_inform_list, node) {
		if ((pos->id == eventid) && (pos->cb == inform_cb)) {
			spin_unlock(&s_inform_list_lock);
			return 0;
		}
	}

	pos = kmalloc(sizeof(inform_node), GFP_ATOMIC);
	if (!pos) {
		spin_unlock(&s_inform_list_lock);
		logt("malloc size %lu failed!", sizeof(inform_node));
		return -1;
	}
	INIT_HLIST_NODE(&pos->node);
	pos->id = eventid;
	pos->cb = inform_cb;
	hlist_add_head(&pos->node, &s_inform_list);
	spin_unlock(&s_inform_list_lock);

	return 0;
}
EXPORT_SYMBOL(register_netlink_inform);


int unregister_netlink_inform(u32 eventid, event_recv_inform inform_cb)
{
	inform_node *pos = NULL;
	struct hlist_node *next = NULL;

	spin_lock(&s_inform_list_lock);

	hlist_for_each_entry_safe(pos, next, &s_inform_list, node) {
		if ((pos->id == eventid) && (pos->cb == inform_cb)) {
			hlist_del(&pos->node);
			kfree(pos);
			spin_unlock(&s_inform_list_lock);
			return 0;
		}
	}
	spin_unlock(&s_inform_list_lock);
	return 0;
}
EXPORT_SYMBOL(unregister_netlink_inform);



static int add_notify_client(u32 pid)
{
	notify_node *pos = NULL;

	spin_lock(&s_notify_list_lock);
	hlist_for_each_entry(pos, &s_notify_list, node) {
		if (pos->notify_pid == pid) {
			spin_unlock(&s_notify_list_lock);
			return 0;
		}
	}

	pos = kmalloc(sizeof(notify_node), GFP_ATOMIC);
	if (!pos) {
		spin_unlock(&s_notify_list_lock);
		logt("malloc size %lu failed!", sizeof(notify_node));
		return -1;
	}
	logt("new notify client: %u", pid);
	INIT_HLIST_NODE(&pos->node);
	pos->notify_pid = pid;
	hlist_add_head(&pos->node, &s_notify_list);
	spin_unlock(&s_notify_list_lock);

	return 0;
}

static int del_notify_client(u32 pid)
{
	notify_node *pos = NULL;
	struct hlist_node *next = NULL;

	spin_lock(&s_notify_list_lock);

	hlist_for_each_entry_safe(pos, next, &s_inform_list, node) {
		if (pos->notify_pid == pid) {
			hlist_del(&pos->node);
			kfree(pos);
			spin_unlock(&s_notify_list_lock);
			return 0;
		}
	}
	logt("del_notify_client not find %u", pid);
	spin_unlock(&s_notify_list_lock);
	return 0;
}



static int send_one_msg(u32 pid, u16 cmd_type, char *data, int len)
{
	int ret = 0;
	void * head = NULL;
	struct sk_buff *skbuff = NULL;
	size_t size;

	size = nla_total_size(len);
	ret = comm_netlink_prepare_usr_msg(COMM_NETLINK_CMD_UP, size, pid, &skbuff);
	if (ret) {
		return ret;
	}

	ret = nla_put(skbuff, cmd_type, len, data);
	if (ret) {
		kfree_skb(skbuff);
		return ret;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skbuff)));
	genlmsg_end(skbuff, head);

	/* send data */
	ret = genlmsg_unicast(&init_net, skbuff, pid);
	if(ret < 0) {
		logt("genlmsg_unicast return error, ret = %d", ret);
		return -1;
	}
	return 0;
}


int notify_netlink_event(char *data, u32 len)
{
	notify_node *pos = NULL;

	spin_lock(&s_notify_list_lock);
	hlist_for_each_entry(pos, &s_notify_list, node) {
		send_one_msg(pos->notify_pid, COMM_NETLINK_MSG_NOTIFY, data, len);
	}
	spin_unlock(&s_notify_list_lock);

	return 0;
}
EXPORT_SYMBOL(notify_netlink_event);

static int comm_netlink_send_rsp_data(u32 pid, int requestId, int eventId, int errCode)
{
	size_t len = 0;
	char *buf = NULL;
	NETLINK_RSP_DATA_DECLARE(errRsp, requestId, eventId, errCode);

	len = netlink__proto__response_message__get_packed_size(&errRsp);
	buf = kmalloc(len, GFP_ATOMIC);
	if (!buf) {
		logt("malloc size %lu failed", len);
		return -1;
	}
	send_one_msg(pid, COMM_NETLINK_MSG_RESPONSE, buf, len);
	kfree(buf);
	return 0;
}


static int comm_netlink_requset_msg(u32 pid, char *data, int len)
{
	Netlink__Proto__RequestMessage *requestMsg = NULL;
	requset_node *pos = NULL;
	event_recv_requset request_cb = NULL;
	rsp_data_free free_fun = NULL;
	char *rsp = NULL;
	u32 rsp_len = 0;
	int ret = 0;

	requestMsg = netlink__proto__request_message__unpack((ProtobufCAllocator*)NULL, (size_t)len, (const uint8_t *)data);
	if (requestMsg == NULL) {
		logt("requestMsg unpack failed!");
		return -1;
	}
	if (requestMsg->header == NULL) {
		logt("header is null!");
		return -1;
	}
	spin_lock(&s_request_list_lock);
	hlist_for_each_entry(pos, &s_request_list, node) {
		if (pos->id == requestMsg->header->eventid) {
			request_cb = pos->cb;
			free_fun = pos->free_cb;
			break;
		}
	}
	spin_unlock(&s_request_list_lock);

	if (!request_cb) {
		comm_netlink_send_rsp_data(pid, requestMsg->header->requestid, requestMsg->header->eventid, COMM_NETLINK_ERR_NONE_RECIEVER);
	} else {
		ret = request_cb(requestMsg->header->eventid, requestMsg, &rsp, &rsp_len);
		if (ret != COMM_NETLINK_SUCC) {
			comm_netlink_send_rsp_data(pid, requestMsg->header->requestid, requestMsg->header->eventid, ret);
		} else {
			send_one_msg(pid, COMM_NETLINK_MSG_RESPONSE, rsp, rsp_len);
		}
		if (rsp && free_fun) {
			free_fun(rsp);
		}
	}
	netlink__proto__request_message__free_unpacked(requestMsg, NULL);

	return 0;
}


static int comm_netlink_inform_msg(u32 pid, char *data, int len)
{
	Netlink__Proto__InformMessage *infoMsg = NULL;
	inform_node *pos = NULL;

	infoMsg = netlink__proto__inform_message__unpack(NULL, len, data);
	if (infoMsg == NULL) {
		logt("infoMsg unpack failed!");
		return -1;
	}
	if (infoMsg->header == NULL) {
		logt("header is null!");
		return -1;
	}

	spin_lock(&s_inform_list_lock);
	hlist_for_each_entry(pos, &s_inform_list, node) {
		if (pos->id == infoMsg->header->eventid) {
			pos->cb(infoMsg->header->eventid, infoMsg);
		}
	}
	spin_unlock(&s_inform_list_lock);
	netlink__proto__inform_message__free_unpacked(infoMsg, NULL);

	return 0;
}

static int comm_netlink_recv_msg(struct sk_buff *skb, struct genl_info *info)
{
	struct genlmsghdr *genlhdr = NULL;
	struct nlattr *nla = NULL;
	u32 pid = 0;

	pid = info->snd_portid;
	genlhdr = info->genlhdr;
	nla = genlmsg_data(genlhdr);

	switch (nla->nla_type) {
	case COMM_NETLINK_MSG_REGISTER:
		add_notify_client(pid);
		break;
	case COMM_NETLINK_MSG_UNREGISTER:
		del_notify_client(pid);
		break;
	case COMM_NETLINK_MSG_REQUEST:
		comm_netlink_requset_msg(pid, nla_data(nla), nla_len(nla));
		break;
	case COMM_NETLINK_MSG_INFORM:
		comm_netlink_inform_msg(pid, nla_data(nla), nla_len(nla));
		break;
	default:
		logt("unsupport nla_type %d", nla->nla_type);
		break;
	}
	return 0;
}



static const struct genl_ops comm_genl_ops[] = {
	{
		.cmd = COMM_NETLINK_CMD_DOWN,
		.flags = 0,
		.doit = comm_netlink_recv_msg,
		.dumpit = NULL,
	},
};


static struct genl_family comm_genl_family = {
	.id = 0,
	.hdrsize = 0,
	.name = NETLINK_COMM_FAMILY_NAME,
	.version = NETLINK_COMM_FAMILY_VERSION,
	.maxattr = COMM_NETLINK_CMD_MAX,
	.ops = comm_genl_ops,
	.n_ops = ARRAY_SIZE(comm_genl_ops),
};


static inline int comm_netlink_prepare_usr_msg(u8 cmd, size_t size, u32 pid, struct sk_buff **skbp)
{
	struct sk_buff *skb;
	/* create a new netlink msg */
	skb = genlmsg_new(size, GFP_ATOMIC);

	if (skb == NULL) {
		return -ENOMEM;
	}

	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &comm_genl_family, 0, cmd);
	*skbp = skb;
	return 0;
}


int comm_netlink_module_init(void)
{
	int ret = 0;

	spin_lock_init(&s_request_list_lock);
	spin_lock_init(&s_inform_list_lock);
	spin_lock_init(&s_notify_list_lock);
	INIT_HLIST_HEAD(&s_request_list);
	INIT_HLIST_HEAD(&s_inform_list);
	INIT_HLIST_HEAD(&s_notify_list);

	ret = genl_register_family(&comm_genl_family);
	logt("genl_register_family init return %d", ret);

	return ret;
}

void comm_netlink_module_exit(void)
{
	genl_unregister_family(&comm_genl_family);
}



