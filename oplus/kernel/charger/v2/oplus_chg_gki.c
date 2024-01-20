#define pr_fmt(fmt) "[GKI]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/power_supply.h>
#include <linux/sched/clock.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/oplus_project.h>
#endif
#include <oplus_chg.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_module.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_vooc.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_ufcs.h>
#include <oplus_chg_wls.h>

struct oplus_gki_device {
	struct device *dev;
	struct power_supply *usb_psy;
	struct power_supply *batt_psy;
	struct power_supply *wls_psy;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *wls_subs;
	struct mms_subscribe *comm_subs;
	struct mms_subscribe *vooc_subs;
	struct oplus_mms *wired_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *main_gauge_topic;
	struct oplus_mms *wls_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *ufcs_topic;
	struct mms_subscribe *ufcs_subs;

	struct work_struct gauge_update_work;

	struct votable *wired_icl_votable;
	struct votable *wired_fcc_votable;
	struct votable *fv_votable;
	struct votable *vooc_curr_votable;
	struct votable *ufcs_curr_votable;
	struct votable *pps_curr_votable;

	struct delayed_work status_keep_clean_work;
	struct delayed_work status_keep_delay_unlock_work;
	struct wakeup_source *status_wake_lock;
	bool status_wake_lock_on;

	bool led_on;

	bool batt_exist;
	bool batt_auth;
	bool batt_hmac;
	bool ui_soc_ready;
	int vbat_mv;
	int vbat_min_mv;
	int temperature;
	int soc;
	int batt_fcc;
	int batt_rm;
	int batt_status;
	int batt_health;
	int batt_chg_type;
	int ui_soc;
	int batt_capacity_mah;
	int time_to_full;

	bool wired_online;
	int wired_type;
	int vbus_mv;
	int charger_cycle;
	bool vooc_charging;
	bool vooc_started;

	bool wls_online;

	bool smart_charging_screenoff;
};

static struct oplus_gki_device *g_gki_dev;

__maybe_unused static bool
is_wired_icl_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->wired_icl_votable)
		chip->wired_icl_votable = find_votable("WIRED_ICL");
	return !!chip->wired_icl_votable;
}

__maybe_unused static bool
is_wired_fcc_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->wired_fcc_votable)
		chip->wired_fcc_votable = find_votable("WIRED_FCC");
	return !!chip->wired_fcc_votable;
}

__maybe_unused static bool
is_fv_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->fv_votable)
		chip->fv_votable = find_votable("FV_MAX");
	return !!chip->fv_votable;
}

__maybe_unused static bool
is_vooc_curr_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->vooc_curr_votable)
		chip->vooc_curr_votable = find_votable("VOOC_CURR");
	return !!chip->vooc_curr_votable;
}

__maybe_unused static bool
is_ufcs_curr_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->ufcs_curr_votable)
		chip->ufcs_curr_votable = find_votable("UFCS_CURR");
	return !!chip->ufcs_curr_votable;
}

__maybe_unused static bool
is_pps_curr_votable_available(struct oplus_gki_device *chip)
{
	if (!chip->pps_curr_votable)
		chip->pps_curr_votable = find_votable("PPS_CURR");
	return !!chip->pps_curr_votable;
}

static bool is_main_gauge_topic_available(struct oplus_gki_device *chip)
{
	if (!chip->main_gauge_topic)
		chip->main_gauge_topic = oplus_mms_get_by_name("gauge:0");

	return !!chip->main_gauge_topic;
}

#define KEEP_CLEAN_INTERVAL	2000
static int wls_psy_get_prop(struct power_supply *psy,
		enum power_supply_property prop,
		union power_supply_propval *pval)
{
	struct oplus_gki_device *chip = power_supply_get_drvdata(psy);
	union mms_msg_data data = { 0 };
	int rc = 0;
	int bms_temp_compensation;
	static bool pre_wls_online;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		pval->intval = chip->wls_online;
		oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
		if (data.intval != WLS_SK_NULL) {
			pval->intval = 1;
		} else {
			if (pre_wls_online && pval->intval == 0) {
				if (!chip->status_wake_lock_on) {
					chg_info("acquire status_wake_lock\n");
					__pm_stay_awake(chip->status_wake_lock);
					chip->status_wake_lock_on = true;
				}
				pre_wls_online = pval->intval;
				oplus_chg_wls_set_status_keep(chip->wls_topic, WLS_SK_BY_KERNEL);
				pval->intval = 1;
				schedule_delayed_work(&chip->status_keep_clean_work, msecs_to_jiffies(KEEP_CLEAN_INTERVAL));
			} else {
				pre_wls_online = pval->intval;
				if (chip->status_wake_lock_on) {
					cancel_delayed_work_sync(&chip->status_keep_clean_work);
					schedule_delayed_work(&chip->status_keep_clean_work, 0);
				}
			}
		}
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_VOUT, &data, true);
		pval->intval = data.intval;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_VOUT, &data, true);
		pval->intval = data.intval * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_IOUT, &data, true);
		pval->intval = data.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_IOUT, &data, true);
		pval->intval = data.intval * 1000;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if (is_support_parallel_battery(chip->gauge_topic) &&
		    is_main_gauge_topic_available(chip)) {
			rc = oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_TEMP, &data, false);
			pval->intval = data.intval;
		} else {
			bms_temp_compensation = oplus_comm_get_bms_heat_temp_compensation(chip->comm_topic);
			if (bms_temp_compensation != 0 && chip->temperature < OPLUS_BMS_HEAT_THRE)
				pval->intval = chip->temperature - bms_temp_compensation;
			else
				pval->intval = chip->temperature;
		}
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		pval->intval = chip->wls_online;
		break;
	default:
		chg_err("get prop %d is not supported\n", prop);
		return -EINVAL;
	}
	if (rc < 0) {
		chg_err("Couldn't get prop %d rc = %d\n", prop, rc);
		return -ENODATA;
	}

	return 0;
}

static int wls_psy_set_prop(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *pval)
{
	return 0;
}

static int wls_psy_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property prop)
{
	return 0;
}

static enum power_supply_property wls_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_PRESENT,
};

static const struct power_supply_desc wls_psy_desc = {
	.name			= "wireless",
	.type			= POWER_SUPPLY_TYPE_WIRELESS,
	.properties		= wls_props,
	.num_properties		= ARRAY_SIZE(wls_props),
	.get_property		= wls_psy_get_prop,
	.set_property		= wls_psy_set_prop,
	.property_is_writeable	= wls_psy_prop_is_writeable,
};


static int oplus_to_psy_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_ACA,
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_DRP,
	POWER_SUPPLY_USB_TYPE_PD_PPS,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_APPLE_BRICK_ID,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
};

static int oplus_to_power_supply_type[] = {
	POWER_SUPPLY_TYPE_UNKNOWN,
	POWER_SUPPLY_TYPE_USB,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_CDP,
	POWER_SUPPLY_TYPE_USB_ACA,
	POWER_SUPPLY_TYPE_USB_TYPE_C,
	POWER_SUPPLY_TYPE_USB_PD,
	POWER_SUPPLY_TYPE_USB_PD_DRP,
	POWER_SUPPLY_TYPE_USB_PD,
	POWER_SUPPLY_TYPE_USB,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
	POWER_SUPPLY_TYPE_USB_DCP,
};

static int usb_psy_get_prop(struct power_supply *psy,
		enum power_supply_property prop,
		union power_supply_propval *pval)
{
	struct oplus_gki_device *chip = power_supply_get_drvdata(psy);
	union mms_msg_data data = { 0 };
	int usb_temp_l, usb_temp_r;
	int rc = 0;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		pval->intval = chip->wired_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		pval->intval = oplus_wired_get_vbus() * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		pval->intval =
			oplus_wired_get_voltage_max(chip->wired_topic) * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pval->intval =  oplus_wired_get_ibus() * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (is_wired_icl_votable_available(chip))
			pval->intval = get_client_vote(chip->wired_icl_votable,
						       SPEC_VOTER) *
				       1000;
		else
			rc = -ENOTSUPP;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if (is_wired_icl_votable_available(chip))
			pval->intval =
				get_effective_result(chip->wired_icl_votable) *
				1000;
		else
			rc = -ENOTSUPP;
		break;
	case POWER_SUPPLY_PROP_USB_TYPE:
		pval->intval = oplus_to_psy_usb_type[chip->wired_type];
		break;
	case POWER_SUPPLY_PROP_TEMP:
		oplus_mms_get_item_data(chip->wired_topic,
					WIRED_ITEM_USB_TEMP_L, &data, true);
		usb_temp_l = data.intval;
		oplus_mms_get_item_data(chip->wired_topic,
					WIRED_ITEM_USB_TEMP_R, &data, true);
		usb_temp_r = data.intval;
		pval->intval = max(usb_temp_l, usb_temp_r);
		break;
	default:
		chg_err("get prop %d is not supported\n", prop);
		return -EINVAL;
	}
	if (rc < 0) {
		chg_err("Couldn't get prop %d rc = %d\n", prop, rc);
		return -ENODATA;
	}

	return 0;
}

static int usb_psy_set_prop(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *pval)
{
	int rc = 0;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		break;
	default:
		chg_err("set prop %d is not supported\n", prop);
		return -EINVAL;
	}

	return rc;
}

static int usb_psy_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return 1;
	default:
		break;
	}

	return 0;
}

static enum power_supply_property usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_usb_type usb_psy_supported_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_ACA,
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_DRP,
	POWER_SUPPLY_USB_TYPE_PD_PPS,
	POWER_SUPPLY_USB_TYPE_APPLE_BRICK_ID,
};

static struct power_supply_desc usb_psy_desc = {
	.name			= "usb",
	.type			= POWER_SUPPLY_TYPE_USB,
	.properties		= usb_props,
	.num_properties		= ARRAY_SIZE(usb_props),
	.get_property		= usb_psy_get_prop,
	.set_property		= usb_psy_set_prop,
	.usb_types		= usb_psy_supported_types,
	.num_usb_types		= ARRAY_SIZE(usb_psy_supported_types),
	.property_is_writeable	= usb_psy_prop_is_writeable,
};

static int oplus_gki_get_ui_soc(struct oplus_gki_device *chip)
{
	union mms_msg_data data = { 0 };
	int ui_soc = 50; /* default soc to use on error */
	int rc = 0;

	if (chip->ui_soc_ready)
		return chip->ui_soc;

	chg_err("ui_soc not ready, use chip soc\n");
	if (chip->gauge_topic == NULL) {
		chg_err("gauge_topic is NULL, use default soc\n");
		return ui_soc;
	}

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				     true);
	if (rc < 0) {
		chg_err("can't get battery soc, rc=%d\n", rc);
		return ui_soc;
	}
	ui_soc = data.intval;

	return ui_soc;
}

#define OPLUS_NS_TO_MS (1000 * 1000)
int oplus_chg_get_curr_time_ms(unsigned long *time_ms)
{
	u64 ts_nsec;

	ts_nsec = local_clock();
	*time_ms = ts_nsec / OPLUS_NS_TO_MS;

	return *time_ms;
}

#define KPOC_FORCE_VBUS_MV 5000
#define FORCE_VBUS_5V_TIME 10000
static int battery_psy_get_prop(struct power_supply *psy,
		enum power_supply_property prop,
		union power_supply_propval *pval)
{
	struct oplus_gki_device *chip = power_supply_get_drvdata(psy);
	union mms_msg_data data = { 0 };
	struct power_supply *wlspsy = NULL;
	union power_supply_propval wlsval = { 0 };
	int rc = 0;
	int bms_temp_compensation;
	static int pre_batt_status = 0;
	unsigned long cur_chg_time = 0;

	if (chip->gauge_topic == NULL)
		return -ENODEV;

	switch (prop) {
	case POWER_SUPPLY_PROP_STATUS:
		if  (!IS_ERR_OR_NULL(chip->wls_topic))
			(void)oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
		if (data.intval) {
			pval->intval = pre_batt_status;
		} else {
			pval->intval = chip->batt_status;
			if  (chip->wls_online) {
				pre_batt_status = pval->intval;
			} else if (pre_batt_status) {
				wlspsy = power_supply_get_by_name("wireless");
				if (wlspsy)
					power_supply_get_property(wlspsy, POWER_SUPPLY_PROP_ONLINE, &wlsval);
				if (wlsval.intval)
					pval->intval = pre_batt_status;
				else
					pre_batt_status = 0;
			}
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		pval->intval = chip->batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		pval->intval = chip->batt_exist;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		pval->intval = chip->batt_chg_type;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		pval->intval = oplus_gki_get_ui_soc(chip);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		if (is_fv_votable_available(chip))
			pval->intval =
				(get_client_vote(chip->fv_votable, SPEC_VOTER) +
				 50) *
				1000;
		else
			rc = -ENOTSUPP;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		rc = oplus_mms_get_item_data(chip->gauge_topic,
					     GAUGE_ITEM_VOL_MAX, &data, false);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		pval->intval = data.intval;
#else
		pval->intval = data.intval * 1000;
#endif
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		rc = oplus_mms_get_item_data(chip->gauge_topic,
					     GAUGE_ITEM_VOL_MAX, &data, false);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		pval->intval = data.intval;
#else
		pval->intval = data.intval * 1000;
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (is_support_parallel_battery(chip->gauge_topic) &&
		    is_main_gauge_topic_available(chip))
			rc = oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_CURR,
						     &data, (chip->wired_online || chip->wls_online));
		else
			rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR,
						     &data, (chip->wired_online || chip->wls_online));
		pval->intval = data.intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		pval->intval = 6500000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		/* not support CHARGE_CONTROL_LIMIT, just 1 level */
		pval->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if (is_support_parallel_battery(chip->gauge_topic) &&
		    is_main_gauge_topic_available(chip)) {
			rc = oplus_mms_get_item_data(chip->main_gauge_topic, GAUGE_ITEM_TEMP,
						     &data, false);
			pval->intval = data.intval;
		} else {
			bms_temp_compensation = oplus_comm_get_bms_heat_temp_compensation(chip->comm_topic);
			if (bms_temp_compensation != 0 && chip->temperature < OPLUS_BMS_HEAT_THRE)
				pval->intval = chip->temperature - bms_temp_compensation;
			else
				pval->intval = chip->temperature;
		}
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		pval->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		pval->intval = chip->batt_rm * 1000;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		pval->intval = chip->charger_cycle;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		pval->intval = chip->batt_capacity_mah * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		pval->intval = chip->batt_capacity_mah * 1000;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		if (!chip->batt_auth || !chip->batt_hmac)
			pval->strval = "uncertified battery";
		else
			pval->strval = "oplus battery";
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG:
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		if (chip->time_to_full > 0 && oplus_gki_get_ui_soc(chip) == 100)
			pval->intval = 0;
		else
			pval->intval = chip->time_to_full;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
		pval->intval = 3600;
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
	case POWER_SUPPLY_PROP_POWER_AVG:
		pval->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		if (chip->wired_online && chip->vooc_started == true &&
				chip->wired_type == OPLUS_CHG_USB_TYPE_SVOOC)
			pval->intval = 10000;
		else if (chip->wls_online)
			pval->intval = 0; /* TODO */
		else
			pval->intval = oplus_wired_get_vbus();
		if (oplus_is_power_off_charging() &&
		    oplus_chg_get_curr_time_ms(&cur_chg_time) < FORCE_VBUS_5V_TIME)
			pval->intval = KPOC_FORCE_VBUS_MV;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		rc = oplus_mms_get_item_data(chip->gauge_topic,
					     GAUGE_ITEM_VOL_MIN, &data, false);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		pval->intval = data.intval;
#else
		pval->intval = data.intval * 1000;
#endif
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		pval->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		if (oplus_gki_get_ui_soc(chip) == 0) {
			pval->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
			chg_err("POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL, should shutdown!!!\n");
			}
		break;
	default:
		chg_err("get prop %d is not supported\n", prop);
		return -EINVAL;
	}
	if (rc < 0) {
		chg_err("Couldn't get prop %d rc = %d\n", prop, rc);
		return -ENODATA;
	}

	return rc;
}

#define TTF_UPDATE_UEVENT_BIT		BIT(30)
#define TTF_VALUE_MASK			GENMASK(29, 0)
static int battery_psy_set_prop(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *pval)
{
	struct oplus_gki_device *chip = power_supply_get_drvdata(psy);
	int val;
	int rc = 0;

	if (chip->gauge_topic == NULL)
		return -ENODEV;

	switch (prop) {
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!chip->smart_charging_screenoff)
			return -ENOTSUPP;
		val = pval->intval & 0XFFFF;
		if (chip->wired_online &&
		    is_wired_icl_votable_available(chip)) {
			if (chip->led_on) {
				vote(chip->wired_icl_votable, HIDL_VOTER, false, 0, true);
				if (is_vooc_curr_votable_available(chip))
					vote(chip->vooc_curr_votable, HIDL_VOTER, false, 0, false);
				if (is_ufcs_curr_votable_available(chip))
					vote(chip->ufcs_curr_votable, HIDL_VOTER, false, 0, false);
				if (is_pps_curr_votable_available(chip))
					vote(chip->pps_curr_votable, HIDL_VOTER, false, 0, false);
			} else {
				vote(chip->wired_icl_votable, HIDL_VOTER, (val == 0) ? false : true, val, true);
				if (is_vooc_curr_votable_available(chip))
					vote(chip->vooc_curr_votable, HIDL_VOTER, (val == 0) ? false : true, val, false);
				if (is_ufcs_curr_votable_available(chip))
					vote(chip->ufcs_curr_votable, HIDL_VOTER, (val == 0) ? false : true, val, false);
				if (is_pps_curr_votable_available(chip))
					vote(chip->pps_curr_votable, HIDL_VOTER, (val == 0) ? false : true, val, false);
			}
		} else {
			rc = -ENOTSUPP;
		}
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		chip->time_to_full = (pval->intval & TTF_VALUE_MASK) > 0 ? (pval->intval & TTF_VALUE_MASK) : 0;
		if ((pval->intval & TTF_UPDATE_UEVENT_BIT) && (!IS_ERR_OR_NULL(chip->batt_psy)))
			power_supply_changed(chip->batt_psy);
		break;
	default:
		chg_err("set prop %d is not supported\n", prop);
		return -EINVAL;
	}

	return rc;
}

static int battery_psy_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		return 1;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return 1;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		return 1;
	default:
		break;
	}

	return 0;
}

static enum power_supply_property battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
#endif
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_POWER_AVG,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
};

static const struct power_supply_desc batt_psy_desc = {
	.name			= "battery",
	.type			= POWER_SUPPLY_TYPE_BATTERY,
	.properties		= battery_props,
	.num_properties		= ARRAY_SIZE(battery_props),
	.get_property		= battery_psy_get_prop,
	.set_property		= battery_psy_set_prop,
	.property_is_writeable	= battery_psy_prop_is_writeable,
};

#define FORCE_UPDATE_TIME	60
#define HIGH_TEMP_UPDATE_THD	500
#define HIGH_TEMP_UPDATE_TIME	10
static void oplus_gki_gauge_update_work(struct work_struct *work)
{
	struct oplus_gki_device *chip =
		container_of(work, struct oplus_gki_device, gauge_update_work);
	union mms_msg_data data = { 0 };
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int eng_version;
#endif
	static unsigned long update_time = 0;

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
				false);
	chip->vbat_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MIN, &data,
				false);
	chip->vbat_min_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				false);
	chip->soc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC, &data,
				false);
	chip->batt_fcc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
				false);
	chip->batt_rm = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_TEMP, &data,
				false);
	chip->temperature = data.intval;

	if (chip->wired_online)
		chip->charger_cycle = oplus_wired_get_charger_cycle();

#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	eng_version = get_eng_version();
	if (eng_version == AGING || eng_version == HIGH_TEMP_AGING ||
	    eng_version == FACTORY || chip->wired_online || chip->wls_online) {
#else
	if (chip->wired_online || chip->wls_online) {
#endif
		if (!IS_ERR_OR_NULL(chip->batt_psy))
			power_supply_changed(chip->batt_psy);
	} else {
		if ((chip->temperature >= HIGH_TEMP_UPDATE_THD
		    && time_is_before_eq_jiffies(update_time + (unsigned long)(HIGH_TEMP_UPDATE_TIME * HZ))) ||
		    (chip->temperature < HIGH_TEMP_UPDATE_THD
		    && time_is_before_eq_jiffies(update_time + (unsigned long)(FORCE_UPDATE_TIME * HZ)))) {
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			update_time = jiffies;
		}
	}
}

static void oplus_gki_gauge_subs_callback(struct mms_subscribe *subs,
					  enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int rc;

	switch (type) {
	case MSG_TYPE_TIMER:
		schedule_work(&chip->gauge_update_work);
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case GAUGE_ITEM_BATT_EXIST:
			rc = oplus_mms_get_item_data(chip->gauge_topic,
						     GAUGE_ITEM_BATT_EXIST,
						     &data, false);
			if (rc < 0)
				chip->batt_exist = false;
			else
				chip->batt_exist = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case GAUGE_ITEM_AUTH:
			rc = oplus_mms_get_item_data(chip->gauge_topic, id,
						     &data, false);
			if (rc < 0) {
				chg_err("can't get GAUGE_ITEM_AUTH data, rc=%d\n",
					rc);
				chip->batt_auth = false;
			} else {
				chip->batt_auth = !!data.intval;
			}
			break;
		case GAUGE_ITEM_HMAC:
			rc = oplus_mms_get_item_data(chip->gauge_topic, id,
						     &data, false);
			if (rc < 0) {
				chg_err("can't get GAUGE_ITEM_HMAC data, rc=%d\n",
					rc);
				chip->batt_hmac = false;
			} else {
				chip->batt_hmac = !!data.intval;
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

static void oplus_gki_subscribe_gauge_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;
	struct power_supply_config psy_cfg = {};
	union mms_msg_data data = { 0 };
	struct power_supply *psy = NULL;
	int rc;

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_gki_gauge_subs_callback, "gki");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}

	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
				true);
	chip->vbat_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MIN, &data,
				true);
	chip->vbat_min_mv = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data,
				true);
	chip->soc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_FCC, &data,
				true);
	chip->batt_fcc = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_RM, &data,
				true);
	chip->batt_rm = data.intval;
	oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_TEMP, &data,
				true);
	chip->temperature = data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_BATT_EXIST,
				     &data, true);
	if (rc < 0)
		chip->batt_exist = false;
	else
		chip->batt_exist = data.intval;

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_AUTH, &data,
				     true);
	if (rc < 0) {
		chg_err("can't get GAUGE_ITEM_AUTH data, rc=%d\n", rc);
		chip->batt_auth = false;
	} else {
		chip->batt_auth = !!data.intval;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_HMAC, &data,
				     true);
	if (rc < 0) {
		chg_err("can't get GAUGE_ITEM_HMAC data, rc=%d\n", rc);
		chip->batt_hmac = false;
	} else {
		chip->batt_hmac = !!data.intval;
	}

	chip->batt_capacity_mah = oplus_gauge_get_batt_capacity_mah(chip->gauge_topic);

	psy = power_supply_get_by_name("battery");
	if (psy != NULL) {
		chg_info("battery power_sypply already registered\n");
		return;
	}

	psy_cfg.drv_data = chip;
	chip->batt_psy = devm_power_supply_register(&topic->dev, &batt_psy_desc, &psy_cfg);
	if (IS_ERR_OR_NULL(chip->batt_psy)) {
		chg_err("Failed to register battery power supply, rc=%ld\n",
			PTR_ERR(chip->batt_psy));
		goto psy_err;
	}

	return;

psy_err:
	chip->batt_psy = NULL;
	oplus_mms_unsubscribe(chip->gauge_subs);
}

static bool oplus_gki_bc12_is_completed(struct oplus_gki_device *chip)
{
	union mms_msg_data data = { 0 };

	if (chip->wired_topic == NULL)
		return false;

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_BC12_COMPLETED,
				&data, true);
	return !!data.intval;
}

static void oplus_gki_wired_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
				false);
			chip->wired_online = data.intval;
			oplus_mms_get_item_data(chip->wired_topic,
						WIRED_ITEM_CHG_TYPE, &data,
						false);
			chip->wired_type = data.intval;
			chg_info("wired_online=%d\n", chip->wired_online);
			if (!chip->wired_online) {
				usb_psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			} else {
				usb_psy_desc.type = oplus_to_power_supply_type[chip->wired_type];
				if ((usb_psy_desc.type == POWER_SUPPLY_TYPE_UNKNOWN) &&
				    oplus_gki_bc12_is_completed(chip))
					usb_psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			}
			chg_info("psy_type=%d\n", usb_psy_desc.type);
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case WIRED_ITEM_CHG_TYPE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
				false);
			chip->wired_type = data.intval;
			if (chip->wired_online) {
				usb_psy_desc.type = oplus_to_power_supply_type[chip->wired_type];
				if ((usb_psy_desc.type == POWER_SUPPLY_TYPE_UNKNOWN) &&
				    oplus_gki_bc12_is_completed(chip))
					usb_psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			}
			chg_info("psy_type=%d\n", usb_psy_desc.type);
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case WIRED_ITEM_BC12_COMPLETED:
			if (chip->wired_type == OPLUS_CHG_USB_TYPE_UNKNOWN)
				usb_psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		default:
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_gki_subscribe_wired_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;
	struct power_supply_config psy_cfg = {};
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_gki_wired_subs_callback, "gki");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = data.intval;
	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_CHG_TYPE, &data,
				true);
	chip->wired_type = data.intval;
	if (chip->wired_online) {
		usb_psy_desc.type = oplus_to_power_supply_type[chip->wired_type];
		if ((usb_psy_desc.type == POWER_SUPPLY_TYPE_UNKNOWN) &&
		    oplus_gki_bc12_is_completed(chip))
			usb_psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
	} else {
		usb_psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
	}
	chg_info("psy_type=%d\n", usb_psy_desc.type);

	chip->charger_cycle = oplus_wired_get_charger_cycle();

	psy_cfg.drv_data = chip;
	chip->usb_psy = devm_power_supply_register(&topic->dev, &usb_psy_desc,
						   &psy_cfg);
	if (IS_ERR_OR_NULL(chip->usb_psy)) {
		chg_err("Failed to register usb power supply, rc=%ld\n",
			PTR_ERR(chip->usb_psy));
		goto psy_err;
	}

	return;

psy_err:
	chip->usb_psy = NULL;
	oplus_mms_unsubscribe(chip->wired_subs);
}

#define WLS_SK_TIME	5000
#define WLS_SK_DELAY_UNLOCK_TIME	100
static void oplus_chg_wls_status_keep_clean_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_gki_device *chip =
		container_of(dwork, struct oplus_gki_device, status_keep_clean_work);
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
	if (data.intval == WLS_SK_BY_HAL) {
		oplus_chg_wls_set_status_keep(chip->wls_topic, WLS_SK_WAIT_TIMEOUT);
		schedule_delayed_work(&chip->status_keep_clean_work, msecs_to_jiffies(WLS_SK_TIME));
		return;
	}

	oplus_chg_wls_set_status_keep(chip->wls_topic, WLS_SK_NULL);
	if (chip->batt_psy)
		power_supply_changed(chip->batt_psy);
	schedule_delayed_work(&chip->status_keep_delay_unlock_work, msecs_to_jiffies(WLS_SK_DELAY_UNLOCK_TIME));
}

static void oplus_chg_wls_status_keep_delay_unlock_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_gki_device *chip =
		container_of(dwork, struct oplus_gki_device, status_keep_delay_unlock_work);

	if (chip->status_wake_lock_on) {
		chg_info("release status_wake_lock\n");
		__pm_relax(chip->status_wake_lock);
		chip->status_wake_lock_on = false;
	}
}

static void oplus_gki_wls_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WLS_ITEM_PRESENT:
			oplus_mms_get_item_data(chip->wls_topic, id, &data, false);
			chip->wls_online = !!data.intval;
			chg_info("wls_present=%d\n", chip->wls_online);
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_gki_subscribe_wls_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;
	struct power_supply_config psy_cfg = {};
	union mms_msg_data data = { 0 };

	chip->wls_topic = topic;
	chip->wls_subs = oplus_mms_subscribe(chip->wls_topic, chip, oplus_gki_wls_subs_callback, "gki");
	if (IS_ERR_OR_NULL(chip->wls_subs)) {
		chg_err("subscribe wls topic error, rc=%ld\n", PTR_ERR(chip->wls_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_PRESENT, &data, true);
	chip->wls_online = !!data.intval;

	psy_cfg.drv_data = chip;
	chip->wls_psy = devm_power_supply_register(&topic->dev, &wls_psy_desc, &psy_cfg);
	if (IS_ERR_OR_NULL(chip->wls_psy)) {
		chg_err("Failed to register wireless power supply, rc=%ld\n", PTR_ERR(chip->wls_psy));
		goto psy_err;
	}

	INIT_DELAYED_WORK(&chip->status_keep_clean_work, oplus_chg_wls_status_keep_clean_work);
	INIT_DELAYED_WORK(&chip->status_keep_delay_unlock_work, oplus_chg_wls_status_keep_delay_unlock_work);
	chip->status_wake_lock = wakeup_source_register(NULL, "status_wake_lock");
	chip->status_wake_lock_on = false;

	return;

psy_err:
	chip->wls_psy = NULL;
	oplus_mms_unsubscribe(chip->wls_subs);
}

static void oplus_gki_comm_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_BATT_STATUS:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_status = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case COMM_ITEM_BATT_HEALTH:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_health = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case COMM_ITEM_BATT_CHG_TYPE:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_chg_type = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case COMM_ITEM_UI_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->ui_soc = data.intval;
			if (chip->ui_soc < 0) {
				chip->ui_soc_ready = false;
				chip->ui_soc = 0;
			} else {
				chip->ui_soc_ready = true;
			}
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case COMM_ITEM_LED_ON:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->led_on = data.intval;
			if (chip->led_on) {
				if (is_vooc_curr_votable_available(chip))
					vote(chip->vooc_curr_votable, HIDL_VOTER, false, 0, false);

				if (is_ufcs_curr_votable_available(chip))
					vote(chip->ufcs_curr_votable, HIDL_VOTER, false, 0, false);

				if (is_pps_curr_votable_available(chip))
					vote(chip->pps_curr_votable, HIDL_VOTER, false, 0, false);
			}
			break;
		case COMM_ITEM_CHARGING_DISABLE:
		case COMM_ITEM_CHARGE_SUSPEND:
		case COMM_ITEM_NOTIFY_CODE:
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_gki_subscribe_comm_topic(struct oplus_mms *topic,
						void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->comm_topic = topic;
	chip->comm_subs = oplus_mms_subscribe(chip->comm_topic, chip,
					      oplus_gki_comm_subs_callback,
					      "gki");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_BATT_STATUS, &data,
				true);
	chip->batt_status = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_BATT_HEALTH, &data,
				true);
	chip->batt_health = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_BATT_CHG_TYPE,
				&data, true);
	chip->batt_chg_type = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_UI_SOC, &data,
				true);
	chip->ui_soc = data.intval;
	if (chip->ui_soc < 0) {
		chip->ui_soc_ready = false;
		chip->ui_soc = 0;
	} else {
		chip->ui_soc_ready = true;
	}
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_LED_ON, &data,
				true);
	chip->led_on = data.intval;
}

static void oplus_gki_vooc_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_VOOC_CHARGING:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_charging = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		case VOOC_ITEM_VOOC_STARTED:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_started = data.intval;
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		default:
			if (!IS_ERR_OR_NULL(chip->batt_psy))
				power_supply_changed(chip->batt_psy);
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_gki_subscribe_vooc_topic(struct oplus_mms *topic,
					   void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->vooc_topic = topic;
	chip->vooc_subs = oplus_mms_subscribe(
		chip->vooc_topic, chip, oplus_gki_vooc_subs_callback, "gki");
	if (IS_ERR_OR_NULL(chip->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(chip->vooc_subs));
		return;
	}

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_CHARGING,
				&data, true);
	chip->vooc_charging = !!data.intval;

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_VOOC_STARTED,
				&data, true);
	chip->vooc_started = !!data.intval;
}

static void oplus_gki_ufcs_subs_callback(struct mms_subscribe *subs,
					     enum mms_msg_type type, u32 id)
{
	struct oplus_gki_device *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_ITEM:
		if (chip->batt_psy)
			power_supply_changed(chip->batt_psy);
		break;
	default:
		break;
	}
}

static void oplus_gki_subscribe_ufcs_topic(struct oplus_mms *topic,
					       void *prv_data)
{
	struct oplus_gki_device *chip = prv_data;

	chip->ufcs_topic = topic;
	chip->ufcs_subs = oplus_mms_subscribe(chip->ufcs_topic, chip, oplus_gki_ufcs_subs_callback, "gki");
	if (IS_ERR_OR_NULL(chip->ufcs_subs)) {
		chg_err("subscribe ufcs topic error, rc=%ld\n",
			PTR_ERR(chip->ufcs_subs));
		return;
	}
}

static int oplus_gki_driver_probe(struct platform_device *pdev)
{
	struct oplus_gki_device *gki_dev = g_gki_dev;
	struct power_supply *psy = NULL;
	struct power_supply_config psy_cfg = {};

	if (gki_dev == NULL) {
		chg_err("g_gki_dev is NULL\n");
		return -ENODEV;
	}

	gki_dev->dev = &pdev->dev;
	psy = power_supply_get_by_name("battery");
	if (psy != NULL) {
		chg_info("battery power_sypply already registered\n");
		return 0;
	}

	psy_cfg.drv_data = gki_dev;
	gki_dev->batt_psy = devm_power_supply_register(gki_dev->dev, &batt_psy_desc, &psy_cfg);
	if (IS_ERR_OR_NULL(gki_dev->batt_psy)) {
		chg_err("Failed to register battery power supply, rc=%ld\n",
			PTR_ERR(gki_dev->batt_psy));
		return PTR_ERR(gki_dev->batt_psy);
	}

	return 0;
}

static int oplus_gki_driver_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id oplus_chg_gki_match[] = {
	{ .compatible = "oplus,chg-gki" },
	{},
};

static struct platform_driver oplus_chg_gki_driver = {
	.driver = {
		.name = "oplus_chg_gki",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_chg_gki_match),
	},
	.probe = oplus_gki_driver_probe,
	.remove = oplus_gki_driver_remove,
};

static __init int oplus_chg_gki_init(void)
{
	struct oplus_gki_device *gki_dev;
	struct device_node *node;

	gki_dev = kzalloc(sizeof(struct oplus_gki_device), GFP_KERNEL);
	if (gki_dev == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	g_gki_dev = gki_dev;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		gki_dev->smart_charging_screenoff = false;
	else
		gki_dev->smart_charging_screenoff = of_property_read_bool(
			node, "oplus,smart_charging_screenoff");

	gki_dev->time_to_full = 0;
	INIT_WORK(&gki_dev->gauge_update_work, oplus_gki_gauge_update_work);

	oplus_mms_wait_topic("gauge", oplus_gki_subscribe_gauge_topic, gki_dev);
	oplus_mms_wait_topic("wired", oplus_gki_subscribe_wired_topic, gki_dev);
	oplus_mms_wait_topic("wireless", oplus_gki_subscribe_wls_topic, gki_dev);
	oplus_mms_wait_topic("common", oplus_gki_subscribe_comm_topic, gki_dev);
	oplus_mms_wait_topic("vooc", oplus_gki_subscribe_vooc_topic, gki_dev);
	oplus_mms_wait_topic("ufcs", oplus_gki_subscribe_ufcs_topic, gki_dev);

	platform_driver_register(&oplus_chg_gki_driver);
	return 0;
}

static __exit void oplus_chg_gki_exit(void)
{
	if (g_gki_dev == NULL)
		return;

	if (g_gki_dev->batt_psy)
		power_supply_unregister(g_gki_dev->batt_psy);
	if (g_gki_dev->usb_psy)
		power_supply_unregister(g_gki_dev->usb_psy);
	if (g_gki_dev->wls_psy)
		power_supply_unregister(g_gki_dev->wls_psy);
	if (!IS_ERR_OR_NULL(g_gki_dev->wired_subs))
		oplus_mms_unsubscribe(g_gki_dev->wired_subs);
	if (!IS_ERR_OR_NULL(g_gki_dev->gauge_subs))
		oplus_mms_unsubscribe(g_gki_dev->gauge_subs);
	if (!IS_ERR_OR_NULL(g_gki_dev->wls_subs))
		oplus_mms_unsubscribe(g_gki_dev->wls_subs);
	if (!IS_ERR_OR_NULL(g_gki_dev->comm_subs))
		oplus_mms_unsubscribe(g_gki_dev->comm_subs);
	if (!IS_ERR_OR_NULL(g_gki_dev->vooc_subs))
		oplus_mms_unsubscribe(g_gki_dev->vooc_subs);
	if (!IS_ERR_OR_NULL(g_gki_dev->ufcs_subs))
		oplus_mms_unsubscribe(g_gki_dev->ufcs_subs);

	kfree(g_gki_dev);
	g_gki_dev = NULL;
	platform_driver_unregister(&oplus_chg_gki_driver);
}

oplus_chg_module_register(oplus_chg_gki);
