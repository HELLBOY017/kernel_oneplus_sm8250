/* SPDX-License-Identifier: GPL-2.0-only  */
/*
 * Copyright (C) 2018-2022 Oplus. All rights reserved.
 */

#ifndef __OPLUS_HAL_GAUGE_MT6375_H__
#define __OPLUS_HAL_GAUGE_MT6375_H__

#include <oplus_chg_ic.h>

#define OPLUS_USE_FAST_CHARGER
#define DRIVER_VERSION			"1.1.0"

#define CAPACITY_SALTATE_COUNTER		4
#define CAPACITY_SALTATE_COUNTER_NOT_CHARGING	20
#define CAPACITY_SALTATE_COUNTER_80		30
#define CAPACITY_SALTATE_COUNTER_90		40
#define CAPACITY_SALTATE_COUNTER_95		60
#define CAPACITY_SALTATE_COUNTER_FULL		120

#define BATTERY_2700MA				0
#define BATTERY_3000MA				1
#define TYPE_INFO_LEN				8

#define DEVICE_TYPE_BQ27541			0x0541
#define DEVICE_TYPE_BQ27411			0x0421
#define DEVICE_TYPE_BQ28Z610			0xFFA5
#define DEVICE_TYPE_ZY0602			0x0602
#define DEVICE_TYPE_ZY0603			0xA5FF

#define DEVICE_BQ27541				0
#define DEVICE_BQ27411				1
#define DEVICE_BQ28Z610				2
#define DEVICE_ZY0602				3
#define DEVICE_ZY0603				4

#define DEVICE_TYPE_FOR_VOOC_BQ27541		0
#define DEVICE_TYPE_FOR_VOOC_BQ27411		1

#define CONTROL_CMD				0x00
#define CONTROL_STATUS				0x00
#define SEAL_POLLING_RETRY_LIMIT		100

#define SEAL_SUBCMD				0x0020

#define U_DELAY_1_MS	1000
#define U_DELAY_5_MS	5000
#define M_DELAY_10_S	10000

struct chip_mt6375_gauge{
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;

	atomic_t locked;

	int soc_pre;
	int temp_pre;
	int batt_vol_pre;
	int current_pre;
	int cc_pre;
	int soh_pre;
	int fcc_pre;
	int rm_pre;
	int fc_pre; /* add gauge reg print log start */
	int qm_pre;
	int pd_pre;
	int rcu_pre;
	int rcf_pre;
	int fcu_pre;
	int fcf_pre;
	int sou_pre;
	int do0_pre;
	int passed_q_pre;
	int doe_pre;
	int trm_pre;
	int pc_pre;
	int qs_pre; /* add gauge reg print log end */
	int volt_1_pre;
	int volt_2_pre;
	int dod0_1_pre;
	int dod0_2_pre;
	int dod_passed_q_pre;
	int qmax_1_pre;
	int qmax_2_pre;
	int qmax_passed_q_pre;
	int device_type;
	int device_type_for_vooc;
	atomic_t suspended;
	int batt_cell_1_vol;
	int batt_cell_2_vol;
	int batt_cell_max_vol;
	int batt_cell_min_vol;
	int max_vol_pre;
	int min_vol_pre;
	int batt_num;

	bool modify_soc_smooth;
	bool modify_soc_calibration;

	bool battery_full_param; /* only for wite battery full param in guage dirver probe on 7250 platform */
	int sha1_key_index;
	struct delayed_work afi_update;
	bool afi_update_done;
	bool protect_check_done;
	bool disabled;
	bool error_occured;
	bool need_check;
	unsigned int afi_count;
	unsigned int zy_dts_qmax_min;
	unsigned int zy_dts_qmax_max;
	const u8 *static_df_checksum_3e;
	const u8 *static_df_checksum_60;
	const u8 **afi_buf;
	unsigned int *afi_buf_len;
	bool enable_sleep_mode;
	int fg_soft_version;
	int gauge_num;
	struct mutex chip_mutex;
	atomic_t i2c_err_count;
	bool i2c_err;
	struct file_operations *authenticate_ops;

	struct delayed_work check_iic_recover;
};

#endif /* __OPLUS_HAL_GAUGE_MT6375_H__ */
