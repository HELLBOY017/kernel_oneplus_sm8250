/* SPDX-License-Identifier: GPL-2.0-only  */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

//#ifndef _OPLUS_ADAPTER_H_
//#define _OPLUS_ADAPTER_H_

#include <linux/workqueue.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#include <linux/wakelock.h>
#endif
#include <linux/timer.h>
#include <linux/slab.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/device_info.h>
#endif
#include <linux/firmware.h>

enum {
        ADAPTER_FW_UPDATE_NONE,
        ADAPTER_FW_NEED_UPDATE,
        ADAPTER_FW_UPDATE_SUCCESS,
        ADAPTER_FW_UPDATE_FAIL,
};


struct oplus_adapter_chip {
		struct delayed_work              adapter_update_work;
        const struct    oplus_adapter_operations    *vops;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
        struct wake_lock                 adapter_wake_lock;
#else
        struct wakeup_source			*adapter_ws;
#endif
};

struct oplus_adapter_operations {
        bool (*adapter_update)(unsigned long tx_pin, unsigned long rx_pin);
};


void oplus_adapter_fw_update(void);
void oplus_vooc_reset_mcu(void);
void oplus_vooc_set_ap_clk_high(void);
int oplus_vooc_get_vooc_switch_val(void);
bool oplus_vooc_check_chip_is_null(void);
void oplus_adapter_init(struct oplus_adapter_chip *chip);
bool oplus_adapter_check_chip_is_null(void);

//#endif /* _OPLUS_ADAPTER_H_ */
