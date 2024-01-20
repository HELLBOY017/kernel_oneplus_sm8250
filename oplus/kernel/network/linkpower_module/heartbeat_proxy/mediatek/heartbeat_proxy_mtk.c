/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/netlink.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/tcp.h>
#include <net/genetlink.h>
#include <net/inet_hashtables.h>
#include <net/inet6_hashtables.h>
#include <mtk_ccci_common.h>
#include "mipc_msg.h"
#include "mipc_msg_tlv_api.h"
#include "mipc_msg_tlv_const.h"
#include "heartbeat_proxy_mtk.h"
#include "../../linkpower_netlink/linkpower_netlink.h"

/* Modem */
#define MAX_HBA_LIST_COUNT 5
#define CCCI_HBA_MIPC_NAME "ttyCMIPC4"
#define CCCI_RX_TASK_NAME "ccci_hba_rx"
#define CCCI_MAX_RECV_BUF 4096
#define MIPC_DEF_VAL 0

/* Netlink */
#define HBA_NETLINK_SKB_QUEUE_LEN 5
extern int netlink_send_to_user(int msg_type, char *data, int data_len);

static unsigned int hba_input_hook_v4(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
static struct nf_hook_ops hba_input_v4_ops[] __read_mostly = {
	{
		.hook = hba_input_hook_v4,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_MANGLE,
	},
};

static unsigned int hba_input_hook_v6(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
static struct nf_hook_ops hba_input_v6_ops[] __read_mostly = {
	{
		.hook = hba_input_hook_v6,
		.pf = NFPROTO_IPV6,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_MANGLE,
	},
};

typedef int (*ip_local_deliver_finish_t)(struct net *net, struct sock *sk, struct sk_buff *skb);

#define HBA_SKB_QUEUE_LEN 10
struct hba_st
{
	char proxy_key[128];
	uint32_t saddr;
	uint32_t daddr;
	uint32_t v6_saddr32[4];
	uint32_t v6_daddr32[4];
	uint16_t sport;
	uint16_t dport;
	struct sock *sk;
	bool is_ipv6;
	bool need_sync;
	hba_state_enum state;
	bool is_pause_timeout;
	struct timer_list pause_timer;
	uint16_t default_phone_id;
	bool is_queue_overflow;
	struct sk_buff *skb_queue[HBA_SKB_QUEUE_LEN];
	ip_local_deliver_finish_t ip_local_deliver_finish;
};
struct hba_list_st
{
	u32 count;
	struct list_head head;
};
struct hba_list_node_st
{
	struct hba_st hba;
	struct list_head list_node;
};

static void hba_netlink_nlmsg_handle_asyn(struct work_struct *work);
static DECLARE_WORK(hba_netlink_handle_work, hba_netlink_nlmsg_handle_asyn);

static struct sk_buff *hba_netlink_skb_queue[HBA_NETLINK_SKB_QUEUE_LEN];
static struct task_struct *ccci_rx_task;
static int hba_ccci_ipc_port_s = -1;
static bool hba_mipc_register_ind = false;
static struct hba_list_st hba_list;
static DEFINE_SPINLOCK(hba_list_lock);
static DEFINE_SPINLOCK(netlink_list_lock);

/**
 * @brief      Customized secure strncpy function.
 *
 * @param      dest  The destination
 * @param[in]  src   The source
 * @param[in]  n     Length of characters to copy.
 */
static void safe_strncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
	if (strlen(src) >= n) {
		dest[n - 1] = '\0';
	}
}

/**
 * @brief      Convert MIPC error codes to HBA error codes.
 *
 * @param[in]  mipc_result  The mipc result
 *
 * @return     The hba result enum.
 */
static hba_result_enum mipc_error_to_my_error(mipc_result_enum mipc_result)
{
	switch (mipc_result) {
	case MIPC_RESULT_SYS_EXT_HBA_PROCESS_OK:
		return HBA_INT_PROCESS_OK;
	case MIPC_RESULT_SYS_EXT_HBA_NUM_ALREADY_MAX:
		return HBA_INT_ERR_NUM_ALREADY_MAX;
	case MIPC_RESULT_SYS_EXT_HBA_PROXY_ALREADY_EXIST:
		return HBA_INT_ERR_PROXY_ALREADY_EXIST;
	case MIPC_RESULT_SYS_EXT_HBA_INTERNAL_ABNORMAL:
		return HBA_INT_ERR_MD_INTERNAL_ABNORMAL;
	case MIPC_RESULT_SYS_EXT_HBA_PROXY_NOT_EXIST:
		return HBA_INT_ERR_PROXY_NOT_EXIST;
	case MIPC_RESULT_SYS_EXT_HBA_HW_FILTER_EXHAUST:
		return HBA_INT_ERR_HW_FILTER_EXHAUST;
	case MIPC_RESULT_SYS_EXT_HBA_PASUE_ERR:
		return HBA_INT_ERR_MD_PASUE;
	case MIPC_RESULT_SYS_EXT_HBA_RESUME_ERR:
		return HBA_INT_ERR_MD_RESUME;
	case MIPC_RESULT_SYS_EXT_HBA_STOP_ERR:
		return HBA_INT_ERR_MD_STOP;
	case MIPC_RESULT_SYS_EXT_HBA_SEDN_TIME_OUT:
		return HBA_INT_ERR_MD_SEDN_TIME_OUT;
	case MIPC_RESULT_SYS_EXT_HBA_ACK_TIME_OUT:
		return HBA_INT_ERR_MD_ACK_TIME_OUT;
	case MIPC_RESULT_SYS_EXT_HBA_RESULT_INVALID:
	default:
		return HBA_INT_ERR_MD_UNKNOWN;
	}
}

/**
 * @brief      Convert MIPC control mode to HBA control mode.
 *
 * @param[in]  ctrl_mode  The control mode
 *
 * @return     The hba control mode enum.
 */
static hba_ctrl_mode_enum mipc_ctrl_mode_to_my_ctrl_mode(mipc_sys_hba_ctrl_mode_const_enum ctrl_mode) {
	switch (ctrl_mode) {
	case MIPC_SYS_HBA_CTRL_MODE_PAUSE:
		return HBA_CTRL_MODE_PAUSE;
	case MIPC_SYS_HBA_CTRL_MODE_STOP:
		return HBA_CTRL_MODE_STOP;
	default:
		return HBA_CTRL_MODE_UNKNOWN;
	}
}

/**
 * @brief      Reset heartbeat skb queue.
 *
 * @param      hba   The hba
 */
static void reset_hba_skb_queue(struct hba_st *hba, bool need_free)
{
	int i = 0;
	for (i = 0; i < HBA_SKB_QUEUE_LEN; i++) {
		if (need_free && hba->skb_queue[i]) {
			kfree_skb(hba->skb_queue[i]);
		}
		hba->skb_queue[i] = NULL;
	}
	hba->is_queue_overflow = false;
}

/**
 * @brief      Lookup sk by 5-tuple information in hba.
 *
 * @param      hba   The heartbeat
 *
 * @return     struct sock* if successful, NULL otherwise.
 */
struct sock *hba_lookup_sk(struct hba_st *hba)
{
	struct sock *sk = NULL;

	if (hba == NULL) {
		return NULL;
	}

	if (!hba->is_ipv6) {
		sk = __inet_lookup_established(&init_net, &tcp_hashinfo, hba->daddr,
		                               ntohs(hba->dport), hba->saddr,
		                               hba->sport, 0, 0);
		if (NULL == sk || !sk_fullsock(sk) || NULL == sk->sk_socket) {
			sk = NULL;
		}
		return sk;
	}
	else {
#if defined(CONFIG_IPV6) && defined(HEARTBEAT_MTK_IPV6)
		const struct in6_addr saddr6 =
			{	{	.u6_addr32 = {hba->v6_saddr32[0], hba->v6_saddr32[1],
					hba->v6_saddr32[2], hba->v6_saddr32[3]
				}
			}
		};
		const struct in6_addr daddr6 =
			{	{	.u6_addr32 = {hba->v6_daddr32[0], hba->v6_daddr32[1],
					hba->v6_daddr32[2], hba->v6_daddr32[3]
				}
			}
		};
		sk = __inet6_lookup_established(&init_net, &tcp_hashinfo, &daddr6,
		                                ntohs(hba->dport), &saddr6,
		                                hba->sport, 0, 0);
		if (NULL == sk || !sk_fullsock(sk) || NULL == sk->sk_socket) {
			sk = NULL;
		}
		return sk;
#endif
	}

	return NULL;
}

/**
 * @brief      Synchronize the Modem's TCP sequence number to the AP TCP protocol stack.
 *
 * @param      hba       The heartbeat
 * @param[in]  seq_sync  The TCP sequence number
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_sync_seq_from_md(struct hba_st *hba, hba_seq_sync_msg_struct seq_sync)
{
	int ret = 0;
	u32 n_snd_win = 1;
	struct tcp_sock *tp = NULL;
	struct sock *sk = NULL;

	if (!hba) {
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_md failed, hba is null!\n");
		return -EINVAL;
	}

	sk = hba_lookup_sk(hba);
	if (sk == NULL) {
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_md failed, proxy_key=%s, sk is null!\n", hba->proxy_key);
		return HBA_INT_ERR_SEQ_SYNC_SK_NULL;
	}

	tp = tcp_sk(sk);
	spin_lock_bh(&sk->sk_lock.slock);
	if (sk->sk_lock.owned) {
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_md failed, proxy_key=%s, sk owned!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_SK_OWNED;
	}
	else {
		n_snd_win = seq_sync.tcp_snd_wnd;
		n_snd_win <<= tp->rx_opt.snd_wscale;
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_md motify seq for proxy_key=%s, "
		       "copied_seq/rcv_nxt=%u/%u 2 %u, snd_nxt/snd_una/write_seq=%u/%u/%u 2 %u, snd_wnd=%u 2 %u.\n",
		       hba->proxy_key, tp->copied_seq, tp->rcv_nxt, seq_sync.tcp_rcv_nxt, tp->snd_nxt, tp->snd_una,
		       tp->write_seq, seq_sync.tcp_snd_nxt, tp->snd_wnd, n_snd_win);
		WRITE_ONCE(tp->copied_seq, seq_sync.tcp_rcv_nxt);
		WRITE_ONCE(tp->rcv_nxt, seq_sync.tcp_rcv_nxt);
		WRITE_ONCE(tp->snd_nxt, seq_sync.tcp_snd_nxt);
		WRITE_ONCE(tp->snd_una, seq_sync.tcp_snd_nxt);
		WRITE_ONCE(tp->write_seq, seq_sync.tcp_snd_nxt);
		WRITE_ONCE(tp->snd_wnd, n_snd_win);
	}
	spin_unlock_bh(&sk->sk_lock.slock);
	return ret;
}

/**
 * @brief      Synchronize the skb tcphdr sequence number to the AP TCP protocol stack.
 *
 * @param      hba   The heartbeat
 * @param[in]  sk    The sock
 * @param      tcph  The tcph
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_sync_seq_from_tcphdr(struct hba_st *hba, struct sock *sk, struct tcphdr *tcph)
{
	int ret = 0;
	u32 n_snd_win = 1;
	struct tcp_sock *tp = NULL;

	tp = tcp_sk(sk);
	spin_lock_bh(&sk->sk_lock.slock);
	if (sk->sk_lock.owned) {
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_tcphdr failed, proxy_key=%s, sk owned!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_SK_OWNED;
	} else {
		n_snd_win = ntohs(tcph->window);
		n_snd_win <<= tp->rx_opt.snd_wscale;
		printk("[heartbeat_proxy_mtk] hba_sync_seq_from_tcphdr motify seq for proxy_key=%s, copied_seq/rcv_nxt=%u/%u 2 %u, "
		       "snd_nxt/snd_una/write_seq=%u/%u/%u 2 %u, snd_wnd=%u 2 %u.\n",
		       hba->proxy_key, tp->copied_seq, tp->rcv_nxt, ntohl(tcph->seq), tp->snd_nxt,
		       tp->snd_una, tp->write_seq, ntohl(tcph->ack_seq), tp->snd_wnd, n_snd_win);
		WRITE_ONCE(tp->copied_seq, ntohl(tcph->seq));
		WRITE_ONCE(tp->rcv_nxt, ntohl(tcph->seq));
		WRITE_ONCE(tp->snd_nxt, ntohl(tcph->ack_seq));
		WRITE_ONCE(tp->snd_una, ntohl(tcph->ack_seq));
		WRITE_ONCE(tp->write_seq, ntohl(tcph->ack_seq));
		WRITE_ONCE(tp->snd_wnd, n_snd_win);
	}
	spin_unlock_bh(&sk->sk_lock.slock);
	return ret;
}

/**
 * @brief      Check whether the Modem's TCP sequence number is consistent with the sequence number of the AP cached packet.
 *             If they are the same, reinject the cached packet to the AP protocol stack, otherwise discard it.
 *
 * @param      hba       The heartbeat
 * @param[in]  seq_sync  The TCP sequence number
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_check_seq_and_reinject(struct hba_st *hba, hba_seq_sync_msg_struct seq_sync)
{
	int ret = 0, i = 0, iph_id = 0;
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct net *net = NULL;
	struct sock *sk = NULL;
	uint32_t tcp_snd_nxt = 0, tcp_rcv_nxt = 0;

	if (!hba) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, hba is null!\n");
		return -EINVAL;
	}

	if (!hba->skb_queue[0]) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, skb queue is null!\n", hba->proxy_key);
		ret = -EINVAL;
		goto error_with_free;
	}

	if (hba->is_queue_overflow) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, skb queue overflow!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_QUEUE_OVERFLOW;
		goto error_with_free;
	}

	sk = hba_lookup_sk(hba);
	if (sk == NULL) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, sk is null!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_SK_NULL;
		goto error_with_free;
	}

	net = sock_net(sk);
	if (NULL == net) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, net is null!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_SK_NULL;
		goto error_with_free;
	}

	iph = ip_hdr(hba->skb_queue[0]);
	tcph = tcp_hdr(hba->skb_queue[0]);
	if (hba->skb_queue[0]->protocol != htons(ETH_P_IP) || (!iph) || iph->protocol != IPPROTO_TCP || !tcph) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, first skb is invalid!\n", hba->proxy_key);
		ret = HBA_INT_ERR_SEQ_SYNC_SKB_INVALID;
		goto error_with_free;
	}

	tcp_snd_nxt = ntohl(tcph->ack_seq);
	tcp_rcv_nxt = ntohl(tcph->seq);

	/* If the sending sequence numbers of the Modem and Ap are different, an
	 * error will be reported directly, because the Ap cannot send the
	 * retransmission packet to the server. */
	if (seq_sync.tcp_snd_nxt != tcp_snd_nxt) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, send seq not match(%u:%u).\n",
		       hba->proxy_key, seq_sync.tcp_snd_nxt, tcp_snd_nxt);
		ret = HBA_INT_ERR_SEQ_SYNC_NOT_MATCH;
		goto error_with_free;
	}

	/* If the receiving sequence number of the Ap is greater than that of Modem,
	 * it means that packet loss has occurred. At this time, reset the receiving
	 * sequence number to the sequence number of the Modem, so as to notify the
	 * server to initiate retransmission. */
	if (seq_sync.tcp_rcv_nxt < tcp_rcv_nxt) {
		printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, rcv seq not match(%u:%u), try reset local seq.\n",
		       hba->proxy_key, seq_sync.tcp_rcv_nxt, tcp_rcv_nxt);
		ret = hba_sync_seq_from_md(hba, seq_sync);
		if (ret != 0) {
			printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject failed, proxy_key=%s, sync err=%d.",
			       hba->proxy_key, ret);
			goto error_with_free;
		}
	}

	for (i = 0; i < HBA_SKB_QUEUE_LEN; i++) {
		if (hba->skb_queue[i]) {
			iph = ip_hdr(hba->skb_queue[i]);
			tcph = tcp_hdr(hba->skb_queue[i]);
			if (hba->skb_queue[i]->protocol != htons(ETH_P_IP) || (!iph) || iph->protocol != IPPROTO_TCP || !tcph) {
				kfree_skb(hba->skb_queue[i]);
				printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject reinject failed, proxy_key=%s index=%d, skb is invalid!\n",
				       hba->proxy_key, i);
			} else {
				iph_id = iph->id;
				if (hba->ip_local_deliver_finish) {
					hba->ip_local_deliver_finish(net, sk, hba->skb_queue[i]);
					printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject proxy_key=%s id=%d index=%d.\n",
					       hba->proxy_key, iph_id, i);
				} else {
					kfree_skb(hba->skb_queue[i]);
					printk("[heartbeat_proxy_mtk] hba_check_seq_and_reinject reinject failed, proxy_key=%s id=%d index=%d, fun is invalid!\n",
					       hba->proxy_key, iph_id, i);
				}
			}
		}
		hba->skb_queue[i] = NULL;
	}

	hba->is_queue_overflow = false;
	return 0;

error_with_free:
	reset_hba_skb_queue(hba, true);
	return ret;
}

/**
 * @brief      Send a message to user space heartbeat program.
 *
 * @param[in]  netlink_id  The netlink identifier
 * @param[in]  proxy_key   The proxy key
 * @param[in]  err         The error
 * @param[in]  curr_cycle  The curr cycle
 * @param      statistics  The statistics
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_msg_to_hba_user(int netlink_id, const char *proxy_key, hba_result_enum err, uint16_t curr_cycle, hba_statistics_msg_struct *statistics)
{
	int ret = 0;
	char msg_buf[sizeof(netlink_ul_hba_response_msg_st)] = { 0 };
	netlink_ul_hba_response_msg_st *response_msg = (netlink_ul_hba_response_msg_st *)msg_buf;

	memset(msg_buf, 0x0, sizeof(msg_buf));

	safe_strncpy(response_msg->proxy_key, proxy_key, sizeof(response_msg->proxy_key));
	response_msg->err = err;
	response_msg->curr_cycle = curr_cycle;
	if (statistics) {
		response_msg->statistics.send_count = statistics->send_count;
		response_msg->statistics.recv_count = statistics->recv_count;
		response_msg->statistics.hitchhike_count = statistics->hitchhike_count;
	}

	ret = netlink_send_to_user(netlink_id, (char *)msg_buf, sizeof(msg_buf));
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] send_msg_to_hba_user failed, ret=%d.\n", ret);
	}

	return ret;
}

/**
 * @brief      The handler of heartbeat pause request timeout.
 *
 * @param      t     timer context
 */
static void hba_pause_timer_handler(struct timer_list *t)
{
	char proxy_key[128];
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;

	memset(proxy_key, 0x0, sizeof(proxy_key));

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if ((&hba_node_entry->hba.pause_timer == t) && (hba_node_entry->hba.state == HBA_STATE_PAUSE_REQUEST)) {
			printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s check pause failed, err=timeout.",
			       hba_node_entry->hba.proxy_key);
			safe_strncpy(proxy_key, hba_node_entry->hba.proxy_key, sizeof(proxy_key));
			hba_node_entry->hba.state = HBA_STATE_ERROR;
			hba_node_entry->hba.is_pause_timeout = true;
			err = HBA_INT_ERR_IPC_TIMEOUT;
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	return;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	send_msg_to_hba_user(NETLINK_UNSOL_HBA_PAUSE, proxy_key, err, 0, NULL);
	return;
}

/**
 * @brief      Removes the specified node from the linked list
 *
 * @param      hba_node  The heartbeat node
 */
static void hba_delete_node(struct hba_list_node_st *hba_node)
{
	if (hba_node) {
		list_del(&hba_node->list_node);
		kfree(hba_node);
		hba_list.count--;
	}
}

/**
 * @brief      Removes the specified node from the linked list with spin lock
 *
 * @param      hba_node  The heartbeat node
 */
static void hba_delete_node_lock(struct hba_list_node_st *hba_node)
{
	spin_lock_bh(&hba_list_lock);
	if (hba_node) {
		list_del(&hba_node->list_node);
		kfree(hba_node);
		hba_list.count--;
	}
	spin_unlock_bh(&hba_list_lock);
}

/**
 * @brief      The handler of heartbeat establish-reply reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_establish_response(mipc_msg_t *msg_ptr)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_estblish_cnf_struct estblish_cnf;
	char *proxy_key = NULL;
	uint16_t proxy_key_len = 0;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_establish_response failed, msg_ptr is null!");
		return -EINVAL;
	}

	memset(&estblish_cnf, 0x0, sizeof(hba_estblish_cnf_struct));

	estblish_cnf.result = mipc_error_to_my_error(mipc_get_result(msg_ptr));
	proxy_key = mipc_sys_hba_establish_cnf_get_proxy_key(msg_ptr, &proxy_key_len);
	if (proxy_key_len == 0 || proxy_key_len > sizeof(estblish_cnf.proxy_key)) {
		printk("[heartbeat_proxy_mtk] hba_mipc_establish_response failed, proxy_key_len is invalid!");
		return -EINVAL;
	}

	safe_strncpy(estblish_cnf.proxy_key, proxy_key, sizeof(estblish_cnf.proxy_key));

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            estblish_cnf.proxy_key, strlen(estblish_cnf.proxy_key)) == 0) {
			if (estblish_cnf.result == HBA_INT_PROCESS_OK) {
				printk("[heartbeat_proxy_mtk] hba_mipc_establish_response proxy_key=%s successfully!",
				       estblish_cnf.proxy_key);
				hba_node_entry->hba.state = HBA_STATE_ESTABLISH_OK;
			}
			else {
				printk("[heartbeat_proxy_mtk] hba_mipc_establish_response proxy_key=%s failed, ret=%d.",
				       estblish_cnf.proxy_key, estblish_cnf.result);
				err = estblish_cnf.result;
				hba_delete_node(hba_node_entry);
			}
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] hba_mipc_establish_response failed, proxy_key=%s is invalid.\n",
	       estblish_cnf.proxy_key);
	return -ENODEV;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_ESTABLISH, estblish_cnf.proxy_key, err, 0, NULL);
	return ret;
}

/**
 * @brief      The handler of heartbeat control-reply reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_ctrl_response(mipc_msg_t *msg_ptr)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_ctrl_cnf_struct ctrl_cnf;
	char *proxy_key = NULL;
	uint16_t proxy_key_len = 0;
	bool need_sync = false;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response failed, msg_ptr is null!");
		return -EINVAL;
	}

	memset(&ctrl_cnf, 0x0, sizeof(hba_ctrl_cnf_struct));

	ctrl_cnf.result = mipc_error_to_my_error(mipc_get_result(msg_ptr));
	proxy_key = mipc_sys_hba_ctrl_cnf_get_proxy_key(msg_ptr, &proxy_key_len);
	if (proxy_key_len == 0 || proxy_key_len > sizeof(ctrl_cnf.proxy_key)) {
		printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response failed, proxy_key_len is invalid!");
		return -EINVAL;
	}

	safe_strncpy(ctrl_cnf.proxy_key, proxy_key, sizeof(ctrl_cnf.proxy_key));

	ctrl_cnf.mode = mipc_ctrl_mode_to_my_ctrl_mode(mipc_sys_hba_ctrl_cnf_get_mode(msg_ptr, MIPC_SYS_HBA_CTRL_MODE_INVALID));
	ctrl_cnf.curr_cycle = mipc_sys_hba_ctrl_cnf_get_current_cycle(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.seq_sync.tcp_snd_wnd = mipc_sys_hba_ctrl_cnf_get_tcp_snd_window(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.seq_sync.tcp_rcv_wnd = mipc_sys_hba_ctrl_cnf_get_tcp_rcv_window(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.seq_sync.tcp_snd_nxt = mipc_sys_hba_ctrl_cnf_get_tcp_snd_nxt_seq(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.seq_sync.tcp_rcv_nxt = mipc_sys_hba_ctrl_cnf_get_tcp_rcv_nxt_seq(msg_ptr, MIPC_DEF_VAL);

	ctrl_cnf.statistics.send_count = mipc_sys_hba_ctrl_cnf_get_send_count(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.statistics.recv_count = mipc_sys_hba_ctrl_cnf_get_recv_count(msg_ptr, MIPC_DEF_VAL);
	ctrl_cnf.statistics.hitchhike_count = mipc_sys_hba_ctrl_cnf_get_hitchhike_count(msg_ptr, MIPC_DEF_VAL);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            ctrl_cnf.proxy_key, strlen(ctrl_cnf.proxy_key)) == 0) {
			if (ctrl_cnf.result == HBA_INT_PROCESS_OK) {
				if (ctrl_cnf.mode == HBA_CTRL_MODE_PAUSE) {
					hba_node_entry->hba.state = HBA_STATE_PAUSE_OK;
					if (hba_node_entry->hba.need_sync) {
						need_sync = true;
						ret = hba_sync_seq_from_md(&hba_node_entry->hba, ctrl_cnf.seq_sync);
						if (ret != 0) {
							hba_node_entry->hba.state = HBA_STATE_ERROR;
							if (ret > 0) {
								err = ret;
							} else {
								err = HBA_INT_ERR_SEQ_SYNC_UNKNOWN;
							}
							printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s sync pause failed, err=%d.",
							       ctrl_cnf.proxy_key, err);
							goto notify_user_with_unlock;
						}
						/* check if the seq number is consistent between Modem and Ap */
					} else {
						if (hba_node_entry->hba.is_pause_timeout) {
							printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s check pause failed, err=timeout.",
							       ctrl_cnf.proxy_key);
							goto return_with_unlock;
						}
						hba_node_entry->hba.is_pause_timeout = false;
						del_timer(&hba_node_entry->hba.pause_timer);
						ret = hba_check_seq_and_reinject(&hba_node_entry->hba, ctrl_cnf.seq_sync);
						if (ret != 0) {
							hba_node_entry->hba.state = HBA_STATE_ERROR;
							if (ret > 0) {
								err = ret;
							} else {
								err = HBA_INT_ERR_SEQ_SYNC_UNKNOWN;
							}
							printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s check pause failed, err=%d.",
							       ctrl_cnf.proxy_key, err);
							goto notify_user_with_unlock;
						}
					}
					printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s pause successfully!",
					       ctrl_cnf.proxy_key);
				}
				else if (ctrl_cnf.mode == HBA_CTRL_MODE_STOP) {
					hba_node_entry->hba.state = HBA_STATE_NONE;
					if (hba_node_entry->hba.need_sync) {
						need_sync = true;
						ret = hba_sync_seq_from_md(&hba_node_entry->hba, ctrl_cnf.seq_sync);
						if (ret != 0) {
							hba_node_entry->hba.state = HBA_STATE_ERROR;
							if (ret > 0) {
								err = ret;
							} else {
								err = HBA_INT_ERR_SEQ_SYNC_UNKNOWN;
							}
							printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s stop failed, err=%d",
							       ctrl_cnf.proxy_key, err);
							goto notify_user_with_unlock;
						}
					}
					list_del(&hba_node_entry->list_node);
					kfree(hba_node_entry);
					hba_list.count--;
					printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s stop successfully!",
					       ctrl_cnf.proxy_key);
				}
			}
			else {
				printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response proxy_key=%s failed, mode=%d ret=%d.",
				       ctrl_cnf.proxy_key, ctrl_cnf.mode, ctrl_cnf.result);
				err = ctrl_cnf.result;
			}
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] hba_mipc_ctrl_response failed, proxy_key=%s is invalid!\n",
	       ctrl_cnf.proxy_key);
	return -ENODEV;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	if (ctrl_cnf.mode == HBA_CTRL_MODE_PAUSE) {
		if (need_sync) {
			ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_PAUSE, ctrl_cnf.proxy_key, err, ctrl_cnf.curr_cycle, &ctrl_cnf.statistics);
		}
		else {
			ret = send_msg_to_hba_user(NETLINK_UNSOL_HBA_PAUSE, ctrl_cnf.proxy_key, err, ctrl_cnf.curr_cycle, &ctrl_cnf.statistics);
		}
	}
	else if (ctrl_cnf.mode == HBA_CTRL_MODE_STOP) {
		ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_STOP, ctrl_cnf.proxy_key, err, ctrl_cnf.curr_cycle, &ctrl_cnf.statistics);
	}
	return ret;

return_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	return ret;
}

/**
 * @brief      The handler of heartbeat resume-reply reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_resume_response(mipc_msg_t *msg_ptr)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_resume_cnf_struct resume_cnf;
	char *proxy_key = NULL;
	uint16_t proxy_key_len = 0;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_resume_response failed, msg_ptr is null!");
		return -EINVAL;
	}

	memset(&resume_cnf, 0x0, sizeof(hba_resume_cnf_struct));

	resume_cnf.result = mipc_error_to_my_error(mipc_get_result(msg_ptr));
	proxy_key = mipc_sys_hba_resume_cnf_get_proxy_key(msg_ptr, &proxy_key_len);
	if (proxy_key_len == 0 || proxy_key_len > sizeof(resume_cnf.proxy_key)) {
		printk("[heartbeat_proxy_mtk] hba_mipc_resume_response failed, proxy_key_len is invalid!");
		return -EINVAL;
	}

	safe_strncpy(resume_cnf.proxy_key, proxy_key, sizeof(resume_cnf.proxy_key));

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            resume_cnf.proxy_key, strlen(resume_cnf.proxy_key)) == 0) {
			if (resume_cnf.result == HBA_INT_PROCESS_OK) {
				printk("[heartbeat_proxy_mtk] hba_mipc_resume_response proxy_key=%s successfully!",
				       resume_cnf.proxy_key);
				hba_node_entry->hba.state = HBA_STATE_ESTABLISH_OK;
			}
			else {
				printk("[heartbeat_proxy_mtk] hba_mipc_resume_response proxy_key=%s failed, ret=%d.",
				       resume_cnf.proxy_key, resume_cnf.result);
				err = resume_cnf.result;
			}
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] hba_mipc_resume_response failed, proxy_key=%s is invalid.\n",
	       resume_cnf.proxy_key);
	return -ENODEV;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_RESUME, resume_cnf.proxy_key, err, 0, NULL);
	return ret;
}

/**
 * @brief      The handler of heartbeat send_now-reply reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_send_now_response(mipc_msg_t *msg_ptr)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_send_now_cnf_struct send_now_cnf;
	char *proxy_key = NULL;
	uint16_t proxy_key_len = 0;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_send_now_response failed, msg_ptr is null!");
		return -EINVAL;
	}

	memset(&send_now_cnf, 0x0, sizeof(hba_send_now_cnf_struct));

	send_now_cnf.result = mipc_error_to_my_error(mipc_get_result(msg_ptr));
	proxy_key = mipc_sys_hba_send_now_cnf_get_proxy_key(msg_ptr, &proxy_key_len);
	if (proxy_key_len == 0 || proxy_key_len > sizeof(send_now_cnf.proxy_key)) {
		printk("[heartbeat_proxy_mtk] hba_mipc_send_now_response failed, proxy_key_len is invalid!");
		return -EINVAL;
	}

	safe_strncpy(send_now_cnf.proxy_key, proxy_key, sizeof(send_now_cnf.proxy_key));

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            send_now_cnf.proxy_key, strlen(send_now_cnf.proxy_key)) == 0) {
			if (send_now_cnf.result == HBA_INT_PROCESS_OK) {
				printk("[heartbeat_proxy_mtk] hba_mipc_send_now_response proxy_key=%s successfully!",
				       send_now_cnf.proxy_key);
			}
			else {
				printk("[heartbeat_proxy_mtk] hba_mipc_send_now_response proxy_key=%s failed, ret=%d.",
				       send_now_cnf.proxy_key, send_now_cnf.result);
				err = send_now_cnf.result;
			}
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] hba_mipc_send_now_response failed, proxy_key=%s is invalid.\n",
	       send_now_cnf.proxy_key);
	return -ENODEV;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_SEND_NOW, send_now_cnf.proxy_key, err, 0, NULL);
	return ret;
}

/**
 * @brief      The handler of heartbeat timeout-indication reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_timeout_ind(mipc_msg_t *msg_ptr)
{
	int ret = 0, sync_ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_timeout_ind_struct timeout_ind;
	char *proxy_key = NULL;
	uint16_t proxy_key_len = 0;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_timeout_ind failed, msg_ptr is null!");
		return -EINVAL;
	}

	memset(&timeout_ind, 0x0, sizeof(hba_timeout_ind_struct));

	timeout_ind.result = mipc_error_to_my_error(mipc_get_result(msg_ptr));
	proxy_key = mipc_sys_hba_timeout_ind_get_proxy_key(msg_ptr, &proxy_key_len);
	if (proxy_key_len == 0 || proxy_key_len > sizeof(timeout_ind.proxy_key)) {
		printk("[heartbeat_proxy_mtk] hba_mipc_timeout_ind failed, proxy_key_len is invalid!");
		return -EINVAL;
	}

	safe_strncpy(timeout_ind.proxy_key, proxy_key, sizeof(timeout_ind.proxy_key));

	timeout_ind.curr_cycle = mipc_sys_hba_timeout_ind_get_current_cycle(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.statistics.send_count = mipc_sys_hba_timeout_ind_get_send_count(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.statistics.recv_count = mipc_sys_hba_timeout_ind_get_recv_count(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.statistics.hitchhike_count = mipc_sys_hba_timeout_ind_get_hitchhike_count(msg_ptr, MIPC_DEF_VAL);

	timeout_ind.seq_sync.tcp_snd_wnd = mipc_sys_hba_timeout_ind_get_tcp_snd_window(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.seq_sync.tcp_rcv_wnd = mipc_sys_hba_timeout_ind_get_tcp_rcv_window(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.seq_sync.tcp_snd_nxt = mipc_sys_hba_timeout_ind_get_tcp_snd_nxt_seq(msg_ptr, MIPC_DEF_VAL);
	timeout_ind.seq_sync.tcp_rcv_nxt = mipc_sys_hba_timeout_ind_get_tcp_rcv_nxt_seq(msg_ptr, MIPC_DEF_VAL);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            timeout_ind.proxy_key, strlen(timeout_ind.proxy_key)) == 0) {
			sync_ret = hba_sync_seq_from_md(&hba_node_entry->hba, timeout_ind.seq_sync);
			printk("[heartbeat_proxy_mtk] hba_mipc_timeout_ind proxy_key=%s ret=%d sync=%d.",
			       timeout_ind.proxy_key, timeout_ind.result, sync_ret);
			err = timeout_ind.result;
			goto notify_user_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] hba_mipc_timeout_ind failed, proxy_key=%s is invalid.\n",
	       timeout_ind.proxy_key);
	return -ENODEV;

notify_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_msg_to_hba_user(NETLINK_UNSOL_HBA_TIMEOUT, timeout_ind.proxy_key, err, timeout_ind.curr_cycle, &timeout_ind.statistics);
	return ret;
}

/**
 * @brief      The handler of heartbeat hardware_state-indication reported by the Modem.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_hw_state_ind(mipc_msg_t *msg_ptr)
{
	hba_hwfltr_st_ind_struct hwfltr_st_ind;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_hw_state_ind failed, msg_ptr is null!");
		return -EINVAL;
	}

	hwfltr_st_ind.hwfltr_st = mipc_sys_hba_hw_filter_src_state_ind_get_hw_filter_src_state(msg_ptr, HBA_HW_FILTER_EXHAUST);

	printk("[heartbeat_proxy_mtk] hba_mipc_hw_state_ind hwfltr state=%d.\n",
	       hwfltr_st_ind.hwfltr_st);
	return 0;
}

/**
 * @brief      The handler of MIPC message reported by the CCCI drive.
 *
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_mipc_msg_hdlr(mipc_msg_t *msg_ptr)
{
	int ret = 0;

	if (NULL == msg_ptr) {
		printk("[heartbeat_proxy_mtk] hba_mipc_msg_hdlr failed, msg_ptr is null!");
		return -EINVAL;
	}

	switch (msg_ptr->hdr.msg_id) {
	case MIPC_SYS_HBA_ESTABLISH_CNF:
		ret = hba_mipc_establish_response(msg_ptr);
		break;
	case MIPC_SYS_HBA_CTRL_CNF:
		ret = hba_mipc_ctrl_response(msg_ptr);
		break;
	case MIPC_SYS_HBA_RESUME_CNF:
		ret = hba_mipc_resume_response(msg_ptr);
		break;
	case MIPC_SYS_HBA_SEND_NOW_CNF:
		ret = hba_mipc_send_now_response(msg_ptr);
		break;
	case MIPC_SYS_HBA_TIMEOUT_IND:
		ret = hba_mipc_timeout_ind(msg_ptr);
		break;
	case MIPC_SYS_HBA_HW_FILTER_SRC_STATE_IND:
		ret = hba_mipc_hw_state_ind(msg_ptr);
		break;
	default:
		printk("[heartbeat_proxy_mtk] hba_mipc_msg_hdlr failed, unknown msg_id=%d.\n", msg_ptr->hdr.msg_id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief      The handler of CCCI message reported by the Modem.
 *
 * @param      arg   The argument
 *
 * @return     0 if successful, negative otherwise.
 */
static int __nocfi hba_ccci_md_msg_hdlr(void *arg)
{
	mipc_msg_t *msg_ptr = NULL;
	mipc_msg_hdr_t *msg_hdr_ptr = NULL;
	int rx_count = 0;
	int recv_buf_len = 0;
	uint8_t recv_buf_ptr[CCCI_MAX_RECV_BUF] = { 0 };

	while (1) {
		if (hba_ccci_ipc_port_s < 0) {
			printk("[heartbeat_proxy_mtk] hba_ccci_md_msg_hdlr failed, port=%d is invalid!\n", hba_ccci_ipc_port_s);
			msleep(5000);
			continue;
		}
		memset(recv_buf_ptr, 0, sizeof(recv_buf_ptr));
		rx_count = mtk_ccci_read_data(hba_ccci_ipc_port_s, recv_buf_ptr, sizeof(mipc_msg_hdr_t));
		if (rx_count != sizeof(mipc_msg_hdr_t)) {
			printk("[heartbeat_proxy_mtk] hba_ccci_md_msg_hdlr failed, mipc header length is invalid!");
			msleep(1000);
			continue;
		}

		msg_hdr_ptr = (mipc_msg_hdr_t *)recv_buf_ptr;
		if (msg_hdr_ptr->msg_len > 0) {
			recv_buf_len = rx_count + msg_hdr_ptr->msg_len;
			rx_count += mtk_ccci_read_data(hba_ccci_ipc_port_s, recv_buf_ptr + rx_count, msg_hdr_ptr->msg_len);
		}
		else {
			recv_buf_len = rx_count;
		}

		if (rx_count != recv_buf_len) {
			printk("[heartbeat_proxy_mtk] hba_ccci_md_msg_hdlr failed, recv_buf_len is invalid!");
			msleep(1000);
			continue;
		}

		if ((msg_ptr = mipc_msg_deserialize(recv_buf_ptr, recv_buf_len)) == NULL) {
			printk("[heartbeat_proxy_mtk] hba_ccci_md_msg_hdlr mipc deserialize failed!");
			msleep(1000);
			continue;
		}

		hba_mipc_msg_hdlr(msg_ptr);
	}

	return 0;
}

/**
 * @brief      Send a MIPC message to Modem.
 *
 * @param[in]  port_id  The port identifier
 * @param      msg_ptr  The message pointer
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_mipc(int port_id, mipc_msg_t *msg_ptr)
{
	uint8_t *send_buf_ptr = NULL;
	uint16_t send_buf_len = 0;

	send_buf_ptr = mipc_msg_serialize(msg_ptr, &send_buf_len);
	if (send_buf_ptr == NULL) {
		printk("[heartbeat_proxy_mtk] send_mipc failed, send buf is null!\n");
		return -ENODEV;
	}

	mtk_ccci_send_data(port_id, send_buf_ptr, send_buf_len);
	return 0;
}

/**
 * @brief      Register with MIPC the message ID that needs to be actively reported by Modem.
 *
 * @param[in]  msg_id  The message identifier
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_ccci_ipc_register_ind(uint16_t msg_id)
{
	mipc_msg_api_result_enum ret = MIPC_MSG_API_RESULT_FAIL;
	mipc_msg_t *msg = NULL;

	msg = mipc_msg_init(MIPC_INTERNAL_REGISTER_IND_REQ, MIPC_MSG_PS0);
	if (!msg)
		return -ENODEV;

	ret = mipc_internal_register_ind_req_add_msg_id(msg, msg_id);
	if (ret != MIPC_MSG_API_RESULT_SUCCESS) {
		return -EINVAL;
	}

	send_mipc(hba_ccci_ipc_port_s, msg);
	mipc_msg_deinit(msg);

	return 0;
}

/**
 * @brief      Open CCCI communication port.
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_ccci_ipc_open_port(void)
{
	int ret = 0;

	hba_ccci_ipc_port_s = mtk_ccci_request_port(CCCI_HBA_MIPC_NAME);
	if (hba_ccci_ipc_port_s < 0) {
		printk("[heartbeat_proxy_mtk] mtk_ccci_request_port failed, port=%d.\n", hba_ccci_ipc_port_s);
		return -ENODEV;
	}

	ret = mtk_ccci_open_port(hba_ccci_ipc_port_s);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] mtk_ccci_open_port failed, port=%d, ret=%d.\n", hba_ccci_ipc_port_s, ret);
		return -ENODEV;
	}

	ccci_rx_task = kthread_run(hba_ccci_md_msg_hdlr, NULL, CCCI_RX_TASK_NAME);
	if (IS_ERR(ccci_rx_task)) {
		printk("[heartbeat_proxy_mtk] kthread_run failed, ccci_rx_task error.\n");
		ccci_rx_task = NULL;
		mtk_ccci_release_port(hba_ccci_ipc_port_s);
		return -ECHILD;
	}

	printk("[heartbeat_proxy_mtk] hba_ccci_ipc_open_port successfully, port=%d.", hba_ccci_ipc_port_s);

	return 0;
}

/**
 * @brief      Close CCCI communication port.
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_ccci_ipc_close_port(void)
{
	if (ccci_rx_task) {
		kthread_stop(ccci_rx_task);
		ccci_rx_task = NULL;
	}

	mtk_ccci_release_port(hba_ccci_ipc_port_s);

	return 0;
}

/**
 * @brief      Gets the uid from sock.
 *
 * @param[in]  sk    The sock
 *
 * @return     The uid from sock.
 */
static uid_t get_uid_from_sock(const struct sock *sk)
{
	uid_t sk_uid = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	const struct file *filp = NULL;
#endif
	if (NULL == sk || !sk_fullsock(sk) || NULL == sk->sk_socket) {
		return 0;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	filp = sk->sk_socket->file;
	if (NULL == filp) {
		return 0;
	}
	sk_uid = __kuid_val(filp->f_cred->fsuid);
#else
	sk_uid = __kuid_val(sk->sk_uid);
#endif
	return sk_uid;
}

/**
 * @brief      Initialize heartbeat IPv4 netfilter hook program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_netfilter_v4_init(void)
{
	int ret = 0;

	ret = nf_register_net_hooks(&init_net, hba_input_v4_ops, ARRAY_SIZE(hba_input_v4_ops));
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] hba_netfilter_v4_init failed, ret=%d.\n", ret);
	}
	else {
		printk("[heartbeat_proxy_mtk] hba_netfilter_v4_init successfully!\n");
	}

	return ret;
}

/**
 * @brief      Initialize heartbeat IPv6 netfilter hook program.
 *
 * @return     0 if successful, negative otherwise.
 */
static int hba_netfilter_v6_init(void)
{
	int ret = 0;

	ret = nf_register_net_hooks(&init_net, hba_input_v6_ops, ARRAY_SIZE(hba_input_v6_ops));
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] hba_netfilter_v6_init failed, ret=%d.\n", ret);
	}
	else {
		printk("[heartbeat_proxy_mtk] hba_netfilter_v6_init successfully!\n");
	}

	return ret;
}

/**
 * @brief      Sends a MIPC estblish message to Modem.
 *
 * @param[in]  estblish_req  The estblish request
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_mipc_estblish_msg_to_md(hba_estblish_req_struct estblish_req)
{
	int i = 0;
	mipc_msg_api_result_enum ret[32] = { MIPC_MSG_API_RESULT_FAIL };
	mipc_msg_t *msg = NULL;

	if (estblish_req.default_phone_id == 0) {
		msg = mipc_msg_init(MIPC_SYS_HBA_ESTABLISH_REQ, MIPC_MSG_PS0);
	} else if (estblish_req.default_phone_id == 1) {
		msg = mipc_msg_init(MIPC_SYS_HBA_ESTABLISH_REQ, MIPC_MSG_PS1);
	} else {
		return -EINVAL;
	}
	if (!msg)
		return -ENODEV;

	ret[0] = mipc_sys_hba_establish_req_add_proxy_key(msg, strlen(estblish_req.proxy_key) + 1, estblish_req.proxy_key);
	ret[1] = mipc_sys_hba_establish_req_add_if_id(msg, estblish_req.interface_id);
	ret[2] = mipc_sys_hba_establish_req_add_hb_pattern(msg, estblish_req.hb_len, estblish_req.hb_pattern);
	ret[3] = mipc_sys_hba_establish_req_add_hb_ack_pattern(msg, estblish_req.hback_len, estblish_req.hback_pattern);
	ret[4] = mipc_sys_hba_establish_req_add_hitchhike_interval(msg, estblish_req.hitchhike_interval);
	ret[5] = mipc_sys_hba_establish_req_add_allow_dynamic_cycle(msg, estblish_req.is_allow_dynamic_cycle);
	ret[6] = mipc_sys_hba_establish_req_add_cycle_value(msg, estblish_req.cycle);
	ret[7] = mipc_sys_hba_establish_req_add_max_cycle(msg, estblish_req.max_cycle);
	ret[8] = mipc_sys_hba_establish_req_add_cycle_step(msg, estblish_req.cycle_step);
	ret[9] = mipc_sys_hba_establish_req_add_cycle_step_success_num(msg, estblish_req.cycle_step_success_num);

	ret[10] = mipc_sys_hba_establish_req_add_ipv4_tos(msg, estblish_req.seq_sync.ipv4_tos);
	ret[11] = mipc_sys_hba_establish_req_add_ipv4_ttl(msg, estblish_req.seq_sync.ipv4_ttl);
	ret[12] = mipc_sys_hba_establish_req_add_ipv4_id(msg, estblish_req.seq_sync.ipv4_id);
	ret[13] = mipc_sys_hba_establish_req_add_ipv6_flow_label(msg, estblish_req.seq_sync.ipv6_flow_label);
	ret[14] = mipc_sys_hba_establish_req_add_ipv6_hop_limit(msg, estblish_req.seq_sync.ipv6_hop_limit);
	ret[15] = mipc_sys_hba_establish_req_add_ipv6_tclass(msg, estblish_req.seq_sync.ipv6_tclass);

	ret[16] = mipc_sys_hba_establish_req_add_tcp_rcv_wscale(msg, estblish_req.seq_sync.tcp_rcv_wscale);
	ret[17] = mipc_sys_hba_establish_req_add_tcp_snd_wscale(msg, estblish_req.seq_sync.tcp_snd_wscale);
	ret[18] = mipc_sys_hba_establish_req_add_tcp_wscale_ok(msg, estblish_req.seq_sync.tcp_wscale_ok);
	ret[19] = mipc_sys_hba_establish_req_add_tcp_snd_window(msg, estblish_req.seq_sync.tcp_snd_wnd);
	ret[20] = mipc_sys_hba_establish_req_add_tcp_rcv_window(msg, estblish_req.seq_sync.tcp_rcv_wnd);
	ret[21] = mipc_sys_hba_establish_req_add_tcp_snd_nxt_seq(msg, estblish_req.seq_sync.tcp_snd_nxt);
	ret[22] = mipc_sys_hba_establish_req_add_tcp_rcv_nxt_seq(msg, estblish_req.seq_sync.tcp_rcv_nxt);

	ret[23] = mipc_sys_hba_establish_req_add_v4_src_addr(msg, sizeof(mipc_data_v4_addr_struct4),
	          (mipc_data_v4_addr_struct4 *)estblish_req.netinfo_ctrl.saddr);
	ret[24] = mipc_sys_hba_establish_req_add_v4_dest_addr(msg, sizeof(mipc_data_v4_addr_struct4),
	          (mipc_data_v4_addr_struct4 *)estblish_req.netinfo_ctrl.daddr);
	ret[25] = mipc_sys_hba_establish_req_add_v6_src_addr(msg, sizeof(mipc_data_v6_addr_struct4),
	          (mipc_data_v6_addr_struct4 *)estblish_req.netinfo_ctrl.v6_saddr8);
	ret[26] = mipc_sys_hba_establish_req_add_v6_dest_addr(msg, sizeof(mipc_data_v6_addr_struct4),
	          (mipc_data_v6_addr_struct4 *)estblish_req.netinfo_ctrl.v6_daddr8);
	ret[27] = mipc_sys_hba_establish_req_add_is_ipv6(msg, estblish_req.netinfo_ctrl.is_ipv6 == 1 ? MIPC_BOOLEAN_TRUE : MIPC_BOOLEAN_FALSE);
	ret[28] = mipc_sys_hba_establish_req_add_protocol(msg, estblish_req.netinfo_ctrl.protocol);
	ret[29] = mipc_sys_hba_establish_req_add_src_port(msg, estblish_req.netinfo_ctrl.sport);
	ret[30] = mipc_sys_hba_establish_req_add_dest_port(msg, estblish_req.netinfo_ctrl.dport);
	ret[31] = mipc_sys_hba_establish_req_add_max_retry_times(msg, estblish_req.netinfo_ctrl.tcp_retries2);

	for (i = 0; i < 32; i++) {
		if (ret[i] != MIPC_MSG_API_RESULT_SUCCESS) {
			mipc_msg_deinit(msg);
			return -EINVAL;
		}
	}

	send_mipc(hba_ccci_ipc_port_s, msg);
	mipc_msg_deinit(msg);

	return 0;
}

/**
 * @brief      Sends a MIPC control message to Modem.
 *
 * @param[in]  ctrl_req  The control request
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_mipc_ctrl_msg_to_md(hba_ctrl_req_struct ctrl_req)
{
	int i = 0;
	mipc_msg_api_result_enum ret[2] = { MIPC_MSG_API_RESULT_FAIL };
	mipc_msg_t *msg = NULL;

	if (ctrl_req.default_phone_id == 0) {
		msg = mipc_msg_init(MIPC_SYS_HBA_CTRL_REQ, MIPC_MSG_PS0);
	} else if (ctrl_req.default_phone_id == 1) {
		msg = mipc_msg_init(MIPC_SYS_HBA_CTRL_REQ, MIPC_MSG_PS1);
	} else {
		return -EINVAL;
	}
	if (!msg)
		return -ENODEV;

	ret[0] = mipc_sys_hba_ctrl_req_add_proxy_key(msg, strlen(ctrl_req.proxy_key) + 1, ctrl_req.proxy_key);
	if (ctrl_req.mode == HBA_CTRL_MODE_PAUSE) {
		ret[1] = mipc_sys_hba_ctrl_req_add_mode(msg, MIPC_SYS_HBA_CTRL_MODE_PAUSE);
	}
	else if (ctrl_req.mode == HBA_CTRL_MODE_STOP) {
		ret[1] = mipc_sys_hba_ctrl_req_add_mode(msg, MIPC_SYS_HBA_CTRL_MODE_STOP);
	}
	else {
		return -EINVAL;
	}

	for (i = 0; i < 2; i++) {
		if (ret[i] != MIPC_MSG_API_RESULT_SUCCESS) {
			mipc_msg_deinit(msg);
			return -EINVAL;
		}
	}

	send_mipc(hba_ccci_ipc_port_s, msg);
	mipc_msg_deinit(msg);

	return 0;
}

/**
 * @brief      Sends a MIPC resume message to Modem.
 *
 * @param[in]  resume_req  The resume request
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_mipc_resume_msg_to_md(hba_resume_req_struct resume_req)
{
	int i = 0;
	mipc_msg_api_result_enum ret[14] = { MIPC_MSG_API_RESULT_FAIL };
	mipc_msg_t *msg = NULL;

	if (resume_req.default_phone_id == 0) {
		msg = mipc_msg_init(MIPC_SYS_HBA_RESUME_REQ, MIPC_MSG_PS0);
	} else if (resume_req.default_phone_id == 1) {
		msg = mipc_msg_init(MIPC_SYS_HBA_RESUME_REQ, MIPC_MSG_PS1);
	} else {
		return -EINVAL;
	}
	if (!msg)
		return -ENODEV;

	ret[0] = mipc_sys_hba_resume_req_add_proxy_key(msg, strlen(resume_req.proxy_key) + 1, resume_req.proxy_key);

	ret[1] = mipc_sys_hba_resume_req_add_ipv4_tos(msg, resume_req.seq_sync.ipv4_tos);
	ret[2] = mipc_sys_hba_resume_req_add_ipv4_ttl(msg, resume_req.seq_sync.ipv4_ttl);
	ret[3] = mipc_sys_hba_resume_req_add_ipv4_id(msg, resume_req.seq_sync.ipv4_id);
	ret[4] = mipc_sys_hba_resume_req_add_ipv6_flow_label(msg, resume_req.seq_sync.ipv6_flow_label);
	ret[5] = mipc_sys_hba_resume_req_add_ipv6_hop_limit(msg, resume_req.seq_sync.ipv6_hop_limit);
	ret[6] = mipc_sys_hba_resume_req_add_ipv6_tclass(msg, resume_req.seq_sync.ipv6_tclass);

	ret[7] = mipc_sys_hba_resume_req_add_tcp_rcv_wscale(msg, resume_req.seq_sync.tcp_rcv_wscale);
	ret[8] = mipc_sys_hba_resume_req_add_tcp_snd_wscale(msg, resume_req.seq_sync.tcp_snd_wscale);
	ret[9] = mipc_sys_hba_resume_req_add_tcp_wscale_ok(msg, resume_req.seq_sync.tcp_wscale_ok);
	ret[10] = mipc_sys_hba_resume_req_add_tcp_snd_window(msg, resume_req.seq_sync.tcp_snd_wnd);
	ret[11] = mipc_sys_hba_resume_req_add_tcp_rcv_window(msg, resume_req.seq_sync.tcp_rcv_wnd);
	ret[12] = mipc_sys_hba_resume_req_add_tcp_snd_nxt_seq(msg, resume_req.seq_sync.tcp_snd_nxt);
	ret[13] = mipc_sys_hba_resume_req_add_tcp_rcv_nxt_seq(msg, resume_req.seq_sync.tcp_rcv_nxt);

	for (i = 0; i < 14; i++) {
		if (ret[i] != MIPC_MSG_API_RESULT_SUCCESS) {
			mipc_msg_deinit(msg);
			return -EINVAL;
		}
	}

	send_mipc(hba_ccci_ipc_port_s, msg);
	mipc_msg_deinit(msg);

	return 0;
}

/**
 * @brief      Sends a MIPC send now message to Modem.
 *
 * @param[in]  send_now_req  The send now request
 *
 * @return     0 if successful, negative otherwise.
 */
static int send_mipc_send_now_msg_to_md(hba_send_now_req_struct send_now_req)
{
	mipc_msg_api_result_enum ret = MIPC_MSG_API_RESULT_FAIL;
	mipc_msg_t *msg = NULL;

	if (send_now_req.default_phone_id == 0) {
		msg = mipc_msg_init(MIPC_SYS_HBA_SEND_NOW_REQ, MIPC_MSG_PS0);
	} else if (send_now_req.default_phone_id == 1) {
		msg = mipc_msg_init(MIPC_SYS_HBA_SEND_NOW_REQ, MIPC_MSG_PS1);
	} else {
		return -EINVAL;
	}
	if (!msg)
		return -ENODEV;

	ret = mipc_sys_hba_send_now_req_add_proxy_key(msg, strlen(send_now_req.proxy_key) + 1, send_now_req.proxy_key);
	if (ret != MIPC_MSG_API_RESULT_SUCCESS) {
		mipc_msg_deinit(msg);
		return -EINVAL;
	}

	send_mipc(hba_ccci_ipc_port_s, msg);
	mipc_msg_deinit(msg);

	return 0;
}

/**
 * @brief      Put skb in a customized queue.
 *
 * @param      hba   The heartbeat
 * @param      skb   The socket buffer
 *
 * @return     Responses from hook functions.
 */
static unsigned int hba_queue_skb(struct hba_st *hba, struct sk_buff *skb)
{
	int i = 0;
	if (hba->state == HBA_STATE_PAUSE_REQUEST) {
		hba->is_queue_overflow = true;
		for (i = 0; i < HBA_SKB_QUEUE_LEN; i++) {
			if (!hba->skb_queue[i]) {
				hba->skb_queue[i] = skb;
				hba->is_queue_overflow = false;
				break;
			}
		}
		if (hba->is_queue_overflow) {
			printk("[heartbeat_proxy_mtk] hba_pause_and_queue proxy_key=%s skb queue overflow, drop now!!!\n", hba->proxy_key);
			return NF_DROP;
		}
	} else {
		reset_hba_skb_queue(hba, true);
		hba->skb_queue[0] = skb;
	}
	return NF_STOLEN;
}

/**
 * @brief      Heartbeat IPv4 hook functions.
 *
 * @param      priv   The priv
 * @param      skb    The socket buffer
 * @param[in]  state  The state
 *
 * @return     Responses from hook functions.
 */
static unsigned int hba_input_hook_v4(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	int ret = 0;
	int hook_ret = NF_ACCEPT;
	int err = HBA_INT_PROCESS_OK;
	uid_t sk_uid = 0;
	struct net_device *dev = NULL;
	struct sock *sk = NULL;
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_ctrl_req_struct ctrl_req;

	memset(&ctrl_req, 0x0, sizeof(hba_ctrl_req_struct));
	ctrl_req.mode = HBA_CTRL_MODE_PAUSE;

	iph = ip_hdr(skb);
	tcph = tcp_hdr(skb);
	if (skb->protocol != htons(ETH_P_IP) || (!iph))
		return NF_ACCEPT;

	if (iph->protocol != IPPROTO_TCP || !tcph)
		return NF_ACCEPT;

	sk = skb_to_full_sk(skb);
	if (!sk)
		return NF_ACCEPT;

	sk_uid = get_uid_from_sock(sk);
	if (sk_uid == 0)
		return NF_ACCEPT;

	dev = skb->dev;
	if (!dev)
		return NF_ACCEPT;

	if (unlikely(tcph->syn))
		return NF_ACCEPT;

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (hba_node_entry->hba.state != HBA_STATE_ESTABLISH_OK && hba_node_entry->hba.state != HBA_STATE_PAUSE_REQUEST) {
			continue;
		}
		if (hba_node_entry->hba.sk == sk) {
			if (hba_node_entry->hba.ip_local_deliver_finish == NULL) {
				hba_node_entry->hba.ip_local_deliver_finish = state->okfn;
			}
			hook_ret = hba_queue_skb(&hba_node_entry->hba, skb);
			if (hba_node_entry->hba.state == HBA_STATE_ESTABLISH_OK) {
				ret = hba_sync_seq_from_tcphdr(&hba_node_entry->hba, sk, tcph);
				if (ret != 0) {
					if (ret > 0) {
						err = ret;
					} else {
						err = HBA_INT_ERR_SEQ_SYNC_UNKNOWN;
					}
					reset_hba_skb_queue(&hba_node_entry->hba, false);
					goto error_with_unlock;
				}
				safe_strncpy(ctrl_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(ctrl_req.proxy_key));
				ctrl_req.default_phone_id = hba_node_entry->hba.default_phone_id;
				hba_node_entry->hba.state = HBA_STATE_PAUSE_REQUEST;
				hba_node_entry->hba.need_sync = false;

				ret = send_mipc_ctrl_msg_to_md(ctrl_req);
				if (ret < 0) {
					printk("[heartbeat_proxy_mtk] input_hook_v4 pause send md failed ret=%d.\n", ret);
					err = HBA_INT_ERR_IPC_MD;
					reset_hba_skb_queue(&hba_node_entry->hba, false);
					goto error_with_unlock;
				}
				else {
					printk("[heartbeat_proxy_mtk] input_hook_v4 pause send md successfully!");
					hba_node_entry->hba.is_pause_timeout = false;
					mod_timer(&hba_node_entry->hba.pause_timer, jiffies + msecs_to_jiffies(1000));
				}
			}
			spin_unlock_bh(&hba_list_lock);
			return hook_ret;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	return NF_ACCEPT;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_msg_to_hba_user(NETLINK_UNSOL_HBA_PAUSE, ctrl_req.proxy_key, err, 0, NULL);
	return NF_DROP;
}

/**
 * @brief      Heartbeat IPv6 hook functions.
 *
 * @param      priv   The priv
 * @param      skb    The socket buffer
 * @param[in]  state  The state
 *
 * @return     Responses from hook functions.
 */
static unsigned int hba_input_hook_v6(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	return NF_ACCEPT;
}

/**
 * @brief      Read the state of the sock to fill the TCP sequence number message.
 *
 * @param[in]  sk        The sock
 * @param      seq_sync  The TCP sequence number
 * @param[in]  is_ipv6   Indicates if IPv6
 *
 * @return     0 if successful, negative otherwise.
 */
static int fill_seq_sync_msg(const struct sock *sk, hba_seq_sync_msg_struct *seq_sync, int is_ipv6)
{
	u32 n_snd_win = 1, n_rcv_win = 1;
	struct net *net = NULL;
	struct tcp_sock *tp = NULL;
	struct inet_sock *isk = NULL;
#if defined(CONFIG_IPV6) && defined(HEARTBEAT_MTK_IPV6)
	struct ipv6_pinfo *np = NULL;
#endif

	if (NULL == sk || !sk_fullsock(sk) || NULL == sk->sk_socket) {
		printk("[heartbeat_proxy_mtk] fill_seq_sync_msg lookup sk failed, sk is null!\n");
		return -ENODEV;
	}

	if (sk->sk_protocol != IPPROTO_TCP) {
		printk("[heartbeat_proxy_mtk] fill_seq_sync_msg lookup sk failed, sk protocol=%d not match!\n",
		       sk->sk_protocol);
		return -ENODEV;
	}

	printk("[heartbeat_proxy_mtk] fill_seq_sync_msg lookup successfully!\n");

	if (sk->sk_write_queue.qlen != 0) {
		printk("[heartbeat_proxy_mtk] fill_seq_sync_msg failed, sk write queue=%d exceed 0.\n",
		       sk->sk_write_queue.qlen);
		return -ENODEV;
	}

	tp = tcp_sk(sk);
	n_snd_win = tp->snd_wnd;
	n_rcv_win = tp->rcv_wnd;
	/* Make sure we do not exceed the maximum possible
	 * scaled window.
	 */
	if (!tp->rx_opt.rcv_wscale && sock_net(sk)->ipv4.sysctl_tcp_workaround_signed_windows)
		n_rcv_win = min(n_rcv_win, MAX_TCP_WINDOW);
	else
		n_rcv_win = min(n_rcv_win, (65535U << tp->rx_opt.rcv_wscale));

	/* RFC1323 scaling applied */
	n_rcv_win >>= tp->rx_opt.rcv_wscale;
	n_snd_win >>= tp->rx_opt.snd_wscale;

	printk("[heartbeat_proxy_mtk] fill_seq_sync_msg snd_nxt=%u, rcv_nxt=%u, snd_wnd=%u, rcv_wnd=%u.\n",
	       tp->snd_nxt, tp->rcv_nxt, tp->snd_wnd, tp->rcv_wnd);
	seq_sync->tcp_snd_nxt = tp->snd_nxt;
	seq_sync->tcp_rcv_nxt = tp->rcv_nxt;

	printk("[heartbeat_proxy_mtk] fill_seq_sync_msg n_snd_win=%u, n_rcv_win=%u.\n", n_snd_win, n_rcv_win);
	seq_sync->tcp_snd_wnd = n_snd_win;
	seq_sync->tcp_rcv_wnd = n_rcv_win;

	printk("[heartbeat_proxy_mtk] fill_seq_sync_msg rcv_wscale=%u, snd_wscale=%u, wscale_ok=%u.\n",
	       tp->rx_opt.rcv_wscale, tp->rx_opt.snd_wscale, tp->rx_opt.wscale_ok);
	seq_sync->tcp_rcv_wscale = tp->rx_opt.rcv_wscale;
	seq_sync->tcp_snd_wscale = tp->rx_opt.snd_wscale;
	seq_sync->tcp_wscale_ok = tp->rx_opt.wscale_ok;

	if (is_ipv6 != 1) {
		isk = inet_sk(sk);
		net = sock_net(sk);
		seq_sync->ipv4_tos = isk->tos;
		if (isk->uc_ttl == -1) {
			seq_sync->ipv4_ttl = net->ipv4.sysctl_ip_default_ttl;
		}
		else {
			seq_sync->ipv4_ttl = isk->uc_ttl;
		}
		seq_sync->ipv4_id = isk->inet_id;
		printk("[heartbeat_proxy_mtk] fill_seq_sync_msg tos=%u, id=%u, ttl=%d.\n",
		       isk->tos, isk->inet_id, seq_sync->ipv4_ttl);
	}
	else {
#if defined(CONFIG_IPV6) && defined(HEARTBEAT_MTK_IPV6)
		np = inet6_sk(sk);
		if (np) {
			printk("[heartbeat_proxy_mtk] fill_seq_sync_msg hop_limit=%u, tclass=%u, flow_label=%u.\n",
			       np->hop_limit, np->tclass, np->flow_label);
			seq_sync->ipv6_hop_limit = np->hop_limit;
			seq_sync->ipv6_tclass = np->tclass;
			seq_sync->ipv6_flow_label = np->flow_label;
		}
		else {
			printk("[heartbeat_proxy_mtk] fill_seq_sync_msg failed, inet6_sk is null!");
			return -ENODEV;
		}
#else
		printk("[heartbeat_proxy_mtk] fill_seq_sync_msg failed, ipv6 not support!");
		return -ENODEV;
#endif
	}

	return 0;
}

/**
 * @brief      The handler of heartbeat establish-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_establish(struct nlattr *nla)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	struct sock *sk = NULL;
	struct hba_list_node_st *hba_node = NULL;
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_estblish_req_struct estblish_req;
	netlink_dl_hba_establish_msg_st *netlink_msg = (netlink_dl_hba_establish_msg_st *)NLA_DATA(nla);

	printk("[heartbeat_proxy_mtk] netlink request_hba_establish, proxy_key=%s.\n", netlink_msg->proxy_key);

	if (netlink_msg->is_ipv6 != 1) {
		sk = __inet_lookup_established(&init_net, &tcp_hashinfo, netlink_msg->daddr,
		                               ntohs(netlink_msg->dport), netlink_msg->saddr,
		                               netlink_msg->sport, 0, 0);
	}
	else {
#if defined(CONFIG_IPV6) && defined(HEARTBEAT_MTK_IPV6)
		const struct in6_addr saddr6 =
			{	{	.u6_addr32 = {netlink_msg->v6_saddr32[0], netlink_msg->v6_saddr32[1],
					netlink_msg->v6_saddr32[2], netlink_msg->v6_saddr32[3]
				}
			}
		};
		const struct in6_addr daddr6 =
			{	{	.u6_addr32 = {netlink_msg->v6_daddr32[0], netlink_msg->v6_daddr32[1],
					netlink_msg->v6_daddr32[2], netlink_msg->v6_daddr32[3]
				}
			}
		};
		sk = __inet6_lookup_established(&init_net, &tcp_hashinfo, &daddr6,
		                                ntohs(netlink_msg->dport), &saddr6,
		                                netlink_msg->sport, 0, 0);
#else
		err = HBA_INT_ERR_IPV6_NOT_SUPPORT;
		goto error;
#endif
	}

	memset(&estblish_req, 0x0, sizeof(hba_estblish_req_struct));
	if (fill_seq_sync_msg(sk, &estblish_req.seq_sync, netlink_msg->is_ipv6) < 0) {
		err = HBA_INT_ERR_INVALID_SOCKET;
		goto error;
	}

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (sk == hba_node_entry->hba.sk) {
			printk("[heartbeat_proxy_mtk] request_hba_establish failed, repeated sk!\n");
			err = HBA_INT_ERR_PROXY_ALREADY_EXIST;
			goto error_with_unlock;
		}
	}
	if (hba_list.count > MAX_HBA_LIST_COUNT) {
		printk("[heartbeat_proxy_mtk] request_hba_establish failed, heartbeat list exceed threshold.\n");
		err = HBA_INT_ERR_NUM_ALREADY_MAX;
		goto error_with_unlock;
	}
	hba_node = kzalloc(sizeof(struct hba_list_node_st), GFP_ATOMIC);
	if (!hba_node) {
		printk("[heartbeat_proxy_mtk] request_hba_establish failed, kzalloc heartbeat node failed.\n");
		goto error_with_unlock;
	}
	memset(hba_node->hba.proxy_key, 0x0, sizeof(hba_node->hba.proxy_key));
	safe_strncpy(hba_node->hba.proxy_key, netlink_msg->proxy_key, sizeof(hba_node->hba.proxy_key));
	hba_node->hba.default_phone_id = netlink_msg->default_phone_id;
	hba_node->hba.saddr = netlink_msg->saddr;
	hba_node->hba.daddr = netlink_msg->daddr;
	memcpy(hba_node->hba.v6_saddr32, (uint8_t *)&netlink_msg->v6_saddr32, 4);
	memcpy(hba_node->hba.v6_daddr32, (uint8_t *)&netlink_msg->v6_daddr32, 4);
	hba_node->hba.sport = netlink_msg->sport;
	hba_node->hba.dport = netlink_msg->dport;
	hba_node->hba.sk = sk;
	hba_node->hba.is_ipv6 = netlink_msg->is_ipv6 == 1 ? true : false;
	hba_node->hba.state = HBA_STATE_ESTABLISH_REQUEST;
	timer_setup(&hba_node->hba.pause_timer, hba_pause_timer_handler, 0);
	list_add(&hba_node->list_node, &hba_list.head);
	hba_list.count++;
	spin_unlock_bh(&hba_list_lock);

	estblish_req.netinfo_ctrl.is_ipv6 = netlink_msg->is_ipv6;
	estblish_req.netinfo_ctrl.protocol = IPPROTO_TCP;
	estblish_req.netinfo_ctrl.tcp_retries2 = netlink_msg->tcp_retries2;
	estblish_req.netinfo_ctrl.sport = netlink_msg->sport;
	estblish_req.netinfo_ctrl.dport = netlink_msg->dport;
	if (netlink_msg->is_ipv6 != 1) {
		memcpy(estblish_req.netinfo_ctrl.saddr, (uint8_t *)&netlink_msg->saddr, 4);
		memcpy(estblish_req.netinfo_ctrl.daddr, (uint8_t *)&netlink_msg->daddr, 4);
	}
	else {
		memcpy(estblish_req.netinfo_ctrl.v6_saddr8, (uint8_t *)&netlink_msg->v6_saddr32, 16);
		memcpy(estblish_req.netinfo_ctrl.v6_daddr8, (uint8_t *)&netlink_msg->v6_daddr32, 16);
	}
	memset(estblish_req.proxy_key, 0x0, sizeof(estblish_req.proxy_key));
	safe_strncpy(estblish_req.proxy_key, netlink_msg->proxy_key, sizeof(estblish_req.proxy_key));
	estblish_req.interface_id = netlink_msg->interface_id;
	estblish_req.default_phone_id = netlink_msg->default_phone_id;
	estblish_req.hb_len = netlink_msg->send_payload_length;
	memcpy(estblish_req.hb_pattern, (uint8_t *)&netlink_msg->send_payload, sizeof(estblish_req.hb_pattern));
	estblish_req.hback_len = netlink_msg->reply_payload_length;
	memcpy(estblish_req.hback_pattern, (uint8_t *)&netlink_msg->reply_payload, sizeof(estblish_req.hback_pattern));
	estblish_req.hitchhike_interval = netlink_msg->hitchhike_interval;
	estblish_req.is_allow_dynamic_cycle = netlink_msg->is_allow_dynamic_cycle;
	estblish_req.cycle = netlink_msg->cycle;
	estblish_req.max_cycle = netlink_msg->max_cycle;
	estblish_req.cycle_step = netlink_msg->cycle_step;
	estblish_req.cycle_step_success_num = netlink_msg->cycle_step_success_num;

	if (!hba_mipc_register_ind) {
		ret = hba_ccci_ipc_register_ind(MIPC_SYS_HBA_TIMEOUT_IND);
		if (ret < 0) {
			printk("[heartbeat_proxy_mtk] request_hba_establish failed, register ind=%d, ret=%d.\n", MIPC_SYS_HBA_TIMEOUT_IND, ret);
			err = HBA_INT_ERR_IPC_MD;
			hba_delete_node_lock(hba_node);
			goto error;
		}

		ret = hba_ccci_ipc_register_ind(MIPC_SYS_HBA_HW_FILTER_SRC_STATE_IND);
		if (ret < 0) {
			printk("[heartbeat_proxy_mtk] request_hba_establish failed, register ind=%d, ret=%d.\n", MIPC_SYS_HBA_HW_FILTER_SRC_STATE_IND, ret);
			err = HBA_INT_ERR_IPC_MD;
			hba_delete_node_lock(hba_node);
			goto error;
		}
		printk("[heartbeat_proxy_mtk] request_hba_establish register ind successfully!");
		hba_mipc_register_ind = true;
	}

	ret = send_mipc_estblish_msg_to_md(estblish_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_establish send md failed, ret=%d.\n", ret);
		err = HBA_INT_ERR_IPC_MD;
		hba_delete_node_lock(hba_node);
		goto error;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_establish send md successfully!");
	}
	return 0;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);

error:
	ret = send_msg_to_hba_user(NETLINK_RESPONSE_HBA_ESTABLISH, netlink_msg->proxy_key, err, 0, NULL);
	return -EAGAIN;
}

/**
 * @brief      The handler of heartbeat pause-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_pause(struct nlattr *nla)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	netlink_dl_hba_ctrl_msg_st *netlink_msg = (netlink_dl_hba_ctrl_msg_st *)NLA_DATA(nla);
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_ctrl_req_struct ctrl_req;

	memset(&ctrl_req, 0x0, sizeof(hba_ctrl_req_struct));
	ctrl_req.mode = HBA_CTRL_MODE_PAUSE;

	printk("[heartbeat_proxy_mtk] netlink request_hba_pause, proxy_key=%s.\n", netlink_msg->proxy_key);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            netlink_msg->proxy_key, strlen(netlink_msg->proxy_key)) == 0) {
			memset(ctrl_req.proxy_key, 0x0, sizeof(ctrl_req.proxy_key));
			safe_strncpy(ctrl_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(ctrl_req.proxy_key));
			ctrl_req.default_phone_id = hba_node_entry->hba.default_phone_id;
			if (hba_node_entry->hba.state == HBA_STATE_ESTABLISH_OK) {
				hba_node_entry->hba.state = HBA_STATE_PAUSE_REQUEST;
				hba_node_entry->hba.need_sync = true;
				goto send_md_with_unlock;
			}
			else if (hba_node_entry->hba.state == HBA_STATE_PAUSE_OK) {
				printk("[heartbeat_proxy_mtk] request_hba_pause failed, has been paused!\n");
				goto send_user_with_unlock;
			}
			else {
				printk("[heartbeat_proxy_mtk] request_hba_pause failed, state=%d not match!\n", hba_node_entry->hba.state);
				err = HBA_INT_ERR_STATE_NOT_MATCH;
				goto error_with_unlock;
			}
		}
	}
	printk("[heartbeat_proxy_mtk] request_hba_pause failed, proxy_key not match!\n");
	err = HBA_INT_ERR_PROXY_NOT_EXIST;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);

error:
	send_msg_to_hba_user(NETLINK_RESPONSE_HBA_PAUSE, netlink_msg->proxy_key, err, 0, NULL);
	return -EAGAIN;

send_md_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_mipc_ctrl_msg_to_md(ctrl_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_pause send md failed, ret=%d.\n", ret);
		err = HBA_INT_ERR_IPC_MD;
		goto error;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_pause send md successfully!");
	}
	return 0;

send_user_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	send_msg_to_hba_user(NETLINK_RESPONSE_HBA_PAUSE, netlink_msg->proxy_key, err, 0, NULL);
	return 0;
}

/**
 * @brief      The handler of heartbeat resume-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_resume(struct nlattr *nla)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	netlink_dl_hba_ctrl_msg_st *netlink_msg = (netlink_dl_hba_ctrl_msg_st *)NLA_DATA(nla);
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_resume_req_struct resume_req;

	memset(&resume_req, 0x0, sizeof(hba_resume_req_struct));

	printk("[heartbeat_proxy_mtk] netlink request_hba_resume, proxy_key=%s.\n", netlink_msg->proxy_key);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            netlink_msg->proxy_key, strlen(netlink_msg->proxy_key)) == 0) {
			memset(resume_req.proxy_key, 0x0, sizeof(resume_req.proxy_key));
			safe_strncpy(resume_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(resume_req.proxy_key));
			resume_req.default_phone_id = hba_node_entry->hba.default_phone_id;
			if (hba_node_entry->hba.state == HBA_STATE_PAUSE_OK) {
				hba_node_entry->hba.state = HBA_STATE_RESUME_REQUEST;
				if (fill_seq_sync_msg(hba_node_entry->hba.sk, &resume_req.seq_sync, hba_node_entry->hba.is_ipv6) < 0) {
					err = HBA_INT_ERR_INVALID_SOCKET;
					goto error_with_unlock;
				}
				goto send_md_with_unlock;
			}
			else {
				printk("[heartbeat_proxy_mtk] request_hba_resume failed, state=%d not match!\n", hba_node_entry->hba.state);
				err = HBA_INT_ERR_STATE_NOT_MATCH;
				goto error_with_unlock;
			}
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] request_hba_resume failed, proxy_key not match!\n");
	err = HBA_INT_ERR_PROXY_NOT_EXIST;

error:
	send_msg_to_hba_user(NETLINK_RESPONSE_HBA_RESUME, netlink_msg->proxy_key, err, 0, NULL);
	return -EAGAIN;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	goto error;

send_md_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_mipc_resume_msg_to_md(resume_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_resume send md failed, ret=%d.\n", ret);
		err = HBA_INT_ERR_IPC_MD;
		goto error;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_resume send md successfully!");
	}
	return 0;
}

/**
 * @brief      The handler of heartbeat stop-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_stop(struct nlattr *nla)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	netlink_dl_hba_ctrl_msg_st *netlink_msg = (netlink_dl_hba_ctrl_msg_st *)NLA_DATA(nla);
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_ctrl_req_struct ctrl_req;

	memset(&ctrl_req, 0x0, sizeof(hba_ctrl_req_struct));
	ctrl_req.mode = HBA_CTRL_MODE_STOP;

	printk("[heartbeat_proxy_mtk] netlink request_hba_stop, proxy_key=%s.\n", netlink_msg->proxy_key);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            netlink_msg->proxy_key, strlen(netlink_msg->proxy_key)) == 0) {
			memset(ctrl_req.proxy_key, 0x0, sizeof(ctrl_req.proxy_key));
			safe_strncpy(ctrl_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(ctrl_req.proxy_key));
			ctrl_req.default_phone_id = hba_node_entry->hba.default_phone_id;
			if (hba_node_entry->hba.state == HBA_STATE_ESTABLISH_OK) {
				hba_node_entry->hba.need_sync = true;
			}
			else {
				hba_node_entry->hba.need_sync = false;
			}
			/* send stop request in any state */
			if (hba_node_entry->hba.state != HBA_STATE_STOP_REQUEST) {
				hba_node_entry->hba.state = HBA_STATE_STOP_REQUEST;
				goto send_md_with_unlock;
			}
			else {
				printk("[heartbeat_proxy_mtk] request_hba_stop failed, state=%d not match!\n", hba_node_entry->hba.state);
				err = HBA_INT_ERR_STATE_NOT_MATCH;
				goto error_with_unlock;
			}
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] request_hba_stop failed, proxy_key not match!\n");
	err = HBA_INT_ERR_PROXY_NOT_EXIST;

error:
	send_msg_to_hba_user(NETLINK_RESPONSE_HBA_STOP, netlink_msg->proxy_key, err, 0, NULL);
	return -EAGAIN;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	goto error;

send_md_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_mipc_ctrl_msg_to_md(ctrl_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_stop send md failed, ret=%d.\n", ret);
		err = HBA_INT_ERR_IPC_MD;
		goto error;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_stop send md successfully!");
	}
	return 0;
}

/**
 * @brief      The handler of heartbeat send_now-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_send_now(struct nlattr *nla)
{
	int ret = 0;
	int err = HBA_INT_PROCESS_OK;
	netlink_dl_hba_ctrl_msg_st *netlink_msg = (netlink_dl_hba_ctrl_msg_st *)NLA_DATA(nla);
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_send_now_req_struct send_now_req;

	memset(&send_now_req, 0x0, sizeof(hba_send_now_req_struct));

	printk("[heartbeat_proxy_mtk] netlink request_hba_send_now, proxy_key=%s.\n", netlink_msg->proxy_key);

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            netlink_msg->proxy_key, strlen(netlink_msg->proxy_key)) == 0) {
			memset(send_now_req.proxy_key, 0x0, sizeof(send_now_req.proxy_key));
			safe_strncpy(send_now_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(send_now_req.proxy_key));
			send_now_req.default_phone_id = hba_node_entry->hba.default_phone_id;
			if (hba_node_entry->hba.state == HBA_STATE_ESTABLISH_OK) {
				goto send_md_with_unlock;
			}
			else {
				printk("[heartbeat_proxy_mtk] request_hba_send_now failed, state=%d not match!\n", hba_node_entry->hba.state);
				err = HBA_INT_ERR_STATE_NOT_MATCH;
				goto error_with_unlock;
			}
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] request_hba_send_now failed, proxy_key not match!\n");
	err = HBA_INT_ERR_PROXY_NOT_EXIST;

error:
	send_msg_to_hba_user(NETLINK_RESPONSE_HBA_SEND_NOW, netlink_msg->proxy_key, err, 0, NULL);
	return -EAGAIN;

error_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	goto error;

send_md_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_mipc_send_now_msg_to_md(send_now_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_send_now send md failed, ret=%d.\n", ret);
		err = HBA_INT_ERR_IPC_MD;
		goto error;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_send_now send md successfully!");
	}
	return 0;
}

/**
 * @brief      The handler of heartbeat delete-request from user space.
 *
 * @param      nla   The nla
 *
 * @return     0 if successful, negative otherwise.
 */
static int request_hba_delete(struct nlattr *nla)
{
	int ret = 0;
	netlink_dl_hba_ctrl_msg_st *netlink_msg = (netlink_dl_hba_ctrl_msg_st *)NLA_DATA(nla);
	struct hba_list_node_st *hba_node_entry = NULL, *n = NULL;
	hba_ctrl_req_struct ctrl_req;

	memset(&ctrl_req, 0x0, sizeof(hba_ctrl_req_struct));
	ctrl_req.mode = HBA_CTRL_MODE_STOP;

	spin_lock_bh(&hba_list_lock);
	list_for_each_entry_safe(hba_node_entry, n, &hba_list.head, list_node) {
		if (strncmp(hba_node_entry->hba.proxy_key,
		            netlink_msg->proxy_key, strlen(netlink_msg->proxy_key)) == 0) {
			memset(ctrl_req.proxy_key, 0x0, sizeof(ctrl_req.proxy_key));
			safe_strncpy(ctrl_req.proxy_key, hba_node_entry->hba.proxy_key, sizeof(ctrl_req.proxy_key));
			ctrl_req.default_phone_id = hba_node_entry->hba.default_phone_id;
			hba_node_entry->hba.state = HBA_STATE_NONE;
			list_del(&hba_node_entry->list_node);
			kfree(hba_node_entry);
			hba_list.count--;
			printk("[heartbeat_proxy_mtk] netlink request_hba_delete proxy_key=%s delete successfully!",
			       ctrl_req.proxy_key);
			/* send stop request in any state */
			goto send_md_with_unlock;
		}
	}
	spin_unlock_bh(&hba_list_lock);

	printk("[heartbeat_proxy_mtk] request_hba_delete failed, proxy_key not match!\n");
	return -EAGAIN;

send_md_with_unlock:
	spin_unlock_bh(&hba_list_lock);
	ret = send_mipc_ctrl_msg_to_md(ctrl_req);
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] request_hba_stop(delete) send md failed, ret=%d.\n", ret);
		return -EAGAIN;
	}
	else {
		printk("[heartbeat_proxy_mtk] request_hba_stop(delete) send md successfully!");
	}
	return 0;
}

/**
 * @brief      Reset heartbeat netlink skb queue.
 */
static void reset_hba_netlink_skb_queue(void)
{
	int i = 0;
	spin_lock_bh(&netlink_list_lock);
	for (i = 0; i < HBA_NETLINK_SKB_QUEUE_LEN; i++) {
		if (hba_netlink_skb_queue[i]) {
			kfree_skb(hba_netlink_skb_queue[i]);
		}
		hba_netlink_skb_queue[i] = NULL;
	}
	spin_unlock_bh(&netlink_list_lock);
}

/**
 * @brief      The asyn handler of heartbeat netlink message from user space.
 *
 * @param      work  The work
 */
static void hba_netlink_nlmsg_handle_asyn(struct work_struct *work)
{
	int i = 0;
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;
	struct sk_buff *netlink_queue[HBA_NETLINK_SKB_QUEUE_LEN];

	spin_lock_bh(&netlink_list_lock);
	for (i = 0; i < HBA_NETLINK_SKB_QUEUE_LEN; i++) {
		netlink_queue[i] = NULL;
		if (hba_netlink_skb_queue[i]) {
			netlink_queue[i] = hba_netlink_skb_queue[i];
		}
		hba_netlink_skb_queue[i] = NULL;
	}
	spin_unlock_bh(&netlink_list_lock);

	for (i = 0; i < HBA_NETLINK_SKB_QUEUE_LEN; i++) {
		if (!netlink_queue[i]) {
			continue;
		}
		nlhdr = nlmsg_hdr(netlink_queue[i]);
		genlhdr = nlmsg_data(nlhdr);
		nla = genlmsg_data(genlhdr);

		printk("[heartbeat_proxy_mtk] hba_netlink_nlmsg_handle_asyn index=%d type=%d.\n", i, nla->nla_type);
		switch (nla->nla_type) {
		case NETLINK_REQUEST_HBA_ESTABLISH:
			request_hba_establish(nla);
			break;
		case NETLINK_REQUEST_HBA_PAUSE:
			request_hba_pause(nla);
			break;
		case NETLINK_REQUEST_HBA_RESUME:
			request_hba_resume(nla);
			break;
		case NETLINK_REQUEST_HBA_STOP:
			request_hba_stop(nla);
			break;
		case NETLINK_REQUEST_HBA_SEND_NOW:
			request_hba_send_now(nla);
			break;
		case NETLINK_REQUEST_HBA_DELETE:
			request_hba_delete(nla);
			break;
		default:
			printk("[heartbeat_proxy_mtk] hba_netlink_nlmsg_handle_asyn failed, unknown nla type=%d.\n", nla->nla_type);
			break;
		}
		kfree_skb(netlink_queue[i]);
		netlink_queue[i] = NULL;
	}
}

/**
 * @brief      The handler of heartbeat netlink message from user space.
 *
 * @param      skb   The socket buffer
 * @param      info  The information
 *
 * @return     0 if successful, negative otherwise.
 */
int hba_netlink_nlmsg_handle(struct sk_buff *skb, struct genl_info *info)
{
	int i = 0;
	bool overflow = true;
	struct sk_buff *copy;

	copy = skb_copy(skb, GFP_ATOMIC);
	if (!copy) {
		return -ENOMEM;
	}
	spin_lock_bh(&netlink_list_lock);
	for (i = 0; i < HBA_NETLINK_SKB_QUEUE_LEN; i++) {
		if (!hba_netlink_skb_queue[i]) {
			hba_netlink_skb_queue[i] = copy;
			overflow = false;
			break;
		}
	}
	spin_unlock_bh(&netlink_list_lock);
	if (!overflow) {
		schedule_work(&hba_netlink_handle_work);
	} else {
		printk("[heartbeat_proxy_mtk] schedule netlink handle work failed, skb queue overflow!\n");
		kfree_skb(copy);
	}

	return 0;
}

/**
 * @brief      Initialize MediaTek heartbeat proxy driver.
 *
 * @return     0 if successful, negative otherwise.
 */
int hba_proxy_mtk_init(void)
{
	int ret = 0;

	reset_hba_netlink_skb_queue();
	INIT_LIST_HEAD(&hba_list.head);

	ret = hba_ccci_ipc_open_port();
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] module failed to init ccci.\n");
		return ret;
	}

	ret = hba_netfilter_v4_init();
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] module failed to init netfilter_v4.\n");
		hba_ccci_ipc_close_port();
		return ret;
	}

	ret = hba_netfilter_v6_init();
	if (ret < 0) {
		printk("[heartbeat_proxy_mtk] module failed to init netfilter_v6.\n");
		hba_ccci_ipc_close_port();
		nf_unregister_net_hooks(&init_net, hba_input_v4_ops, ARRAY_SIZE(hba_input_v4_ops));
		return ret;
	}

	printk("[heartbeat_proxy_mtk] module init successfully!");

	return ret;
}

/**
 * @brief      Uninstall MediaTek heartbeat proxy driver.
 */
void hba_proxy_mtk_fini(void)
{
	nf_unregister_net_hooks(&init_net, hba_input_v4_ops, ARRAY_SIZE(hba_input_v4_ops));
	nf_unregister_net_hooks(&init_net, hba_input_v6_ops, ARRAY_SIZE(hba_input_v6_ops));
	hba_ccci_ipc_close_port();
	reset_hba_netlink_skb_queue();
}
