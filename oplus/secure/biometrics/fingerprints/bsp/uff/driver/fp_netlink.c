/*******************************************************************************************
* Copyright (c) 2023 - 2029 OPLUS Mobile Comm Corp., Ltd.
*
* File: fp_netlink.c
* Description: add netlink module to uff-fp-driver
* Version: 1.1
* Date : 2023-8-24
* Author:  tengyuhang
** -----------------------------Revision History: -----------------------
**  <author>      <date>            <desc>
**  Yuhang.teng   2023/08/24        create the file
**  Yuhang.teng   2023/10/20        amend the file
*********************************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>
#include "fp_driver.h"
#define MAX_KERNEL_TO_USER_MSGSIZE 272 // fingerprint_message_t max size is 256+16
#define MAX_USER_TO_KERNEL_MSGSIZE 100 // only recv len = 3(it can define any size in HAL),only to make sure the msgsend is ok.

static int g_pid = -1;
static struct sock *nl_sk = NULL;

static void write_fingerprint_msg(struct fingerprint_message_t* g_fingerprint_msg, int module, int event,
                                  void *data, unsigned int size)
{
    switch (module) {
        case E_FP_TP:
            g_fingerprint_msg->module = E_FP_TP;
            g_fingerprint_msg->event = event == 1 ? E_FP_EVENT_TP_TOUCHDOWN : E_FP_EVENT_TP_TOUCHUP;
            g_fingerprint_msg->out_size = size <= MAX_MESSAGE_SIZE ? size : MAX_MESSAGE_SIZE;
            memcpy(g_fingerprint_msg->out_buf, data, g_fingerprint_msg->out_size);
            break;
        case E_FP_LCD:
            g_fingerprint_msg->module = E_FP_LCD;
            g_fingerprint_msg->event = event == 1 ? E_FP_EVENT_UI_READY : E_FP_EVENT_UI_DISAPPEAR;
            // pr_info("kernel module:%d event:%d - %d", g_fingerprint_msg->module, event, g_fingerprint_msg->event);
            break;
        case E_FP_HAL:
            g_fingerprint_msg->module = E_FP_HAL;
            g_fingerprint_msg->event = E_FP_EVENT_STOP_INTERRUPT;
            break;
        default:
            g_fingerprint_msg->module = module;
            g_fingerprint_msg->event = event;
            pr_info("%s, unknow module, ignored\n", __func__);
            break;
        }
}

static void fp_nl_data_ready(struct sk_buff *__skb)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    char str[MAX_USER_TO_KERNEL_MSGSIZE];
    skb = skb_get(__skb); // why set input param to local param?
    if (skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(skb);
        if (nlh != NULL) {
            memcpy(str, NLMSG_DATA(nlh), sizeof(str));
            g_pid = nlh->nlmsg_pid;
        }
        kfree_skb(skb);
    }
}

void fp_sendnlmsg(int module, int event, void *data, unsigned int size)
{
    struct sk_buff *skb_1;
    struct nlmsghdr *nlh;
    struct fingerprint_message_t g_fingerprint_msg;
    int len = NLMSG_SPACE(MAX_KERNEL_TO_USER_MSGSIZE);
    int ret = 0;

    memset(&g_fingerprint_msg, 0, sizeof(g_fingerprint_msg));
    write_fingerprint_msg(&g_fingerprint_msg, module, event, data, size);
    if (!nl_sk || !g_pid) {
        pr_err("%s, nl_sk is null or g_pid:%d\n", __func__, g_pid);
        return;
    }
    if (get_fp_driver_evt_type() != FP_DRIVER_NETLINK) {
        pr_err("%s, NETLINK is not enable\n", __func__);
        return;
    }
    skb_1 = alloc_skb(len, GFP_KERNEL);
    if (!skb_1) {
        pr_err("%s, alloc_skb error\n", __func__);
        return;
    }

    nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_KERNEL_TO_USER_MSGSIZE, 0);

    NETLINK_CB(skb_1).portid = 0;
    NETLINK_CB(skb_1).dst_group = 0;

    if (nlh != NULL) {
        pr_debug("%s, event_change:%d - %d, out_size:%d\n", __func__, event, g_fingerprint_msg.event, g_fingerprint_msg.out_size);
        pr_info("%s, module:%d, event:%d\n", __func__, g_fingerprint_msg.module, g_fingerprint_msg.event);
        memcpy(NLMSG_DATA(nlh), &g_fingerprint_msg, sizeof(struct fingerprint_message_t));
    }

    ret = netlink_unicast(nl_sk, skb_1, g_pid, MSG_DONTWAIT);
    if (!ret) {
        pr_err("%s, send msg from kernel to userspace failed ret 0x%x\n", __func__, ret);
    }
}

int fp_netlink_init(void)
{
    struct netlink_kernel_cfg netlink_cfg;

    memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));

    netlink_cfg.groups = 0;
    netlink_cfg.flags = 0;
    netlink_cfg.input = fp_nl_data_ready;
    netlink_cfg.cb_mutex = NULL;

    nl_sk = netlink_kernel_create(&init_net, NETLINKROUTE,
            &netlink_cfg);

    if (!nl_sk) {
        pr_err("%s, create netlink socket error\n", __func__);
        return 1;
    }

    return 0;
}

void fp_netlink_exit(void)
{
    if (nl_sk != NULL) {
        netlink_kernel_release(nl_sk);
        nl_sk = NULL;
    }

    pr_info("%s, netlink exited\n", __func__);
}

