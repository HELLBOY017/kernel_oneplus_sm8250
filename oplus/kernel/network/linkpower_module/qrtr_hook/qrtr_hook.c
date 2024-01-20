/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/kprobes.h>
#include <linux/module.h>
#include "../linkpower_netlink/linkpower_netlink.h"

/* Netlink */
extern int netlink_send_to_user(int msg_type, char *data, int data_len);

/* Kprobe */
#define QRTR_PRINT_WAKEUP_REASON_NAME "qrtr_print_wakeup_reason"
#define QRTR_PRINT_WAKEUP_REASON_OFFSET_V1 0x134
#define QRTR_PRINT_WAKEUP_REASON_OFFSET_V2 0x184
#define QRTR_PRINT_SERVICE_ID_REGS_INDEX 24
#define QRTR_PRINT_MSG_ID_REGS_INDEX 6
#define QRTR_MSG_ID_MASK 0xFF000000
#define QRTR_MSG_ID_SHIFT_COUNT 24
#define QRTR_SERVICE_ID_INVALID_CHECK_MASK 0xFFFF0000

static int handler_qrtr_print_wakeup_reason(struct kprobe *kp, struct pt_regs *regs);

static struct kprobe kp_qrtr_print_wakeup_reason_v1 = {
	.symbol_name = QRTR_PRINT_WAKEUP_REASON_NAME,
	.pre_handler = handler_qrtr_print_wakeup_reason,
	.offset = QRTR_PRINT_WAKEUP_REASON_OFFSET_V1,
};

static struct kprobe kp_qrtr_print_wakeup_reason_v2 = {
	.symbol_name = QRTR_PRINT_WAKEUP_REASON_NAME,
	.pre_handler = handler_qrtr_print_wakeup_reason,
	.offset = QRTR_PRINT_WAKEUP_REASON_OFFSET_V2,
};

/* Statistics */
#define QRTR_WAKEUP_ARRAY_LEN 30
struct qrtr_wakeup_st
{
	uint16_t service_id;
	uint16_t msg_id;
	uint32_t count;
};
static struct qrtr_wakeup_st qrtr_wakeup_array[QRTR_WAKEUP_ARRAY_LEN];

/**
 * @brief      The handler of kprobe hook.
 *
 * @param      kp    The kprobe
 * @param      regs  The regs
 *
 * @return     0
 */
static int handler_qrtr_print_wakeup_reason(struct kprobe *kp, struct pt_regs *regs)
{
	int i = 0;
	bool array_overflow = true;
	uint64_t service_id = 0;
	uint64_t msg_id = 0;

	service_id = regs->regs[QRTR_PRINT_SERVICE_ID_REGS_INDEX];
	msg_id = (regs->regs[QRTR_PRINT_MSG_ID_REGS_INDEX] & QRTR_MSG_ID_MASK) >> QRTR_MSG_ID_SHIFT_COUNT;

	if (((service_id & QRTR_SERVICE_ID_INVALID_CHECK_MASK) == 0) && (msg_id != 0)) {
		for (i = 0; i < QRTR_WAKEUP_ARRAY_LEN; i++) {
			if ((qrtr_wakeup_array[i].service_id == service_id) && (qrtr_wakeup_array[i].msg_id == msg_id)) {
				++qrtr_wakeup_array[i].count;
				array_overflow = false;
				break;
			}
			if ((qrtr_wakeup_array[i].service_id == 0) && (qrtr_wakeup_array[i].msg_id == 0)) {
				qrtr_wakeup_array[i].service_id = service_id;
				qrtr_wakeup_array[i].msg_id = msg_id;
				qrtr_wakeup_array[i].count = 1;
				array_overflow = false;
				break;
			}
		}
		if (array_overflow) {
			printk("[qrtr_hook] service_id=0x%x msg_id=0x%x, array overflow!", service_id, msg_id);
		} else {
			printk("[qrtr_hook] service_id=0x%x msg_id=0x%x", service_id, msg_id);
		}
	} else {
		printk("[qrtr_hook] failed to hook qrtr wakeup, invalid format [0x%x 0x%x]",
		       regs->regs[QRTR_PRINT_SERVICE_ID_REGS_INDEX], regs->regs[QRTR_PRINT_MSG_ID_REGS_INDEX]);
	}

	return 0;
}

/**
 * @brief      The handler of request qrtr wakeup count from user space.
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_qrtr_wakeup(void)
{
	int ret = 0;
	char msg_buf[sizeof(struct qrtr_wakeup_st) * QRTR_WAKEUP_ARRAY_LEN] = { 0 };

	memcpy(msg_buf, qrtr_wakeup_array, sizeof(struct qrtr_wakeup_st) * QRTR_WAKEUP_ARRAY_LEN);
	memset(qrtr_wakeup_array, 0x0, sizeof(struct qrtr_wakeup_st) * QRTR_WAKEUP_ARRAY_LEN);

	ret = netlink_send_to_user(NETLINK_RESPONSE_QRTR_WAKEUP, (char *)msg_buf, sizeof(msg_buf));
	if (ret < 0) {
		printk("[qrtr_hook] request_qrtr_wakeup failed, netlink_send_to_user ret=%d.\n", ret);
	}

	printk("[qrtr_hook] request and reset qrtr wakeup!");
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
int qrtr_hook_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;

	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	switch (nla->nla_type) {
	case NETLINK_REQUEST_QRTR_WAKEUP:
		ret = request_qrtr_wakeup();
		break;
	default:
		printk("[qrtr_hook] qrtr_hook_netlink_nlmsg_handle failed, unknown nla type=%d.\n", nla->nla_type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief      Initialize qrtr wakeup hook.
 *
 * @return     0 if successful, negative otherwise.
 */
int qrtr_hook_init(void)
{
	int ret = 0;

	ret = register_kprobe(&kp_qrtr_print_wakeup_reason_v1);
	if (ret < 0) {
		printk("[qrtr_hook] register qrtr kprobe v1 failed with %d", ret);
	}

	ret = register_kprobe(&kp_qrtr_print_wakeup_reason_v2);
	if (ret < 0) {
		printk("[qrtr_hook] register qrtr kprobe v2 failed with %d", ret);
	}

	memset(qrtr_wakeup_array, 0x0, sizeof(struct qrtr_wakeup_st) * QRTR_WAKEUP_ARRAY_LEN);

	printk("[qrtr_hook] module init successfully!");
	return 0;
}

/**
 * @brief      Uninstall qrtr wakeup hook.
 */
void qrtr_hook_fini(void)
{
	unregister_kprobe(&kp_qrtr_print_wakeup_reason_v1);
	unregister_kprobe(&kp_qrtr_print_wakeup_reason_v2);
}
