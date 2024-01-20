// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#ifndef __OPLUS_CHG_PPS_H__
#define __OPLUS_CHG_PPS_H__

#include <oplus_mms.h>

#define PPS_PDO_MAX			7

#define PPS_PDO_VOL_MAX(pdo)		(pdo * 100)
#define PPS_PDO_VOL_MIN(pdo)		(pdo * 100)
#define PPS_PDO_CURR_MAX(pdo)		(pdo * 50)
#define PPS_STATUS_VOLT(pps_status) (((pps_status) >> 0) & 0xFFFF)
#define PPS_STATUS_CUR(pps_status) (((pps_status) >> 16) & 0xFF)

enum pps_topic_item {
	PPS_ITEM_ONLINE,
	PPS_ITEM_CHARGING,
	PPS_ITEM_ADAPTER_ID,
	PPS_ITEM_OPLUS_ADAPTER,
};

typedef enum
{
	USBPD_PDMSG_PDOTYPE_FIXED_SUPPLY,
	USBPD_PDMSG_PDOTYPE_BATTERY,
	USBPD_PDMSG_PDOTYPE_VARIABLE_SUPPLY,
	USBPD_PDMSG_PDOTYPE_AUGMENTED
}USBPD_PDMSG_PDOTYPE_TYPE;

typedef union
{
	u32 pdo_data;
	struct {
		u32 max_current50ma           : 8;    /*bit [ 6: 0]*/
		u32 min_voltage100mv          : 8;    /*bit [15: 8]*/
		u32                           : 1;    /*bit [16:16]*/
		u32 max_voltage100mv          : 8;    /*bit [24:17]*/
		u32                           : 2;    /*bit [26:25]*/
		u32 pps_power_limited         : 1;    /*bit [27:27]*/
		u32 pps                       : 2;    /*bit [29:28]*/
		u32 pdo_type                  : 2;    /*bit [31:30]*/
	};
} pps_msg_data;

struct pps_dev_ops {
	int (*pps_pdo_set)(int vol_mv, int curr_ma);
	int (*verify_adapter)(void);
	int (*get_pdo_info)(u32 *pdo, int num);
	u32 (*get_pps_status)(void);
};

struct oplus_pps_phy_ic {
	struct pps_dev_ops *ops;
	int phy_ic_exist;
};

enum pps_fastchg_type {
	PPS_FASTCHG_TYPE_UNKOWN,
	PPS_FASTCHG_TYPE_THIRD,
	PPS_FASTCHG_TYPE_V1,
	PPS_FASTCHG_TYPE_V2,
	PPS_FASTCHG_TYPE_V3,
	PPS_FASTCHG_TYPE_OTHER,
};

enum pps_power_type {
	PPS_POWER_TYPE_UNKOWN = 0,
	PPS_POWER_TYPE_THIRD = 33,
	PPS_POWER_TYPE_V1 = 125,
	PPS_POWER_TYPE_V2 = 150,
	PPS_POWER_TYPE_V3 = 240,
	OPLUS_PPS_POWER_MAX = 0xFFFF,
};

int oplus_pps_current_to_level(int curr);
enum fastchg_protocol_type oplus_pps_adapter_id_to_protocol_type(u32 id);
int oplus_pps_adapter_id_to_power(u32 id);
#endif /* __OPLUS_CHG_PPS_H__ */
