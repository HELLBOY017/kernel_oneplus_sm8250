// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[NOTIFY]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "ufcs_core.h"
#include "ufcs_notify.h"

static ATOMIC_NOTIFIER_HEAD(ufcs_event_notifier);

int ufcs_reg_event_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&ufcs_event_notifier, nb);
}
EXPORT_SYMBOL_GPL(ufcs_reg_event_notifier);

void ufcs_unreg_event_notifier(struct notifier_block *nb)
{
	atomic_notifier_chain_unregister(&ufcs_event_notifier, nb);
}
EXPORT_SYMBOL_GPL(ufcs_unreg_event_notifier);

void ufcs_send_state(enum ufcs_notify_state state, void *v)
{
	atomic_notifier_call_chain(&ufcs_event_notifier, state, v);
}
