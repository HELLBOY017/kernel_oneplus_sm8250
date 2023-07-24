/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: wzry_stats.c
** Description: wangzherongyao game kernel stats
**
** Version: 1.0
** Date : 2022/7/7
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/7/7 1.0 build this module
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
#include <net/inet_connection_sock.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/route.h>
#include <net/tcp.h>
#include <net/tcp_states.h>
#include <net/udp.h>
#include <linux/netfilter_ipv6.h>
#include <linux/timekeeping.h>
#include <trace/events/net.h>
#include <linux/kprobes.h>



#include "../include/ring_data.h"
#include "../include/comm_def.h"
#include "../include/netlink_api.h"
#include "../include/dpi_api.h"

#include "wzry_stats.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
#define USE_TRACE_FUNC
#endif


#define LOG_TAG "GAME_STATS"

static int s_debug = 0;


#define logt(fmt, args...) LOG(LOG_TAG, fmt, ##args)
#define logi(fmt, args...) do { if (s_debug) LOG(LOG_TAG, fmt, ##args); } while (0)


#define SKB_MARK_MASK_UL 0x80000000
#define SKB_MARK_MASK_DL 0x00100000


#define MAX_SAVE_ITEM_COUNT 10

static spinlock_t s_wzry_lock;
static wzry_delay_item s_delay_array[MAX_SAVE_ITEM_COUNT];
static RING_DATA_STRUCT(s_delay_ring, s_delay_array, sizeof(wzry_delay_item), ARRAY_SIZE(s_delay_array));

static wzry_delay_save s_delay_current;
static int s_up_calc = 0;
static int s_down_calc = 0;
static int s_get_intvl = 1; /* unit:s */
static struct timer_list s_get_delay_timer;


static void dump_buf(char *buf, u32 len) __attribute__((unused));

static void dump_buf(char *buf, u32 len)
{
	int i = 0;
	char dumpbuf[128] = {0};
	int dump_len = 0;

	for (i = 0; i < len; i++) {
		dump_len += sprintf(dumpbuf + dump_len, "%d,", buf[i]);
		if (((i+1) % 16) == 0) {
			printk("%s\n", dumpbuf);
			memset(dumpbuf, 0, sizeof(dumpbuf));
			dump_len = 0;
		}
	}
	if (dump_len != 0) {
		printk("%s\n", dumpbuf);
	}
}

static u64 get_time_ts64_ns_boot(void)
{
	struct timespec64 ts;
	u64 result = 0;

	ktime_get_ts64(&ts);
	result = ts.tv_sec * 1000000000 + ts.tv_nsec;

	return result;
}

static u64 get_time_ts64_ns(void)
{
#ifdef CONFIG_ANDROID_KABI_RESERVE
	return ktime_get_real();
#else
	return get_time_ts64_ns_boot();
#endif
}

static void save_top_count_value(u64 *data, u64 delay, int size)
{
	int i = 0, j = 0;
	for (i = (size - 1); i >= 0; i--) {
		if (data[i] < delay) {
			continue;
		} else {
			break;
		}
	}
	if (i < (size - 2)) {
		for (j = (size - 1); j >= (i + 2); j--) {
			data[j] = data[j - 1];
		}
	}
	if (i < (size - 1)) {
		data[i + 1] = delay;
	}
}


static int check_wzry_data_skb(struct sk_buff * skb, int dir)
{
	u64 dpi_result = get_skb_dpi_id(skb, dir, 0);

	return (dpi_result == DPI_ID_TMGP_SGAME_STREAM_GAME_DATA) ? 1 : 0;
}

#ifdef CONFIG_ANDROID_KABI_RESERVE
static void wzry_record_delay(struct sk_buff *skb, enum wzry_event_type type)
{
	u64 cur_time = get_time_ts64_ns();
	wzry_delay_save *save = &s_delay_current;

	switch (type) {
	case WZRY_ENTRY_RX_IP:
		skb->android_kabi_reserved1 = 0;
		if (!s_down_calc) {
			s_down_calc = 1;
			save->rx_dev_time = skb_get_ktime(skb);
			save->rx_ip_time = cur_time;
			skb->android_kabi_reserved1 = 1;
			logi("WZRY_ENTRY_RX_IP enter 2 %llu,%llu,%llu", save->rx_dev_time, save->rx_ip_time, cur_time);
		}
		break;
	case WZRY_ENTRY_RX_UDP:
		if (skb->android_kabi_reserved1) {
			save->rx_udp_time = cur_time;
			logi("WZRY_ENTRY_RX_UDP enter 2 %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_RX_USER:
		if (skb->android_kabi_reserved1) {
			save->rx_user_time = cur_time;
			logi("WZRY_ENTRY_RX_USER enter 2 %llu", cur_time);
		}
		save->down_packets += 1;
		if (save->last_down_time != 0) {
			u64 delay = cur_time - save->last_down_time;
			save_top_count_value(save->down_packet_delay, delay, ARRAY_SIZE(save->down_packet_delay));
		}
		save->last_down_time = cur_time;
		break;
	case WZRY_ENTRY_TX_USER:
		skb->android_kabi_reserved1 = 0;
		if (!s_up_calc) {
			s_up_calc = 1;
			save->tx_user_time = cur_time;
			skb->android_kabi_reserved1 = 1;
			logi("WZRY_ENTRY_TX_USER enter 2 %llu", cur_time);
		}
		save->up_packets += 1;
		break;
	case WZRY_ENTRY_TX_UDP:
		if (skb->android_kabi_reserved1) {
			save->tx_udp_time = cur_time;
			logi("WZRY_ENTRY_TX_UDP enter 2 %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_TX_IP:
		if (skb->android_kabi_reserved1) {
			save->tx_ip_time = cur_time;
			logi("WZRY_ENTRY_TX_IP enter 2 %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_TX_DEV:
		if (skb->android_kabi_reserved1) {
			save->tx_dev_time = cur_time;
			logi("WZRY_ENTRY_TX_DEV enter 2 %llu", cur_time);
		}
		break;
	default:
		logt("unknown wzry entry type %d", type);
		break;
	}
}

#else

static void wzry_record_delay(struct sk_buff *skb, enum wzry_event_type type)
{
	u64 cur_time = get_time_ts64_ns();
	wzry_delay_save *save = &s_delay_current;

	switch (type) {
	case WZRY_ENTRY_RX_DEV:
		if (!s_down_calc) {
			s_down_calc = 1;
			save->rx_dev_time = cur_time;
			skb->mark |= SKB_MARK_MASK_DL;
			logi("WZRY_ENTRY_RX_DEV enter %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_RX_IP:
		if (skb->mark & SKB_MARK_MASK_DL) {
			save->rx_ip_time = cur_time;
			logi("WZRY_ENTRY_RX_IP enter %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_RX_UDP:
		if (skb->mark & SKB_MARK_MASK_DL) {
			save->rx_udp_time = cur_time;
			logi("WZRY_ENTRY_RX_UDP enter %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_RX_USER:
		if (skb->mark & SKB_MARK_MASK_DL) {
			save->rx_user_time = cur_time;
			logi("WZRY_ENTRY_RX_USER enter %llu", cur_time);
		}
		save->down_packets += 1;
		if (save->last_down_time != 0) {
			u64 delay = cur_time - save->last_down_time;
			save_top_count_value(save->down_packet_delay, delay, ARRAY_SIZE(save->down_packet_delay));
		}
		save->last_down_time = cur_time;
		break;
	case WZRY_ENTRY_TX_USER:
		if (!s_up_calc) {
			s_up_calc = 1;
			save->tx_user_time = cur_time;
			skb->mark |= SKB_MARK_MASK_UL;
			logi("WZRY_ENTRY_TX_USER enter %llu", cur_time);
		}
		save->up_packets += 1;
		break;
	case WZRY_ENTRY_TX_UDP:
		if (skb->mark & SKB_MARK_MASK_UL) {
			save->tx_udp_time = cur_time;
			logi("WZRY_ENTRY_TX_UDP enter %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_TX_IP:
		if (skb->mark & SKB_MARK_MASK_UL) {
			save->tx_ip_time = cur_time;
			logi("WZRY_ENTRY_TX_IP enter %llu", cur_time);
		}
		break;
	case WZRY_ENTRY_TX_DEV:
		if (skb->mark & SKB_MARK_MASK_UL) {
			save->tx_dev_time = cur_time;
			logi("WZRY_ENTRY_TX_DEV enter %llu", cur_time);
		}
		break;
	default:
		logt("unknown wzry entry type %d", type);
		break;
	}
}
#endif



static void wzry_delay_entry(struct sk_buff *skb, enum wzry_event_type type)
{
	int dir = (type >= WZRY_ENTRY_TX_USER) ? 1 : 0;

	if (check_wzry_data_skb(skb, dir)) {
		spin_lock_bh(&s_wzry_lock);
		wzry_record_delay(skb, type);
		spin_unlock_bh(&s_wzry_lock);
	}
}

static unsigned int wzry_prerouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	wzry_delay_entry(skb, WZRY_ENTRY_RX_IP);
	return NF_ACCEPT;
}

static unsigned int wzry_input_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	wzry_delay_entry(skb, WZRY_ENTRY_RX_UDP);
	return NF_ACCEPT;
}

static unsigned int wzry_output_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	wzry_delay_entry(skb, WZRY_ENTRY_TX_USER);
	return NF_ACCEPT;
}

static unsigned int wzry_postrouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	wzry_delay_entry(skb, WZRY_ENTRY_TX_UDP);
	return NF_ACCEPT;
}



static struct nf_hook_ops wzry_netfilter_ops[] __read_mostly = {
	{
		.hook = wzry_prerouting_hook,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_PRE_ROUTING,
		.priority = NF_IP_PRI_FILTER + 1,
	},
	{
		.hook = wzry_input_hook,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_FILTER + 1,
	},
	{
		.hook = wzry_output_hook,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FILTER + 1,
	},
	{
		.hook = wzry_postrouting_hook,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_POST_ROUTING,
		.priority = NF_IP_PRI_FILTER + 1,
	},
};

#ifdef USE_TRACE_FUNC
static void probe_netif_rx(void *data, struct sk_buff *skb)
{
#ifndef CONFIG_ANDROID_KABI_RESERVE
	wzry_delay_entry(skb, WZRY_ENTRY_RX_DEV);
#endif
}

static void probe_net_dev_queue(void *data, struct sk_buff *skb)
{
	wzry_delay_entry(skb, WZRY_ENTRY_TX_IP);
}

#endif

#ifdef USE_TRACE_FUNC
static int __skb_recv_udp_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	unsigned long ret = regs_return_value(regs);
	struct sk_buff *skb = (struct sk_buff *)ret;

	if (skb == NULL) {
		return 0;
	}

	wzry_delay_entry(skb, WZRY_ENTRY_RX_USER);
	return 0;
}

static int xmit_one_pre_handler(struct kprobe *kp, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff *)regs->regs[0];

	if (skb == NULL) {
		return 0;
	}

	wzry_delay_entry(skb, WZRY_ENTRY_TX_DEV);
	return 0;
}


static struct kretprobe kretp__skb_recv_udp = {
	.kp.symbol_name = "__skb_recv_udp",
	.handler = __skb_recv_udp_ret_handler,
};

static struct kprobe kp_xmit_one = {
	.symbol_name = "xmit_one",
	.pre_handler = xmit_one_pre_handler,
};
#endif

static int probe_func_init(void)
{
#ifdef USE_TRACE_FUNC
	int ret = 0;

	ret = register_trace_netif_rx(probe_netif_rx, NULL);
	logt("register_trace_netif_rx_entry return %d", ret);
	if (ret) {
		return -1;
	}

	ret = register_kretprobe(&kretp__skb_recv_udp);
	if (ret < 0) {
		logt("register_kretprobe register kretp__skb_recv_udp failed with %d", ret);
		goto kprobe_recv_udp_failed;
	}

	ret = register_trace_net_dev_queue(probe_net_dev_queue, NULL);
	logt("register_trace_net_dev_queue return %d", ret);
	if (ret) {
		goto net_dev_queue_fail;
	}

	ret = register_kprobe(&kp_xmit_one);
	if (ret < 0) {
		logt("register_kprobe register kp_xmit_one failed with %d", ret);
		goto kprobe_dev_start_xmit_failed;
	}

	return 0;

kprobe_recv_udp_failed:
	unregister_trace_netif_rx(probe_netif_rx, NULL);
net_dev_queue_fail:
	unregister_kretprobe(&kretp__skb_recv_udp);
kprobe_dev_start_xmit_failed:
	unregister_trace_net_dev_queue(probe_net_dev_queue, NULL);

	return -1;
#endif
	return 0;
}


static void probe_func_deinit(void)
{
#ifdef USE_TRACE_FUNC
	int ret = 0;

	ret = unregister_trace_netif_rx(probe_netif_rx, NULL);
	logt("unregister_trace_netif_rx_entry return %d", ret);

	unregister_kretprobe(&kretp__skb_recv_udp);
	logt("unregister_kretprobe kretp__skb_recv_udp return %d", 0);

	ret = unregister_trace_net_dev_queue(probe_net_dev_queue, NULL);
	logt("unregister_trace_net_dev_queue return %d", ret);

	unregister_kprobe(&kp_xmit_one);
	logt("unregister_kprobe kp_xmit_one return %d", 0);

#endif
}




static struct ctl_table_header *s_wzry_table_header = NULL;

static struct ctl_table s_wzry_sysctl_table[] __read_mostly = {
	{
		.procname = "debug",
		.data = &s_debug,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "delay_intvl",
		.data = &s_get_intvl,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_up_calc",
		.data = &s_up_calc,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "s_down_calc",
		.data = &s_down_calc,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{}
};


static void gameDelayFree(Netlink__Proto__ResponseGetGameDelay *delay)
{
	int i = 0;

	if (!delay) {
		return;
	}

	for (i = 0; i < delay->n_stats; i++) {
		if (delay->stats[i]->down_delay_max) {
			kfree(delay->stats[i]->down_delay_max);
			delay->stats[i]->down_delay_max = NULL;
		}
	}
	if (delay->stats) {
		kfree(delay->stats);
	}
}


static int wzry_request_get_battle_delay(u32 eventid, Netlink__Proto__RequestMessage *requestMsg, char **rsp_data, u32 *rsp_len)
{
	size_t out_buf_len = 0, pack_len = 0;
	char *out_buf = NULL;
	wzry_delay_item *copy_delay_arry = NULL;
	int copy_count = 0;
	Netlink__Proto__ResponseGetGameDelay getGameDelay = NETLINK__PROTO__RESPONSE_GET_GAME_DELAY__INIT;

	NETLINK_RSP_DATA_DECLARE(rsp_name, requestMsg->header->requestid, requestMsg->header->eventid, COMM_NETLINK_SUCC);
	logt("wzry_request_get_battle_delay");

	copy_delay_arry = (wzry_delay_item *)kmalloc(sizeof(wzry_delay_item) * MAX_SAVE_ITEM_COUNT, GFP_ATOMIC);
	if (copy_delay_arry == NULL) {
		logt("malloc size %lu failed!", sizeof(wzry_delay_item) * MAX_SAVE_ITEM_COUNT);
		return -1;
	}
	memset(copy_delay_arry, 0, sizeof(wzry_delay_item) * MAX_SAVE_ITEM_COUNT);
	logi("wzry_request_get_battle_delay memset data");

	spin_lock_bh(&s_wzry_lock);
	copy_count = ring_copy_by_order(copy_delay_arry, &s_delay_ring);
	spin_unlock_bh(&s_wzry_lock);
	logi("wzry_request_get_battle_delay copy data copy count=%d", copy_count);
	if (copy_count != 0) {
		int i = 0;
		size_t malloc_size = copy_count * (sizeof(Netlink__Proto__GameDelayStats *) + sizeof(Netlink__Proto__GameDelayStats));
		getGameDelay.n_stats = copy_count;
		getGameDelay.stats = (Netlink__Proto__GameDelayStats **)kmalloc(malloc_size, GFP_ATOMIC);
		if (getGameDelay.stats == NULL) {
			kfree(copy_delay_arry);
			logt("malloc data failed! %lu", malloc_size);
			return COMM_NETLINK_ERR_MEMORY;
		}
		logi("wzry_request_get_battle_delay after malloc");
		memset((char *)getGameDelay.stats, 0, malloc_size);
		for (i = 0; i < copy_count; i++) {
			Netlink__Proto__GameDelayStats *stats = (Netlink__Proto__GameDelayStats *)((char *)getGameDelay.stats
				+ copy_count * sizeof(Netlink__Proto__GameDelayStats *) + i * sizeof(Netlink__Proto__GameDelayStats));
			getGameDelay.stats[i] = stats;
			netlink__proto__game_delay_stats__init((ProtobufCMessage *)stats);
			stats->timestamp = copy_delay_arry[i].timestamp;
			stats->rx_ip_delay = copy_delay_arry[i].rx_ip_delay;
			stats->rx_udp_delay = copy_delay_arry[i].rx_udp_delay;
			stats->rx_user_delay = copy_delay_arry[i].rx_user_delay;
			stats->tx_udp_delay = copy_delay_arry[i].tx_udp_delay;
			stats->tx_ip_delay = copy_delay_arry[i].tx_ip_delay;
			stats->tx_sched_delay = copy_delay_arry[i].tx_sched_delay;
			stats->down_packets = copy_delay_arry[i].down_packets;
			stats->up_packets = copy_delay_arry[i].up_packets;
			stats->n_down_delay_max = ARRAY_SIZE(copy_delay_arry[i].down_packet_delay);
			stats->down_delay_max = kmalloc(stats->n_down_delay_max * sizeof(u64), GFP_ATOMIC);
			if (stats->down_delay_max == NULL) {
				logt("malloc stats->down_delay_max failed! %lu", stats->n_down_delay_max * sizeof(u64));
				getGameDelay.n_stats = i;
				gameDelayFree(&getGameDelay);
				kfree(copy_delay_arry);
				return COMM_NETLINK_ERR_MEMORY;
			}
			memcpy(stats->down_delay_max, copy_delay_arry[i].down_packet_delay, stats->n_down_delay_max * sizeof(u64));
			logi("after getGameDelay copy  count=%d", i);
		}
	}
	kfree(copy_delay_arry);

	rsp_name.response_data_case = NETLINK__PROTO__RESPONSE_MESSAGE__RESPONSE_DATA_RSP_GAME_DELAY;
	rsp_name.rspgamedelay = &getGameDelay;
	out_buf_len = netlink__proto__response_message__get_packed_size(&rsp_name);
	out_buf = kmalloc(out_buf_len, GFP_ATOMIC);
	if (!out_buf) {
		logt("malloc speed out buf failed!");
		gameDelayFree(&getGameDelay);
		return COMM_NETLINK_ERR_MEMORY;
	}

	pack_len = netlink__proto__response_message__pack(&rsp_name, out_buf);
	logt("get_dpi_stream_speed_uid_request pack len %lu, buf len %lu", pack_len, out_buf_len);

	gameDelayFree(&getGameDelay);
	*rsp_data = out_buf;
	*rsp_len = out_buf_len;

	return 0;
}

static void wzry_netlink_free(void *data)
{
	if (data) {
		kfree(data);
	}
}


static void reset_save_data(wzry_delay_save *save)
{
	memset(save, 0, sizeof(wzry_delay_save));
}

static void wzry_stats_get_test(void) __attribute__((unused));

static void wzry_stats_get_test(void)
{
	int ret = 0;
	char *buf = NULL;
	u32 rsp_len = 0;
	Netlink__Proto__RequestMessage request = NETLINK__PROTO__REQUEST_MESSAGE__INIT;
	Netlink__Proto__MessageHeader header = NETLINK__PROTO__MESSAGE_HEADER__INIT;

	header.eventid = 99;
	header.requestid = 165;
	request.header = &header;

	ret = wzry_request_get_battle_delay(0, &request, &buf, &rsp_len);
	if (ret) {
		logt("wzry_request_get_battle_delay failed!");
		return;
	}

	logt("wzry_stats_get_test rsp:");
	dump_buf(buf, rsp_len);
	kfree(buf);
}


static void wzry_delay_timer_function(struct timer_list *t)
{
	wzry_delay_save *save = NULL;
	wzry_delay_item *item = NULL;

	spin_lock_bh(&s_wzry_lock);
	save = &s_delay_current;
	if ((s_up_calc != 1) || (s_down_calc != 1)) {
		logt("s_up_calc or s_down_calc is not 0, %d-%d", s_up_calc, s_down_calc);
	}
	/* copy data; */
	item = ring_get_next_item(&s_delay_ring);
	item->timestamp = get_time_ts64_ns_boot();
	item->rx_ip_delay = save->rx_ip_time - save->rx_dev_time;
	item->rx_udp_delay = save->rx_udp_time - save->rx_ip_time;
	item->rx_user_delay = save->rx_user_time - save->rx_udp_time;
	item->tx_udp_delay = save->tx_udp_time - save->tx_user_time;
	item->tx_ip_delay = save->tx_ip_time - save->tx_udp_time;
	item->tx_sched_delay = save->tx_dev_time - save->tx_ip_time;
	item->up_packets = save->up_packets;
	item->down_packets = save->down_packets;
	memcpy(item->down_packet_delay, save->down_packet_delay, sizeof(save->down_packet_delay));
	logi("copy item data %llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u", item->timestamp, item->rx_ip_delay, item->rx_udp_delay, item->rx_user_delay,
		item->tx_udp_delay, item->tx_ip_delay, item->tx_sched_delay, item->up_packets, item->down_packets);

	/* set value */
	reset_save_data(&s_delay_current);
	s_up_calc = 0;
	s_down_calc = 0;
	spin_unlock_bh(&s_wzry_lock);
	/* wzry_stats_get_test(); */
	mod_timer(&s_get_delay_timer, jiffies + s_get_intvl * HZ);
}


static int wzry_action_event(u64 dpi_id, int startStop)
{
	logt("wzry_action_event %llx, %d", dpi_id, startStop);
	if (startStop) {
		mod_timer(&s_get_delay_timer, jiffies + s_get_intvl * HZ);
	} else {
		del_timer_sync(&s_get_delay_timer);
	}
	return 0;
}


int wzry_stats_init(void)
{
	int ret = 0;

	spin_lock_init(&s_wzry_lock);
	memset(&s_delay_array, 0, sizeof(s_delay_array));
	reset_save_data(&s_delay_current);

	ret = nf_register_net_hooks(&init_net, wzry_netfilter_ops, ARRAY_SIZE(wzry_netfilter_ops));
	if (ret) {
		logt("nf_register_net_hooks failed! return %d", ret);
		return -1;
	}

	s_wzry_table_header = register_net_sysctl(&init_net, "net/tmgp_sgame", s_wzry_sysctl_table);
	logt("register_net_sysctl return %p", s_wzry_table_header);
	if (!s_wzry_table_header) {
		goto ctl_failed;
	}
	ret = dpi_register_result_notify(DPI_ID_TMGP_SGAME_STREAM_GAME_DATA, wzry_action_event);
	if (ret) {
		logt("dpi_register_result_notify return %d", ret);
		goto dpi_failed;
	}
	ret = register_netlink_request(COMM_NETLINK_EVENT_GET_WZRY_DELAY, wzry_request_get_battle_delay, wzry_netlink_free);
	if (ret) {
		logt("register cmd COMM_NETLINK_EVENT_GET_WZRY_DELAY failed ");
		goto netlink_failed;
	}

	ret = probe_func_init();
	if (ret) {
		goto probe_fail;
	}

	timer_setup(&s_get_delay_timer, (void *)wzry_delay_timer_function, 0);

	return 0;
probe_fail:
	unregister_netlink_request(COMM_NETLINK_EVENT_GET_WZRY_DELAY);
netlink_failed:
	dpi_unregister_result_notify(DPI_ID_TMGP_SGAME_STREAM_GAME_DATA);
dpi_failed:
	unregister_net_sysctl_table(s_wzry_table_header);
ctl_failed:
	nf_unregister_net_hooks(&init_net, wzry_netfilter_ops, ARRAY_SIZE(wzry_netfilter_ops));
	return -1;
}

void wzry_stats_fini(void)
{
	del_timer_sync(&s_get_delay_timer);
	probe_func_deinit();
	unregister_netlink_request(COMM_NETLINK_EVENT_GET_WZRY_SERVER_IP);
	dpi_unregister_result_notify(DPI_ID_TMGP_SGAME_STREAM_GAME_DATA);
	if (s_wzry_table_header) {
		unregister_net_sysctl_table(s_wzry_table_header);
	}
	nf_unregister_net_hooks(&init_net, wzry_netfilter_ops, ARRAY_SIZE(wzry_netfilter_ops));
}

