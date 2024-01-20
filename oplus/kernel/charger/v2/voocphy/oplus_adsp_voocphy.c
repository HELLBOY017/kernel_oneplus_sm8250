#define pr_fmt(fmt) "[ADSP_VOOCPHY]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/sched/clock.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/iio/consumer.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/rpmsg.h>
#include <linux/pm_wakeup.h>
#include <oplus_chg_module.h>
#if defined(CONFIG_OPLUS_ADSP_SM8450_CHARGER)
#include <../charger_ic/oplus_hal_sm8450.h>
#elif defined(CONFIG_OPLUS_ADSP_CHARGER)
#include <../charger_ic/oplus_hal_adsp.h>
#endif
#include "oplus_adsp_voocphy.h"
#include "chglib/oplus_chglib.h"
#include <oplus_chg_comm.h>

#define SEND_FASTCHG_ONGOING_NOTIFY_INTERVAL 2000 /* ms */
#define BTBOVER_5V1A_CHARGE_STD	0x01

struct oplus_voocphy_manager *g_voocphy_chip;

__maybe_unused static bool is_batt_psy_available(struct oplus_voocphy_manager *chip)
{
        if (!chip->batt_psy)
                chip->batt_psy = power_supply_get_by_name("battery");
        return !!chip->batt_psy;
}

static struct oplus_voocphy_operations oplus_adsp_voocphy_ops = {
	.adsp_voocphy_enable = oplus_adsp_voocphy_enable,
	.adsp_voocphy_reset_again = oplus_adsp_voocphy_reset_again,
	.adsp_set_ap_fastchg_allow = oplus_chg_set_ap_fastchg_allow_to_voocphy,
};

#define VOLTAGE_2000MV   2000
#define TIME_3SEC     3000

static void oplus_adsp_voocphy_clear_status(struct device *dev)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);

	chip->fastchg_dummy_start = false;
	chip->fastchg_ing = false;
	chip->fastchg_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
}

static void oplus_voocphy_check_charger_out_work_func(struct work_struct *work)
{
	struct oplus_voocphy_manager *chip = container_of(work,
		struct oplus_voocphy_manager, voocphy_check_charger_out_work.work);
	int chg_vol = 0;

	chg_vol = oplus_chglib_get_charger_voltage();
	if (chg_vol >= 0 && chg_vol < VOLTAGE_2000MV) {
		cancel_delayed_work(&chip->voocphy_send_ongoing_notify);
		oplus_adsp_voocphy_clear_status(chip->dev);
		if (oplus_chglib_get_vooc_is_started(chip->dev))
			oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
		chg_info("charger out, fastchg_dummy_start:%d\n",
			 chip->fastchg_dummy_start);
	}
}

static void oplus_voocphy_check_charger_out(struct oplus_voocphy_manager *chip,
					    unsigned int delay_ms)
{
	chg_info("check chg out after %d ms\n", delay_ms);
	schedule_delayed_work(&chip->voocphy_check_charger_out_work,
			      round_jiffies_relative(msecs_to_jiffies(delay_ms)));
}

static void oplus_adsp_voocphy_send_ongoing_notify(struct work_struct *work)
{
	struct oplus_voocphy_manager *chip = container_of(work,
		struct oplus_voocphy_manager, voocphy_send_ongoing_notify.work);
	int chg_vol = 0;

	if (!chip->fastchg_start) {
		chg_debug("fastchg_start is false\n");
		return;
	}

	chg_vol = oplus_chglib_get_charger_voltage();
	if ((chg_vol >= 0 && chg_vol < VOLTAGE_2000MV)) {
		chg_debug("vbus does not exist\n");
		return;
	}

	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ONGOING);
	schedule_delayed_work(&chip->voocphy_send_ongoing_notify,
			      msecs_to_jiffies(SEND_FASTCHG_ONGOING_NOTIFY_INTERVAL));
}

static void oplus_adsp_voocphy_switch_chg_mode(struct device *dev, int mode)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);
	chg_err("oplus_adsp_voocphy_switch_chg_mode :%d\n", mode);
	if (mode == 1) {
		/*
		 * Voocphy sends fastchg status 0x54 to notify
		 * vooc to update the current soc and temp range.
		 */
		oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_LOW_TEMP_FULL);
		oplus_chg_set_match_temp_ui_soc_to_voocphy();
		g_voocphy_chip->ops->adsp_voocphy_enable(1);
	} else {
		g_voocphy_chip->ops->adsp_voocphy_enable(0);
	}
}

static void oplus_adsp_voocphy_set_ap_fastchg_allow(struct device *dev, int allow, bool dummy)
{
	chg_err("allow :%d\n", allow);
	if (allow == 1) {
		g_voocphy_chip->ops->adsp_set_ap_fastchg_allow(1);
	} else {
		g_voocphy_chip->ops->adsp_set_ap_fastchg_allow(0);
	}

	if (dummy) {
		g_voocphy_chip->fastchg_notify_status = FAST_NOTIFY_DUMMY_START;
		cancel_delayed_work(&g_voocphy_chip->voocphy_send_ongoing_notify);
	}
}

static int oplus_adsp_voocphy_get_fastchg_type(struct device *dev)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);

	return chip->fast_chg_type;
}

static int oplus_adsp_voocphy_get_fastchg_notify_status(struct device *dev)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);

	return chip->fastchg_notify_status;
}

static void oplus_adsp_voocphy_disconnect_detect(struct device *dev)
{
        struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);

	oplus_voocphy_check_charger_out(chip, 0);
}

static void oplus_adsp_voocphy_handle_track_status(
				struct oplus_voocphy_manager *chip, int event)
{
	int track_status;

	track_status = (event >> 16) & 0xFF;
	chg_err("track_status[0x%x]\n", track_status);

	switch(track_status) {
	case ADSP_VPHY_FAST_NOTIFY_BAD_CONNECTED:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_BAD_CONNECTED);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ERR_COMMU:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_FRAME_H_ERR);
		break;
	case ADSP_VPHY_FAST_NOTIFY_COMMU_CLK_ERR:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_CLK_ERR);
		break;
	case ADSP_VPHY_FAST_NOTIFY_HW_VBATT_HIGH:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_HW_VBATT_HIGH);
		break;
	case ADSP_VPHY_FAST_NOTIFY_HW_TBATT_HIGH:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_HW_TBATT_HIGH);
		break;
	case ADSP_VPHY_FAST_NOTIFY_COMMU_TIME_OUT:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_COMMU_TIME_OUT);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ADAPTER_COPYCAT:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_ADAPTER_COPYCAT);
		break;
	case ADSP_VPHY_FAST_NOTIFY_BTB_TEMP_OVER:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_BTB_TEMP_OVER);
		break;
	case ADSP_VPHY_FAST_NOTIFY_PRESENT:
	case ADSP_VPHY_FAST_NOTIFY_DUMMY_START:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_BREAK_DEFAULT);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ONGOING:
		if (chip->fast_chg_type == FASTCHG_CHARGER_TYPE_UNKOWN)
			oplus_chglib_push_break_code(chip->dev,
						     TRACK_ADSP_VOOCPHY_BREAK_DEFAULT);
		break;
	case ADSP_VPHY_FAST_NOTIFY_FULL:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_FULL);
		break;
	case ADSP_VPHY_FAST_NOTIFY_SWITCH_TEMP_RANGE:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_SWITCH_TEMP_RANGE);
		break;
	case ADSP_VPHY_FAST_NOTIFY_BATT_TEMP_OVER:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_BATT_TEMP_OVER);
		break;
	default:
		oplus_chglib_push_break_code(chip->dev,
					     TRACK_ADSP_VOOCPHY_OTHER);
		break;
	}
}

static void oplus_adsp_voocphy_handle_present_event(struct oplus_voocphy_manager *chip,
						    int event)
{
	chip->fastchg_start = true;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fast_chg_type = ((event >> 8) & 0x7F) ;
	chip->fastchg_notify_status = FAST_NOTIFY_PRESENT;
	chg_info("fastchg start: [%d], adapter version: [0x%0x]\n",
		 chip->fastchg_start, chip->fast_chg_type);
	oplus_adsp_voocphy_handle_track_status(chip, event);
	if (is_batt_psy_available(chip)) {
		power_supply_changed(chip->batt_psy);
	}
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_PRESENT);
	if (chip->fast_chg_type == FASTCHG_CHARGER_TYPE_VOOC) {
		schedule_delayed_work(&chip->voocphy_send_ongoing_notify,
			msecs_to_jiffies(SEND_FASTCHG_ONGOING_NOTIFY_INTERVAL));
	}
}

static void oplus_adsp_voocphy_handle_dummy_event(struct oplus_voocphy_manager *chip,
						  int event)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = true;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->fast_chg_type = ((event >> 8) & 0x7F);
	chip->fastchg_notify_status = FAST_NOTIFY_DUMMY_START;
	chg_info("fastchg dummy start:[%d], adapter version:[0x%0x]\n",
		 chip->fastchg_dummy_start, chip->fast_chg_type);
	oplus_adsp_voocphy_handle_track_status(chip, event);
	if (is_batt_psy_available(chip)) {
		power_supply_changed(chip->batt_psy);
	}
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_PRESENT);
}

static void oplus_adsp_voocphy_handle_copycat_event(struct oplus_voocphy_manager *chip,
						  int event)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = true;
	chip->fastchg_ing = false;
	chip->fastchg_notify_status = FAST_NOTIFY_ADAPTER_COPYCAT;
	chg_info("fastchg to normal: FAST_NOTIFY_ADAPTER_COPYCAT\n");

	oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
}

static void oplus_adsp_voocphy_handle_ongoing_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_ing = true;
	chg_info("fastchg ongoing\n");
	chip->fastchg_notify_status = FAST_NOTIFY_ONGOING;
	if (chip->fast_chg_type == FASTCHG_CHARGER_TYPE_UNKOWN) {
		chip->fastchg_start = true;
		chip->fast_chg_type = oplus_adsp_voocphy_get_fast_chg_type();
		if (is_batt_psy_available(chip))
			power_supply_changed(chip->batt_psy);
	}
	oplus_chglib_disable_charger(true);
	oplus_chglib_suspend_charger(true);
	if (oplus_chglib_get_vooc_sid_is_config(chip->dev))
		oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ONGOING);
	else
		chg_info("wait send FAST_NOTIFY_ONGOING event\n");
}

static void oplus_adsp_voocphy_common_handle(struct oplus_voocphy_manager *chip,
					     int event)
{
	int real_fastchg_status = 0;
	int btbover_std_version = 0;

	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = true;
	chip->fastchg_ing = false;
	if (is_batt_psy_available(chip))
		power_supply_changed(chip->batt_psy);
	if ((event & 0xFF) == ADSP_VPHY_FAST_NOTIFY_FULL) {
		chip->fastchg_notify_status = FAST_NOTIFY_FULL;
		real_fastchg_status = (event >> 16) & 0xFF;
		chg_info("fastchg to normal: FAST_NOTIFY_FULL\n");
	} else {
		chip->fastchg_notify_status = FAST_NOTIFY_BAD_CONNECTED;
		chg_info("fastchg to normal: FAST_NOTIFY_BAD_CONNECTED\n");
	}
	if (real_fastchg_status == ADSP_VPHY_FAST_NOTIFY_HW_TBATT_HIGH ||
	    real_fastchg_status == ADSP_VPHY_FAST_NOTIFY_BTB_TEMP_OVER) {
		chip->fastchg_notify_status = FAST_NOTIFY_BTB_TEMP_OVER;
		btbover_std_version = ((event >> 8) & 0xFF);
		if (btbover_std_version == BTBOVER_5V1A_CHARGE_STD)
			chip->btb_temp_over = true;
		else
			chip->btb_temp_over = false;
	} else {
		chip->btb_temp_over = false;
	}

	oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
}

static void oplus_adsp_voocphy_handle_batt_temp_over_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = true;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fastchg_notify_status = FAST_NOTIFY_BATT_TEMP_OVER;
	chg_info("fastchg to warm: [%d]\n", chip->fastchg_to_warm);
	if (is_batt_psy_available(chip))
		power_supply_changed(chip->batt_psy);
	oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
}

static void oplus_adsp_voocphy_handle_err_commu_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	chip->fastchg_notify_status = FAST_NOTIFY_ERR_COMMU;
	oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
	chg_info("fastchg err commu\n");
}

static void oplus_adsp_voocphy_handle_crash_event(struct oplus_voocphy_manager *chip)
{
	chg_info("crash start\n");
	if (chip->fastchg_start == true) {
		chip->fastchg_start = false;
		chip->fastchg_to_warm = false;
		chip->fastchg_dummy_start = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_ing = false;
		chip->btb_temp_over = false;
		chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
		chip->fastchg_notify_status = FAST_NOTIFY_ERR_ADSP_CRASH;
		oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
		chg_info("FAST_NOTIFY_ERR_ADSP_CRASH\n");
	}
}

static void oplus_adsp_voocphy_handle_switch_temp_range_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = true;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fastchg_notify_status = FAST_NOTIFY_SWITCH_TEMP_RANGE;
	if (is_batt_psy_available(chip))
		power_supply_changed(chip->batt_psy);
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ONGOING);
	chg_info("fastchg switch temp range\n");
}

static void oplus_adsp_voocphy_handle_clk_err_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fastchg_notify_status = FAST_NOTIFY_ABSENT;
	if (is_batt_psy_available(chip))
		power_supply_changed(chip->batt_psy);
	chg_info("fastchg commu clk err\n");
	if (oplus_chglib_is_pd_svooc_adapter(chip->dev) &&
	    oplus_chglib_is_wired_present(chip->dev) &&
	    oplus_chglib_get_vooc_is_started(chip->dev))
		msleep(100);
	else
		chg_info("1+ abnormal adpater icon hold[%d %d %d]\n",
			 oplus_chglib_is_pd_svooc_adapter(chip->dev),
			 oplus_chglib_is_wired_present(chip->dev),
			 oplus_chglib_get_vooc_is_started(chip->dev));

	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
}

static void oplus_adsp_voocphy_handle_vbatt_high_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
	chg_info("fastchg hw vbatt high\n");
}

static void oplus_adsp_voocphy_handle_tbatt_high_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
	chg_info("fastchg hw tbatt high\n");
}

static void oplus_adsp_voocphy_handle_commu_time_out_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->btb_temp_over = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	chip->fastchg_notify_status = FAST_NOTIFY_ABSENT;
	oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
	chg_info("fastchg commu timeout\n");
}

static void oplus_adsp_voocphy_handle_unknown_event(struct oplus_voocphy_manager *chip)
{
	chip->fastchg_start = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_dummy_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_ing = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	chip->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
	oplus_chglib_notify_ap(chip->dev, chip->fastchg_notify_status);
}

void oplus_adsp_voocphy_fastchg_event_handle(int event)
{
	struct oplus_voocphy_manager *chip = g_voocphy_chip;

	if (!chip) {
		chg_info("chip is null, return\n");
		return;
	}

	chg_info("status: [0x%x]\n", event);
	if (((event & 0xFF) != ADSP_VPHY_FAST_NOTIFY_PRESENT) &&
	    ((event & 0xFF) != ADSP_VPHY_FAST_NOTIFY_DUMMY_START))
		oplus_adsp_voocphy_handle_track_status(chip, event);
	chip->adsp_voocphy_rx_data = (event & 0xFF);

	switch (event & 0xFF) {
	case ADSP_VPHY_FAST_NOTIFY_PRESENT:
		oplus_adsp_voocphy_handle_present_event(chip, event);
		break;
	case ADSP_VPHY_FAST_NOTIFY_DUMMY_START:
		oplus_adsp_voocphy_handle_dummy_event(chip, event);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ADAPTER_COPYCAT:
		oplus_adsp_voocphy_handle_copycat_event(chip, event);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ONGOING:
		oplus_adsp_voocphy_handle_ongoing_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_FULL:
	case ADSP_VPHY_FAST_NOTIFY_BAD_CONNECTED:
		oplus_adsp_voocphy_common_handle(chip, event);
		break;
	case ADSP_VPHY_FAST_NOTIFY_BATT_TEMP_OVER:
		oplus_adsp_voocphy_handle_batt_temp_over_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_ERR_COMMU:
		oplus_adsp_voocphy_handle_err_commu_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_SWITCH_TEMP_RANGE:
		oplus_adsp_voocphy_handle_switch_temp_range_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_COMMU_CLK_ERR:
		oplus_adsp_voocphy_handle_clk_err_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_HW_VBATT_HIGH:
		oplus_adsp_voocphy_handle_vbatt_high_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_HW_TBATT_HIGH:
		oplus_adsp_voocphy_handle_tbatt_high_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_COMMU_TIME_OUT:
		oplus_adsp_voocphy_handle_commu_time_out_event(chip);
		break;
	case ADSP_VPHY_FAST_NOTIFY_CRASH:
		oplus_adsp_voocphy_handle_crash_event(chip);
		break;
	default:
		chg_info("non handle status: [%d]\n", event);
		oplus_adsp_voocphy_handle_unknown_event(chip);
		break;
	}
}

void oplus_adsp_voocphy_reset_status(void)
{
	struct oplus_voocphy_manager *chip = g_voocphy_chip;

	if (!chip) {
		chg_info("chip is null, return\n");
		return;
	}

	if (chip->fast_chg_type != FASTCHG_CHARGER_TYPE_UNKOWN) {
		chip->fastchg_dummy_start = true;
	}
	chip->fastchg_ing = false;
	chip->fastchg_start = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
	if (oplus_chglib_get_vooc_is_started(chip->dev))
		oplus_chglib_notify_ap(chip->dev, FAST_NOTIFY_ABSENT);
}

#define OPLUS_SMART_QUICK_GAIN_LED_ON_DEVIATION         1000
#define OPLUS_SMART_QUICK_GAIN_LED_OFF_DEVIATION        250
static bool oplus_chg_get_led_status(void)
{
	bool led_status = false;
	struct oplus_mms *comm_topic = NULL;
	union mms_msg_data data = { 0 };
	int rc;

	comm_topic = oplus_mms_get_by_name("common");
	if (!comm_topic)
		return 0;

	rc = oplus_mms_get_item_data(comm_topic, COMM_ITEM_LED_ON, &data, true);
	if (!rc)
		led_status = !!data.intval;

	return led_status;
}

static int oplus_chg_get_gauge_current(void)
{
	int icharging = 0;
	struct oplus_mms *gauge_topic = NULL;
	union mms_msg_data data = { 0 };
	int rc;

	gauge_topic = oplus_mms_get_by_name("gauge");
	if (!gauge_topic)
		return -1;

	rc = oplus_mms_get_item_data(gauge_topic, GAUGE_ITEM_CURR, &data, true);
	if (!rc)
		icharging = data.intval;

	return icharging;
}

static int oplus_adsp_voocphy_batt_curve_current(void)
{
	int icharging = 0;
	int curr= 0;
	bool led_on = false;

	icharging = oplus_chg_get_gauge_current();
	led_on = oplus_chg_get_led_status();

	if (led_on)
		curr = (-icharging + OPLUS_SMART_QUICK_GAIN_LED_ON_DEVIATION);
	else
		curr = (-icharging + OPLUS_SMART_QUICK_GAIN_LED_OFF_DEVIATION);

	if (curr < 0)
		curr = 0;

	chg_debug("icharging = %d to curr = %d, led_on = %d\n",
		  icharging, curr, led_on);

	return curr;
}

static int oplus_adsp_voocphy_get_batt_curve_current(struct device *dev)
{
	struct oplus_voocphy_manager *chip;

	if (dev == NULL)
		return 100;
	chip = dev_get_drvdata(dev);

	return oplus_adsp_voocphy_batt_curve_current();
}

static void oplus_adsp_voocphy_set_vooc_current(struct device *dev, int data, int curr_ma)
{
	struct oplus_voocphy_manager *chip = g_voocphy_chip;
	int cool_down = 0;

	if (!chip) {
		chg_info("chip is null, return\n");
		return;
	}
	cool_down = data >> 1;
	chg_info("set cool_down %d\n", cool_down);
	oplus_adsp_voocphy_set_cool_down(cool_down);
}

int oplus_adsp_voocphy_get_bcc_max_curr(struct device *dev)
{
	int current_bcc_max;
	struct oplus_voocphy_manager *chip = g_voocphy_chip;

	if (!chip) {
		chg_info("chip is null, return\n");
		return 0;
	}
	current_bcc_max = oplus_adsp_voocphy_get_bcc_max_current();

	return current_bcc_max;
}

int oplus_adsp_voocphy_get_bcc_min_curr(struct device *dev)
{
	int current_bcc_min;
	struct oplus_voocphy_manager *chip = g_voocphy_chip;

	if (!chip) {
		chg_info("chip is null, return\n");
		return 0;
	}
	current_bcc_min = oplus_adsp_voocphy_get_bcc_min_current();

	return current_bcc_min;
}

int oplus_adsp_voocphy_get_bcc_exit_curr(struct device *dev)
{
	int current_bcc_exit;
	struct oplus_voocphy_manager *chip = g_voocphy_chip;

	if (!chip) {
		chg_info("chip is null, return\n");
		return 0;
	}
	current_bcc_exit = oplus_adsp_voocphy_get_atl_last_geat_current();

	return current_bcc_exit;
}
struct hw_vphy_info adsp_voocphy_vinf = {
	.vphy_switch_chg_mode = oplus_adsp_voocphy_switch_chg_mode,
	.vphy_get_fastchg_type  = oplus_adsp_voocphy_get_fastchg_type,
	.vphy_get_fastchg_notify_status = oplus_adsp_voocphy_get_fastchg_notify_status,
	.vphy_disconnect_detect = oplus_adsp_voocphy_disconnect_detect,
	.vphy_clear_status = oplus_adsp_voocphy_clear_status,
	.vphy_get_batt_curve_current = oplus_adsp_voocphy_get_batt_curve_current,
	.vphy_set_fastchg_ap_allow = oplus_adsp_voocphy_set_ap_fastchg_allow,
	.vphy_set_vooc_current = oplus_adsp_voocphy_set_vooc_current,
	.vphy_get_bcc_max_curr = oplus_adsp_voocphy_get_bcc_max_curr,
	.vphy_get_bcc_min_curr = oplus_adsp_voocphy_get_bcc_min_curr,
	.vphy_get_bcc_exit_curr = oplus_adsp_voocphy_get_bcc_exit_curr,
};

static int adsp_voocphy_probe(struct platform_device *pdev)
{
	struct oplus_voocphy_manager *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_voocphy_manager),
			    GFP_KERNEL);
	if (!chip) {
		pr_err("oplus_voocphy_manager devm_kzalloc failed.\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	chip->ops = &oplus_adsp_voocphy_ops;
	platform_set_drvdata(pdev, chip);
	chip->vchip = oplus_chglib_register_vphy(chip->dev, &adsp_voocphy_vinf);
	if (IS_ERR(chip->vchip)) {
		chg_err("[%s]: fail to register vphy\n", __func__);
		return PTR_ERR(chip->vchip);
	}
	oplus_adsp_voocphy_clear_status(chip->dev);
	INIT_DELAYED_WORK(&chip->voocphy_check_charger_out_work,
			  oplus_voocphy_check_charger_out_work_func);
	INIT_DELAYED_WORK(&chip->voocphy_send_ongoing_notify,
			  oplus_adsp_voocphy_send_ongoing_notify);
	g_voocphy_chip = chip;

	chg_info("[%s]: adsp voocphy probe success\n", __func__);

	return 0;
}

static const struct of_device_id adsp_voocphy_table[] = {
	{ .compatible = "oplus,adsp-voocphy" },
	{},
};

static struct platform_driver adsp_voocphy_driver = {
	.driver = {
		.name = "oplus_adsp_voocphy",
		.of_match_table = adsp_voocphy_table,
	},
	.probe = adsp_voocphy_probe,
};

int adsp_voocphy_init(void)
{
	int ret;
	chg_info("start\n");
	ret = platform_driver_register(&adsp_voocphy_driver);
	return ret;
}

void adsp_voocphy_exit(void)
{
	platform_driver_unregister(&adsp_voocphy_driver);
}
oplus_chg_module_register(adsp_voocphy);

MODULE_DESCRIPTION("oplus adsp voocphy driver");
MODULE_LICENSE("GPL v2");
