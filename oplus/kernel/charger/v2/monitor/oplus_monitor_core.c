#define pr_fmt(fmt) "[MONITOR]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/power_supply.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#endif
#include <oplus_chg_module.h>
#include <oplus_chg_vooc.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_comm.h>
#include <oplus_battery_log.h>
#include <oplus_smart_chg.h>
#include <oplus_chg_ufcs.h>
#include <oplus_chg_wls.h>

#include "oplus_monitor_internal.h"
#include <oplus_chg_dual_chan.h>

__maybe_unused static bool is_fv_votable_available(struct oplus_monitor *chip)
{
	if (!chip->fv_votable)
		chip->fv_votable = find_votable("FV_MIN");
	return !!chip->fv_votable;
}

__maybe_unused static bool
is_wired_icl_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wired_icl_votable)
		chip->wired_icl_votable = find_votable("WIRED_ICL");
	return !!chip->wired_icl_votable;
}

__maybe_unused static bool
is_wired_fcc_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wired_fcc_votable)
		chip->wired_fcc_votable = find_votable("WIRED_FCC");
	return !!chip->wired_fcc_votable;
}

__maybe_unused static bool
is_wired_charge_suspend_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wired_charge_suspend_votable)
		chip->wired_charge_suspend_votable =
			find_votable("WIRED_CHARGE_SUSPEND");
	return !!chip->wired_charge_suspend_votable;
}

__maybe_unused static bool
is_wired_charging_disable_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wired_charging_disable_votable)
		chip->wired_charging_disable_votable =
			find_votable("WIRED_CHARGING_DISABLE");
	return !!chip->wired_charging_disable_votable;
}

__maybe_unused static bool
is_wls_icl_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_icl_votable)
		chip->wls_icl_votable = find_votable("WLS_NOR_ICL");
	return !!chip->wls_icl_votable;
}

__maybe_unused static bool
is_wls_fcc_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_fcc_votable)
		chip->wls_fcc_votable = find_votable("WLS_FCC");
	return !!chip->wls_fcc_votable;
}

__maybe_unused static bool
is_wls_charge_suspend_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_charge_suspend_votable)
		chip->wls_charge_suspend_votable =
			find_votable("WLS_CHARGE_SUSPEND");
	return !!chip->wls_charge_suspend_votable;
}

__maybe_unused static bool
is_wls_charging_disable_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_charging_disable_votable)
		chip->wls_charging_disable_votable =
			find_votable("WLS_NOR_OUT_DISABLE");
	return !!chip->wls_charging_disable_votable;
}

__maybe_unused static bool
is_chg_disable_votable_available(struct oplus_monitor *chip)
{
	if (!chip->chg_disable_votable)
		chip->chg_disable_votable = find_votable("CHG_DISABLE");
	return !!chip->chg_disable_votable;
}

__maybe_unused static bool
is_vooc_curr_votable_available(struct oplus_monitor *chip)
{
	if (!chip->vooc_curr_votable)
		chip->vooc_curr_votable = find_votable("VOOC_CURR");
	return !!chip->vooc_curr_votable;
}

static bool is_main_gauge_topic_available(struct oplus_monitor *chip)
{
	if (!chip->main_gauge_topic)
		chip->main_gauge_topic = oplus_mms_get_by_name("gauge:0");

	return !!chip->main_gauge_topic;
}

static bool is_sub_gauge_topic_available(struct oplus_monitor *chip)
{
	if (!chip->sub_gauge_topic)
		chip->sub_gauge_topic = oplus_mms_get_by_name("gauge:1");

	if (!chip->sub_gauge_topic)
		chg_err(" get gauge:1 error\n");

	return !!chip->sub_gauge_topic;
}

static void oplus_monitor_update_charge_info(struct oplus_monitor *chip)
{
	union mms_msg_data data = { 0 };

	if (is_wired_icl_votable_available(chip))
		chip->wired_icl_ma = get_effective_result(chip->wired_icl_votable);
	if (is_wired_charge_suspend_votable_available(chip))
		chip->wired_suspend = get_effective_result(chip->wired_charge_suspend_votable);
	if (is_wired_charge_suspend_votable_available(chip))
		chip->wired_user_suspend = get_client_vote(chip->wired_charge_suspend_votable, USER_VOTER);
	chip->wired_vbus_mv = oplus_wired_get_vbus();

	if (is_wls_icl_votable_available(chip))
		chip->wls_icl_ma = get_effective_result(chip->wls_icl_votable);
	if (is_wls_charge_suspend_votable_available(chip))
		chip->wls_suspend = get_effective_result(chip->wls_charge_suspend_votable);
	if (is_wls_charge_suspend_votable_available(chip))
		chip->wls_user_suspend = get_client_vote(chip->wls_charge_suspend_votable, USER_VOTER);

	if (is_chg_disable_votable_available(chip))
		chip->mmi_chg = !get_client_vote(chip->chg_disable_votable, MMI_CHG_VOTER);
	if (is_vooc_curr_votable_available(chip))
		chip->bcc_current = get_client_vote(chip->vooc_curr_votable, BCC_VOTER);

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_USB_STATUS, &data, true);
	chip->usb_status = data.intval;

	chip->normal_cool_down = oplus_smart_chg_get_normal_cool_down();
	chip->otg_switch_status = oplus_wired_get_otg_switch_status();

	if (chip->wired_online) {
		if (is_wired_fcc_votable_available(chip))
			chip->fcc_ma = get_effective_result(chip->wired_fcc_votable);
		if (is_fv_votable_available(chip))
			chip->fv_mv = get_effective_result(chip->fv_votable);
		if (is_wired_charging_disable_votable_available(chip))
			chip->chg_disable = get_effective_result(chip->wired_charging_disable_votable);
		if (is_wired_charging_disable_votable_available(chip))
			chip->chg_user_disable = get_client_vote(chip->wired_charging_disable_votable, USER_VOTER);

		chip->wired_ibus_ma = oplus_wired_get_ibus();
	} else if (chip->wls_online) {
		if (is_wls_fcc_votable_available(chip))
			chip->fcc_ma = get_effective_result(chip->wls_fcc_votable);
		if (is_fv_votable_available(chip))
			chip->fv_mv = get_effective_result(chip->fv_votable);
		if (is_wls_charging_disable_votable_available(chip))
			chip->chg_disable = get_effective_result(chip->wls_charging_disable_votable);
		if (is_wls_charging_disable_votable_available(chip))
			chip->chg_user_disable = get_client_vote(chip->wls_charging_disable_votable, USER_VOTER);
		if (chip->wls_topic) {
			oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_IOUT, &data, true);
			chip->wls_iout_ma = data.intval;
			oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_VOUT, &data, true);
			chip->wls_vout_mv = data.intval;
			oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_WLS_TYPE, &data, true);
			chip->wls_charge_type = data.intval;
		}
	} else {
		chip->fcc_ma = 0;
		chip->fv_mv = 0;
		chip->chg_disable = true;
		chip->chg_user_disable = true;

		chip->wired_ibus_ma = 0;

		chip->wls_iout_ma = 0;
		chip->wls_vout_mv = 0;
		chip->wls_charge_type = 0;
	}
}

static bool oplus_monitor_all_topic_is_ready(struct oplus_monitor *chip)
{
	if (!chip->wired_topic) {
		chg_err("wired topic not ready\n");
		return false;
	}
	/* TODO: wirelsee */
	if (!chip->gauge_topic) {
		chg_err("gauge topic not ready\n");
		return false;
	}
	if (!chip->vooc_topic) {
		chg_err("vooc topic not ready\n");
		return false;
	}
	if (!chip->comm_topic) {
		chg_err("common topic not ready\n");
		return false;
	}

	return true;
}

#define DUMP_REG_LOG_CNT_30S	3
static void oplus_monitor_charge_info_update_work(struct work_struct *work)
{
	struct oplus_monitor *chip = container_of(work, struct oplus_monitor,
						  charge_info_update_work);
	union mms_msg_data data = { 0 };
	static int dump_count = 0;
	int rc;

	if (chip->wired_online || chip->wls_online)
		oplus_mms_restore_publish(chip->err_topic);
	else
		oplus_mms_stop_publish(chip->err_topic);

	oplus_monitor_update_charge_info(chip);

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP,
				     &data, false);
	if (!rc)
		chip->shell_temp = data.intval;

	/*
	 * wait for all dependent topics to be prepared to ensure that the
	 * collected data is normal
	 */
	if (oplus_monitor_all_topic_is_ready(chip))
		oplus_chg_track_comm_monitor(chip);

	printk(KERN_INFO "OPLUS_CHG[oplus_charge_info]: "
		"BATTERY[%d %d %d %d %d %d %d %d %d %d %d 0x%x], "
		"CHARGE[%d %d %d %d], "
		"WIRED[%d %d %d %d %d 0x%x %d %d %d %d %d], "
		"WIRELESS[%d %d %d %d %d 0x%x %d %d], "
		"VOOC[%d %d %d %d 0x%x], "
		"UFCS[%d %d %d 0x%x], "
		"COMMON[%d %d %d 0x%x %d]",
		chip->batt_temp, chip->shell_temp, chip->vbat_mv,
		chip->vbat_min_mv, chip->ibat_ma, chip->batt_soc, chip->ui_soc,
		chip->smooth_soc, chip->batt_rm, chip->batt_fcc, chip->batt_exist,
		chip->batt_err_code,
		chip->fv_mv, chip->fcc_ma, chip->chg_disable, chip->chg_user_disable,
		chip->wired_online, chip->wired_ibus_ma, chip->wired_vbus_mv,
		chip->wired_icl_ma, chip->wired_charge_type, chip->wired_err_code,
		chip->wired_suspend, chip->wired_user_suspend, chip->cc_mode,
		chip->cc_detect, chip->otg_enable,
		chip->wls_online, chip->wls_iout_ma, chip->wls_vout_mv,
		chip->wls_icl_ma, chip->wls_charge_type, chip->wls_err_code,
		chip->wls_suspend, chip->wls_user_suspend,
		chip->vooc_online, chip->vooc_started, chip->vooc_charging,
		chip->vooc_online_keep, chip->vooc_sid,
		chip->ufcs_online, chip->ufcs_charging, chip->ufcs_oplus_adapter,
		chip->ufcs_adapter_id,
		chip->temp_region, chip->ffc_status, chip->cool_down,
		chip->notify_code, chip->led_on);

	if (!chip->wired_online)
		dump_count++;

	if ((chip->wired_online || dump_count == DUMP_REG_LOG_CNT_30S)
		&& (!chip->vooc_started || oplus_vooc_get_voocphy_support(chip->vooc_topic) == ADSP_VOOCPHY)) {
		dump_count = 0;
		oplus_wired_dump_regs();
	}
}

static int comm_info_dump_log_data(char *buffer, int size, void *dev_data)
{
	struct oplus_monitor *chip = dev_data;

	if (!buffer || !chip)
		return -ENOMEM;

	snprintf(buffer, size, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
		"%d,%d,%d,%d,%d,%d,%d,%d,%d",
		chip->batt_temp, chip->shell_temp, chip->vbat_mv, chip->vbat_min_mv, chip->ibat_ma,
		chip->batt_soc, chip->ui_soc, chip->wired_online, chip->wired_charge_type, chip->notify_code,
		chip->wired_ibus_ma, chip->wired_vbus_mv, chip->smooth_soc, chip->led_on, chip->fv_mv,
		chip->fcc_ma, chip->wired_icl_ma, chip->otg_switch_status, chip->cool_down, chip->bcc_current,
		chip->normal_cool_down, chip->chg_cycle_status, chip->mmi_chg, chip->usb_status, chip->cc_detect,
		chip->batt_full, chip->rechging, chip->pd_svooc, chip->batt_status);

	return 0;
}

static int comm_info_get_log_head(char *buffer, int size, void *dev_data)
{
	struct oplus_monitor *chip = dev_data;

	if (!buffer || !chip)
		return -ENOMEM;

	snprintf(buffer, size,
		",batt_temp,shell_temp,vbat_mv,vbat_min_mv,ibat_ma,"
		"batt_soc,ui_soc,wired_online,charge_type,notify_code,"
		"wired_ibus_ma,wired_vbus_mv,smooth_soc,led_on,fv_mv,"
		"fcc_ma,wired_icl_ma,otg_switch,cool_down,bcc_current,normal_cool_down,chg_cycle,"
		"mmi_chg,usb_status,cc_detect,batt_full,rechging,pd_svooc,prop_status");

	return 0;
}

static struct battery_log_ops battlog_comm_ops = {
	.dev_name = "comm_info",
	.dump_log_head = comm_info_get_log_head,
	.dump_log_content = comm_info_dump_log_data,
};

static void oplus_monitor_dual_chan_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case SWITCH_ITEM_DUAL_CHAN_STATUS:
			oplus_mms_get_item_data(chip->dual_chan_topic, id, &data,
						false);
			if (!!data.intval)
				oplus_chg_track_record_dual_chan_start(chip);
			else
				oplus_chg_track_record_dual_chan_end(chip);
			break;
		default:
			break;
		}
		    break;
	default:
		    break;
	}
}

static void oplus_monitor_subscribe_dual_chan_topic(struct oplus_mms *topic,
					void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->dual_chan_topic = topic;
	chip->dual_chan_subs =
		oplus_mms_subscribe(chip->dual_chan_topic, chip,
				    oplus_monitor_dual_chan_subs_callback,
				    "monitor");
	if (IS_ERR_OR_NULL(chip->dual_chan_subs)) {
		chg_err("subscribe dual_chan topic error, rc=%ld\n",
			PTR_ERR(chip->dual_chan_subs));
		return;
	}
	oplus_mms_get_item_data(chip->dual_chan_topic, SWITCH_ITEM_DUAL_CHAN_STATUS, &data,
				true);
	if (!!data.intval)
		oplus_chg_track_record_dual_chan_start(chip);
	else
		oplus_chg_track_record_dual_chan_end(chip);
}


static void oplus_monitor_gauge_subs_callback(struct mms_subscribe *subs,
					      enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX,
					&data, false);
		chip->vbat_mv = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_TEMP,
					&data, false);
		chip->batt_temp = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MIN,
					&data, false);
		chip->vbat_min_mv = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC,
					&data, false);
		chip->batt_soc = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR,
					&data, false);
		chip->ibat_ma = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC,
					&data, false);
		chip->batt_fcc = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
					false);
		chip->batt_rm = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CC, &data,
					false);
		chip->batt_cc = data.intval;
		oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOH, &data,
					false);
		chip->batt_soh = data.intval;
		schedule_work(&chip->charge_info_update_work);
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case GAUGE_ITEM_BATT_EXIST:
			oplus_mms_get_item_data(chip->gauge_topic, id, &data,
						false);
			chip->batt_exist = !!data.intval;
			break;
		case GAUGE_ITEM_ERR_CODE:
			oplus_mms_get_item_data(chip->gauge_topic, id, &data,
						false);
			chip->batt_err_code = (unsigned int)data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_gauge_topic(struct oplus_mms *topic,
					    void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_monitor_gauge_subs_callback,
				    "monitor");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
				true);
	chip->vbat_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_TEMP, &data,
				true);
	chip->batt_temp = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MIN, &data,
				true);
	chip->vbat_min_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				true);
	chip->batt_soc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data,
				true);
	chip->ibat_ma = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC, &data,
				true);
	chip->batt_fcc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
				true);
	chip->batt_rm = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CC, &data,
				true);
	chip->batt_cc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOH, &data,
				true);
	chip->batt_soh = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_BATT_EXIST, &data,
				true);
	chip->batt_exist = !!data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_ERR_CODE, &data,
				true);
	chip->batt_err_code = (unsigned int)data.intval;
}

static void oplus_monitor_ufcs_subs_callback(struct mms_subscribe *subs,
					     enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int rc;

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case UFCS_ITEM_ONLINE:
			rc = oplus_mms_get_item_data(chip->ufcs_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->ufcs_online = !!data.intval;
			break;
		case UFCS_ITEM_CHARGING:
			rc = oplus_mms_get_item_data(chip->ufcs_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->ufcs_charging = !!data.intval;
			break;
		case UFCS_ITEM_ADAPTER_ID:
			rc = oplus_mms_get_item_data(chip->ufcs_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->ufcs_adapter_id = (u32)data.intval;
			break;
		case UFCS_ITEM_OPLUS_ADAPTER:
			rc = oplus_mms_get_item_data(chip->ufcs_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->ufcs_oplus_adapter = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_ufcs_topic(struct oplus_mms *topic,
					       void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->ufcs_topic = topic;
	chip->ufcs_subs =
		oplus_mms_subscribe(chip->ufcs_topic, chip,
				    oplus_monitor_ufcs_subs_callback,
				    "monitor");
	if (IS_ERR_OR_NULL(chip->ufcs_subs)) {
		chg_err("subscribe ufcs topic error, rc=%ld\n",
			PTR_ERR(chip->ufcs_subs));
		return;
	}

	rc = oplus_mms_get_item_data(chip->ufcs_topic, UFCS_ITEM_ONLINE, &data, true);
	if (rc >= 0)
		chip->ufcs_online = !!data.intval;
	rc = oplus_mms_get_item_data(chip->ufcs_topic, UFCS_ITEM_CHARGING, &data, true);
	if (rc >= 0)
		chip->ufcs_charging = !!data.intval;
	rc = oplus_mms_get_item_data(chip->ufcs_topic, UFCS_ITEM_ADAPTER_ID, &data, true);
	if (rc >= 0)
		chip->ufcs_adapter_id = (u32)data.intval;
	rc = oplus_mms_get_item_data(chip->ufcs_topic, UFCS_ITEM_OPLUS_ADAPTER, &data, true);
	if (rc >= 0)
		chip->ufcs_oplus_adapter = !!data.intval;
}

#define OPLUS_CHG_IC_USB_PLUGOUT_DELAY 5000
static void oplus_monitor_wired_plugin_work(struct work_struct *work)
{
	struct oplus_monitor *chip =
		container_of(work, struct oplus_monitor, wired_plugin_work);
	oplus_chg_track_check_wired_charging_break(chip->wired_online);

	if (chip->liquid_inlet_detection_switch) {
		chg_info("wired_online:%d", chip->wired_online);
		if(chip->wired_online == 1) {
			chip->water_inlet_plugin_count += 1;
			cancel_delayed_work_sync(&chip->water_inlet_detect_work);
		} else {
			oplus_chg_water_inlet_detect(TRACK_CMD_LIQUID_INTAKE, chip->water_inlet_plugin_count);
			schedule_delayed_work(&chip->water_inlet_detect_work,
			msecs_to_jiffies(OPLUS_CHG_IC_USB_PLUGOUT_DELAY));
		}
	}
}

static void oplus_mms_water_inlet_detect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_monitor *chip =
		container_of(dwork, struct oplus_monitor, water_inlet_detect_work);

	chip->water_inlet_plugin_count = 0;
	oplus_chg_water_inlet_detect(TRACK_CMD_CLEAR_TIMER, 0);
	chg_err("water_inlet_detect_work plugout timeout!\n");
}

static void oplus_monitor_wired_subs_callback(struct mms_subscribe *subs,
					  enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_online = !!data.intval;
			chip->notify_flag = 0;
			if (!chip->wired_online)
				oplus_chg_track_record_dual_chan_end(chip);
			schedule_work(&chip->charge_info_update_work);
			schedule_work(&chip->wired_plugin_work);
			break;
		case WIRED_ITEM_ERR_CODE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_err_code = (unsigned int)data.intval;
			break;
		case WIRED_ITEM_CHG_TYPE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_charge_type = data.intval;
			oplus_chg_track_handle_wired_type_info(chip, TRACK_CHG_GET_THTS_TIME_TYPE);
			break;
		case WIRED_ITEM_CC_MODE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->cc_mode = data.intval;
			break;
		case WIRED_ITEM_CC_DETECT:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->cc_detect = data.intval;
			break;
		case WIRED_ITEM_OTG_ENABLE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->otg_enable = !!data.intval;
			break;
		case WIRED_TIME_ABNORMAL_ADAPTER:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->pd_svooc = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_wired_topic(struct oplus_mms *topic,
					    void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_monitor_wired_subs_callback, "monitor");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = !!data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ERR_CODE, &data,
				true);
	chip->wired_err_code = (unsigned int)data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_CHG_TYPE, &data,
				true);
	chip->wired_charge_type = data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_CC_MODE, &data,
				true);
	chip->cc_mode = data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_CC_DETECT, &data,
				true);
	chip->cc_detect = data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_OTG_ENABLE, &data,
				true);
	chip->otg_enable = !!data.intval;

	oplus_mms_get_item_data(chip->wired_topic, WIRED_TIME_ABNORMAL_ADAPTER, &data,
				true);
	chip->pd_svooc = !!data.intval;
}

static void oplus_monitor_wls_subs_callback(struct mms_subscribe *subs,
					enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WLS_ITEM_PRESENT:
			oplus_mms_get_item_data(chip->wls_topic, id, &data, false);
			chip->wls_online = !!data.intval;
			schedule_work(&chip->charge_info_update_work);
			oplus_chg_track_check_wls_charging_break(!!data.intval);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_wls_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wls_topic = topic;
	chip->wls_subs = oplus_mms_subscribe(chip->wls_topic, chip, oplus_monitor_wls_subs_callback, "monitor");
	if (IS_ERR_OR_NULL(chip->wls_subs)) {
		chg_err("subscribe wls topic error, rc=%ld\n", PTR_ERR(chip->wls_subs));
		return;
	}
	oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_PRESENT, &data, true);
	chip->wls_online = !!data.intval;
	if (chip->wls_online)
		schedule_work(&chip->charge_info_update_work);
}

static void oplus_monitor_record_ffc_soc(struct oplus_monitor *chip, bool start)
{
	union mms_msg_data data = { 0 };
	int main_soc = -1, sub_soc = -1;

	if (is_main_gauge_topic_available(chip)) {
		oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_SOC,
					&data, false);
		main_soc = data.intval;
	}
	if (is_sub_gauge_topic_available(chip)) {
		oplus_mms_get_item_data(chip->sub_gauge_topic, GAUGE_ITEM_SOC,
					&data, false);
		sub_soc = data.intval;
	}
	oplus_chg_track_record_ffc_soc(chip, main_soc, sub_soc, start);
}

static void oplus_monitor_ffc_step_change_work(struct work_struct *work)
{
	struct oplus_monitor *chip =
		container_of(work, struct oplus_monitor, ffc_step_change_work);
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_FFC_STEP, &data, false);
	if (rc < 0) {
		chg_err("can't get ffc step, rc=%d\n", rc);
		return;
	}

	if (chip->ffc_status == FFC_FAST) {
		oplus_chg_track_aging_ffc_check(chip, data.intval);
		oplus_monitor_record_ffc_soc(chip, true);
	}
}

static void oplus_monitor_ffc_end_work(struct work_struct *work)
{
	struct oplus_monitor *chip =
		container_of(work, struct oplus_monitor, ffc_end_work);

	oplus_chg_track_record_ffc_end_time(chip);
	oplus_monitor_record_ffc_soc(chip, false);
}

static void oplus_monitor_comm_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int pre_batt_status = POWER_SUPPLY_STATUS_UNKNOWN;

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_TEMP_REGION:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->temp_region = data.intval;
			oplus_chg_track_cal_tbatt_status(chip);
			break;
		case COMM_ITEM_FFC_STATUS:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			if (chip->ffc_status == FFC_FAST && data.intval == FFC_DEFAULT)
				schedule_work(&chip->ffc_end_work);
			chip->ffc_status = (unsigned int)data.intval;
			if (chip->ffc_status == FFC_FAST)
				schedule_work(&chip->ffc_step_change_work);
			break;
		case COMM_ITEM_COOL_DOWN:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->cool_down = data.intval;
			break;
		case COMM_ITEM_UI_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->ui_soc = data.intval;
			if (chip->ui_soc < 0) {
				chg_err("ui soc not ready, rc=%d\n",
					chip->ui_soc);
				chip->ui_soc = 0;
				chip->ui_soc_ready = false;
			} else {
				chip->ui_soc_ready = true;
				if (chip->ui_soc == 1)
					oplus_chg_track_set_uisoc_1_start(chip);
			}
			break;
		case COMM_ITEM_SHUTDOWN_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->soc_load = data.intval;
			break;
		case COMM_ITEM_SMOOTH_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->smooth_soc = data.intval;
			break;
		case COMM_ITEM_NOTIFY_CODE:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->notify_code = (unsigned int)data.intval;
			break;
		case COMM_ITEM_NOTIFY_FLAG:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->notify_flag = (unsigned int)data.intval;
			break;
		case COMM_ITEM_SHELL_TEMP:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->shell_temp = data.intval;
			break;
		case COMM_ITEM_LED_ON:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->led_on = !!data.intval;
			break;
		case COMM_ITEM_BATT_STATUS:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_status = data.intval;
			if (chip->batt_status != pre_batt_status &&
			    chip->batt_status == POWER_SUPPLY_STATUS_FULL)
				oplus_chg_track_charge_full(chip);
			pre_batt_status = chip->batt_status;
			break;
		case COMM_ITEM_CHG_FULL:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_full = !!data.intval;
			break;
		case COMM_ITEM_RECHGING:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->rechging = !!data.intval;
			break;
		case COMM_ITEM_FFC_STEP:
			schedule_work(&chip->ffc_step_change_work);
			break;
		case COMM_ITEM_CHG_CYCLE_STATUS:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->chg_cycle_status = data.intval;
			break;
		case COMM_ITEM_SLOW_CHG:
			oplus_mms_get_item_data(chip->comm_topic, id, &data, false);
			chip->slow_chg_pct = SLOW_CHG_TO_PCT(data.intval);
			chip->slow_chg_watt = SLOW_CHG_TO_WATT(data.intval);
			chip->slow_chg_enable = !!SLOW_CHG_TO_ENABLE(data.intval);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_comm_topic(struct oplus_mms *topic,
					   void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->comm_topic = topic;
	chip->comm_subs =
		oplus_mms_subscribe(chip->comm_topic, chip,
				    oplus_monitor_comm_subs_callback, "monitor");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_TEMP_REGION, &data,
				true);
	chip->temp_region = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_FFC_STATUS, &data,
				true);
	chip->ffc_status = (unsigned int)data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_COOL_DOWN, &data,
				true);
	chip->cool_down = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_UI_SOC, &data,
				true);
	chip->ui_soc = data.intval;
	if (chip->ui_soc < 0) {
		chg_err("ui soc not ready, rc=%d\n", chip->ui_soc);
		chip->ui_soc = 0;
		chip->ui_soc_ready = false;
	} else {
		chip->ui_soc_ready = true;
		if (chip->ui_soc == 1)
			oplus_chg_track_set_uisoc_1_start(chip);
	}
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SMOOTH_SOC, &data,
				true);
	chip->smooth_soc = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_NOTIFY_CODE, &data,
				true);
	chip->notify_code = (unsigned int)data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_NOTIFY_FLAG, &data,
				true);
	chip->notify_flag = (unsigned int)data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data,
				true);
	chip->shell_temp = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_LED_ON, &data,
				true);
	chip->led_on = !!data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_RECHGING, &data,
				true);
	chip->rechging = !!data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_CHG_CYCLE_STATUS, &data,
				true);
	chip->chg_cycle_status = data.intval;

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SLOW_CHG, &data, true);
	chip->slow_chg_pct = SLOW_CHG_TO_PCT(data.intval);
	chip->slow_chg_watt = SLOW_CHG_TO_WATT(data.intval);
	chip->slow_chg_enable = !!SLOW_CHG_TO_ENABLE(data.intval);
}

static void oplus_monitor_vooc_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_monitor *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_online = !!data.intval;
			if (chip->vooc_online)
				chip->pre_vooc_sid = 0;
			break;
		case VOOC_ITEM_VOOC_STARTED:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_started = !!data.intval;
			break;
		case VOOC_ITEM_VOOC_CHARGING:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_charging = !!data.intval;
			break;
		case VOOC_ITEM_SID:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_sid = (unsigned int)data.intval;
			if (chip->vooc_sid != 0)
				chip->pre_vooc_sid = chip->vooc_sid;
			break;
		case VOOC_ITEM_ONLINE_KEEP:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_online_keep = !!data.intval;
			break;
		case VOOC_ITEM_BREAK_CODE:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			oplus_chg_track_set_fastchg_break_code(data.intval);
			break;
		case VOOC_ITEM_VOOC_BY_NORMAL_PATH:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			if (!!data.intval)
				chip->chg_ctrl_by_vooc = true;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_monitor_subscribe_vooc_topic(struct oplus_mms *topic,
					   void *prv_data)
{
	struct oplus_monitor *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->vooc_topic = topic;
	chip->vooc_subs =
		oplus_mms_subscribe(chip->vooc_topic, chip,
				    oplus_monitor_vooc_subs_callback, "monitor");
	if (IS_ERR_OR_NULL(chip->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(chip->vooc_subs));
		return;
	}

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_ONLINE, &data,
				true);
	chip->vooc_online = !!data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_STARTED, &data,
				true);
	chip->vooc_started = !!data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_CHARGING,
				&data, true);
	chip->vooc_charging = !!data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_SID, &data, true);
	chip->vooc_sid = (unsigned int)data.intval;
	chip->pre_vooc_sid = chip->vooc_sid;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_ONLINE_KEEP, &data,
				true);
	chip->vooc_online_keep = !!data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_BY_NORMAL_PATH,
				&data, true);
	if (!!data.intval)
		chip->chg_ctrl_by_vooc = true;
}

static void oplus_monitor_update(struct oplus_mms *mms, bool publish)
{
	/* TODO Active exception check */
}

static struct mms_item oplus_monitor_item[] = {
	{
		.desc = {
			.item_id = ERR_ITEM_IC,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_USBTEMP,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_VBAT_TOO_LOW,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_VBAT_DIFF_OVER,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_UI_SOC_SHUTDOWN,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_DUAL_CHAN,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_MMI_CHG,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_SLOW_CHG,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_CHG_CYCLE,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_WLS_INFO,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
	{
		.desc = {
			.item_id = ERR_ITEM_UFCS,
			.str_data = true,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	},
};

static const struct oplus_mms_desc oplus_monitor_desc = {
	.name = "error",
	.type = OPLUS_MMS_TYPE_ERROR,
	.item_table = oplus_monitor_item,
	.item_num = ARRAY_SIZE(oplus_monitor_item),
	.update_items = NULL,
	.update_items_num = 0,
	.update_interval = 60000, /* 1 min */
	.update = oplus_monitor_update,
};

static int oplus_monitor_topic_init(struct oplus_monitor *chip)
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

	chip->err_topic = devm_oplus_mms_register(chip->dev, &oplus_monitor_desc, &mms_cfg);
	if (IS_ERR(chip->err_topic)) {
		chg_err("Couldn't register error topic\n");
		rc = PTR_ERR(chip->err_topic);
		return rc;
	}

	oplus_mms_stop_publish(chip->err_topic);

	return 0;
}

static int oplus_monitor_probe(struct platform_device *pdev)
{
	struct oplus_monitor *chip;
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_monitor),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	of_platform_populate(chip->dev->of_node, NULL, NULL, chip->dev);

	chip->liquid_inlet_detection_switch = of_property_read_bool(chip->dev->of_node,
		"oplus,chg_into_liquid");
	chg_info("liquid_inlet_detection_switch = %d",
		chip->liquid_inlet_detection_switch);

	rc = oplus_monitor_topic_init(chip);
	if (rc < 0)
		goto topic_init_err;
	rc = oplus_chg_track_driver_init(chip);
	if (rc < 0) {
		chg_err("track driver init error, rc=%d\n", rc);
		goto track_init_err;
	}
	battlog_comm_ops.dev_data = (void *)chip;
	battery_log_ops_register(&battlog_comm_ops);

	chip->water_inlet_plugin_count = 0;
	INIT_WORK(&chip->charge_info_update_work,
		  oplus_monitor_charge_info_update_work);
	INIT_WORK(&chip->wired_plugin_work, oplus_monitor_wired_plugin_work);
	INIT_WORK(&chip->ffc_step_change_work, oplus_monitor_ffc_step_change_work);
	INIT_WORK(&chip->ffc_end_work, oplus_monitor_ffc_end_work);
	INIT_DELAYED_WORK(&chip->water_inlet_detect_work, oplus_mms_water_inlet_detect_work);

	oplus_mms_wait_topic("wired", oplus_monitor_subscribe_wired_topic, chip);
	oplus_mms_wait_topic("wireless", oplus_monitor_subscribe_wls_topic, chip);
	oplus_mms_wait_topic("vooc", oplus_monitor_subscribe_vooc_topic, chip);
	oplus_mms_wait_topic("common", oplus_monitor_subscribe_comm_topic, chip);
	oplus_mms_wait_topic("dual_chan", oplus_monitor_subscribe_dual_chan_topic, chip);
	oplus_mms_wait_topic("gauge", oplus_monitor_subscribe_gauge_topic, chip);
	oplus_mms_wait_topic("ufcs", oplus_monitor_subscribe_ufcs_topic, chip);

	chg_info("probe success\n");
	return 0;

track_init_err:
topic_init_err:
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, chip);

	chg_info("probe error, rc=%d\n", rc);
	return rc;
}

static int oplus_monitor_remove(struct platform_device *pdev)
{
	struct oplus_monitor *chip = platform_get_drvdata(pdev);

	if (!IS_ERR_OR_NULL(chip->gauge_subs))
		oplus_mms_unsubscribe(chip->gauge_subs);
	if (!IS_ERR_OR_NULL(chip->comm_subs))
		oplus_mms_unsubscribe(chip->comm_subs);
	if (!IS_ERR_OR_NULL(chip->vooc_subs))
		oplus_mms_unsubscribe(chip->vooc_subs);
	if (!IS_ERR_OR_NULL(chip->wls_subs))
		oplus_mms_unsubscribe(chip->wls_subs);
	if (!IS_ERR_OR_NULL(chip->wired_subs))
		oplus_mms_unsubscribe(chip->wired_subs);
	if (!IS_ERR_OR_NULL(chip->ufcs_subs))
		oplus_mms_unsubscribe(chip->ufcs_subs);
	oplus_chg_track_driver_exit(chip);
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, chip);

	return 0;
}

static void oplus_monitor_shutdown(struct platform_device *pdev)
{
}

static const struct of_device_id oplus_monitor_match[] = {
	{ .compatible = "oplus,monitor" },
	{},
};

static struct platform_driver oplus_monitor_driver = {
	.driver		= {
		.name = "oplus-monitor",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_monitor_match),
	},
	.probe		= oplus_monitor_probe,
	.remove		= oplus_monitor_remove,
	.shutdown	= oplus_monitor_shutdown,
};

static __init int oplus_monitor_init(void)
{
	return platform_driver_register(&oplus_monitor_driver);
}

static __exit void oplus_monitor_exit(void)
{
	platform_driver_unregister(&oplus_monitor_driver);
}

oplus_chg_module_late_register(oplus_monitor);
