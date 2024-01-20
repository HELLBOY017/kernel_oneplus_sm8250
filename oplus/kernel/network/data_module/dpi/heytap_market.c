/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: heytap_market.c
** Description: add heytap market dpi
**
** Version: 1.0
** Date : 2022/10/17
** Author: Yejunjie
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** yejunjie 2022/10/17 1.0 build this module
****************************************************************/
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/genetlink.h>

#include "../include/dpi_api.h"
#include "../include/comm_def.h"
#include "dpi_core.h"
#include "heytap_market.h"

#define LOG_TAG "HEYTAP_MARKET"

#define logt(fmt, args...) LOG(LOG_TAG, fmt, ##args)

u32 s_heytap_market_uid = 0;

static int is_tcp_or_udp(struct sk_buff *skb)
{
	struct iphdr *iph = NULL;
	struct ipv6hdr *ip6h = NULL;

	if (skb->protocol == htons(ETH_P_IP)) {
		iph = ip_hdr(skb);
		if (iph && (iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP)) {
			return 1;
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		ip6h = ipv6_hdr(skb);
		if (ip6h && (ip6h->nexthdr == IPPROTO_TCP || ip6h->nexthdr == IPPROTO_UDP)) {
			return 1;
		}
	}

	return 0;
}

static int heytap_market_match(struct sk_buff *skb, int dir, dpi_match_data_t *data)
{
	if (!dir && is_tcp_or_udp(skb)) {
		data->dpi_result = DPI_ID_HEYTAP_MARKET_STREAM_DOWNLOAD_DATA;
		data->state = DPI_MATCH_STATE_COMPLETE;
		return 0;
	}
	data->dpi_result = DPI_ID_HEYTAP_MARKET_APP;
	data->state = DPI_MATCH_STATE_COMPLETE;
	return 0;
}

int set_heytap_market_uid(u32 uid)
{
	int ret = 0;

	u32 old_uid = s_heytap_market_uid;
	s_heytap_market_uid = uid;
	if (s_heytap_market_uid != old_uid) {
		if (old_uid == 0) {
			ret = dpi_register_app_match(s_heytap_market_uid, heytap_market_match);
			logt("dpi_register_app_match heytap_market uid %u return %d", s_heytap_market_uid, ret);
		} else if (s_heytap_market_uid == 0) {
			ret = dpi_unregister_app_match(s_heytap_market_uid);
			logt("dpi_unregister_app_match tmgp uid %u return %d", s_heytap_market_uid, ret);
		} else {
			ret |= dpi_unregister_app_match(old_uid);
			ret |= dpi_register_app_match(s_heytap_market_uid, heytap_market_match);
			logt("dpi app uid change! tmgp uid %u %u return %d", s_heytap_market_uid, old_uid, ret);
		}
	}

	return ret;
}
