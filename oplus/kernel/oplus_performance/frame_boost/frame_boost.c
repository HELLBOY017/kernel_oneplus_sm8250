// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include "frame_boost.h"


struct fbg_vendor_hook fbg_hook;
unsigned int sysctl_frame_boost_enable = 1;
unsigned int sysctl_frame_boost_debug = 0;
EXPORT_SYMBOL(sysctl_frame_boost_debug);

/******************
 * moduler function
 *******************/
static int __init oplus_frame_boost_init(void)
{
	int ret = 0;

	ret = frame_ioctl_init();
	if (ret != 0)
		return ret;

	ret = frame_info_init();
	if (ret != 0)
		goto out;

	ret = frame_group_init();
	if (ret != 0)
		goto out;

	/* kernel version < 5.10, we put the hook in the call function*/

	ofb_debug("oplus_bsp_frame_boost.ko init succeed!!\n");

out:
	return ret;
}

module_init(oplus_frame_boost_init);
MODULE_DESCRIPTION("Oplus Frame Boost Moduler");
MODULE_LICENSE("GPL v2");
