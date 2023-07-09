/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/time.h>
#include <net/genetlink.h>

/* Netlink(reuse qrtr hook netlink) */
enum
{
	QRTR_HOOK_NETLINK_CMD_UNSPEC,
	QRTR_HOOK_NETLINK_CMD_DOWNLINK,
	QRTR_HOOK_NETLINK_CMD_UPLINK,
	__QRTR_HOOK_NETLINK_CMD_MAX
};

enum
{
	/* req/cnf ids */
	NETLINK_REQUEST_QRTR_WAKEUP = 1,
	NETLINK_RESPONSE_QRTR_WAKEUP,

	NETLINK_REQUEST_CCCI_WAKEUP,
	NETLINK_RESPONSE_CCCI_WAKEUP,

	__QRTR_HOOK_NETLINK_MSG_MAX = 10,
};

#define QRTR_HOOK_NETLINK_MSG_MAX (__QRTR_HOOK_NETLINK_MSG_MAX - 1)
#define QRTR_HOOK_NETLINK_CMD_MAX (__QRTR_HOOK_NETLINK_CMD_MAX - 1)
#define QRTR_HOOK_FAMILY_NAME "qrtr_hook" /* ccci_wakeup_hook */
#define QRTR_HOOK_FAMILY_VERSION 1
#define NLA_DATA(nla) ((char *)((char *)(nla) + NLA_HDRLEN))

static int qrtr_hook_netlink_pid = 0;

static int qrtr_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info);
static const struct genl_ops qrtr_hook_genl_ops[] = {
	{
		.cmd = QRTR_HOOK_NETLINK_CMD_DOWNLINK,
		.flags = 0,
		.doit = qrtr_hook_netlink_nlmsg_handle,
		.dumpit = NULL,
	},
};

static struct genl_family qrtr_hook_genl_family = {
	.id = 0,
	.hdrsize = 0,
	.name = QRTR_HOOK_FAMILY_NAME,
	.version = QRTR_HOOK_FAMILY_VERSION,
	.maxattr = QRTR_HOOK_NETLINK_MSG_MAX,
	.ops = qrtr_hook_genl_ops,
	.n_ops = ARRAY_SIZE(qrtr_hook_genl_ops),
};

/* Kprobe */
#define CCCI_CCIF_RX_COLLECT_NAME "ccif_rx_collect"
#define CCCI_CCIF_RESUME_NOIRQ_NAME "ccif_resume_noirq"
#define CCCI_CCIF_RX_COLLECT_OFFSET 0x224
#define CCCI_CCIF_RESUME_NOIRQ_OFFSET 0x25C
#define CCCI_CHANNEL_ID_REGS_INDEX 24
#define CCCI_CHANNEL_ID_INVALID_CHECK_MASK 0xFFFF0000
#define CCCI_CCB_CHANNEL_ID 999

static int handler_ccif_rx_collect(struct kprobe *kp, struct pt_regs *regs);
static struct kprobe kp_ccif_rx_collect = {
	.symbol_name = CCCI_CCIF_RX_COLLECT_NAME,
	.pre_handler = handler_ccif_rx_collect,
	.offset = CCCI_CCIF_RX_COLLECT_OFFSET,
};

static int handler_ccif_resume_noirq(struct kprobe *kp, struct pt_regs *regs);
static struct kprobe kp_ccif_resume_noirq = {
	.symbol_name = CCCI_CCIF_RESUME_NOIRQ_NAME,
	.pre_handler = handler_ccif_resume_noirq,
	.offset = CCCI_CCIF_RESUME_NOIRQ_OFFSET,
};

/* Statistics(reuse qrtr hook statistics struct) */
/* Make sure CCCI_WAKEUP_ARRAY_LEN == QRTR_WAKEUP_ARRAY_LEN */
#define CCCI_WAKEUP_ARRAY_LEN 30
#define CCCI_WAKEUP_IGNORE_TIME_MS 1000
/* Make sure sizeof(ccci_wakeup_st) == sizeof(qrtr_wakeup_st) */
struct ccci_wakeup_st
{
	uint32_t channel_id;
	uint32_t count;
};
static struct ccci_wakeup_st ccci_wakeup_array[CCCI_WAKEUP_ARRAY_LEN];
static uint64_t ccci_wakeup_stamp = 0;

static uint64_t my_current_kernel_time_ms(void)
{
	struct timespec64 ts64;

	ktime_get_real_ts64(&ts64);
	return ts64.tv_sec * 1000 + ts64.tv_nsec / 1000000;
}

/**
 * @brief      The handler of kprobe hook.
 *
 * @param      kp    The kprobe
 * @param      regs  The regs
 *
 * @return     0
 */
static int handler_ccif_rx_collect(struct kprobe *kp, struct pt_regs *regs)
{
	int i = 0;
	bool array_overflow = true;
	uint64_t now = 0;
	uint64_t channel_id = 0;

	now = my_current_kernel_time_ms();
	channel_id = regs->regs[CCCI_CHANNEL_ID_REGS_INDEX];

	if ((now - ccci_wakeup_stamp) < CCCI_WAKEUP_IGNORE_TIME_MS) {
		printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu, ignore wakeup!", channel_id, now);
		return 0;
	}
	ccci_wakeup_stamp = now;

	if ((channel_id & CCCI_CHANNEL_ID_INVALID_CHECK_MASK) == 0) {
		for (i = 0; i < CCCI_WAKEUP_ARRAY_LEN; i++) {
			if (ccci_wakeup_array[i].channel_id == channel_id) {
				++ccci_wakeup_array[i].count;
				array_overflow = false;
				break;
			}
			if (ccci_wakeup_array[i].channel_id == 0) {
				ccci_wakeup_array[i].channel_id = channel_id;
				ccci_wakeup_array[i].count = 1;
				array_overflow = false;
				break;
			}
		}
		if (array_overflow) {
			printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu, array overflow!", channel_id, now);
		} else {
			printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu", channel_id, now);
		}
	} else {
		printk("[ccci_wakeup_hook] failed to hook ccci wakeup, invalid format [%lu]",
		       regs->regs[CCCI_CHANNEL_ID_REGS_INDEX]);
	}

	return 0;
}

/**
 * @brief      The handler of kprobe hook.
 *
 * @param      kp    The kprobe
 * @param      regs  The regs
 *
 * @return     0
 */
static int handler_ccif_resume_noirq(struct kprobe *kp, struct pt_regs *regs)
{
	int i = 0;
	bool array_overflow = true;
	uint64_t now = 0;
	uint64_t channel_id = CCCI_CCB_CHANNEL_ID;

	now = my_current_kernel_time_ms();

	if ((now - ccci_wakeup_stamp) < CCCI_WAKEUP_IGNORE_TIME_MS) {
		printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu, ignore wakeup!", channel_id, now);
		return 0;
	}
	ccci_wakeup_stamp = now;

	for (i = 0; i < CCCI_WAKEUP_ARRAY_LEN; i++) {
		if (ccci_wakeup_array[i].channel_id == channel_id) {
			++ccci_wakeup_array[i].count;
			array_overflow = false;
			break;
		}
		if (ccci_wakeup_array[i].channel_id == 0) {
			ccci_wakeup_array[i].channel_id = channel_id;
			ccci_wakeup_array[i].count = 1;
			array_overflow = false;
			break;
		}
	}
	if (array_overflow) {
		printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu, array overflow!", channel_id, now);
	} else {
		printk("[ccci_wakeup_hook] channel_id=%lu stamp=%lu", channel_id, now);
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
	qrtr_hook_netlink_pid = nlhdr->nlmsg_pid;

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
	genlmsg_put(skb, pid, 0, &qrtr_hook_genl_family, 0, cmd);
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

	if (!qrtr_hook_netlink_pid) {
		printk("[ccci_wakeup_hook] netlink_send_to_user failed, qrtr_hook_netlink_pid=0.\n");
		return -EINVAL;
	}

	size = nla_total_size(data_len);
	ret = genl_msg_prepare_usr_msg(QRTR_HOOK_NETLINK_CMD_UPLINK, size, qrtr_hook_netlink_pid, &skbuff);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] netlink_send_to_user failed, prepare_usr_msg ret=%d.\n", ret);
		return ret;
	}

	ret = genl_msg_mk_usr_msg(skbuff, msg_type, data, data_len);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] netlink_send_to_user failed, mk_usr_msg ret=%d.\n", ret);
		kfree_skb(skbuff);
		return ret;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skbuff)));
	genlmsg_end(skbuff, head);

	ret = genlmsg_unicast(&init_net, skbuff, qrtr_hook_netlink_pid);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] netlink_send_to_user failed, genlmsg_unicast ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief      The handler of request ccci wakeup count from user space.
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_ccci_wakeup()
{
	int ret = 0;
	char msg_buf[sizeof(struct ccci_wakeup_st) * CCCI_WAKEUP_ARRAY_LEN] = { 0 };

	memcpy(msg_buf, ccci_wakeup_array, sizeof(struct ccci_wakeup_st) * CCCI_WAKEUP_ARRAY_LEN);
	memset(ccci_wakeup_array, 0x0, sizeof(struct ccci_wakeup_st) * CCCI_WAKEUP_ARRAY_LEN);

	ret = netlink_send_to_user(NETLINK_RESPONSE_CCCI_WAKEUP, (char *)msg_buf, sizeof(msg_buf));
	if (ret < 0) {
		printk("[ccci_wakeup_hook] request_ccci_wakeup failed, netlink_send_to_user ret=%d.\n", ret);
	}

	printk("[ccci_wakeup_hook] request and reset ccci wakeup!");
	return ret;
}

/**
 * @brief      The handler of qrtr hook netlink message from user space.
 *
 * @param      skb   The socket buffer
 * @param      info  The information
 *
 * @return     0 if successful, negative otherwise.
 */
static int qrtr_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;

	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	switch (nla->nla_type) {
	case NETLINK_REQUEST_CCCI_WAKEUP:
		ret = set_android_pid(nlhdr);
		ret = request_ccci_wakeup();
		break;
	default:
		printk("[ccci_wakeup_hook] qrtr_hook_netlink_nlmsg_handle failed, unknown nla type=%d.\n", nla->nla_type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief      Initialize qrtr hook netlink program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int qrtr_hook_netlink_init(void)
{
	int ret = 0;

	ret = genl_register_family(&qrtr_hook_genl_family);
	if (ret) {
		printk("[ccci_wakeup_hook] genl_register_family failed, ret=%d.\n", ret);
		return ret;
	}
	else {
		printk("[ccci_wakeup_hook] genl_register_family complete, id=%d.\n", qrtr_hook_genl_family.id);
	}

	return 0;
}

/**
 * @brief      Uninstall qrtr hook netlink program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int qrtr_hook_netlink_exit(void)
{
	genl_unregister_family(&qrtr_hook_genl_family);

	return 0;
}

/**
 * @brief      Initialize ccci wakeup hook.
 *
 * @return     0 if successful, negative otherwise.
 */
static int __init ccci_wakeup_hook_init(void)
{
	int ret = 0;

	ret = qrtr_hook_netlink_init();
	if (ret < 0) {
		printk("[ccci_wakeup_hook] module failed to init netlink.\n");
		return ret;
	}

	ret = register_kprobe(&kp_ccif_rx_collect);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] register ccif_rx_collect kprobe failed with %d", ret);
	}

	ret = register_kprobe(&kp_ccif_resume_noirq);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] register ccif_resume_noirq kprobe failed with %d", ret);
	}

	memset(ccci_wakeup_array, 0x0, sizeof(struct ccci_wakeup_st) * CCCI_WAKEUP_ARRAY_LEN);

	printk("[ccci_wakeup_hook] module init successfully!");

	return 0;
}

/**
 * @brief      Uninstall ccci wakeup hook.
 */
static void __exit ccci_wakeup_hook_fini(void)
{
	unregister_kprobe(&kp_ccif_rx_collect);
	unregister_kprobe(&kp_ccif_resume_noirq);
	qrtr_hook_netlink_exit();
}

module_init(ccci_wakeup_hook_init);
module_exit(ccci_wakeup_hook_fini);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aska");
MODULE_VERSION("1:0.1");
MODULE_DESCRIPTION("OPLUS ccci wakeup hook");
