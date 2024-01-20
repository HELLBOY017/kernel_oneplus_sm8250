// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[UFCS]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_vooc.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_cpa.h>
#include <oplus_impedance_check.h>
#include <oplus_chg_monitor.h>
#include <oplus_chg_ufcs.h>
#include <ufcs_class.h>

#define UFCS_MONITOR_CYCLE_MS		1500
#define UFCS_WATCHDOG_TIME_MS		3000
#define UFCS_START_DEF_CURR_MA		1000
#define UFCS_START_DEF_VOL_MV		5000
#define UFCS_MONITOR_TIME_MS		500
#define UFCS_STOP_DELAY_TIME		300
#define UFCS_TEMP_SWITCH_DELAY		100
#define UFCS_ADSP_TEMP_SWITCH_DELAY	600

#define UFCS_UPDATE_PDO_TIME		5
#define UFCS_UPDATE_FASTCHG_TIME	1
#define UFCS_UPDATE_TEMP_TIME		1
#define UFCS_UPDATE_IBAT_TIME		1

#define UFCS_TEMP_OVER_COUNTS		2
#define UFCS_TEMP_WARM_RANGE_THD	10
#define UFCS_TEMP_LOW_RANGE_THD		20

#define UFCS_R_AVG_NUM			10
#define UFCS_R_ROW_NUM			7

#define FULL_UFCS_SYS_MAX		6

#define UFCS_BCC_CURRENT_MIN		(1000 / 100)
#define UFCS_BATT_CURR_TO_BCC_CURR	100

#define UFCS_VERIFY_CURR_THR_MA		3000

enum {
	UFCS_BAT_TEMP_NATURAL = 0,
	UFCS_BAT_TEMP_HIGH0,
	UFCS_BAT_TEMP_HIGH1,
	UFCS_BAT_TEMP_HIGH2,
	UFCS_BAT_TEMP_HIGH3,
	UFCS_BAT_TEMP_HIGH4,
	UFCS_BAT_TEMP_HIGH5,
	UFCS_BAT_TEMP_LOW0,
	UFCS_BAT_TEMP_LOW1,
	UFCS_BAT_TEMP_LOW2,
	UFCS_BAT_TEMP_LITTLE_COOL,
	UFCS_BAT_TEMP_LITTLE_COOL_LOW,
	UFCS_BAT_TEMP_COOL,
	UFCS_BAT_TEMP_NORMAL_LOW,
	UFCS_BAT_TEMP_NORMAL_HIGH,
	UFCS_BAT_TEMP_LITTLE_COLD,
	UFCS_BAT_TEMP_WARM,
	UFCS_BAT_TEMP_EXIT,
	UFCS_BAT_TEMP_SWITCH_CURVE,
};

enum {
	UFCS_TEMP_RANGE_INIT = 0,
	UFCS_TEMP_RANGE_LITTLE_COLD, /* 0 ~ 5 */
	UFCS_TEMP_RANGE_COOL, /* 5 ~ 12 */
	UFCS_TEMP_RANGE_LITTLE_COOL, /* 12~20 */
	UFCS_TEMP_RANGE_NORMAL_LOW, /* 20~35 */
	UFCS_TEMP_RANGE_NORMAL_HIGH, /* 35~44 */
	UFCS_TEMP_RANGE_WARM, /* 44-52 */
	UFCS_TEMP_RANGE_NORMAL,
};

enum {
	UFCS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL,
	UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW,
	UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH,
	UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX,
};

enum ufcs_emark_power {
	UFCS_EMARK_POWER_DEFAULT = 3000,
	UFCS_EMARK_POWER_THIRD = 5000,
	UFCS_EMARK_POWER_V0 = 6500,
	UFCS_EMARK_POWER_V1 = 10000,
	UFCS_EMARK_POWER_V2 = 12000,
	UFCS_EMARK_POWER_V3 = 13700,
	UFCS_EMARK_POWER_V4 = 15000,
	UFCS_EMARK_POWER_V5 = 20000,
	UFCS_EMARK_POWER_V6 = 24000,
	UFCS_EMARK_POWER_MAX = 30000,
};

enum ufcs_emark_id {
	UFCS_EMARK_ID_V0,
	UFCS_EMARK_ID_V1 = 6,
	UFCS_EMARK_ID_V2 = 7,
	UFCS_EMARK_ID_V3 = 8,
	UFCS_EMARK_ID_V4 = 9,
	UFCS_EMARK_ID_V5 = 10,
	UFCS_EMARK_ID_V6 = 11,
	UFCS_EMARK_ID_MAX = 255,
};

struct oplus_ufcs_config {
	unsigned int target_vbus_mv;
	int curr_max_ma;
	bool adsp_ufcs_project;
};

struct oplus_ufcs_timer {
	struct timespec pdo_timer;
	struct timespec fastchg_timer;
	struct timespec temp_timer;
	struct timespec ibat_timer;
	int ufcs_max_time_ms;
	unsigned long monitor_jiffies;
};

struct ufcs_protection_counts {
	int cool_fw;
	int sw_full;
	int hw_full;
	int low_curr_full;
	int ibat_low;
	int ibat_high;
	int btb_high;
	int tbatt_over;
	int tfg_over;
	int output_low;
	int ibus_over;
};

struct oplus_ufcs_limits {
	int default_ufcs_normal_high_temp;
	int default_ufcs_little_cool_temp;
	int default_ufcs_cool_temp;
	int default_ufcs_little_cold_temp;
	int default_ufcs_normal_low_temp;
	int ufcs_warm_allow_vol;
	int ufcs_warm_allow_soc;

	int ufcs_batt_over_low_temp;
	int ufcs_little_cold_temp;
	int ufcs_cool_temp;
	int ufcs_little_cool_temp;
	int ufcs_normal_low_temp;
	int ufcs_normal_high_temp;
	int ufcs_batt_over_high_temp;
	int ufcs_strategy_temp_num;

	int ufcs_strategy_soc_over_low;
	int ufcs_strategy_soc_min;
	int ufcs_strategy_soc_low;
	int ufcs_strategy_soc_mid_low;
	int ufcs_strategy_soc_mid;
	int ufcs_strategy_soc_mid_high;
	int ufcs_strategy_soc_high;
	int ufcs_strategy_soc_num;

	int ufcs_strategy_normal_current;
	int ufcs_strategy_batt_high_temp0;
	int ufcs_strategy_batt_high_temp1;
	int ufcs_strategy_batt_high_temp2;
	int ufcs_strategy_batt_low_temp2;
	int ufcs_strategy_batt_low_temp1;
	int ufcs_strategy_batt_low_temp0;
	int ufcs_strategy_high_current0;
	int ufcs_strategy_high_current1;
	int ufcs_strategy_high_current2;
	int ufcs_strategy_low_current2;
	int ufcs_strategy_low_current1;
	int ufcs_strategy_low_current0;

	int ufcs_low_curr_full_cool_temp;
	int ufcs_low_curr_full_little_cool_temp;
	int ufcs_low_curr_full_normal_low_temp;
	int ufcs_low_curr_full_normal_high_temp;

	int ufcs_over_high_or_low_current;
	int ufcs_strategy_change_count;
	int ufcs_full_cool_sw_vbat;
	int ufcs_full_normal_sw_vbat;
	int ufcs_full_normal_hw_vbat;
	int ufcs_full_warm_vbat;
	int ufcs_full_ffc_vbat;
	int ufcs_full_cool_sw_vbat_third;
	int ufcs_full_normal_sw_vbat_third;
	int ufcs_full_normal_hw_vbat_third;
	int ufcs_timeout_third;
	int ufcs_timeout_oplus;
	int ufcs_ibat_over_third;
	int ufcs_ibat_over_oplus;

	int ufcs_low_temp;
	int ufcs_high_temp;
	int ufcs_low_soc;
	int ufcs_high_soc;
};

struct ufcs_full_curve {
	unsigned int iterm;
	unsigned int vterm;
	bool exit;
};

struct ufcs_full_curves_temp {
	struct ufcs_full_curve full_curves[FULL_UFCS_SYS_MAX];
	int full_curve_num;
};

#define UFCS_CMD_BUF_SIZE		128
#define UFCS_GET_AUTH_DATA_TIMEOUT_MS	3000000 /* wait for the daemon to start */
enum ufcs_dev_cmd_type {
	UFCS_DEV_CMD_EXIT,
	UFCS_DEV_CMD_GET_AUTH_DATA,
};

struct ufcs_dev_cmd {
	unsigned int cmd;
	unsigned int data_size;
	unsigned char data_buf[UFCS_CMD_BUF_SIZE];
};

struct ufcs_bcc_info {
	int bcc_min_curr;
	int bcc_max_curr;
	int bcc_exit_curr;
};

struct oplus_ufcs {
	struct device *dev;
	struct oplus_mms *ufcs_topic;
	struct oplus_mms *cpa_topic;
	struct mms_subscribe *cpa_subs;
	struct oplus_mms *wired_topic;
	struct mms_subscribe *wired_subs;
	struct oplus_mms *comm_topic;
	struct mms_subscribe *comm_subs;
	struct oplus_mms *gauge_topic;
	struct mms_subscribe *gauge_subs;
	struct oplus_mms *err_topic;
	struct oplus_chg_ic_dev *ufcs_ic;
	struct oplus_chg_ic_dev *cp_ic;
	struct oplus_chg_ic_dev *dpdm_switch;

	struct notifier_block nb;

	struct votable *ufcs_curr_votable;
	struct votable *ufcs_disable_votable;
	struct votable *ufcs_not_allow_votable;
	struct votable *ufcs_boot_votable;
	struct votable *wired_suspend_votable;

	struct delayed_work switch_check_work;
	struct delayed_work monitor_work;
	struct delayed_work current_work;
	struct delayed_work imp_uint_init_work;
	struct delayed_work wait_auth_data_work;

	struct work_struct wired_online_work;
	struct work_struct force_exit_work;
	struct work_struct soft_exit_work;
	struct work_struct gauge_update_work;
	struct work_struct err_flag_push_work;

	wait_queue_head_t read_wq;
	struct miscdevice misc_dev;
	struct mutex read_lock;
	struct mutex cmd_data_lock;
	struct completion cmd_ack;
	struct ufcs_dev_cmd cmd;
	bool cmd_data_ok;
	char auth_data[UFCS_VERIFY_AUTH_DATA_SIZE];
	bool auth_data_ok;
	bool wait_auth_data_done;

	struct oplus_chg_strategy *oplus_curve_strategy;
	struct oplus_chg_strategy *third_curve_strategy;
	struct oplus_chg_strategy *strategy;

	struct oplus_impedance_node *input_imp_node;
	struct oplus_impedance_unit *imp_uint;

	struct oplus_ufcs_config config;
	struct oplus_ufcs_timer timer;
	struct ufcs_protection_counts count;
	struct oplus_ufcs_limits limits;
	struct ufcs_bcc_info bcc;

	struct ufcs_full_curves_temp low_curr_full_curves_temp[UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX];

	u64 dev_info;
	u64 src_info;
	u64 cable_info;
	u64 emark_info;
	u64 pdo[UFCS_OUTPUT_MODE_MAX];
	int pdo_num;
	u64 pie[UFCS_OPLUS_VND_POWER_INFO_MAX];
	int pie_num;
	unsigned int err_flag;
	bool ufcs_online;
	bool ufcs_charging;
	bool oplus_ufcs_adapter;
	bool ufcs_disable;
	bool ufcs_not_allow;
	enum oplus_cp_work_mode cp_work_mode;
	int target_curr_ma;
	int target_vbus_mv;
	int curr_set_ma;
	int vol_set_mv;
	int bcc_max_curr;
	int bcc_min_curr;
	int bcc_exit_curr;
	int ufcs_current_change_timeout;
	int rmos_mohm;
	int cool_down;
	int curr_table_type;
	u32 adapter_id;

	bool wired_online;

	int ui_soc;
	int shell_temp;
	enum oplus_temp_region batt_temp_region;
	bool shell_temp_ready;

	int ufcs_fastchg_batt_temp_status;
	int ufcs_temp_cur_range;
	int ufcs_low_curr_full_temp_status;
};

struct current_level {
	int level;
	int curr_ma;
};

static const struct current_level g_ufcs_current_table[] = {
	{ 1, 1500 },   { 2, 1500 },   { 3, 2000 },   { 4, 2500 },   { 5, 3000 },  { 6, 3500 },   { 7, 4000 },
	{ 8, 4500 },   { 9, 5000 },   { 10, 5500 },  { 11, 6000 },  { 12, 6300 }, { 13, 6500 },  { 14, 7000 },
	{ 15, 7300 },  { 16, 8000 },  { 17, 8500 },  { 18, 9000 },  { 19, 9500 }, { 20, 10000 },  { 21, 10500 },
	{ 22, 11000 },  { 23, 11500 },  { 24, 12000 },  { 25, 12500 },  { 26, 13000 }, { 27, 13500 }, { 28, 14000 },
	{ 29, 14500 }, { 30, 15000 },
};

static const int ufcs_cool_down_oplus_curve[] = {
	15000, 1500,  1500,  2000,  2500,  3000,  3500,  4000,  4500,  5000,
	5500,  6000,  6300,  6500,  7000,  7300,  8000,  8500,  9000,  9500,
	10000, 10500, 11000, 11500, 12000, 12500, 13000, 13500, 14000, 14500, 15000
};

/* cp  batt curr from 3.6 spec table, at last it will convert to ibus and sent to cp voophy */
static const int ufcs_cp_cool_down_oplus_curve[] = {
	20000, 2000, 2000, 2400, 3000, 3400, 4000, 4400, 5000, 5400, 6000, 6400, 7000, 7400, 8000,
	9000, 10000, 11000, 12000, 12600, 13000, 14000, 15000, 16000, 17000, 18000, 19000, 20000
};

static const struct current_level g_ufcs_cp_current_table[] = {
	{ 1, 2000 },   { 2, 2000 },   { 3, 2400 },   { 4, 3000 },   { 5, 3400 },   { 6, 4000 },  { 7, 4400 },
	{ 8, 5000 },   { 9, 5400 },   { 10, 6000 },  { 11, 6400 },  { 12, 7000 },  { 13, 7400 }, { 14, 8000 },
	{ 15, 9000 },  { 16, 10000 }, { 17, 11000 }, { 18, 12000 }, { 19, 12600 }, { 20, 13000 }, { 21, 14000 },
	{ 22, 15000 }, { 23, 16000 }, { 24, 17000 }, { 25, 18000 }, { 26, 19000 }, { 27, 20000 },
};

static const char *const strategy_low_curr_full[] = {
	[UFCS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL] = "strategy_temp_little_cool",
	[UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW] = "strategy_temp_normal_low",
	[UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH] = "strategy_temp_normal_high",
};

static const char *const ufcs_user_err_type_str[] = {
	[UFCS_ERR_IBUS_LIMIT] = "ibus_limit",
	[UFCS_ERR_CP_ENABLE] = "cp_enable",
	[UFCS_ERR_R_COOLDOWN] = "r_cooldown",
	[UFCS_ERR_BATT_BTB_COOLDOWN] = "batbtb_cooldown",
	[UFCS_ERR_IBAT_OVER] = "ibat_over",
	[UFCS_ERR_BTB_OVER] = "btb_over",
	[UFCS_ERR_MOS_OVER] = "mos_over",
	[UFCS_ERR_USBTEMP_OVER] = "usbtemp_over",
	[UFCS_ERR_TFG_OVER] = "tfg_over",
	[UFCS_ERR_VBAT_DIFF] = "vbat_diff",
	[UFCS_ERR_STARTUP_FAIL] = "startup_fail",
	[UFCS_ERR_CIRCUIT_SWITCH] = "circuit_switch",
	[UFCS_ERR_ANTHEN_ERR] = "ahthen_err",
	[UFCS_ERR_PDO_ERR] = "pdo_err",
	[UFCS_ERR_IMP] = "imp",
};

__maybe_unused static bool
is_wired_suspend_votable_available(struct oplus_ufcs *chip)
{
	if (!chip->wired_suspend_votable)
		chip->wired_suspend_votable = find_votable("WIRED_CHARGE_SUSPEND");
	return !!chip->wired_suspend_votable;
}

__maybe_unused static bool
is_gauge_topic_available(struct oplus_ufcs *chip)
{
	if (!chip->gauge_topic)
		chip->gauge_topic = oplus_mms_get_by_name("gauge");
	return !!chip->gauge_topic;
}

__maybe_unused
static bool is_err_topic_available(struct oplus_ufcs *chip)
{
	if (!chip->err_topic)
		chip->err_topic = oplus_mms_get_by_name("error");
	return !!chip->err_topic;
}

__maybe_unused
static int ufcs_find_current_to_level(int val, const struct current_level * const table, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (table[i].level == val)
			return table[i].curr_ma;
	}
	return 0;
}

static int ufcs_find_level_to_current(int val, const struct current_level * const table, int len)
{
	int i;
	bool find_out_flag = false;

	for (i = 0; i < len; i++) {
		if (table[i].curr_ma > val) {
			find_out_flag = true;
			break;
		}
		find_out_flag = false;
	}
	if (find_out_flag == true && i != 0)
		return table[i - 1].level;
	return 0;
}

static enum oplus_cp_work_mode vbus_to_cp_work_mode(int vbus_mv)
{
	enum oplus_cp_work_mode mode;
	int batt_num;

#define BATT_VOL_MAX_MV	5000

	batt_num = oplus_gauge_get_batt_num();

	if (vbus_mv >= batt_num * BATT_VOL_MAX_MV) {
		switch (vbus_mv / (batt_num * BATT_VOL_MAX_MV)) {
		case 1:
			mode = CP_WORK_MODE_BYPASS;
			break;
		case 2:
			mode = CP_WORK_MODE_2_TO_1;
			break;
		case 3:
			mode = CP_WORK_MODE_3_TO_1;
			break;
		case 4:
			mode = CP_WORK_MODE_4_TO_1;
			break;
		default:
			mode = CP_WORK_MODE_UNKNOWN;
			break;
		}
	} else {
		/* TODO */
		mode = CP_WORK_MODE_UNKNOWN;
	}

	return mode;
}

__maybe_unused
static int32_t oplus_ufcs_get_curve_vbus(struct oplus_ufcs *chip)
{
	struct puc_strategy_ret_data data;
	int rc;

	if (chip->strategy == NULL)
		return -EINVAL;

	rc = oplus_chg_strategy_get_data(chip->strategy, &data);
	if (rc < 0) {
		chg_err("can't get curve vbus, rc=%d\n", rc);
		return rc;
	}

	return data.target_vbus;
}

__maybe_unused
static int32_t oplus_ufcs_get_curve_ibus(struct oplus_ufcs *chip)
{
	struct puc_strategy_ret_data data;
	int rc;

	if (chip->strategy == NULL)
		return -EINVAL;

	rc = oplus_chg_strategy_get_data(chip->strategy, &data);
	if (rc < 0) {
		chg_err("can't get curve ibus, rc=%d\n", rc);
		return rc;
	}

	return data.target_ibus;
}

static int oplus_ufcs_switch_to_normal(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->dpdm_switch == NULL) {
		chg_err("dpdm_switch ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->dpdm_switch,
		OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE, DPDM_SWITCH_TO_AP);

	return rc;
}

__maybe_unused
static int oplus_ufcs_switch_to_ufcs(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->dpdm_switch == NULL) {
		chg_err("dpdm_switch ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->dpdm_switch,
		OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE, DPDM_SWITCH_TO_UFCS);

	return rc;
}

static int oplus_ufcs_set_online(struct oplus_ufcs *chip, bool online)
{
	struct mms_msg *msg;
	int rc;

	if (chip->ufcs_online == online)
		return 0;

	chip->ufcs_online = online;
	chg_info("set ufcs_online=%s\n", online ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  UFCS_ITEM_ONLINE);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->ufcs_topic, msg);
	if (rc < 0) {
		chg_err("publish ufcs online msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_ufcs_set_charging(struct oplus_ufcs *chip, bool charging)
{
	struct mms_msg *msg;
	int rc;

	if (chip->ufcs_charging == charging)
		return 0;

	chip->ufcs_charging = charging;
	chg_info("set ufcs_charging=%s\n", charging ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  UFCS_ITEM_CHARGING);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->ufcs_topic, msg);
	if (rc < 0) {
		chg_err("publish ufcs charging msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_ufcs_set_adapter_id(struct oplus_ufcs *chip, u16 id)
{
	struct mms_msg *msg;
	int rc;

	if (chip->adapter_id == id)
		return 0;

	chip->adapter_id = id;
	chg_info("set adapter_id=%u\n", id);

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  UFCS_ITEM_ADAPTER_ID);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->ufcs_topic, msg);
	if (rc < 0) {
		chg_err("publish ufcs adapter id msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_ufcs_set_oplus_adapter(struct oplus_ufcs *chip, bool oplus_adapter)
{
	struct mms_msg *msg;
	int rc;

	if (chip->oplus_ufcs_adapter == oplus_adapter)
		return 0;

	chip->oplus_ufcs_adapter = oplus_adapter;
	chg_info("set oplus_ufcs_adapter=%s\n", oplus_adapter ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  UFCS_ITEM_OPLUS_ADAPTER);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->ufcs_topic, msg);
	if (rc < 0) {
		chg_err("publish oplus adapter msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static const char *oplus_ufcs_get_err_type_str(enum ufcs_user_err_type type)
{
	if (type < UFCS_ERR_IBUS_LIMIT || type >= UFCS_ERR_MAX)
		return "Inval";
	return ufcs_user_err_type_str[type];
}

static void oplus_ufcs_push_err_info(struct oplus_ufcs *chip, enum ufcs_user_err_type type, int value)
{
	struct mms_msg *msg;
	int rc;
	char *buf;
	int i;
	size_t index;

	if (!is_err_topic_available(chip))
		return;

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return;

	index = snprintf(buf, PAGE_SIZE, "$$err_reason@@%s$$value@@%d$$dev_info@@0x%llx"
		"$$cable_info@@0x%llx$$emark_info@@0x%llx",
		oplus_ufcs_get_err_type_str(type), value, chip->dev_info, chip->cable_info, chip->emark_info);
	if (chip->pdo_num > 0)
		index += snprintf(buf + index, PAGE_SIZE, "$$pdo_info@@");
	for (i = 0; i < chip->pdo_num; i++) {
		if (i == chip->pdo_num - 1)
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx", chip->pdo[i]);
		else
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx,", chip->pdo[i]);
	}

	if (chip->pie_num > 0)
		index += snprintf(buf + index, PAGE_SIZE, "$$pie_info@@");
	for (i = 0; i < chip->pie_num; i++) {
		if (i == chip->pie_num - 1)
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx", chip->pie[i]);
		else
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx,", chip->pie[i]);
	}
	index += snprintf(buf + index, PAGE_SIZE, "$$power_max@@%d",
		oplus_cpa_protocol_get_power(chip->cpa_topic, CHG_PROTOCOL_UFCS));

	msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_UFCS, buf);
	kfree(buf);
	rc = oplus_mms_publish_msg_sync(chip->err_topic, msg);
	if (rc < 0) {
		chg_err("publish usbtemp error msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

__maybe_unused
static int oplus_ufcs_handshake(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_HANDSHAKE);

	return rc;
}

__maybe_unused
static int oplus_ufcs_pdo_set(struct oplus_ufcs *chip, int vol_mv, int curr_ma)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_PDO_SET, vol_mv, curr_ma);
	if (rc == 0) {
		chip->curr_set_ma = curr_ma;
		chip->vol_set_mv = vol_mv;
	}

	return rc;
}

__maybe_unused
static int oplus_ufcs_hard_reset(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_HARD_RESET);

	return rc;
}

__maybe_unused
static int oplus_ufcs_exit_ufcs_mode(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_EXIT);
	if (rc < 0)
		chg_err("exit ufcs mode error, rc=%d", rc);

	return rc;
}

__maybe_unused
static int oplus_ufcs_config_wd(struct oplus_ufcs *chip, u16 time_ms)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_CONFIG_WD, time_ms);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_dev_info(struct oplus_ufcs *chip, u64 *dev_info)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_DEV_INFO, dev_info);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_err_info(struct oplus_ufcs *chip, u64 *err_info)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_ERR_INFO, err_info);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_src_info(struct oplus_ufcs *chip, u64 *src_info)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_SRC_INFO, src_info);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_cable_info(struct oplus_ufcs *chip, u64 *cable_info)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_CABLE_INFO, cable_info);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_pdo_info(struct oplus_ufcs *chip, u64 *pdo, int num)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_PDO_INFO, pdo, num);

	return rc;
}

static int oplus_ufcs_verify_adapter(struct oplus_ufcs *chip,
	u8 index, u8 *auth_data, u8 data_len)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_VERIFY_ADAPTER,
		index, auth_data, data_len);

	return rc;
}

__maybe_unused
static int oplus_ufcs_get_power_change_info(
	struct oplus_ufcs *chip, u32 *pwr_change_info, int num)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_POWER_CHANGE_INFO,
		pwr_change_info, num);

	return rc;
}

__maybe_unused
static int oplus_chg_ufcs_get_emark_info(struct oplus_ufcs *chip, u64 *info)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_EMARK_INFO, info);
	chg_err("emark_info=%llx\n", *info);

	return rc;
}

__maybe_unused
static int oplus_chg_ufcs_get_power_info_ext(struct oplus_ufcs *chip, u64 *info, int num)
{
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_GET_POWER_INFO_EXT, info, num);

	return rc;
}

__maybe_unused
static bool oplus_chg_ufcs_is_test_mode(struct oplus_ufcs *chip)
{
	bool en;
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return false;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_IS_TEST_MODE, &en);
	if (rc < 0) {
		chg_err("can't get test mode info, rc=%d\n", rc);
		return false;
	}

	return en;
}

__maybe_unused
static bool oplus_chg_ufcs_is_vol_acc_test_mode(struct oplus_ufcs *chip)
{
	bool en;
	int rc;

	if (chip->ufcs_ic == NULL) {
		chg_err("ufcs_ic is NULL\n");
		return false;
	}

	rc = oplus_chg_ic_func(chip->ufcs_ic, OPLUS_IC_FUNC_UFCS_IS_VOL_ACC_TEST_MODE, &en);
	if (rc < 0) {
		chg_err("can't get vol acc test mode info, rc=%d\n", rc);
		return false;
	}

	return en;
}

__maybe_unused
static int oplus_ufcs_cp_enable(struct oplus_ufcs *chip, bool en)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_ENABLE, en);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_hw_init(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_HW_INTI);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_set_work_mode(struct oplus_ufcs *chip, enum oplus_cp_work_mode mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_WORK_MODE, mode);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_work_mode(struct oplus_ufcs *chip, enum oplus_cp_work_mode *mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_WORK_MODE, mode);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_check_work_mode_support(struct oplus_ufcs *chip, enum oplus_cp_work_mode mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT, mode);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_set_iin(struct oplus_ufcs *chip, int iin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_IIN, iin);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_vin(struct oplus_ufcs *chip, int *vin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VIN, vin);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_iin(struct oplus_ufcs *chip, int *iin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IIN, iin);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_vout(struct oplus_ufcs *chip, int *vout)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VOUT, vout);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_iout(struct oplus_ufcs *chip, int *iout)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IOUT, iout);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_vac(struct oplus_ufcs *chip, int *vac)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VAC, vac);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_set_work_start(struct oplus_ufcs *chip, bool start)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_WORK_START, start);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_get_work_status(struct oplus_ufcs *chip, bool *start)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_WORK_STATUS, start);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_adc_enable(struct oplus_ufcs *chip, bool en)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_ADC_ENABLE, en);

	return rc;
}

__maybe_unused
static int oplus_ufcs_cp_reg_dump(struct oplus_ufcs *chip)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_REG_DUMP);

	return rc;
}

static bool oplus_ufcs_charge_allow_check(struct oplus_ufcs *chip)
{
	if (chip->shell_temp < chip->limits.ufcs_low_temp ||
	    chip->shell_temp > chip->limits.ufcs_high_temp)
		vote(chip->ufcs_not_allow_votable, BATT_TEMP_VOTER, true, 1, false);
	else
		vote(chip->ufcs_not_allow_votable, BATT_TEMP_VOTER, false, 0, false);

	if (chip->ui_soc < chip->limits.ufcs_low_soc ||
	    chip->ui_soc > chip->limits.ufcs_high_soc)
		vote(chip->ufcs_not_allow_votable, BATT_SOC_VOTER, true, 1, false);
	else
		vote(chip->ufcs_not_allow_votable, BATT_SOC_VOTER, false, 0, false);

	return !chip->ufcs_not_allow;
}

static void oplus_ufcs_count_init(struct oplus_ufcs *chip)
{
	chip->count.cool_fw = 0;
	chip->count.sw_full = 0;
	chip->count.hw_full = 0;
	chip->count.low_curr_full = 0;
	chip->count.ibat_low = 0;
	chip->count.ibat_high = 0;
	chip->count.btb_high = 0;
	chip->count.tbatt_over = 0;
	chip->count.tfg_over = 0;
	chip->count.output_low = 0;
	chip->count.ibus_over = 0;
}

static void oplus_ufcs_votable_reset(struct oplus_ufcs *chip)
{
	vote(chip->ufcs_disable_votable, NO_DATA_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, IBAT_OVER_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, BTB_TEMP_OVER_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, BATT_TEMP_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, SWITCH_RANGE_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, TFG_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, IMP_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, TIMEOUT_VOTER, false, 0, false);
	vote(chip->ufcs_disable_votable, IOUT_CURR_VOTER, false, 0, false);

	vote(chip->ufcs_curr_votable, IMP_VOTER, false, 0, false);
	vote(chip->ufcs_curr_votable, STEP_VOTER, false, 0, false);
	vote(chip->ufcs_curr_votable, BATT_TEMP_VOTER, false, 0, false);
	vote(chip->ufcs_curr_votable, COOL_DOWN_VOTER, false, 0, false);
	vote(chip->ufcs_curr_votable, BCC_VOTER, false, 0, false);
}

static int oplus_ufcs_temp_cur_range_init(struct oplus_ufcs *chip)
{
	int vbat_temp_cur;

	vbat_temp_cur = chip->shell_temp;
	if (vbat_temp_cur < chip->limits.ufcs_little_cold_temp) { /*0-5C*/
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COLD;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_LITTLE_COLD;
	} else if (vbat_temp_cur < chip->limits.ufcs_cool_temp) { /*5-12C*/
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_COOL;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_COOL;
	} else if (vbat_temp_cur < chip->limits.ufcs_little_cool_temp) { /*12-20C*/
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COOL;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_LITTLE_COOL;
	} else if (vbat_temp_cur < chip->limits.ufcs_normal_low_temp) { /*20-35C*/
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_LOW;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NORMAL_LOW;
	} else if (vbat_temp_cur < chip->limits.ufcs_normal_high_temp) { /*35C-43C*/
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_HIGH;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NORMAL_HIGH;
	} else {
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_WARM;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_WARM;
	}

	return 0;
}

static void oplus_ufcs_variables_early_init(struct oplus_ufcs *chip)
{
	oplus_ufcs_count_init(chip);

	chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NATURAL;
	chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_INIT;

	chip->src_info = 0;
	chip->dev_info = 0;
	chip->emark_info = 0;
	chip->cable_info = 0;
	chip->pdo_num = 0;
	chip->pie_num = 0;
}

static void oplus_ufcs_variables_init(struct oplus_ufcs *chip)
{
	chip->timer.fastchg_timer = oplus_current_kernel_time();
	chip->timer.temp_timer = oplus_current_kernel_time();
	chip->timer.pdo_timer = oplus_current_kernel_time();
	chip->timer.ibat_timer = oplus_current_kernel_time();
	if (!chip->oplus_ufcs_adapter)
		chip->timer.ufcs_max_time_ms = chip->limits.ufcs_timeout_third * 1000;
	else
		chip->timer.ufcs_max_time_ms = chip->limits.ufcs_timeout_oplus * 1000;

	chip->ufcs_low_curr_full_temp_status =
		UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW;

	(void)oplus_imp_uint_reset(chip->imp_uint);
}

static void oplus_ufcs_force_exit(struct oplus_ufcs *chip)
{
	oplus_ufcs_set_charging(chip, false);
	oplus_ufcs_set_oplus_adapter(chip, false);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
	oplus_ufcs_cp_set_work_start(chip, false);
	oplus_ufcs_exit_ufcs_mode(chip);
	msleep(UFCS_STOP_DELAY_TIME);
	oplus_ufcs_cp_enable(chip, false);
	oplus_ufcs_cp_adc_enable(chip, false);
	oplus_ufcs_switch_to_normal(chip);
	oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_UFCS);
	oplus_ufcs_set_adapter_id(chip, 0);
	oplus_ufcs_set_online(chip, false);
	vote(chip->ufcs_curr_votable, STEP_VOTER, false, 0, false);
	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, UFCS_VOTER, false, 0, false);
}

static void oplus_ufcs_soft_exit(struct oplus_ufcs *chip)
{
	oplus_ufcs_set_charging(chip, false);
	oplus_ufcs_set_oplus_adapter(chip, false);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
	oplus_ufcs_cp_set_work_start(chip, false);
	oplus_ufcs_exit_ufcs_mode(chip);
	msleep(UFCS_STOP_DELAY_TIME);
	oplus_ufcs_cp_enable(chip, false);
	oplus_ufcs_cp_adc_enable(chip, false);
	oplus_ufcs_switch_to_normal(chip);
	vote(chip->ufcs_curr_votable, STEP_VOTER, false, 0, false);
	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, UFCS_VOTER, false, 0, false);
}

static int oplus_ufcs_get_verify_data(struct oplus_ufcs *chip, int index)
{
	if (index < 1) {
		chg_err("index(=%d) error\n", index);
		return -EINVAL;
	}
	/* The index of the hal layer starts from 0 */
	index -= 1;

	mutex_lock(&chip->cmd_data_lock);
	memset(&chip->cmd, 0, sizeof(struct ufcs_dev_cmd));
	memset(chip->auth_data, 0, UFCS_VERIFY_AUTH_DATA_SIZE);
	chip->cmd.cmd = UFCS_DEV_CMD_GET_AUTH_DATA;
	chip->cmd.data_size = sizeof(index);
	*(int *)chip->cmd.data_buf = cpu_to_le32(index);
	chip->cmd_data_ok = true;
	mutex_unlock(&chip->cmd_data_lock);
	reinit_completion(&chip->cmd_ack);
	wake_up(&chip->read_wq);

	return 0;
}

static int oplus_ufcs_exit_daemon_process(struct oplus_ufcs *chip)
{
	mutex_lock(&chip->cmd_data_lock);
	memset(&chip->cmd, 0, sizeof(struct ufcs_dev_cmd));
	chip->cmd.cmd = UFCS_DEV_CMD_EXIT;
	chip->cmd.data_size = 0;
	chip->cmd_data_ok = true;
	mutex_unlock(&chip->cmd_data_lock);
	reinit_completion(&chip->cmd_ack);
	wake_up(&chip->read_wq);

	return 0;
}

static void oplus_ufcs_wait_auth_data_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_ufcs *chip =
		container_of(dwork, struct oplus_ufcs, wait_auth_data_work);

	chip->wait_auth_data_done = true;
	vote(chip->ufcs_boot_votable, AUTH_VOTER, false, 0, false);
	(void)oplus_ufcs_exit_daemon_process(chip);
}

static int oplus_ufcs_deal_emark_info(struct oplus_ufcs *chip)
{
	int rc;

	rc = oplus_chg_ufcs_get_emark_info(chip, &chip->emark_info);
	if (rc < 0) {
		chg_err("can't get emark info, rc=%d\n", rc);
		chip->emark_info = 0;
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_DEFAULT, false);
		return rc;
	}

	chg_info("emark: cable_type=%s, hw_ver=%llu, curr_max=%llu mA, vol_max=%llumV, \n",
		 ufcs_get_oplus_emark_info_str(UFCS_OPLUS_VND_EMARK_INFO(chip->emark_info)),
		 UFCS_OPLUS_VND_EMARK_HW_VER(chip->emark_info),
		 UFCS_OPLUS_VND_EMARK_CURR_MAX(chip->emark_info),
		 UFCS_OPLUS_VND_EMARK_VOL_MAX(chip->emark_info));

	switch (UFCS_OPLUS_VND_EMARK_INFO(chip->emark_info)) {
	case UFCS_OPLUS_EMARK_INFO_ERR:
	case UFCS_OPLUS_EMARK_INFO_OTHER:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_DEFAULT, false);
		return 0;
	case UFCS_OPLUS_EMARK_INFO_OTHER_CABLE:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_THIRD, false);
		return 0;
	case UFCS_OPLUS_EMARK_INFO_OPLUS_CABLE:
		if (chip->config.target_vbus_mv > UFCS_OPLUS_VND_EMARK_VOL_MAX(chip->emark_info)) {
			chg_err("the max withstand voltage of the cable is lower than the target_vbus");
			return -EFAULT;
		}
		break;
	default:
		chg_err("unknown emark info type\n");
		return -EFAULT;
	}

	switch (UFCS_OPLUS_VND_EMARK_HW_VER(chip->emark_info)) {
	case UFCS_EMARK_ID_V0:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_V0, false);
		break;
	case UFCS_EMARK_ID_V1:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_V1, false);
		break;
	case UFCS_EMARK_ID_V2:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_V2, false);
		break;
	case UFCS_EMARK_ID_V3:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_V3, false);
		break;
	default:
		vote(chip->ufcs_curr_votable, CABLE_MAX_VOTER, true, UFCS_EMARK_POWER_V0, false);
		break;
	}

	return 0;
}

static int oplus_ufcs_deal_power_info(struct oplus_ufcs *chip)
{
	int rc;
	int i;
	int power_mw;
	enum oplus_chg_protocol_type type;

	rc = oplus_chg_ufcs_get_power_info_ext(chip, chip->pie, UFCS_OPLUS_VND_POWER_INFO_MAX);
	if (rc < 0) {
		chg_err("ufcs get power info ext error\n");
		chip->pie_num = 0;
		return rc;
	} else {
		chip->pie_num = rc;
	}

	for (i = 0; i < chip->pie_num; i++) {
		chg_info("power_info[%d]: index=%llu, type=%s, curr_max=%llu, vol_max=%llu\n", i,
			 UFCS_OPLUS_VND_POWER_INFO_INDEX(chip->pie[i]),
			 ufcs_get_oplus_power_info_type_str(UFCS_OPLUS_VND_POWER_INFO_TYPE(chip->pie[i])),
			 UFCS_OPLUS_VND_POWER_INFO_CURR_MAX(chip->pie[i]),
			 UFCS_OPLUS_VND_POWER_INFO_VOL_MAX(chip->pie[i]));

		power_mw = UFCS_OPLUS_VND_POWER_INFO_CURR_MAX(chip->pie[i]) *
			UFCS_OPLUS_VND_POWER_INFO_VOL_MAX(chip->pie[i]) / 1000;
		switch (UFCS_OPLUS_VND_POWER_INFO_TYPE(chip->pie[i])) {
		case UFCS_OPLUS_POWER_INFO_UFCS:
			rc = oplus_cpa_protocol_get_power(chip->cpa_topic, CHG_PROTOCOL_UFCS);
			if (rc < 0) {
				chg_err("can't get ufcs protocol power info\n");
				continue;
			}
			if (power_mw <= rc)
				continue;
			rc = oplus_cpa_protocol_set_power(chip->cpa_topic, CHG_PROTOCOL_UFCS, power_mw);
			if (rc < 0) {
				chg_err("can't set ufcs protocol power data\n");
				continue;
			}
			break;
		case UFCS_OPLUS_POWER_INFO_PPS:
			rc = oplus_cpa_protocol_get_power(chip->cpa_topic, CHG_PROTOCOL_PPS);
			if (rc < 0) {
				chg_err("can't get pps protocol power info\n");
				continue;
			}
			if (power_mw <= rc)
				continue;
			rc = oplus_cpa_protocol_set_power(chip->cpa_topic, CHG_PROTOCOL_PPS, power_mw);
			if (rc < 0) {
				chg_err("can't set pps protocol power data\n");
				continue;
			}
			break;
		case UFCS_OPLUS_POWER_INFO_SVOOC:
			rc = oplus_cpa_protocol_get_power(chip->cpa_topic, CHG_PROTOCOL_VOOC);
			if (rc < 0) {
				chg_err("can't get vooc protocol power info\n");
				continue;
			}
			if (power_mw <= rc)
				continue;
			rc = oplus_cpa_protocol_set_power(chip->cpa_topic, CHG_PROTOCOL_VOOC, power_mw);
			if (rc < 0) {
				chg_err("can't set vooc protocol power data\n");
				continue;
			}
			break;
		default:
			chg_err("unknown power info type\n");
			continue;
		}
	}

	type = oplus_cpa_curr_high_prio_protocol_type(chip->cpa_topic);
	if (type == CHG_PROTOCOL_INVALID || type == CHG_PROTOCOL_UFCS)
		return 0;

	/*
	 * Need to switch to a higher priority protocol, so ufcs needs to
	 * re-request. The return value here should be -EBUSY.
	 */
	oplus_cpa_request(chip->cpa_topic, CHG_PROTOCOL_UFCS);

	return -EBUSY;
}

static void oplus_ufcs_switch_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_ufcs *chip =
		container_of(dwork, struct oplus_ufcs, switch_check_work);
	enum oplus_cp_work_mode cp_mode;
	int rc;
	int i;
	bool pdo_ok = false;
	int max_curr = 0;

	oplus_ufcs_set_charging(chip, false);
	oplus_cpa_switch_start(chip->cpa_topic, CHG_PROTOCOL_UFCS);

	oplus_ufcs_variables_early_init(chip);
	oplus_ufcs_votable_reset(chip);
	if (chip->ufcs_disable) {
		oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_UFCS);
		return;
	}

	if (chip->ufcs_ic == NULL || chip->cp_ic == NULL || !is_gauge_topic_available(chip)) {
		oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_UFCS);
		return;
	}

	oplus_ufcs_switch_to_ufcs(chip);
	rc = oplus_ufcs_handshake(chip);
	if (rc < 0) {
		chg_err("ufcs handshake error\n");
		goto err;
	}
	chg_info("ufcs handshake success\n");

	rc = oplus_ufcs_get_dev_info(chip, &chip->dev_info);
	if (rc < 0) {
		chg_err("ufcs get dev info error\n");
		goto err;
	}
	chg_info("dev_info=0x%llx\n", chip->dev_info);
	oplus_ufcs_set_adapter_id(chip, UFCS_DEVICE_INFO_HW_VER(chip->dev_info));

	if (oplus_chg_ufcs_is_test_mode(chip)) {
		chg_info("test mode\n");
		oplus_ufcs_set_online(chip, true);
		return;
	}

	rc = oplus_ufcs_config_wd(chip, UFCS_WATCHDOG_TIME_MS);
	if (rc < 0) {
		chg_err("ufcs config watchdog error\n");
		goto err;
	}

	if (UFCS_DEVICE_INFO_IC_VENDOR(chip->dev_info) == 0) {
		if (UFCS_DEVICE_INFO_DEV_VENDOR(chip->dev_info) == UFCS_OPLUS_DEV_ID) {
			if (unlikely(!chip->auth_data_ok)) {
				chg_err("auth data not ready");
			} else {
				rc = oplus_ufcs_verify_adapter(chip, 1, chip->auth_data, UFCS_VERIFY_AUTH_DATA_SIZE);
				if (rc < 0) {
					chg_err("adapter verify error, rc=%d\n", rc);
					oplus_ufcs_push_err_info(chip, UFCS_ERR_ANTHEN_ERR, 0);
					goto next;
				} else if (!!rc) {
					chg_info("adapter verify pass\n");
					oplus_ufcs_set_oplus_adapter(chip, true);
				} else {
					chg_err("adapter verify fail\n");
					oplus_ufcs_push_err_info(chip, UFCS_ERR_ANTHEN_ERR, 0);
					goto next;
				}
			}
		} else if (UFCS_DEVICE_INFO_DEV_VENDOR(chip->dev_info) == 0) {
			chg_err("abnormal adapter id");
		}
	}
	chg_info("oplus_ufcs_adapter=%s\n", chip->oplus_ufcs_adapter ? "true" : "false");

	if (chip->oplus_ufcs_adapter) {
		rc = oplus_ufcs_deal_power_info(chip);
		if (rc < 0)
			goto next;
		rc = oplus_ufcs_deal_emark_info(chip);
		if (rc < 0)
			goto err;
	}
	oplus_ufcs_set_online(chip, true);

	rc = oplus_ufcs_get_pdo_info(chip, chip->pdo, UFCS_OUTPUT_MODE_MAX);
	if (rc < 0) {
		chg_err("ufcs get pdo info error\n");
		goto err;
	}
	chip->pdo_num = rc;
	for (i = 0; i < chip->pdo_num; i++)
		chg_info("pdo[%d]: voltage[%llu-%llumV, step:%llumV], current[%llu-%llumA, step:%llumA]\n", i,
			 UFCS_OUTPUT_MODE_VOL_MIN(chip->pdo[i]), UFCS_OUTPUT_MODE_VOL_MAX(chip->pdo[i]),
			 UFCS_OUTPUT_MODE_VOL_STEP(chip->pdo[i]), UFCS_OUTPUT_MODE_CURR_MIN(chip->pdo[i]),
			 UFCS_OUTPUT_MODE_CURR_MAX(chip->pdo[i]), UFCS_OUTPUT_MODE_CURR_STEP(chip->pdo[i]));
	for (i = 0; i < chip->pdo_num; i++) {
		int target_vbus = chip->config.target_vbus_mv;
		if (i > 0) {
			if (UFCS_OUTPUT_MODE_VOL_MIN(chip->pdo[i]) !=
			    UFCS_OUTPUT_MODE_VOL_MAX(chip->pdo[i - 1])) {
				chg_err("the output voltage range is discontinuous\n");
				break;
			}
		} else {
			if (UFCS_OUTPUT_MODE_VOL_MIN(chip->pdo[0]) > 5000) {
				chg_err("The output voltage does not support 5V\n");
				break;
			}
		}
		if (target_vbus <= UFCS_OUTPUT_MODE_VOL_MAX(chip->pdo[i]) &&
		    target_vbus >= UFCS_OUTPUT_MODE_VOL_MIN(chip->pdo[i])) {
			pdo_ok = true;
			if (UFCS_OUTPUT_MODE_CURR_MAX(chip->pdo[i]) > max_curr)
				max_curr = UFCS_OUTPUT_MODE_CURR_MAX(chip->pdo[i]);
		}
	}
	if (!pdo_ok) {
		chg_err("The pdo range of the current adapter does not support UFCS charging\n");
		oplus_ufcs_push_err_info(chip, UFCS_ERR_PDO_ERR, 0);
		goto err;
	}
	vote(chip->ufcs_curr_votable, BASE_MAX_VOTER, true, max_curr, false);

/* TODO
	rc = oplus_ufcs_get_cable_info(chip, &chip->cable_info);
	if (rc < 0) {
		chg_err("can't get cable info, rc=%d\n", rc);
		chip->cable_info = 0;
	} else {
		chg_err("cable_info=0x%llx\n", chip->cable_info);
	}
*/

	oplus_ufcs_variables_init(chip);
	if (!oplus_ufcs_charge_allow_check(chip)) {
		chg_info("ufcs charge not allow, exit ufcs mode\n");
		goto exit;
	}

	cp_mode = vbus_to_cp_work_mode(chip->config.target_vbus_mv);
	if (cp_mode == CP_WORK_MODE_UNKNOWN) {
		chg_err("can't find a suitable CP working mode\n");
		goto err;
	}
	rc = oplus_ufcs_cp_check_work_mode_support(chip, cp_mode);
	if (rc <= 0) {
		chg_err("cp not support %d mode, rc=%d\n", cp_mode, rc);
		goto err;
	}
	rc = oplus_ufcs_cp_set_work_mode(chip, cp_mode);
	if (rc < 0) {
		chg_err("cp set %d mode error, rc=%d\n", cp_mode, rc);
		goto err;
	}
	chip->cp_work_mode = cp_mode;

	oplus_ufcs_cp_adc_enable(chip, true);

	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, UFCS_VOTER, true, 1, false);

	rc = oplus_ufcs_pdo_set(chip, UFCS_START_DEF_VOL_MV, UFCS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("pdo set error, rc=%d\n", rc);
		goto err;
	}

	schedule_delayed_work(&chip->monitor_work, 0);

	return;
err:
	chg_err("error, ufcs exit\n");
next:
	oplus_ufcs_force_exit(chip);
	return;

exit:
	oplus_ufcs_exit_ufcs_mode(chip);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
}

enum {
	OPLUS_UFCS_VOLT_UPDATE_V1 = 100,
	OPLUS_UFCS_VOLT_UPDATE_V2 = 200,
	OPLUS_UFCS_VOLT_UPDATE_V3 = 500,
	OPLUS_UFCS_VOLT_UPDATE_V4 = 1000,
	OPLUS_UFCS_VOLT_UPDATE_V5 = 2000,
	OPLUS_UFCS_VOLT_UPDATE_V6 = 5000,
};

static int oplus_ufcs_charge_start(struct oplus_ufcs *chip)
{
	union mms_msg_data data = { 0 };
	int vbat_mv;
	int rc;
	int target_vbus, update_size, req_vol;
	int cp_vin;
	static int retry_count = 0;
	static bool start_check = false;
	int batt_num;

#define UFCS_START_VOL_THR_4_TO_1_MV	600
#define UFCS_START_VOL_THR_3_TO_1_MV	400
#define UFCS_START_VOL_THR_2_TO_1_MV	200
#define UFCS_START_VOL_THR_1_TO_1_MV	300
#define UFCS_START_RETAY_MAX		3
#define UFCS_START_CHECK_DELAY_MS	500
#define UFCS_START_PDO_DELAY_MS		50
#define ADSP_UFCS_START_PDO_DELAY_MS	100

	batt_num = oplus_gauge_get_batt_num();
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}
	vbat_mv = data.intval * batt_num;

	switch (chip->cp_work_mode) {
	case CP_WORK_MODE_4_TO_1:
		target_vbus = ((vbat_mv * 4) / 100) * 100 + UFCS_START_VOL_THR_4_TO_1_MV;
		break;
	case CP_WORK_MODE_3_TO_1:
		target_vbus = ((vbat_mv * 3) / 100) * 100 + UFCS_START_VOL_THR_3_TO_1_MV;
		break;
	case CP_WORK_MODE_2_TO_1:
		target_vbus = ((vbat_mv * 2) / 100) * 100 + UFCS_START_VOL_THR_2_TO_1_MV;
		break;
	case CP_WORK_MODE_BYPASS:
		target_vbus = (vbat_mv / 100) * 100 + UFCS_START_VOL_THR_1_TO_1_MV;
		break;
	default:
		chg_err("unsupported cp work mode, mode=%d\n", chip->cp_work_mode);
		return -ENOTSUPP;
	}

	rc = oplus_ufcs_cp_get_vin(chip, &cp_vin);
	if (rc < 0) {
		chg_err("can't get cp input voltage, rc=%d\n", rc);
		return rc;
	}
	if ((cp_vin >= target_vbus && cp_vin <= (target_vbus + OPLUS_UFCS_VOLT_UPDATE_V2)) || start_check) {
		if (start_check) {
			bool work_start;

			rc = oplus_ufcs_cp_get_work_status(chip, &work_start);
			if (rc < 0) {
				chg_err("can't get cp work status, rc=%d\n", rc);
			} else {
				if (work_start) {
					if (chip->oplus_ufcs_adapter)
						chip->strategy = chip->oplus_curve_strategy;
					else
						chip->strategy = chip->third_curve_strategy;
					rc = oplus_chg_strategy_init(chip->strategy);
					if (rc < 0) {
						chg_err("strategy_init error, not support ufcs fast charge\n");
						return rc;
					}
					rc = oplus_ufcs_temp_cur_range_init(chip);
					if (rc < 0)
						return rc;

					retry_count = 0;
					start_check = false;
					oplus_ufcs_set_charging(chip, true);
					chip->target_vbus_mv = chip->config.target_vbus_mv;
					chip->timer.monitor_jiffies = jiffies;
					schedule_delayed_work(&chip->current_work, 0);
					oplus_ufcs_cp_reg_dump(chip);
					return 0;
				}
				chg_err("cp not work, retry=%d\n", retry_count);
			}
			if (retry_count >= UFCS_START_RETAY_MAX) {
				retry_count = 0;
				start_check = false;
				oplus_ufcs_cp_set_work_start(chip, false);
				oplus_ufcs_cp_enable(chip, false);
				oplus_ufcs_cp_adc_enable(chip, false);
				oplus_ufcs_cp_reg_dump(chip);
				chg_err("cp not working\n");
				return -EFAULT;
			}
			retry_count++;
			return UFCS_START_CHECK_DELAY_MS;
		}
		rc = oplus_ufcs_cp_enable(chip, true);
		if (rc < 0) {
			chg_err("set cp enable error, rc=%d\n", rc);
			return rc;
		}
		rc = oplus_ufcs_cp_set_work_start(chip, true);
		if (rc < 0) {
			chg_err("set cp work start error, rc=%d\n", rc);
			return rc;
		}
		retry_count = 0;
		start_check = true;

		return UFCS_START_CHECK_DELAY_MS;
	}

	retry_count = 0;
	start_check = false;

	if (chip->vol_set_mv > chip->config.target_vbus_mv) {
		chg_err("the requested voltage has reached the max value, exit ufcs\n");
		return -EFAULT;
	}

	if (abs(cp_vin - target_vbus) >= OPLUS_UFCS_VOLT_UPDATE_V5)
		update_size = OPLUS_UFCS_VOLT_UPDATE_V5;
	else if (abs(cp_vin - target_vbus) >= OPLUS_UFCS_VOLT_UPDATE_V4)
		update_size = OPLUS_UFCS_VOLT_UPDATE_V4;
	else if (abs(cp_vin - target_vbus) >= OPLUS_UFCS_VOLT_UPDATE_V3)
		update_size = OPLUS_UFCS_VOLT_UPDATE_V3;
	else if (abs(cp_vin - target_vbus) >= OPLUS_UFCS_VOLT_UPDATE_V2)
		update_size = OPLUS_UFCS_VOLT_UPDATE_V2;
	else
		update_size = OPLUS_UFCS_VOLT_UPDATE_V1;

	req_vol = chip->vol_set_mv;
	if (cp_vin < target_vbus)
		req_vol += update_size;
	else
		req_vol -= update_size;
	if (req_vol > chip->config.target_vbus_mv)
		req_vol = chip->config.target_vbus_mv;
	chg_info("cp_vin=%d, target_vbus=%d, update_size=%d, req_vol=%d\n",
		 cp_vin, target_vbus, update_size, req_vol);

	rc = oplus_ufcs_cp_set_iin(chip, UFCS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("set cp input current error, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_ufcs_pdo_set(chip, req_vol, UFCS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("pdo set error, rc=%d\n", rc);
		return rc;
	}

	if (chip->config.adsp_ufcs_project)
		return ADSP_UFCS_START_PDO_DELAY_MS;
	else
		return UFCS_START_PDO_DELAY_MS;
}

static void oplus_ufcs_reset_temp_range(struct oplus_ufcs *chip)
{
	chip->limits.ufcs_normal_high_temp =
		chip->limits.default_ufcs_normal_high_temp;
	chip->limits.ufcs_little_cold_temp =
		chip->limits.default_ufcs_little_cold_temp;
	chip->limits.ufcs_cool_temp = chip->limits.default_ufcs_cool_temp;
	chip->limits.ufcs_little_cool_temp =
		chip->limits.default_ufcs_little_cool_temp;
	chip->limits.ufcs_normal_low_temp =
		chip->limits.default_ufcs_normal_low_temp;
}

static int oplus_ufcs_set_current_warm_range(struct oplus_ufcs *chip,
					     int vbat_temp_cur)
{
	int ret = chip->limits.ufcs_strategy_normal_current;

	if (chip->limits.ufcs_batt_over_high_temp != -EINVAL &&
	    vbat_temp_cur > chip->limits.ufcs_batt_over_high_temp) {
		chip->limits.ufcs_strategy_change_count++;
		if (chip->limits.ufcs_strategy_change_count >=
		    UFCS_TEMP_OVER_COUNTS) {
			chip->limits.ufcs_strategy_change_count = 0;
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_EXIT;
			ret = chip->limits.ufcs_over_high_or_low_current;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur < chip->limits.ufcs_normal_high_temp) {
		chip->limits.ufcs_strategy_change_count++;
		if (chip->limits.ufcs_strategy_change_count >=
		    UFCS_TEMP_OVER_COUNTS) {
			chip->limits.ufcs_strategy_change_count = 0;
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_SWITCH_CURVE;
			chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_INIT;
			ret = chip->limits.ufcs_strategy_normal_current;
			oplus_ufcs_reset_temp_range(chip);
			(void)oplus_chg_strategy_init(chip->strategy);
			chip->limits.ufcs_normal_high_temp +=
				UFCS_TEMP_WARM_RANGE_THD;
			chg_err("switch temp range:%d", vbat_temp_cur);
		}

	} else {
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_WARM;
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_WARM;
		ret = chip->limits.ufcs_strategy_normal_current;
	}

	return ret;
}

static int
oplus_ufcs_set_current_temp_normal_range(struct oplus_ufcs *chip,
					 int vbat_temp_cur)
{
	int ret = chip->limits.ufcs_strategy_normal_current;
	int batt_soc, batt_vol;
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return -ENODEV;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return rc;
	}
	batt_vol = data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data, false);
	if (rc < 0) {
		chg_err("can't get soc, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return rc;
	}
	batt_soc = data.intval;

	switch (chip->ufcs_fastchg_batt_temp_status) {
	case UFCS_BAT_TEMP_NORMAL_HIGH:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp0) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH0;
			ret = chip->limits.ufcs_strategy_high_current0;
		} else if (vbat_temp_cur >= chip->limits.ufcs_normal_low_temp) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.ufcs_strategy_normal_current;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_NORMAL_LOW;
			chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_LOW;
			ret = chip->limits.ufcs_strategy_normal_current;
			oplus_ufcs_reset_temp_range(chip);
			chip->limits.ufcs_normal_low_temp +=
				UFCS_TEMP_WARM_RANGE_THD;
		}
		break;
	case UFCS_BAT_TEMP_HIGH0:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp1) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH1;
			ret = chip->limits.ufcs_strategy_high_current1;
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp0) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW0;
			ret = chip->limits.ufcs_strategy_low_current0;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH0;
			ret = chip->limits.ufcs_strategy_high_current0;
		}
		break;
	case UFCS_BAT_TEMP_HIGH1:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp2) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH2;
			ret = chip->limits.ufcs_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp1) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW1;
			ret = chip->limits.ufcs_strategy_low_current1;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH1;
			ret = chip->limits.ufcs_strategy_high_current1;
		}
		break;
	case UFCS_BAT_TEMP_HIGH2:
		if (chip->limits.ufcs_normal_high_temp != -EINVAL &&
		    vbat_temp_cur > chip->limits.ufcs_normal_high_temp) {
			chip->limits.ufcs_strategy_change_count++;
			if (chip->limits.ufcs_strategy_change_count >=
			    UFCS_TEMP_OVER_COUNTS) {
				chip->limits.ufcs_strategy_change_count = 0;
				if (batt_soc < chip->limits.ufcs_warm_allow_soc &&
				    batt_vol < chip->limits.ufcs_warm_allow_vol) {
					chip->ufcs_fastchg_batt_temp_status =
						UFCS_BAT_TEMP_SWITCH_CURVE;
					chip->ufcs_temp_cur_range =
						UFCS_TEMP_RANGE_INIT;
					ret = chip->limits.ufcs_strategy_high_current2;
					oplus_ufcs_reset_temp_range(chip);
					(void)oplus_chg_strategy_init(chip->strategy);
					chip->limits.ufcs_normal_high_temp -=
						UFCS_TEMP_WARM_RANGE_THD;
					chg_err("switch temp range:%d", vbat_temp_cur);
				} else {
					chg_err("high temp_over:%d", vbat_temp_cur);
					chip->ufcs_fastchg_batt_temp_status =
						UFCS_BAT_TEMP_EXIT;
					ret = chip->limits
						      .ufcs_over_high_or_low_current;
				}
			}
		} else if (chip->limits.ufcs_batt_over_high_temp != -EINVAL &&
			   vbat_temp_cur >
				   chip->limits.ufcs_batt_over_high_temp) {
			chip->limits.ufcs_strategy_change_count++;
			if (chip->limits.ufcs_strategy_change_count >=
			    UFCS_TEMP_OVER_COUNTS) {
				chip->limits.ufcs_strategy_change_count = 0;
				chip->ufcs_fastchg_batt_temp_status =
					UFCS_BAT_TEMP_EXIT;
				ret = chip->limits.ufcs_over_high_or_low_current;
				chg_err("over_high temp_over:%d", vbat_temp_cur);
			}
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp2) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW2;
			ret = chip->limits.ufcs_strategy_low_current2;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH2;
			ret = chip->limits.ufcs_strategy_high_current2;
		}
		break;
	case UFCS_BAT_TEMP_LOW0:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp0) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH0;
			ret = chip->limits.ufcs_strategy_high_current0;
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp0) { /* T<39C */
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.ufcs_strategy_normal_current;
			oplus_ufcs_reset_temp_range(chip);
			chip->limits.ufcs_strategy_batt_low_temp0 +=
				UFCS_TEMP_WARM_RANGE_THD;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW0;
			ret = chip->limits.ufcs_strategy_low_current0;
		}
		break;
	case UFCS_BAT_TEMP_LOW1:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp1) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH1;
			ret = chip->limits.ufcs_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp0) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW0;
			ret = chip->limits.ufcs_strategy_low_current0;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW1;
			ret = chip->limits.ufcs_strategy_low_current1;
		}
		break;
	case UFCS_BAT_TEMP_LOW2:
		if (vbat_temp_cur >
		    chip->limits.ufcs_strategy_batt_high_temp2) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_HIGH2;
			ret = chip->limits.ufcs_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.ufcs_strategy_batt_low_temp1) {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW1;
			ret = chip->limits.ufcs_strategy_low_current1;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LOW2;
			ret = chip->limits.ufcs_strategy_low_current2;
		}
		break;
	default:
		break;
	}
	chg_info("the ret: %d, the temp =%d, status = %d\r\n", ret,
		 vbat_temp_cur, chip->ufcs_fastchg_batt_temp_status);
	return ret;
}

static int
oplus_ufcs_set_current_temp_low_normal_range(struct oplus_ufcs *chip,
					     int vbat_temp_cur)
{
	int ret = chip->limits.ufcs_strategy_normal_current;

	if (vbat_temp_cur < chip->limits.ufcs_normal_low_temp &&
	    vbat_temp_cur >=
		    chip->limits.ufcs_little_cool_temp) { /* 20C<=T<35C */
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_LOW;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NORMAL_LOW;
		ret = chip->limits.ufcs_strategy_normal_current;
	} else {
		if (vbat_temp_cur >= chip->limits.ufcs_normal_low_temp) {
			chip->limits.ufcs_normal_low_temp -=
				UFCS_TEMP_LOW_RANGE_THD;
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.ufcs_strategy_normal_current;
			chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_HIGH;
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_LITTLE_COOL;
			chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COOL;
			ret = chip->limits.ufcs_strategy_normal_current;
			oplus_ufcs_reset_temp_range(chip);
			chip->limits.ufcs_little_cool_temp +=
				UFCS_TEMP_LOW_RANGE_THD;
		}
	}

	return ret;
}

static int
oplus_ufcs_set_current_temp_little_cool_range(struct oplus_ufcs *chip,
					      int vbat_temp_cur)
{
	int ret = 0;

	if (vbat_temp_cur < chip->limits.ufcs_little_cool_temp &&
	    vbat_temp_cur >= chip->limits.ufcs_cool_temp) { /* 12C<=T<20C */
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_LITTLE_COOL;
		ret = chip->limits.ufcs_strategy_normal_current;
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COOL;
	} else if (vbat_temp_cur >= chip->limits.ufcs_little_cool_temp) {
		chip->limits.ufcs_little_cool_temp -= UFCS_TEMP_LOW_RANGE_THD;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NORMAL_LOW;
		ret = chip->limits.ufcs_strategy_normal_current;
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_NORMAL_LOW;
	} else {
		if (chip->ui_soc <= chip->limits.ufcs_strategy_soc_high) {
			chip->limits.ufcs_strategy_change_count++;
			if (chip->limits.ufcs_strategy_change_count >=
			    UFCS_TEMP_OVER_COUNTS) {
				chip->limits.ufcs_strategy_change_count = 0;
				chip->ufcs_fastchg_batt_temp_status =
					UFCS_BAT_TEMP_SWITCH_CURVE;
				chip->ufcs_temp_cur_range =
					UFCS_TEMP_RANGE_INIT;
				(void)oplus_chg_strategy_init(chip->strategy);
				chg_err("switch temp range:%d", vbat_temp_cur);
			}
		} else {
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_COOL;
			chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_COOL;
		}
		ret = chip->limits.ufcs_strategy_normal_current;
		oplus_ufcs_reset_temp_range(chip);
		chip->limits.ufcs_cool_temp += UFCS_TEMP_LOW_RANGE_THD;
	}

	return ret;
}

static int oplus_ufcs_set_current_temp_cool_range(struct oplus_ufcs *chip,
						  int vbat_temp_cur)
{
	int ret = 0;
	if (chip->limits.ufcs_batt_over_low_temp != -EINVAL &&
	    vbat_temp_cur < chip->limits.ufcs_batt_over_low_temp) {
		chip->limits.ufcs_strategy_change_count++;
		if (chip->limits.ufcs_strategy_change_count >=
		    UFCS_TEMP_OVER_COUNTS) {
			chip->limits.ufcs_strategy_change_count = 0;
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_EXIT;
			ret = chip->limits.ufcs_over_high_or_low_current;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur < chip->limits.ufcs_cool_temp &&
		   vbat_temp_cur >=
			   chip->limits.ufcs_little_cold_temp) { /* 5C <=T<12C */
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_COOL;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_COOL;
		ret = chip->limits.ufcs_strategy_normal_current;
	} else if (vbat_temp_cur >= chip->limits.ufcs_cool_temp) {
		chip->limits.ufcs_strategy_change_count++;
		if (chip->limits.ufcs_strategy_change_count >=
		    UFCS_TEMP_OVER_COUNTS) {
			chip->limits.ufcs_strategy_change_count = 0;
			if (chip->ui_soc <= chip->limits.ufcs_strategy_soc_high) {
				chip->ufcs_fastchg_batt_temp_status =
					UFCS_BAT_TEMP_SWITCH_CURVE;
				chip->ufcs_temp_cur_range =
					UFCS_TEMP_RANGE_INIT;
				(void)oplus_chg_strategy_init(chip->strategy);
				chg_info("switch temp range:%d", vbat_temp_cur);
			} else {
				chip->ufcs_fastchg_batt_temp_status =
					UFCS_BAT_TEMP_LITTLE_COOL;
				chip->ufcs_temp_cur_range =
					UFCS_TEMP_RANGE_LITTLE_COOL;
			}
			oplus_ufcs_reset_temp_range(chip);
			chip->limits.ufcs_cool_temp -= UFCS_TEMP_LOW_RANGE_THD;
		}

		ret = chip->limits.ufcs_strategy_normal_current;
	} else {
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_LITTLE_COLD;
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COLD;
		ret = chip->limits.ufcs_strategy_normal_current;
		oplus_ufcs_reset_temp_range(chip);
		chip->limits.ufcs_little_cold_temp += UFCS_TEMP_LOW_RANGE_THD;
	}
	return ret;
}

static int
oplus_ufcs_set_current_temp_little_cold_range(struct oplus_ufcs *chip,
					      int vbat_temp_cur)
{
	int ret = chip->limits.ufcs_strategy_normal_current;

	if (chip->limits.ufcs_batt_over_low_temp != -EINVAL &&
	    vbat_temp_cur < chip->limits.ufcs_batt_over_low_temp) {
		chip->limits.ufcs_strategy_change_count++;
		if (chip->limits.ufcs_strategy_change_count >=
		    UFCS_TEMP_OVER_COUNTS) {
			chip->limits.ufcs_strategy_change_count = 0;
			ret = chip->limits.ufcs_over_high_or_low_current;
			chip->ufcs_fastchg_batt_temp_status =
				UFCS_BAT_TEMP_EXIT;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur <
		   chip->limits.ufcs_little_cold_temp) { /* 0C<=T<5C */
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_LITTLE_COLD;
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_LITTLE_COLD;
		ret = chip->limits.ufcs_strategy_normal_current;
	} else {
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_COOL;
		ret = chip->limits.ufcs_strategy_normal_current;
		chip->ufcs_temp_cur_range = UFCS_TEMP_RANGE_COOL;
		oplus_ufcs_reset_temp_range(chip);
		chip->limits.ufcs_little_cold_temp -= UFCS_TEMP_LOW_RANGE_THD;
	}

	return ret;
}

static int oplus_ufcs_get_batt_temp_curr(struct oplus_ufcs *chip)

{
	int ret, rc;
	int vbat_temp_cur;
	union mms_msg_data data = { 0 };

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return -ENODEV;
	}

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, false);
	if (rc < 0) {
		chg_err("can't get shell temp data, rc=%d", rc);
	} else {
		chip->shell_temp = data.intval;
	}

	vbat_temp_cur = chip->shell_temp;
	ret = chip->limits.ufcs_strategy_normal_current;
	switch (chip->ufcs_temp_cur_range) {
	case UFCS_TEMP_RANGE_WARM:
		ret = oplus_ufcs_set_current_warm_range(chip, vbat_temp_cur);
		break;
	case UFCS_TEMP_RANGE_NORMAL_HIGH:
		ret = oplus_ufcs_set_current_temp_normal_range(chip,
							       vbat_temp_cur);
		break;
	case UFCS_TEMP_RANGE_NORMAL_LOW:
		ret = oplus_ufcs_set_current_temp_low_normal_range(
			chip, vbat_temp_cur);
		break;
	case UFCS_TEMP_RANGE_LITTLE_COOL:
		ret = oplus_ufcs_set_current_temp_little_cool_range(
			chip, vbat_temp_cur);
		break;
	case UFCS_TEMP_RANGE_COOL:
		ret = oplus_ufcs_set_current_temp_cool_range(chip,
							     vbat_temp_cur);
		break;
	case UFCS_TEMP_RANGE_LITTLE_COLD:
		ret = oplus_ufcs_set_current_temp_little_cold_range(
			chip, vbat_temp_cur);
		break;
	default:
		break;
	}

	if (ret > 0)
		vote(chip->ufcs_curr_votable, BATT_TEMP_VOTER, true, ret, false);
	else if (ret == 0)
		ret = -EINVAL;

	return ret;
}

static void oplus_ufcs_check_low_curr_temp_status(struct oplus_ufcs *chip)
{
	static int t_cnts = 0;
	int vbat_temp_cur;
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_REAL_TEMP, &data, false);
	if (rc < 0) {
		chg_err("can't get battery temp, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	vbat_temp_cur = data.intval;

	if (((vbat_temp_cur >=
	      chip->limits.ufcs_low_curr_full_normal_high_temp) ||
	     (vbat_temp_cur < chip->limits.ufcs_low_curr_full_cool_temp)) &&
	    (chip->ufcs_low_curr_full_temp_status !=
	     UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX)) {
		t_cnts++;
		if (t_cnts >= UFCS_TEMP_OVER_COUNTS) {
			chip->ufcs_low_curr_full_temp_status =
				UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX;
			t_cnts = 0;
		}
	} else if ((vbat_temp_cur >=
		    chip->limits.ufcs_low_curr_full_normal_low_temp) &&
		   (chip->ufcs_low_curr_full_temp_status !=
		    UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH)) {
		t_cnts++;
		if (t_cnts >= UFCS_TEMP_OVER_COUNTS) {
			chip->ufcs_low_curr_full_temp_status =
				UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH;
			t_cnts = 0;
		}
	} else if ((vbat_temp_cur >=
		    chip->limits.ufcs_low_curr_full_little_cool_temp) &&
		   (chip->ufcs_low_curr_full_temp_status !=
		    UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW)) {
		t_cnts++;
		if (t_cnts >= UFCS_TEMP_OVER_COUNTS) {
			chip->ufcs_low_curr_full_temp_status =
				UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW;
			t_cnts = 0;
		}
	} else if (chip->ufcs_low_curr_full_temp_status !=
		   UFCS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL) {
		t_cnts++;
		if (t_cnts >= UFCS_TEMP_OVER_COUNTS) {
			chip->ufcs_low_curr_full_temp_status =
				UFCS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL;
			t_cnts = 0;
		}
	} else {
		t_cnts = 0;
	}
}

static void oplus_ufcs_check_sw_full(struct oplus_ufcs *chip, struct puc_strategy_ret_data *data)
{
	int cool_sw_vth, normal_sw_vth, normal_hw_vth;
	union mms_msg_data mms_data = { 0 };
	int batt_temp, vbat_mv;
	int rc;

#define UFCS_FULL_COUNTS_COOL		6
#define UFCS_FULL_COUNTS_SW		6
#define UFCS_FULL_COUNTS_HW		3

	if (!chip->oplus_ufcs_adapter) {
		cool_sw_vth = chip->limits.ufcs_full_cool_sw_vbat_third;
		normal_sw_vth = chip->limits.ufcs_full_normal_sw_vbat_third;
		normal_hw_vth = chip->limits.ufcs_full_normal_hw_vbat_third;
	} else {
		cool_sw_vth = chip->limits.ufcs_full_cool_sw_vbat;
		normal_sw_vth = chip->limits.ufcs_full_normal_sw_vbat;
		normal_hw_vth = chip->limits.ufcs_full_normal_hw_vbat;
	}

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_REAL_TEMP, &mms_data, false);
	if (rc < 0) {
		chg_err("can't get battery temp, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	batt_temp = mms_data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &mms_data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	vbat_mv = mms_data.intval;

	if ((batt_temp < chip->limits.ufcs_cool_temp) && (vbat_mv > cool_sw_vth)) {
		chip->count.cool_fw++;
		if (chip->count.cool_fw >= UFCS_FULL_COUNTS_COOL) {
			chip->count.cool_fw = 0;
			vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.cool_fw = 0;
	}

	if ((batt_temp >= chip->limits.ufcs_cool_temp) &&
	    (batt_temp < chip->limits.ufcs_batt_over_high_temp)) {
		if ((vbat_mv > normal_sw_vth) && data->last_gear) {
			chip->count.sw_full++;
			if (chip->count.sw_full >= UFCS_FULL_COUNTS_SW) {
				chip->count.sw_full = 0;
				vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
				return;
			}
		}

		if ((vbat_mv > normal_hw_vth)) {
			chip->count.hw_full++;
			if (chip->count.hw_full >= UFCS_FULL_COUNTS_HW) {
				chip->count.hw_full = 0;
				vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
				return;
			}
		}
	} else {
		chip->count.sw_full = 0;
		chip->count.hw_full = 0;
	}

	if ((chip->ufcs_fastchg_batt_temp_status == UFCS_BAT_TEMP_WARM) &&
	    (vbat_mv > chip->limits.ufcs_full_warm_vbat)) {
		chip->count.cool_fw++;
		if (chip->count.cool_fw >= UFCS_FULL_COUNTS_COOL) {
			chip->count.cool_fw = 0;
			vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.cool_fw = 0;
	}
}

static void oplus_ufcs_check_low_curr_full(struct oplus_ufcs *chip)
{
	int i, temp_current, temp_vbatt, temp_status, iterm, vterm;
	bool low_curr = false;
	union mms_msg_data data = { 0 };
	int rc;

#define UFCS_FULL_COUNTS_LOW_CURR	6

	oplus_ufcs_check_low_curr_temp_status(chip);

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	temp_vbatt = data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	temp_current = -data.intval;

	temp_status = chip->ufcs_low_curr_full_temp_status;
	if (temp_status == UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX)
		return;

	for (i = 0; i < chip->low_curr_full_curves_temp[temp_status].full_curve_num; i++) {
		iterm = chip->low_curr_full_curves_temp[temp_status].full_curves[i].iterm;
		vterm = chip->low_curr_full_curves_temp[temp_status].full_curves[i].vterm;

		if ((temp_current <= iterm) && (temp_vbatt >= vterm)) {
			low_curr = true;
			break;
		}
	}

	if (low_curr) {
		chip->count.low_curr_full++;
		if (chip->count.low_curr_full > UFCS_FULL_COUNTS_LOW_CURR) {
			chip->count.low_curr_full = 0;
			vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
		}
	} else {
		chip->count.low_curr_full = 0;
	}
}

static void oplus_ufcs_check_timeout(struct oplus_ufcs *chip)
{
	unsigned long tmp_time;

	tmp_time = jiffies - chip->timer.monitor_jiffies;
	chip->timer.monitor_jiffies = jiffies;
	if (chip->timer.ufcs_max_time_ms <= jiffies_to_msecs(tmp_time)) {
		chip->timer.ufcs_max_time_ms = 0;
		vote(chip->ufcs_disable_votable, TIMEOUT_VOTER, true, 1, false);
		return;
	}
	chip->timer.ufcs_max_time_ms -= jiffies_to_msecs(tmp_time);
}

static void oplus_ufcs_check_full(struct oplus_ufcs *chip, struct puc_strategy_ret_data *data)
{
	oplus_ufcs_check_sw_full(chip, data);
	oplus_ufcs_check_low_curr_full(chip);
}

static bool oplus_ufcs_btb_temp_check(struct oplus_ufcs *chip)
{
	bool btb_status = true;
	int btb_temp, usb_temp;
	static unsigned char temp_over_count = 0;
#define UFCS_BTB_TEMP_MAX		80
#define UFCS_USB_TEMP_MAX		80
#define UFCS_BTB_USB_OVER_CNTS		9

	btb_temp = oplus_wired_get_batt_btb_temp();
	usb_temp = oplus_wired_get_usb_btb_temp();

	if (btb_temp >= UFCS_BTB_TEMP_MAX || usb_temp >= UFCS_USB_TEMP_MAX) {
		temp_over_count++;
		if (temp_over_count > UFCS_BTB_USB_OVER_CNTS) {
			btb_status = false;
			chg_err("btb or usb temp over");
			if (btb_temp >= UFCS_BTB_TEMP_MAX)
				oplus_ufcs_push_err_info(chip, UFCS_ERR_BTB_OVER, btb_temp);
			else
				oplus_ufcs_push_err_info(chip, UFCS_ERR_USBTEMP_OVER, usb_temp);
		}
	} else {
		temp_over_count = 0;
	}

	return btb_status;
}

static void oplus_ufcs_check_ibat_safety(struct oplus_ufcs *chip)
{
	struct timespec ts_current;
	union mms_msg_data data = { 0 };
	int ibat;
	int chg_ith;
	int rc;

#define UFCS_IBAT_LOW_MIN	2000
#define UFCS_IBAT_LOW_CNT	4
#define UFCS_IBAT_HIGH_CNT	8

	ts_current = oplus_current_kernel_time();
	if ((ts_current.tv_sec - chip->timer.ibat_timer.tv_sec) < UFCS_UPDATE_IBAT_TIME)
		return;
	chip->timer.ibat_timer = ts_current;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		vote(chip->ufcs_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	ibat = data.intval;

	if (ibat > UFCS_IBAT_LOW_MIN) {
		chip->count.ibat_low++;
		if (chip->count.ibat_low >= UFCS_IBAT_LOW_CNT) {
			vote(chip->ufcs_disable_votable, IBAT_OVER_VOTER, true, 1, false);
			oplus_ufcs_push_err_info(chip, UFCS_ERR_IBAT_OVER, ibat);
			return;
		}
	} else {
		chip->count.ibat_low = 0;
	}

	if (!chip->oplus_ufcs_adapter)
		chg_ith = -chip->limits.ufcs_ibat_over_third;
	else
		chg_ith = -chip->limits.ufcs_ibat_over_oplus;
	if (ibat < chg_ith) {
		chip->count.ibat_high++;
		if (chip->count.ibat_high >= UFCS_IBAT_HIGH_CNT) {
			vote(chip->ufcs_disable_votable, IBAT_OVER_VOTER, true, 1, false);
			oplus_ufcs_push_err_info(chip, UFCS_ERR_IBAT_OVER, ibat);
			return;
		}
	} else {
		chip->count.ibat_high = 0;
	}
}

static void oplus_ufcs_check_temp(struct oplus_ufcs *chip)
{
	struct timespec ts_current;
	union mms_msg_data data = { 0 };
	int batt_temp;
	int rc;

#define UFCS_FG_TEMP_PROTECTION	800
#define UFCS_TFG_OV_CNT		6
#define UFCS_BTB_OV_CNT		8
#define UFCS_TBATT_OV_CNT	1

	ts_current = oplus_current_kernel_time();
	if ((ts_current.tv_sec - chip->timer.temp_timer.tv_sec) < UFCS_UPDATE_TEMP_TIME)
		return;

	chip->timer.temp_timer = ts_current;

	if (!oplus_ufcs_btb_temp_check(chip)) {
		chip->count.btb_high++;
		if (chip->count.btb_high >= UFCS_BTB_OV_CNT) {
			chip->count.btb_high = 0;
			vote(chip->ufcs_disable_votable, BTB_TEMP_OVER_VOTER, true, 1, false);
			oplus_ufcs_push_err_info(chip, UFCS_ERR_BTB_OVER, 0);
		}
	} else {
		chip->count.btb_high = 0;
	}

	if (chip->ufcs_fastchg_batt_temp_status == UFCS_BAT_TEMP_EXIT) {
		chg_err("ufcs battery temp out of range\n");
		chip->count.tbatt_over++;
		if (chip->count.tbatt_over >= UFCS_TBATT_OV_CNT)
			vote(chip->ufcs_not_allow_votable, BATT_TEMP_VOTER, true, 1, false);
	} else {
		chip->count.tbatt_over = 0;
	}

	if (chip->ufcs_fastchg_batt_temp_status == UFCS_BAT_TEMP_SWITCH_CURVE) {
		chg_err("ufcs battery temp switch curve range\n");
		vote(chip->ufcs_disable_votable, SWITCH_RANGE_VOTER, true, 1, false);
	}

	if (is_gauge_topic_available(chip)) {
		rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_REAL_TEMP, &data, false);
		if (rc >= 0) {
			batt_temp = data.intval;
			if (batt_temp >= UFCS_FG_TEMP_PROTECTION) {
				chg_err("ufcs tfg > 80\n");
				chip->count.tfg_over++;
				if (chip->count.tfg_over >= UFCS_TFG_OV_CNT) {
					chip->count.tfg_over = 0;
					vote(chip->ufcs_disable_votable, TFG_VOTER, true, 1, false);
					oplus_ufcs_push_err_info(chip, UFCS_ERR_BTB_OVER, batt_temp);
				}
			} else {
				chip->count.tfg_over = 0;
			}
		}
	}
}

static void oplus_ufcs_check_bcc_curr(struct oplus_ufcs *chip, struct puc_strategy_ret_data *data)
{
	int bcc_min_curr, bcc_max_curr, bcc_exit_curr;
	struct puc_strategy_temp_curves *curve;
	int curr_ratio;

	switch (chip->cp_work_mode) {
	case CP_WORK_MODE_4_TO_1:
		curr_ratio = 4;
		break;
	case CP_WORK_MODE_3_TO_1:
		curr_ratio = 3;
		break;
	case CP_WORK_MODE_2_TO_1:
		curr_ratio = 2;
		break;
	case CP_WORK_MODE_BYPASS:
		curr_ratio = 1;
		break;
	default:
		chg_err("unsupported cp work mode, mode=%d\n", chip->cp_work_mode);
		return;
	}

	bcc_min_curr = data->target_ibus * curr_ratio / 100; /* mA to 0.1A */
	bcc_max_curr = data->target_ibus * curr_ratio / 100; /* mA to 0.1A */
	curve = puc_strategy_get_curr_curve_data(chip->strategy);
	if (curve == NULL) {
		chg_err("can't get charger curve\n");
		return;
	}
	bcc_exit_curr = curve->data[curve->num - 1].target_ibus * curr_ratio;

	if ((bcc_min_curr - UFCS_BCC_CURRENT_MIN) < (bcc_exit_curr / UFCS_BATT_CURR_TO_BCC_CURR))
		bcc_min_curr = bcc_exit_curr / UFCS_BATT_CURR_TO_BCC_CURR;
	else
		bcc_min_curr = bcc_min_curr - UFCS_BCC_CURRENT_MIN;

	chip->bcc.bcc_min_curr = bcc_min_curr;
	chip->bcc.bcc_max_curr = bcc_max_curr;
	chip->bcc.bcc_exit_curr = bcc_exit_curr;
	chg_debug("bcc_info: index=%d, min=%d, max=%d, exit=%d\n",
		  data->index, bcc_min_curr, bcc_max_curr, bcc_exit_curr);

	return;
}

static void oplus_ufcs_set_cool_down_curr(struct oplus_ufcs *chip, int cool_down)
{
	int target_curr;

	if (chip->curr_table_type == UFCS_CURR_CP_TABLE) {
		if (cool_down >= ARRAY_SIZE(ufcs_cp_cool_down_oplus_curve))
			cool_down = ARRAY_SIZE(ufcs_cp_cool_down_oplus_curve) - 1;
		target_curr = ufcs_cp_cool_down_oplus_curve[cool_down];
	} else {
		if (cool_down >= ARRAY_SIZE(ufcs_cool_down_oplus_curve))
			cool_down = ARRAY_SIZE(ufcs_cool_down_oplus_curve) - 1;
		target_curr = ufcs_cool_down_oplus_curve[cool_down];
	}
	switch (chip->cp_work_mode) {
	case CP_WORK_MODE_4_TO_1:
		target_curr /= 4;
		break;
	case CP_WORK_MODE_3_TO_1:
		target_curr /= 3;
		break;
	case CP_WORK_MODE_2_TO_1:
		target_curr /= 2;
		break;
	case CP_WORK_MODE_BYPASS:
		break;
	default:
		chg_err("unsupported cp work mode, mode=%d\n", chip->cp_work_mode);
		return;
	}

	vote(chip->ufcs_curr_votable, COOL_DOWN_VOTER, true, target_curr, false);
}

static void oplus_ufcs_imp_check(struct oplus_ufcs *chip)
{
	int curr;

	if (!chip->imp_uint)
		return;

	curr = oplus_imp_uint_get_allow_curr(chip->imp_uint);
	if (curr < 0) {
		chg_err("can't get ufcs channel impedance current limite, exit ufcs charging\n");
		vote(chip->ufcs_disable_votable, IMP_VOTER, true, 1, false);
	} else if (curr == 0) {
		chg_err("ufcs channel impedance is too high, exit ufcs charging\n");
		vote(chip->ufcs_disable_votable, IMP_VOTER, true, 1, false);
		oplus_ufcs_push_err_info(chip, UFCS_ERR_IMP, 0);
	} else {
		vote(chip->ufcs_curr_votable, IMP_VOTER, true, curr, false);
	}
}

static void oplus_ufcs_watchdog(struct oplus_ufcs *chip)
{
	int rc;
	bool work_start;

	rc = oplus_ufcs_cp_get_work_status(chip, &work_start);
	if (rc < 0) {
		chg_err("can't get cp work status, rc=%d\n", rc);
		return;
	}
	if (!work_start)
		return;
	rc = oplus_ufcs_cp_set_work_start(chip, true);
	if (rc < 0)
		chg_err("set cp work start error, rc=%d\n", rc);
}

static void oplus_ufcs_check_current_low(struct oplus_ufcs *chip)
{
	static int count = 0;

#define UFCS_CURR_LOW_LIMIT	500
#define UFCS_CURR_LOW_CNT	3

	if (UFCS_SOURCE_INFO_CURR(chip->src_info) > UFCS_CURR_LOW_LIMIT) {
		count = 0;
		return;
	}

	count++;
	if (count >= UFCS_CURR_LOW_CNT) {
		chg_info("ufcs ibus low, exit ufcs fast charge\n");
		vote(chip->ufcs_disable_votable, IOUT_CURR_VOTER, true, 1, false);
		count = 0;
	}
}

static void oplus_ufcs_protection_check(struct oplus_ufcs *chip, struct puc_strategy_ret_data *data)
{
	int rc;

	rc = oplus_ufcs_get_src_info(chip, &chip->src_info);
	if (rc < 0) {
		chg_err("ufcs get src info error\n");
		chip->src_info = 0;
	}

	oplus_ufcs_check_timeout(chip);
	oplus_ufcs_check_full(chip, data);
	oplus_ufcs_check_ibat_safety(chip);
	oplus_ufcs_check_current_low(chip);
	oplus_ufcs_check_temp(chip);
	oplus_ufcs_imp_check(chip);
	oplus_ufcs_watchdog(chip);
}

static void oplus_ufcs_monitor_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_ufcs *chip =
		container_of(dwork, struct oplus_ufcs, monitor_work);
	struct puc_strategy_ret_data data;
	int rc;
	int delay = UFCS_MONITOR_TIME_MS;
	int range_switch_dealy = UFCS_TEMP_SWITCH_DELAY;
	bool switch_to_ffc = false;

	if (!chip->ufcs_charging) {
		rc = oplus_ufcs_charge_start(chip);
		if (rc < 0)
			goto exit;
		delay = rc;
	} else {
		oplus_ufcs_cp_reg_dump(chip);
		rc = oplus_ufcs_get_batt_temp_curr(chip);
		if (rc < 0)
			goto exit;

		rc = oplus_chg_strategy_get_data(chip->strategy, &data);
		if (rc < 0) {
			chg_err("can't get strategy data, rc=%d", rc);
			goto exit;
		}
		if (data.exit) {
			chg_info("exit ufcs fast charge, start ffc\n");
			vote(chip->ufcs_disable_votable, CHG_FULL_VOTER, true, 1, false);
			switch_to_ffc = true;
			goto exit;
		}

		oplus_ufcs_check_bcc_curr(chip, &data);
		oplus_ufcs_set_cool_down_curr(chip, chip->cool_down);
		oplus_ufcs_protection_check(chip, &data);
		if (get_client_vote(chip->ufcs_disable_votable, CHG_FULL_VOTER) > 0) {
			chg_info("exit ufcs fast charge, start ffc\n");
			switch_to_ffc = true;
			goto exit;
		}
		if (chip->ufcs_not_allow || chip->ufcs_disable) {
			chg_info("ufcs charge not allow or disable, exit ufcs mode\n");
			goto exit;
		}

		chip->target_vbus_mv = data.target_vbus;
		vote(chip->ufcs_curr_votable, STEP_VOTER, true, data.target_ibus, false);
	}

	if (chip->wired_online)
		schedule_delayed_work(&chip->monitor_work, msecs_to_jiffies(delay));
	return;

exit:
	oplus_ufcs_soft_exit(chip);
	if (switch_to_ffc)
		oplus_comm_switch_ffc(chip->comm_topic);
	if (chip->config.adsp_ufcs_project)
		range_switch_dealy = UFCS_ADSP_TEMP_SWITCH_DELAY;
	if (chip->ufcs_fastchg_batt_temp_status == UFCS_BAT_TEMP_SWITCH_CURVE) {
		chg_info("ufcs need retry enter ufcs\n");
		chip->ufcs_fastchg_batt_temp_status = UFCS_BAT_TEMP_NATURAL;
		schedule_delayed_work(&chip->switch_check_work, msecs_to_jiffies(range_switch_dealy));
	}
}

enum {
	OPLUS_UFCS_CURR_UPDATE_V1 = 50,
	OPLUS_UFCS_CURR_UPDATE_V2 = 100,
	OPLUS_UFCS_CURR_UPDATE_V3 = 200,
	OPLUS_UFCS_CURR_UPDATE_V4 = 300,
	OPLUS_UFCS_CURR_UPDATE_V5 = 400,
	OPLUS_UFCS_CURR_UPDATE_V6 = 500,
	OPLUS_UFCS_CURR_UPDATE_V7 = 1000,
	OPLUS_UFCS_CURR_UPDATE_V8 = 1500,
	OPLUS_UFCS_CURR_UPDATE_V9 = 2000,
	OPLUS_UFCS_CURR_UPDATE_V10 = 2500,
	OPLUS_UFCS_CURR_UPDATE_V11 = 3000,
};

static void oplus_ufcs_current_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_ufcs *chip =
		container_of(dwork, struct oplus_ufcs, current_work);
	int target_vbus;
	int curr_set, update_size;
	int rc;

	if (!chip->ufcs_charging)
		return;

#define UFCS_CURR_CHANGE_UPDATE_DELAY		200
#define UFCS_CURR_NO_CHANGE_UPDATE_DELAY	500
	curr_set = chip->curr_set_ma;
	target_vbus = chip->target_vbus_mv;

	if (curr_set > chip->target_curr_ma) {
		if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V10)
			update_size = OPLUS_UFCS_CURR_UPDATE_V10;
		else if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V8)
			update_size = OPLUS_UFCS_CURR_UPDATE_V8;
		else if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V7)
			update_size = OPLUS_UFCS_CURR_UPDATE_V7;
		else if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V6)
			update_size = OPLUS_UFCS_CURR_UPDATE_V6;
		else if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V4)
			update_size = OPLUS_UFCS_CURR_UPDATE_V4;
		else
			update_size = OPLUS_UFCS_CURR_UPDATE_V1;
		curr_set -= update_size;
	} else if (curr_set < chip->target_curr_ma) {
		if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V6)
			update_size = OPLUS_UFCS_CURR_UPDATE_V6;
		else if (abs(curr_set - chip->target_curr_ma) >= OPLUS_UFCS_CURR_UPDATE_V4)
			update_size = OPLUS_UFCS_CURR_UPDATE_V4;
		else
			update_size = OPLUS_UFCS_CURR_UPDATE_V1;
		curr_set += update_size;
	} else {
		curr_set = chip->target_curr_ma;
	}

	chg_err("curr_set=%d, curr_set_ma=%d, target_curr_ma=%d\n", curr_set, chip->curr_set_ma, chip->target_curr_ma);

	if ((target_vbus != chip->vol_set_mv) || (curr_set != chip->curr_set_ma)) {
		rc = oplus_ufcs_cp_set_iin(chip, curr_set);
		if (rc < 0) {
			chg_err("set cp input current error, rc=%d\n", rc);
		} else {
			if (oplus_wired_get_bcc_curr_done_status(chip->wired_topic) == BCC_CURR_DONE_REQUEST)
				oplus_wired_check_bcc_curr_done(chip->wired_topic);
			rc = oplus_ufcs_pdo_set(chip, target_vbus, curr_set);
			if (rc < 0)
				chg_err("pdo set error, rc=%d\n", rc);
		}
	}

	if (chip->curr_set_ma != chip->target_curr_ma)
		schedule_delayed_work(&chip->current_work, msecs_to_jiffies(UFCS_CURR_CHANGE_UPDATE_DELAY));
	else
		schedule_delayed_work(&chip->current_work, msecs_to_jiffies(UFCS_CURR_NO_CHANGE_UPDATE_DELAY));
}

static void oplus_ufcs_wired_online_work(struct work_struct *work)
{
	struct oplus_ufcs *chip =
		container_of(work, struct oplus_ufcs, wired_online_work);

	if (!chip->wired_online) {
		oplus_ufcs_force_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);
	}
}

static void oplus_ufcs_force_exit_work(struct work_struct *work)
{
	struct oplus_ufcs *chip =
		container_of(work, struct oplus_ufcs, force_exit_work);

	if (chip->ufcs_online) {
		oplus_ufcs_force_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);
	}
}

static void oplus_ufcs_soft_exit_work(struct work_struct *work)
{
	struct oplus_ufcs *chip =
		container_of(work, struct oplus_ufcs, soft_exit_work);

	if (chip->ufcs_online) {
		oplus_ufcs_soft_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);
	}
}

static void oplus_ufcs_allow_recover_check(struct oplus_ufcs *chip)
{
	if (is_client_vote_enabled(chip->ufcs_not_allow_votable, BATT_TEMP_VOTER)) {
		if (chip->shell_temp >= chip->limits.ufcs_low_temp &&
		    chip->shell_temp <= chip->limits.ufcs_high_temp) {
			chg_info("allow ufcs charging, shell_temp=%d\n", chip->shell_temp);
			vote(chip->ufcs_not_allow_votable, BATT_TEMP_VOTER, false, 0, false);
		}
	}
	if (is_client_vote_enabled(chip->ufcs_not_allow_votable, BATT_SOC_VOTER)) {
		if (chip->ui_soc >= chip->limits.ufcs_low_soc &&
		    chip->ui_soc <= chip->limits.ufcs_high_soc) {
			chg_info("allow ufcs charging, ui_soc=%d\n", chip->ui_soc);
			vote(chip->ufcs_not_allow_votable, BATT_SOC_VOTER, false, 0, false);
		}
	}
}

static void oplus_ufcs_gauge_update_work(struct work_struct *work)
{
	struct oplus_ufcs *chip =
		container_of(work, struct oplus_ufcs, gauge_update_work);

	oplus_ufcs_allow_recover_check(chip);
}

static void oplus_ufcs_err_flag_push_work(struct work_struct *work)
{
	struct oplus_ufcs *chip =
		container_of(work, struct oplus_ufcs, err_flag_push_work);
	struct mms_msg *msg;
	unsigned int ignored_flag;
	int rc;
	char *buf;
	int i;
	size_t index;

	ignored_flag = BIT(UFCS_HW_ERR_HARD_RESET) |
		BIT(UFCS_RECV_ERR_SENT_CMP) |
		BIT(UFCS_RECV_ERR_DATA_READY) |
		BIT(UFCS_COMM_ERR_BAUD_RATE_CHANGE);
	if ((chip->err_flag & (~ignored_flag)) == 0)
		return;

	if (!is_err_topic_available(chip))
		return;

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return;

	index = snprintf(buf, PAGE_SIZE, "$$reason@@0x%x$$dev_info@@0x%llx"
		"$$cable_info@@0x%llx$$emark_info@@0x%llx",
		chip->err_flag, chip->dev_info, chip->cable_info, chip->emark_info);
	if (chip->pdo_num > 0)
		index += snprintf(buf + index, PAGE_SIZE, "$$pdo_info@@");
	for (i = 0; i < chip->pdo_num; i++) {
		if (i == chip->pdo_num - 1)
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx", chip->pdo[i]);
		else
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx,", chip->pdo[i]);
	}

	if (chip->pie_num > 0)
		index += snprintf(buf + index, PAGE_SIZE, "$$pie_info@@");
	for (i = 0; i < chip->pie_num; i++) {
		if (i == chip->pie_num - 1)
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx", chip->pie[i]);
		else
			index += snprintf(buf + index, PAGE_SIZE, "0x%llx,", chip->pie[i]);
	}
	index += snprintf(buf + index, PAGE_SIZE, "$$power_max@@%d",
		oplus_cpa_protocol_get_power(chip->cpa_topic, CHG_PROTOCOL_UFCS));

	msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_UFCS, buf);
	kfree(buf);
	rc = oplus_mms_publish_msg_sync(chip->err_topic, msg);
	if (rc < 0) {
		chg_err("publish usbtemp error msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

static void oplus_ufcs_comm_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_ufcs *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int rc;

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_UI_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->ui_soc = data.intval;
			break;
		case COMM_ITEM_TEMP_REGION:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_temp_region = !!data.intval;
			break;
		case COMM_ITEM_CHARGING_DISABLE:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get charging disable status, rc=%d", rc);
			else
				vote(chip->ufcs_not_allow_votable, MMI_CHG_VOTER, !!data.intval, data.intval, false);
			break;
		case COMM_ITEM_CHARGE_SUSPEND:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get charge suspend status, rc=%d", rc);
			else
				vote(chip->ufcs_not_allow_votable, USER_VOTER, !!data.intval, data.intval, false);
			break;
		case COMM_ITEM_COOL_DOWN:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get cool down data, rc=%d", rc);
			else
				chip->cool_down = data.intval;
			break;
		case COMM_ITEM_SHELL_TEMP:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0) {
				chg_err("can't get shell temp data, rc=%d", rc);
			} else {
				chip->shell_temp = data.intval;
				if (unlikely(!chip->shell_temp_ready && chip->shell_temp != GAUGE_INVALID_TEMP)) {
					chip->shell_temp_ready = true;
					vote(chip->ufcs_boot_votable, SHELL_TEMP_VOTER, false, 0, false);
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_ufcs_subscribe_comm_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_ufcs *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->comm_topic = topic;
	chip->comm_subs =
		oplus_mms_subscribe(topic, chip,
				    oplus_ufcs_comm_subs_callback, "ufcs");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe comm topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(topic, COMM_ITEM_UI_SOC, &data, true);
	chip->ui_soc = data.intval;
	oplus_mms_get_item_data(topic, COMM_ITEM_TEMP_REGION, &data, true);
	chip->batt_temp_region = !!data.intval;
	rc = oplus_mms_get_item_data(topic, COMM_ITEM_CHARGING_DISABLE, &data, true);
	if (rc < 0)
		chg_err("can't get charging disable status, rc=%d", rc);
	else
		vote(chip->ufcs_not_allow_votable, MMI_CHG_VOTER, !!data.intval, data.intval, false);
	rc = oplus_mms_get_item_data(topic, COMM_ITEM_CHARGE_SUSPEND, &data, true);
	if (rc < 0)
		chg_err("can't get charge suspend status, rc=%d", rc);
	else
		vote(chip->ufcs_not_allow_votable, USER_VOTER, !!data.intval, data.intval, false);
	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_COOL_DOWN, &data, true);
	if (rc < 0)
		chg_err("can't get cool down data, rc=%d", rc);
	else
		chip->cool_down = data.intval;
	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, true);
	if (rc < 0) {
		chg_err("can't get shell temp data, rc=%d", rc);
	} else {
		chip->shell_temp = data.intval;
		if (unlikely(!chip->shell_temp_ready && chip->shell_temp != GAUGE_INVALID_TEMP)) {
			chip->shell_temp_ready = true;
			vote(chip->ufcs_boot_votable, SHELL_TEMP_VOTER, false, 0, false);
		}
	}

	vote(chip->ufcs_boot_votable, COMM_TOPIC_VOTER, false, 0, false);
}

static void oplus_ufcs_wired_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_ufcs *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_online = !!data.intval;
			schedule_work(&chip->wired_online_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_ufcs_subscribe_wired_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_ufcs *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_ufcs_wired_subs_callback, "ufcs");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = !!data.intval;

	vote(chip->ufcs_boot_votable, WIRED_TOPIC_VOTER, false, 0, false);
}

static void oplus_ufcs_cpa_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_ufcs *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case CPA_ITEM_ALLOW:
			oplus_mms_get_item_data(chip->cpa_topic, id, &data,
						false);
			if (data.intval == CHG_PROTOCOL_UFCS)
				schedule_delayed_work(&chip->switch_check_work, 0);
			break;
		case CPA_ITEM_TIMEOUT:
			oplus_mms_get_item_data(chip->cpa_topic, id, &data,
						false);
			if (data.intval == CHG_PROTOCOL_UFCS) {
				oplus_ufcs_switch_to_normal(chip);
				oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_UFCS);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_ufcs_subscribe_cpa_topic(struct oplus_mms *topic,
					   void *prv_data)
{
	struct oplus_ufcs *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->cpa_topic = topic;
	chip->cpa_subs =
		oplus_mms_subscribe(chip->cpa_topic, chip,
				    oplus_ufcs_cpa_subs_callback, "ufcs");
	if (IS_ERR_OR_NULL(chip->cpa_subs)) {
		chg_err("subscribe cpa topic error, rc=%ld\n",
			PTR_ERR(chip->cpa_subs));
		return;
	}

	oplus_mms_get_item_data(chip->cpa_topic, CPA_ITEM_ALLOW, &data, true);
	if (data.intval == CHG_PROTOCOL_UFCS)
		schedule_delayed_work(&chip->switch_check_work, msecs_to_jiffies(UFCS_MONITOR_CYCLE_MS));

	if (chip->wait_auth_data_done)
		vote(chip->ufcs_boot_votable, AUTH_VOTER, false, 0, false);
	vote(chip->ufcs_boot_votable, CPA_TOPIC_VOTER, false, 0, false);
}

static void oplus_ufcs_gauge_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_ufcs *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_TIMER:
		schedule_work(&chip->gauge_update_work);
		break;
	default:
		break;
	}
}

static void oplus_ufcs_subscribe_gauge_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_ufcs *chip = prv_data;

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_ufcs_gauge_subs_callback, "ufcs");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}

	vote(chip->ufcs_boot_votable, GAUGE_TOPIC_VOTER, false, 0, false);
}

static int oplus_ufcs_update_online(struct oplus_mms *mms,
				    union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->ufcs_online;

	return 0;
}

static int oplus_ufcs_update_charging(struct oplus_mms *mms,
				      union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->ufcs_charging;

	return 0;
}

static int oplus_ufcs_update_adapter_id(struct oplus_mms *mms,
					union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = (int)chip->adapter_id;

	return 0;
}

static int oplus_ufcs_update_oplus_adapter(struct oplus_mms *mms,
					   union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->oplus_ufcs_adapter;

	return 0;
}

static int oplus_ufcs_update_bcc_max_curr(struct oplus_mms *mms,
					  union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->bcc.bcc_max_curr;

	return 0;
}

static int oplus_ufcs_update_bcc_min_curr(struct oplus_mms *mms,
					  union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->bcc.bcc_min_curr;

	return 0;
}

static int oplus_ufcs_update_bcc_exit_curr(struct oplus_mms *mms,
					   union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->bcc.bcc_exit_curr;

	return 0;
}

static int oplus_ufcs_update_bcc_temp_range(struct oplus_mms *mms,
					    union mms_msg_data *data)
{
	struct oplus_ufcs *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	if ((chip->ufcs_temp_cur_range == UFCS_TEMP_RANGE_NORMAL_LOW) ||
	    (chip->ufcs_temp_cur_range == UFCS_TEMP_RANGE_NORMAL_HIGH))
		data->intval = true;
	else
		data->intval = false;

	return 0;
}

static void oplus_ufcs_topic_update(struct oplus_mms *mms, bool publish)
{
}

static struct mms_item oplus_ufcs_item[] = {
	{
		.desc = {
			.item_id = UFCS_ITEM_ONLINE,
			.update = oplus_ufcs_update_online,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_CHARGING,
			.update = oplus_ufcs_update_charging,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_ADAPTER_ID,
			.update = oplus_ufcs_update_adapter_id,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_OPLUS_ADAPTER,
			.update = oplus_ufcs_update_oplus_adapter,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_BCC_MAX_CURR,
			.update = oplus_ufcs_update_bcc_max_curr,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_BCC_MIN_CURR,
			.update = oplus_ufcs_update_bcc_min_curr,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_BCC_EXIT_CURR,
			.update = oplus_ufcs_update_bcc_exit_curr,
		}
	},
	{
		.desc = {
			.item_id = UFCS_ITEM_BCC_TEMP_RANGE,
			.update = oplus_ufcs_update_bcc_temp_range,
		}
	},
};

static const struct oplus_mms_desc oplus_ufcs_desc = {
	.name = "ufcs",
	.type = OPLUS_MMS_TYPE_UFCS,
	.item_table = oplus_ufcs_item,
	.item_num = ARRAY_SIZE(oplus_ufcs_item),
	.update_items = NULL,
	.update_items_num = 0,
	.update_interval = 0, /* ms */
	.update = oplus_ufcs_topic_update,
};

static int oplus_ufcs_topic_init(struct oplus_ufcs *chip)
{
	struct oplus_mms_config mms_cfg = {};
	int rc;

	mms_cfg.drv_data = chip;
	mms_cfg.of_node = chip->dev->of_node;

	if (of_property_read_bool(mms_cfg.of_node,
				  "oplus,topic-update-interval")) {
		rc = of_property_read_u32(mms_cfg.of_node,
					  "oplus,topic-update-interval",
					  &mms_cfg.update_interval);
		if (rc < 0)
			mms_cfg.update_interval = 0;
	}

	chip->ufcs_topic =
		devm_oplus_mms_register(chip->dev, &oplus_ufcs_desc, &mms_cfg);
	if (IS_ERR(chip->ufcs_topic)) {
		chg_err("Couldn't register ufcs topic\n");
		rc = PTR_ERR(chip->ufcs_topic);
		return rc;
	}

	return 0;
}

static int oplus_ufcs_virq_reg(struct oplus_ufcs *chip)
{
	return 0;
}

static void oplus_ufcs_virq_unreg(struct oplus_ufcs *chip)
{}

static void oplus_ufcs_ic_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_ufcs *chip;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	chip->ufcs_ic = ic;
	rc = oplus_ufcs_virq_reg(chip);
	if (rc < 0) {
		chg_err("virq register error, rc=%d\n", rc);
		chip->ufcs_ic = NULL;
		return;
	}

	rc = oplus_ufcs_topic_init(chip);
	if (rc < 0)
		goto topic_reg_err;

	oplus_mms_wait_topic("common", oplus_ufcs_subscribe_comm_topic, chip);
	oplus_mms_wait_topic("wired", oplus_ufcs_subscribe_wired_topic, chip);
	oplus_mms_wait_topic("cpa", oplus_ufcs_subscribe_cpa_topic, chip);
	oplus_mms_wait_topic("gauge", oplus_ufcs_subscribe_gauge_topic, chip);
	return;

topic_reg_err:
	oplus_ufcs_virq_unreg(chip);
}

static int oplus_ufcs_dpdm_switch_virq_reg(struct oplus_ufcs *chip)
{
	return 0;
}

static void oplus_ufcs_dpdm_switch_virq_unreg(struct oplus_ufcs *chip)
{}

static void oplus_ufcs_dpdm_switch_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_ufcs *chip;
	const char *name;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	rc = oplus_ufcs_dpdm_switch_virq_reg(chip);
	if (rc < 0) {
		chg_err("dpdm_switch virq register error, rc=%d\n", rc);
		return;
	}
	chip->dpdm_switch = ic;

	name = of_get_oplus_chg_ic_name(chip->dev->of_node, "oplus,ufcs_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_ufcs_ic_reg_callback, chip);
}

static int oplus_ufcs_cp_virq_reg(struct oplus_ufcs *chip)
{
	return 0;
}

static void oplus_ufcs_cp_virq_unreg(struct oplus_ufcs *chip)
{}

static void oplus_ufcs_cp_ic_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_ufcs *chip;
	const char *name;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	rc = oplus_ufcs_cp_virq_reg(chip);
	if (rc < 0) {
		chg_err("cp virq register error, rc=%d\n", rc);
		return;
	}
	chip->cp_ic = ic;

	name = of_get_oplus_chg_ic_name(chip->dev->of_node, "oplus,dpdm_switch_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_ufcs_dpdm_switch_reg_callback, chip);
}

static int oplus_ufcs_curr_vote_callback(struct votable *votable, void *data,
					 int curr_ma, const char *client,
					 bool step)
{
	struct oplus_ufcs *chip = data;

	if (curr_ma < 0)
		return -EINVAL;

	chip->target_curr_ma = curr_ma;
	chg_info("%s set ufcs target current to %d mA\n", client, curr_ma);

	return 0;
}

static int oplus_ufcs_disable_vote_callback(struct votable *votable, void *data,
					    int disable, const char *client,
					    bool step)
{
	struct oplus_ufcs *chip = data;

	if (disable < 0)
		chip->ufcs_disable = false;
	else
		chip->ufcs_disable = !!disable;
	chg_info("%s set ufcs disable to %s\n", client, chip->ufcs_disable ? "true" : "false");

	return 0;
}

static int oplus_ufcs_not_allow_vote_callback(struct votable *votable, void *data,
					      int not_allow, const char *client,
					      bool step)
{
	struct oplus_ufcs *chip = data;
	union mms_msg_data mms_data = { 0 };
	int rc;

	if (not_allow < 0)
		chip->ufcs_not_allow = false;
	else
		chip->ufcs_not_allow = !!not_allow;
	chg_info("%s set ufcs not allow to %s\n", client, chip->ufcs_not_allow ? "true" : "false");

	if (!chip->ufcs_not_allow) {
		rc = oplus_mms_get_item_data(chip->cpa_topic, CPA_ITEM_ALLOW, &mms_data, false);
		if (rc < 0)
			return 0;
		if (mms_data.intval == CHG_PROTOCOL_UFCS)
			schedule_delayed_work(&chip->switch_check_work, 0);
	}

	return 0;
}

static int oplus_ufcs_boot_vote_callback(struct votable *votable, void *data,
					 int boot, const char *client,
					 bool step)
{
	struct oplus_ufcs *chip = data;

	/* The UFCS module startup has not been completed and will not be processed temporarily */
	if (!!boot)
		return 0;

	oplus_cpa_protocol_ready(chip->cpa_topic, CHG_PROTOCOL_UFCS);
	return 0;
}

static int oplus_ufcs_parse_charge_strategy(struct oplus_ufcs *chip)
{
	int rc;
	int length = 0;
	int soc_tmp[7] = { 0, 15, 30, 50, 75, 85, 95 };
	int rang_temp_tmp[7] = { 0, 50, 120, 200, 350, 440, 510 };
	int high_temp_tmp[6] = { 425, 430, 435, 400, 415, 420 };
	int high_curr_tmp[6] = { 4000, 3000, 2000, 3000, 4000, 4000 };
	int low_curr_temp_tmp[4] = { 120, 200, 300, 440 };
	struct device_node *node = chip->dev->of_node;

	rc = of_property_read_u32(node, "oplus,ufcs_warm_allow_vol",
				  &chip->limits.ufcs_warm_allow_vol);
	if (rc)
		chip->limits.ufcs_warm_allow_vol = 4000;
	else
		chg_info("oplus,ufcs_warm_allow_vol is %d\n",
			 chip->limits.ufcs_warm_allow_vol);

	rc = of_property_read_u32(node, "oplus,ufcs_warm_allow_soc",
				  &chip->limits.ufcs_warm_allow_soc);
	if (rc)
		chip->limits.ufcs_warm_allow_soc = 50;
	else
		chg_info("oplus,ufcs_warm_allow_soc is %d\n",
			 chip->limits.ufcs_warm_allow_soc);

	rc = of_property_read_u32(node, "oplus,ufcs_strategy_normal_current",
				  &chip->limits.ufcs_strategy_normal_current);
	if (rc)
		chip->limits.ufcs_strategy_normal_current = 6000;
	else
		chg_info("oplus,ufcs_strategy_normal_current is %d\n",
			 chip->limits.ufcs_strategy_normal_current);

	rc = of_property_read_u32(node, "oplus,ufcs_over_high_or_low_current",
				  &chip->limits.ufcs_over_high_or_low_current);
	if (rc)
		chip->limits.ufcs_over_high_or_low_current = -EINVAL;
	else
		chg_info("oplus,ufcs_over_high_or_low_current is %d\n",
			 chip->limits.ufcs_over_high_or_low_current);

	rc = of_property_read_u32(node, "oplus,ufcs_full_cool_sw_vbat",
				  &chip->limits.ufcs_full_cool_sw_vbat);
	if (rc)
		chip->limits.ufcs_full_cool_sw_vbat = 4430;
	else
		chg_info("oplus,ufcs_full_cool_sw_vbat is %d\n",
			 chip->limits.ufcs_full_cool_sw_vbat);

	rc = of_property_read_u32(node, "oplus,ufcs_full_normal_sw_vbat",
				  &chip->limits.ufcs_full_normal_sw_vbat);
	if (rc)
		chip->limits.ufcs_full_normal_sw_vbat = 4495;
	else
		chg_info("oplus,ufcs_full_normal_sw_vbat %d\n",
			 chip->limits.ufcs_full_normal_sw_vbat);

	rc = of_property_read_u32(node, "oplus,ufcs_full_normal_hw_vbat",
				  &chip->limits.ufcs_full_normal_hw_vbat);
	if (rc)
		chip->limits.ufcs_full_normal_hw_vbat = 4500;
	else
		chg_info("oplus,ufcs_full_normal_hw_vbat is %d\n",
			 chip->limits.ufcs_full_normal_hw_vbat);

	rc = of_property_read_u32(node, "oplus,ufcs_full_cool_sw_vbat_third",
				  &chip->limits.ufcs_full_cool_sw_vbat_third);
	if (rc)
		chip->limits.ufcs_full_cool_sw_vbat_third = 4430;
	else
		chg_info("oplus,ufcs_full_cool_sw_vbat_third is %d\n",
			 chip->limits.ufcs_full_cool_sw_vbat_third);

	rc = of_property_read_u32(node, "oplus,ufcs_full_normal_sw_vbat_third",
				  &chip->limits.ufcs_full_normal_sw_vbat_third);
	if (rc)
		chip->limits.ufcs_full_normal_sw_vbat_third = 4430;
	else
		chg_info("oplus,ufcs_full_normal_sw_vbat_third is %d\n",
			 chip->limits.ufcs_full_normal_sw_vbat_third);

	rc = of_property_read_u32(node, "oplus,ufcs_full_normal_hw_vbat_third",
				  &chip->limits.ufcs_full_normal_hw_vbat_third);
	if (rc)
		chip->limits.ufcs_full_normal_hw_vbat_third = 4440;
	else
		chg_info("oplus,ufcs_full_normal_hw_vbat_third is %d\n",
			 chip->limits.ufcs_full_normal_hw_vbat_third);

	rc = of_property_read_u32(node, "oplus,ufcs_full_ffc_vbat",
				  &chip->limits.ufcs_full_ffc_vbat);
	if (rc)
		chip->limits.ufcs_full_ffc_vbat = 4420;
	else
		chg_info("oplus,ufcs_full_ffc_vbat is %d\n",
			 chip->limits.ufcs_full_ffc_vbat);

	rc = of_property_read_u32(node, "oplus,ufcs_full_warm_vbat",
				  &chip->limits.ufcs_full_warm_vbat);
	if (rc)
		chip->limits.ufcs_full_warm_vbat = 4150;
	else
		chg_info("oplus,ufcs_full_warm_vbat is %d\n",
			 chip->limits.ufcs_full_warm_vbat);

	rc = of_property_read_u32(node, "oplus,ufcs_timeout_third",
				  &chip->limits.ufcs_timeout_third);
	if (rc)
		chip->limits.ufcs_timeout_third = 9000;
	else
		chg_info("oplus,ufcs_timeout_third is %d\n",
			 chip->limits.ufcs_timeout_third);

	rc = of_property_read_u32(node, "oplus,ufcs_timeout_oplus",
				  &chip->limits.ufcs_timeout_oplus);
	if (rc)
		chip->limits.ufcs_timeout_oplus = 7200;
	else
		chg_info("oplus,ufcs_timeout_oplus is %d\n",
			 chip->limits.ufcs_timeout_oplus);

	rc = of_property_read_u32(node, "oplus,ufcs_ibat_over_third",
				  &chip->limits.ufcs_ibat_over_third);
	if (rc)
		chip->limits.ufcs_ibat_over_third = 4000;
	else
		chg_info("oplus,ufcs_ibat_over_third is %d\n",
			 chip->limits.ufcs_ibat_over_third);

	rc = of_property_read_u32(node, "oplus,ufcs_ibat_over_oplus",
				  &chip->limits.ufcs_ibat_over_oplus);
	if (rc)
		chip->limits.ufcs_ibat_over_oplus = 17000;
	else
		chg_info("oplus,ufcs_ibat_over_oplus is %d\n",
			 chip->limits.ufcs_ibat_over_oplus);

	rc = of_property_read_u32(node, "oplus,ufcs_current_change_timeout",
				  &chip->ufcs_current_change_timeout);
	if (rc)
		chip->ufcs_current_change_timeout = 100;
	chg_info("ufcs_current_change_timeout is %d\n",
		 chip->ufcs_current_change_timeout);

	rc = of_property_read_u32(node, "oplus,ufcs_bcc_max_curr",
				  &chip->bcc_max_curr);
	if (rc)
		chip->bcc_max_curr = 15000;
	else
		chg_info("oplus,ufcs_bcc_max_curr is %d\n", chip->bcc_max_curr);

	rc = of_property_read_u32(node, "oplus,ufcs_bcc_min_curr",
				  &chip->bcc_min_curr);
	if (rc)
		chip->bcc_min_curr = 1000;
	else
		chg_info("oplus,ufcs_bcc_min_curr is %d\n", chip->bcc_min_curr);

	rc = of_property_read_u32(node, "oplus,ufcs_bcc_exit_curr",
				  &chip->bcc_exit_curr);
	if (rc)
		chip->bcc_exit_curr = 1000;
	else
		chg_info("oplus,ufcs_bcc_exit_curr is %d\n",
			 chip->bcc_exit_curr);

	rc = of_property_count_elems_of_size(
		node, "oplus,ufcs_strategy_batt_high_temp", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 6)
			length = 6;
		rc = of_property_read_u32_array(node, "oplus,ufcs_strategy_batt_high_temp",
						(u32 *)high_temp_tmp, length);
		if (rc < 0)
			chg_err("parse ufcs_strategy_batt_high_temp failed, "
				 "rc=%d\n", rc);
	} else {
		length = 6;
		chg_err("parse ufcs_strategy_batt_high_temp error, rc=%d\n", rc);
	}

	chip->limits.ufcs_strategy_batt_high_temp0 = high_temp_tmp[0];
	chip->limits.ufcs_strategy_batt_high_temp1 = high_temp_tmp[1];
	chip->limits.ufcs_strategy_batt_high_temp2 = high_temp_tmp[2];
	chip->limits.ufcs_strategy_batt_low_temp0 = high_temp_tmp[3];
	chip->limits.ufcs_strategy_batt_low_temp1 = high_temp_tmp[4];
	chip->limits.ufcs_strategy_batt_low_temp2 = high_temp_tmp[5];
	chg_info("length = %d, high_temp[%d, %d, %d, %d, %d, %d]\n", length,
		 high_temp_tmp[0], high_temp_tmp[1], high_temp_tmp[2],
		 high_temp_tmp[3], high_temp_tmp[4], high_temp_tmp[5]);

	rc = of_property_count_elems_of_size(
		node, "oplus,ufcs_strategy_high_current", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 6)
			length = 6;
		rc = of_property_read_u32_array(node, "oplus,ufcs_strategy_high_current",
						(u32 *)high_curr_tmp, length);
		if (rc < 0) {
			chg_err("parse ufcs_strategy_high_current failed, "
				 "rc=%d\n", rc);
		}
	} else {
		length = 6;
		chg_err("parse ufcs_strategy_high_current error, rc=%d\n", rc);
	}
	chip->limits.ufcs_strategy_high_current0 = high_curr_tmp[0];
	chip->limits.ufcs_strategy_high_current1 = high_curr_tmp[1];
	chip->limits.ufcs_strategy_high_current2 = high_curr_tmp[2];
	chip->limits.ufcs_strategy_low_current0 = high_curr_tmp[3];
	chip->limits.ufcs_strategy_low_current1 = high_curr_tmp[4];
	chip->limits.ufcs_strategy_low_current2 = high_curr_tmp[5];
	chg_info("length = %d, high_current[%d, %d, %d, %d, %d, %d]\n", length,
		 high_curr_tmp[0], high_curr_tmp[1], high_curr_tmp[2],
		 high_curr_tmp[3], high_curr_tmp[4], high_curr_tmp[5]);

	rc = of_property_count_elems_of_size(
		node, "oplus,ufcs_charge_strategy_temp", sizeof(u32));
	if (rc >= 0) {
		chip->limits.ufcs_strategy_temp_num = rc;
		if (chip->limits.ufcs_strategy_temp_num > 7)
			chip->limits.ufcs_strategy_temp_num = 7;
		rc = of_property_read_u32_array(
			node, "oplus,ufcs_charge_strategy_temp",
			(u32 *)rang_temp_tmp,
			chip->limits.ufcs_strategy_temp_num);
		if (rc < 0)
			chg_err("parse ufcs_charge_strategy_temp failed, "
				"rc=%d\n", rc);
	} else {
		chip->limits.ufcs_strategy_temp_num = 7;
		chg_err("parse ufcs_charge_strategy_temp error, rc=%d\n", rc);
	}
	chip->limits.ufcs_batt_over_low_temp = rang_temp_tmp[0];
	chip->limits.ufcs_little_cold_temp = rang_temp_tmp[1];
	chip->limits.ufcs_cool_temp = rang_temp_tmp[2];
	chip->limits.ufcs_little_cool_temp = rang_temp_tmp[3];
	chip->limits.ufcs_normal_low_temp = rang_temp_tmp[4];
	chip->limits.ufcs_normal_high_temp = rang_temp_tmp[5];
	chip->limits.ufcs_batt_over_high_temp = rang_temp_tmp[6];
	chg_info("ufcs_charge_strategy_temp num = %d, [%d, %d, %d, %d, %d, "
		 "%d, %d]\n",
		 chip->limits.ufcs_strategy_temp_num, rang_temp_tmp[0],
		 rang_temp_tmp[1], rang_temp_tmp[2], rang_temp_tmp[3],
		 rang_temp_tmp[4], rang_temp_tmp[5], rang_temp_tmp[6]);
	chip->limits.default_ufcs_normal_high_temp =
		chip->limits.ufcs_normal_high_temp;
	chip->limits.default_ufcs_normal_low_temp =
		chip->limits.ufcs_normal_low_temp;
	chip->limits.default_ufcs_little_cool_temp =
		chip->limits.ufcs_little_cool_temp;
	chip->limits.default_ufcs_cool_temp = chip->limits.ufcs_cool_temp;
	chip->limits.default_ufcs_little_cold_temp =
		chip->limits.ufcs_little_cold_temp;

	rc = of_property_count_elems_of_size(
		node, "oplus,ufcs_charge_strategy_soc", sizeof(u32));
	if (rc >= 0) {
		chip->limits.ufcs_strategy_soc_num = rc;
		if (chip->limits.ufcs_strategy_soc_num > 7)
			chip->limits.ufcs_strategy_soc_num = 7;
		rc = of_property_read_u32_array(
			node, "oplus,ufcs_charge_strategy_soc", (u32 *)soc_tmp,
			chip->limits.ufcs_strategy_soc_num);
		if (rc < 0)
			chg_err("parse ufcs_charge_strategy_soc failed, "
				"rc=%d\n", rc);
	} else {
		chip->limits.ufcs_strategy_soc_num = 7;
		chg_err("parse ufcs_charge_strategy_soc error, rc=%d\n", rc);
	}

	chip->limits.ufcs_strategy_soc_over_low = soc_tmp[0];
	chip->limits.ufcs_strategy_soc_min = soc_tmp[1];
	chip->limits.ufcs_strategy_soc_low = soc_tmp[2];
	chip->limits.ufcs_strategy_soc_mid_low = soc_tmp[3];
	chip->limits.ufcs_strategy_soc_mid = soc_tmp[4];
	chip->limits.ufcs_strategy_soc_mid_high = soc_tmp[5];
	chip->limits.ufcs_strategy_soc_high = soc_tmp[6];
	chg_info("ufcs_charge_strategy_soc num = %d, [%d, %d, %d, %d, %d, %d, "
		 "%d]\n",
		 chip->limits.ufcs_strategy_soc_num, soc_tmp[0], soc_tmp[1],
		 soc_tmp[2], soc_tmp[3], soc_tmp[4], soc_tmp[5], soc_tmp[6]);

	rc = of_property_count_elems_of_size(
		node, "oplus,ufcs_low_curr_full_strategy_temp", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 4)
			length = 4;
		rc = of_property_read_u32_array(
			node, "oplus,ufcs_low_curr_full_strategy_temp",
			(u32 *)low_curr_temp_tmp, length);
		if (rc < 0)
			chg_err("parse ufcs_low_curr_full_strategy_temp "
				"failed, rc=%d\n", rc);
	} else {
		length = 4;
		chg_err("parse ufcs_low_curr_full_strategy_temp error, rc=%d\n", rc);
	}

	chip->limits.ufcs_low_curr_full_cool_temp = low_curr_temp_tmp[0];
	chip->limits.ufcs_low_curr_full_little_cool_temp = low_curr_temp_tmp[1];
	chip->limits.ufcs_low_curr_full_normal_low_temp = low_curr_temp_tmp[2];
	chip->limits.ufcs_low_curr_full_normal_high_temp = low_curr_temp_tmp[3];
	chg_info("length = %d, low_curr_temp[%d, %d, %d, %d]\n", length,
		 low_curr_temp_tmp[0], low_curr_temp_tmp[1],
		 low_curr_temp_tmp[2], low_curr_temp_tmp[3]);

	rc = of_property_read_u32(node, "oplus_spec,ufcs_low_temp", &chip->limits.ufcs_low_temp);
	if (rc < 0)
		chip->limits.ufcs_low_temp = 0;
	else
		chg_info("oplus_spec,ufcs_low_temp is %d\n", chip->limits.ufcs_low_temp);
	rc = of_property_read_u32(node, "oplus_spec,ufcs_high_temp", &chip->limits.ufcs_high_temp);
	if (rc < 0)
		chip->limits.ufcs_high_temp = 500;
	else
		chg_info("oplus_spec,ufcs_high_temp is %d\n", chip->limits.ufcs_high_temp);
	rc = of_property_read_u32(node, "oplus_spec,ufcs_low_soc", &chip->limits.ufcs_low_soc);
	if (rc < 0)
		chip->limits.ufcs_low_soc = 0;
	else
		chg_info("oplus_spec,ufcs_low_soc is %d\n", chip->limits.ufcs_low_soc);
	rc = of_property_read_u32(node, "oplus_spec,ufcs_high_soc", &chip->limits.ufcs_high_soc);
	if (rc < 0)
		chip->limits.ufcs_high_soc = 90;
	else
		chg_info("oplus_spec,ufcs_high_soc is %d\n", chip->limits.ufcs_high_soc);

	return rc;
}

static int oplus_ufcs_parse_low_curr_full_curves(struct oplus_ufcs *chip)
{
	struct device_node *node, *full_node;
	int rc = 0, i, length;

	if (!chip)
		return -ENODEV;

	node = chip->dev->of_node;

	full_node = of_get_child_by_name(node, "ufcs_charge_low_curr_full");
	if (!full_node) {
		chg_err("Can not find ufcs_charge_low_curr_full node\n");
		return -EINVAL;
	}

	for (i = 0; i < UFCS_LOW_CURR_FULL_CURVE_TEMP_MAX; i++) {
		rc = of_property_count_elems_of_size(full_node, strategy_low_curr_full[i], sizeof(u32));
		if (rc < 0) {
			chg_err("Count low curr %s failed, rc=%d\n", strategy_low_curr_full[i], rc);
			return rc;
		}

		length = rc;
		switch (i) {
		case UFCS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL:
			rc = of_property_read_u32_array(full_node, strategy_low_curr_full[i],
							(u32 *)chip->low_curr_full_curves_temp[i].full_curves, length);
			chip->low_curr_full_curves_temp[i].full_curve_num = length / 3;
			break;
		case UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW:
			rc = of_property_read_u32_array(full_node, strategy_low_curr_full[i],
							(u32 *)chip->low_curr_full_curves_temp[i].full_curves, length);
			chip->low_curr_full_curves_temp[i].full_curve_num = length / 3;
			break;
		case UFCS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH:
			rc = of_property_read_u32_array(full_node, strategy_low_curr_full[i],
							(u32 *)chip->low_curr_full_curves_temp[i].full_curves, length);
			chip->low_curr_full_curves_temp[i].full_curve_num = length / 3;
			break;
		default:
			break;
		}
	}

	return rc;
}

static int oplus_ufcs_parse_dt(struct oplus_ufcs *chip)
{
	struct device_node *node = chip->dev->of_node;
	struct oplus_ufcs_config *config = &chip->config;
	int rc;

	rc = of_property_read_u32(node, "oplus,target_vbus_mv",
				  &config->target_vbus_mv);
	if (rc < 0) {
		chg_err("get oplus,target_vbus_mv property error, rc=%d\n",
			rc);
		config->target_vbus_mv = 11000;
	}

	rc = of_property_read_u32(node, "oplus,curr_max_ma",
				  &config->curr_max_ma);
	if (rc < 0) {
		chg_err("get oplus,curr_max_ma property error, rc=%d\n",
			rc);
		config->curr_max_ma = 3000;
	}
	config->adsp_ufcs_project = of_property_read_bool(node, "oplus,adsp_ufcs_project");

	rc = of_property_read_u32(node, "oplus,curr_table_type",
				  &chip->curr_table_type);
	if (rc < 0) {
		chip->curr_table_type = UFCS_CURR_BIDIRECT_TABLE;
		chg_err("UFCS_CURR_CP_TABLE not obtained,curr_table_type = %d\n",
			chip->curr_table_type);
	}

	(void)oplus_ufcs_parse_charge_strategy(chip);
	(void)oplus_ufcs_parse_low_curr_full_curves(chip);

	return 0;
}

static int oplus_ufcs_vote_init(struct oplus_ufcs *chip)
{
	int rc;

	chip->ufcs_curr_votable = create_votable("UFCS_CURR", VOTE_MIN, oplus_ufcs_curr_vote_callback, chip);
	if (IS_ERR(chip->ufcs_curr_votable)) {
		rc = PTR_ERR(chip->ufcs_curr_votable);
		chg_err("creat ufcs_curr_votable error, rc=%d\n", rc);
		chip->ufcs_curr_votable = NULL;
		return rc;
	}
	vote(chip->ufcs_curr_votable, MAX_VOTER, true, chip->config.curr_max_ma, false);

	chip->ufcs_disable_votable =
		create_votable("UFCS_DISABLE", VOTE_SET_ANY, oplus_ufcs_disable_vote_callback, chip);
	if (IS_ERR(chip->ufcs_disable_votable)) {
		rc = PTR_ERR(chip->ufcs_disable_votable);
		chg_err("creat ufcs_disable_votable error, rc=%d\n", rc);
		chip->ufcs_disable_votable = NULL;
		goto creat_disable_votable_err;
	}

	chip->ufcs_not_allow_votable =
		create_votable("UFCS_NOT_ALLOW", VOTE_MIN, oplus_ufcs_not_allow_vote_callback, chip);
	if (IS_ERR(chip->ufcs_not_allow_votable)) {
		rc = PTR_ERR(chip->ufcs_not_allow_votable);
		chg_err("creat ufcs_not_allow_votable error, rc=%d\n", rc);
		chip->ufcs_not_allow_votable = NULL;
		goto creat_not_allow_votable_err;
	}

	chip->ufcs_boot_votable =
		create_votable("UFCS_BOOT", VOTE_SET_ANY, oplus_ufcs_boot_vote_callback, chip);
	if (IS_ERR(chip->ufcs_boot_votable)) {
		rc = PTR_ERR(chip->ufcs_boot_votable);
		chg_err("creat ufcs_boot_votable error, rc=%d\n", rc);
		chip->ufcs_boot_votable = NULL;
		goto creat_boot_votable_err;
	}
	vote(chip->ufcs_boot_votable, AUTH_VOTER, true, 1, false);
	vote(chip->ufcs_boot_votable, SHELL_TEMP_VOTER, true, 1, false);
	vote(chip->ufcs_boot_votable, COMM_TOPIC_VOTER, true, 1, false);
	vote(chip->ufcs_boot_votable, WIRED_TOPIC_VOTER, true, 1, false);
	vote(chip->ufcs_boot_votable, GAUGE_TOPIC_VOTER, true, 1, false);
	vote(chip->ufcs_boot_votable, CPA_TOPIC_VOTER, true, 1, false);

	return 0;

creat_boot_votable_err:
	destroy_votable(chip->ufcs_not_allow_votable);
creat_not_allow_votable_err:
	destroy_votable(chip->ufcs_disable_votable);
creat_disable_votable_err:
	destroy_votable(chip->ufcs_curr_votable);
	return rc;
}

static int oplus_ufcs_get_input_node_impedance(void *data)
{
	struct oplus_ufcs *chip;
	int vac, iin, vchg;
	int r_mohm;
	int rc;

	if (data == NULL)
		return -EINVAL;
	chip = data;

	if (!chip->ufcs_charging) {
		chg_err("ufcs_charging is false, can't read input node impedance\n");
		return -EINVAL;
	}

	rc = oplus_ufcs_cp_get_vac(chip, &vac);
	if (rc < 0) {
		chg_debug("not support get cp vac, rc=%d, try vin\n", rc);
		rc = oplus_ufcs_cp_get_vin(chip, &vac);
		if (rc < 0) {
			chg_err("cant get cp vin, rc=%d\n", rc);
			return rc;
		}
	}
	rc = oplus_ufcs_cp_get_iin(chip, &iin);
	if (rc < 0 || iin == 0) {
		chg_debug("direct charge project use sourceinfo iin, rc=%d\n", rc);
		iin =  UFCS_SOURCE_INFO_CURR(chip->src_info);
	}

	vchg = UFCS_SOURCE_INFO_VOL(chip->src_info);

	r_mohm = (vchg - vac) * 1000 / iin;
	if (r_mohm < 0) {
		chg_err("ufcs_input_node: r_mohm=%d\n", r_mohm);
		r_mohm = 0;
	}
	chg_debug("r_mohm = %d, vchg = %d, vac = %d, iin = %d\n", r_mohm, vchg, vac, iin);

	return r_mohm;
}

static int oplus_ufcs_init_imp_node(struct oplus_ufcs *chip)
{
	struct device_node *imp_node;
	struct device_node *child;
	const char *name;
	int rc;

	imp_node = of_get_child_by_name(chip->dev->of_node, "oplus,impedance_node");
	if (imp_node == NULL)
		return 0;

	for_each_child_of_node(imp_node, child) {
		rc = of_property_read_string(child, "node_name", &name);
		if (rc < 0) {
			chg_err("can't read %s node_name, rc=%d\n", child->name, rc);
			continue;
		}
		if (!strcmp(name, "ufcs_input")) {
			chip->input_imp_node =
				oplus_imp_node_register(child, chip->dev, chip, oplus_ufcs_get_input_node_impedance);
			if (IS_ERR_OR_NULL(chip->input_imp_node)) {
				chg_err("%s register error, rc=%ld\n", child->name, PTR_ERR(chip->input_imp_node));
				chip->input_imp_node = NULL;
				continue;
			}
			oplus_imp_node_set_active(chip->input_imp_node, true);
		} else {
			chg_err("unknown node_name: %s\n", name);
		}
	}

	return 0;
}

static void oplus_ufcs_imp_uint_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_ufcs *chip =
		container_of(dwork, struct oplus_ufcs, imp_uint_init_work);
	static int retry_count = 0;
#define IMP_UINT_INIT_RETRY_MAX	500

	if (!of_property_read_bool(chip->dev->of_node, "oplus,impedance_unit")) {
		chg_err("oplus,impedance_unit not found\n");
		return;
	}

	chip->imp_uint = of_find_imp_uint(chip->dev->of_node, "oplus,impedance_unit");
	if (chip->imp_uint != NULL) {
		vote(chip->ufcs_not_allow_votable, IMP_VOTER, false, 0, false);
		return;
	}

	retry_count++;
	if (retry_count > IMP_UINT_INIT_RETRY_MAX) {
		chg_err("can't get ufcs imp uint\n");
		return;
	}

	/* Try once every 100ms, accumulatively try 50s */
	schedule_delayed_work(&chip->imp_uint_init_work, msecs_to_jiffies(100));
}

static int oplus_ufcs_dev_open(struct inode *inode, struct file *filp)
{
	struct oplus_ufcs *chip = container_of(filp->private_data,
		struct oplus_ufcs, misc_dev);

	filp->private_data = chip;
	chg_info("ufcs_dev: %d-%d\n", imajor(inode), iminor(inode));
	return 0;
}

static ssize_t oplus_ufcs_dev_read(struct file *filp, char __user *buf,
				   size_t count, loff_t *offset)
{
	struct oplus_ufcs *chip = filp->private_data;
	struct ufcs_dev_cmd cmd;
	int rc;

	mutex_lock(&chip->read_lock);
	rc = wait_event_interruptible(chip->read_wq, chip->cmd_data_ok);
	mutex_unlock(&chip->read_lock);
	if (rc)
		return rc;
	if (!chip->cmd_data_ok)
		chg_err("ufcs false wakeup, rc=%d\n", rc);

	mutex_lock(&chip->cmd_data_lock);
	chip->cmd_data_ok = false;
	memcpy(&cmd, &chip->cmd, sizeof(struct ufcs_dev_cmd));
	mutex_unlock(&chip->cmd_data_lock);
	if (copy_to_user(buf, &cmd, sizeof(struct ufcs_dev_cmd))) {
		chg_err("failed to copy to user space\n");
		return -EFAULT;
	}

	return sizeof(struct ufcs_dev_cmd);
}

#define UFCS_IOC_MAGIC			0x66
#define UFCS_NOTIFY_GET_AUTH_DATA	_IOW(UFCS_IOC_MAGIC, 1, char)

static long oplus_ufcs_dev_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct oplus_ufcs *chip = filp->private_data;
	void __user *argp = (void __user *)arg;
	int rc;

	switch (cmd) {
	case UFCS_NOTIFY_GET_AUTH_DATA:
		rc = copy_from_user(&chip->auth_data, argp, UFCS_VERIFY_AUTH_DATA_SIZE);
		if (rc) {
			chg_err("failed copy to user space\n");
			return rc;
		}
		chip->auth_data_ok = true;
		chg_info("auth data ok\n");
		if (delayed_work_pending(&chip->wait_auth_data_work)) {
			cancel_delayed_work_sync(&chip->wait_auth_data_work);
			schedule_delayed_work(&chip->wait_auth_data_work, 0);
		}
		break;
	default:
		chg_err("bad ioctl %u\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t oplus_ufcs_dev_write(struct file *filp, const char __user *buf,
				    size_t count, loff_t *offset)
{
	return count;
}

static const struct file_operations oplus_ufcs_dev_fops = {
	.owner			= THIS_MODULE,
	.llseek			= no_llseek,
	.write			= oplus_ufcs_dev_write,
	.read			= oplus_ufcs_dev_read,
	.open			= oplus_ufcs_dev_open,
	.unlocked_ioctl		= oplus_ufcs_dev_ioctl,
};

static int oplus_ufcs_misc_dev_reg(struct oplus_ufcs *chip)
{
	int rc;

	mutex_init(&chip->read_lock);
	mutex_init(&chip->cmd_data_lock);
	init_waitqueue_head(&chip->read_wq);
	chip->cmd_data_ok = false;
	init_completion(&chip->cmd_ack);

	chip->misc_dev.minor = MISC_DYNAMIC_MINOR;
	chip->misc_dev.name = "ufcs_dev";
	chip->misc_dev.fops = &oplus_ufcs_dev_fops;
	rc = misc_register(&chip->misc_dev);
	if (rc)
		chg_err("misc_register failed, rc=%d\n", rc);

	return rc;
}

static int oplus_ufcs_event_notifier_call(struct notifier_block *nb, unsigned long val, void *v)
{
	struct oplus_ufcs *chip = container_of(nb, struct oplus_ufcs, nb);

	switch (val) {
	case UFCS_NOTIFY_SOURCE_HW_RESET:
		chg_info("source hard reset\n");
		schedule_work(&chip->force_exit_work);
		break;
	case UFCS_NOTIFY_CABLE_HW_RESET:
		chg_info("cable hard reset\n");
		break;
	case UFCS_NOTIFY_POWER_CHANGE:
		chg_info("power changed\n");
		break;
	case UFCS_NOTIFY_EXIT:
		chg_info("exit ufcs mode\n");
		schedule_work(&chip->soft_exit_work);
		break;
	case UFCS_NOTIFY_TEST_MODE_CHANGED:
		if (!oplus_chg_ufcs_is_test_mode(chip)) {
			chg_info("exit test mode\n");
			schedule_work(&chip->soft_exit_work);
		} else {
			chg_info("enter test mode\n");
		}
		break;
	case UFCS_NOTIFY_VOL_ACC_TEST_MODE_CHANGED:
		chg_info("vol_acc_test_mode=%d\n", oplus_chg_ufcs_is_vol_acc_test_mode(chip));
		break;
	case UFCS_NOTIFY_ERR_FLAG:
		chip->err_flag = *((unsigned int *)v);
		chg_info("err_flag=%x\n", chip->err_flag);
		schedule_work(&chip->err_flag_push_work);
		break;
	default:
		chg_err("unknown event=%lu\n", val);
		break;
	}

	return NOTIFY_OK;
}

static int oplus_ufcs_notify_reg(struct oplus_ufcs *chip)
{
	int rc;

	chip->nb.notifier_call = oplus_ufcs_event_notifier_call;
	rc = ufcs_reg_event_notifier(&chip->nb);
	if (rc < 0) {
		chg_err("register ufcs event notifier error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int oplus_ufcs_probe(struct platform_device *pdev)
{
	struct oplus_ufcs *chip;
	const char *name;
	struct device_node *startegy_node;
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_ufcs), GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc oplus_ufcs struct buffer error\n");
		return 0;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	oplus_ufcs_parse_dt(chip);
	INIT_DELAYED_WORK(&chip->switch_check_work, oplus_ufcs_switch_check_work);
	INIT_DELAYED_WORK(&chip->monitor_work, oplus_ufcs_monitor_work);
	INIT_DELAYED_WORK(&chip->current_work, oplus_ufcs_current_work);
	INIT_DELAYED_WORK(&chip->imp_uint_init_work, oplus_ufcs_imp_uint_init_work);
	INIT_DELAYED_WORK(&chip->wait_auth_data_work, oplus_ufcs_wait_auth_data_work);
	INIT_WORK(&chip->wired_online_work, oplus_ufcs_wired_online_work);
	INIT_WORK(&chip->force_exit_work, oplus_ufcs_force_exit_work);
	INIT_WORK(&chip->soft_exit_work, oplus_ufcs_soft_exit_work);
	INIT_WORK(&chip->gauge_update_work, oplus_ufcs_gauge_update_work);
	INIT_WORK(&chip->err_flag_push_work, oplus_ufcs_err_flag_push_work);

	rc = oplus_ufcs_init_imp_node(chip);
	if (rc < 0)
		goto imp_node_init_err;

	rc = oplus_ufcs_vote_init(chip);
	if (rc < 0)
		goto vote_init_err;

	rc = oplus_ufcs_misc_dev_reg(chip);
	if (rc < 0)
		goto misc_dev_reg_err;

	rc = oplus_ufcs_notify_reg(chip);
	if (rc < 0)
		goto nb_reg_err;

	startegy_node = of_get_child_by_name(pdev->dev.of_node, "ufcs_charge_third_strategy");
	if (startegy_node == NULL) {
		chg_err("ufcs_charge_third_strategy not found\n");
		rc = -ENODEV;
		goto third_startegy_err;
	}
	chip->third_curve_strategy = oplus_chg_strategy_alloc_by_node("pps_ufcs_curve", startegy_node);
	if (IS_ERR_OR_NULL(chip->third_curve_strategy)) {
		chg_err("alloc third curve startegy error, rc=%ld", PTR_ERR(chip->third_curve_strategy));
		chip->third_curve_strategy = NULL;
		rc = -EFAULT;
		goto third_startegy_err;
	}
	startegy_node = of_get_child_by_name(pdev->dev.of_node, "ufcs_charge_oplus_strategy");
	if (startegy_node == NULL) {
		chg_err("ufcs_charge_oplus_strategy not found\n");
		rc = -ENODEV;
		goto oplus_startegy_err;
	}
	chip->oplus_curve_strategy = oplus_chg_strategy_alloc_by_node("pps_ufcs_curve", startegy_node);
	if (IS_ERR_OR_NULL(chip->oplus_curve_strategy)) {
		chg_err("alloc oplus curve startegy error, rc=%ld", PTR_ERR(chip->oplus_curve_strategy));
		chip->oplus_curve_strategy = NULL;
		rc = -EFAULT;
		goto oplus_startegy_err;
	}

	/* Encryption is not required when the maximum current is less than 3A */
	if (chip->config.curr_max_ma <= UFCS_VERIFY_CURR_THR_MA) {
		chg_info("ignore encrypted data\n");
		chip->wait_auth_data_done = true;
		chip->auth_data_ok = true;
	} else {
		chip->wait_auth_data_done = false;
		chip->auth_data_ok = false;
	}

	name = of_get_oplus_chg_ic_name(pdev->dev.of_node, "oplus,cp_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_ufcs_cp_ic_reg_callback, chip);
	if (of_property_read_bool(chip->dev->of_node, "oplus,impedance_unit")) {
		vote(chip->ufcs_not_allow_votable, IMP_VOTER, true, 1, false);
		schedule_delayed_work(&chip->imp_uint_init_work, 0);
	} else {
		chg_err("oplus,impedance_unit not found\n");
	}

	rc = oplus_ufcs_get_verify_data(chip, 1);
	if (rc < 0)
		chg_err("get verify auth data fail, rc=%d\n", rc);
	schedule_delayed_work(&chip->wait_auth_data_work, msecs_to_jiffies(UFCS_GET_AUTH_DATA_TIMEOUT_MS));

	return 0;

oplus_startegy_err:
	if (chip->third_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->third_curve_strategy);
third_startegy_err:
	ufcs_unreg_event_notifier(&chip->nb);
nb_reg_err:
	misc_deregister(&chip->misc_dev);
misc_dev_reg_err:
	if (chip->ufcs_boot_votable != NULL)
		destroy_votable(chip->ufcs_boot_votable);
	if (chip->ufcs_not_allow_votable != NULL)
		destroy_votable(chip->ufcs_not_allow_votable);
	if (chip->ufcs_disable_votable != NULL)
		destroy_votable(chip->ufcs_disable_votable);
	if (chip->ufcs_curr_votable != NULL)
		destroy_votable(chip->ufcs_curr_votable);
vote_init_err:
	if (chip->input_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->input_imp_node);
imp_node_init_err:
	devm_kfree(&pdev->dev, chip);
	return rc;
}

static int oplus_ufcs_remove(struct platform_device *pdev)
{
	struct oplus_ufcs *chip = platform_get_drvdata(pdev);

	if (chip->ufcs_ic)
		oplus_ufcs_virq_unreg(chip);
	if (chip->dpdm_switch)
		oplus_ufcs_dpdm_switch_virq_unreg(chip);
	if (chip->cp_ic)
		oplus_ufcs_cp_virq_unreg(chip);
	if (!IS_ERR_OR_NULL(chip->cpa_subs))
		oplus_mms_unsubscribe(chip->cpa_subs);
	if (!IS_ERR_OR_NULL(chip->wired_subs))
		oplus_mms_unsubscribe(chip->wired_subs);
	if (!IS_ERR_OR_NULL(chip->comm_subs))
		oplus_mms_unsubscribe(chip->comm_subs);
	if (!IS_ERR_OR_NULL(chip->gauge_subs))
		oplus_mms_unsubscribe(chip->gauge_subs);
	if (chip->oplus_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->oplus_curve_strategy);
	if (chip->third_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->third_curve_strategy);
	ufcs_unreg_event_notifier(&chip->nb);
	misc_deregister(&chip->misc_dev);
	if (chip->ufcs_boot_votable != NULL)
		destroy_votable(chip->ufcs_boot_votable);
	if (chip->ufcs_not_allow_votable != NULL)
		destroy_votable(chip->ufcs_not_allow_votable);
	if (chip->ufcs_disable_votable != NULL)
		destroy_votable(chip->ufcs_disable_votable);
	if (chip->ufcs_curr_votable != NULL)
		destroy_votable(chip->ufcs_curr_votable);
	if (chip->input_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->input_imp_node);
	devm_kfree(&pdev->dev, chip);

	return 0;
}

static const struct of_device_id oplus_ufcs_match[] = {
	{ .compatible = "oplus,ufcs_charge" },
	{},
};

static struct platform_driver oplus_ufcs_driver = {
	.driver		= {
		.name = "oplus-ufcs_charge",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_ufcs_match),
	},
	.probe		= oplus_ufcs_probe,
	.remove		= oplus_ufcs_remove,
};

static __init int oplus_ufcs_init(void)
{
	return platform_driver_register(&oplus_ufcs_driver);
}

static __exit void oplus_ufcs_exit(void)
{
	platform_driver_unregister(&oplus_ufcs_driver);
}

oplus_chg_module_register(oplus_ufcs);

int oplus_ufcs_current_to_level(struct oplus_mms *mms, int curr)
{
	int level = 0;
	struct oplus_ufcs *chip;

	if (curr <= 0)
		return level;
	if (mms == NULL) {
		chg_err("mms is NULL");
		return level;
	}

	chip = oplus_mms_get_drvdata(mms);
	if (chip->curr_table_type == UFCS_CURR_CP_TABLE)
		level = ufcs_find_level_to_current(curr, g_ufcs_cp_current_table, ARRAY_SIZE(g_ufcs_cp_current_table));
	else
		level = ufcs_find_level_to_current(curr, g_ufcs_current_table, ARRAY_SIZE(g_ufcs_current_table));

	return level;
}

enum fastchg_protocol_type oplus_ufcs_adapter_id_to_protocol_type(u32 id)
{
	switch (id) {
	case UFCS_FASTCHG_TYPE_V1:
		return PROTOCOL_CHARGING_UFCS_OPLUS;
	case UFCS_FASTCHG_TYPE_THIRD:
		return PROTOCOL_CHARGING_UFCS_THIRD;
	default:
		return PROTOCOL_CHARGING_UFCS_OPLUS;
	}
}

int oplus_ufcs_adapter_id_to_power(u32 id)
{
	switch (id) {
	case UFCS_FASTCHG_TYPE_V1:
		return UFCS_POWER_TYPE_V1;
	case UFCS_FASTCHG_TYPE_THIRD:
		return UFCS_POWER_TYPE_V1;
	default:
		return UFCS_POWER_TYPE_V1;
	}
}
