#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/version.h>
#include <net/tcp.h>
#include <net/dst.h>
#include <linux/file.h>
#include <net/tcp_states.h>
#include <linux/netlink.h>
#include <net/genetlink.h>
#include <linux/netfilter_ipv4.h>
#include <linux/tcp.h>
#include <net/inet_connection_sock.h>
#include <linux/spinlock.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include "oplus_kernel_common/oplus_kernel_common.h"
#include "oplus_tcp_syn/oplus_tcp_syn.h"
#include "oplus_tcp_congest_control/oplus_tcp_congest_control.h"
#include "oplus_dev_check/oplus_dev_check.h"

#define OPLUS_NETWORK_TUNING_FAMILY_NAME "kernel_tuning"
#define OPLUS_NETWORK_TUNING_FAMILY_VERSION	1
#define OPLUS_NETWORK_TUNING_MSG_MAX (__OPLUS_TUNING_MSG_MAX - 1)


static int __init oplus_kernel_tuning_init(void)
{
	int ret;
	ret = oplus_tuning_tcpsyn_init();
	if(ret < 0) {
		return -1;
	}

	oplus_tcp_congest_control_init();
	oplus_dev_check_init();

	return ret;
}

static void __exit oplus_kernel_tuning_fini(void)
{
	oplus_tuning_tcpsyn_fini();
	oplus_tcp_congest_control_fini();
	oplus_dev_check_fini();
}

module_init(oplus_kernel_tuning_init);
module_exit(oplus_kernel_tuning_fini);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("oplus_network_tuning");
