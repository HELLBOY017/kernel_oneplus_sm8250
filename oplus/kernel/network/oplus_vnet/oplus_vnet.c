/***********************************************************
** Copyright (C), 2008-2021, oplus Mobile Comm Corp., Ltd.
** File: oplus_vnet.c
** Description: Add virtuan network for data shareds.
**
** Version: 1.0
** Date : 2022/8/12
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
****************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <net/pkt_sched.h>
#include <net/net_namespace.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>

static int s_debug = 0;
static int s_enable = 1;

#define MAX_TXQUEUE_LEN 500


#define logi(fmt, args...) \
	do { \
		if (s_debug) { \
			printk("OVNET:" fmt "\n", ##args); \
		} \
	} while (0)

#define logk(fmt, args...) \
	do { \
		printk("OVNET:" fmt "\n", ##args); \
	} while (0)


enum {
	IFLA_OVNET_UNSPEC,
	IFLA_OVNET_BIND,
	IFLA_OVNET_PEER,
	__IFLA_OVNET_MAX,
};

#define IFLA_OVNET_MAX (__IFLA_OVNET_MAX - 1)

#define VIRTUAL_NET_MAX 16

struct ovnet_dev_private {
	u32 bind_if;
	u32 peer_ip;
	struct net_device *bind_dev;
	struct nf_hook_ops nf_ops;
};

struct ovnet_if_kv {
	u32 ovnet_if;
	u32 org_if;
};

static struct ovnet_if_kv s_if_map[VIRTUAL_NET_MAX];
static int s_used_count = 0;
static spinlock_t s_if_lock;

static int get_ovnet_if_by_org(u32 org_if, u32 *ovnet_if)
{
	int i = 0;

	if (org_if <= 0) {
		return -1;
	}

	spin_lock(&s_if_lock);
	for (i = 0; i < VIRTUAL_NET_MAX; i++) {
		if (s_if_map[i].org_if == org_if) {
			*ovnet_if = s_if_map[i].ovnet_if;
			spin_unlock(&s_if_lock);
			return 0;
		}
	}
	spin_unlock(&s_if_lock);

	return -1;
}


static int __attribute__((unused)) get_org_if_by_ovnet_if(u32 ovnet_if, u32 *org_if)
{
	int i = 0;

	if (ovnet_if <= 0) {
		return -1;
	}

	spin_lock(&s_if_lock);
	for (i = 0; i < VIRTUAL_NET_MAX; i++) {
		if (s_if_map[i].ovnet_if == ovnet_if) {
			*org_if = s_if_map[i].org_if;
			spin_unlock(&s_if_lock);
			return 0;
		}
	}
	spin_unlock(&s_if_lock);

	return -1;
}

static int update_ovnet_org_if(u32 org_if, u32 ovnet_if)
{
	int ret = -1;
	int i = 0;
	int first_usable = -1;

	spin_lock(&s_if_lock);
	for (i = 0; i < VIRTUAL_NET_MAX; i++) {
		if (s_if_map[i].ovnet_if == 0) {
			if (first_usable == -1) {
				first_usable = i;
			}
		} else if (s_if_map[i].ovnet_if == ovnet_if) {
			if (s_if_map[i].org_if != org_if) {
				logk("ifmap update!!! ");
				s_if_map[i].org_if = org_if;
			} else {
				logk("ifmap not update!!! ");
			}
			spin_unlock(&s_if_lock);
			return 0;
		} else if (s_if_map[i].org_if == org_if) {
			if (s_if_map[i].ovnet_if != ovnet_if) {
				logk("ifmap 2 update!!! ");
				s_if_map[i].ovnet_if = ovnet_if;
			} else {
				logk("ifmap 2 not update!!! ");
			}
			spin_unlock(&s_if_lock);
			return 0;
		}
	}

	if (first_usable != -1) {
		s_if_map[first_usable].ovnet_if = ovnet_if;
		s_if_map[first_usable].org_if = org_if;
		s_used_count++;
		ret = 0;
	} else {
		logk("unavailable position for new key-value");
	}

	spin_unlock(&s_if_lock);
	return ret;
}

static int __attribute__((unused)) del_if_map_by_org(u32 org_if)
{
	int i = 0;

	spin_lock(&s_if_lock);
	for (i = 0; i < VIRTUAL_NET_MAX; i++) {
		if (s_if_map[i].org_if == org_if) {
			s_if_map[i].org_if = 0;
			s_if_map[i].ovnet_if = 0;
			s_used_count--;
			break;
		}
	}

	spin_unlock(&s_if_lock);
	return 0;
}


static int del_if_map_by_ovnet(u32 ovnet_if)
{
	int i = 0;

	spin_lock(&s_if_lock);
	for (i = 0; i < VIRTUAL_NET_MAX; i++) {
		if (s_if_map[i].ovnet_if == ovnet_if) {
			s_if_map[i].org_if = 0;
			s_if_map[i].ovnet_if = 0;
			s_used_count--;
			break;
		}
	}

	spin_unlock(&s_if_lock);
	return 0;
}


static int check_is_local_skb(struct sk_buff *skb, int dir, u32 peerip)
{
	struct iphdr *iph = NULL;
	struct ipv6hdr *ipv6h = NULL;
	u32 mask = 0x00FFFFFF;
	/* struct udphdr *udph = NULL; */

	if (skb->protocol == htons(ETH_P_IP)) {
		iph = ip_hdr(skb);
		if (!iph) {
			return 1;
		}
		if (dir) {
			if (((iph->daddr & mask) == (peerip & mask)) || (iph->daddr == 0xffffffff)) {
				return 1;
			}
		} else {
			if (((iph->saddr & mask) == (peerip & mask)) || (iph->saddr == 0) || (iph->daddr == 0xffffffff)) {
				return 1;
			}
		}
		return 0;
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		ipv6h = ipv6_hdr(skb);
		if (!ipv6h) {
			return 1;
		}
		/* TODO: need to check v6 addr? */
		return 1;
	} else {
		return 1;
	}
}


static unsigned int ovnet_dev_ingress_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	int ret = 0;
	int redirect_iif = 0;
	struct net_device *rediret_dev = NULL;
	struct ovnet_dev_private *private = NULL;

	logi("cell_dev_ingress_hook enter!");
	if (!s_enable) {
		return NF_ACCEPT;
	}

	ret = get_ovnet_if_by_org(skb->skb_iif, &redirect_iif);
	if (ret) {
		return NF_ACCEPT;
	}

	rcu_read_lock();
	rediret_dev = dev_get_by_index_rcu(dev_net(skb->dev), redirect_iif);
	if (!rediret_dev) {
		rcu_read_unlock();
		logi("cannot find redirect device for %u", redirect_iif);
		return NF_ACCEPT;
	}

	private = netdev_priv(rediret_dev);
	if (check_is_local_skb(skb, 0, private->peer_ip)) {
		logi("recv skb is local");
		rcu_read_unlock();
		return NF_ACCEPT;
	}

	skb->dev = rediret_dev;
	skb->skb_iif = rediret_dev->ifindex;
	skb->dev->stats.rx_packets += 1;
	skb->dev->stats.rx_bytes += skb->len;
	rcu_read_unlock();
	ret = netif_receive_skb_core(skb);
	logi("netif_receive_skb_core return %d", ret);

	return NF_STOLEN;
}


static int ovnet_open(struct net_device *dev)
{
	logk("device open...");
	netif_tx_start_all_queues(dev);
	return 0;
}


static int ovnet_close(struct net_device *dev)
{
	logk("device close...");
	netif_tx_stop_all_queues(dev);
	return 0;
}


static netdev_tx_t ovnet_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int ret = 0;
	struct ovnet_dev_private *private = netdev_priv(dev);
	struct net_device *org_dev = NULL;
	struct iphdr *iph = NULL;
	int is_local_ip = 0;
	u32 xmit_dst_ip = 0;

	logi("ovnet_xmit start! %d", dev->ifindex);
	if (skb->protocol != htons(ETH_P_IP)) {
		logi("ovnet_xmit start! protocol not support %d", skb->protocol);
		return NETDEV_TX_OK;
	}
	iph = ip_hdr(skb);

	org_dev = private->bind_dev;
	if (!org_dev) {
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		logi("cannot find out device! %d", dev->ifindex);
		return NETDEV_TX_OK;
	}

	is_local_ip = check_is_local_skb(skb, 1, private->peer_ip);
	if (is_local_ip) {
		logi("warn: local packet send to this device");
		xmit_dst_ip = iph->daddr;
	} else {
		dev->stats.tx_packets += 1;
		dev->stats.tx_bytes += skb->len;
		xmit_dst_ip = private->peer_ip;
	}

	skb->dev = org_dev;
	ret = neigh_xmit(NEIGH_ARP_TABLE, skb->dev, &xmit_dst_ip, skb);
	logi("ovnet_xmit skb %s return %d", skb->dev->name, ret);

	return NETDEV_TX_OK;
}


static int ovnet_validate_addr(struct net_device *dev)
{
	logk("ovnet_validate_addr ");
	return 0;
}


static int ovnet_init_dev(struct net_device *dev)
{
	logk("ovnet_init_dev ");
	return 0;
}


static const struct net_device_ops ovnet_netdev_ops = {
	.ndo_open = ovnet_open,
	.ndo_stop = ovnet_close,
	.ndo_start_xmit = ovnet_xmit,
	.ndo_validate_addr = ovnet_validate_addr,
	.ndo_init = ovnet_init_dev,
};


static int ovnet_register_dev(struct net_device *dev, u32 ifindex)
{
	struct net *net = dev_net(dev);
	struct ovnet_dev_private *private = netdev_priv(dev);
	struct net_device *bind_dev = NULL;
	int ret = 0;

	logk("register_dev .. %u", ifindex);
	if (ifindex == 0) {
		return -1;
	}

	rcu_read_lock();
	bind_dev = dev_get_by_index_rcu(net, ifindex);
	if(!bind_dev) {
		rcu_read_unlock();
		logk("get dev failed!");
		return -1;
	}

	logk("register_dev_ingress enter %s -> %s", dev->name, bind_dev->name);
	private->nf_ops.dev = bind_dev;
	private->nf_ops.hook = ovnet_dev_ingress_hook;
	private->nf_ops.pf = NFPROTO_NETDEV;
	private->nf_ops.hooknum = NF_NETDEV_INGRESS;
	private->nf_ops.priority = NF_IP_PRI_FILTER + 1;

	ret = nf_register_net_hook(&init_net, &private->nf_ops);
	if(ret) {
		logk("nf_register_net_hook failed! %d ", ret);
	} else {
		logk("nf_register_net_hook succ");
		dev_hold(bind_dev);
		private->bind_dev = bind_dev;
	}
	rcu_read_unlock();

	return ret;
}


static int ovnet_unregister_dev(struct net_device *dev)
{
	struct ovnet_dev_private *private = netdev_priv(dev);

	logk("ovnet_unregister_dev .... %s, bind if %u", dev->name, private->bind_if);
	if (private->bind_dev) {
		logk("unregister net hook! %s", private->bind_dev->name);
		rcu_read_lock();
		nf_unregister_net_hook(&init_net, &private->nf_ops);
		memset(&private->nf_ops, 0, sizeof(private->nf_ops));
		dev_put(private->bind_dev);
		private->bind_dev = NULL;
		rcu_read_unlock();
	} else {
		logk("bind_dev is null!");
	}

	return 0;
}


static void ovnet_dev_free(struct net_device *dev)
{
	logk("ovnet_dev_free %s...", dev->name);
	return;
}


#define OVNET_FEATURE (NETIF_F_HW_CSUM | NETIF_F_SG  | NETIF_F_FRAGLIST | \
						NETIF_F_TSO_ECN | NETIF_F_TSO | NETIF_F_TSO6      | \
						NETIF_F_GSO_ENCAP_ALL                             | \
						NETIF_F_HIGHDMA | NETIF_F_HW_VLAN_CTAG_TX         | \
						NETIF_F_HW_VLAN_STAG_TX)



static void ovnet_setup(struct net_device *dev)
{
	logk("ovnet_setup  %s..", dev->name);

	dev->netdev_ops = &ovnet_netdev_ops;
	ether_setup(dev);
	dev->tx_queue_len = MAX_TXQUEUE_LEN;
	dev->features |= OVNET_FEATURE;
	dev->hw_features |= dev->features;
	dev->hw_enc_features |= dev->features;
	dev->vlan_features |= OVNET_FEATURE & ~(NETIF_F_HW_VLAN_CTAG_TX |
											NETIF_F_HW_VLAN_STAG_TX);
	dev->flags |= IFF_NOARP | IFF_POINTOPOINT;
	dev->priv_flags &= ~IFF_TX_SKB_SHARING;
	netif_keep_dst(dev);
	eth_hw_addr_random(dev);
	dev->needs_free_netdev = true;
	dev->priv_destructor = ovnet_dev_free;
}


static int ovnet_validate(struct nlattr *tb[], struct nlattr *data[], struct netlink_ext_ack *extack)
{
	logk("cell_validate %p %p %p", tb, data, extack);
	return 0;
}


static int ovnet_newlink(struct net *src_net, struct net_device *dev, struct nlattr *tb[], struct nlattr *data[], struct netlink_ext_ack *extack)
{
	struct ovnet_dev_private *private = netdev_priv(dev);
	int ret = 0;

	logk("ovnet_newlink ...%s %p %p %p", dev->name, tb, data, extack);
	if (!data) {
		return 0;
	}

	ret = register_netdevice(dev);
	if (ret) {
		logk("register_netdevice failed! %d", ret);
		return ret;
	}
	logk("register_netdevice succ!");

	logk("ovnet_newlink check data %d", dev->ifindex);
	if (data[IFLA_OVNET_BIND]) {
		private->bind_if = nla_get_u32(data[IFLA_OVNET_BIND]);

		logk("recv bind param %u", private->bind_if);
		ret = ovnet_register_dev(dev, private->bind_if);
		if (ret) {
			logk("ovnet_register_dev failed! %d", ret);
			return ret;
		}
		ret = update_ovnet_org_if(private->bind_if, dev->ifindex);
		logk("update_ovnet_org_if return ret");
	}

	if (data[IFLA_OVNET_PEER]) {
		private->peer_ip = nla_get_u32(data[IFLA_OVNET_PEER]);
		logk("recv peer addr %u", private->peer_ip);
	}

	return 0;
}


static int ovnet_changelink(struct net_device *dev, struct nlattr *tb[], struct nlattr *data[], struct netlink_ext_ack *extack)
{
	struct ovnet_dev_private *private = netdev_priv(dev);

	logk("ovnet_changelink ....%s %p %p %p", dev->name, tb, data, extack);
	if (!data) {
		return 0;
	}

	if (data[IFLA_OVNET_BIND]) {
		u32 bind_if = nla_get_u32(data[IFLA_OVNET_BIND]);
		if (bind_if != private->bind_if) {
			logk("change bind if %u -> %u", private->bind_if, bind_if);
			ovnet_unregister_dev(dev);
			private->bind_if = bind_if;
			if (private->bind_if != 0) {
				ovnet_register_dev(dev, private->bind_if);
				update_ovnet_org_if(private->bind_if, dev->ifindex);
			} else {
				del_if_map_by_ovnet(dev->ifindex);
			}
		}
	}

	if (data[IFLA_OVNET_PEER]) {
		private->peer_ip = nla_get_u32(data[IFLA_OVNET_PEER]);
		logk("recv peer addr %u", private->peer_ip);
	}

	return 0;
}



static void ovnet_dellink(struct net_device *dev, struct list_head *head)
{
	logk("ovnet_dellink ....%s", dev->name);
	del_if_map_by_ovnet(dev->ifindex);
	ovnet_unregister_dev(dev);
	unregister_netdevice_queue(dev, head);
	return;
}


static size_t ovnet_get_size(const struct net_device *dev)
{
	return nla_total_size(sizeof(u32)) + nla_total_size(sizeof(u32));
}

static int ovnet_fillinfo(struct sk_buff *skb, const struct net_device *dev)
{
	struct ovnet_dev_private *private = netdev_priv(dev);

	if (nla_put_u32(skb, IFLA_OVNET_BIND, private->bind_if)) {
		goto nla_put_failure;
	}
	if (nla_put_u32(skb, IFLA_OVNET_PEER, private->peer_ip)) {
		goto nla_put_failure;
	}
	return 0;
nla_put_failure:
	return -EMSGSIZE;
}


static const struct nla_policy ovnet_nl_policy[IFLA_OVNET_MAX + 1] = {
	[IFLA_OVNET_BIND] = { .type = NLA_U32 },
	[IFLA_OVNET_PEER] = { .type = NLA_U32 },
};


static struct rtnl_link_ops ovnet_link_ops __read_mostly = {
	.kind			= "ovnet",
	.priv_size		= sizeof(struct ovnet_dev_private),
	.setup			= ovnet_setup,
	.validate		= ovnet_validate,
	.newlink		= ovnet_newlink,
	.changelink		= ovnet_changelink,
	.dellink		= ovnet_dellink,
	.get_size		= ovnet_get_size,
	.maxtype		= IFLA_OVNET_MAX,
	.policy			= ovnet_nl_policy,
	.fill_info		= ovnet_fillinfo,
};



static struct ctl_table s_ovnet_dev_sysctl_table[] __read_mostly = {
	{
		.procname = "debug",
		.data = &s_debug,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "enable",
		.data = &s_enable,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{}
};

static struct ctl_table_header * s_ovnet_table_header = NULL;


static int numvnets = 3;
module_param(numvnets, int, 0);
MODULE_PARM_DESC(numvnets, "Number of ovnet devices");


static int init_dev_one(int index)
{
	struct net_device *dev_vnet;
	int err;

	dev_vnet = alloc_netdev(sizeof(struct ovnet_dev_private), "ovnet%d", NET_NAME_UNKNOWN, ovnet_setup);

	if (!dev_vnet)
		return -ENOMEM;

	dev_vnet->rtnl_link_ops = &ovnet_link_ops;
	err = register_netdevice(dev_vnet);
	if (err < 0)
		goto err;

	return 0;

err:
	free_netdev(dev_vnet);
	return err;
}

static int init_ovnet_dev_all(void)
{
	int err = 0;
	int i = 0;

	for (i = 0; i < numvnets && !err; i++) {
		err = init_dev_one(i);
		cond_resched();
	}

	return err;
}


static int __init ovnet_dev_init(void)
{
	int ret = 0;

	logk("ovnet_dev_init...");
	spin_lock_init(&s_if_lock);
	memset((char *)s_if_map, 0, sizeof(s_if_map));

	rtnl_lock();
	ret = __rtnl_link_register(&ovnet_link_ops);
	if (ret) {
		rtnl_unlock();
		logk("rtnl_link_register failed! %d", ret);
		return -1;
	}
	ret = init_ovnet_dev_all();
	if (ret) {
		__rtnl_link_unregister(&ovnet_link_ops);
		rtnl_unlock();
		logk("init default ovnet device failed! %d", ret);
		return -1;
	}
	rtnl_unlock();

	s_ovnet_table_header = register_net_sysctl(&init_net, "net/ovnet_dev", s_ovnet_dev_sysctl_table);

	return 0;
}


static void __exit ovnet_dev_fini(void)
{
	/* TODO: del all devices */

	rtnl_link_unregister(&ovnet_link_ops);

	if (s_ovnet_table_header) {
		unregister_net_sysctl_table(s_ovnet_table_header);
	}

	logk("ovnet_dev_fini...");
}

module_init(ovnet_dev_init);
module_exit(ovnet_dev_fini);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("ovnet");



