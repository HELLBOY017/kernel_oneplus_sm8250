#ifndef __OPLUS_MONITOR_INTERNAL_H__
#define __OPLUS_MONITOR_INTERNAL_H__

#include <linux/device.h>
#include <oplus_chg.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_monitor.h>
#include "oplus_chg_track.h"

struct oplus_monitor {
	struct device *dev;
	struct oplus_mms *err_topic;
	struct oplus_mms *wired_topic;
	struct oplus_mms *wls_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *main_gauge_topic;
	struct oplus_mms *sub_gauge_topic;
	struct oplus_mms *dual_chan_topic;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *wls_subs;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *vooc_subs;
	struct mms_subscribe *dual_chan_subs;
	struct mms_subscribe *comm_subs;
	struct oplus_mms *ufcs_topic;
	struct mms_subscribe *ufcs_subs;

	struct oplus_chg_track *track;

	struct work_struct charge_info_update_work;
	struct work_struct wired_plugin_work;
	struct work_struct ffc_step_change_work;
	struct work_struct ffc_end_work;
	struct votable *fv_votable;
	struct delayed_work water_inlet_detect_work;
	struct votable *wired_icl_votable;
	struct votable *wired_fcc_votable;
	struct votable *wired_charge_suspend_votable;
	struct votable *wired_charging_disable_votable;
	struct votable *wls_icl_votable;
	struct votable *wls_fcc_votable;
	struct votable *wls_charge_suspend_votable;
	struct votable *wls_charging_disable_votable;
	struct votable *chg_disable_votable;
	struct votable *vooc_curr_votable;

	/* battery */
	int vbat_mv;
	int vbat_min_mv;
	int ibat_ma;
	int batt_temp;
	int shell_temp;
	int batt_fcc;
	int batt_rm;
	int batt_cc;
	int batt_soh;
	int batt_soc;
	int ui_soc;
	int smooth_soc;
	int soc_load;
	int batt_status;
	bool batt_exist;
	bool batt_full;
	unsigned int batt_err_code;

	/* charge */
	int fcc_ma;
	int fv_mv;
	int total_time;
	bool chg_disable;
	bool chg_user_disable;
	bool sw_full;
	bool hw_full_by_sw;
	bool hw_full;

	/* wired */
	int wired_ibus_ma;
	int wired_vbus_mv;
	int wired_icl_ma;
	int wired_charge_type;
	int cc_mode;
	unsigned int wired_err_code;
	bool wired_online;
	bool wired_suspend;
	bool wired_user_suspend;
	int cc_detect;
	bool otg_enable;
	bool pd_svooc;
	bool liquid_inlet_detection_switch;
	int water_inlet_plugin_count;

	/* wireless */
	int wls_iout_ma;
	int wls_vout_mv;
	int wls_icl_ma;
	int wls_charge_type;
	unsigned int wls_err_code;
	bool wls_online;
	bool wls_suspend;
	bool wls_user_suspend;

	/* common */
	enum oplus_temp_region temp_region;
	enum oplus_chg_ffc_status ffc_status;
	int cool_down;
	int normal_cool_down;
	unsigned int notify_code;
	unsigned int notify_flag;
	bool led_on;
	bool rechging;
	bool ui_soc_ready;
	int chg_cycle_status;
	bool slow_chg_enable;
	int slow_chg_pct;
	int slow_chg_watt;
	int bcc_current;
	int mmi_chg;
	int usb_status;
	bool otg_switch_status;

	/* vooc */
	bool vooc_online;
	bool vooc_started;
	bool vooc_charging;
	bool vooc_online_keep;
	unsigned vooc_sid;
	unsigned pre_vooc_sid;
	bool chg_ctrl_by_vooc;

	/* ufcs */
	bool ufcs_online;
	bool ufcs_charging;
	u32 ufcs_adapter_id;
	bool ufcs_oplus_adapter;
};

#endif /* __OPLUS_MONITOR_INTERNAL_H__ */
