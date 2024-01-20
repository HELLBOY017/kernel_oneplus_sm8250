/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <net/inet_sock.h>
#include "../linkpower_netlink/linkpower_netlink.h"

/* Netlink */
extern int netlink_send_to_user(int msg_type, char *data, int data_len);

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
 * @brief      The handler of request sport and pid info from user space.
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_sk_port_and_pid(void)
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
int sk_pid_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
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
 * @brief      Initialize sk pid hook.
 *
 * @return     0 if successful, negative otherwise.
 */
int sk_pid_hook_init(void)
{
	int ret = 0;

	ret = register_kprobe(&kp_tcp_connect);
	if (ret < 0) {
		printk("[sk_pid_hook] register tcp connect kprobe failed with %d", ret);
		return ret;
	}

	memset(sk_pid_array, 0x0, sizeof(struct sk_pid_st) * SK_PID_ARRAY_LEN);

	printk("[sk_pid_hook] module init successfully!");
	return 0;
}

/**
 * @brief      Uninstall sk pid hook.
 */
void sk_pid_hook_fini(void)
{
	unregister_kprobe(&kp_tcp_connect);
}
