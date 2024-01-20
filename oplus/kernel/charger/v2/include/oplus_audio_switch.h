// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */
#ifndef __OPLUS_AUDIO_SWITCH_H__
#define __OPLUS_AUDIO_SWITCH_H__

#if IS_ENABLED(CONFIG_OPLUS_AUDIO_SWITCH_GLINK)
int register_chg_glink_notifier(struct notifier_block *nb);
int unregister_chg_glink_notifier(struct notifier_block *nb);
#else
static inline int register_chg_glink_notifier(struct notifier_block *nb)
{
	return -ENODEV;
}
static inline int unregister_chg_glink_notifier(struct notifier_block *nb)
{
	return -ENODEV;
}
#endif
#endif/*__OPLUS_AUDIO_SWITCH_H__*/
