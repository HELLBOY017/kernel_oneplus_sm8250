// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[FS]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/configfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/nls.h>
#include <linux/kdev_t.h>

#include <oplus_chg.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_module.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_vooc.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_smart_chg.h>
#include <oplus_battery_log.h>
#include <oplus_parallel.h>
#include <oplus_chg_exception.h>
#include <oplus_chg_dual_chan.h>
#include <oplus_chg_ufcs.h>
#include <oplus_chg_pps.h>
#include <oplus_chg_monitor.h>
#include <oplus_chg_wls.h>
#include "monitor/oplus_chg_track.h"

struct oplus_configfs_device {
	struct class *oplus_chg_class;
	struct device *oplus_usb_dir;
	struct device *oplus_battery_dir;
	struct device *oplus_wireless_dir;
	struct device *oplus_common_dir;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *wls_subs;
	struct mms_subscribe *vooc_subs;
	struct mms_subscribe *comm_subs;
	struct oplus_mms *wired_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *wls_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *parallel_topic;
	struct oplus_mms *ufcs_topic;
	struct oplus_mms *pps_topic;
	struct oplus_mms *err_topic;
	struct mms_subscribe *ufcs_subs;
	struct mms_subscribe *pps_subs;

	struct work_struct gauge_update_work;

	struct votable *wired_icl_votable;
	struct votable *wired_fcc_votable;
	struct votable *chg_disable_votable;
	struct votable *chg_suspend_votable;
	struct votable *cool_down_votable;
	struct votable *vooc_curr_votable;
	struct votable *pd_boost_disable_votable;
	struct votable *pd_svooc_votable;
	struct votable *ufcs_curr_votable;
	struct votable *pps_curr_votable;

	bool batt_exist;
	int vbat_mv;
	int batt_temp;
	int ibat_ma;
	int soc;
	int batt_fcc;
	int batt_cc;
	int batt_rm;
	int batt_soh;
	int gauge_vbat;
	int debug_batt_cc;

	bool wired_online;
	int wired_type;

	bool wls_online;
	int wls_type;

	bool vooc_started;
	bool vooc_online;
	bool vooc_charging;
	bool vooc_online_keep;
	unsigned int vooc_sid;

	bool ship_mode;
	bool slow_chg_enable;
	int slow_chg_pct;
	int slow_chg_watt;
	unsigned int notify_code;

	int bcc_current;
	bool mos_test_result;

	bool ufcs_online;
	bool ufcs_charging;
	bool ufcs_oplus_adapter;
	u32 ufcs_adapter_id;

	bool pps_online;
	bool pps_charging;
	bool pps_oplus_adapter;
	u32 pps_adapter_id;
};

static struct oplus_configfs_device *g_cfg_dev;

__maybe_unused static bool
is_chg_disable_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->chg_disable_votable)
		chip->chg_disable_votable = find_votable("CHG_DISABLE");
	return !!chip->chg_disable_votable;
}

__maybe_unused static bool
is_chg_suspend_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->chg_suspend_votable)
		chip->chg_suspend_votable = find_votable("CHG_SUSPEND");
	return !!chip->chg_suspend_votable;
}

__maybe_unused static bool
is_cool_down_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->cool_down_votable)
		chip->cool_down_votable = find_votable("COOL_DOWN");
	return !!chip->cool_down_votable;
}

__maybe_unused static bool
is_vooc_curr_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->vooc_curr_votable)
		chip->vooc_curr_votable = find_votable("VOOC_CURR");
	return !!chip->vooc_curr_votable;
}

__maybe_unused static bool
is_ufcs_curr_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->ufcs_curr_votable)
		chip->ufcs_curr_votable = find_votable("UFCS_CURR");
	return !!chip->ufcs_curr_votable;
}

__maybe_unused static bool
is_pps_curr_votable_available(struct oplus_configfs_device *chip)
{
	if (!chip->pps_curr_votable)
		chip->pps_curr_votable = find_votable("PPS_CURR");
	return !!chip->pps_curr_votable;
}

static bool is_parallel_topic_available(struct oplus_configfs_device *chip)
{
	if (!chip->parallel_topic)
		chip->parallel_topic = oplus_mms_get_by_name("parallel");

	return !!chip->parallel_topic;
}

__maybe_unused static bool is_err_topic_available(struct oplus_configfs_device *chip)
{
	if (!chip->err_topic)
		chip->err_topic = oplus_mms_get_by_name("error");
	return !!chip->err_topic;
}

static int oplus_configfs_push_mmi_chg_info_msg(struct oplus_configfs_device *chip, int mmi_chg)
{
	struct mms_msg *msg;
	int rc;

	if (!is_err_topic_available(chip)) {
		chg_err("error topic not found\n");
		return -ENODEV;
	}

	msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_MMI_CHG, "$$mmi_chg@@%d", mmi_chg);
	if (msg == NULL) {
		chg_err("alloc mmi chg error msg error\n");
		return -ENOMEM;
	}

	rc = oplus_mms_publish_msg(chip->err_topic, msg);
	if (rc < 0) {
		chg_err("publish mmi chg error msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_configfs_push_slow_chg_info_msg(struct oplus_configfs_device *chip, int pct, int watt, int en)
{
	struct mms_msg *msg;
	int rc;

	if (!is_err_topic_available(chip)) {
		chg_err("error topic not found\n");
		return -ENODEV;
	}

	msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_SLOW_CHG,
				      "$$slow_chg_pct@@%d$$slow_chg_watt@@%d$$slow_chg_en@@%d", pct, watt, en);
	if (msg == NULL) {
		chg_err("alloc slow chg error msg error\n");
		return -ENOMEM;
	}

	rc = oplus_mms_publish_msg(chip->err_topic, msg);
	if (rc < 0) {
		chg_err("publish slow chg error msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int fast_chg_type_by_user = -1;
static ssize_t fast_chg_type_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int fast_chg_type = 0;
	bool pd_use_default = false;
	union mms_msg_data data = { 0 };
	int rc;

	if (!chip->pd_boost_disable_votable)
		chip->pd_boost_disable_votable = find_votable("PD_BOOST_DISABLE");

	if (!chip->pd_svooc_votable)
		chip->pd_svooc_votable = find_votable("PD_SVOOC");

	if (chip->vooc_sid == 0) {
		switch (chip->wired_type) {
		case OPLUS_CHG_USB_TYPE_QC2:
		case OPLUS_CHG_USB_TYPE_QC3:
			fast_chg_type = CHARGER_SUBTYPE_QC;
			break;
		case OPLUS_CHG_USB_TYPE_UFCS:
			fast_chg_type = CHARGER_SUBTYPE_UFCS;
			break;
		case OPLUS_CHG_USB_TYPE_PD_PPS:
			if (chip->pps_online) {
				fast_chg_type = CHARGER_SUBTYPE_PPS;
				break;
			}
			fallthrough;
		case OPLUS_CHG_USB_TYPE_PD:
		case OPLUS_CHG_USB_TYPE_PD_DRP:
			if (chip->pd_boost_disable_votable &&
			    is_client_vote_enabled(chip->pd_boost_disable_votable,
				   SVID_VOTER))
				pd_use_default = true;
			if (chip->pd_svooc_votable &&
			    !!get_effective_result(chip->pd_svooc_votable))
				pd_use_default = true;

			if (pd_use_default)
				fast_chg_type = CHARGER_SUBTYPE_DEFAULT;
			else
				fast_chg_type = CHARGER_SUBTYPE_PD;

			chg_info("pd_use_default [%s] fast_chg_type [%d]\n",
				pd_use_default == true ?"true":"false",
				fast_chg_type);

			break;
		default:
			break;
		}
	} else {
		if (sid_to_adapter_id(chip->vooc_sid) == 0) {
			if (sid_to_adapter_chg_type(chip->vooc_sid) ==
			    CHARGER_TYPE_VOOC)
				fast_chg_type = CHARGER_SUBTYPE_FASTCHG_VOOC;
			else if (sid_to_adapter_chg_type(chip->vooc_sid) ==
				 CHARGER_TYPE_SVOOC)
				fast_chg_type = CHARGER_SUBTYPE_FASTCHG_SVOOC;
		} else {
			fast_chg_type = sid_to_adapter_id(chip->vooc_sid);
		}
	}

	if (chip->wls_online) {
		rc = oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_WLS_TYPE, &data, true);
		if (rc < 0)
			fast_chg_type = CHARGER_SUBTYPE_DEFAULT;
		else if (data.intval == OPLUS_CHG_WLS_VOOC)
			fast_chg_type = CHARGER_SUBTYPE_FASTCHG_VOOC;
		else if (data.intval == OPLUS_CHG_WLS_SVOOC || data.intval == OPLUS_CHG_WLS_PD_65W)
			fast_chg_type = WLS_ADAPTER_TYPE_SVOOC;
		else
			fast_chg_type = CHARGER_SUBTYPE_DEFAULT;
	}

	if (fast_chg_type_by_user > 0)
		fast_chg_type = fast_chg_type_by_user;

	return sprintf(buf, "%d\n", fast_chg_type);
}

static ssize_t fast_chg_type_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	fast_chg_type_by_user = val;

	return count;
}
static DEVICE_ATTR_RW(fast_chg_type);

static ssize_t otg_online_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int otg_online = 0;

	otg_online = oplus_wired_get_otg_online_status(chip->wired_topic);

	return sprintf(buf, "%d\n", otg_online);
}
static DEVICE_ATTR_RO(otg_online);

static ssize_t otg_switch_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", oplus_wired_get_otg_switch_status());
}

static ssize_t otg_switch_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	rc = oplus_wired_set_otg_switch_status(val);
	if (rc < 0) {
		chg_err("can't %s otg switch\n", !!val ? "open" : "close");
		return rc;
	}

	return count;
}
static DEVICE_ATTR_RW(otg_switch);

static ssize_t usb_status_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_USB_STATUS, &data,
				true);

	return sprintf(buf, "%d\n", data.intval);
}
static DEVICE_ATTR_RO(usb_status);

static ssize_t usbtemp_volt_l_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_USB_TEMP_VOLT_L,
				&data, true);

	return sprintf(buf, "%d\n", data.intval);
}
static DEVICE_ATTR_RO(usbtemp_volt_l);

static ssize_t usbtemp_volt_r_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_USB_TEMP_VOLT_R,
				&data, true);

	return sprintf(buf, "%d\n", data.intval);
}
static DEVICE_ATTR_RO(usbtemp_volt_r);

static ssize_t typec_cc_orientation_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%d\n", oplus_wired_get_cc_orientation());
}
static DEVICE_ATTR_RO(typec_cc_orientation);

static struct device_attribute *oplus_usb_attributes[] = {
	&dev_attr_otg_online,
	&dev_attr_otg_switch,
	&dev_attr_usb_status,
	&dev_attr_typec_cc_orientation,
	&dev_attr_fast_chg_type,
	&dev_attr_usbtemp_volt_l,
	&dev_attr_usbtemp_volt_r,
	NULL
};

/**********************************************************************
* battery device nodes
**********************************************************************/
static ssize_t authenticate_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oplus_gauge_get_batt_authenticate());
}
static DEVICE_ATTR_RO(authenticate);

static ssize_t battery_cc_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->batt_cc);
}
static DEVICE_ATTR_RO(battery_cc);

static ssize_t battery_fcc_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->batt_fcc);
}
static DEVICE_ATTR_RO(battery_fcc);

static ssize_t battery_rm_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->batt_rm);
}
static DEVICE_ATTR_RO(battery_rm);

static ssize_t battery_soh_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->batt_soh);
}
static DEVICE_ATTR_RO(battery_soh);

static ssize_t design_capacity_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int val;

	val = oplus_gauge_get_batt_capacity_mah(chip->gauge_topic);

	return sprintf(buf, "%d\n", val);
}
static DEVICE_ATTR_RO(design_capacity);

static ssize_t smartchg_soh_support_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val;

	val = oplus_smart_chg_get_soh_support();
	if (val < 0)
		return val;

	return sprintf(buf, "%d\n", val);
}
static DEVICE_ATTR_RO(smartchg_soh_support);

#ifdef CONFIG_OPLUS_CALL_MODE_SUPPORT
static ssize_t call_mode_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->calling_on); */
	return 0;
}

static ssize_t call_mode_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(call_mode);
#endif /*CONFIG_OPLUS_CALL_MODE_SUPPORT*/

static ssize_t charge_technology_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%u\n", oplus_vooc_get_project(chip->vooc_topic));
}

static ssize_t charge_technology_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	oplus_vooc_set_project(chip->vooc_topic, (uint32_t)val);
	chg_info("val = %d, vooc_project = %d\n", val, oplus_vooc_get_project(chip->vooc_topic));

	return count;
}
static DEVICE_ATTR_RW(charge_technology);

#ifdef CONFIG_OPLUS_CHIP_SOC_NODE
static ssize_t chip_soc_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->soc);
}
static DEVICE_ATTR_RO(chip_soc);
#endif /*CONFIG_OPLUS_CHIP_SOC_NODE*/

static ssize_t gauge_vbat_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->gauge_vbat);
}
static DEVICE_ATTR_RO(gauge_vbat);

#ifdef CONFIG_OPLUS_SMART_CHARGER_SUPPORT
static ssize_t cool_down_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };
	int rc;

	if (chip->comm_topic)
		rc = oplus_mms_get_item_data(chip->comm_topic,
					     COMM_ITEM_COOL_DOWN, &data, false);
	else
		rc = -ENOTSUPP;
	if (rc < 0) {
		chg_err("can't get cool_down, rc=%d\n", rc);
		return rc;
	}

	return sprintf(buf, "%d\n", data.intval);
}

static ssize_t cool_down_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	if (is_cool_down_votable_available(chip))
		rc = vote(chip->cool_down_votable, USER_VOTER,
			  val > 0 ? true : false, val, false);
	else
		rc = -ENOTSUPP;
	if (rc < 0) {
		chg_err("can't set cool_down to %d, rc=%d\n", val, rc);
		return rc;
	}

	return count;
}
static DEVICE_ATTR_RW(cool_down);
#endif /*CONFIG_OPLUS_SMART_CHARGER_SUPPORT*/

static ssize_t fast_charge_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };
	bool fastchg;
	int rc;

	fastchg = chip->vooc_online || chip->vooc_online_keep;

	fastchg |= chip->ufcs_online && chip->ufcs_oplus_adapter &&
		   (oplus_ufcs_adapter_id_to_protocol_type(chip->ufcs_adapter_id) == PROTOCOL_CHARGING_UFCS_OPLUS);

	fastchg |= chip->pps_online && chip->pps_oplus_adapter &&
		   (oplus_pps_adapter_id_to_protocol_type(chip->pps_adapter_id) == PROTOCOL_CHARGING_PPS_OPLUS);

	if (chip->wls_online) {
		rc = oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_WLS_TYPE, &data, true);
		if (rc < 0)
			chg_err("can't get wls type, rc=%d\n", rc);
		else if (data.intval == OPLUS_CHG_WLS_SVOOC || data.intval == OPLUS_CHG_WLS_PD_65W)
			fastchg = true;
	}

	return sprintf(buf, "%d\n", fastchg);
}
static DEVICE_ATTR_RO(fast_charge);

static ssize_t mmi_charging_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int val;

	if (is_chg_disable_votable_available(chip))
		val = !get_client_vote(chip->chg_disable_votable, MMI_CHG_VOTER);
	else
		return -ENOTSUPP;

	return sprintf(buf, "%d\n", val);
}

static ssize_t mmi_charging_enable_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	oplus_configfs_push_mmi_chg_info_msg(chip, !!val);
	if (is_chg_disable_votable_available(chip))
		rc = vote(chip->chg_disable_votable, MMI_CHG_VOTER,
			  !val ? true : false, !val, false);
	else
		rc = -ENOTSUPP;
	if (rc < 0)
		chg_err("can't set charging %s, rc=%d\n",
		       !val ? "disable" : "enable", rc);
	else
		chg_info("mmi set charging %s\n", !val ? "disable" : "enable");

	return (rc < 0) ? rc : count;
}
static DEVICE_ATTR_RW(mmi_charging_enable);

static ssize_t battery_notify_code_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%u\n", chip->notify_code);
}
static DEVICE_ATTR_RO(battery_notify_code);

#ifdef CONFIG_OPLUS_SHIP_MODE_SUPPORT
static ssize_t ship_mode_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->ship_mode);
}

static ssize_t ship_mode_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	rc = oplus_wired_shipmode_enable(!!val);
	if (rc < 0) {
		chg_err("can't %s ship mode, rc=%d\n",
		       !!val ? "enable" : "disable", rc);
	} else {
		chg_err("%s charging\n", !!val ? "enable" : "disable");
		chip->ship_mode = !!val;
	}

	return (rc < 0) ? rc : count;
}
static DEVICE_ATTR_RW(ship_mode);
#endif /*CONFIG_OPLUS_SHIP_MODE_SUPPORT*/

#ifdef CONFIG_OPLUS_SHORT_C_BATT_CHECK
#ifdef CONFIG_OPLUS_SHORT_USERSPACE
static ssize_t short_c_limit_chg_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", (int)chip->short_c_batt.limit_chg); */
	return 0;
}

static ssize_t short_c_limit_chg_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(short_c_limit_chg);

static ssize_t short_c_limit_rechg_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", (int)chip->short_c_batt.limit_rechg); */
	return 0;
}

static ssize_t short_c_limit_rechg_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(short_c_limit_rechg);

static ssize_t charge_term_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->limits.iterm_ma); */
	return 0;
}
static DEVICE_ATTR_RO(charge_term_current);

static ssize_t input_current_settled_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", val);
}
static DEVICE_ATTR_RO(input_current_settled);
#endif /*CONFIG_OPLUS_SHORT_USERSPACE*/
#endif /*CONFIG_OPLUS_SHORT_C_BATT_CHECK*/

#ifdef CONFIG_OPLUS_SHORT_HW_CHECK
static ssize_t short_c_hw_feature_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->short_c_batt.is_feature_hw_on); */
	return 0;
}

static ssize_t short_c_hw_feature_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(short_c_hw_feature);

static ssize_t short_c_hw_status_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->short_c_batt.shortc_gpio_status); */
	return 0;
}
static DEVICE_ATTR_RO(short_c_hw_status);
#endif /*CONFIG_OPLUS_SHORT_HW_CHECK*/

#ifdef CONFIG_OPLUS_SHORT_IC_CHECK
static ssize_t short_ic_otp_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->short_c_batt.ic_short_otp_st); */
	return 0;
}
static DEVICE_ATTR_RO(short_ic_otp_status);

static ssize_t short_ic_volt_thresh_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", chip->short_c_batt.ic_volt_threshold); */
	return 0;
}

static ssize_t short_ic_volt_thresh_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int val = 0;
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(short_ic_volt_thresh);

static ssize_t short_ic_otp_value_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	/* return sprintf(buf, "%d\n", oplus_short_ic_get_otp_error_value(chip)); */
	return 0;
}
static DEVICE_ATTR_RO(short_ic_otp_value);
#endif /*CONFIG_OPLUS_SHORT_IC_CHECK*/

static ssize_t voocchg_ing_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	return sprintf(buf, "%d\n", chip->vooc_charging);
}
static DEVICE_ATTR_RO(voocchg_ing);

static ssize_t ppschg_ing_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int val = 0;

	if (chip->ufcs_online) {
		if (chip->ufcs_oplus_adapter)
			val = oplus_ufcs_adapter_id_to_protocol_type(chip->ufcs_adapter_id);
		else
			val = PROTOCOL_CHARGING_UFCS_THIRD;
	} else 	if (chip->pps_online) {
		if (chip->pps_oplus_adapter)
			val = oplus_pps_adapter_id_to_protocol_type(chip->pps_adapter_id);
		else
			val = PROTOCOL_CHARGING_PPS_THIRD;
	}

	return sprintf(buf, "%d\n", val);
}
static DEVICE_ATTR_RO(ppschg_ing);

static ssize_t ppschg_power_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int power = 0;

	if (chip->ufcs_online) {
		if (chip->ufcs_oplus_adapter)
			power = oplus_ufcs_adapter_id_to_power(chip->ufcs_adapter_id);
		else
			power = oplus_ufcs_adapter_id_to_power(UFCS_FASTCHG_TYPE_THIRD);
	} else 	if (chip->pps_online) {
		if (chip->pps_oplus_adapter)
			power = oplus_pps_adapter_id_to_power(chip->pps_adapter_id);
		else
			power = oplus_pps_adapter_id_to_power(PPS_FASTCHG_TYPE_THIRD);
	}

	return sprintf(buf, "%d\n", power);
}
static DEVICE_ATTR_RO(ppschg_power);

static ssize_t bcc_exception_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	chg_err("power off\n");

	return count;
}
static DEVICE_ATTR_WO(bcc_exception);

int __attribute__((weak)) oplus_gauge_get_bcc_parameters(char *buf)
{
	return 0;
}

int __attribute__((weak)) oplus_gauge_set_bcc_parameters(const char *buf)
{
	return 0;
}

#define VOOC_MCU_PROJECT 7
#define VOOC_MCU_PROJECT_100W	8
static ssize_t bcc_parms_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int val = 0;
	ssize_t len = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };
	int rc;
	bool fastchg_started = false;
	int vooc_project = 0;
	int voocphy_support = 0;
	int chg_type = oplus_wired_get_chg_type();

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (chip->vooc_topic) {
		rc = oplus_mms_get_item_data(chip->vooc_topic,
					     VOOC_ITEM_VOOC_STARTED, &data, true);
		vooc_project = oplus_vooc_get_project(chip->vooc_topic);
		voocphy_support = oplus_vooc_get_voocphy_support(chip->vooc_topic);
	} else {
		rc = -ENOTSUPP;
	}
	if (rc < 0) {
		chg_err("can't get vooc_started, rc=%d\n", rc);
	}

	fastchg_started = data.intval;

	/* I2C access conflict may occur when using MCU fast charging scheme.
	   only SVOOC charging need to get the bcc parameters.*/
	if ((vooc_project == VOOC_MCU_PROJECT || vooc_project == VOOC_MCU_PROJECT_100W)
		&& chg_type == OPLUS_CHG_USB_TYPE_SVOOC && voocphy_support != ADSP_VOOCPHY) {
		val = oplus_gauge_get_prev_bcc_parameters(buf);
		val = oplus_smart_chg_get_prev_battery_bcc_parameters(buf);
	} else {
		val = oplus_gauge_get_bcc_parameters(buf);
		val = oplus_smart_chg_get_battery_bcc_parameters(buf);
	}

	len = strlen(buf);
	chg_err("bcc_parms_show len: %ld\n", len);

	return len;
}

static ssize_t bcc_parms_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	ret = oplus_gauge_set_bcc_parameters(buf);
	ret = oplus_smart_chg_set_bcc_debug_parameters(buf);
	if (ret < 0) {
		chg_err("error\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(bcc_parms);

static ssize_t bcc_current_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	return sprintf(buf, "%d\n", chip->bcc_current);
}

static ssize_t bcc_current_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	if (is_vooc_curr_votable_available(chip)) {
		chg_err("bcc current = %d\n", val);
		/* turn current to mA, hidl will send as 73, it means 7300mA */
		val = val * 100;
		vote(chip->vooc_curr_votable, BCC_VOTER, (val == 0) ? false : true, val, false);
	}

	if (chip->ufcs_online && is_ufcs_curr_votable_available(chip)) {
		chg_info("ufcs bcc current = %d\n", val);
		val = val * 100; /* 0.1A to mA */
		vote(chip->ufcs_curr_votable, BCC_VOTER, (val == 0) ? false : true, val, false);
	}

	if (chip->pps_online && is_pps_curr_votable_available(chip)) {
		chg_info("pps bcc current = %d\n", val);
		val = val * 100; /* 0.1A to mA */
		vote(chip->pps_curr_votable, BCC_VOTER, (val == 0) ? false : true, val, false);
	}

	oplus_wired_set_bcc_curr_request(chip->wired_topic);

	/* oplus_wired_get_bcc_curr_done_status(); */

	return count;
}
static DEVICE_ATTR_RW(bcc_current);

static ssize_t normal_current_now_store(struct device *dev, struct device_attribute *attr, const char *buf,
					size_t count)
{
	int val = 0;
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	val = val & 0XFFFF;
	chg_info("val:%d\n", val);
	rc = oplus_smart_chg_set_normal_current(val);
	if (rc < 0)
		return rc;

	return count;
}
DEVICE_ATTR_WO(normal_current_now);

static ssize_t normal_cool_down_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	int rc;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}
	chg_info("val:%d\n", val);
	rc = oplus_smart_chg_set_normal_cool_down(val);
	if (rc < 0)
		return rc;

	return count;
}
DEVICE_ATTR_WO(normal_cool_down);

static ssize_t get_quick_mode_time_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	long rc;

	rc = oplus_smart_chg_get_quick_mode_time_gain();
	if (rc < 0)
		return rc;

	return sprintf(buf, "%ld\n", rc);
}
static DEVICE_ATTR_RO(get_quick_mode_time_gain);

static ssize_t get_quick_mode_percent_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc;

	rc = oplus_smart_chg_get_quick_mode_percent_gain();
	if (rc < 0)
		return rc;

	return sprintf(buf, "%d\n", rc);
}
static DEVICE_ATTR_RO(get_quick_mode_percent_gain);

static ssize_t aging_ffc_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int version, max_step;
	int ffc_pre_normal_fv_max, ffc_normal_fv_max;
	int index = 0;
	int i;

	version = oplus_comm_get_wired_aging_ffc_version(chip->comm_topic);
	max_step = oplus_comm_get_wired_ffc_step_max(chip->comm_topic);

	/* TODO: parallel charger */
	index += sprintf(buf, "%d,%d,%d,%d,%d", version, oplus_gauge_get_batt_num(),
			 0, chip->debug_batt_cc, chip->batt_cc);
	for (i = 0; i < max_step && i < FFC_CHG_STEP_MAX; i++) {
		ffc_pre_normal_fv_max =
			oplus_comm_get_wired_ffc_cutoff_fv(chip->comm_topic, i, FFC_TEMP_REGION_PRE_NORMAL) +
			oplus_comm_get_wired_aging_ffc_offset(chip->comm_topic, i);
		if (ffc_pre_normal_fv_max <= 0)
			break;
		ffc_normal_fv_max = oplus_comm_get_wired_ffc_cutoff_fv(chip->comm_topic, i, FFC_TEMP_REGION_NORMAL) +
				    oplus_comm_get_wired_aging_ffc_offset(chip->comm_topic, i);
		if (ffc_normal_fv_max <= 0)
			break;
		index += sprintf(buf + index, ",%d,%d", ffc_pre_normal_fv_max, ffc_normal_fv_max);
	}
	index += sprintf(buf + index, "\n");

	return index;
}

static ssize_t aging_ffc_data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	chip->debug_batt_cc = val;

	return count;
}
static DEVICE_ATTR_RW(aging_ffc_data);

static ssize_t battery_log_head_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (oplus_battery_log_support() != true) {
		return -ENODEV;
	}

	return battery_log_common_operate(BATTERY_LOG_DUMP_LOG_HEAD,
		buf, BATTERY_LOG_MAX_SIZE);
}
static DEVICE_ATTR_RO(battery_log_head);

static ssize_t battery_log_content_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (oplus_battery_log_support() != true) {
		return -ENODEV;
	}

	return battery_log_common_operate(BATTERY_LOG_DUMP_LOG_CONTENT,
		buf, BATTERY_LOG_MAX_SIZE);
}
static DEVICE_ATTR_RO(battery_log_content);

static ssize_t parallel_chg_mos_test_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };
	bool mos_status;
	int balancing_bat_status;
	int mos_test_result;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}
	if (!is_support_parallel_battery(chip->gauge_topic))
		return -ENODEV;

	if (is_parallel_topic_available(chip)) {
		oplus_mms_get_item_data(chip->parallel_topic, SWITCH_ITEM_HW_ENABLE_STATUS,
					&data, true);
		mos_status = data.intval;
		oplus_mms_get_item_data(chip->parallel_topic, SWITCH_ITEM_STATUS,
					&data, true);
		balancing_bat_status = data.intval;
	} else {
		chg_err("can't get parallel_topic\n");
		return -ENODEV;
	}


	if (!mos_status ||
	    balancing_bat_status == PARALLEL_BAT_BALANCE_ERROR_STATUS8 ||
	    balancing_bat_status == PARALLEL_BAT_BALANCE_ERROR_STATUS9) {
			chg_err("mos: %d, test next time!\n", mos_status);
			return 0;
	}
	mos_test_result = oplus_parallel_chg_mos_test(chip->parallel_topic);

	return sprintf(buf, "%d\n", mos_test_result);
}
static DEVICE_ATTR_RO(parallel_chg_mos_test);

static ssize_t parallel_chg_mos_status_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}
	if (!is_support_parallel_battery(chip->gauge_topic))
		return -ENODEV;

	if (is_parallel_topic_available(chip)) {
		oplus_mms_get_item_data(chip->parallel_topic, SWITCH_ITEM_HW_ENABLE_STATUS,
					&data, true);
	} else {
		chg_err("can't get parallel_topic\n");
		return -ENODEV;
	}

	return sprintf(buf, "%d\n", data.intval);
}
static DEVICE_ATTR_RO(parallel_chg_mos_status);

static ssize_t bms_heat_temp_compensation_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int bms_heat_temp_compensation;

	bms_heat_temp_compensation = oplus_comm_get_bms_heat_temp_compensation(chip->comm_topic);
	chg_debug("get bms compensation = %d\n", bms_heat_temp_compensation);

	return sprintf(buf, "%d\n", bms_heat_temp_compensation);
}

static ssize_t bms_heat_temp_compensation_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	int bms_heat_temp_compensation;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	bms_heat_temp_compensation = val;
	chg_debug("set bms compensation = %d\n", bms_heat_temp_compensation);

	oplus_comm_set_bms_heat_temp_compensation(chip->comm_topic, val);

	return count;
}
static DEVICE_ATTR_RW(bms_heat_temp_compensation);

static ssize_t pkg_name_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	oplus_chg_set_app_info(buf);
	return count;
}
static DEVICE_ATTR_WO(pkg_name);

static ssize_t dual_chan_info_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int val = 0;
	ssize_t len = 0;

	val = oplus_get_dual_chan_info(buf);

	len = strlen(buf);
	chg_info("dual_chan_info_show len: %ld\n", len);

	return len;
}

static DEVICE_ATTR_RO(dual_chan_info);

static ssize_t slow_chg_en_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int slow_chg_batt_limit = 0;
	union mms_msg_data data = { 0 };
	int rc;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (chip->vooc_online && chip->vooc_topic) {
		rc = oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_SLOW_CHG_BATT_LIMIT, &data, true);
		if (rc == 0)
			slow_chg_batt_limit = data.intval;
	}

	return sprintf(buf, "%d,%d,%d,%d\n", chip->slow_chg_pct, chip->slow_chg_watt, chip->slow_chg_enable,
		       slow_chg_batt_limit);
}

static ssize_t slow_chg_en_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int pct = 0, watt = 0, en = 0;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (!buf) {
		chg_err("buf is NULL\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%d,%d,%d", &pct, &watt, &en) != 3) {
		chg_err("invalid buff %s\n", buf);
		return -EINVAL;
	}

	if (pct <= 0 || pct > 100 || watt <= 0 || watt >= 0xffff) {
		chg_err("pct %d or watt %d invalid\n", pct, watt);
		return -EINVAL;
	}

	oplus_configfs_push_slow_chg_info_msg(chip, pct, watt, (bool)!!en);
	oplus_comm_set_slow_chg(chip->comm_topic, pct, watt, (bool)!!en);
	chg_info("%d %d %d\n", pct, watt, en);
	return count;
}
static DEVICE_ATTR_RW(slow_chg_en);

static struct device_attribute *oplus_battery_attributes[] = {
	&dev_attr_authenticate,
	&dev_attr_battery_cc,
	&dev_attr_battery_fcc,
	&dev_attr_battery_rm,
	&dev_attr_battery_soh,
#ifdef CONFIG_OPLUS_CALL_MODE_SUPPORT
	&dev_attr_call_mode,
#endif
	&dev_attr_charge_technology,
#ifdef CONFIG_OPLUS_CHIP_SOC_NODE
	&dev_attr_chip_soc,
#endif
	&dev_attr_gauge_vbat,
#ifdef CONFIG_OPLUS_SMART_CHARGER_SUPPORT
	&dev_attr_cool_down,
#endif
	&dev_attr_fast_charge,
	&dev_attr_mmi_charging_enable,
	&dev_attr_battery_notify_code,
#ifdef CONFIG_OPLUS_SHIP_MODE_SUPPORT
	&dev_attr_ship_mode,
#endif
#ifdef CONFIG_OPLUS_SHORT_C_BATT_CHECK
#ifdef CONFIG_OPLUS_SHORT_USERSPACE
	&dev_attr_short_c_limit_chg,
	&dev_attr_short_c_limit_rechg,
	&dev_attr_charge_term_current,
	&dev_attr_input_current_settled,
#endif
#endif
#ifdef CONFIG_OPLUS_SHORT_HW_CHECK
	&dev_attr_short_c_hw_feature,
	&dev_attr_short_c_hw_status,
#endif
#ifdef CONFIG_OPLUS_SHORT_IC_CHECK
	&dev_attr_short_ic_otp_status,
	&dev_attr_short_ic_volt_thresh,
	&dev_attr_short_ic_otp_value,
#endif
	&dev_attr_voocchg_ing,
	&dev_attr_ppschg_ing,
	&dev_attr_ppschg_power,
	&dev_attr_bcc_parms,
	&dev_attr_bcc_current,
	&dev_attr_bcc_exception,
	&dev_attr_normal_cool_down,
	&dev_attr_normal_current_now,
	&dev_attr_get_quick_mode_time_gain,
	&dev_attr_get_quick_mode_percent_gain,
	&dev_attr_aging_ffc_data,
	&dev_attr_design_capacity,
	&dev_attr_smartchg_soh_support,
	&dev_attr_battery_log_head,
	&dev_attr_battery_log_content,
	&dev_attr_parallel_chg_mos_test,
	&dev_attr_parallel_chg_mos_status,
	&dev_attr_bms_heat_temp_compensation,
	&dev_attr_pkg_name,
	&dev_attr_dual_chan_info,
	&dev_attr_slow_chg_en,
	NULL
};

/**********************************************************************
* wireless device nodes
**********************************************************************/
static ssize_t tx_voltage_now_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(tx_voltage_now);

static ssize_t tx_current_now_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(tx_current_now);

static ssize_t cp_voltage_now_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(cp_voltage_now);

static ssize_t cp_current_now_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(cp_current_now);

static ssize_t wireless_mode_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(wireless_mode);

static ssize_t wireless_type_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(wireless_type);

static ssize_t cep_info_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(cep_info);

static ssize_t real_type_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	union mms_msg_data data = { 0 };
	enum power_supply_type real_type = POWER_SUPPLY_TYPE_UNKNOWN;
	int rc;

	rc = oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_REAL_TYPE, &data, true);
	if (rc < 0)
		chg_err("can't get real type, rc=%d\n", rc);
	else
		real_type = data.intval;

	return sprintf(buf, "%d\n", real_type);
}
static DEVICE_ATTR_RO(real_type);

static ssize_t status_keep_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status_keep = 0;
	struct oplus_configfs_device *chip = NULL;
	union mms_msg_data data = { 0 };
	int rc;

	if (!dev || !buf) {
		chg_err("dev or buf is NULL\n");
		return -EINVAL;
	}

	chip = dev->driver_data;
	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}
	rc = oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
	if (rc < 0)
		chg_err("can't get status_keep, rc=%d\n", rc);
	else
		status_keep = data.intval;
	return sprintf(buf, "%d\n", status_keep);
}

static ssize_t status_keep_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	struct oplus_configfs_device *chip = NULL;
	union mms_msg_data data = { 0 };
	int rc;

	if (!dev || !buf) {
		chg_err("dev or buf is NULL\n");
		return -EINVAL;
	}

	chip = dev->driver_data;
	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	chg_info("set wls_status_keep=%d\n", val);
	if (val == WLS_SK_BY_HAL) {
		rc = oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
		if (rc < 0) {
			chg_err("can't get status_keep, rc=%d\n", rc);
		} else {
			if (data.intval == WLS_SK_NULL) {
				val = WLS_SK_NULL;
				chg_info("force to set wls_status_keep=%d\n", val);
			}
		}
	}
	(void)oplus_chg_wls_set_status_keep(chip->wls_topic, val);

	return count;
}
static DEVICE_ATTR_RW(status_keep);

#ifdef WLS_QI_DEBUG
ssize_t __attribute__((weak))
oplus_chg_wls_upgrade_fw_show(struct oplus_mms *mms, char *buf)
{
	return 0;
}

ssize_t __attribute__((weak))
oplus_chg_wls_upgrade_fw_store(struct oplus_mms *mms, const char *buf, size_t count)
{
	return 0;
}

static ssize_t upgrade_firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	return oplus_chg_wls_upgrade_fw_show(chip->wls_topic, buf);
}

static ssize_t upgrade_firmware_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	count = oplus_chg_wls_upgrade_fw_store(chip->wls_topic, buf, count);

	return count;
}
static DEVICE_ATTR_RW(upgrade_firmware);
#endif /*WLS_QI_DEBUG*/

static struct device_attribute *oplus_wireless_attributes[] = {
	&dev_attr_tx_voltage_now,
	&dev_attr_tx_current_now,
	&dev_attr_cp_voltage_now,
	&dev_attr_cp_current_now,
	&dev_attr_wireless_mode,
	&dev_attr_wireless_type,
	&dev_attr_cep_info,
	&dev_attr_real_type,
	&dev_attr_status_keep,
#ifdef WLS_QI_DEBUG
	&dev_attr_upgrade_firmware,
#endif
	NULL
};

/**********************************************************************
* common device nodes
**********************************************************************/
static ssize_t common_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	/* struct oplus_configfs_device *chip = dev->driver_data; */

	return sprintf(buf, "%s\n", "common: hello world!");
}
static DEVICE_ATTR(common, S_IRUGO, common_show, NULL);

static ssize_t track_hidl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}


	oplus_chg_track_set_hidl_info(buf, count);

	return count;
}
static DEVICE_ATTR_WO(track_hidl);

static ssize_t boot_completed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oplus_comm_get_boot_completed());
}
static DEVICE_ATTR_RO(boot_completed);

static ssize_t chg_olc_config_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	oplus_chg_olc_config_set(buf);
	return count;
}

static ssize_t chg_olc_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;
	int len = 0;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	len = oplus_chg_olc_config_get(buf);
	return len;
}
static DEVICE_ATTR_RW(chg_olc_config);

static ssize_t time_zone_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	oplus_chg_track_time_zone_set(buf);
	return count;
}

static ssize_t time_zone_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	return oplus_chg_track_time_zone_get(buf);
}
static DEVICE_ATTR_RW(time_zone);

static ssize_t battlog_push_config_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	char buffer[2] = { 0 };
	int val = 0;
	struct oplus_configfs_device *chip = dev->driver_data;
	int rc = 0;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (count < 0 || count > sizeof(buffer) - 1) {
		chg_err("%s: count[%zu] -EFAULT.\n", __func__, count);
		return -EFAULT;
	}

	if (copy_from_user(buffer, buf, count)) {
		chg_err("%s:  error.\n", __func__);
		return -EFAULT;
	}
	buffer[count] = '\0';

	if (kstrtos32(buffer, 0, &val)) {
		chg_err("buffer error\n");
		return -EINVAL;
	}

	if (!!val) {
		rc = oplus_chg_batterylog_exception_push();
		if (rc < 0)
			chg_err("push batterylog failed, rc=%d\n", rc);
		else
			chg_info("push batterylog successed\n");
	}

	return (rc < 0) ? rc : count;
}
static DEVICE_ATTR_WO(battlog_push_config);

static ssize_t mutual_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	rc = oplus_chg_wls_send_mutual_cmd(chip->wls_topic, buf);

	return rc;
}

static ssize_t mutual_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_configfs_device *chip = dev->driver_data;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	oplus_chg_wls_response_mutual_cmd(chip->wls_topic, buf, count);

	return count;
}
static DEVICE_ATTR_RW(mutual_cmd);

static struct device_attribute *oplus_common_attributes[] = {
	&dev_attr_common,
	&dev_attr_boot_completed,
	&dev_attr_chg_olc_config,
	&dev_attr_track_hidl,
	&dev_attr_time_zone,
	&dev_attr_battlog_push_config,
	&dev_attr_mutual_cmd,
	NULL
};

static int oplus_usb_dir_create(struct oplus_configfs_device *chip)
{
	int status = 0;
	dev_t devt;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	status = alloc_chrdev_region(&devt, 0, 1, "usb");
	if (status < 0) {
		chg_err("alloc_chrdev_region usb fail!\n");
		return -ENOMEM;
	}
	chip->oplus_usb_dir = device_create(chip->oplus_chg_class, NULL, devt,
					    NULL, "%s", "usb");
	chip->oplus_usb_dir->devt = devt;
	dev_set_drvdata(chip->oplus_usb_dir, chip);

	attrs = oplus_usb_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_usb_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_usb_dir->class,
				       chip->oplus_usb_dir->devt);
			return err;
		}
	}

	return 0;
}

static void oplus_usb_dir_destroy(struct device *dir_dev)
{
	struct device_attribute **attrs;
	struct device_attribute *attr;

	attrs = oplus_usb_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(dir_dev, attr);
	device_destroy(dir_dev->class, dir_dev->devt);
	unregister_chrdev_region(dir_dev->devt, 1);
}

static int oplus_battery_dir_create(struct oplus_configfs_device *chip)
{
	int status = 0;
	dev_t devt;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	status = alloc_chrdev_region(&devt, 0, 1, "battery");
	if (status < 0) {
		chg_err("alloc_chrdev_region battery fail!\n");
		return -ENOMEM;
	}

	chip->oplus_battery_dir = device_create(chip->oplus_chg_class, NULL,
						devt, NULL, "%s", "battery");
	chip->oplus_battery_dir->devt = devt;
	dev_set_drvdata(chip->oplus_battery_dir, chip);

	attrs = oplus_battery_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_battery_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_battery_dir->class,
				       chip->oplus_battery_dir->devt);
			return err;
		}
	}

	return 0;
}

static void oplus_battery_dir_destroy(struct device *dir_dev)
{
	struct device_attribute **attrs;
	struct device_attribute *attr;

	attrs = oplus_battery_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(dir_dev, attr);
	device_destroy(dir_dev->class, dir_dev->devt);
	unregister_chrdev_region(dir_dev->devt, 1);
}

static int oplus_wireless_dir_create(struct oplus_configfs_device *chip)
{
	int status = 0;
	dev_t devt;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	if (chip->oplus_wireless_dir) {
		chg_info("oplus_wireless_dir already exist!\n");
		return 0;
	}

	status = alloc_chrdev_region(&devt, 0, 1, "wireless");
	if (status < 0) {
		chg_err("alloc_chrdev_region wireless fail!\n");
		return -ENOMEM;
	}

	chip->oplus_wireless_dir = device_create(chip->oplus_chg_class, NULL,
						 devt, NULL, "%s", "wireless");
	chip->oplus_wireless_dir->devt = devt;
	dev_set_drvdata(chip->oplus_wireless_dir, chip);

	attrs = oplus_wireless_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_wireless_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_wireless_dir->class,
				       chip->oplus_wireless_dir->devt);
			return err;
		}
	}

	return 0;
}

static void oplus_wireless_dir_destroy(struct device *dir_dev)
{
	struct device_attribute **attrs;
	struct device_attribute *attr;

	attrs = oplus_wireless_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(dir_dev, attr);
	device_destroy(dir_dev->class, dir_dev->devt);
	unregister_chrdev_region(dir_dev->devt, 1);
}

static int oplus_common_dir_create(struct oplus_configfs_device *chip)
{
	int status = 0;
	dev_t devt;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (!chip) {
		chg_err("chip is NULL\n");
		return -EINVAL;
	}

	status = alloc_chrdev_region(&devt, 0, 1, "common");
	if (status < 0) {
		chg_err("alloc_chrdev_region common fail!\n");
		return -ENOMEM;
	}

	chip->oplus_common_dir = device_create(chip->oplus_chg_class, NULL,
					       devt, NULL, "%s", "common");
	chip->oplus_common_dir->devt = devt;
	dev_set_drvdata(chip->oplus_common_dir, chip);

	attrs = oplus_common_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_common_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_common_dir->class,
				       chip->oplus_common_dir->devt);
			return err;
		}
	}

	return 0;
}

static void oplus_common_dir_destroy(struct device *dir_dev)
{
	struct device_attribute **attrs;
	struct device_attribute *attr;

	attrs = oplus_common_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(dir_dev, attr);
	device_destroy(dir_dev->class, dir_dev->devt);
	unregister_chrdev_region(dir_dev->devt, 1);
}

/**********************************************************************
* configfs init APIs
**********************************************************************/
int oplus_usb_node_add(struct device_attribute **usb_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_usb_dir == NULL)
		return -ENODEV;

	attrs = usb_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_usb_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_usb_dir->class,
				       chip->oplus_usb_dir->devt);
			return err;
		}
	}

	return 0;
}

int oplus_usb_node_delete(struct device_attribute **usb_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_usb_dir == NULL)
		return -ENODEV;

	attrs = usb_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(chip->oplus_usb_dir, attr);

	return 0;
}

int oplus_battery_node_add(struct device_attribute **battery_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_battery_dir == NULL)
		return -ENODEV;

	attrs = battery_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_battery_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_battery_dir->class,
				       chip->oplus_battery_dir->devt);
			return err;
		}
	}

	return 0;
}

int oplus_battery_node_delete(struct device_attribute **battery_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_battery_dir == NULL)
		return -ENODEV;

	attrs = battery_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(chip->oplus_battery_dir, attr);

	return 0;
}

int oplus_wireless_node_add(struct device_attribute **wireless_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_wireless_dir == NULL)
		return -ENODEV;

	attrs = wireless_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_wireless_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_wireless_dir->class,
				       chip->oplus_wireless_dir->devt);
			return err;
		}
	}

	return 0;
}

int oplus_wireless_node_delete(struct device_attribute **wireless_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_wireless_dir == NULL)
		return -ENODEV;

	attrs = wireless_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(chip->oplus_wireless_dir, attr);

	return 0;
}

int oplus_common_node_add(struct device_attribute **common_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_common_dir == NULL)
		return -ENODEV;

	attrs = common_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs)) {
		int err;

		err = device_create_file(chip->oplus_common_dir, attr);
		if (err) {
			chg_err("device_create_file fail!\n");
			device_destroy(chip->oplus_common_dir->class,
				       chip->oplus_common_dir->devt);
			return err;
		}
	}

	return 0;
}

int oplus_common_node_delete(struct device_attribute **common_attributes)
{
	struct oplus_configfs_device *chip = g_cfg_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	if (chip == NULL)
		return -ENODEV;
	if (chip->oplus_common_dir == NULL)
		return -ENODEV;

	attrs = common_attributes;
	for (attr = *attrs; attr != NULL; attr = *(++attrs))
		device_remove_file(chip->oplus_common_dir, attr);

	return 0;
}

static void oplus_configfs_gauge_update_work(struct work_struct *work)
{
	struct oplus_configfs_device *chip = container_of(
		work, struct oplus_configfs_device, gauge_update_work);
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				false);
	chip->soc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CC, &data,
				false);
	chip->batt_cc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC, &data,
				false);
	chip->batt_fcc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
				false);
	chip->batt_rm = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOH, &data,
				false);
	chip->batt_soh = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_GAUGE_VBAT, &data,
				false);
	chip->gauge_vbat = data.intval;
}

static void oplus_configfs_gauge_subs_callback(struct mms_subscribe *subs,
					       enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_TIMER:
		schedule_work(&chip->gauge_update_work);
		break;
	default:
		break;
	}
}

static void oplus_configfs_subscribe_gauge_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_configfs_gauge_subs_callback,
				    "configfs");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				true);
	chip->soc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CC, &data,
				true);
	chip->batt_cc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC, &data,
				true);
	chip->batt_fcc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
				true);
	chip->batt_rm = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOH, &data,
				true);
	chip->batt_soh = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_GAUGE_VBAT, &data,
				true);
	chip->gauge_vbat = data.intval;

	rc = oplus_battery_dir_create(chip);
	if (rc < 0) {
		chg_err("oplus_battery_dir_create fail!, rc=%d\n", rc);
		goto dir_err;
	}

	return;

dir_err:
	chip->oplus_battery_dir = NULL;
	oplus_mms_unsubscribe(chip->gauge_subs);
}

static void oplus_configfs_wired_subs_callback(struct mms_subscribe *subs,
					       enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_online = data.intval;
			break;
		case WIRED_ITEM_CHG_TYPE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_type = data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_configfs_subscribe_wired_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	/* union mms_msg_data data = { 0 }; */
	int rc;

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_configfs_wired_subs_callback,
				    "configfs");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	rc = oplus_usb_dir_create(chip);
	if (rc < 0) {
		chg_err("oplus_usb_dir_create fail!, rc=%d\n", rc);
		goto dir_err;
	}

	return;

dir_err:
	chip->oplus_usb_dir = NULL;
	oplus_mms_unsubscribe(chip->wired_subs);
}

static void oplus_configfs_wls_subs_callback(struct mms_subscribe *subs,
					     enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WLS_ITEM_PRESENT:
			oplus_mms_get_item_data(chip->wls_topic, id, &data, false);
			chip->wls_online = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_configfs_subscribe_wls_topic(struct oplus_mms *topic,
					       void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->wls_topic = topic;
	chip->wls_subs = oplus_mms_subscribe(chip->wls_topic, chip,
					     oplus_configfs_wls_subs_callback,
					     "configfs");
	if (IS_ERR_OR_NULL(chip->wls_subs)) {
		chg_err("subscribe wls topic error, rc=%ld\n", PTR_ERR(chip->wls_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_PRESENT, &data, true);
	chip->wls_online = !!data.intval;

	rc = oplus_wireless_dir_create(chip);
	if (rc < 0) {
		chg_err("oplus_wireless_dir_create fail!, rc=%d\n", rc);
		goto dir_err;
	}

	return;

dir_err:
	chip->oplus_wireless_dir = NULL;
	oplus_mms_unsubscribe(chip->wls_subs);
}

static void oplus_configfs_vooc_subs_callback(struct mms_subscribe *subs,
					      enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_VOOC_STARTED:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_started = data.intval;
			break;
		case VOOC_ITEM_VOOC_CHARGING:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_charging = data.intval;
			break;
		case VOOC_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_online = data.intval;
			break;
		case VOOC_ITEM_SID:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_sid = (unsigned int)data.intval;
			break;
		case VOOC_ITEM_ONLINE_KEEP:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_online_keep = (unsigned int)data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_configfs_subscribe_vooc_topic(struct oplus_mms *topic,
						void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->vooc_topic = topic;
	chip->vooc_subs = oplus_mms_subscribe(chip->vooc_topic, chip,
					      oplus_configfs_vooc_subs_callback,
					      "configfs");
	if (IS_ERR_OR_NULL(chip->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(chip->vooc_subs));
		return;
	}

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_STARTED, &data,
				true);
	chip->vooc_started = data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_CHARGING,
				&data, true);
	chip->vooc_charging = data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_ONLINE, &data,
				true);
	chip->vooc_online = data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_ONLINE_KEEP, &data,
				true);
	chip->vooc_online_keep = data.intval;
	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_SID, &data, true);
	chip->vooc_sid = (unsigned int)data.intval;
}

static void oplus_configfs_comm_subs_callback(struct mms_subscribe *subs,
					      enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_TEMP_REGION:
			break;
		case COMM_ITEM_FCC_GEAR:
			break;
		case COMM_ITEM_NOTIFY_CODE:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->notify_code = data.intval;
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

static void oplus_configfs_subscribe_comm_topic(struct oplus_mms *topic,
						void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->comm_topic = topic;
	chip->comm_subs = oplus_mms_subscribe(chip->comm_topic, chip,
					      oplus_configfs_comm_subs_callback,
					      "configfs");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_NOTIFY_CODE, &data,
				true);
	chip->notify_code = data.intval;

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SLOW_CHG, &data, true);
	chip->slow_chg_pct = SLOW_CHG_TO_PCT(data.intval);
	chip->slow_chg_watt = SLOW_CHG_TO_WATT(data.intval);
	chip->slow_chg_enable = !!SLOW_CHG_TO_ENABLE(data.intval);

	rc = oplus_common_dir_create(chip);
	if (rc < 0) {
		chg_err("oplus_common_dir_create fail!, rc=%d\n", rc);
		goto dir_err;
	}

	return;

dir_err:
	chip->oplus_common_dir = NULL;
	oplus_mms_unsubscribe(chip->comm_subs);
}

static void oplus_configfs_ufcs_subs_callback(struct mms_subscribe *subs,
					     enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
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

static void oplus_configfs_subscribe_ufcs_topic(struct oplus_mms *topic,
					       void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->ufcs_topic = topic;
	chip->ufcs_subs =
		oplus_mms_subscribe(chip->ufcs_topic, chip,
				    oplus_configfs_ufcs_subs_callback,
				    "configfs");
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

static void oplus_configfs_pps_subs_callback(struct mms_subscribe *subs,
					     enum mms_msg_type type, u32 id)
{
	struct oplus_configfs_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int rc;

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case PPS_ITEM_ONLINE:
			rc = oplus_mms_get_item_data(chip->pps_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->pps_online = !!data.intval;
			break;
		case PPS_ITEM_CHARGING:
			rc = oplus_mms_get_item_data(chip->pps_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->pps_charging = !!data.intval;
			break;
		case PPS_ITEM_ADAPTER_ID:
			rc = oplus_mms_get_item_data(chip->pps_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->pps_adapter_id = (u32)data.intval;
			break;
		case PPS_ITEM_OPLUS_ADAPTER:
			rc = oplus_mms_get_item_data(chip->pps_topic, id, &data, false);
			if (rc < 0)
				break;
			chip->pps_oplus_adapter = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_configfs_subscribe_pps_topic(struct oplus_mms *topic,
					       void *prv_data)
{
	struct oplus_configfs_device *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->pps_topic = topic;
	chip->pps_subs =
		oplus_mms_subscribe(chip->pps_topic, chip,
				    oplus_configfs_pps_subs_callback,
				    "configfs");
	if (IS_ERR_OR_NULL(chip->pps_subs)) {
		chg_err("subscribe ufcs topic error, rc=%ld\n",
			PTR_ERR(chip->ufcs_subs));
		return;
	}

	rc = oplus_mms_get_item_data(chip->pps_topic, PPS_ITEM_ONLINE, &data, true);
	if (rc >= 0)
		chip->pps_online = !!data.intval;
	rc = oplus_mms_get_item_data(chip->pps_topic, PPS_ITEM_CHARGING, &data, true);
	if (rc >= 0)
		chip->pps_charging = !!data.intval;
	rc = oplus_mms_get_item_data(chip->pps_topic, PPS_ITEM_ADAPTER_ID, &data, true);
	if (rc >= 0)
		chip->pps_adapter_id = (u32)data.intval;
	rc = oplus_mms_get_item_data(chip->pps_topic, PPS_ITEM_OPLUS_ADAPTER, &data, true);
	if (rc >= 0)
		chip->pps_oplus_adapter = !!data.intval;
}

static __init int oplus_configfs_init(void)
{
	struct oplus_configfs_device *chip;
	int rc;

	chip = kzalloc(sizeof(struct oplus_configfs_device), GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	g_cfg_dev = chip;

	INIT_WORK(&chip->gauge_update_work, oplus_configfs_gauge_update_work);

	chip->oplus_chg_class = class_create(THIS_MODULE, "oplus_chg");
	if (IS_ERR(chip->oplus_chg_class)) {
		chg_err("oplus_chg_configfs_init fail!\n");
		rc = PTR_ERR(chip->oplus_chg_class);
		goto class_create_err;
	}

	/* TODO: need to delete the wireless dir create after the wireless is supported */
	rc = oplus_wireless_dir_create(chip);
	if (rc < 0) {
		chg_err("oplus_usb_dir_create fail!, rc=%d\n", rc);
		chip->oplus_wireless_dir = NULL;
	}

	oplus_mms_wait_topic("gauge", oplus_configfs_subscribe_gauge_topic,
			     chip);
	oplus_mms_wait_topic("wired", oplus_configfs_subscribe_wired_topic,
			     chip);
	oplus_mms_wait_topic("wireless", oplus_configfs_subscribe_wls_topic,
			     chip);
	oplus_mms_wait_topic("vooc", oplus_configfs_subscribe_vooc_topic, chip);
	oplus_mms_wait_topic("common", oplus_configfs_subscribe_comm_topic,
			     chip);
	oplus_mms_wait_topic("ufcs", oplus_configfs_subscribe_ufcs_topic, chip);
	oplus_mms_wait_topic("pps", oplus_configfs_subscribe_pps_topic, chip);

	return 0;

class_create_err:
	kfree(chip);
	return rc;
}

static __exit void oplus_configfs_exit(void)
{
	if (g_cfg_dev == NULL)
		return;

	if (g_cfg_dev->oplus_usb_dir)
		oplus_usb_dir_destroy(g_cfg_dev->oplus_usb_dir);
	if (g_cfg_dev->oplus_battery_dir)
		oplus_battery_dir_destroy(g_cfg_dev->oplus_battery_dir);
	if (g_cfg_dev->oplus_wireless_dir)
		oplus_wireless_dir_destroy(g_cfg_dev->oplus_wireless_dir);
	if (g_cfg_dev->oplus_common_dir)
		oplus_common_dir_destroy(g_cfg_dev->oplus_common_dir);
	if (!IS_ERR_OR_NULL(g_cfg_dev->wired_subs))
		oplus_mms_unsubscribe(g_cfg_dev->wired_subs);
	if (!IS_ERR_OR_NULL(g_cfg_dev->gauge_subs))
		oplus_mms_unsubscribe(g_cfg_dev->gauge_subs);
	if (!IS_ERR_OR_NULL(g_cfg_dev->wls_subs))
		oplus_mms_unsubscribe(g_cfg_dev->wls_subs);
	if (!IS_ERR_OR_NULL(g_cfg_dev->ufcs_subs))
		oplus_mms_unsubscribe(g_cfg_dev->ufcs_subs);
	if (!IS_ERR_OR_NULL(g_cfg_dev->pps_subs))
		oplus_mms_unsubscribe(g_cfg_dev->pps_subs);

	if (!IS_ERR(g_cfg_dev->oplus_chg_class))
		class_destroy(g_cfg_dev->oplus_chg_class);

	kfree(g_cfg_dev);
	g_cfg_dev = NULL;
}

oplus_chg_module_register(oplus_configfs);
