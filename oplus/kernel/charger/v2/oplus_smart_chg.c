#define pr_fmt(fmt) "[SMART_CHG]([%s][%d]): " fmt, __func__, __LINE__

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
#include <linux/gfp.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_vooc.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_ufcs.h>
#include <oplus_chg_wls.h>
#include <oplus_smart_chg.h>

#define QUICK_MODE_POWER_THR_W	100

struct oplus_smart_charge {
	struct device *dev;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *wired_topic;
	struct oplus_mms *ufcs_topic;
	struct oplus_mms *wls_topic;
	struct mms_subscribe *vooc_subs;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *ufcs_subs;
	struct mms_subscribe *wls_subs;
	struct votable *cool_down_votable;
	struct votable *vooc_curr_votable;

	struct work_struct quick_mode_check_work;

	bool wired_online;
	bool vooc_online;
	bool wls_online;
	unsigned int vooc_sid;

	bool ufcs_online;
	bool ufcs_charging;

	int normal_cool_down;
	int smart_normal_cool_down;
	struct timespec quick_mode_time;
	long start_time;
	long quick_mode_start_time;
	long quick_mode_stop_time;
	long quick_mode_gain_time_ms;
	int quick_mode_start_cap;
	int quick_mode_stop_cap;
	int quick_mode_stop_temp;
	int quick_mode_stop_soc;
	bool quick_mode_need_update;
	bool smart_chg_bcc_support;
	bool smart_chg_soh_support;
	bool quick_mode_gain_support;

	int batt_soc;
	int batt_rm;
	int shell_temp;
	int bcc_current;
	int bcc_buf[BCC_PARMS_COUNT];
};

static struct oplus_smart_charge *g_smart_chg;

__maybe_unused static bool
is_vooc_curr_votable_available(struct oplus_smart_charge *chip)
{
	if (!chip->vooc_curr_votable)
		chip->vooc_curr_votable = find_votable("VOOC_CURR");
	return !!chip->vooc_curr_votable;
}

static bool is_cool_down_votable_available(struct oplus_smart_charge *smart_chg)
{
	if (!smart_chg->cool_down_votable)
		smart_chg->cool_down_votable = find_votable("COOL_DOWN");
	return !!smart_chg->cool_down_votable;
}

static void oplus_smart_chg_quick_mode_init(struct oplus_smart_charge *smart_chg)
{
	smart_chg->quick_mode_time = oplus_current_kernel_time();
	smart_chg->start_time = smart_chg->quick_mode_time.tv_sec;
	smart_chg->quick_mode_gain_time_ms = 0;
	smart_chg->quick_mode_start_time = 0;
	smart_chg->quick_mode_stop_time = 0;
	smart_chg->quick_mode_start_cap = -1;
	smart_chg->quick_mode_stop_cap = -1;
	smart_chg->quick_mode_stop_temp = -1;
	smart_chg->quick_mode_stop_soc = -1;
}

static void oplus_smart_chg_gauge_update(struct oplus_smart_charge *smart_chg)
{
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(smart_chg->gauge_topic, GAUGE_ITEM_RM, &data, false);
	smart_chg->batt_rm = data.intval;
	oplus_mms_get_item_data(smart_chg->gauge_topic, GAUGE_ITEM_SOC, &data, false);
	smart_chg->batt_soc = data.intval;

	if (smart_chg->comm_topic) {
		oplus_mms_get_item_data(smart_chg->comm_topic, COMM_ITEM_SHELL_TEMP, &data, false);
		smart_chg->shell_temp = data.intval;
	}
}

static void oplus_smart_chg_quick_mode_check(struct oplus_smart_charge *smart_chg)
{
	bool quick_mode_check = false;
	struct timespec ts_now;
	long diff_time, gain_time_ms;
	int batt_curve_current, cool_down, current_cool_down, current_normal_cool_down;
	int led_on = 0;
	union mms_msg_data data = { 0 };

	if (!is_cool_down_votable_available(smart_chg)) {
		chg_err("cool_down votable not found\n");
		return;
	}
	if (g_smart_chg->comm_topic) {
		oplus_mms_get_item_data(g_smart_chg->comm_topic, COMM_ITEM_LED_ON, &data, false);
		led_on = !!data.intval;
	}
	cool_down = get_effective_result(smart_chg->cool_down_votable);

	if (smart_chg->vooc_online &&
		smart_chg->quick_mode_gain_support &&
		(sid_to_adapter_power(smart_chg->vooc_sid) >= QUICK_MODE_POWER_THR_W)) {
		quick_mode_check = true;
	}

	/* TODO: PPS */

	if (!quick_mode_check)
		return;

	ts_now = oplus_current_kernel_time();
	diff_time = ts_now.tv_sec - smart_chg->quick_mode_time.tv_sec;
	smart_chg->quick_mode_time = ts_now;

	if (smart_chg->vooc_online) {
		if (smart_chg->vooc_topic == NULL) {
			chg_err("vooc topic not found\n");
			return;
		}
		batt_curve_current = oplus_vooc_get_batt_curve_current(smart_chg->vooc_topic);
		if (batt_curve_current < 0) {
			chg_err("can't get batt_curve_current, rc=%d\n",
				batt_curve_current);
			return;
		}
		if (led_on == 0 && is_vooc_curr_votable_available(smart_chg)) {
			current_cool_down = get_effective_result(smart_chg->vooc_curr_votable);
		} else {
			current_cool_down = oplus_vooc_level_to_current(smart_chg->vooc_topic, cool_down);
		}
		if (current_cool_down < 0) {
			chg_err("can't get current_cool_down, cool_down=%d, rc=%d\n",
				cool_down, current_cool_down);
			return;
		}
		current_normal_cool_down =
			oplus_vooc_level_to_current(smart_chg->vooc_topic, smart_chg->normal_cool_down);
		if (current_normal_cool_down <= 0) {
			chg_err("can't get current_normal_cool_down, cool_down=%d, rc=%d\n",
				cool_down, current_normal_cool_down);
			return;
		}
	} else {
		/* TODO: PPS */
		return;
	}

	if ((current_cool_down != current_normal_cool_down) && (current_cool_down > 0)) {
		if (smart_chg->quick_mode_start_time == 0) {
			smart_chg->quick_mode_start_time = ts_now.tv_sec;
			smart_chg->quick_mode_start_cap = smart_chg->batt_rm;
		}
		smart_chg->quick_mode_need_update = true;
	}
	if ((current_cool_down == current_normal_cool_down) && smart_chg->quick_mode_need_update) {
		smart_chg->quick_mode_stop_time = ts_now.tv_sec;
		smart_chg->quick_mode_stop_cap = smart_chg->batt_rm;
		smart_chg->quick_mode_stop_temp = smart_chg->shell_temp;
		smart_chg->quick_mode_stop_soc = smart_chg->batt_soc;
		smart_chg->quick_mode_need_update = false;
	}

	if (current_cool_down > batt_curve_current) {
		if (current_normal_cool_down > batt_curve_current) {
			gain_time_ms = 0;
		} else {
			gain_time_ms = ((batt_curve_current * diff_time * 1000) / current_normal_cool_down) -
				       (diff_time * 1000);
		}
	} else {
		gain_time_ms = ((current_cool_down * diff_time * 1000) / current_normal_cool_down) - (diff_time * 1000);
	}

	if (current_cool_down == current_normal_cool_down) {
		gain_time_ms = 0;
	} else if (current_cool_down == 0) {
		if (current_normal_cool_down < batt_curve_current)
			gain_time_ms = ((batt_curve_current * diff_time * 1000) / current_normal_cool_down) -
				(diff_time * 1000);
		else
			gain_time_ms = 0;
	}
	if (gain_time_ms < 0) {
		chg_err("gain_time_ms:%ld, force set 0 \n", gain_time_ms);
		gain_time_ms = 0;
	}
	smart_chg->quick_mode_gain_time_ms = smart_chg->quick_mode_gain_time_ms + gain_time_ms;

	chg_err("quick_mode_gain_time_ms:%ld, start_cap:%d, stop_cap:%d, "
		"start_time:%ld, stop_time:%ld, diff_time:%ld, gain_time_ms:%ld\n",
		smart_chg->quick_mode_gain_time_ms, smart_chg->quick_mode_start_cap, smart_chg->quick_mode_stop_cap,
		smart_chg->quick_mode_start_time, smart_chg->quick_mode_time.tv_sec, diff_time, gain_time_ms);
}

static void oplus_smart_chg_quick_mode_check_work(struct work_struct *work)
{
	struct oplus_smart_charge *smart_chg =
		container_of(work, struct oplus_smart_charge, quick_mode_check_work);

	if (smart_chg->wired_online && smart_chg->vooc_online) {
		oplus_smart_chg_quick_mode_check(smart_chg);
	} else if (smart_chg->quick_mode_need_update) {
		smart_chg->quick_mode_time = oplus_current_kernel_time();
		smart_chg->quick_mode_stop_time = smart_chg->quick_mode_time.tv_sec;
		smart_chg->quick_mode_stop_temp = smart_chg->shell_temp;
		smart_chg->quick_mode_stop_soc = smart_chg->batt_soc;
		smart_chg->quick_mode_need_update = false;
	}
}

static void oplus_smart_chg_vooc_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_smart_charge *smart_chg = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_ONLINE:
			oplus_mms_get_item_data(smart_chg->vooc_topic, id, &data, false);
			smart_chg->vooc_online = !!data.intval;
			break;
		case VOOC_ITEM_SID:
			oplus_mms_get_item_data(smart_chg->vooc_topic, id, &data, false);
			smart_chg->vooc_sid = (unsigned int)data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_smart_chg_subscribe_vooc_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	smart_chg->vooc_topic = topic;
	smart_chg->vooc_subs =
		oplus_mms_subscribe(smart_chg->vooc_topic, smart_chg,
				    oplus_smart_chg_vooc_subs_callback, "smart_chg");
	if (IS_ERR_OR_NULL(smart_chg->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(smart_chg->vooc_subs));
		return;
	}

	rc = oplus_mms_get_item_data(smart_chg->vooc_topic, VOOC_ITEM_ONLINE, &data, true);
	if (rc < 0) {
		chg_err("can't get vooc online status, rc=%d\n", rc);
		smart_chg->vooc_online = false;
	} else {
		smart_chg->vooc_online = !!data.intval;
	}
	rc = oplus_mms_get_item_data(smart_chg->vooc_topic, VOOC_ITEM_SID, &data, false);
	if (rc < 0) {
		chg_err("can't get vooc sid, rc=%d\n", rc);
		smart_chg->vooc_sid = 0;
	} else {
		smart_chg->vooc_sid = (unsigned int)data.intval;
	}
}

static void oplus_smart_chg_gauge_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_smart_charge *smart_chg = subs->priv_data;

	switch (type) {
	case MSG_TYPE_TIMER:
		oplus_smart_chg_gauge_update(smart_chg);
		schedule_work(&smart_chg->quick_mode_check_work);
		break;
	case MSG_TYPE_ITEM:
		break;
	default:
		break;
	}
}

static void oplus_smart_chg_subscribe_gauge_topic(struct oplus_mms *topic,
						  void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;
	union mms_msg_data data = { 0 };

	smart_chg->gauge_topic = topic;
	smart_chg->gauge_subs =
		oplus_mms_subscribe(smart_chg->gauge_topic, smart_chg,
				    oplus_smart_chg_gauge_subs_callback, "smart_chg");
	if (IS_ERR_OR_NULL(smart_chg->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(smart_chg->gauge_subs));
		return;
	}

	oplus_mms_get_item_data(smart_chg->gauge_topic, GAUGE_ITEM_RM, &data, true);
	smart_chg->batt_rm = data.intval;
	oplus_mms_get_item_data(smart_chg->gauge_topic, GAUGE_ITEM_SOC, &data, true);
	smart_chg->batt_soc = data.intval;
}

static void oplus_smart_chg_wired_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_smart_charge *smart_chg = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(smart_chg->wired_topic, id, &data, false);
			smart_chg->wired_online = !!data.intval;
			if (smart_chg->wired_online && !smart_chg->vooc_online)
				oplus_smart_chg_quick_mode_init(smart_chg);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_smart_chg_subscribe_wired_topic(struct oplus_mms *topic,
						  void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;
	union mms_msg_data data = { 0 };

	smart_chg->wired_topic = topic;
	smart_chg->wired_subs =
		oplus_mms_subscribe(smart_chg->wired_topic, smart_chg,
				    oplus_smart_chg_wired_subs_callback, "smart_chg");
	if (IS_ERR_OR_NULL(smart_chg->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(smart_chg->wired_subs));
		return;
	}

	oplus_mms_get_item_data(smart_chg->wired_topic, WIRED_ITEM_ONLINE, &data, true);
	smart_chg->wired_online = !!data.intval;
}

static void oplus_smart_chg_ufcs_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_smart_charge *smart_chg = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case UFCS_ITEM_ONLINE:
			oplus_mms_get_item_data(smart_chg->ufcs_topic, id, &data, false);
			smart_chg->ufcs_online = !!data.intval;
			break;
		case UFCS_ITEM_CHARGING:
			oplus_mms_get_item_data(smart_chg->ufcs_topic, id, &data, false);
			smart_chg->ufcs_charging = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_smart_chg_subscribe_ufcs_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	smart_chg->ufcs_topic = topic;
	smart_chg->ufcs_subs =
		oplus_mms_subscribe(smart_chg->ufcs_topic, smart_chg,
				    oplus_smart_chg_ufcs_subs_callback, "smart_chg");
	if (IS_ERR_OR_NULL(smart_chg->ufcs_subs)) {
		chg_err("subscribe ufcs topic error, rc=%ld\n",
			PTR_ERR(smart_chg->ufcs_subs));
		return;
	}

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_ONLINE, &data, true);
	if (rc < 0) {
		chg_err("can't get ufcs online status, rc=%d\n", rc);
		smart_chg->ufcs_online = false;
	} else {
		smart_chg->ufcs_online = !!data.intval;
	}

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_CHARGING, &data, true);
	if (rc < 0) {
		chg_err("can't get ufcs charging status, rc=%d\n", rc);
		smart_chg->ufcs_charging = false;
	} else {
		smart_chg->ufcs_charging = !!data.intval;
	}
}

static void oplus_smart_chg_wls_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_smart_charge *smart_chg = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WLS_ITEM_PRESENT:
			oplus_mms_get_item_data(smart_chg->wls_topic, id, &data, false);
			smart_chg->wls_online = !!data.intval;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
static void oplus_smart_chg_subscribe_wls_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;

	smart_chg->wls_topic = topic;
	smart_chg->wls_subs =
		oplus_mms_subscribe(smart_chg->wls_topic, smart_chg,
				    oplus_smart_chg_wls_subs_callback, "smart_chg");
	if (IS_ERR_OR_NULL(smart_chg->wls_subs)) {
		chg_err("subscribe wls topic error, rc=%ld\n",
			PTR_ERR(smart_chg->wls_subs));
		return;
	}
}

static void oplus_smart_chg_common_topic_ready(struct oplus_mms *topic,
						void *prv_data)
{
	struct oplus_smart_charge *smart_chg = prv_data;
	union mms_msg_data data = { 0 };

	smart_chg->comm_topic = topic;
	oplus_mms_get_item_data(smart_chg->comm_topic, COMM_ITEM_SHELL_TEMP, &data, true);
	smart_chg->shell_temp = data.intval;

	oplus_mms_wait_topic("wired", oplus_smart_chg_subscribe_wired_topic, smart_chg);
	oplus_mms_wait_topic("vooc", oplus_smart_chg_subscribe_vooc_topic, smart_chg);
	oplus_mms_wait_topic("gauge", oplus_smart_chg_subscribe_gauge_topic, smart_chg);
	oplus_mms_wait_topic("ufcs", oplus_smart_chg_subscribe_ufcs_topic, smart_chg);
	oplus_mms_wait_topic("wireless", oplus_smart_chg_subscribe_wls_topic, smart_chg);
	/* TODO: wait pps & wireless topic? */
}

static int oplus_smart_charge_parse_dt(struct oplus_smart_charge *smart_chg)
{
	bool bcc_support = 0;
	struct oplus_mms *vooc_topic;
	struct device_node *node = smart_chg->dev->of_node;

	vooc_topic = g_smart_chg->vooc_topic;

	bcc_support = oplus_vooc_get_bcc_support_for_smartchg(vooc_topic);
	chg_info("oplus,smart_chg_bcc_support is %d %d %p\n",
		smart_chg->smart_chg_bcc_support, bcc_support, node);

	smart_chg->smart_chg_soh_support =
		of_property_read_bool(node, "oplus,smart_chg_soh_support");
	smart_chg->quick_mode_gain_support =
                of_property_read_bool(node, "oplus,quick_mode_gain_support");

	chg_info("oplus,smart_chg_soh_support is %d, quick_mode_gain_support = %d\n",
		smart_chg->smart_chg_soh_support,
		smart_chg->quick_mode_gain_support);

	return 0;
}

#define BCC_SET_DEBUG_PARMS 1
#define BCC_PAGE_SIZE 256
#define BCC_N_DEBUG 0
#define BCC_Y_DEBUG 1
#define BCC_TEMP_RANGE_WRONG 0
static int bcc_debug_mode  = BCC_N_DEBUG;
static char bcc_debug_buf[BCC_PAGE_SIZE] = {0};
static int oplus_vooc_check_bcc_max_curr(void)
{
	int bcc_max_curr = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_GET_BCC_MAX_CURR, &data, true);
	if (!rc)
		bcc_max_curr = data.intval;

	return bcc_max_curr;
}

static int oplus_vooc_check_bcc_min_curr(void)
{
	int bcc_min_curr = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_GET_BCC_MIN_CURR, &data, true);
	if (!rc)
		bcc_min_curr = data.intval;

	return bcc_min_curr;
}

static int oplus_vooc_get_bcc_exit_curr(void)
{
	int bcc_stop_curr = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_GET_BCC_STOP_CURR, &data, true);
	if (!rc)
		bcc_stop_curr = data.intval;

	return bcc_stop_curr;
}

static int oplus_vooc_check_bcc_temp(void)
{
	int bcc_temp = 0;
	struct oplus_mms *comm_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	comm_topic = g_smart_chg->comm_topic;
	if (!comm_topic)
		return 0;

	rc = oplus_mms_get_item_data(comm_topic, COMM_ITEM_SHELL_TEMP, &data, true);
	if (!rc)
		bcc_temp = data.intval;

	return bcc_temp;
}

static int oplus_vooc_check_bcc_temp_range(void)
{
	int bcc_temp_status = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_GET_BCC_TEMP_RANGE, &data, true);
	if (!rc)
		bcc_temp_status = data.intval;

	return bcc_temp_status;
}

static bool oplus_wls_get_fastchg_ing(void)
{
	int fastchg_status = 0;
	struct oplus_mms *wls_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return false;

	wls_topic = g_smart_chg->wls_topic;
	if (!wls_topic)
		return false;

	rc = oplus_mms_get_item_data(wls_topic, WLS_ITEM_FASTCHG_STATUS, &data, true);
	if (!rc)
		fastchg_status = data.intval;

	if (fastchg_status == true)
		return true;

	return false;
}

static int oplus_wls_get_bcc_max_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_max_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->wls_topic, WLS_ITEM_BCC_MAX_CURR, &data, true);
	if (!rc)
		bcc_max_curr = data.intval;

	return bcc_max_curr;
}

static int oplus_wls_get_bcc_min_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_min_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->wls_topic, WLS_ITEM_BCC_MIN_CURR, &data, true);
	if (!rc)
		bcc_min_curr = data.intval;

	return bcc_min_curr;
}

static int oplus_wls_get_bcc_exit_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_stop_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->wls_topic, WLS_ITEM_BCC_EXIT_CURR, &data, true);
	if (!rc)
		bcc_stop_curr = data.intval;

	return bcc_stop_curr;
}

static bool oplus_vooc_get_fastchg_ing(void)
{
	int fastchg_status = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return false;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return false;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_VOOC_CHARGING, &data, true);
	if (!rc)
		fastchg_status = data.intval;

	if (fastchg_status == true)
		return true;

	return false;
}

static bool oplus_voocphy_get_fastchg_ing(void)
{
	int fastchg_status = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return false;

	vooc_topic = g_smart_chg->vooc_topic;
	if (!vooc_topic)
		return false;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_VOOCPHY_BCC_GET_FASTCHG_ING, &data, true);
	if (!rc)
		fastchg_status = data.intval;

	if (fastchg_status == true)
		return true;

	return false;
}

static int oplus_vooc_get_fast_chg_type(void)
{
	int svooc_type = 0;
	struct oplus_mms *vooc_topic;
	union mms_msg_data data = { 0 };
	int rc;

	if (NULL == g_smart_chg)
		return 0;

	vooc_topic = g_smart_chg->vooc_topic;

	if (!vooc_topic)
		return 0;

	rc = oplus_mms_get_item_data(vooc_topic, VOOC_ITEM_GET_BCC_SVOOC_TYPE, &data, true);
	if (!rc)
		svooc_type = data.intval;
	return svooc_type;
}

static int oplus_ufcs_get_bcc_max_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_max_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_BCC_MAX_CURR, &data, true);
	if (!rc)
		bcc_max_curr = data.intval;

	return bcc_max_curr;
}

static int oplus_ufcs_get_bcc_min_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_min_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_BCC_MIN_CURR, &data, true);
	if (!rc)
		bcc_min_curr = data.intval;

	return bcc_min_curr;
}

static int oplus_ufcs_get_bcc_exit_curr(struct oplus_smart_charge *smart_chg)
{
	int bcc_stop_curr = 0;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_BCC_EXIT_CURR, &data, true);
	if (!rc)
		bcc_stop_curr = data.intval;

	return bcc_stop_curr;
}

static bool oplus_ufcs_check_bcc_temp_range(struct oplus_smart_charge *smart_chg)
{
	bool bcc_temp_status = false;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->ufcs_topic, UFCS_ITEM_BCC_TEMP_RANGE, &data, true);
	if (!rc)
		bcc_temp_status = !!data.intval;

	return bcc_temp_status;
}

static bool oplus_wls_check_bcc_temp_range(struct oplus_smart_charge *smart_chg)
{
	bool bcc_temp_status = false;
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(smart_chg->wls_topic, WLS_ITEM_BCC_TEMP_RANGE, &data, true);
	if (!rc)
		bcc_temp_status = !!data.intval;

	return bcc_temp_status;
}

static void oplus_smart_chg_bcc_set_buffer(int *buffer)
{
	int batt_qmax_1 = 0;
	int batt_qmax_2 = 0;
	int batt_qmax_passed_q = 0;
	int batt_dod0_1 = 0;
	int batt_dod0_2 = 0;
	int batt_dod0_passed_q = 0;
	int soc_ext_1 = 0, soc_ext_2 = 0;
	int btemp = 0;
	int batt_current = 0;
	int voltage_cell1 = 0, voltage_cell2 = 0;
	int bcc_current_max = 0, bcc_current_min = 0;
	int atl_last_gear_current = 0;
	int gauge_type;
	struct oplus_mms *gauge_topic;

	if (NULL == buffer) {
		chg_err("oplus_smart_chg_bcc_set_buffer input error!");
	    return;
	}

	if (true == oplus_voocphy_get_fastchg_ing() ||
		(oplus_vooc_get_fastchg_ing() && oplus_vooc_get_fast_chg_type() != BCC_TYPE_IS_VOOC)){
		bcc_current_max = oplus_vooc_check_bcc_max_curr();
		bcc_current_min = oplus_vooc_check_bcc_min_curr();
		atl_last_gear_current = oplus_vooc_get_bcc_exit_curr();
	} else if (g_smart_chg->ufcs_charging) {
		bcc_current_max = oplus_ufcs_get_bcc_max_curr(g_smart_chg);
		bcc_current_min = oplus_ufcs_get_bcc_min_curr(g_smart_chg);
		atl_last_gear_current = oplus_ufcs_get_bcc_exit_curr(g_smart_chg);
	} else if (g_smart_chg->wls_online) {
		bcc_current_max = oplus_wls_get_bcc_max_curr(g_smart_chg);
		bcc_current_min = oplus_wls_get_bcc_min_curr(g_smart_chg);
		atl_last_gear_current = oplus_wls_get_bcc_exit_curr(g_smart_chg);
	}

	gauge_topic = g_smart_chg->gauge_topic;
	oplus_gauge_get_gauge_type(gauge_topic, 0, &gauge_type);

	if ((DEVICE_ZY0603 == gauge_type) || (DEVICE_ZY0602 == gauge_type)) {
		buffer[17] = SW_GAUGE;
	} else if ((DEVICE_BQ27411 == gauge_type) || (DEVICE_BQ27541 == gauge_type)) {
		buffer[17] = TI_GAUGE;
	} else {
		buffer[17] = UNKNOWN_GAUGE_TYPE;
	}

	if ((DEVICE_ZY0602 == gauge_type) || (DEVICE_BQ27411 == gauge_type)) {
		buffer[18] = SINGLE_CELL;
	} else {
		buffer[18] = DOUBLE_SERIES_WOUND_CELLS;
	}

	oplus_gauge_get_qmax(gauge_topic, 0, &batt_qmax_1);
	oplus_gauge_get_qmax(gauge_topic, 1, &batt_qmax_2);
	oplus_gauge_get_qmax_passed_q(gauge_topic, 0, &batt_qmax_passed_q);
	oplus_gauge_get_dod0(gauge_topic, 0, &batt_dod0_1);
	oplus_gauge_get_dod0(gauge_topic, 1, &batt_dod0_2);
	oplus_gauge_get_dod0_passed_q(gauge_topic, 0, &batt_dod0_passed_q);

	oplus_gauge_get_volt(gauge_topic, 0, &voltage_cell1);
	oplus_gauge_get_volt(gauge_topic, 1, &voltage_cell2);

	batt_current = oplus_gauge_get_batt_current();

	btemp = oplus_vooc_check_bcc_temp();

	if (DEVICE_ZY0603 == gauge_type) {
		batt_dod0_passed_q = 0;
		buffer[17] = 1;
	}

	buffer[0] = batt_dod0_1;
	buffer[1] = batt_dod0_2;
	buffer[2] = batt_dod0_passed_q;
	buffer[3] = batt_qmax_1;
	buffer[4] = batt_qmax_2;
	buffer[5] = batt_qmax_passed_q;
	buffer[6] = voltage_cell1;
	buffer[7] = btemp;
	buffer[8] = batt_current;
	buffer[9] = bcc_current_max;
	buffer[10] = bcc_current_min;
	buffer[11] = voltage_cell2;
	buffer[12] = soc_ext_1;
	buffer[13] = soc_ext_2;
	buffer[14] = atl_last_gear_current;

	return;
}

int oplus_smart_chg_get_battery_bcc_parameters(char *buf)
{
	unsigned char *tmpbuf;
	unsigned int buffer[BCC_PARMS_COUNT] = { 0 };
	int len = 0;
	int i = 0;
	int idx = 0;
	struct oplus_mms *wired_topic;
	bool vooc_get_fastchg_ing;
	int vooc_get_fast_chg_type;
	bool voocphy_get_fastchg_ing;
	int vooc_check_bcc_temp_range;
	bool wls_fastchg_charging;

	if ((NULL == buf) || (NULL == g_smart_chg)) {
		chg_err("input buf or g_smart_chg error");
		return -ENOMEM;
	}

	oplus_smart_chg_bcc_set_buffer(buffer);
	vooc_get_fastchg_ing = oplus_vooc_get_fastchg_ing();
	vooc_get_fast_chg_type = oplus_vooc_get_fast_chg_type();
	voocphy_get_fastchg_ing = oplus_voocphy_get_fastchg_ing();
	vooc_check_bcc_temp_range = oplus_vooc_check_bcc_temp_range();
	wls_fastchg_charging = oplus_wls_get_fastchg_ing();

	if ((vooc_get_fastchg_ing && vooc_get_fast_chg_type != BCC_TYPE_IS_VOOC) ||
	    voocphy_get_fastchg_ing || g_smart_chg->ufcs_charging || wls_fastchg_charging) {
		buffer[15] = 1;
	} else {
		buffer[15] = 0;
	}

	if (buffer[9] == 0) {
		buffer[15] = 0;
	}

	wired_topic = g_smart_chg->wired_topic;
	if (NULL == wired_topic) {
		chg_err("input wired_topic error");
		return -ENOMEM;
	}

	tmpbuf = (unsigned char *)get_zeroed_page(GFP_KERNEL);
	if (NULL == tmpbuf) {
		chg_err("input tmpbuf error");
		return -ENOMEM;
	}

	buffer[16] = oplus_wired_get_bcc_curr_done_status(wired_topic);

	if (voocphy_get_fastchg_ing ||
	    (vooc_get_fastchg_ing && (vooc_get_fast_chg_type != BCC_TYPE_IS_VOOC))) {
		if (vooc_check_bcc_temp_range == BCC_TEMP_RANGE_WRONG) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	} else if (g_smart_chg->ufcs_charging) {
		if (!oplus_ufcs_check_bcc_temp_range(g_smart_chg)) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	} else if (wls_fastchg_charging) {
		if (!oplus_wls_check_bcc_temp_range(g_smart_chg)) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	}

	chg_info("----dod0_1[%d], dod0_2[%d], dod0_passed_q[%d], qmax_1[%d], qmax_2[%d], qmax_passed_q[%d], "
		"voltage_cell1[%d], temperature[%d], batt_current[%d], max_current[%d], min_current[%d], voltage_cell2[%d], "
		"soc_ext_1[%d], soc_ext_2[%d], atl_last_geat_current[%d], charging_flag[%d], bcc_curr_done[%d], is_zy0603[%d], "
		"batt_type[%d]\n bcc_voocphy buf:%d,%d,%d,%d\n bcc_ufcs buf:%d %d\n bcc_wls buf:%d %d\n",
		buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],
		buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15], buffer[16],
		buffer[17], buffer[18], vooc_get_fastchg_ing, vooc_get_fast_chg_type, vooc_check_bcc_temp_range,
		voocphy_get_fastchg_ing, g_smart_chg->ufcs_charging, oplus_ufcs_check_bcc_temp_range(g_smart_chg),
		wls_fastchg_charging, oplus_wls_check_bcc_temp_range(g_smart_chg));

	for (i = 0; i < BCC_PARMS_COUNT - 1; i++) {
		len = snprintf(tmpbuf, BCC_PAGE_SIZE - idx, "%d,", buffer[i]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
	}
	len = snprintf(tmpbuf, BCC_PAGE_SIZE - idx, "%d", buffer[i]);
	memcpy(&buf[idx], tmpbuf, len);

	free_page((unsigned long)tmpbuf);


#ifdef BCC_SET_DEBUG_PARMS
	if (bcc_debug_mode & BCC_Y_DEBUG) {
		memcpy(&buf[0], bcc_debug_buf, BCC_PAGE_SIZE);
		chg_info("bcc_debug_buf:%s\n", bcc_debug_buf);
		return 0;
	}
#endif

	chg_info("buf:%s\n", buf);
	return 0;
}

static void oplus_smart_chg_bcc_pre_calibration(int *buffer)
{
	if (!g_smart_chg || (NULL == buffer)) {
		chg_err("input g_smart_chg or buffer error");
		return;
	}

	memcpy(g_smart_chg->bcc_buf, buffer, BCC_PARMS_COUNT_LEN);
}

int oplus_smart_chg_get_fastchg_battery_bcc_parameters(char *buf)
{
	unsigned int buffer[BCC_PARMS_COUNT] = {0};
	struct oplus_mms *wired_topic;

	if (!g_smart_chg || (NULL == buf)) {
		chg_err("input g_smart_chg or buf error");
		return -ENOMEM;
	}

	wired_topic = g_smart_chg->wired_topic;
	if (NULL == wired_topic) {
		chg_err("input wired_topic error");
		return -ENOMEM;
	}

	oplus_smart_chg_bcc_set_buffer(buffer);

	if (oplus_vooc_get_fastchg_ing() && oplus_vooc_get_fast_chg_type() == BCC_TYPE_IS_SVOOC) {
		buffer[15] = 1;
	} else {
		buffer[15] = 0;
	}

	if (buffer[9] == 0) {
		buffer[15] = 0;
	}

	buffer[16] = oplus_wired_get_bcc_curr_done_status(wired_topic);

	chg_info("----dod0_1[%d], dod0_2[%d], dod0_passed_q[%d], qmax_1[%d], qmax_2[%d], qmax_passed_q[%d], "
		"voltage_cell1[%d], temperature[%d], batt_current[%d], max_current[%d], min_current[%d], voltage_cell2[%d], "
		"soc_ext_1[%d], soc_ext_2[%d], atl_last_geat_current[%d], charging_flag[%d], bcc_curr_done[%d], "
		"is_zy0603[%d], batt_type[%d]\n",
		buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],
		buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15], buffer[16],
		buffer[17], buffer[18]);

	oplus_smart_chg_bcc_pre_calibration(buffer);

	chg_info("buf:%s\n", buf);
	return 0;
}

int oplus_smart_chg_get_prev_battery_bcc_parameters(char *buf)
{
	int i = 0;
	int idx = 0;
	unsigned char *tmpbuf;
	int len = 0;
	struct oplus_mms *wired_topic;
	bool vooc_get_fastchg_ing;
	int vooc_get_fast_chg_type;
	bool voocphy_get_fastchg_ing;
	int vooc_check_bcc_temp_range;
	unsigned int buffer[BCC_PARMS_COUNT] = { 0 };
	bool wls_fastchg_charging;

	if (!g_smart_chg || (NULL == buf)) {
		chg_err("input g_smart_chg or buf error");
		return -ENOMEM;
	}
	wired_topic = g_smart_chg->wired_topic;

	if (NULL == wired_topic) {
		chg_err("input wired_topic error!");
		return -ENOMEM;
	}

	tmpbuf = (unsigned char *)get_zeroed_page(GFP_KERNEL);
	if (NULL == tmpbuf) {
		chg_err("input tmpbuf error !");
		return -ENOMEM;
	}

	memcpy(buffer, g_smart_chg->bcc_buf, BCC_PARMS_COUNT_LEN);
	vooc_get_fastchg_ing = oplus_vooc_get_fastchg_ing();
	vooc_get_fast_chg_type = oplus_vooc_get_fast_chg_type();
	voocphy_get_fastchg_ing = oplus_voocphy_get_fastchg_ing();
	vooc_check_bcc_temp_range = oplus_vooc_check_bcc_temp_range();
	wls_fastchg_charging = oplus_wls_get_fastchg_ing();

	if ((vooc_get_fastchg_ing && vooc_get_fast_chg_type != BCC_TYPE_IS_VOOC) ||
	    voocphy_get_fastchg_ing) {
		buffer[9] = oplus_vooc_check_bcc_max_curr();
		buffer[10] = oplus_vooc_check_bcc_min_curr();
	} else if (g_smart_chg->ufcs_charging) {
		buffer[9] = oplus_ufcs_get_bcc_max_curr(g_smart_chg);
		buffer[10] = oplus_ufcs_get_bcc_min_curr(g_smart_chg);
	} else if (g_smart_chg->wls_online) {
		buffer[9] = oplus_wls_get_bcc_max_curr(g_smart_chg);
		buffer[10] = oplus_wls_get_bcc_min_curr(g_smart_chg);
	}

	if ((vooc_get_fastchg_ing && vooc_get_fast_chg_type != BCC_TYPE_IS_VOOC) ||
	    voocphy_get_fastchg_ing || g_smart_chg->ufcs_charging) {
		buffer[15] = 1;
	} else {
		buffer[15] = 0;
	}

	if (buffer[9] == 0) {
		buffer[15] = 0;
	}

	buffer[16] = oplus_wired_get_bcc_curr_done_status(wired_topic);

	if ((vooc_get_fastchg_ing && vooc_get_fast_chg_type != BCC_TYPE_IS_VOOC) ||
	    voocphy_get_fastchg_ing) {
		if (vooc_check_bcc_temp_range == BCC_TEMP_RANGE_WRONG) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	} else if (g_smart_chg->ufcs_charging) {
		if (!oplus_ufcs_check_bcc_temp_range(g_smart_chg)) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	} else if (wls_fastchg_charging) {
		if (!oplus_wls_check_bcc_temp_range(g_smart_chg)) {
			buffer[9] = 0;
			buffer[10] = 0;
			buffer[14] = 0;
			buffer[15] = 0;
		}
	}

	chg_info("bcc_voocphy buf:%d,%d,%d,%d\n", vooc_get_fastchg_ing,
		vooc_get_fast_chg_type, vooc_check_bcc_temp_range, voocphy_get_fastchg_ing);

	for (i = 0; i < BCC_PARMS_COUNT - 1; i++) {
		len = snprintf(tmpbuf, BCC_PAGE_SIZE - idx, "%d,", buffer[i]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
	}
	len = snprintf(tmpbuf, BCC_PAGE_SIZE - idx, "%d", buffer[i]);
	memcpy(&buf[idx], tmpbuf, len);

	free_page((unsigned long)tmpbuf);

#ifdef BCC_SET_DEBUG_PARMS
	if (bcc_debug_mode & BCC_Y_DEBUG) {
		memcpy(&buf[0], bcc_debug_buf, BCC_PAGE_SIZE);
		chg_info("bcc_debug_buf:%s\n", bcc_debug_buf);
		return 0;
	}
#endif

	chg_info("bcc_buf:%s\n", buf);
	return 0;
}

int oplus_smart_chg_set_bcc_debug_parameters(const char *buf)
{
	int ret = 0;
#ifdef BCC_SET_DEBUG_PARMS
	char temp_buf[10] = {0};
#endif

	if (NULL == buf) {
		return -ENOMEM;
	}

#ifdef BCC_SET_DEBUG_PARMS
	if (strlen(buf) <= BCC_PAGE_SIZE) {
		if (strncpy(temp_buf, buf, 7)) {
			chg_info("temp_buf:%s\n", temp_buf);
		}
		if (!strncmp(temp_buf, "Y_DEBUG", 7)) {
			bcc_debug_mode = BCC_Y_DEBUG;
			chg_info("BCC_Y_DEBUG:%d\n", bcc_debug_mode);
		} else {
			bcc_debug_mode = BCC_N_DEBUG;
			chg_info("BCC_N_DEBUG:%d\n", bcc_debug_mode);
		}
		strncpy(bcc_debug_buf, buf + 8, strlen(buf) - 8);
		chg_info("bcc_debug_buf:%s, temp_buf:%s\n",
			 bcc_debug_buf, temp_buf);
		return ret;
	}
#endif

	chg_info("buf:%s\n", buf);
	return ret;
}

static int oplus_smart_charge_probe(struct platform_device *pdev)
{
	struct oplus_smart_charge *smart_chg;

	if (pdev == NULL) {
		chg_err("oplus_smart_charge_probe input pdev error\n");
		return -ENODEV;
	}

	smart_chg = devm_kzalloc(&pdev->dev, sizeof(struct oplus_smart_charge), GFP_KERNEL);
	if (smart_chg == NULL) {
		chg_err("alloc smart_chg buffer error\n");
		return 0;
	}
	smart_chg->dev = &pdev->dev;
	platform_set_drvdata(pdev, smart_chg);

	INIT_WORK(&smart_chg->quick_mode_check_work, oplus_smart_chg_quick_mode_check_work);
	oplus_mms_wait_topic("common", oplus_smart_chg_common_topic_ready, smart_chg);
	g_smart_chg = smart_chg;

	oplus_smart_charge_parse_dt(smart_chg);

	return 0;
}

static int oplus_smart_charge_remove(struct platform_device *pdev)
{
	struct oplus_smart_charge *smart_chg = platform_get_drvdata(pdev);

	if (!IS_ERR_OR_NULL(smart_chg->gauge_subs))
		oplus_mms_unsubscribe(smart_chg->gauge_subs);
	if (!IS_ERR_OR_NULL(smart_chg->vooc_subs))
		oplus_mms_unsubscribe(smart_chg->vooc_subs);
	if (!IS_ERR_OR_NULL(smart_chg->wired_subs))
		oplus_mms_unsubscribe(smart_chg->wired_subs);
	if (!IS_ERR_OR_NULL(smart_chg->ufcs_subs))
		oplus_mms_unsubscribe(smart_chg->ufcs_subs);
	devm_kfree(&pdev->dev, smart_chg);

	return 0;
}

static const struct of_device_id oplus_smart_charge_match[] = {
	{ .compatible = "oplus,smart_charge" },
	{},
};

static struct platform_driver oplus_smart_charge_driver = {
	.driver		= {
		.name = "oplus-smart_charge",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_smart_charge_match),
	},
	.probe		= oplus_smart_charge_probe,
	.remove		= oplus_smart_charge_remove,
};

/* never return an error value */
static __init int oplus_smart_charge_init(void)
{
	return platform_driver_register(&oplus_smart_charge_driver);
}

static __exit void oplus_smart_charge_exit(void)
{
	platform_driver_unregister(&oplus_smart_charge_driver);
}

oplus_chg_module_register(oplus_smart_charge);

/* smart charge module api */
int oplus_smart_chg_set_normal_current(int curr)
{
	bool led_on = false;
	union mms_msg_data data = { 0 };
	int rc;

	if (g_smart_chg == NULL)
		return -ENODEV;

	if (g_smart_chg->comm_topic) {
		oplus_mms_get_item_data(g_smart_chg->comm_topic, COMM_ITEM_LED_ON, &data, false);
		led_on = !!data.intval;
	}
	if (led_on)
		return 0;
	if (g_smart_chg->smart_normal_cool_down != 0)
		return 0;

	/* TODO: PPS */
	if (g_smart_chg->vooc_topic) {
		rc = oplus_vooc_current_to_level(g_smart_chg->vooc_topic, curr);
		if (rc < 0) {
			chg_err("can't get normal_cool_down, curr=%d, rc=%d\n",
				curr, rc);
			return rc;
		}
		g_smart_chg->normal_cool_down = rc;
	}
	chg_info("set normal_cool_down=%d\n", g_smart_chg->normal_cool_down);

	return 0;
}

int oplus_smart_chg_set_normal_cool_down(int cool_down)
{
	if (g_smart_chg == NULL)
		return -ENODEV;

	g_smart_chg->smart_normal_cool_down = cool_down;
	g_smart_chg->normal_cool_down = cool_down;
	chg_info("set normal_cool_down=%d\n", cool_down);

	return 0;
}

int oplus_smart_chg_get_normal_cool_down(void)
{
	if (g_smart_chg == NULL)
		return -ENODEV;

	return g_smart_chg->normal_cool_down;
}

long oplus_smart_chg_get_quick_mode_time_gain(void)
{
	long total_time, gain_time;

	if (g_smart_chg == NULL)
		return -ENODEV;

	total_time = g_smart_chg->quick_mode_time.tv_sec - g_smart_chg->start_time;
	gain_time = g_smart_chg->quick_mode_gain_time_ms / 1000;
	if (gain_time < 0)
		gain_time = 0;
	chg_info("total_time:%ld, gain_time:%ld, quick_mode_gain_time_ms:%ld\n",
		 total_time, gain_time, g_smart_chg->quick_mode_gain_time_ms);

	return gain_time;
}

int oplus_smart_chg_get_quick_mode_percent_gain(void)
{
	int percent;
	long total_time, gain_time;

	if (g_smart_chg == NULL)
		return -ENODEV;

	total_time = g_smart_chg->quick_mode_time.tv_sec - g_smart_chg->start_time;
	gain_time = g_smart_chg->quick_mode_gain_time_ms / 1000;
	percent = (gain_time * 100) / (total_time + gain_time);
	chg_info("total_time:%ld, gain_time:%ld, percent:%d\n",
		 total_time, gain_time, percent);

	return percent;
}

int oplus_smart_chg_get_soh_support(void)
{
	if (g_smart_chg == NULL)
		return -ENODEV;

	return g_smart_chg->smart_chg_soh_support;
}
