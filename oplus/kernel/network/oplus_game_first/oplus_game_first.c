// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2028 Oplus. All rights reserved.
 */
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/version.h>
#include <net/tcp.h>
#include <net/dst.h>
#include <linux/file.h>
#include <net/tcp_states.h>
#include <linux/netlink.h>
#include <net/genetlink.h>
#include <linux/tcp.h>
#include <net/inet_connection_sock.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#ifdef TRACE_FUNC_ON
#include <trace/hooks/ipv4.h>
#endif
#include <trace/events/net.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/preempt.h>

#define LOG_TAG "GAME_FIRST"
#define LOGK(fmt, args...)	\
	do {							\
		if (s_debug) {				\
			printk("[%s]:" fmt "\n", LOG_TAG, ##args);	\
		}												\
	} while (0)

#define OPLUS_TRUE 1
#define OPLUS_FALSE 0

#define DPI_ID_TMGP_SGAME_STREAM_GAME_DATA  0x10101
#define GAME_SKB_PRIORITY 6
#define GAME_THREAD_NAME "Apoll"

static int s_debug = 0;
static int s_game_uid = 0;
static int s_battle_flag = OPLUS_FALSE;
static int s_game_alive = OPLUS_FALSE;
static struct ctl_table_header *oplus_game_first_ctl_header = NULL;
static int oplus_game_first_user_pid = 0;
static int s_last_tgid = 0;
static int s_last_pid = 0;
static int s_tx_first = OPLUS_FALSE;
static int s_rx_first = OPLUS_FALSE;
static int s_test_tgid = 0;
static int s_test_pid = 0;
static int s_test_mode = 0;

enum oplus_game_first_msg_type_et {
	OPLUS_GAME_FIRST_MSG_UNSPEC,
	OPLUS_GAME_FIRST_MSG_SET_GAME_UID,
	OPLUS_GAME_FIRST_MSG_BATTLE_STATE,
	OPLUS_GAME_FIRST_MSG_GAME_EXIT,
	OPLUS_GAME_FIRST_MSG_REPORT_THREAD_INFO,
	__OPLUS_GAME_FIRST_MSG_MAX,
};
#define OPLUS_GAME_FIRST_MSG_MAX (__OPLUS_GAME_FIRST_MSG_MAX - 1)

enum oplus_game_first_cmd_type_et {
	OPLUS_GAME_FIRST_CMD_UNSPEC,
	OPLUS_GAME_FIRST_CMD_DOWNLINK,
	__OPLUS_GAME_FIRST_CMD_MAX,
};
#define OPLUS_GAME_FIRST_CMD_MAX (__OPLUS_GAME_FIRST_CMD_MAX - 1)

#define OPLUS_GAME_FIRST_FAMILY_VERSION	1
#define OPLUS_GAME_FIRST_FAMILY_NAME "game_first"
#define NLA_DATA(na)		((char *)((char*)(na) + NLA_HDRLEN))

struct game_thread_info_st
{
	u32 uid;
	u32 tgid;
	u32 pid;
};

static int oplus_game_first_netlink_rcv_msg(struct sk_buff *skb, struct genl_info *info);
static int oplus_game_first_send_netlink_msg(int msg_type, char *payload, int payload_len);
static const struct genl_ops oplus_game_first_genl_ops[] =
{
	{
		.cmd = OPLUS_GAME_FIRST_CMD_DOWNLINK,
		.flags = 0,
		.doit = oplus_game_first_netlink_rcv_msg,
		.dumpit = NULL,
	},
};

static struct genl_family oplus_game_first_genl_family =
{
	.id = 0,
	.hdrsize = 0,
	.name = OPLUS_GAME_FIRST_FAMILY_NAME,
	.version = OPLUS_GAME_FIRST_FAMILY_VERSION,
	.maxattr = OPLUS_GAME_FIRST_MSG_MAX,
	.ops = oplus_game_first_genl_ops,
	.n_ops = ARRAY_SIZE(oplus_game_first_genl_ops),
};

static struct ctl_table oplus_game_first_sysctl_table[] = {
	{
		.procname = "s_debug",
		.data = &s_debug,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_game_uid",
		.data = &s_game_uid,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_test_tgid",
		.data = &s_test_tgid,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_test_pid",
		.data = &s_test_pid,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_test_mode",
		.data = &s_test_mode,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "tx_first",
		.data = &s_tx_first,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "rx_first",
		.data = &s_rx_first,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{}
};

static inline int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp)
{
	struct sk_buff *skb;
	/* create a new netlink msg */
	skb = genlmsg_new(size, GFP_ATOMIC);
	if (skb == NULL) {
		return -ENOMEM;
	}

	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &oplus_game_first_genl_family, 0, cmd);
	*skbp = skb;
	return 0;
}

static inline int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
	int ret;
	/* add a netlink attribute to a socket buffer */
	if ((ret = nla_put(skb, type, len, data)) != 0) {
		return ret;
	}

	return 0;
}

/* send to user space */
static int oplus_game_first_send_netlink_msg(int msg_type, char *payload, int payload_len)
{
	int ret = 0;
	void * head;
	struct sk_buff *skbuff;
	size_t size;

	if (!oplus_game_first_user_pid) {
		printk("oplus_game_first_send_netlink_msg, oplus_game_first_user_pid=0\n");
		return -1;
	}

	/* allocate new buffer cache */
	size = nla_total_size(payload_len);
	ret = genl_msg_prepare_usr_msg(OPLUS_GAME_FIRST_CMD_DOWNLINK, size, oplus_game_first_user_pid, &skbuff);
	if (ret) {
		return ret;
	}

	ret = genl_msg_mk_usr_msg(skbuff, msg_type, payload, payload_len);
	if (ret) {
		kfree_skb(skbuff);
		return ret;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skbuff)));
	genlmsg_end(skbuff, head);

	/* send data */
	ret = genlmsg_unicast(&init_net, skbuff, oplus_game_first_user_pid);
	if (ret < 0) {
		if (net_ratelimit()) {
			LOGK("oplus_game_first_send_netlink_msg error, ret = %d", ret);
		}
		return -1;
	}

	return 0;
}

static void oplus_game_first_handle_set_s_game_uid(struct nlattr *nla)
{
	u32 *data = (u32*)NLA_DATA(nla);
	s_game_uid = data[0];
	LOGK("set s_game_uid = %u", s_game_uid);
	return;
}

static void oplus_game_first_handle_battle_state(struct nlattr *nla)
{
	u32 *data = (u32*)NLA_DATA(nla);
	s_battle_flag = data[0];
	s_tx_first = data[1];
	s_rx_first = data[2];
	printk("[GAME_FIRST]:s_battle_flag=%u,s_tx_first=%d,s_rx_first=%d\n", s_battle_flag, s_tx_first, s_rx_first);

	if (s_battle_flag) {
		s_game_alive = OPLUS_TRUE;
		s_last_tgid = 0;
		s_last_pid = 0;
	}

	return;
}

static void oplus_game_first_handle_game_exit(struct nlattr *nla)
{
	s_battle_flag = OPLUS_FALSE;
	s_game_alive = OPLUS_FALSE;
	s_last_tgid = 0;
	s_last_pid = 0;
}

static int oplus_game_first_netlink_rcv_msg(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	if (oplus_game_first_user_pid == 0) {
		oplus_game_first_user_pid = nlhdr->nlmsg_pid;
		LOGK("update oplus_game_first_user_pid=%u", oplus_game_first_user_pid);
	}

	/* to do: may need to some head check here*/
	LOGK("oplus_game_first_netlink_rcv_msg type=%u", nla->nla_type);
	switch (nla->nla_type) {
	case OPLUS_GAME_FIRST_MSG_SET_GAME_UID:
		oplus_game_first_handle_set_s_game_uid(nla);
		break;
	case OPLUS_GAME_FIRST_MSG_BATTLE_STATE:
		oplus_game_first_handle_battle_state(nla);
		break;
	case OPLUS_GAME_FIRST_MSG_GAME_EXIT:
		oplus_game_first_handle_game_exit(nla);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

#ifdef TRACE_FUNC_ON
static void oplus_game_first_udp_recvmsg(void *data, struct sock *sk)
{
	struct game_thread_info_st game_thread_info;

	if (s_test_mode) {
		if (s_game_uid && s_test_tgid && s_test_pid) {
			game_thread_info.uid = s_game_uid;
			game_thread_info.tgid = s_test_tgid;
			game_thread_info.pid = s_test_pid;
			LOGK("s_test_mode,uid=%u,tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
				s_game_uid, s_test_tgid, s_test_pid, s_last_tgid, s_last_tgid);
			oplus_game_first_send_netlink_msg(OPLUS_GAME_FIRST_MSG_REPORT_THREAD_INFO, (char*)&game_thread_info, sizeof(game_thread_info));
		}

		return;
	}

	if (!s_battle_flag || !s_game_alive || !s_rx_first) {
		return;
	}

	if (!sk || !sk_fullsock(sk)) {
		return;
	}

	if (sk->sk_uid.val != s_game_uid) {
		return;
	}

#ifdef CONFIG_ANDROID_VENDOR_OEM_DATA
	if (sk->android_oem_data1 == DPI_ID_TMGP_SGAME_STREAM_GAME_DATA) {
		struct task_struct *task = current;
		if(task == NULL) {
			return;
		}

		if (!s_last_tgid && !s_last_pid) {
			LOGK("udp_msg hook:task=%s,tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
				task->comm, task->tgid, task->pid, s_last_tgid, s_last_tgid);
			game_thread_info.uid = s_game_uid;
			game_thread_info.tgid = task->tgid;
			game_thread_info.pid = task->pid;
			s_last_tgid = task->tgid;
			s_last_pid = task->pid;
			oplus_game_first_send_netlink_msg(OPLUS_GAME_FIRST_MSG_REPORT_THREAD_INFO, (char*)&game_thread_info, sizeof(game_thread_info));
		} else if ((s_last_tgid != task->tgid) || (s_last_pid != task->pid)) {
			LOGK("warning:udp_msg hook tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
				task->tgid, task->pid, s_last_tgid, s_last_pid);
		}
	}
#endif

	return;
}
#endif

static int init_oplus_game_first_ctl()
{
	oplus_game_first_ctl_header = register_net_sysctl(&init_net, "net/oplus_game_first", oplus_game_first_sysctl_table);
	return oplus_game_first_ctl_header == NULL ? -ENOMEM : 0;
}

static void deinit_oplus_game_first_ctl()
{
	if(oplus_game_first_ctl_header) {
		unregister_net_sysctl_table(oplus_game_first_ctl_header);
		oplus_game_first_ctl_header = NULL;
	}
}

static int oplus_game_first_netlink_init(void)
{
	int ret;
	ret = genl_register_family(&oplus_game_first_genl_family);
	if (ret) {
		printk("[OPLUS_GAME_FIRST]:genl_register_family:%s failed,ret = %d\n", OPLUS_GAME_FIRST_FAMILY_NAME, ret);
		return ret;
	} else {
		printk("[OPLUS_GAME_FIRST]:genl_register_family complete, id = %d!\n", oplus_game_first_genl_family.id);
	}

	return 0;
}

static void oplus_game_first_netlink_exit(void)
{
	genl_unregister_family(&oplus_game_first_genl_family);
}

#ifdef TRACE_FUNC_ON
static void oplus_game_first_net_dev_queue(void *data, struct sk_buff *skb) {
	struct sock *sk = skb_to_full_sk(skb);

	if (!sk) {
		return;
	}

	#ifdef CONFIG_ANDROID_VENDOR_OEM_DATA
	if (s_tx_first && (sk->android_oem_data1 == DPI_ID_TMGP_SGAME_STREAM_GAME_DATA)) {
	#else
	if (s_tx_first && (!(s_game_uid)) && (sk->sk_uid.val == s_game_uid)) {
	#endif
		skb->priority = GAME_SKB_PRIORITY;
	}
}
#endif

static int register_hook_func(void) {
	int ret = 0;

#ifdef TRACE_FUNC_ON
	ret = register_trace_android_rvh_udp_recvmsg(oplus_game_first_udp_recvmsg, NULL);
	if (ret != 0) {
		LOGK("register_trace_android_rvh_udp_recvmsg fail, return");
		return ret;
	}

	ret = register_trace_net_dev_queue(oplus_game_first_net_dev_queue, NULL);
	if (ret) {
		LOGK("register_trace_net_dev_queue fail, return");
		return ret;
	}
#endif

	return ret;
}

static void unregister_hook_func(void) {
#ifdef TRACE_FUNC_ON
	unregister_trace_net_dev_queue(oplus_game_first_net_dev_queue, NULL);
#endif
}

static unsigned int oplus_tx_class_postrouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct sock *sk;

	sk = skb_to_full_sk(skb);
	if (!sk) {
		return NF_ACCEPT;
	}

	#ifdef CONFIG_ANDROID_VENDOR_OEM_DATA
	if (s_tx_first && (sk->android_oem_data1 == DPI_ID_TMGP_SGAME_STREAM_GAME_DATA)) {
	#else
	if (s_tx_first && (!s_game_uid) && (sk->sk_uid.val == s_game_uid)) {
	#endif
		skb->priority = GAME_SKB_PRIORITY;
	}

	return NF_ACCEPT;
}

static unsigned int oplus_rx_thread_info_output_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
	struct sock *sk;
	struct game_thread_info_st game_thread_info;

	sk = skb_to_full_sk(skb);
	if (!sk) {
		return NF_ACCEPT;
	}

	if (s_test_mode) {
		if (s_game_uid && s_test_tgid && s_test_pid) {
			game_thread_info.uid = s_game_uid;
			game_thread_info.tgid = s_test_tgid;
			game_thread_info.pid = s_test_pid;
			LOGK("s_test_mode,uid=%u,tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
				s_game_uid, s_test_tgid, s_test_pid, s_last_tgid, s_last_tgid);
			oplus_game_first_send_netlink_msg(OPLUS_GAME_FIRST_MSG_REPORT_THREAD_INFO, (char*)&game_thread_info, sizeof(game_thread_info));
		}

		return NF_ACCEPT;
	}

	if (!s_battle_flag || !s_game_alive) {
		return NF_ACCEPT;
	}

	if (s_rx_first && in_task()) {
		#ifdef CONFIG_ANDROID_VENDOR_OEM_DATA
		if (sk->android_oem_data1 == DPI_ID_TMGP_SGAME_STREAM_GAME_DATA) {
			struct task_struct *task = current;
			if(task == NULL) {
				return NF_ACCEPT;
			}

			if (strnstr(task->comm, GAME_THREAD_NAME, 5) == 0) {
				LOGK("task name not match, return");
				return NF_ACCEPT;
			}

			if (!s_last_tgid && !s_last_pid) {
				LOGK("output hook:task=%s,tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
					task->comm, task->tgid, task->pid, s_last_tgid, s_last_tgid);

				game_thread_info.uid = s_game_uid;
				game_thread_info.tgid = task->tgid;
				game_thread_info.pid = task->pid;
				s_last_tgid = task->tgid;
				s_last_pid = task->pid;
				oplus_game_first_send_netlink_msg(OPLUS_GAME_FIRST_MSG_REPORT_THREAD_INFO, (char*)&game_thread_info, sizeof(game_thread_info));
			} else if ((s_last_tgid != task->tgid) || (s_last_pid != task->pid)) {
				LOGK("warning:output hook:task=%s,tgid=%u,pid=%u,s_last_tgid=%d,s_last_pid=%d",
					task->comm, task->tgid, task->pid, s_last_tgid, s_last_pid);
				s_last_tgid = task->tgid;
				s_last_pid = task->pid;
			}
		}
		#endif
	}

	return NF_ACCEPT;
}

static struct nf_hook_ops oplus_tx_class_netfilter_ops[] __read_mostly =
{
	{
		.hook		= oplus_rx_thread_info_output_hook,
		.pf			= NFPROTO_IPV4,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FIRST ,
	},
	{
		.hook		= oplus_rx_thread_info_output_hook,
		.pf			= NFPROTO_IPV6,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FIRST ,
	},
	{
		.hook		= oplus_tx_class_postrouting_hook,
		.pf			= NFPROTO_IPV4,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FILTER + 1,
	},
	{
		.hook		= oplus_tx_class_postrouting_hook,
		.pf			= NFPROTO_IPV6,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FILTER + 1,
	},
};

static int __init oplus_game_first_init(void)
{
	int ret = 0;

	ret = register_hook_func();
	if (ret != 0) {
		printk("[GAME_FIRST]:register_trace_android_rvh_udp_recvmsg fail, return");
		return ret;
	}

	ret = nf_register_net_hooks(&init_net, oplus_tx_class_netfilter_ops, ARRAY_SIZE(oplus_tx_class_netfilter_ops));
	if (ret < 0) {
		return ret;
	}

	ret = init_oplus_game_first_ctl();
	if (ret != 0) {
		printk("[GAME_FIRST]:init_oplus_game_first_ctl fail,return");
		nf_unregister_net_hooks(&init_net, oplus_tx_class_netfilter_ops, ARRAY_SIZE(oplus_tx_class_netfilter_ops));
		return ret;
	}

	ret = oplus_game_first_netlink_init();
	if (ret != 0) {
		nf_unregister_net_hooks(&init_net, oplus_tx_class_netfilter_ops, ARRAY_SIZE(oplus_tx_class_netfilter_ops));
		deinit_oplus_game_first_ctl();
		return ret;
	}

	printk("[GAME_FIRST]:oplus_game_first_init, success");
	return 0;
}

static void __exit oplus_game_first_exit(void)
{
	deinit_oplus_game_first_ctl();
	unregister_hook_func();
	oplus_game_first_netlink_exit();
	printk("[GAME_FIRST]:oplus_game_first_exit");
	return;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
module_init(oplus_game_first_init);
module_exit(oplus_game_first_exit);
MODULE_LICENSE("GPL v2");
#endif
