/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/module.h>

extern int linkpower_netlink_init(void);
extern void linkpower_netlink_fini(void);
extern int sk_pid_hook_init(void);
extern void sk_pid_hook_fini(void);

#if defined MTK_PLATFORM
extern int hba_proxy_mtk_init(void);
extern void hba_proxy_mtk_fini(void);
extern int ccci_wakeup_hook_init(void);
extern void ccci_wakeup_hook_fini(void);
#elif defined QCOM_PLATFORM
extern int hba_proxy_qcom_init(void);
extern void hba_proxy_qcom_fini(void);
extern int qrtr_hook_init(void);
extern void qrtr_hook_fini(void);
#endif

/**
 * @brief      Initialize linkpower module.
 *
 * @return     0 if successful, negative otherwise.
 */
static int __init linkpower_main_init(void)
{
	int ret = 0;

	ret = linkpower_netlink_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init netlink.\n");
		return ret;
	}
	ret = sk_pid_hook_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init sk_pid_hook.\n");
		linkpower_netlink_fini();
		return ret;
	}
	printk("[linkpower_main] generic module init successfully!");
#if defined MTK_PLATFORM
	ret = hba_proxy_mtk_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init hba_proxy_mtk.\n");
		linkpower_netlink_fini();
		sk_pid_hook_fini();
		return ret;
	}
	ret = ccci_wakeup_hook_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init ccci_wakeup_hook.\n");
		linkpower_netlink_fini();
		sk_pid_hook_fini();
		hba_proxy_mtk_fini();
		return ret;
	}
	printk("[linkpower_main] mtk module init successfully!");
#elif defined QCOM_PLATFORM
	ret = hba_proxy_qcom_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init hba_proxy_qcom.\n");
		linkpower_netlink_fini();
		sk_pid_hook_fini();
		return ret;
	}
	ret = qrtr_hook_init();
	if (ret < 0) {
		printk("[linkpower_main] module failed to init hba_proxy_qcom.\n");
		linkpower_netlink_fini();
		sk_pid_hook_fini();
		hba_proxy_qcom_fini();
		return ret;
	}
	printk("[linkpower_main] qcom module init successfully!");
#endif

	printk("[linkpower_main] all module init successfully!");
	return 0;
}

/**
 * @brief      Uninstall linkpower module.
 */
static void __exit linkpower_main_fini(void)
{
	linkpower_netlink_fini();
	sk_pid_hook_fini();
	printk("[linkpower_main] generic module fini successfully!");
#if defined MTK_PLATFORM
	hba_proxy_mtk_fini();
	ccci_wakeup_hook_fini();
	printk("[linkpower_main] mtk module fini successfully!");
#elif defined QCOM_PLATFORM
	hba_proxy_qcom_fini();
	qrtr_hook_fini();
	printk("[linkpower_main] qcom module fini successfully!");
#endif
	printk("[linkpower_main] all module fini successfully!");
}

module_init(linkpower_main_init);
module_exit(linkpower_main_fini);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aska");
MODULE_VERSION("1:1.0");
MODULE_DESCRIPTION("OPLUS linkpower module");
