/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include "linkpower_netlink.h"

#define LINKPOWER_NETLINK_MSG_MAX (__LINKPOWER_NETLINK_MSG_MAX - 1)
#define LINKPOWER_NETLINK_CMD_MAX (__LINKPOWER_NETLINK_CMD_MAX - 1)
#define LINKPOWER_NETLINK_FAMILY_NAME "linkpower"
#define LINKPOWER_NETLINK_FAMILY_VERSION 1

extern int sk_pid_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
#if defined MTK_PLATFORM
extern int hba_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
extern int ccci_wakeup_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
#elif defined QCOM_PLATFORM
/*int hba_qcom_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);*/
extern int qrtr_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
#endif

static int linkpower_netlink_pid = 0;

static int linkpower_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
static const struct genl_ops linkpower_netlink_genl_ops[] = {
	{
		.cmd = LINKPOWER_NETLINK_CMD_DOWNLINK,
		.flags = 0,
		.doit = linkpower_netlink_nlmsg_handle,
		.dumpit = NULL,
	},
};

static struct genl_family linkpower_netlink_genl_family = {
	.id = 0,
	.hdrsize = 0,
	.name = LINKPOWER_NETLINK_FAMILY_NAME,
	.version = LINKPOWER_NETLINK_FAMILY_VERSION,
	.maxattr = LINKPOWER_NETLINK_MSG_MAX,
	.ops = linkpower_netlink_genl_ops,
	.n_ops = ARRAY_SIZE(linkpower_netlink_genl_ops),
};

/**
 * @brief      Sets the netlink pid.
 *
 * @param      nlhdr  The nlhdr
 *
 * @return     0 if successful, negative otherwise.
 */
static int set_android_pid(struct nlmsghdr *nlhdr)
{
	linkpower_netlink_pid = nlhdr->nlmsg_pid;

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
	genlmsg_put(skb, pid, 0, &linkpower_netlink_genl_family, 0, cmd);
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
int netlink_send_to_user(int msg_type, char *data, int data_len)
{
	int ret = 0;
	void *head;
	struct sk_buff *skbuff;
	size_t size;

	if (!linkpower_netlink_pid) {
		printk("[linkpower_netlink] netlink_send_to_user failed, linkpower_netlink_pid=0.\n");
		return -EINVAL;
	}

	size = nla_total_size(data_len);
	ret = genl_msg_prepare_usr_msg(LINKPOWER_NETLINK_CMD_UPLINK, size, linkpower_netlink_pid, &skbuff);
	if (ret < 0) {
		printk("[linkpower_netlink] netlink_send_to_user failed, prepare_usr_msg ret=%d.\n", ret);
		return ret;
	}

	ret = genl_msg_mk_usr_msg(skbuff, msg_type, data, data_len);
	if (ret < 0) {
		printk("[linkpower_netlink] netlink_send_to_user failed, mk_usr_msg ret=%d.\n", ret);
		kfree_skb(skbuff);
		return ret;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skbuff)));
	genlmsg_end(skbuff, head);

	ret = genlmsg_unicast(&init_net, skbuff, linkpower_netlink_pid);
	if (ret < 0) {
		printk("[linkpower_netlink] netlink_send_to_user failed, genlmsg_unicast ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief      The handler of linkpower netlink message from user space.
 *
 * @param      skb   The socket buffer
 * @param      info  The information
 *
 * @return     0 if successful, negative otherwise.
 */
static int linkpower_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;
	int nla_type = -1;
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);
	ret = set_android_pid(nlhdr);
	nla_type = nla->nla_type;

	if ((nla_type > NETLINK_ID_SK_PID_HOOK_BEGIN) && (nla_type < NETLINK_ID_SK_PID_HOOK_END)) {
		return sk_pid_hook_netlink_nlmsg_handle(skb, info);
#if defined MTK_PLATFORM
	} else if ((nla_type > NETLINK_ID_HBA_PROXY_MTK_BEGIN) && (nla_type < NETLINK_ID_HBA_PROXY_MTK_END)) {
		return hba_netlink_nlmsg_handle(skb, info);
	} else if ((nla_type > NETLINK_ID_CCCI_WAKEUP_HOOK_BEGIN) && (nla_type < NETLINK_ID_CCCI_WAKEUP_HOOK_END)) {
		return ccci_wakeup_hook_netlink_nlmsg_handle(skb, info);
#elif defined QCOM_PLATFORM
		/*    } else if ((nla_type > NETLINK_ID_HBA_PROXY_QCOM_BEGIN) && (nla_type < NETLINK_ID_HBA_PROXY_QCOM_END)) {
		        return hba_qcom_netlink_nlmsg_handle(skb, info);*/
	} else if ((nla_type > NETLINK_ID_QRTR_HOOK_BEGIN) && (nla_type < NETLINK_ID_QRTR_HOOK_END)) {
		return qrtr_hook_netlink_nlmsg_handle(skb, info);
#endif
	} else {
		printk("[linkpower_netlink] linkpower_netlink_nlmsg_handle failed, unknown nla type=%d.\n", nla_type);
		ret = -EINVAL;
	}
	return ret;
}

/**
 * @brief      Initialize linkpower netlink.
 *
 * @return     0 if successful, negative otherwise.
 */
int linkpower_netlink_init(void)
{
	int ret = 0;

	ret = genl_register_family(&linkpower_netlink_genl_family);
	if (ret < 0) {
		printk("[linkpower_netlink] module failed to init netlink.\n");
		return ret;
	} else {
		printk("[linkpower_netlink] genl_register_family complete, id=%d.\n", linkpower_netlink_genl_family.id);
	}

	printk("[linkpower_netlink] module init successfully!");
	return 0;
}

/**
 * @brief      Uninstall sk pid hook.
 */
void linkpower_netlink_fini(void)
{
	genl_unregister_family(&linkpower_netlink_genl_family);
}
