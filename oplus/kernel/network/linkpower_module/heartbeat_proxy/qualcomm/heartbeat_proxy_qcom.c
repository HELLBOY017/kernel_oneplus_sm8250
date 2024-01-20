/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/module.h>

/**
 * @brief      Initialize Qualcomm heartbeat proxy driver.
 *
 * @return     0 if successful, negative otherwise.
 */
int hba_proxy_qcom_init(void)
{
	printk("[heartbeat_proxy_qcom] module init successfully!");

	return 0;
}

/**
 * @brief      Uninstall Qualcomm heartbeat proxy driver.
 */
void hba_proxy_qcom_fini(void)
{
}
