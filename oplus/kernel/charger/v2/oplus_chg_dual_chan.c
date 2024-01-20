// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 . Oplus All rights reserved.
 */

#define pr_fmt(fmt) "[DUAL_CHAN]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/power_supply.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#endif
#include <linux/sched/clock.h>
#include <linux/proc_fs.h>
#include <linux/firmware.h>
#include <linux/timer.h>
#include <linux/debugfs.h>
#include <linux/sched.h>

#include <oplus_chg.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_module.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_mms_wired.h>
#include <oplus_mms_gauge.h>
#include <oplus_smart_chg.h>
#include <oplus_chg_monitor.h>
#include <oplus_chg_vooc.h>
#include <oplus_parallel.h>
#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
#include "oplus_cfg.h"
#endif
#include <oplus_chg_dual_chan.h>
#include "monitor/oplus_chg_track.h"

#define OPLUS_DUAL_CHAN_CHECK_MAX	10
#define DUAL_CHAN_INFO_COUNT		4
#define DUAL_CHAN_PAGE_SIZE		64
#define OPLUS_DUAL_CHAN_BUCK_COUNT	2
#define OPLUS_CHG_INTERVAL(time)	round_jiffies_relative(msecs_to_jiffies(time * 1000))
#define DUAL_CHAN_CHECK_DEALY		1
#define UCP_DOWN			0
#define UCP_UP				1

struct oplus_dual_chan_chip {
	struct device *dev;
	struct oplus_mms *common_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *main_gauge_topic;
	struct oplus_mms *sub_gauge_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *dual_chan_topic;
	struct mms_subscribe *common_subs;
	struct mms_subscribe *vooc_subs;
	struct oplus_chg_ic_dev *voocphy_ic;
	bool led_on;
	bool last_enable_normal_chg;
	bool enable_normal_chg;
	int curr_level;
	int buck_ibus_check_count;
	int buck_curr_gear;
	int buck_curr_pre_gear;
	int buck_ibus_check[OPLUS_DUAL_CHAN_CHECK_MAX];
	int buck_curr_change_count[OPLUS_DUAL_CHAN_CHECK_MAX];
	int buck_ucp_thre;
	int open_buck_thre;
	int close_buck_thre;
	int open_buck_count;
	int close_buck_count;
	struct mutex dual_chan_chg_mutex;
	struct delayed_work dual_chan_init_work;
	struct delayed_work dual_chan_check_work;
	struct delayed_work dual_chan_reset_vote_work;
	struct delayed_work dual_chan_reset_work;
	struct delayed_work dual_chan_track_work;
	bool support_dual_chan_buck_charge;
	int buck_ibus_data[OPLUS_DUAL_CHAN_CHECK_MAX];
	int buck_ibus_data_count;
	int buck_ibat_data[OPLUS_DUAL_CHAN_CHECK_MAX];
	int buck_ibat_data_count;
	unsigned int vooc_sid;
	bool vooc_started;
	int track_err_type;
	bool err_report_exit;
	bool err_report_enter;
	struct votable *chg_suspend_votable;
	int debug_dual_chan_err;
	int batt_volt;
	int sub_batt_volt;
	int main_ibat_ma;
	int sub_ibat_ma;
	int main_batt_temp;
	int sub_batt_temp;
	int main_batt_soc;
	int sub_batt_soc;
};

static struct oplus_dual_chan_chip *g_dual_chan_chip;

static bool is_gauge_topic_available(struct oplus_dual_chan_chip *chip)
{
	if (!chip->gauge_topic)
		chip->gauge_topic = oplus_mms_get_by_name("gauge");

	return !!chip->gauge_topic;
}

static bool is_main_gauge_topic_available(struct oplus_dual_chan_chip *chip)
{
	if (!chip->main_gauge_topic)
		chip->main_gauge_topic = oplus_mms_get_by_name("gauge:0");

	return !!chip->main_gauge_topic;
}

static bool is_sub_gauge_topic_available(struct oplus_dual_chan_chip *chip)
{
	if (!chip->sub_gauge_topic)
		chip->sub_gauge_topic = oplus_mms_get_by_name("gauge:1");

	if (!chip->sub_gauge_topic)
		chg_err(" get gauge:1 error\n");

	return !!chip->sub_gauge_topic;
}

static bool is_voocphy_ic_available(struct oplus_dual_chan_chip *chip)
{
	if (!chip->voocphy_ic)
		chip->voocphy_ic = of_get_oplus_chg_ic(chip->dev->of_node,
						       "oplus,voocphy_ic", 0);

	return !!chip->voocphy_ic;
}

static bool is_chg_suspend_votable_available(struct oplus_dual_chan_chip *chip)
{
	if (!chip->chg_suspend_votable)
		chip->chg_suspend_votable = find_votable("WIRED_CHARGE_SUSPEND");
	return !!chip->chg_suspend_votable;
}

static void reset_variables(struct oplus_dual_chan_chip *chip)
{
	chip->curr_level = 0;
	chip->enable_normal_chg = false;
	chip->last_enable_normal_chg = false;
	chip->buck_curr_gear = -1;
	chip->buck_curr_pre_gear = -1;
	chip->open_buck_count = 0;
	chip->close_buck_count = 0;
	memset(chip->buck_curr_change_count, 0, OPLUS_DUAL_CHAN_CHECK_MAX);
	chip->track_err_type = 0;
	chip->err_report_enter = false;
	chip->err_report_exit = false;
}

static int oplus_dual_chan_get_ibat_check_gear_from_dt(struct oplus_dual_chan_chip *chip,
			struct device_node *node)
{
	int length = 0;
	int rc;
	int i;

	if (!node) {
		chg_err("device tree info. missing\n");
		return -ENOMEM;
	}

	length = of_property_count_elems_of_size(node, "oplus,dual_chan_buck_ichg_check", sizeof(u32));
	if (length < 0) {
		chg_err("Count buck_ibus_check_count failed, rc=%d\n", length);
		chip->buck_ibus_check_count = 0;
		return length;
	}

	if (length > OPLUS_DUAL_CHAN_CHECK_MAX) {
		length = OPLUS_DUAL_CHAN_CHECK_MAX;
	}

	chip->buck_ibus_check_count = length;
	rc = of_property_read_u32_array(node, "oplus,dual_chan_buck_ichg_check",
							(u32 *)&chip->buck_ibus_check[0],
							length);
	if (rc < 0) {
		chg_err("parse buck_ibus_check failed, rc=%d\n", rc);
		chip->buck_ibus_check_count = 0;
	} else {
		chg_info("buck_ibus_check_count length =%d\n", chip->buck_ibus_check_count);
		for (i = 0; i < chip->buck_ibus_check_count; i++) {
			chg_info("index: %d curr: %d\n", i, chip->buck_ibus_check[i]);
		}
	}

	chg_info("count: %d\n", chip->buck_ibus_check_count);

	return 0;
}

static int dual_chan_get_ibus_and_ibat_from_dt(struct oplus_dual_chan_chip *chip, struct device_node *node)
{
	int length = 0;
	int rc;
	int i;

	if (!node) {
		chg_err("device tree info. missing\n");
		return -ENOMEM;
	}

	length = of_property_count_elems_of_size(node, "oplus,dual_chan_buck_ibus", sizeof(u32));
	if (length < 0) {
		chg_err("Count dual_chan_buck_ibus failed, rc=%d\n", length);
		chip->buck_ibus_data_count = 0;
		return length;
	}

	if (length > OPLUS_DUAL_CHAN_CHECK_MAX)
		length = OPLUS_DUAL_CHAN_CHECK_MAX;

	chip->buck_ibus_data_count = length;
	rc = of_property_read_u32_array(node, "oplus,dual_chan_buck_ibus",
					(u32 *)&chip->buck_ibus_data[0],
					length);
	if (rc < 0) {
		chg_err("parse buck_ibus_data failed, rc=%d\n", rc);
		chip->buck_ibus_data_count = 0;
	} else {
		for (i = 0; i < chip->buck_ibus_data_count; i++) {
			chg_info("index: %d curr: %d\n", i, chip->buck_ibus_data[i]);
		}
	}

	length = of_property_count_elems_of_size(node, "oplus,dual_chan_buck_ibat", sizeof(u32));
	if (length < 0) {
		chg_err("Count dual_chan_buck_ibat failed, rc=%d\n", length);
		chip->buck_ibat_data_count = 0;
		return length;
	}

	if (length > OPLUS_DUAL_CHAN_CHECK_MAX)
		length = OPLUS_DUAL_CHAN_CHECK_MAX;

	chip->buck_ibat_data_count = length;
	rc = of_property_read_u32_array(node, "oplus,dual_chan_buck_ibat",
					(u32 *)&chip->buck_ibat_data[0],
					length);
	if (rc < 0) {
		chg_err("parse buck_ibat_data failed, rc=%d\n", rc);
		chip->buck_ibat_data_count = 0;
	} else {
		for (i = 0; i < chip->buck_ibat_data_count; i++) {
			chg_info("index: %d curr: %d\n", i, chip->buck_ibat_data[i]);
		}
	}

	return 0;
}

static int oplus_dual_chan_parse_dt(struct oplus_dual_chan_chip *chip)
{
	int rc = 0;
	struct device_node *node = NULL;

	if (!chip || !chip->dev) {
		chg_err("oplus_mos_dev null!\n");
		return -1;
	}

	node = chip->dev->of_node;

	oplus_dual_chan_get_ibat_check_gear_from_dt(chip, node);
	dual_chan_get_ibus_and_ibat_from_dt(chip, node);

	rc = of_property_read_u32(node, "oplus,dual_chan_buck_ucp_ibat_thre",
	                          &chip->buck_ucp_thre);
	if (rc) {
		chip->buck_ucp_thre = 0;
	}
	chg_info("oplus dual chan buck_ucp_thre is %d\n", chip->buck_ucp_thre);

	rc = of_property_read_u32(node, "oplus,dual_chan_open_buck_thre",
	                          &chip->open_buck_thre);
	if (rc) {
		chip->open_buck_thre = 0;
	}
	chg_info("oplus dual chan open_buck_thre is %d\n", chip->open_buck_thre);

	rc = of_property_read_u32(node, "oplus,dual_chan_close_buck_thre",
	                          &chip->close_buck_thre);
	if (rc) {
		chip->close_buck_thre = 0;
	}
	chg_info("oplus dual chan close_buck_thre is %d\n", chip->close_buck_thre);

	return 0;
}

static int oplus_check_ibus_for_the_buck(struct oplus_dual_chan_chip *chip, int curr)
{
	int i = 0;

	for (i = 0; i < chip->buck_ibus_check_count; i++) {
		if (curr > chip->buck_ibus_check[i]) {
			chg_info("get i = %d buck ibus = %d buck ibus count=%d curr level =%d\n",
					i, chip->buck_ibus_check[i], chip->buck_curr_change_count[i], chip->curr_level);
			if (chip->buck_curr_change_count[i] == OPLUS_DUAL_CHAN_BUCK_COUNT && chip->curr_level != i + 1) {
				chip->curr_level = i + 1;
				memset(chip->buck_curr_change_count, 0, OPLUS_DUAL_CHAN_CHECK_MAX);
			} else {
				if (i != chip->buck_curr_gear)
					chip->buck_curr_change_count[i] = chip->buck_curr_change_count[i] + 1;
			}
			break;
		} else {
			chip->buck_curr_change_count[i] = 0;
		}
	}

	/* From the logic of the code, -1 is the expected initial value */
	/* the init value of gear is -1 */
	return chip->curr_level - 1;
}

#define OPLUS_DUAL_CHAN_VOLT_JUDGE 50
static bool oplus_dual_chan_get_vbat_status(struct oplus_dual_chan_chip *chip)
{
	int fv_mv = 0;
	int gauge_vbatt, main_vbatt = 0, sub_vbatt = 0;
	union mms_msg_data data = {0};
	int rc = 0;

	if (!chip) {
		chg_err("chip is NULL, return!\n");
		return false;
	}

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic is null, return!\n");
		return false;
	}

	if (is_support_parallel_battery(chip->gauge_topic)) {
		if (is_main_gauge_topic_available(chip)) {
			oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
			main_vbatt = data.intval;
		}
		if (is_sub_gauge_topic_available(chip)) {
			oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
			sub_vbatt = data.intval;
		}
		gauge_vbatt = main_vbatt > sub_vbatt ? main_vbatt : sub_vbatt;
	} else {
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
		gauge_vbatt = data.intval;
	}

	rc = oplus_wired_get_fv(&fv_mv);
	if (rc < 0)
		return false;
	chg_debug("dual chan get fv = %d, gauge vbatt = %d\n", fv_mv, gauge_vbatt);

	if (gauge_vbatt >= (fv_mv - OPLUS_DUAL_CHAN_VOLT_JUDGE))
		return false;
	else
		return true;
}

static bool oplus_get_vooc_normal_temp_status(struct oplus_dual_chan_chip *chip)
{
	union mms_msg_data data = {0};

	if (!chip)
		return false;

	if (!chip->vooc_topic)
		return false;

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_TEMP_RANGE, &data, true);
	if (data.intval == FASTCHG_TEMP_RANGE_NORMAL_HIGH ||
	    data.intval == FASTCHG_TEMP_RANGE_NORMAL_LOW)
		return true;
	else
		return false;
}

static bool oplus_get_led_status_only_for_dual_chan(struct oplus_dual_chan_chip *chip)
{
	chg_debug("get led status %s\n", chip->led_on ? "on" : "off");

/* add for 806 aging. According to this scenario,
the dual channel should be opened when the screen is on*/
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	if (get_eng_version() == FACTORY)
		return false;
	else
		return chip->led_on;
#else
	return chip->led_on;
#endif
}

static bool oplus_get_dual_chan_status(struct oplus_dual_chan_chip *chip)
{
	if (!chip)
		return false;

	if (!chip->enable_normal_chg && !oplus_wired_input_is_enable()) {
		return false;
	} else if (chip->enable_normal_chg && oplus_wired_input_is_enable()) {
		return true;
	} else {
		chg_err("normal chg flag !=  register status!\n");
		return false;
	}
}

static int oplus_dual_chan_disable_charger(bool disable)
{
	struct votable *disable_votable;
	int rc;

	disable_votable = find_votable("WIRED_CHARGING_DISABLE");
	if (!disable_votable) {
		chg_err("WIRED_CHARGING_DISABLE votable not found\n");
		return -EINVAL;
	}

	rc = vote(disable_votable, FASTCHG_VOTER, disable,
		  disable ? 1 : 0, false);
	if (rc < 0)
		chg_err("%s charger error, rc = %d\n",
			disable ? "disable" : "enable", rc);
	else
		chg_info("%s charger\n", disable ? "disable" : "enable");

	return rc;
}

static int oplus_dual_chan_suspend_charger(bool suspend)
{
	struct votable *suspend_votable;
	int rc;

	suspend_votable = find_votable("WIRED_CHARGE_SUSPEND");
	if (!suspend_votable) {
		chg_err("WIRED_CHARGE_SUSPEND votable not found\n");
		return -EINVAL;
	}

	rc = vote(suspend_votable, FASTCHG_VOTER, suspend,
		  suspend ? 1 : 0, false);
	if (rc < 0)
		chg_err("%s charger error, rc = %d\n",
			suspend ? "suspend" : "unsuspend", rc);
	else
		chg_info("%s charger\n", suspend ? "suspend" : "unsuspend");

        return rc;
}

#define OPLUS_DUAL_CHAN_BUCK_DEFAULT_INPUT_CURR 150
#define OPLUS_DUAL_CHAN_BUCK_DEFAULT_CHARGING_CURR 300
static int oplus_dual_chan_set_icl(int icl_curr, bool aicl, bool enable)
{
	struct votable *wired_icl_votable;
	int rc;

	wired_icl_votable = find_votable("WIRED_ICL");
	if (!wired_icl_votable) {
		chg_err("WIRED_ICL votable not found\n");
		return -EINVAL;
	}

	rc = vote(wired_icl_votable, FASTCHG_VOTER, enable, icl_curr, aicl);
	if (rc < 0)
		chg_err("vote icl = %d rc = %d\n", icl_curr, rc);
	else
		chg_info("set icl success = %d\n", icl_curr);

	return rc;
}

static int oplus_dual_chan_set_fcc(int fcc_curr, bool enable)
{
	struct votable *wired_fcc_votable;
	int rc;

	wired_fcc_votable = find_votable("WIRED_FCC");
	if (!wired_fcc_votable) {
		chg_err("WIRED_FCC votable not found\n");
		return -EINVAL;
	}

	rc = vote(wired_fcc_votable, FASTCHG_VOTER, enable, fcc_curr, true);
	if (rc < 0)
		chg_err("vote fcc = %d rc = %d\n", fcc_curr, rc);
	else
		chg_info("set fcc success = %d\n", fcc_curr);

	return rc;
}

static void oplus_dual_chan_chg_enable(bool enable)
{
	chg_debug("enable = %d\n", enable);
	if (enable) {
		oplus_dual_chan_disable_charger(!enable);
		oplus_dual_chan_set_icl(OPLUS_DUAL_CHAN_BUCK_DEFAULT_INPUT_CURR, false, true);
		oplus_dual_chan_suspend_charger(!enable);
		oplus_dual_chan_set_icl(OPLUS_DUAL_CHAN_BUCK_DEFAULT_INPUT_CURR, false, true);
		oplus_dual_chan_set_fcc(OPLUS_DUAL_CHAN_BUCK_DEFAULT_CHARGING_CURR, true);
	} else {
		oplus_dual_chan_disable_charger(!enable);
		oplus_dual_chan_suspend_charger(!enable);
		oplus_dual_chan_set_icl(OPLUS_DUAL_CHAN_BUCK_DEFAULT_INPUT_CURR, false, false);
		oplus_dual_chan_set_fcc(OPLUS_DUAL_CHAN_BUCK_DEFAULT_CHARGING_CURR, false);
	}
}

static int oplus_dual_chan_set_buck_status(struct oplus_dual_chan_chip *chip, bool enable)
{
	int rc;
	struct mms_msg *msg;

	if (!chip) {
		return -EINVAL;
	}

	mutex_lock(&chip->dual_chan_chg_mutex);
	if (enable) {
		if (is_voocphy_ic_available(chip)) {
			rc = oplus_chg_ic_func(chip->voocphy_ic,
					       OPLUS_IC_FUNC_VOOCPHY_SET_UCP_TIME, UCP_UP);
			if (rc < 0) {
				chg_err("can't set ucp, rc=%d\n", rc);
				mutex_unlock(&chip->dual_chan_chg_mutex);
				return rc;
			}
		} else {
			chg_err("voocphy ic is null, can't set ucp\n");
		}
		oplus_dual_chan_chg_enable(true);
		chip->enable_normal_chg = true;
		chip->open_buck_count = 0;
		chg_info("set dual chan open!\n");
	} else {
		if (is_voocphy_ic_available(chip)) {
			rc = oplus_chg_ic_func(chip->voocphy_ic,
					       OPLUS_IC_FUNC_VOOCPHY_SET_UCP_TIME, UCP_DOWN);
			if (rc < 0) {
				chg_err("can't set ucp, rc=%d\n", rc);
				mutex_unlock(&chip->dual_chan_chg_mutex);
				return rc;
			}
		} else {
			chg_err("voocphy ic is null, can't set ucp\n");
		}
		oplus_dual_chan_chg_enable(false);
		chip->enable_normal_chg = false;
		chip->close_buck_count = 0;
		chip->curr_level = 0;
		chg_info("disable buck chg!\n");
	}
	mutex_unlock(&chip->dual_chan_chg_mutex);

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  SWITCH_ITEM_DUAL_CHAN_STATUS);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -EINVAL;
	}
	rc = oplus_mms_publish_msg(chip->dual_chan_topic, msg);
	if (rc < 0) {
		chg_err("publish SWITCH_ITEM_DUAL_CHAN_STATUS msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return 0;
}

void oplus_dual_chan_buck_reset(void)
{
	if (!g_dual_chan_chip)
		return;

	if (oplus_get_dual_chan_status(g_dual_chan_chip))
		schedule_delayed_work(&g_dual_chan_chip->dual_chan_reset_work, 0);
}

void oplus_dual_chan_curr_vote_reset(void)
{
	if (!g_dual_chan_chip)
		return;

	schedule_delayed_work(&g_dual_chan_chip->dual_chan_reset_vote_work, 0);
}

bool oplus_get_dual_chan_support(void)
{
	if (g_dual_chan_chip)
		return true;
	else
		return false;
}

static void oplus_dual_chan_err_report(int type)
{
	if (!g_dual_chan_chip)
		return;

	chg_err("dual_chan_err_report %d\n", type);
	g_dual_chan_chip->track_err_type = type;
	schedule_delayed_work(&g_dual_chan_chip->dual_chan_track_work, 0);
}

int oplus_get_dual_chan_info(char *buf)
{
	unsigned char tmpbuf[DUAL_CHAN_PAGE_SIZE] = {0};
	int buffer[DUAL_CHAN_INFO_COUNT] = {0};
	int len = 0;
	int i = 0;
	int idx = 0;

	if (!g_dual_chan_chip)
		return -ENODEV;

	if (oplus_get_dual_chan_support())
		buffer[0] = 1;
	else
		buffer[0] = 0;

	if (oplus_dual_chan_get_vbat_status(g_dual_chan_chip))
		buffer[1] = 1;
	else
		buffer[1] = 0;

	if (oplus_get_dual_chan_status(g_dual_chan_chip))
		buffer[2] = 1;
	else
		buffer[2] = 0;

	if (oplus_get_vooc_normal_temp_status(g_dual_chan_chip))
		buffer[3] = 1;
	else
		buffer[3] = 0;

	chg_info("get info, support = %d, vbat_status = %d, status = %d, temp_range = %d\n",
		buffer[0], buffer[1], buffer[2], buffer[3]);

	for (i = 0; i <= DUAL_CHAN_INFO_COUNT - 1; i++) {
		len = snprintf(tmpbuf, DUAL_CHAN_PAGE_SIZE - idx, "%d,", buffer[i]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
	}

	chg_info("buf:%s\n", buf);

	return 0;
}

static int oplus_dual_chan_set_curr_gear(struct oplus_dual_chan_chip *chip, int gear)
{
	int buck_ibus = 0;
	int buck_ibat = 0;
	int max_count = 0;

	if (chip->buck_ibus_data_count > chip->buck_ibat_data_count)
		max_count = chip->buck_ibat_data_count - 1;
	else
		max_count = chip->buck_ibus_data_count - 1;

	if (max_count == -1) {
		chg_err("max count is -1, please check dts!\n");
		return -EINVAL;
	}

	chg_info("gear = %d max_count = %d\n", gear, max_count);

	if (gear < 0) {
		gear = 0;
	} else if (gear >= max_count) {
		gear = max_count;
	}

	buck_ibus = chip->buck_ibus_data[gear];
	buck_ibat = chip->buck_ibat_data[gear];

	chg_info("buck ibus = %d buck ibat = %d\n", buck_ibus, buck_ibat);
	oplus_dual_chan_set_icl(buck_ibus, false, true);
	oplus_dual_chan_set_fcc(buck_ibat, true);

	return 0;
}

static void oplus_mms_dual_chan_update(struct oplus_mms *mms, bool publish)
{
}

static int oplus_chg_update_dual_chan_status(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_dual_chan_chip *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	data->intval = chip->enable_normal_chg;

	return 0;
}

static struct mms_item oplus_chg_dual_chan_item[] = {
	{
		.desc = {
			.item_id = SWITCH_ITEM_DUAL_CHAN_STATUS,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_update_dual_chan_status,
		}
	}
};

static const u32 oplus_dual_chan_update_item[] = {
	SWITCH_ITEM_DUAL_CHAN_STATUS,
};

static const struct oplus_mms_desc oplus_mms_dual_chan_desc = {
	.name = "dual_chan",
	.type = OPLUS_MMS_TYPE_DUAL_CHAN,
	.item_table = oplus_chg_dual_chan_item,
	.item_num = ARRAY_SIZE(oplus_chg_dual_chan_item),
	.update_items = oplus_dual_chan_update_item,
	.update_items_num = ARRAY_SIZE(oplus_dual_chan_update_item),
	.update_interval = 0, /* ms */
	.update = oplus_mms_dual_chan_update,
};

static int oplus_chg_dual_chan_topic_init(struct oplus_dual_chan_chip *chip)
{
	struct oplus_mms_config mms_cfg = {};
	int rc;

	mms_cfg.drv_data = chip;
	mms_cfg.of_node = chip->dev->of_node;

	chip->dual_chan_topic = devm_oplus_mms_register(chip->dev, &oplus_mms_dual_chan_desc, &mms_cfg);
	if (IS_ERR(chip->dual_chan_topic)) {
		chg_err("Couldn't register dual_chan_topic\n");
		rc = PTR_ERR(chip->dual_chan_topic);
		return rc;
	}

	return 0;
}

static int dual_chan_track_debugfs_init(struct oplus_dual_chan_chip *chip)
{
	int ret = 0;
	struct dentry *debugfs_root;
	struct dentry *debugfs_dual_chan;

	debugfs_root = oplus_chg_track_get_debugfs_root();
	if (!debugfs_root) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_dual_chan = debugfs_create_dir("dual_chan", debugfs_root);
	if (!debugfs_dual_chan) {
		ret = -ENOENT;
		return ret;
	}

	chip->debug_dual_chan_err = 0;
	debugfs_create_u32("debug_dual_chan_err", 0644,
			   debugfs_dual_chan, &(chip->debug_dual_chan_err));

	return ret;
}

static void oplus_chg_dual_chan_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_dual_chan_chip *chip = container_of(dwork,
		struct oplus_dual_chan_chip, dual_chan_init_work);

	oplus_chg_dual_chan_topic_init(chip);
	if (dual_chan_track_debugfs_init(chip) < 0)
		chg_err("init dual_chan_track_debugfs fail\n");
}

static void oplus_chg_dual_chan_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_dual_chan_chip *chip = container_of(dwork,
		struct oplus_dual_chan_chip, dual_chan_check_work);
	union mms_msg_data data = {0};
	int vbat_temp_cur;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic is null, return!\n");
		return;
	}

	if (chip->debug_dual_chan_err) {
		chg_err("debug_dual_chan_err set:%d \n", chip->debug_dual_chan_err);
		oplus_dual_chan_err_report(chip->debug_dual_chan_err);
	}

	if (chip->enable_normal_chg && !oplus_wired_input_is_enable() &&
	    is_chg_suspend_votable_available(chip) &&
	    !get_effective_result(chip->chg_suspend_votable) && !chip->err_report_exit) {
		chg_err("buck off when dual chan enable\n");
		chip->err_report_exit = true;
		oplus_dual_chan_err_report(DUAL_CHAN_EXIT_ERR);
	}

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
	vbat_temp_cur = -data.intval;
	chg_info("led_status = %d, vbat_status = %d!, curr = %d, open_buck_thre = %d, temp_status = %d, dual_chan_status = %d\n",
		 oplus_get_led_status_only_for_dual_chan(chip), oplus_dual_chan_get_vbat_status(chip),
		 vbat_temp_cur, chip->open_buck_thre, oplus_get_vooc_normal_temp_status(chip), oplus_get_dual_chan_status(chip));
	if (vbat_temp_cur > chip->open_buck_thre &&
	    !oplus_get_led_status_only_for_dual_chan(chip) &&
	    oplus_dual_chan_get_vbat_status(chip) &&
	    oplus_get_vooc_normal_temp_status(chip) &&
	    !oplus_get_dual_chan_status(chip)) {
		if (chip->open_buck_count >= OPLUS_DUAL_CHAN_BUCK_COUNT) {
			oplus_dual_chan_set_buck_status(chip, true);
		} else {
			chip->open_buck_count = chip->open_buck_count + 1;
		}
	} else if ((vbat_temp_cur < chip->close_buck_thre ||
		oplus_get_led_status_only_for_dual_chan(chip) ||
		!oplus_dual_chan_get_vbat_status(chip) ||
		!oplus_get_vooc_normal_temp_status(chip)) &&
		oplus_get_dual_chan_status(chip)) {
		if (chip->close_buck_count >= OPLUS_DUAL_CHAN_BUCK_COUNT) {
			oplus_dual_chan_set_buck_status(chip, false);
		} else {
			chip->close_buck_count = chip->close_buck_count + 1;
		}
	} else {
		chip->open_buck_count = 0;
		chip->close_buck_count = 0;
	}

	chg_info("open count = %d close count = %d! enable_normal_chg = %d\n",
		chip->open_buck_count, chip->close_buck_count, chip->enable_normal_chg);
	if (oplus_get_dual_chan_status(chip)) {
		chip->buck_curr_gear = oplus_check_ibus_for_the_buck(chip, vbat_temp_cur);
		chg_info("gear = %d pre_gear = %d!\n", chip->buck_curr_gear, chip->buck_curr_pre_gear);
		if ((chip->buck_curr_gear != chip->buck_curr_pre_gear) && (chip->buck_curr_gear != -1)) {
			chip->buck_curr_pre_gear = chip->buck_curr_gear;
			oplus_dual_chan_set_curr_gear(chip, chip->buck_curr_gear);
		}
	}

	if (chip->last_enable_normal_chg && !chip->enable_normal_chg && oplus_wired_input_is_enable() &&
	    is_chg_suspend_votable_available(chip) &&
	    get_effective_result(chip->chg_suspend_votable) && !chip->err_report_enter) {
		chg_err("buck on when dual chan disable\n");
		chip->err_report_enter = true;
		oplus_dual_chan_err_report(DUAL_CHAN_ENTER_ERR);
	}
	chip->last_enable_normal_chg = chip->enable_normal_chg;
	schedule_delayed_work(&chip->dual_chan_check_work, OPLUS_CHG_INTERVAL(DUAL_CHAN_CHECK_DEALY));
	return;
}

static void oplus_chg_dual_chan_reset_vote_work(struct work_struct *work)
{
	oplus_dual_chan_set_icl(OPLUS_DUAL_CHAN_BUCK_DEFAULT_INPUT_CURR, false, false);
	oplus_dual_chan_set_fcc(OPLUS_DUAL_CHAN_BUCK_DEFAULT_CHARGING_CURR, false);
	return;
}

static void oplus_chg_dual_chan_reset_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_dual_chan_chip *chip = container_of(dwork,
		struct oplus_dual_chan_chip, dual_chan_reset_work);

	chg_info("dual chan reset\n");
	oplus_dual_chan_set_buck_status(chip, false);
	return;
}

#define TRACK_LOCAL_T_NS_TO_S_THD		1000000000
#define TRACK_UPLOAD_COUNT_MAX			10
#define TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD	(24 * 3600)
static int track_get_local_time_s(void)
{
	int local_time_s;

	local_time_s = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;
	pr_info("local_time_s:%d\n", local_time_s);

	return local_time_s;
}

#define REASON_LENGTH_MAX 256
static int oplus_dual_chan_get_track_info(struct oplus_dual_chan_chip *chip,
	char *temp_str, int index)
{
	union mms_msg_data data = {0};

	if (!chip)
		return 0;

	if (is_main_gauge_topic_available(chip)) {
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					true);
		chip->batt_volt = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_CURR, &data,
					true);
		chip->main_ibat_ma = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_TEMP, &data,
					true);
		chip->main_batt_temp = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_SOC, &data,
					true);
		chip->main_batt_soc = data.intval;
	} else {
		chip->batt_volt = 0;
		chip->main_ibat_ma = 0;
		chip->main_batt_temp = 0;
		chip->main_batt_soc = 0;
	}
	if (is_sub_gauge_topic_available(chip)) {
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					true);
		chip->sub_batt_volt = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_CURR, &data,
					true);
		chip->sub_ibat_ma = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_TEMP, &data,
					true);
		chip->sub_batt_temp = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_SOC, &data,
					true);
		chip->sub_batt_soc = data.intval;
	} else {
		chip->sub_batt_volt = 0;
		chip->sub_ibat_ma = 0;
		chip->sub_batt_temp = 0;
		chip->sub_batt_soc = 0;
	}

	return snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index,
			"main_sub_soc %d %d,"
			"main_sub_volt %d %d,"
			"main_sub_curr %d %d,"
			"main_sub_temp %d %d,"
			"buck ibus %d",
			chip->main_batt_soc, chip->sub_batt_soc,
			chip->batt_volt, chip->sub_batt_volt,
			chip->main_ibat_ma, chip->sub_ibat_ma,
			chip->main_batt_temp, chip->sub_batt_temp,
			oplus_wired_get_ibus());
}

static void oplus_chg_dual_chan_track_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_dual_chan_chip *chip = container_of(dwork,
		struct oplus_dual_chan_chip, dual_chan_track_work);
	struct oplus_mms *err_topic;
	struct mms_msg *msg = NULL;
	union mms_msg_data data = {0};
	int bat_curr;
	int rc;
	static int upload_count = 0;
	static int pre_upload_time = 0;
	int curr_time;
	char temp_str[REASON_LENGTH_MAX] = {0};
	int index = 0;

	if (is_gauge_topic_available(chip)) {
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
		bat_curr = -data.intval;
	} else {
		bat_curr = 0;
	}

	err_topic = oplus_mms_get_by_name("error");
	if (!err_topic) {
		chg_err("error topic not found\n");
		return;
	}

	curr_time = track_get_local_time_s();
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;
	if (chip->track_err_type && upload_count < TRACK_UPLOAD_COUNT_MAX) {
		upload_count++;
		pre_upload_time = track_get_local_time_s();
		chg_info("dual chan track trigger: %d\n", chip->track_err_type);
		switch(chip->track_err_type) {
		case DUAL_CHAN_EXIT_ERR:
			index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index,
					  "$$reason@@%s$$status@@", "dual_chan_exit_err");
			break;
		case DUAL_CHAN_ENTER_ERR:
			index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index,
					  "$$reason@@%s$$status@@", "dual_chan_enter_err");
			break;
		default:
			break;
		}
		index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index,
				  "curr:%d led:%d vbat_status:%d "
			          "temp_status:%d buck_status:%d ",
			          bat_curr,
				  oplus_get_led_status_only_for_dual_chan(chip),
				  oplus_dual_chan_get_vbat_status(chip),
				  oplus_get_vooc_normal_temp_status(chip),
				  oplus_wired_input_is_enable());
		index += oplus_dual_chan_get_track_info(chip, temp_str, index);
		msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_DUAL_CHAN,
					      temp_str);
		if (msg == NULL) {
			chg_err("alloc dual chan error msg error\n");
			return;
		}
		rc = oplus_mms_publish_msg(err_topic, msg);
		if (rc < 0) {
			chg_err("publish dual chan error msg error, rc=%d\n", rc);
			kfree(msg);
		}
		chip->track_err_type = 0;
	}

	return;
}


static void oplus_dual_chan_common_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	struct oplus_dual_chan_chip *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_LED_ON:
			oplus_mms_get_item_data(chip->common_topic, id, &data,
						false);
			chip->led_on = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_dual_chan_subscribe_common_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_dual_chan_chip *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->common_topic = topic;
	chip->common_subs = oplus_mms_subscribe(chip->common_topic, chip,
						oplus_dual_chan_common_subs_callback,
						"dual_chan");
	if (IS_ERR_OR_NULL(chip->common_subs))
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->common_subs));

	oplus_mms_get_item_data(chip->common_topic, COMM_ITEM_LED_ON, &data,
				true);
	chip->led_on = !!data.intval;
}

static void oplus_dual_chan_vooc_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	struct oplus_dual_chan_chip *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_VOOC_STARTED:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_started = !!data.intval;
			if (chip->vooc_started && sid_to_adapter_chg_type(chip->vooc_sid) == CHARGER_TYPE_SVOOC) {
				reset_variables(chip);
				schedule_delayed_work(&chip->dual_chan_check_work, 0);
			} else if (!chip->vooc_started) {
				cancel_delayed_work_sync(&chip->dual_chan_check_work);
				reset_variables(chip);
			}
			break;
		case VOOC_ITEM_SID:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_sid = (unsigned int)data.intval;
			if (chip->vooc_started && sid_to_adapter_chg_type(chip->vooc_sid) == CHARGER_TYPE_SVOOC) {
				reset_variables(chip);
				schedule_delayed_work(&chip->dual_chan_check_work, 0);
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

static void oplus_dual_chan_subscribe_vooc_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_dual_chan_chip *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->vooc_topic = topic;
	chip->vooc_subs = oplus_mms_subscribe(chip->vooc_topic, chip,
					       oplus_dual_chan_vooc_subs_callback,
					       "dual_chan");
	if (IS_ERR_OR_NULL(chip->vooc_subs))
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(chip->vooc_subs));

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_STARTED, &data,
				false);
	chip->vooc_started = !!data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_SID, &data,
				false);
	chip->vooc_sid = (unsigned int)data.intval;

	if (chip->vooc_started && sid_to_adapter_chg_type(chip->vooc_sid) == CHARGER_TYPE_SVOOC) {
		reset_variables(chip);
		schedule_delayed_work(&chip->dual_chan_check_work, 0);
	}
}

static int oplus_dual_chan_probe(struct platform_device *pdev)
{
	struct oplus_dual_chan_chip *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_dual_chan_chip),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	of_platform_populate(chip->dev->of_node, NULL, NULL, chip->dev);

	oplus_dual_chan_parse_dt(chip);
	mutex_init(&chip->dual_chan_chg_mutex);
	INIT_DELAYED_WORK(&chip->dual_chan_init_work, oplus_chg_dual_chan_init_work);
	INIT_DELAYED_WORK(&chip->dual_chan_check_work, oplus_chg_dual_chan_check_work);
	INIT_DELAYED_WORK(&chip->dual_chan_reset_vote_work, oplus_chg_dual_chan_reset_vote_work);
	INIT_DELAYED_WORK(&chip->dual_chan_reset_work, oplus_chg_dual_chan_reset_work);
	INIT_DELAYED_WORK(&chip->dual_chan_track_work, oplus_chg_dual_chan_track_work);
	oplus_mms_wait_topic("common", oplus_dual_chan_subscribe_common_topic, chip);
	oplus_mms_wait_topic("vooc", oplus_dual_chan_subscribe_vooc_topic, chip);
	reset_variables(chip);
	g_dual_chan_chip = chip;
	schedule_delayed_work(&chip->dual_chan_init_work, 0);
	chg_info("probe success\n");

	return 0;
}

static int oplus_dual_chan_remove(struct platform_device *pdev)
{
	struct oplus_dual_chan_chip *chip = platform_get_drvdata(pdev);

	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_dual_chan_match[] = {
	{ .compatible = "oplus,dual_chan" },
	{},
};

static struct platform_driver oplus_dual_chan_driver = {
	.driver		= {
		.name = "oplus-dual_chan",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_dual_chan_match),
	},
	.probe		= oplus_dual_chan_probe,
	.remove		= oplus_dual_chan_remove,
};

static __init int oplus_dual_chan_init(void)
{
	return platform_driver_register(&oplus_dual_chan_driver);
}

static __exit void oplus_dual_chan_exit(void)
{
	platform_driver_unregister(&oplus_dual_chan_driver);
}

oplus_chg_module_register(oplus_dual_chan);
