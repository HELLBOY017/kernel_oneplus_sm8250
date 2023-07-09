#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/types.h>
#include <linux/version.h>



extern int comm_netlink_module_init(void);
extern void comm_netlink_module_exit(void);

extern int oplus_dpi_module_init(void);
extern void oplus_dpi_module_fini(void);

extern int cls_dpi_mod_init(void);
extern void cls_dpi_mod_exit(void);

extern int tmgp_sgame_init(void);
extern void tmgp_sgame_fini(void);

extern int wzry_stats_init(void);
extern void wzry_stats_fini(void);

extern int log_stream_init(void);
extern void log_stream_fini(void);

static int __init data_modules_init(void)
{
	int ret = 0;

	ret = comm_netlink_module_init();
	if (ret) {
		return ret;
	}
	ret = oplus_dpi_module_init();
	if (ret) {
		goto dpi_failed;
	}
	ret = cls_dpi_mod_init();
	if (ret) {
		goto cls_failed;
	}
	ret = tmgp_sgame_init();
	if (ret) {
		goto tgmp_failed;
	}
	ret = wzry_stats_init();
	if (ret) {
		goto wzry_failed;
	}
	ret = log_stream_init();
	if(ret){
	    goto log_failed;
	}
	printk("data_modules_init succ \n");
	return 0;

log_failed:
    wzry_stats_fini();
wzry_failed:
	tmgp_sgame_fini();
tgmp_failed:
	cls_dpi_mod_exit();
cls_failed:
	oplus_dpi_module_fini();
dpi_failed:
	comm_netlink_module_exit();
	printk("data_modules_init failed!\n");
	return -1;
}

static void __exit data_modules_exit(void)
{
	log_stream_fini();
	wzry_stats_fini();
	tmgp_sgame_fini();
	cls_dpi_mod_exit();
	oplus_dpi_module_fini();
	comm_netlink_module_exit();
	printk("data_modules_exit\n");
	return;
}

module_init(data_modules_init);
module_exit(data_modules_exit);
MODULE_LICENSE("GPL");
