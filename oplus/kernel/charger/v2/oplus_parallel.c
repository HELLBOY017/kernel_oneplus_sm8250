// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#define pr_fmt(fmt) "[OPLUS_PARALLEL]([%s][%d]): " fmt, __func__, __LINE__

#include "oplus_parallel.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/sched/clock.h>
#include <linux/of_platform.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_chg_monitor.h>
#include <oplus_mms_wired.h>
#include <oplus_mms_gauge.h>
#include <oplus_chg_vooc.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_voter.h>
#ifdef CONFIG_OPLUS_CHARGER_MTK
#include <mtk_boot_common.h>
#endif
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <oplus_chg.h>
#include "monitor/oplus_chg_track.h"
#include <linux/sched.h>

#define VBAT_GAP_STATUS1	800
#define VBAT_GAP_STATUS2	600
#define VBAT_GAP_STATUS3	150
#define VBAT_GAP_STATUS7	400
#define MAIN_STATUS_CHG		1
#define SUB_STATUS_CHG		2
#define ALL_STATUS_CHG		3
#define HYSTERISIS_DECIDEGC	20
#define HYSTERISIS_DECIDEGC_0C	5
#define RATIO_ACC		100
#define OUT_OF_BALANCE_COUNT	10
#define FG_I2C_ERROR 		-400

#define MOS_TEST_DEFAULT_CURRENT 2000
#define SUB_BATT_CURRENT_50_MA 50

#define OPLUS_CHG_UPDATE_INTERVAL(time)	round_jiffies_relative(msecs_to_jiffies(time * 1000))
#define CHECK_TIME_WITH_CHARGER 5
#define CHECK_TIME_WITHOUT_CHARGER 10
#define CURR_SPEC_CHECK_TIME 1

#define OPLUS_CHG_GET_SUB_CURRENT          _IOWR('M', 1, char[256])
#define OPLUS_CHG_GET_SUB_VOLTAGE          _IOWR('M', 2, char[256])
#define OPLUS_CHG_GET_SUB_SOC              _IOWR('M', 3, char[256])
#define OPLUS_CHG_GET_SUB_TEMPERATURE      _IOWR('M', 4, char[256])
#define OPLUS_CHG_GET_PARALLEL_SUPPORT     _IOWR('M', 5, char[256])

struct batt_spec {
	int volt;
	int main_curr;
	int sub_curr;
};

struct parallel_bat_table {
	struct batt_spec * batt_table;
	int length;
};

enum {
	MAIN_TEMP_INIT,
	SUB_TEMP_INIT,
	ALL_TEMP_INIT
};


enum {
	BATTERY_STATUS__REMOVED, /*<-19C*/
	BATTERY_STATUS__LOW_TEMP, /*-19C~-10C*/
	BATTERY_STATUS__COLD_TEMP, /*-10C~0C*/
	BATTERY_STATUS__LITTLE_COLD_TEMP, /*0C~5C*/
	BATTERY_STATUS__COOL_TEMP, /*5C~12C*/
	BATTERY_STATUS__LITTLE_COOL_TEMP, /*12C~16C*/
	BATTERY_STATUS__NORMAL, /*16C~44C*/
	BATTERY_STATUS__WARM_TEMP, /*44C~53C*/
	BATTERY_STATUS__HIGH_TEMP, /*>53C*/
	BATTERY_STATUS__INVALID
};

struct oplus_parallel_chip {
	struct i2c_client *client;
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	struct oplus_switch_operations *switch_ops;
	int error_status;
	int ctrl_type;
	bool normal_chg_check;

	int parallel_vbat_gap_abnormal;
	int parallel_vbat_gap_full;
	int parallel_vbat_gap_recov;
	int parallel_mos_abnormal_litter_curr;
	int parallel_mos_abnormal_gap_curr;

	int main_tbatt_status;
	int main_pre_shake;
	int sub_tbatt_status;
	int sub_pre_shake;

	struct parallel_bat_table *parallel_bat_data;
	int pre_spec_index;
	int pre_sub_spec_index;

	int track_unbalance_high;
	int track_unbalance_low;
	int track_unbalance_soc;
	unsigned int unbalance_count;
	int notify_flag;
	int debug_force_mos_err;

	bool soc_not_full_report;
	struct delayed_work parallel_init_work;
	struct delayed_work parallel_check_work;
	struct delayed_work parallel_curr_spec_check_work;
	struct delayed_work curr_adaptive_check_work;
	struct delayed_work parallel_chg_mos_test_work;
	struct work_struct err_handler_work;

	struct oplus_mms *gauge_topic;
	struct oplus_mms *main_gauge_topic;
	struct oplus_mms *sub_gauge_topic;
	struct oplus_mms *parallel_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *wired_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *err_topic;
	struct mms_subscribe *comm_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *main_gauge_subs;
	struct mms_subscribe *sub_gauge_subs;
	struct mms_subscribe *vooc_subs;
	bool wired_online;
	int balancing_bat_status;
	int batt_volt;
	int sub_batt_volt;
	int main_ibat_ma;
	int sub_ibat_ma;
	int main_batt_temp;
	int sub_batt_temp;
	int main_batt_soc;
	int sub_batt_soc;
	int tbatt_status;
	bool main_batt_full;
	bool sub_batt_full;
	int parallel_error_flag;
	bool vooc_online_keep;
	struct votable *chg_disable_votable;
	struct votable *vooc_disable_votable;
	struct votable *wired_fcc_votable;
	struct votable *vooc_curr_votable;
	struct votable *wired_charging_disable_votable;
	int anti_shake_bound[BATTERY_STATUS__INVALID];
	int main_bat_decidegc[BATTERY_STATUS__INVALID];
	int sub_bat_decidegc[BATTERY_STATUS__INVALID];
	bool mos_test_started;
	int mos_test_result;
	atomic_t mos_lock;
	int target_curr_limit;
	int batt_spec_status;
	atomic_t file_opened;
};

struct oplus_parallel_chip *g_parallel_chip;
static int oplus_chg_parallel_support(void);
static int update_batt_info(struct oplus_parallel_chip *chip, bool update);

__maybe_unused static bool
is_err_topic_available(struct oplus_parallel_chip *chip)
{
	if (!chip->err_topic)
		chip->err_topic = oplus_mms_get_by_name("error");
	return !!chip->err_topic;
}

int oplus_switching_get_error_status(void)
{
/*
	if (!g_parallel_chip || g_parallel_chip->switch_ops ||
	    g_parallel_chip->switch_ops->switching_get_fastcharge_current) {
		return 0;
	}

	if (g_parallel_chip->error_status) {
		chg_err("error_status:%d\n", g_parallel_chip->error_status);
		return g_parallel_chip->error_status;
	} else {
		g_parallel_chip->switch_ops->switching_get_fastcharge_current();
		chg_err("error_status:%d\n", g_parallel_chip->error_status);
		return g_parallel_chip->error_status;
	}
*/
	return 0;
}

__maybe_unused static bool
is_wired_charging_disable_votable_available(struct oplus_parallel_chip *chip)
{
	if (!chip->wired_charging_disable_votable)
		chip->wired_charging_disable_votable =
			find_votable("WIRED_CHARGING_DISABLE");
	return !!chip->wired_charging_disable_votable;
}

int oplus_switching_hw_enable(int en)
{
	int rc;

	if (!g_parallel_chip) {
		return -1;
	}
	rc = oplus_chg_ic_func(g_parallel_chip->ic_dev,
			       OPLUS_IC_FUNC_SET_HW_ENABLE, en);
	if (rc < 0) {
		chg_err("error set switch rc=%d\n", rc);
		return -1;
	}
	return 0;
}

int oplus_parallel_chg_mos_test(struct oplus_mms *topic)
{
	struct oplus_parallel_chip *chip;

	if (topic == NULL) {
		chg_err("topic is NULL\n");
		return 0;
	}
	chip = oplus_mms_get_drvdata(topic);

	if (chip->mos_test_result) {
		chg_err("mos test success, use last result!\n");
		return 1;
	}

	if (!chip->mos_test_started) {
		chip->mos_test_started = true;
		schedule_delayed_work(&chip->parallel_chg_mos_test_work, 0);
	}

	return 0;
}

int oplus_switching_set_fastcharge_current(int curr_ma)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops ||
	    !g_parallel_chip->switch_ops->switching_set_fastcharge_current) {
		return -1;
	}

	return g_parallel_chip->switch_ops->switching_set_fastcharge_current(curr_ma);
*/
	return -1;
}

int oplus_switching_enable_charge(int en)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops ||
	    !g_parallel_chip->switch_ops->switching_enable_charge) {
		return -1;
	} else {
		chg_err("success\n");
		return g_parallel_chip->switch_ops->switching_enable_charge(en);
	}
*/
	return -1;
}

int oplus_switching_get_hw_enable(void)
{
	bool en;
	int rc;

	if (!g_parallel_chip) {
		return -1;
	}
	rc = oplus_chg_ic_func(g_parallel_chip->ic_dev,
			       OPLUS_IC_FUNC_GET_HW_ENABLE, &en);
	if (rc < 0) {
		chg_err("error get switch rc=%d\n", rc);
		return -1;
	}
	return en;
}

bool oplus_switching_get_charge_enable(void)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops
		|| !g_parallel_chip->switch_ops->switching_get_charge_enable) {
		return -1;
	} else {
		return g_parallel_chip->switch_ops->switching_get_charge_enable();
	}
*/
	return -1;
}

int oplus_switching_get_fastcharge_current(void)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops
		|| !g_parallel_chip->switch_ops->switching_get_fastcharge_current) {
		return -1;
	} else {
		return g_parallel_chip->switch_ops->switching_get_fastcharge_current();
	}
*/
	return -1;
}

int oplus_switching_get_discharge_current(void)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops
		|| !g_parallel_chip->switch_ops->switching_get_discharge_current) {
		return -1;
	} else {
		return g_parallel_chip->switch_ops->switching_get_discharge_current();
	}
*/
	return -1;
}

int oplus_switching_set_current(int current_ma)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops
		|| !g_parallel_chip->switch_ops->switching_set_fastcharge_current) {
		return -1;
	}

	chg_err("current_ma:%d\n", current_ma);
	g_parallel_chip->switch_ops->switching_set_fastcharge_current(current_ma);

	return 0;
*/
	return -1;
}

int oplus_switching_set_discharge_current(int current_ma)
{
/*
	if (!g_parallel_chip || !g_parallel_chip->switch_ops
		|| !g_parallel_chip->switch_ops->switching_set_discharge_current) {
		return -1;
	}

	chg_err("current_ma:%d\n", current_ma);
	g_parallel_chip->switch_ops->switching_set_discharge_current(current_ma);

	return 0;
*/
	return -1;
}

static ssize_t oplus_chg_read(struct file *fp, char __user *buff, size_t count, loff_t *off)
{
	char page[256] = { 0 };
	int len;
	update_batt_info(g_parallel_chip, true);
	len = sprintf(page, "sub_current=%d\nsub_voltage=%d\nsub_soc=%d\nsub_temperature=%d\nmain_soc=%d\n",
		      g_parallel_chip->sub_ibat_ma,  g_parallel_chip->sub_batt_volt,
		      g_parallel_chip->sub_batt_soc, g_parallel_chip->sub_batt_temp,
		      g_parallel_chip->main_batt_soc);

	if (len > *off) {
		len -= *off;
	} else {
		len = 0;
	}
	if (copy_to_user(buff, page, (len < count ? len : count))) {
		chg_err("copy_to_user error\n");
		return -EFAULT;
	}
	*off += len < count ? len : count;
	return (len < count ? len : count);
}

static ssize_t oplus_chg_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	return count;
}

static long oplus_chg_ioctl(struct file *fp, unsigned code, unsigned long value)
{
	char src[256] = { 0 };
	int ret;

	update_batt_info(g_parallel_chip, true);
	switch (code) {
	case OPLUS_CHG_GET_SUB_CURRENT:
		ret = sprintf(src, "sub_current=%d\n", g_parallel_chip->sub_ibat_ma);
		break;
	case OPLUS_CHG_GET_SUB_VOLTAGE:
		ret = sprintf(src, "sub_voltage=%d\n", g_parallel_chip->sub_batt_volt);
		break;
	case OPLUS_CHG_GET_SUB_SOC:
		ret = sprintf(src, "sub_soc=%d\n", g_parallel_chip->sub_batt_soc);
		break;
	case OPLUS_CHG_GET_SUB_TEMPERATURE:
		ret = sprintf(src, "sub_temperature=%d\n", g_parallel_chip->sub_batt_temp);
		break;
	case OPLUS_CHG_GET_PARALLEL_SUPPORT:
		if (oplus_chg_parallel_support()) {
			ret = sprintf(src, "support\n");
			return ret;
		} else {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (copy_to_user((void __user *)value, src, ret))
		ret = -EFAULT;
	return ret;
}

static int oplus_chg_open(struct inode *ip, struct file *fp)
{
	if (atomic_xchg(&g_parallel_chip->file_opened, 1)) {
		chg_err("oplus_chg_open failed\n");
		return -EBUSY;
	}
	fp->private_data = g_parallel_chip;
	return 0;
}

static int oplus_chg_release(struct inode *ip, struct file *fp)
{
	WARN_ON(!atomic_xchg(&g_parallel_chip->file_opened, 0));
	/* indicate that we are disconnected
	 * still could be online so don't touch online flag
	 */
	return 0;
}

static const struct file_operations oplus_chg_fops = {
	.owner = THIS_MODULE,
	.read = oplus_chg_read,
	.write = oplus_chg_write,
	.unlocked_ioctl = oplus_chg_ioctl,
	.open = oplus_chg_open,
	.release = oplus_chg_release,
};

static struct miscdevice oplus_chg_device = {
	.name = "oplus_chg",
	.fops = &oplus_chg_fops,
};

static int oplus_vooc_get_fastchg_started(void)
{
	int fastchg_started_status = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	vooc_topic = oplus_mms_get_by_name("vooc");
	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_VOOC_STARTED, &data, true);
	if (!rc)
		fastchg_started_status = data.intval;

	chg_err("get fastchg started status = %d\n", fastchg_started_status);
	return fastchg_started_status;
}

static int oplus_switching_get_if_need_balance_bat(int vbat0_mv, int vbat1_mv)
{
	int diff_volt = 0;
	struct oplus_parallel_chip *chip = g_parallel_chip;
	static int error_count = 0;
	static int fg_error_count = 0;
	static int pre_error_reason = 0;
	int error_reason = 0;
	union mms_msg_data data = { 0 };
	int mmi_chg = 1;

	chg_info("vbat0_mv:%d, vbat1_mv:%d\n", vbat0_mv, vbat1_mv);
	if (!atomic_read(&chip->mos_lock)) {
		chg_err("mos test start, check next time!");
		return 0;
	}

	if (!chip) {
		chg_err("fail\n");
		return -1;
	} else {
		diff_volt = abs(vbat0_mv - vbat1_mv);
		if (chip->sub_batt_temp == FG_I2C_ERROR || chip->main_batt_temp == FG_I2C_ERROR) {
			if (fg_error_count <= BATT_FGI2C_RETRY_COUNT)
				fg_error_count++;
			else
				error_reason |= REASON_I2C_ERROR;
		} else {
			fg_error_count = 0;
		}

		if (oplus_switching_get_error_status()
		    && oplus_chg_parallel_support() == PARALLEL_SWITCH_IC) {
			return PARALLEL_BAT_BALANCE_ERROR_STATUS8;
		}

		if (oplus_vooc_get_fastchg_started() == true && atomic_read(&chip->mos_lock)) {
			if (oplus_switching_get_hw_enable() &&
			    (abs(chip->sub_ibat_ma) < g_parallel_chip->parallel_mos_abnormal_litter_curr ||
			    abs(chip->main_ibat_ma) < g_parallel_chip->parallel_mos_abnormal_litter_curr) &&
			    (abs(chip->main_ibat_ma - chip->sub_ibat_ma) >= g_parallel_chip->parallel_mos_abnormal_gap_curr)) {
				if (error_count < BATT_OPEN_RETRY_COUNT)
					error_count++;
				else
					error_reason |= REASON_MOS_OPEN_ERROR;
			} else {
				error_count = 0;
			}
		}
		oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_TEMP_REGION, &data, false);
		chip->tbatt_status = data.intval;
		if (is_wired_charging_disable_votable_available(chip)) {
			chip->main_batt_full =
				is_client_vote_enabled(chip->wired_charging_disable_votable, CHG_FULL_VOTER);
		} else {
			oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_CHG_FULL, &data, false);
			chip->main_batt_full = data.intval;
		}
		oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_CHG_SUB_BATT_FULL, &data, false);
		chip->sub_batt_full = data.intval;
		if (chip->chg_disable_votable)
			mmi_chg = !get_client_vote(chip->chg_disable_votable,
						   MMI_CHG_VOTER);
		/* if fg_error_count not 0, diff_volt is not believable, skip it */
		if (oplus_chg_parallel_support() == PARALLEL_MOS_CTRL && atomic_read(&chip->mos_lock) &&
		    fg_error_count == 0) {
			if (chip->tbatt_status != TEMP_REGION_WARM &&
			    chip->sub_batt_full &&
			    !chip->main_batt_full &&
			    chip->wired_online &&
			    diff_volt < g_parallel_chip->parallel_vbat_gap_full &&
			    mmi_chg)
		    		error_reason |= REASON_SUB_BATT_FULL;

			if (diff_volt >= g_parallel_chip->parallel_vbat_gap_abnormal ||
			    ((diff_volt >= g_parallel_chip->parallel_vbat_gap_recov) &&
			     (pre_error_reason & REASON_VBAT_GAP_BIG)))
				error_reason |= REASON_VBAT_GAP_BIG;

			if ((pre_error_reason & REASON_SUB_BATT_FULL) &&
			    !oplus_switching_get_hw_enable() &&
			    (diff_volt > g_parallel_chip->parallel_vbat_gap_full ||
			     !mmi_chg)) {
				error_reason &= ~REASON_SUB_BATT_FULL;
				chg_err("sub full,but diff_volt > %d need to recovery MOS\n", g_parallel_chip->parallel_vbat_gap_full);
			}

			if ((pre_error_reason & REASON_VBAT_GAP_BIG)
			    && diff_volt < g_parallel_chip->parallel_vbat_gap_recov)
				error_reason &= ~REASON_VBAT_GAP_BIG;
		}
		if (error_reason != 0) {
			pre_error_reason = error_reason;
			chip->parallel_error_flag &= ~(REASON_SUB_BATT_FULL | REASON_VBAT_GAP_BIG);
			chip->parallel_error_flag |= error_reason;
			chg_err("mos open %d\n", error_reason);
			if ((error_reason & (REASON_I2C_ERROR | REASON_MOS_OPEN_ERROR)) != 0) {
				return PARALLEL_BAT_BALANCE_ERROR_STATUS8;
			}
			return PARALLEL_BAT_BALANCE_ERROR_STATUS9;
		} else if (oplus_chg_parallel_support() == PARALLEL_MOS_CTRL) {
			pre_error_reason = error_reason;
			chip->parallel_error_flag &= ~(REASON_SUB_BATT_FULL | REASON_VBAT_GAP_BIG);
			chip->parallel_error_flag |= error_reason;
			return PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
		}

		if (diff_volt < VBAT_GAP_STATUS3) {
			return PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
		}

		if (vbat0_mv >= 3400 && vbat1_mv < 3400) {
			if (vbat1_mv < 3100) {
				if (vbat0_mv - vbat1_mv <= 1000) {
					return PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE;
				} else {
					return PARALLEL_BAT_BALANCE_ERROR_STATUS6__STOP_CHARGE;
				}
			} else {
				return PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE;
			}
		} else if (vbat0_mv >= vbat1_mv) {
			if (diff_volt >= VBAT_GAP_STATUS1) {
				return PARALLEL_NEED_BALANCE_BAT_STATUS1__STOP_CHARGE;
			} else if (diff_volt >= VBAT_GAP_STATUS2) {
				return PARALLEL_NEED_BALANCE_BAT_STATUS2__STOP_CHARGE;
			} else if (diff_volt >= VBAT_GAP_STATUS3) {
				return PARALLEL_NEED_BALANCE_BAT_STATUS3__STOP_CHARGE;
			} else {
				return PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
			}
		} else if (vbat0_mv < vbat1_mv) {
			if (diff_volt >= VBAT_GAP_STATUS7) {
				return PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE;
			} else if (diff_volt >= VBAT_GAP_STATUS3) {
				return PARALLEL_NEED_BALANCE_BAT_STATUS4__STOP_CHARGE;
			} else {
				return PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
			}
		}
	}
	return -1;
}

int oplus_switching_set_balance_bat_status(int status)
{
	chg_err("status:%d\n", status);
	switch (status) {
	case PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(2800);
		oplus_switching_set_current(2800);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS1__STOP_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(500);
		oplus_switching_set_current(500);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS2__STOP_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(1000);
		oplus_switching_set_current(1000);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS3__STOP_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(2800);
		oplus_switching_set_current(2800);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS4__STOP_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(2800);
		oplus_switching_set_current(2800);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE:
		oplus_switching_hw_enable(1);
		oplus_switching_set_discharge_current(200);
		oplus_switching_set_current(200);
		oplus_switching_enable_charge(1);
		break;
	case PARALLEL_BAT_BALANCE_ERROR_STATUS6__STOP_CHARGE:
		oplus_switching_hw_enable(0);
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE:
		oplus_switching_hw_enable(0);
		break;
	case PARALLEL_BAT_BALANCE_ERROR_STATUS8:
	case PARALLEL_BAT_BALANCE_ERROR_STATUS9:
		oplus_switching_hw_enable(0);
		break;
	default:
		break;
	}
	return 0;
}

static const char * const batt_temp_table[] = {
	[BATTERY_STATUS__COLD_TEMP]		= "cold_temp",
	[BATTERY_STATUS__LITTLE_COLD_TEMP]	= "little_cold_temp",
	[BATTERY_STATUS__COOL_TEMP]		= "cool_temp",
	[BATTERY_STATUS__LITTLE_COOL_TEMP]	= "little_cool_temp",
	[BATTERY_STATUS__NORMAL] 		= "normal_temp",
	[BATTERY_STATUS__WARM_TEMP] 		= "warm_temp",
	[BATTERY_STATUS__REMOVED] 		= "NA",
	[BATTERY_STATUS__LOW_TEMP] 		= "NA",
	[BATTERY_STATUS__HIGH_TEMP] 		= "NA",
};

static void set_bat_decidegc_default(struct oplus_parallel_chip *chip)
{
	chip->anti_shake_bound[BATTERY_STATUS__LOW_TEMP] = -190;
	chip->anti_shake_bound[BATTERY_STATUS__COLD_TEMP] = -100;
	chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COLD_TEMP] = 0;
	chip->anti_shake_bound[BATTERY_STATUS__COOL_TEMP] = 50;
	chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COOL_TEMP] = 120;
	chip->anti_shake_bound[BATTERY_STATUS__NORMAL] = 160;
	chip->anti_shake_bound[BATTERY_STATUS__WARM_TEMP] = 450;
	chip->anti_shake_bound[BATTERY_STATUS__HIGH_TEMP] = 530;
}

static int oplus_parallel_parse_dt(struct oplus_parallel_chip *chip)
{
	int rc = 0;
	struct device_node *node = NULL;
	struct device_node *temp_node = NULL;
	int i;
	int j;
	int length = 0;

	if (!chip || !chip->dev) {
		chg_err("oplus_mos_dev null!\n");
		return -1;
	}

	node = chip->dev->of_node;

	rc = of_property_read_u32(node, "oplus,parallel_vbat_gap_abnormal", &chip->parallel_vbat_gap_abnormal);
	if (rc) {
		chip->parallel_vbat_gap_abnormal = 150;
	}

	rc = of_property_read_u32(node, "oplus,parallel_vbat_gap_full", &chip->parallel_vbat_gap_full);
	if (rc) {
		chip->parallel_vbat_gap_full = 200;
	}

	rc = of_property_read_u32(node, "oplus,parallel_vbat_gap_recov", &chip->parallel_vbat_gap_recov);
	if (rc) {
		chip->parallel_vbat_gap_recov = 100;
	}

	rc = of_property_read_u32(node, "oplus,parallel_mos_abnormal_litter_curr", &chip->parallel_mos_abnormal_litter_curr);
	if (rc) {
		chip->parallel_mos_abnormal_litter_curr = 100;
	}

	rc = of_property_read_u32(node, "oplus,parallel_mos_abnormal_gap_curr", &chip->parallel_mos_abnormal_gap_curr);
	if (rc) {
		chip->parallel_mos_abnormal_gap_curr = 2000;
	}
	chg_info("parallel_vbat_gap_abnormal %d,"
		"parallel_vbat_gap_full %d,"
		"parallel_vbat_gap_recov %d,"
		"parallel_mos_abnormal_litter_curr %d,"
		"parallel_mos_abnormal_gap_curr %d \n",
		chip->parallel_vbat_gap_abnormal, chip->parallel_vbat_gap_full,
		chip->parallel_vbat_gap_recov, chip->parallel_mos_abnormal_litter_curr,
		chip->parallel_mos_abnormal_gap_curr);

	chip->normal_chg_check = of_property_read_bool(node, "normal_chg_check_support");

	rc = of_property_read_u32(node, "track_unbalance_high", &chip->track_unbalance_high);
	if (rc) {
		chip->track_unbalance_high = 100;
	}
	rc = of_property_read_u32(node, "track_unbalance_low", &chip->track_unbalance_low);
	if (rc) {
		chip->track_unbalance_low = 0;
	}
	rc = of_property_read_u32(node, "track_unbalance_soc", &chip->track_unbalance_soc);
	if (rc) {
		chip->track_unbalance_soc = 100;
	}

	temp_node = of_get_child_by_name(node, "parallel_bat_table");
	chip->parallel_bat_data = devm_kzalloc(chip->dev, BATTERY_STATUS__INVALID * sizeof(struct parallel_bat_table), GFP_KERNEL);
	if (temp_node && chip->parallel_bat_data) {
		for (i = 0; i < BATTERY_STATUS__INVALID; i++) {
			rc = of_property_count_elems_of_size(temp_node, batt_temp_table[i], sizeof(u32));
			if (rc > 0 && rc % (sizeof(struct batt_spec)/sizeof(int)) == 0) {
				length = rc;
				chip->parallel_bat_data[i].length = length / (sizeof(struct batt_spec) / sizeof(int));
				chip->parallel_bat_data[i].batt_table = devm_kzalloc(chip->dev, length * sizeof(struct batt_spec), GFP_KERNEL);
				if (chip->parallel_bat_data[i].batt_table) {
					rc = of_property_read_u32_array(temp_node, batt_temp_table[i],
									(u32 *)chip->parallel_bat_data[i].batt_table,
									length);
					if (rc < 0) {
						chg_err("parse bat_table failed, rc=%d\n", rc);
						chip->parallel_bat_data[i].length = 0;
						devm_kfree(chip->dev, chip->parallel_bat_data[i].batt_table);
					} else {
						chg_err("%s length =%d\n",
							batt_temp_table[i], chip->parallel_bat_data[i].length);
						for (j = 0; j < chip->parallel_bat_data[i].length; j++) {
							chg_err("vbatt: %d main_curr: %d sub_curr:%d \n",
								chip->parallel_bat_data[i].batt_table[j].volt,
								chip->parallel_bat_data[i].batt_table[j].main_curr,
								chip->parallel_bat_data[i].batt_table[j].sub_curr);
						}
					}
				}
			}
		}
	}

	rc = of_property_count_elems_of_size(node,  "oplus,bat_decidegc", sizeof(u32));
	length = rc;
	if (rc == BATTERY_STATUS__INVALID - 1) {
		rc = of_property_read_u32_array(node, "oplus,bat_decidegc",
			(u32 *)&chip->anti_shake_bound[BATTERY_STATUS__LOW_TEMP], rc);
		if (rc < 0) {
			chg_err("parse oplus,bat_decidegc failed, rc=%d\n", rc);
			set_bat_decidegc_default(chip);
		} else {
			chip->anti_shake_bound[BATTERY_STATUS__LOW_TEMP] =
				-chip->anti_shake_bound[BATTERY_STATUS__LOW_TEMP];
			chip->anti_shake_bound[BATTERY_STATUS__COLD_TEMP] =
				-chip->anti_shake_bound[BATTERY_STATUS__COLD_TEMP];
			chg_info(" bat_decidegc %d %d %d %d %d %d %d %d\n",
				 chip->anti_shake_bound[BATTERY_STATUS__LOW_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__COLD_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COLD_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__COOL_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COOL_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__NORMAL],
				 chip->anti_shake_bound[BATTERY_STATUS__WARM_TEMP],
				 chip->anti_shake_bound[BATTERY_STATUS__HIGH_TEMP]);
		}
	} else {
		chg_err("parse oplus,bat_decidegc failed, rc=%d\n", rc);
		set_bat_decidegc_default(chip);
	}
	return 0;
}

void oplus_chg_parellel_variables_reset(void)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;

	if (!chip) {
		chg_err("chip not ready\n");
		return;
	}

	chip->pre_spec_index = -1;
	chip->pre_sub_spec_index = -1;
	chip->target_curr_limit = 0;
	chip->main_tbatt_status = -1;
	chip->sub_tbatt_status = -1;
	chip->soc_not_full_report = false;
	chip->notify_flag = 0;
}

static int mos_track_debugfs_init(struct oplus_parallel_chip *chip)
{
	int ret = 0;
	struct dentry *debugfs_root;
	struct dentry *debugfs_mos;

	debugfs_root = oplus_chg_track_get_debugfs_root();
	if (!debugfs_root) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_mos = debugfs_create_dir("mos", debugfs_root);
	if (!debugfs_mos) {
		ret = -ENOENT;
		return ret;
	}

	chip->debug_force_mos_err = TRACK_MOS_ERR_DEFAULT;
	debugfs_create_u32("debug_force_mos_err", 0644,
			   debugfs_mos, &(chip->debug_force_mos_err));

	return ret;
}

static void oplus_parallel_init(struct oplus_parallel_chip *chip)
{
	if (!chip) {
		chg_err("oplus_parallel_chip not specified!\n");
		return;
	}

	oplus_parallel_parse_dt(chip);
	chip->balancing_bat_status = PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
	oplus_chg_parellel_variables_reset();
	chip->mos_test_started = false;
	chip->mos_test_result = 0;
	atomic_set(&chip->mos_lock, 1);
	chip->unbalance_count = 0;
	chip->parallel_error_flag = 0;
}

static int oplus_chg_parallel_support(void)
{
	int type = NO_PARALLEL_TYPE;
	int rc;

	if (!g_parallel_chip) {
		return type;
        } else {
        	rc = oplus_chg_ic_func(g_parallel_chip->ic_dev,
			       OPLUS_IC_FUNC_GET_DEVICE_TYPE, &type);
		if (rc < 0) {
			chg_err("error get switch device type rc=%d\n", rc);
		}
		return type;
	}
}

static bool oplus_support_normal_batt_spec_check(void)
{
	if (!g_parallel_chip) {
		return false;
        } else {
		return g_parallel_chip->normal_chg_check;
	}
}

void oplus_init_parallel_temp_threshold(int init_type)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int i;

	if (!chip || !g_parallel_chip) {
		chg_err("oplus_chg_chip not specified!\n");
		return;
	}

	if (init_type == MAIN_TEMP_INIT || init_type == ALL_TEMP_INIT) {
		for (i = BATTERY_STATUS__HIGH_TEMP; i>= BATTERY_STATUS__LOW_TEMP; i--) {
			chip->main_bat_decidegc[i] = chip->anti_shake_bound[i];
		}
	}
	if (init_type == SUB_TEMP_INIT || init_type == ALL_TEMP_INIT) {
		for (i = BATTERY_STATUS__HIGH_TEMP; i>= BATTERY_STATUS__LOW_TEMP; i--) {
			chip->sub_bat_decidegc[i] = chip->anti_shake_bound[i];
		}
	}
}

static void parallel_battery_anti_shake(int change_type, int *bat_decidegc, int temp_status,
	int low_shake, int high_shake, int low_shake_0c, int high_shake_0c)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;

	oplus_init_parallel_temp_threshold(change_type);
	if (temp_status == BATTERY_STATUS__HIGH_TEMP) {
		bat_decidegc[BATTERY_STATUS__HIGH_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__HIGH_TEMP] + low_shake;
	} else if (temp_status == BATTERY_STATUS__LOW_TEMP) {
		bat_decidegc[BATTERY_STATUS__COLD_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__COLD_TEMP] + high_shake;
	} else if (temp_status == BATTERY_STATUS__COLD_TEMP) {
		bat_decidegc[BATTERY_STATUS__LITTLE_COLD_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COLD_TEMP] + high_shake_0c;
	} else if (temp_status == BATTERY_STATUS__LITTLE_COLD_TEMP) {
		bat_decidegc[BATTERY_STATUS__LITTLE_COLD_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COLD_TEMP] + low_shake_0c;
		bat_decidegc[BATTERY_STATUS__COOL_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__COOL_TEMP] + high_shake;
	} else if (temp_status == BATTERY_STATUS__COOL_TEMP) {
		bat_decidegc[BATTERY_STATUS__COOL_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__COOL_TEMP] + low_shake;
		bat_decidegc[BATTERY_STATUS__LITTLE_COOL_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COOL_TEMP] + high_shake;
	} else if (temp_status == BATTERY_STATUS__LITTLE_COOL_TEMP) {
		bat_decidegc[BATTERY_STATUS__LITTLE_COOL_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__LITTLE_COOL_TEMP] + low_shake;
		bat_decidegc[BATTERY_STATUS__NORMAL] =
			chip->anti_shake_bound[BATTERY_STATUS__NORMAL] + high_shake;
	} else if (temp_status == BATTERY_STATUS__NORMAL) {
		bat_decidegc[BATTERY_STATUS__NORMAL] =
			chip->anti_shake_bound[BATTERY_STATUS__NORMAL] + low_shake;
		bat_decidegc[BATTERY_STATUS__WARM_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__WARM_TEMP] + high_shake;
	} else if (temp_status == BATTERY_STATUS__WARM_TEMP) {
		bat_decidegc[BATTERY_STATUS__WARM_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__WARM_TEMP] + low_shake;
		bat_decidegc[BATTERY_STATUS__HIGH_TEMP] =
			chip->anti_shake_bound[BATTERY_STATUS__HIGH_TEMP] + high_shake;
	}
}

static void parallel_battery_anti_shake_handle(int status_change, int main_temp, int sub_temp)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int tbatt_cur_shake, low_shake = 0, high_shake = 0;
	int low_shake_0c = 0, high_shake_0c = 0;

	if (status_change == MAIN_STATUS_CHG || status_change == ALL_STATUS_CHG) {
		tbatt_cur_shake = main_temp;
		if (tbatt_cur_shake > chip->main_pre_shake) {			/* get warmer */
			low_shake = -HYSTERISIS_DECIDEGC;
			high_shake = 0;
			low_shake_0c = -HYSTERISIS_DECIDEGC_0C;
			high_shake_0c = 0;
		} else if (tbatt_cur_shake < chip->main_pre_shake) {	/* get cooler */
			low_shake = 0;
			high_shake = HYSTERISIS_DECIDEGC;
			low_shake_0c = 0;
			high_shake_0c = HYSTERISIS_DECIDEGC_0C;
		} else {
			chg_err("status change but temp not change");
		}
		parallel_battery_anti_shake(MAIN_STATUS_CHG, chip->main_bat_decidegc, chip->main_tbatt_status,
			low_shake, high_shake, low_shake_0c, high_shake_0c);
		chg_err("MAIN_BAT [%d-%d-%d-%d-%d-%d-%d] t=[%d %d] s=%d\n",
			chip->main_bat_decidegc[BATTERY_STATUS__COLD_TEMP],
			chip->main_bat_decidegc[BATTERY_STATUS__LITTLE_COLD_TEMP],
			chip->main_bat_decidegc[BATTERY_STATUS__COOL_TEMP],
			chip->main_bat_decidegc[BATTERY_STATUS__LITTLE_COOL_TEMP],
			chip->main_bat_decidegc[BATTERY_STATUS__NORMAL],
			chip->main_bat_decidegc[BATTERY_STATUS__WARM_TEMP],
			chip->main_bat_decidegc[BATTERY_STATUS__HIGH_TEMP],
			chip->main_pre_shake,
			tbatt_cur_shake,
			chip->main_tbatt_status);
		chip->main_pre_shake = tbatt_cur_shake;
	}

	if (status_change == SUB_STATUS_CHG || status_change == ALL_STATUS_CHG) {
		tbatt_cur_shake = sub_temp;
		if (tbatt_cur_shake > chip->sub_pre_shake) {			/* get warmer */
			low_shake = -HYSTERISIS_DECIDEGC;
			high_shake = 0;
			low_shake_0c = -HYSTERISIS_DECIDEGC_0C;
			high_shake_0c = 0;
		} else if (tbatt_cur_shake < chip->sub_pre_shake) {	/* get cooler */
			low_shake = 0;
			high_shake = HYSTERISIS_DECIDEGC;
			low_shake_0c = 0;
			high_shake_0c = HYSTERISIS_DECIDEGC_0C;
		} else {
			chg_err("status change but temp not change\n");
		}
		parallel_battery_anti_shake(SUB_STATUS_CHG, chip->sub_bat_decidegc, chip->sub_tbatt_status,
			low_shake, high_shake, low_shake_0c, high_shake_0c);
		chg_err("SUB_BAT [%d-%d-%d-%d-%d-%d-%d] t=[%d %d] s=%d\n",
			chip->sub_bat_decidegc[BATTERY_STATUS__COLD_TEMP],
			chip->sub_bat_decidegc[BATTERY_STATUS__LITTLE_COLD_TEMP],
			chip->sub_bat_decidegc[BATTERY_STATUS__COOL_TEMP],
			chip->sub_bat_decidegc[BATTERY_STATUS__LITTLE_COOL_TEMP],
			chip->sub_bat_decidegc[BATTERY_STATUS__NORMAL],
			chip->sub_bat_decidegc[BATTERY_STATUS__WARM_TEMP],
			chip->sub_bat_decidegc[BATTERY_STATUS__HIGH_TEMP],
			chip->sub_pre_shake,
			tbatt_cur_shake,
			chip->sub_tbatt_status);
		chip->sub_pre_shake = tbatt_cur_shake;
	}
}

static void oplus_chg_check_parallel_tbatt_status(int main_temp, int sub_temp, int *main_status, int *sub_status)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int status_change = 0;
	int i;
	bool find = false;

	for (i = BATTERY_STATUS__HIGH_TEMP; i >= BATTERY_STATUS__LOW_TEMP; i--) {
		if (main_temp >= chip->main_bat_decidegc[i]) {
			*main_status = i;
			find = true;
			break;
		}
	}
	if (!find)
		*main_status = BATTERY_STATUS__REMOVED;

	find = false;
	for (i = BATTERY_STATUS__HIGH_TEMP; i >= BATTERY_STATUS__LOW_TEMP; i--) {
		if (sub_temp >= chip->sub_bat_decidegc[i]) {
			*sub_status = i;
			find = true;
			break;
		}
	}
	if (!find)
		*sub_status = BATTERY_STATUS__REMOVED;

	if (chip->main_tbatt_status == -1 || chip->sub_tbatt_status == -1)
		chg_info("first enter, don't need anti shake \n");
	else if (chip->main_tbatt_status != *main_status && chip->sub_tbatt_status != *sub_status)
		status_change = ALL_STATUS_CHG;
	else if (chip->main_tbatt_status != *main_status)
		status_change = MAIN_STATUS_CHG;
	else if (chip->sub_tbatt_status != *sub_status)
		status_change = SUB_STATUS_CHG;

	chip->main_tbatt_status = *main_status;
	chip->sub_tbatt_status = *sub_status;
	if (status_change)
		parallel_battery_anti_shake_handle(status_change, main_temp, sub_temp);
}

int oplus_chg_is_parellel_ibat_over_spec(int main_temp, int sub_temp, int *target_curr)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int main_volt;
	int sub_volt;
	int main_curr_now;
	int sub_curr_now;
	int i;
	int curr_radio = 0;
	int main_curr_radio = 0;
	int sub_curr_radio = 0;
	int curr_need_change = 0;
	int main_curr_need_change = 0;
	int sub_curr_need_change = 0;
	int index_main = 0;
	int index_sub = 0;
	int target_main_curr = 0;
	int target_sub_curr = 0;
	int main_tstatus = 0;
	int sub_tstatus = 0;
	static bool init_temp_thr = false;
	static int pre_main_tstatus = 0;
	static int pre_sub_tstatus = 0;

	if (!chip || !target_curr) {
		chg_err("chip or target_curr is null\n");
		return 0;
	}

	if (!init_temp_thr) {
		oplus_init_parallel_temp_threshold(ALL_TEMP_INIT);
		init_temp_thr = true;
	}

	oplus_chg_check_parallel_tbatt_status(main_temp, sub_temp, &main_tstatus, &sub_tstatus);
	if (main_tstatus != pre_main_tstatus) {
		pre_main_tstatus = main_tstatus;
		chip->pre_spec_index = -1;
	}
	if (sub_tstatus != pre_sub_tstatus) {
		pre_sub_tstatus = sub_tstatus;
		chip->pre_sub_spec_index = -1;
	}

	if (!batt_temp_table[main_tstatus]
	    || !batt_temp_table[sub_tstatus]
	    || !chip->parallel_bat_data
	    || !chip->parallel_bat_data[main_tstatus].batt_table
	    || !chip->parallel_bat_data[sub_tstatus].batt_table) {
		chg_err("parallel batt spec not fond, limit to min curr\n");
		return -1;
	}
	main_volt = chip->batt_volt;
	main_curr_now = -chip->main_ibat_ma;
	sub_volt = chip->sub_batt_volt;
	sub_curr_now = -chip->sub_ibat_ma;
	if ((main_curr_now <= 0 || sub_curr_now <= 0)
	     && oplus_switching_get_hw_enable()) {
		chg_err("current is negative %d %d\n", main_curr_now, sub_curr_now);
		return 0;
	}

	if (!oplus_switching_get_hw_enable()) {
		curr_radio = RATIO_ACC;
		main_curr_radio = RATIO_ACC;
		sub_curr_radio = 0;
	} else {
		if (main_curr_now + sub_curr_now != 0)
			curr_radio = (main_curr_now * RATIO_ACC) / (main_curr_now + sub_curr_now);
		if (main_curr_now > 0)
			main_curr_radio = (main_curr_now + sub_curr_now) * RATIO_ACC / main_curr_now;
		if (sub_curr_now > 0)
			sub_curr_radio = (main_curr_now + sub_curr_now) * RATIO_ACC / sub_curr_now;

		if (oplus_vooc_get_fastchg_started() &&
		    (curr_radio > chip->track_unbalance_high ||
		     curr_radio < chip->track_unbalance_low)) {
			if (chip->unbalance_count < OUT_OF_BALANCE_COUNT)
				chip->unbalance_count++;
		} else if (chip->unbalance_count < OUT_OF_BALANCE_COUNT) {
			chip->unbalance_count = 0;
		}
	}

	for (i = 0; i < chip->parallel_bat_data[main_tstatus].length; i++) {
		if (main_volt > chip->parallel_bat_data[main_tstatus].batt_table[i].volt)
			continue;

		if (main_curr_now > chip->parallel_bat_data[main_tstatus].batt_table[i].main_curr) {
			main_curr_need_change = MAIN_BATT_OVER_CURR;
		}
		index_main = i;
		break;
	}

	for (i = 0; i < chip->parallel_bat_data[sub_tstatus].length; i++) {
		if (sub_volt > chip->parallel_bat_data[sub_tstatus].batt_table[i].volt)
			continue;

		if (sub_curr_now > chip->parallel_bat_data[sub_tstatus].batt_table[i].sub_curr) {
			sub_curr_need_change = SUB_BATT_OVER_CURR;
		}
		index_sub = i;
		break;
	}
	chg_err("pre_spec_index: %d index_main: %d pre_sub_spec_index: %d index_sub: %d\n",
		chip->pre_spec_index, index_main, chip->pre_sub_spec_index, index_sub);

	if (chip->pre_spec_index != -1 && chip->pre_spec_index > index_main)
		index_main = chip->pre_spec_index;
	if (chip->pre_sub_spec_index != -1 && chip->pre_sub_spec_index > index_sub)
		index_sub = chip->pre_sub_spec_index;

	target_main_curr = chip->parallel_bat_data[main_tstatus].batt_table[index_main].main_curr;
	target_main_curr = target_main_curr * main_curr_radio / RATIO_ACC;

	target_sub_curr = chip->parallel_bat_data[sub_tstatus].batt_table[index_sub].sub_curr;
	target_sub_curr = target_sub_curr * sub_curr_radio / RATIO_ACC;

	if (main_curr_need_change == MAIN_BATT_OVER_CURR && sub_curr_need_change == SUB_BATT_OVER_CURR)
		curr_need_change = (target_main_curr > target_sub_curr) ? SUB_BATT_OVER_CURR : MAIN_BATT_OVER_CURR;
	else if (main_curr_need_change == MAIN_BATT_OVER_CURR)
		curr_need_change = MAIN_BATT_OVER_CURR;
	else if (sub_curr_need_change == SUB_BATT_OVER_CURR)
		curr_need_change = SUB_BATT_OVER_CURR;

	if (!oplus_switching_get_hw_enable())
		*target_curr = target_main_curr;
	else
		*target_curr = (target_main_curr > target_sub_curr) ? target_sub_curr : target_main_curr;

	if (*target_curr < 0)
		*target_curr = 0;

	if (curr_need_change)
		chg_err("%s battery over current", (curr_need_change == MAIN_BATT_OVER_CURR) ? "main" : "sub");

	chip->pre_spec_index = index_main;
	chip->pre_sub_spec_index = index_sub;

	chg_err("main_volt:%d main_curr_now:%d sub_volt: %d sub_curr_now: %d "
		  "main_temp: %d sub_temp: %d main_tstatus: %s sub_tstatus: %s "
		  "curr_radio: %d target_main_curr: %d target_sub_curr: %d "
		  "target_curr: %d curr_need_change: %d\n",
		  main_volt, main_curr_now, sub_volt, sub_curr_now,
		  main_temp, sub_temp, batt_temp_table[main_tstatus], batt_temp_table[sub_tstatus],
		  curr_radio, target_main_curr, target_sub_curr,
		  *target_curr, curr_need_change);

	return curr_need_change;
}

bool oplus_chg_check_is_soc_gap_big(int main_soc, int sub_soc)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	static bool track_soc_gap_big;

	if (!chip) {
		chg_err("chip not ready");
		return false;
	}

	if (abs(main_soc - sub_soc) > chip->track_unbalance_soc
	    && !track_soc_gap_big) {
		track_soc_gap_big = true;
		chg_err("soc gap too big: main %d, sub %d, gap %d",
			main_soc, sub_soc, chip->track_unbalance_soc);
		return true;
	} else if (abs(main_soc - sub_soc) < chip->track_unbalance_soc) {
		track_soc_gap_big = false;
	}

	return false;
}

static int oplus_vooc_fastchg_disable(const char *client_str, bool disable)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int rc;

	chip->vooc_disable_votable = find_votable("VOOC_DISABLE");
	if (!chip->vooc_disable_votable) {
		chg_err("VOOC_DISABLE votable not found\n");
		return -EINVAL;
	}

	rc = vote(chip->vooc_disable_votable, client_str, disable,
		  disable ? 1 : 0, false);
	if (rc < 0)
		chg_err("%s vooc charger error, rc = %d\n",
			disable ? "disable" : "enable", rc);
	else
		chg_info("%s vooc charger\n", disable ? "disable" : "enable");

        return rc;
}

static int oplus_comm_chg_disable(const char *client_str, bool disable)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int rc;

	chip->chg_disable_votable = find_votable("CHG_DISABLE");
	if (!chip->chg_disable_votable) {
		chg_err("VOOC_DISABLE votable not found\n");
		return -EINVAL;
	}

	rc = vote(chip->chg_disable_votable, client_str, disable,
		  disable ? 1 : 0, false);
	if (rc < 0)
		chg_err("%s comm chg error, rc = %d\n",
			disable ? "disable" : "enable", rc);
	else
		chg_info("%s comm chg\n", disable ? "disable" : "enable");

        return rc;
}

static int oplus_comm_chg_vote_fcc(const char *client_str, int curr)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int rc;

	chip->wired_fcc_votable = find_votable("WIRED_FCC");
	if (!chip->wired_fcc_votable) {
		chg_err("WIRED_FCC votable not found\n");
		return -EINVAL;
	}

	rc = vote(chip->wired_fcc_votable, client_str, true,
		  curr, false);
	if (rc < 0)
		chg_err("wired_fcc_votable %d error, rc = %d\n",
			curr, rc);
	else
		chg_info("vote wired_fcc %d\n", curr);

        return rc;
}

static int oplus_comm_chg_disable_vote_fcc(const char *client_str)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int rc;

	chip->wired_fcc_votable = find_votable("WIRED_FCC");
	if (!chip->wired_fcc_votable) {
		chg_err("WIRED_FCC votable not found\n");
		return -EINVAL;
	}

	rc = vote(chip->wired_fcc_votable, client_str, false,
		  0, false);
	if (rc < 0)
		chg_err("disable wired_fcc_votable error, rc = %d\n", rc);
	else
		chg_info("disable vote wired_fcc\n");

        return rc;
}

static void parallel_chg_set_balance_bat_status(int status)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int mmi_chg = 1;
	struct mms_msg *msg;
	int rc;

	if (chip->chg_disable_votable)
		mmi_chg = !get_client_vote(chip->chg_disable_votable,
					   MMI_CHG_VOTER);
	switch (status) {
	case PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE:
	case PARALLEL_NEED_BALANCE_BAT_STATUS1__STOP_CHARGE:
	case PARALLEL_NEED_BALANCE_BAT_STATUS2__STOP_CHARGE:
	case PARALLEL_NEED_BALANCE_BAT_STATUS3__STOP_CHARGE:
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS4__STOP_CHARGE:
		if (chip->balancing_bat_status == PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE &&
		    chip->wired_online) {
			status = PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE;
		}
		break;
	case PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE:
	case PARALLEL_BAT_BALANCE_ERROR_STATUS6__STOP_CHARGE:
	case PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE:
	default:
		break;
	}

	if (chip->balancing_bat_status != status) {
		switch (status) {
		case PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE:
			oplus_comm_chg_disable(PARALLEL_VOTER, false);
			oplus_vooc_fastchg_disable(PARALLEL_VOTER, false);
			break;
		case PARALLEL_NEED_BALANCE_BAT_STATUS1__STOP_CHARGE:
		case PARALLEL_NEED_BALANCE_BAT_STATUS2__STOP_CHARGE:
		case PARALLEL_NEED_BALANCE_BAT_STATUS3__STOP_CHARGE:
		case PARALLEL_NEED_BALANCE_BAT_STATUS4__STOP_CHARGE:
		case PARALLEL_NEED_BALANCE_BAT_STATUS5__STOP_CHARGE:
			if (chip->wired_online) {
				oplus_comm_chg_disable(PARALLEL_VOTER, true);
			}
			break;
		case PARALLEL_BAT_BALANCE_ERROR_STATUS6__STOP_CHARGE:
			oplus_vooc_fastchg_disable(PARALLEL_VOTER, false);
			break;
		case PARALLEL_NEED_BALANCE_BAT_STATUS7__START_CHARGE:
			oplus_comm_chg_disable(PARALLEL_VOTER, false);
			oplus_vooc_fastchg_disable(PARALLEL_VOTER, true);
			break;
		case PARALLEL_BAT_BALANCE_ERROR_STATUS8:
			oplus_vooc_fastchg_disable(PARALLEL_VOTER, true);
			break;
		default:
			break;
		}
		chip->balancing_bat_status = status;
		oplus_switching_set_balance_bat_status(chip->balancing_bat_status);
		msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
					  SWITCH_ITEM_STATUS);
		if (msg == NULL) {
			chg_err("alloc msg error\n");
			return;
		}
		rc = oplus_mms_publish_msg(chip->parallel_topic, msg);
		if (rc < 0) {
			chg_err("publish SWITCH_ITEM_STATUS msg error, rc=%d\n", rc);
			kfree(msg);
		}
		msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
					  SWITCH_ITEM_HW_ENABLE_STATUS);
		if (msg == NULL) {
			chg_err("alloc msg error\n");
			return;
		}
		rc = oplus_mms_publish_msg(chip->parallel_topic, msg);
		if (rc < 0) {
			chg_err("publish SWITCH_ITEM_HW_ENABLE_STATUS msg error, rc=%d\n", rc);
			kfree(msg);
		}
	}
}

#define RETRY_CNT 3
static int parallel_chg_check_balance_bat_status(void)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	union mms_msg_data data = { 0 };
	int status;
	int retry = RETRY_CNT;

	if (!chip) {
		return 0;
	}
	if (!atomic_read(&chip->mos_lock)) {
		chg_err("mos test start, check next time!");
		return 0;
	}

	while (retry) {
		status = oplus_switching_get_if_need_balance_bat(chip->batt_volt, chip->sub_batt_volt);
		if (status == PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE) {
			break;
		}
		msleep(50);
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					true);
		chip->batt_volt = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					true);
		chip->sub_batt_volt = data.intval;
		retry--;
	}
	chg_err("balancing_bat_status:%d, current_status:%d", chip->balancing_bat_status, status);

	if (chip->balancing_bat_status == PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE &&
	    status != PARALLEL_BAT_BALANCE_ERROR_STATUS8 && status != PARALLEL_BAT_BALANCE_ERROR_STATUS9) {
		return 0;
	}
	if (chip->balancing_bat_status == PARALLEL_BAT_BALANCE_ERROR_STATUS8) {
		return 0;
	}
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_ONLINE_KEEP,
  				&data, false);
	chip->vooc_online_keep = data.intval;
	if (chip->vooc_online_keep &&
	    !oplus_vooc_get_fastchg_started() &&
	    status != PARALLEL_BAT_BALANCE_ERROR_STATUS8 && status != PARALLEL_BAT_BALANCE_ERROR_STATUS9) {
		chg_err("fastchg_to_normal or fastchg_to_warm,no need balance");
		chip->balancing_bat_status = PARALLEL_NOT_NEED_BALANCE_BAT__START_CHARGE;
		oplus_switching_set_balance_bat_status(chip->balancing_bat_status);
		return 0;
	}

	if (!atomic_dec_and_test(&chip->mos_lock)) {
		chg_err("mos test start, check next time!");
		atomic_inc(&chip->mos_lock);
		return 0;
	}

	parallel_chg_set_balance_bat_status(status);

	atomic_inc(&chip->mos_lock);

	return 0;
}

static void oplus_chg_switch_err_handler(struct oplus_chg_ic_dev *ic_dev,
					void *virq_data)
{
	struct oplus_parallel_chip *chip = virq_data;

	schedule_work(&chip->err_handler_work);
	return;
}

static int oplus_chg_switch_virq_register(struct oplus_parallel_chip *chip)
{
	int rc;

	rc = oplus_chg_ic_virq_register(chip->ic_dev, OPLUS_IC_VIRQ_ERR,
		oplus_chg_switch_err_handler, chip);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);

	return 0;
}

static int oplus_chg_switch_update_status(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_parallel_chip *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	data->intval = chip->balancing_bat_status;

	return 0;
}

static int oplus_chg_switch_update_curr_limit(struct oplus_mms *mms,
	union mms_msg_data *data)
{
	struct oplus_parallel_chip *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	if (chip->batt_spec_status < 0)
		data->intval = -1;
	else
		data->intval = chip->target_curr_limit;
	if (chip->mos_test_started)
		data->intval = MOS_TEST_DEFAULT_CURRENT;

	return 0;
}

static int oplus_chg_switch_update_hw_enable_status(struct oplus_mms *mms,
	union mms_msg_data *data)
{
	struct oplus_parallel_chip *chip;
	int status;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	status = oplus_switching_get_hw_enable();
	if (status < 0)
		data->intval = 0;
	else
		data->intval = !!status;

	return 0;
}

static int oplus_chg_switch_update_err_notify(struct oplus_mms *mms,
	union mms_msg_data *data)
{
	struct oplus_parallel_chip *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	data->intval = chip->notify_flag;

	return 0;
}

static void oplus_mms_parallel_update(struct oplus_mms *mms, bool publish)
{
	struct oplus_parallel_chip *chip;
	struct mms_msg *msg;
	int i, rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return;
	}
	chip = oplus_mms_get_drvdata(mms);

	for (i = 0; i < mms->desc->update_items_num; i++)
		oplus_mms_item_update(mms, mms->desc->update_items[i], true);
	if (publish) {
		msg = oplus_mms_alloc_msg(MSG_TYPE_TIMER, MSG_PRIO_MEDIUM, 0);
		if (msg == NULL) {
			chg_err("alloc msg buf error\n");
			return;
		}
		rc = oplus_mms_publish_msg(mms, msg);
		if (rc < 0) {
			chg_err("publish msg error, rc=%d\n", rc);
			kfree(msg);
			return;
		}
	}
}

static struct mms_item oplus_chg_parallel_item[] = {
	{
		.desc = {
			.item_id = SWITCH_ITEM_STATUS,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_switch_update_status,
		}
	}, {
		.desc = {
			.item_id = SWITCH_ITEM_CURR_LIMIT,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_switch_update_curr_limit,
		}
	}, {
		.desc = {
			.item_id = SWITCH_ITEM_HW_ENABLE_STATUS,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_switch_update_hw_enable_status,
		}
	}, {
		.desc = {
			.item_id = SWITCH_ITEM_ERR_NOTIFY,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_switch_update_err_notify,
		}
	}
};

static const u32 oplus_chg_status_update_item[] = {
	SWITCH_ITEM_STATUS,
	SWITCH_ITEM_CURR_LIMIT,
	SWITCH_ITEM_HW_ENABLE_STATUS,
};

static const struct oplus_mms_desc oplus_mms_parallel_desc = {
	.name = "parallel",
	.type = OPLUS_MMS_TYPE_PARALLEL,
	.item_table = oplus_chg_parallel_item,
	.item_num = ARRAY_SIZE(oplus_chg_parallel_item),
	.update_items = oplus_chg_status_update_item,
	.update_items_num = ARRAY_SIZE(oplus_chg_status_update_item),
	.update_interval = 0, /* ms */
	.update = oplus_mms_parallel_update,
};

static void oplus_switch_comm_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_switch_subscribe_comm_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;

	chip->comm_topic = topic;
	chip->comm_subs =
		oplus_mms_subscribe(chip->comm_topic, chip,
				    oplus_switch_comm_subs_callback,
				    "chg_switch");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}
	if (!chip->chg_disable_votable)
		chip->chg_disable_votable = find_votable("CHG_DISABLE");
}

static void oplus_switch_wired_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	struct oplus_parallel_chip *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			if (chip->wired_online != data.intval) {
				oplus_chg_parellel_variables_reset();
				chip->wired_online = data.intval;
				chg_info("wired_online:%d\n", chip->wired_online);
				if (chip->wired_online &&
				    oplus_chg_parallel_support() == PARALLEL_MOS_CTRL) {
					schedule_delayed_work(&chip->parallel_curr_spec_check_work, 0);
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

static void oplus_switch_subscribe_wired_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_switch_wired_subs_callback,
				    "chg_switch");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
		PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = !!data.intval;
	if (chip->wired_online)
		schedule_delayed_work(&chip->parallel_curr_spec_check_work, 0);
}

static void oplus_switch_vooc_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
static void oplus_switch_subscribe_vooc_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;

	chip->vooc_topic = topic;
	chip->vooc_subs =
	oplus_mms_subscribe(chip->vooc_topic, chip,
			    oplus_switch_vooc_subs_callback,
			    "chg_switch");
	if (IS_ERR_OR_NULL(chip->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
		PTR_ERR(chip->vooc_subs));
		return;
	}
}

static void oplus_switch_gauge_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	default:
		break;
	}
}

static void oplus_switch_subscribe_gauge_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_switch_gauge_subs_callback, "chg_switch");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}
}

static void oplus_switch_main_gauge_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	default:
		break;
	}
}

static void oplus_switch_subscribe_main_gauge_topic(struct oplus_mms *topic,
	void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;
	int error_status = 0;
	int rc;

	chip->main_gauge_topic = topic;
	chip->main_gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_switch_main_gauge_subs_callback, "chg_switch");
	if (IS_ERR_OR_NULL(chip->main_gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->main_gauge_subs));
		return;
	}
	if (!IS_ERR_OR_NULL(chip->sub_gauge_subs) &&
	    oplus_chg_parallel_support() == PARALLEL_MOS_CTRL) {
		update_batt_info(chip, true);
		if (chip->sub_batt_temp == FG_I2C_ERROR ||
		    chip->main_batt_temp == FG_I2C_ERROR) {
			chg_err("gauge i2c error! disable mos\n");
			error_status = REASON_I2C_ERROR;
			chip->parallel_error_flag |= REASON_I2C_ERROR;
		}
		if (abs(chip->batt_volt - chip->sub_batt_volt) > chip->parallel_vbat_gap_abnormal) {
			chg_err("two battery vol gap > %d\n", chip->parallel_vbat_gap_abnormal);
			error_status = REASON_VBAT_GAP_BIG;
			chip->parallel_error_flag |= REASON_VBAT_GAP_BIG;
		}
		if (error_status == 0) {
			rc = oplus_chg_ic_func(chip->ic_dev, OPLUS_IC_FUNC_SET_HW_ENABLE, true);
			if (rc < 0)
				chg_err("switching_ic set mos error\n");
		}
#ifdef CONFIG_OPLUS_CHARGER_MTK
		if (get_boot_mode() == ATE_FACTORY_BOOT || get_boot_mode() == FACTORY_BOOT) {
#else
		if (get_boot_mode() == MSM_BOOT_MODE__FACTORY) {
#endif
			if (oplus_is_rf_ftm_mode()) {
				chg_err("boot_mode: %d, disabled mos\n", get_boot_mode());
				rc = oplus_chg_ic_func(chip->ic_dev, OPLUS_IC_FUNC_SET_HW_ENABLE, false);
				if (rc < 0)
					chg_err("switching_ic set mos error\n");
			}
		}
	}
}

static void oplus_switch_sub_gauge_subs_callback(struct mms_subscribe *subs,
	enum mms_msg_type type, u32 id)
{
	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	default:
		break;
	}
}

static void oplus_switch_subscribe_sub_gauge_topic(struct oplus_mms *topic,
							void *prv_data)
{
	struct oplus_parallel_chip *chip = prv_data;
	int error_status = 0;
	int rc;

	chip->sub_gauge_topic = topic;
	chip->sub_gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_switch_sub_gauge_subs_callback, "chg_switch");
	if (IS_ERR_OR_NULL(chip->sub_gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->sub_gauge_subs));
		return;
	}
	if (!IS_ERR_OR_NULL(chip->main_gauge_subs) &&
	    oplus_chg_parallel_support() == PARALLEL_MOS_CTRL) {
		update_batt_info(chip, true);
		if (chip->sub_batt_temp == FG_I2C_ERROR ||
		    chip->main_batt_temp == FG_I2C_ERROR) {
			chg_err("gauge i2c error! disable mos\n");
			error_status = REASON_I2C_ERROR;
			chip->parallel_error_flag |= REASON_I2C_ERROR;
		}
		if (abs(chip->batt_volt - chip->sub_batt_volt) > chip->parallel_vbat_gap_abnormal) {
			chg_err("two battery vol gap > %d\n", chip->parallel_vbat_gap_abnormal);
			error_status = REASON_VBAT_GAP_BIG;
			chip->parallel_error_flag |= REASON_VBAT_GAP_BIG;
		}
		if (error_status == 0) {
			rc = oplus_chg_ic_func(chip->ic_dev, OPLUS_IC_FUNC_SET_HW_ENABLE, true);
			if (rc < 0)
				chg_err("switching_ic set mos error\n");
		}
#ifdef CONFIG_OPLUS_CHARGER_MTK
		if (get_boot_mode() == ATE_FACTORY_BOOT || get_boot_mode() == FACTORY_BOOT) {
#else
		if (get_boot_mode() == MSM_BOOT_MODE__FACTORY) {
#endif
			if (oplus_is_rf_ftm_mode()) {
				chg_err("boot_mode: %d, disabled mos\n", get_boot_mode());
				rc = oplus_chg_ic_func(chip->ic_dev, OPLUS_IC_FUNC_SET_HW_ENABLE, false);
				if (rc < 0)
					chg_err("switching_ic set mos error\n");
			}
		}
	}
}

static int oplus_chg_parallel_topic_init(struct oplus_parallel_chip *chip)
{
	struct oplus_mms_config mms_cfg = {};
	int rc;

	mms_cfg.drv_data = chip;
	mms_cfg.of_node = chip->dev->of_node;

	chip->parallel_topic = devm_oplus_mms_register(chip->dev, &oplus_mms_parallel_desc, &mms_cfg);
	if (IS_ERR(chip->parallel_topic)) {
		chg_err("Couldn't register parallel_topic\n");
		rc = PTR_ERR(chip->parallel_topic);
		return rc;
	}

	oplus_mms_wait_topic("common", oplus_switch_subscribe_comm_topic, chip);
	oplus_mms_wait_topic("wired", oplus_switch_subscribe_wired_topic, chip);
	oplus_mms_wait_topic("vooc", oplus_switch_subscribe_vooc_topic, chip);
	oplus_mms_wait_topic("gauge", oplus_switch_subscribe_gauge_topic, chip);
	oplus_mms_wait_topic("gauge:0", oplus_switch_subscribe_main_gauge_topic, chip);
	oplus_mms_wait_topic("gauge:1", oplus_switch_subscribe_sub_gauge_topic, chip);

	return 0;
}

static int update_batt_info(struct oplus_parallel_chip *chip, bool update)
{
	union mms_msg_data data = { 0 };

	if (is_support_parallel_battery(chip->gauge_topic)) {
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					update);
		chip->batt_volt = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_CURR, &data,
					update);
		chip->main_ibat_ma = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_TEMP, &data,
					update);
		chip->main_batt_temp = data.intval;
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_SOC, &data,
					update);
		chip->main_batt_soc = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
					update);
		chip->sub_batt_volt = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_CURR, &data,
					update);
		chip->sub_ibat_ma = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_TEMP, &data,
					update);
		chip->sub_batt_temp = data.intval;
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_SOC, &data,
					update);
		chip->sub_batt_soc = data.intval;
		return 1;
	} else {
		return 0;
	}
}

static void oplus_parallel_err_handler_work(struct work_struct *work)
{
	struct oplus_parallel_chip *chip = container_of(work, struct oplus_parallel_chip,
		err_handler_work);
	struct oplus_chg_ic_err_msg *msg, *tmp;
	struct list_head msg_list;

	INIT_LIST_HEAD(&msg_list);
	spin_lock(&chip->ic_dev->err_list_lock);
	if (!list_empty(&chip->ic_dev->err_list))
		list_replace_init(&chip->ic_dev->err_list, &msg_list);
	spin_unlock(&chip->ic_dev->err_list_lock);

	list_for_each_entry_safe(msg, tmp, &msg_list, list) {
		if (is_err_topic_available(chip))
			oplus_mms_publish_ic_err_msg(chip->err_topic,
						     ERR_ITEM_IC, msg);
		oplus_print_ic_err(msg);
		list_del(&msg->list);
		kfree(msg);
	}
}

#define REASON_LENGTH_MAX 256
static int oplus_parallel_get_track_info(char *temp_str, int index)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;

	if (!chip)
		return 0;

	return snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index,
			"$$mos_err_status@@"
			"main_sub_soc %d %d,"
			"main_sub_volt %d %d,"
			"main_sub_curr %d %d,"
			"main_sub_temp %d %d,"
			"mos %s,",
			chip->main_batt_soc, chip->sub_batt_soc,
			chip->batt_volt, chip->sub_batt_volt,
			chip->main_ibat_ma, chip->sub_ibat_ma,
			chip->main_batt_temp, chip->sub_batt_temp,
			oplus_switching_get_hw_enable() ? "on" : "off");
}

#define SOC_NOT_FULL_REPORT 97
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

static void oplus_parallel_track_check(void)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	static bool track_curr_unbalance;
	static int pre_error_flag;
	char temp_str[REASON_LENGTH_MAX] = {0};
	int index = 0;
	int need_upload = false;
	static int upload_count[TRACK_MOS_VBAT_GAP_BIG + 1] = {0};
	static int pre_upload_time[TRACK_MOS_VBAT_GAP_BIG + 1] = {0};
	int curr_time;
	int i;

	if (!chip)
		return;

	curr_time = track_get_local_time_s();
	for (i = TRACK_MOS_SOC_NOT_FULL; i <= TRACK_MOS_VBAT_GAP_BIG; i++) {
		if (curr_time - pre_upload_time[i] > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
			upload_count[i] = 0;
	}

	if (((chip->main_batt_full && !chip->soc_not_full_report &&
	     (chip->main_batt_soc <= SOC_NOT_FULL_REPORT || chip->sub_batt_soc <= SOC_NOT_FULL_REPORT) &&
	     (chip->tbatt_status != TEMP_REGION_COLD &&
	      chip->tbatt_status != TEMP_REGION_WARM &&
	      chip->tbatt_status != TEMP_REGION_HOT)) ||
	     chip->debug_force_mos_err == TRACK_MOS_SOC_NOT_FULL) &&
	    upload_count[TRACK_MOS_SOC_NOT_FULL] < TRACK_UPLOAD_COUNT_MAX) {
		chg_err("soc too low %d %d when full\n",
			chip->main_batt_soc, chip->sub_batt_soc);
		chip->soc_not_full_report = true;
		chip->notify_flag |= 1 << NOTIFY_FAST_CHG_END_ERROR;
		upload_count[TRACK_MOS_SOC_NOT_FULL]++;
		pre_upload_time[TRACK_MOS_SOC_NOT_FULL] = track_get_local_time_s();
		index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "full_at_soc_not_full");
		index += oplus_parallel_get_track_info(temp_str, index);
		oplus_chg_ic_creat_err_msg(chip->ic_dev, OPLUS_IC_ERR_PARALLEL_UNBALANCE, 0, temp_str);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
		index = 0;
	}

	if ((oplus_chg_check_is_soc_gap_big(chip->main_batt_soc, chip->sub_batt_soc) ||
	     chip->debug_force_mos_err == TRACK_MOS_SOC_GAP_TOO_BIG) &&
	    upload_count[TRACK_MOS_SOC_GAP_TOO_BIG] < TRACK_UPLOAD_COUNT_MAX)  {
		chg_err("soc gap too big %d %d when full\n",
			chip->main_batt_soc, chip->sub_batt_soc);
		upload_count[TRACK_MOS_SOC_GAP_TOO_BIG]++;
		pre_upload_time[TRACK_MOS_SOC_GAP_TOO_BIG] = track_get_local_time_s();
		index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "soc_gap_too_big");
		index += oplus_parallel_get_track_info(temp_str, index);
		oplus_chg_ic_creat_err_msg(chip->ic_dev, OPLUS_IC_ERR_PARALLEL_UNBALANCE, 0, temp_str);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
		index = 0;
	}

	if (((chip->unbalance_count >= OUT_OF_BALANCE_COUNT && !track_curr_unbalance) ||
	     chip->debug_force_mos_err == TRACK_MOS_CURRENT_UNBALANCE) &&
	    upload_count[TRACK_MOS_CURRENT_UNBALANCE] < TRACK_UPLOAD_COUNT_MAX) {
		chg_err("current out of balance\n");
		track_curr_unbalance = true;
		chip->notify_flag |= 1 << NOTIFY_CURRENT_UNBALANCE;
		upload_count[TRACK_MOS_CURRENT_UNBALANCE]++;
		pre_upload_time[TRACK_MOS_CURRENT_UNBALANCE] = track_get_local_time_s();
		index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "current_unbalance");
		index += oplus_parallel_get_track_info(temp_str, index);
		oplus_chg_ic_creat_err_msg(chip->ic_dev, OPLUS_IC_ERR_PARALLEL_UNBALANCE, 0, temp_str);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
		index = 0;
	}

	if (((pre_error_flag | ~REASON_SUB_BATT_FULL) < (chip->parallel_error_flag | ~REASON_SUB_BATT_FULL)) ||
	    chip->debug_force_mos_err >= TRACK_MOS_I2C_ERROR) {
		chg_err("mos error %d \n", chip->parallel_error_flag);
		if ((chip->parallel_error_flag & REASON_I2C_ERROR ||
		     chip->debug_force_mos_err == TRACK_MOS_I2C_ERROR) &&
		    upload_count[TRACK_MOS_I2C_ERROR] < TRACK_UPLOAD_COUNT_MAX) {
			upload_count[TRACK_MOS_I2C_ERROR]++;
			pre_upload_time[TRACK_MOS_I2C_ERROR] = track_get_local_time_s();
			index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "i2c_error ");
			need_upload = true;
		}
		if ((chip->parallel_error_flag & REASON_MOS_OPEN_ERROR ||
		     chip->debug_force_mos_err == TRACK_MOS_OPEN_ERROR) &&
		    upload_count[TRACK_MOS_OPEN_ERROR] < TRACK_UPLOAD_COUNT_MAX) {
			chip->notify_flag |= 1 << NOTIFY_MOS_OPEN_ERROR;
			upload_count[TRACK_MOS_OPEN_ERROR]++;
			pre_upload_time[TRACK_MOS_OPEN_ERROR] = track_get_local_time_s();
			index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "mos_open_error ");
			need_upload = true;
		}
		if ((chip->parallel_error_flag & REASON_VBAT_GAP_BIG ||
		     chip->debug_force_mos_err == TRACK_MOS_VBAT_GAP_BIG) &&
		    upload_count[TRACK_MOS_VBAT_GAP_BIG] < TRACK_UPLOAD_COUNT_MAX) {
			chip->notify_flag |= 1 << NOTIFY_CURRENT_UNBALANCE;
			upload_count[TRACK_MOS_VBAT_GAP_BIG]++;
			pre_upload_time[TRACK_MOS_VBAT_GAP_BIG] = track_get_local_time_s();
			index += snprintf(&(temp_str[index]), REASON_LENGTH_MAX - index, "vbat_gap_big ");
			need_upload = true;
		}
		if (need_upload) {
			index += oplus_parallel_get_track_info(temp_str, index);
			oplus_chg_ic_creat_err_msg(chip->ic_dev, OPLUS_IC_ERR_MOS_ERROR, 0, temp_str);
			oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
		}
		index = 0;
	}
	pre_error_flag = chip->parallel_error_flag;
}

static void oplus_chg_parallel_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_parallel_chip *chip = container_of(dwork,
		struct oplus_parallel_chip, parallel_check_work);

	if (IS_ERR_OR_NULL(chip->main_gauge_subs) ||
	    IS_ERR_OR_NULL(chip->sub_gauge_subs)) {
		chg_err("wait gauge toptic\n");
		goto subs_error;
	}

	if(chip->wired_online)
		update_batt_info(chip, false);
	else
		update_batt_info(chip, true);
	parallel_chg_check_balance_bat_status();
	if (oplus_chg_parallel_support() == PARALLEL_MOS_CTRL)
		oplus_parallel_track_check();

subs_error:
	if(chip->wired_online)
		schedule_delayed_work(&chip->parallel_check_work,
				      OPLUS_CHG_UPDATE_INTERVAL(CHECK_TIME_WITH_CHARGER));
	else
		schedule_delayed_work(&chip->parallel_check_work,
				      OPLUS_CHG_UPDATE_INTERVAL(CHECK_TIME_WITHOUT_CHARGER));

	return;
}

static void oplus_parallel_curr_limit_publish(struct oplus_parallel_chip *chip)
{
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  SWITCH_ITEM_CURR_LIMIT);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(chip->parallel_topic, msg);
	if (rc < 0) {
		chg_err("publish SWITCH_ITEM_CURR_LIMIT msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

#define CURR_STEP 100
static void oplus_parallel_curr_spec_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_parallel_chip *chip = container_of(dwork,
		struct oplus_parallel_chip, parallel_curr_spec_check_work);
	int target_curr = 0;
	int spec_status;

	if (IS_ERR_OR_NULL(chip->main_gauge_subs) ||
	    IS_ERR_OR_NULL(chip->sub_gauge_subs) ||
	    IS_ERR_OR_NULL(chip->wired_subs)) {
	    chg_err("wait gauge toptic\n");
	    return;
	}
	update_batt_info(chip, true);
	spec_status = oplus_chg_is_parellel_ibat_over_spec(
		chip->main_batt_temp, chip->sub_batt_temp, &target_curr);
	chip->batt_spec_status = spec_status;
	if (spec_status < 0) {
		oplus_parallel_curr_limit_publish(chip);
	} else {
		target_curr = target_curr / CURR_STEP * CURR_STEP;
		if (chip->wired_online) {
			chip->target_curr_limit = target_curr;
			oplus_parallel_curr_limit_publish(chip);
			if (oplus_vooc_get_fastchg_started()) {
				if (oplus_support_normal_batt_spec_check())
					oplus_comm_chg_disable_vote_fcc(PARALLEL_VOTER);
			} else {
				if (oplus_support_normal_batt_spec_check()) {
					if (target_curr == 0)
						oplus_comm_chg_disable_vote_fcc(PARALLEL_VOTER);
					else
						oplus_comm_chg_vote_fcc(PARALLEL_VOTER, target_curr);
				}
			}
		} else {
			chip->target_curr_limit = 0;
			if (oplus_support_normal_batt_spec_check())
				oplus_comm_chg_disable_vote_fcc(PARALLEL_VOTER);
		}
	}

	if(chip->wired_online)
		schedule_delayed_work(&chip->parallel_curr_spec_check_work,
				      OPLUS_CHG_UPDATE_INTERVAL(CURR_SPEC_CHECK_TIME));
	else
		oplus_chg_parellel_variables_reset();
	return;
}

static void oplus_parallel_chg_mos_test_work(struct work_struct *work)
{
	struct oplus_parallel_chip *chip = g_parallel_chip;
	int sub_current;
	union mms_msg_data data = { 0 };

	if (!chip || !chip->sub_gauge_topic) {
		chg_err("chip is NULL\n");
		return;
	}
	chg_err("mos test start!\n");
	oplus_parallel_curr_limit_publish(chip);
	msleep(1500);	/* wait 1.5s until current adjusts */

	if (!atomic_dec_and_test(&chip->mos_lock)) {
		chg_err("mos is changing, wait 500ms!\n");
		atomic_inc(&chip->mos_lock);
		msleep(500);
		if (!atomic_dec_and_test(&chip->mos_lock)) {
			chg_err("set_balance_bat_status error, return!\n");
			atomic_inc(&chip->mos_lock);
			chip->mos_test_started = false;
			oplus_parallel_curr_limit_publish(chip);
			return;
		}
	}
	if (!oplus_switching_get_hw_enable() ||
	    chip->balancing_bat_status == PARALLEL_BAT_BALANCE_ERROR_STATUS8 ||
	    chip->balancing_bat_status == PARALLEL_BAT_BALANCE_ERROR_STATUS9) {
		chg_err("mos: %d, test next time!\n", oplus_switching_get_hw_enable());
		chip->mos_test_started = false;
		oplus_parallel_curr_limit_publish(chip);
		atomic_inc(&chip->mos_lock);
		return;
	}
	oplus_switching_hw_enable(0);
	msleep(2000);	/* wait 2s until mos open */

	oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_CURR, &data,
				true);
	sub_current = data.intval;
	chg_err("after mos open, sub_current = %d!\n", sub_current);
	oplus_switching_hw_enable(1);
	msleep(1000);	/* wait 1s after mos on, make sure guage has updated*/

	atomic_inc(&chip->mos_lock);
	if (-sub_current < SUB_BATT_CURRENT_50_MA) {
		chip->mos_test_result = 1;
	}
	chip->mos_test_started = false;
	oplus_parallel_curr_limit_publish(chip);
	chg_err("mos test end!\n");
}

static void oplus_chg_parallel_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_parallel_chip *chip = container_of(dwork,
		struct oplus_parallel_chip, parallel_init_work);
	struct device_node *node = chip->dev->of_node;
	static int retry = OPLUS_CHG_IC_INIT_RETRY_MAX;
	int rc;

	chip->ic_dev = of_get_oplus_chg_ic(node, "oplus,switching_ic", 0);
	if (chip->ic_dev == NULL) {
		chg_err("not find switching ic\n");
		goto init_try_again;
	}

	rc = oplus_chg_ic_func(chip->ic_dev, OPLUS_IC_FUNC_INIT);
	if (rc == -EAGAIN) {
		chg_err("switching_ic init timeout\n");
		goto init_try_again;
	} else if (rc < 0) {
		chg_err("switching_ic init error, rc=%d\n", rc);
		retry = 0;
		goto init_error;
	}
	retry = 0;

	oplus_chg_switch_virq_register(chip);
	g_parallel_chip = chip;

	(void)oplus_chg_parallel_topic_init(chip);
	misc_register(&oplus_chg_device);
	rc = mos_track_debugfs_init(chip);
	if (rc < 0)
		chg_err("track debugfs init fail\n");
	schedule_delayed_work(&chip->parallel_check_work, 0);

	return;
init_try_again:
	if (retry > 0) {
		retry--;
		schedule_delayed_work(&chip->parallel_init_work,
			msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
	} else {
		chg_err("oplus,switching ic not found\n");
	}
init_error:
	chip->ic_dev = NULL;
	return;
}

static int oplus_chg_parallel_probe(struct platform_device *pdev)
{
	struct oplus_parallel_chip *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_parallel_chip),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	of_platform_populate(chip->dev->of_node, NULL, NULL, chip->dev);
	oplus_parallel_init(chip);

	INIT_DELAYED_WORK(&chip->parallel_init_work, oplus_chg_parallel_init_work);
	INIT_DELAYED_WORK(&chip->parallel_check_work, oplus_chg_parallel_check_work);
	INIT_DELAYED_WORK(&chip->parallel_curr_spec_check_work, oplus_parallel_curr_spec_check_work);
	INIT_DELAYED_WORK(&chip->parallel_chg_mos_test_work, oplus_parallel_chg_mos_test_work);
	INIT_WORK(&chip->err_handler_work, oplus_parallel_err_handler_work);

	schedule_delayed_work(&chip->parallel_init_work, 0);

	chg_info("probe success\n");
	return 0;
}

static int oplus_chg_parallel_remove(struct platform_device *pdev)
{
	struct oplus_parallel_chip *chip = platform_get_drvdata(pdev);

	if (!IS_ERR_OR_NULL(chip->comm_subs))
		oplus_mms_unsubscribe(chip->comm_subs);
	if (!IS_ERR_OR_NULL(chip->wired_subs))
		oplus_mms_unsubscribe(chip->wired_subs);
	if (!IS_ERR_OR_NULL(chip->gauge_subs))
		oplus_mms_unsubscribe(chip->gauge_subs);
	if (!IS_ERR_OR_NULL(chip->vooc_subs))
		oplus_mms_unsubscribe(chip->vooc_subs);

	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_chg_parallel_match[] = {
	{ .compatible = "oplus,parallel" },
	{},
};

static struct platform_driver oplus_chg_parallel_driver = {
	.driver		= {
		.name = "oplus-chg_switching",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_chg_parallel_match),
	},
	.probe		= oplus_chg_parallel_probe,
	.remove		= oplus_chg_parallel_remove,
};

static __init int oplus_chg_parallel_init(void)
{
	return platform_driver_register(&oplus_chg_parallel_driver);
}

static __exit void oplus_chg_parallel_exit(void)
{
	platform_driver_unregister(&oplus_chg_parallel_driver);
}

oplus_chg_module_register(oplus_chg_parallel);
