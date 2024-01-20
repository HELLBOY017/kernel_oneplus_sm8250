#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <net/sock.h>
#include "netlink_es.h"
#include <net/netfilter/nf_conntrack.h>
#include <linux/netlink.h>
#include <net/genetlink.h>

#define LOG_TAG "netlink_esreport"

static int s_debug = 0;

#define LOGK(flag, fmt, args...)     \
    do {                             \
        if (flag || s_debug) {       \
            printk("[%s]:" fmt "\n", LOG_TAG, ##args);\
        }                                             \
    } while (0)

static int __init nl_es_init(void)
{
    //es_nl_msg_establish msg;
	int ret = 0;

    LOGK(1, "esnotify: %s initialed ok!\n",__func__);

    return ret;
}

static void __exit nl_es_exit(void)
{
    LOGK(1, "esnotify: %s existing...\n",__func__);
}


module_init(nl_es_init);
module_exit(nl_es_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(" netlink establish report protocol");
MODULE_AUTHOR("TS");

