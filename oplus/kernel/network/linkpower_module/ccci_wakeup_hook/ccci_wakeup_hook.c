/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/kprobes.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/time.h>
#include "../linkpower_netlink/linkpower_netlink.h"

/* Netlink */
extern int netlink_send_to_user(int msg_type, char *data, int data_len);

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
int ccci_wakeup_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
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
		ret = request_ccci_wakeup();
		break;
	default:
		printk("[ccci_wakeup_hook] ccci_wakeup_hook_netlink_nlmsg_handle failed, unknown nla type=%d.\n", nla->nla_type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief      Initialize ccci wakeup hook.
 *
 * @return     0 if successful, negative otherwise.
 */
int ccci_wakeup_hook_init(void)
{
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)))
	int ret = 0;

	ret = register_kprobe(&kp_ccif_rx_collect);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] register ccif_rx_collect kprobe failed with %d", ret);
	}

	ret = register_kprobe(&kp_ccif_resume_noirq);
	if (ret < 0) {
		printk("[ccci_wakeup_hook] register ccif_resume_noirq kprobe failed with %d", ret);
	}
	printk("[ccci_wakeup_hook] register ccci kprobe successfully!");
#else
	printk("[ccci_wakeup_hook] not support ccci kprobe rx_collect=%u resume_noirq=%u",
	       kp_ccif_rx_collect.offset, kp_ccif_resume_noirq.offset);
#endif
	memset(ccci_wakeup_array, 0x0, sizeof(struct ccci_wakeup_st) * CCCI_WAKEUP_ARRAY_LEN);
	printk("[ccci_wakeup_hook] module init successfully!");

	return 0;
}

/**
 * @brief      Uninstall ccci wakeup hook.
 */
void ccci_wakeup_hook_fini(void)
{
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)))
	unregister_kprobe(&kp_ccif_rx_collect);
	unregister_kprobe(&kp_ccif_resume_noirq);
#endif
}
