#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <net/dst.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include "oplus_dev_check.h"

int oplus_dev_check_init(void)
{
	return 0;
}

void oplus_dev_check_fini(void)
{
}
