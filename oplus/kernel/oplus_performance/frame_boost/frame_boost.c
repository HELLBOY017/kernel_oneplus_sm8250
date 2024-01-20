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
int stune_boost[BOOST_MAX_TYPE];
unsigned int sysctl_frame_boost_enable = 1;
unsigned int sysctl_frame_boost_debug = 0;
EXPORT_SYMBOL(sysctl_frame_boost_debug);
static inline bool vaild_stune_boost_type(unsigned int type)
{
	return (type >= 0) && (type < BOOST_MAX_TYPE);
}

void fbg_set_stune_boost(int value, unsigned int type)
{
	if (!vaild_stune_boost_type(type))
		return;

	stune_boost[type] = value;
}
EXPORT_SYMBOL_GPL(fbg_set_stune_boost);

int fbg_get_stune_boost(unsigned int type)
{
	if (!vaild_stune_boost_type(type))
		return 0;

	return stune_boost[type];
}
EXPORT_SYMBOL_GPL(fbg_get_stune_boost);

/******************
 * moduler function
 *******************/
static int __init oplus_frame_boost_init(void)
{
	int ret = 0;

	ret = frame_info_init();
	if (ret != 0)
		goto out;

	ret = frame_group_init();
	if (ret != 0)
		goto out;


	ofb_debug("oplus_bsp_frame_boost.ko init succeed!!\n");

#ifdef CONFIG_ARCH_MEDIATEK
	stune_boost[BOOST_SF_FREQ_GPU] = 60;
	stune_boost[BOOST_SF_MIGR_GPU] = 60;
#else
	stune_boost[BOOST_SF_FREQ_GPU] = 30;
	stune_boost[BOOST_SF_MIGR_GPU] = 30;
#endif /* CONFIG_ARCH_MEDIATEK */

out:
	return ret;
}

module_init(oplus_frame_boost_init);
MODULE_DESCRIPTION("Oplus Frame Boost Moduler");
MODULE_LICENSE("GPL v2");
