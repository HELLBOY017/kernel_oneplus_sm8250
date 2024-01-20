#define pr_fmt(fmt) "[TRACK]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/iio/consumer.h>
#include <linux/power_supply.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/rtc.h>
#include <linux/ktime.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/sched/clock.h>
#include <linux/string.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#endif
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) ||	\
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE)
#include <soc/oplus/system/kernel_fb.h>
#elif defined(CONFIG_OPLUS_KEVENT_UPLOAD)
#include <linux/oplus_kevent.h>
#endif
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/oplus_project.h>
#endif

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_mms_gauge.h>
#include <oplus_chg_vooc.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_exception.h>
#include <oplus_chg_wls.h>
#include "oplus_monitor_internal.h"
#include "oplus_chg_track.h"
#include "oplus_mms_wired.h"

#define OPLUS_CHG_TRACK_WAIT_TIME_MS			3000
#define OPLUS_CHG_UPDATE_INFO_DELAY_MS			500
#define OPLUS_CHG_TRIGGER_MSG_LEN			(1024 * 2)
#define OPLUS_CHG_TRIGGER_REASON_TAG_LEN		32
#define OPLUS_CHG_TRACK_LOG_TAG				"OplusCharger"
#define OPLUS_CHG_TRACK_EVENT_ID			"charge_monitor"
#define OPLUS_CHG_TRACK_DWORK_RETRY_CNT			3

#define OPLUS_CHG_TRACK_UI_SOC_LOAD_JUMP_THD		5
#define OPLUS_CHG_TRACK_SOC_JUMP_THD			3
#define OPLUS_CHG_TRACK_UI_SOC_JUMP_THD			5
#define OPLUS_CHG_TRACK_UI_SOC_TO_SOC_JUMP_THD		3
#define OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID		0XFF

#define OPLUS_CHG_TRACK_POWER_TYPE_LEN			24
#define OPLUS_CHG_TRACK_BATT_FULL_REASON_LEN		24
#define OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LEN		24
#define OPLUS_CHG_TRACK_COOL_DOWN_STATS_LEN		24
#define OPLUS_CHG_TRACK_COOL_DOWN_PACK_LEN		320
#define OPLUS_CHG_TRACK_FASTCHG_BREAK_REASON_LEN	24
#define OPLUS_CHG_TRACK_VOOCPHY_NAME_LEN		16
#define OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LENS	160

#define TRACK_WLS_ADAPTER_TYPE_UNKNOWN 			0x00
#define TRACK_WLS_ADAPTER_TYPE_VOOC 			0x01
#define TRACK_WLS_ADAPTER_TYPE_SVOOC 			0x02
#define TRACK_WLS_ADAPTER_TYPE_USB 			0x03
#define TRACK_WLS_ADAPTER_TYPE_NORMAL 			0x04
#define TRACK_WLS_ADAPTER_TYPE_EPP 			0x05
#define TRACK_WLS_ADAPTER_TYPE_SVOOC_50W 		0x06
#define TRACK_WLS_ADAPTER_TYPE_PD_65W 			0x07

#define TRACK_WLS_DOCK_MODEL_0				0x00
#define TRACK_WLS_DOCK_MODEL_1				0x01
#define TRACK_WLS_DOCK_MODEL_2				0x02
#define TRACK_WLS_DOCK_MODEL_3				0x03
#define TRACK_WLS_DOCK_MODEL_4				0x04
#define TRACK_WLS_DOCK_MODEL_5				0x05
#define TRACK_WLS_DOCK_MODEL_6				0x06
#define TRACK_WLS_DOCK_MODEL_7				0x07
#define TRACK_WLS_DOCK_MODEL_8				0x08
#define TRACK_WLS_DOCK_MODEL_9				0x09
#define TRACK_WLS_DOCK_MODEL_A				0x0a
#define TRACK_WLS_DOCK_MODEL_B				0x0b
#define TRACK_WLS_DOCK_MODEL_10				0x10
#define TRACK_WLS_DOCK_MODEL_11				0x11
#define TRACK_WLS_DOCK_MODEL_12				0x12
#define TRACK_WLS_DOCK_MODEL_13				0x13
#define TRACK_WLS_DOCK_THIRD_PARTY			0x1F

#define TRACK_POWER_2000MW				2000
#define TRACK_POWER_2500MW				2500
#define TRACK_POWER_5000MW				5000
#define TRACK_POWER_6000MW				6000
#define TRACK_POWER_7000MW				7000
#define TRACK_POWER_7500MW				7500
#define TRACK_POWER_10000MW				10000
#define TRACK_POWER_12000MW				12000
#define TRACK_POWER_15000MW				15000
#define TRACK_POWER_18000MW				18000
#define TRACK_POWER_20000MW				20000
#define TRACK_POWER_30000MW				30000
#define TRACK_POWER_40000MW				40000
#define TRACK_POWER_50000MW				50000
#define TRACK_POWER_65000MW				65000
#define TRACK_POWER_100000MW				100000
#define TRACK_POWER_120000MW				120000
#define TRACK_POWER_150000MW				150000

#define TRACK_INPUT_VOL_5000MV				5000
#define TRACK_INPUT_VOL_10000MV				10000
#define TRACK_INPUT_VOL_11000MV				11000
#define TRACK_INPUT_CURR_3000MA				3000
#define TRACK_INPUT_CURR_4000MA				4000
#define TRACK_INPUT_CURR_5000MA				5000
#define TRACK_INPUT_CURR_6000MA				6000
#define TRACK_INPUT_CURR_6100MA				6100
#define TRACK_INPUT_CURR_6500MA				6500
#define TRACK_INPUT_CURR_7300MA				7300
#define TRACK_INPUT_CURR_8000MA				8000
#define TRACK_INPUT_CURR_9100MA				9100
#define TRACK_INPUT_CURR_11000MA			11000
#define TRACK_INPUT_CURR_11400MA			11400

#define TRACK_CYCLE_RECORDIING_TIME_2MIN		120
#define TRACK_CYCLE_RECORDIING_TIME_90S			90

#define TRACK_TIME_1MIN_THD				60
#define TRACK_TIME_30MIN_THD				1800
#define TRACK_TIME_1HOU_THD				3600
#define TRACK_TIME_7S_JIFF_THD				(7 * 1000)
#define TRACK_TIME_500MS_JIFF_THD			(5 * 100)
#define TRACK_TIME_1000MS_JIFF_THD			(1 * 1000)
#define TRACK_TIME_5MIN_JIFF_THD			(5 * 60 * 1000)
#define TRACK_TIME_10MIN_JIFF_THD			(10 * 60 * 1000)
#define TRACK_TIME_SCHEDULE_UI_SOC_LOAD_JUMP		90000
#define TRACK_THRAD_PERIOD_TIME_S			5
#define TRACK_NO_CHRGING_TIME_PCT			70
#define TRACK_COOLDOWN_CHRGING_TIME_PCT			70
#define TRACK_WLS_SKEWING_CHRGING_TIME_PCT		2

#define TRACK_PERIOD_CHG_CAP_MAX_SOC			100
#define TRACK_PERIOD_CHG_CAP_INIT			(-101)
#define TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT		(-10000)
#define TRACK_T_THD_1000_MS				1000
#define TRACK_T_THD_500_MS				500
#define TRACK_T_THD_6000_MS				6000

#define TRACK_LOCAL_T_NS_TO_MS_THD			1000000
#define TRACK_LOCAL_T_NS_TO_S_THD			1000000000

#define TRACK_LED_MONITOR_SOC_POINT			98
#define TRACK_CHG_VOOC_BATT_VOL_DIFF_MV			100

#define TRACK_REF_SOC_50				50
#define TRACK_REF_SOC_75				75
#define TRACK_REF_SOC_90				90
#define TRACK_REF_VOL_5000MV				5000
#define TRACK_REF_VOL_10000MV				10000
#define TRACK_REF_TIME_6S				6
#define TRACK_REF_TIME_8S				8
#define TRACK_REF_TIME_10S				10
#define TRACK_HIDL_PARALLELCHG_FOLDMODE_INFO_LEN	512
#define TRACK_HIDL_TTF_INFO_LEN				512

#define DEBUG_WATER_INLET_PARA (3 * 60 * 60 * 1000) /* 3 hours */
#define TRACK_HIDL_DATA_LEN			512
#define TRACK_HIDL_UISOH_INFO_LEN		512
#define TRACK_SOFT_ABNORMAL_UPLOAD_PERIOD	(24 * 3600)
#define TRACK_SOFT_SOH_UPLOAD_COUNT_MAX		2000

#define TRACK_APP_REAL_NAME_LEN			48
#define TRACK_APP_TOP_INDEX_DEFAULT		255
#define TRACK_APP_REAL_NAME_DEFAULT		"com.android.launcher"

enum oplus_chg_track_voocphy_type {
	TRACK_NO_VOOCPHY = 0,
	TRACK_ADSP_VOOCPHY,
	TRACK_AP_SINGLE_CP_VOOCPHY,
	TRACK_AP_DUAL_CP_VOOCPHY,
	TRACK_MCU_VOOCPHY,
};

enum oplus_chg_track_fastchg_status {
	TRACK_FASTCHG_STATUS_UNKOWN = 0,
	TRACK_FASTCHG_STATUS_START,
	TRACK_FASTCHG_STATUS_NORMAL,
	TRACK_FASTCHG_STATUS_WARM,
	TRACK_FASTCHG_STATUS_DUMMY,
};

enum oplus_chg_track_power_type {
	TRACK_CHG_TYPE_UNKNOW,
	TRACK_CHG_TYPE_WIRE,
	TRACK_CHG_TYPE_WIRELESS,
};

enum oplus_chg_track_soc_section {
	TRACK_SOC_SECTION_DEFAULT,
	TRACK_SOC_SECTION_LOW,
	TRACK_SOC_SECTION_MEDIUM,
	TRACK_SOC_SECTION_HIGH,
	TRACK_SOC_SECTION_OVER,
};

enum oplus_chg_track_hidl_type {
	TRACK_HIDL_DEFAULT,
	TRACK_HIDL_BCC_INFO,
	TRACK_HIDL_BCC_ERR,
	TRACK_HIDL_BMS_INFO,
	TRACK_HIDL_HYPER_INFO,
	TRACK_HIDL_WLS_THIRD_ERR,
	TRACK_HIDL_UISOH_INFO,
	TRACK_HIDL_PARALLELCHG_FOLDMODE_INFO,
	TRACK_HIDL_TTF_INFO,
};

struct oplus_chg_track_app_ref {
	u8 *alias_name;
	u8 *real_name;
	u32 cont_time;
};

struct oplus_chg_track_app_status{
	struct mutex app_lock;
	u32 change_t;
	bool app_cal;
	u8 curr_top_index;
	u8 pre_top_name[TRACK_APP_REAL_NAME_LEN];
	u8 curr_top_name[TRACK_APP_REAL_NAME_LEN];
};

struct oplus_chg_track_vooc_type {
	int chg_type;
	int vol;
	int cur;
	char name[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
};

struct oplus_chg_track_type {
	int type;
	int power;
	char name[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
};

struct oplus_chg_track_wired_type {
	int power;
	int adapter_id;
	char adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
};

struct oplus_chg_track_wls_type {
	int power;
	char adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
	char dock_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
};

struct oplus_chg_track_power {
	int power_type;
	char power_mode[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
	struct oplus_chg_track_wired_type wired_info;
	struct oplus_chg_track_wls_type wls_info;
};

struct oplus_chg_track_batt_full_reason {
	int notify_flag;
	char reason[OPLUS_CHG_TRACK_BATT_FULL_REASON_LEN];
};

struct oplus_chg_track_chg_abnormal_reason {
	int notify_code;
	char reason[OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LEN];
	bool happened;
};

struct oplus_chg_track_cool_down_stats {
	int level;
	int time;
	char level_name[OPLUS_CHG_TRACK_COOL_DOWN_STATS_LEN];
};

struct oplus_chg_track_cfg {
	int track_ver;
	int fast_chg_break_t_thd;
	int general_chg_break_t_thd;
	int wls_chg_break_t_thd;
	int voocphy_type;
	int wired_fast_chg_scheme;
	int wls_fast_chg_scheme;
	int wls_epp_chg_scheme;
	int wls_bpp_chg_scheme;
	int wls_max_power;
	int wired_max_power;
	struct exception_data exception_data;
};

struct oplus_chg_track_fastchg_break {
	int code;
	char name[OPLUS_CHG_TRACK_FASTCHG_BREAK_REASON_LEN];
};

struct oplus_chg_track_voocphy_info {
	int voocphy_type;
	char name[OPLUS_CHG_TRACK_VOOCPHY_NAME_LEN];
};

struct oplus_chg_track_speed_ref {
	int ref_soc;
	int ref_power;
	int ref_curr;
};

struct oplus_chg_track_hidl_cmd {
	u32 cmd;
	u32 data_size;
	u8 data_buf[TRACK_HIDL_DATA_LEN];
};

struct oplus_chg_track_hidl_uisoh_info_cmd {
	u8 data_buf[TRACK_HIDL_UISOH_INFO_LEN];
};
struct oplus_chg_track_hidl_uisoh_info{
	struct mutex track_uisoh_info_lock;
	bool uisoh_info_uploading;
	oplus_chg_track_trigger *uisoh_info_load_trigger;
	struct delayed_work uisoh_info_load_trigger_work;
	struct oplus_chg_track_hidl_uisoh_info_cmd uisoh_info;
};

struct oplus_parallelchg_track_hidl_foldmode_info_cmd {
	u8 data_buf[TRACK_HIDL_PARALLELCHG_FOLDMODE_INFO_LEN];
};

struct oplus_parallelchg_track_hidl_foldmode_info {
	struct mutex track_lock;
	bool info_uploading;
	oplus_chg_track_trigger *load_trigger_info;
	struct delayed_work load_trigger_work;
	struct oplus_parallelchg_track_hidl_foldmode_info_cmd parallelchg_foldmode_info;
};

struct oplus_chg_track_hidl_ttf_info_cmd {
	u8 data_buf[TRACK_HIDL_TTF_INFO_LEN];
};

struct oplus_chg_track_hidl_ttf_info {
	struct mutex track_lock;
	bool info_uploading;
	oplus_chg_track_trigger *load_trigger_info;
	struct delayed_work load_trigger_work;
	struct oplus_chg_track_hidl_ttf_info_cmd ttf_info;
};

struct oplus_chg_track_status {
	int curr_soc;
	int pre_soc;
	int curr_smooth_soc;
	int pre_smooth_soc;
	int curr_uisoc;
	int pre_uisoc;
	int pre_vbatt;
	int pre_time_utc;
	int pre_rm;
	int pre_fcc;
	bool soc_jumped;
	bool uisoc_jumped;
	bool uisoc_to_soc_jumped;
	bool uisoc_load_jumped;

	u8 debug_soc;
	u8 debug_uisoc;
	u8 debug_normal_charging_state;
	u8 debug_fast_prop_status;
	u8 debug_normal_prop_status;
	u8 debug_no_charging;
	u8 debug_slow_charging_force;
	u32 debug_chg_notify_flag;
	u32 debug_chg_notify_code;
	u8 debug_slow_charging_reason;
	bool debug_close_3hours;
	u32 debug_water_inlet_para;
	u8 debug_plugout_state;
	u8 debug_break_code;

	struct oplus_chg_track_power power_info;
	struct mutex track_status_lock;
	int fast_chg_type;
	int pre_fastchg_type;
	int pre_wired_type;

	int chg_start_rm;
	int chg_start_soc;
	int chg_end_soc;
	int chg_start_temp;
	int batt_start_temp;
	int batt_max_temp;
	int batt_max_vol;
	int batt_max_curr;
	int chg_max_vol;
	int chg_start_time;
	int chg_end_time;
	int chg_fast_full_time;
	int chg_normal_full_time;
	int chg_report_full_time;
	bool has_judge_speed;
	bool aging_ffc_trig;
	int aging_ffc_judge_vol[FFC_CHG_STEP_MAX];
	int aging_ffc_start_time;
	int ffc_end_time;
	int ffc_time;
	int cv_time;
	int ffc_start_main_soc;
	int ffc_start_sub_soc;
	int ffc_end_main_soc;
	int ffc_end_sub_soc;
	int aging_ffc_to_full_time;
	int chg_plugin_utc_t;
	int chg_plugout_utc_t;
	int rechg_counts;
	int in_rechging;
	struct rtc_time chg_plugin_rtc_t;
	struct rtc_time chg_plugout_rtc_t;
	struct rtc_time mmi_chg_open_rtc_t;
	struct rtc_time mmi_chg_close_rtc_t;

	int chg_five_mins_cap;
	int chg_ten_mins_cap;
	int chg_average_speed;
	char batt_full_reason[OPLUS_CHG_TRACK_BATT_FULL_REASON_LEN];

	int chg_max_temp;
	int chg_no_charging_cnt;
	int continue_ledon_time;
	int continue_ledoff_time;
	int ledon_ave_speed;
	int ledoff_ave_speed;
	int ledon_rm;
	int ledoff_rm;
	int ledon_time;
	int ledoff_time;
	int led_on;
	int led_change_t;
	int prop_status;
	int led_change_rm;
	bool led_onoff_power_cal;
	int chg_total_cnt;
	unsigned long long chg_attach_time_ms;
	unsigned long long chg_detach_time_ms;
	unsigned long long wls_attach_time_ms;
	unsigned long long wls_detach_time_ms;
	struct oplus_chg_track_fastchg_break fastchg_break_info;
	char wired_break_crux_info[OPLUS_CHG_TRACK_CURX_INFO_LEN];
	char wls_break_crux_info[OPLUS_CHG_TRACK_CURX_INFO_LEN];

	bool mmi_chg;
	bool once_mmi_chg;
	int mmi_chg_open_t;
	int mmi_chg_close_t;
	int mmi_chg_constant_t;

	int slow_chg_open_t;
	int slow_chg_close_t;
	int slow_chg_open_n_t;
	int slow_chg_duration;
	int slow_chg_open_cnt;
	int slow_chg_watt;
	int slow_chg_pct;

	bool chg_speed_is_slow;
	bool tbatt_warm_once;
	bool tbatt_cold_once;
	int cool_down_effect_cnt;
	int cool_down_status;
	int cool_down_status_change_t;
	int soc_sect_status;
	int soc_low_sect_incr_rm;
	int soc_low_sect_cont_time;
	int soc_medium_sect_incr_rm;
	int soc_medium_sect_cont_time;
	int soc_high_sect_incr_rm;
	int soc_high_sect_cont_time;
	struct oplus_chg_track_speed_ref *wired_speed_ref;
	struct oplus_chg_track_speed_ref *wls_airvooc_speed_ref;
	struct oplus_chg_track_speed_ref *wls_epp_speed_ref;
	struct oplus_chg_track_speed_ref *wls_bpp_speed_ref;

	bool wls_status_keep;
	bool wls_need_upload;
	bool wired_need_upload;
	int wls_prop_status;
	int wls_skew_effect_cnt;
	bool chg_verity;
	char chg_abnormal_reason[OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LENS];
	struct oplus_chg_track_hidl_uisoh_info uisoh_info_s;
	struct oplus_parallelchg_track_hidl_foldmode_info parallelchg_info;
	struct oplus_chg_track_hidl_ttf_info *ttf_info;
	int wired_max_power;
	int wls_max_power;
	struct oplus_chg_track_app_status app_status;
	int once_chg_cycle_status;
	int dual_chan_start_time;
	int dual_chan_time;
	int dual_chan_open_count;
	int track_gmtoff;
};

struct oplus_chg_track {
	struct device *dev;
	struct oplus_monitor *monitor;
	struct task_struct *track_upload_kthread;

	struct mms_subscribe *err_subs;

	bool trigger_data_ok;
	struct mutex upload_lock;
	struct mutex trigger_data_lock;
	struct mutex trigger_ack_lock;
	oplus_chg_track_trigger trigger_data;
	struct completion trigger_ack;
	wait_queue_head_t upload_wq;

	struct workqueue_struct *trigger_upload_wq;
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
	struct kernel_packet_info *dcs_info;
#endif
	struct delayed_work upload_info_dwork;
	struct mutex dcs_info_lock;
	int dwork_retry_cnt;

	struct oplus_chg_track_status track_status;
	struct oplus_chg_track_cfg track_cfg;

	int uisoc_1_start_vbatt_max;
	int uisoc_1_start_vbatt_min;
	int uisoc_1_start_batt_rm;
	int uisoc_1_start_time;

	oplus_chg_track_trigger uisoc_load_trigger;
	oplus_chg_track_trigger soc_trigger;
	oplus_chg_track_trigger uisoc_trigger;
	oplus_chg_track_trigger uisoc_to_soc_trigger;
	oplus_chg_track_trigger charger_info_trigger;
	oplus_chg_track_trigger no_charging_trigger;
	oplus_chg_track_trigger slow_charging_trigger;
	oplus_chg_track_trigger charging_break_trigger;
	oplus_chg_track_trigger wls_charging_break_trigger;
	oplus_chg_track_trigger usbtemp_load_trigger;
	oplus_chg_track_trigger vbatt_too_low_load_trigger;
	oplus_chg_track_trigger vbatt_diff_over_load_trigger;
	oplus_chg_track_trigger uisoc_keep_1_t_load_trigger;
	oplus_chg_track_trigger ic_err_msg_load_trigger;
	oplus_chg_track_trigger chg_into_liquid_load_trigger;
	oplus_chg_track_trigger plugout_state_trigger;
	oplus_chg_track_trigger dual_chan_err_load_trigger;
	struct delayed_work uisoc_load_trigger_work;
	struct delayed_work soc_trigger_work;
	struct delayed_work uisoc_trigger_work;
	struct delayed_work uisoc_to_soc_trigger_work;
	struct delayed_work charger_info_trigger_work;
	struct delayed_work cal_chg_five_mins_capacity_work;
	struct delayed_work cal_chg_ten_mins_capacity_work;
	struct delayed_work no_charging_trigger_work;
	struct delayed_work slow_charging_trigger_work;
	struct delayed_work charging_break_trigger_work;
	struct delayed_work wls_charging_break_trigger_work;
	struct delayed_work usbtemp_load_trigger_work;
	struct delayed_work vbatt_too_low_load_trigger_work;
	struct delayed_work vbatt_diff_over_load_trigger_work;
	struct delayed_work uisoc_keep_1_t_load_trigger_work;
	struct delayed_work ic_err_msg_trigger_work;
	struct delayed_work chg_into_liquid_trigger_work;
	struct delayed_work chg_into_liquid_trigger_work_timeout;
	struct delayed_work plugout_state_work;
	struct delayed_work dual_chan_err_load_trigger_work;

	oplus_chg_track_trigger *mmi_chg_info_trigger;
	oplus_chg_track_trigger *slow_chg_info_trigger;
	oplus_chg_track_trigger *chg_cycle_info_trigger;
	oplus_chg_track_trigger *wls_info_trigger;
	oplus_chg_track_trigger *ufcs_info_trigger;
	struct delayed_work mmi_chg_info_trigger_work;
	struct delayed_work slow_chg_info_trigger_work;
	struct delayed_work chg_cycle_info_trigger_work;
	struct delayed_work wls_info_trigger_work;
	struct delayed_work ufcs_info_trigger_work;
	struct mutex mmi_chg_info_lock;
	struct mutex slow_chg_info_lock;
	struct mutex chg_cycle_info_lock;
	struct mutex wls_info_lock;
	struct mutex ufcs_info_lock;

	char voocphy_name[OPLUS_CHG_TRACK_VOOCPHY_NAME_LEN];

	struct mutex access_lock;

	bool oplus_liquid_intake_enable;
};

struct type_reason_table {
	int type_reason;
	char type_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN];
};

struct flag_reason_table {
	int flag_reason;
	char flag_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN];
};

static struct oplus_chg_track *g_track_chip;
static struct dentry *track_debugfs_root;
static struct oplus_chg_track_status *temp_track_status;
static DEFINE_MUTEX(debugfs_root_mutex);

#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
static int oplus_chg_track_pack_dcs_info(struct oplus_chg_track *chip);
#endif
static int
oplus_chg_track_get_charger_type(struct oplus_monitor *monitor,
				 struct oplus_chg_track_status *track_status,
				 int type);
static int oplus_chg_track_obtain_wls_break_sub_crux_info(
	struct oplus_chg_track *track_chip, char *crux_info);
static int oplus_chg_track_upload_trigger_data(oplus_chg_track_trigger data);
static int oplus_chg_track_get_local_time_s(void);
static bool oplus_chg_track_get_mmi_chg(void);
static int oplus_chg_track_obtain_power_info(char *power_info, int len);

#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
static struct type_reason_table track_type_reason_table[] = {
	{ TRACK_NOTIFY_TYPE_DEFAULT, "default" },
	{ TRACK_NOTIFY_TYPE_SOC_JUMP, "soc_error" },
	{ TRACK_NOTIFY_TYPE_GENERAL_RECORD, "general_record" },
	{ TRACK_NOTIFY_TYPE_NO_CHARGING, "no_charging" },
	{ TRACK_NOTIFY_TYPE_CHARGING_SLOW, "charge_slow" },
	{ TRACK_NOTIFY_TYPE_CHARGING_BREAK, "charge_break" },
	{ TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL, "device_abnormal" },
	{ TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL, "software_abnormal" },
};

static struct flag_reason_table track_flag_reason_table[] = {
	{ TRACK_NOTIFY_FLAG_DEFAULT, "default" },
	{ TRACK_NOTIFY_FLAG_UI_SOC_LOAD_JUMP, "UiSoc_LoadSocJump" },
	{ TRACK_NOTIFY_FLAG_SOC_JUMP, "SocJump" },
	{ TRACK_NOTIFY_FLAG_UI_SOC_JUMP, "UiSocJump" },
	{ TRACK_NOTIFY_FLAG_UI_SOC_TO_SOC_JUMP, "UiSoc-SocJump" },

	{ TRACK_NOTIFY_FLAG_CHARGER_INFO, "ChargerInfo" },
	{ TRACK_NOTIFY_FLAG_UISOC_KEEP_1_T_INFO, "UisocKeep1TInfo" },
	{ TRACK_NOTIFY_FLAG_VBATT_TOO_LOW_INFO, "VbattTooLowInfo" },
	{ TRACK_NOTIFY_FLAG_USBTEMP_INFO, "UsbTempInfo" },
	{ TRACK_NOTIFY_FLAG_VBATT_DIFF_OVER_INFO, "VbattDiffOverInfo" },
	{ TRACK_NOTIFY_FLAG_SERVICE_UPDATE_WLS_THIRD_INFO, "UpdateWlsThirdInfo" },
	{ TRACK_NOTIFY_FLAG_WLS_INFO, "WlsInfo" },
	{ TRACK_NOTIFY_FLAG_PARALLELCHG_FOLDMODE_INFO, "ParallelChgFoldModeInfo" },
	{ TRACK_NOTIFY_FLAG_MMI_CHG_INFO, "MmiChgInfo" },
	{ TRACK_NOTIFY_FLAG_SLOW_CHG_INFO, "SlowChgInfo" },
	{ TRACK_NOTIFY_FLAG_CHG_CYCLE_INFO, "ChgCycleInfo" },
	{ TRACK_NOTIFY_FLAG_TTF_INFO, "TtfInfo" },
	{ TRACK_NOTIFY_FLAG_UISOH_INFO, "UiSohInfo" },

	{ TRACK_NOTIFY_FLAG_NO_CHARGING, "NoCharging" },
	{ TRACK_NOTIFY_FLAG_NO_CHARGING_OTG_ONLINE, "OtgOnline" },
	{ TRACK_NOTIFY_FLAG_NO_CHARGING_VBATT_LEAK, "VBattLeakage" },

	{ TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_WARM, "BattTempWarm" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_COLD, "BattTempCold" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_NON_STANDARD_PA, "NonStandardAdatpter" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_BATT_CAP_HIGH, "BattCapHighWhenPlugin" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_COOLDOWN, "CoolDownCtlLongTime" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_WLS_SKEW, "WlsSkew" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_VERITY_FAIL, "VerityFail" },
	{ TRACK_NOTIFY_FLAG_CHG_SLOW_OTHER, "Other" },

	{ TRACK_NOTIFY_FLAG_FAST_CHARGING_BREAK, "FastChgBreak" },
	{ TRACK_NOTIFY_FLAG_GENERAL_CHARGING_BREAK, "GeneralChgBreak" },
	{ TRACK_NOTIFY_FLAG_WLS_CHARGING_BREAK, "WlsChgBreak" },
	{ TRACK_NOTIFY_FLAG_CHG_FEED_LIQUOR, "ChgintoliquidAbnormal" },

	{ TRACK_NOTIFY_FLAG_WLS_ABNORMAL, "WlsAbnormal" },
	{ TRACK_NOTIFY_FLAG_GPIO_ABNORMAL, "GpioAbnormal" },
	{ TRACK_NOTIFY_FLAG_CP_ABNORMAL, "CpAbnormal" },
	{ TRACK_NOTIFY_FLAG_PLAT_PMIC_ABNORMAL, "PlatPmicAbnormal" },
	{ TRACK_NOTIFY_FLAG_EXTERN_PMIC_ABNORMAL, "ExternPmicAbnormal" },
	{ TRACK_NOTIFY_FLAG_GAGUE_ABNORMAL, "GagueAbnormal" },
	{ TRACK_NOTIFY_FLAG_DCHG_ABNORMAL, "DchgAbnormal" },
	{ TRACK_NOTIFY_FLAG_PARALLEL_UNBALANCE_ABNORMAL, "ParallelUnbalance" },
	{ TRACK_NOTIFY_FLAG_MOS_ERROR_ABNORMAL, "MosError" },
	{ TRACK_NOTIFY_FLAG_HK_ABNORMAL, "HouseKeepingAbnormal" },
	{ TRACK_NOTIFY_FLAG_UFCS_IC_ABNORMAL, "UFCSICAbnormal" },
	{ TRACK_NOTIFY_FLAG_ADAPTER_ABNORMAL, "AdapterAbnormal" },

	{ TRACK_NOTIFY_FLAG_UFCS_ABNORMAL, "UfcsAbnormal" },
	{ TRACK_NOTIFY_FLAG_COOLDOWN_ABNORMAL, "CoolDownAbnormal" },
	{ TRACK_NOTIFY_FLAG_SMART_CHG_ABNORMAL, "SmartChgAbnormal" },
	{ TRACK_NOTIFY_FLAG_WLS_THIRD_ENCRY_ABNORMAL, "WlsThirdEncryAbnormal" },
	{ TRACK_NOTIFY_FLAG_PEN_MATCH_STATE_ABNORMAL, "PenMatchStateAbnormal" },
	{ TRACK_NOTIFY_FLAG_PPS_ABNORMAL, "PPSAbnormal" },
	{ TRACK_NOTIFY_FLAG_FASTCHG_START_ABNORMAL, "FastchgStartClearError" },
	{ TRACK_NOTIFY_FLAG_DUAL_CHAN_ABNORMAL, "DualChanAbnormal" },
	{ TRACK_NOTIFY_FLAG_DUMMY_START_ABNORMAL, "DummyStartClearError" },
};
#endif

static struct oplus_chg_track_type wired_type_table[] = {
	{ OPLUS_CHG_USB_TYPE_UNKNOWN, TRACK_POWER_2500MW, "unknow" },
	{ OPLUS_CHG_USB_TYPE_SDP, TRACK_POWER_2500MW, "sdp" },
	{ OPLUS_CHG_USB_TYPE_DCP, TRACK_POWER_10000MW, "dcp" },
	{ OPLUS_CHG_USB_TYPE_CDP, TRACK_POWER_7500MW, "cdp" },
	{ OPLUS_CHG_USB_TYPE_ACA, TRACK_POWER_10000MW, "dcp" },
	{ OPLUS_CHG_USB_TYPE_C, TRACK_POWER_10000MW, "dcp" },
	{ OPLUS_CHG_USB_TYPE_APPLE_BRICK_ID, TRACK_POWER_10000MW, "dcp" },
	{ OPLUS_CHG_USB_TYPE_PD_SDP, TRACK_POWER_10000MW, "pd_sdp" },
	{ OPLUS_CHG_USB_TYPE_PD, TRACK_POWER_18000MW, "pd" },
	{ OPLUS_CHG_USB_TYPE_PD_DRP, TRACK_POWER_18000MW, "pd" },
	{ OPLUS_CHG_USB_TYPE_QC2, TRACK_POWER_18000MW, "qc" },
	{ OPLUS_CHG_USB_TYPE_QC3, TRACK_POWER_18000MW, "qc" },
	{ OPLUS_CHG_USB_TYPE_PD_PPS, TRACK_POWER_30000MW, "pps" },
	{ OPLUS_CHG_USB_TYPE_VOOC, TRACK_POWER_20000MW, "vooc" },
	{ OPLUS_CHG_USB_TYPE_SVOOC, 0, "svooc" },
	{ OPLUS_CHG_USB_TYPE_UFCS, TRACK_POWER_100000MW, "ufcs" },
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static struct oplus_chg_track_type wls_adapter_type_table[] = {
	{ OPLUS_CHG_WLS_UNKNOWN, TRACK_POWER_5000MW, "unknow" },
	{ OPLUS_CHG_WLS_VOOC, TRACK_POWER_20000MW, "airvooc" },
	{ OPLUS_CHG_WLS_BPP, TRACK_POWER_5000MW, "bpp" },
	{ OPLUS_CHG_WLS_EPP, TRACK_POWER_10000MW, "epp" },
	{ OPLUS_CHG_WLS_EPP_PLUS, TRACK_POWER_10000MW, "epp" },
	{ OPLUS_CHG_WLS_SVOOC, TRACK_POWER_50000MW, "airsvooc" },
	{ OPLUS_CHG_WLS_PD_65W, TRACK_POWER_65000MW, "airsvooc" },
};
#else
static struct oplus_chg_track_type wls_adapter_type_table[] = {
	{ TRACK_WLS_ADAPTER_TYPE_UNKNOWN, TRACK_POWER_5000MW, "unknow" },
	{ TRACK_WLS_ADAPTER_TYPE_VOOC, TRACK_POWER_20000MW, "airvooc" },
	{ TRACK_WLS_ADAPTER_TYPE_SVOOC, TRACK_POWER_50000MW, "airsvooc" },
	{ TRACK_WLS_ADAPTER_TYPE_USB, TRACK_POWER_5000MW, "bpp" },
	{ TRACK_WLS_ADAPTER_TYPE_NORMAL, TRACK_POWER_5000MW, "bpp" },
	{ TRACK_WLS_ADAPTER_TYPE_EPP, TRACK_POWER_10000MW, "epp" },
	{ TRACK_WLS_ADAPTER_TYPE_SVOOC_50W, TRACK_POWER_50000MW, "airsvooc" },
	{ TRACK_WLS_ADAPTER_TYPE_PD_65W, TRACK_POWER_65000MW, "airsvooc" },
};
#endif

static struct oplus_chg_track_type wls_dock_type_table[] = {
	{ TRACK_WLS_DOCK_MODEL_0, TRACK_POWER_30000MW, "model_0" },
	{ TRACK_WLS_DOCK_MODEL_1, TRACK_POWER_40000MW, "model_1" },
	{ TRACK_WLS_DOCK_MODEL_2, TRACK_POWER_50000MW, "model_2" },
	{ TRACK_WLS_DOCK_MODEL_3, TRACK_POWER_50000MW, "model_3" },
	{ TRACK_WLS_DOCK_MODEL_4, TRACK_POWER_50000MW, "model_4" },
	{ TRACK_WLS_DOCK_MODEL_5, TRACK_POWER_50000MW, "model_5" },
	{ TRACK_WLS_DOCK_MODEL_6, TRACK_POWER_50000MW, "model_6" },
	{ TRACK_WLS_DOCK_MODEL_7, TRACK_POWER_50000MW, "model_7" },
	{ TRACK_WLS_DOCK_MODEL_8, TRACK_POWER_50000MW, "model_8" },
	{ TRACK_WLS_DOCK_MODEL_9, TRACK_POWER_50000MW, "model_9" },
	{ TRACK_WLS_DOCK_MODEL_A, TRACK_POWER_100000MW, "model_a" },
	{ TRACK_WLS_DOCK_MODEL_B, TRACK_POWER_100000MW, "model_b" },
	{ TRACK_WLS_DOCK_MODEL_10, TRACK_POWER_100000MW, "model_10" },
	{ TRACK_WLS_DOCK_MODEL_11, TRACK_POWER_30000MW, "model_11" },
	{ TRACK_WLS_DOCK_MODEL_12, TRACK_POWER_30000MW, "model_12" },
	{ TRACK_WLS_DOCK_MODEL_13, TRACK_POWER_30000MW, "model_13" },
	{ TRACK_WLS_DOCK_THIRD_PARTY, TRACK_POWER_50000MW, "third_party" },
};

static struct oplus_chg_track_vooc_type vooc_type_table[] = {
	{ 0x01, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },
	{ 0x13, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },
	{ 0x15, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },
	{ 0x16, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },
	{ 0x34, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },
	{ 0x45, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_4000MA, "vooc" },

	{ 0x17, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x18, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x19, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x29, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x41, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x42, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x43, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x44, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },
	{ 0x46, TRACK_INPUT_VOL_5000MV, TRACK_INPUT_CURR_6000MA, "vooc" },

	{ 0x1A, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_3000MA, "svooc" },
	{ 0x1B, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_3000MA, "svooc" },
	{ 0x49, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_3000MA, "svooc" },
	{ 0x4A, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_3000MA, "svooc" },
	{ 0x61, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_3000MA, "svooc" },

	{ 0x1C, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_4000MA, "svooc" },
	{ 0x1D, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_4000MA, "svooc" },
	{ 0x1E, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_4000MA, "svooc" },
	{ 0x22, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_4000MA, "svooc" },

	{ 0x11, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x12, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x21, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x23, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x31, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x33, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x62, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_5000MA, "svooc" },

	{ 0x24, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x25, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0x26, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_5000MA, "svooc" },
	{ 0X27, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_5000MA, "svooc" },

	{ 0x14, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x28, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x2A, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x35, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x63, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x66, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },
	{ 0x6E, TRACK_INPUT_VOL_10000MV, TRACK_INPUT_CURR_6500MA, "svooc" },

	{ 0x2B, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6000MA, "svooc" },
	{ 0x36, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6000MA, "svooc" },
	{ 0x64, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6000MA, "svooc" },

	{ 0x2C, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6100MA, "svooc" },
	{ 0x2D, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6100MA, "svooc" },
	{ 0x2E, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6100MA, "svooc" },
	{ 0x6C, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6100MA, "svooc" },
	{ 0x6D, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_6100MA, "svooc" },

	{ 0x4B, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_7300MA, "svooc" },
	{ 0x4C, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_7300MA, "svooc" },
	{ 0x4D, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_7300MA, "svooc" },
	{ 0x4E, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_7300MA, "svooc" },
	{ 0x65, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_7300MA, "svooc" },

	{ 0x37, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_8000MA, "svooc" },
	{ 0x38, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_8000MA, "svooc" },
	{ 0x39, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_8000MA, "svooc" },
	{ 0x3A, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_8000MA, "svooc" },

	{ 0x3B, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },
	{ 0x3C, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },
	{ 0x3D, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },
	{ 0x3E, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },
	{ 0x69, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },
	{ 0x6A, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_9100MA, "svooc" },

	{ 0x32, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11000MA, "svooc" },
	{ 0x47, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11000MA, "svooc" },
	{ 0x48, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11000MA, "svooc" },
	{ 0x6B, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11000MA, "svooc" },

	{ 0x51, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11400MA, "svooc" },
	{ 0x67, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11400MA, "svooc" },
	{ 0x68, TRACK_INPUT_VOL_11000MV, TRACK_INPUT_CURR_11400MA, "svooc" },
};

static struct oplus_chg_track_batt_full_reason batt_full_reason_table[] = {
	{ NOTIFY_BAT_FULL, "full_normal" },
	{ NOTIFY_BAT_FULL_PRE_LOW_TEMP, "full_low_temp" },
	{ NOTIFY_BAT_FULL_PRE_HIGH_TEMP, "full_high_temp" },
	{ NOTIFY_BAT_FULL_THIRD_BATTERY, "full_third_batt" },
	{ NOTIFY_BAT_NOT_CONNECT, "full_no_batt" },
};

static struct oplus_chg_track_chg_abnormal_reason chg_abnormal_reason_table[] = {
	{ NOTIFY_CHGING_OVERTIME, "chg_over_time", 0 },
	{ NOTIFY_CHARGER_OVER_VOL, "chg_over_vol", 0 },
	{ NOTIFY_CHARGER_LOW_VOL, "chg_low_vol", 0 },
	{ NOTIFY_BAT_OVER_TEMP, "batt_over_temp", 0 },
	{ NOTIFY_BAT_LOW_TEMP, "batt_low_temp", 0 },
	{ NOTIFY_BAT_OVER_VOL, "batt_over_vol", 0 },
	{ NOTIFY_BAT_NOT_CONNECT, "batt_no_conn", 0 },
	{ NOTIFY_BAT_FULL_THIRD_BATTERY, "batt_no_auth", 0 },
};

static struct oplus_chg_track_cool_down_stats cool_down_stats_table[] = {
	{ 0, 0, "L_0" },   { 1, 0, "L_1" },   { 2, 0, "L_2" },
	{ 3, 0, "L_3" },   { 4, 0, "L_4" },   { 5, 0, "L_5" },
	{ 6, 0, "L_6" },   { 7, 0, "L_7" },   { 8, 0, "L_8" },
	{ 9, 0, "L_9" },   { 10, 0, "L_10" }, { 11, 0, "L_11" },
	{ 12, 0, "L_12" }, { 13, 0, "L_13" }, { 14, 0, "L_14" },
	{ 15, 0, "L_15" }, { 16, 0, "L_16" }, { 17, 0, "L_17" },
	{ 18, 0, "L_18" }, { 19, 0, "L_19" }, { 20, 0, "L_20" },
	{ 21, 0, "L_21" }, { 22, 0, "L_22" }, { 23, 0, "L_23" },
	{ 24, 0, "L_24" }, { 25, 0, "L_25" }, { 26, 0, "L_26" },
	{ 27, 0, "L_27" }, { 28, 0, "L_28" }, { 29, 0, "L_29" },
	{ 30, 0, "L_30" }, { 31, 0, "L_31" },
};

static struct oplus_chg_track_fastchg_break mcu_voocphy_break_table[] = {
	{ TRACK_MCU_VOOCPHY_FAST_ABSENT, "absent" },
	{ TRACK_MCU_VOOCPHY_BAD_CONNECTED, "bad_connect" },
	{ TRACK_MCU_VOOCPHY_BTB_TEMP_OVER, "btb_temp_over" },
	{ TRACK_MCU_VOOCPHY_DATA_ERROR, "data_error" },
	{ TRACK_MCU_VOOCPHY_OTHER, "other" },
	{ TRACK_MCU_VOOCPHY_HEAD_ERROR, "head_error" },
	{ TRACK_MCU_VOOCPHY_ADAPTER_FW_UPDATE, "adapter_fw_update" },
	{ TRACK_MCU_VOOCPHY_CURR_OVER_ERROR, "curr_over_error" },
	{ TRACK_MCU_VOOCPHY_ADAPTER_COPYCAT, "adapter_copycat" },
};

static struct oplus_chg_track_fastchg_break adsp_voocphy_break_table[] = {
	{ TRACK_ADSP_VOOCPHY_BAD_CONNECTED, "bad_connect" },
	{ TRACK_ADSP_VOOCPHY_FRAME_H_ERR, "frame_head_error" },
	{ TRACK_ADSP_VOOCPHY_CLK_ERR, "clk_error" },
	{ TRACK_ADSP_VOOCPHY_HW_VBATT_HIGH, "hw_vbatt_high" },
	{ TRACK_ADSP_VOOCPHY_HW_TBATT_HIGH, "hw_ibatt_high" },
	{ TRACK_ADSP_VOOCPHY_COMMU_TIME_OUT, "commu_time_out" },
	{ TRACK_ADSP_VOOCPHY_ADAPTER_COPYCAT, "adapter_copycat" },
	{ TRACK_ADSP_VOOCPHY_BTB_TEMP_OVER, "btb_temp_over" },
	{ TRACK_ADSP_VOOCPHY_OTHER, "other" },
};

static struct oplus_chg_track_fastchg_break ap_voocphy_break_table[] = {
	{ TRACK_CP_VOOCPHY_FAST_ABSENT, "absent" },
	{ TRACK_CP_VOOCPHY_BAD_CONNECTED, "bad_connect" },
	{ TRACK_CP_VOOCPHY_FRAME_H_ERR, "frame_head_error" },
	{ TRACK_CP_VOOCPHY_BTB_TEMP_OVER, "btb_temp_over" },
	{ TRACK_CP_VOOCPHY_COMMU_TIME_OUT, "commu_time_out" },
	{ TRACK_CP_VOOCPHY_ADAPTER_COPYCAT, "adapter_copycat" },
	{ TRACK_CP_VOOCPHY_CURR_LIMIT_SMALL, "curr_limit_small" },
	{ TRACK_CP_VOOCPHY_ADAPTER_ABNORMAL, "adapter_abnormal" },
	{ TRACK_CP_VOOCPHY_OTHER, "other" },
};

static struct oplus_chg_track_voocphy_info voocphy_info_table[] = {
	{ TRACK_NO_VOOCPHY, "unknow" },
	{ TRACK_ADSP_VOOCPHY, "adsp" },
	{ TRACK_AP_SINGLE_CP_VOOCPHY, "ap" },
	{ TRACK_AP_DUAL_CP_VOOCPHY, "ap" },
	{ TRACK_MCU_VOOCPHY, "mcu" },
};

static struct oplus_chg_track_speed_ref wired_series_double_cell_125w_150w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_20000MW,
	  TRACK_POWER_20000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_18000MW,
	  TRACK_POWER_18000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_12000MW,
	  TRACK_POWER_12000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref
	wired_series_double_cell_65w_80w_100w[] = {
		{ TRACK_REF_SOC_50, TRACK_POWER_15000MW,
		  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_10000MV },
		{ TRACK_REF_SOC_75, TRACK_POWER_15000MW,
		  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_10000MV },
		{ TRACK_REF_SOC_90, TRACK_POWER_12000MW,
		  TRACK_POWER_12000MW * 1000 / TRACK_REF_VOL_10000MV },
	};

static struct oplus_chg_track_speed_ref wired_equ_single_cell_60w_67w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_12000MW,
	  TRACK_POWER_12000MW * 1000 / TRACK_REF_VOL_5000MV },
};

static struct oplus_chg_track_speed_ref wired_equ_single_cell_30w_33w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_12000MW,
	  TRACK_POWER_12000MW * 1000 / TRACK_REF_VOL_5000MV },
};

static struct oplus_chg_track_speed_ref wired_single_cell_18w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_10000MW,
	  TRACK_POWER_10000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_7000MW,
	  TRACK_POWER_7000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_6000MW,
	  TRACK_POWER_6000MW * 1000 / TRACK_REF_VOL_5000MV },
};

static struct oplus_chg_track_speed_ref wls_series_double_cell_40w_45w_50w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_15000MW,
	  TRACK_POWER_15000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_12000MW,
	  TRACK_POWER_12000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref wls_series_double_cell_20w_30w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_10000MW,
	  TRACK_POWER_10000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_10000MW,
	  TRACK_POWER_10000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_10000MW,
	  TRACK_POWER_10000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref wls_series_double_cell_12w_15w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref wls_equ_single_cell_12w_15w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
};

static struct oplus_chg_track_speed_ref wls_series_double_cell_epp15w_epp10w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref wls_equ_single_cell_epp15w_epp10w[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_5000MW,
	  TRACK_POWER_5000MW * 1000 / TRACK_REF_VOL_5000MV },
};

static struct oplus_chg_track_speed_ref wls_series_double_cell_bpp[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_2000MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_2000MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_10000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_2500MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_10000MV },
};

static struct oplus_chg_track_speed_ref wls_equ_single_cell_bpp[] = {
	{ TRACK_REF_SOC_50, TRACK_POWER_2000MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_75, TRACK_POWER_2000MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_5000MV },
	{ TRACK_REF_SOC_90, TRACK_POWER_2000MW,
	  TRACK_POWER_2000MW * 1000 / TRACK_REF_VOL_5000MV },
};

/*
 * digital represents wls bpp charging scheme
 * 0: wls 5w series dual cell bpp charging scheme
 * 1: wls 5w single cell bpp charging scheme
 * 2: wls 5w parallel double cell bpp charging scheme
 */
static struct oplus_chg_track_speed_ref *g_wls_bpp_speed_ref_standard[] = {
	wls_series_double_cell_bpp,
	wls_equ_single_cell_bpp,
	wls_equ_single_cell_bpp,
};

/*
 * digital represents wls epp charging scheme
 * 0: wls 10w or 15w series dual cell epp charging scheme
 * 1: wls 10w or 15w single cell epp charging scheme
 * 2: wls 10w or 15w parallel double cell epp charging scheme
 */
static struct oplus_chg_track_speed_ref *g_wls_epp_speed_ref_standard[] = {
	wls_series_double_cell_epp15w_epp10w,
	wls_equ_single_cell_epp15w_epp10w,
	wls_equ_single_cell_epp15w_epp10w,
};

/*
 * digital represents wls fast charging scheme
 * 0: wls 40w or 45w or 50w series dual cell charging scheme
 * 1: wls 20w or 30w series dual cell charging scheme
 * 2: wls 12w or 15w series dual cell charging scheme
 * 3: wls 12w or 15w parallel double cell charging scheme
 */
static struct oplus_chg_track_speed_ref* g_wls_fast_speed_ref_standard[] = {
	wls_series_double_cell_40w_45w_50w,
	wls_series_double_cell_20w_30w,
	wls_series_double_cell_12w_15w,
	wired_equ_single_cell_60w_67w,
	wls_equ_single_cell_12w_15w,
	wls_equ_single_cell_12w_15w,
};

/*
 * digital represents wired fast charging scheme
 * 0: wired 120w or 150w series dual cell charging scheme
 * 1: wired 65w or 80w and 100w series dual cell charging scheme
 * 2: wired 60w or 67w single cell charging scheme
 * 3: wired 60w or 67w parallel double cell charging scheme
 * 4: wired 30w or 33w single cell charging scheme
 * 5: wired 30w or 33w parallel double cell charging scheme
 * 6: wired 18w single cell charging scheme
 */
static struct oplus_chg_track_speed_ref *g_wired_speed_ref_standard[] = {
	wired_series_double_cell_125w_150w,
	wired_series_double_cell_65w_80w_100w,
	wired_equ_single_cell_60w_67w,
	wired_equ_single_cell_60w_67w,
	wired_equ_single_cell_30w_33w,
	wired_equ_single_cell_30w_33w,
	wired_single_cell_18w,
};

static struct oplus_chg_track_app_ref app_table[] = {
	{"T01", "pkg_01", 0},
	{"T02", "pkg_02", 0},
	{"T03", "pkg_03", 0},
	{"T04", "pkg_04", 0},
	{"T05", "pkg_05", 0},
	{"T06", "pkg_06", 0},
	{"T07", "pkg_07", 0},
	{"T08", "pkg_08", 0},
	{"T09", "pkg_09", 0},
	{"T10", "pkg_10", 0},
	{"T11", "pkg_11", 0},
	{"T12", "pkg_12", 0},
	{"T13", "pkg_13", 0},
	{"T14", "pkg_14", 0},
	{"T15", "pkg_15", 0},
	{"T16", "pkg_16", 0},
	{"T17", "pkg_17", 0},
	{"T18", "pkg_18", 0},
	{"T19", "pkg_19", 0},
	{"T20", "pkg_20", 0},
	{"T21", "pkg_21", 0},
	{"T22", "pkg_22", 0},
	{"T23", "pkg_23", 0},
	{"T24", "pkg_24", 0},
	{"T25", "pkg_25", 0},
	{"T26", "pkg_26", 0},
	{"T27", "pkg_27", 0},
	{"T28", "pkg_28", 0},
	{"T29", "pkg_29", 0},
	{"T30", "pkg_30", 0},
	{"T31", "pkg_31", 0},
	{"T32", "pkg_32", 0},
	{"T33", "pkg_33", 0},
	{"T34", "pkg_34", 0},
	{"T35", "pkg_35", 0},
	{"T36", "pkg_36", 0},
	{"T37", "pkg_37", 0},
	{"T38", "pkg_38", 0},
	{"T39", "pkg_39", 0},
	{"T40", "pkg_40", 0},
	{"T41", "pkg_41", 0},
	{"T42", "pkg_42", 0},
	{"T43", "pkg_43", 0},
	{"T44", "pkg_44", 0},
	{"T45", "pkg_45", 0},
	{"T46", "pkg_46", 0},
	{"T47", "pkg_47", 0},
	{"T48", "pkg_48", 0},
	{"T49", "pkg_49", 0},
	{"T50", "pkg_50", 0},
	{"Txx", "pkg_xx", 0},
};

__maybe_unused static bool is_fv_votable_available(struct oplus_monitor *chip)
{
	if (!chip->fv_votable)
		chip->fv_votable = find_votable("FV_MIN");
	return !!chip->fv_votable;
}

__maybe_unused static bool is_wls_icl_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_icl_votable)
		chip->wls_icl_votable = find_votable("WLS_NOR_ICL");
	return !!chip->wls_icl_votable;
}

__maybe_unused static bool is_wls_fcc_votable_available(struct oplus_monitor *chip)
{
	if (!chip->wls_fcc_votable)
		chip->wls_fcc_votable = find_votable("WLS_FCC");
	return !!chip->wls_fcc_votable;
}

static int oplus_chg_track_pack_app_stats(u8 *curx, int *index)
{
	int i;
	int record_index = 0;
	int second_index = 0;
	u32 max_time = 0;

	if (!curx || !index)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(app_table) - 1; i++) {
		if (app_table[i].cont_time >= max_time) {
			second_index = record_index;
			record_index = i;
			max_time = app_table[i].cont_time;
		} else if (second_index == record_index ||
				app_table[i].cont_time > app_table[second_index].cont_time) {
			second_index = i;
		}
	}

	*index += snprintf(&(curx[*index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - *index,
		"$$ledon_app@@%s,%d;%s,%d;%s,%d",
		app_table[record_index].alias_name, app_table[record_index].cont_time,
		app_table[second_index].alias_name, app_table[second_index].cont_time,
		app_table[ARRAY_SIZE(app_table) - 1].alias_name,
		app_table[ARRAY_SIZE(app_table) - 1].cont_time);

	return 0;
}

static void oplus_chg_track_clear_app_time(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(app_table); i++)
		app_table[i].cont_time = 0;
}

static int oplus_chg_track_match_app_info(u8 *app_name)
{
	int i;

	if (!app_name)
		return TRACK_APP_TOP_INDEX_DEFAULT;

	for (i = 0; i < ARRAY_SIZE(app_table); i++) {
		if (!strcmp(app_table[i].real_name, app_name))
			return i;
	}

	return TRACK_APP_TOP_INDEX_DEFAULT;
}

int oplus_chg_track_set_app_info(const char *buf)
{
	struct oplus_chg_track *track_chip = g_track_chip;
	struct oplus_chg_track_app_status *p_app_status;
	struct oplus_monitor *monitor;
	bool charger_exist;

	if (!track_chip || !buf)
		return -EINVAL;

	chg_debug("pkg_name:%s", buf);
	monitor = track_chip->monitor;
	charger_exist = monitor->wired_online || monitor->wls_online;
	p_app_status = &(track_chip->track_status.app_status);
	mutex_lock(&p_app_status->app_lock);
	if (charger_exist)
		strncpy(p_app_status->curr_top_name,
			buf, TRACK_APP_REAL_NAME_LEN - 1);
	mutex_unlock(&p_app_status->app_lock);
	return 0;
}

int oplus_chg_track_olc_config_set(const char *buf)
{
	struct oplus_chg_track_cfg *cfg_chip = NULL;
	char config_buf[OLC_CONFIG_SIZE] = { 0 };
	char *tmpbuf;
	int num = 0;
	int ret = 0;
	int len = 0;
	char *config = NULL;

	if (!g_track_chip)
		return -1;

	cfg_chip = &g_track_chip->track_cfg;
	strlcpy(config_buf, buf, OLC_CONFIG_SIZE);
	tmpbuf = config_buf;

	config = strsep(&tmpbuf, ",");
	while (config != NULL) {
		len = strlen(config);
		if (len > OLC_CONFIG_BIT_NUM) {	/* FFFFFFFFFFFFFFFF */
			chg_err("set wrong olc config\n");
			break;
		}

		ret = kstrtoull(config, 16, &cfg_chip->exception_data.olc_config[num]);
		if (ret < 0)
			chg_err("parse the %d olc config failed\n", num);
		else
			chg_info("set the %d olc config:%llx", num, cfg_chip->exception_data.olc_config[num]);
		num++;
		if (num >= OLC_CONFIG_NUM_MAX) {
			chg_err("set wrong olc config size\n");
			break;
		}
		config = strsep(&tmpbuf, ",");
	}

	return 0;
}

int oplus_chg_track_olc_config_get(char *buf)
{
	struct oplus_chg_track_cfg *cfg_chip = NULL;
	char tmpbuf[OLC_CONFIG_SIZE] = { 0 };
	int idx = 0;
	int num;
	int len = 0;

	if (!g_track_chip)
		return len;

	cfg_chip = &g_track_chip->track_cfg;

	for (num = 0; num < OLC_CONFIG_NUM_MAX; num++) {
		len = snprintf(tmpbuf, OLC_CONFIG_SIZE - idx, "%llx,",
				cfg_chip->exception_data.olc_config[num]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
	}

	return (idx - 1);
}

int oplus_chg_track_time_zone_set(const char *buf)
{
	int val = 0;
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	track_chip = g_track_chip;
	track_status = &track_chip->track_status;

	if (!track_chip || !track_status)
		return -1;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	track_status->track_gmtoff = val;

	return 0;
}

int oplus_chg_track_time_zone_get(char *buf)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	track_chip = g_track_chip;
	track_status = &track_chip->track_status;
	if (!track_chip || !track_status)
                return -1;

	return sprintf(buf, "%d\n", track_status->track_gmtoff);
}

static int oplus_parallelchg_track_foldmode_info(
	struct oplus_chg_track_hidl_cmd *cmd, struct oplus_chg_track *track_chip)
{
	struct oplus_parallelchg_track_hidl_foldmode_info_cmd *parallelchg_info_cmd;
	struct oplus_parallelchg_track_hidl_foldmode_info *parallelchg_info_p;

	if (!cmd)
		return -EINVAL;

	if (cmd->data_size != strlen(cmd->data_buf)) {
		chg_err("!!!size not match struct, ignore\n");
		return -EINVAL;
	}

	parallelchg_info_p = &(track_chip->track_status.parallelchg_info);
	mutex_lock(&parallelchg_info_p->track_lock);
	if (parallelchg_info_p->info_uploading) {
		chg_info(" parallelchg_track_foldmode info uploading, should return\n");
		mutex_unlock(&parallelchg_info_p->track_lock);
		return 0;
	}
	parallelchg_info_cmd = (struct oplus_parallelchg_track_hidl_foldmode_info_cmd *)(cmd->data_buf);
	memcpy(&parallelchg_info_p->parallelchg_foldmode_info, parallelchg_info_cmd, sizeof(*parallelchg_info_cmd));
	mutex_unlock(&parallelchg_info_p->track_lock);

	schedule_delayed_work(&parallelchg_info_p->load_trigger_work, 0);
	return 0;
}

static int oplus_chg_track_set_hidl_ttf_info(
	struct oplus_chg_track_hidl_cmd *cmd, struct oplus_chg_track *track_chip)
{
	struct oplus_chg_track_hidl_ttf_info_cmd *ttf_info_cmd;
	struct oplus_chg_track_hidl_ttf_info *ttf_info_p;

	if (!cmd)
		return -EINVAL;

	if (cmd->data_size != sizeof(struct oplus_chg_track_hidl_ttf_info_cmd)) {
		chg_err("!!!size not match struct, ignore\n");
		return -EINVAL;
	}

	ttf_info_p = track_chip->track_status.ttf_info;
	if (IS_ERR_OR_NULL(ttf_info_p)) {
		chg_err("ttf_info_p is null\n");
		return -EINVAL;
	}
	mutex_lock(&ttf_info_p->track_lock);
	if (ttf_info_p->info_uploading) {
		chg_info("uploading, should return\n");
		mutex_unlock(&ttf_info_p->track_lock);
		return 0;
	}
	ttf_info_cmd = (struct oplus_chg_track_hidl_ttf_info_cmd *)(cmd->data_buf);
	memcpy(&ttf_info_p->ttf_info, ttf_info_cmd, sizeof(*ttf_info_cmd));
	mutex_unlock(&ttf_info_p->track_lock);

	schedule_delayed_work(&ttf_info_p->load_trigger_work, 0);
	return 0;
}

static void oplus_track_upload_ttf_info(struct work_struct *work)
{
	int index = 0;

	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track_hidl_ttf_info *ttf_info_p = container_of(
		dwork, struct oplus_chg_track_hidl_ttf_info, load_trigger_work);

	mutex_lock(&ttf_info_p->track_lock);
	if (ttf_info_p->info_uploading) {
		chg_info("ttf_info_uploading, should return\n");
		mutex_unlock(&ttf_info_p->track_lock);
		return;
	}

	if (ttf_info_p->load_trigger_info)
		kfree(ttf_info_p->load_trigger_info);
	ttf_info_p->load_trigger_info = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!ttf_info_p->load_trigger_info) {
		chg_err("ttf_info_load_trigger memory alloc fail\n");
		mutex_unlock(&ttf_info_p->track_lock);
		return;
	}
	ttf_info_p->load_trigger_info->type_reason = TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	ttf_info_p->load_trigger_info->flag_reason = TRACK_NOTIFY_FLAG_TTF_INFO;
	ttf_info_p->info_uploading = true;
	mutex_unlock(&ttf_info_p->track_lock);

	index += snprintf(&(ttf_info_p->load_trigger_info->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"%s", ttf_info_p->ttf_info.data_buf);

	oplus_chg_track_obtain_power_info(&(ttf_info_p->load_trigger_info->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	oplus_chg_track_upload_trigger_data(*(ttf_info_p->load_trigger_info));
	if (ttf_info_p->load_trigger_info) {
		kfree(ttf_info_p->load_trigger_info);
		ttf_info_p->load_trigger_info = NULL;
	}
	memset(&ttf_info_p->ttf_info, 0, sizeof(ttf_info_p->ttf_info));
	ttf_info_p->info_uploading = false;
	return;
}

static int oplus_chg_track_set_hidl_uisoh_info(
	struct oplus_chg_track_hidl_cmd *cmd, struct oplus_chg_track *track_chip)
{
	struct oplus_chg_track_hidl_uisoh_info_cmd *uisoh_info_cmd;
	struct oplus_chg_track_hidl_uisoh_info *uisoh_info_p;

	if (!cmd)
		return -EINVAL;

	if (cmd->data_size != sizeof(struct oplus_chg_track_hidl_uisoh_info_cmd)) {
		chg_err("!!!size not match struct, ignore\n");
		return -EINVAL;
	}

	uisoh_info_p = &(track_chip->track_status.uisoh_info_s);
	mutex_lock(&uisoh_info_p->track_uisoh_info_lock);
	if (uisoh_info_p->uisoh_info_uploading) {
		chg_debug("uisoh_info_uploading, should return\n");
		mutex_unlock(&uisoh_info_p->track_uisoh_info_lock);
		return 0;
	}
	uisoh_info_cmd = (struct oplus_chg_track_hidl_uisoh_info_cmd *)(cmd->data_buf);
	memcpy(&uisoh_info_p->uisoh_info, uisoh_info_cmd, sizeof(*uisoh_info_cmd));
	mutex_unlock(&uisoh_info_p->track_uisoh_info_lock);

	schedule_delayed_work(&uisoh_info_p->uisoh_info_load_trigger_work, 0);
	return 0;
}

static void oplus_track_upload_parallelchg_foldmode_info(struct work_struct *work)
{
	int index = 0;

	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_parallelchg_track_hidl_foldmode_info *parallel_foldmode_p = container_of(
		dwork, struct oplus_parallelchg_track_hidl_foldmode_info, load_trigger_work);
	struct oplus_chg_track *track_chip = g_track_chip;
	struct oplus_monitor *monitor;
	struct oplus_chg_track_status *track_status;

	if (!parallel_foldmode_p || !track_chip || !track_chip->monitor) {
		chg_err(" parallel_foldmode_p or track_chip or monitor null return\n");
		return;
	}
	monitor = track_chip->monitor;
	track_status = &track_chip->track_status;

	mutex_lock(&parallel_foldmode_p->track_lock);
	if (parallel_foldmode_p->info_uploading) {
		chg_info("uisoh_info_uploading, should return\n");
		mutex_unlock(&parallel_foldmode_p->track_lock);
		return;
	}

	if (parallel_foldmode_p->load_trigger_info)
		kfree(parallel_foldmode_p->load_trigger_info);
	parallel_foldmode_p->load_trigger_info = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!parallel_foldmode_p->load_trigger_info) {
		chg_err("uisoh_info_load_trigger memery alloc fail\n");
		mutex_unlock(&parallel_foldmode_p->track_lock);
		return;
	}
	parallel_foldmode_p->load_trigger_info->type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	parallel_foldmode_p->load_trigger_info->flag_reason =
		TRACK_NOTIFY_FLAG_PARALLELCHG_FOLDMODE_INFO;
	parallel_foldmode_p->info_uploading = true;

	mutex_unlock(&parallel_foldmode_p->track_lock);


	index += snprintf(&(parallel_foldmode_p->load_trigger_info->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"%s", parallel_foldmode_p->parallelchg_foldmode_info.data_buf);

	index += snprintf(&(parallel_foldmode_p->load_trigger_info->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$other@@BATTERY[%d %d %d %d %d %d %d %d %d %d %d 0x%x], "
			"CHARGE[%d %d %d %d], "
			"WIRED[%d %d %d %d %d 0x%x %d %d %d %d %d], "
			"WIRELESS[%d %d %d %d %d 0x%x %d %d], "
			"VOOC[%d %d %d %d 0x%x], "
			"COMMON[%d %d %d 0x%x %d], "
			"SLOW[%d %d %d %d %d %d %d]",
			monitor->batt_temp, monitor->shell_temp, monitor->vbat_mv,
			monitor->vbat_min_mv, monitor->ibat_ma, monitor->batt_soc,
			monitor->smooth_soc, monitor->ui_soc, monitor->batt_rm, monitor->batt_fcc,
			monitor->batt_exist, monitor->batt_err_code, monitor->fv_mv,
			monitor->fcc_ma, monitor->chg_disable,
			monitor->chg_user_disable, monitor->wired_online,
			monitor->wired_ibus_ma, monitor->wired_vbus_mv,
			monitor->wired_icl_ma, monitor->wired_charge_type,
			monitor->wired_err_code, monitor->wired_suspend,
			monitor->wired_user_suspend, monitor->cc_mode,
			monitor->cc_detect, monitor->otg_enable, monitor->wls_online,
			monitor->wls_iout_ma, monitor->wls_vout_mv, monitor->wls_icl_ma,
			monitor->wls_charge_type, monitor->wls_err_code,
			monitor->wls_suspend, monitor->wls_user_suspend,
			monitor->vooc_online, monitor->vooc_started,
			monitor->vooc_charging, monitor->vooc_online_keep,
			monitor->vooc_sid, monitor->temp_region, monitor->ffc_status,
			monitor->cool_down, monitor->notify_code, monitor->led_on,
			track_status->has_judge_speed,
			track_status->soc_low_sect_incr_rm,
			track_status->soc_low_sect_cont_time,
			track_status->soc_medium_sect_incr_rm,
			track_status->soc_medium_sect_cont_time,
			track_status->soc_high_sect_incr_rm,
			track_status->soc_high_sect_cont_time);

	oplus_chg_track_upload_trigger_data(*(parallel_foldmode_p->load_trigger_info));
	if (parallel_foldmode_p->load_trigger_info) {
		kfree(parallel_foldmode_p->load_trigger_info);
		parallel_foldmode_p->load_trigger_info = NULL;
	}
	memset(&parallel_foldmode_p->parallelchg_foldmode_info, 0,
		sizeof(parallel_foldmode_p->parallelchg_foldmode_info));
	parallel_foldmode_p->info_uploading = false;
	return;
}

static void oplus_track_upload_uisoh_info(struct work_struct *work)
{
	int index = 0;
	int curr_time;
	static int upload_count = 0;
	static int pre_upload_time = 0;
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track_hidl_uisoh_info *uisoh_info_p = container_of(
		dwork, struct oplus_chg_track_hidl_uisoh_info, uisoh_info_load_trigger_work);

	if (!uisoh_info_p)
		return;

	curr_time = oplus_chg_track_get_local_time_s();
	if (curr_time - pre_upload_time > TRACK_SOFT_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (upload_count > TRACK_SOFT_SOH_UPLOAD_COUNT_MAX)
		return;

	mutex_lock(&uisoh_info_p->track_uisoh_info_lock);
	if (uisoh_info_p->uisoh_info_uploading) {
		chg_debug("uisoh_info_uploading, should return\n");
		mutex_unlock(&uisoh_info_p->track_uisoh_info_lock);
		return;
	}

	if (uisoh_info_p->uisoh_info_load_trigger)
		kfree(uisoh_info_p->uisoh_info_load_trigger);
	uisoh_info_p->uisoh_info_load_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!uisoh_info_p->uisoh_info_load_trigger) {
		chg_err("uisoh_info_load_trigger memery alloc fail\n");
		mutex_unlock(&uisoh_info_p->track_uisoh_info_lock);
		return;
	}
	uisoh_info_p->uisoh_info_load_trigger->type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	uisoh_info_p->uisoh_info_load_trigger->flag_reason =
		TRACK_NOTIFY_FLAG_UISOH_INFO;
	uisoh_info_p->uisoh_info_uploading = true;
	upload_count++;
	pre_upload_time = oplus_chg_track_get_local_time_s();
	mutex_unlock(&uisoh_info_p->track_uisoh_info_lock);

	index += snprintf(
		&(uisoh_info_p->uisoh_info_load_trigger->crux_info[index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$err_scene@@%s",
		"uisoh_info");

	index += snprintf(&(uisoh_info_p->uisoh_info_load_trigger->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$curx_info@@%s", uisoh_info_p->uisoh_info.data_buf);

	oplus_chg_track_upload_trigger_data(*(uisoh_info_p->uisoh_info_load_trigger));
	if (uisoh_info_p->uisoh_info_load_trigger) {
		kfree(uisoh_info_p->uisoh_info_load_trigger);
		uisoh_info_p->uisoh_info_load_trigger = NULL;
	}
	memset(&uisoh_info_p->uisoh_info, 0, sizeof(uisoh_info_p->uisoh_info));
	uisoh_info_p->uisoh_info_uploading = false;
	chg_debug("success\n");
}
static int oplus_chg_track_uisoh_err_init(struct oplus_chg_track *chip)
{
	struct oplus_chg_track_hidl_uisoh_info *uisoh_info_p;

	if (!chip)
		return - EINVAL;

	uisoh_info_p = &(chip->track_status.uisoh_info_s);
	mutex_init(&uisoh_info_p->track_uisoh_info_lock);
	uisoh_info_p->uisoh_info_uploading = false;
	uisoh_info_p->uisoh_info_load_trigger = NULL;

	memset(&uisoh_info_p->uisoh_info, 0, sizeof(uisoh_info_p->uisoh_info));
	INIT_DELAYED_WORK(&uisoh_info_p->uisoh_info_load_trigger_work,
		oplus_track_upload_uisoh_info);

	return 0;
}

static int oplus_parallelchg_track_foldmode_init(struct oplus_chg_track *chip)
{
	struct oplus_parallelchg_track_hidl_foldmode_info *parallelchg_foldmode_info_p;

	if (!chip)
		return - EINVAL;

	parallelchg_foldmode_info_p = &(chip->track_status.parallelchg_info);
	mutex_init(&parallelchg_foldmode_info_p->track_lock);
	parallelchg_foldmode_info_p->info_uploading = false;
	parallelchg_foldmode_info_p->load_trigger_info = NULL;

	memset(&parallelchg_foldmode_info_p->parallelchg_foldmode_info, 0,
		sizeof(parallelchg_foldmode_info_p->parallelchg_foldmode_info));

	INIT_DELAYED_WORK(&parallelchg_foldmode_info_p->load_trigger_work,
		oplus_track_upload_parallelchg_foldmode_info);

	return 0;
}

static int oplus_chg_track_ttf_info_init(struct oplus_chg_track *chip)
{
	if (!chip)
		return - EINVAL;

	chip->track_status.ttf_info = (struct oplus_chg_track_hidl_ttf_info *)kzalloc(
		sizeof(struct oplus_chg_track_hidl_ttf_info), GFP_KERNEL);
	if (!chip->track_status.ttf_info) {
		chg_err("kzalloc mem fail\n");
		return -ENOMEM;
	}

	mutex_init(&chip->track_status.ttf_info->track_lock);
	chip->track_status.ttf_info->info_uploading = false;
	chip->track_status.ttf_info->load_trigger_info = NULL;

	INIT_DELAYED_WORK(&chip->track_status.ttf_info->load_trigger_work,
		oplus_track_upload_ttf_info);

	return 0;
}

int oplus_chg_track_set_hidl_info(const char *buf, size_t count)
{
	struct oplus_chg_track *track_chip = g_track_chip;
	struct oplus_chg_track_hidl_cmd *p_cmd;

	if (!track_chip)
		return -EINVAL;

	p_cmd = (struct oplus_chg_track_hidl_cmd *)buf;
	if (count != sizeof(struct oplus_chg_track_hidl_cmd)) {
		chg_err("!!!size of buf is not matched\n");
		return -EINVAL;
	}

	chg_debug("!!!cmd[%d]\n", p_cmd->cmd);
	switch (p_cmd->cmd) {
	case TRACK_HIDL_BCC_INFO:
		break;
	case TRACK_HIDL_BCC_ERR:
		break;
	case TRACK_HIDL_BMS_INFO:
		break;
	case TRACK_HIDL_HYPER_INFO:
		break;
	case TRACK_HIDL_WLS_THIRD_ERR:
		break;
	case TRACK_HIDL_UISOH_INFO:
		oplus_chg_track_set_hidl_uisoh_info(p_cmd, track_chip);
		break;
	case TRACK_HIDL_PARALLELCHG_FOLDMODE_INFO:
		oplus_parallelchg_track_foldmode_info(p_cmd, track_chip);
		break;
	case TRACK_HIDL_TTF_INFO:
		oplus_chg_track_set_hidl_ttf_info(p_cmd, track_chip);
		break;
	default:
		chg_err("!!!cmd error\n");
		break;
	}

	return 0;
}

struct dentry *oplus_chg_track_get_debugfs_root(void)
{
	mutex_lock(&debugfs_root_mutex);
	if (!track_debugfs_root) {
		track_debugfs_root =
			debugfs_create_dir("oplus_chg_track", NULL);
	}
	mutex_unlock(&debugfs_root_mutex);

	return track_debugfs_root;
}

static int oplus_chg_track_clear_cool_down_stats_time(
	struct oplus_chg_track_status *track_status)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cool_down_stats_table); i++)
		cool_down_stats_table[i].time = 0;
	return 0;
}

static int oplus_chg_track_set_voocphy_name(struct oplus_chg_track *track_chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(voocphy_info_table); i++) {
		if (voocphy_info_table[i].voocphy_type ==
		    track_chip->track_cfg.voocphy_type) {
			strncpy(track_chip->voocphy_name,
				voocphy_info_table[i].name,
				OPLUS_CHG_TRACK_VOOCPHY_NAME_LEN - 1);
			break;
		}
	}

	if (i == ARRAY_SIZE(voocphy_info_table))
		strncpy(track_chip->voocphy_name, voocphy_info_table[0].name,
			OPLUS_CHG_TRACK_VOOCPHY_NAME_LEN - 1);

	return 0;
}

static int
oplus_chg_track_get_wired_type_info(int charge_type,
				    struct oplus_chg_track_status *track_status)
{
	int i;
	int charge_index = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(wired_type_table); i++) {
		if (wired_type_table[i].type == charge_type) {
			strncpy(track_status->power_info.wired_info.adapter_type,
				wired_type_table[i].name,
				OPLUS_CHG_TRACK_POWER_TYPE_LEN - 1);
			track_status->power_info.wired_info.power =
				wired_type_table[i].power;
			charge_index = i;
			break;
		}
	}

	return charge_index;
}

static int
oplus_chg_track_get_vooc_type_info(int vooc_type,
				   struct oplus_chg_track_status *track_status)
{
	int i;
	int vooc_index = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(vooc_type_table); i++) {
		if (vooc_type_table[i].chg_type == vooc_type) {
			strncpy(track_status->power_info.wired_info.adapter_type,
				vooc_type_table[i].name,
				OPLUS_CHG_TRACK_POWER_TYPE_LEN - 1);
			track_status->power_info.wired_info.power =
				vooc_type_table[i].vol *
				vooc_type_table[i].cur / 1000 / 500;
			track_status->power_info.wired_info.power *= 500;
			track_status->power_info.wired_info.adapter_id =
				vooc_type_table[i].chg_type;
			vooc_index = i;
			break;
		}
	}

	return vooc_index;
}

__maybe_unused static int oplus_chg_track_get_wls_adapter_type_info(
	int charge_type, struct oplus_chg_track_status *track_status)
{
	int i;
	int charge_index = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(wls_adapter_type_table); i++) {
		if (wls_adapter_type_table[i].type == charge_type) {
			strncpy(track_status->power_info.wls_info.adapter_type,
				wls_adapter_type_table[i].name,
				OPLUS_CHG_TRACK_POWER_TYPE_LEN - 1);
			track_status->power_info.wls_info.power =
				wls_adapter_type_table[i].power;
			charge_index = i;
			break;
		}
	}

	return charge_index;
}

__maybe_unused static int oplus_chg_track_get_wls_dock_type_info(
	int charge_type, struct oplus_chg_track_status *track_status)
{
	int i;
	int charge_index = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(wls_dock_type_table); i++) {
		if (wls_dock_type_table[i].type == charge_type) {
			strncpy(track_status->power_info.wls_info.dock_type,
				wls_dock_type_table[i].name,
				OPLUS_CHG_TRACK_POWER_TYPE_LEN - 1);
			charge_index = i;
			break;
		}
	}

	return charge_index;
}

static int oplus_chg_track_get_batt_full_reason_info(
	int notify_flag, struct oplus_chg_track_status *track_status)
{
	int i;
	int charge_index = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(batt_full_reason_table); i++) {
		if (batt_full_reason_table[i].notify_flag == notify_flag) {
			strncpy(track_status->batt_full_reason,
				batt_full_reason_table[i].reason,
				OPLUS_CHG_TRACK_BATT_FULL_REASON_LEN - 1);
			charge_index = i;
			break;
		}
	}

	return charge_index;
}

static int oplus_chg_track_get_chg_abnormal_reason_info(
	int notify_code, struct oplus_chg_track_status *track_status)
{
	int i;
	int charge_index = -EINVAL;
	int index;

	index = strlen(track_status->chg_abnormal_reason);
	for (i = 0; i < ARRAY_SIZE(chg_abnormal_reason_table); i++) {
		if (!chg_abnormal_reason_table[i].happened &&
		    chg_abnormal_reason_table[i].notify_code == notify_code) {
			chg_abnormal_reason_table[i].happened = true;
			if (!index)
				index += snprintf(
					&(track_status
						  ->chg_abnormal_reason[index]),
					OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LENS -
						index,
					"%s",
					chg_abnormal_reason_table[i].reason);
			else
				index += snprintf(
					&(track_status
						  ->chg_abnormal_reason[index]),
					OPLUS_CHG_TRACK_CHG_ABNORMAL_REASON_LENS -
						index,
					",%s",
					chg_abnormal_reason_table[i].reason);
			charge_index = i;
			break;
		}
	}

	return charge_index;
}

static int oplus_chg_track_parse_dt(struct oplus_chg_track *track_dev)
{
	int rc = 0;
	struct device_node *node = track_dev->dev->of_node;
	int i = 0;
	int length = 0;

	rc = of_property_read_u32(node, "track,fast_chg_break_t_thd",
				  &(track_dev->track_cfg.fast_chg_break_t_thd));
	if (rc < 0) {
		chg_err("track,fast_chg_break_t_thd reading failed, rc=%d\n",
			rc);
		track_dev->track_cfg.fast_chg_break_t_thd = TRACK_T_THD_1000_MS;
	}

	rc = of_property_read_u32(
		node, "track,general_chg_break_t_thd",
		&(track_dev->track_cfg.general_chg_break_t_thd));
	if (rc < 0) {
		chg_err("track,general_chg_break_t_thd reading failed, rc=%d\n",
			rc);
		track_dev->track_cfg.general_chg_break_t_thd =
			TRACK_T_THD_500_MS;
	}

	rc = of_property_read_u32(node, "track,voocphy_type",
				  &(track_dev->track_cfg.voocphy_type));
	if (rc < 0) {
		chg_err("track,voocphy_type reading failed, rc=%d\n", rc);
		track_dev->track_cfg.voocphy_type = TRACK_NO_VOOCPHY;
	}

	rc = of_property_read_u32(node, "track,wls_chg_break_t_thd",
				  &(track_dev->track_cfg.wls_chg_break_t_thd));
	if (rc < 0) {
		chg_err("track,wls_chg_break_t_thd reading failed, rc=%d\n",
			rc);
		track_dev->track_cfg.wls_chg_break_t_thd = TRACK_T_THD_6000_MS;
	}

	rc = of_property_read_u32(
		node, "track,wired_fast_chg_scheme",
		&(track_dev->track_cfg.wired_fast_chg_scheme));
	if (rc < 0) {
		chg_err("track,wired_fast_chg_scheme reading failed, rc=%d\n",
			rc);
		track_dev->track_cfg.wired_fast_chg_scheme = -1;
	}

	rc = of_property_read_u32(node, "track,wls_fast_chg_scheme",
				  &(track_dev->track_cfg.wls_fast_chg_scheme));
	if (rc < 0) {
		chg_err("track,wls_fast_chg_scheme reading failed, rc=%d\n",
			rc);
		track_dev->track_cfg.wls_fast_chg_scheme = -1;
	}

	rc = of_property_read_u32(node, "track,wls_epp_chg_scheme",
				  &(track_dev->track_cfg.wls_epp_chg_scheme));
	if (rc < 0) {
		chg_err("track,wls_epp_chg_scheme reading failed, rc=%d\n", rc);
		track_dev->track_cfg.wls_epp_chg_scheme = -1;
	}

	rc = of_property_read_u32(node, "track,wls_bpp_chg_scheme",
				  &(track_dev->track_cfg.wls_bpp_chg_scheme));
	if (rc < 0) {
		chg_err("track,wls_bpp_chg_scheme reading failed, rc=%d\n", rc);
		track_dev->track_cfg.wls_bpp_chg_scheme = -1;
	}

	rc = of_property_read_u32(node, "track,wls_max_power",
				  &(track_dev->track_cfg.wls_max_power));
	if (rc < 0) {
		chg_err("track,wls_max_power reading failed, rc=%d\n", rc);
		track_dev->track_cfg.wls_max_power = 0;
	}

	rc = of_property_read_u32(node, "track,wired_max_power",
				  &(track_dev->track_cfg.wired_max_power));
	if (rc < 0) {
		chg_err("track,wired_max_power reading failed, rc=%d\n", rc);
		track_dev->track_cfg.wired_max_power = 0;
	}

	rc = of_property_read_u32(node, "track,track_ver",
				  &(track_dev->track_cfg.track_ver));
	if (rc < 0) {
		chg_err("track,track_ver reading failed, rc=%d\n", rc);
		track_dev->track_cfg.track_ver = 3;
	}

	memset(&track_dev->track_cfg.exception_data, 0, sizeof(struct exception_data));
	rc = of_property_count_elems_of_size(node, "track,olc_config", sizeof(u64));
	if (rc < 0) {
		chg_err("Count track_olc_config failed, rc=%d\n", rc);
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
		if (get_eng_version() == PREVERSION) {
			chg_err("preversion open SocJump NoCharging FastChgBreak olc config\n");
			track_dev->track_cfg.exception_data.olc_config[0] = 0x2;
			track_dev->track_cfg.exception_data.olc_config[2] = 0x1;
			track_dev->track_cfg.exception_data.olc_config[4] = 0x1;
		}
#endif
	} else {
		length = rc;
		if (length > OLC_CONFIG_NUM_MAX)
			length = OLC_CONFIG_NUM_MAX;
		chg_info("parse olc_config, size=%d\n", length);
		rc = of_property_read_u64_array(node, "track,olc_config",
				track_dev->track_cfg.exception_data.olc_config, length);
		if (rc < 0) {
			chg_err("parse chg_olc_config failed, rc=%d\n", rc);
		} else {
			for (i = 0; i < length; i++)
				chg_info("parse chg_olc_config[%d]=%llu\n", i, track_dev->track_cfg.exception_data.olc_config[i]);
		}
	}

	return 0;
}

static void oplus_chg_track_uisoc_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, uisoc_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->uisoc_load_trigger);
}

static void oplus_chg_track_soc_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip =
		container_of(dwork, struct oplus_chg_track, soc_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->soc_trigger);
}

static void oplus_chg_track_uisoc_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip =
		container_of(dwork, struct oplus_chg_track, uisoc_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->uisoc_trigger);
}

static void oplus_chg_track_uisoc_to_soc_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, uisoc_to_soc_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->uisoc_to_soc_trigger);
}

static int
oplus_chg_track_record_general_info(struct oplus_monitor *monitor,
				    struct oplus_chg_track_status *track_status,
				    oplus_chg_track_trigger *p_trigger_data,
				    int index)
{
	if (!monitor || !p_trigger_data || !track_status)
		return -1;

	if (index < 0 || index >= OPLUS_CHG_TRACK_CURX_INFO_LEN) {
		chg_err("index is invalid\n");
		return -1;
	}

	index += snprintf(
		&(p_trigger_data->crux_info[index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
		"$$other@@BATTERY[%d %d %d %d %d %d %d %d %d %d %d 0x%x], "
		"CHARGE[%d %d %d %d], "
		"WIRED[%d %d %d %d %d 0x%x %d %d %d %d %d], "
		"WIRELESS[%d %d %d %d %d 0x%x %d %d], "
		"VOOC[%d %d %d %d 0x%x], "
		"COMMON[%d %d %d 0x%x %d], "
		"SLOW[%d %d %d %d %d %d %d]",
		monitor->batt_temp, monitor->shell_temp, monitor->vbat_mv,
		monitor->vbat_min_mv, monitor->ibat_ma, monitor->batt_soc,
		monitor->smooth_soc, monitor->ui_soc, monitor->batt_rm, monitor->batt_fcc,
		monitor->batt_exist, monitor->batt_err_code, monitor->fv_mv,
		monitor->fcc_ma, monitor->chg_disable,
		monitor->chg_user_disable, monitor->wired_online,
		monitor->wired_ibus_ma, monitor->wired_vbus_mv,
		monitor->wired_icl_ma, monitor->wired_charge_type,
		monitor->wired_err_code, monitor->wired_suspend,
		monitor->wired_user_suspend, monitor->cc_mode,
		monitor->cc_detect, monitor->otg_enable, monitor->wls_online,
		monitor->wls_iout_ma, monitor->wls_vout_mv, monitor->wls_icl_ma,
		monitor->wls_charge_type, monitor->wls_err_code,
		monitor->wls_suspend, monitor->wls_user_suspend,
		monitor->vooc_online, monitor->vooc_started,
		monitor->vooc_charging, monitor->vooc_online_keep,
		monitor->vooc_sid, monitor->temp_region, monitor->ffc_status,
		monitor->cool_down, monitor->notify_code, monitor->led_on,
		track_status->has_judge_speed,
		track_status->soc_low_sect_incr_rm,
		track_status->soc_low_sect_cont_time,
		track_status->soc_medium_sect_incr_rm,
		track_status->soc_medium_sect_cont_time,
		track_status->soc_high_sect_incr_rm,
		track_status->soc_high_sect_cont_time);

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS) {
		if (strlen(track_status->wls_break_crux_info))
			index += snprintf(&(p_trigger_data->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "%s ",
					  track_status->wls_break_crux_info);
	}
	chg_info("index:%d\n", index);
	return 0;
}

static int oplus_chg_track_pack_cool_down_stats(
	struct oplus_chg_track_status *track_status, char *cool_down_pack)
{
	int i;
	int index = 0;

	if (cool_down_pack == NULL || track_status == NULL)
		return -1;

	for (i = 0; i < ARRAY_SIZE(cool_down_stats_table) - 1; i++) {
		index += snprintf(&(cool_down_pack[index]),
				  OPLUS_CHG_TRACK_COOL_DOWN_PACK_LEN - index,
				  "%s,%d;", cool_down_stats_table[i].level_name,
				  cool_down_stats_table[i].time);
	}

	index += snprintf(&(cool_down_pack[index]),
			  OPLUS_CHG_TRACK_COOL_DOWN_PACK_LEN - index, "%s,%d",
			  cool_down_stats_table[i].level_name,
			  cool_down_stats_table[i].time *
				  TRACK_THRAD_PERIOD_TIME_S);
	chg_info("i=%d, cool_down_pack[%s]\n", i, cool_down_pack);

	return 0;
}

static void
oplus_chg_track_record_charger_info(struct oplus_monitor *monitor,
				    oplus_chg_track_trigger *p_trigger_data,
				    struct oplus_chg_track_status *track_status)
{
	int index = 0;
	char cool_down_pack[OPLUS_CHG_TRACK_COOL_DOWN_PACK_LEN] = { 0 };
	int i;

	if (monitor == NULL || p_trigger_data == NULL || track_status == NULL)
		return;

	memset(p_trigger_data->crux_info, 0, sizeof(p_trigger_data->crux_info));
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$power_mode@@%s",
			  track_status->power_info.power_mode);

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRE) {
		index += snprintf(
			&(p_trigger_data->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$adapter_t@@%s",
			track_status->power_info.wired_info.adapter_type);
		if (track_status->power_info.wired_info.adapter_id)
			index += snprintf(
				&(p_trigger_data->crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"$$adapter_id@@0x%x",
				track_status->power_info.wired_info.adapter_id);
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$power@@%d",
				  track_status->power_info.wired_info.power);

		if (track_status->wired_max_power <= 0)
			index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d", -1);
		else
			index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d",
				(track_status->power_info.wired_info.power >= track_status->wired_max_power));
	} else if (track_status->power_info.power_type ==
		   TRACK_CHG_TYPE_WIRELESS) {
		index += snprintf(
			&(p_trigger_data->crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$adapter_t@@%s",
			track_status->power_info.wls_info.adapter_type);
		if (strlen(track_status->power_info.wls_info.dock_type))
			index += snprintf(
				&(p_trigger_data->crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"$$dock_type@@%s",
				track_status->power_info.wls_info.dock_type);
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$power@@%d",
				  track_status->power_info.wls_info.power);

		if (track_status->wls_max_power <= 0)
			index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d", -1);
		else
			index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d",
				(track_status->power_info.wls_info.power >= track_status->wls_max_power));
	}

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$start_soc@@%d", track_status->chg_start_soc);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$end_soc@@%d", track_status->chg_end_soc);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$start_temp@@%d", track_status->chg_start_temp);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$max_temp@@%d", track_status->chg_max_temp);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_start_temp@@%d",
			  track_status->batt_start_temp);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_max_temp@@%d", track_status->batt_max_temp);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_max_vol@@%d", track_status->batt_max_vol);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_max_curr@@%d", track_status->batt_max_curr);
	index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$chg_max_vol@@%d", track_status->chg_max_vol);

	index += snprintf(&(p_trigger_data->crux_info[index]),
			 OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			 "$$ledon_time@@%d", track_status->continue_ledon_time);
	if (track_status->ledon_ave_speed)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$ledon_ave_speed@@%d",
				  track_status->ledon_ave_speed);

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$ledoff_time@@%d",
			  track_status->continue_ledoff_time);
	if (track_status->ledoff_ave_speed)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$ledoff_ave_speed@@%d",
				  track_status->ledoff_ave_speed);

	if (track_status->chg_five_mins_cap != TRACK_PERIOD_CHG_CAP_INIT)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$chg_five_mins_cap@@%d",
				  track_status->chg_five_mins_cap);

	if (track_status->chg_ten_mins_cap != TRACK_PERIOD_CHG_CAP_INIT)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$chg_ten_mins_cap@@%d",
				  track_status->chg_ten_mins_cap);

	if (track_status->chg_average_speed !=
	    TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$chg_average_speed@@%d",
				  track_status->chg_average_speed);

	if (track_status->chg_fast_full_time)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$fast_full_time@@%d",
				  track_status->chg_fast_full_time);

	if (track_status->chg_report_full_time)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$report_full_time@@%d",
				  track_status->chg_report_full_time);

	if (track_status->chg_normal_full_time)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$normal_full_time@@%d",
				  track_status->chg_normal_full_time);

	if (strlen(track_status->batt_full_reason))
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$full_reason@@%s",
				  track_status->batt_full_reason);

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$chg_warm_once@@%d", track_status->tbatt_warm_once);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_fcc@@%d", monitor->batt_fcc);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_soh@@%d", monitor->batt_soh);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$batt_cc@@%d", monitor->batt_cc);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$rechg_counts@@%d", track_status->rechg_counts);

	if (strlen(track_status->chg_abnormal_reason))
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$chg_abnormal@@%s",
				  track_status->chg_abnormal_reason);

	oplus_chg_track_pack_cool_down_stats(track_status, cool_down_pack);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$cool_down_sta@@%s", cool_down_pack);

	oplus_chg_track_pack_app_stats(p_trigger_data->crux_info, &index);
	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$plugin_utc_t@@%d", track_status->chg_plugin_utc_t);

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$plugout_utc_t@@%d",
			  track_status->chg_plugout_utc_t);

	if (track_status->aging_ffc_trig && index < OPLUS_CHG_TRACK_CURX_INFO_LEN) {
		index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$aging_ffc@@%d", track_status->aging_ffc_trig);
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			if (track_status->aging_ffc_judge_vol[i] <= 0 ||
			    index >= OPLUS_CHG_TRACK_CURX_INFO_LEN)
				break;
			index += snprintf(&(p_trigger_data->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  ",%d", track_status->aging_ffc_judge_vol[i]);
		}
		if (track_status->aging_ffc_to_full_time && index < OPLUS_CHG_TRACK_CURX_INFO_LEN) {
			index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$aging_ffc_t@@%d", track_status->aging_ffc_to_full_time);
		}
	}

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$mmi_chg@@%d", track_status->once_mmi_chg);
	if(track_status->mmi_chg_open_t) {
		if(!track_status->mmi_chg_close_t) {
			track_status->mmi_chg_close_t = track_status->chg_plugout_utc_t;
			track_status->mmi_chg_constant_t =
				track_status->chg_plugout_utc_t - track_status->mmi_chg_open_t;
		}
		index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"$$mmi_sta@@open,%d;", track_status->mmi_chg_open_t);
		index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"close,%d;", track_status->mmi_chg_close_t);
		index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"constant,%d", track_status->mmi_chg_constant_t);
	}

	if (track_status->slow_chg_open_t) {
		if (!track_status->slow_chg_close_t) {
			if (track_status->slow_chg_open_n_t)
				track_status->slow_chg_duration +=
					track_status->chg_plugout_utc_t - track_status->slow_chg_open_n_t;
			else
				track_status->slow_chg_duration +=
					track_status->chg_plugout_utc_t - track_status->slow_chg_open_t;
		}
		index += snprintf(&(p_trigger_data->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$slow_chg@@%d,%d,%d,%d,%d,%d,%d", track_status->slow_chg_open_t,
				  track_status->slow_chg_open_n_t, track_status->slow_chg_close_t,
				  track_status->slow_chg_open_cnt, track_status->slow_chg_duration,
				  track_status->slow_chg_pct, track_status->slow_chg_watt);
	}

	index += snprintf(&(p_trigger_data->crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$chg_cycle_status@@%d", track_status->once_chg_cycle_status);
	if (track_status->ffc_time) {
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$ffc_cv_status@@"
				  "%d,%d,%d,%d,%d,%d",
				  track_status->ffc_time, track_status->ffc_start_main_soc,
				  track_status->ffc_start_sub_soc, track_status->ffc_end_main_soc,
				  track_status->ffc_end_sub_soc, track_status->cv_time);
	}
	if (track_status->dual_chan_time)
		index += snprintf(&(p_trigger_data->crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$dual_chan_status@@%d,%d",
				  track_status->dual_chan_open_count, track_status->dual_chan_time);

	oplus_chg_track_record_general_info(monitor, track_status,
					    p_trigger_data, index);
}

static void oplus_chg_track_charger_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, charger_info_trigger_work);

	if (!chip)
		return;

	chip->track_status.wls_need_upload = false;
	chip->track_status.wls_need_upload = false;
	oplus_chg_track_upload_trigger_data(chip->charger_info_trigger);
}

static void oplus_chg_track_no_charging_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, no_charging_trigger_work);

	if (!chip)
		return;

	chip->track_status.wls_need_upload = false;
	chip->track_status.wls_need_upload = false;
	oplus_chg_track_upload_trigger_data(chip->no_charging_trigger);
}

static void oplus_chg_track_slow_charging_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, slow_charging_trigger_work);

	if (!chip)
		return;

	chip->track_status.wls_need_upload = false;
	chip->track_status.wls_need_upload = false;
	oplus_chg_track_upload_trigger_data(chip->slow_charging_trigger);
}

static void
oplus_chg_track_charging_break_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, charging_break_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->charging_break_trigger);
}

static void
oplus_chg_track_wls_charging_break_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, wls_charging_break_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->wls_charging_break_trigger);
}

static void oplus_chg_track_usbtemp_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, usbtemp_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->usbtemp_load_trigger);
}

static void
oplus_chg_track_vbatt_too_low_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, vbatt_too_low_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->vbatt_too_low_load_trigger);
}

static void
oplus_chg_track_vbatt_diff_over_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip =
		container_of(dwork, struct oplus_chg_track,
			     vbatt_diff_over_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->vbatt_diff_over_load_trigger);
}

static void
oplus_chg_track_uisoc_keep_1_t_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip =
		container_of(dwork, struct oplus_chg_track,
			     uisoc_keep_1_t_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->uisoc_keep_1_t_load_trigger);
}

static void oplus_chg_track_ic_err_msg_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, ic_err_msg_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(chip->ic_err_msg_load_trigger);
}

static void oplus_chg_track_chg_into_liquid_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, chg_into_liquid_trigger_work);

	if (!chip->track_status.debug_close_3hours)
		chip->oplus_liquid_intake_enable = false;
	oplus_chg_track_upload_trigger_data(chip->chg_into_liquid_load_trigger);
}

static void oplus_chg_track_chg_into_liquid_trigger_timeout_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, chg_into_liquid_trigger_work_timeout);

	chip->oplus_liquid_intake_enable = true;
}

static void
oplus_chg_track_dual_chan_err_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, dual_chan_err_load_trigger_work);

	oplus_chg_track_upload_trigger_data(chip->dual_chan_err_load_trigger);
}

static void
oplus_chg_track_cal_chg_five_mins_capacity_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, cal_chg_five_mins_capacity_work);
	struct oplus_monitor *monitor = chip->monitor;

	chip->track_status.chg_five_mins_cap =
		monitor->batt_soc - chip->track_status.chg_start_soc;
	chg_info(
		"chg_five_mins_soc:%d, start_chg_soc:%d, chg_five_mins_cap:%d\n",
		monitor->batt_soc, chip->track_status.chg_start_soc,
		chip->track_status.chg_five_mins_cap);
}

static void
oplus_chg_track_cal_chg_ten_mins_capacity_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, cal_chg_ten_mins_capacity_work);
	struct oplus_monitor *monitor = chip->monitor;

	chip->track_status.chg_ten_mins_cap =
		monitor->batt_soc - chip->track_status.chg_start_soc;
	chg_info("chg_ten_mins_soc:%d, start_chg_soc:%d, chg_ten_mins_cap:%d\n",
		 monitor->batt_soc, chip->track_status.chg_start_soc,
		 chip->track_status.chg_ten_mins_cap);
}

static void oplus_chg_track_check_plugout_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(
		dwork, struct oplus_chg_track, plugout_state_work);
	struct oplus_monitor *monitor = chip->monitor;
	struct oplus_chg_track_status *track_status = &chip->track_status;

	if (((monitor->wired_online == false) &&
		(monitor->wired_vbus_mv < 2500)) &&
		((monitor->vooc_started == true))) {

		if (monitor->vooc_started == true)
			chip->plugout_state_trigger.flag_reason = TRACK_NOTIFY_FLAG_FASTCHG_START_ABNORMAL;

		oplus_chg_track_record_charger_info(monitor, &chip->plugout_state_trigger, track_status);
		oplus_chg_track_upload_trigger_data(chip->plugout_state_trigger);
	}

	if (track_status->debug_plugout_state) {
		chip->plugout_state_trigger.flag_reason = track_status->debug_plugout_state;
		oplus_chg_track_record_charger_info(monitor, &chip->plugout_state_trigger, track_status);
		oplus_chg_track_upload_trigger_data(chip->plugout_state_trigger);
		track_status->debug_plugout_state = 0;
	}
}

static void oplus_chg_track_mmi_chg_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(dwork, struct oplus_chg_track, mmi_chg_info_trigger_work);

	if (chip->mmi_chg_info_trigger) {
		oplus_chg_track_upload_trigger_data(*(chip->mmi_chg_info_trigger));
		kfree(chip->mmi_chg_info_trigger);
		chip->mmi_chg_info_trigger = NULL;
	}
	mutex_unlock(&chip->mmi_chg_info_lock);
}

static void oplus_chg_track_slow_chg_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(dwork, struct oplus_chg_track, slow_chg_info_trigger_work);

	if (chip->slow_chg_info_trigger) {
		oplus_chg_track_upload_trigger_data(*(chip->slow_chg_info_trigger));
		kfree(chip->slow_chg_info_trigger);
		chip->slow_chg_info_trigger = NULL;
	}
	mutex_unlock(&chip->slow_chg_info_lock);
}

static void oplus_chg_track_chg_cycle_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(dwork, struct oplus_chg_track, chg_cycle_info_trigger_work);

	if (chip->chg_cycle_info_trigger) {
		oplus_chg_track_upload_trigger_data(*(chip->chg_cycle_info_trigger));
		kfree(chip->chg_cycle_info_trigger);
		chip->chg_cycle_info_trigger = NULL;
	}
	mutex_unlock(&chip->chg_cycle_info_lock);
}

static void oplus_chg_track_wls_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(dwork, struct oplus_chg_track, wls_info_trigger_work);

	if (chip->wls_info_trigger) {
		oplus_chg_track_upload_trigger_data(*(chip->wls_info_trigger));
		kfree(chip->wls_info_trigger);
		chip->wls_info_trigger = NULL;
	}
	mutex_unlock(&chip->wls_info_lock);
}

static void oplus_chg_track_ufcs_info_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip = container_of(dwork, struct oplus_chg_track, ufcs_info_trigger_work);

	if (chip->ufcs_info_trigger) {
		oplus_chg_track_upload_trigger_data(*(chip->ufcs_info_trigger));
		kfree(chip->ufcs_info_trigger);
		chip->ufcs_info_trigger = NULL;
	}
	mutex_unlock(&chip->ufcs_info_lock);
}

static int oplus_chg_track_speed_ref_init(struct oplus_chg_track *chip)
{
	if (!chip)
		return -1;

	if (chip->track_cfg.wired_fast_chg_scheme >= 0 &&
	    chip->track_cfg.wired_fast_chg_scheme <
		    ARRAY_SIZE(g_wired_speed_ref_standard))
		chip->track_status.wired_speed_ref = g_wired_speed_ref_standard
			[chip->track_cfg.wired_fast_chg_scheme];

	if (chip->track_cfg.wls_fast_chg_scheme >= 0 &&
	    chip->track_cfg.wls_fast_chg_scheme <
		    ARRAY_SIZE(g_wls_fast_speed_ref_standard))
		chip->track_status.wls_airvooc_speed_ref =
			g_wls_fast_speed_ref_standard
				[chip->track_cfg.wls_fast_chg_scheme];

	if (chip->track_cfg.wls_epp_chg_scheme >= 0 &&
	    chip->track_cfg.wls_epp_chg_scheme <
		    ARRAY_SIZE(g_wls_epp_speed_ref_standard))
		chip->track_status.wls_epp_speed_ref =
			g_wls_epp_speed_ref_standard
				[chip->track_cfg.wls_epp_chg_scheme];

	if (chip->track_cfg.wls_bpp_chg_scheme >= 0 &&
	    chip->track_cfg.wls_bpp_chg_scheme <
		    ARRAY_SIZE(g_wls_bpp_speed_ref_standard))
		chip->track_status.wls_bpp_speed_ref =
			g_wls_bpp_speed_ref_standard
				[chip->track_cfg.wls_bpp_chg_scheme];

	return 0;
}

static int oplus_chg_track_init(struct oplus_chg_track *track_dev)
{
	int ret = 0;
	struct oplus_chg_track *chip = track_dev;

	chip->trigger_data_ok = false;
	mutex_init(&chip->upload_lock);
	mutex_init(&chip->trigger_data_lock);
	mutex_init(&chip->trigger_ack_lock);
	init_waitqueue_head(&chip->upload_wq);
	init_completion(&chip->trigger_ack);
	mutex_init(&track_dev->dcs_info_lock);
	mutex_init(&chip->access_lock);
	mutex_init(&chip->track_status.app_status.app_lock);
	mutex_init(&chip->mmi_chg_info_lock);
	mutex_init(&chip->slow_chg_info_lock);
	mutex_init(&chip->chg_cycle_info_lock);
	mutex_init(&chip->wls_info_lock);
	mutex_init(&chip->ufcs_info_lock);

	chip->track_status.curr_soc = -EINVAL;
	chip->track_status.curr_smooth_soc = -EINVAL;
	chip->track_status.curr_uisoc = -EINVAL;
	chip->track_status.pre_soc = -EINVAL;
	chip->track_status.pre_smooth_soc = -EINVAL;
	chip->track_status.pre_uisoc = -EINVAL;
	chip->track_status.soc_jumped = false;
	chip->track_status.uisoc_jumped = false;
	chip->track_status.uisoc_to_soc_jumped = false;
	chip->track_status.uisoc_load_jumped = false;
	chip->track_status.debug_soc = OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID;
	chip->track_status.debug_uisoc =
		OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID;
	chip->track_status.debug_close_3hours = false;
	chip->track_status.debug_water_inlet_para = DEBUG_WATER_INLET_PARA;
	chip->uisoc_load_trigger.type_reason = TRACK_NOTIFY_TYPE_SOC_JUMP;
	chip->uisoc_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_UI_SOC_LOAD_JUMP;
	chip->soc_trigger.type_reason = TRACK_NOTIFY_TYPE_SOC_JUMP;
	chip->soc_trigger.flag_reason = TRACK_NOTIFY_FLAG_SOC_JUMP;
	chip->uisoc_trigger.type_reason = TRACK_NOTIFY_TYPE_SOC_JUMP;
	chip->uisoc_trigger.flag_reason = TRACK_NOTIFY_FLAG_UI_SOC_JUMP;
	chip->uisoc_to_soc_trigger.type_reason = TRACK_NOTIFY_TYPE_SOC_JUMP;
	chip->uisoc_to_soc_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_UI_SOC_TO_SOC_JUMP;
	chip->charger_info_trigger.type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->charger_info_trigger.flag_reason = TRACK_NOTIFY_FLAG_CHARGER_INFO;
	chip->no_charging_trigger.type_reason = TRACK_NOTIFY_TYPE_NO_CHARGING;
	chip->no_charging_trigger.flag_reason = TRACK_NOTIFY_FLAG_NO_CHARGING;
	chip->slow_charging_trigger.type_reason =
		TRACK_NOTIFY_TYPE_CHARGING_SLOW;
	chip->charging_break_trigger.type_reason =
		TRACK_NOTIFY_TYPE_CHARGING_BREAK;
	chip->wls_charging_break_trigger.type_reason =
		TRACK_NOTIFY_TYPE_CHARGING_BREAK;
	chip->uisoc_keep_1_t_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->uisoc_keep_1_t_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_UISOC_KEEP_1_T_INFO;
	chip->vbatt_too_low_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->vbatt_too_low_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_VBATT_TOO_LOW_INFO;
	chip->usbtemp_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->usbtemp_load_trigger.flag_reason = TRACK_NOTIFY_FLAG_USBTEMP_INFO;
	chip->vbatt_diff_over_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->vbatt_diff_over_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_VBATT_DIFF_OVER_INFO;
	chip->ic_err_msg_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL;
	chip->ic_err_msg_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_DEFAULT;
	chip->chg_into_liquid_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_CHARGING_BREAK;
	chip->chg_into_liquid_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_CHG_FEED_LIQUOR;
	chip->dual_chan_err_load_trigger.type_reason =
		TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL;
	chip->dual_chan_err_load_trigger.flag_reason =
		TRACK_NOTIFY_FLAG_DUAL_CHAN_ABNORMAL;

	memset(&(chip->track_status.power_info), 0,
	       sizeof(chip->track_status.power_info));
	strcpy(chip->track_status.power_info.power_mode, "unknow");
	chip->track_status.wired_max_power = chip->track_cfg.wired_max_power;
	chip->track_status.wls_max_power = chip->track_cfg.wls_max_power;
	chip->track_status.chg_no_charging_cnt = 0;
	chip->track_status.chg_total_cnt = 0;
	chip->track_status.chg_max_temp = 0;
	chip->track_status.chg_fast_full_time = 0;
	chip->track_status.chg_normal_full_time = 0;
	chip->track_status.chg_report_full_time = 0;
	chip->track_status.debug_normal_charging_state =
		POWER_SUPPLY_STATUS_CHARGING;
	chip->track_status.debug_fast_prop_status = TRACK_FASTCHG_STATUS_UNKOWN;
	chip->track_status.debug_normal_prop_status =
		POWER_SUPPLY_STATUS_UNKNOWN;
	chip->track_status.debug_no_charging = 0;
	chip->track_status.chg_five_mins_cap = TRACK_PERIOD_CHG_CAP_INIT;
	chip->track_status.chg_ten_mins_cap = TRACK_PERIOD_CHG_CAP_INIT;
	chip->track_status.chg_average_speed =
		TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT;
	chip->track_status.chg_attach_time_ms =
		chip->track_cfg.fast_chg_break_t_thd;
	chip->track_status.chg_detach_time_ms = 0;
	chip->track_status.wls_attach_time_ms =
		chip->track_cfg.wls_chg_break_t_thd;
	chip->track_status.wls_detach_time_ms = 0;
	chip->track_status.soc_sect_status = TRACK_SOC_SECTION_DEFAULT;
	chip->track_status.chg_speed_is_slow = false;
	chip->track_status.tbatt_warm_once = false;
	chip->track_status.tbatt_cold_once = false;
	chip->track_status.cool_down_effect_cnt = 0;
	chip->track_status.wls_need_upload = false;
	chip->track_status.wired_need_upload = false;
	chip->oplus_liquid_intake_enable = true;

	chip->track_status.app_status.app_cal = false;
	chip->track_status.app_status.curr_top_index = TRACK_APP_TOP_INDEX_DEFAULT;
	strncpy(chip->track_status.app_status.curr_top_name,
		TRACK_APP_REAL_NAME_DEFAULT, TRACK_APP_REAL_NAME_LEN - 1);
	chip->track_status.once_mmi_chg = false;
	chip->track_status.once_chg_cycle_status = CHG_CYCLE_VOTER__NONE;
	chip->track_status.mmi_chg_open_t = 0;
	chip->track_status.mmi_chg_close_t = 0;
	chip->track_status.mmi_chg_constant_t = 0;

	chip->track_status.slow_chg_open_t = 0;
	chip->track_status.slow_chg_close_t = 0;
	chip->track_status.slow_chg_open_n_t = 0;
	chip->track_status.slow_chg_duration = 0;
	chip->track_status.slow_chg_open_cnt = 0;
	chip->track_status.slow_chg_watt = 0;
	chip->track_status.slow_chg_pct = 0;

	chip->track_status.debug_plugout_state = 0;
	chip->track_status.debug_break_code = 0;
	chip->track_status.aging_ffc_start_time = 0;
	chip->track_status.ffc_end_time = 0;
	chip->track_status.ffc_time = 0;
	chip->track_status.cv_time = 0;
	chip->track_status.ffc_start_main_soc = 0;
	chip->track_status.ffc_start_sub_soc = 0;
	chip->track_status.ffc_end_main_soc = 0;
	chip->track_status.ffc_end_sub_soc = 0;
	chip->track_status.aging_ffc_to_full_time = 0;
	chip->track_status.dual_chan_start_time = 0;
	chip->track_status.dual_chan_time = 0;
	chip->track_status.dual_chan_open_count = 0;

	memset(&(chip->track_status.fastchg_break_info), 0,
	       sizeof(chip->track_status.fastchg_break_info));
	memset(&(chip->track_status.wired_break_crux_info), 0,
	       sizeof(chip->track_status.wired_break_crux_info));
	memset(&(chip->track_status.wls_break_crux_info), 0,
	       sizeof(chip->track_status.wls_break_crux_info));

	memset(&(chip->track_status.batt_full_reason), 0,
	       sizeof(chip->track_status.batt_full_reason));
	oplus_chg_track_clear_cool_down_stats_time(&(chip->track_status));

	memset(&(chip->voocphy_name), 0, sizeof(chip->voocphy_name));
	oplus_chg_track_set_voocphy_name(chip);
	oplus_chg_track_speed_ref_init(chip);

	INIT_DELAYED_WORK(&chip->uisoc_load_trigger_work,
			  oplus_chg_track_uisoc_load_trigger_work);
	INIT_DELAYED_WORK(&chip->soc_trigger_work,
			  oplus_chg_track_soc_trigger_work);
	INIT_DELAYED_WORK(&chip->uisoc_trigger_work,
			  oplus_chg_track_uisoc_trigger_work);
	INIT_DELAYED_WORK(&chip->uisoc_to_soc_trigger_work,
			  oplus_chg_track_uisoc_to_soc_trigger_work);
	INIT_DELAYED_WORK(&chip->charger_info_trigger_work,
			  oplus_chg_track_charger_info_trigger_work);
	INIT_DELAYED_WORK(&chip->cal_chg_five_mins_capacity_work,
			  oplus_chg_track_cal_chg_five_mins_capacity_work);
	INIT_DELAYED_WORK(&chip->cal_chg_ten_mins_capacity_work,
			  oplus_chg_track_cal_chg_ten_mins_capacity_work);
	INIT_DELAYED_WORK(&chip->no_charging_trigger_work,
			  oplus_chg_track_no_charging_trigger_work);
	INIT_DELAYED_WORK(&chip->slow_charging_trigger_work,
			  oplus_chg_track_slow_charging_trigger_work);
	INIT_DELAYED_WORK(&chip->charging_break_trigger_work,
			  oplus_chg_track_charging_break_trigger_work);
	INIT_DELAYED_WORK(&chip->wls_charging_break_trigger_work,
			  oplus_chg_track_wls_charging_break_trigger_work);
	INIT_DELAYED_WORK(&chip->usbtemp_load_trigger_work,
			  oplus_chg_track_usbtemp_load_trigger_work);
	INIT_DELAYED_WORK(&chip->vbatt_too_low_load_trigger_work,
			  oplus_chg_track_vbatt_too_low_load_trigger_work);
	INIT_DELAYED_WORK(&chip->vbatt_diff_over_load_trigger_work,
			  oplus_chg_track_vbatt_diff_over_load_trigger_work);
	INIT_DELAYED_WORK(&chip->uisoc_keep_1_t_load_trigger_work,
			  oplus_chg_track_uisoc_keep_1_t_load_trigger_work);
	INIT_DELAYED_WORK(&chip->ic_err_msg_trigger_work,
			  oplus_chg_track_ic_err_msg_trigger_work);
	INIT_DELAYED_WORK(&chip->chg_into_liquid_trigger_work,
			  oplus_chg_track_chg_into_liquid_trigger_work);
	INIT_DELAYED_WORK(&chip->chg_into_liquid_trigger_work_timeout,
			  oplus_chg_track_chg_into_liquid_trigger_timeout_work);
	INIT_DELAYED_WORK(&chip->plugout_state_work,
			  oplus_chg_track_check_plugout_work);
	INIT_DELAYED_WORK(&chip->dual_chan_err_load_trigger_work,
			  oplus_chg_track_dual_chan_err_load_trigger_work);
	INIT_DELAYED_WORK(&chip->mmi_chg_info_trigger_work, oplus_chg_track_mmi_chg_info_trigger_work);
	INIT_DELAYED_WORK(&chip->slow_chg_info_trigger_work, oplus_chg_track_slow_chg_info_trigger_work);
	INIT_DELAYED_WORK(&chip->chg_cycle_info_trigger_work, oplus_chg_track_chg_cycle_info_trigger_work);
	INIT_DELAYED_WORK(&chip->wls_info_trigger_work, oplus_chg_track_wls_info_trigger_work);
	INIT_DELAYED_WORK(&chip->ufcs_info_trigger_work, oplus_chg_track_ufcs_info_trigger_work);
	return ret;
}

#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
static int oplus_chg_track_get_type_tag(int type_reason, char *type_reason_tag)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(track_type_reason_table); i++) {
		if (track_type_reason_table[i].type_reason == type_reason) {
			strncpy(type_reason_tag,
				track_type_reason_table[i].type_reason_tag,
				OPLUS_CHG_TRIGGER_REASON_TAG_LEN - 1);
			break;
		}
	}
	return i;
}

static int oplus_chg_track_get_flag_tag(int flag_reason, char *flag_reason_tag)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(track_flag_reason_table); i++) {
		if (track_flag_reason_table[i].flag_reason == flag_reason) {
			strncpy(flag_reason_tag,
				track_flag_reason_table[i].flag_reason_tag,
				OPLUS_CHG_TRIGGER_REASON_TAG_LEN - 1);
			break;
		}
	}
	return i;
}
#else
static int oplus_chg_track_get_flag_tag(int flag_reason, char *flag_reason_tag)
{
	return 0;
}
#endif

static bool oplus_chg_track_trigger_data_is_valid(oplus_chg_track_trigger *pdata)
{
	int i;
	int len;
	bool ret = false;
	int type_reason = pdata->type_reason;
	int flag_reason = pdata->flag_reason;

	len = strlen(pdata->crux_info);
	if (!len) {
		chg_err("crux_info lens is invaild\n");
		return ret;
	}

	switch (type_reason) {
	case TRACK_NOTIFY_TYPE_SOC_JUMP:
		for (i = TRACK_NOTIFY_FLAG_SOC_JUMP_FIRST; i <= TRACK_NOTIFY_FLAG_SOC_JUMP_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_GENERAL_RECORD:
		for (i = TRACK_NOTIFY_FLAG_GENERAL_RECORD_FIRST; i <= TRACK_NOTIFY_FLAG_GENERAL_RECORD_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_NO_CHARGING:
		for (i = TRACK_NOTIFY_FLAG_NO_CHARGING_FIRST; i <= TRACK_NOTIFY_FLAG_NO_CHARGING_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_CHARGING_SLOW:
		for (i = TRACK_NOTIFY_FLAG_CHARGING_SLOW_FIRST; i <= TRACK_NOTIFY_FLAG_CHARGING_SLOW_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_CHARGING_BREAK:
		for (i = TRACK_NOTIFY_FLAG_CHARGING_BREAK_FIRST; i <= TRACK_NOTIFY_FLAG_CHARGING_BREAK_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL:
		for (i = TRACK_NOTIFY_FLAG_DEVICE_ABNORMAL_FIRST; i <= TRACK_NOTIFY_FLAG_DEVICE_ABNORMAL_LAST; i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	case TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL:
		for (i = TRACK_NOTIFY_FLAG_SOFTWARE_ABNORMAL_FIRST; i <= TRACK_NOTIFY_FLAG_SOFTWARE_ABNORMAL_LAST;
		     i++) {
			if (flag_reason == i) {
				ret = true;
				break;
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	if (!ret)
		chg_err("type_reason or flag_reason is invaild\n");
	return ret;
}

static int oplus_chg_track_upload_trigger_data(oplus_chg_track_trigger data)
{
	int rc;
	struct oplus_chg_track *chip = g_track_chip;
	char flag_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN] = { 0 };

	if (!g_track_chip)
		return TRACK_CMD_ERROR_CHIP_NULL;

	if (!oplus_chg_track_trigger_data_is_valid(&data))
		return TRACK_CMD_ERROR_DATA_INVALID;

	mutex_lock(&chip->trigger_ack_lock);
	mutex_lock(&chip->trigger_data_lock);
	memset(&chip->trigger_data, 0, sizeof(oplus_chg_track_trigger));
	chip->trigger_data.type_reason = data.type_reason;
	chip->trigger_data.flag_reason = data.flag_reason;
	strncpy(chip->trigger_data.crux_info, data.crux_info,
		OPLUS_CHG_TRACK_CURX_INFO_LEN - 1);
	chg_info("type_reason:%d, flag_reason:%d, crux_info[%s]\n",
		 chip->trigger_data.type_reason, chip->trigger_data.flag_reason,
		 chip->trigger_data.crux_info);
	chip->trigger_data_ok = true;
	oplus_chg_track_get_flag_tag(chip->trigger_data.flag_reason, flag_reason_tag);
	chg_exception_report(&chip->track_cfg.exception_data, chip->trigger_data.type_reason,
				chip->trigger_data.flag_reason, flag_reason_tag, sizeof(flag_reason_tag));
	mutex_unlock(&chip->trigger_data_lock);
	reinit_completion(&chip->trigger_ack);
	wake_up(&chip->upload_wq);

	rc = wait_for_completion_timeout(
		&chip->trigger_ack,
		msecs_to_jiffies(OPLUS_CHG_TRACK_WAIT_TIME_MS));
	if (!rc) {
		if (delayed_work_pending(&chip->upload_info_dwork))
			cancel_delayed_work_sync(&chip->upload_info_dwork);
		chg_err("Error, timed out upload trigger data\n");
		mutex_unlock(&chip->trigger_ack_lock);
		return TRACK_CMD_ERROR_TIME_OUT;
	}
	rc = 0;
	chg_info("success\n");
	mutex_unlock(&chip->trigger_ack_lock);

	return rc;
}

static int oplus_chg_track_thread(void *data)
{
	int rc = 0;
	struct oplus_chg_track *chip = (struct oplus_chg_track *)data;

	while (!kthread_should_stop()) {
		mutex_lock(&chip->upload_lock);
		rc = wait_event_interruptible(chip->upload_wq,
					      chip->trigger_data_ok);
		mutex_unlock(&chip->upload_lock);
		if (rc)
			return rc;
		if (!chip->trigger_data_ok)
			chg_err("oplus chg false wakeup, rc=%d\n", rc);
		mutex_lock(&chip->trigger_data_lock);
		chip->trigger_data_ok = false;
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
		oplus_chg_track_pack_dcs_info(chip);
#endif
		chip->dwork_retry_cnt = OPLUS_CHG_TRACK_DWORK_RETRY_CNT;
		queue_delayed_work(chip->trigger_upload_wq,
				   &chip->upload_info_dwork, 0);
		mutex_unlock(&chip->trigger_data_lock);
	}

	return rc;
}

static int oplus_chg_track_thread_init(struct oplus_chg_track *track_dev)
{
	int rc = 0;
	struct oplus_chg_track *chip = track_dev;

	chip->track_upload_kthread = kthread_run(oplus_chg_track_thread, chip,
						 "track_upload_kthread");
	if (IS_ERR(chip->track_upload_kthread)) {
		chg_err("failed to create oplus_chg_track_thread\n");
		return -1;
	}

	return rc;
}

static int oplus_chg_track_get_current_time_s(struct rtc_time *tm)
{
	struct timespec ts;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, tm);
	tm->tm_year = tm->tm_year + 1900;
	tm->tm_mon = tm->tm_mon + 1;
	return ts.tv_sec;
}

static int oplus_chg_track_get_current_time(struct rtc_time *tm)
{
	struct timespec ts;
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	track_chip = g_track_chip;
	track_status = &track_chip->track_status;

	if (!track_chip || !track_status)
		return -1;

	getnstimeofday(&ts);
	ts.tv_sec += track_status->track_gmtoff;
	rtc_time_to_tm(ts.tv_sec, tm);
	tm->tm_year = tm->tm_year + 1900;
	tm->tm_mon = tm->tm_mon + 1;

	return ts.tv_sec;
}

static int oplus_chg_track_get_local_time_s(void)
{
	int local_time_s;

	local_time_s = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;
	chg_debug("local_time_s:%d\n", local_time_s);

	return local_time_s;
}

/*
* track sub version
* 3: default version for chg track
* 3.1: compatible with historical keywords
* 3.2: add the pd_sdp type to solve the problem that the type is unknown
*      add match_power to judge the power match between the adapter and the mobile phone
*      add 44W/55W/88W/125W adapter id
*      add pd_svooc type
* 3.3: add app record track feature
* 3.4: update reserve soc track
*/
#define TRACK_VERSION	"3.4"

#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
static int oplus_chg_track_pack_dcs_info(struct oplus_chg_track *chip)
{
	int ret = 0;
	int len;
	struct rtc_time tm;
	char log_tag[] = OPLUS_CHG_TRACK_LOG_TAG;
	char event_id[] = OPLUS_CHG_TRACK_EVENT_ID;
	char *p_data = (char *)(chip->dcs_info);
	char type_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN] = { 0 };
	char flag_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN] = { 0 };

	memset(p_data, 0x0, sizeof(char) * OPLUS_CHG_TRIGGER_MSG_LEN);
	ret += sizeof(struct kernel_packet_info);
	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret,
			OPLUS_CHG_TRACK_EVENT_ID);

	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret,
			"$$track_ver@@%s", TRACK_VERSION);

	oplus_chg_track_get_type_tag(chip->trigger_data.type_reason,
				     type_reason_tag);
	type_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN - 1] = 0;
	oplus_chg_track_get_flag_tag(chip->trigger_data.flag_reason,
				     flag_reason_tag);
	flag_reason_tag[OPLUS_CHG_TRIGGER_REASON_TAG_LEN - 1] = 0;
	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret,
			"$$type_reason@@%s", type_reason_tag);
	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret,
			"$$flag_reason@@%s", flag_reason_tag);

	oplus_chg_track_get_current_time(&tm);
	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret,
			"$$time@@[%04d-%02d-%02d %02d:%02d:%02d]", tm.tm_year,
			tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min,
			tm.tm_sec);

	ret += snprintf(&p_data[ret], OPLUS_CHG_TRIGGER_MSG_LEN - ret, "%s",
			chip->trigger_data.crux_info);

	len = strlen(&(p_data[sizeof(struct kernel_packet_info)]));
	if (len) {
		mutex_lock(&chip->dcs_info_lock);
		memset(chip->dcs_info, 0x0, sizeof(struct kernel_packet_info));

		chip->dcs_info->type = 1;
		memcpy(chip->dcs_info->log_tag, log_tag, strlen(log_tag));
		memcpy(chip->dcs_info->event_id, event_id, strlen(event_id));
		chip->dcs_info->payload_length = len + 1;
		mutex_unlock(&chip->dcs_info_lock);
		chg_info("%s\n", chip->dcs_info->payload);
		return 0;
	}

	return -1;
}
#endif

static void oplus_chg_track_upload_info_dwork(struct work_struct *work)
{
	int ret = 0;
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_track *chip =
		container_of(dwork, struct oplus_chg_track, upload_info_dwork);

	if (!chip)
		return;

	mutex_lock(&chip->dcs_info_lock);
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) ||	\
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE)
	ret = fb_kevent_send_to_user(chip->dcs_info);
#elif defined(CONFIG_OPLUS_KEVENT_UPLOAD)
	ret = kevent_send_to_user(chip->dcs_info);
#endif
	mutex_unlock(&chip->dcs_info_lock);
	if (!ret)
		complete(&chip->trigger_ack);
	else if (chip->dwork_retry_cnt > 0)
		queue_delayed_work(
			chip->trigger_upload_wq, &chip->upload_info_dwork,
			msecs_to_jiffies(OPLUS_CHG_UPDATE_INFO_DELAY_MS));

	chg_info("retry_cnt: %d, ret = %d\n", chip->dwork_retry_cnt, ret);
	chip->dwork_retry_cnt--;
}

int oplus_chg_track_handle_wired_type_info(
	struct oplus_monitor *monitor, int type)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	if (track_status->power_info.wired_info.adapter_id ||
	    !strcmp(track_status->power_info.wired_info.adapter_type, "vooc") ||
	    (!strcmp(track_status->power_info.wired_info.adapter_type, "pd") &&
	    monitor->wired_charge_type == OPLUS_CHG_USB_TYPE_DCP) ||
	    !strcmp(track_status->power_info.wired_info.adapter_type, "qc")) {
		chg_info("has know type and not handle\n");
		return 0;
	}

	if (!monitor->wired_charge_type) {
		chg_info("wired_charge_type is unknow, no update\n");
		return 0;
	}

	track_status->power_info.power_type = TRACK_CHG_TYPE_WIRE;
	memset(track_status->power_info.power_mode, 0,
	       sizeof(track_status->power_info.power_mode));
	strcpy(track_status->power_info.power_mode, "wired");

	if (type == TRACK_CHG_GET_THTS_TIME_TYPE) {
		track_status->pre_wired_type = monitor->wired_charge_type;
		oplus_chg_track_get_wired_type_info(monitor->wired_charge_type,
						    track_status);
	} else {
		oplus_chg_track_get_wired_type_info(
			track_status->pre_wired_type, track_status);
	}

	if (type == TRACK_CHG_GET_THTS_TIME_TYPE) {
		track_status->fast_chg_type =
			sid_to_adapter_id(monitor->vooc_sid);
		oplus_chg_track_get_vooc_type_info(track_status->fast_chg_type,
						   track_status);
	} else {
		oplus_chg_track_get_vooc_type_info(
			track_status->pre_fastchg_type, track_status);
	}

	if (monitor->pd_svooc && !strcmp(track_status->power_info.wired_info.adapter_type, "svooc"))
		strncpy(track_status->power_info.wired_info.adapter_type,
			"pd_svooc", OPLUS_CHG_TRACK_POWER_TYPE_LEN - 1);

	chg_info("power_mode:%s, type:%s, adapter_id:0x%0x, power:%d\n",
		track_status->power_info.power_mode,
		track_status->power_info.wired_info.adapter_type,
		track_status->power_info.wired_info.adapter_id,
		track_status->power_info.wired_info.power);

	return 0;
}

static int oplus_chg_track_handle_wls_type_info(
	struct oplus_chg_track_status *track_status)
{
	struct oplus_chg_track *track_chip;
	struct oplus_monitor *monitor;
	union mms_msg_data data = { 0 };
	int adapter_type;
	int dock_type;
	int max_wls_power;

	if (!g_track_chip)
		return -1;

	track_chip = g_track_chip;
	monitor = track_chip->monitor;

	track_status->power_info.power_type = TRACK_CHG_TYPE_WIRELESS;
	memset(track_status->power_info.power_mode, 0, sizeof(track_status->power_info.power_mode));
	strcpy(track_status->power_info.power_mode, "wireless");

	if (!track_chip->monitor || !track_chip->monitor->wls_topic)
		return false;
	oplus_mms_get_item_data(track_chip->monitor->wls_topic, WLS_ITEM_WLS_TYPE, &data, true);
	adapter_type = data.intval;
	oplus_mms_get_item_data(track_chip->monitor->wls_topic, WLS_ITEM_DOCK_TYPE, &data, true);
	dock_type = data.intval;
	oplus_mms_get_item_data(track_chip->monitor->wls_topic, WLS_ITEM_MAX_POWER, &data, true);
	max_wls_power = data.intval;

	oplus_chg_track_get_wls_adapter_type_info(adapter_type, track_status);
	oplus_chg_track_get_wls_dock_type_info(dock_type, track_status);
	if (max_wls_power)
		track_status->power_info.wls_info.power = max_wls_power;

	chg_info("power_mode:%s, type:%s, dock_type:%s, power:%d\n", track_status->power_info.power_mode,
		track_status->power_info.wls_info.adapter_type, track_status->power_info.wls_info.dock_type,
		track_status->power_info.wls_info.power);

	return 0;
}

static int oplus_chg_track_handle_batt_full_reason(
	struct oplus_monitor *monitor,
	struct oplus_chg_track_status *track_status)
{
	int notify_flag = NOTIFY_BAT_FULL;

	if (monitor == NULL || track_status == NULL)
		return -1;

	if (monitor->notify_flag == NOTIFY_BAT_FULL ||
	    monitor->notify_flag == NOTIFY_BAT_FULL_PRE_LOW_TEMP ||
	    monitor->notify_flag == NOTIFY_BAT_FULL_PRE_HIGH_TEMP ||
	    monitor->notify_flag == NOTIFY_BAT_NOT_CONNECT ||
	    monitor->notify_flag == NOTIFY_BAT_FULL_THIRD_BATTERY)
		notify_flag = monitor->notify_flag;

	if (track_status->debug_chg_notify_flag)
		notify_flag = track_status->debug_chg_notify_flag;
	if (!strlen(track_status->batt_full_reason))
		oplus_chg_track_get_batt_full_reason_info(notify_flag,
							  track_status);
	chg_info("track_notify_flag:%d, chager_notify_flag:%d, full_reason[%s]\n",
		notify_flag, monitor->notify_flag,
		track_status->batt_full_reason);

	return 0;
}

static int
oplus_chg_track_check_chg_abnormal(struct oplus_monitor *monitor,
				   struct oplus_chg_track_status *track_status)
{
	int notify_code;

	if (monitor == NULL || track_status == NULL)
		return -1;

	if (track_status->debug_chg_notify_code)
		notify_code = track_status->debug_chg_notify_code;
	else
		notify_code = monitor->notify_code;

	if (notify_code & (1 << NOTIFY_CHGING_OVERTIME))
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_CHGING_OVERTIME, track_status);

	if (notify_code & (1 << NOTIFY_CHARGER_OVER_VOL)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_CHARGER_OVER_VOL, track_status);
	} else if (notify_code & (1 << NOTIFY_CHARGER_LOW_VOL)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_CHARGER_LOW_VOL, track_status);
	}

	if (notify_code & (1 << NOTIFY_BAT_OVER_TEMP)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_BAT_OVER_TEMP, track_status);
	} else if (notify_code & (1 << NOTIFY_BAT_LOW_TEMP)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_BAT_LOW_TEMP, track_status);
	}

	if (notify_code & (1 << NOTIFY_BAT_OVER_VOL)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_BAT_OVER_VOL, track_status);
	}

	if (notify_code & (1 << NOTIFY_BAT_NOT_CONNECT)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_BAT_NOT_CONNECT, track_status);
	} else if (notify_code & (1 << NOTIFY_BAT_FULL_THIRD_BATTERY)) {
		oplus_chg_track_get_chg_abnormal_reason_info(
			NOTIFY_BAT_FULL_THIRD_BATTERY, track_status);
	}

	chg_info(
		"track_notify_code:0x%x, chager_notify_code:0x%x, abnormal_reason[%s]\n",
		notify_code, monitor->notify_code,
		track_status->chg_abnormal_reason);

	return 0;
}

static int
oplus_chg_track_cal_chg_common_mesg(struct oplus_monitor *monitor,
				    struct oplus_chg_track_status *track_status)
{
	bool mmi_chg = false;
	static bool pre_slow_chg = false;
	struct rtc_time tm;
	int cep_curr_ma = 0;

	if (monitor == NULL || track_status == NULL)
		return -1;

	if (monitor->shell_temp > track_status->chg_max_temp)
		track_status->chg_max_temp = monitor->shell_temp;

	if (monitor->batt_temp > track_status->batt_max_temp)
		track_status->batt_max_temp = monitor->batt_temp;

	if (monitor->ibat_ma < track_status->batt_max_curr)
		track_status->batt_max_curr = monitor->ibat_ma;

	if (monitor->vbat_mv > track_status->batt_max_vol)
		track_status->batt_max_vol = monitor->vbat_mv;

	if (monitor->wired_vbus_mv > track_status->chg_max_vol)
		track_status->chg_max_vol = monitor->wired_vbus_mv;

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS) {
		if (is_wls_fcc_votable_available(monitor)) {
			if (is_client_vote_enabled(monitor->wls_fcc_votable, CEP_VOTER))
				cep_curr_ma = get_client_vote(monitor->wls_fcc_votable, CEP_VOTER);
			else if (is_client_vote_enabled(monitor->wls_icl_votable, CEP_VOTER))
				cep_curr_ma = get_client_vote(monitor->wls_icl_votable, CEP_VOTER);
		}
		if (cep_curr_ma > 0 && monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING)
			track_status->wls_skew_effect_cnt += 1;
	}

	mmi_chg = oplus_chg_track_get_mmi_chg();
	if (!track_status->once_mmi_chg && !mmi_chg) {
		track_status->once_mmi_chg = true;
		track_status->mmi_chg_open_t =
			oplus_chg_track_get_current_time_s(&track_status->mmi_chg_open_rtc_t);
	}

	if ((track_status->once_mmi_chg == true) && mmi_chg) {
		track_status->mmi_chg_close_t =
			oplus_chg_track_get_current_time_s(&track_status->mmi_chg_close_rtc_t);
		track_status->mmi_chg_constant_t =
			track_status->mmi_chg_close_t - track_status->mmi_chg_open_t;
	}

	if (!track_status->once_chg_cycle_status && monitor->chg_cycle_status)
		track_status->once_chg_cycle_status = monitor->chg_cycle_status;

	if (!pre_slow_chg && monitor->slow_chg_enable) {
		track_status->slow_chg_watt = monitor->slow_chg_watt;
		track_status->slow_chg_pct = monitor->slow_chg_pct;
		if (!track_status->slow_chg_open_t)
			track_status->slow_chg_open_t = oplus_chg_track_get_current_time_s(&tm);
		else
			track_status->slow_chg_open_n_t = oplus_chg_track_get_current_time_s(&tm);
		track_status->slow_chg_open_cnt++;
		track_status->slow_chg_close_t = 0;
	} else if (pre_slow_chg && !monitor->slow_chg_enable) {
		track_status->slow_chg_close_t = oplus_chg_track_get_current_time_s(&tm);
		if (!track_status->slow_chg_open_n_t) {
			track_status->slow_chg_duration +=
				track_status->slow_chg_close_t - track_status->slow_chg_open_t;
		} else {
			track_status->slow_chg_duration +=
				track_status->slow_chg_close_t - track_status->slow_chg_open_n_t;
			track_status->slow_chg_open_n_t = 0;
		}
	}
	pre_slow_chg = monitor->slow_chg_enable;

	chg_info("chg_max_temp:%d, batt_max_temp:%d, batt_max_curr:%d, "
		"batt_max_vol:%d, once_mmi_chg:%d, once_chg_cycle_status:%d\n",
		track_status->chg_max_temp, track_status->batt_max_temp,
		track_status->batt_max_curr, track_status->batt_max_vol,
		track_status->once_mmi_chg, track_status->once_chg_cycle_status);

	return 0;
}

static int
oplus_chg_track_cal_cool_down_stats(struct oplus_monitor *monitor,
				    struct oplus_chg_track_status *track_status)
{
	int cool_down_max;
	int curr_time;
	bool chg_start;

	if (monitor == NULL || track_status == NULL)
		return -1;

	cool_down_max = ARRAY_SIZE(cool_down_stats_table) - 1;
	if (monitor->cool_down > cool_down_max || monitor->cool_down < 0) {
		chg_err("cool_down is invalid\n");
		return -1;
	}

	if (monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING &&
	    monitor->cool_down)
		track_status->cool_down_effect_cnt++;

	curr_time = oplus_chg_track_get_local_time_s();
	chg_start =
		(track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING &&
		 monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING);
	if (chg_start ||
	    (track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING)) {
		track_status->cool_down_status = monitor->cool_down;
		track_status->cool_down_status_change_t = curr_time;
		return 0;
	}

	if (track_status->cool_down_status != monitor->cool_down) {
		cool_down_stats_table[monitor->cool_down].time +=
			curr_time - track_status->cool_down_status_change_t;
	} else {
		cool_down_stats_table[track_status->cool_down_status].time +=
			curr_time - track_status->cool_down_status_change_t;
	}
	track_status->cool_down_status = monitor->cool_down;
	track_status->cool_down_status_change_t = curr_time;

	chg_info("cool_down_status:%d, cool_down_tatus_change_t:%d\n",
		 track_status->cool_down_status,
		 track_status->cool_down_status_change_t);

	return 0;
}

static int
oplus_chg_track_cal_rechg_counts(struct oplus_monitor *monitor,
				 struct oplus_chg_track_status *track_status)

{
	if (monitor == NULL || track_status == NULL)
		return -1;

	if (!track_status->in_rechging && monitor->rechging)
		track_status->rechg_counts++;

	track_status->in_rechging = monitor->rechging;
	return 0;
}

static int oplus_chg_track_cal_no_charging_stats(
	struct oplus_monitor *monitor,
	struct oplus_chg_track_status *track_status)
{
	static int otg_online_cnt = 0;
	static int vbatt_leak_cnt = 0;
	bool no_charging_state = false;
	struct oplus_chg_track *track_chip;

	if (monitor == NULL || track_status == NULL)
		return -1;

	if (!g_track_chip)
		return -1;

	track_chip = g_track_chip;
	if (monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING) {
		track_status->chg_total_cnt++;
		if (monitor->ibat_ma > 0) {
			track_status->chg_no_charging_cnt++;
			no_charging_state = true;
		}
		if (no_charging_state == true) {
			if (monitor->otg_enable)
				otg_online_cnt++;
			else if (monitor->vbat_mv >= monitor->wired_vbus_mv)
				vbatt_leak_cnt++;
		} else {
			otg_online_cnt = 0;
			vbatt_leak_cnt = 0;
		}
	} else {
		otg_online_cnt = 0;
		vbatt_leak_cnt = 0;
	}
	if (otg_online_cnt > 10)
		track_chip->no_charging_trigger.flag_reason =
					TRACK_NOTIFY_FLAG_NO_CHARGING_OTG_ONLINE;
	else if (vbatt_leak_cnt > 10)
		track_chip->no_charging_trigger.flag_reason =
					TRACK_NOTIFY_FLAG_NO_CHARGING_VBATT_LEAK;
	if (track_status->debug_no_charging)
		track_chip->no_charging_trigger.flag_reason =
					track_status->debug_no_charging + TRACK_NOTIFY_FLAG_NO_CHARGING_FIRST - 1;


	return 0;
}

static int oplus_chg_track_cal_ledon_ledoff_average_speed(
	struct oplus_chg_track_status *track_status)
{
	if (track_status->led_onoff_power_cal)
		return 0;

	if (!track_status->ledon_ave_speed) {
		if (!track_status->ledon_time)
			track_status->ledon_ave_speed = 0;
		else
			track_status->ledon_ave_speed =
				TRACK_TIME_1MIN_THD * track_status->ledon_rm /
				track_status->ledon_time;
	}

	if (!track_status->ledoff_ave_speed) {
		if (!track_status->ledoff_time)
			track_status->ledoff_ave_speed = 0;
		else
			track_status->ledoff_ave_speed =
				TRACK_TIME_1MIN_THD * track_status->ledoff_rm /
				track_status->ledoff_time;
	}

	track_status->led_onoff_power_cal = true;
	chg_info("ledon_ave_speed:%d, ledoff_ave_speed:%d\n",
		 track_status->ledon_ave_speed, track_status->ledoff_ave_speed);
	return 0;
}

static int
oplus_chg_track_cal_app_stats(struct oplus_monitor *monitor,
			      struct oplus_chg_track_status *track_status)
{
	int curr_time;
	bool chg_start;
	bool chg_end;
	bool charger_exist;

	if (monitor == NULL || track_status == NULL)
		return -EINVAL;

	if (track_status->app_status.app_cal)
		return 0;

	charger_exist = monitor->wired_online || monitor->wls_online;
	curr_time = oplus_chg_track_get_local_time_s();
	chg_start =
		(track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING &&
		 monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING);
	chg_end = (track_status->prop_status == POWER_SUPPLY_STATUS_CHARGING &&
		   monitor->batt_status != POWER_SUPPLY_STATUS_CHARGING);
	if (chg_start || (!track_status->led_on && monitor->led_on) ||
	    (track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING)) {
		strncpy(track_status->app_status.pre_top_name,
			track_status->app_status.curr_top_name,
			TRACK_APP_REAL_NAME_LEN - 1);
		track_status->app_status.pre_top_name[TRACK_APP_REAL_NAME_LEN - 1] = '\0';
		track_status->app_status.change_t = curr_time;
		track_status->app_status.curr_top_index =
			oplus_chg_track_match_app_info(
			track_status->app_status.pre_top_name);
		return 0;
	}

	if (strcmp(track_status->app_status.pre_top_name,
	    track_status->app_status.curr_top_name) || chg_end ||
	    (track_status->led_on && !monitor->led_on)) {
		chg_info("!!!app change or chg_end or led change, need update\n");
		if (monitor->led_on ||  (track_status->led_on && !monitor->led_on)) {
			if (track_status->app_status.curr_top_index <
			   (ARRAY_SIZE(app_table) - 1))
				app_table[track_status->app_status.curr_top_index].cont_time +=
				curr_time - track_status->app_status.change_t;
			else
				app_table[ARRAY_SIZE(app_table) - 1].cont_time +=
				curr_time - track_status->app_status.change_t;
		}
		track_status->app_status.change_t = curr_time;
		strncpy(track_status->app_status.pre_top_name,
			track_status->app_status.curr_top_name,
			TRACK_APP_REAL_NAME_LEN - 1);
		track_status->app_status.pre_top_name[TRACK_APP_REAL_NAME_LEN - 1] = '\0';
		track_status->app_status.curr_top_index =
			oplus_chg_track_match_app_info(
			track_status->app_status.pre_top_name);
	}

	if (!charger_exist || track_status->chg_report_full_time) {
		track_status->app_status.app_cal = true;
	}

	chg_debug("ch_t:%d, app_cal:%d, curr_top_index:%d, curr_top_name:%s\n",
		track_status->app_status.change_t, track_status->app_status.app_cal,
		track_status->app_status.curr_top_index,
		track_status->app_status.curr_top_name);
	return 0;
}

static int
oplus_chg_track_cal_led_on_stats(struct oplus_monitor *monitor,
				 struct oplus_chg_track_status *track_status)
{
	int curr_time;
	bool chg_start;
	bool chg_end;
	bool need_update = false;
	bool charger_exist;

	if (monitor == NULL || track_status == NULL)
		return -1;

	charger_exist = monitor->wired_online || monitor->wls_online;

	curr_time = oplus_chg_track_get_local_time_s();
	chg_start =
		(track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING &&
		 monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING);
	chg_end = (track_status->prop_status == POWER_SUPPLY_STATUS_CHARGING &&
		   monitor->batt_status != POWER_SUPPLY_STATUS_CHARGING);
	if (chg_start ||
	    (track_status->prop_status != POWER_SUPPLY_STATUS_CHARGING)) {
		track_status->led_on = monitor->led_on;
		track_status->led_change_t = curr_time;
		track_status->led_change_rm = monitor->batt_rm;
		return 0;
	}

	if (chg_end || track_status->chg_fast_full_time ||
	    monitor->batt_soc >= TRACK_LED_MONITOR_SOC_POINT)
		need_update = true;

	if (track_status->led_on && (!monitor->led_on || need_update)) {
		if (curr_time - track_status->led_change_t >
		    TRACK_CYCLE_RECORDIING_TIME_2MIN) {
			track_status->ledon_rm +=
				monitor->batt_rm - track_status->led_change_rm;
			track_status->ledon_time +=
				curr_time - track_status->led_change_t;
		}
		track_status->continue_ledon_time +=
			curr_time - track_status->led_change_t;
		track_status->led_change_t = curr_time;
		track_status->led_change_rm = monitor->batt_rm;
	} else if (!track_status->led_on && (monitor->led_on || need_update)) {
		if (curr_time - track_status->led_change_t >
		    TRACK_CYCLE_RECORDIING_TIME_2MIN) {
			track_status->ledoff_rm +=
				monitor->batt_rm - track_status->led_change_rm;
			track_status->ledoff_time +=
				curr_time - track_status->led_change_t;
		}
		track_status->continue_ledoff_time +=
			curr_time - track_status->led_change_t;
		track_status->led_change_t = curr_time;
		track_status->led_change_rm = monitor->batt_rm;
	}
	track_status->led_on = monitor->led_on;
	chg_info("continue_ledoff_t:%d, continue_ledon_t:%d\n",
		 track_status->continue_ledoff_time,
		 track_status->continue_ledon_time);

	if (!charger_exist || track_status->chg_report_full_time ||
	    track_status->chg_fast_full_time ||
	    monitor->batt_soc >= TRACK_LED_MONITOR_SOC_POINT)
		oplus_chg_track_cal_ledon_ledoff_average_speed(track_status);

	chg_info("ch_t:%d, ch_rm:%d, ledoff_rm:%d, ledoff_t:%d, ledon_rm:%d, ledon_t:%d\n",
		track_status->led_change_t, track_status->led_change_rm,
		track_status->ledoff_rm, track_status->ledoff_time,
		track_status->ledon_rm, track_status->ledon_time);

	return 0;
}

static bool
oplus_chg_track_is_no_charging(struct oplus_chg_track_status *track_status)
{
	bool ret = false;
	char wired_adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
	char wls_adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];

	if (track_status == NULL)
		return ret;

	if ((track_status->chg_total_cnt <= 24) || (track_status->curr_uisoc == 100)) /* 5s*24=120s */
		return ret;

	if ((track_status->chg_no_charging_cnt * 100) /
		    track_status->chg_total_cnt >
	    TRACK_NO_CHRGING_TIME_PCT)
		ret = true;

	memcpy(wired_adapter_type,
	       track_status->power_info.wired_info.adapter_type,
	       OPLUS_CHG_TRACK_POWER_TYPE_LEN);
	memcpy(wls_adapter_type, track_status->power_info.wls_info.adapter_type,
	       OPLUS_CHG_TRACK_POWER_TYPE_LEN);
	chg_info("wired_adapter_type:%s, wls_adapter_type:%s\n",
		 wired_adapter_type, wls_adapter_type);
	if (!strcmp(wired_adapter_type, "unknow") ||
	    !strcmp(wired_adapter_type, "sdp") ||
	    !strcmp(wls_adapter_type, "unknow"))
		ret = false;

	if (track_status->debug_no_charging)
		ret = true;

	chg_info("chg_no_charging_cnt:%d, chg_total_cnt:%d, debug_no_charging:%d, ret:%d",
		track_status->chg_no_charging_cnt, track_status->chg_total_cnt,
		track_status->debug_no_charging, ret);

	return ret;
}

static void oplus_chg_track_record_break_charging_info(
	struct oplus_chg_track *track_chip, struct oplus_monitor *monitor,
	struct oplus_chg_track_power power_info, const char *sub_crux_info)
{
	int index = 0;
	struct oplus_chg_track_status *track_status;

	if (track_chip == NULL)
		return;

	chg_info("start, type=%d\n", power_info.power_type);
	track_status = &(track_chip->track_status);

	if (power_info.power_type == TRACK_CHG_TYPE_WIRE) {
		memset(track_chip->charging_break_trigger.crux_info, 0,
		       sizeof(track_chip->charging_break_trigger.crux_info));
		index += snprintf(
			&(track_chip->charging_break_trigger.crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$power_mode@@%s", power_info.power_mode);
		index += snprintf(
			&(track_chip->charging_break_trigger.crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			"$$adapter_t@@%s", power_info.wired_info.adapter_type);
		if (power_info.wired_info.adapter_id)
			index += snprintf(&(track_chip->charging_break_trigger
						    .crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$adapter_id@@0x%x",
					  power_info.wired_info.adapter_id);
		index += snprintf(
			&(track_chip->charging_break_trigger.crux_info[index]),
			OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$power@@%d",
			power_info.wired_info.power);

		if (track_status->wired_max_power <= 0)
			index += snprintf(&(track_chip->charging_break_trigger.crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d", -1);
		else
			index += snprintf(&(track_chip->charging_break_trigger.crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "$$match_power@@%d",
				(track_status->power_info.wired_info.power >= track_status->wired_max_power));

		if (strlen(track_status->fastchg_break_info.name)) {
			index += snprintf(&(track_chip->charging_break_trigger
						    .crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$voocphy_name@@%s",
					  track_chip->voocphy_name);
			index +=
				snprintf(&(track_chip->charging_break_trigger
						   .crux_info[index]),
					 OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					 "$$reason@@%s",
					 track_status->fastchg_break_info.name);
		}
		if (strlen(sub_crux_info)) {
			index += snprintf(&(track_chip->charging_break_trigger
						    .crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$crux_info@@%s", sub_crux_info);
		}
		oplus_chg_track_record_general_info(
			monitor, track_status,
			&(track_chip->charging_break_trigger), index);
		chg_info("wired[%s]\n",
			 track_chip->charging_break_trigger.crux_info);
	} else if (power_info.power_type == TRACK_CHG_TYPE_WIRELESS) {
		memset(track_chip->wls_charging_break_trigger.crux_info, 0,
		       sizeof(track_chip->wls_charging_break_trigger.crux_info));
		index += snprintf(&(track_chip->wls_charging_break_trigger
					    .crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$power_mode@@%s", power_info.power_mode);
		index += snprintf(&(track_chip->wls_charging_break_trigger
					    .crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$adapter_t@@%s",
				  power_info.wls_info.adapter_type);
		if (strlen(power_info.wls_info.dock_type))
			index += snprintf(
				&(track_chip->wls_charging_break_trigger
					  .crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"$$dock_type@@%s",
				power_info.wls_info.dock_type);
		index += snprintf(&(track_chip->wls_charging_break_trigger
					    .crux_info[index]),
				  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				  "$$power@@%d", power_info.wls_info.power);

		if (track_status->wls_max_power <= 0)
			index += snprintf(&(track_chip->wls_charging_break_trigger.crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$match_power@@%d", -1);
		else
			index += snprintf(&(track_chip->wls_charging_break_trigger.crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
					  "$$match_power@@%d",
					  (track_status->power_info.wls_info.power >= track_status->wls_max_power));
		if (strlen(sub_crux_info)) {
			index += snprintf(
				&(track_chip->wls_charging_break_trigger
					  .crux_info[index]),
				OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
				"$$crux_info@@%s", sub_crux_info);
		}
		oplus_chg_track_record_general_info(
			monitor, track_status,
			&(track_chip->wls_charging_break_trigger), index);
		chg_info("wls[%s]\n",
			 track_chip->wls_charging_break_trigger.crux_info);
	}
}

static int oplus_chg_track_get_fv_when_vooc(struct oplus_monitor *monitor)
{
	int flv;

	if (monitor->chg_ctrl_by_vooc && (monitor->ffc_status == FFC_DEFAULT)) {
		if (is_fv_votable_available(monitor))
			flv = get_effective_result(monitor->fv_votable);
		else
			flv = monitor->fv_mv;
	} else {
		flv = 0;
	}
	chg_info("flv=%d\n", flv);

	return flv;
}

int oplus_chg_track_set_fastchg_break_code(int fastchg_break_code)
{
	int flv;
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;
	struct oplus_monitor *monitor;
	unsigned vooc_sid;

	if (!g_track_chip)
		return -1;

	track_chip = g_track_chip;
	monitor = track_chip->monitor;
	track_status = &track_chip->track_status;
	track_status->fastchg_break_info.code = fastchg_break_code;
	if (track_chip->monitor->vooc_sid != 0)
		vooc_sid = track_chip->monitor->vooc_sid;
	else
		vooc_sid = track_chip->monitor->pre_vooc_sid;
	track_status->pre_fastchg_type = sid_to_adapter_chg_type(vooc_sid);
	flv = oplus_chg_track_get_fv_when_vooc(monitor);
	if ((track_status->pre_fastchg_type == CHARGER_TYPE_VOOC ||
	    track_status->pre_fastchg_type == CHARGER_TYPE_SVOOC) &&
	    flv && (flv - TRACK_CHG_VOOC_BATT_VOL_DIFF_MV < monitor->vbat_mv)) {
		if (g_track_chip->track_cfg.voocphy_type ==
		    TRACK_ADSP_VOOCPHY) {
			if (fastchg_break_code ==
			    TRACK_ADSP_VOOCPHY_COMMU_TIME_OUT)
				track_status->fastchg_break_info.code =
					TRACK_ADSP_VOOCPHY_BREAK_DEFAULT;
		} else if (g_track_chip->track_cfg.voocphy_type ==
				   TRACK_AP_SINGLE_CP_VOOCPHY ||
			   g_track_chip->track_cfg.voocphy_type ==
				   TRACK_AP_DUAL_CP_VOOCPHY) {
			if (fastchg_break_code ==
			    TRACK_CP_VOOCPHY_COMMU_TIME_OUT)
				track_status->fastchg_break_info.code =
					TRACK_CP_VOOCPHY_BREAK_DEFAULT;
		} else if (g_track_chip->track_cfg.voocphy_type == TRACK_MCU_VOOCPHY) {
			if (fastchg_break_code == TRACK_MCU_VOOCPHY_FAST_ABSENT)
				track_status->fastchg_break_info.code =
					TRACK_CP_VOOCPHY_BREAK_DEFAULT;
		}
	}

	return 0;
}

static int oplus_chg_track_match_mcu_voocphy_break_reason(
	struct oplus_chg_track_status *track_status)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mcu_voocphy_break_table); i++) {
		if (mcu_voocphy_break_table[i].code ==
		    track_status->fastchg_break_info.code) {
			strncpy(track_status->fastchg_break_info.name,
				mcu_voocphy_break_table[i].name,
				OPLUS_CHG_TRACK_FASTCHG_BREAK_REASON_LEN - 1);
			break;
		}
	}

	return 0;
}

static int oplus_chg_track_match_adsp_voocphy_break_reason(
	struct oplus_chg_track_status *track_status)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(adsp_voocphy_break_table); i++) {
		if (adsp_voocphy_break_table[i].code ==
		    track_status->fastchg_break_info.code) {
			strncpy(track_status->fastchg_break_info.name,
				adsp_voocphy_break_table[i].name,
				OPLUS_CHG_TRACK_FASTCHG_BREAK_REASON_LEN - 1);
			break;
		}
	}

	return 0;
}

static int oplus_chg_track_match_ap_voocphy_break_reason(
	struct oplus_chg_track_status *track_status)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ap_voocphy_break_table); i++) {
		if (ap_voocphy_break_table[i].code ==
		    track_status->fastchg_break_info.code) {
			strncpy(track_status->fastchg_break_info.name,
				ap_voocphy_break_table[i].name,
				OPLUS_CHG_TRACK_FASTCHG_BREAK_REASON_LEN - 1);
			break;
		}
	}

	return 0;
}

static int
oplus_chg_track_match_fastchg_break_reason(struct oplus_chg_track *track_chip)
{
	struct oplus_chg_track_status *track_status;

	if (!track_chip)
		return -1;
	track_status = &track_chip->track_status;

	chg_info("voocphy_type:%d, code:0x%x\n",
		 track_chip->track_cfg.voocphy_type,
		 track_status->fastchg_break_info.code);
	switch (track_chip->track_cfg.voocphy_type) {
	case TRACK_ADSP_VOOCPHY:
		oplus_chg_track_match_adsp_voocphy_break_reason(track_status);
		break;
	case TRACK_AP_SINGLE_CP_VOOCPHY:
	case TRACK_AP_DUAL_CP_VOOCPHY:
		oplus_chg_track_match_ap_voocphy_break_reason(track_status);
		break;
	case TRACK_MCU_VOOCPHY:
		oplus_chg_track_match_mcu_voocphy_break_reason(track_status);
		break;
	default:
		chg_info("!!!voocphy type is error, should not go here\n");
		break;
	}

	return 0;
}

static int oplus_chg_track_obtain_wls_break_sub_crux_info(
	struct oplus_chg_track *track_chip, char *crux_info)
{
	if (!track_chip || !track_chip->monitor || !track_chip->monitor->wls_topic || !crux_info)
		return -EINVAL;

	oplus_chg_wls_get_break_sub_crux_info(track_chip->monitor->wls_topic, crux_info);

	return 0;
}

static bool oplus_chg_track_wls_is_status_keep(struct oplus_chg_track *track_chip)
{
	union mms_msg_data data = { 0 };
	int rc;

	if (!track_chip || !track_chip->monitor || !track_chip->monitor->wls_topic)
		return false;
	rc = oplus_mms_get_item_data(track_chip->monitor->wls_topic, WLS_ITEM_STATUS_KEEP, &data, true);
	if (rc < 0)
		chg_err("can't get status_keep, rc=%d\n", rc);
	else
		return !!data.intval;

	return false;
}

int oplus_chg_track_check_wls_charging_break(int wls_connect)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;
	struct oplus_monitor *monitor;
	static struct oplus_chg_track_power power_info;
	static bool break_recording = 0;
	static bool pre_wls_connect = false;
	unsigned long long detal_time_ms;

	if (!g_track_chip)
		return -1;

	track_chip = g_track_chip;
	track_status = &track_chip->track_status;
	monitor = track_chip->monitor;

	if (pre_wls_connect == wls_connect)
		return 0;

	if (wls_connect) {
		track_status->wls_status_keep = oplus_chg_track_wls_is_status_keep(track_chip);
		track_status->wls_attach_time_ms = local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;
		detal_time_ms = track_status->wls_attach_time_ms - track_status->wls_detach_time_ms;
		if (detal_time_ms < track_chip->track_cfg.wls_chg_break_t_thd && track_status->wls_status_keep) {
			if (!break_recording) {
				break_recording = true;
				track_chip->wls_charging_break_trigger.flag_reason =
					TRACK_NOTIFY_FLAG_WLS_CHARGING_BREAK;
				oplus_chg_track_record_break_charging_info(track_chip, monitor,
					power_info, track_status->wls_break_crux_info);
				schedule_delayed_work(&track_chip->wls_charging_break_trigger_work, 0);
			}
			if (!track_status->wired_need_upload) {
				cancel_delayed_work_sync(&track_chip->charger_info_trigger_work);
				cancel_delayed_work_sync(&track_chip->no_charging_trigger_work);
				cancel_delayed_work_sync(&track_chip->slow_charging_trigger_work);
			}
		} else {
			/*record one time in 6s*/
			break_recording = 0;
		}
		chg_info("pre_wls_connect[%d], wls_connect[%d], break_recording[%d], status_keep[%d], "
			"detal_t:%llu, wls_attach_time:%llu\n",
			pre_wls_connect, wls_connect, break_recording, track_status->wls_status_keep,
			detal_time_ms, track_status->wls_attach_time_ms);
	} else {
		track_status->wls_detach_time_ms = local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;
		oplus_chg_track_handle_wls_type_info(track_status);
		oplus_chg_track_obtain_wls_break_sub_crux_info(track_chip, track_status->wls_break_crux_info);
		power_info = track_status->power_info;
		/* oplus_chg_wake_update_work(); TODO:need check */
		chg_info("wls_detach_time = %llu\n", track_status->wls_detach_time_ms);
	}
	pre_wls_connect = wls_connect;

	return 0;
}

static bool
oplus_chg_track_wired_fastchg_exit_code(struct oplus_chg_track *track_chip)
{
	bool ret = true;
	int code;
	struct oplus_chg_track_status *track_status;

	if (!track_chip)
		return -1;
	track_status = &track_chip->track_status;
	code = track_status->fastchg_break_info.code;

	chg_info("voocphy_type:%d, code:0x%x\n",
		 track_chip->track_cfg.voocphy_type,
		 track_status->fastchg_break_info.code);
	switch (track_chip->track_cfg.voocphy_type) {
	case TRACK_ADSP_VOOCPHY:
		if (!code || code == TRACK_ADSP_VOOCPHY_FULL ||
		    code == TRACK_ADSP_VOOCPHY_BATT_TEMP_OVER ||
		    code == TRACK_ADSP_VOOCPHY_SWITCH_TEMP_RANGE)
			ret = true;
		else
			ret = false;
		break;
	case TRACK_AP_SINGLE_CP_VOOCPHY:
	case TRACK_AP_DUAL_CP_VOOCPHY:
		if (!code || code == TRACK_CP_VOOCPHY_FULL ||
		    code == TRACK_CP_VOOCPHY_BATT_TEMP_OVER ||
		    code == TRACK_CP_VOOCPHY_USER_EXIT_FASTCHG ||
		    code == TRACK_CP_VOOCPHY_SWITCH_TEMP_RANGE)
			ret = true;
		else
			ret = false;
		break;
	case TRACK_MCU_VOOCPHY:
		if (!code || code == TRACK_MCU_VOOCPHY_TEMP_OVER ||
		    code == TRACK_MCU_VOOCPHY_NORMAL_TEMP_FULL ||
		    code == TRACK_MCU_VOOCPHY_LOW_TEMP_FULL ||
		    code == TRACK_MCU_VOOCPHY_BAT_TEMP_EXIT)
			ret = true;
		else
			ret = false;
		break;
	default:
		chg_info("!!!voocphy type is error, should not go here\n");
		break;
	}

	return ret;
}

static bool oplus_chg_track_get_mmi_chg(void)
{
	struct votable *common_chg_suspend_votable;

	common_chg_suspend_votable = find_votable("CHG_DISABLE");
	if (!common_chg_suspend_votable)
		return true;

	return !get_client_vote(common_chg_suspend_votable, MMI_CHG_VOTER);
}

int oplus_chg_track_check_wired_charging_break(int vbus_rising)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;
	static struct oplus_chg_track_power power_info;
	struct oplus_monitor *monitor;
	static bool break_recording = 0;
	static bool pre_vbus_rising = false;
	bool fastchg_code_ok;

	if (!g_track_chip)
		return -1;

	track_chip = g_track_chip;
	monitor = track_chip->monitor;
	track_status = &track_chip->track_status;

	chg_info("pre_vbus_rising[%d], vbus_rising[%d], break_recording[%d]\n",
		 pre_vbus_rising, vbus_rising, break_recording);

	if (vbus_rising && (pre_vbus_rising != vbus_rising)) {
		track_status->chg_attach_time_ms =
			local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;
		if (track_status->debug_break_code)
			track_status->fastchg_break_info.code = track_status->debug_break_code;
		fastchg_code_ok =
			oplus_chg_track_wired_fastchg_exit_code(track_chip);
		chg_info(
			"detal_t:%llu, chg_attach_time = %llu, fastchg_break_code=0x%x\n",
			track_status->chg_attach_time_ms -
				track_status->chg_detach_time_ms,
			track_status->chg_attach_time_ms,
			track_status->fastchg_break_info.code);
		if ((track_status->chg_attach_time_ms -
			     track_status->chg_detach_time_ms <
		     track_chip->track_cfg.fast_chg_break_t_thd) &&
		    !fastchg_code_ok && track_status->mmi_chg) {
			if (!break_recording) {
				break_recording = true;
				track_chip->charging_break_trigger.flag_reason =
					TRACK_NOTIFY_FLAG_FAST_CHARGING_BREAK;
				oplus_chg_track_match_fastchg_break_reason(
					track_chip);
				oplus_chg_track_record_break_charging_info(
					track_chip, monitor, power_info,
					track_status->wired_break_crux_info);
				memset(&(track_status->fastchg_break_info), 0,
				       sizeof(track_status->fastchg_break_info));
				schedule_delayed_work(
					&track_chip->charging_break_trigger_work,
					0);
			}
			if (!track_status->wls_need_upload) {
				cancel_delayed_work_sync(
					&track_chip->charger_info_trigger_work);
				cancel_delayed_work_sync(
					&track_chip->no_charging_trigger_work);
				cancel_delayed_work_sync(
					&track_chip->slow_charging_trigger_work);
			}
		} else if ((track_status->chg_attach_time_ms -
				    track_status->chg_detach_time_ms <
			    track_chip->track_cfg.general_chg_break_t_thd) &&
			   !track_status->fastchg_break_info.code &&
			   track_status->mmi_chg) {
			if (!break_recording) {
				break_recording = true;
				track_chip->charging_break_trigger.flag_reason =
					TRACK_NOTIFY_FLAG_GENERAL_CHARGING_BREAK;
				oplus_chg_track_record_break_charging_info(
					track_chip, monitor, power_info,
					track_status->wired_break_crux_info);
				schedule_delayed_work(
					&track_chip->charging_break_trigger_work,
					0);
			}
			if (!track_status->wls_need_upload) {
				cancel_delayed_work_sync(
					&track_chip->charger_info_trigger_work);
				cancel_delayed_work_sync(
					&track_chip->no_charging_trigger_work);
				cancel_delayed_work_sync(
					&track_chip->slow_charging_trigger_work);
			}
		} else {
			break_recording = 0;
		}
		memset(&(track_status->power_info), 0, sizeof(track_status->power_info));
		strcpy(track_status->power_info.power_mode, "unknow");
	} else if (!vbus_rising && (pre_vbus_rising != vbus_rising)) {
		track_status->chg_detach_time_ms =
			local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;
		oplus_chg_track_handle_wired_type_info(
			monitor, TRACK_CHG_GET_LAST_TIME_TYPE);
		power_info = track_status->power_info;
		track_status->mmi_chg = oplus_chg_track_get_mmi_chg();
		chg_info("chg_detach_time = %llu, mmi_chg=%d\n",
			 track_status->chg_detach_time_ms,
			 track_status->mmi_chg);
	}

	if (vbus_rising) {
		track_status->mmi_chg = oplus_chg_track_get_mmi_chg();
		track_chip->monitor->pre_vooc_sid = 0;
		oplus_chg_track_set_fastchg_break_code(
			TRACK_VOOCPHY_BREAK_DEFAULT);
	}
	pre_vbus_rising = vbus_rising;

	return 0;
}

#define TIMER_SIZE 5
int oplus_chg_water_inlet_detect(int reason,int plugin_count)
{
	static unsigned long long timer[TIMER_SIZE][TRACK_DATA_PLUGOUT_MAX] = {0};
	static unsigned long long pre_timer = 0;
	unsigned long long plugin_timer_sum = 0;
	int i = 0;
	struct oplus_chg_track *chip;

	chg_info("in oplus_chg_water_inlet_detect, reason = %d, plugin_count = %d\n", reason, plugin_count);

	if(!g_track_chip) {
		chg_info("!g_track_chip\n");
		return -1;
	}

	chip = g_track_chip;

	switch (reason) {
	case TRACK_CMD_LIQUID_INTAKE:
		if (!chip->oplus_liquid_intake_enable) {
			chg_info("water inlet buried point bypass.\n");
			break;
		}
		/* Check whether this is the first entry */
		if (pre_timer == 0) {
			pre_timer = local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;
			break;
		}

		/* Make sure you can make a difference */
		if (plugin_count > 0) {
			timer[(plugin_count - 1) % TIMER_SIZE][TRACK_DATA_PLUGOUT_INTERVAL] =
				local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD - pre_timer;
			timer[(plugin_count - 1) % TIMER_SIZE][TRACK_DATA_PLUGOUT_WIRE_CONNECT_STATE] =
				oplus_wired_get_hw_detect();

			chg_info("timer:%llu, pre_timer:%llu, timer[%d][0]:%llu, plugin_count:%d, CID:%d\n",
				local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD, pre_timer, (plugin_count - 1) % TIMER_SIZE,
				timer[(plugin_count - 1) % TIMER_SIZE][TRACK_DATA_PLUGOUT_INTERVAL], plugin_count,
				oplus_wired_get_hw_detect());
		}
		/* Store the time of the current removal */
		pre_timer = local_clock() / TRACK_LOCAL_T_NS_TO_MS_THD;

		/* Time of calculation */
		for (i = 0; i < TIMER_SIZE; i++) {
			if(timer[i][TRACK_DATA_PLUGOUT_INTERVAL] == 0)
				goto end;
			plugin_timer_sum += timer[i][TRACK_DATA_PLUGOUT_INTERVAL];
		}
		/* Trigger burial point */
		if( plugin_timer_sum <= 5000 ) {
			chg_info("plugin_timer_sum:%llu, timer0:%llu, timer1:%llu, timer2:%llu, timer3:%llu, timer4:%llu, \n",
				plugin_timer_sum, timer[0][0], timer[1][0], timer[2][0], timer[3][0], timer[4][0]);
			snprintf(chip->chg_into_liquid_load_trigger.crux_info,OPLUS_CHG_TRACK_CURX_INFO_LEN,
				"plugin_timer_sum:%llu, timer0:%llu,%llu timer1:%llu,%llu timer2:%llu,%llu timer3:%llu,%llu timer4:%llu,%llu",
				plugin_timer_sum, timer[0][0], timer[0][1], timer[1][0], timer[1][1], timer[2][0], timer[2][1],
				timer[3][0], timer[3][1], timer[4][0], timer[4][1]);

			schedule_delayed_work(&chip->chg_into_liquid_trigger_work, 0);
			schedule_delayed_work(&chip->chg_into_liquid_trigger_work_timeout,
				msecs_to_jiffies(chip->track_status.debug_water_inlet_para));
			pre_timer = 0;
			memset(timer, '\0', sizeof(timer));
		}
end:
		break;
	case TRACK_CMD_CLEAR_TIMER:
		pre_timer = 0;
		memset(timer, '\0', sizeof(timer));
		break;
	default:
		chg_info("!!!err cmd\n");
		break;
	}

	return 0;
}

int oplus_chg_track_cal_tbatt_status(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	if (monitor->batt_status != POWER_SUPPLY_STATUS_CHARGING)
	{
		chg_debug("not charging, should return\n");
		return 0;
	}

	if (!track_status->tbatt_warm_once &&
	    ((monitor->temp_region == TEMP_REGION_WARM) ||
	     (monitor->temp_region == TEMP_REGION_HOT)))
		track_status->tbatt_warm_once = true;

	if (!track_status->tbatt_cold_once &&
	    (monitor->temp_region == TEMP_REGION_COLD))
		track_status->tbatt_cold_once = true;

	chg_info("tbatt_warm_once:%d, tbatt_cold_once:%d\n",
		 track_status->tbatt_warm_once, track_status->tbatt_cold_once);

	return 0;
}

int oplus_chg_track_aging_ffc_check(struct oplus_monitor *monitor, int step)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;
	int rc;
	int cutoff_fv;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	if (step >= FFC_CHG_STEP_MAX) {
		chg_err("step(=%d) is too big\n", step);
		return -EINVAL;
	} else if (step < 0) {
		chg_err("step(=%d) is too small\n", step);
		return -EINVAL;
	}

	if (monitor->wired_online) {
		if (step == 0)
			track_status->aging_ffc_start_time = oplus_chg_track_get_local_time_s();
		rc = oplus_comm_get_wired_aging_ffc_offset(monitor->comm_topic, step);
		if (rc <= 0)
			return rc;
		track_status->aging_ffc_trig = true;
		cutoff_fv = oplus_comm_get_current_wired_ffc_cutoff_fv(monitor->comm_topic, step);
		if (cutoff_fv < 0)
			return cutoff_fv;
		track_status->aging_ffc_judge_vol[step] = cutoff_fv + rc;
	}

	return 0;
}

int oplus_chg_track_record_ffc_end_time(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	if (monitor->wired_online) {
		track_status->ffc_end_time = oplus_chg_track_get_local_time_s();
	}

	return 0;
}

int oplus_chg_track_record_ffc_soc(struct oplus_monitor *monitor,
	int main_soc, int sub_soc, bool start)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	if (start) {
		track_status->ffc_start_main_soc = main_soc;
		track_status->ffc_start_sub_soc = sub_soc;
	} else {
		track_status->ffc_end_main_soc = main_soc;
		track_status->ffc_end_sub_soc = sub_soc;
	}

	return 0;
}

int oplus_chg_track_record_dual_chan_start(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	track_status->dual_chan_open_count++;
	track_status->dual_chan_start_time = oplus_chg_track_get_local_time_s();

	return 0;
}

int oplus_chg_track_record_dual_chan_end(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_chip;
	struct oplus_chg_track_status *track_status;
	int dual_chan_end_time;

	if (!monitor)
		return -EINVAL;
	track_chip = monitor->track;
	if (!track_chip)
		return -EINVAL;
	track_status = &track_chip->track_status;

	dual_chan_end_time = oplus_chg_track_get_local_time_s();
	if (track_status->dual_chan_start_time) {
		track_status->dual_chan_time +=
			dual_chan_end_time - track_status->dual_chan_start_time;
		track_status->dual_chan_start_time = 0;
		chg_info("record_dual_chan_end: start_time:%d end_time:%d time:%d\n",
			 track_status->dual_chan_start_time, dual_chan_end_time, track_status->dual_chan_time);
	}

	return 0;
}

static int oplus_chg_track_cal_section_soc_inc_rm(
	struct oplus_monitor *monitor,
	struct oplus_chg_track_status *track_status)
{
	static int time_go_next_status;
	static int rm_go_next_status;
	int curr_time;
	static int pre_soc_low_sect_incr_rm;
	static int pre_soc_low_sect_cont_time;
	static int pre_soc_medium_sect_incr_rm;
	static int pre_soc_medium_sect_cont_time;
	static int pre_soc_high_sect_incr_rm;
	static int pre_soc_high_sect_cont_time;

	if (!monitor || !track_status)
		return -1;

	if (track_status->soc_sect_status == TRACK_SOC_SECTION_DEFAULT) {
		track_status->soc_low_sect_incr_rm = 0;
		track_status->soc_low_sect_cont_time = 0;
		track_status->soc_medium_sect_incr_rm = 0;
		track_status->soc_medium_sect_cont_time = 0;
		track_status->soc_high_sect_incr_rm = 0;
		track_status->soc_high_sect_cont_time = 0;
		pre_soc_low_sect_incr_rm = 0;
		pre_soc_low_sect_cont_time = 0;
		pre_soc_medium_sect_incr_rm = 0;
		pre_soc_medium_sect_cont_time = 0;
		pre_soc_high_sect_incr_rm = 0;
		pre_soc_high_sect_cont_time = 0;
		if (monitor->batt_soc <= TRACK_REF_SOC_50)
			track_status->soc_sect_status = TRACK_SOC_SECTION_LOW;
		else if (monitor->batt_soc <= TRACK_REF_SOC_75)
			track_status->soc_sect_status =
				TRACK_SOC_SECTION_MEDIUM;
		else if (monitor->batt_soc <= TRACK_REF_SOC_90)
			track_status->soc_sect_status = TRACK_SOC_SECTION_HIGH;
		else
			track_status->soc_sect_status = TRACK_SOC_SECTION_OVER;
		time_go_next_status = oplus_chg_track_get_local_time_s();
		rm_go_next_status = monitor->batt_rm;
	}

	if (monitor->batt_status != POWER_SUPPLY_STATUS_CHARGING) {
		if (monitor->batt_soc <= TRACK_REF_SOC_50)
			track_status->soc_sect_status = TRACK_SOC_SECTION_LOW;
		else if (monitor->batt_soc <= TRACK_REF_SOC_75)
			track_status->soc_sect_status =
				TRACK_SOC_SECTION_MEDIUM;
		else if (monitor->batt_soc <= TRACK_REF_SOC_90)
			track_status->soc_sect_status = TRACK_SOC_SECTION_HIGH;
		else
			track_status->soc_sect_status = TRACK_SOC_SECTION_OVER;
		pre_soc_low_sect_cont_time =
			track_status->soc_low_sect_cont_time;
		pre_soc_low_sect_incr_rm = track_status->soc_low_sect_incr_rm;
		pre_soc_medium_sect_cont_time =
			track_status->soc_medium_sect_cont_time;
		pre_soc_medium_sect_incr_rm =
			track_status->soc_medium_sect_incr_rm;
		pre_soc_high_sect_cont_time =
			track_status->soc_high_sect_cont_time;
		pre_soc_high_sect_incr_rm = track_status->soc_high_sect_incr_rm;
		time_go_next_status = oplus_chg_track_get_local_time_s();
		rm_go_next_status = monitor->batt_rm;
		return 0;
	}

	switch (track_status->soc_sect_status) {
	case TRACK_SOC_SECTION_LOW:
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->soc_low_sect_cont_time =
			(curr_time - time_go_next_status) +
			pre_soc_low_sect_cont_time;
		track_status->soc_low_sect_incr_rm =
			(monitor->batt_rm - rm_go_next_status) +
			pre_soc_low_sect_incr_rm;
		if (monitor->batt_soc > TRACK_REF_SOC_50) {
			pre_soc_low_sect_cont_time =
				track_status->soc_low_sect_cont_time;
			pre_soc_low_sect_incr_rm =
				track_status->soc_low_sect_incr_rm;
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status =
				TRACK_SOC_SECTION_MEDIUM;
		}
		break;
	case TRACK_SOC_SECTION_MEDIUM:
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->soc_medium_sect_cont_time =
			(curr_time - time_go_next_status) +
			pre_soc_medium_sect_cont_time;
		track_status->soc_medium_sect_incr_rm =
			(monitor->batt_rm - rm_go_next_status) +
			pre_soc_medium_sect_incr_rm;
		if (monitor->batt_soc <= TRACK_REF_SOC_50) {
			pre_soc_medium_sect_cont_time =
				track_status->soc_medium_sect_cont_time;
			pre_soc_medium_sect_incr_rm =
				track_status->soc_medium_sect_incr_rm;
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status = TRACK_SOC_SECTION_LOW;
		} else if (monitor->batt_soc > TRACK_REF_SOC_75) {
			pre_soc_medium_sect_cont_time =
				track_status->soc_medium_sect_cont_time;
			pre_soc_medium_sect_incr_rm =
				track_status->soc_medium_sect_incr_rm;
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status = TRACK_SOC_SECTION_HIGH;
		}
		break;
	case TRACK_SOC_SECTION_HIGH:
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->soc_high_sect_cont_time =
			(curr_time - time_go_next_status) +
			pre_soc_high_sect_cont_time;
		track_status->soc_high_sect_incr_rm =
			(monitor->batt_rm - rm_go_next_status) +
			pre_soc_high_sect_incr_rm;
		if (monitor->batt_soc <= TRACK_REF_SOC_75) {
			pre_soc_high_sect_cont_time =
				track_status->soc_high_sect_cont_time;
			pre_soc_high_sect_incr_rm =
				track_status->soc_high_sect_incr_rm;
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status =
				TRACK_SOC_SECTION_MEDIUM;
		} else if (monitor->batt_soc > TRACK_REF_SOC_90) {
			pre_soc_high_sect_cont_time =
				track_status->soc_high_sect_cont_time;
			pre_soc_high_sect_incr_rm =
				track_status->soc_high_sect_incr_rm;
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status = TRACK_SOC_SECTION_OVER;
		}
		break;
	case TRACK_SOC_SECTION_OVER:
		curr_time = oplus_chg_track_get_local_time_s();
		if (monitor->batt_soc < TRACK_REF_SOC_90) {
			time_go_next_status = curr_time;
			rm_go_next_status = monitor->batt_rm;
			track_status->soc_sect_status = TRACK_SOC_SECTION_HIGH;
		}
		break;
	default:
		chg_err("!!!soc section status is invalid\n");
		break;
	}

	chg_info(
		"soc_sect_status:%d, time_go_next_status:%d, rm_go_next_status:%d\n",
		track_status->soc_sect_status, time_go_next_status,
		rm_go_next_status);

	chg_info("soc_low_sect_cont_time:%d, soc_low_sect_incr_rm:%d, "
		 "soc_medium_sect_cont_time:%d, soc_medium_sect_incr_rm:%d "
		 "soc_high_sect_cont_time:%d, soc_high_sect_incr_rm:%d\n",
		 track_status->soc_low_sect_cont_time,
		 track_status->soc_low_sect_incr_rm,
		 track_status->soc_medium_sect_cont_time,
		 track_status->soc_medium_sect_incr_rm,
		 track_status->soc_high_sect_cont_time,
		 track_status->soc_high_sect_incr_rm);
	return 0;
}

static bool oplus_chg_track_burst_soc_sect_speed(
	struct oplus_chg_track_status *track_status,
	struct oplus_chg_track_speed_ref *speed_ref)
{
	bool ret = false;
	int soc_low_sect_incr_rm;
	int soc_medium_sect_incr_rm;
	int soc_high_sect_incr_rm;
	int batt_num;

	if (!track_status || !speed_ref)
		return false;

	if (!track_status->soc_high_sect_cont_time &&
	    !track_status->soc_medium_sect_cont_time &&
	    !track_status->soc_low_sect_cont_time)
		return true;

	chg_info("low_ref_curr:%d, medium_ref_curr:%d, high_ref_curr:%d\n",
		 speed_ref[TRACK_SOC_SECTION_LOW - 1].ref_curr,
		 speed_ref[TRACK_SOC_SECTION_MEDIUM - 1].ref_curr,
		 speed_ref[TRACK_SOC_SECTION_HIGH - 1].ref_curr);

	soc_low_sect_incr_rm = track_status->soc_low_sect_incr_rm;
	soc_medium_sect_incr_rm = track_status->soc_medium_sect_incr_rm;
	soc_high_sect_incr_rm = track_status->soc_high_sect_incr_rm;

	batt_num = oplus_gauge_get_batt_num();
	soc_low_sect_incr_rm *= (TRACK_TIME_1HOU_THD / batt_num);
	soc_medium_sect_incr_rm *= (TRACK_TIME_1HOU_THD / batt_num);
	soc_high_sect_incr_rm *= (TRACK_TIME_1HOU_THD / batt_num);

	if ((track_status->soc_low_sect_cont_time > TRACK_REF_TIME_6S) &&
	    ((soc_low_sect_incr_rm / track_status->soc_low_sect_cont_time) <
	     speed_ref[TRACK_SOC_SECTION_LOW - 1].ref_curr)) {
		chg_info("slow charging when soc low section\n");
		ret = true;
	}

	if (!ret &&
	    (track_status->soc_medium_sect_cont_time > TRACK_REF_TIME_8S) &&
	    ((soc_medium_sect_incr_rm /
	      track_status->soc_medium_sect_cont_time) <
	     speed_ref[TRACK_SOC_SECTION_MEDIUM - 1].ref_curr)) {
		chg_info("slow charging when soc medium section\n");
		ret = true;
	}

	if (!ret &&
	    (track_status->soc_high_sect_cont_time > TRACK_REF_TIME_10S) &&
	    ((soc_high_sect_incr_rm / track_status->soc_high_sect_cont_time) <
	     speed_ref[TRACK_SOC_SECTION_HIGH - 1].ref_curr)) {
		chg_info("slow charging when soc high section\n");
		ret = true;
	}

	return ret;
}

static int oplus_chg_track_get_speed_slow_reason(
	struct oplus_chg_track_status *track_status)
{
	struct oplus_chg_track *chip = g_track_chip;
	char wired_adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];
	char wls_adapter_type[OPLUS_CHG_TRACK_POWER_TYPE_LEN];

	if (!track_status || !chip)
		return -1;

	memcpy(wired_adapter_type,
	       track_status->power_info.wired_info.adapter_type,
	       OPLUS_CHG_TRACK_POWER_TYPE_LEN);
	memcpy(wls_adapter_type, track_status->power_info.wls_info.adapter_type,
	       OPLUS_CHG_TRACK_POWER_TYPE_LEN);
	chg_info("wired_adapter_type:%s, wls_adapter_type:%s\n",
		 wired_adapter_type, wls_adapter_type);

	if (track_status->tbatt_warm_once)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_WARM;
	else if (track_status->tbatt_cold_once)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_COLD;
	else if ((track_status->cool_down_effect_cnt * 100 /
		  track_status->chg_total_cnt) >
		 TRACK_COOLDOWN_CHRGING_TIME_PCT)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_COOLDOWN;
	else if (track_status->chg_start_soc >= TRACK_REF_SOC_90)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_BATT_CAP_HIGH;
	else if ((track_status->power_info.power_type ==
		  TRACK_CHG_TYPE_UNKNOW) ||
		 (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRE &&
		  chip->track_cfg.wired_max_power >
			  track_status->power_info.wired_info.power))
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_NON_STANDARD_PA;
	else if ((track_status->power_info.power_type ==
		  TRACK_CHG_TYPE_WIRELESS) &&
		 (chip->track_cfg.wls_max_power - TRACK_POWER_10000MW >
		  track_status->power_info.wls_info.power) &&
		 (strcmp(wls_adapter_type, "bpp")) &&
		 (strcmp(wls_adapter_type, "epp")))
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_NON_STANDARD_PA;
	else if ((track_status->wls_skew_effect_cnt * 100 /
		  track_status->chg_total_cnt) >
			 TRACK_WLS_SKEWING_CHRGING_TIME_PCT &&
		 track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_WLS_SKEW;
	else if (!track_status->chg_verity &&
		 track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS)
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_VERITY_FAIL;
	else
		chip->slow_charging_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CHG_SLOW_OTHER;

	if (track_status->debug_slow_charging_reason)
		chip->slow_charging_trigger.flag_reason =
			track_status->debug_slow_charging_reason +
			TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_WARM - 1;

	chg_info("flag_reason:%d\n", chip->slow_charging_trigger.flag_reason);

	return 0;
}

static bool oplus_chg_track_judge_speed_slow(struct oplus_monitor *monitor,
					     struct oplus_chg_track *track_chip)
{
	struct oplus_chg_track_status *track_status;

	if (!track_chip || !monitor)
		return 0;

	track_status = &(track_chip->track_status);
	mutex_lock(&track_chip->access_lock);
	if (track_status->has_judge_speed) {
		mutex_unlock(&track_chip->access_lock);
		return track_status->chg_speed_is_slow;
	}
	track_status->has_judge_speed = true;
	mutex_unlock(&track_chip->access_lock);

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_UNKNOW) {
		track_status->chg_speed_is_slow = true;
	} else if (track_status->power_info.power_type ==
		   TRACK_CHG_TYPE_WIRELESS) {
		if (!strcmp(track_status->power_info.wls_info.adapter_type,
			    "unknow")) {
			track_status->chg_speed_is_slow = true;
		} else if (!strcmp(track_status->power_info.wls_info.adapter_type,"epp")) {
			if (!track_status->wls_epp_speed_ref)
				return false;
			track_status->chg_speed_is_slow =
				oplus_chg_track_burst_soc_sect_speed(
					track_status,
					track_status->wls_epp_speed_ref);
		} else if (!strcmp(track_status->power_info.wls_info.adapter_type,"bpp")) {
			if (!track_status->wls_bpp_speed_ref)
				return false;
			track_status->chg_speed_is_slow =
				oplus_chg_track_burst_soc_sect_speed(
					track_status,
					track_status->wls_bpp_speed_ref);
		} else {
			if (!track_status->wls_airvooc_speed_ref)
				return false;
			track_status->chg_speed_is_slow =
				oplus_chg_track_burst_soc_sect_speed(
					track_status,
					track_status->wls_airvooc_speed_ref);
		}
	} else {
		if (!track_status->wired_speed_ref)
			return false;
		track_status->chg_speed_is_slow =
			oplus_chg_track_burst_soc_sect_speed(
				track_status, track_status->wired_speed_ref);
	}

	if (track_status->chg_speed_is_slow ||
	    track_status->debug_slow_charging_force) {
		oplus_chg_track_get_speed_slow_reason(track_status);
	}

	return (track_status->chg_speed_is_slow ||
		track_status->debug_slow_charging_force);
}

static int
oplus_chg_track_cal_batt_full_time(struct oplus_monitor *monitor,
				   struct oplus_chg_track_status *track_status)
{
	int curr_time;

	if (!monitor || !track_status || !g_track_chip)
		return -1;

	if (!track_status->chg_fast_full_time &&
	    ((monitor->ffc_status != FFC_DEFAULT) ||
	     track_status->wls_prop_status == TRACK_WLS_FASTCHG_FULL ||
	     track_status->debug_fast_prop_status ==
		     TRACK_FASTCHG_STATUS_NORMAL)) {
		track_status->chg_report_full_time = 0;
		track_status->chg_normal_full_time = 0;
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->chg_fast_full_time =
			curr_time - track_status->chg_start_time;
		track_status->chg_fast_full_time /= TRACK_TIME_1MIN_THD;
		if (!track_status->chg_fast_full_time)
			track_status->chg_fast_full_time =
				TRACK_TIME_1MIN_THD / TRACK_TIME_1MIN_THD;

		if (track_status->chg_average_speed ==
		    TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT)
			track_status->chg_average_speed =
				TRACK_TIME_1MIN_THD *
				(monitor->batt_rm -
				 track_status->chg_start_rm) /
				(curr_time - track_status->chg_start_time);
		chg_info("curr_time:%d, start_time:%d, fast_full_time:%d"
			 "curr_rm:%d, chg_start_rm:%d, chg_average_speed:%d\n",
			 curr_time, track_status->chg_start_time,
			 track_status->chg_fast_full_time, monitor->batt_rm,
			 track_status->chg_start_rm,
			 track_status->chg_average_speed);
	}

	if (!track_status->chg_report_full_time &&
	    (monitor->batt_status == POWER_SUPPLY_STATUS_FULL ||
	     track_status->debug_normal_prop_status ==
		     POWER_SUPPLY_STATUS_FULL)) {
		oplus_chg_track_handle_batt_full_reason(monitor, track_status);
		oplus_chg_track_judge_speed_slow(monitor, g_track_chip);
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->chg_report_full_time =
			curr_time - track_status->chg_start_time;
		track_status->chg_report_full_time /= TRACK_TIME_1MIN_THD;
		if (!track_status->chg_report_full_time)
			track_status->chg_report_full_time =
				TRACK_TIME_1MIN_THD / TRACK_TIME_1MIN_THD;
	}

	if (!track_status->chg_normal_full_time &&
	    (monitor->batt_status == POWER_SUPPLY_STATUS_FULL ||
	     track_status->debug_normal_charging_state ==
		     POWER_SUPPLY_STATUS_FULL)) {
		curr_time = oplus_chg_track_get_local_time_s();
		if (track_status->aging_ffc_start_time)
			track_status->aging_ffc_to_full_time = curr_time - track_status->aging_ffc_start_time;
		if (track_status->ffc_end_time) {
			track_status->ffc_time =
				track_status->ffc_end_time - track_status->aging_ffc_start_time;
			track_status->cv_time = curr_time - track_status->ffc_end_time;
		}
		track_status->chg_normal_full_time =
			curr_time - track_status->chg_start_time;
		track_status->chg_normal_full_time /= TRACK_TIME_1MIN_THD;
		if (!track_status->chg_normal_full_time)
			track_status->chg_normal_full_time =
				TRACK_TIME_1MIN_THD / TRACK_TIME_1MIN_THD;
		if (track_status->chg_average_speed ==
		    TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT)
			track_status->chg_average_speed =
				TRACK_TIME_1MIN_THD *
				(monitor->batt_rm -
				 track_status->chg_start_rm) /
				(curr_time - track_status->chg_start_time);
		chg_info("curr_time:%d, start_time:%d, normal_full_time:%d"
			 "curr_rm:%d, chg_start_rm:%d, chg_average_speed:%d\n",
			 curr_time, track_status->chg_start_time,
			 track_status->chg_normal_full_time, monitor->batt_rm,
			 track_status->chg_start_rm,
			 track_status->chg_average_speed);
	}

	return 0;
}

static int
oplus_chg_track_get_charger_type(struct oplus_monitor *monitor,
				 struct oplus_chg_track_status *track_status,
				 int type)
{
	if (monitor == NULL || track_status == NULL)
		return -EINVAL;

	if (monitor->wls_online)
		oplus_chg_track_handle_wls_type_info(track_status);
	else if (monitor->wired_online)
		oplus_chg_track_handle_wired_type_info(monitor, type);

	return 0;
}

static int oplus_chg_track_obtain_power_info(char *power_info, int len)
{
	int index = 0;
	struct oplus_chg_track_status *track_status = temp_track_status;
	struct oplus_monitor *monitor;

	if (!power_info || !g_track_chip || !track_status)
		return -1;
	mutex_lock(&temp_track_status->track_status_lock);
	memset(track_status, 0, sizeof(struct oplus_chg_track_status));
	monitor = g_track_chip->monitor;
	oplus_chg_track_get_charger_type(monitor, track_status,
					 TRACK_CHG_GET_THTS_TIME_TYPE);

	index += snprintf(&(power_info[index]), len - index, "$$power_mode@@%s",
			  track_status->power_info.power_mode);

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRE) {
		index += snprintf(
			&(power_info[index]), len - index, "$$adapter_t@@%s",
			track_status->power_info.wired_info.adapter_type);
		if (track_status->power_info.wired_info.adapter_id)
			index += snprintf(
				&(power_info[index]), len - index,
				"$$adapter_id@@0x%x",
				track_status->power_info.wired_info.adapter_id);
		index += snprintf(&(power_info[index]), len - index,
				  "$$power@@%d",
				  track_status->power_info.wired_info.power);
		if (g_track_chip->track_status.wired_max_power <= 0)
			index += snprintf(&(power_info[index]), len - index, "$$match_power@@%d", -1);
		else
			index += snprintf(&(power_info[index]), len - index, "$$match_power@@%d",
					  (track_status->power_info.wired_info.power >= g_track_chip->track_status.wired_max_power));
	} else if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS) {
		index += snprintf(
			&(power_info[index]), len - index, "$$adapter_t@@%s",
			track_status->power_info.wls_info.adapter_type);
		if (strlen(track_status->power_info.wls_info.dock_type))
			index += snprintf(
				&(power_info[index]), len - index,
				"$$dock_type@@%s",
				track_status->power_info.wls_info.dock_type);
		index += snprintf(&(power_info[index]), len - index,
				  "$$power@@%d",
				  track_status->power_info.wls_info.power);
		if (g_track_chip->track_status.wls_max_power <= 0)
			index += snprintf(&(power_info[index]), len - index, "$$match_power@@%d", -1);
		else
			index += snprintf(&(power_info[index]), len - index, "$$match_power@@%d",
					  (track_status->power_info.wls_info.power >= g_track_chip->track_status.wls_max_power));
	}

	index += snprintf(&(power_info[index]), len - index, "$$soc@@%d",
			  monitor->batt_soc);
	index += snprintf(&(power_info[index]), len - index, "$$smooth_soc@@%d",
			  monitor->smooth_soc);
	index += snprintf(&(power_info[index]), len - index, "$$uisoc@@%d",
			  monitor->ui_soc);
	index += snprintf(&(power_info[index]), len - index, "$$batt_temp@@%d",
			  monitor->batt_temp);
	index += snprintf(&(power_info[index]), len - index, "$$shell_temp@@%d",
			  monitor->shell_temp);
	index += snprintf(&(power_info[index]), len - index, "$$batt_vol@@%d",
			  monitor->vbat_mv);
	index += snprintf(&(power_info[index]), len - index, "$$batt_curr@@%d",
			  monitor->ibat_ma);
	mutex_unlock(&temp_track_status->track_status_lock);

	return 0;
}

static int oplus_chg_track_get_err_comm_info(struct oplus_chg_track *track, char *buf, int buf_size)
{
	struct oplus_chg_track_status *track_status;
	struct oplus_monitor *monitor;
	int index;

	monitor = track->monitor;
	track_status = &track->track_status;

	oplus_chg_track_get_charger_type(monitor, track_status,
					 TRACK_CHG_GET_THTS_TIME_TYPE);
	index = snprintf(buf, buf_size, "$$power_mode@@%s",
			 track_status->power_info.power_mode);

	if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRE) {
		index += snprintf(
			buf + index, buf_size - index, "$$adapter_t@@%s",
			track_status->power_info.wired_info.adapter_type);
		if (track_status->power_info.wired_info.adapter_id)
			index += snprintf(
				buf + index, buf_size - index,
				"$$adapter_id@@0x%x",
				track_status->power_info.wired_info.adapter_id);
		index += snprintf(buf + index, buf_size - index, "$$power@@%d",
				  track_status->power_info.wired_info.power);
		index += snprintf(buf + index, buf_size - index, "$$vbus@@%d",
				  monitor->wired_vbus_mv);
		if (track_status->wired_max_power <= 0)
			index += snprintf(buf + index, buf_size - index, "$$match_power@@%d", -1);
		else
			index += snprintf(buf + index, buf_size - index, "$$match_power@@%d",
					  (track_status->power_info.wired_info.power >= track_status->wired_max_power));
	} else if (track_status->power_info.power_type == TRACK_CHG_TYPE_WIRELESS) {
		index += snprintf(
			buf + index, buf_size - index, "$$adapter_t@@%s",
			track_status->power_info.wls_info.adapter_type);
		if (strlen(track_status->power_info.wls_info.dock_type))
			index += snprintf(
				buf + index, buf_size - index,
				"$$dock_type@@%s",
				track_status->power_info.wls_info.dock_type);
		index += snprintf(buf + index, buf_size - index, "$$power@@%d",
				  track_status->power_info.wls_info.power);
		index += snprintf(buf + index, buf_size - index, "$$vbus@@%d",
				  monitor->wls_vout_mv);
		if (track_status->wls_max_power <= 0)
			index += snprintf(buf + index, buf_size - index, "$$match_power@@%d", -1);
		else
			index += snprintf(buf + index, buf_size - index, "$$match_power@@%d",
					  (track_status->power_info.wls_info.power >= track_status->wls_max_power));
	}

	return index;
}

static int oplus_chg_track_upload_ic_err_info(struct oplus_chg_track *track)
{
	union mms_msg_data data = { 0 };
	int index;
	int name_index, msg_index;
	int err_type, sub_err_type;
	char *msg_buf, *track_buf;
	int rc;

	rc = oplus_mms_get_item_data(track->monitor->err_topic, ERR_ITEM_IC,
				     &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		return rc;
	}
	msg_buf = data.strval;
	track_buf = track->ic_err_msg_load_trigger.crux_info;

	msg_buf = kzalloc(TOPIC_MSG_STR_BUF, GFP_KERNEL);
	if (msg_buf == NULL) {
		chg_err("alloc msg buf error");
		return -ENOMEM;
	}
	memcpy(msg_buf, data.strval, TOPIC_MSG_STR_BUF);

	rc = oplus_mms_analysis_ic_err_msg(msg_buf, TOPIC_MSG_STR_BUF,
					   &name_index, &err_type,
					   &sub_err_type, &msg_index);
	if (rc < 0) {
		chg_err("error msg could not be analysis,  rc=%d\n", rc);
		kfree(msg_buf);
		return rc;
	}
	chg_info("err_type=%d, sub_err_type=%d\n", err_type, sub_err_type);

	index = snprintf(track_buf, OPLUS_CHG_TRACK_CURX_INFO_LEN,
			 "$$device_id@@%s$$err_type@@%d", msg_buf + name_index,
			 sub_err_type);

	switch (err_type) {
	case OPLUS_IC_ERR_GPIO:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_GPIO_ABNORMAL;
		break;
	case OPLUS_IC_ERR_PLAT_PMIC:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_PLAT_PMIC_ABNORMAL;
		index += oplus_chg_track_get_err_comm_info(track,
			track_buf + index, OPLUS_CHG_TRIGGER_MSG_LEN - index);
		break;
	case OPLUS_IC_ERR_BUCK_BOOST:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_EXTERN_PMIC_ABNORMAL;
		break;
	case OPLUS_IC_ERR_GAUGE:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_GAGUE_ABNORMAL;
		break;
	case OPLUS_IC_ERR_WLS_RX:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_WLS_ABNORMAL;
		index += oplus_chg_track_get_err_comm_info(track,
			track_buf + index, OPLUS_CHG_TRIGGER_MSG_LEN - index);
		break;
	case OPLUS_IC_ERR_CP:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_CP_ABNORMAL;
		break;
	case OPLUS_IC_ERR_CC_LOGIC:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_EXTERN_PMIC_ABNORMAL;
		break;
	case OPLUS_IC_ERR_PARALLEL_UNBALANCE:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_PARALLEL_UNBALANCE_ABNORMAL;
		break;
	case OPLUS_IC_ERR_MOS_ERROR:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_MOS_ERROR_ABNORMAL;
		break;
	case OPLUS_IC_ERR_UFCS:
		track->ic_err_msg_load_trigger.flag_reason =
			TRACK_NOTIFY_FLAG_UFCS_IC_ABNORMAL;
		index += oplus_chg_track_obtain_power_info(track_buf + index,
			OPLUS_CHG_TRIGGER_MSG_LEN - index);
		break;
	case OPLUS_IC_ERR_I2C:
	case OPLUS_IC_ERR_UNKNOWN:
	default:
		chg_err("unsupported error type(%d)\n", err_type);
		kfree(msg_buf);
		return -EINVAL;
	}

	index += snprintf(track_buf + index, OPLUS_CHG_TRIGGER_MSG_LEN - index,
			  "$$ic_msg@@%s", msg_buf + msg_index);

	schedule_delayed_work(&track->ic_err_msg_trigger_work, 0);
	chg_info("%s\n", track_buf);
	kfree(msg_buf);

	return 0;
}

static int oplus_chg_track_upload_usbtemp_info(struct oplus_chg_track *track)
{
	union mms_msg_data data = { 0 };
	int index;
	int rc;

	rc = oplus_mms_get_item_data(track->monitor->err_topic,
				     ERR_ITEM_USBTEMP, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		return rc;
	}
	index = snprintf(track->usbtemp_load_trigger.crux_info,
			 OPLUS_CHG_TRACK_CURX_INFO_LEN, "%s", data.strval);
	oplus_chg_track_obtain_power_info(
		&track->usbtemp_load_trigger.crux_info[index],
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&track->usbtemp_load_trigger_work, 0);
	chg_info("%s\n", track->usbtemp_load_trigger.crux_info);

	return 0;
}

static int
oplus_chg_track_upload_vbatt_too_low_info(struct oplus_chg_track *track)
{
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(track->monitor->err_topic,
				     ERR_ITEM_VBAT_TOO_LOW, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		return rc;
	}
	snprintf(track->vbatt_too_low_load_trigger.crux_info,
		 OPLUS_CHG_TRACK_CURX_INFO_LEN, "%s", data.strval);

	schedule_delayed_work(&track->vbatt_too_low_load_trigger_work, 0);
	chg_info("%s\n", track->vbatt_too_low_load_trigger.crux_info);

	return 0;
}

static int
oplus_chg_track_upload_vbatt_diff_over_info(struct oplus_chg_track *track)
{
	struct oplus_monitor *monitor = track->monitor;

	(void)snprintf(track->vbatt_diff_over_load_trigger.crux_info,
		       OPLUS_CHG_TRACK_CURX_INFO_LEN,
		       "$$soc@@%d$$smooth_soc@@%d$$uisoc@@%d$$vbatt_max@@%d$$vbatt_min@@%d"
		       "$$vbatt_diff@@%d$$batt_rm@@%d$$batt_fcc@@%d$$batt_cc@@%d"
			"$$charger_exist@@%d$$vdiff_type@@%s",
		       monitor->batt_soc, monitor->smooth_soc, monitor->ui_soc,
		       monitor->vbat_mv, monitor->vbat_min_mv,
		       monitor->vbat_mv - monitor->vbat_min_mv,
		       monitor->batt_rm, monitor->batt_fcc, monitor->batt_cc,
		       (monitor->wired_online || monitor->wls_online),
		       (monitor->batt_status == POWER_SUPPLY_STATUS_FULL) ?
			       "full" : "shutdown");

	schedule_delayed_work(&track->vbatt_diff_over_load_trigger_work, 0);
	chg_info("%s\n", track->vbatt_diff_over_load_trigger.crux_info);

	return 0;
}

int oplus_chg_track_charge_full(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *chip;

	if (monitor == NULL) {
		chg_err("monitor is NULL\n");
		return -ENODEV;
	}

	chip = monitor->track;
	if (monitor->batt_status == POWER_SUPPLY_STATUS_FULL)
		oplus_chg_track_upload_vbatt_diff_over_info(chip);

	return 0;
}

static int
oplus_chg_track_upload_uisoc_keep_1_t_info(struct oplus_chg_track *chip)
{
	struct oplus_monitor *monitor = chip->monitor;
	int index = 0;
	int uisoc_1_end_time;

	uisoc_1_end_time = oplus_chg_track_get_local_time_s();
	chg_info("uisoc_1_end_time:%d, uisoc_1_start_time:%d\n",
		 uisoc_1_end_time, chip->uisoc_1_start_time);
	memset(chip->uisoc_keep_1_t_load_trigger.crux_info, 0,
	       sizeof(chip->uisoc_keep_1_t_load_trigger.crux_info));
	index += snprintf(&(chip->uisoc_keep_1_t_load_trigger.crux_info[index]),
			  OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$uisoc_keep_1_t@@%d",
			  uisoc_1_end_time - chip->uisoc_1_start_time);

	index += snprintf(
		&(chip->uisoc_keep_1_t_load_trigger.crux_info[index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
		"$$pre_vbatt_max@@%d$$pre_vbatt_min@@%d$$curr_vbatt_max@@%d"
		"$$curr_vbatt_min@@%d$$soc@@%d$$smooth_soc@@%d$$uisoc@@%d$$start_batt_rm@@%d"
		"$$curr_batt_rm@@%d$$batt_curr@@%d",
		chip->uisoc_1_start_vbatt_max, chip->uisoc_1_start_vbatt_min,
		monitor->vbat_mv, monitor->vbat_min_mv, monitor->batt_soc,
		monitor->smooth_soc, monitor->ui_soc, chip->uisoc_1_start_batt_rm,
		monitor->batt_rm, monitor->ibat_ma);

	schedule_delayed_work(&chip->uisoc_keep_1_t_load_trigger_work, 0);
	chg_info("%s\n", chip->uisoc_keep_1_t_load_trigger.crux_info);
	msleep(200);

	return 0;
}

static int
oplus_chg_track_upload_dual_chan_err_info(struct oplus_chg_track *track)
{
	union mms_msg_data data = { 0 };
	int rc;

	rc = oplus_mms_get_item_data(track->monitor->err_topic,
				     ERR_ITEM_DUAL_CHAN, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		return rc;
	}
	snprintf(track->dual_chan_err_load_trigger.crux_info,
		 OPLUS_CHG_TRACK_CURX_INFO_LEN, "%s", data.strval);

	schedule_delayed_work(&track->dual_chan_err_load_trigger_work, 0);
	chg_info("%s\n", track->dual_chan_err_load_trigger.crux_info);

	return 0;
}

int oplus_chg_track_set_uisoc_1_start(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *chip;

	if (monitor == NULL) {
		chg_err("monitor is NULL\n");
		return -ENODEV;
	}

	chip = monitor->track;
	chip->uisoc_1_start_time = oplus_chg_track_get_local_time_s();
	chip->uisoc_1_start_batt_rm = monitor->batt_rm;
	chip->uisoc_1_start_vbatt_max = monitor->vbat_mv;
	chip->uisoc_1_start_vbatt_min = monitor->vbat_min_mv;

	return 0;
}

static int oplus_chg_track_upload_mmi_chg_info(struct oplus_chg_track *chip)
{
	int index = 0;
	union mms_msg_data data = { 0 };
	int rc = 0;

	if (!chip)
		return -EINVAL;

	mutex_lock(&chip->mmi_chg_info_lock);
	if (chip->mmi_chg_info_trigger)
		kfree(chip->mmi_chg_info_trigger);

	chip->mmi_chg_info_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->mmi_chg_info_trigger) {
		pr_err("mmi_chg_info_trigger memery alloc fail\n");
		mutex_unlock(&chip->mmi_chg_info_lock);
		return -ENOMEM;
	}

	chip->mmi_chg_info_trigger->type_reason = TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->mmi_chg_info_trigger->flag_reason = TRACK_NOTIFY_FLAG_MMI_CHG_INFO;

	rc = oplus_mms_get_item_data(chip->monitor->err_topic, ERR_ITEM_MMI_CHG, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		kfree(chip->mmi_chg_info_trigger);
		chip->mmi_chg_info_trigger = NULL;
		mutex_unlock(&chip->mmi_chg_info_lock);
		return rc;
	}

	index += snprintf(&(chip->mmi_chg_info_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s",
			  data.strval);
	oplus_chg_track_obtain_power_info(&(chip->mmi_chg_info_trigger->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&chip->mmi_chg_info_trigger_work, 0);
	chg_info("success\n");
	return 0;
}

static int oplus_chg_track_upload_slow_chg_info(struct oplus_chg_track *chip)
{
	int index = 0;
	union mms_msg_data data = { 0 };
	int rc = 0;

	if (!chip)
		return -EINVAL;

	mutex_lock(&chip->slow_chg_info_lock);
	if (chip->slow_chg_info_trigger)
		kfree(chip->slow_chg_info_trigger);

	chip->slow_chg_info_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->slow_chg_info_trigger) {
		pr_err("slow_chg_info_trigger memery alloc fail\n");
		mutex_unlock(&chip->slow_chg_info_lock);
		return -ENOMEM;
	}

	chip->slow_chg_info_trigger->type_reason = TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->slow_chg_info_trigger->flag_reason = TRACK_NOTIFY_FLAG_SLOW_CHG_INFO;

	rc = oplus_mms_get_item_data(chip->monitor->err_topic, ERR_ITEM_SLOW_CHG, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		kfree(chip->slow_chg_info_trigger);
		chip->slow_chg_info_trigger = NULL;
		mutex_unlock(&chip->slow_chg_info_lock);
		return rc;
	}

	index += snprintf(&(chip->slow_chg_info_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s",
			  data.strval);
	oplus_chg_track_obtain_power_info(&(chip->slow_chg_info_trigger->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&chip->slow_chg_info_trigger_work, 0);
	chg_info("success\n");
	return 0;
}

static int oplus_chg_track_upload_chg_cycle_info(struct oplus_chg_track *chip)
{
	int index = 0;
	union mms_msg_data data = { 0 };
	int rc = 0;

	if (!chip)
		return -EINVAL;

	mutex_lock(&chip->chg_cycle_info_lock);
	if (chip->chg_cycle_info_trigger)
		kfree(chip->chg_cycle_info_trigger);

	chip->chg_cycle_info_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->chg_cycle_info_trigger) {
		pr_err("chg_cycle_info_trigger memery alloc fail\n");
		mutex_unlock(&chip->chg_cycle_info_lock);
		return -ENOMEM;
	}

	chip->chg_cycle_info_trigger->type_reason = TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->chg_cycle_info_trigger->flag_reason = TRACK_NOTIFY_FLAG_CHG_CYCLE_INFO;

	rc = oplus_mms_get_item_data(chip->monitor->err_topic, ERR_ITEM_CHG_CYCLE, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		kfree(chip->chg_cycle_info_trigger);
		chip->chg_cycle_info_trigger = NULL;
		mutex_unlock(&chip->chg_cycle_info_lock);
		return rc;
	}

	index += snprintf(&(chip->chg_cycle_info_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "%s$$status@@%d", data.strval, chip->monitor->chg_cycle_status);
	oplus_chg_track_obtain_power_info(&(chip->chg_cycle_info_trigger->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&chip->chg_cycle_info_trigger_work, 0);
	chg_info("success\n");
	return 0;
}

static int oplus_chg_track_upload_wls_info(struct oplus_chg_track *chip)
{
	int index = 0;
	union mms_msg_data data = { 0 };
	int rc = 0;

	if (!chip)
		return -EINVAL;

	mutex_lock(&chip->wls_info_lock);
	if (chip->wls_info_trigger)
		kfree(chip->wls_info_trigger);

	chip->wls_info_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->wls_info_trigger) {
		chg_err("wls_info_trigger memery alloc fail\n");
		mutex_unlock(&chip->wls_info_lock);
		return -ENOMEM;
	}

	chip->wls_info_trigger->type_reason = TRACK_NOTIFY_TYPE_GENERAL_RECORD;
	chip->wls_info_trigger->flag_reason = TRACK_NOTIFY_FLAG_WLS_INFO;

	rc = oplus_mms_get_item_data(chip->monitor->err_topic, ERR_ITEM_WLS_INFO, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		kfree(chip->wls_info_trigger);
		chip->wls_info_trigger = NULL;
		mutex_unlock(&chip->wls_info_lock);
		return rc;
	}

	index += snprintf(&(chip->wls_info_trigger->crux_info[index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s", data.strval);
	oplus_chg_track_obtain_power_info(&(chip->wls_info_trigger->crux_info[index]),
		OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&chip->wls_info_trigger_work, 0);
	chg_info("success\n");
	return 0;
}

static int oplus_chg_track_upload_ufcs_info(struct oplus_chg_track *chip)
{
	int index = 0;
	union mms_msg_data data = { 0 };
	int rc = 0;

	if (!chip)
		return -EINVAL;

	mutex_lock(&chip->ufcs_info_lock);
	if (chip->ufcs_info_trigger)
		kfree(chip->ufcs_info_trigger);

	chip->ufcs_info_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->ufcs_info_trigger) {
		chg_err("ufcs_info_trigger memery alloc fail\n");
		mutex_unlock(&chip->ufcs_info_lock);
		return -ENOMEM;
	}

	chip->ufcs_info_trigger->type_reason = TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL;
	chip->ufcs_info_trigger->flag_reason = TRACK_NOTIFY_FLAG_UFCS_ABNORMAL;

	rc = oplus_mms_get_item_data(chip->monitor->err_topic, ERR_ITEM_UFCS, &data, false);
	if (rc < 0) {
		chg_err("get msg data error, rc=%d\n", rc);
		kfree(chip->ufcs_info_trigger);
		chip->ufcs_info_trigger = NULL;
		mutex_unlock(&chip->ufcs_info_lock);
		return rc;
	}

	index += snprintf(&(chip->ufcs_info_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "%s", data.strval);
	oplus_chg_track_obtain_power_info(&(chip->ufcs_info_trigger->crux_info[index]),
					  OPLUS_CHG_TRACK_CURX_INFO_LEN - index);

	schedule_delayed_work(&chip->ufcs_info_trigger_work, 0);
	chg_info("success\n");
	return 0;
}

static int
oplus_chg_track_cal_period_chg_capaticy(struct oplus_chg_track *track_chip)
{
	int ret = 0;

	if (!track_chip)
		return -EFAULT;

	if (track_chip->track_status.chg_start_soc >
	    TRACK_PERIOD_CHG_CAP_MAX_SOC)
		return ret;

	chg_debug("enter\n");
	schedule_delayed_work(&track_chip->cal_chg_five_mins_capacity_work,
			      msecs_to_jiffies(TRACK_TIME_5MIN_JIFF_THD));
	schedule_delayed_work(&track_chip->cal_chg_ten_mins_capacity_work,
			      msecs_to_jiffies(TRACK_TIME_10MIN_JIFF_THD));

	return ret;
}

static int oplus_chg_track_cancel_cal_period_chg_capaticy(
	struct oplus_chg_track *track_chip)
{
	int ret = 0;

	if (!track_chip)
		return -EFAULT;

	chg_debug("enter\n");
	if (delayed_work_pending(&track_chip->cal_chg_five_mins_capacity_work))
		cancel_delayed_work_sync(
			&track_chip->cal_chg_five_mins_capacity_work);

	if (delayed_work_pending(&track_chip->cal_chg_ten_mins_capacity_work))
		cancel_delayed_work_sync(
			&track_chip->cal_chg_ten_mins_capacity_work);

	return ret;
}

static int oplus_chg_track_cal_period_chg_average_speed(
	struct oplus_chg_track_status *track_status,
	struct oplus_monitor *monitor)
{
	int ret = 0;
	int curr_time;

	if (!track_status || !monitor)
		return -EFAULT;

	chg_debug("enter\n");
	if (track_status->chg_average_speed ==
	    TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT) {
		curr_time = oplus_chg_track_get_local_time_s();
		track_status->chg_average_speed =
			TRACK_TIME_1MIN_THD *
			(monitor->batt_rm - track_status->chg_start_rm) /
			(curr_time - track_status->chg_start_time);
		chg_info("curr_rm:%d, chg_start_rm:%d, curr_time:%d, chg_start_time:%d,"
			"chg_average_speed:%d\n",
			monitor->batt_rm, track_status->chg_start_rm, curr_time,
			track_status->chg_start_time,
			track_status->chg_average_speed);
	}

	return ret;
}

static int
oplus_chg_track_cal_chg_end_soc(struct oplus_monitor *monitor,
				struct oplus_chg_track_status *track_status)
{
	if (!track_status || !monitor)
		return -EFAULT;

	if (monitor->batt_status == POWER_SUPPLY_STATUS_CHARGING)
		track_status->chg_end_soc = monitor->batt_soc;

	return 0;
}

static void oplus_chg_track_reset_chg_abnormal_happened_flag(
	struct oplus_chg_track_status *track_status)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(chg_abnormal_reason_table); i++)
		chg_abnormal_reason_table[i].happened = false;
}

static int
oplus_chg_track_status_reset(struct oplus_chg_track_status *track_status)
{
	track_status->chg_no_charging_cnt = 0;
	track_status->ledon_ave_speed = 0;
	track_status->ledoff_ave_speed = 0;
	track_status->ledon_rm = 0;
	track_status->ledoff_rm = 0;
	track_status->ledon_time = 0;
	track_status->ledoff_time = 0;
	track_status->continue_ledon_time = 0;
	track_status->continue_ledoff_time = 0;
	track_status->chg_total_cnt = 0;
	track_status->chg_fast_full_time = 0;
	track_status->chg_normal_full_time = 0;
	track_status->chg_report_full_time = 0;
	track_status->aging_ffc_trig = false;
	memset(&(track_status->aging_ffc_judge_vol), 0, sizeof(track_status->aging_ffc_judge_vol));
	track_status->aging_ffc_start_time = 0;
	track_status->aging_ffc_to_full_time = 0;
	track_status->ffc_end_time = 0;
	track_status->ffc_time = 0;
	track_status->cv_time = 0;
	track_status->ffc_start_main_soc = 0;
	track_status->ffc_start_sub_soc = 0;
	track_status->ffc_end_main_soc = 0;
	track_status->ffc_end_sub_soc = 0;
	track_status->dual_chan_open_count = 0;
	track_status->dual_chan_start_time = 0;
	track_status->dual_chan_time = 0;
	track_status->chg_five_mins_cap = TRACK_PERIOD_CHG_CAP_INIT;
	track_status->chg_ten_mins_cap = TRACK_PERIOD_CHG_CAP_INIT;
	track_status->chg_average_speed = TRACK_PERIOD_CHG_AVERAGE_SPEED_INIT;
	track_status->soc_sect_status = TRACK_SOC_SECTION_DEFAULT;
	track_status->tbatt_warm_once = false;
	track_status->tbatt_cold_once = false;
	track_status->cool_down_effect_cnt = 0;
	track_status->chg_speed_is_slow = false;
	track_status->in_rechging = false;
	track_status->rechg_counts = 0;
	track_status->led_onoff_power_cal = false;
	track_status->cool_down_status = 0;
	track_status->has_judge_speed = false;
	track_status->wls_prop_status = TRACK_CHG_DEFAULT;
	track_status->wls_skew_effect_cnt = 0;
	track_status->chg_verity = true;
	memset(&(track_status->batt_full_reason), 0,
	       sizeof(track_status->batt_full_reason));
	oplus_chg_track_clear_cool_down_stats_time(track_status);
	memset(&(track_status->chg_abnormal_reason), 0,
	       sizeof(track_status->chg_abnormal_reason));
	oplus_chg_track_reset_chg_abnormal_happened_flag(track_status);

	track_status->app_status.app_cal = false;
	track_status->app_status.curr_top_index = TRACK_APP_TOP_INDEX_DEFAULT;
	mutex_lock(&track_status->app_status.app_lock);
	strncpy(track_status->app_status.curr_top_name,
		TRACK_APP_REAL_NAME_DEFAULT, TRACK_APP_REAL_NAME_LEN - 1);
	mutex_unlock(&track_status->app_status.app_lock);
	oplus_chg_track_clear_app_time();

	return 0;
}

static int oplus_chg_track_status_reset_when_plugin(
	struct oplus_monitor *monitor,
	struct oplus_chg_track_status *track_status)
{
	track_status->soc_sect_status = TRACK_SOC_SECTION_DEFAULT;
	track_status->chg_speed_is_slow = false;
	track_status->chg_start_time = oplus_chg_track_get_local_time_s();
	track_status->chg_end_time = track_status->chg_start_time;
	track_status->chg_start_soc = monitor->batt_soc;
	track_status->led_on = monitor->led_on;
	track_status->led_change_t = track_status->chg_start_time;
	track_status->led_change_rm = monitor->batt_rm;
	track_status->chg_start_temp = monitor->shell_temp;
	track_status->batt_start_temp = monitor->batt_temp;
	track_status->batt_max_temp = monitor->batt_temp;
	track_status->batt_max_vol = monitor->vbat_mv;
	track_status->batt_max_curr = monitor->ibat_ma;
	track_status->chg_max_vol = monitor->wired_vbus_mv;
	track_status->chg_start_rm = monitor->batt_rm;
	track_status->chg_max_temp = monitor->shell_temp;
	track_status->ledon_time = 0;
	track_status->ledoff_time = 0;
	track_status->continue_ledon_time = 0;
	track_status->continue_ledoff_time = 0;
	track_status->cool_down_status = monitor->cool_down;
	track_status->cool_down_status_change_t = track_status->chg_start_time;
	track_status->has_judge_speed = false;
	track_status->wls_prop_status = TRACK_CHG_DEFAULT;
	track_status->wls_skew_effect_cnt = 0;
	track_status->chg_verity = true;
	track_status->chg_plugin_utc_t = oplus_chg_track_get_current_time_s(
		&track_status->chg_plugin_rtc_t);
	oplus_chg_track_cal_period_chg_capaticy(g_track_chip);
	track_status->prop_status = monitor->batt_status;
	strncpy(track_status->app_status.pre_top_name,
		track_status->app_status.curr_top_name, TRACK_APP_REAL_NAME_LEN - 1);
	track_status->app_status.pre_top_name[TRACK_APP_REAL_NAME_LEN - 1] = '\0';
	track_status->app_status.change_t = track_status->chg_start_time;
	track_status->app_status.curr_top_index =
		oplus_chg_track_match_app_info(
		track_status->app_status.pre_top_name);
	track_status->once_mmi_chg = false;
	track_status->once_chg_cycle_status = CHG_CYCLE_VOTER__NONE;
	track_status->mmi_chg_open_t = 0;
	track_status->mmi_chg_close_t = 0;
	track_status->mmi_chg_constant_t = 0;
	track_status->slow_chg_open_t = 0;
	track_status->slow_chg_close_t = 0;
	track_status->slow_chg_open_n_t = 0;
	track_status->slow_chg_duration = 0;
	track_status->slow_chg_open_cnt = 0;
	track_status->slow_chg_watt = 0;
	track_status->slow_chg_pct = 0;

	chg_info("chg_start_time:%d, chg_start_soc:%d, chg_start_temp:%d, prop_status:%d\n",
		track_status->chg_start_time, track_status->chg_start_soc,
		track_status->chg_start_temp, track_status->prop_status);

	return 0;
}

static int oplus_chg_track_speed_check(struct oplus_monitor *monitor)
{
	int ret = 0;
	int wls_break_work_delay_t;
	int wired_break_work_delay_t;
	int track_recording_time;
	static bool track_reset = true;
	static bool track_record_charger_info = false;
	struct oplus_chg_track_status *track_status;

	if (!g_track_chip)
		return -EFAULT;

	track_status = &g_track_chip->track_status;
	wls_break_work_delay_t = g_track_chip->track_cfg.wls_chg_break_t_thd +
				 TRACK_TIME_1000MS_JIFF_THD;
	wired_break_work_delay_t =
		g_track_chip->track_cfg.fast_chg_break_t_thd +
		TRACK_TIME_500MS_JIFF_THD;

	if (!monitor->wired_online && !monitor->wls_online) {
		if (track_record_charger_info) {
			chg_info("record charger info and upload charger info\n");
			track_status->chg_plugout_utc_t =
				oplus_chg_track_get_current_time_s(
					&track_status->chg_plugout_rtc_t);
			oplus_chg_track_cal_app_stats(monitor, track_status);
			oplus_chg_track_cal_led_on_stats(monitor, track_status);
			oplus_chg_track_cal_period_chg_average_speed(
				track_status, monitor);
			oplus_chg_track_cal_ledon_ledoff_average_speed(
				track_status);
			if (track_status->power_info.power_type ==
			    TRACK_CHG_TYPE_WIRELESS)
				track_status->wls_need_upload = true;
			else
				track_status->wired_need_upload = true;
			if (oplus_chg_track_is_no_charging(track_status)) {
				oplus_chg_track_record_charger_info(
					monitor,
					&g_track_chip->no_charging_trigger,
					track_status);
				if (track_status->power_info.power_type ==
				    TRACK_CHG_TYPE_WIRELESS)
					schedule_delayed_work(
						&g_track_chip->no_charging_trigger_work,
						msecs_to_jiffies(wls_break_work_delay_t));
				else
					schedule_delayed_work(
						&g_track_chip->no_charging_trigger_work,
						msecs_to_jiffies(wired_break_work_delay_t));
			} else if (oplus_chg_track_judge_speed_slow(
					   monitor, g_track_chip)) {
				oplus_chg_track_record_charger_info(
					monitor,
					&g_track_chip->slow_charging_trigger,
					track_status);
				if (track_status->power_info.power_type ==
				    TRACK_CHG_TYPE_WIRELESS)
					schedule_delayed_work(
						&g_track_chip->slow_charging_trigger_work,
						msecs_to_jiffies(wls_break_work_delay_t));
				else
					schedule_delayed_work(
						&g_track_chip->slow_charging_trigger_work,
						msecs_to_jiffies(wired_break_work_delay_t));
			} else {
				oplus_chg_track_record_charger_info(
					monitor,
					&g_track_chip->charger_info_trigger,
					track_status);
				if (track_status->power_info.power_type ==
				    TRACK_CHG_TYPE_WIRELESS)
					schedule_delayed_work(
						&g_track_chip->charger_info_trigger_work,
						msecs_to_jiffies(wls_break_work_delay_t));
				else
					schedule_delayed_work(
						&g_track_chip->charger_info_trigger_work,
						msecs_to_jiffies(wired_break_work_delay_t));
			}
		}
		track_reset = true;
		track_record_charger_info = false;
		oplus_chg_track_cancel_cal_period_chg_capaticy(g_track_chip);
		oplus_chg_track_status_reset(track_status);
		return ret;
	}

	if ((monitor->wired_online || monitor->wls_online) && track_reset) {
		track_reset = false;
		g_track_chip->no_charging_trigger.flag_reason = TRACK_NOTIFY_FLAG_NO_CHARGING;
		oplus_chg_track_status_reset_when_plugin(monitor, track_status);
	}

	track_status->chg_end_time = oplus_chg_track_get_local_time_s();
	track_recording_time =
		track_status->chg_end_time - track_status->chg_start_time;
	if (!track_record_charger_info &&
	    track_recording_time <= TRACK_CYCLE_RECORDIING_TIME_90S)
		oplus_chg_track_get_charger_type(monitor, track_status,
						 TRACK_CHG_GET_THTS_TIME_TYPE);

	oplus_chg_track_cal_tbatt_status(monitor);
	oplus_chg_track_cal_section_soc_inc_rm(monitor, track_status);
	oplus_chg_track_cal_batt_full_time(monitor, track_status);
	oplus_chg_track_cal_chg_common_mesg(monitor, track_status);
	oplus_chg_track_cal_cool_down_stats(monitor, track_status);
	oplus_chg_track_cal_no_charging_stats(monitor, track_status);
	oplus_chg_track_cal_app_stats(monitor, track_status);
	oplus_chg_track_cal_led_on_stats(monitor, track_status);
	oplus_chg_track_check_chg_abnormal(monitor, track_status);
	oplus_chg_track_cal_rechg_counts(monitor, track_status);
	oplus_chg_track_cal_chg_end_soc(monitor, track_status);

	track_status->prop_status = monitor->batt_status;
	if (!track_record_charger_info &&
	    monitor->batt_status == POWER_SUPPLY_STATUS_FULL &&
	    track_recording_time < TRACK_CYCLE_RECORDIING_TIME_2MIN)
		oplus_chg_track_cancel_cal_period_chg_capaticy(g_track_chip);

	if (!track_record_charger_info &&
	    track_recording_time >= TRACK_CYCLE_RECORDIING_TIME_2MIN)
		track_record_charger_info = true;

	chg_debug("track_recording_time=%d, track_record_charger_info=%d\n",
		  track_recording_time, track_record_charger_info);

	return ret;
}

static int oplus_chg_track_uisoc_soc_jump_check(struct oplus_monitor *monitor)
{
	int ret = 0;
	int curr_time_utc;
	int curr_vbatt;
	struct oplus_chg_track_status *track_status;
	int judge_curr_soc = 0;
	struct rtc_time tm;
	int curr_fcc = 0, curr_rm = 0;
	int avg_current = 0;
	static int pre_local_time = 0;
	int curr_local_time = oplus_chg_track_get_local_time_s();

	if (!g_track_chip)
		return -EFAULT;

	curr_time_utc = oplus_chg_track_get_current_time_s(&tm);
	curr_vbatt = monitor->vbat_mv;
	curr_fcc = monitor->batt_fcc;
	curr_rm = monitor->batt_rm;
	track_status = &g_track_chip->track_status;
	if (track_status->curr_soc == -EINVAL) {
		track_status->curr_soc = monitor->batt_soc;
		track_status->pre_soc = monitor->batt_soc;
		track_status->curr_smooth_soc = monitor->smooth_soc;
		track_status->pre_smooth_soc = monitor->smooth_soc;
		track_status->curr_uisoc = monitor->ui_soc;
		track_status->pre_uisoc = monitor->ui_soc;
		track_status->pre_vbatt = curr_vbatt;
		track_status->pre_time_utc = curr_time_utc;
		track_status->pre_fcc = curr_fcc;
		track_status->pre_rm = curr_rm;
		pre_local_time = curr_local_time;
		judge_curr_soc = track_status->curr_smooth_soc;
		if (monitor->soc_load >= 0 && abs(judge_curr_soc - monitor->soc_load) > OPLUS_CHG_TRACK_UI_SOC_LOAD_JUMP_THD) {
			track_status->uisoc_load_jumped = true;
			chg_info("The gap between loaded uisoc and soc is too large\n");
			memset(g_track_chip->uisoc_load_trigger.crux_info, 0,
			       sizeof(g_track_chip->uisoc_load_trigger.crux_info));
			ret = snprintf(g_track_chip->uisoc_load_trigger.crux_info, OPLUS_CHG_TRACK_CURX_INFO_LEN,
				       "$$curr_uisoc@@%d$$curr_soc@@%d$$load_uisoc_soc_gap@@%d"
				       "$$pre_vbatt@@%d$$curr_vbatt@@%d"
				       "$$pre_time_utc@@%d$$curr_time_utc@@%d"
				       "$$charger_exist@@%d$$curr_smooth_soc@@%d"
				       "$$curr_fcc@@%d$$curr_rm@@%d$$current@@%d"
				       "$$soc_load@@%d$$judge_curr_soc@@%d",
				       track_status->curr_uisoc, track_status->curr_soc,
				       judge_curr_soc - monitor->soc_load, track_status->pre_vbatt, curr_vbatt,
				       track_status->pre_time_utc, curr_time_utc,
				       (monitor->wired_online || monitor->wls_online), track_status->curr_smooth_soc,
				       curr_fcc, curr_rm, monitor->ibat_ma, monitor->soc_load, judge_curr_soc);
			schedule_delayed_work(&g_track_chip->uisoc_load_trigger_work,
					      msecs_to_jiffies(TRACK_TIME_SCHEDULE_UI_SOC_LOAD_JUMP));
		}
	} else {
		track_status->curr_soc = track_status->debug_soc != OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID ?
						 track_status->debug_soc :
						 monitor->batt_soc;
		track_status->curr_smooth_soc = track_status->debug_soc != OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID ?
							track_status->debug_soc :
							monitor->smooth_soc;
		track_status->curr_uisoc = track_status->debug_uisoc != OPLUS_CHG_TRACK_DEBUG_UISOC_SOC_INVALID ?
						   track_status->debug_uisoc :
						   monitor->ui_soc;
	}

	if (curr_time_utc > track_status->pre_time_utc)
		avg_current = (track_status->pre_rm - curr_rm) / oplus_gauge_get_batt_num() * TRACK_TIME_1HOU_THD /
			      (curr_time_utc - track_status->pre_time_utc);

	if (!track_status->soc_jumped &&
	    abs(track_status->curr_soc - track_status->pre_soc) > OPLUS_CHG_TRACK_SOC_JUMP_THD) {
		track_status->soc_jumped = true;
		chg_info("The gap between curr_soc and pre_soc is too large\n");
		memset(g_track_chip->soc_trigger.crux_info, 0, sizeof(g_track_chip->soc_trigger.crux_info));
		ret = snprintf(g_track_chip->soc_trigger.crux_info, OPLUS_CHG_TRACK_CURX_INFO_LEN,
			       "$$curr_soc@@%d$$pre_soc@@%d$$curr_soc_pre_soc_gap@@%d"
			       "$$pre_vbatt@@%d$$curr_vbatt@@%d"
			       "$$pre_time_utc@@%d$$curr_time_utc@@%d$$kernel_diff_t@@%d"
			       "$$charger_exist@@%d$$avg_current@@%d$$current@@%d"
			       "$$pre_fcc@@%d$$pre_rm@@%d$$curr_fcc@@%d$$curr_rm@@%d",
			       track_status->curr_soc, track_status->pre_soc,
			       track_status->curr_soc - track_status->pre_soc, track_status->pre_vbatt, curr_vbatt,
			       track_status->pre_time_utc, curr_time_utc, (curr_local_time - pre_local_time),
			       (monitor->wired_online || monitor->wls_online), avg_current, monitor->ibat_ma,
			       track_status->pre_fcc, track_status->pre_rm, curr_fcc, curr_rm);
		schedule_delayed_work(&g_track_chip->soc_trigger_work, 0);
	} else {
		if (track_status->soc_jumped && track_status->curr_soc == track_status->pre_soc)
			track_status->soc_jumped = false;
	}

	if (!track_status->uisoc_jumped &&
	    abs(track_status->curr_uisoc - track_status->pre_uisoc) > OPLUS_CHG_TRACK_UI_SOC_JUMP_THD) {
		track_status->uisoc_jumped = true;
		chg_info("The gap between curr_uisoc and pre_uisoc is too large\n");
		memset(g_track_chip->uisoc_trigger.crux_info, 0, sizeof(g_track_chip->uisoc_trigger.crux_info));
		ret = snprintf(g_track_chip->uisoc_trigger.crux_info, OPLUS_CHG_TRACK_CURX_INFO_LEN,
			       "$$curr_uisoc@@%d$$pre_uisoc@@%d$$curr_uisoc_pre_uisoc_gap@@%d"
			       "$$pre_vbatt@@%d$$curr_vbatt@@%d"
			       "$$pre_time_utc@@%d$$curr_time_utc@@%d$$kernel_diff_t@@%d"
			       "$$charger_exist@@%d$$avg_current@@%d$$current@@%d"
			       "$$pre_fcc@@%d$$pre_rm@@%d$$curr_fcc@@%d$$curr_rm@@%d",
			       track_status->curr_uisoc, track_status->pre_uisoc,
			       track_status->curr_uisoc - track_status->pre_uisoc, track_status->pre_vbatt, curr_vbatt,
			       track_status->pre_time_utc, curr_time_utc, (curr_local_time - pre_local_time),
			       (monitor->wired_online || monitor->wls_online), avg_current, monitor->ibat_ma,
			       track_status->pre_fcc, track_status->pre_rm, curr_fcc, curr_rm);
		schedule_delayed_work(&g_track_chip->uisoc_trigger_work, 0);
	} else {
		if (track_status->uisoc_jumped && track_status->curr_uisoc == track_status->pre_uisoc)
			track_status->uisoc_jumped = false;
	}

	judge_curr_soc = track_status->curr_smooth_soc;

	if (!track_status->uisoc_to_soc_jumped && !track_status->uisoc_load_jumped &&
	    abs(track_status->curr_uisoc - judge_curr_soc) > OPLUS_CHG_TRACK_UI_SOC_TO_SOC_JUMP_THD) {
		track_status->uisoc_to_soc_jumped = true;
		chg_info("The gap between curr_uisoc and curr_soc is too large\n");
		memset(g_track_chip->uisoc_to_soc_trigger.crux_info, 0,
		       sizeof(g_track_chip->uisoc_to_soc_trigger.crux_info));
		ret = snprintf(g_track_chip->uisoc_to_soc_trigger.crux_info, OPLUS_CHG_TRACK_CURX_INFO_LEN,
			       "$$curr_uisoc@@%d$$curr_soc@@%d$$curr_uisoc_curr_soc_gap@@%d"
			       "$$pre_vbatt@@%d$$curr_vbatt@@%d"
			       "$$pre_time_utc@@%d$$curr_time_utc@@%d$$kernel_diff_t@@%d"
			       "$$charger_exist@@%d$$curr_smooth_soc@@%d$$avg_current@@%d$$current@@%d"
			       "$$pre_fcc@@%d$$pre_rm@@%d$$curr_fcc@@%d$$curr_rm@@%d",
			       track_status->curr_uisoc, track_status->curr_soc,
			       track_status->curr_uisoc - judge_curr_soc, track_status->pre_vbatt, curr_vbatt,
			       track_status->pre_time_utc, curr_time_utc, (curr_local_time - pre_local_time),
			       (monitor->wired_online || monitor->wls_online), track_status->curr_smooth_soc,
			       avg_current, monitor->ibat_ma,
			       track_status->pre_fcc, track_status->pre_rm, curr_fcc, curr_rm);
		schedule_delayed_work(&g_track_chip->uisoc_to_soc_trigger_work, 0);
	} else {
		if (track_status->curr_uisoc == judge_curr_soc) {
			track_status->uisoc_to_soc_jumped = false;
			track_status->uisoc_load_jumped = false;
		}
	}

	chg_debug("debug_soc:0x%x, debug_uisoc:0x%x, pre_soc:%d, curr_soc:%d,"
		  "pre_uisoc:%d, curr_uisoc:%d, pre_smooth_soc:%d, curr_smooth_soc:%d\n",
		  track_status->debug_soc, track_status->debug_uisoc, track_status->pre_soc, track_status->curr_soc,
		  track_status->pre_uisoc, track_status->curr_uisoc, track_status->pre_smooth_soc,
		  track_status->curr_smooth_soc);

	track_status->pre_soc = track_status->curr_soc;
	track_status->pre_smooth_soc = track_status->curr_smooth_soc;
	track_status->pre_uisoc = track_status->curr_uisoc;
	track_status->pre_vbatt = curr_vbatt;
	track_status->pre_time_utc = curr_time_utc;
	track_status->pre_fcc = curr_fcc;
	track_status->pre_rm = curr_rm;
	pre_local_time = curr_local_time;

	return ret;
}

int oplus_chg_track_comm_monitor(struct oplus_monitor *monitor)
{
	int ret = 0;

	if (!monitor)
		return -EFAULT;

	if (likely(monitor->ui_soc_ready))
		ret = oplus_chg_track_uisoc_soc_jump_check(monitor);
	ret |= oplus_chg_track_speed_check(monitor);

	return ret;
}

static void oplus_chg_track_err_subs_callback(struct mms_subscribe *subs,
					      enum mms_msg_type type, u32 id)
{
	struct oplus_chg_track *track = subs->priv_data;

	switch (type) {
	case MSG_TYPE_ITEM:
		chg_err("err msg id:%d\n", id);
		switch (id) {
		case ERR_ITEM_IC:
			oplus_chg_track_upload_ic_err_info(track);
			break;
		case ERR_ITEM_USBTEMP:
			oplus_chg_track_upload_usbtemp_info(track);
			break;
		case ERR_ITEM_VBAT_TOO_LOW:
			oplus_chg_track_upload_vbatt_too_low_info(track);
			break;
		case ERR_ITEM_VBAT_DIFF_OVER:
			break;
		case ERR_ITEM_UI_SOC_SHUTDOWN:
			oplus_chg_track_upload_vbatt_diff_over_info(track);
			oplus_chg_track_upload_uisoc_keep_1_t_info(track);
			break;
		case ERR_ITEM_DUAL_CHAN:
			oplus_chg_track_upload_dual_chan_err_info(track);
			break;
		case ERR_ITEM_MMI_CHG:
			oplus_chg_track_upload_mmi_chg_info(track);
			break;
		case ERR_ITEM_SLOW_CHG:
			oplus_chg_track_upload_slow_chg_info(track);
			break;
		case ERR_ITEM_CHG_CYCLE:
			oplus_chg_track_upload_chg_cycle_info(track);
			break;
		case ERR_ITEM_WLS_INFO:
			oplus_chg_track_upload_wls_info(track);
			break;
		case ERR_ITEM_UFCS:
			oplus_chg_track_upload_ufcs_info(track);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static int oplus_chg_track_subscribe_err_topic(struct oplus_chg_track *track)
{
	track->err_subs =
		oplus_mms_subscribe(track->monitor->err_topic, track,
				    oplus_chg_track_err_subs_callback, "track");
	if (IS_ERR_OR_NULL(track->err_subs)) {
		chg_err("subscribe error topic error, rc=%ld\n",
			PTR_ERR(track->err_subs));
		return PTR_ERR(track->err_subs);
	}

	return 0;
}

static int oplus_chg_track_debugfs_init(struct oplus_chg_track *track_dev)
{
	int ret = 0;
	struct dentry *debugfs_root;
	struct dentry *debugfs_general;
	struct dentry *debugfs_chg_slow;

	debugfs_root = oplus_chg_track_get_debugfs_root();
	if (!debugfs_root) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_general = debugfs_create_dir("general", debugfs_root);
	if (!debugfs_general) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_chg_slow = debugfs_create_dir("chg_slow", debugfs_general);
	if (!debugfs_chg_slow) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_create_u8("debug_soc", 0644, debugfs_general,
			  &(track_dev->track_status.debug_soc));
	debugfs_create_u8("debug_uisoc", 0644, debugfs_general,
			  &(track_dev->track_status.debug_uisoc));
	debugfs_create_u8("debug_fast_prop_status", 0644, debugfs_general,
			  &(track_dev->track_status.debug_fast_prop_status));
	debugfs_create_u8(
		"debug_normal_charging_state", 0644, debugfs_general,
		&(track_dev->track_status.debug_normal_charging_state));
	debugfs_create_u8("debug_normal_prop_status", 0644, debugfs_general,
			  &(track_dev->track_status.debug_normal_prop_status));
	debugfs_create_u8("debug_no_charging", 0644, debugfs_general,
			  &(track_dev->track_status.debug_no_charging));
	debugfs_create_u8("debug_slow_charging_force", 0644, debugfs_chg_slow,
			  &(track_dev->track_status.debug_slow_charging_force));
	debugfs_create_u8(
		"debug_slow_charging_reason", 0644, debugfs_chg_slow,
		&(track_dev->track_status.debug_slow_charging_reason));
	debugfs_create_u32("debug_fast_chg_break_t_thd", 0644, debugfs_general,
			   &(track_dev->track_cfg.fast_chg_break_t_thd));
	debugfs_create_u32("debug_general_chg_break_t_thd", 0644,
			   debugfs_general,
			   &(track_dev->track_cfg.general_chg_break_t_thd));
	debugfs_create_u32("debug_wls_chg_break_t_thd", 0644, debugfs_general,
			   &(track_dev->track_cfg.wls_chg_break_t_thd));
	debugfs_create_u32("debug_chg_notify_flag", 0644, debugfs_general,
			   &(track_dev->track_status.debug_chg_notify_flag));
	debugfs_create_u32("debug_chg_notify_code", 0644, debugfs_general,
			   &(track_dev->track_status.debug_chg_notify_code));
	debugfs_create_bool("debug_close_3hours", 0644, debugfs_general,
				&(track_dev->track_status.debug_close_3hours));
	debugfs_create_u32("debug_water_inlet_para", 0644, debugfs_general,
				&(track_dev->track_status.debug_water_inlet_para));
	debugfs_create_u8("debug_plugout_state", 0644, debugfs_general,
			  &(track_dev->track_status.debug_plugout_state));
	debugfs_create_u8("debug_break_code", 0644, debugfs_general,
			  &(track_dev->track_status.debug_break_code));
	return ret;
}

int oplus_chg_track_driver_init(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_dev;
	int rc;

	if (!monitor) {
		chg_err("monitor is NULL\n");
		return -ENODEV;
	}

	track_dev = devm_kzalloc(monitor->dev, sizeof(struct oplus_chg_track),
				 GFP_KERNEL);
	if (track_dev == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	monitor->track = track_dev;
	track_dev->monitor = monitor;

#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
	track_dev->dcs_info = (struct kernel_packet_info *)kmalloc(
		sizeof(char) * OPLUS_CHG_TRIGGER_MSG_LEN, GFP_KERNEL);
	if (track_dev->dcs_info == NULL) {
		rc = -ENOMEM;
		goto dcs_info_kmalloc_fail;
	}
#endif

	temp_track_status = (struct oplus_chg_track_status *)kmalloc(
		sizeof(struct oplus_chg_track_status), GFP_KERNEL);
	if (temp_track_status == NULL) {
		rc = -ENOMEM;
		goto dcs_info_kmalloc_fail;
	}
	mutex_init(&temp_track_status->track_status_lock);

	track_dev->dev = monitor->dev;

	rc = oplus_chg_track_debugfs_init(track_dev);
	if (rc < 0) {
		rc = -ENOENT;
		chg_err("oplus chg track debugfs init fail, rc=%d\n", rc);
		goto debugfs_create_fail;
	}

	rc = oplus_chg_track_parse_dt(track_dev);
	if (rc < 0) {
		chg_err("oplus chg track parse dts error, rc=%d\n", rc);
		goto parse_dt_err;
	}

	oplus_chg_track_init(track_dev);
	rc = oplus_chg_track_thread_init(track_dev);
	if (rc < 0) {
		chg_err("oplus chg track mod init error, rc=%d\n", rc);
		goto track_kthread_init_err;
	}

	rc = oplus_chg_track_subscribe_err_topic(track_dev);
	oplus_chg_track_uisoh_err_init(track_dev);
	if (rc < 0)
		goto subscribe_err_topic_err;

	track_dev->trigger_upload_wq =
		create_workqueue("oplus_chg_trigger_upload_wq");

	INIT_DELAYED_WORK(&track_dev->upload_info_dwork,
			  oplus_chg_track_upload_info_dwork);
	oplus_parallelchg_track_foldmode_init(track_dev);
	oplus_chg_track_ttf_info_init(track_dev);
	g_track_chip = track_dev;
	chg_info("probe done\n");

	return 0;

subscribe_err_topic_err:
track_kthread_init_err:
parse_dt_err:
	if (track_debugfs_root)
		debugfs_remove_recursive(track_debugfs_root);
debugfs_create_fail:
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
	kfree(track_dev->dcs_info);
dcs_info_kmalloc_fail:
#endif
	devm_kfree(monitor->dev, track_dev);
	monitor->track = NULL;
	return rc;
}

int oplus_chg_track_driver_exit(struct oplus_monitor *monitor)
{
	struct oplus_chg_track *track_dev;

	if (!monitor) {
		chg_err("monitor is NULL\n");
		return -ENODEV;
	}
	track_dev = monitor->track;
	if (!track_dev) {
		chg_err("track_dev is NULL\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(track_dev->err_subs))
		oplus_mms_unsubscribe(track_dev->err_subs);

	if (track_debugfs_root)
		debugfs_remove_recursive(track_debugfs_root);
#if defined(CONFIG_OPLUS_FEATURE_FEEDBACK) || \
	defined(CONFIG_OPLUS_FEATURE_FEEDBACK_MODULE) || \
	defined(CONFIG_OPLUS_KEVENT_UPLOAD)
	kfree(track_dev->dcs_info);
#endif
	devm_kfree(monitor->dev, track_dev);
	return 0;
}
