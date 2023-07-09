/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <net/inet_sock.h>
#include <net/genetlink.h>

/* Netlink */
enum
{
	SK_PID_HOOK_NETLINK_CMD_UNSPEC,
	SK_PID_HOOK_NETLINK_CMD_DOWNLINK,
	SK_PID_HOOK_NETLINK_CMD_UPLINK,
	__SK_PID_HOOK_NETLINK_CMD_MAX
};

enum
{
	/* req/cnf ids */
	NETLINK_REQUEST_SK_PORT_AND_PID = 1,
	NETLINK_RESPONSE_SK_PORT_AND_PID,

	__SK_PID_HOOK_NETLINK_MSG_MAX = 10,
};

#define SK_PID_HOOK_NETLINK_MSG_MAX (__SK_PID_HOOK_NETLINK_MSG_MAX - 1)
#define SK_PID_HOOK_NETLINK_CMD_MAX (__SK_PID_HOOK_NETLINK_CMD_MAX - 1)
#define SK_PID_HOOK_FAMILY_NAME "sk_pid_hook"
#define SK_PID_HOOK_FAMILY_VERSION 1
#define NLA_DATA(nla) ((char *)((char *)(nla) + NLA_HDRLEN))

static int sk_pid_hook_netlink_pid = 0;

static int sk_pid_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
static const struct genl_ops sk_pid_hook_genl_ops[] = {
	{
		.cmd = SK_PID_HOOK_NETLINK_CMD_DOWNLINK,
		.flags = 0,
		.doit = sk_pid_hook_netlink_nlmsg_handle,
		.dumpit = NULL,
	},
};

static struct genl_family sk_pid_hook_genl_family = {
	.id = 0,
	.hdrsize = 0,
	.name = SK_PID_HOOK_FAMILY_NAME,
	.version = SK_PID_HOOK_FAMILY_VERSION,
	.maxattr = SK_PID_HOOK_NETLINK_MSG_MAX,
	.ops = sk_pid_hook_genl_ops,
	.n_ops = ARRAY_SIZE(sk_pid_hook_genl_ops),
};

/* Kprobe */
#define TCP_CONNECT_NAME "tcp_connect"
#define SYSTEM_UID 1000

static int handler_tcp_connect(struct kprobe *kp, struct pt_regs *regs);

static struct kprobe kp_tcp_connect = {
	.symbol_name = TCP_CONNECT_NAME,
	.pre_handler = handler_tcp_connect,
};

/* Statistics */
#define SK_PID_ARRAY_LEN 50
struct sk_pid_st
{
	uint16_t sport;
	uint16_t pid;
};
static struct sk_pid_st sk_pid_array[SK_PID_ARRAY_LEN];

/**
 * @brief      The handler of kprobe hook.
 *
 * @param      kp    The kprobe
 * @param      regs  The regs
 *
 * @return     0
 */
static int handler_tcp_connect(struct kprobe *kp, struct pt_regs *regs)
{
	int i = 0;
	bool array_overflow = true;
	int uid = 0, sk_pid = 0;
	int inet_sport = 0;
	struct sock *sk = NULL;
	struct inet_sock *inet = NULL;

	uid = current_uid().val;
	if (uid != SYSTEM_UID) {
		return 0;
	}

	sk_pid = current->tgid;
	sk = (struct sock *) regs->regs[0];
	inet = inet_sk(sk);
	inet_sport = ntohs(inet->inet_sport);

	if ((inet_sport == 0) || (sk_pid == 0)) {
		return 0;
	}
	for (i = 0; i < SK_PID_ARRAY_LEN; i++) {
		if ((sk_pid_array[i].sport == 0) && (sk_pid_array[i].pid == 0)) {
			sk_pid_array[i].sport = (uint16_t) inet_sport;
			sk_pid_array[i].pid = (uint16_t) sk_pid;
			array_overflow = false;
			break;
		}
	}
	if (array_overflow) {
		printk("[sk_pid_hook] sk_pid=%d, sport=%d, array overflow!", sk_pid, inet_sport);
	} else {
		printk("[sk_pid_hook] sk_pid=%d, sport=%d", sk_pid, inet_sport);
	}

	return 0;
}

/**
 * @brief      Sets the netlink pid.
 *
 * @param      nlhdr  The nlhdr
 *
 * @return     0 if successful, negative otherwise.
 */
static int set_android_pid(struct nlmsghdr *nlhdr)
{
	sk_pid_hook_netlink_pid = nlhdr->nlmsg_pid;

	return 0;
}

/**
 * @brief      Prepare netlink packets to be delivered to user space.
 *
 * @param[in]  cmd   The command
 * @param[in]  size  The size
 * @param[in]  pid   The pid
 * @param      skbp  The skbp
 *
 * @return     0 if successful, negative otherwise.
 */
static int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp)
{
	struct sk_buff *skb = NULL;
	/* Create a new netlink msg */
	skb = genlmsg_new(size, GFP_ATOMIC);
	if (skb == NULL) {
		return -ENOMEM;
	}
	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &sk_pid_hook_genl_family, 0, cmd);
	*skbp = skb;

	return 0;
}

/**
 * @brief      Make netlink packets to be delivered to user space.
 *
 * @param      skb   The socket buffer
 * @param[in]  type  The type
 * @param      data  The data
 * @param[in]  len   The length
 *
 * @return     0 if successful, negative otherwise.
 */
static int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
	int ret = 0;
	/* add a netlink attribute to a socket buffer */
	if ((ret = nla_put(skb, type, len, data)) != 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief      Send netlink packet to user space.
 *
 * @param[in]  msg_type  The message type
 * @param      data      The data
 * @param[in]  data_len  The data length
 *
 * @return     0 if successful, negative otherwise.
 */
static int netlink_send_to_user(int msg_type, char *data, int data_len)
{
	int ret = 0;
	void *head;
	struct sk_buff *skbuff;
	size_t size;

	if (!sk_pid_hook_netlink_pid) {
		printk("[sk_pid_hook] netlink_send_to_user failed, sk_pid_hook_netlink_pid=0.\n");
		return -EINVAL;
	}

	size = nla_total_size(data_len);
	ret = genl_msg_prepare_usr_msg(SK_PID_HOOK_NETLINK_CMD_UPLINK, size, sk_pid_hook_netlink_pid, &skbuff);
	if (ret < 0) {
		printk("[sk_pid_hook] netlink_send_to_user failed, prepare_usr_msg ret=%d.\n", ret);
		return ret;
	}

	ret = genl_msg_mk_usr_msg(skbuff, msg_type, data, data_len);
	if (ret < 0) {
		printk("[sk_pid_hook] netlink_send_to_user failed, mk_usr_msg ret=%d.\n", ret);
		kfree_skb(skbuff);
		return ret;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skbuff)));
	genlmsg_end(skbuff, head);

	ret = genlmsg_unicast(&init_net, skbuff, sk_pid_hook_netlink_pid);
	if (ret < 0) {
		printk("[sk_pid_hook] netlink_send_to_user failed, genlmsg_unicast ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief      The handler of request sport and pid info from user space.
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_sk_port_and_pid()
{
	int ret = 0;
	char msg_buf[sizeof(struct sk_pid_st) * SK_PID_ARRAY_LEN] = { 0 };

	memcpy(msg_buf, sk_pid_array, sizeof(struct sk_pid_st) * SK_PID_ARRAY_LEN);
	memset(sk_pid_array, 0x0, sizeof(struct sk_pid_st) * SK_PID_ARRAY_LEN);

	ret = netlink_send_to_user(NETLINK_RESPONSE_SK_PORT_AND_PID, (char *)msg_buf, sizeof(msg_buf));
	if (ret < 0) {
		printk("[sk_pid_hook] request_sk_port_and_pid failed, netlink_send_to_user ret=%d.\n", ret);
	}

	printk("[sk_pid_hook] request and reset sk port and pid!");
	return ret;
}

/**
 * @brief      The handler of sk pid hook netlink message from user space.
 *
 * @param      skb   The socket buffer
 * @param      info  The information
 *
 * @return     0 if successful, negative otherwise.
 */
static int sk_pid_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;

	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	switch (nla->nla_type) {
	case NETLINK_REQUEST_SK_PORT_AND_PID:
		ret = set_android_pid(nlhdr);
		ret = request_sk_port_and_pid();
		break;
	default:
		printk("[sk_pid_hook] sk_pid_hook_netlink_nlmsg_handle failed, unknown nla type=%d.\n", nla->nla_type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief      Initialize sk pid hook netlink program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int sk_pid_hook_netlink_init(void)
{
	int ret = 0;

	ret = genl_register_family(&sk_pid_hook_genl_family);
	if (ret) {
		printk("[sk_pid_hook] genl_register_family failed, ret=%d.\n", ret);
		return ret;
	}
	else {
		printk("[sk_pid_hook] genl_register_family complete, id=%d.\n", sk_pid_hook_genl_family.id);
	}

	return 0;
}

/**
 * @brief      Uninstall sk_pid hook netlink program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int sk_pid_hook_netlink_exit(void)
{
	genl_unregister_family(&sk_pid_hook_genl_family);

	return 0;
}

/**
 * @brief      Initialize sk pid hook.
 *
 * @return     0 if successful, negative otherwise.
 */
static int __init sk_pid_hook_init(void)
{
	int ret = 0;

	ret = register_kprobe(&kp_tcp_connect);
	if (ret < 0) {
		printk("[sk_pid_hook] register tcp connect kprobe failed with %d", ret);
		return ret;
	}

	ret = sk_pid_hook_netlink_init();
	if (ret < 0) {
		printk("[sk_pid_hook] module failed to init netlink.\n");
		unregister_kprobe(&kp_tcp_connect);
		return ret;
	}

	memset(sk_pid_array, 0x0, sizeof(struct sk_pid_st) * SK_PID_ARRAY_LEN);

	printk("[sk_pid_hook] module init successfully!");

	return 0;
}

/**
 * @brief      Uninstall sk pid hook.
 */
static void __exit sk_pid_hook_fini(void)
{
	unregister_kprobe(&kp_tcp_connect);
	sk_pid_hook_netlink_exit();
}

module_init(sk_pid_hook_init);
module_exit(sk_pid_hook_fini);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aska");
MODULE_VERSION("1:0.1");
MODULE_DESCRIPTION("OPLUS sock sport and pid hook");
