// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _OPLUS_SWITCHING_H_
#define _OPLUS_SWITCHING_H_

#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <oplus_mms.h>

#define SWITCH_ERROR_OVER_VOLTAGE      1
#define SWITCH_ERROR_I2C_ERROR         1

#define REASON_I2C_ERROR	       1
#define REASON_MOS_OPEN_ERROR	       2
#define REASON_SUB_BATT_FULL	       4
#define REASON_VBAT_GAP_BIG	       8
#define REASON_SOC_NOT_FULL	       16
#define REASON_CURRENT_UNBALANCE       32
#define REASON_RECORD_SOC	       48
#define REASON_SOC_GAP_TOO_BIG	       64 /* low 4 bit use to MOS error */

#define BATT_OPEN_RETRY_COUNT	       12
#define BATT_FGI2C_RETRY_COUNT	       2

enum {
	MAIN_BATT_OVER_CURR = 1,
	SUB_BATT_OVER_CURR,
};

enum {
	PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE,
	PARALLEL_NEED_BALANCE_BAT_STATUS1__STOP_CHARGE,           /*VBAT0 - VBAT1 >= 800mv*/
	PARALLEL_NEED_BALANCE_BAT_STATUS2__STOP_CHARGE,           /*800mv > VBAT0 - VBAT1 >= 600mv*/
	PARALLEL_NEED_BALANCE_BAT_STATUS3__STOP_CHARGE,           /*600mv > VBAT0 - VBAT1 >= 50mv*/
	PARALLEL_NEED_BALANCE_BAT_STATUS4__STOP_CHARGE,           /*400mv > VBAT1 - VBAT0 > 50mv*/
	PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE,           /*VBAT0 > 3400mv && VBAT1 < 3400 && VBAT0 - VBAT1 <= 1000mv*/
	PARALLEL_BAT_BALANCE_ERROR_STATUS6__STOP_CHARGE,          /*VBAT0 > 3400mv && VBAT1 < 3100 && VBAT0 - VBAT1 > 1000mv*/
	PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE,          /*VBAT1 - VBAT0 > 400mv */
	PARALLEL_BAT_BALANCE_ERROR_STATUS8,                       /*bat error*/
	PARALLEL_BAT_BALANCE_ERROR_STATUS9,			  /* bat error but need to check recovery */
};

enum {
	NO_PARALLEL_TYPE = 0,
	PARALLEL_SWITCH_IC,
	PARALLEL_MOS_CTRL,
	INVALID_PARALLEL,
};

enum switch_topic_item {
	SWITCH_ITEM_STATUS,
	SWITCH_ITEM_CURR_LIMIT,
	SWITCH_ITEM_HW_ENABLE_STATUS,
	SWITCH_ITEM_ERR_NOTIFY,
};
#if IS_ENABLED(CONFIG_OPLUS_CHG_PARALLEL)
int oplus_parallel_chg_mos_test(struct oplus_mms *topic);
#else
static inline int oplus_parallel_chg_mos_test(struct oplus_mms *topic)
{
	return 0;
}
#endif

#endif /* _OPLUS_GAUGE_H */
