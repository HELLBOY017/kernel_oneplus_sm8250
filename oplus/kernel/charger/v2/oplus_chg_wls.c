// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[CHG_WLS]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/power_supply.h>
#include <linux/miscdevice.h>
#include <linux/sched/clock.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#endif
#ifdef CONFIG_OPLUS_CHARGER_MTK
#include <mtk_boot_common.h>
#endif
#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_monitor.h>
#include <oplus_mms.h>
#include <oplus_mms_wired.h>
#include <oplus_mms_gauge.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_wired.h>
#include <oplus_chg_wls.h>
#include <oplus_hal_wls.h>
#include "monitor/oplus_chg_track.h"

#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
#include "oplus_cfg.h"
#endif

#define OPLUS_CHG_WLS_BREAK_DETECT_DELAY 6000
#define OPLUS_CHG_WLS_START_DETECT_DELAY 3000

#define BCC_TO_ICL 			100
#define OPLUS_WLS_BCC_UPDATE_TIME	500
#define OPLUS_WLS_BCC_UPDATE_INTERVAL	round_jiffies_relative(msecs_to_jiffies(OPLUS_WLS_BCC_UPDATE_TIME))

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0))
#define pde_data(inode) PDE_DATA(inode)
#endif

#ifdef WLS_QI_DEBUG
static int wls_dbg_fcc_ma = 0;
module_param(wls_dbg_fcc_ma, int, 0644);
MODULE_PARM_DESC(wls_dbg_fcc_ma, "debug wls fcc ma");

static int wls_dbg_vout_mv = 0;
module_param(wls_dbg_vout_mv, int, 0644);
MODULE_PARM_DESC(wls_dbg_vout_mv, "debug wls vout mv");
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#ifdef MODULE
#undef printk_ratelimit
#define printk_ratelimit() 0
#endif
#endif

enum wls_err_reason {
	WLS_ERR_NONE,
	WLS_ERR_VERITY,
	WLS_ERR_VOUT_ABNORMAL,
	WLS_ERR_OTHER,
};

struct oplus_chg_cmd {
	unsigned int cmd;
	unsigned int data_size;
	unsigned char data_buf[CHG_CMD_DATA_LEN];
};

struct oplus_chg_wls_fod_cal_data {
	int fod_bpp;
	int fod_epp;
	int fod_fast;
};

struct wls_base_type {
	u8 id;
	int power_max_mw;
};

struct wls_pwr_table {
	u8 f2_id;
	int r_power;/*watt*/
	int t_power;/*watt*/
};

struct battery_info {
	int soc;
	int ui_soc;
	int batt_temp;
	int vbat_mv;
	int ibat_ma;
};

struct wls_mms_info {
	bool rx_present;
	bool rx_online;
	enum oplus_chg_wls_event_code event_code;
	enum wls_status_keep_type wls_status_keep;
};

struct oplus_chg_rx_msg {
	u8 msg_type;
	u8 data;
	u8 buf[3];
	u8 respone_type;
	bool pending;
	bool long_data;
};

struct wls_dev_cmd {
	unsigned int cmd;
	unsigned int data_size;
	unsigned char data_buf[128];
};

struct wls_auth_result {
	u8 random_num[WLS_AUTH_RANDOM_LEN];
	u8 encode_num[WLS_AUTH_ENCODE_LEN];
};

struct wls_third_part_auth_result {
	int effc_key_index;
	u8 aes_random_num[WLS_AUTH_AES_RANDOM_LEN];
	u8 aes_encode_num[WLS_AUTH_AES_ENCODE_LEN];
};

struct oplus_chg_wls_range_data {
	int32_t low_threshold;
	int32_t high_threshold;
	uint32_t curr_ma;
	uint32_t vol_max_mv;
	int32_t need_wait;
} __attribute__((packed));

struct oplus_chg_wls_bcc_data {
	uint32_t max_batt_volt;
	uint32_t max_curr;
	uint32_t min_curr;
	int32_t exit;
} __attribute__((packed));

struct oplus_chg_wls_skin_range_data {
	int32_t low_threshold;
	int32_t high_threshold;
	uint32_t curr_ma;
} __attribute__((packed));

struct oplus_chg_wls_skewing_data {
	uint32_t curr_ma;
	int32_t fallback_step;
	int32_t switch_to_bpp;
} __attribute__((packed));

struct oplus_chg_wls_fcc_step {
	int max_step;
	struct oplus_chg_wls_range_data fcc_step[WLS_MAX_STEP_CHG_ENTRIES];
	bool allow_fallback[WLS_MAX_STEP_CHG_ENTRIES];
	unsigned long fcc_wait_timeout;
} __attribute__((packed));

struct oplus_chg_wls_fcc_steps {
        struct oplus_chg_wls_fcc_step fcc_step[WLS_FAST_TEMP_MAX];
} __attribute__((packed));

struct oplus_chg_wls_bcc_step {
	int max_step;
	struct oplus_chg_wls_bcc_data bcc_step[BCC_MAX_STEP_ENTRIES];
} __attribute__((packed));

struct oplus_chg_wls_bcc_steps {
	struct oplus_chg_wls_bcc_step bcc_step[WLS_BCC_TEMP_MAX];
} __attribute__((packed));

struct oplus_chg_wls_non_ffc_step {
	int max_step;
	struct oplus_chg_wls_range_data *non_ffc_step;
};

struct oplus_chg_wls_cv_step {
	int max_step;
	struct oplus_chg_wls_range_data *cv_step;
};

struct oplus_chg_wls_cool_down_step {
	int max_step;
	int *cool_down_step;
};

struct oplus_chg_wls_skewing_step {
	int max_step;
	struct oplus_chg_wls_skewing_data *skewing_step;
};

struct oplus_chg_wls_skin_step {
	int max_step;
	struct oplus_chg_wls_skin_range_data *skin_step;
};

struct wls_match_q_type {
	u8 id;
	u8 q_value;
};

struct oplus_chg_wls_static_config {
	bool fastchg_fod_enable;
	bool fastchg_12v_fod_enable;
	u8 fastchg_fod_parm[WLS_FOD_PARM_LENGTH];
	u8 fastchg_fod_parm_12v[WLS_FOD_PARM_LENGTH];
	struct wls_match_q_type fastchg_match_q[WLS_BASE_NUM_MAX];
};

struct oplus_chg_wls_dynamic_config {
	int32_t batt_vol_max_mv;
	int32_t iclmax_ma[OPLUS_WLS_CHG_BATT_CL_MAX][OPLUS_WLS_CHG_MODE_MAX][BATT_TEMP_MAX];
	struct oplus_chg_wls_range_data fcc_step[WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_fcc_steps fcc_steps[WLS_FAST_SOC_MAX];
	struct oplus_chg_wls_range_data non_ffc_step[OPLUS_WLS_CHG_MODE_MAX][WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_range_data cv_step[OPLUS_WLS_CHG_MODE_MAX][WLS_MAX_STEP_CHG_ENTRIES];
	int32_t cool_down_step[OPLUS_WLS_CHG_MODE_MAX][WLS_COOL_DOWN_LEVEL_MAX];
	struct oplus_chg_wls_skewing_data skewing_step[OPLUS_WLS_SKEWING_MAX][WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_skin_range_data epp_plus_skin_step[WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_skin_range_data epp_skin_step[WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_skin_range_data bpp_skin_step[WLS_MAX_STEP_CHG_ENTRIES];
	int32_t fastchg_curr_max_ma;
	struct oplus_chg_wls_skin_range_data epp_plus_led_on_skin_step[WLS_MAX_STEP_CHG_ENTRIES];
	struct oplus_chg_wls_skin_range_data epp_led_on_skin_step[WLS_MAX_STEP_CHG_ENTRIES];
	int32_t wls_fast_chg_call_on_curr_ma;
	int32_t wls_fast_chg_camera_on_curr_ma;
	int32_t bpp_vol_mv;
	int32_t epp_vol_mv;
	int32_t epp_plus_vol_mv;
	int32_t vooc_vol_mv;
	int32_t svooc_vol_mv;
	int32_t fastchg_max_soc;
	int32_t fastchg_max_temp;
	int32_t cool_down_12v_thr;
	int32_t verity_curr_max_ma;
	uint32_t bcc_stop_curr_0_to_30[WLS_BCC_STOP_CURR_NUM];
	uint32_t bcc_stop_curr_30_to_70[WLS_BCC_STOP_CURR_NUM];
	uint32_t bcc_stop_curr_70_to_90[WLS_BCC_STOP_CURR_NUM];
	int32_t wls_strategy_soc[WLS_SOC_NUM_MAX];
	struct oplus_chg_wls_fcc_step epp_plus_steps;
	struct oplus_chg_wls_fcc_step epp_steps;
} __attribute__((packed));

struct oplus_wls_chg_rx {
	struct oplus_chg_wls *wls_dev;
	struct device *dev;
	struct oplus_chg_ic_dev *rx_ic;
	struct delayed_work cep_check_work;
	struct completion cep_ok_ack;
	struct wakeup_source *update_fw_wake_lock;

	bool cep_is_ok;
	bool clean_source;

	int vol_set_mv;
};

struct oplus_wls_chg_fast {
	struct oplus_chg_wls *wls_dev;
	struct device *dev;
	struct oplus_chg_ic_dev *fast_ic;
};

struct oplus_wls_chg_normal {
	struct oplus_chg_wls *wls_dev;
	struct device *dev;
	struct oplus_chg_ic_dev *nor_ic;
	struct delayed_work icl_set_work;
	struct delayed_work fcc_set_work;

	struct mutex icl_lock;
	struct mutex fcc_lock;

	int icl_target_ma;
	int icl_step_ma;
	int icl_set_ma;
	int fcc_target_ma;
	int fcc_step_ma;
	int fcc_set_ma;

	bool clean_source;
};

struct oplus_chg_wls_status {
	u8 adapter_type;
	u8 adapter_id;
	int vendor_id;
	bool tx_extern_cmd_done;
	u32 product_id;
	u8 aes_key_num;
	bool verify_by_aes;
	u8 adapter_power;
	u8 charge_type;
	bool tx_product_id_done;
	enum oplus_chg_wls_rx_state current_rx_state;
	enum oplus_chg_wls_rx_state next_rx_state;
	enum oplus_chg_wls_rx_state target_rx_state;
	enum oplus_chg_wls_rx_state target_rx_state_backup;
	enum oplus_chg_wls_trx_state trx_state;
	enum oplus_chg_wls_type wls_type;

	int batt_temp_dynamic_thr[BATT_TEMP_MAX - 1];
	int ffc_temp_dynamic_thr[FFC_TEMP_REGION_MAX];
	enum oplus_temp_region comm_temp_region;
	enum oplus_chg_temp_region temp_region;
	enum oplus_ffc_temp_region ffc_temp_region;

	int state_sub_step;
	int iout_ma;
	int iout_ma_conunt;
	int vout_mv;
	int vrect_mv;
	int trx_curr_ma;
	int trx_vol_mv;
	int fastchg_target_curr_ma;
	int fastchg_target_vol_mv;
	int fastchg_curr_max_ma;
	int fastchg_ibat_max_ma;
	int fastchg_level;
	/*Record the initial temperature when switching to the next gear.*/
	int fastchg_level_init_temp;
	int tx_pwr_mw;
	int rx_pwr_mw;
	int ploos_mw;
	int epp_plus_skin_step;
	int epp_skin_step;
	int bpp_skin_step;
	int epp_plus_led_on_skin_step;
	int epp_led_on_skin_step;
	int pwr_max_mw;
	int quiet_mode_need;
	int adapter_info_cmd_count;
	int fastchg_retry_count;
	int fastchg_err_count;
	int non_fcc_level;
	int skewing_level;
	int fast_cep_check;
	int cool_down;
	bool trx_close_delay;
	int bcc_current;
	int wls_bcc_max_curr;
	int wls_bcc_min_curr;
	int wls_bcc_stop_curr;
	int bcc_curve_idx;
	int bcc_true_idx;
	int bcc_temp_range;

	unsigned long cep_ok_wait_timeout;
	unsigned long fastchg_retry_timer;
	unsigned long fastchg_err_timer;
	bool rx_online;
	bool rx_present;
	bool trx_online;
	bool trx_usb_present_once;
	int trx_transfer_start_time;
	int trx_transfer_end_time;
	bool trx_present;
	bool trx_rxac;
	bool trx_present_keep;
	bool is_op_trx;
	bool epp_working;
	bool epp_5w;
	bool quiet_mode;
	bool switch_quiet_mode;
	bool quiet_mode_init;
	bool cep_timeout_adjusted;
	bool upgrade_fw_pending;
	bool fw_upgrading;
	bool fastchg_started;
	bool fastchg_disable;
	bool fastchg_vol_set_ok;
	bool fastchg_curr_set_ok;
	bool fastchg_curr_need_dec;
	bool normal_chg_disabled;
	bool ffc_check;
	bool wait_cep_stable;
	bool fastchg_restart;
	bool ffc_done;
	bool online_keep;
	bool boot_online_keep; /*The driver loading phase of the shutdown charging is set to true*/
	bool led_on;
	bool rx_adc_test_enable;
	bool rx_adc_test_pass;
	bool fod_parm_for_fastchg;
	bool chg_done;
	bool chg_done_quiet_mode;

	bool aes_verity_done;
	bool verity_pass;
	bool verity_data_ok;
	bool aes_verity_data_ok;
	bool verity_started;
	bool verity_state_keep;
	int verity_count;
	int break_count;
	u8 trx_err;
	u8 encrypt_data[WLS_AUTH_RANDOM_LEN];
	u8 aes_encrypt_data[WLS_AUTH_AES_RANDOM_LEN];
	struct wls_auth_result verfity_data;
	unsigned long verity_wait_timeout;
	struct wls_third_part_auth_result aes_verfity_data;

	int epp_chg_level;
};

struct oplus_chg_wls {
	struct device *dev;
	wait_queue_head_t read_wq;
	struct miscdevice misc_dev;
	struct oplus_mms *wls_topic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *wired_topic;
	struct oplus_mms *comm_topic;
	struct oplus_mms *err_topic;
	struct mms_subscribe *wls_subs;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *comm_subs;
	struct battery_info batt_info;
	struct wls_mms_info mms_info;

	struct power_supply *wls_psy;
	struct power_supply *batt_psy;
#ifdef MAYBE_DELETE
	struct notifier_block wls_changed_nb;
	struct notifier_block wls_event_nb;
	struct notifier_block wls_mod_nb;
#endif
	struct workqueue_struct *wls_wq;
	struct delayed_work wls_rx_sm_work;
	struct delayed_work wls_trx_sm_work;
	struct delayed_work wls_connect_work;
	struct delayed_work wls_send_msg_work;
	struct delayed_work wls_upgrade_fw_work;
	struct delayed_work wls_data_update_work;
	struct delayed_work usb_int_work;
	struct delayed_work wls_trx_disconnect_work;
	struct delayed_work usb_connect_work;
	struct delayed_work wls_start_work;
	struct delayed_work wls_break_work;
	struct delayed_work fod_cal_work;
	struct delayed_work rx_restore_work;
	struct delayed_work fast_fault_check_work;
	struct delayed_work rx_iic_restore_work;
	struct delayed_work rx_restart_work;
	struct delayed_work rx_verity_restore_work;
	struct delayed_work online_keep_remove_work;
	struct delayed_work verity_state_remove_work;
	struct delayed_work wls_verity_work;
	struct delayed_work wls_get_third_part_verity_data_work;
	struct delayed_work wls_clear_trx_work;
	struct delayed_work wls_skewing_work;
	struct delayed_work wls_bcc_curr_update_work;
	struct delayed_work wls_vout_err_work;
	struct delayed_work wls_mms_init_work;
	struct delayed_work wls_otg_enable_work;
	struct work_struct wls_err_handler_work;
	struct work_struct wls_present_handler_work;
	struct work_struct wls_online_handler_work;
	struct work_struct wls_offline_handler_work;
	struct work_struct wls_event_changed_handler_work;
	struct work_struct temp_region_update_work;
	struct work_struct cool_down_update_work;
	struct wakeup_source *rx_wake_lock;
	struct wakeup_source *trx_wake_lock;
	struct mutex connect_lock;
	struct mutex read_lock;
	struct mutex cmd_data_lock;
	struct mutex send_msg_lock;
	struct mutex update_data_lock;

	struct votable *fcc_votable;
	struct votable *fastchg_disable_votable;
	struct votable *rx_disable_votable;
	struct votable *wrx_en_votable;
	struct votable *nor_icl_votable;
	struct votable *nor_fcc_votable;
	struct votable *nor_fv_votable;
	struct votable *nor_out_disable_votable;
	struct votable *nor_input_disable_votable;

	struct oplus_wls_chg_rx *wls_rx;
	struct oplus_wls_chg_fast *wls_fast;
	struct oplus_wls_chg_normal *wls_nor;

	struct oplus_chg_wls_status wls_status;
	struct oplus_chg_wls_static_config static_config;
	struct oplus_chg_wls_dynamic_config dynamic_config;
	struct oplus_chg_wls_bcc_step wls_bcc_step;
	struct oplus_chg_wls_bcc_steps bcc_steps[WLS_BCC_SOC_MAX];
	struct oplus_chg_wls_fcc_step wls_fcc_step;
	struct oplus_chg_wls_fcc_steps fcc_steps[WLS_FAST_SOC_MAX];
	struct oplus_chg_wls_fcc_steps fcc_third_part_steps[WLS_FAST_SOC_MAX];
	struct oplus_chg_wls_non_ffc_step non_ffc_step[OPLUS_WLS_CHG_MODE_MAX];
	struct oplus_chg_wls_cv_step cv_step[OPLUS_WLS_CHG_MODE_MAX];
	struct oplus_chg_wls_cool_down_step cool_down_step[OPLUS_WLS_CHG_MODE_MAX];
	struct oplus_chg_wls_skewing_step skewing_step[OPLUS_WLS_SKEWING_MAX];
	struct oplus_chg_wls_skin_step epp_plus_skin_step;
	struct oplus_chg_wls_skin_step epp_skin_step;
	struct oplus_chg_wls_skin_step bpp_skin_step;
	struct oplus_chg_wls_skin_step epp_plus_led_on_skin_step;
	struct oplus_chg_wls_skin_step epp_led_on_skin_step;

	struct oplus_chg_rx_msg rx_msg;
	struct completion msg_ack;
	struct completion nor_ic_ack;
	struct wls_dev_cmd cmd;

#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
	struct oplus_cfg debug_cfg;
#endif

	u8 *fw_buf;
	int fw_size;
	bool fw_upgrade_by_buf;

	int wrx_en_gpio;
	int ext_pwr_en_gpio;
	int usb_int_gpio;
	int usb_int_irq;
	struct pinctrl *pinctrl;
	struct pinctrl_state *wrx_en_active;
	struct pinctrl_state *wrx_en_sleep;
	struct pinctrl_state *ext_pwr_en_active;
	struct pinctrl_state *ext_pwr_en_sleep;

	bool support_fastchg;
	bool support_get_tx_pwr;
	bool support_epp_plus;
	bool support_tx_boost;
	bool support_wls_and_tx_boost;
	bool support_wls_chg_bcc;
	bool rx_wake_lock_on;
	bool trx_wake_lock_on;
	bool usb_present;
	bool charge_enable;
	bool batt_charge_enable;
	bool ftm_mode;
	bool debug_mode;
	bool factory_mode;
	bool fod_is_cal; /*fod is calibrated*/
	bool fod_cal_data_ok;
	bool msg_callback_ok;
	bool cmd_data_ok;

	struct oplus_chg_wls_fod_cal_data cal_data;
	enum oplus_chg_wls_force_type force_type;
	enum oplus_chg_wls_path_ctrl_type path_ctrl_type;

	int icl_max_ma[OPLUS_WLS_CHG_MODE_MAX];

	int32_t wls_power_mw;
	u32 wls_phone_id;
	unsigned int wls_bcc_fcc_to_icl_factor;

	bool mutual_cmd_data_ok;
	bool mutual_hidl_handle_cmd_ready;
	struct mutex mutual_read_lock;
	struct mutex mutual_cmd_data_lock;
	struct mutex mutual_cmd_ack_lock;
	struct oplus_chg_cmd mutual_cmd;
	struct completion mutual_cmd_ack;
	wait_queue_head_t mutual_read_wq;
};

struct oplus_chg_wls_state_handler {
	int (*enter_state)(struct oplus_chg_wls *wls_dev);
	int (*handle_state)(struct oplus_chg_wls *wls_dev);
	int (*exit_state)(struct oplus_chg_wls *wls_dev);
};

static struct wls_base_type wls_base_table[] = {
	{ 0x00, 30000 }, { 0x01, 40000 }, { 0x02, 50000 }, { 0x03, 50000 }, { 0x04, 50000 },
	{ 0x05, 50000 }, { 0x06, 50000 }, { 0x07, 50000 }, { 0x08, 50000 }, { 0x09, 50000 },
	{ 0x0a, 100000 }, { 0x0b, 100000 }, { 0x10, 100000 }, { 0x11, 100000 }, { 0x12, 100000 },
	{ 0x13, 100000 }, { 0x1f, 50000 },
};

static u8 oplus_trx_id_table[] = {
	0x02, 0x03, 0x04, 0x05, 0x06
};

/*static int oplus_chg_wls_pwr_table[] = {0, 12000, 12000, 35000, 50000};*/
static struct wls_pwr_table oplus_chg_wls_pwr_table[] = {/*(f2_id, r_power, t_power)*/
	{ 0x00, 12, 15 }, { 0x01, 12, 20 }, { 0x02, 12, 30 }, { 0x03, 35, 50 }, { 0x04, 45, 65 },
	{ 0x05, 50, 75 }, { 0x06, 60, 85 }, { 0x07, 65, 95 }, { 0x08, 75, 105 }, { 0x09, 80, 115 },
	{ 0x0A, 90, 125 }, { 0x0B, 20, 20 }, { 0x0C, 100, 140 }, { 0x0D, 115, 160 }, { 0x0E, 130, 180 },
	{ 0x0F, 145, 200 },
	{ 0x11, 35, 50 }, { 0x12, 35, 50 }, { 0x13, 12, 20 }, { 0x14, 45, 65 }, { 0x15, 12, 20 },
	{ 0x16, 12, 20 }, { 0x17, 12, 30 }, { 0x18, 12, 30 }, { 0x19, 12, 30 }, { 0x1A, 12, 33 },
	{ 0x1B, 12, 33 }, { 0x1C, 12, 44 }, { 0x1D, 12, 44 }, { 0x1E, 12, 44 },
	{ 0x21, 35, 50 }, { 0x22, 12, 44 }, { 0x23, 35, 50 }, { 0x24, 35, 55 }, { 0x25, 35, 55 },
	{ 0x26, 35, 55 }, { 0x27, 35, 55 }, { 0x28, 45, 65 }, { 0x29, 12, 30 }, { 0x2A, 45, 65 },
	{ 0x2B, 45, 66 }, { 0x2C, 45, 67 }, { 0x2D, 45, 67 }, { 0x2E, 45, 67 },
	{ 0x31, 35, 50 }, { 0x32, 90, 120 }, { 0x33, 35, 50 }, { 0x34, 12, 20 }, { 0x35, 45, 65 },
	{ 0x36, 45, 66 }, { 0x37, 50, 88 }, { 0x38, 50, 88 }, { 0x39, 50, 88 }, { 0x3A, 50, 88 },
	{ 0x3B, 75, 100 }, { 0x3C, 75, 100 }, { 0x3D, 75, 100 }, { 0x3E, 75, 100 },
	{ 0x41, 12, 30 }, { 0x42, 12, 30 }, { 0x43, 12, 30 }, { 0x44, 12, 30 }, { 0x45, 12, 30 },
	{ 0x46, 12, 30 }, { 0x47, 90, 120 }, { 0x48, 90, 120 }, { 0x49, 12, 33 }, { 0x4A, 12, 33 },
	{ 0x4B, 50, 80 }, { 0x4C, 50, 80 }, { 0x4D, 50, 80 }, { 0x4E, 50, 80 },
	{ 0x51, 90, 125 },
	{ 0x61, 12, 33 }, { 0x62, 35, 50 }, { 0x63, 45, 65 }, { 0x64, 45, 66 }, { 0x65, 50, 80 },
	{ 0x66, 45, 65 }, { 0x67, 90, 125 }, { 0x68, 90, 125 }, { 0x69, 75, 100 }, { 0x6A, 75, 100 },
	{ 0x6B, 90, 120 }, { 0x6C, 45, 67 }, { 0x6D, 45, 67 }, { 0x6E, 45, 65 },
	{ 0x7F, 30, 0 },
};


static struct wls_pwr_table oplus_chg_wls_tripartite_pwr_table[] = {
	{0x01, 20, 20}, {0x02, 30, 30}, {0x03, 40, 40},  {0x04, 50, 50},
};

static const char * const oplus_chg_wls_rx_state_text[] = {
	[OPLUS_CHG_WLS_RX_STATE_DEFAULT] = "default",
	[OPLUS_CHG_WLS_RX_STATE_BPP] = "bpp",
	[OPLUS_CHG_WLS_RX_STATE_EPP] = "epp",
	[OPLUS_CHG_WLS_RX_STATE_EPP_PLUS] = "epp-plus",
	[OPLUS_CHG_WLS_RX_STATE_FAST] = "fast",
	[OPLUS_CHG_WLS_RX_STATE_FFC] = "ffc",
	[OPLUS_CHG_WLS_RX_STATE_DONE] = "done",
	[OPLUS_CHG_WLS_RX_STATE_QUIET] = "quiet",
	[OPLUS_CHG_WLS_RX_STATE_STOP] = "stop",
	[OPLUS_CHG_WLS_RX_STATE_DEBUG] = "debug",
	[OPLUS_CHG_WLS_RX_STATE_FTM] = "ftm",
	[OPLUS_CHG_WLS_RX_STATE_ERROR] = "error",
};

static const char * const oplus_chg_wls_trx_state_text[] = {
	[OPLUS_CHG_WLS_TRX_STATE_DEFAULT] = "default",
	[OPLUS_CHG_WLS_TRX_STATE_READY] = "ready",
	[OPLUS_CHG_WLS_TRX_STATE_WAIT_PING] = "wait_ping",
	[OPLUS_CHG_WLS_TRX_STATE_TRANSFER] = "transfer",
	[OPLUS_CHG_WLS_TRX_STATE_OFF] = "off",
};

static const char * const wls_err_reason_text[] = {
	[WLS_ERR_NONE] = "none",
	[WLS_ERR_VERITY] = "verity",
	[WLS_ERR_VOUT_ABNORMAL] = "vout_abnormal",
	[WLS_ERR_OTHER] = "other",
};

static u8 oplus_chg_wls_disable_fod_parm[WLS_FOD_PARM_LENGTH] = {
	0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f,
	0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f,
};

static struct oplus_chg_wls_dynamic_config default_config = {
	.fcc_step = {
		{},
		{},
		{},
		{},
	},
};

const int oplus_strategy_soc_default_para[WLS_SOC_NUM_MAX] = {30, 70, 90};

const int32_t default_wls_epp_strategy[] = {
	0, 375, 900, 1100, 1,
	350, 430, 800, 1000, 1,
	370, 450, 600, 800, 1,
	400, 530, 400, 500, 1
};

const int32_t default_wls_epp_plus_strategy[] = {
	0, 375, 1200, 1500, 1,
	350, 430, 800, 1000, 1,
	370, 450, 600, 800, 1,
	400, 530, 400, 500, 1
};

static int oplus_chg_wls_track_upload_wls_err_info(struct oplus_chg_wls *wls_dev,
	enum wls_err_scene scene_type, enum wls_err_reason reason_type);
static int oplus_chg_wls_set_mutual_cmd(struct oplus_chg_wls *wls_dev, u32 cmd, u32 data_size, const void *data_buf);

__maybe_unused static bool is_nor_fv_votable_available(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev->nor_fv_votable)
		wls_dev->nor_fv_votable = find_votable("FV_MAX");
	return !!wls_dev->nor_fv_votable;
}

__maybe_unused static bool is_wls_psy_available(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev->wls_psy)
		wls_dev->wls_psy = power_supply_get_by_name("wireless");
	return !!wls_dev->wls_psy;
}

__maybe_unused static bool is_batt_psy_available(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev->batt_psy)
		wls_dev->batt_psy = power_supply_get_by_name("battery");
	return !!wls_dev->batt_psy;
}

__maybe_unused static bool is_err_topic_available(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev->err_topic)
		wls_dev->err_topic = oplus_mms_get_by_name("error");
	return !!wls_dev->err_topic;
}

__maybe_unused static bool is_rx_ic_available(struct oplus_wls_chg_rx *wls_rx)
{
	struct device_node *node = wls_rx->dev->of_node;

	if (wls_rx->rx_ic == NULL)
		wls_rx->rx_ic = of_get_oplus_chg_ic(node, "oplus,rx_ic", 0);
	return !!wls_rx->rx_ic;
}

__maybe_unused static bool oplus_chg_wls_is_usb_present(struct oplus_chg_wls *wls_dev)
{
	union mms_msg_data data = { 0 };

	if (!wls_dev || !wls_dev->wired_topic)
		return false;
	oplus_mms_get_item_data(wls_dev->wired_topic, WIRED_ITEM_PRESENT, &data, false);
	return !!data.intval;
}

__maybe_unused static bool oplus_chg_wls_is_usb_connected(struct oplus_chg_wls *wls_dev)
{
	if (!gpio_is_valid(wls_dev->usb_int_gpio)) {
		chg_err("usb_int_gpio invalid\n");
		return false;
	}

	return !!gpio_get_value(wls_dev->usb_int_gpio);
}

static int oplus_chg_wls_get_base_power_max(u8 id)
{
	int i;
	int pwr = WLS_VOOC_PWR_MAX_MW;

	for (i = 0; i < ARRAY_SIZE(wls_base_table); i++) {
		if (wls_base_table[i].id == id) {
			pwr = wls_base_table[i].power_max_mw;
			return pwr;
		}
	}

	return pwr;
}

static int oplus_chg_wls_get_r_power(struct oplus_chg_wls *wls_dev, u8 f2_data)
{
	int i = 0;
	int r_pwr = WLS_RECEIVE_POWER_DEFAULT;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	for (i = 0; i < ARRAY_SIZE(oplus_chg_wls_pwr_table); i++) {
		if (oplus_chg_wls_pwr_table[i].f2_id == (f2_data & 0x7F)) {
			r_pwr = oplus_chg_wls_pwr_table[i].r_power * 1000;
			break;
		}
	}
	if (wls_status->wls_type == OPLUS_CHG_WLS_PD_65W)
		return WLS_RECEIVE_POWER_PD65W;
	return r_pwr;
}

static int oplus_chg_wls_get_tripartite_r_power(u8 f2_data)
{
	int i = 0;
	int r_pwr = WLS_RECEIVE_POWER_DEFAULT;

	for (i = 0; i < ARRAY_SIZE(oplus_chg_wls_tripartite_pwr_table); i++) {
		if (oplus_chg_wls_tripartite_pwr_table[i].f2_id == (f2_data & 0x7F)) {
			r_pwr = oplus_chg_wls_tripartite_pwr_table[i].r_power * 1000;
			break;
		}
	}
	return r_pwr;
}

#define WLS_BASE_ID_DEFAUTL  0x02
static u8 oplus_chg_wls_get_q_value(struct oplus_chg_wls *wls_dev, u8 id)
{
	int i;

	for (i = 0; i < WLS_BASE_NUM_MAX; i++) {
		if (wls_dev->static_config.fastchg_match_q[i].id == id)
			return wls_dev->static_config.fastchg_match_q[i].q_value;
	}

	for (i = 0; i < WLS_BASE_NUM_MAX; i++) {
		if (wls_dev->static_config.fastchg_match_q[i].id == WLS_BASE_ID_DEFAUTL)
			return wls_dev->static_config.fastchg_match_q[i].q_value;
	}

	return 0;
}

static int oplus_chg_wls_get_ibat(struct oplus_chg_wls *wls_dev, int *ibat_ma)
{
	union mms_msg_data data = { 0 };

	if (!wls_dev || !wls_dev->gauge_topic) {
		chg_err("wls_dev or gauge_topic is null\n");
		return -ENODEV;
	}
	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_CURR, &data, true);
	*ibat_ma = data.intval;
	return 0;
}

static int oplus_chg_wls_get_vbat(struct oplus_chg_wls *wls_dev, int *vbat_mv)
{
	union mms_msg_data data = { 0 };

	if (!wls_dev || !wls_dev->gauge_topic) {
		chg_err("wls_dev or gauge_topic is null\n");
		return -ENODEV;
	}
	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, true);
	*vbat_mv = data.intval;
	return 0;
}

static int oplus_chg_wls_get_real_soc(struct oplus_chg_wls *wls_dev, int *soc)
{
	if (!wls_dev) {
		chg_err("wls_dev is null\n");
		return -ENODEV;
	}
	*soc = wls_dev->batt_info.soc;

	return 0;
}

static int oplus_chg_wls_get_ui_soc(struct oplus_chg_wls *wls_dev, int *ui_soc)
{
	if (!wls_dev) {
		chg_err("wls_dev is null\n");
		return -ENODEV;
	}
	*ui_soc = min(wls_dev->batt_info.ui_soc, 100);

	return 0;
}

static int oplus_chg_wls_get_batt_temp(struct oplus_chg_wls *wls_dev, int *temp)
{
	if (!wls_dev) {
		chg_err("wls_dev is null\n");
		return -ENODEV;
	}
	*temp = wls_dev->batt_info.batt_temp;

	return 0;
}

static int oplus_chg_wls_get_batt_num(struct oplus_chg_wls *wls_dev, int *num)
{
	*num = oplus_gauge_get_batt_num();
	return 0;
}

static int oplus_chg_wls_get_skin_temp(struct oplus_chg_wls *wls_dev, int *temp)
{
	*temp = 250;

	return 0;
}

static int oplus_chg_wls_set_wrx_enable(struct oplus_chg_wls *wls_dev, bool en)
{
	int rc;

	if (!gpio_is_valid(wls_dev->wrx_en_gpio)) {
		rc = oplus_chg_wls_nor_set_usb_drv(wls_dev->wls_nor->nor_ic, en);
		if (rc < 0)
			chg_err("can't %s wrx, rc=%d\n", en ? "enable" : "disable", rc);
		else
			chg_info("set wrx %s with usb drv\n", en ? "enable" : "disable");

		return rc;
	}

	if (IS_ERR_OR_NULL(wls_dev->pinctrl) ||
	    IS_ERR_OR_NULL(wls_dev->wrx_en_active) ||
	    IS_ERR_OR_NULL(wls_dev->wrx_en_sleep)) {
		chg_err("wrx_en pinctrl error\n");
		return -ENODEV;
	}

	rc = pinctrl_select_state(wls_dev->pinctrl,
		en ? wls_dev->wrx_en_active : wls_dev->wrx_en_sleep);
	if (rc < 0)
		chg_err("can't %s wrx, rc=%d\n", en ? "enable" : "disable", rc);
	else
		chg_info("set wrx %s\n", en ? "enable" : "disable");
	chg_info("wrx: set value:%d, gpio_val:%d\n", en, gpio_get_value(wls_dev->wrx_en_gpio));

	return rc;
}

/**********************************************************************
* wls rx funcs:
**********************************************************************/
static int oplus_chg_wls_rx_set_vout_ms(struct oplus_wls_chg_rx *wls_rx, int vol_mv, int wait_time_ms)
{
	int cep;
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	chg_info("set vout to %d mV\n", vol_mv);
	rc = oplus_chg_wls_rx_set_vout_raw(wls_rx->rx_ic, vol_mv);
	if (rc < 0) {
		chg_err("can't set vout to %dmV, rc=%d\n", vol_mv, rc);
		return rc;
	}
	wls_rx->vol_set_mv = vol_mv;
	wls_rx->cep_is_ok = false;
	(void)oplus_chg_wls_get_cep_check_update(wls_rx->rx_ic, &cep);
	if (wait_time_ms > 0) {
		reinit_completion(&wls_rx->cep_ok_ack);
		schedule_delayed_work(&wls_rx->cep_check_work, msecs_to_jiffies(100));
		rc = wait_for_completion_timeout(&wls_rx->cep_ok_ack, msecs_to_jiffies(wait_time_ms));
		if (!rc) {
			chg_err("wait cep timeout\n");
			cancel_delayed_work_sync(&wls_rx->cep_check_work);
			return -ETIMEDOUT;
		}
		if (wls_rx->clean_source)
			return -EINVAL;
	} else if (wait_time_ms < 0) {
		reinit_completion(&wls_rx->cep_ok_ack);
		schedule_delayed_work(&wls_rx->cep_check_work, msecs_to_jiffies(100));
		wait_for_completion(&wls_rx->cep_ok_ack);
		if (wls_rx->clean_source)
			return -EINVAL;
	}

	return 0;
}

static int oplus_chg_wls_rx_set_vout(struct oplus_wls_chg_rx *wls_rx, int vol_mv, int wait_time_s)
{
	unsigned long wait_time_ms;
	int rc;

	if (wait_time_s > 0)
		wait_time_ms = wait_time_s * 1000;
	else
		wait_time_ms = wait_time_s;

	rc = oplus_chg_wls_rx_set_vout_ms(wls_rx, vol_mv, wait_time_ms);
	if (rc < 0)
		return rc;

	return 0;
}

static int oplus_chg_wls_rx_set_vout_step(struct oplus_wls_chg_rx *wls_rx,
				int target_vol_mv, int step_mv, int wait_time_s)
{
	int next_step_mv;
	unsigned long remaining_time_ms;
	unsigned long stop_time;
	unsigned long start_time = jiffies;
	bool no_timeout;
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	if (wait_time_s == 0) {
		chg_err("waiting time must be greater than 0\n");
		return -EINVAL;
	} else if (wait_time_s > 0) {
		no_timeout = false;
		remaining_time_ms = wait_time_s * 1000;
		stop_time = msecs_to_jiffies(wait_time_s * 1000) + start_time;
	} else {
		no_timeout = true;
	}

next_step:
	if (target_vol_mv >= wls_rx->vol_set_mv) {
		next_step_mv = wls_rx->vol_set_mv + step_mv;
		if (next_step_mv > target_vol_mv)
			next_step_mv = target_vol_mv;
	} else {
		next_step_mv = wls_rx->vol_set_mv - step_mv;
		if (next_step_mv < target_vol_mv)
			next_step_mv = target_vol_mv;
	}
	rc = oplus_chg_wls_rx_set_vout_ms(wls_rx, next_step_mv, no_timeout ? -1 : remaining_time_ms);
	if (rc < 0) {
		chg_err("can't set vout to %dmV, rc=%d\n", next_step_mv, rc);
		return rc;
	}
	if (next_step_mv != target_vol_mv) {
		if (!no_timeout) {
			if (time_is_before_jiffies(stop_time)) {
				chg_err("can't set vout to %dmV\n", next_step_mv);
				return -ETIMEDOUT;
			}
			remaining_time_ms = remaining_time_ms - jiffies_to_msecs(jiffies - start_time);
			start_time = jiffies;
		}
		goto next_step;
	}

	return 0;
}

static void oplus_chg_wls_cep_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_wls_chg_rx *wls_rx = container_of(dwork, struct oplus_wls_chg_rx, cep_check_work);
	int rc;
	int cep;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return;
	}

	if (!wls_rx->wls_dev->wls_status.rx_online) {
		chg_info("rx is offline, exit\n");
		complete(&wls_rx->cep_ok_ack);
		return;
	}

	rc = oplus_chg_wls_get_cep_check_update(wls_rx->rx_ic, &cep);
	if (rc < 0)
		goto out;
	chg_info("wkcs: cep=%d\n", cep);
	if (abs(cep) < 3) {
		wls_rx->cep_is_ok = true;
		complete(&wls_rx->cep_ok_ack);
		return;
	}

out:
	schedule_delayed_work(&wls_rx->cep_check_work, msecs_to_jiffies(100));
}

static int oplus_chg_wls_rx_send_msg(struct oplus_wls_chg_rx *wls_rx, unsigned char msg, unsigned char data)
{
	unsigned char buf[4] = {msg, ~msg, data, ~data};
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	chg_info("send msg, msg=0x%02x, data=0x%02x\n", msg, data);
	rc = oplus_chg_wls_rx_send_msg_raw(wls_rx->rx_ic, buf, ARRAY_SIZE(buf));
	if (rc < 0)
		chg_err("can't send msg, rc=%d\n", rc);

	return rc;
}

static int oplus_chg_wls_rx_send_data(struct oplus_wls_chg_rx *wls_rx, unsigned char msg, unsigned char data[], int len)
{
	unsigned char *buf;
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	buf = devm_kzalloc(wls_rx->dev, len + 1, GFP_KERNEL);
	if (!buf) {
		chg_err("alloc data buf error\n");
		return -ENOMEM;
	}
	buf[0] = msg;
	memcpy(buf + 1, data, len);

	rc = oplus_chg_wls_rx_send_msg_raw(wls_rx->rx_ic, buf, len + 1);

	return rc;
}

static int oplus_chg_wls_rx_upgrade_firmware_by_buf(struct oplus_wls_chg_rx *wls_rx, unsigned char *buf, int len)
{
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	__pm_stay_awake(wls_rx->update_fw_wake_lock);
	wls_rx->wls_dev->wls_status.fw_upgrading = true;
	rc = oplus_chg_wls_rx_upgrade_firmware_by_buf_raw(wls_rx->rx_ic, buf, len);
	if (rc < 0)
		chg_err("can't upgrade firmware by buf, rc=%d\n", rc);
	wls_rx->wls_dev->wls_status.fw_upgrading = false;
	__pm_relax(wls_rx->update_fw_wake_lock);

	return rc;
}

static int oplus_chg_wls_rx_upgrade_firmware_by_img(struct oplus_wls_chg_rx *wls_rx)
{
	int rc;

	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	__pm_stay_awake(wls_rx->update_fw_wake_lock);
	wls_rx->wls_dev->wls_status.fw_upgrading = true;
	rc = oplus_chg_wls_rx_upgrade_firmware_by_img_raw(wls_rx->rx_ic);
	if (rc < 0)
		chg_err("can't upgrade firmware by img, rc=%d\n", rc);
	wls_rx->wls_dev->wls_status.fw_upgrading = false;
	__pm_relax(wls_rx->update_fw_wake_lock);

	return rc;
}

static int oplus_chg_wls_rx_clean_source(struct oplus_wls_chg_rx *wls_rx)
{
	if (wls_rx == NULL) {
		chg_err("wls_rx is NULL\n");
		return -ENODEV;
	}

	wls_rx->clean_source = true;
	cancel_delayed_work_sync(&wls_rx->cep_check_work);
	complete(&wls_rx->cep_ok_ack);
	wls_rx->cep_is_ok = true;
	wls_rx->vol_set_mv = 5000;
	wls_rx->clean_source = false;

	return 0;
}

static int oplus_chg_wls_rx_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_rx *wls_rx;

	wls_rx = devm_kzalloc(wls_dev->dev, sizeof(struct oplus_wls_chg_rx), GFP_KERNEL);
	if (wls_rx == NULL) {
		chg_err("alloc memory error\n");
		devm_kfree(wls_dev->dev, wls_rx);
		return -ENOMEM;
	}

	wls_dev->wls_rx = wls_rx;
	wls_rx->dev = wls_dev->dev;
	wls_rx->wls_dev = wls_dev;

	INIT_DELAYED_WORK(&wls_rx->cep_check_work, oplus_chg_wls_cep_check_work);
	init_completion(&wls_rx->cep_ok_ack);
	wls_rx->update_fw_wake_lock = wakeup_source_register(wls_rx->dev, "wls_update_lock");

	return 0;
}

static int oplus_chg_wls_rx_remove(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_rx *wls_rx = wls_dev->wls_rx;

	devm_kfree(wls_dev->dev, wls_rx);
	wls_dev->wls_rx = NULL;

	return 0;
}

/**********************************************************************
* wls nor funcs:
**********************************************************************/
static int oplus_chg_wls_nor_set_icl(struct oplus_wls_chg_normal *wls_nor, int icl_ma)
{
	int rc;

	if (wls_nor == NULL) {
		chg_err("wls_nor is NULL\n");
		return -ENODEV;
	}

	chg_info("set icl to %d ma\n", icl_ma);

	mutex_lock(&wls_nor->icl_lock);
	rc = oplus_chg_wls_nor_set_icl_raw(wls_nor->nor_ic, icl_ma);
	if (rc < 0) {
		chg_err("set icl to %d mA error, rc=%d\n", icl_ma, rc);
		mutex_unlock(&wls_nor->icl_lock);
		return rc;
	}

	wls_nor->icl_set_ma = icl_ma;
	mutex_unlock(&wls_nor->icl_lock);

	return 0;
}

static void oplus_chg_wls_nor_icl_set_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_wls_chg_normal *wls_nor = container_of(dwork, struct oplus_wls_chg_normal, icl_set_work);
	int icl_next_ma;
	int rc;

	if (!wls_nor->wls_dev->wls_status.rx_present)
		return;
	if (wls_nor->clean_source)
		return;

	mutex_lock(&wls_nor->icl_lock);
	if (wls_nor->icl_target_ma >= wls_nor->icl_set_ma) {
		icl_next_ma = wls_nor->icl_set_ma + wls_nor->icl_step_ma;
		if (icl_next_ma > wls_nor->icl_target_ma)
			icl_next_ma = wls_nor->icl_target_ma;
	} else {
		icl_next_ma = wls_nor->icl_set_ma - wls_nor->icl_step_ma;
		if (icl_next_ma < wls_nor->icl_target_ma)
			icl_next_ma = wls_nor->icl_target_ma;
	}
	mutex_unlock(&wls_nor->icl_lock);
	rc = oplus_chg_wls_nor_set_icl(wls_nor, icl_next_ma);
	if (rc < 0) {
		chg_err("set icl to %d mA error, rc=%d\n", icl_next_ma, rc);
		return;
	}
	if (icl_next_ma != wls_nor->icl_target_ma) {
		if (!wls_nor->wls_dev->wls_status.rx_present)
			return;
		schedule_delayed_work(&wls_nor->icl_set_work, msecs_to_jiffies(100));
		return;
	}
}

static int oplus_chg_wls_nor_set_icl_by_step(struct oplus_wls_chg_normal *wls_nor, int icl_ma, int step_ma, bool block)
{
	int icl_next_ma;
	int rc;

	if (wls_nor == NULL) {
		chg_err("wls_nor is NULL\n");
		return -ENODEV;
	}

	if (block) {
		mutex_lock(&wls_nor->icl_lock);
		wls_nor->icl_target_ma = icl_ma;
		wls_nor->icl_step_ma = step_ma;
		mutex_unlock(&wls_nor->icl_lock);
next_step:
		mutex_lock(&wls_nor->icl_lock);
		if (icl_ma >= wls_nor->icl_set_ma) {
			icl_next_ma = wls_nor->icl_set_ma + step_ma;
			if (icl_next_ma > icl_ma)
				icl_next_ma = icl_ma;
		} else {
			icl_next_ma = wls_nor->icl_set_ma - step_ma;
			if (icl_next_ma < icl_ma)
				icl_next_ma = icl_ma;
		}
		mutex_unlock(&wls_nor->icl_lock);
		rc = oplus_chg_wls_nor_set_icl(wls_nor, icl_next_ma);
		if (rc < 0) {
			chg_err("set icl to %d mA error, rc=%d\n", icl_next_ma, rc);
			return rc;
		}
		if (wls_nor->clean_source)
			return -EINVAL;
		if (icl_next_ma != icl_ma) {
			msleep(100);
			goto next_step;
		}
	} else {
		mutex_lock(&wls_nor->icl_lock);
		wls_nor->icl_target_ma = icl_ma;
		wls_nor->icl_step_ma = step_ma;
		mutex_unlock(&wls_nor->icl_lock);
		schedule_delayed_work(&wls_nor->icl_set_work, 0);
	}

	return 0;
}

static int oplus_chg_wls_nor_set_fcc(struct oplus_wls_chg_normal *wls_nor, int fcc_ma)
{
	int rc;

	if (wls_nor == NULL) {
		chg_err("wls_nor is NULL\n");
		return -ENODEV;
	}
	if (oplus_chg_wls_is_usb_present(wls_nor->wls_dev)) {
		chg_debug("usb present, exit\n");
		return -EPERM;
	}

	mutex_lock(&wls_nor->fcc_lock);
	rc = oplus_chg_wls_nor_set_fcc_raw(wls_nor->nor_ic, fcc_ma);
	if (rc < 0) {
		chg_err("set fcc to %d mA error, rc=%d\n", fcc_ma, rc);
		mutex_unlock(&wls_nor->fcc_lock);
		return rc;
	} else {
		chg_info("set fcc to %d mA\n", fcc_ma);
	}
	wls_nor->fcc_set_ma = fcc_ma;
	mutex_unlock(&wls_nor->fcc_lock);

	return 0;
}

static void oplus_chg_wls_nor_fcc_set_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_wls_chg_normal *wls_nor = container_of(dwork, struct oplus_wls_chg_normal, fcc_set_work);
	int fcc_next_ma;
	int rc;

	if (!wls_nor->wls_dev->wls_status.rx_present)
		return;
	if (wls_nor->clean_source)
		return;

	mutex_lock(&wls_nor->fcc_lock);
	if (wls_nor->fcc_target_ma >= wls_nor->fcc_set_ma) {
		fcc_next_ma = wls_nor->fcc_set_ma + wls_nor->fcc_step_ma;
		if (fcc_next_ma > wls_nor->fcc_target_ma)
			fcc_next_ma = wls_nor->fcc_target_ma;
	} else {
		fcc_next_ma = wls_nor->fcc_set_ma - wls_nor->fcc_step_ma;
		if (fcc_next_ma < wls_nor->fcc_target_ma)
			fcc_next_ma = wls_nor->fcc_target_ma;
	}
	mutex_unlock(&wls_nor->fcc_lock);
	rc = oplus_chg_wls_nor_set_fcc(wls_nor, fcc_next_ma);
	if (rc < 0) {
		chg_err("set fcc to %d mA error, rc=%d\n", fcc_next_ma, rc);
		return;
	}
	if (fcc_next_ma != wls_nor->fcc_target_ma) {
		if (!wls_nor->wls_dev->wls_status.rx_present)
			return;
		schedule_delayed_work(&wls_nor->fcc_set_work, msecs_to_jiffies(100));
		return;
	}
}

static int oplus_chg_wls_nor_set_fcc_by_step(struct oplus_wls_chg_normal *wls_nor, int fcc_ma, int step_ma, bool block)
{
	int fcc_next_ma;
	int rc;

	if (wls_nor == NULL) {
		chg_err("wls_nor is NULL\n");
		return -ENODEV;
	}

	if (block) {
		mutex_lock(&wls_nor->fcc_lock);
		wls_nor->fcc_target_ma = fcc_ma;
		wls_nor->fcc_step_ma = step_ma;
		mutex_unlock(&wls_nor->fcc_lock);
next_step:
		mutex_lock(&wls_nor->fcc_lock);
		if (fcc_ma >= wls_nor->fcc_set_ma) {
			fcc_next_ma = wls_nor->fcc_set_ma + step_ma;
			if (fcc_next_ma > fcc_ma)
				fcc_next_ma = fcc_ma;
		} else {
			fcc_next_ma = wls_nor->fcc_set_ma - step_ma;
			if (fcc_next_ma < fcc_ma)
				fcc_next_ma = fcc_ma;
		}
		mutex_unlock(&wls_nor->fcc_lock);
		rc = oplus_chg_wls_nor_set_fcc(wls_nor, fcc_next_ma);
		if (rc < 0) {
			chg_err("set fcc to %d mA error, rc=%d\n", fcc_next_ma, rc);
			return rc;
		}
		if (wls_nor->clean_source)
			return -EINVAL;
		if (fcc_next_ma != fcc_ma) {
			msleep(100);
			goto next_step;
		}
	} else {
		mutex_lock(&wls_nor->fcc_lock);
		wls_nor->fcc_target_ma = fcc_ma;
		wls_nor->fcc_step_ma = step_ma;
		mutex_unlock(&wls_nor->fcc_lock);
		schedule_delayed_work(&wls_nor->fcc_set_work, 0);
	}

	return 0;
}

static int oplus_chg_wls_nor_clean_source(struct oplus_wls_chg_normal *wls_nor)
{
	if (wls_nor == NULL) {
		chg_err("wls_nor is NULL\n");
		return -ENODEV;
	}

	wls_nor->clean_source = true;
	cancel_delayed_work_sync(&wls_nor->icl_set_work);
	cancel_delayed_work_sync(&wls_nor->fcc_set_work);
	mutex_lock(&wls_nor->icl_lock);
	wls_nor->icl_target_ma = 0;
	wls_nor->icl_step_ma = 0;
	wls_nor->icl_set_ma = 0;
	mutex_unlock(&wls_nor->icl_lock);
	mutex_lock(&wls_nor->fcc_lock);
	wls_nor->fcc_target_ma = 0;
	wls_nor->fcc_step_ma = 0;
	wls_nor->fcc_set_ma = 0;
	mutex_unlock(&wls_nor->fcc_lock);
	wls_nor->clean_source = false;

	return 0;
}

static int oplus_chg_wls_nor_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_normal *wls_nor;

	wls_nor = devm_kzalloc(wls_dev->dev, sizeof(struct oplus_wls_chg_normal), GFP_KERNEL);
	if (wls_nor == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	wls_dev->wls_nor = wls_nor;
	wls_nor->dev = wls_dev->dev;
	wls_nor->wls_dev = wls_dev;

	INIT_DELAYED_WORK(&wls_nor->icl_set_work, oplus_chg_wls_nor_icl_set_work);
	INIT_DELAYED_WORK(&wls_nor->fcc_set_work, oplus_chg_wls_nor_fcc_set_work);
	mutex_init(&wls_nor->icl_lock);
	mutex_init(&wls_nor->fcc_lock);

	return 0;
}

static int oplus_chg_wls_nor_remove(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_normal *wls_nor = wls_dev->wls_nor;

	devm_kfree(wls_dev->dev, wls_nor);
	wls_dev->wls_nor = NULL;

	return 0;
}

/**********************************************************************
* wls fast funcs:
**********************************************************************/
static int oplus_chg_wls_fast_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_fast *wls_fast;

	wls_fast = devm_kzalloc(wls_dev->dev, sizeof(struct oplus_wls_chg_fast), GFP_KERNEL);
	if (wls_fast == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	wls_dev->wls_fast = wls_fast;
	wls_fast->dev = wls_dev->dev;
	wls_fast->wls_dev = wls_dev;

	return 0;
}

static int oplus_chg_wls_fast_remove(struct oplus_chg_wls *wls_dev)
{
	struct oplus_wls_chg_fast *wls_fast = wls_dev->wls_fast;

	devm_kfree(wls_dev->dev, wls_fast);
	wls_dev->wls_fast = NULL;

	return 0;
}

static int oplus_chg_wls_fcc_vote_callback(struct votable *votable, void *data,
				int fcc_ma, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;

	if (fcc_ma < 0)
		return 0;

	if (fcc_ma > dynamic_cfg->fastchg_curr_max_ma)
		fcc_ma = dynamic_cfg->fastchg_curr_max_ma;

#ifdef WLS_QI_DEBUG
	if (wls_dbg_fcc_ma != 0)
		fcc_ma = wls_dbg_fcc_ma;
#endif

	wls_status->fastchg_target_curr_ma = fcc_ma;
	chg_info("set target current to %d by %s\n", wls_status->fastchg_target_curr_ma, client);

	return 0;
}

static int oplus_chg_wls_fastchg_disable_vote_callback(struct votable *votable, void *data,
				int disable, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	wls_status->fastchg_disable = disable;
	chg_info("%s wireless fast charge by %s\n", disable ? "disable" : "enable", client);

	return 0;
}

static int oplus_chg_wls_wrx_en_vote_callback(struct votable *votable, void *data,
				int en, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	rc = oplus_chg_wls_set_wrx_enable(wls_dev, en);
	chg_info("%s wireless wrx_en by %s\n", en ? "enable" : "disable", client);

	return rc;
}

static int oplus_chg_wls_nor_icl_vote_callback(struct votable *votable, void *data,
				int icl_ma, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	if (icl_ma < 0)
		return 0;

	if (step)
		rc = oplus_chg_wls_nor_set_icl_by_step(wls_dev->wls_nor, icl_ma, 200, false);
	else
		rc = oplus_chg_wls_nor_set_icl(wls_dev->wls_nor, icl_ma);

	return rc;
}

static int oplus_chg_wls_nor_fcc_vote_callback(struct votable *votable, void *data,
				int fcc_ma, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	if (fcc_ma < 0)
		return 0;

	if (step)
		rc = oplus_chg_wls_nor_set_fcc_by_step(wls_dev->wls_nor, fcc_ma, 100, false);
	else
		rc = oplus_chg_wls_nor_set_fcc(wls_dev->wls_nor, fcc_ma);

	return rc;
}

__maybe_unused static int oplus_chg_wls_nor_fv_vote_callback(struct votable *votable, void *data,
				int fv_mv, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	if (fv_mv < 0)
		return 0;

	rc = oplus_chg_wls_nor_set_fv(wls_dev->wls_nor->nor_ic, fv_mv);

	return rc;
}

static int oplus_chg_wls_nor_out_disable_vote_callback(struct votable *votable,
			void *data, int disable, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	rc = oplus_chg_wls_nor_set_output_enable(wls_dev->wls_nor->nor_ic, !disable);
	if (rc < 0)
		chg_err("can't %s wireless nor out charge\n", disable ? "disable" : "enable");
	else
		chg_info("%s wireless nor out charge by %s\n", disable ? "disable" : "enable", client);

	return rc;
}

static int oplus_chg_wls_nor_input_disable_vote_callback(struct votable *votable,
			void *data, int disable, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	rc = oplus_chg_wls_nor_set_input_enable(wls_dev->wls_nor->nor_ic, !disable);
	if (rc < 0)
		chg_err("can't %s wireless nor input charge\n", disable ? "disable" : "enable");
	else
		chg_info("%s wireless nor input charge\n", disable ? "disable" : "enable");

	return rc;
}

static int oplus_chg_wls_rx_disable_vote_callback(struct votable *votable, void *data,
				int disable, const char *client, bool step)
{
	struct oplus_chg_wls *wls_dev = data;
	int rc;

	rc = oplus_chg_wls_rx_enable(wls_dev->wls_rx->rx_ic, !disable);
	if (rc < 0)
		chg_err("can't %s wireless charge\n", disable ? "disable" : "enable");
	else
		chg_info("%s wireless charge\n", disable ? "disable" : "enable");

	return rc;
}

static int oplus_chg_wls_set_ext_pwr_enable(struct oplus_chg_wls *wls_dev, bool en)
{
	int rc;

	if (IS_ERR_OR_NULL(wls_dev->pinctrl) ||
	    IS_ERR_OR_NULL(wls_dev->ext_pwr_en_active) ||
	    IS_ERR_OR_NULL(wls_dev->ext_pwr_en_sleep)) {
		chg_err("ext_pwr_en pinctrl error\n");
		return -ENODEV;
	}

	rc = pinctrl_select_state(wls_dev->pinctrl,
		en ? wls_dev->ext_pwr_en_active : wls_dev->ext_pwr_en_sleep);
	if (rc < 0)
		chg_err("can't %s ext_pwr, rc=%d\n", en ? "enable" : "disable", rc);
	else
		chg_info("set ext_pwr %s\n", en ? "enable" : "disable");

	chg_info("ext_pwr: set value:%d, gpio_val:%d\n", en, gpio_get_value(wls_dev->ext_pwr_en_gpio));

	return rc;
}

static void oplus_chg_wls_get_third_part_verity_data_work(
				struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev =
		container_of(dwork, struct oplus_chg_wls, wls_get_third_part_verity_data_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	oplus_chg_wls_set_mutual_cmd(wls_dev, CMD_WLS_THIRD_PART_AUTH,
		sizeof(wls_status->vendor_id), &(wls_status->vendor_id));
}

static void oplus_chg_wls_standard_msg_handler(struct oplus_chg_wls *wls_dev,
					       u8 mask, u8 data)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	bool msg_ok = false;
	int pwr_mw;
	u8 q_value;

	chg_info("mask=0x%02x, msg_type=0x%02x, data=0x%02x\n", mask, rx_msg->msg_type, data);
	switch (mask) {
	case WLS_RESPONE_EXTERN_CMD:
		if (rx_msg->msg_type == WLS_CMD_GET_EXTERN_CMD) {
			chg_info("tx extern cmd=0x%02x\n", data);
			wls_status->tx_extern_cmd_done = true;
		}
		break;
	case WLS_RESPONE_VENDOR_ID:
		if (rx_msg->msg_type == WLS_CMD_GET_VENDOR_ID) {
			wls_status->vendor_id = data;
			wls_status->aes_verity_data_ok = false;
			if (!wls_status->verify_by_aes)
				schedule_delayed_work(&wls_dev->wls_get_third_part_verity_data_work, 0);
			wls_status->verify_by_aes = true;
			chg_info("verify_by_aes=%d, vendor_id=0x%02x\n",
				 wls_status->verify_by_aes, wls_status->vendor_id);
		}
		break;
	case WLS_RESPONE_ADAPTER_TYPE:
		if (rx_msg->msg_type == WLS_CMD_INDENTIFY_ADAPTER) {
			wls_status->adapter_type = data & WLS_ADAPTER_TYPE_MASK;
			wls_status->adapter_id = (data & WLS_ADAPTER_ID_MASK) >> 3;
			chg_info("wkcs: adapter_id = %d\n", wls_status->adapter_id);
			chg_info("wkcs: adapter_type = 0x%02x\n", wls_status->adapter_type);
			pwr_mw = oplus_chg_wls_get_base_power_max(wls_status->adapter_id);
			vote(wls_dev->fcc_votable, BASE_MAX_VOTER, true, (pwr_mw * 1000 / WLS_RX_VOL_MAX_MV), false);
			if ((wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC) ||
			    (wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC) ||
			    (wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W)) {
				q_value = oplus_chg_wls_get_q_value(wls_dev, wls_status->adapter_id);
				(void)oplus_chg_wls_rx_send_match_q(wls_dev->wls_rx->rx_ic, q_value);
				msleep(200);
			}
		}
		break;
	case WLS_RESPONE_INTO_FASTCHAGE:
		if (rx_msg->msg_type == WLS_CMD_INTO_FASTCHAGE) {
			wls_status->adapter_power = data;
			if (wls_status->adapter_id != WLS_ADAPTER_THIRD_PARTY)
				wls_status->pwr_max_mw = oplus_chg_wls_get_r_power(wls_dev, wls_status->adapter_power);
			else
				wls_status->pwr_max_mw = oplus_chg_wls_get_tripartite_r_power(wls_status->adapter_power);
			if (wls_status->pwr_max_mw > 0) {
				vote(wls_dev->fcc_votable, RX_MAX_VOTER, true,
				     (wls_status->pwr_max_mw * 1000 / WLS_RX_VOL_MAX_MV),
				     false);
			}
			wls_status->charge_type = WLS_CHARGE_TYPE_FAST;
			if (wls_dev->static_config.fastchg_fod_enable) {
				if (wls_status->fod_parm_for_fastchg)
					(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
						wls_dev->static_config.fastchg_fod_parm, WLS_FOD_PARM_LENGTH);
				else if (wls_dev->static_config.fastchg_12v_fod_enable)
					(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
						wls_dev->static_config.fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
			} else {
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
			}
			if (!wls_status->verity_started)
				vote(wls_dev->fcc_votable, VERITY_VOTER, true, dynamic_cfg->verity_curr_max_ma, false);
		}
		break;
	case WLS_RESPONE_INTO_NORMAL_MODE:
		if (rx_msg->msg_type == WLS_CMD_SET_NORMAL_MODE) {
			wls_status->quiet_mode = false;
			wls_status->quiet_mode_init = true;
		}
		break;
	case WLS_RESPONE_INTO_QUIET_MODE:
		if (rx_msg->msg_type == WLS_CMD_SET_QUIET_MODE) {
			wls_status->quiet_mode = true;
			wls_status->quiet_mode_init = true;
		}
		break;
	case WLS_RESPONE_FAN_SPEED:
		if (rx_msg->msg_type == WLS_CMD_SET_FAN_SPEED) {
			if (data <= FAN_PWM_PULSE_IN_SILENT_MODE_THR)
				wls_status->quiet_mode = true;
			else
				wls_status->quiet_mode = false;
			wls_status->quiet_mode_init = true;
		}
		break;
	case WLS_RESPONE_CEP_TIMEOUT:
		if (rx_msg->msg_type == WLS_CMD_SET_CEP_TIMEOUT) {
			wls_status->cep_timeout_adjusted = true;
		}
		break;
	case WLS_RESPONE_READY_FOR_EPP:
		wls_status->adapter_type = WLS_ADAPTER_TYPE_EPP;
		break;
	case WLS_RESPONE_WORKING_IN_EPP:
		wls_status->epp_working = true;
		break;
	default:
		break;
	}

	mutex_lock(&wls_dev->send_msg_lock);
	switch (mask) {
	case WLS_RESPONE_ADAPTER_TYPE:
		if (rx_msg->msg_type == WLS_CMD_INDENTIFY_ADAPTER)
			msg_ok = true;
		break;
	case WLS_RESPONE_INTO_FASTCHAGE:
		if (rx_msg->msg_type == WLS_CMD_INTO_FASTCHAGE)
			msg_ok = true;
		break;
	case WLS_RESPONE_INTO_NORMAL_MODE:
		if (rx_msg->msg_type == WLS_CMD_SET_NORMAL_MODE)
			msg_ok = true;
		break;
	case WLS_RESPONE_INTO_QUIET_MODE:
		if (rx_msg->msg_type == WLS_CMD_SET_QUIET_MODE)
			msg_ok = true;
		break;
	case WLS_RESPONE_CEP_TIMEOUT:
		if (rx_msg->msg_type == WLS_CMD_SET_CEP_TIMEOUT)
			msg_ok = true;
		break;
	case WLS_RESPONE_LED_BRIGHTNESS:
		if (rx_msg->msg_type == WLS_CMD_SET_LED_BRIGHTNESS)
			msg_ok = true;
		break;
	case WLS_RESPONE_FAN_SPEED:
		if (rx_msg->msg_type == WLS_CMD_SET_FAN_SPEED)
			msg_ok = true;
		break;
	case WLS_RESPONE_VENDOR_ID:
		if (rx_msg->msg_type == WLS_CMD_GET_VENDOR_ID)
			msg_ok = true;
		break;
	case WLS_RESPONE_EXTERN_CMD:
		if (rx_msg->msg_type == WLS_CMD_GET_EXTERN_CMD)
			msg_ok = true;
		break;
	default:
		chg_err("unknown msg respone(=%d)\n", mask);
	}
	mutex_unlock(&wls_dev->send_msg_lock);

	if (msg_ok) {
		cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
		complete(&wls_dev->msg_ack);
	}
}

static void oplus_chg_wls_data_msg_handler(struct oplus_chg_wls *wls_dev,
					   u8 mask, u8 data[3])
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	bool msg_ok = false;

	switch (mask) {
	case WLS_RESPONE_ENCRYPT_DATA4:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA4) {
			wls_status->encrypt_data[0] = data[0];
			wls_status->encrypt_data[1] = data[1];
			wls_status->encrypt_data[2] = data[2];
		}
		break;
	case WLS_RESPONE_ENCRYPT_DATA5:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA5) {
			wls_status->encrypt_data[3] = data[0];
			wls_status->encrypt_data[4] = data[1];
			wls_status->encrypt_data[5] = data[2];
		}
		break;
	case WLS_RESPONE_ENCRYPT_DATA6:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA6) {
			wls_status->encrypt_data[6] = data[0];
			wls_status->encrypt_data[7] = data[1];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA1:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA1) {
			wls_status->aes_encrypt_data[0] = data[0];
			wls_status->aes_encrypt_data[1] = data[1];
			wls_status->aes_encrypt_data[2] = data[2];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA2:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA2) {
			wls_status->aes_encrypt_data[3] = data[0];
			wls_status->aes_encrypt_data[4] = data[1];
			wls_status->aes_encrypt_data[5] = data[2];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA3:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA3) {
			wls_status->aes_encrypt_data[6] = data[0];
			wls_status->aes_encrypt_data[7] = data[1];
			wls_status->aes_encrypt_data[8] = data[2];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA4:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA4) {
			wls_status->aes_encrypt_data[9] = data[0];
			wls_status->aes_encrypt_data[10] = data[1];
			wls_status->aes_encrypt_data[11] = data[2];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA5:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA5) {
			wls_status->aes_encrypt_data[12] = data[0];
			wls_status->aes_encrypt_data[13] = data[1];
			wls_status->aes_encrypt_data[14] = data[2];
		}
		break;
	case WLS_RESPONE_GET_AES_DATA6:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA6) {
			wls_status->aes_encrypt_data[15] = data[0];
		}
		break;
	case WLS_RESPONE_PRODUCT_ID:
		if (rx_msg->msg_type == WLS_CMD_GET_PRODUCT_ID) {
			wls_status->tx_product_id_done = true;
			wls_status->product_id = (data[0] << 8) | data[1];
			chg_info("product_id:0x%x, tx_product_id_done:%d\n",
				wls_status->product_id, wls_status->tx_product_id_done);
		}
		break;
	case WLS_RESPONE_BATT_TEMP_SOC:
		if (rx_msg->msg_type == WLS_CMD_SEND_BATT_TEMP_SOC) {
			chg_info("temp:%d, soc:%d\n", (data[0] << 8) | data[1], data[2]);
		}
		break;
	default:
		break;
	}

	mutex_lock(&wls_dev->send_msg_lock);
	switch (mask) {
	case WLS_RESPONE_ENCRYPT_DATA1:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA1)
			msg_ok = true;
		break;
	case WLS_RESPONE_ENCRYPT_DATA2:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA2)
			msg_ok = true;
		break;
	case WLS_RESPONE_ENCRYPT_DATA3:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA3)
			msg_ok = true;
		break;
	case WLS_RESPONE_ENCRYPT_DATA4:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA4)
			msg_ok = true;
		break;
	case WLS_RESPONE_ENCRYPT_DATA5:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA5)
			msg_ok = true;
		break;
	case WLS_RESPONE_ENCRYPT_DATA6:
		if (rx_msg->msg_type == WLS_CMD_GET_ENCRYPT_DATA6)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA1:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA1)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA2:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA2)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA3:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA3)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA4:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA4)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA5:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA5)
			msg_ok = true;
		break;
	case WLS_RESPONE_SET_AES_DATA6:
		if (rx_msg->msg_type == WLS_CMD_SET_AES_DATA6)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA1:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA1)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA2:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA2)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA3:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA3)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA4:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA4)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA5:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA5)
			msg_ok = true;
		break;
	case WLS_RESPONE_GET_AES_DATA6:
		if (rx_msg->msg_type == WLS_CMD_GET_AES_DATA6)
			msg_ok = true;
		break;
	case WLS_RESPONE_PRODUCT_ID:
		if (rx_msg->msg_type == WLS_CMD_GET_PRODUCT_ID)
			msg_ok = true;
		break;
	case WLS_RESPONE_BATT_TEMP_SOC:
		if (rx_msg->msg_type == WLS_CMD_SEND_BATT_TEMP_SOC)
			msg_ok = true;
		break;
	default:
		chg_err("unknown msg respone(=%d)\n", mask);
	}
	mutex_unlock(&wls_dev->send_msg_lock);

	if (msg_ok) {
		cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
		complete(&wls_dev->msg_ack);
	}
}

static bool oplus_chg_wls_is_oplus_trx(u8 id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(oplus_trx_id_table); i++) {
		if (oplus_trx_id_table[i] == id)
			return true;
	}

	return false;
}

static void oplus_chg_wls_extended_msg_handler(struct oplus_chg_wls *wls_dev,
					       u8 data[])
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	bool msg_ok = false;

	chg_info("msg_type=0x%02x\n", data[0]);

	switch (data[0]) {
	case WLS_CMD_GET_TX_ID:
		if (rx_msg->msg_type != WLS_CMD_GET_TX_ID)
			break;
		if (data[4] == 0x04 && data[2] == 0x05 && oplus_chg_wls_is_oplus_trx(data[3])) {
			wls_status->is_op_trx = true;
			chg_info("is oplus trx\n");
		}
		msg_ok = true;
		break;
	case WLS_CMD_GET_TX_PWR:
		wls_status->tx_pwr_mw = (data[2] << 8) | data[1];
		wls_status->rx_pwr_mw = (data[4] << 8) | data[3];
		wls_status->ploos_mw = wls_status->tx_pwr_mw - wls_status->rx_pwr_mw;
		chg_info("tx_pwr=%d, rx_pwr=%d, ploos=%d\n", wls_status->tx_pwr_mw,
			wls_status->rx_pwr_mw, wls_status->ploos_mw);
		break;
	default:
		chg_err("unknown msg_type(=%d)\n", data[0]);
	}

	if (msg_ok) {
		cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
		complete(&wls_dev->msg_ack);
	}
}

static void oplus_chg_wls_rx_msg_callback(void *dev_data, u8 data[])
{
	struct oplus_chg_wls *wls_dev = dev_data;
	struct oplus_chg_wls_status *wls_status;
	struct oplus_chg_rx_msg *rx_msg;
	u8 temp[2];

	if (wls_dev == NULL) {
		chg_err("wls_dev is NULL\n");
		return;
	}

	wls_status = &wls_dev->wls_status;
	rx_msg = &wls_dev->rx_msg;

	temp[0] = ~data[2];
	temp[1] = ~data[4];
	switch (data[0]) {
	case WLS_MSG_TYPE_STANDARD_MSG:
		chg_info("received: standrad msg\n");
		if (!((data[1] >= WLS_RESPONE_ENCRYPT_DATA1 && data[1] <= WLS_RESPONE_ENCRYPT_DATA6) ||
		    (data[1] >= WLS_RESPONE_SET_AES_DATA1 && data[1] <= WLS_RESPONE_GET_AES_DATA6) ||
		     data[1] == WLS_RESPONE_PRODUCT_ID || data[1] == WLS_RESPONE_BATT_TEMP_SOC)) {
			if ((data[1] == temp[0]) && (data[3] == temp[1])) {
				chg_info("Received TX command: 0x%02X, data: 0x%02X\n",
					 data[1], data[3]);
				oplus_chg_wls_standard_msg_handler(wls_dev, data[1], data[3]);
			} else {
				chg_err("msg data error\n");
			}
		} else {
			chg_info("Received TX data: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", data[1], data[2], data[3], data[4]);
			oplus_chg_wls_data_msg_handler(wls_dev, data[1], &data[2]);
		}
		break;
	case WLS_MSG_TYPE_EXTENDED_MSG:
		oplus_chg_wls_extended_msg_handler(wls_dev, &data[1]);
		break;
	default:
		chg_err("Unknown msg type(=%d)\n", data[0]);
	}
}

static void oplus_chg_wls_send_msg_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev =
		container_of(dwork, struct oplus_chg_wls, wls_send_msg_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	int delay_ms = 1000;
	int cep;
	int rc;

	if (!wls_status->rx_online) {
		chg_err("wireless charge is not online\n");
		complete(&wls_dev->msg_ack);
		return;
	}

	if (rx_msg->msg_type == WLS_CMD_GET_TX_PWR || rx_msg->long_data) {
		/*need wait cep*/
		rc = oplus_chg_wls_get_cep(wls_dev->wls_rx->rx_ic, &cep);
		if (rc < 0) {
			chg_err("can't read cep, rc=%d\n", rc);
			complete(&wls_dev->msg_ack);
			return;
		}
		chg_info("wkcs: cep = %d\n", cep);
		if (abs(cep) > 3) {
			delay_ms = 3000;
			goto out;
		}
		chg_info("wkcs: get tx pwr\n");
	}
	if (rx_msg->long_data)
		rc = oplus_chg_wls_rx_send_data(wls_dev->wls_rx, rx_msg->msg_type,
			rx_msg->buf, ARRAY_SIZE(rx_msg->buf));
	else
		rc = oplus_chg_wls_rx_send_msg(wls_dev->wls_rx, rx_msg->msg_type, rx_msg->data);
	if (rc < 0) {
		chg_err("rx send msg error, rc=%d\n", rc);
		complete(&wls_dev->msg_ack);
		return;
	}

out:
	schedule_delayed_work(&wls_dev->wls_send_msg_work, msecs_to_jiffies(delay_ms));
}

static int oplus_chg_wls_send_msg(struct oplus_chg_wls *wls_dev, u8 msg, u8 data, int wait_time_s)
{
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	int cep;
	int rc;

	if (!wls_dev->msg_callback_ok) {
		rc = oplus_chg_wls_rx_register_msg_callback(wls_dev->wls_rx->rx_ic, wls_dev,
			oplus_chg_wls_rx_msg_callback);
		if (rc < 0) {
			chg_err("can't reg msg callback, rc=%d\n", rc);
			return rc;
		} else {
			wls_dev->msg_callback_ok = true;
		}
	}

	if (rx_msg->pending) {
		chg_err("msg pending\n");
		return -EAGAIN;
	}

	if (msg == WLS_CMD_GET_TX_PWR) {
		/*need wait cep*/
		rc = oplus_chg_wls_get_cep(wls_dev->wls_rx->rx_ic, &cep);
		if (rc < 0) {
			chg_err("can't read cep, rc=%d\n", rc);
			return rc;
		}
		if (abs(cep) > 3) {
			chg_info("wkcs: cep = %d\n", cep);
			return -EAGAIN;
		}
	}

	rc = oplus_chg_wls_rx_send_msg(wls_dev->wls_rx, msg, data);
	if (rc) {
		chg_err("send msg error, rc=%d\n", rc);
		return rc;
	}

	mutex_lock(&wls_dev->send_msg_lock);
	rx_msg->msg_type = msg;
	rx_msg->data = data;
	rx_msg->long_data = false;
	mutex_unlock(&wls_dev->send_msg_lock);
	if (wait_time_s > 0) {
		rx_msg->pending = true;
		reinit_completion(&wls_dev->msg_ack);
		schedule_delayed_work(&wls_dev->wls_send_msg_work, msecs_to_jiffies(1000));
		rc = wait_for_completion_timeout(&wls_dev->msg_ack, msecs_to_jiffies(wait_time_s * 1000));
		if (!rc) {
			chg_err("Error, timed out sending message\n");
			cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
			rc = -ETIMEDOUT;
		}
		rx_msg->msg_type = 0;
		rx_msg->data = 0;
		rx_msg->buf[0] = 0;
		rx_msg->buf[1] = 0;
		rx_msg->buf[2] = 0;
		rx_msg->respone_type = 0;
		rx_msg->long_data = false;
		rx_msg->pending = false;
	} else if (wait_time_s < 0) {
		rx_msg->pending = false;
		schedule_delayed_work(&wls_dev->wls_send_msg_work, msecs_to_jiffies(1000));
	}

	return rc;
}

static int oplus_chg_wls_send_data(struct oplus_chg_wls *wls_dev, u8 msg, u8 data[3], int wait_time_s)
{
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;
	int cep;
	int rc;

	if (!wls_dev->msg_callback_ok) {
		rc = oplus_chg_wls_rx_register_msg_callback(wls_dev->wls_rx->rx_ic, wls_dev,
			oplus_chg_wls_rx_msg_callback);
		if (rc < 0) {
			chg_err("can't reg msg callback, rc=%d\n", rc);
			return rc;
		} else {
			wls_dev->msg_callback_ok = true;
		}
	}

	if (rx_msg->pending) {
		chg_err("msg pending\n");
		return -EAGAIN;
	}

	/*need wait cep*/
	rc = oplus_chg_wls_get_cep(wls_dev->wls_rx->rx_ic, &cep);
	if (rc < 0) {
		chg_err("can't read cep, rc=%d\n", rc);
		return rc;
	}
	if (abs(cep) > 3) {
		chg_info("wkcs: cep = %d\n", cep);
		return -EAGAIN;
	}

	rc = oplus_chg_wls_rx_send_data(wls_dev->wls_rx, msg, data, 3);
	if (rc) {
		chg_err("send data error, rc=%d\n", rc);
		return rc;
	}

	mutex_lock(&wls_dev->send_msg_lock);
	rx_msg->msg_type = msg;
	rx_msg->buf[0] = data[0];
	rx_msg->buf[1] = data[1];
	rx_msg->buf[2] = data[2];
	rx_msg->long_data = true;
	mutex_unlock(&wls_dev->send_msg_lock);
	if (wait_time_s > 0) {
		rx_msg->pending = true;
		reinit_completion(&wls_dev->msg_ack);
		schedule_delayed_work(&wls_dev->wls_send_msg_work, msecs_to_jiffies(1000));
		rc = wait_for_completion_timeout(&wls_dev->msg_ack, msecs_to_jiffies(wait_time_s * 1000));
		if (!rc) {
			chg_err("Error, timed out sending message\n");
			cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
			rc = -ETIMEDOUT;
		}
		rx_msg->msg_type = 0;
		rx_msg->data = 0;
		rx_msg->buf[0] = 0;
		rx_msg->buf[1] = 0;
		rx_msg->buf[2] = 0;
		rx_msg->respone_type = 0;
		rx_msg->long_data = false;
		rx_msg->pending = false;
	} else if (wait_time_s < 0) {
		rx_msg->pending = false;
		schedule_delayed_work(&wls_dev->wls_send_msg_work, msecs_to_jiffies(1000));
	}

	return rc;
}

static void oplus_chg_wls_clean_msg(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_rx_msg *rx_msg = &wls_dev->rx_msg;

	cancel_delayed_work_sync(&wls_dev->wls_send_msg_work);
	if (rx_msg->pending)
		complete(&wls_dev->msg_ack);
	rx_msg->msg_type = 0;
	rx_msg->data = 0;
	rx_msg->buf[0] = 0;
	rx_msg->buf[1] = 0;
	rx_msg->buf[2] = 0;
	rx_msg->respone_type = 0;
	rx_msg->long_data = false;
	rx_msg->pending = false;
}

static int oplus_chg_wls_get_verity_data(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	mutex_lock(&wls_dev->cmd_data_lock);
	memset(&wls_dev->cmd, 0, sizeof(struct wls_dev_cmd));
	wls_dev->cmd.cmd = WLS_DEV_CMD_WLS_AUTH;
	wls_dev->cmd.data_size = 0;
	wls_dev->cmd_data_ok = true;
	wls_status->verity_data_ok = false;
	mutex_unlock(&wls_dev->cmd_data_lock);
	wake_up(&wls_dev->read_wq);

	return 0;
}

#define VERITY_TIMEOUT_MAX	40
static void oplus_chg_wls_des_verity(struct oplus_chg_wls *wls_dev)
{
	u8 buf[3];
	int rc;
	struct oplus_chg_wls_status *wls_status;

	if (!wls_dev)
		return;

	wls_status = &wls_dev->wls_status;
	if (!wls_status->rx_online)
		return;

	wls_status->verity_started = true;
#ifndef OPLUS_CHG_WLS_AUTH_ENABLE
	wls_status->verity_pass = true;
	goto done;
#endif

	wls_status->verity_wait_timeout = jiffies + VERITY_TIMEOUT_MAX * HZ;
retry:
	if (!wls_status->verity_data_ok) {
		chg_err("verity data not ready\n");
		if (wls_status->verity_count < 5) {
			wls_status->verity_count++;
			oplus_chg_wls_get_verity_data(wls_dev);
			schedule_delayed_work(&wls_dev->wls_verity_work,
					      msecs_to_jiffies(500));
			return;
		}
		wls_status->verity_pass = false;
		goto done;
	}

	buf[0] = wls_status->verfity_data.random_num[0];
	buf[1] = wls_status->verfity_data.random_num[1];
	buf[2] = wls_status->verfity_data.random_num[2];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_GET_ENCRYPT_DATA1, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA1, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	buf[0] = wls_status->verfity_data.random_num[3];
	buf[1] = wls_status->verfity_data.random_num[4];
	buf[2] = wls_status->verfity_data.random_num[5];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_GET_ENCRYPT_DATA2, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA2, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	buf[0] = wls_status->verfity_data.random_num[6];
	buf[1] = wls_status->verfity_data.random_num[7];
	buf[2] = WLS_ENCODE_MASK;
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_GET_ENCRYPT_DATA3, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA3, rc);
		wls_status->verity_pass = false;
		goto done;
	}

	/* Wait for the base encryption to complete */
	msleep(500);

	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_ENCRYPT_DATA4, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA4, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_ENCRYPT_DATA5, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA5, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_ENCRYPT_DATA6, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_ENCRYPT_DATA6, rc);
		wls_status->verity_pass = false;
		goto done;
	}

	chg_info("encrypt_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
		 wls_status->encrypt_data[0], wls_status->encrypt_data[1],
		 wls_status->encrypt_data[2], wls_status->encrypt_data[3],
		 wls_status->encrypt_data[4], wls_status->encrypt_data[5],
		 wls_status->encrypt_data[6], wls_status->encrypt_data[7]);

	if (memcmp(&wls_status->encrypt_data,
		   &wls_status->verfity_data.encode_num, WLS_AUTH_ENCODE_LEN)) {
		chg_err("verity faile\n");
		wls_status->verity_pass = false;
	} else {
		chg_info("verity pass\n");
		wls_status->verity_pass = true;
	}

done:
	wls_status->verity_count = 0;
	if (!wls_status->verity_pass && wls_status->rx_online &&
	    time_before(jiffies, wls_status->verity_wait_timeout))
		goto retry;
	vote(wls_dev->fcc_votable, VERITY_VOTER, false, 0, false);
	if (!wls_status->verity_pass && wls_status->rx_online) {
		wls_status->online_keep = true;
		wls_status->verity_state_keep = true;
		vote(wls_dev->rx_disable_votable, VERITY_VOTER, true, 1, false);
		schedule_delayed_work(&wls_dev->rx_verity_restore_work, msecs_to_jiffies(500));
		oplus_chg_wls_track_upload_wls_err_info(wls_dev, WLS_ERR_SCENE_RX, WLS_ERR_VERITY);
	}
}

static void oplus_chg_wls_aes_verity(struct oplus_chg_wls *wls_dev)
{
	u8 buf[3];
	int rc;
	int ret;
	struct oplus_chg_wls_status *wls_status;

	if (!wls_dev)
		return;

	wls_status = &wls_dev->wls_status;
	if (!wls_status->rx_online)
		return;

	wls_status->verity_started = true;
#ifndef OPLUS_CHG_WLS_AUTH_ENABLE
	wls_status->verity_pass = true;
	goto done;
#endif

	wls_status->verity_wait_timeout = jiffies + VERITY_TIMEOUT_MAX * HZ;
retry:
	if (!wls_status->aes_verity_data_ok) {
		chg_err("verity data not ready\n");
		ret = oplus_chg_wls_set_mutual_cmd(wls_dev, CMD_WLS_THIRD_PART_AUTH,
			sizeof(wls_status->vendor_id), &(wls_status->vendor_id));
		if (ret == CMD_ERROR_HIDL_NOT_READY) {
			schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(1000));
			return;
		} else if (ret != CMD_ACK_OK) {
			if (wls_status->verity_count < 5) {
				wls_status->verity_count++;
				schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(1500));
				return;
			}
			wls_status->verity_pass = false;
			goto done;
		}
	}

	buf[0] = wls_status->aes_verfity_data.aes_random_num[0];
	buf[1] = wls_status->aes_verfity_data.aes_random_num[1];
	buf[2] = wls_status->aes_verfity_data.aes_random_num[2];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA1, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA1, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	buf[0] = wls_status->aes_verfity_data.aes_random_num[3];
	buf[1] = wls_status->aes_verfity_data.aes_random_num[4];
	buf[2] = wls_status->aes_verfity_data.aes_random_num[5];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA2, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA2, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	buf[0] = wls_status->aes_verfity_data.aes_random_num[6];
	buf[1] = wls_status->aes_verfity_data.aes_random_num[7];
	buf[2] = wls_status->aes_verfity_data.aes_random_num[8];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA3, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA3, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	buf[0] = wls_status->aes_verfity_data.aes_random_num[9];
	buf[1] = wls_status->aes_verfity_data.aes_random_num[10];
	buf[2] = wls_status->aes_verfity_data.aes_random_num[11];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA4, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA4, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	buf[0] = wls_status->aes_verfity_data.aes_random_num[12];
	buf[1] = wls_status->aes_verfity_data.aes_random_num[13];
	buf[2] = wls_status->aes_verfity_data.aes_random_num[14];
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA5, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA5, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	buf[0] = wls_status->aes_verfity_data.aes_random_num[15];
	buf[1] = wls_status->vendor_id;
	buf[2] = wls_status->aes_key_num;
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_SET_AES_DATA6, buf, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_SET_AES_DATA6, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	/* Wait for the base encryption to complete */
	msleep(500);

	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA1, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA1, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA2, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA2, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA3, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA3, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA4, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA4, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA5, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA5, rc);
		wls_status->verity_pass = false;
		goto done;
	}
	msleep(500);
	wls_status->verity_count = 0;
	do {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_AES_DATA6, 0xff, 5);
		if (rc < 0) {
			if (rc != -EAGAIN)
				wls_status->verity_count++;
			msleep(500);
		}
	} while (rc < 0 && wls_status->verity_count < 5 && wls_status->rx_online);
	if (rc < 0 || !wls_status->rx_online) {
		chg_err("send 0x%02x error, rc=%d\n", WLS_CMD_GET_AES_DATA6, rc);
		wls_status->verity_pass = false;
		goto done;
	}

	chg_info("aes encrypt_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
		 "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
		 wls_status->aes_encrypt_data[0], wls_status->aes_encrypt_data[1],
		 wls_status->aes_encrypt_data[2], wls_status->aes_encrypt_data[3],
		 wls_status->aes_encrypt_data[4], wls_status->aes_encrypt_data[5],
		 wls_status->aes_encrypt_data[6], wls_status->aes_encrypt_data[7],
		 wls_status->aes_encrypt_data[8], wls_status->aes_encrypt_data[9],
		 wls_status->aes_encrypt_data[10], wls_status->aes_encrypt_data[11],
		 wls_status->aes_encrypt_data[12], wls_status->aes_encrypt_data[13],
		 wls_status->aes_encrypt_data[14], wls_status->aes_encrypt_data[15]);

	if (memcmp(&wls_status->aes_encrypt_data,
		   &wls_status->aes_verfity_data.aes_encode_num, WLS_AUTH_AES_ENCODE_LEN)) {
		chg_err("verity faile\n");
		wls_status->verity_pass = false;
	} else {
		chg_info("verity pass\n");
		wls_status->verity_pass = true;
	}

done:
	wls_status->verity_count = 0;
	if (!wls_status->verity_pass && wls_status->rx_online &&
	    time_before(jiffies, wls_status->verity_wait_timeout))
		goto retry;
	wls_status->aes_verity_done = true;
	vote(wls_dev->fcc_votable, VERITY_VOTER, false, 0, false);
	if (!wls_status->verity_pass && wls_status->rx_online) {
		wls_status->online_keep = true;
		wls_status->verity_state_keep = true;
		vote(wls_dev->rx_disable_votable, VERITY_VOTER, true, 1, false);
		schedule_delayed_work(&wls_dev->rx_verity_restore_work, msecs_to_jiffies(500));
		oplus_chg_wls_track_upload_wls_err_info(wls_dev, WLS_ERR_SCENE_RX, WLS_ERR_VERITY);
	}
}

static void oplus_chg_wls_verity_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev =
		container_of(dwork, struct oplus_chg_wls, wls_verity_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (!wls_status->verify_by_aes)
		oplus_chg_wls_des_verity(wls_dev);
	else
		oplus_chg_wls_aes_verity(wls_dev);
}

static void oplus_chg_wls_exchange_batt_mesg(struct oplus_chg_wls *wls_dev)
{
	int soc = 0, temp = 0;
	u8 buf[3];
	struct oplus_chg_wls_status *wls_status;
	int rc;

	if (!wls_dev)
		return;

	wls_status = &wls_dev->wls_status;
	if (!wls_status->rx_online || !wls_status->aes_verity_done ||
	    wls_status->adapter_id != WLS_ADAPTER_THIRD_PARTY)
		return;

	rc = oplus_chg_wls_get_ui_soc(wls_dev, &soc);
	if (rc < 0) {
		chg_err("can't get ui soc, rc=%d\n", rc);
		return;
	}
	rc = oplus_chg_wls_get_batt_temp(wls_dev, &temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return;
	}

	buf[0] = (temp >> 8) & 0xff;
	buf[1] = temp & 0xff;
	buf[2] = soc & 0xff;
	chg_info("soc:%d, temp:%d\n", soc, temp);

	mutex_lock(&wls_dev->update_data_lock);
	oplus_chg_wls_send_data(wls_dev, WLS_CMD_SEND_BATT_TEMP_SOC, buf, 0);
	msleep(100);
	mutex_unlock(&wls_dev->update_data_lock);
}

static void oplus_chg_wls_reset_variables(struct oplus_chg_wls *wls_dev) {
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;

	wls_status->adapter_type = WLS_ADAPTER_TYPE_UNKNOWN;
	wls_status->adapter_id = 0;
	wls_status->adapter_power = 0;
	wls_status->charge_type = WLS_CHARGE_TYPE_DEFAULT;
	wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
	wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
	wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
	wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_DEFAULT;
	wls_status->wls_type = OPLUS_CHG_WLS_UNKNOWN;
	wls_status->temp_region = BATT_TEMP_COLD;

	wls_status->is_op_trx = false;
	wls_status->epp_working = false;
	wls_status->epp_5w = false;
	wls_status->quiet_mode = false;
	wls_status->switch_quiet_mode = false;
	wls_status->quiet_mode_init = false;
	wls_status->cep_timeout_adjusted = false;
	wls_status->upgrade_fw_pending = false;
	wls_status->fw_upgrading = false;
	wls_status->trx_present = false;
	wls_status->trx_online = false;
	wls_status->trx_transfer_start_time = 0;
	wls_status->trx_transfer_end_time = 0;
	wls_status->trx_usb_present_once = false;
	wls_status->fastchg_started = false;
	wls_status->fastchg_disable = false;
	wls_status->fastchg_vol_set_ok = false;
	wls_status->fastchg_curr_set_ok = false;
	wls_status->fastchg_curr_need_dec = false;
	wls_status->ffc_check = false;
	wls_status->wait_cep_stable = false;
	wls_status->fastchg_restart = false;
	wls_status->rx_adc_test_enable = false;
	wls_status->rx_adc_test_pass = false;
	wls_status->boot_online_keep = false;
	wls_status->chg_done = false;
	wls_status->chg_done_quiet_mode = false;

	wls_status->state_sub_step = 0;
	wls_status->iout_ma = 0;
	wls_status->iout_ma_conunt = 0;
	wls_status->vout_mv = 0;
	wls_status->vrect_mv = 0;
	wls_status->trx_curr_ma = 0;
	wls_status->trx_vol_mv = 0;
	wls_status->fastchg_target_vol_mv = 0;
	wls_status->fastchg_ibat_max_ma = 0;
	wls_status->tx_pwr_mw = 0;
	wls_status->rx_pwr_mw = 0;
	wls_status->ploos_mw = 0;
	wls_status->epp_plus_skin_step = 0;
	wls_status->epp_skin_step = 0;
	wls_status->bpp_skin_step = 0;
	wls_status->epp_plus_led_on_skin_step = 0;
	wls_status->epp_led_on_skin_step = 0;
	wls_status->pwr_max_mw = 0;
	wls_status->quiet_mode_need = WLS_QUIET_MODE_UNKOWN;
	wls_status->adapter_info_cmd_count = 0;
	wls_status->fastchg_retry_count = 0;
	wls_status->fastchg_err_count = 0;
	wls_status->non_fcc_level = 0;
	wls_status->skewing_level = 0;
	wls_status->fast_cep_check = 0;
	wls_status->cool_down = 0;

	wls_status->cep_ok_wait_timeout = jiffies;
	wls_status->fastchg_retry_timer = jiffies;
	wls_status->fastchg_err_timer = jiffies;
	wls_dev->batt_charge_enable = true;

	wls_status->epp_chg_level = -1;
	dynamic_cfg->epp_steps.fcc_wait_timeout = jiffies;

	/*wls encrypt*/
	if (!wls_status->verity_state_keep) {
		/*
		 * After the AP is actively disconnected, the verification
		 * status flag does not need to change.
		 */
		wls_status->verity_pass = true;
	}
	wls_status->verity_started = false;
	wls_status->verity_data_ok = false;
	wls_status->verity_count = 0;
	wls_status->aes_verity_done = false;
	wls_status->verify_by_aes = false;
	wls_status->tx_extern_cmd_done = false;
	wls_status->tx_product_id_done = false;
	wls_dev->mms_info.event_code = WLS_EVENT_RX_UNKNOWN;
	memset(&wls_status->encrypt_data, 0, ARRAY_SIZE(wls_status->encrypt_data));
	memset(&wls_status->verfity_data, 0, sizeof(struct wls_auth_result));

	vote(wls_dev->nor_icl_votable, USER_VOTER, true, wls_status->rx_present ? 0 : 100, false);
	vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 0, false);
	vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, MAX_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, SKIN_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, RX_MAX_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, UOVP_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, CHG_DONE_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, NON_FFC_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, CV_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, COOL_DOWN_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, CEP_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, BCC_CURRENT_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, EPP_CURVE_VOTER, false, 0, false);
	vote(wls_dev->nor_fcc_votable, MAX_VOTER, false, 0, false);
	if (is_nor_fv_votable_available(wls_dev))
		vote(wls_dev->nor_fv_votable, USER_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, MAX_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, DEF_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, STEP_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, FCC_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, JEITA_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, CEP_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, SKIN_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, DEBUG_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, BASE_MAX_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, RX_MAX_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, COOL_DOWN_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, VERITY_VOTER, false, 0, false);
	vote(wls_dev->fcc_votable, BCC_CURRENT_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, QUIET_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, CEP_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, FCC_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, SKIN_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, BATT_VOL_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, BATT_CURR_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, RX_FAST_ERR_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, IOUT_CURR_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, STARTUP_CEP_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, HW_ERR_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, CURR_ERR_VOTER, false, 0, false);
	vote(wls_dev->fastchg_disable_votable, COOL_DOWN_VOTER, false, 0, false);
	vote(wls_dev->nor_out_disable_votable, FFC_VOTER, false, 0, false);
	vote(wls_dev->nor_out_disable_votable, USER_VOTER, false, 0, false);
	vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
	vote(wls_dev->nor_out_disable_votable, UOVP_VOTER, false, 0, false);
	vote(wls_dev->nor_input_disable_votable, UOVP_VOTER, false, 0, false);
	vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
	rerun_election(wls_dev->nor_icl_votable, false);
	rerun_election(wls_dev->nor_fcc_votable, false);
	if (is_nor_fv_votable_available(wls_dev))
		rerun_election(wls_dev->nor_fv_votable, false);
	rerun_election(wls_dev->fcc_votable, false);
	rerun_election(wls_dev->fastchg_disable_votable, false);
	rerun_election(wls_dev->nor_out_disable_votable, false);
	rerun_election(wls_dev->nor_input_disable_votable, false);

	/*if (is_comm_ocm_available(wls_dev))
		oplus_chg_comm_status_init(wls_dev->comm_ocm);*/
}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_FAULT_INJECT_CHG)
noinline
#endif

static void oplus_chg_wls_update_track_info(struct oplus_chg_wls *wls_dev,
					    char *crux_info, bool clear)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int batt_temp = 0;
	u32 rx_version = 0;
	u32 trx_version = 0;
	int cep_curr_ma = 0;
	static int highest_temp = 0;
	static int max_iout = 0;
	static int min_cool_down = 0;
	static int min_skewing_current = 0;

	if (clear) {
		highest_temp = 0;
		max_iout = 0;
		min_cool_down = 0;
		min_skewing_current = 0;
		return;
	}

	oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	highest_temp = max(highest_temp, batt_temp);
	max_iout = max(max_iout, wls_status->iout_ma);
	if (wls_status->cool_down) {
		if (!min_cool_down)
			min_cool_down = wls_status->cool_down;
		else
			min_cool_down = min(min_cool_down, wls_status->cool_down);
	}

	if (is_client_vote_enabled(wls_dev->fcc_votable, CEP_VOTER))
		cep_curr_ma = get_client_vote(wls_dev->fcc_votable, CEP_VOTER);
	else if (is_client_vote_enabled(wls_dev->nor_icl_votable, CEP_VOTER))
		cep_curr_ma = get_client_vote(wls_dev->nor_icl_votable, CEP_VOTER);

	if (cep_curr_ma) {
		if (!min_skewing_current)
			min_skewing_current = cep_curr_ma;
		else
			min_skewing_current = min(min_skewing_current, cep_curr_ma);
	}

	oplus_chg_wls_rx_get_trx_version(wls_dev->wls_rx->rx_ic, &trx_version);
	oplus_chg_wls_rx_get_rx_version(wls_dev->wls_rx->rx_ic, &rx_version);
	if (crux_info) {
		snprintf(crux_info,
			OPLUS_CHG_TRACK_CURX_INFO_LEN,
			"$$wls_general_info@@tx_version=%d,rx_version=%d,adapter_type_wpc=%d,"
			"dock_version=%d,fastchg_ing=%d,vout=%d,"
			"iout=%d,break_count=%d,wpc_chg_err=%d,"
			"highest_temp=%d,max_iout=%d,min_cool_down=%d,"
			"min_skewing_current=%d,wls_auth_fail=%d,work_silent_mode=%d",
			trx_version, rx_version, wls_status->adapter_type,
			wls_status->adapter_id, wls_status->fastchg_started, wls_status->vout_mv,
			wls_status->iout_ma, wls_status->break_count, wls_status->trx_err,
			highest_temp, max_iout, min_cool_down,
			min_skewing_current, !wls_status->verity_pass, wls_status->switch_quiet_mode);
		chg_info("%s\n", crux_info);
	}
}

#define WLS_TRX_INFO_UPLOAD_THD_2MINS		(2 * 60)
#define WLS_LOCAL_T_NS_TO_S_THD			1000000000
#define WLS_TRX_INFO_THD_1MIN			60
static int oplus_chg_wls_get_local_time_s(void)
{
	int local_time_s;

	local_time_s = local_clock() / WLS_LOCAL_T_NS_TO_S_THD;
	chg_info("local_time_s:%d\n", local_time_s);

	return local_time_s;
}

static int oplus_chg_wls_track_upload_tx_general_info(struct oplus_chg_wls *wls_dev, bool usb_present_once)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct mms_msg *msg;
	char *tx_crux_info;
	int rc;

	if (!is_err_topic_available(wls_dev)) {
		chg_err("error topic not found\n");
		return -ENODEV;
	}

	tx_crux_info = kzalloc(OPLUS_CHG_TRACK_CURX_INFO_LEN, GFP_KERNEL);
	if (!tx_crux_info) {
		chg_err("tx_crux_info memery alloc fail\n");
		return -ENOMEM;
	}

	oplus_chg_wls_update_track_info(wls_dev, tx_crux_info, false);
	tx_crux_info[OPLUS_CHG_TRACK_CURX_INFO_LEN - 1] = '\0';
	msg = oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, ERR_ITEM_WLS_INFO,
		"$$power_mode@@wireless"
		"$$total_time@@%d"
		"$$usb_present_once@@%d"
		"%s",
		(wls_status->trx_transfer_end_time - wls_status->trx_transfer_start_time) / WLS_TRX_INFO_THD_1MIN,
		usb_present_once,
		tx_crux_info);
	if (msg == NULL) {
		chg_err("alloc wls info msg error\n");
		kfree(tx_crux_info);
		return -ENOMEM;
	}

	rc = oplus_mms_publish_msg(wls_dev->err_topic, msg);
	if (rc < 0) {
		chg_err("publish wls info msg error, rc=%d\n", rc);
		kfree(msg);
	}
	kfree(tx_crux_info);

	return rc;
}

static int oplus_chg_wls_track_upload_wls_err_info(struct oplus_chg_wls *wls_dev,
	enum wls_err_scene scene_type, enum wls_err_reason reason_type)
{
	char *rx_crux_info;

	if (!is_err_topic_available(wls_dev)) {
		chg_err("error topic not found\n");
		return -ENODEV;
	}
	if (scene_type >= ARRAY_SIZE(wls_err_scene_text) || scene_type < 0) {
		chg_err("wls err scene inval\n");
		return -EINVAL;
	}
	if (reason_type >= ARRAY_SIZE(wls_err_reason_text) || reason_type < 0) {
		chg_err("wls err reason inval\n");
		return -EINVAL;
	}

	rx_crux_info = kzalloc(OPLUS_CHG_TRACK_CURX_INFO_LEN, GFP_KERNEL);
	if (!rx_crux_info) {
		chg_err("rx_crux_info memery alloc fail\n");
		return -ENOMEM;
	}

	oplus_chg_wls_update_track_info(wls_dev, rx_crux_info, false);
	rx_crux_info[OPLUS_CHG_TRACK_CURX_INFO_LEN - 1] = '\0';
	oplus_chg_ic_creat_err_msg(wls_dev->wls_rx->rx_ic, OPLUS_IC_ERR_WLS_RX, 0,
		"$$err_scene@@%s$$err_reason@@%s"
		"%s",
		wls_err_scene_text[scene_type], wls_err_reason_text[reason_type],
		rx_crux_info);
	oplus_chg_ic_virq_trigger(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_ERR);
	kfree(rx_crux_info);

	return 0;
}

static int oplus_chg_wls_set_trx_enable(struct oplus_chg_wls *wls_dev, bool en)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc = 0;

	if (en && wls_status->fw_upgrading) {
		chg_err("FW is upgrading, reverse charging cannot be used\n");
		return -EFAULT;
	}
	if (en && wls_dev->usb_present && !wls_dev->support_tx_boost &&
	    !wls_dev->support_wls_and_tx_boost) {
		chg_err("during USB charging, reverse charging cannot be used\n");
		return -EFAULT;
	}
	if (en && wls_status->rx_present) {
		chg_err("wireless charging, reverse charging cannot be used\n");
		return -EFAULT;
	}

	mutex_lock(&wls_dev->connect_lock);
	if (en) {
		if (wls_status->wls_type == OPLUS_CHG_WLS_TRX)
			goto out;
		cancel_delayed_work_sync(&wls_dev->wls_connect_work);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		if (!wls_dev->usb_present)
			vote(wls_dev->wrx_en_votable, TRX_EN_VOTER, true, 1, false);
		else
			vote(wls_dev->wrx_en_votable, TRX_EN_VOTER, false, 0, false);
		msleep(20);
#endif
		rc = oplus_chg_wls_nor_set_boost_vol(wls_dev->wls_nor->nor_ic, WLS_TRX_MODE_VOL_MV);
		if (rc < 0) {
			chg_err("can't set trx boost vol, rc=%d\n", rc);
			goto out;
		}
		oplus_chg_wls_nor_set_boost_en(wls_dev->wls_nor->nor_ic, true);
		if (rc < 0) {
			chg_err("can't enable trx boost, rc=%d\n", rc);
			goto out;
		}
		msleep(500);
		oplus_chg_wls_reset_variables(wls_dev);
		wls_status->wls_type = OPLUS_CHG_WLS_TRX;
		/*set trx_present true after power up for factory mode*/
		wls_status->trx_present = true;
		wls_status->trx_present_keep = false;
		wls_status->trx_rxac = false;
		oplus_chg_wls_rx_set_trx_start(wls_dev->wls_rx->rx_ic);

		if (!wls_dev->trx_wake_lock_on) {
			chg_info("acquire trx_wake_lock\n");
			__pm_stay_awake(wls_dev->trx_wake_lock);
			wls_dev->trx_wake_lock_on = true;
		} else {
			chg_err("trx_wake_lock is already stay awake\n");
		}
		cancel_delayed_work_sync(&wls_dev->wls_trx_sm_work);
		queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_trx_sm_work, 0);
	} else {
		if (wls_status->wls_type != OPLUS_CHG_WLS_TRX)
			goto out;
		if (wls_status->trx_rxac == true)
			vote(wls_dev->rx_disable_votable, RXAC_VOTER, true, 1, false);
		cancel_delayed_work_sync(&wls_dev->wls_trx_sm_work);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		vote(wls_dev->rx_disable_votable, TRX_EN_VOTER, true, 0, false);
#endif
		oplus_chg_wls_nor_set_boost_en(wls_dev->wls_nor->nor_ic, false);
		wls_status->wls_type = OPLUS_CHG_WLS_UNKNOWN;
		wls_status->trx_present = false;
		wls_status->trx_online = false;
		msleep(100);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		vote(wls_dev->wrx_en_votable, TRX_EN_VOTER, false, 0, false);
		vote(wls_dev->rx_disable_votable, TRX_EN_VOTER, false, 0, false);
#endif
		if (wls_status->trx_rxac == true)
			vote(wls_dev->rx_disable_votable, RXAC_VOTER, false, 0, false);
		wls_status->trx_transfer_end_time = oplus_chg_wls_get_local_time_s();
		chg_info("trx_online=%d, usb_present_once=%d,start_time=%d, end_time=%d\n",
			wls_status->trx_online, wls_status->trx_usb_present_once,
			wls_status->trx_transfer_start_time,
			wls_status->trx_transfer_end_time);
		if (wls_status->trx_transfer_start_time &&
		   (wls_status->trx_transfer_end_time - wls_status->trx_transfer_start_time >
		    WLS_TRX_INFO_UPLOAD_THD_2MINS)) {
			oplus_chg_wls_track_upload_tx_general_info(
				wls_dev, wls_status->trx_usb_present_once);
		}
		oplus_chg_wls_reset_variables(wls_dev);
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
		if (wls_dev->trx_wake_lock_on) {
			chg_info("release trx_wake_lock\n");
			__pm_relax(wls_dev->trx_wake_lock);
			wls_dev->trx_wake_lock_on = false;
		} else {
			chg_err("trx_wake_lock is already relax\n");
		}
	}

out:
	mutex_unlock(&wls_dev->connect_lock);
	return rc;
}

#ifdef MAYBE_DELETE
#ifdef OPLUS_CHG_DEBUG
static int oplus_chg_wls_path_ctrl(struct oplus_chg_wls *wls_dev,
				enum oplus_chg_wls_path_ctrl_type type)
{
	if (wls_dev->force_type != OPLUS_CHG_WLS_FORCE_TYPE_FAST)
		return -EINVAL;
	if (!wls_dev->support_fastchg)
		return -EINVAL;

	switch(type) {
	case OPLUS_CHG_WLS_PATH_CTRL_TYPE_DISABLE_ALL:
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 0, false);
		break;
	case OPLUS_CHG_WLS_PATH_CTRL_TYPE_ENABLE_NOR:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		break;
	case OPLUS_CHG_WLS_PATH_CTRL_TYPE_DISABLE_NOR:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 0, false);
		break;
	case OPLUS_CHG_WLS_PATH_CTRL_TYPE_ENABLE_FAST:
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast, true);
		(void)oplus_chg_wls_fast_start(wls_dev->wls_fast);
		break;
	case OPLUS_CHG_WLS_PATH_CTRL_TYPE_DISABLE_FAST:
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast, false);
		break;
	default:
		chg_err("unknown path ctrl type(=%d), \n", type);
		return -EINVAL;
	}

	return 0;
}
#endif /* OPLUS_CHG_DEBUG */
#endif /*MAYBE_DELETE*/

static void oplus_chg_wls_set_bcc_current_iout(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int bcc_icl = 0;

	if (!wls_dev->support_wls_chg_bcc)
		return;

	bcc_icl = wls_status->bcc_current / wls_dev->wls_bcc_fcc_to_icl_factor;

	if (bcc_icl > 0 &&
	    (wls_status->wls_type == OPLUS_CHG_WLS_SVOOC ||
	    wls_status->wls_type == OPLUS_CHG_WLS_PD_65W)) {
		vote(wls_dev->nor_icl_votable, BCC_CURRENT_VOTER, true, bcc_icl, false);
		vote(wls_dev->fcc_votable, BCC_CURRENT_VOTER, true, bcc_icl, false);
	}

	chg_info("set bcc_icl=%d, wls_status->bcc_current=%d\n", bcc_icl, wls_status->bcc_current);
}

static void oplus_chg_wls_set_cool_down_iout(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int cool_down = wls_status->cool_down;
	int icl_ma = 0;
	int cool_down_max_step = 0;
	int index = 0;

	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_BPP:
		index = OPLUS_WLS_CHG_MODE_BPP;
		break;
	case OPLUS_CHG_WLS_EPP:
		index = OPLUS_WLS_CHG_MODE_EPP;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		index = OPLUS_WLS_CHG_MODE_EPP_PLUS;
		break;
	case OPLUS_CHG_WLS_VOOC:
		index = OPLUS_WLS_CHG_MODE_AIRVOOC;
		break;
	case OPLUS_CHG_WLS_SVOOC:
	case OPLUS_CHG_WLS_PD_65W:
		index = OPLUS_WLS_CHG_MODE_AIRSVOOC;
		break;
	default:
		index = OPLUS_WLS_CHG_MODE_BPP;
		break;
	}

	cool_down_max_step = wls_dev->cool_down_step[index].max_step;
	if (cool_down >= cool_down_max_step && cool_down > 1)
		cool_down = cool_down_max_step - 1;
	icl_ma = dynamic_cfg->cool_down_step[index][cool_down];

	if (cool_down == 0 || wls_dev->factory_mode) {
		vote(wls_dev->nor_icl_votable, COOL_DOWN_VOTER, false, 0, false);
		vote(wls_dev->fcc_votable, COOL_DOWN_VOTER, false, 0, false);
		icl_ma = 0;
	} else {
		if (icl_ma > 0) {
			vote(wls_dev->nor_icl_votable, COOL_DOWN_VOTER, true, icl_ma, false);
			vote(wls_dev->fcc_votable, COOL_DOWN_VOTER, true, icl_ma, false);
		}
	}

	chg_info("icl_cool_down_ma=%d, cool_down_level=%d\n", icl_ma, wls_status->cool_down);
}

static enum oplus_chg_temp_region oplus_chg_wls_get_temp_region(struct oplus_chg_wls *wls_dev)
{
	enum oplus_chg_temp_region temp_region;
	int batt_temp;
	int rc;

	switch (wls_dev->wls_status.comm_temp_region) {
	case TEMP_REGION_COLD:
		temp_region = BATT_TEMP_COLD;
		break;
	case TEMP_REGION_LITTLE_COLD:
		temp_region = BATT_TEMP_LITTLE_COLD;
		break;
	case TEMP_REGION_COOL:
		temp_region = BATT_TEMP_COOL;
		break;
	case TEMP_REGION_LITTLE_COOL:
		temp_region = BATT_TEMP_LITTLE_COOL;
		break;
	case TEMP_REGION_PRE_NORMAL:
		temp_region = BATT_TEMP_PRE_NORMAL;
		break;
	case TEMP_REGION_NORMAL:
		rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
		if (rc < 0 || batt_temp < 375)
			temp_region = BATT_TEMP_NORMAL;
		else
			temp_region = BATT_TEMP_LITTLE_WARM;
		break;
	case TEMP_REGION_WARM:
		temp_region = BATT_TEMP_WARM;
		break;
	case TEMP_REGION_HOT:
		temp_region = BATT_TEMP_HOT;
		break;
	case TEMP_REGION_MAX:
		temp_region = BATT_TEMP_MAX;
		break;
	default:
		temp_region = BATT_TEMP_NORMAL;
		break;
	}
	return temp_region;
}

static int oplus_chg_wls_choose_fastchg_curve(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	enum oplus_chg_temp_region temp_region;
	int batt_soc_plugin, batt_temp_plugin;
	int real_soc = 100;
	int rc = 0;
	int i = 0;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	if (temp_region <= BATT_TEMP_COOL) {
		batt_temp_plugin = WLS_FAST_TEMP_0_TO_50;
	} else if (temp_region == BATT_TEMP_LITTLE_COOL) {
		batt_temp_plugin = WLS_FAST_TEMP_50_TO_120;
	} else if (temp_region == BATT_TEMP_PRE_NORMAL) {
		batt_temp_plugin = WLS_FAST_TEMP_120_TO_160;
	} else if (temp_region == BATT_TEMP_NORMAL) {
		batt_temp_plugin = WLS_FAST_TEMP_160_TO_400;
	} else if (temp_region == BATT_TEMP_LITTLE_WARM) {
		batt_temp_plugin = WLS_FAST_TEMP_400_TO_440;
	} else {
		batt_temp_plugin = WLS_FAST_TEMP_400_TO_440;
	}

	rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
	if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
		chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
		return -EPERM;
	}

	if (real_soc < dynamic_cfg->wls_strategy_soc[0]) {
		batt_soc_plugin = WLS_FAST_SOC_0_TO_30;
	} else if (real_soc < dynamic_cfg->wls_strategy_soc[1]) {
		batt_soc_plugin = WLS_FAST_SOC_30_TO_70;
	} else if (real_soc < dynamic_cfg->wls_strategy_soc[2]) {
		batt_soc_plugin = WLS_FAST_SOC_70_TO_90;
	} else {
		batt_soc_plugin = WLS_FAST_SOC_70_TO_90;
	}

	if ((batt_temp_plugin != WLS_FAST_TEMP_MAX)
			&& (batt_soc_plugin != WLS_FAST_SOC_MAX)) {
		if (wls_dev->wls_status.adapter_id != WLS_ADAPTER_THIRD_PARTY) {
			wls_dev->wls_fcc_step.max_step =
			    wls_dev->fcc_steps[batt_soc_plugin].fcc_step[batt_temp_plugin].max_step;
			memcpy(&wls_dev->wls_fcc_step.fcc_step,
			    wls_dev->fcc_steps[batt_soc_plugin].fcc_step[batt_temp_plugin].fcc_step,
			    sizeof(struct oplus_chg_wls_range_data) * wls_dev->wls_fcc_step.max_step);
		} else {
			wls_dev->wls_fcc_step.max_step =
			    wls_dev->fcc_third_part_steps[batt_soc_plugin].fcc_step[batt_temp_plugin].max_step;
			memcpy(&wls_dev->wls_fcc_step.fcc_step,
			    wls_dev->fcc_third_part_steps[batt_soc_plugin].fcc_step[batt_temp_plugin].fcc_step,
			    sizeof(struct oplus_chg_wls_range_data) * wls_dev->wls_fcc_step.max_step);
		}
		for (i=0; i < wls_dev->wls_fcc_step.max_step; i++) {
			chg_info(" %d %d %d %d %d\n",
				wls_dev->wls_fcc_step.fcc_step[i].low_threshold,
				wls_dev->wls_fcc_step.fcc_step[i].high_threshold,
				wls_dev->wls_fcc_step.fcc_step[i].curr_ma,
				wls_dev->wls_fcc_step.fcc_step[i].vol_max_mv,
				wls_dev->wls_fcc_step.fcc_step[i].need_wait);
		}
	} else {
		return -EPERM;
	}

	return 0;
}

static int oplus_chg_wls_bcc_choose_curve(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	enum oplus_chg_temp_region temp_region;
	int batt_soc_plugin, batt_temp_plugin;
	int real_soc = 100;
	int rc = 0;
	int i = 0;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	if (temp_region <= BATT_TEMP_COOL)
		batt_temp_plugin = WLS_BCC_TEMP_0_TO_50;
	else if (temp_region == BATT_TEMP_LITTLE_COOL)
		batt_temp_plugin = WLS_BCC_TEMP_50_TO_120;
	else if (temp_region == BATT_TEMP_PRE_NORMAL)
		batt_temp_plugin = WLS_BCC_TEMP_120_TO_160;
	else if (temp_region == BATT_TEMP_NORMAL)
		batt_temp_plugin = WLS_BCC_TEMP_160_TO_400;
	else if (temp_region == BATT_TEMP_LITTLE_WARM)
		batt_temp_plugin = WLS_BCC_TEMP_400_TO_440;
	else
		batt_temp_plugin = WLS_BCC_TEMP_400_TO_440;

	wls_status->bcc_temp_range = batt_temp_plugin;

	rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
	if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
		chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
		return -EPERM;
	}

	if (real_soc < dynamic_cfg->wls_strategy_soc[0])
		batt_soc_plugin = WLS_BCC_SOC_0_TO_30;
	else if (real_soc < dynamic_cfg->wls_strategy_soc[1])
		batt_soc_plugin = WLS_BCC_SOC_30_TO_70;
	else if (real_soc < dynamic_cfg->wls_strategy_soc[2])
		batt_soc_plugin = WLS_BCC_SOC_70_TO_90;
	else
		batt_soc_plugin = WLS_BCC_SOC_70_TO_90;

	if ((batt_temp_plugin != WLS_BCC_TEMP_MAX) &&
	    (batt_soc_plugin != WLS_BCC_SOC_MAX)) {
		wls_dev->wls_bcc_step.max_step =
			wls_dev->bcc_steps[batt_soc_plugin].bcc_step[batt_temp_plugin].max_step;
		memcpy(&wls_dev->wls_bcc_step.bcc_step,
			wls_dev->bcc_steps[batt_soc_plugin].bcc_step[batt_temp_plugin].bcc_step,
			sizeof(struct oplus_chg_wls_bcc_data) * wls_dev->wls_bcc_step.max_step);
		for (i = 0; i < wls_dev->wls_bcc_step.max_step; i++) {
			chg_info(" %d %d %d %d\n",
				wls_dev->wls_bcc_step.bcc_step[i].max_batt_volt,
				wls_dev->wls_bcc_step.bcc_step[i].max_curr,
				wls_dev->wls_bcc_step.bcc_step[i].min_curr,
				wls_dev->wls_bcc_step.bcc_step[i].exit);
		}
	}

	return 0;
}

static void oplus_chg_wls_bcc_get_curve(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_bcc_step *bcc_chg = &wls_dev->wls_bcc_step;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int bcc_batt_volt_now;
	int i, rc;
	int curve_idx = 0;

	rc = oplus_chg_wls_bcc_choose_curve(wls_dev);
	if (rc < 0) {
		chg_err("choose bcc curve failed, rc = %d\n", rc);
		return;
	}

	rc = oplus_chg_wls_get_vbat(wls_dev, &bcc_batt_volt_now);
	if (rc < 0) {
		chg_err("can't get batt vol, rc=%d\n", rc);
		return;
	}

	for (i = 0; i < wls_dev->wls_bcc_step.max_step; i++) {
		chg_debug("bcc_batt is %d target_volt is %d\n", bcc_batt_volt_now,
			bcc_chg->bcc_step[i].max_batt_volt);
		if (bcc_batt_volt_now < bcc_chg->bcc_step[i].max_batt_volt) {
			curve_idx = i;
			chg_info("curve idx = %d\n", curve_idx);
			break;
		}
	}

	wls_status->wls_bcc_max_curr = bcc_chg->bcc_step[curve_idx].max_curr;
	wls_status->wls_bcc_min_curr = bcc_chg->bcc_step[curve_idx].min_curr;
	chg_info("choose max curr is %d, min curr is %d\n", wls_status->wls_bcc_max_curr, wls_status->wls_bcc_min_curr);
}

#define CURVE_CHANGE_COUNT 20
static void oplus_chg_wls_bcc_curr_update_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev =
		container_of(dwork, struct oplus_chg_wls, wls_bcc_curr_update_work);
	struct oplus_chg_wls_bcc_step *bcc_chg = &wls_dev->wls_bcc_step;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int bcc_batt_volt_now;
	int i, rc;
	static int pre_curve_idx = 0;
	static int idx_cnt = 0;

	rc = oplus_chg_wls_get_vbat(wls_dev, &bcc_batt_volt_now);
	if (rc < 0) {
		chg_err("can't get batt vol, rc=%d\n", rc);
		return;
	}

	for (i = wls_status->bcc_true_idx; i < wls_dev->wls_bcc_step.max_step; i++) {
		chg_debug("bcc_batt is %d target_volt now is %d\n", bcc_batt_volt_now,
			bcc_chg->bcc_step[i].max_batt_volt);
		if (bcc_batt_volt_now < bcc_chg->bcc_step[i].max_batt_volt) {
			wls_status->bcc_curve_idx = i;
			chg_info("curve idx = %d\n", wls_status->bcc_curve_idx);
			break;
		}
	}

	if (pre_curve_idx == wls_status->bcc_curve_idx) {
		idx_cnt = idx_cnt + 1;
		if (idx_cnt >= CURVE_CHANGE_COUNT) {
			wls_status->wls_bcc_max_curr = bcc_chg->bcc_step[wls_status->bcc_curve_idx].max_curr;
			wls_status->wls_bcc_min_curr = bcc_chg->bcc_step[wls_status->bcc_curve_idx].min_curr;
			wls_status->bcc_true_idx = wls_status->bcc_curve_idx;
			chg_info("choose max curr is %d, min curr is %d true idx is %d\n",
				wls_status->wls_bcc_max_curr, wls_status->wls_bcc_min_curr, wls_status->bcc_true_idx);
		}
	} else {
		idx_cnt = 0;
	}
	pre_curve_idx = wls_status->bcc_curve_idx;

	schedule_delayed_work(&wls_dev->wls_bcc_curr_update_work, OPLUS_WLS_BCC_UPDATE_INTERVAL);
}

static bool oplus_chg_wls_wake_bcc_update_work(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev)
		return true;

	schedule_delayed_work(&wls_dev->wls_bcc_curr_update_work, OPLUS_WLS_BCC_UPDATE_INTERVAL);

	return true;
}

static void oplus_chg_wls_cancel_bcc_update_work(struct oplus_chg_wls *wls_dev)
{
	if (!wls_dev)
		return;

	cancel_delayed_work_sync(&wls_dev->wls_bcc_curr_update_work);
}

#define OPLUS_WLS_BCC_MAX_CURR_INIT 5000
#define OPLUS_WLS_BCC_MIN_CURR_INIT 4000
#define OPLUS_WLS_BCC_STOP_CURR_INIT 1000
static void oplus_chg_wls_bcc_parms_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (!wls_dev)
		return;

	wls_status->wls_bcc_max_curr = OPLUS_WLS_BCC_MAX_CURR_INIT;
	wls_status->wls_bcc_min_curr = OPLUS_WLS_BCC_MIN_CURR_INIT;
	wls_status->wls_bcc_stop_curr = OPLUS_WLS_BCC_STOP_CURR_INIT;
	wls_status->bcc_curve_idx = 0;
	wls_status->bcc_true_idx = 0;
	wls_status->bcc_temp_range = 0;
}

static int oplus_chg_wls_get_ffc_status(void)
{
	int ffc_status = FFC_DEFAULT;
	struct oplus_mms *comm_topic;
	union mms_msg_data data = { 0 };
	int rc;

	comm_topic = oplus_mms_get_by_name("common");
	if (!comm_topic)
		return FFC_DEFAULT;

	rc = oplus_mms_get_item_data(comm_topic, COMM_ITEM_FFC_STATUS, &data, true);
	if (!rc)
		ffc_status = data.intval;

	return ffc_status;
}

static void oplus_chg_wls_config(struct oplus_chg_wls *wls_dev)
{
	enum oplus_chg_temp_region temp_region;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	enum oplus_chg_ffc_status ffc_status;
	int icl_max_ma;
	int fcc_max_ma;
	int icl_index;
	static bool pre_temp_abnormal;

	ffc_status = oplus_chg_wls_get_ffc_status();
	if (ffc_status != FFC_DEFAULT) {
		chg_err("ffc charging, exit\n");
		return;
	}

	if (!wls_status->rx_online && !wls_status->online_keep) {
		chg_err("rx is offline\n");
		return;
	}

	/*if (oplus_chg_comm_batt_vol_over_cl_thr(wls_dev->comm_ocm))
		icl_index = OPLUS_WLS_CHG_BATT_CL_HIGH;
	else*/
	icl_index = OPLUS_WLS_CHG_BATT_CL_LOW;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);
	switch (temp_region) {
	case BATT_TEMP_COLD:
	case BATT_TEMP_HOT:
		if (!pre_temp_abnormal) {
			pre_temp_abnormal = true;
			wls_status->online_keep = true;
			vote(wls_dev->rx_disable_votable, JEITA_VOTER, true, 1, false);
			vote(wls_dev->nor_icl_votable, JEITA_VOTER, true, WLS_CURR_JEITA_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, JEITA_VOTER, true, 1, false);
			schedule_delayed_work(&wls_dev->rx_restore_work, msecs_to_jiffies(500));
		} else {
			vote(wls_dev->rx_disable_votable, JEITA_VOTER, false, 0, false);
			vote(wls_dev->nor_icl_votable, JEITA_VOTER, true, WLS_CURR_JEITA_CHG_MA, true);
		}
		break;
	default:
		pre_temp_abnormal = false;
		vote(wls_dev->rx_disable_votable, JEITA_VOTER, false, 0, false);
		vote(wls_dev->nor_icl_votable, JEITA_VOTER, false, 0, true);
		vote(wls_dev->nor_out_disable_votable, JEITA_VOTER, false, 0, false);
		break;
	}

	if (wls_status->wls_type == OPLUS_CHG_WLS_VOOC) {
		if (wls_status->temp_region == BATT_TEMP_COOL) {
			if (temp_region == BATT_TEMP_LITTLE_COLD) {
				wls_status->online_keep = true;
				schedule_delayed_work(&wls_dev->rx_restart_work, 0);
			}
		}

		if (wls_status->temp_region == BATT_TEMP_LITTLE_WARM) {
			if (temp_region == BATT_TEMP_WARM) {
				wls_status->online_keep = true;
				schedule_delayed_work(&wls_dev->rx_restart_work, 0);
			}
		}
	}

	wls_status->temp_region = temp_region;

	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_BPP:
		icl_max_ma = dynamic_cfg->iclmax_ma[icl_index][OPLUS_WLS_CHG_MODE_BPP][temp_region];
		fcc_max_ma = 1200;
		break;
	case OPLUS_CHG_WLS_EPP:
		icl_max_ma = dynamic_cfg->iclmax_ma[icl_index][OPLUS_WLS_CHG_MODE_EPP][temp_region];
		fcc_max_ma = 2800;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		icl_max_ma = dynamic_cfg->iclmax_ma[icl_index][OPLUS_WLS_CHG_MODE_EPP_PLUS][temp_region];
		fcc_max_ma = 4000;
		break;
	case OPLUS_CHG_WLS_VOOC:
		icl_max_ma = dynamic_cfg->iclmax_ma[icl_index][OPLUS_WLS_CHG_MODE_AIRVOOC][temp_region];
		fcc_max_ma = 3600;
		break;
	case OPLUS_CHG_WLS_SVOOC:
	case OPLUS_CHG_WLS_PD_65W:
		icl_max_ma = dynamic_cfg->iclmax_ma[icl_index][OPLUS_WLS_CHG_MODE_AIRSVOOC][temp_region];
		fcc_max_ma = 3600;
		break;
	default:
		chg_err("Unsupported charging mode(=%d)\n", wls_status->wls_type);
		return;
	}
	oplus_chg_wls_set_cool_down_iout(wls_dev);
	if (wls_dev->support_wls_chg_bcc)
		oplus_chg_wls_set_bcc_current_iout(wls_dev);
	vote(wls_dev->nor_icl_votable, MAX_VOTER, true, icl_max_ma, true);
	vote(wls_dev->nor_fcc_votable, MAX_VOTER, true, fcc_max_ma, false);
	chg_info("chg_type=%d, temp_region=%d, fcc=%d, icl_max=%d\n",
		 wls_status->wls_type, temp_region, fcc_max_ma, icl_max_ma);
}

#ifdef OPLUS_CHG_DEBUG
#define UPGRADE_START 0
#define UPGRADE_FW    1
#define UPGRADE_END   2
struct oplus_chg_fw_head {
	u8 magic[4];
	int size;
};

ssize_t oplus_chg_wls_upgrade_fw_show(struct oplus_mms *mms, char *buf)
{
	int rc = 0;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -ENODEV;
	}

	rc = sprintf(buf, "wireless\n");
	return rc;
}

ssize_t oplus_chg_wls_upgrade_fw_store(struct oplus_mms *mms, const char *buf, size_t count)
{
	struct oplus_chg_wls *wls_dev = NULL;
	u8 temp_buf[sizeof(struct oplus_chg_fw_head)];
	static u8 *fw_buf;
	static int upgrade_step = UPGRADE_START;
	static int fw_index;
	static int fw_size;
	struct oplus_chg_fw_head *fw_head;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -ENODEV;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
start:
	switch (upgrade_step) {
	case UPGRADE_START:
		if (count < sizeof(struct oplus_chg_fw_head)) {
			chg_err("<FW UPDATE>image format error\n");
			return -EINVAL;
		}
		memset(temp_buf, 0, sizeof(struct oplus_chg_fw_head));
		memcpy(temp_buf, buf, sizeof(struct oplus_chg_fw_head));
		fw_head = (struct oplus_chg_fw_head *)temp_buf;
		if (fw_head->magic[0] == 0x02 && fw_head->magic[1] == 0x00 &&
		    fw_head->magic[2] == 0x03 && fw_head->magic[3] == 0x00) {
			fw_size = fw_head->size;
			fw_buf = kzalloc(fw_size, GFP_KERNEL);
			if (fw_buf == NULL) {
				chg_err("<FW UPDATE>alloc fw_buf err\n");
				return -ENOMEM;
			}
			wls_dev->fw_buf = fw_buf;
			wls_dev->fw_size = fw_size;
			chg_info("<FW UPDATE>image header verification succeeded, fw_size=%d\n", fw_size);
			memcpy(fw_buf, buf + sizeof(struct oplus_chg_fw_head), count - sizeof(struct oplus_chg_fw_head));
			fw_index = count - sizeof(struct oplus_chg_fw_head);
			chg_info("<FW UPDATE>Receiving image, fw_size=%d, fw_index=%d\n", fw_size, fw_index);
			if (fw_index >= fw_size) {
				upgrade_step = UPGRADE_END;
				goto start;
			} else {
				upgrade_step = UPGRADE_FW;
			}
		} else {
			chg_err("<FW UPDATE>image format error\n");
			return -EINVAL;
		}
		break;
	case UPGRADE_FW:
		memcpy(fw_buf + fw_index, buf, count);
		fw_index += count;
		chg_info("<FW UPDATE>Receiving image, fw_size=%d, fw_index=%d\n", fw_size, fw_index);
		if (fw_index >= fw_size) {
			upgrade_step = UPGRADE_END;
			goto start;
		}
		break;
	case UPGRADE_END:
		wls_dev->fw_upgrade_by_buf = true;
		schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, 0);
		fw_buf = NULL;
		upgrade_step = UPGRADE_START;
		break;
	default:
		upgrade_step = UPGRADE_START;
		chg_err("<FW UPDATE>status error\n");
		if (fw_buf != NULL) {
			kfree(fw_buf);
			fw_buf = NULL;
			wls_dev->fw_buf = NULL;
			wls_dev->fw_size = 0;
			wls_dev->fw_upgrade_by_buf = false;
		}
		break;
	}

	return count;
}

int oplus_chg_wls_get_break_sub_crux_info(struct oplus_mms *mms, char *crux_info)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -ENODEV;
	}
	if (crux_info == NULL) {
		chg_err("crux_info is NULL");
		return -ENODEV;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	chg_info("start\n");
	oplus_chg_wls_update_track_info(wls_dev, crux_info, false);
	return 0;
}

static int oplus_chg_wls_get_max_wireless_power(struct oplus_chg_wls *wls_dev)
{
	int max_wls_power = 0;
	int max_wls_base_power = 0;
	int max_wls_r_power = 0;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (!wls_status || !wls_status->rx_online)
		return 0;
	max_wls_base_power = oplus_chg_wls_get_base_power_max(wls_status->adapter_id);
	max_wls_r_power = oplus_chg_wls_get_r_power(wls_dev, wls_status->adapter_power);
	max_wls_r_power = max_wls_base_power > max_wls_r_power ? max_wls_r_power : max_wls_base_power;
	max_wls_power = max_wls_r_power > wls_dev->wls_power_mw ? wls_dev->wls_power_mw : max_wls_r_power;
	if (wls_status->wls_type != OPLUS_CHG_WLS_SVOOC && wls_status->wls_type != OPLUS_CHG_WLS_PD_65W)
		max_wls_power = min(max_wls_power, WLS_VOOC_PWR_MAX_MW);
	if (wls_status->adapter_id == WLS_ADAPTER_THIRD_PARTY)
		max_wls_power = 0;

	return max_wls_power;
}

#ifdef MAYBE_DELETE
static ssize_t oplus_chg_wls_path_curr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct oplus_chg_mod *ocm = dev_get_drvdata(dev);
	struct oplus_chg_wls *wls_dev = oplus_chg_mod_get_drvdata(ocm);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int nor_curr_ma, fast_curr_ma;
	ssize_t rc;

	rc = sscanf(buf, "%d,%d", &nor_curr_ma, &fast_curr_ma);
	if (rc < 0) {
		chg_err("can't read input string, rc=%d\n", rc);
		return rc;
	}
	nor_curr_ma = nor_curr_ma /1000;
	fast_curr_ma = fast_curr_ma / 1000;

	switch (wls_dev->force_type) {
	case OPLUS_CHG_WLS_FORCE_TYPE_NOEN:
	case OPLUS_CHG_WLS_FORCE_TYPE_AUTO:
		return -EINVAL;
	case OPLUS_CHG_WLS_FORCE_TYPE_BPP:
	case OPLUS_CHG_WLS_FORCE_TYPE_EPP:
	case OPLUS_CHG_WLS_FORCE_TYPE_EPP_PLUS:
		if (nor_curr_ma >= 0) {
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, nor_curr_ma, false);
		} else {
			chg_err("Parameter error\n");
			return -EINVAL;
		}
		break;
	case OPLUS_CHG_WLS_FORCE_TYPE_FAST:
		if (fast_curr_ma > 0) {
			wls_status->fastchg_started = true;
			vote(wls_dev->fcc_votable, DEBUG_VOTER, true, fast_curr_ma, false);
		} else {
			wls_status->fastchg_started = false;
			vote(wls_dev->fcc_votable, DEBUG_VOTER, false, 0, false);
		}
		if (nor_curr_ma >= 0)
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, nor_curr_ma, false);
	}

	return count;
}
#endif /*MAYBE_DELETE*/
#endif /* OPLUS_CHG_DEBUG */

#ifdef MAYBE_DELETE
static ssize_t oplus_chg_wls_ftm_test_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct oplus_chg_mod *ocm = dev_get_drvdata(dev);
	struct oplus_chg_wls *wls_dev = oplus_chg_mod_get_drvdata(ocm);
	int rc;
	ssize_t index = 0;

	if (oplus_chg_wls_is_usb_present(wls_dev) && !wls_dev->support_tx_boost &&
	    !wls_dev->support_wls_and_tx_boost) {
		chg_info("usb online, can't run rx smt test\n");
		index += sprintf(buf + index, "%d,%s\n", WLS_PATH_RX, "usb_online");
		goto skip_rx_check;
	}

	if (wls_dev->wls_status.rx_present) {
		chg_info("wls online, can't run rx smt test\n");
		index += sprintf(buf + index, "%d,%s\n", WLS_PATH_RX, "wls_online");
		goto skip_rx_check;
	}

	vote(wls_dev->wrx_en_votable, FTM_TEST_VOTER, true, 1, false);
	rc = oplus_chg_wls_rx_smt_test(wls_dev->wls_rx);
	if (rc != 0)
		index += sprintf(buf + index, "%d,%d\n", WLS_PATH_RX, rc);
	vote(wls_dev->wrx_en_votable, FTM_TEST_VOTER, true, 0, false);

skip_rx_check:
	if (wls_dev->support_fastchg) {
		rc = oplus_chg_wls_fast_smt_test(wls_dev->wls_fast);
		if (rc != 0)
			index += sprintf(buf + index, "%d,%d\n", WLS_PATH_FAST, rc);
	}

	if (index == 0)
		index += sprintf(buf + index, "OK\r\n");
	else
		index += sprintf(buf + index, "ERROR\r\n");

	return index;
}
#endif /*MAYBE_DELETE*/

#ifdef MAYBE_DELETE
static enum oplus_chg_mod_property oplus_chg_wls_props[] = {
	OPLUS_CHG_PROP_ONLINE,
	OPLUS_CHG_PROP_PRESENT,
	OPLUS_CHG_PROP_VOLTAGE_NOW,
	OPLUS_CHG_PROP_VOLTAGE_MAX,
	OPLUS_CHG_PROP_VOLTAGE_MIN,
	OPLUS_CHG_PROP_CURRENT_NOW,
	OPLUS_CHG_PROP_CURRENT_MAX,
	OPLUS_CHG_PROP_INPUT_CURRENT_NOW,
	OPLUS_CHG_PROP_WLS_TYPE,
	OPLUS_CHG_PROP_FASTCHG_STATUS,
	OPLUS_CHG_PROP_ADAPTER_SID,
	OPLUS_CHG_PROP_ADAPTER_TYPE,
	OPLUS_CHG_PROP_DOCK_TYPE,
	OPLUS_CHG_PROP_WLS_SKEW_CURR,
	OPLUS_CHG_PROP_VERITY,
	OPLUS_CHG_PROP_CHG_ENABLE,
	OPLUS_CHG_PROP_TRX_VOLTAGE_NOW,
	OPLUS_CHG_PROP_TRX_CURRENT_NOW,
	OPLUS_CHG_PROP_TRX_STATUS,
	OPLUS_CHG_PROP_TRX_ONLINE,
	OPLUS_CHG_PROP_DEVIATED,
#ifdef OPLUS_CHG_DEBUG
	OPLUS_CHG_PROP_FORCE_TYPE,
	OPLUS_CHG_PROP_PATH_CTRL,
#endif
	OPLUS_CHG_PROP_STATUS_DELAY,
	OPLUS_CHG_PROP_QUIET_MODE,
	OPLUS_CHG_PROP_VRECT_NOW,
	OPLUS_CHG_PROP_TRX_POWER_EN,
	OPLUS_CHG_PROP_TRX_POWER_VOL,
	OPLUS_CHG_PROP_TRX_POWER_CURR_LIMIT,
	OPLUS_CHG_PROP_FACTORY_MODE,
	OPLUS_CHG_PROP_TX_POWER,
	OPLUS_CHG_PROP_RX_POWER,
	OPLUS_CHG_PROP_RX_MAX_POWER,
	OPLUS_CHG_PROP_FOD_CAL,
	OPLUS_CHG_PROP_BATT_CHG_ENABLE,
	OPLUS_CHG_PROP_ONLINE_KEEP,
	OPLUS_CHG_PROP_TX_VOLTAGE_NOW,
	OPLUS_CHG_PROP_TX_CURRENT_NOW,
	OPLUS_CHG_PROP_CP_VOLTAGE_NOW,
	OPLUS_CHG_PROP_CP_CURRENT_NOW,
	OPLUS_CHG_PROP_WIRELESS_MODE,
	OPLUS_CHG_PROP_WIRELESS_TYPE,
	OPLUS_CHG_PROP_CEP_INFO,
	OPLUS_CHG_PROP_REAL_TYPE,
	OPLUS_CHG_PROP_COOL_DOWN,
	OPLUS_CHG_PROP_RX_VOUT_UVP,
	OPLUS_CHG_PROP_FW_UPGRADING,
	OPLUS_CHG_PROP_BCC_SUPPORT,
	OPLUS_CHG_PROP_BCC_CURRENT,
	OPLUS_CHG_PROP_WLS_BCC_MAX,
	OPLUS_CHG_PROP_WLS_BCC_MIN,
	OPLUS_CHG_PROP_WLS_BCC_STOP,
};

static enum oplus_chg_mod_property oplus_chg_wls_uevent_props[] = {
	OPLUS_CHG_PROP_ONLINE,
	OPLUS_CHG_PROP_PRESENT,
	OPLUS_CHG_PROP_VOLTAGE_NOW,
	OPLUS_CHG_PROP_CURRENT_NOW,
	OPLUS_CHG_PROP_WLS_TYPE,
	OPLUS_CHG_PROP_FASTCHG_STATUS,
	OPLUS_CHG_PROP_ADAPTER_SID,
	OPLUS_CHG_PROP_TRX_VOLTAGE_NOW,
	OPLUS_CHG_PROP_TRX_CURRENT_NOW,
	OPLUS_CHG_PROP_TRX_STATUS,
	OPLUS_CHG_PROP_TRX_ONLINE,
	OPLUS_CHG_PROP_QUIET_MODE,
	OPLUS_CHG_PROP_BCC_SUPPORT,
	OPLUS_CHG_PROP_BCC_CURRENT,
	OPLUS_CHG_PROP_WLS_BCC_MAX,
	OPLUS_CHG_PROP_WLS_BCC_MIN,
	OPLUS_CHG_PROP_WLS_BCC_STOP,
};

static struct oplus_chg_exten_prop oplus_chg_wls_exten_props[] = {
#ifdef OPLUS_CHG_DEBUG
	OPLUS_CHG_EXTEN_RWATTR(OPLUS_CHG_EXTERN_PROP_UPGRADE_FW, oplus_chg_wls_upgrade_fw),
	OPLUS_CHG_EXTEN_WOATTR(OPLUS_CHG_EXTERN_PROP_PATH_CURRENT, oplus_chg_wls_path_curr),
#endif
	OPLUS_CHG_EXTEN_ROATTR(OPLUS_CHG_PROP_FTM_TEST, oplus_chg_wls_ftm_test),
};

static int oplus_chg_wls_get_prop(struct oplus_chg_mod *ocm,
			enum oplus_chg_mod_property prop,
			union oplus_chg_mod_propval *pval)
{
	struct oplus_chg_wls *wls_dev = oplus_chg_mod_get_drvdata(ocm);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc = 0;

	switch (prop) {
	case OPLUS_CHG_PROP_ONLINE:
		pval->intval = wls_status->rx_online;
		break;
	case OPLUS_CHG_PROP_PRESENT:
		pval->intval = wls_status->rx_present;
		break;
	case OPLUS_CHG_PROP_VOLTAGE_NOW:
		pval->intval = wls_status->vout_mv;
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_vol(wls_dev->wls_rx, &wls_status->trx_vol_mv);
			if (rc < 0)
				chg_err("can't get trx vol, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_vol_mv;
		}
		break;
	case OPLUS_CHG_PROP_VOLTAGE_MAX:
		pval->intval = wls_status->vout_mv * 1000;
		break;
	case OPLUS_CHG_PROP_VOLTAGE_MIN:
		pval->intval = wls_status->vout_mv * 1000;
		break;
	case OPLUS_CHG_PROP_CURRENT_NOW:
		pval->intval = wls_status->iout_ma;
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_curr(wls_dev->wls_rx, &wls_status->trx_curr_ma);
			if (rc < 0)
				chg_err("can't get trx curr, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_curr_ma;
		}
		break;
	case OPLUS_CHG_PROP_CURRENT_MAX:
		pval->intval = wls_status->iout_ma * 1000;
		break;
	case OPLUS_CHG_PROP_INPUT_CURRENT_NOW:
		rc = oplus_chg_wls_nor_get_input_curr(wls_dev->wls_nor,
						      &pval->intval);
		break;
	case OPLUS_CHG_PROP_WLS_TYPE:
		pval->intval = wls_status->wls_type;
		break;
	case OPLUS_CHG_PROP_FASTCHG_STATUS:
		pval->intval = wls_status->fastchg_started;
		break;
	case OPLUS_CHG_PROP_ADAPTER_SID:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_ADAPTER_TYPE:
		pval->intval = wls_status->adapter_type;
		break;
	case OPLUS_CHG_PROP_DOCK_TYPE:
		pval->intval = wls_status->adapter_id;
		break;
	case OPLUS_CHG_PROP_WLS_SKEW_CURR:
		pval->intval = oplus_chg_wls_get_skewing_current(wls_dev);
		break;
	case OPLUS_CHG_PROP_VERITY:
		pval->intval = wls_status->verity_pass;
		break;
	case OPLUS_CHG_PROP_CHG_ENABLE:
		pval->intval = wls_dev->charge_enable;
		break;
	case OPLUS_CHG_PROP_TRX_VOLTAGE_NOW:
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_vol(wls_dev->wls_rx, &wls_status->trx_vol_mv);
			if (rc < 0)
				chg_err("can't get trx vol, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_vol_mv * 1000;
		} else {
			pval->intval = 0;
		}
		break;
	case OPLUS_CHG_PROP_TRX_CURRENT_NOW:
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_curr(wls_dev->wls_rx, &wls_status->trx_curr_ma);
			if (rc < 0)
				chg_err("can't get trx curr, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_curr_ma * 1000;
		} else {
			pval->intval = 0;
		}
		break;
	case OPLUS_CHG_PROP_TRX_STATUS:
		if (wls_status->trx_online)
			pval->intval = OPLUS_CHG_WLS_TRX_STATUS_CHARGING;
		else if (wls_status->trx_present || wls_status->trx_present_keep)
			pval->intval = OPLUS_CHG_WLS_TRX_STATUS_ENABLE;
		else
			pval->intval = OPLUS_CHG_WLS_TRX_STATUS_DISENABLE;
		break;
	case OPLUS_CHG_PROP_TRX_ONLINE:
		pval->intval = wls_status->trx_present;
		break;
	case OPLUS_CHG_PROP_DEVIATED:
		pval->intval = 0;
		break;
#ifdef OPLUS_CHG_DEBUG
	case OPLUS_CHG_PROP_FORCE_TYPE:
		pval->intval = wls_dev->force_type;
		break;
	case OPLUS_CHG_PROP_PATH_CTRL:
		rc = 0;
		break;
#endif
	case OPLUS_CHG_PROP_STATUS_DELAY:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_QUIET_MODE:
		pval->intval = wls_status->switch_quiet_mode;
		break;
	case OPLUS_CHG_PROP_VRECT_NOW:
		pval->intval = wls_status->vrect_mv *1000;
		break;
	case OPLUS_CHG_PROP_TRX_POWER_EN:
	case OPLUS_CHG_PROP_TRX_POWER_VOL:
	case OPLUS_CHG_PROP_TRX_POWER_CURR_LIMIT:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_FACTORY_MODE:
		pval->intval = wls_dev->factory_mode;
		break;
	case OPLUS_CHG_PROP_TX_POWER:
		pval->intval = wls_status->tx_pwr_mw;
		break;
	case OPLUS_CHG_PROP_RX_POWER:
		pval->intval = wls_status->rx_pwr_mw;
		break;
	case OPLUS_CHG_PROP_RX_MAX_POWER:
		pval->intval = wls_status->pwr_max_mw;
		break;
	case OPLUS_CHG_PROP_FOD_CAL:
		pval->intval = wls_dev->fod_is_cal;
		break;
	case OPLUS_CHG_PROP_ONLINE_KEEP:
		pval->intval = wls_status->online_keep || wls_status->boot_online_keep;
		break;
	case OPLUS_CHG_PROP_BATT_CHG_ENABLE:
		pval->intval = wls_dev->batt_charge_enable;
		break;
	case OPLUS_CHG_PROP_TX_VOLTAGE_NOW:
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_vol(wls_dev->wls_rx, &wls_status->trx_vol_mv);
			if (rc < 0)
				chg_err("can't get trx vol, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_vol_mv * 1000;
		} else {
			pval->intval = 0;
		}
		break;
	case OPLUS_CHG_PROP_TX_CURRENT_NOW:
		if (wls_status->trx_present) {
			rc = oplus_chg_wls_rx_get_trx_curr(wls_dev->wls_rx, &wls_status->trx_curr_ma);
			if (rc < 0)
				chg_err("can't get trx curr, rc=%d\n", rc);
			else
				pval->intval = wls_status->trx_curr_ma * 1000;
		} else {
			pval->intval = 0;
		}
		break;
	case OPLUS_CHG_PROP_CP_VOLTAGE_NOW:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_CP_CURRENT_NOW:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_WIRELESS_MODE:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_WIRELESS_TYPE:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_CEP_INFO:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_REAL_TYPE:
		switch (wls_status->wls_type) {
		case OPLUS_CHG_WLS_BPP:
			pval->intval = POWER_SUPPLY_TYPE_USB_DCP;
			break;
		case OPLUS_CHG_WLS_EPP:
			pval->intval = POWER_SUPPLY_TYPE_USB_PD;
			break;
		case OPLUS_CHG_WLS_EPP_PLUS:
			pval->intval = POWER_SUPPLY_TYPE_USB_PD;
			break;
		case OPLUS_CHG_WLS_VOOC:
		case OPLUS_CHG_WLS_SVOOC:
		case OPLUS_CHG_WLS_PD_65W:
			pval->intval = POWER_SUPPLY_TYPE_USB_DCP;
			break;
		default:
			chg_err("Unsupported charging mode(=%d)\n", wls_status->wls_type);
			pval->intval = POWER_SUPPLY_TYPE_UNKNOWN;
			break;
		}
		break;
	case OPLUS_CHG_PROP_COOL_DOWN:
		pval->intval = wls_status->cool_down;
		break;
	case OPLUS_CHG_PROP_BCC_SUPPORT:
		pval->intval = oplus_wls_check_bcc_support(wls_dev);
		break;
	case OPLUS_CHG_PROP_BCC_CURRENT:
		pval->intval = wls_status->bcc_current / BCC_TO_ICL;
		break;
	case OPLUS_CHG_PROP_RX_VOUT_UVP:
		pval->intval = 0;
		break;
	case OPLUS_CHG_PROP_FW_UPGRADING:
		pval->intval = wls_status->fw_upgrading;
		break;
	case OPLUS_CHG_PROP_WLS_BCC_MAX:
		pval->intval = wls_status->wls_bcc_max_curr / BCC_TO_ICL;
		break;
	case OPLUS_CHG_PROP_WLS_BCC_MIN:
		pval->intval = wls_status->wls_bcc_min_curr / BCC_TO_ICL;
		break;
	case OPLUS_CHG_PROP_WLS_BCC_STOP:
		pval->intval = oplus_wls_bcc_get_stop_curr(wls_dev);
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

static int oplus_chg_wls_set_prop(struct oplus_chg_mod *ocm,
			enum oplus_chg_mod_property prop,
			const union oplus_chg_mod_propval *pval)
{
	struct oplus_chg_wls *wls_dev = oplus_chg_mod_get_drvdata(ocm);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc = 0;

	switch (prop) {
	case OPLUS_CHG_PROP_VOLTAGE_MAX:
		if (wls_dev->debug_mode) {
			wls_status->fastchg_started = false;
			rc = oplus_chg_wls_rx_set_vout(wls_dev->wls_rx,
				pval->intval / 1000, 0);
			vote(wls_dev->fcc_votable, DEBUG_VOTER, false, 0, false);
		} else {
			rc = -EINVAL;
			chg_err("need to open debug mode first\n");
		}
		break;
	case OPLUS_CHG_PROP_VOLTAGE_MIN:
		break;
	case OPLUS_CHG_PROP_CURRENT_MAX:
		if (wls_status->fastchg_started)
			rc = vote(wls_dev->fcc_votable, MAX_VOTER, true, pval->intval / 1000, false);
		else
			rc = 0;
		break;
	case OPLUS_CHG_PROP_CHG_ENABLE:
		wls_dev->charge_enable = !!pval->intval;
		vote(wls_dev->rx_disable_votable, DEBUG_VOTER, !wls_dev->charge_enable, 1, false);
		break;
	case OPLUS_CHG_PROP_TRX_ONLINE:
		if (wls_status->wls_type == OPLUS_CHG_WLS_TRX && !pval->intval)
			wls_status->trx_present_keep = true;
		rc = oplus_chg_wls_set_trx_enable(wls_dev, !!pval->intval);
		break;
#ifdef OPLUS_CHG_DEBUG
	case OPLUS_CHG_PROP_FORCE_TYPE:
		wls_dev->force_type = pval->intval;
		if (wls_dev->force_type == OPLUS_CHG_WLS_FORCE_TYPE_NOEN) {
			wls_dev->debug_mode = false;
			vote(wls_dev->fcc_votable, DEBUG_VOTER, false, 0, false);
		} else if (wls_dev->force_type == OPLUS_CHG_WLS_FORCE_TYPE_AUTO) {
			wls_dev->debug_mode = false;
			vote(wls_dev->fcc_votable, DEBUG_VOTER, false, 0, false);
		} else {
			wls_dev->debug_mode = true;
		}
		break;
	case OPLUS_CHG_PROP_PATH_CTRL:
		rc = oplus_chg_wls_path_ctrl(wls_dev, pval->intval);
		break;
#endif
	case OPLUS_CHG_PROP_STATUS_DELAY:
		break;
	case OPLUS_CHG_PROP_QUIET_MODE:
		if (!wls_status->rx_present ||
		 wls_status->adapter_type == WLS_ADAPTER_TYPE_USB ||
		 wls_status->adapter_type == WLS_ADAPTER_TYPE_NORMAL ||
		 wls_status->adapter_type == WLS_ADAPTER_TYPE_UNKNOWN) {
			chg_err("wls not present or isn't op_tx, can't %s quiet mode\n",
				!!pval->intval ? "enable" : "disable");
			rc = -EINVAL;
			break;
		}
		chg_info("%s quiet mode\n", !!pval->intval ? "enable" : "disable");
		wls_status->switch_quiet_mode = !!pval->intval;
		if (wls_status->switch_quiet_mode != wls_status->quiet_mode
				|| wls_status->quiet_mode_init == false) {
			cancel_delayed_work(&wls_dev->wls_rx_sm_work);
			queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_rx_sm_work, 0);
		}
		break;
	case OPLUS_CHG_PROP_TRX_POWER_EN:
		rc = oplus_chg_wls_nor_set_boost_en(wls_dev->wls_nor, !!pval->intval);
		break;
	case OPLUS_CHG_PROP_TRX_POWER_VOL:
		rc = oplus_chg_wls_nor_set_boost_vol(wls_dev->wls_nor, pval->intval);
		break;
	case OPLUS_CHG_PROP_TRX_POWER_CURR_LIMIT:
		rc = oplus_chg_wls_nor_set_boost_curr_limit(wls_dev->wls_nor, pval->intval);
		break;
	case OPLUS_CHG_PROP_FACTORY_MODE:
		wls_dev->factory_mode = !!pval->intval;
		break;
	case OPLUS_CHG_PROP_FOD_CAL:
		wls_dev->fod_is_cal = false;
		schedule_delayed_work(&wls_dev->fod_cal_work, 0);
		break;
	case OPLUS_CHG_PROP_BATT_CHG_ENABLE:
		wls_dev->batt_charge_enable = !!pval->intval;
		cancel_delayed_work_sync(&wls_dev->wls_rx_sm_work);
		queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_rx_sm_work, 0);
		break;
	case OPLUS_CHG_PROP_COOL_DOWN:
		wls_status->cool_down = pval->intval < 0 ? 0 : pval->intval;
		chg_info("set cool down level to %d\n", wls_status->cool_down);
		oplus_chg_wls_config(wls_dev);
		break;
	case OPLUS_CHG_PROP_RX_VOUT_UVP:
		if (!!pval->intval) {
			vote(wls_dev->nor_input_disable_votable, UOVP_VOTER, true, 0, false);
		} else {
			vote(wls_dev->nor_input_disable_votable, UOVP_VOTER, false, 0, false);
		}
		break;
	case OPLUS_CHG_PROP_FW_UPGRADING:
		break;
	case OPLUS_CHG_PROP_BCC_CURRENT:
		if (wls_dev->support_wls_chg_bcc && pval->intval > 0) {
			wls_status->bcc_current = pval->intval * BCC_TO_ICL;
			chg_info("set bcc current to %d\n", wls_status->bcc_current);
			oplus_chg_wls_config(wls_dev);
		}
		break;
	default:
		chg_err("set prop %d is not supported\n", prop);
		rc = -EINVAL;
		break;
	}

	return rc;
}
#endif /*MAYBE_DELETE*/

#ifdef MAYBE_DELETE
static int oplus_chg_wls_prop_is_writeable(struct oplus_chg_mod *ocm,
				enum oplus_chg_mod_property prop)
{
	switch (prop) {
	case OPLUS_CHG_PROP_VOLTAGE_MAX:
	case OPLUS_CHG_PROP_VOLTAGE_MIN:
	case OPLUS_CHG_PROP_CURRENT_MAX:
	case OPLUS_CHG_PROP_CHG_ENABLE:
	case OPLUS_CHG_PROP_TRX_ONLINE:
#ifdef OPLUS_CHG_DEBUG
	case OPLUS_CHG_PROP_FORCE_TYPE:
	case OPLUS_CHG_PROP_PATH_CTRL:
#endif
	case OPLUS_CHG_PROP_STATUS_DELAY:
	case OPLUS_CHG_PROP_QUIET_MODE:
	case OPLUS_CHG_PROP_FACTORY_MODE:
#ifdef OPLUS_CHG_DEBUG
	case OPLUS_CHG_EXTERN_PROP_UPGRADE_FW:
	case OPLUS_CHG_EXTERN_PROP_PATH_CURRENT:
#endif
	case OPLUS_CHG_PROP_FOD_CAL:
	case OPLUS_CHG_PROP_BATT_CHG_ENABLE:
	case OPLUS_CHG_PROP_COOL_DOWN:
		return 1;
	default:
		break;
	}

	return 0;
}

static const struct oplus_chg_mod_desc oplus_chg_wls_mod_desc = {
	.name = "wireless",
	.type = OPLUS_CHG_MOD_WIRELESS,
	.properties = oplus_chg_wls_props,
	.num_properties = ARRAY_SIZE(oplus_chg_wls_props),
	.uevent_properties = oplus_chg_wls_uevent_props,
	.uevent_num_properties = ARRAY_SIZE(oplus_chg_wls_uevent_props),
	.exten_properties = oplus_chg_wls_exten_props,
	.num_exten_properties = ARRAY_SIZE(oplus_chg_wls_exten_props),
	.get_property = oplus_chg_wls_get_prop,
	.set_property = oplus_chg_wls_set_prop,
	.property_is_writeable	= oplus_chg_wls_prop_is_writeable,
};

static int oplus_chg_wls_event_notifier_call(struct notifier_block *nb,
		unsigned long val, void *v)
{
	struct oplus_chg_wls *wls_dev = container_of(nb, struct oplus_chg_wls, wls_event_nb);
	struct oplus_chg_mod *owner_ocm = v;

	switch(val) {
	case OPLUS_CHG_EVENT_ONLINE:
		if (owner_ocm == NULL) {
			chg_err("This event(=%d) does not support anonymous sending\n",
				val);
			return NOTIFY_BAD;
		}
		if (!strcmp(owner_ocm->desc->name, "usb")) {
			chg_info("usb online\n");
			wls_dev->usb_present = true;
			schedule_delayed_work(&wls_dev->usb_int_work, 0);
			if (wls_dev->wls_ocm)
				oplus_chg_mod_changed(wls_dev->wls_ocm);
		}
		break;
	case OPLUS_CHG_EVENT_OFFLINE:
		if (owner_ocm == NULL) {
			chg_err("This event(=%d) does not support anonymous sending\n",
				val);
			return NOTIFY_BAD;
		}
		if (!strcmp(owner_ocm->desc->name, "usb")) {
			chg_info("usb offline\n");
			wls_dev->usb_present = false;
			schedule_delayed_work(&wls_dev->usb_int_work, 0);
			if (wls_dev->wls_status.upgrade_fw_pending) {
				wls_dev->wls_status.upgrade_fw_pending = false;
				schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, 0);
			}
		}
		break;
	case OPLUS_CHG_EVENT_PRESENT:
		if (owner_ocm == NULL) {
			chg_err("This event(=%d) does not support anonymous sending\n",
				val);
			return NOTIFY_BAD;
		}
		if (!strcmp(owner_ocm->desc->name, "usb")) {
			chg_info("usb present\n");
			wls_dev->usb_present = true;
			schedule_delayed_work(&wls_dev->usb_int_work, 0);
			if (wls_dev->wls_ocm)
				oplus_chg_mod_changed(wls_dev->wls_ocm);
		}
		break;
	case OPLUS_CHG_EVENT_ADSP_STARTED:
		chg_info("adsp started\n");
		adsp_started = true;
		if (online_pending) {
			online_pending = false;
			schedule_delayed_work(&wls_dev->wls_connect_work, 0);
		} else {
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#ifndef CONFIG_OPLUS_CHARGER_MTK
			if (get_boot_mode() != MSM_BOOT_MODE__CHARGE) {
#else
			if (get_boot_mode() != KERNEL_POWER_OFF_CHARGING_BOOT) {
#endif
#endif
				schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, 0);
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
			} else {
				chg_info("check connect\n");
				wls_dev->wls_status.boot_online_keep = true;
				(void)oplus_chg_wls_rx_connect_check(wls_dev->wls_rx);
			}
#endif
		}
		break;
	case OPLUS_CHG_EVENT_OTG_ENABLE:
		vote(wls_dev->wrx_en_votable, OTG_EN_VOTER, true, 1, false);
		break;
	case OPLUS_CHG_EVENT_OTG_DISABLE:
		vote(wls_dev->wrx_en_votable, OTG_EN_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_EVENT_CHARGE_DONE:
		wls_dev->wls_status.chg_done = true;
		break;
	case OPLUS_CHG_EVENT_CLEAN_CHARGE_DONE:
		wls_dev->wls_status.chg_done = false;
		wls_dev->wls_status.chg_done_quiet_mode = false;
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int oplus_chg_wls_mod_notifier_call(struct notifier_block *nb,
		unsigned long val, void *v)
{
	struct oplus_chg_wls *wls_dev = container_of(nb, struct oplus_chg_wls, wls_mod_nb);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	enum oplus_chg_wls_rx_mode rx_mode;
	enum oplus_chg_temp_region temp_region;
	int rc;

	switch(val) {
	case OPLUS_CHG_EVENT_ONLINE:
		chg_info("wls online\n");
		wls_status->boot_online_keep = false;
		if (wls_status->rx_online)
			break;
		wls_status->rx_online = true;
		wls_status->online_keep = false;
		if (adsp_started)
			schedule_delayed_work(&wls_dev->wls_connect_work, 0);
		else
			online_pending = true;
		if (wls_dev->wls_ocm) {
			oplus_chg_global_event(wls_dev->wls_ocm, OPLUS_CHG_EVENT_ONLINE);
			oplus_chg_mod_changed(wls_dev->wls_ocm);
		}
		schedule_delayed_work(&wls_dev->wls_start_work, 0);
		break;
	case OPLUS_CHG_EVENT_OFFLINE:
		chg_info("wls offline\n");
		wls_status->boot_online_keep = false;
		if (!wls_status->rx_online && !wls_status->rx_present)
			break;
		wls_status->rx_present = false;
		wls_status->rx_online = false;
		schedule_delayed_work(&wls_dev->wls_connect_work, 0);
		cancel_delayed_work(&wls_dev->wls_start_work);
		schedule_delayed_work(&wls_dev->wls_break_work, msecs_to_jiffies(OPLUS_CHG_WLS_BREAK_DETECT_DELAY));
		if (wls_dev->wls_ocm) {
			oplus_chg_global_event(wls_dev->wls_ocm, OPLUS_CHG_EVENT_OFFLINE);
			oplus_chg_mod_changed(wls_dev->wls_ocm);
		}
		break;
	case OPLUS_CHG_EVENT_PRESENT:
		chg_info("wls present\n");
		wls_status->boot_online_keep = false;
		if (wls_status->rx_present)
			break;
		wls_status->rx_present = true;
		wls_status->online_keep = false;
		oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor, WLS_VINDPM_BPP);
		if (wls_dev->wls_ocm) {
			oplus_chg_global_event(wls_dev->wls_ocm, OPLUS_CHG_EVENT_PRESENT);
			oplus_chg_mod_changed(wls_dev->wls_ocm);
		}
		break;
	case OPLUS_CHG_EVENT_OP_TRX:
		wls_status->is_op_trx = true;
		break;
	case OPLUS_CHG_EVENT_CHECK_TRX:
		if (wls_status->trx_present) {
			cancel_delayed_work(&wls_dev->wls_trx_sm_work);
			queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_trx_sm_work, 0);
		} else {
			chg_err("trx not present\n");
		}
		break;
	case OPLUS_CHG_EVENT_POWER_CHANGED:
		temp_region = oplus_chg_wls_get_temp_region(wls_dev);
		if ((BATT_TEMP_COOL <= temp_region) && (temp_region <= BATT_TEMP_LITTLE_WARM)) {
			rc = oplus_chg_wls_choose_fastchg_curve(wls_dev);
			if (rc < 0) {
				chg_err("choose fastchg curve failed, rc = %d\n", rc);
			} else {
				wls_status->fastchg_level = 0;
			}
		}
		oplus_chg_wls_config(wls_dev);
		break;
	case OPLUS_CHG_EVENT_LCD_ON:
		chg_info("lcd on\n");
		wls_status->led_on = true;
		break;
	case OPLUS_CHG_EVENT_LCD_OFF:
		chg_info("lcd off\n");
		wls_status->led_on = false;
		break;
	case OPLUS_CHG_EVENT_CALL_ON:
		chg_info("call on\n");
		vote(wls_dev->fcc_votable, CALL_VOTER, true, dynamic_cfg->wls_fast_chg_call_on_curr_ma, false);
		break;
	case OPLUS_CHG_EVENT_CALL_OFF:
		chg_info("call off\n");
		vote(wls_dev->fcc_votable, CALL_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_EVENT_CAMERA_ON:
		chg_info("camera on\n");
		vote(wls_dev->fcc_votable, CAMERA_VOTER, true, dynamic_cfg->wls_fast_chg_camera_on_curr_ma, false);
		break;
	case OPLUS_CHG_EVENT_CAMERA_OFF:
		chg_info("camera off\n");
		vote(wls_dev->fcc_votable, CAMERA_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_EVENT_RX_IIC_ERR:
		if (wls_status->rx_present) {
			chg_info("Restart the rx disable\n");
			wls_dev->wls_status.online_keep = true;
			vote(wls_dev->rx_disable_votable, RX_IIC_VOTER, true, 1, false);
			schedule_delayed_work(&wls_dev->rx_iic_restore_work, msecs_to_jiffies(500));
		}
		break;
	case OPLUS_CHG_EVENT_RX_FAST_ERR:
		if (wls_status->rx_present) {
			chg_info("Wireless fast_chg fault\n");
			schedule_delayed_work(&wls_dev->fast_fault_check_work, 0);
		}
		break;
	case OPLUS_CHG_EVENT_TX_EPP_CAP:
		oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx, &rx_mode);
		if (rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_5W
				|| rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP
				|| rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_PLUS)
			oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor, WLS_VINDPM_EPP);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int oplus_chg_wls_changed_notifier_call(struct notifier_block *nb,
		unsigned long val, void *v)
{
	struct oplus_chg_wls *wls_dev = container_of(nb, struct oplus_chg_wls, wls_changed_nb);
	struct oplus_chg_mod *owner_ocm = v;

	switch(val) {
	case OPLUS_CHG_EVENT_CHANGED:
		if (!strcmp(owner_ocm->desc->name, "wireless") && is_wls_psy_available(wls_dev))
			power_supply_changed(wls_dev->wls_psy);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#endif /*MAYBE_DELETE*/

#ifdef MAYBE_DELETE
static int oplus_chg_wls_init_mod(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_mod_config ocm_cfg = {};
	int rc;

	ocm_cfg.drv_data = wls_dev;
	ocm_cfg.of_node = wls_dev->dev->of_node;

	wls_dev->wls_ocm = oplus_chg_mod_register(wls_dev->dev,
					   &oplus_chg_wls_mod_desc,
					   &ocm_cfg);
	if (IS_ERR(wls_dev->wls_ocm)) {
		chg_err("Couldn't register wls ocm\n");
		rc = PTR_ERR(wls_dev->wls_ocm);
		return rc;
	}
	wls_dev->wls_ocm->notifier = &wls_ocm_notifier;
	wls_dev->wls_mod_nb.notifier_call = oplus_chg_wls_mod_notifier_call;
	rc = oplus_chg_reg_mod_notifier(wls_dev->wls_ocm, &wls_dev->wls_mod_nb);
	if (rc) {
		chg_err("register wls mod notifier error, rc=%d\n", rc);
		goto reg_wls_mod_notifier_err;
	}
	wls_dev->wls_event_nb.notifier_call = oplus_chg_wls_event_notifier_call;
	rc = oplus_chg_reg_event_notifier(&wls_dev->wls_event_nb);
	if (rc) {
		chg_err("register wls event notifier error, rc=%d\n", rc);
		goto reg_wls_event_notifier_err;
	}
	wls_dev->wls_changed_nb.notifier_call = oplus_chg_wls_changed_notifier_call;
	rc = oplus_chg_reg_changed_notifier(&wls_dev->wls_changed_nb);
	if (rc) {
		chg_err("register wls changed notifier error, rc=%d\n", rc);
		goto reg_wls_changed_notifier_err;
	}

	return 0;

reg_wls_changed_notifier_err:
	oplus_chg_unreg_event_notifier(&wls_dev->wls_event_nb);
reg_wls_event_notifier_err:
	oplus_chg_unreg_mod_notifier(wls_dev->wls_ocm, &wls_dev->wls_mod_nb);
reg_wls_mod_notifier_err:
	oplus_chg_mod_unregister(wls_dev->wls_ocm);
	return rc;
}
#endif /*MAYBE_DELETE*/

static int oplus_chg_wls_wireless_notifier_call(struct oplus_chg_wls *wls_dev, enum wls_topic_item id)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	union mms_msg_data data = { 0 };

	switch (id) {
	case WLS_ITEM_PRESENT:
		oplus_mms_get_item_data(wls_dev->wls_topic, WLS_ITEM_PRESENT, &data, true);
		if (data.intval == 0)
			return 0;
		chg_info("wls present\n");
		wls_status->boot_online_keep = false;
		if (wls_status->rx_present)
			break;
		wls_status->rx_present = true;
		wls_status->online_keep = false;
		oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_BPP);
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
		break;
	case WLS_ITEM_ONLINE:
		oplus_mms_get_item_data(wls_dev->wls_topic, WLS_ITEM_ONLINE, &data, true);
		if (data.intval) {
			chg_info("wls online\n");
			wls_status->boot_online_keep = false;
			if (wls_status->rx_online)
				break;
			wls_status->rx_online = true;
			wls_status->online_keep = false;
			/*if (adsp_started)*/
				schedule_delayed_work(&wls_dev->wls_connect_work, 0);
			/*else
				online_pending = true;*/
			if (is_batt_psy_available(wls_dev))
				power_supply_changed(wls_dev->batt_psy);
			schedule_delayed_work(&wls_dev->wls_start_work, 0);
		} else {
			chg_info("wls offline\n");
			wls_status->boot_online_keep = false;
			if (!wls_status->rx_online && !wls_status->rx_present)
				break;
			wls_status->rx_present = false;
			wls_status->rx_online = false;
			schedule_delayed_work(&wls_dev->wls_connect_work, 0);
			cancel_delayed_work(&wls_dev->wls_start_work);
			schedule_delayed_work(&wls_dev->wls_break_work,
				msecs_to_jiffies(OPLUS_CHG_WLS_BREAK_DETECT_DELAY));
			if (is_batt_psy_available(wls_dev))
				power_supply_changed(wls_dev->batt_psy);
		}
		break;
	default:
		chg_info("unknown wls_topic_item[%d]\n", id);
		break;
	}
	return 0;
}

static void oplus_chg_wls_usb_int_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, usb_int_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (wls_dev->usb_present) {
		vote(wls_dev->rx_disable_votable, USB_VOTER, true, 1, false);
		if (!wls_dev->support_tx_boost && !wls_dev->support_wls_and_tx_boost)
			(void)oplus_chg_wls_set_trx_enable(wls_dev, false);
		/*oplus_chg_anon_mod_event(wls_dev->wls_ocm, OPLUS_CHG_EVENT_OFFLINE);*/
		if (wls_status->trx_present && wls_dev->support_wls_and_tx_boost) {
			if (wls_status->wls_type == OPLUS_CHG_WLS_TRX)
				wls_status->trx_present_keep = true;
			(void)oplus_chg_wls_set_trx_enable(wls_dev, false);
			/*for HW spec,need 100ms delay*/
			msleep(100);
			(void)oplus_chg_wls_set_trx_enable(wls_dev, true);
		}
	} else {
		vote(wls_dev->rx_disable_votable, USB_VOTER, false, 0, false);
	}
}

#define OPLUS_CHG_WLS_START_DETECT_CNT 10
static void oplus_chg_wls_start_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_start_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	static int wpc_online_cnt;

	if (wls_status->rx_online) {
		wpc_online_cnt++;
		if (wpc_online_cnt >= OPLUS_CHG_WLS_START_DETECT_CNT) {
			wpc_online_cnt = 0;
			oplus_chg_wls_update_track_info(wls_dev, NULL, false);
			chg_info("wireless chg start after connect 30s\n");
		} else {
			schedule_delayed_work(&wls_dev->wls_start_work,
				round_jiffies_relative(msecs_to_jiffies(OPLUS_CHG_WLS_START_DETECT_DELAY)));
		}
	} else {
		chg_info("wireless chg not online within connect 30s, cnt=%d\n", wpc_online_cnt);
		wpc_online_cnt = 0;
	}
}

static void oplus_chg_wls_break_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_break_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (wls_status->rx_online) {
		wls_status->break_count++;
		chg_info("wireless disconnect less than 6s, count=%d\n", wls_status->break_count);
	} else {
		chg_info("wireless disconnect more than 6s, charging stop\n");
		wls_status->break_count = 0;
		oplus_chg_wls_update_track_info(wls_dev, NULL, true);
	}
}

static void oplus_chg_wls_connect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_connect_work);
	/*struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;*/
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_wls_chg_rx *wls_rx = wls_dev->wls_rx;
	struct oplus_wls_chg_normal *wls_nor = wls_dev->wls_nor;
	unsigned long delay_time = jiffies + msecs_to_jiffies(500);
	int skin_temp;
	int rc;

	if (wls_status->rx_online) {
		chg_err("!!!!!wls connect >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		if (!wls_dev->rx_wake_lock_on) {
			chg_info("acquire rx_wake_lock\n");
			__pm_stay_awake(wls_dev->rx_wake_lock);
			wls_dev->rx_wake_lock_on = true;
		} else {
			chg_err("rx_wake_lock is already stay awake\n");
		}

		cancel_delayed_work_sync(&wls_dev->wls_clear_trx_work);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		vote(wls_dev->wrx_en_votable, WLS_ONLINE_VOTER, true, 1, false);
#endif
		oplus_chg_wls_reset_variables(wls_dev);
		/*
		 * The verity_state_keep flag needs to be cleared immediately
		 * after reconnection to ensure that the subsequent verification
		 * function is normal.
		 */
		wls_status->verity_state_keep = false;
		chg_info("nor_fcc_votable: client:%s, result=%d\n",
			get_effective_client(wls_dev->nor_fcc_votable),
			get_effective_result(wls_dev->nor_fcc_votable));
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 1500, false);
		rerun_election(wls_dev->nor_fcc_votable, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 100, false);
		rerun_election(wls_dev->nor_icl_votable, false);
		/* reset charger status */
		vote(wls_dev->nor_out_disable_votable, USER_VOTER, true, 1, false);
		vote(wls_dev->nor_out_disable_votable, USER_VOTER, false, 0, false);
		schedule_delayed_work(&wls_dev->wls_data_update_work, 0);
		schedule_delayed_work(&wls_dev->wls_vout_err_work, 0);
		queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_rx_sm_work, 0);
		rc = oplus_chg_wls_get_skin_temp(wls_dev, &skin_temp);
		if (rc < 0) {
			chg_err("can't get skin temp, rc=%d\n", rc);
			skin_temp = 250;
		}
	} else {
		chg_err("!!!!!wls disconnect <<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		vote(wls_dev->rx_disable_votable, CONNECT_VOTER, true, 1, false);
		/* should clean all resource first after disconnect */
		(void)oplus_chg_wls_clean_msg(wls_dev);
		(void)oplus_chg_wls_rx_clean_source(wls_rx);
		(void)oplus_chg_wls_nor_clean_source(wls_nor);
		cancel_delayed_work_sync(&wls_dev->wls_data_update_work);
		cancel_delayed_work_sync(&wls_dev->wls_rx_sm_work);
		cancel_delayed_work_sync(&wls_dev->wls_trx_sm_work);
		cancel_delayed_work_sync(&wls_dev->wls_verity_work);
		cancel_delayed_work_sync(&wls_dev->wls_skewing_work);
		cancel_delayed_work_sync(&wls_dev->wls_vout_err_work);
		oplus_chg_wls_cancel_bcc_update_work(wls_dev);
		if (wls_dev->support_fastchg) {
			(void)oplus_chg_wls_rx_set_dcdc_enable(wls_dev->wls_rx->rx_ic, false);
			msleep(10);
			oplus_chg_wls_set_ext_pwr_enable(wls_dev, false);
			oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
#ifdef CONFIG_OPLUS_CHARGER_MTK
			msleep(100);
			vote(wls_dev->wrx_en_votable, WLS_ONLINE_VOTER, false, 0, false);
#endif
		}
		oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_DEFAULT);
		oplus_chg_wls_reset_variables(wls_dev);
		if (time_is_after_jiffies(delay_time)) {
			delay_time = delay_time - jiffies;
			msleep(jiffies_to_msecs(delay_time));
		}
		vote(wls_dev->rx_disable_votable, CONNECT_VOTER, false, 0, false);
		oplus_chg_wls_reset_variables(wls_dev);
		if (wls_status->online_keep) {
			schedule_delayed_work(&wls_dev->online_keep_remove_work, msecs_to_jiffies(2000));
		} else {
			if (wls_dev->rx_wake_lock_on) {
				chg_info("release rx_wake_lock\n");
				__pm_relax(wls_dev->rx_wake_lock);
				wls_dev->rx_wake_lock_on = false;
			} else {
				chg_err("rx_wake_lock is already relax\n");
			}
		}
	}
}

static int oplus_chg_wls_nor_skin_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_skin_step *skin_step;
	int *skin_step_count;
	int skin_temp;
	int curr_ma;
	int rc;

	rc = oplus_chg_wls_get_skin_temp(wls_dev, &skin_temp);
	if (rc < 0) {
		chg_err("can't get skin temp, rc=%d\n", rc);
		return rc;
	}

	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_BPP:
		skin_step = &wls_dev->bpp_skin_step;
		skin_step_count = &wls_status->bpp_skin_step;
		break;
	case OPLUS_CHG_WLS_EPP:
		skin_step = &wls_dev->epp_skin_step;
		skin_step_count = &wls_status->epp_skin_step;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		skin_step = &wls_dev->epp_plus_skin_step;
		skin_step_count = &wls_status->epp_plus_skin_step;
		break;
	case OPLUS_CHG_WLS_VOOC:
	case OPLUS_CHG_WLS_SVOOC:
	case OPLUS_CHG_WLS_PD_65W:
		skin_step = &wls_dev->epp_plus_skin_step;
		skin_step_count = &wls_status->epp_plus_skin_step;
		break;
	default:
		chg_err("Unsupported charging mode(=%d)\n", wls_status->wls_type);
		return -EINVAL;
	}

	if (skin_step->max_step == 0)
		return 0;

	if ((skin_temp < skin_step->skin_step[*skin_step_count].low_threshold) &&
	    (*skin_step_count > 0)) {
		chg_info("skin_temp=%d, switch to the previous gear\n", skin_temp);
		*skin_step_count = *skin_step_count - 1;
	} else if ((skin_temp > skin_step->skin_step[*skin_step_count].high_threshold) &&
		   (*skin_step_count < (skin_step->max_step - 1))) {
		chg_info("skin_temp=%d, switch to the next gear\n", skin_temp);
		*skin_step_count = *skin_step_count + 1;
	}
	curr_ma = skin_step->skin_step[*skin_step_count].curr_ma;
	vote(wls_dev->nor_icl_votable, SKIN_VOTER, true, curr_ma, true);

	return 0;
}

static void oplus_chg_wls_fast_fcc_param_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_fcc_step *fcc_chg = &wls_dev->wls_fcc_step;
	int i;

	for(i = 0; i < fcc_chg->max_step; i++) {
		if (fcc_chg->fcc_step[i].low_threshold > 0)
			fcc_chg->allow_fallback[i] = true;
		else
			fcc_chg->allow_fallback[i] = false;
	}
}

static void oplus_chg_wls_fast_switch_next_step(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_fcc_step *fcc_chg = &wls_dev->wls_fcc_step;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	enum oplus_chg_temp_region temp_region;
	u32 batt_vol_max = fcc_chg->fcc_step[wls_status->fastchg_level].vol_max_mv;
	int batt_vol_mv;
	int batt_temp;
	int rc;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);
	rc = oplus_chg_wls_get_vbat(wls_dev, &batt_vol_mv);
	if (rc < 0) {
		chg_err("can't get batt vol, rc=%d\n", rc);
		return;
	}
	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return;
	}

	if (fcc_chg->fcc_step[wls_status->fastchg_level].need_wait == 0) {
		if (batt_vol_mv >= batt_vol_max) {
			/* Must delay 1 sec and wait for the batt voltage to drop */
			fcc_chg->fcc_wait_timeout = jiffies + HZ * 5;
		} else {
			fcc_chg->fcc_wait_timeout = jiffies;
		}
	} else {
		/* Delay 1 minute and wait for the temperature to drop */
		fcc_chg->fcc_wait_timeout = jiffies + HZ * 60;
	}

	wls_status->fastchg_level++;
	chg_info("switch to next level=%d\n", wls_status->fastchg_level);
	if (wls_status->fastchg_level >= fcc_chg->max_step) {
		if (batt_vol_mv >= batt_vol_max) {
			chg_info("run normal charge ffc\n");
			wls_status->ffc_check = true;
			/*oplus_chg_wls_track_set_prop(
				wls_dev, OPLUS_CHG_PROP_STATUS,
				TRACK_WLS_FASTCHG_FULL);*/
		}
	} else {
		wls_status->wait_cep_stable = true;
		vote(wls_dev->fcc_votable, FCC_VOTER, true,
		     fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma, false);
	}
	wls_status->fastchg_level_init_temp = batt_temp;
	if (batt_vol_mv >= batt_vol_max) {
		if (wls_status->fastchg_level < fcc_chg->max_step)
			fcc_chg->allow_fallback[wls_status->fastchg_level] = false;
	}
}

static void oplus_chg_wls_fast_switch_prev_step(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_fcc_step *fcc_chg = &wls_dev->wls_fcc_step;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	wls_status->fastchg_level--;
	chg_info("switch to prev level=%d\n", wls_status->fastchg_level);
	vote(wls_dev->fcc_votable, FCC_VOTER, true,
	     fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma, false);
	wls_status->fastchg_level_init_temp = 0;
	fcc_chg->fcc_wait_timeout = jiffies;
}

static int oplus_chg_wls_fast_temp_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_fcc_step *fcc_chg = &wls_dev->wls_fcc_step;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	enum oplus_chg_temp_region temp_region;
	int batt_temp;
	int batt_vol_mv;
	int def_curr_ma;
	int fcc_curr_ma;
	int next_curr_ma;
	/*
	 * We want the temperature to drop when switching to a lower current range.
	 * If the temperature rises by 2 degrees before the next gear begins to
	 * detect temperature, then you should immediately switch to a lower gear.
	 */
	int temp_diff;
	u32 batt_vol_max = fcc_chg->fcc_step[wls_status->fastchg_level].vol_max_mv;
	int rc;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	if ((temp_region < BATT_TEMP_COOL) || (temp_region > BATT_TEMP_LITTLE_WARM)) {
		chg_info("Abnormal battery temperature, exit fast charge\n");
		vote(wls_dev->fastchg_disable_votable, FCC_VOTER, true, 1, false);
		return -EPERM;
	}
	rc = oplus_chg_wls_get_vbat(wls_dev, &batt_vol_mv);
	if (rc < 0) {
		chg_err("can't get batt vol, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return rc;
	}
	def_curr_ma = get_client_vote(wls_dev->fcc_votable, JEITA_VOTER);
	if (def_curr_ma <= 0)
		def_curr_ma = get_client_vote(wls_dev->fcc_votable, MAX_VOTER);
	else
		def_curr_ma = min(get_client_vote(wls_dev->fcc_votable, MAX_VOTER), def_curr_ma);
	chg_info("wkcs: def_curr_ma=%d, max_step=%d\n", def_curr_ma, fcc_chg->max_step);
	fcc_curr_ma = fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma;
	if (wls_status->fastchg_level + 1 < fcc_chg->max_step)
		next_curr_ma = fcc_chg->fcc_step[wls_status->fastchg_level + 1].curr_ma;
	else
		next_curr_ma = 0;
	if (wls_status->fastchg_level_init_temp != 0)
		temp_diff = batt_temp - wls_status->fastchg_level_init_temp;
	else
		temp_diff = 0;

	chg_info("battery temp = %d, vol = %d, level = %d, temp_diff = %d, fcc_curr_ma=%d\n",
		batt_temp, batt_vol_mv, wls_status->fastchg_level, temp_diff, fcc_curr_ma);

	if (wls_status->fastchg_level == 0) {
		if ((batt_temp > fcc_chg->fcc_step[wls_status->fastchg_level].high_threshold) ||
		    (batt_vol_mv >= batt_vol_max)) {
			oplus_chg_wls_fast_switch_next_step(wls_dev);
			return 0;
		}
		vote(wls_dev->fcc_votable, FCC_VOTER, true,
			fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma, false);
	} else if (wls_status->fastchg_level >= fcc_chg->max_step) {  /*switch to pmic*/
		vote(wls_dev->fastchg_disable_votable, FCC_VOTER, true, 1, false);
		return -EPERM;
	} else {
		if (batt_vol_mv >= dynamic_cfg->batt_vol_max_mv) {
			chg_info("batt voltage too high, switch next step\n");
			oplus_chg_wls_fast_switch_next_step(wls_dev);
			return 0;
		}
		if ((batt_temp < fcc_chg->fcc_step[wls_status->fastchg_level].low_threshold) &&
		    fcc_chg->allow_fallback[wls_status->fastchg_level] &&
		    (def_curr_ma > fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma)) {
			chg_info("target current too low, switch next step\n");
			oplus_chg_wls_fast_switch_prev_step(wls_dev);
			return 0;
		}
		chg_info("jiffies=%lu, timeout=%lu, high_threshold=%d, batt_vol_max=%d\n",
			 jiffies, fcc_chg->fcc_wait_timeout,
			 fcc_chg->fcc_step[wls_status->fastchg_level].high_threshold,
			 batt_vol_max);
		if (time_after(jiffies, fcc_chg->fcc_wait_timeout) || (temp_diff > 200) ||
		    (get_effective_result(wls_dev->fcc_votable) <= next_curr_ma)) {
			if ((batt_temp > fcc_chg->fcc_step[wls_status->fastchg_level].high_threshold) ||
			    (batt_vol_mv >= batt_vol_max)) {
				oplus_chg_wls_fast_switch_next_step(wls_dev);
			}
		}
	}

	return 0;
}

#define CEP_ERR_MAX 12/*(3*2s=12*500ms)*/
#define CEP_OK_MAX 10
#define CEP_WAIT_MAX 40
#define CEP_OK_TIMEOUT_MAX 30
static void oplus_chg_wls_fast_cep_check(struct oplus_chg_wls *wls_dev, int cep)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int curr_ma, cep_curr_ma;
	static int wait_cep_count;
	static int cep_err_count;
	static int cep_ok_count;

	if (!wls_status->wait_cep_stable) {
		/* Insufficient energy only when CEP is positive */
		if (cep < 3) {
			cep_ok_count++;
			cep_err_count = 0;
			if ((cep_ok_count >= CEP_OK_MAX) &&
			    time_after(jiffies, wls_status->cep_ok_wait_timeout) &&
			    is_client_vote_enabled(wls_dev->fcc_votable, CEP_VOTER)) {
				chg_info("skewing: recovery charging current\n");
				cep_ok_count = 0;
				wls_status->wait_cep_stable = true;
				wls_status->cep_ok_wait_timeout = jiffies + CEP_OK_TIMEOUT_MAX * HZ;
				wait_cep_count = 0;
				vote(wls_dev->fcc_votable, CEP_VOTER, false, 0, false);
			}
		} else {
			cep_ok_count = 0;
			cep_err_count++;
			if (cep_err_count >= CEP_ERR_MAX) {
				chg_info("skewing: reduce charging current\n");
				cep_err_count = 0;
				wls_status->wait_cep_stable = true;
				wait_cep_count = 0;
				if (is_client_vote_enabled(wls_dev->fcc_votable, CEP_VOTER))
					cep_curr_ma = get_client_vote(wls_dev->fcc_votable, CEP_VOTER);
				else
					cep_curr_ma = 0;
				if ((cep_curr_ma > 0) && (cep_curr_ma <= WLS_FASTCHG_CURR_MIN_MA)) {
					chg_info("skewing: Energy is too low, exit fast charge\n");
					vote(wls_dev->fastchg_disable_votable, CEP_VOTER, true, 1, false);
					wls_status->cep_ok_wait_timeout = jiffies + CEP_OK_TIMEOUT_MAX * HZ;
					wls_status->fast_cep_check = -1;
					return;
				} else {
					curr_ma = wls_status->iout_ma;
					/* Target current is adjusted in 50ma steps*/
					curr_ma = curr_ma - (curr_ma % WLS_FASTCHG_CURR_ERR_MA) - WLS_FASTCHG_CURR_ERR_MA;
					if (curr_ma < WLS_FASTCHG_CURR_MIN_MA)
						curr_ma = WLS_FASTCHG_CURR_MIN_MA;
					vote(wls_dev->fcc_votable, CEP_VOTER, true, curr_ma, false);
				}
				wls_status->cep_ok_wait_timeout = jiffies + CEP_OK_TIMEOUT_MAX * HZ;
			}
		}
	} else {
		if (wait_cep_count < CEP_WAIT_MAX) {
			wait_cep_count++;
		} else {
			wls_status->wait_cep_stable = false;
			wait_cep_count = 0;
			cep_err_count = 0;
			cep_ok_count = 0;
		}
	}

	return;
}

static int oplus_chg_wls_fast_ibat_check(struct oplus_chg_wls *wls_dev)
{
	int ibat_ma = 0;
	int rc;

	rc = oplus_chg_wls_get_ibat(wls_dev, &ibat_ma);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		return rc;
	}

	if (ibat_ma >= WLS_FASTCHG_CURR_DISCHG_MAX_MA) {
		chg_err("discharge current is too large, exit fast charge\n");
		vote(wls_dev->fastchg_disable_votable, BATT_CURR_VOTER, true, 1, false);
		return -1;
	}

	return 0;
}

#define IOUT_SMALL_COUNT 30
static int oplus_chg_wls_fast_iout_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int iout_ma = 0;

	iout_ma = wls_status->iout_ma;

	if (iout_ma <= WLS_FASTCHG_IOUT_CURR_MIN_MA) {
		wls_status->iout_ma_conunt++;
		if (wls_status->iout_ma_conunt >= IOUT_SMALL_COUNT) {
			chg_err("iout current is too small, exit fast charge\n");
			vote(wls_dev->fastchg_disable_votable, IOUT_CURR_VOTER, true, 1, false);
			return -1;
		}
	} else {
		wls_status->iout_ma_conunt = 0;
	}
	return 0;
}

static void oplus_chg_wls_fast_skin_temp_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int skin_temp;
	/*int curr_ma;*/
	int rc;

	if (wls_dev->factory_mode) {
		vote(wls_dev->fcc_votable, SKIN_VOTER, false, 0, false);
		return;
	}

	rc = oplus_chg_wls_get_skin_temp(wls_dev, &skin_temp);
	if (rc < 0) {
		chg_err("can't get skin temp, rc=%d\n", rc);
		skin_temp = 250;
	}

	if (wls_status->led_on) {
		vote(wls_dev->fcc_votable, SKIN_VOTER, false, 0, false);
	} else {
		/*if (wls_dev->wls_fast_chg_led_off_strategy.initialized) {
			curr_ma = oplus_chg_strategy_get_data(&wls_dev->wls_fast_chg_led_off_strategy,
				&wls_dev->wls_fast_chg_led_off_strategy.temp_region, skin_temp);
			chg_info("led is off, curr = %d\n", curr_ma);
			vote(wls_dev->fcc_votable, SKIN_VOTER, true, curr_ma, false);
		}*/
	}
}

static int oplus_chg_wls_fast_cool_down_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int cool_down = wls_status->cool_down;

	if (!wls_dev->factory_mode) {
		if (cool_down > 0 && cool_down <= dynamic_cfg->cool_down_12v_thr) {
			chg_err("cool_down level < %d, exit fast charge\n", dynamic_cfg->cool_down_12v_thr);
			vote(wls_dev->fastchg_disable_votable, COOL_DOWN_VOTER, true, 1, false);
			return -1;
		}
	}

	return 0;
}

static void oplus_chg_wls_set_quiet_mode(struct oplus_chg_wls *wls_dev, bool quiet_mode)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int fan_pwm_pulse_fastchg = 0;
	int fan_pwm_pulse_silent = 0;

	if (wls_status->adapter_id == WLS_ADAPTER_MODEL_1) {
		fan_pwm_pulse_fastchg = FAN_PWM_PULSE_IN_FASTCHG_MODE_V01;
		fan_pwm_pulse_silent = FAN_PWM_PULSE_IN_SILENT_MODE_V01;
	} else if (wls_status->adapter_id == WLS_ADAPTER_MODEL_2) {
		fan_pwm_pulse_fastchg = FAN_PWM_PULSE_IN_FASTCHG_MODE_V02;
		fan_pwm_pulse_silent = FAN_PWM_PULSE_IN_SILENT_MODE_V02;
	} else if (wls_status->adapter_id <= WLS_ADAPTER_MODEL_7) {
		fan_pwm_pulse_fastchg = FAN_PWM_PULSE_IN_FASTCHG_MODE_V03_07;
		fan_pwm_pulse_silent = FAN_PWM_PULSE_IN_SILENT_MODE_V03_07;
	} else if (wls_status->adapter_id <= WLS_ADAPTER_MODEL_15) {
		fan_pwm_pulse_fastchg = FAN_PWM_PULSE_IN_FASTCHG_MODE_V08_15;
		fan_pwm_pulse_silent = FAN_PWM_PULSE_IN_SILENT_MODE_V08_15;
	} else {
		fan_pwm_pulse_fastchg = FAN_PWM_PULSE_IN_FASTCHG_MODE_DEFAULT;
		fan_pwm_pulse_silent = FAN_PWM_PULSE_IN_SILENT_MODE_DEFAULT;
	}

	if (wls_status->adapter_id == WLS_ADAPTER_MODEL_0)
		(void)oplus_chg_wls_send_msg(wls_dev, quiet_mode ?
			WLS_CMD_SET_QUIET_MODE : WLS_CMD_SET_NORMAL_MODE, 0xff, 2);
	else
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_SET_FAN_SPEED,
			quiet_mode ? fan_pwm_pulse_silent : fan_pwm_pulse_fastchg, 2);

	(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_SET_LED_BRIGHTNESS,
		quiet_mode ? QUIET_MODE_LED_BRIGHTNESS : 100, 2);
}

static void oplus_chg_wls_check_quiet_mode(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (wls_status->charge_type != WLS_CHARGE_TYPE_FAST)
		return;

	if (wls_status->switch_quiet_mode) {
		if ((!wls_status->quiet_mode || wls_status->quiet_mode_init == false)
				&& !wls_status->chg_done_quiet_mode)
			(void)oplus_chg_wls_set_quiet_mode(wls_dev, wls_status->switch_quiet_mode);
	} else {
		if (wls_status->quiet_mode || wls_status->quiet_mode_init == false)
			(void)oplus_chg_wls_set_quiet_mode(wls_dev, wls_status->switch_quiet_mode);
	}
}

static void oplus_chg_wls_check_term_charge(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int curr_ma;
	int skin_temp;
	int rc;

	if (!wls_status->cep_timeout_adjusted &&
	    !wls_status->fastchg_started &&
	    (wls_status->charge_type == WLS_CHARGE_TYPE_FAST) &&
	    ((wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_EPP) ||
	     (wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_EPP_PLUS) ||
	     (wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_FFC) ||
	     (wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_DONE))) {
		oplus_chg_wls_send_msg(wls_dev, WLS_CMD_SET_CEP_TIMEOUT, 0xff, 2);
	}
	curr_ma = get_client_vote(wls_dev->nor_icl_votable, CHG_DONE_VOTER);
	rc = oplus_chg_wls_get_skin_temp(wls_dev, &skin_temp);
	if (rc < 0) {
		chg_err("can't get skin temp, rc=%d\n", rc);
		skin_temp = 250;
	}

	if (wls_status->chg_done) {
		if (curr_ma <= 0) {
			chg_info("chg done, set icl to %dma\n", WLS_CURR_STOP_CHG_MA);
			vote(wls_dev->nor_icl_votable, CHG_DONE_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
		}
		if (skin_temp < CHARGE_FULL_FAN_THREOD_LO)
			wls_status->chg_done_quiet_mode = true;
		else if (skin_temp > CHARGE_FULL_FAN_THREOD_HI)
			wls_status->chg_done_quiet_mode = false;
	} else {
		if (curr_ma > 0) {
			vote(wls_dev->nor_icl_votable, CHG_DONE_VOTER, false, 0, true);
		}
		wls_status->chg_done_quiet_mode = false;
	}
}

static int oplus_chg_wls_fastchg_restart_check(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	enum oplus_chg_temp_region temp_region;
	int batt_temp;
	int real_soc = 100;
	int ibat_ma = 0;
	int rc;

	if (!wls_status->fastchg_disable || wls_status->switch_quiet_mode ||
	    !wls_dev->batt_charge_enable)
		return -EPERM;

	rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
	if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
		chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
		return -EPERM;
	}

	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0 || batt_temp >= dynamic_cfg->fastchg_max_temp) {
		chg_err("can't get batt temp, or batt_temp is too high rc=%d\n", rc);
		return -EPERM;
	}

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);
	if ((temp_region < BATT_TEMP_COOL) || (temp_region > BATT_TEMP_LITTLE_WARM)) {
		chg_info("Abnormal battery temperature, can not restart fast charge\n");
		return -EPERM;
	}

	rc = oplus_chg_wls_get_ibat(wls_dev, &ibat_ma);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		return rc;
	}

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, RX_FAST_ERR_VOTER) &&
		(wls_status->fastchg_err_count < 2) &&
		time_is_before_jiffies(wls_status->fastchg_err_timer))
		vote(wls_dev->fastchg_disable_votable, RX_FAST_ERR_VOTER, false, 0, false);

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, FCC_VOTER)) {
		vote(wls_dev->fastchg_disable_votable, FCC_VOTER, false, 0, false);
	}

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, BATT_CURR_VOTER) && (ibat_ma < 0))
		vote(wls_dev->fastchg_disable_votable, BATT_CURR_VOTER, false, 0, false);


	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, STARTUP_CEP_VOTER) &&
	    (wls_status->fastchg_retry_count < 10) &&
	    time_is_before_jiffies(wls_status->fastchg_retry_timer))
		vote(wls_dev->fastchg_disable_votable, STARTUP_CEP_VOTER, false, 0, false);

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, QUIET_VOTER))
                vote(wls_dev->fastchg_disable_votable, QUIET_VOTER, false, 0, false);

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, COOL_DOWN_VOTER)) {
		if ((dynamic_cfg->cool_down_12v_thr > 0 && wls_status->cool_down > dynamic_cfg->cool_down_12v_thr) ||
		    wls_status->cool_down == 0)
			vote(wls_dev->fastchg_disable_votable, COOL_DOWN_VOTER, false, 0, false);
	}

	return 0;
}

#define FALLBACK_COUNT_MAX	3
static int oplus_chg_wls_set_non_ffc_current(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	enum oplus_chg_temp_region temp_region;
	static u32 fallback_count = 0;
	int batt_temp = 0;
	int batt_vol_mv = 0;
	int cv_icl = 0;
	int non_ffc_max_step = 0;
	int rc = 0;
	int i = 0;
	int j = 0;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	rc = oplus_chg_wls_get_vbat(wls_dev, &batt_vol_mv);
	if (rc < 0) {
		chg_err("can't get batt vol, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return rc;
	}

	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_BPP:
		i = OPLUS_WLS_CHG_MODE_BPP;
		break;
	case OPLUS_CHG_WLS_EPP:
		i = OPLUS_WLS_CHG_MODE_EPP;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		i = OPLUS_WLS_CHG_MODE_EPP_PLUS;
		break;
	case OPLUS_CHG_WLS_VOOC:
		i = OPLUS_WLS_CHG_MODE_AIRVOOC;
		break;
	case OPLUS_CHG_WLS_SVOOC:
	case OPLUS_CHG_WLS_PD_65W:
		i = OPLUS_WLS_CHG_MODE_AIRSVOOC;
		break;
	default:
		i = OPLUS_WLS_CHG_MODE_BPP;
		break;
	}

	non_ffc_max_step = wls_dev->non_ffc_step[i].max_step;

	for (j = 0; j < non_ffc_max_step; j++) {
		if (batt_vol_mv < dynamic_cfg->non_ffc_step[i][j].vol_max_mv) {
			if (wls_status->non_fcc_level < j)
				wls_status->non_fcc_level = j;
			break;
		}
	}
	if (dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma > 0) {
		if ((wls_status->non_fcc_level == non_ffc_max_step - 1) && (wls_status->non_fcc_level > 0)) {
			if (batt_vol_mv <= dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level - 1].vol_max_mv - 50)
				fallback_count++;
			else
				fallback_count = 0;
			if (fallback_count >= FALLBACK_COUNT_MAX) {
				wls_status->non_fcc_level = non_ffc_max_step - 2;
				vote(wls_dev->nor_icl_votable, NON_FFC_VOTER, true,
					dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma, true);
				chg_info("fallback: non_ffc_icl=%d, j=%d, level=%d\n",
					dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma,
					j, wls_status->non_fcc_level);
			} else {
				vote(wls_dev->nor_icl_votable, NON_FFC_VOTER, true,
					dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma, true);
				chg_info("non_ffc_icl=%d, j=%d, level=%d\n",
					dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma,
					j, wls_status->non_fcc_level);
			}
		} else {
			fallback_count = 0;
			vote(wls_dev->nor_icl_votable, NON_FFC_VOTER, true,
				dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma, true);
			chg_info("non_ffc_icl=%d, j=%d, level=%d\n",
				dynamic_cfg->non_ffc_step[i][wls_status->non_fcc_level].curr_ma,
				j, wls_status->non_fcc_level);
		}
	}

	for (j = 0; j < wls_dev->cv_step[i].max_step; j++) {
		cv_icl = dynamic_cfg->cv_step[i][j].curr_ma;
		if (temp_region < BATT_TEMP_WARM)
			break;
	}
	if (j > wls_dev->cv_step[i].max_step - 1)
		j = wls_dev->cv_step[i].max_step - 1;
	if (batt_vol_mv >= dynamic_cfg->cv_step[i][j].vol_max_mv && cv_icl > 0) {
		vote(wls_dev->nor_icl_votable, CV_VOTER, true, cv_icl, true);
	}
	if (batt_vol_mv <= dynamic_cfg->cv_step[i][j].vol_max_mv - 50) {
		vote(wls_dev->nor_icl_votable, CV_VOTER, false, 0, false);
	}

	return 0;
}

static int oplus_chg_wls_get_third_adapter_ext_cmd_p_id(struct oplus_chg_wls *wls_dev)
{
	int rc = 0;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int try_count = 0;
	char buf[3] = {0};
	int soc = 0, temp = 0;

	if (wls_status->adapter_id != WLS_ADAPTER_THIRD_PARTY)
		return rc;

	if (!wls_status->tx_extern_cmd_done) {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_EXTERN_CMD, 0xff, 5);
		if (rc < 0) {
			chg_err("can't extern cmd, rc=%d\n", rc);
			wls_status->tx_extern_cmd_done = false;
			return rc;
		}
		msleep(200);
	}

	if (!wls_status->tx_product_id_done) {
		buf[0] = (wls_dev->wls_phone_id >> 8) & 0xff;
		buf[1] = wls_dev->wls_phone_id & 0xff;
		chg_info("wls_phone_id=0x%x\n", wls_dev->wls_phone_id);
		do {
			rc = oplus_chg_wls_send_data(wls_dev, WLS_CMD_GET_PRODUCT_ID, buf, 5);
			if (rc < 0) {
				if (rc != -EAGAIN)
					try_count++;
				msleep(200);
			}
		} while (rc < 0 && try_count < 2 && wls_status->rx_online);
		if (rc < 0 || !wls_status->rx_online) {
			chg_err("can't get product id, rc=%d\n", rc);
			wls_status->tx_product_id_done = false;
			return rc;
		}
		msleep(200);
	}

	rc = oplus_chg_wls_get_ui_soc(wls_dev, &soc);
	if (rc < 0) {
		chg_err("can't get ui soc, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_wls_get_batt_temp(wls_dev, &temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return rc;
	}

	buf[0] = (temp >> 8) & 0xff;
	buf[1] = temp & 0xff;
	buf[2] = soc & 0xff;
	chg_info("soc:%d, temp:%d\n", soc, temp);
	oplus_chg_wls_send_data(wls_dev, WLS_CMD_SEND_BATT_TEMP_SOC, buf, 0);

	msleep(1000);
	return rc;
}

static int oplus_chg_wls_get_third_adapter_v_id(struct oplus_chg_wls *wls_dev)
{
	int rc = 0;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (wls_status->adapter_id != WLS_ADAPTER_THIRD_PARTY)
		return rc;

	if (!wls_status->verify_by_aes) {
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_VENDOR_ID, 0xff, 5);
		if (rc < 0) {
			chg_err("can't vendor id, rc=%d\n", rc);
			wls_status->verify_by_aes = false;
			wls_status->adapter_type = WLS_ADAPTER_TYPE_UNKNOWN;
			return rc;
		}
	}

	return rc;
}

#define WAIT_FOR_2CEP_INTERVAL_MS	300
static int oplus_chg_wls_rx_handle_state_default(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
#ifdef WLS_SUPPORT_OPLUS_CHG
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int real_soc = 100;
#endif
	enum oplus_chg_temp_region temp_region;
	enum oplus_chg_wls_rx_mode rx_mode;
	int rc;
	bool psy_changed = false;

#ifdef WLS_SUPPORT_OPLUS_CHG
	vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
	oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx->rx_ic, &rx_mode);
	switch (rx_mode) {
	case OPLUS_CHG_WLS_RX_MODE_EPP_5W:
		wls_status->epp_5w = true;
		fallthrough;
	case OPLUS_CHG_WLS_RX_MODE_EPP:
		wls_status->epp_working = true;
		wls_status->wls_type = OPLUS_CHG_WLS_EPP;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		goto out;
	case OPLUS_CHG_WLS_RX_MODE_EPP_PLUS:
		wls_status->epp_working = true;
		wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		goto out;
	default:
		break;
	}

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, false);

	if (wls_status->verity_pass) {
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INDENTIFY_ADAPTER, 0xff, 0);
		msleep(WAIT_FOR_2CEP_INTERVAL_MS);
	}

	vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, USER_VOTER, true, 750, true);
	if (wls_status->verity_pass && wls_status->adapter_type == WLS_ADAPTER_TYPE_UNKNOWN)
		rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INDENTIFY_ADAPTER, 0xff, 5);
	else
		rc = -EINVAL;
	if (rc < 0 && wls_status->adapter_type == WLS_ADAPTER_TYPE_UNKNOWN) {
		chg_info("can't get adapter type, rc=%d\n", rc);
		wls_status->wls_type = OPLUS_CHG_WLS_UNKNOWN;
	} else {
		switch (wls_status->adapter_type) {
		case WLS_ADAPTER_TYPE_USB:
		case WLS_ADAPTER_TYPE_NORMAL:
			wls_status->wls_type = OPLUS_CHG_WLS_BPP;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
			goto out;
		/*case WLS_ADAPTER_TYPE_EPP:
			rc = oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx, &rx_mode);
			if (rc < 0) {
				chg_err("get rx mode error, rc=%d\n", rc);
				wls_status->wls_type = OPLUS_CHG_WLS_EPP;
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
				goto out;
			}
			if (rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_PLUS) {
				wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
			} else if (rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP){
				wls_status->wls_type = OPLUS_CHG_WLS_EPP;
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
			} else {
				wls_status->wls_type = OPLUS_CHG_WLS_BPP;
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
			}
			goto out;*/
		case WLS_ADAPTER_TYPE_VOOC:
			rc = oplus_chg_wls_get_third_adapter_v_id(wls_dev);
			if (rc < 0)
				goto out;
			if (wls_dev->support_fastchg) {
				wls_status->wls_type = OPLUS_CHG_WLS_VOOC;
				psy_changed = true;
			} else {
				wls_status->epp_working = true;
				wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
			}
			if (temp_region > BATT_TEMP_LITTLE_COLD && temp_region < BATT_TEMP_WARM) {
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
			} else {
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
			}
			goto out;
		case WLS_ADAPTER_TYPE_SVOOC:
		case WLS_ADAPTER_TYPE_PD_65W:
			rc = oplus_chg_wls_get_third_adapter_v_id(wls_dev);
			if (rc < 0)
				goto out;

			if (wls_dev->support_fastchg) {
				if (wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC)
					wls_status->wls_type = OPLUS_CHG_WLS_SVOOC;
				else
					wls_status->wls_type = OPLUS_CHG_WLS_PD_65W;
				psy_changed = true;
				if (wls_status->switch_quiet_mode)
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
				else if (!wls_dev->batt_charge_enable)
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
				else
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
				/*
		 		* The fan of the 30w wireless charger cannot be reset automatically.
		 		* Actively turn on the fan once when wireless charging is connected.
		 		*/
				if (wls_status->adapter_id == WLS_ADAPTER_MODEL_0 &&
				    !wls_status->quiet_mode_init &&
				    !wls_status->switch_quiet_mode)
					(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_SET_NORMAL_MODE, 0xff, 2);

				rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
				if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
					chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
				}
				if ((temp_region < BATT_TEMP_COOL) || (temp_region > BATT_TEMP_LITTLE_WARM)) {
					chg_err("Abnormal battery temperature, temp_region=%d\n", temp_region);
					vote(wls_dev->fastchg_disable_votable, QUIET_VOTER, true, 0, false);
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
				}
			} else {
				wls_status->epp_working = true;
				wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
			}
			if (wls_dev->debug_mode)
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;
			goto out;
		default:
			wls_status->wls_type = OPLUS_CHG_WLS_UNKNOWN;
		}
	}
#endif
	rc = oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx->rx_ic, &rx_mode);
	if (rc < 0) {
		chg_err("get rx mode error, rc=%d\n", rc);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		goto out;
	}

	switch (rx_mode) {
	case OPLUS_CHG_WLS_RX_MODE_BPP:
		wls_status->wls_type = OPLUS_CHG_WLS_BPP;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		break;
	case OPLUS_CHG_WLS_RX_MODE_EPP_5W:
		wls_status->epp_5w = true;
		fallthrough;
	case OPLUS_CHG_WLS_RX_MODE_EPP:
		wls_status->epp_working = true;
		wls_status->wls_type = OPLUS_CHG_WLS_EPP;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		break;
	case OPLUS_CHG_WLS_RX_MODE_EPP_PLUS:
		wls_status->epp_working = true;
		wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		break;
	default:
		wls_status->wls_type = OPLUS_CHG_WLS_BPP;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		break;
	}

out:
	/*if (is_comm_ocm_available(wls_dev))
		oplus_chg_comm_update_config(wls_dev->comm_ocm);*/
	if (psy_changed && is_wls_psy_available(wls_dev))
		power_supply_changed(wls_dev->wls_psy);
	oplus_chg_wls_config(wls_dev);
	return 0;
}

static int oplus_chg_wls_rx_exit_state_default(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_BPP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
		break;
	case OPLUS_CHG_WLS_RX_STATE_EPP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
		break;
	case OPLUS_CHG_WLS_RX_STATE_EPP_PLUS:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
		break;
	case OPLUS_CHG_WLS_RX_STATE_FAST:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_QUIET:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_STOP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DEBUG:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DONE:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

#define WLS_CURR_OP_TRX_CHG_MA	600
static int oplus_chg_wls_rx_enter_state_bpp(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int icl_max_ma = wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_BPP];
	int wait_time_ms = 0;
#ifdef WLS_SUPPORT_OPLUS_CHG
	int rc = 0;

	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 1200, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 750, true);
		if (wls_status->adapter_type != WLS_ADAPTER_TYPE_UNKNOWN) {
			/* (void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, 5000, 0); */
		} else {
			rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_ID, 0xff, 5);
			if (rc < 0)
				chg_info("can't get tx id, it's not op tx\n");
		}
		wait_time_ms = 100;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		if (wls_status->is_op_trx) {
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, WLS_CURR_OP_TRX_CHG_MA, false);
			oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, false);
			oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
			wls_status->state_sub_step = 0;
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		} else {
			wls_status->state_sub_step = 2;
			wait_time_ms = 100;
		}
		wls_status->adapter_info_cmd_count = 0;
		break;
	case 2:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
		wls_status->state_sub_step = 0;
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#else
	switch (wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 2000, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor, true);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 200, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 2;
		break;
	case 2:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 400, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 3;
		break;
	case 3:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 700, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 4;
		break;
	case 4:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 1000, true);
		wls_status->state_sub_step = 0;
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#endif
	return wait_time_ms;
}

#define A1_COUNT_MAX	30
static int oplus_chg_wls_rx_handle_state_bpp(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int iout_ma, vout_mv, vrect_mv;
	int wait_time_ms = 4000;
#ifdef WLS_SUPPORT_OPLUS_CHG
	int rc;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int real_soc = 100;
	enum oplus_chg_temp_region temp_region;
	enum oplus_chg_wls_rx_mode rx_mode;
	bool psy_changed = false;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);

	if (wls_dev->factory_mode && wls_dev->support_get_tx_pwr) {
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, 0);
		wait_time_ms = 3000;
	} else {
		if (wls_status->adapter_info_cmd_count < A1_COUNT_MAX && wls_status->verity_pass) {
			(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INDENTIFY_ADAPTER, 0xff, 0);
			wait_time_ms = 1000;
			wls_status->adapter_info_cmd_count++;
		}
	}
#endif

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

#ifdef WLS_SUPPORT_OPLUS_CHG
	switch (wls_status->adapter_type) {
	case WLS_ADAPTER_TYPE_EPP:
		rc = oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx->rx_ic, &rx_mode);
		if (rc < 0) {
			chg_err("get rx mode error, rc=%d\n", rc);
			wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
			break;
		}
		if (rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_PLUS) {
			wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		} else {
			wls_status->wls_type = OPLUS_CHG_WLS_EPP;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		}
		break;
	case WLS_ADAPTER_TYPE_VOOC:
		rc = oplus_chg_wls_get_third_adapter_v_id(wls_dev);
		if (rc < 0)
			goto out;

		if (wls_dev->support_fastchg) {
			wls_status->wls_type = OPLUS_CHG_WLS_VOOC;
			psy_changed = true;
		} else {
			wls_status->epp_working = true;
			wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
		}
		if (temp_region > BATT_TEMP_LITTLE_COLD && temp_region < BATT_TEMP_WARM) {
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
		} else {
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_BPP;
		}
		break;
	case WLS_ADAPTER_TYPE_SVOOC:
	case WLS_ADAPTER_TYPE_PD_65W:
		rc = oplus_chg_wls_get_third_adapter_v_id(wls_dev);
		if (rc < 0)
			goto out;

		if (wls_dev->support_fastchg) {
			if (wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC)
				wls_status->wls_type = OPLUS_CHG_WLS_SVOOC;
			else
				wls_status->wls_type = OPLUS_CHG_WLS_PD_65W;
			psy_changed = true;
			if (wls_status->switch_quiet_mode)
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
			else if (!wls_dev->batt_charge_enable)
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
			else
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
			rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
			if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
				chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			}
			if ((temp_region < BATT_TEMP_LITTLE_COLD) || (temp_region > BATT_TEMP_WARM)) {
				chg_err("Abnormal battery temperature, temp_region=%d\n", temp_region);
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			}
		} else {
			wls_status->epp_working = true;
			wls_status->wls_type = OPLUS_CHG_WLS_EPP_PLUS;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		}
		if (wls_dev->debug_mode)
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;
		break;
	default:
		goto out;
	}
	wait_time_ms = 100;
	if (psy_changed && is_wls_psy_available(wls_dev))
		power_supply_changed(wls_dev->wls_psy);
	oplus_chg_wls_config(wls_dev);
out:
#endif
	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);

	iout_ma = wls_status->iout_ma;
	vout_mv = wls_status->vout_mv;
	vrect_mv = wls_status->vrect_mv;
	chg_info("wkcs: iout=%d, vout=%d, vrect=%d\n", iout_ma, vout_mv, vrect_mv);

	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_bpp(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_EPP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
		break;
	case OPLUS_CHG_WLS_RX_STATE_EPP_PLUS:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor->nor_ic, true);
		break;
	case OPLUS_CHG_WLS_RX_STATE_FAST:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_QUIET:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_STOP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DEBUG:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DONE:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DEFAULT:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_DEFAULT;
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

#define SKEWING_WORK_DELAY	3000
static int oplus_chg_wls_rx_enter_state_epp(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int icl_max_ma = wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP];
	int wait_time_ms = 0;
	int iout_ma = 0;
#ifdef WLS_SUPPORT_OPLUS_CHG
	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 2800, false);
		if (wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC ||
		    wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC ||
		    wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W) {
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, 100, false);
			vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, false);
			oplus_chg_wls_rx_get_iout(wls_dev->wls_rx->rx_ic, &iout_ma);
			if (iout_ma > 200) {
				wait_time_ms = 100;
				break;
			}
			(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, dynamic_cfg->vooc_vol_mv, 0);
			if(wls_dev->support_fastchg)
				(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
			else
				(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0x01, -1);
			wls_status->fod_parm_for_fastchg = true;
			(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, dynamic_cfg->vooc_vol_mv, 15);

			if (wls_status->pwr_max_mw > 0) {
				vote(wls_dev->nor_icl_votable, RX_MAX_VOTER, true,
				     wls_status->pwr_max_mw * 1000 /dynamic_cfg->vooc_vol_mv, false);
			}
		}
		wait_time_ms = 100;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		if (wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC
				|| wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC
				|| wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W) {
			if (wls_status->vout_mv > 11500) {
				vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
				oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_AIRVOOC);
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
				wls_status->state_sub_step = 0;
				wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
			} else {
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, true);
			}
		} else {
			if (wls_status->epp_5w || wls_status->vout_mv < 8000)
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, 450, true);
			else
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);

			wls_status->state_sub_step = 0;
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		}
		wait_time_ms = 100;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#else
	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 2000, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor, true);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 200, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 2;
		break;
	case 2:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 3;
		break;
	case 3:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 900, false);
		wls_status->state_sub_step = 0;
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#endif
	if (wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_EPP)
		schedule_delayed_work(&wls_dev->wls_skewing_work, msecs_to_jiffies(SKEWING_WORK_DELAY));

	return wait_time_ms;
}

#define EPP_STRAGE_CHANGE_WAIT 60
static int oplus_chg_wls_choose_epp_curve(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_fcc_step *epp_cfg;
	int batt_temp = 0;
	int rc, i;
	int last_epp_chg_level;

	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		return -EPERM;
	}
	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_EPP:
		epp_cfg = &dynamic_cfg->epp_steps;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		epp_cfg = &dynamic_cfg->epp_plus_steps;
		break;
	default:
		return -EPERM;
	}

	if (wls_status->epp_chg_level == -1) {
		for (i = 0; i < epp_cfg->max_step; i++) {
			if (batt_temp < epp_cfg->fcc_step[i].high_threshold)
				break;
		}
		wls_status->epp_chg_level = (i >= epp_cfg->max_step) ? (epp_cfg->max_step - 1) : i;
		dynamic_cfg->epp_steps.fcc_wait_timeout = jiffies + HZ * EPP_STRAGE_CHANGE_WAIT;
		vote(wls_dev->nor_icl_votable, EPP_CURVE_VOTER, true,
			epp_cfg->fcc_step[wls_status->epp_chg_level].curr_ma, true);
		return 0;
	}

	if (!time_after(jiffies, dynamic_cfg->epp_steps.fcc_wait_timeout))
		return 0;

	last_epp_chg_level = wls_status->epp_chg_level;
	if (batt_temp > epp_cfg->fcc_step[wls_status->epp_chg_level].high_threshold &&
	    (wls_status->epp_chg_level + 1 < epp_cfg->max_step)) {
		wls_status->epp_chg_level += 1;
		dynamic_cfg->epp_steps.fcc_wait_timeout = jiffies + HZ * EPP_STRAGE_CHANGE_WAIT;
	} else if (batt_temp < epp_cfg->fcc_step[wls_status->epp_chg_level].low_threshold &&
		   wls_status->epp_chg_level - 1 >= 0) {
			wls_status->epp_chg_level -= 1;
			dynamic_cfg->epp_steps.fcc_wait_timeout = jiffies + HZ * EPP_STRAGE_CHANGE_WAIT;
	}
	vote(wls_dev->nor_icl_votable, EPP_CURVE_VOTER, true, epp_cfg->fcc_step[wls_status->epp_chg_level].curr_ma, true);

	if (last_epp_chg_level != wls_status->epp_chg_level)
		chg_info("[%s %d %d %d]\n", oplus_chg_wls_rx_state_text[wls_status->wls_type],
			batt_temp, wls_status->epp_chg_level, epp_cfg->max_step);

	return 0;
}

static int oplus_chg_wls_rx_handle_state_epp(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int icl_max_ma = wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP];
	int iout_ma, icl_ma, vout_mv, vrect_mv;
	int nor_input_curr_ma;
	int wait_time_ms = 4000;
	int rc = 0;

#ifdef WLS_SUPPORT_OPLUS_CHG
	if (wls_dev->factory_mode && wls_dev->support_get_tx_pwr) {
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, 0);
		wait_time_ms = 3000;
	} else {
		oplus_chg_wls_check_quiet_mode(wls_dev);
	}
#endif

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

	if (wls_status->epp_5w || wls_status->vout_mv < 8000) {
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 450, true);
		if (!wls_status->epp_5w)
			wait_time_ms = 500;
	} else {
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
	}

	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_choose_epp_curve(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);

	if ((wls_status->adapter_type != WLS_ADAPTER_TYPE_UNKNOWN) &&
	     !wls_status->verity_started) {
		icl_ma = get_effective_result(wls_dev->nor_icl_votable);
		rc = oplus_chg_wls_nor_get_input_curr(wls_dev->wls_nor->nor_ic, &nor_input_curr_ma);
		if (rc < 0)
			nor_input_curr_ma = wls_status->iout_ma;
		if (icl_ma - nor_input_curr_ma < 300)
			schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));
	}

	iout_ma = wls_status->iout_ma;
	vout_mv = wls_status->vout_mv;
	vrect_mv = wls_status->vrect_mv;
	chg_info("wkcs: iout=%d, vout=%d, vrect=%d\n", iout_ma, vout_mv, vrect_mv);
	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_epp(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

static int oplus_chg_wls_rx_enter_state_epp_plus(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int icl_max_ma = wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP_PLUS];
	int wait_time_ms = 0;
	int iout_ma = 0;
#ifdef WLS_SUPPORT_OPLUS_CHG
	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 4000, false);
		if (wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC ||
		    wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC ||
		    wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W) {
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, 100, false);
			vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, true);
			oplus_chg_wls_rx_get_iout(wls_dev->wls_rx->rx_ic, &iout_ma);
			if (iout_ma > 200) {
				wait_time_ms = 100;
				break;
			}
			(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, dynamic_cfg->vooc_vol_mv, 0);
			if(wls_dev->support_fastchg)
				(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
			else
				(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0x01, -1);
			wls_status->fod_parm_for_fastchg = true;
			(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, dynamic_cfg->vooc_vol_mv, 15);
			if (wls_status->pwr_max_mw > 0) {
				vote(wls_dev->nor_icl_votable, RX_MAX_VOTER, true,
				     wls_status->pwr_max_mw * 1000 / dynamic_cfg->vooc_vol_mv, false);
			} else {
				/*vote a default max icl for vooc*/
				vote(wls_dev->nor_icl_votable, RX_MAX_VOTER, true,
					12000 * 1000 / dynamic_cfg->vooc_vol_mv, false);
			}
		}
		wait_time_ms = 100;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		if (wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC
				|| wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC
				|| wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W) {
			if (wls_status->vout_mv > 11500) {
				vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
				oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_AIRVOOC);
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
				wls_status->state_sub_step = 0;
				wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
			} else {
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, true);
			}
		} else {
			if (wls_status->vout_mv < 8000)
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, 400, true);
			else
				vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
			wls_status->state_sub_step = 0;
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		}

		wait_time_ms = 100;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#else
	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 3000, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 1;
		break;
	case 1:
		oplus_chg_wls_nor_set_aicl_enable(wls_dev->wls_nor, true);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 200, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 2;
		break;
	case 2:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 3;
		break;
	case 3:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 900, false);
		wait_time_ms = 300;
		wls_status->state_sub_step = 4;
		break;
	case 4:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 1250, false);
		wls_status->state_sub_step = 0;
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_EPP_PLUS;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}
#endif
	if (wls_status->current_rx_state == OPLUS_CHG_WLS_RX_STATE_EPP_PLUS)
		schedule_delayed_work(&wls_dev->wls_skewing_work, msecs_to_jiffies(SKEWING_WORK_DELAY));
	return wait_time_ms;
}

static int oplus_chg_wls_rx_handle_state_epp_plus(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int icl_max_ma = wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP_PLUS];
	int iout_ma, icl_ma, vout_mv, vrect_mv;
	int nor_input_curr_ma;
	int wait_time_ms = 4000;
	int rc = 0;

#ifdef WLS_SUPPORT_OPLUS_CHG
	if (wls_dev->factory_mode && wls_dev->support_get_tx_pwr) {
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, 0);
		wait_time_ms = 3000;
	} else {
		oplus_chg_wls_check_quiet_mode(wls_dev);
	}
#endif

	if (wls_status->adapter_type == WLS_ADAPTER_TYPE_VOOC
			|| wls_status->adapter_type == WLS_ADAPTER_TYPE_SVOOC
			|| wls_status->adapter_type == WLS_ADAPTER_TYPE_PD_65W) {
		if (wls_status->pwr_max_mw > 0) {
			vote(wls_dev->nor_icl_votable, RX_MAX_VOTER, true,
				wls_status->pwr_max_mw * 1000 / dynamic_cfg->vooc_vol_mv, false);
		}
	}

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

	if (wls_status->vout_mv < 8000) {
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 400, true);
		wait_time_ms = 500;
	} else {
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, icl_max_ma, true);
	}

	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_choose_epp_curve(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);

	if ((wls_status->adapter_type != WLS_ADAPTER_TYPE_UNKNOWN) &&
	     !wls_status->verity_started) {
		icl_ma = get_effective_result(wls_dev->nor_icl_votable);
		rc = oplus_chg_wls_nor_get_input_curr(wls_dev->wls_nor->nor_ic, &nor_input_curr_ma);
		if (rc < 0)
			nor_input_curr_ma = wls_status->iout_ma;
		if (icl_ma - nor_input_curr_ma < 300)
			schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));
	}

	iout_ma = wls_status->iout_ma;
	vout_mv = wls_status->vout_mv;
	vrect_mv = wls_status->vrect_mv;
	chg_info("wkcs: iout=%d, vout=%d, vrect=%d\n", iout_ma, vout_mv, vrect_mv);
	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_epp_plus(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

#define CP_OPEN_OFFSET		100
#define CURR_ERR_COUNT_MAX	200
static int oplus_chg_wls_rx_enter_state_fast(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_fcc_step *fcc_chg = &wls_dev->wls_fcc_step;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int vbat_mv, vout_mv, batt_num, batt_temp;
	static int curr_err_count;
	int delay_ms = 0;
	int real_soc;
	int iout_max_ma;
	int scale_factor;
	int rc;
	int iout_ma = 0;
	int i = 0;

	if (wls_status->switch_quiet_mode &&
	    wls_status->state_sub_step > OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_FAST) {
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		return 0;
	}

	if (!wls_dev->batt_charge_enable) {
		if (wls_status->state_sub_step > OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_FAST) {
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
			return 0;
		}
	} else {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	}

	rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
	if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
		chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	rc = oplus_chg_wls_get_vbat(wls_dev, &vbat_mv);
	if (rc < 0) {
		if (printk_ratelimit())
			chg_err("can't get vbat, rc=%d\n", rc);
		delay_ms = 100;
		return delay_ms;
	}

	rc = oplus_chg_wls_get_batt_temp(wls_dev, &batt_temp);
	if (rc < 0) {
		chg_err("can't get batt temp, rc=%d\n", rc);
		delay_ms = 100;
		return delay_ms;
	}

	chg_info("state_sub_step=%d\n", wls_status->state_sub_step);

	switch(wls_status->state_sub_step) {
	case OPLUS_CHG_WLS_FAST_SUB_STATE_INIT:
		rc = oplus_chg_wls_choose_fastchg_curve(wls_dev);
		if (rc < 0) {
			chg_err("choose fastchg curve failed, rc = %d\n", rc);
			return 0;
		}
		oplus_chg_wls_bcc_parms_init(wls_dev);
		oplus_chg_wls_bcc_get_curve(wls_dev);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 100, false);
		rerun_election(wls_dev->nor_icl_votable, false);
		if (wls_status->vout_mv < 9000)
			vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, false);
		oplus_chg_wls_rx_get_iout(wls_dev->wls_rx->rx_ic, &iout_ma);
		if (iout_ma > 200) {
			delay_ms = 100;
			break;
		}
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, 0);
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
		wls_status->fod_parm_for_fastchg = true;
		wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_FAST;
		oplus_chg_wls_get_verity_data(wls_dev);
		delay_ms = 100;
		break;
	case OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_FAST:
		if (wls_status->charge_type != WLS_CHARGE_TYPE_FAST)
			return 500;
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, -1);
		oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_AIRSVOOC);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, WLS_CURR_WAIT_FAST_MA, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		if (wls_status->rx_online)
			(void)oplus_chg_wls_set_ext_pwr_enable(wls_dev, true);
		/*need about 600ms for 9415*/
		msleep(600);
		if (wls_status->rx_online)
			(void)oplus_chg_wls_rx_set_dcdc_enable(wls_dev->wls_rx->rx_ic, true);
		curr_err_count = 0;
		if (wls_status->rx_online && wls_status->adapter_id == WLS_ADAPTER_THIRD_PARTY) {
			rc = oplus_chg_wls_get_third_adapter_ext_cmd_p_id(wls_dev);
			if (rc < 0) {
				if (!wls_status->rx_online)
					return 0;
				chg_err("get product id fail\n");
				wls_status->online_keep = true;
				vote(wls_dev->rx_disable_votable, VERITY_VOTER, true, 1, false);
				schedule_delayed_work(&wls_dev->rx_verity_restore_work, msecs_to_jiffies(500));
				return 500;
			}
		}
		if (wls_status->iout_ma < 100) {
			wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_IOUT;
			curr_err_count++;
			delay_ms = 100;
		} else {
			wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_VOUT;
			curr_err_count = 0;
			delay_ms = 0;
		}
		break;
	case OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_IOUT:
		if (wls_status->iout_ma < 100) {
			if (curr_err_count == CURR_ERR_COUNT_MAX / 2)
				oplus_chg_wls_nor_set_aicl_rerun(wls_dev->wls_nor->nor_ic);
			curr_err_count++;
			delay_ms = 100;
		} else {
			curr_err_count = 0;
			delay_ms = 0;
			wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_VOUT;
		}
		if (curr_err_count > CURR_ERR_COUNT_MAX) {
			curr_err_count = 0;
			wls_status->state_sub_step = 0;
			vote(wls_dev->fastchg_disable_votable, CURR_ERR_VOTER, true, 1, false);
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			delay_ms = 0;
		}
		break;
	case OPLUS_CHG_WLS_FAST_SUB_STATE_WAIT_VOUT:
		rc = oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx, 4 * vbat_mv + CP_OPEN_OFFSET + 200, 1000, 50);
		if (rc < 0) {
			chg_err("can't set vout to %d, rc=%d\n", 4 * vbat_mv + CP_OPEN_OFFSET, rc);
			wls_status->state_sub_step = 0;
			wls_status->fastchg_retry_count++;
			wls_status->fastchg_retry_timer = jiffies + (unsigned long)(300 * HZ); /*5 min*/
			vote(wls_dev->fastchg_disable_votable, STARTUP_CEP_VOTER, true, 1, false);
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			delay_ms = 0;
			break;
		}
		wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_CHECK_IOUT;
		delay_ms = 0;
		break;
	case OPLUS_CHG_WLS_FAST_SUB_STATE_CHECK_IOUT:
		rc = oplus_chg_wls_rx_get_vout(wls_dev->wls_rx->rx_ic, &vout_mv);
		if (rc < 0) {
			chg_err("can't get vout, rc=%d\n", rc);
			delay_ms = 100;
			break;
		}
		if (vout_mv < vbat_mv * 4 + CP_OPEN_OFFSET) {
			chg_info("rx vout(=%d) < %d, retry\n", vout_mv, vbat_mv * 4 + CP_OPEN_OFFSET);
			(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx, 4 * vbat_mv + CP_OPEN_OFFSET + 300, 1000, 10);
			delay_ms = 0;
			break;
		}
		wls_status->state_sub_step = OPLUS_CHG_WLS_FAST_SUB_STATE_START;
		delay_ms = 0;
		break;
	case OPLUS_CHG_WLS_FAST_SUB_STATE_START:
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, true);
		(void)oplus_chg_wls_fast_start(wls_dev->wls_fast->fast_ic);
		rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
		if (rc < 0) {
			chg_err("can't get real soc, rc=%d\n", rc);
			goto err;
		}
		rc = oplus_chg_wls_get_batt_num(wls_dev, &batt_num);
		if (rc == 0) {
			scale_factor = WLS_RX_VOL_MAX_MV / 5000 / batt_num;
		} else {
			scale_factor = 1;
		}
		if ((wls_status->wls_type == OPLUS_CHG_WLS_SVOOC) ||
		    (wls_status->wls_type == OPLUS_CHG_WLS_PD_65W)) {
			wls_status->fastchg_ibat_max_ma = dynamic_cfg->fastchg_curr_max_ma * scale_factor;
			iout_max_ma = WLS_FASTCHG_CURR_50W_MAX_MA;
			if (wls_status->wls_type == OPLUS_CHG_WLS_SVOOC && iout_max_ma > WLS_FASTCHG_CURR_45W_MAX_MA
					&& wls_status->adapter_power <= WLS_ADAPTER_POWER_45W_MAX_ID)
				iout_max_ma = WLS_FASTCHG_CURR_45W_MAX_MA;
			vote(wls_dev->fcc_votable, MAX_VOTER, true, iout_max_ma, false);
		} else {
			wls_status->fastchg_ibat_max_ma = WLS_FASTCHG_CURR_15W_MAX_MA * scale_factor;
			vote(wls_dev->fcc_votable, MAX_VOTER, true, WLS_FASTCHG_CURR_15W_MAX_MA, false);
		}
		wls_status->state_sub_step = 0;
		wls_status->fastchg_started = true;
		wls_status->fastchg_level_init_temp = 0;
		wls_status->wait_cep_stable = true;
		wls_status->fastchg_retry_count = 0;
		wls_status->iout_ma_conunt = 0;
		fcc_chg->fcc_wait_timeout = jiffies;

		oplus_chg_wls_wake_bcc_update_work(wls_dev);

		if (!wls_status->fastchg_restart) {
			for (i = 0; i < fcc_chg->max_step; i++) {
				if ((batt_temp < fcc_chg->fcc_step[i].high_threshold) &&
				    (vbat_mv < fcc_chg->fcc_step[i].vol_max_mv))
					break;
			}
			wls_status->fastchg_level = i;
			vote(wls_dev->fcc_votable, FCC_VOTER, true,
				fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma, false);
			oplus_chg_wls_fast_fcc_param_init(wls_dev);
			wls_status->fastchg_restart = true;
		} else {
			vote(wls_dev->fcc_votable, FCC_VOTER, true,
				fcc_chg->fcc_step[wls_status->fastchg_level].curr_ma, false);
		}
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		if (is_nor_fv_votable_available(wls_dev))
			vote(wls_dev->nor_fv_votable, USER_VOTER, true,
				dynamic_cfg->batt_vol_max_mv + 100, false);
		delay_ms = 0;
		break;
	}

	return delay_ms;

err:
	wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
	wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
	return 0;
}

static int oplus_chg_wls_rx_handle_state_fast(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int wait_time_ms = 4000;
	int rc;

	oplus_chg_wls_check_quiet_mode(wls_dev);

	if ((wls_status->iout_ma > 500) && (wls_dev->wls_nor->icl_set_ma != 0)) {
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 0, false);
	}

	if (wls_dev->factory_mode&& wls_dev->support_get_tx_pwr) {
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, 0);
		wait_time_ms = 3000;
	}

	if (wls_dev->force_type == OPLUS_CHG_WLS_FORCE_TYPE_AUTO)
		return wait_time_ms;

	if (wls_status->switch_quiet_mode) {
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		return 0;
	}

	if (!wls_dev->batt_charge_enable) {
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		return 0;
	}

	if (is_client_vote_enabled(wls_dev->fastchg_disable_votable, RX_FAST_ERR_VOTER)) {
		chg_info("exit wls fast charge by fast_chg err\n");
		wls_status->fastchg_err_count++;
		wls_status->fastchg_err_timer = jiffies + (unsigned long)(300 * HZ); /*5 min*/
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	if (!wls_status->verity_started &&
	    (wls_status->fastchg_target_curr_ma - wls_status->iout_ma <
	     WLS_CURR_ERR_MIN_MA)) {
		schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));
	}

	rc = oplus_chg_wls_fast_temp_check(wls_dev);
	if (rc < 0) {
		chg_info("exit wls fast charge, exit_code=%d\n", rc);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	if (wls_status->fast_cep_check < 0) {
		chg_info("exit wls fast charge by cep check\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	rc = oplus_chg_wls_fast_ibat_check(wls_dev);
	if (rc < 0) {
		chg_info("exit wls fast charge by ibat check\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	rc = oplus_chg_wls_fast_iout_check(wls_dev);
	if (rc < 0) {
		chg_info("exit wls fast charge by iout check\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	oplus_chg_wls_fast_skin_temp_check(wls_dev);
	rc = oplus_chg_wls_fast_cool_down_check(wls_dev);
	if (rc < 0) {
		chg_info("exit wls fast charge by cool down\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	if (wls_status->ffc_check) {
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FFC;
		wls_status->ffc_check = false;
		return 0;
	}
	oplus_chg_wls_exchange_batt_mesg(wls_dev);

	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_fast(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	vote(wls_dev->fcc_votable, EXIT_VOTER, true, WLS_FASTCHG_CURR_EXIT_MA, false);
	while (wls_status->iout_ma > (WLS_FASTCHG_CURR_EXIT_MA + WLS_FASTCHG_CURR_ERR_MA)) {
		if (!wls_status->rx_online) {
			chg_info("wireless charge is offline\n");
			return 0;
		}
		msleep(500);
	}
	wls_status->fastchg_started = false;
	if (is_nor_fv_votable_available(wls_dev))
		vote(wls_dev->nor_fv_votable, USER_VOTER, false, 0, false);
	oplus_chg_wls_cancel_bcc_update_work(wls_dev);

	/*if (is_comm_ocm_available(wls_dev))
		oplus_chg_comm_update_config(wls_dev->comm_ocm);*/

	(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
		oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_DONE:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		msleep(100);
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 1000, -1);
		vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_QUIET:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		msleep(100);
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 1000, -1);
		vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_STOP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		msleep(100);
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 1000, -1);
		vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_DEBUG:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		msleep(100);
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 1000, -1);
		vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
		break;
	case OPLUS_CHG_WLS_RX_STATE_FFC:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FFC;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 300, false);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
		msleep(100);
		(void)oplus_chg_wls_fast_set_enable(wls_dev->wls_fast->fast_ic, false);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 1000, -1);
		vote(wls_dev->fcc_votable, EXIT_VOTER, false, 0, false);
		break;
	default:
		chg_err("unsupported target status(=%s)\n",
			oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}
	return 0;
}

static int oplus_chg_wls_rx_enter_state_ffc(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	static unsigned long stop_time;
	int wait_time_ms = 0;
	int rc;

	switch(wls_status->state_sub_step) {
	case 0:
		vote(wls_dev->nor_out_disable_votable, FFC_VOTER, true, 1, false);
		stop_time = jiffies + msecs_to_jiffies(30000);
		wait_time_ms = (int)jiffies_to_msecs(stop_time - jiffies);
		wls_status->fod_parm_for_fastchg = false;

		if (wls_dev->static_config.fastchg_12v_fod_enable)
			(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
				wls_dev->static_config.fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
		else
			(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
				oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);

		oplus_chg_wls_get_verity_data(wls_dev);
		wls_status->state_sub_step = 1;
		break;
	case 1:
		if (!time_is_before_jiffies(stop_time)) {
			wait_time_ms = (int)jiffies_to_msecs(stop_time - jiffies);
			break;
		}
		vote(wls_dev->nor_out_disable_votable, FFC_VOTER, false, 0, false);
		if (wls_dev->comm_topic) {
			rc = oplus_comm_switch_ffc(wls_dev->comm_topic);
			if (rc < 0) {
				chg_err("can't switch to ffc charge, rc=%d\n", rc);
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			}
		} else {
			chg_err("comm_topic is null, can't switch to ffc charge\n");
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		}
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_FFC;
		wls_status->state_sub_step = 0;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}

	return wait_time_ms;
}

static int oplus_chg_wls_rx_handle_state_ffc(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int iout_ma, vout_mv, vrect_mv;
	/*int rc;*/

	oplus_chg_wls_check_quiet_mode(wls_dev);

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

	/*if (is_comm_ocm_available(wls_dev)) {
		rc = oplus_chg_comm_check_ffc(wls_dev->comm_ocm);
		if (rc < 0) {
			chg_err("ffc check error, exit ffc charge, rc=%d\n", rc);
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			return 0;
		} else if (rc > 0) {
			chg_info("ffc done\n");
			wls_status->ffc_done = true;
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			return 0;
		}
	} else {
		chg_err("comm mod not found, exit ffc charge\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}*/
	if (oplus_chg_wls_get_ffc_status() == FFC_DEFAULT) {
		chg_info("ffc charging, exit\n");
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		return 0;
	}

	iout_ma = wls_status->iout_ma;
	vout_mv = wls_status->vout_mv;
	vrect_mv = wls_status->vrect_mv;
	chg_info("wkcs: iout=%d, vout=%d, vrect=%d\n", iout_ma, vout_mv, vrect_mv);

	if (!wls_status->verity_started)
		schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));

	return 4000;
}

static int oplus_chg_wls_rx_exit_state_ffc(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_DONE:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		break;
	case OPLUS_CHG_WLS_RX_STATE_QUIET:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		break;
	case OPLUS_CHG_WLS_RX_STATE_STOP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		break;
	case OPLUS_CHG_WLS_RX_STATE_DEBUG:
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_enter_state_done(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;

	switch (wls_status->state_sub_step) {
	case 0:
		oplus_chg_wls_get_verity_data(wls_dev);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 100, true);
		vote(wls_dev->nor_input_disable_votable, USER_VOTER, true, 0, false);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 0);
		wls_status->state_sub_step = 1;
		fallthrough;
	case 1:
		if (wls_status->charge_type != WLS_CHARGE_TYPE_FAST) {
			rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, 0);
			if (rc < 0) {
				chg_err("send WLS_CMD_INTO_FASTCHAGE err, rc=%d\n", rc);
				return 100;
			}
			wls_status->fod_parm_for_fastchg = false;
			return 1000;
		} else {
			if (wls_status->adapter_id == WLS_ADAPTER_THIRD_PARTY) {
				rc = oplus_chg_wls_get_third_adapter_ext_cmd_p_id(wls_dev);
				if (rc < 0) {
					if (!wls_status->rx_online)
						return 0;
					chg_err("get product id fail\n");
					wls_status->online_keep = true;
					vote(wls_dev->rx_disable_votable, VERITY_VOTER, true, 1, false);
					schedule_delayed_work(&wls_dev->rx_verity_restore_work, msecs_to_jiffies(500));
					return 100;
				}
			}

			if (wls_dev->static_config.fastchg_12v_fod_enable)
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					wls_dev->static_config.fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
			else
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		}
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, -1);
		wls_status->state_sub_step = 2;
		break;
	case 2:
		if (wls_status->vout_mv > 11500) {
			vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 3600, false);
			vote(wls_dev->nor_icl_votable, USER_VOTER, true, 950, true);
			oplus_chg_wls_set_non_ffc_current(wls_dev);
			vote(wls_dev->nor_input_disable_votable, USER_VOTER, false, 0, false);
			oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_AIRSVOOC);
			rc = oplus_chg_wls_choose_fastchg_curve(wls_dev);
			if (rc < 0)
				chg_err("choose fastchg curve failed, rc = %d\n", rc);
			wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			wls_status->state_sub_step = 0;
		} else {
			return 100;
		}
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_handle_state_done(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int wait_time_ms = 4000;
	int iout_ma, vout_mv, vrect_mv;
	int rc;

	oplus_chg_wls_check_quiet_mode(wls_dev);

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

	rc = oplus_chg_wls_fastchg_restart_check(wls_dev);
	if ((rc >= 0) && (!wls_status->fastchg_disable))
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;

	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);
	oplus_chg_wls_exchange_batt_mesg(wls_dev);

	iout_ma = wls_status->iout_ma;
	vout_mv = wls_status->vout_mv;
	vrect_mv = wls_status->vrect_mv;
	chg_info("wkcs: iout=%d, vout=%d, vrect=%d\n", iout_ma, vout_mv, vrect_mv);

	if (!wls_status->verity_started)
		schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));

	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_done(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_FAST:
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, true);
		(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
			oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_enter_state_quiet(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;

	switch (wls_status->state_sub_step) {
	case 0:
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, 0);
		wls_status->state_sub_step = 1;
		fallthrough;
	case 1:
		if (wls_status->charge_type != WLS_CHARGE_TYPE_FAST) {
			rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, 0);
			if (rc < 0) {
				chg_err("send WLS_CMD_INTO_FASTCHAGE err, rc=%d\n", rc);
				return 100;
			}
			wls_status->fod_parm_for_fastchg = false;
			return 1000;
		} else {
			if (wls_status->adapter_id == WLS_ADAPTER_THIRD_PARTY) {
				rc = oplus_chg_wls_get_third_adapter_ext_cmd_p_id(wls_dev);
				if (rc < 0) {
					if (!wls_status->rx_online)
						return 0;
					chg_err("get product id fail\n");
					wls_status->online_keep = true;
					vote(wls_dev->rx_disable_votable, VERITY_VOTER, true, 1, false);
					schedule_delayed_work(&wls_dev->rx_verity_restore_work, msecs_to_jiffies(500));
					return 100;
				}
			}

			if (wls_dev->static_config.fastchg_12v_fod_enable)
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					wls_dev->static_config.fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
			else
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		}

		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, -1);
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 1500, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 1250, true);
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		wls_status->state_sub_step = 0;
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_handle_state_quiet(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int icl_ma, nor_input_curr_ma;
	int wait_time_ms = 4000;
	int rc;

	oplus_chg_wls_check_quiet_mode(wls_dev);

	if (!wls_status->switch_quiet_mode || wls_status->chg_done_quiet_mode) {
		if (wls_status->quiet_mode && !wls_status->chg_done_quiet_mode) {
			wait_time_ms = 1000;
			goto out;
		} else {
			if (wls_dev->batt_charge_enable) {
				if (!wls_status->chg_done_quiet_mode)
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
				else
					wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
			} else {
				wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
			}
			return 0;
		}
	}

	if (wls_dev->batt_charge_enable) {
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
	} else {
		if (!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, true, WLS_CURR_STOP_CHG_MA, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 0, false);
		}
	}

out:
	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);
	oplus_chg_wls_exchange_batt_mesg(wls_dev);

	if ((wls_status->adapter_type != WLS_ADAPTER_TYPE_UNKNOWN) &&
	     !wls_status->verity_started) {
		icl_ma = get_effective_result(wls_dev->nor_icl_votable);
		rc = oplus_chg_wls_nor_get_input_curr(wls_dev->wls_nor->nor_ic, &nor_input_curr_ma);
		if (rc < 0)
			nor_input_curr_ma = wls_status->iout_ma;
		if (icl_ma - nor_input_curr_ma < 300)
			schedule_delayed_work(&wls_dev->wls_verity_work, msecs_to_jiffies(500));
	}

	return 4000;
}

static int oplus_chg_wls_rx_exit_state_quiet(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_FAST:
		if (!!get_client_vote(wls_dev->nor_out_disable_votable, STOP_VOTER)) {
			vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
			vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		}
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, true);
		(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
			oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		break;
	case OPLUS_CHG_WLS_RX_STATE_STOP:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		break;
	case OPLUS_CHG_WLS_RX_STATE_DONE:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_DONE;
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_enter_state_stop(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;

	switch (wls_status->state_sub_step) {
	case 0:
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx,
			WLS_VOUT_FASTCHG_INIT_MV, 0);
		wls_status->state_sub_step = 1;
		fallthrough;
	case 1:
		if (wls_status->charge_type != WLS_CHARGE_TYPE_FAST) {
			rc = oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, 0);
			if (rc < 0) {
				chg_err("send WLS_CMD_INTO_FASTCHAGE err, rc=%d\n", rc);
				return 100;
			}
			wls_status->fod_parm_for_fastchg = false;
			return 1000;
		} else {
			if (wls_dev->static_config.fastchg_12v_fod_enable)
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					wls_dev->static_config.fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
			else
				(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
					oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		}

		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, -1);
		vote(wls_dev->nor_icl_votable, STOP_VOTER, true, 300, true);
		vote(wls_dev->nor_out_disable_votable, STOP_VOTER, true, 1, false);
		wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_STOP;
		wls_status->state_sub_step = 0;
		WARN_ON(wls_dev->batt_charge_enable);
		break;
	default:
		wls_status->state_sub_step = 0;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_handle_state_stop(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int wait_time_ms = 4000;

	if (wls_dev->batt_charge_enable) {
		vote(wls_dev->nor_icl_votable, STOP_VOTER, false, 0, true);
		vote(wls_dev->nor_out_disable_votable, STOP_VOTER, false, 0, false);
		if (wls_status->switch_quiet_mode)
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		else
			wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		return 0;
	}

	oplus_chg_wls_check_quiet_mode(wls_dev);

	(void)oplus_chg_wls_nor_skin_check(wls_dev);
	oplus_chg_wls_check_term_charge(wls_dev);
	oplus_chg_wls_set_non_ffc_current(wls_dev);
	return wait_time_ms;
}

static int oplus_chg_wls_rx_exit_state_stop(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	switch (wls_status->target_rx_state) {
	case OPLUS_CHG_WLS_RX_STATE_FAST:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_FAST;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, true);
		(void)oplus_chg_wls_rx_set_fod_parm(wls_dev->wls_rx->rx_ic,
			oplus_chg_wls_disable_fod_parm, WLS_FOD_PARM_LENGTH);
		break;
	case OPLUS_CHG_WLS_RX_STATE_QUIET:
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_QUIET;
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, true);
		break;
	default:
		chg_err("unsupported target status(=%s)\n", oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);
		wls_status->target_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		wls_status->next_rx_state = OPLUS_CHG_WLS_RX_STATE_ERROR;
		break;
	}

	return 0;
}

static int oplus_chg_wls_rx_enter_state_debug(struct oplus_chg_wls *wls_dev)
{
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	int scale_factor;
	int batt_num;
	int rc;

	switch (wls_dev->force_type) {
	case OPLUS_CHG_WLS_FORCE_TYPE_BPP:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 1500, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, 5000, 0);
		msleep(500);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 1000, false);
		if (wls_dev->factory_mode)
			(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, -1);
		break;
	case OPLUS_CHG_WLS_FORCE_TYPE_EPP:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 1500, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_EPP_MV, 0);
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_EPP_MV, -1);
		msleep(500);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 850, false);
		if (wls_dev->factory_mode)
			(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, -1);
		break;
	case OPLUS_CHG_WLS_FORCE_TYPE_EPP_PLUS:
		vote(wls_dev->nor_fcc_votable, USER_VOTER, true, 2000, false);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_EPP_MV, 0);
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_EPP_MV, -1);
		msleep(500);
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 1250, false);
		if (wls_dev->factory_mode)
			(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_GET_TX_PWR, 0xff, -1);
		break;
	case OPLUS_CHG_WLS_FORCE_TYPE_FAST:
		vote(wls_dev->nor_icl_votable, USER_VOTER, true, 500, false);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, 0);
		(void)oplus_chg_wls_send_msg(wls_dev, WLS_CMD_INTO_FASTCHAGE, 0xff, -1);
		(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, WLS_VOUT_FASTCHG_INIT_MV, -1);
		(void)oplus_chg_wls_rx_set_vout_step(wls_dev->wls_rx, 15000, 1000, -1);
		rc = oplus_chg_wls_get_batt_num(wls_dev, &batt_num);
		if (rc == 0) {
			scale_factor = WLS_RX_VOL_MAX_MV / 5000 / batt_num;
		} else {
			scale_factor = 1;
		}
		wls_status->fastchg_ibat_max_ma = dynamic_cfg->fastchg_curr_max_ma * scale_factor;
		vote(wls_dev->fcc_votable, MAX_VOTER, true, WLS_FASTCHG_CURR_45W_MAX_MA, false);
		break;
	default:
		break;
	}

	wls_status->current_rx_state = OPLUS_CHG_WLS_RX_STATE_DEBUG;

	return 0;
}

static int oplus_chg_wls_rx_handle_state_debug(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 4000;
}

static int oplus_chg_wls_rx_exit_state_debug(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

static int oplus_chg_wls_rx_enter_state_ftm(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

static int oplus_chg_wls_rx_handle_state_ftm(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 4000;
}

static int oplus_chg_wls_rx_exit_state_ftm(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

static int oplus_chg_wls_rx_enter_state_error(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

static int oplus_chg_wls_rx_handle_state_error(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 4000;
}

static int oplus_chg_wls_rx_exit_state_error(struct oplus_chg_wls *wls_dev)
{
	/*struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int rc;*/

	return 0;
}

struct oplus_chg_wls_state_handler oplus_chg_wls_rx_state_handler[] = {
	[OPLUS_CHG_WLS_RX_STATE_DEFAULT] = {
		NULL,
		oplus_chg_wls_rx_handle_state_default,
		oplus_chg_wls_rx_exit_state_default
	},
	[OPLUS_CHG_WLS_RX_STATE_BPP] = {
		oplus_chg_wls_rx_enter_state_bpp,
		oplus_chg_wls_rx_handle_state_bpp,
		oplus_chg_wls_rx_exit_state_bpp
	},
	[OPLUS_CHG_WLS_RX_STATE_EPP] = {
		oplus_chg_wls_rx_enter_state_epp,
		oplus_chg_wls_rx_handle_state_epp,
		oplus_chg_wls_rx_exit_state_epp
	},
	[OPLUS_CHG_WLS_RX_STATE_EPP_PLUS] = {
		oplus_chg_wls_rx_enter_state_epp_plus,
		oplus_chg_wls_rx_handle_state_epp_plus,
		oplus_chg_wls_rx_exit_state_epp_plus,
	},
	[OPLUS_CHG_WLS_RX_STATE_FAST] = {
		oplus_chg_wls_rx_enter_state_fast,
		oplus_chg_wls_rx_handle_state_fast,
		oplus_chg_wls_rx_exit_state_fast,
	},
	[OPLUS_CHG_WLS_RX_STATE_FFC] = {
		oplus_chg_wls_rx_enter_state_ffc,
		oplus_chg_wls_rx_handle_state_ffc,
		oplus_chg_wls_rx_exit_state_ffc,
	},
	[OPLUS_CHG_WLS_RX_STATE_DONE] = {
		oplus_chg_wls_rx_enter_state_done,
		oplus_chg_wls_rx_handle_state_done,
		oplus_chg_wls_rx_exit_state_done,
	},
	[OPLUS_CHG_WLS_RX_STATE_QUIET] = {
		oplus_chg_wls_rx_enter_state_quiet,
		oplus_chg_wls_rx_handle_state_quiet,
		oplus_chg_wls_rx_exit_state_quiet,
	},
	[OPLUS_CHG_WLS_RX_STATE_STOP] = {
		oplus_chg_wls_rx_enter_state_stop,
		oplus_chg_wls_rx_handle_state_stop,
		oplus_chg_wls_rx_exit_state_stop,
	},
	[OPLUS_CHG_WLS_RX_STATE_DEBUG] = {
		oplus_chg_wls_rx_enter_state_debug,
		oplus_chg_wls_rx_handle_state_debug,
		oplus_chg_wls_rx_exit_state_debug,
	},
	[OPLUS_CHG_WLS_RX_STATE_FTM] = {
		oplus_chg_wls_rx_enter_state_ftm,
		oplus_chg_wls_rx_handle_state_ftm,
		oplus_chg_wls_rx_exit_state_ftm,
	},
	[OPLUS_CHG_WLS_RX_STATE_ERROR] = {
		oplus_chg_wls_rx_enter_state_error,
		oplus_chg_wls_rx_handle_state_error,
		oplus_chg_wls_rx_exit_state_error,
	},
};

static void oplus_chg_wls_rx_sm(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_rx_sm_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int delay_ms = 4000;

	if (!wls_status->rx_online) {
		chg_info("wireless charge is offline\n");
		return;
	}

	chg_info("curr_state=%s, next_state=%s, target_state=%s\n",
		oplus_chg_wls_rx_state_text[wls_status->current_rx_state],
		oplus_chg_wls_rx_state_text[wls_status->next_rx_state],
		oplus_chg_wls_rx_state_text[wls_status->target_rx_state]);

	if (wls_status->current_rx_state != wls_status->target_rx_state) {
		if (wls_status->current_rx_state != wls_status->next_rx_state) {
			if (oplus_chg_wls_rx_state_handler[wls_status->next_rx_state].enter_state != NULL) {
				delay_ms = oplus_chg_wls_rx_state_handler[wls_status->next_rx_state].enter_state(wls_dev);
			} else {
				delay_ms = 0;
			}
		} else {
			if (oplus_chg_wls_rx_state_handler[wls_status->current_rx_state].exit_state != NULL) {
				delay_ms = oplus_chg_wls_rx_state_handler[wls_status->current_rx_state].exit_state(wls_dev);
			} else {
				delay_ms = 0;
			}
		}
	} else {
		if (oplus_chg_wls_rx_state_handler[wls_status->current_rx_state].handle_state != NULL) {
			delay_ms = oplus_chg_wls_rx_state_handler[wls_status->current_rx_state].handle_state(wls_dev);
		}
	}

	queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_rx_sm_work, msecs_to_jiffies(delay_ms));
}

static void oplus_chg_wls_trx_disconnect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_trx_disconnect_work);
	static int retry_num;
	int rc;

	rc = oplus_chg_wls_set_trx_enable(wls_dev, false);
	if (rc < 0) {
		retry_num++;
		chg_err("can't disable trx, retry_num=%d, rc=%d\n", retry_num, rc);
		if (retry_num < 5)
			schedule_delayed_work(&wls_dev->wls_trx_disconnect_work, msecs_to_jiffies(100));
		else
			retry_num = 0;
	}
	retry_num = 0;
}

static void oplus_chg_wls_clear_trx_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_clear_trx_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;

	if (!wls_status->rx_present && !wls_status->rx_online) {
		wls_status->trx_close_delay = false;
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
	}
}

static void oplus_chg_wls_trx_sm(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_trx_sm_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	u8 trx_status, trx_err;
	static int err_count;
	static bool pre_trx_online = false;
	int delay_ms = 5000;
	int rc;

	rc = oplus_chg_wls_rx_get_trx_err(wls_dev->wls_rx->rx_ic, &trx_err);
	if (rc < 0) {
		chg_err("can't get trx err code, rc=%d\n", rc);
		goto out;
	}
	chg_info("wkcs: trx_err=0x%02x\n", trx_err);
	wls_status->trx_err = trx_err;

	if (trx_err & WLS_TRX_ERR_RXAC) {
		chg_info("trx err: RXAC\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
		wls_status->trx_rxac = true;
		wls_status->trx_close_delay = true;
	}
	if (trx_err & WLS_TRX_ERR_OCP) {
		chg_info("trx err: OCP\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
		wls_status->trx_close_delay = true;
	}
	if (trx_err & WLS_TRX_ERR_OVP) {
		chg_info("trx err: OVP\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	}
	if (trx_err & WLS_TRX_ERR_LVP) {
		chg_info("trx err: LVP\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	}
	if (trx_err & WLS_TRX_ERR_FOD) {
		chg_info("trx err: FOD\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	}
	if (trx_err & WLS_TRX_ERR_OTP) {
		chg_info("trx err: OTP\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	}
	if (trx_err & WLS_TRX_ERR_CEPTIMEOUT) {
		chg_info("trx err: CEPTIMEOUT\n");
	}
	if (trx_err & WLS_TRX_ERR_RXEPT) {
		chg_info("trx err: RXEPT\n");
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	}
	if (wls_status->trx_state == OPLUS_CHG_WLS_TRX_STATE_OFF)
		goto err;

	rc = oplus_chg_wls_rx_get_trx_status(wls_dev->wls_rx->rx_ic, &trx_status);
	if (rc < 0) {
		chg_err("can't get trx err code, rc=%d\n", rc);
		goto out;
	}
	chg_info("wkcs: trx_status=0x%02x\n", trx_status);
	if (wls_status->wls_type != OPLUS_CHG_WLS_TRX)
		goto out;
	if (trx_status & WLS_TRX_STATUS_READY) {
		wls_status->trx_present = true;
		wls_status->trx_online = false;
		pre_trx_online = false;
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
		rc = oplus_chg_wls_rx_set_trx_enable(wls_dev->wls_rx->rx_ic, true);
		if (rc < 0) {
			chg_err("can't enable trx, rc=%d\n", rc);
			goto out;
		}
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_READY;
		delay_ms = 200;
	} else if ((trx_status & WLS_TRX_STATUS_DIGITALPING)
			|| (trx_status & WLS_TRX_STATUS_ANALOGPING)) {
		wls_status->trx_present = true;
		wls_status->trx_online = false;
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_WAIT_PING;
		delay_ms = 2000;
	} else if (trx_status & WLS_TRX_STATUS_TRANSFER) {
		wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_TRANSFER;
		wls_status->trx_online = true;
		if (!wls_status->trx_usb_present_once)
			wls_status->trx_usb_present_once = oplus_chg_wls_is_usb_present(wls_dev);
		if (is_batt_psy_available(wls_dev))
			power_supply_changed(wls_dev->batt_psy);
		rc = oplus_chg_wls_rx_get_trx_curr(wls_dev->wls_rx->rx_ic, &wls_status->trx_curr_ma);
		if (rc < 0)
			chg_err("can't get trx curr, rc=%d\n", rc);
		rc = oplus_chg_wls_rx_get_trx_vol(wls_dev->wls_rx->rx_ic, &wls_status->trx_vol_mv);
		if (rc < 0)
			chg_err("can't get trx vol, rc=%d\n", rc);
		chg_info("trx_vol=%d, trx_curr=%d\n", wls_status->trx_vol_mv, wls_status->trx_curr_ma);
		delay_ms = 5000;
	} else if (trx_status == 0) {
		goto out;
	}

	if (pre_trx_online && !wls_status->trx_online) {
		wls_status->trx_transfer_end_time = oplus_chg_wls_get_local_time_s();
		chg_info("trx_online:%d, trx_start_time:%d, trx_end_time:%d,"
			"trx_usb_present_once:%d\n",
			wls_status->trx_online, wls_status->trx_transfer_start_time,
			wls_status->trx_transfer_end_time,
			wls_status->trx_usb_present_once);
		if (wls_status->trx_transfer_end_time - wls_status->trx_transfer_start_time >
		    WLS_TRX_INFO_UPLOAD_THD_2MINS) {
			oplus_chg_wls_track_upload_tx_general_info(
				wls_dev, wls_status->trx_usb_present_once);
		}
		wls_status->trx_usb_present_once = false;
	} else if (!pre_trx_online && wls_status->trx_online) {
		wls_status->trx_usb_present_once = false;
		wls_status->trx_transfer_start_time =
			oplus_chg_wls_get_local_time_s();
		chg_info("trx_online=%d, trx_start_time=%d, trx_end_time=%d\n",
			wls_status->trx_online, wls_status->trx_transfer_start_time,
			wls_status->trx_transfer_end_time);
	}
	pre_trx_online = wls_status->trx_online;

	chg_info("trx_state=%s\n", oplus_chg_wls_trx_state_text[wls_status->trx_state]);
	err_count = 0;

schedule:
	queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_trx_sm_work, msecs_to_jiffies(delay_ms));
	return;
out:
	err_count++;
	delay_ms = 200;
	if (err_count > (2 * 60 * 5) || wls_status->wls_type != OPLUS_CHG_WLS_TRX) {
		chg_err("trx status err, exit\n");
		goto err;
	}
	goto schedule;
err:
	wls_status->trx_state = OPLUS_CHG_WLS_TRX_STATE_OFF;
	err_count = 0;
	schedule_delayed_work(&wls_dev->wls_trx_disconnect_work, 0);
}

static void oplus_chg_wls_upgrade_fw_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_upgrade_fw_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	bool usb_present;
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int boot_mode;
#endif
	static int retry_num;
	int rc;

#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	boot_mode = get_boot_mode();
#ifndef CONFIG_OPLUS_CHARGER_MTK
	if (boot_mode == MSM_BOOT_MODE__FACTORY || boot_mode == MSM_BOOT_MODE__RF) {
#else
	if (boot_mode == META_BOOT || boot_mode == FACTORY_BOOT ||
	    boot_mode == ADVMETA_BOOT || boot_mode == ATE_FACTORY_BOOT) {
#endif
		chg_err("is factory mode, can't upgrade fw\n");
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, true, 1, false);
		msleep(1000);
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, false, 0, false);
		retry_num = 0;
		return;
	}
#endif

	usb_present = oplus_chg_wls_is_usb_present(wls_dev);
#ifndef CONFIG_OPLUS_CHARGER_MTK
	if (usb_present && !wls_dev->support_tx_boost) {
#else
	if (usb_present) {
#endif
		chg_info("usb online, wls fw upgrade pending\n");
		wls_status->upgrade_fw_pending = true;
		return;
	}

	if (wls_status->rx_present) {
		chg_err("rx present, exit upgrade\n");
		retry_num = 0;
		return;
	}

	if (wls_dev->fw_upgrade_by_buf) {
		if (wls_dev->fw_buf == NULL || wls_dev->fw_size == 0) {
			chg_err("fw buf is NULL or fw size is 0, can't upgrade\n");
			wls_dev->fw_upgrade_by_buf = false;
			wls_dev->fw_size = 0;
			return;
		}
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, true, 1, false);
		vote(wls_dev->wrx_en_votable, UPGRADE_FW_VOTER, true, 1, false);
		rc = oplus_chg_wls_rx_upgrade_firmware_by_buf(wls_dev->wls_rx, wls_dev->fw_buf, wls_dev->fw_size);
		vote(wls_dev->wrx_en_votable, UPGRADE_FW_VOTER, false, 0, false);
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, false, 0, false);
		if (rc < 0)
			chg_err("upgrade error, rc=%d\n", rc);
		kfree(wls_dev->fw_buf);
		wls_dev->fw_buf = NULL;
		wls_dev->fw_upgrade_by_buf = false;
		wls_dev->fw_size = 0;
	} else {
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, true, 1, false);
		vote(wls_dev->wrx_en_votable, UPGRADE_FW_VOTER, true, 1, false);
		rc = oplus_chg_wls_rx_upgrade_firmware_by_img(wls_dev->wls_rx);
		vote(wls_dev->wrx_en_votable, UPGRADE_FW_VOTER, false, 0, false);
		vote(wls_dev->rx_disable_votable, UPGRADE_VOTER, false, 0, false);
		if (rc < 0) {
			retry_num++;
			chg_err("upgrade error, retry_num=%d, rc=%d\n", retry_num, rc);
			goto out;
		}
	}
	chg_info("update successed\n");

	return;

out:
	if (retry_num >= 5) {
		retry_num = 0;
		return;
	}
	schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, msecs_to_jiffies(1000));
}

#define DELTA_IOUT_500MA	500
#define DELTA_IOUT_300MA	300
#define DELTA_IOUT_100MA	100
#define VOUT_200MV		200
#define VOUT_100MV		100
#define VOUT_50MV		50
#define VOUT_20MV		20
static void oplus_chg_wls_data_update_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_data_update_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int ibat_ma, tmp_val, ibat_err, cep, vol_set_mv;
	int iout_ma, vout_mv, vrect_mv;
	static int cep_err_count;
	bool skip_cep_check = false;
	bool cep_ok = false;
	int rc;

	if (!wls_status->rx_online) {
		chg_err("wireless charge is not online\n");
		return;
	}

	oplus_chg_wls_update_track_info(wls_dev, NULL, false);

	rc = oplus_chg_wls_rx_get_iout(wls_dev->wls_rx->rx_ic, &iout_ma);
	if (rc < 0)
		chg_err("can't get rx iout, rc=%d\n", rc);
	rc = oplus_chg_wls_rx_get_vout(wls_dev->wls_rx->rx_ic, &vout_mv);
	if (rc < 0)
		chg_err("can't get rx vout, rc=%d\n", rc);
	rc = oplus_chg_wls_rx_get_vrect(wls_dev->wls_rx->rx_ic, &vrect_mv);
	if (rc < 0)
		chg_err("can't get rx vrect, rc=%d\n", rc);
	WRITE_ONCE(wls_status->iout_ma, iout_ma);
	WRITE_ONCE(wls_status->vout_mv, vout_mv);
	WRITE_ONCE(wls_status->vrect_mv, vrect_mv);

	if (!wls_status->fastchg_started)
		goto out;

	rc = oplus_chg_wls_get_ibat(wls_dev, &ibat_ma);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		goto out;
	}

	tmp_val = wls_status->fastchg_target_curr_ma - wls_status->iout_ma;
	ibat_err = ((wls_status->fastchg_ibat_max_ma - abs(ibat_ma)) / 4) - (WLS_CURR_ERR_MIN_MA / 2);
	/* Prevent the voltage from increasing too much, ibat exceeds expectations */
	if ((ibat_err > -(WLS_CURR_ERR_MIN_MA / 2)) && (ibat_err < 0) && (tmp_val > 0)) {
		/*
		 * When ibat is greater than 5800mA, the current is not
		 * allowed to continue to increase, preventing fluctuations.
		 */
		tmp_val = 0;
	} else {
		tmp_val = tmp_val > ibat_err ? ibat_err : tmp_val;
	}
	rc = oplus_chg_wls_get_cep_check_update(wls_dev->wls_rx->rx_ic, &cep);
	if (rc < 0) {
		chg_err("can't get cep, rc=%d\n", rc);
		cep_ok = false;
	} else {
		if (abs(cep) < 3)
			cep_ok = true;
		else
			cep_ok = false;
	}
	if (tmp_val < 0) {
		if (!cep_ok)
			cep_err_count++;
		else
			cep_err_count = 0;
		if (!wls_status->fastchg_curr_need_dec || cep_err_count >= WLS_CEP_ERR_MAX) {
			skip_cep_check = true;
			wls_status->fastchg_curr_need_dec = true;
			cep_err_count = 0;
		}
	} else {
		cep_err_count = 0;
		wls_status->fastchg_curr_need_dec = false;
	}
	vol_set_mv = wls_dev->wls_rx->vol_set_mv;
	if (cep_ok || skip_cep_check) {
		if (tmp_val > WLS_CURR_ERR_MIN_MA || tmp_val < -WLS_CURR_ERR_MIN_MA) {
			if (tmp_val > WLS_CURR_ERR_MIN_MA) {
				if (tmp_val > DELTA_IOUT_500MA)
					vol_set_mv += VOUT_200MV;
				else if (tmp_val > DELTA_IOUT_300MA)
					vol_set_mv += VOUT_100MV;
				else if (tmp_val > DELTA_IOUT_100MA)
					vol_set_mv += VOUT_50MV;
				else
					vol_set_mv += VOUT_20MV;
			} else {
				if (tmp_val < -DELTA_IOUT_500MA)
					vol_set_mv -= VOUT_200MV;
				else if (tmp_val < -DELTA_IOUT_300MA)
					vol_set_mv -= VOUT_100MV;
				else if (tmp_val < -DELTA_IOUT_100MA)
					vol_set_mv -= VOUT_50MV;
				else
					vol_set_mv -= VOUT_20MV;
			}
			if (vol_set_mv > WLS_RX_VOL_MAX_MV)
				vol_set_mv = WLS_RX_VOL_MAX_MV;
			if (vol_set_mv < WLS_FASTCHG_MODE_VOL_MIN_MV)
				vol_set_mv = WLS_FASTCHG_MODE_VOL_MIN_MV;
			mutex_lock(&wls_dev->update_data_lock);
			(void)oplus_chg_wls_rx_set_vout(wls_dev->wls_rx, vol_set_mv, 0);
			mutex_unlock(&wls_dev->update_data_lock);
		}
	}

	chg_info("Iout: target=%d, out=%d, ibat_max=%d, ibat=%d; vout=%d; vrect=%d; cep=%d\n",
		wls_status->fastchg_target_curr_ma, wls_status->iout_ma,
		wls_status->fastchg_ibat_max_ma, ibat_ma,
		wls_status->vout_mv, wls_status->vrect_mv,
		cep);
	oplus_chg_wls_fast_cep_check(wls_dev, cep);

out:
	schedule_delayed_work(&wls_dev->wls_data_update_work, msecs_to_jiffies(500));
}

#define CEP_SKEWING_VALUE	3
#define SKEWING_INTERVAL	500
static void oplus_chg_wls_skewing_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_skewing_work);
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int cep = 0;
	static int cep_ok_count = 0;
	static int cep_err_count = 0;
	int skewing_icl = 0;
	int skewing_max_step = 0;
	int pre_level = 0;
	int rc = 0;
	int i = 0;

	if (!wls_status->rx_online) {
		cep_ok_count = 0;
		cep_err_count = 0;
		return;
	}

	rc = oplus_chg_wls_get_cep(wls_dev->wls_rx->rx_ic, &cep);
	if (rc < 0) {
		chg_err("can't get cep, rc=%d\n", rc);
		goto out;
	}

	switch (wls_status->wls_type) {
	case OPLUS_CHG_WLS_EPP:
		i = OPLUS_WLS_SKEWING_EPP;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		i = OPLUS_WLS_SKEWING_EPP_PLUS;
		break;
	case OPLUS_CHG_WLS_VOOC:
		i = OPLUS_WLS_SKEWING_AIRVOOC;
		break;
	default:
		i = OPLUS_WLS_SKEWING_MAX;
		break;
	}
	if (i >= OPLUS_WLS_SKEWING_MAX)
		goto out;

	skewing_max_step = wls_dev->skewing_step[i].max_step;
	pre_level = wls_status->skewing_level;

	if (cep < CEP_SKEWING_VALUE) {
		cep_ok_count++;
		cep_err_count = 0;
		if (cep_ok_count >= CEP_OK_MAX &&
		    dynamic_cfg->skewing_step[i][wls_status->skewing_level].fallback_step &&
		    time_after(jiffies, wls_status->cep_ok_wait_timeout)) {
			cep_ok_count = 0;
			wls_status->skewing_level =
				dynamic_cfg->skewing_step[i][wls_status->skewing_level].fallback_step - 1;
			wls_status->cep_ok_wait_timeout = jiffies + HZ;
			chg_info("skewing: recovery charging current, level[%d]\n",
				wls_status->skewing_level);
		}
	} else {
		cep_ok_count = 0;
		cep_err_count++;
		if (cep_err_count >= CEP_ERR_MAX) {
			if (wls_status->skewing_level < (skewing_max_step - 1)) {
				cep_err_count = 0;
				wls_status->skewing_level++;
				wls_status->cep_ok_wait_timeout = jiffies + HZ;
				chg_info("skewing: reduce charging current, level[%d]\n",
					wls_status->skewing_level);
			} else {
				if (dynamic_cfg->skewing_step[i][wls_status->skewing_level].switch_to_bpp) {
					chg_info("skewing: switch to bpp, level[%d]\n", wls_status->skewing_level);
					oplus_chg_wls_rx_set_rx_mode(wls_dev->wls_rx->rx_ic, OPLUS_CHG_WLS_RX_MODE_BPP);
				}
			}
		}
	}

	skewing_icl = dynamic_cfg->skewing_step[i][wls_status->skewing_level].curr_ma;
	if (skewing_icl > 0 && pre_level != wls_status->skewing_level)
		vote(wls_dev->nor_icl_votable, CEP_VOTER, true, skewing_icl, true);

	chg_info("level=%d, icl=%d, cep=%d\n", wls_status->skewing_level, skewing_icl, cep);
out:
	schedule_delayed_work(&wls_dev->wls_skewing_work, msecs_to_jiffies(SKEWING_INTERVAL));
}

static void oplus_chg_wls_usb_connect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, usb_connect_work);
	bool connected, pre_connected;

	pre_connected = oplus_chg_wls_is_usb_connected(wls_dev);
retry:
	msleep(10);
	connected = oplus_chg_wls_is_usb_connected(wls_dev);
	if (connected != pre_connected) {
		pre_connected = connected;
		goto retry;
	}
}

static void oplus_chg_wls_fod_cal_work(struct work_struct *work)
{
}

static void oplus_chg_wls_rx_restore_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, rx_restore_work);

	vote(wls_dev->rx_disable_votable, JEITA_VOTER, false, 0, false);
	vote(wls_dev->nor_icl_votable, JEITA_VOTER, true, WLS_CURR_JEITA_CHG_MA, true);
}

static void oplus_chg_wls_rx_iic_restore_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, rx_iic_restore_work);

	vote(wls_dev->rx_disable_votable, RX_IIC_VOTER, false, 0, false);
}

static void oplus_chg_wls_rx_verity_restore_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, rx_verity_restore_work);

	vote(wls_dev->rx_disable_votable, VERITY_VOTER, false, 0, false);
	schedule_delayed_work(&wls_dev->verity_state_remove_work, msecs_to_jiffies(10000));
}

#define FAST_FAULT_SIZE 80
static void oplus_chg_wls_fast_fault_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, fast_fault_check_work);
	char *fast_fault;
	int rc;

	fast_fault = kzalloc(FAST_FAULT_SIZE, GFP_KERNEL);
	if (fast_fault == NULL) {
		chg_err("<FAST_FAULT>alloc fast_fault err\n");
		goto alloc_error;
	}

	rc = oplus_chg_wls_fast_get_fault(wls_dev->wls_fast->fast_ic, fast_fault);
	if (rc < 0)
		chg_err("can't get fast fault code, rc=%d\n", rc);
	else
		chg_info("fast fault =%s \n", fast_fault);

	kfree(fast_fault);
alloc_error:
	vote(wls_dev->fastchg_disable_votable, RX_FAST_ERR_VOTER, true, 1, false);
}

static void oplus_chg_wls_rx_restart_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, rx_restart_work);
	static int retry_count;

	if (!is_rx_ic_available(wls_dev->wls_rx)) {
		if (retry_count > 5) {
			chg_err("can't found wls rx ic\n");
			retry_count = 0;
			return;
		}
		retry_count++;
		schedule_delayed_work(&wls_dev->rx_restart_work, msecs_to_jiffies(500));
	}
	if (!oplus_chg_wls_rx_is_connected(wls_dev->wls_rx->rx_ic)) {
		chg_info("wireless charging is not connected\n");
		return;
	}

	retry_count = 0;
	wls_dev->wls_status.online_keep = true;
	vote(wls_dev->rx_disable_votable, USER_VOTER, true, 1, false);
	msleep(1000);
	vote(wls_dev->rx_disable_votable, USER_VOTER, false, 0, false);
	if (READ_ONCE(wls_dev->wls_status.online_keep))
		schedule_delayed_work(&wls_dev->online_keep_remove_work, msecs_to_jiffies(4000));
}

static void oplus_chg_wls_online_keep_remove_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, online_keep_remove_work);

	wls_dev->wls_status.online_keep = false;
	if (!wls_dev->wls_status.rx_online) {
		if (wls_dev->rx_wake_lock_on) {
			chg_info("release rx_wake_lock\n");
			__pm_relax(wls_dev->rx_wake_lock);
			wls_dev->rx_wake_lock_on = false;
		} else {
			chg_err("rx_wake_lock is already relax\n");
		}
	}
}

static void oplus_chg_wls_verity_state_remove_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, verity_state_remove_work);

	wls_dev->wls_status.verity_state_keep = false;
}

#define VOUT_ERR_CHECK_TIME	500
#define VOUT_ERR_CHECK_COUNT	5
#define VOUT_ERR_THRESHOLD_MV	3000
static void oplus_chg_wls_vout_err_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_vout_err_work);
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	int vout_mv;
	static int vout_err_count = 0;

	if (!wls_dev->support_fastchg || !wls_status->rx_online) {
		vout_err_count = 0;
		chg_info("wireless charge is offline or not support fastchg\n");
		return;
	}

	vout_mv = wls_status->vout_mv;

#ifdef WLS_QI_DEBUG
	if (wls_dbg_vout_mv != 0)
		vout_mv = wls_dbg_vout_mv;
#endif

	if (vout_mv < VOUT_ERR_THRESHOLD_MV)
		vout_err_count++;
	else
		vout_err_count = 0;
	if (vout_err_count > VOUT_ERR_CHECK_COUNT) {
		vout_err_count = 0;
		chg_err("vout is err, disable ext pwr!\n");
		oplus_chg_wls_track_upload_wls_err_info(wls_dev, WLS_ERR_SCENE_RX, WLS_ERR_VOUT_ABNORMAL);
		(void)oplus_chg_wls_rx_set_dcdc_enable(wls_dev->wls_rx->rx_ic, false);
		msleep(10);
		(void)oplus_chg_wls_set_ext_pwr_enable(wls_dev, false);
		return;
	}

	schedule_delayed_work(&wls_dev->wls_vout_err_work, msecs_to_jiffies(VOUT_ERR_CHECK_TIME));
}

static int oplus_chg_wls_dev_open(struct inode *inode, struct file *filp)
{
	struct oplus_chg_wls *wls_dev = container_of(filp->private_data,
		struct oplus_chg_wls, misc_dev);

	filp->private_data = wls_dev;
	chg_debug("%d,%d\n", imajor(inode), iminor(inode));
	return 0;
}

static ssize_t oplus_chg_wls_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	struct oplus_chg_wls *wls_dev = filp->private_data;
	struct wls_dev_cmd cmd;
	int rc = 0;

	mutex_lock(&wls_dev->read_lock);
	rc = wait_event_interruptible(wls_dev->read_wq, wls_dev->cmd_data_ok);
	mutex_unlock(&wls_dev->read_lock);
	if (rc)
		return rc;
	if (!wls_dev->cmd_data_ok)
		chg_err("wlchg false wakeup, rc=%d\n", rc);
	mutex_lock(&wls_dev->cmd_data_lock);
	wls_dev->cmd_data_ok = false;
	memcpy(&cmd, &wls_dev->cmd, sizeof(struct wls_dev_cmd));
	mutex_unlock(&wls_dev->cmd_data_lock);
	if (copy_to_user(buf, &cmd, sizeof(struct wls_dev_cmd))) {
		chg_err("failed to copy to user space\n");
		return -EFAULT;
	}

	return sizeof(struct wls_dev_cmd);
}

#define WLS_IOC_MAGIC			0xf1
#define WLS_NOTIFY_WLS_AUTH		_IOW(WLS_IOC_MAGIC, 1, struct wls_auth_result)

static long oplus_chg_wls_dev_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct oplus_chg_wls *wls_dev = filp->private_data;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	void __user *argp = (void __user *)arg;
	int rc;

	switch (cmd) {
	case WLS_NOTIFY_WLS_AUTH:
		rc = copy_from_user(&wls_status->verfity_data, argp, sizeof(struct wls_auth_result));
		if (rc) {
			chg_err("failed copy to user space\n");
			return rc;
		}
		wls_status->verity_data_ok = true;
		break;
	default:
		chg_err("bad ioctl %u\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t oplus_chg_wls_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	return count;
}

static const struct file_operations oplus_chg_wls_dev_fops = {
	.owner			= THIS_MODULE,
	.llseek			= no_llseek,
	.write			= oplus_chg_wls_dev_write,
	.read			= oplus_chg_wls_dev_read,
	.open			= oplus_chg_wls_dev_open,
	.unlocked_ioctl	= oplus_chg_wls_dev_ioctl,
};

static irqreturn_t oplus_chg_wls_usb_int_handler(int irq, void *dev_id)
{
	struct oplus_chg_wls *wls_dev = dev_id;

	schedule_delayed_work(&wls_dev->usb_connect_work, 0);
	return IRQ_HANDLED;
}

static int read_skin_range_data_from_node(struct device_node *node,
		const char *prop_str, struct oplus_chg_wls_skin_range_data *ranges,
		int max_threshold, u32 max_value)
{
	int rc = 0, i, length, per_tuple_length, tuples;

	if (!node || !prop_str || !ranges) {
		chg_err("Invalid parameters passed\n");
		return -EINVAL;
	}

	rc = of_property_count_elems_of_size(node, prop_str, sizeof(u32));
	if (rc < 0) {
		chg_err("Count %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	length = rc;
	per_tuple_length = sizeof(struct oplus_chg_wls_skin_range_data) / sizeof(u32);
	if (length % per_tuple_length) {
		chg_err("%s length (%d) should be multiple of %d\n", prop_str, length, per_tuple_length);
		return -EINVAL;
	}
	tuples = length / per_tuple_length;

	if (tuples > WLS_MAX_STEP_CHG_ENTRIES) {
		chg_err("too many entries(%d), only %d allowed\n", tuples, WLS_MAX_STEP_CHG_ENTRIES);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(node, prop_str, (u32 *)ranges, length);
	if (rc) {
		chg_err("Read %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	for (i = 0; i < tuples; i++) {
		if (ranges[i].low_threshold > ranges[i].high_threshold) {
			chg_err("%s thresholds should be in ascendant ranges\n", prop_str);
			rc = -EINVAL;
			goto clean;
		}

		if (ranges[i].low_threshold > max_threshold)
			ranges[i].low_threshold = max_threshold;
		if (ranges[i].high_threshold > max_threshold)
			ranges[i].high_threshold = max_threshold;
		if (ranges[i].curr_ma > max_value)
			ranges[i].curr_ma = max_value;
	}

	return tuples;
clean:
	memset(ranges, 0, tuples * sizeof(struct oplus_chg_wls_skin_range_data));
	return rc;
}

static const char * const strategy_soc[] = {
	[WLS_FAST_SOC_0_TO_30]	= "strategy_soc_0_to_30",
	[WLS_FAST_SOC_30_TO_70]	= "strategy_soc_30_to_70",
	[WLS_FAST_SOC_70_TO_90]	= "strategy_soc_70_to_90",
};

static const char * const strategy_temp[] = {
	[WLS_FAST_TEMP_0_TO_50]		= "strategy_temp_0_to_50",
	[WLS_FAST_TEMP_50_TO_120]	= "strategy_temp_50_to_120",
	[WLS_FAST_TEMP_120_TO_160]	= "strategy_temp_120_to_160",
	[WLS_FAST_TEMP_160_TO_400]	= "strategy_temp_160_to_400",
	[WLS_FAST_TEMP_400_TO_440]	= "strategy_temp_400_to_440",
};

static const char * const bcc_soc[] = {
	[WLS_BCC_SOC_0_TO_30]	= "bcc_soc_0_to_30",
	[WLS_BCC_SOC_30_TO_70]	= "bcc_soc_30_to_70",
	[WLS_BCC_SOC_70_TO_90]	= "bcc_soc_70_to_90",
};

static const char * const bcc_temp[] = {
	[WLS_BCC_TEMP_0_TO_50]		= "bcc_temp_0_to_50",
	[WLS_BCC_TEMP_50_TO_120]	= "bcc_temp_50_to_120",
	[WLS_BCC_TEMP_120_TO_160]	= "bcc_temp_120_to_160",
	[WLS_BCC_TEMP_160_TO_400]	= "bcc_temp_160_to_400",
	[WLS_BCC_TEMP_400_TO_440]	= "bcc_temp_400_to_440",
};

static const char * const bcc_stop_curr[] = {
	[WLS_BCC_STOP_0_TO_30]	= "bcc_stop_curr_0_to_30",
	[WLS_BCC_STOP_30_TO_70]	= "bcc_stop_curr_30_to_70",
	[WLS_BCC_STOP_70_TO_90]	= "bcc_stop_curr_70_to_90",
};

static const char * const strategy_non_ffc[] = {
	[OPLUS_WLS_CHG_MODE_BPP]	= "non-ffc-bpp",
	[OPLUS_WLS_CHG_MODE_EPP]	= "non-ffc-epp",
	[OPLUS_WLS_CHG_MODE_EPP_PLUS]	= "non-ffc-epp-plus",
	[OPLUS_WLS_CHG_MODE_AIRVOOC]	= "non-ffc-airvooc",
	[OPLUS_WLS_CHG_MODE_AIRSVOOC]	= "non-ffc-airsvooc",
};

static const char * const strategy_cv[] = {
	[OPLUS_WLS_CHG_MODE_BPP]	= "cv-bpp",
	[OPLUS_WLS_CHG_MODE_EPP]	= "cv-epp",
	[OPLUS_WLS_CHG_MODE_EPP_PLUS]	= "cv-epp-plus",
	[OPLUS_WLS_CHG_MODE_AIRVOOC]	= "cv-airvooc",
	[OPLUS_WLS_CHG_MODE_AIRSVOOC]	= "cv-airsvooc",
};

static const char * const strategy_cool_down[] = {
	[OPLUS_WLS_CHG_MODE_BPP]	= "cool-down-bpp",
	[OPLUS_WLS_CHG_MODE_EPP]	= "cool-down-epp",
	[OPLUS_WLS_CHG_MODE_EPP_PLUS]	= "cool-down-epp-plus",
	[OPLUS_WLS_CHG_MODE_AIRVOOC]	= "cool-down-airvooc",
	[OPLUS_WLS_CHG_MODE_AIRSVOOC]	= "cool-down-airsvooc",
};

static const char * const strategy_skewing[] = {
	[OPLUS_WLS_SKEWING_EPP]		= "skewing-epp",
	[OPLUS_WLS_SKEWING_EPP_PLUS]	= "skewing-epp-plus",
	[OPLUS_WLS_SKEWING_AIRVOOC]	= "skewing-airvooc",
};

static void oplus_chg_wls_flag_parse_dt(struct oplus_chg_wls *wls_dev)
{
	struct device_node *node = wls_dev->dev->of_node;

	wls_dev->support_epp_plus = of_property_read_bool(node, "oplus,support_epp_plus");
	wls_dev->support_fastchg = of_property_read_bool(node, "oplus,support_fastchg");
	wls_dev->support_get_tx_pwr = of_property_read_bool(node, "oplus,support_get_tx_pwr");
	wls_dev->support_tx_boost = of_property_read_bool(node, "oplus,support_tx_boost");
	wls_dev->support_wls_and_tx_boost = of_property_read_bool(node, "oplus,support_wls_and_tx_boost");
	wls_dev->support_wls_chg_bcc = of_property_read_bool(node, "oplus,support_wls_chg_bcc");
}

static int oplus_chg_wls_parse_dt(struct oplus_chg_wls *wls_dev)
{
	struct device_node *node = wls_dev->dev->of_node;
	struct device_node *wls_strategy_node, *soc_strategy_node;
	struct device_node *wls_third_part_strategy_node;
	struct device_node *wls_bcc_table_node, *soc_bcc_node;
	struct oplus_chg_wls_static_config *static_cfg = &wls_dev->static_config;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg = &wls_dev->dynamic_config;
	struct oplus_chg_wls_skin_step *skin_step;
	int i, m, j, k, length;
	int rc;

	oplus_chg_wls_flag_parse_dt(wls_dev);

	rc = of_property_read_u32(node, "oplus,wls_phone_id", &wls_dev->wls_phone_id);
	if (rc < 0) {
		chg_err("oplus,wls_phone_id reading failed, rc=%d\n", rc);
		wls_dev->wls_phone_id = 0x000A;
	}
	rc = of_property_read_u32(node, "oplus,wls_power_mw", &wls_dev->wls_power_mw);
	if (rc < 0) {
		chg_err("oplus,oplus,wls_power_mw reading failed, rc=%d\n", rc);
		wls_dev->wls_power_mw = 30000;
	}

	rc = of_property_count_elems_of_size(node, "oplus,fastchg-match-q", sizeof(u8));
	if (rc < 0) {
		chg_err("Count oplus,fastchg-match-q failed, rc=%d\n", rc);
	} else {
		length = rc;
		rc = of_property_read_u8_array(node, "oplus,fastchg-match-q", (u8 *)&static_cfg->fastchg_match_q, length);
		if (rc < 0)
			chg_err("oplus,fastchg-match-q reading failed, rc=%d\n", rc);
		for (j = 0; j < length / 2; j++)
			chg_info("match-q: 0x%x 0x%x\n", static_cfg->fastchg_match_q[j].id,
				static_cfg->fastchg_match_q[j].q_value);
	}

	static_cfg->fastchg_fod_enable = of_property_read_bool(node, "oplus,fastchg-fod-enable");
	if (static_cfg->fastchg_fod_enable) {
		rc = of_property_read_u8_array(node, "oplus,fastchg-fod-parm",
			(u8 *)&static_cfg->fastchg_fod_parm, WLS_FOD_PARM_LENGTH);
		if (rc < 0) {
			static_cfg->fastchg_fod_enable = false;
			chg_err("Read oplus,fastchg-fod-parm failed, rc=%d\n", rc);
		}
		rc = of_property_read_u8_array(node, "oplus,fastchg-fod-parm-12V",
			(u8 *)&static_cfg->fastchg_fod_parm_12v, WLS_FOD_PARM_LENGTH);
		static_cfg->fastchg_12v_fod_enable = true;
		if (rc < 0) {
			static_cfg->fastchg_12v_fod_enable = false;
			chg_err("Read oplus,fastchg-fod-parm-12V failed, rc=%d\n", rc);
		}
	}

	rc = of_property_read_u32(node, "oplus,max-voltage-mv", &dynamic_cfg->batt_vol_max_mv);
	if (rc < 0) {
		chg_err("oplus,max-voltage-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->batt_vol_max_mv = 4550;
	}

	rc = of_property_read_u32(node, "oplus,fastchg_curr_max_ma", &dynamic_cfg->fastchg_curr_max_ma);
	if (rc < 0) {
		chg_err("oplus,fastchg_curr_max_ma reading failed, rc=%d\n", rc);
		dynamic_cfg->fastchg_curr_max_ma = 1500;
	}

	rc = of_property_read_u32(node, "oplus,verity_curr_max_ma", &dynamic_cfg->verity_curr_max_ma);
	if (rc < 0) {
		chg_err("oplus,verity_curr_max_ma reading failed, rc=%d\n", rc);
		dynamic_cfg->verity_curr_max_ma = dynamic_cfg->fastchg_curr_max_ma;
	}

	wls_strategy_node = of_get_child_by_name(node, "wireless_fastchg_strategy");
	if (!wls_strategy_node) {
		chg_err("Can not find wireless_fastchg_strategy node\n");
		return -EINVAL;
	}
	for (i = 0; i < WLS_FAST_SOC_MAX; i++) {
		soc_strategy_node = of_get_child_by_name(wls_strategy_node, strategy_soc[i]);
		if (!soc_strategy_node) {
			chg_err("Can not find %s node\n", strategy_soc[i]);
			return -EINVAL;
		}
		for (j = 0; j < WLS_FAST_TEMP_MAX; j++) {
			rc = of_property_count_elems_of_size(soc_strategy_node, strategy_temp[j], sizeof(u32));
			if (rc < 0) {
				chg_err("Count %s failed, rc=%d\n", strategy_temp[j], rc);
				return rc;
			}
			length = rc;
			rc = of_property_read_u32_array(soc_strategy_node, strategy_temp[j],
				(u32 *)wls_dev->fcc_steps[i].fcc_step[j].fcc_step, length);
			wls_dev->fcc_steps[i].fcc_step[j].max_step = length/5;
		}
	}
	for (i = 0; i < WLS_FAST_SOC_MAX; i++) {
		for (j = 0; j < WLS_FAST_TEMP_MAX; j++) {
			for (k = 0; k < wls_dev->fcc_steps[i].fcc_step[j].max_step; k++) {
				chg_info("%d %d %d %d %d\n",
					wls_dev->fcc_steps[i].fcc_step[j].fcc_step[k].low_threshold,
					wls_dev->fcc_steps[i].fcc_step[j].fcc_step[k].high_threshold,
					wls_dev->fcc_steps[i].fcc_step[j].fcc_step[k].curr_ma,
					wls_dev->fcc_steps[i].fcc_step[j].fcc_step[k].vol_max_mv,
					wls_dev->fcc_steps[i].fcc_step[j].fcc_step[k].need_wait);
			}
		}
	}

	rc = of_property_count_elems_of_size(node, "oplus,wls_epp_strategy", sizeof(u32));
	if (rc < 0) {
		chg_err("Count oplus,wls_epp_strategy, rc=%d\n", rc);
		length = sizeof(default_wls_epp_strategy) / sizeof(int32_t);
		memcpy(&dynamic_cfg->epp_steps.fcc_step, default_wls_epp_strategy, length * sizeof(int32_t));
	} else {
		length = rc;
		rc = of_property_read_u32_array(node, "oplus,wls_epp_strategy",
			(u32 *)&dynamic_cfg->epp_steps.fcc_step, length);
		if (rc < 0) {
			chg_err("read oplus,wls_epp_strategy failed, rc=%d\n", rc);
			return rc;
		}
	}
	dynamic_cfg->epp_steps.max_step = length / 5;
	for (k = 0; k < dynamic_cfg->epp_steps.max_step; k++) {
		chg_info("wls_epp_strategy[%d]%d %d %d %d %d\n", k,
			dynamic_cfg->epp_steps.fcc_step[k].low_threshold,
			dynamic_cfg->epp_steps.fcc_step[k].high_threshold,
			dynamic_cfg->epp_steps.fcc_step[k].curr_ma,
			dynamic_cfg->epp_steps.fcc_step[k].vol_max_mv,
			dynamic_cfg->epp_steps.fcc_step[k].need_wait);
	}

	rc = of_property_count_elems_of_size(node, "oplus,wls_epp_plus_strategy", sizeof(u32));
	if (rc < 0) {
		chg_err("Count oplus,wls_epp_plus_strategy, rc=%d\n", rc);
		length = sizeof(default_wls_epp_plus_strategy) / sizeof(int32_t);
		memcpy(&dynamic_cfg->epp_plus_steps.fcc_step, default_wls_epp_plus_strategy, length * (sizeof(int32_t)));
	} else {
		length = rc;
		rc = of_property_read_u32_array(node, "oplus,wls_epp_plus_strategy",
			(u32 *)&dynamic_cfg->epp_plus_steps.fcc_step, length);
		if (rc < 0) {
			chg_err("read oplus,wls_epp_plus_strategy failed, rc=%d\n", rc);
			return rc;
		}
	}
	dynamic_cfg->epp_plus_steps.max_step = length / 5;
	for (k = 0; k < dynamic_cfg->epp_plus_steps.max_step; k++) {
		chg_info("wls_epp_plus_strategy[%d]%d %d %d %d %d\n", k,
			dynamic_cfg->epp_plus_steps.fcc_step[k].low_threshold,
			dynamic_cfg->epp_plus_steps.fcc_step[k].high_threshold,
			dynamic_cfg->epp_plus_steps.fcc_step[k].curr_ma,
			dynamic_cfg->epp_plus_steps.fcc_step[k].vol_max_mv,
			dynamic_cfg->epp_plus_steps.fcc_step[k].need_wait);
	}

	wls_bcc_table_node = of_get_child_by_name(node, "wireless_bcc_table");
	if (!wls_bcc_table_node) {
		chg_err("Can not find wireless_bcc_table node\n");
	} else {
		rc = of_property_read_u32(node, "oplus,wls-bcc-fcc-to-icl-factor", &wls_dev->wls_bcc_fcc_to_icl_factor);
		if (rc < 0) {
			chg_err("oplus,wls-bcc-fcc-to-icl-factor reading failed, rc=%d\n", rc);
			wls_dev->wls_bcc_fcc_to_icl_factor = 2;
		}
		for (i = 0; i < WLS_BCC_SOC_MAX; i++) {
			soc_bcc_node = of_get_child_by_name(wls_bcc_table_node, bcc_soc[i]);
			if (!soc_bcc_node)
				chg_err("Can not find %s node\n", bcc_soc[i]);
			for (j = 0; j < WLS_BCC_TEMP_MAX; j++) {
				rc = of_property_count_elems_of_size(soc_bcc_node, bcc_temp[j], sizeof(u32));
				if (rc < 0) {
					chg_err("Count %s failed, rc=%d\n", bcc_temp[j], rc);
					return rc;
				}
				length = rc;
				rc = of_property_read_u32_array(soc_bcc_node, bcc_temp[j],
					(u32 *)wls_dev->bcc_steps[i].bcc_step[j].bcc_step, length);
				wls_dev->bcc_steps[i].bcc_step[j].max_step = length/4;
			}
		}

		for (i = 0; i < WLS_BCC_SOC_MAX; i++) {
			for (j = 0; j < WLS_BCC_TEMP_MAX; j++) {
				for (k = 0; k < wls_dev->bcc_steps[i].bcc_step[j].max_step; k++) {
					chg_info("bcc_step: %d %d %d %d\n",
						wls_dev->bcc_steps[i].bcc_step[j].bcc_step[k].max_batt_volt,
						wls_dev->bcc_steps[i].bcc_step[j].bcc_step[k].max_curr,
						wls_dev->bcc_steps[i].bcc_step[j].bcc_step[k].min_curr,
						wls_dev->bcc_steps[i].bcc_step[j].bcc_step[k].exit);
				}
			}
		}
	}

	rc = read_unsigned_data_from_node(node, "bcc_stop_curr_0_to_30",
		dynamic_cfg->bcc_stop_curr_0_to_30, WLS_BCC_STOP_CURR_NUM);
	if (rc < 0) {
		chg_err("get wls,bcc_stop_curr_0_to_30 property error, rc=%d\n", rc);
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			dynamic_cfg->bcc_stop_curr_0_to_30[i] = 1000;
	} else {
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			chg_info(" bcc_stop_0_30: %d", dynamic_cfg->bcc_stop_curr_0_to_30[i]);
	}
	rc = read_unsigned_data_from_node(node, "bcc_stop_curr_30_to_70",
		dynamic_cfg->bcc_stop_curr_30_to_70, WLS_BCC_STOP_CURR_NUM);
	if (rc < 0) {
		chg_err("get wls,bcc_stop_curr_30_to_70 property error, rc=%d\n", rc);
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			dynamic_cfg->bcc_stop_curr_30_to_70[i] = 1000;
	} else {
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			chg_info(" bcc_stop_30_70: %d", dynamic_cfg->bcc_stop_curr_30_to_70[i]);
	}
	rc = read_unsigned_data_from_node(node, "bcc_stop_curr_70_to_90",
		dynamic_cfg->bcc_stop_curr_70_to_90, WLS_BCC_STOP_CURR_NUM);
	if (rc < 0) {
		chg_err("get wls,bcc_stop_curr_70_to_90 property error, rc=%d\n", rc);
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			dynamic_cfg->bcc_stop_curr_70_to_90[i] = 1000;
	} else {
		for (i = 0; i < WLS_BCC_STOP_CURR_NUM; i++)
			chg_info(" bcc_stop_70_90: %d", dynamic_cfg->bcc_stop_curr_70_to_90[i]);
	}

	wls_third_part_strategy_node = of_get_child_by_name(node, "wireless_fastchg_third_part_strategy");
	if (!wls_third_part_strategy_node) {
		chg_err("Can not find wls third_part_strategy node\n");
		memcpy(wls_dev->fcc_third_part_steps, wls_dev->fcc_steps, sizeof(wls_dev->fcc_steps));
	} else {
		for (i = 0; i < WLS_FAST_SOC_MAX; i++) {
			soc_strategy_node = of_get_child_by_name(wls_third_part_strategy_node, strategy_soc[i]);
			if (!soc_strategy_node) {
				chg_err("Can not find %s node\n", strategy_soc[i]);
				return -EINVAL;
			}
			for (j = 0; j < WLS_FAST_TEMP_MAX; j++) {
				rc = of_property_count_elems_of_size(soc_strategy_node, strategy_temp[j], sizeof(u32));
				if (rc < 0) {
					chg_err("Count %s failed, rc=%d\n", strategy_temp[j], rc);
					return rc;
				}
				length = rc;
				rc = of_property_read_u32_array(soc_strategy_node, strategy_temp[j],
					(u32 *)wls_dev->fcc_third_part_steps[i].fcc_step[j].fcc_step, length);
				wls_dev->fcc_third_part_steps[i].fcc_step[j].max_step = length/5;
			}
		}
	}

	/* non-ffc */
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		rc = of_property_count_elems_of_size(node, strategy_non_ffc[i], sizeof(u32));
		if (rc < 0) {
			chg_err("Count %s failed, rc=%d\n", strategy_non_ffc[i], rc);
			return rc;
		}
		length = rc;
		rc = of_property_read_u32_array(node, strategy_non_ffc[i],
			(u32 *)dynamic_cfg->non_ffc_step[i], length);
		wls_dev->non_ffc_step[i].max_step = length / 5;
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		for (j = 0; j < wls_dev->non_ffc_step[i].max_step; j++) {
			chg_info("%d %d %d %d %d\n",
				dynamic_cfg->non_ffc_step[i][j].low_threshold,
				dynamic_cfg->non_ffc_step[i][j].high_threshold,
				dynamic_cfg->non_ffc_step[i][j].curr_ma,
				dynamic_cfg->non_ffc_step[i][j].vol_max_mv,
				dynamic_cfg->non_ffc_step[i][j].need_wait);
		}
	}

	/* cv */
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		rc = of_property_count_elems_of_size(node, strategy_cv[i], sizeof(u32));
		if (rc < 0) {
			chg_err("Count %s failed, rc=%d\n", strategy_cv[i], rc);
			return rc;
		}
		length = rc;
		rc = of_property_read_u32_array(node, strategy_cv[i],
			(u32 *)dynamic_cfg->cv_step[i], length);
		wls_dev->cv_step[i].max_step = length / 5;
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		for (j = 0; j < wls_dev->cv_step[i].max_step; j++) {
			chg_info("%d %d %d %d %d\n",
				dynamic_cfg->cv_step[i][j].low_threshold,
				dynamic_cfg->cv_step[i][j].high_threshold,
				dynamic_cfg->cv_step[i][j].curr_ma,
				dynamic_cfg->cv_step[i][j].vol_max_mv,
				dynamic_cfg->cv_step[i][j].need_wait);
		}
	}

	/* cool down */
	rc = of_property_read_u32(node, "oplus,cool-down-12v-thr", &dynamic_cfg->cool_down_12v_thr);
	if (rc < 0) {
		chg_err("cool-down-12v-thr reading failed, rc=%d\n", rc);
		dynamic_cfg->cool_down_12v_thr = -EINVAL;
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		rc = of_property_count_elems_of_size(node, strategy_cool_down[i], sizeof(u32));
		if (rc < 0) {
			chg_err("Count %s failed, rc=%d\n", strategy_cool_down[i], rc);
			return rc;
		}
		length = rc;
		rc = read_unsigned_data_from_node(node, strategy_cool_down[i],
			(u32 *)dynamic_cfg->cool_down_step[i], length);
		wls_dev->cool_down_step[i].max_step = length;
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		for (j = 0; j < wls_dev->cool_down_step[i].max_step; j++)
			chg_info(" %d\n", dynamic_cfg->cool_down_step[i][j]);
	}

	/* skewing */
	for (i = 0; i < OPLUS_WLS_SKEWING_MAX; i++) {
		rc = of_property_count_elems_of_size(node, strategy_skewing[i], sizeof(u32));
		if (rc < 0) {
			chg_err("Count %s failed, rc=%d\n", strategy_skewing[i], rc);
			return rc;
		}
		length = rc;
		rc = read_unsigned_data_from_node(node, strategy_skewing[i],
			(u32 *)dynamic_cfg->skewing_step[i], length);
		wls_dev->skewing_step[i].max_step = length / 3;
	}
	for (i = 0; i < OPLUS_WLS_SKEWING_MAX; i++) {
		for (j = 0; j < wls_dev->skewing_step[i].max_step; j++) {
			chg_info("%d %d %d\n",
				dynamic_cfg->skewing_step[i][j].curr_ma,
				dynamic_cfg->skewing_step[i][j].fallback_step,
				dynamic_cfg->skewing_step[i][j].switch_to_bpp);
		}
	}

	rc = of_property_read_u32(node, "oplus,bpp-vol-mv", &dynamic_cfg->bpp_vol_mv);
	if (rc < 0) {
		chg_err("oplus,bpp-vol-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->bpp_vol_mv = 5000;
	}
	rc = of_property_read_u32(node, "oplus,epp-vol-mv", &dynamic_cfg->epp_vol_mv);
	if (rc < 0) {
		chg_err("oplus,epp-vol-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->epp_vol_mv = 10000;
	}
	rc = of_property_read_u32(node, "oplus,epp_plus-vol-mv", &dynamic_cfg->epp_plus_vol_mv);
	if (rc < 0) {
		chg_err("oplus,epp_plus-vol-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->epp_plus_vol_mv = 10000;
	}
	rc = of_property_read_u32(node, "oplus,vooc-vol-mv", &dynamic_cfg->vooc_vol_mv);
	if (rc < 0) {
		chg_err("oplus,vooc-vol-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->vooc_vol_mv = 10000;
	}
	rc = of_property_read_u32(node, "oplus,svooc-vol-mv", &dynamic_cfg->svooc_vol_mv);
	if (rc < 0) {
		chg_err("oplus,svooc-vol-mv reading failed, rc=%d\n", rc);
		dynamic_cfg->svooc_vol_mv = 12000;
	}

	rc = read_unsigned_data_from_node(node, "oplus,iclmax-ma",
		(u32 *)(&dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW]),
		BATT_TEMP_MAX * OPLUS_WLS_CHG_MODE_MAX);
	if (rc < 0) {
		chg_err("get oplus,iclmax-ma property error, rc=%d\n", rc);
		for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
			for (m = 0; m < BATT_TEMP_MAX; m++)
				dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][m] =
					default_config.iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][m];
		}
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		wls_dev->icl_max_ma[i] = 0;
		for (m = 0; m < BATT_TEMP_MAX; m++) {
			if (dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][m] > wls_dev->icl_max_ma[i])
				wls_dev->icl_max_ma[i] = dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][m];
		}
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		chg_info("OPLUS_WLS_CHG_BATT_CL_LOW: %d %d %d %d %d %d %d %d %d\n",
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][0],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][1],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][2],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][3],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][4],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][5],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][6],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][7],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_LOW][i][8]);
	}

	rc = read_unsigned_data_from_node(node, "oplus,iclmax-batt-high-ma",
		(u32 *)(&dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH]),
		BATT_TEMP_MAX * OPLUS_WLS_CHG_MODE_MAX);
	if (rc < 0) {
		chg_err("get oplus,iclmax-ma property error, rc=%d\n", rc);
		for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
			for (m = 0; m < BATT_TEMP_MAX; m++)
				dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][m] =
					default_config.iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][m];
		}
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		for (m = 0; m < BATT_TEMP_MAX; m++) {
			if (dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][m] > wls_dev->icl_max_ma[i])
				wls_dev->icl_max_ma[i] = dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][m];
		}
	}
	for (i = 0; i < OPLUS_WLS_CHG_MODE_MAX; i++) {
		chg_info("OPLUS_WLS_CHG_BATT_CL_HIGH: %d %d %d %d %d %d %d %d %d\n",
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][0],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][1],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][2],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][3],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][4],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][5],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][6],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][7],
			dynamic_cfg->iclmax_ma[OPLUS_WLS_CHG_BATT_CL_HIGH][i][8]);
	}

	skin_step = &wls_dev->epp_plus_skin_step;
	skin_step->skin_step = dynamic_cfg->epp_plus_skin_step;
	rc = read_skin_range_data_from_node(node, "oplus,epp_plus-skin-step", skin_step->skin_step,
		WLS_SKIN_TEMP_MAX, wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP_PLUS]);
	if (rc < 0) {
		for (i = 0; i < WLS_MAX_STEP_CHG_ENTRIES; i++) {
			skin_step->skin_step[i].low_threshold = default_config.epp_plus_skin_step[i].low_threshold;
			skin_step->skin_step[i].high_threshold = default_config.epp_plus_skin_step[i].high_threshold;
			skin_step->skin_step[i].curr_ma = default_config.epp_plus_skin_step[i].curr_ma;
			if ((skin_step->skin_step[i].low_threshold == 0) &&
			    (skin_step->skin_step[i].high_threshold == 0) &&
			    (skin_step->skin_step[i].curr_ma == 0)) {
				skin_step->max_step = i;
				break;
			}
		}
	} else {
		skin_step->max_step = rc;
	}
	for (i = 0; i < skin_step->max_step; i++) {
		chg_info("epp_plus-skin-step: %d %d %d\n", skin_step->skin_step[i].low_threshold,
			skin_step->skin_step[i].high_threshold, skin_step->skin_step[i].curr_ma);
	}

	skin_step = &wls_dev->epp_skin_step;
	skin_step->skin_step = dynamic_cfg->epp_skin_step;
	rc = read_skin_range_data_from_node(node, "oplus,epp-skin-step", skin_step->skin_step,
		WLS_SKIN_TEMP_MAX, wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP]);
	if (rc < 0) {
		for (i = 0; i < WLS_MAX_STEP_CHG_ENTRIES; i++) {
			skin_step->skin_step[i].low_threshold = default_config.epp_skin_step[i].low_threshold;
			skin_step->skin_step[i].high_threshold = default_config.epp_skin_step[i].high_threshold;
			skin_step->skin_step[i].curr_ma = default_config.epp_skin_step[i].curr_ma;
			if ((skin_step->skin_step[i].low_threshold == 0) &&
			    (skin_step->skin_step[i].high_threshold == 0) &&
			    (skin_step->skin_step[i].curr_ma == 0)) {
				skin_step->max_step = i;
				break;
			}
		}
	} else {
		skin_step->max_step = rc;
	}
	for (i = 0; i < skin_step->max_step; i++) {
		chg_info("epp-skin-step: %d %d %d\n", skin_step->skin_step[i].low_threshold,
			skin_step->skin_step[i].high_threshold, skin_step->skin_step[i].curr_ma);
	}

	skin_step = &wls_dev->bpp_skin_step;
	skin_step->skin_step = dynamic_cfg->bpp_skin_step;
	rc = read_skin_range_data_from_node(node, "oplus,bpp-skin-step", skin_step->skin_step,
		WLS_SKIN_TEMP_MAX, wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_BPP]);
	if (rc < 0) {
		for (i = 0; i < WLS_MAX_STEP_CHG_ENTRIES; i++) {
			skin_step->skin_step[i].low_threshold =
				default_config.bpp_skin_step[i].low_threshold;
			skin_step->skin_step[i].high_threshold =
				default_config.bpp_skin_step[i].high_threshold;
			skin_step->skin_step[i].curr_ma =
				default_config.bpp_skin_step[i].curr_ma;
			if ((skin_step->skin_step[i].low_threshold == 0) &&
			    (skin_step->skin_step[i].high_threshold == 0) &&
			    (skin_step->skin_step[i].curr_ma == 0)) {
				skin_step->max_step = i;
				break;
			}
		}
	} else {
		skin_step->max_step = rc;
	}
	for (i = 0; i < skin_step->max_step; i++) {
		chg_info("bpp-skin-step: %d %d %d\n", skin_step->skin_step[i].low_threshold,
			skin_step->skin_step[i].high_threshold, skin_step->skin_step[i].curr_ma);
	}

	skin_step = &wls_dev->epp_plus_led_on_skin_step;
	skin_step->skin_step = dynamic_cfg->epp_plus_led_on_skin_step;
	rc = read_skin_range_data_from_node(node, "oplus,epp_plus-led-on-skin-step", skin_step->skin_step,
		WLS_SKIN_TEMP_MAX, wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP_PLUS]);
	if (rc < 0) {
		for (i = 0; i < WLS_MAX_STEP_CHG_ENTRIES; i++) {
			skin_step->skin_step[i].low_threshold = default_config.epp_plus_led_on_skin_step[i].low_threshold;
			skin_step->skin_step[i].high_threshold = default_config.epp_plus_led_on_skin_step[i].high_threshold;
			skin_step->skin_step[i].curr_ma = default_config.epp_plus_led_on_skin_step[i].curr_ma;
			if ((skin_step->skin_step[i].low_threshold == 0) &&
			    (skin_step->skin_step[i].high_threshold == 0) &&
			    (skin_step->skin_step[i].curr_ma == 0)) {
				skin_step->max_step = i;
				break;
			}
		}
	} else {
		skin_step->max_step = rc;
	}
	for (i = 0; i < skin_step->max_step; i++) {
		chg_info("epp_plus-led-on-skin-step: %d %d %d\n", skin_step->skin_step[i].low_threshold,
			skin_step->skin_step[i].high_threshold, skin_step->skin_step[i].curr_ma);
	}

	skin_step = &wls_dev->epp_led_on_skin_step;
	skin_step->skin_step = dynamic_cfg->epp_led_on_skin_step;
	rc = read_skin_range_data_from_node(node, "oplus,epp-led-on-skin-step", skin_step->skin_step,
		WLS_SKIN_TEMP_MAX, wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP]);
	if (rc < 0) {
		for (i = 0; i < WLS_MAX_STEP_CHG_ENTRIES; i++) {
			skin_step->skin_step[i].low_threshold = default_config.epp_led_on_skin_step[i].low_threshold;
			skin_step->skin_step[i].high_threshold = default_config.epp_led_on_skin_step[i].high_threshold;
			skin_step->skin_step[i].curr_ma = default_config.epp_led_on_skin_step[i].curr_ma;
			if ((skin_step->skin_step[i].low_threshold == 0) &&
			    (skin_step->skin_step[i].high_threshold == 0) &&
			    (skin_step->skin_step[i].curr_ma == 0)) {
				skin_step->max_step = i;
				break;
			}
		}
	} else {
		skin_step->max_step = rc;
	}
	for (i = 0; i < skin_step->max_step; i++) {
		chg_info("epp-led-on-skin-step: %d %d %d\n", skin_step->skin_step[i].low_threshold,
			skin_step->skin_step[i].high_threshold, skin_step->skin_step[i].curr_ma);
	}

	chg_info("icl_max_ma BPP EPP EPP_PLUS AIRVOOC AIRSVOOC: %d %d %d %d %d\n",
		wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_BPP],
		wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP],
		wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_EPP_PLUS],
		wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_AIRVOOC],
		wls_dev->icl_max_ma[OPLUS_WLS_CHG_MODE_AIRSVOOC]);

	rc = of_property_read_u32(node, "oplus,wls-fast-chg-call-on-curr-ma",
		&dynamic_cfg->wls_fast_chg_call_on_curr_ma);
	if (rc < 0) {
		chg_err("oplus,oplus,wls-fast-chg-call-on-curr-ma reading failed, rc=%d\n", rc);
		dynamic_cfg->wls_fast_chg_call_on_curr_ma = 600;
	}
	rc = of_property_read_u32(node, "oplus,wls-fast-chg-camera-on-curr-ma",
		&dynamic_cfg->wls_fast_chg_camera_on_curr_ma);
	if (rc < 0) {
		chg_err("oplus,oplus,wls-fast-chg-camera-on-curr-ma reading failed, rc=%d\n", rc);
		dynamic_cfg->wls_fast_chg_camera_on_curr_ma = 600;
	}

	rc = of_property_read_u32(node, "oplus,fastchg-max-soc", &dynamic_cfg->fastchg_max_soc);
	if (rc < 0) {
		chg_err("oplus,fastchg-max-soc reading failed, rc=%d\n", rc);
		dynamic_cfg->fastchg_max_soc = 90;
	}

	rc = of_property_read_u32(node, "oplus,fastchg-max-temp", &dynamic_cfg->fastchg_max_temp);
	if (rc < 0) {
		chg_err("oplus,fastchg-max-temp reading failed, rc=%d\n", rc);
		dynamic_cfg->fastchg_max_temp = 440;
	}

	rc = read_unsigned_data_from_node(node, "oplus,strategy_soc",
		dynamic_cfg->wls_strategy_soc, WLS_SOC_NUM_MAX);
	if (rc < 0) {
		chg_err("get oplus,strategy_soc property error, rc=%d\n", rc);
		for (i = 0; i < WLS_SOC_NUM_MAX; i++)
			dynamic_cfg->wls_strategy_soc[i] = oplus_strategy_soc_default_para[i];
	} else {
		for (i = 0; i < WLS_SOC_NUM_MAX; i++)
			chg_info(" strategy_soc: %d", dynamic_cfg->wls_strategy_soc[i]);
	}

	return 0;
}

static int oplus_chg_wls_gpio_init(struct oplus_chg_wls *wls_dev)
{
	int rc = 0;
	struct device_node *node = wls_dev->dev->of_node;

	wls_dev->pinctrl = devm_pinctrl_get(wls_dev->dev);
	if (IS_ERR_OR_NULL(wls_dev->pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -ENODEV;
	}

	wls_dev->wrx_en_gpio = of_get_named_gpio(node, "oplus,wrx_en-gpio", 0);
	if (gpio_is_valid(wls_dev->wrx_en_gpio)) {
		rc = gpio_request(wls_dev->wrx_en_gpio, "wrx_en-gpio");
		if (rc < 0) {
			chg_err("wrx_en_gpio request error, rc=%d\n", rc);
			return rc;
		}
		wls_dev->wrx_en_active = pinctrl_lookup_state(wls_dev->pinctrl, "wrx_en_active");
		if (IS_ERR_OR_NULL(wls_dev->wrx_en_active)) {
			chg_err("get wrx_en_active fail\n");
			goto free_wrx_en_gpio;
		}
		wls_dev->wrx_en_sleep = pinctrl_lookup_state(wls_dev->pinctrl, "wrx_en_sleep");
		if (IS_ERR_OR_NULL(wls_dev->wrx_en_sleep)) {
			chg_err("get wrx_en_sleep fail\n");
			goto free_wrx_en_gpio;
		}
		gpio_direction_output(wls_dev->wrx_en_gpio, 0);
		pinctrl_select_state(wls_dev->pinctrl, wls_dev->wrx_en_sleep);
	}

	if (!wls_dev->support_fastchg)
		return 0;

	wls_dev->ext_pwr_en_gpio = of_get_named_gpio(node, "oplus,ext_pwr_en-gpio", 0);
	if (!gpio_is_valid(wls_dev->ext_pwr_en_gpio)) {
		chg_err("ext_pwr_en_gpio not specified\n");
		goto free_wrx_en_gpio;
	}
	rc = gpio_request(wls_dev->ext_pwr_en_gpio, "ext_pwr_en-gpio");
	if (rc < 0) {
		chg_err("ext_pwr_en_gpio request error, rc=%d\n", rc);
		goto free_wrx_en_gpio;
	}
	wls_dev->ext_pwr_en_active = pinctrl_lookup_state(wls_dev->pinctrl, "ext_pwr_en_active");
	if (IS_ERR_OR_NULL(wls_dev->ext_pwr_en_active)) {
		chg_err("get ext_pwr_en_active fail\n");
		goto free_ext_pwr_en_gpio;
	}
	wls_dev->ext_pwr_en_sleep = pinctrl_lookup_state(wls_dev->pinctrl, "ext_pwr_en_sleep");
	if (IS_ERR_OR_NULL(wls_dev->ext_pwr_en_sleep)) {
		chg_err("get ext_pwr_en_sleep fail\n");
		goto free_ext_pwr_en_gpio;
	}
	gpio_direction_output(wls_dev->ext_pwr_en_gpio, 0);
	pinctrl_select_state(wls_dev->pinctrl, wls_dev->ext_pwr_en_sleep);

	wls_dev->usb_int_gpio = of_get_named_gpio(node, "oplus,usb_int-gpio", 0);
	if (!gpio_is_valid(wls_dev->usb_int_gpio)) {
		chg_err("usb_int_gpio not specified\n");
		goto out;
	}
	rc = gpio_request(wls_dev->usb_int_gpio, "usb_int-gpio");
	if (rc < 0) {
		chg_err("usb_int_gpio request error, rc=%d\n", rc);
		goto free_ext_pwr_en_gpio;
	}
	gpio_direction_input(wls_dev->usb_int_gpio);
	wls_dev->usb_int_irq = gpio_to_irq(wls_dev->usb_int_gpio);
	rc = devm_request_irq(wls_dev->dev, wls_dev->usb_int_irq,
			      oplus_chg_wls_usb_int_handler,
			      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			      "usb_int_irq", wls_dev);
	if (rc < 0) {
		chg_err("usb_int_irq request error, rc=%d\n", rc);
		goto free_int_gpio;
	}
	enable_irq_wake(wls_dev->usb_int_irq);

out:
	return 0;

free_int_gpio:
	if (!gpio_is_valid(wls_dev->usb_int_gpio))
		gpio_free(wls_dev->usb_int_gpio);
free_ext_pwr_en_gpio:
	if (!gpio_is_valid(wls_dev->ext_pwr_en_gpio))
		gpio_free(wls_dev->ext_pwr_en_gpio);
free_wrx_en_gpio:
	if (!gpio_is_valid(wls_dev->wrx_en_gpio))
		gpio_free(wls_dev->wrx_en_gpio);
	return rc;
}

static ssize_t oplus_chg_wls_proc_deviated_read(struct file *file,
						char __user *buf, size_t count,
						loff_t *ppos)
{
	uint8_t ret = 0;
	char page[7];
	size_t len = 7;

	memset(page, 0, 7);
	len = snprintf(page, len, "%s\n", "false");
	ret = simple_read_from_buffer(buf, count, ppos, page, len);

	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_deviated_ops =
{
	.read = oplus_chg_wls_proc_deviated_read,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_deviated_ops = {
	.proc_read = oplus_chg_wls_proc_deviated_read,
	.proc_write = NULL,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_tx_read(struct file *file, char __user *buf,
					  size_t count, loff_t *ppos)
{
	uint8_t ret = 0;
	char page[10] = { 0 };
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	union mms_msg_data data = { 0 };

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (wls_dev->wls_status.trx_online)
		data.intval = OPLUS_CHG_WLS_TRX_STATUS_CHARGING;
	else if (wls_dev->wls_status.trx_present || wls_dev->wls_status.trx_present_keep)
		data.intval = OPLUS_CHG_WLS_TRX_STATUS_ENABLE;
	else
		data.intval = OPLUS_CHG_WLS_TRX_STATUS_DISENABLE;

	/*
	*When the wireless reverse charging error occurs,
	*the upper layer turns off the reverse charging button.
	*/
	if (wls_dev->wls_status.trx_close_delay) {
		data.intval = OPLUS_CHG_WLS_TRX_STATUS_ENABLE;
		schedule_delayed_work(&wls_dev->wls_clear_trx_work, msecs_to_jiffies(3000));
	}

	switch (data.intval) {
	case OPLUS_CHG_WLS_TRX_STATUS_ENABLE:
		snprintf(page, 10, "%s\n", "enable");
		break;
	case OPLUS_CHG_WLS_TRX_STATUS_CHARGING:
		snprintf(page, 10, "%s\n", "charging");
		break;
	case OPLUS_CHG_WLS_TRX_STATUS_DISENABLE:
	default:
		snprintf(page, 10, "%s\n", "disable");
		break;
	}

	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));
	return ret;
}

static ssize_t oplus_chg_wls_proc_tx_write(struct file *file,
					   const char __user *buf, size_t count,
					   loff_t *lo)
{
	char buffer[5] = { 0 };
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int val;
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (count > sizeof(buffer) - 1)
		return -EFAULT;

	if (copy_from_user(buffer, buf, count)) {
		chg_err("%s: error.\n", __func__);
		return -EFAULT;
	}
	buffer[count] = '\0';
	chg_info("buffer=%s\n", buffer);
	rc = kstrtoint(strstrip(buffer), 0, &val);
	if (rc != 0)
		return -EINVAL;
	chg_info("val = %d\n", val);

	/*
	*When the wireless reverse charging error occurs,
	*the upper layer turns off the reverse charging button.
	*/
	wls_dev->wls_status.trx_close_delay = false;

	rc = oplus_chg_wls_set_trx_enable(wls_dev, !!val);
	if (rc < 0) {
		chg_err("can't enable trx, rc=%d\n", rc);
		return (ssize_t)rc;
	}
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_tx_ops =
{
	.read = oplus_chg_wls_proc_tx_read,
	.write = oplus_chg_wls_proc_tx_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_tx_ops = {
	.proc_read = oplus_chg_wls_proc_tx_read,
	.proc_write = oplus_chg_wls_proc_tx_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_rx_read(struct file *file, char __user *buf,
					  size_t count, loff_t *ppos)
{
	uint8_t ret = 0;
	char page[3];
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	memset(page, 0, 3);
	snprintf(page, 3, "%c\n", wls_dev->charge_enable ? '1' : '0');
	ret = simple_read_from_buffer(buf, count, ppos, page, 3);
	return ret;
}

static ssize_t oplus_chg_wls_proc_rx_write(struct file *file,
					   const char __user *buf, size_t count,
					   loff_t *lo)
{
	char buffer[5] = { 0 };
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int val;
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (count > sizeof(buffer) - 1)
		return -EFAULT;

	if (copy_from_user(buffer, buf, count)) {
		chg_err("%s: error.\n", __func__);
		return -EFAULT;
	}
	buffer[count] = '\0';
	chg_info("buffer=%s", buffer);
	rc = kstrtoint(strstrip(buffer), 0, &val);
	if (rc != 0)
		return -EINVAL;
	chg_info("val = %d", val);

	wls_dev->charge_enable = !!val;
	vote(wls_dev->rx_disable_votable, DEBUG_VOTER, !wls_dev->charge_enable,
	     1, false);
	chg_info("%s wls rx\n", wls_dev->charge_enable ? "enable" : "disable");
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_rx_ops =
{
	.read = oplus_chg_wls_proc_rx_read,
	.write = oplus_chg_wls_proc_rx_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_rx_ops = {
	.proc_read = oplus_chg_wls_proc_rx_read,
	.proc_write = oplus_chg_wls_proc_rx_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_user_sleep_mode_read(struct file *file,
						       char __user *buf,
						       size_t count,
						       loff_t *ppos)
{
	uint8_t ret = 0;
	char page[10];
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	chg_info("quiet_mode_need = %d.\n", wls_dev->wls_status.quiet_mode_need);
	sprintf(page, "%d", wls_dev->wls_status.quiet_mode_need);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_user_sleep_mode_write(struct file *file,
							const char __user *buf,
							size_t len, loff_t *lo)
{
	char buffer[5] = { 0 };
	int pmw_pulse = 0;
	int rc = -1;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	/*union oplus_chg_mod_propval pval;
	union mms_msg_data data = { 0 };*/

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (len > sizeof(buffer) - 1) {
		chg_err("len[%lu] -EFAULT\n", len);
		return -EFAULT;
	}

	if (copy_from_user(buffer, buf, len)) {
		chg_err("copy from user error\n");
		return -EFAULT;
	}
	buffer[len] = '\0';
	chg_info("user mode: buffer=%s\n", buffer);
	rc = kstrtoint(strstrip(buffer), 0, &pmw_pulse);
	if (rc != 0)
		return -EINVAL;
	if (pmw_pulse == WLS_FASTCHG_MODE) {
		/*pval.intval = 0;
		rc = oplus_chg_mod_set_property(wls_dev->wls_ocm,
				OPLUS_CHG_PROP_QUIET_MODE, &pval);
		if (rc == 0)*/
			wls_dev->wls_status.quiet_mode_need = WLS_FASTCHG_MODE;
		chg_info("set user mode: %d, fastchg mode, rc: %d\n", pmw_pulse,
		       rc);
	} else if (pmw_pulse == WLS_SILENT_MODE) {
		/*pval.intval = 1;
		rc = oplus_chg_mod_set_property(wls_dev->wls_ocm,
				OPLUS_CHG_PROP_QUIET_MODE, &pval);
		if (rc == 0)*/
			wls_dev->wls_status.quiet_mode_need = WLS_SILENT_MODE;
		chg_info("set user mode: %d, silent mode, rc: %d\n", pmw_pulse,
		       rc);
		/*nu1619_set_dock_led_pwm_pulse(3);*/
	} else if (pmw_pulse == WLS_BATTERY_FULL_MODE) {
		chg_info("set user mode: %d, battery full mode\n", pmw_pulse);
		wls_dev->wls_status.quiet_mode_need = WLS_BATTERY_FULL_MODE;
		/*nu1619_set_dock_fan_pwm_pulse(60);*/
	} else if (pmw_pulse == WLS_CALL_MODE) {
		chg_info("set user mode: %d, call mode\n", pmw_pulse);
		/*chip->nu1619_chg_status.call_mode = true;*/
	} else if (pmw_pulse == WLS_EXIT_CALL_MODE) {
		/*chip->nu1619_chg_status.call_mode = false;*/
		chg_info("set user mode: %d, exit call mode\n", pmw_pulse);
	} else {
		chg_info("user sleep mode: pmw_pulse: %d\n", pmw_pulse);
		wls_dev->wls_status.quiet_mode_need = pmw_pulse;
		/*nu1619_set_dock_fan_pwm_pulse(pmw_pulse);*/
	}

	return len;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_user_sleep_mode_ops =
{
	.read = oplus_chg_wls_proc_user_sleep_mode_read,
	.write = oplus_chg_wls_proc_user_sleep_mode_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_user_sleep_mode_ops = {
	.proc_read = oplus_chg_wls_proc_user_sleep_mode_read,
	.proc_write = oplus_chg_wls_proc_user_sleep_mode_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_idt_adc_test_read(struct file *file,
						    char __user *buf,
						    size_t count, loff_t *ppos)
{
	uint8_t ret = 0;
	char page[10];
	int rx_adc_result = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (wls_dev->wls_status.rx_adc_test_pass == true) {
		rx_adc_result = 1;
	} else {
		rx_adc_result = 0;
	}
	rx_adc_result = 1; /*needn't test*/
	/*chg_info("rx_adc_test: result %d.\n", rx_adc_result);*/
	sprintf(page, "%d", rx_adc_result);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_idt_adc_test_write(struct file *file,
						     const char __user *buf,
						     size_t len, loff_t *lo)
{
	char buffer[5] = { 0 };
	int rx_adc_cmd = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (len > sizeof(buffer) - 1) {
		chg_err("%s: len[%lu] -EFAULT.\n", __func__, len);
		return -EFAULT;
	}

	if (copy_from_user(buffer, buf, len)) {
		chg_err("%s:  error.\n", __func__);
		return -EFAULT;
	}
	buffer[len] = '\0';
	rc = kstrtoint(strstrip(buffer), 0, &rx_adc_cmd);
	if (rc != 0)
		return -EINVAL;
	if (rx_adc_cmd == 0) {
		chg_info("rx_adc_test: set 0.\n");
		wls_dev->wls_status.rx_adc_test_enable = false;
	} else if (rx_adc_cmd == 1) {
		chg_info("rx_adc_test: set 1.\n");
		wls_dev->wls_status.rx_adc_test_enable = true;
	} else {
		chg_info("rx_adc_test: set %d, invalid.\n", rx_adc_cmd);
	}

	return len;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_idt_adc_test_ops =
{
	.read = oplus_chg_wls_proc_idt_adc_test_read,
	.write = oplus_chg_wls_proc_idt_adc_test_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_idt_adc_test_ops = {
	.proc_read = oplus_chg_wls_proc_idt_adc_test_read,
	.proc_write = oplus_chg_wls_proc_idt_adc_test_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_rx_power_read(struct file *file,
						char __user *buf, size_t count,
						loff_t *ppos)
{
	uint8_t ret = 0;
	char page[16] = { 0 };
	int rx_power = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	rx_power = wls_dev->wls_status.rx_pwr_mw;
	if (wls_dev->wls_status.trx_online)
		rx_power = 1563;

	/*chg_info("rx_power = %d\n", rx_power);*/
	sprintf(page, "%d\n", rx_power);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_rx_power_write(struct file *file,
						 const char __user *buf,
						 size_t count, loff_t *lo)
{
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_rx_power_ops =
{
	.read = oplus_chg_wls_proc_rx_power_read,
	.write = oplus_chg_wls_proc_rx_power_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_rx_power_ops = {
	.proc_read = oplus_chg_wls_proc_rx_power_read,
	.proc_write = oplus_chg_wls_proc_rx_power_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_tx_power_read(struct file *file,
						char __user *buf, size_t count,
						loff_t *ppos)
{
	uint8_t ret = 0;
	char page[16] = { 0 };
	int tx_power = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	tx_power = wls_dev->wls_status.tx_pwr_mw;
	if (wls_dev->wls_status.trx_online)
		tx_power = wls_dev->wls_status.trx_vol_mv * wls_dev->wls_status.trx_curr_ma / 1000;

	/*chg_info("tx_power = %d\n", tx_power);*/
	sprintf(page, "%d\n", tx_power);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_tx_power_write(struct file *file,
						 const char __user *buf,
						 size_t count, loff_t *lo)
{
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_tx_power_ops =
{
	.read = oplus_chg_wls_proc_tx_power_read,
	.write = oplus_chg_wls_proc_tx_power_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_tx_power_ops = {
	.proc_read = oplus_chg_wls_proc_tx_power_read,
	.proc_write = oplus_chg_wls_proc_tx_power_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_rx_version_read(struct file *file,
						  char __user *buf,
						  size_t count, loff_t *ppos)
{
	uint8_t ret = 0;
	char page[16] = { 0 };
	u32 rx_version = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	rc = oplus_chg_wls_rx_get_rx_version(wls_dev->wls_rx->rx_ic, &rx_version);
	if (rc < 0) {
		chg_err("can't get rx version, rc=%d\n", rc);
		return rc;
	}

	/*chg_info("rx_version = 0x%x\n", rx_version);*/
	sprintf(page, "0x%x\n", rx_version);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_rx_version_write(struct file *file,
						   const char __user *buf,
						   size_t count, loff_t *lo)
{
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_rx_version_ops =
{
	.read = oplus_chg_wls_proc_rx_version_read,
	.write = oplus_chg_wls_proc_rx_version_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_rx_version_ops = {
	.proc_read = oplus_chg_wls_proc_rx_version_read,
	.proc_write = oplus_chg_wls_proc_rx_version_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_tx_version_read(struct file *file,
						  char __user *buf,
						  size_t count, loff_t *ppos)
{
	uint8_t ret = 0;
	char page[16] = { 0 };
	u32 tx_version = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	rc = oplus_chg_wls_rx_get_trx_version(wls_dev->wls_rx->rx_ic, &tx_version);
	if (rc < 0) {
		chg_err("can't get tx version, rc=%d\n", rc);
		return rc;
	}

	/*chg_info("tx_version = 0x%x\n", tx_version);*/
	sprintf(page, "0x%x\n", tx_version);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

static ssize_t oplus_chg_wls_proc_tx_version_write(struct file *file,
						   const char __user *buf,
						   size_t count, loff_t *lo)
{
	return count;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_tx_version_ops =
{
	.read = oplus_chg_wls_proc_tx_version_read,
	.write = oplus_chg_wls_proc_tx_version_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_tx_version_ops = {
	.proc_read = oplus_chg_wls_proc_tx_version_read,
	.proc_write = oplus_chg_wls_proc_tx_version_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static ssize_t oplus_chg_wls_proc_ftm_mode_read(struct file *file,
						char __user *buf, size_t count,
						loff_t *ppos)
{
	uint8_t ret = 0;
	char page[256];
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	sprintf(page, "ftm_mode[%d], engineering_mode[%d]\n", wls_dev->ftm_mode,
		wls_dev->factory_mode);
	ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

	return ret;
}

#define FTM_MODE_DISABLE 0
#define FTM_MODE_ENABLE 1
#define ENGINEERING_MODE_ENABLE 2
#define ENGINEERING_MODE_DISABLE 3
static ssize_t oplus_chg_wls_proc_ftm_mode_write(struct file *file,
						 const char __user *buf,
						 size_t len, loff_t *lo)
{
	char buffer[4] = { 0 };
	int ftm_mode = 0;
	struct oplus_chg_wls *wls_dev = pde_data(file_inode(file));
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is not fount\n");
		return -ENODEV;
	}

	if (len > sizeof(buffer) - 1) {
		chg_err("len[%lu] -EFAULT\n", len);
		return -EFAULT;
	}

	if (copy_from_user(buffer, buf, len)) {
		chg_err("copy from user error\n");
		return -EFAULT;
	}
	buffer[len] = '\0';
	chg_info("ftm mode: buffer=%s\n", buffer);
	rc = kstrtoint(strstrip(buffer), 0, &ftm_mode);
	if (rc != 0)
		return -EINVAL;
	if (ftm_mode == FTM_MODE_DISABLE) {
		wls_dev->ftm_mode = false;
	} else if (ftm_mode == FTM_MODE_ENABLE) {
		wls_dev->ftm_mode = true;
	} else if (ftm_mode == ENGINEERING_MODE_ENABLE) {
		wls_dev->factory_mode = true;
	} else if (ftm_mode == ENGINEERING_MODE_DISABLE) {
		wls_dev->factory_mode = false;
	}

	return len;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_chg_wls_proc_ftm_mode_ops =
{
	.read = oplus_chg_wls_proc_ftm_mode_read,
	.write = oplus_chg_wls_proc_ftm_mode_write,
	.open  = simple_open,
	.llseek = seq_lseek,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_chg_wls_proc_ftm_mode_ops = {
	.proc_read = oplus_chg_wls_proc_ftm_mode_read,
	.proc_write = oplus_chg_wls_proc_ftm_mode_write,
	.proc_open = simple_open,
	.proc_lseek = seq_lseek,
};
#endif

static int oplus_chg_wls_init_charge_proc(struct oplus_chg_wls *wls_dev)
{
	int ret = 0;
	struct proc_dir_entry *pr_entry_da = NULL;
	struct proc_dir_entry *pr_entry_tmp = NULL;

	pr_entry_da = proc_mkdir("wireless", NULL);
	if (pr_entry_da == NULL) {
		chg_err("Couldn't create wireless proc entry\n");
		return -ENOMEM;
	}

	pr_entry_tmp = proc_create_data("enable_tx", 0664, pr_entry_da,
					&oplus_chg_wls_proc_tx_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create enable_tx proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("deviated", 0664, pr_entry_da,
					&oplus_chg_wls_proc_deviated_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create deviated proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("user_sleep_mode", 0664, pr_entry_da,
					&oplus_chg_wls_proc_user_sleep_mode_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create user_sleep_mode proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("idt_adc_test", 0664, pr_entry_da,
					&oplus_chg_wls_proc_idt_adc_test_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create idt_adc_test proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("enable_rx", 0664, pr_entry_da,
					&oplus_chg_wls_proc_rx_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create enable_rx proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("rx_power", 0664, pr_entry_da,
					&oplus_chg_wls_proc_rx_power_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create rx_power proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("tx_power", 0664, pr_entry_da,
					&oplus_chg_wls_proc_tx_power_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create tx_power proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("rx_version", 0664, pr_entry_da,
					&oplus_chg_wls_proc_rx_version_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create rx_version proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("tx_version", 0664, pr_entry_da,
					&oplus_chg_wls_proc_tx_version_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create tx_version proc entry\n");
		goto fail;
	}

	pr_entry_tmp = proc_create_data("ftm_mode", 0664, pr_entry_da,
					&oplus_chg_wls_proc_ftm_mode_ops, wls_dev);
	if (pr_entry_tmp == NULL) {
		ret = -ENOMEM;
		chg_err("Couldn't create ftm_mode proc entry\n");
		goto fail;
	}

	return 0;

fail:
	remove_proc_entry("wireless", NULL);
	return ret;
}

static void oplus_chg_wls_mutual_init(struct oplus_chg_wls *wls_dev)
{
	wls_dev->mutual_cmd_data_ok = false;
	wls_dev->mutual_hidl_handle_cmd_ready = false;
	mutex_init(&wls_dev->mutual_read_lock);
	mutex_init(&wls_dev->mutual_cmd_data_lock);
	mutex_init(&wls_dev->mutual_cmd_ack_lock);
	init_waitqueue_head(&wls_dev->mutual_read_wq);
	init_completion(&wls_dev->mutual_cmd_ack);

	return;
}

static int oplus_chg_wls_mutual_event(struct oplus_chg_wls *wls_dev, void *buf)
{
	int i;
	struct oplus_chg_wls_status *wls_status = &wls_dev->wls_status;
	struct wls_third_part_auth_result *aes_auth_result;

	if (((struct oplus_chg_cmd *)buf)->cmd != CMD_WLS_THIRD_PART_AUTH) {
		chg_err("cmd is not matching, should return\n");
		return -EINVAL;
	}

	if (((struct oplus_chg_cmd *)buf)->data_size != sizeof(struct wls_third_part_auth_result)) {
		chg_err("data_len is not ok, datas is invalid\n");
		return -EINVAL;
	}

	aes_auth_result = (struct wls_third_part_auth_result *)(((struct oplus_chg_cmd *)buf)->data_buf);
	if (aes_auth_result) {
		wls_status->aes_key_num = aes_auth_result->effc_key_index;
		chg_info("aes_key_num:%d\n", wls_status->aes_key_num);

		memcpy(wls_status->aes_verfity_data.aes_random_num, aes_auth_result->aes_random_num,
			sizeof(aes_auth_result->aes_random_num));
		for (i = 0; i < WLS_AUTH_AES_ENCODE_LEN; i++)
			chg_info("aes_random_num[%d]:0x%02x\n", i, (wls_status->aes_verfity_data.aes_random_num)[i]);

		memcpy(wls_status->aes_verfity_data.aes_encode_num, aes_auth_result->aes_encode_num,
			sizeof(aes_auth_result->aes_encode_num));
		wls_status->aes_verity_data_ok = true;
		for (i = 0; i < WLS_AUTH_AES_ENCODE_LEN; i++)
			chg_info("aes_encode_num[%d]:0x%02x ", i, (wls_status->aes_verfity_data.aes_encode_num)[i]);
	}

	return 0;
}

static int oplus_chg_wls_set_mutual_cmd(struct oplus_chg_wls *wls_dev, u32 cmd, u32 data_size, const void *data_buf)
{
	int rc;

	if (wls_dev == NULL) {
		chg_err("wls dev is NULL\n");
		return CMD_ERROR_CHIP_NULL;
	}

	if (!data_buf)
		return CMD_ERROR_DATA_NULL;

	if (data_size > CHG_CMD_DATA_LEN) {
		chg_err("cmd data size is invalid\n");
		return CMD_ERROR_DATA_INVALID;
	}

	if (!wls_dev->mutual_hidl_handle_cmd_ready) {
		chg_err("hidl not read\n");
		return CMD_ERROR_HIDL_NOT_READY;
	}

	chg_info("start\n");
	mutex_lock(&wls_dev->mutual_cmd_ack_lock);
	mutex_lock(&wls_dev->mutual_cmd_data_lock);
	memset(&wls_dev->mutual_cmd, 0, sizeof(struct oplus_chg_cmd));
	wls_dev->mutual_cmd.cmd = cmd;
	wls_dev->mutual_cmd.data_size = data_size;
	memcpy(wls_dev->mutual_cmd.data_buf, data_buf, data_size);
	wls_dev->mutual_cmd_data_ok = true;
	mutex_unlock(&wls_dev->mutual_cmd_data_lock);
	wake_up(&wls_dev->mutual_read_wq);

	reinit_completion(&wls_dev->mutual_cmd_ack);
	rc = wait_for_completion_timeout(&wls_dev->mutual_cmd_ack, msecs_to_jiffies(CHG_CMD_TIME_MS));
	if (!rc) {
		chg_err("Error, timed out sending message\n");
		mutex_unlock(&wls_dev->mutual_cmd_ack_lock);
		return CMD_ERROR_TIME_OUT;
	}
	rc = CMD_ACK_OK;
	chg_info("success\n");
	mutex_unlock(&wls_dev->mutual_cmd_ack_lock);

	return rc;
}

ssize_t oplus_chg_wls_send_mutual_cmd(struct oplus_mms *mms, char *buf)
{
	int rc = 0;
	struct oplus_chg_cmd cmd;
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL || buf == NULL) {
		chg_err("mms or buf is NULL\n");
		return -ENODEV;
	}

	wls_dev = oplus_mms_get_drvdata(mms);
	if (wls_dev == NULL) {
		chg_err("wls dev is NULL\n");
		return -ENODEV;
	}

	wls_dev->mutual_hidl_handle_cmd_ready = true;

	mutex_lock(&wls_dev->mutual_read_lock);
	rc = wait_event_interruptible(wls_dev->mutual_read_wq, wls_dev->mutual_cmd_data_ok);
	mutex_unlock(&wls_dev->mutual_read_lock);
	if (rc)
		return rc;
	if (!wls_dev->mutual_cmd_data_ok)
		chg_err("oplus chg false wakeup, rc=%d\n", rc);
	chg_info("send\n");
	mutex_lock(&wls_dev->mutual_cmd_data_lock);
	wls_dev->mutual_cmd_data_ok = false;
	memcpy(&cmd, &wls_dev->mutual_cmd, sizeof(struct oplus_chg_cmd));
	mutex_unlock(&wls_dev->mutual_cmd_data_lock);
	memcpy(buf, &cmd, sizeof(struct oplus_chg_cmd));
	chg_info("success to copy to user space cmd:%d, size:%d\n",
		((struct oplus_chg_cmd *)buf)->cmd, ((struct oplus_chg_cmd *)buf)->data_size);

	return sizeof(struct oplus_chg_cmd);
}

void oplus_chg_wls_response_mutual_cmd(struct oplus_mms *mms, const char *buf, size_t count)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL || buf == NULL) {
		chg_err("mms or buf is NULL\n");
		return;
	}

	wls_dev = oplus_mms_get_drvdata(mms);
	if (wls_dev == NULL) {
		chg_err("wls dev is NULL\n");
		return;
	}

	if (count != sizeof(struct oplus_chg_cmd)) {
		chg_err("!!!size of buf is not matched\n");
		return;
	}
	chg_info("!!!cmd[%d]\n", ((struct oplus_chg_cmd *)buf)->cmd);

	oplus_chg_wls_mutual_event(wls_dev, (void *)buf);
	complete(&wls_dev->mutual_cmd_ack);

	return;
}

static void oplus_chg_wls_gauge_subs_callback(struct mms_subscribe *subs, enum mms_msg_type type, u32 id)
{
	struct oplus_chg_wls *wls_dev = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_SOC, &data, false);
		wls_dev->batt_info.soc = data.intval;
		oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_CURR, &data, false);
		wls_dev->batt_info.ibat_ma = data.intval;
		oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
		wls_dev->batt_info.vbat_mv = data.intval;
		oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_TEMP, &data, false);
		wls_dev->batt_info.batt_temp = data.intval;
		break;
	default:
		break;
	}
}

static void oplus_chg_wls_subscribe_gauge_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_chg_wls *wls_dev = prv_data;
	union mms_msg_data data = { 0 };

	wls_dev->gauge_topic = topic;
	wls_dev->gauge_subs = oplus_mms_subscribe(wls_dev->gauge_topic, wls_dev,
						   oplus_chg_wls_gauge_subs_callback,
						   "chg_wls");
	if (IS_ERR_OR_NULL(wls_dev->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n", PTR_ERR(wls_dev->gauge_subs));
		return;
	}

	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_SOC, &data, true);
	wls_dev->batt_info.soc = data.intval;
	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_CURR, &data, true);
	wls_dev->batt_info.ibat_ma = data.intval;
	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, true);
	wls_dev->batt_info.vbat_mv = data.intval;
	oplus_mms_get_item_data(wls_dev->gauge_topic, GAUGE_ITEM_TEMP, &data, true);
	wls_dev->batt_info.batt_temp = data.intval;
}

static void oplus_chg_wls_otg_enable_item_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork, struct oplus_chg_wls, wls_otg_enable_work);
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(wls_dev->wired_topic, WIRED_ITEM_OTG_ENABLE, &data, false);
	if (!data.intval) {
		/*for HW spec,need 100ms delay*/
		msleep(100);
		vote(wls_dev->wrx_en_votable, OTG_EN_VOTER, false, 0, false);
	}
}

static void oplus_chg_wls_wired_subs_callback(struct mms_subscribe *subs, enum mms_msg_type type, u32 id)
{
	struct oplus_chg_wls *wls_dev = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_PRESENT:
			oplus_mms_get_item_data(wls_dev->wired_topic, id, &data, false);
			if (data.intval) {
				chg_info("usb present\n");
				wls_dev->usb_present = true;
				schedule_delayed_work(&wls_dev->usb_int_work, 0);
				if (is_batt_psy_available(wls_dev))
					power_supply_changed(wls_dev->batt_psy);
			}
			break;
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(wls_dev->wired_topic, id, &data, false);
			if (data.intval) {
				chg_info("usb online\n");
				wls_dev->usb_present = true;
				schedule_delayed_work(&wls_dev->usb_int_work, 0);
				if (is_batt_psy_available(wls_dev))
					power_supply_changed(wls_dev->batt_psy);
			} else {
				chg_info("usb offline\n");
				wls_dev->usb_present = false;
				schedule_delayed_work(&wls_dev->usb_int_work, 0);
				if (wls_dev->wls_status.upgrade_fw_pending) {
					wls_dev->wls_status.upgrade_fw_pending = false;
					schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, 0);
				}
			}
			break;
		case WIRED_ITEM_PRE_OTG_ENABLE:
			vote(wls_dev->wrx_en_votable, OTG_EN_VOTER, true, 1, false);
			/*for HW spec,need 100ms delay*/
			msleep(100);
			break;
		case WIRED_ITEM_OTG_ENABLE:
			schedule_delayed_work(&wls_dev->wls_otg_enable_work, 0);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_chg_wls_subscribe_wired_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_chg_wls *wls_dev = prv_data;
	union mms_msg_data data = { 0 };

	wls_dev->wired_topic = topic;
	wls_dev->wired_subs = oplus_mms_subscribe(wls_dev->wired_topic, wls_dev,
						  oplus_chg_wls_wired_subs_callback,
						  "chg_wls");
	if (IS_ERR_OR_NULL(wls_dev->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n", PTR_ERR(wls_dev->wired_subs));
		return;
	}

	complete_all(&wls_dev->nor_ic_ack);
	oplus_mms_get_item_data(wls_dev->wired_topic, WIRED_ITEM_PRESENT, &data, true);
	oplus_mms_get_item_data(wls_dev->wired_topic, WIRED_ITEM_ONLINE, &data, true);
	oplus_mms_get_item_data(wls_dev->wired_topic, WIRED_ITEM_OTG_ENABLE, &data, true);
}

static void oplus_chg_wls_comm_subs_callback(struct mms_subscribe *subs, enum mms_msg_type type, u32 id)
{
	struct oplus_chg_wls *wls_dev = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_TEMP_REGION:
			schedule_work(&wls_dev->temp_region_update_work);
			break;
		case COMM_ITEM_UI_SOC:
			oplus_mms_get_item_data(wls_dev->comm_topic, id, &data, false);
			wls_dev->batt_info.ui_soc = data.intval;
			break;
		case COMM_ITEM_COOL_DOWN:
			schedule_work(&wls_dev->cool_down_update_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_chg_wls_subscribe_comm_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_chg_wls *wls_dev = prv_data;
	union mms_msg_data data = { 0 };

	wls_dev->comm_topic = topic;
	wls_dev->comm_subs = oplus_mms_subscribe(wls_dev->comm_topic, wls_dev,
						   oplus_chg_wls_comm_subs_callback,
						   "chg_wls");
	if (IS_ERR_OR_NULL(wls_dev->comm_subs)) {
		chg_err("subscribe comm topic error, rc=%ld\n", PTR_ERR(wls_dev->comm_subs));
		return;
	}

	oplus_mms_get_item_data(wls_dev->comm_topic, COMM_ITEM_TEMP_REGION, &data, true);
	wls_dev->wls_status.comm_temp_region = data.intval;
	oplus_mms_get_item_data(wls_dev->comm_topic, COMM_ITEM_UI_SOC, &data, true);
	wls_dev->batt_info.ui_soc = data.intval;
	oplus_mms_get_item_data(wls_dev->comm_topic, COMM_ITEM_COOL_DOWN, &data, true);
	wls_dev->wls_status.cool_down = data.intval < 0 ? 0 : data.intval;
}

static void oplus_chg_wls_temp_region_update_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, temp_region_update_work);
	enum oplus_chg_temp_region temp_region;
	enum oplus_chg_temp_region pre_temp_region;
	union mms_msg_data data = { 0 };
	int rc;

	if (wls_dev->comm_topic == NULL) {
		chg_err("comm_topic not ready\n");
		return;
	}

	pre_temp_region = oplus_chg_wls_get_temp_region(wls_dev);
	oplus_mms_get_item_data(wls_dev->comm_topic, COMM_ITEM_TEMP_REGION, &data, false);
	wls_dev->wls_status.comm_temp_region = data.intval;
	temp_region = oplus_chg_wls_get_temp_region(wls_dev);
	if (wls_dev->wls_status.fastchg_started) {
		if (!((pre_temp_region == BATT_TEMP_LITTLE_COOL && temp_region == BATT_TEMP_PRE_NORMAL) ||
		    (pre_temp_region == BATT_TEMP_PRE_NORMAL && temp_region == BATT_TEMP_LITTLE_COOL)))
			return;
	}
	if ((BATT_TEMP_COOL <= temp_region) && (temp_region <= BATT_TEMP_LITTLE_WARM)) {
		rc = oplus_chg_wls_choose_fastchg_curve(wls_dev);
		if (rc < 0)
			chg_err("choose fastchg curve failed, rc = %d\n", rc);
		else
			wls_dev->wls_status.fastchg_level = 0;
	}
	oplus_chg_wls_config(wls_dev);
}

static void oplus_chg_wls_cool_down_update_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, cool_down_update_work);
	union mms_msg_data data = { 0 };

	if (wls_dev->comm_topic == NULL) {
		chg_err("comm_topic not ready\n");
		return;
	}
	oplus_mms_get_item_data(wls_dev->comm_topic, COMM_ITEM_COOL_DOWN, &data, false);
	wls_dev->wls_status.cool_down = data.intval < 0 ? 0 : data.intval;
	chg_info("set cool down level to %d\n", wls_dev->wls_status.cool_down);
	oplus_chg_wls_config(wls_dev);
}

static void oplus_chg_wls_err_handler_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, wls_err_handler_work);
	struct oplus_chg_ic_err_msg *msg, *tmp;
	struct list_head msg_list;

	INIT_LIST_HEAD(&msg_list);
	spin_lock(&wls_dev->wls_rx->rx_ic->err_list_lock);
	if (!list_empty(&wls_dev->wls_rx->rx_ic->err_list))
		list_replace_init(&wls_dev->wls_rx->rx_ic->err_list, &msg_list);
	spin_unlock(&wls_dev->wls_rx->rx_ic->err_list_lock);

	list_for_each_entry_safe(msg, tmp, &msg_list, list) {
		if (is_err_topic_available(wls_dev))
			oplus_mms_publish_ic_err_msg(wls_dev->err_topic, ERR_ITEM_IC, msg);
		oplus_print_ic_err(msg);
		list_del(&msg->list);
		kfree(msg);
	}
}

static void oplus_chg_wls_present_handler_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, wls_present_handler_work);
	struct mms_msg *msg;
	int rc;

	if (wls_dev->wls_topic == NULL) {
		chg_err("wls_topic not ready\n");
		return;
	}

	wls_dev->mms_info.rx_present = oplus_chg_wls_rx_is_connected(wls_dev->wls_rx->rx_ic);
	chg_info("wls present=%d\n", wls_dev->mms_info.rx_present);
	oplus_chg_wls_wireless_notifier_call(wls_dev, WLS_ITEM_PRESENT);
	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, WLS_ITEM_PRESENT);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(wls_dev->wls_topic, msg);
	if (rc < 0) {
		chg_err("publish wls present msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

static void oplus_chg_wls_online_handler_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, wls_online_handler_work);
	struct mms_msg *msg;
	int rc;

	if (wls_dev->wls_topic == NULL) {
		chg_err("wls_topic not ready\n");
		return;
	}

	chg_info("wls online\n");
	wls_dev->mms_info.rx_online = true;
	oplus_chg_wls_wireless_notifier_call(wls_dev, WLS_ITEM_ONLINE);
	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, WLS_ITEM_ONLINE);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(wls_dev->wls_topic, msg);
	if (rc < 0) {
		chg_err("publish wls present msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

static void oplus_chg_wls_offline_handler_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, wls_offline_handler_work);
	struct mms_msg *msg;
	int rc;

	if (wls_dev->wls_topic == NULL) {
		chg_err("wls_topic not ready\n");
		return;
	}

	chg_info("wls offline\n");
	wls_dev->mms_info.rx_online = false;
	oplus_chg_wls_wireless_notifier_call(wls_dev, WLS_ITEM_ONLINE);
	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM, WLS_ITEM_ONLINE);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(wls_dev->wls_topic, msg);
	if (rc < 0) {
		chg_err("publish wls offline msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

static void oplus_chg_wls_event_changed_handler_work(struct work_struct *work)
{
	struct oplus_chg_wls *wls_dev =
		container_of(work, struct oplus_chg_wls, wls_event_changed_handler_work);
	enum oplus_chg_wls_event_code event_code;
	enum oplus_chg_wls_rx_mode rx_mode;
	int rc;

	if (wls_dev->wls_topic == NULL) {
		chg_err("wls_topic not ready\n");
		return;
	}

	rc = oplus_chg_wls_rx_get_event_code(wls_dev->wls_rx->rx_ic, &event_code);
	if (rc < 0) {
		chg_err("get event code err\n");
		return;
	}
	wls_dev->mms_info.event_code = event_code;
	switch (event_code) {
	case WLS_EVENT_RX_EPP_CAP:
		oplus_chg_wls_rx_get_rx_mode(wls_dev->wls_rx->rx_ic, &rx_mode);
		if (rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_5W ||
		    rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP ||
		    rx_mode == OPLUS_CHG_WLS_RX_MODE_EPP_PLUS)
			oplus_chg_wls_nor_set_vindpm(wls_dev->wls_nor->nor_ic, WLS_VINDPM_EPP);
		break;
	case WLS_EVENT_RX_UVP_ALARM:
		vote(wls_dev->nor_input_disable_votable, UOVP_VOTER, true, 0, false);
		break;
	case WLS_EVENT_RX_UVP_CLEAR:
		vote(wls_dev->nor_input_disable_votable, UOVP_VOTER, false, 0, false);
		break;
	case WLS_EVENT_TRX_CHECK:
		if (wls_dev->wls_status.trx_present) {
			cancel_delayed_work(&wls_dev->wls_trx_sm_work);
			queue_delayed_work(wls_dev->wls_wq, &wls_dev->wls_trx_sm_work, 0);
		} else {
			chg_err("trx not present\n");
		}
		break;
	default:
		break;
	}
}

static void oplus_chg_wls_err_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_chg_wls *wls_dev = virq_data;

	schedule_work(&wls_dev->wls_err_handler_work);
}

static void oplus_chg_wls_present_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_chg_wls *wls_dev = virq_data;

	schedule_work(&wls_dev->wls_present_handler_work);
}

static void oplus_chg_wls_online_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_chg_wls *wls_dev = virq_data;

	schedule_work(&wls_dev->wls_online_handler_work);
}

static void oplus_chg_wls_offline_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_chg_wls *wls_dev = virq_data;

	schedule_work(&wls_dev->wls_offline_handler_work);
}

static void oplus_chg_wls_event_changed_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_chg_wls *wls_dev = virq_data;

	schedule_work(&wls_dev->wls_event_changed_handler_work);
}

static int oplus_chg_wls_virq_register(struct oplus_chg_wls *wls_dev)
{
	int rc;

	rc = oplus_chg_ic_virq_register(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_ERR,
		oplus_chg_wls_err_handler, wls_dev);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_PRESENT,
		oplus_chg_wls_present_handler, wls_dev);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_PRESENT error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_ONLINE,
		oplus_chg_wls_online_handler, wls_dev);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_OFFLINE,
		oplus_chg_wls_offline_handler, wls_dev);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_OFFLINE error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(wls_dev->wls_rx->rx_ic, OPLUS_IC_VIRQ_EVENT_CHANGED,
		oplus_chg_wls_event_changed_handler, wls_dev);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_EVENT_CHANGED error, rc=%d", rc);

	return 0;
}

static int oplus_chg_wls_mms_update_present(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	bool present = false;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	present = wls_dev->wls_status.online_keep | wls_dev->wls_status.boot_online_keep;
	present = present | wls_dev->mms_info.rx_present;

	data->intval = present;
	return 0;
}

static int oplus_chg_wls_mms_update_online(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	bool online = false;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	online = wls_dev->mms_info.rx_online;

	data->intval = online;
	return 0;
}

static int oplus_chg_wls_mms_update_vout(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	int vout = 0;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	vout = wls_dev->wls_status.vout_mv;
	if (wls_dev->wls_status.trx_present) {
		rc = oplus_chg_wls_rx_get_trx_vol(wls_dev->wls_rx->rx_ic, &wls_dev->wls_status.trx_vol_mv);
		if (rc < 0)
			chg_err("can't get trx vol, rc=%d\n", rc);
		else
			vout = wls_dev->wls_status.trx_vol_mv;
	}

	data->intval = vout;
	return 0;
}

static int oplus_chg_wls_mms_update_iout(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	int iout = 0;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	iout = wls_dev->wls_status.iout_ma;
	if (wls_dev->wls_status.trx_present) {
		rc = oplus_chg_wls_rx_get_trx_curr(wls_dev->wls_rx->rx_ic, &wls_dev->wls_status.trx_curr_ma);
		if (rc < 0)
			chg_err("can't get trx curr, rc=%d\n", rc);
		else
			iout = wls_dev->wls_status.trx_curr_ma;
	}

	data->intval = iout;
	return 0;
}

static int oplus_chg_wls_mms_update_wls_type(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	enum oplus_chg_wls_type type = OPLUS_CHG_WLS_UNKNOWN;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	type = wls_dev->wls_status.wls_type;

	data->intval = type;
	return 0;
}

static int oplus_chg_wls_mms_update_real_type(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	enum power_supply_type real_type = POWER_SUPPLY_TYPE_UNKNOWN;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	switch (wls_dev->wls_status.wls_type) {
	case OPLUS_CHG_WLS_BPP:
		real_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	case OPLUS_CHG_WLS_EPP:
		real_type = POWER_SUPPLY_TYPE_USB_PD;
		break;
	case OPLUS_CHG_WLS_EPP_PLUS:
		real_type = POWER_SUPPLY_TYPE_USB_PD;
		break;
	case OPLUS_CHG_WLS_VOOC:
	case OPLUS_CHG_WLS_SVOOC:
	case OPLUS_CHG_WLS_PD_65W:
		real_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	default:
		chg_err("Unsupported charging mode(=%d)\n", wls_dev->wls_status.wls_type);
		real_type = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	}

	data->intval = real_type;
	return 0;
}

static int oplus_chg_wls_mms_update_status_keep(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	enum wls_status_keep_type status_keep = WLS_SK_NULL;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	status_keep = wls_dev->mms_info.wls_status_keep;

	data->intval = status_keep;
	return 0;
}

int oplus_chg_wls_set_status_keep(struct oplus_mms *mms, enum wls_status_keep_type status_keep)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -ENODEV;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
	wls_dev->mms_info.wls_status_keep = status_keep;
	return 0;
}

static int oplus_chg_wls_mms_update_dock_type(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	data->intval = wls_dev->wls_status.adapter_id;

	return 0;
}

static int oplus_chg_wls_mms_update_max_power(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	data->intval = oplus_chg_wls_get_max_wireless_power(wls_dev);

	return 0;
}

static int oplus_chg_wls_mms_update_fastchg_status(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);

	data->intval = wls_dev->wls_status.fastchg_started;

	return 0;
}

static int oplus_chg_wls_mms_update_bcc_max_curr(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	struct oplus_chg_wls_status *wls_status;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
	wls_status = &wls_dev->wls_status;

	data->intval = wls_status->wls_bcc_max_curr / BCC_TO_ICL;

	return 0;
}

static int oplus_chg_wls_mms_update_bcc_min_curr(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	struct oplus_chg_wls_status *wls_status;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
	wls_status = &wls_dev->wls_status;

	data->intval = wls_status->wls_bcc_min_curr / BCC_TO_ICL;

	return 0;
}

static int oplus_chg_wls_mms_update_bcc_exit_curr(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	struct oplus_chg_wls_dynamic_config *dynamic_cfg;
	struct oplus_chg_wls_status *wls_status;
	enum oplus_chg_temp_region temp_region;
	int batt_soc_plugin, batt_temp_plugin;
	int real_soc = 100;
	int rc = 0;
	int wls_stop_curr = 1000;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
	dynamic_cfg = &wls_dev->dynamic_config;
	wls_status = &wls_dev->wls_status;

	rc = oplus_chg_wls_get_real_soc(wls_dev, &real_soc);
	if ((rc < 0) || (real_soc >= dynamic_cfg->fastchg_max_soc)) {
		chg_err("can't get real soc or soc is too high, rc=%d\n", rc);
		return -EPERM;
	}

	if (real_soc < 30)
		batt_soc_plugin = WLS_BCC_SOC_0_TO_30;
	else if (real_soc < 70)
		batt_soc_plugin = WLS_BCC_SOC_30_TO_70;
	else if (real_soc < 90)
		batt_soc_plugin = WLS_BCC_SOC_70_TO_90;
	else
		batt_soc_plugin = WLS_BCC_SOC_70_TO_90;

	temp_region = oplus_chg_wls_get_temp_region(wls_dev);


	if (temp_region <= BATT_TEMP_COOL)
		batt_temp_plugin = WLS_BCC_TEMP_0_TO_50;
	else if (temp_region == BATT_TEMP_LITTLE_COOL)
		batt_temp_plugin = WLS_BCC_TEMP_50_TO_120;
	else if (temp_region == BATT_TEMP_PRE_NORMAL)
		batt_temp_plugin = WLS_BCC_TEMP_120_TO_160;
	else if (temp_region == BATT_TEMP_NORMAL)
		batt_temp_plugin = WLS_BCC_TEMP_160_TO_400;
	else if (temp_region == BATT_TEMP_LITTLE_WARM)
		batt_temp_plugin = WLS_BCC_TEMP_400_TO_440;
	else
		batt_temp_plugin = WLS_BCC_TEMP_400_TO_440;

	if (batt_soc_plugin == WLS_BCC_SOC_0_TO_30) {
		switch(batt_temp_plugin) {
		case WLS_BCC_TEMP_0_TO_50:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_0_to_30[WLS_BCC_TEMP_0_TO_50];
			break;
		case WLS_BCC_TEMP_50_TO_120:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_0_to_30[WLS_BCC_TEMP_50_TO_120];
			break;
		case WLS_BCC_TEMP_120_TO_160:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_0_to_30[WLS_BCC_TEMP_120_TO_160];
			break;
		case WLS_BCC_TEMP_160_TO_400:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_0_to_30[WLS_BCC_TEMP_160_TO_400];
			break;
		case WLS_BCC_TEMP_400_TO_440:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_0_to_30[WLS_BCC_TEMP_400_TO_440];
			break;
		default:
			break;
		}
	}

	if (batt_soc_plugin == WLS_BCC_SOC_30_TO_70) {
		switch(batt_temp_plugin) {
		case WLS_BCC_TEMP_0_TO_50:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_30_to_70[WLS_BCC_TEMP_0_TO_50];
			break;
		case WLS_BCC_TEMP_50_TO_120:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_30_to_70[WLS_BCC_TEMP_50_TO_120];
			break;
		case WLS_BCC_TEMP_120_TO_160:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_30_to_70[WLS_BCC_TEMP_120_TO_160];
			break;
		case WLS_BCC_TEMP_160_TO_400:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_30_to_70[WLS_BCC_TEMP_160_TO_400];
			break;
		case WLS_BCC_TEMP_400_TO_440:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_30_to_70[WLS_BCC_TEMP_400_TO_440];
			break;
		default:
			break;
		}
	}

	if (batt_soc_plugin == WLS_BCC_SOC_70_TO_90) {
		switch(batt_temp_plugin) {
		case WLS_BCC_TEMP_0_TO_50:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_70_to_90[WLS_BCC_TEMP_0_TO_50];
			break;
		case WLS_BCC_TEMP_50_TO_120:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_70_to_90[WLS_BCC_TEMP_50_TO_120];
			break;
		case WLS_BCC_TEMP_120_TO_160:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_70_to_90[WLS_BCC_TEMP_120_TO_160];
			break;
		case WLS_BCC_TEMP_160_TO_400:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_70_to_90[WLS_BCC_TEMP_160_TO_400];
			break;
		case WLS_BCC_TEMP_400_TO_440:
			wls_stop_curr = dynamic_cfg->bcc_stop_curr_70_to_90[WLS_BCC_TEMP_400_TO_440];
			break;
		default:
			break;
		}
	}
	chg_info("get stop curr, enter soc is [%d], batt_temp_plugin[%d], stop curr is %d\n",
	         batt_soc_plugin, batt_temp_plugin, wls_stop_curr);
	data->intval = wls_stop_curr  / BCC_TO_ICL;

	return 0;
}

static int oplus_chg_wls_update_bcc_temp_range(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_chg_wls *wls_dev;
	struct oplus_chg_wls_status *wls_status;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	wls_dev = oplus_mms_get_drvdata(mms);
	wls_status = &wls_dev->wls_status;

	if (wls_dev->support_wls_chg_bcc &&
	    (wls_status->bcc_temp_range == WLS_BCC_TEMP_160_TO_400 ||
	     wls_status->bcc_temp_range == WLS_BCC_TEMP_400_TO_440))
	     data->intval = true;

	return 0;
}

static void oplus_chg_wls_mms_update(struct oplus_mms *mms, bool publish)
{
	/*only for auto update*/
	return;
}

static struct mms_item oplus_chg_wls_mms_item[] = {
	{
		.desc = {
			.item_id = WLS_ITEM_PRESENT,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_present,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_ONLINE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_online,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_VOUT,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_vout,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_IOUT,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_iout,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_WLS_TYPE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_wls_type,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_REAL_TYPE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_real_type,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_STATUS_KEEP,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_status_keep,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_DOCK_TYPE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_dock_type,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_MAX_POWER,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_max_power,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_FASTCHG_STATUS,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_fastchg_status,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_BCC_MAX_CURR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_bcc_max_curr,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_BCC_MIN_CURR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_bcc_min_curr,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_BCC_EXIT_CURR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_mms_update_bcc_exit_curr,
		}
	},
	{
		.desc = {
			.item_id = WLS_ITEM_BCC_TEMP_RANGE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_chg_wls_update_bcc_temp_range,
		}
	},
};

static const struct oplus_mms_desc oplus_chg_wls_mms_desc = {
	.name = "wireless",
	.type = OPLUS_MMS_TYPE_WLS,
	.item_table = oplus_chg_wls_mms_item,
	.item_num = ARRAY_SIZE(oplus_chg_wls_mms_item),
	.update_items = NULL,
	.update_items_num = 0,
	.update_interval = 0, /* ms */
	.update = oplus_chg_wls_mms_update,
};

static int oplus_chg_wls_topic_init(struct oplus_chg_wls *wls_dev)
{
	struct oplus_mms_config mms_cfg = {};
	int rc;

	mms_cfg.drv_data = wls_dev;
	mms_cfg.of_node = wls_dev->dev->of_node;

	if (of_property_read_bool(mms_cfg.of_node, "oplus,topic-update-interval")) {
		rc = of_property_read_u32(mms_cfg.of_node, "oplus,topic-update-interval", &mms_cfg.update_interval);
		if (rc < 0)
			mms_cfg.update_interval = 0;
	}

	wls_dev->wls_topic = devm_oplus_mms_register(wls_dev->dev, &oplus_chg_wls_mms_desc, &mms_cfg);
	if (IS_ERR(wls_dev->wls_topic)) {
		rc = PTR_ERR(wls_dev->wls_topic);
		wls_dev->wls_topic = NULL;
		chg_err("Couldn't register wls topic\n");
		return rc;
	}

	return 0;
}

#define NOR_IC_INIT_TIMEOUT_MS	((OPLUS_CHG_IC_INIT_RETRY_DELAY) * (OPLUS_CHG_IC_INIT_RETRY_MAX))
static void oplus_chg_wls_mms_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_chg_wls *wls_dev = container_of(dwork,
		struct oplus_chg_wls, wls_mms_init_work);
	struct device_node *node = wls_dev->dev->of_node;
	static int retry = OPLUS_CHG_IC_INIT_RETRY_MAX;
	int rc;

	wls_dev->wls_rx->rx_ic = of_get_oplus_chg_ic(node, "oplus,rx_ic", 0);
	if (wls_dev->wls_rx->rx_ic == NULL) {
		if (retry > 0) {
			retry--;
			schedule_delayed_work(&wls_dev->wls_mms_init_work,
				msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
			return;
		} else {
			chg_err("oplus,rx_ic not found\n");
		}
		retry = 0;
		return;
	}

	/*wls topic init after rx ic and before nor ic, becase nor ic init very, very late in adsp project*/
	if (wls_dev->wls_topic == NULL)
		(void)oplus_chg_wls_topic_init(wls_dev);

	wls_dev->wls_nor->nor_ic = of_get_oplus_chg_ic(node, "oplus,nor_ic", 0);
	if (wls_dev->wls_nor->nor_ic == NULL) {
		if (retry > 0) {
			retry--;
			schedule_delayed_work(&wls_dev->wls_mms_init_work,
				msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
			return;
		} else {
			chg_err("oplus,nor_ic not found\n");
		}
		retry = 0;
		return;
	}

	if (wls_dev->support_fastchg) {
		wls_dev->wls_fast->fast_ic = of_get_oplus_chg_ic(node, "oplus,fast_ic", 0);
		if (wls_dev->wls_fast->fast_ic == NULL) {
			if (retry > 0) {
				retry--;
				schedule_delayed_work(&wls_dev->wls_mms_init_work,
					msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
				return;
			} else {
				chg_err("oplus,fast_ic not found\n");
			}
			retry = 0;
			return;
		}
	}

	/*virq_register need before OPLUS_IC_FUNC_INIT*/
	(void)oplus_chg_wls_virq_register(wls_dev);

	/*need nor_ic init done before rx_ic init*/
	rc = wait_for_completion_timeout(&wls_dev->nor_ic_ack, msecs_to_jiffies(NOR_IC_INIT_TIMEOUT_MS));
	if (!rc) {
		chg_err("nor_ic init timedout\n");
		return;
	}

	rc = oplus_chg_ic_func(wls_dev->wls_rx->rx_ic, OPLUS_IC_FUNC_INIT);
	if (rc == -EAGAIN) {
		if (retry > 0) {
			retry--;
			schedule_delayed_work(&wls_dev->wls_mms_init_work,
				msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
			return;
		} else {
			chg_err("rx_ic init timeout\n");
		}
		retry = 0;
		return;
	} else if (rc < 0) {
		chg_err("rx_ic init error, rc=%d\n", rc);
		retry = 0;
		return;
	}
	retry = 0;
	chg_info("wls topic OPLUS_IC_FUNC_INIT success\n");

	schedule_delayed_work(&wls_dev->wls_upgrade_fw_work, msecs_to_jiffies(1000));
	return;
}

#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
#include "config/dynamic_cfg/oplus_wls_cfg.c"
#endif

static int oplus_chg_wls_driver_probe(struct platform_device *pdev)
{
	struct oplus_chg_wls *wls_dev;
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int boot_mode;
#endif
	bool usb_present;
	int rc;

	wls_dev = devm_kzalloc(&pdev->dev, sizeof(struct oplus_chg_wls), GFP_KERNEL);
	if (wls_dev == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}

	wls_dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, wls_dev);

	rc = oplus_chg_wls_parse_dt(wls_dev);
	if (rc < 0) {
		chg_err("oplus chg wls parse dts error, rc=%d\n", rc);
		goto parse_dt_err;
	}

	wls_dev->wls_wq = alloc_ordered_workqueue("wls_wq", WQ_FREEZABLE | WQ_HIGHPRI);
	if (!wls_dev->wls_wq) {
		chg_err("alloc wls work error\n");
		rc = -ENOMEM;
		goto alloc_work_err;
	}

	rc = oplus_chg_wls_rx_init(wls_dev);
	if (rc < 0) {
		goto rx_init_err;
	}
	rc = oplus_chg_wls_rx_register_msg_callback(wls_dev->wls_rx->rx_ic, wls_dev,
		oplus_chg_wls_rx_msg_callback);
	if (!rc)
		wls_dev->msg_callback_ok = true;
	rc = oplus_chg_wls_nor_init(wls_dev);
	if (rc < 0) {
		goto nor_init_err;
	}
	if (wls_dev->support_fastchg) {
		rc = oplus_chg_wls_fast_init(wls_dev);
		if (rc < 0) {
			goto fast_init_err;
		}
	}

	rc = oplus_chg_wls_gpio_init(wls_dev);
	if (rc < 0)
		goto gpio_init_err;

	wls_dev->fcc_votable = create_votable("WLS_FCC", VOTE_MIN,
				oplus_chg_wls_fcc_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->fcc_votable)) {
		rc = PTR_ERR(wls_dev->fcc_votable);
		wls_dev->fcc_votable = NULL;
		goto create_fcc_votable_err;
	}

	wls_dev->fastchg_disable_votable = create_votable("WLS_FASTCHG_DISABLE",
				VOTE_SET_ANY,
				oplus_chg_wls_fastchg_disable_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->fastchg_disable_votable)) {
		rc = PTR_ERR(wls_dev->fastchg_disable_votable);
		wls_dev->fastchg_disable_votable = NULL;
		goto create_disable_votable_err;
	}
	wls_dev->wrx_en_votable = create_votable("WLS_WRX_EN",
				VOTE_SET_ANY,
				oplus_chg_wls_wrx_en_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->wrx_en_votable)) {
		rc = PTR_ERR(wls_dev->wrx_en_votable);
		wls_dev->wrx_en_votable = NULL;
		goto create_wrx_en_votable_err;
	}
	wls_dev->nor_icl_votable = create_votable("WLS_NOR_ICL",
				VOTE_MIN,
				oplus_chg_wls_nor_icl_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->nor_icl_votable)) {
		rc = PTR_ERR(wls_dev->nor_icl_votable);
		wls_dev->nor_icl_votable = NULL;
		goto create_nor_icl_votable_err;
	}
	wls_dev->nor_fcc_votable = create_votable("WLS_NOR_FCC",
				VOTE_MIN,
				oplus_chg_wls_nor_fcc_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->nor_fcc_votable)) {
		rc = PTR_ERR(wls_dev->nor_fcc_votable);
		wls_dev->nor_fcc_votable = NULL;
		goto create_nor_fcc_votable_err;
	}
	/*wls_dev->nor_fv_votable = create_votable("WLS_NOR_FV",
				VOTE_MIN,
				oplus_chg_wls_nor_fv_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->nor_fv_votable)) {
		rc = PTR_ERR(wls_dev->nor_fv_votable);
		wls_dev->nor_fv_votable = NULL;
		goto create_nor_fv_votable_err;
	}*/
	wls_dev->nor_out_disable_votable = create_votable("WLS_NOR_OUT_DISABLE",
				VOTE_SET_ANY,
				oplus_chg_wls_nor_out_disable_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->nor_out_disable_votable)) {
		rc = PTR_ERR(wls_dev->nor_out_disable_votable);
		wls_dev->nor_out_disable_votable = NULL;
		goto create_nor_out_disable_votable_err;
	}
	wls_dev->nor_input_disable_votable = create_votable("WLS_NOR_INPUT_DISABLE",
				VOTE_SET_ANY,
				oplus_chg_wls_nor_input_disable_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->nor_input_disable_votable)) {
		rc = PTR_ERR(wls_dev->nor_input_disable_votable);
		wls_dev->nor_input_disable_votable = NULL;
		goto create_nor_input_disable_votable_err;
	}
	wls_dev->rx_disable_votable = create_votable("WLS_RX_DISABLE",
				VOTE_SET_ANY,
				oplus_chg_wls_rx_disable_vote_callback,
				wls_dev);
	if (IS_ERR(wls_dev->rx_disable_votable)) {
		rc = PTR_ERR(wls_dev->rx_disable_votable);
		wls_dev->rx_disable_votable = NULL;
		goto create_rx_disable_votable_err;
	}

	wls_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	wls_dev->misc_dev.name = "wls_dev";
	wls_dev->misc_dev.fops = &oplus_chg_wls_dev_fops;
	rc = misc_register(&wls_dev->misc_dev);
	if (rc) {
		chg_err("misc_register failed, rc=%d\n", rc);
		goto misc_reg_err;
	}

	rc = oplus_chg_wls_init_charge_proc(wls_dev);
	if (rc < 0) {
		chg_err("can't init charge proc, rc=%d\n", rc);
		goto init_charge_proc_err;
	}

	oplus_chg_wls_mutual_init(wls_dev);

	INIT_DELAYED_WORK(&wls_dev->wls_rx_sm_work, oplus_chg_wls_rx_sm);
	INIT_DELAYED_WORK(&wls_dev->wls_trx_sm_work, oplus_chg_wls_trx_sm);
	INIT_DELAYED_WORK(&wls_dev->wls_connect_work, oplus_chg_wls_connect_work);
	INIT_DELAYED_WORK(&wls_dev->wls_send_msg_work, oplus_chg_wls_send_msg_work);
	INIT_DELAYED_WORK(&wls_dev->wls_upgrade_fw_work, oplus_chg_wls_upgrade_fw_work);
	INIT_DELAYED_WORK(&wls_dev->usb_int_work, oplus_chg_wls_usb_int_work);
	INIT_DELAYED_WORK(&wls_dev->wls_data_update_work, oplus_chg_wls_data_update_work);
	INIT_DELAYED_WORK(&wls_dev->wls_trx_disconnect_work, oplus_chg_wls_trx_disconnect_work);
	INIT_DELAYED_WORK(&wls_dev->usb_connect_work, oplus_chg_wls_usb_connect_work);
	INIT_DELAYED_WORK(&wls_dev->wls_start_work, oplus_chg_wls_start_work);
	INIT_DELAYED_WORK(&wls_dev->wls_break_work, oplus_chg_wls_break_work);
	INIT_DELAYED_WORK(&wls_dev->fod_cal_work, oplus_chg_wls_fod_cal_work);
	INIT_DELAYED_WORK(&wls_dev->rx_restore_work, oplus_chg_wls_rx_restore_work);
	INIT_DELAYED_WORK(&wls_dev->rx_iic_restore_work, oplus_chg_wls_rx_iic_restore_work);
	INIT_DELAYED_WORK(&wls_dev->rx_verity_restore_work, oplus_chg_wls_rx_verity_restore_work);
	INIT_DELAYED_WORK(&wls_dev->fast_fault_check_work, oplus_chg_wls_fast_fault_check_work);
	INIT_DELAYED_WORK(&wls_dev->rx_restart_work, oplus_chg_wls_rx_restart_work);
	INIT_DELAYED_WORK(&wls_dev->online_keep_remove_work, oplus_chg_wls_online_keep_remove_work);
	INIT_DELAYED_WORK(&wls_dev->verity_state_remove_work, oplus_chg_wls_verity_state_remove_work);
	INIT_DELAYED_WORK(&wls_dev->wls_verity_work, oplus_chg_wls_verity_work);
	INIT_DELAYED_WORK(&wls_dev->wls_get_third_part_verity_data_work,
		oplus_chg_wls_get_third_part_verity_data_work);
	INIT_DELAYED_WORK(&wls_dev->wls_clear_trx_work, oplus_chg_wls_clear_trx_work);
	INIT_DELAYED_WORK(&wls_dev->wls_skewing_work, oplus_chg_wls_skewing_work);
	INIT_DELAYED_WORK(&wls_dev->wls_bcc_curr_update_work, oplus_chg_wls_bcc_curr_update_work);
	INIT_DELAYED_WORK(&wls_dev->wls_vout_err_work, oplus_chg_wls_vout_err_work);
	INIT_DELAYED_WORK(&wls_dev->wls_mms_init_work, oplus_chg_wls_mms_init_work);
	INIT_DELAYED_WORK(&wls_dev->wls_otg_enable_work, oplus_chg_wls_otg_enable_item_work);
	INIT_WORK(&wls_dev->wls_err_handler_work, oplus_chg_wls_err_handler_work);
	INIT_WORK(&wls_dev->wls_present_handler_work, oplus_chg_wls_present_handler_work);
	INIT_WORK(&wls_dev->wls_online_handler_work, oplus_chg_wls_online_handler_work);
	INIT_WORK(&wls_dev->wls_offline_handler_work, oplus_chg_wls_offline_handler_work);
	INIT_WORK(&wls_dev->wls_event_changed_handler_work, oplus_chg_wls_event_changed_handler_work);
	INIT_WORK(&wls_dev->temp_region_update_work, oplus_chg_wls_temp_region_update_work);
	INIT_WORK(&wls_dev->cool_down_update_work, oplus_chg_wls_cool_down_update_work);
	init_completion(&wls_dev->msg_ack);
	init_completion(&wls_dev->nor_ic_ack);
	mutex_init(&wls_dev->connect_lock);
	mutex_init(&wls_dev->read_lock);
	mutex_init(&wls_dev->cmd_data_lock);
	mutex_init(&wls_dev->send_msg_lock);
	mutex_init(&wls_dev->update_data_lock);
	init_waitqueue_head(&wls_dev->read_wq);

	wls_dev->rx_wake_lock = wakeup_source_register(wls_dev->dev, "rx_wake_lock");
	wls_dev->trx_wake_lock = wakeup_source_register(wls_dev->dev, "trx_wake_lock");

	wls_dev->charge_enable = true;
	wls_dev->batt_charge_enable = true;

#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	boot_mode = get_boot_mode();
#ifndef CONFIG_OPLUS_CHARGER_MTK
	if (boot_mode == MSM_BOOT_MODE__FACTORY)
#else
	if (boot_mode == FACTORY_BOOT || boot_mode == ATE_FACTORY_BOOT)
#endif
		wls_dev->ftm_mode = true;
#endif

	usb_present = oplus_chg_wls_is_usb_present(wls_dev);
	if (usb_present) {
		wls_dev->usb_present = true;
		schedule_delayed_work(&wls_dev->usb_int_work, 0);
	} else {
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#ifndef CONFIG_OPLUS_CHARGER_MTK
		if (boot_mode == MSM_BOOT_MODE__CHARGE)
#else
		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
#endif
			wls_dev->wls_status.boot_online_keep = true;
#endif
	}

#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
	(void)oplus_wls_reg_debug_config(wls_dev);
#endif

	oplus_mms_wait_topic("gauge", oplus_chg_wls_subscribe_gauge_topic, wls_dev);
	oplus_mms_wait_topic("wired", oplus_chg_wls_subscribe_wired_topic, wls_dev);
	oplus_mms_wait_topic("common", oplus_chg_wls_subscribe_comm_topic, wls_dev);
	schedule_delayed_work(&wls_dev->wls_mms_init_work, 0);

	chg_info("probe done\n");

	return 0;


init_charge_proc_err:
	misc_deregister(&wls_dev->misc_dev);

misc_reg_err:
	destroy_votable(wls_dev->rx_disable_votable);
create_rx_disable_votable_err:
	destroy_votable(wls_dev->nor_input_disable_votable);
create_nor_input_disable_votable_err:
	destroy_votable(wls_dev->nor_out_disable_votable);
create_nor_out_disable_votable_err:
	/*destroy_votable(wls_dev->nor_fv_votable);
create_nor_fv_votable_err:*/
	destroy_votable(wls_dev->nor_fcc_votable);
create_nor_fcc_votable_err:
	destroy_votable(wls_dev->nor_icl_votable);
create_nor_icl_votable_err:
	destroy_votable(wls_dev->wrx_en_votable);
create_wrx_en_votable_err:
	destroy_votable(wls_dev->fastchg_disable_votable);
create_disable_votable_err:
	destroy_votable(wls_dev->fcc_votable);
create_fcc_votable_err:
	disable_irq(wls_dev->usb_int_irq);
	if (!gpio_is_valid(wls_dev->usb_int_gpio))
		gpio_free(wls_dev->usb_int_gpio);
	if (!gpio_is_valid(wls_dev->ext_pwr_en_gpio))
		gpio_free(wls_dev->ext_pwr_en_gpio);
	if (!gpio_is_valid(wls_dev->wrx_en_gpio))
		gpio_free(wls_dev->wrx_en_gpio);
gpio_init_err:
	if (wls_dev->support_fastchg)
		oplus_chg_wls_fast_remove(wls_dev);
fast_init_err:
	oplus_chg_wls_nor_remove(wls_dev);
nor_init_err:
	oplus_chg_wls_rx_remove(wls_dev);
rx_init_err:
	destroy_workqueue(wls_dev->wls_wq);
alloc_work_err:
#ifdef MAYBE_DELETE
	oplus_chg_unreg_changed_notifier(&wls_dev->wls_changed_nb);
	oplus_chg_unreg_event_notifier(&wls_dev->wls_event_nb);
	oplus_chg_unreg_mod_notifier(wls_dev->wls_ocm, &wls_dev->wls_mod_nb);
	oplus_chg_mod_unregister(wls_dev->wls_ocm);
#endif
parse_dt_err:
	devm_kfree(&pdev->dev, wls_dev);
	return rc;
}

static int oplus_chg_wls_driver_remove(struct platform_device *pdev)
{
	struct oplus_chg_wls *wls_dev = platform_get_drvdata(pdev);

#if IS_ENABLED(CONFIG_OPLUS_DYNAMIC_CONFIG_CHARGER)
	oplus_wls_unreg_debug_config(wls_dev);
#endif
	remove_proc_entry("wireless", NULL);

	if (!IS_ERR_OR_NULL(wls_dev->comm_subs))
		oplus_mms_unsubscribe(wls_dev->comm_subs);
	if (!IS_ERR_OR_NULL(wls_dev->wired_subs))
		oplus_mms_unsubscribe(wls_dev->wired_subs);
	if (!IS_ERR_OR_NULL(wls_dev->gauge_subs))
		oplus_mms_unsubscribe(wls_dev->gauge_subs);
	if (!IS_ERR_OR_NULL(wls_dev->wls_subs))
		oplus_mms_unsubscribe(wls_dev->wls_subs);

	misc_deregister(&wls_dev->misc_dev);
	destroy_votable(wls_dev->rx_disable_votable);
	destroy_votable(wls_dev->nor_out_disable_votable);
	destroy_votable(wls_dev->nor_input_disable_votable);
	/*destroy_votable(wls_dev->nor_fv_votable);*/
	destroy_votable(wls_dev->nor_fcc_votable);
	destroy_votable(wls_dev->nor_icl_votable);
	destroy_votable(wls_dev->wrx_en_votable);
	destroy_votable(wls_dev->fastchg_disable_votable);
	destroy_votable(wls_dev->fcc_votable);
	disable_irq(wls_dev->usb_int_irq);
	if (!gpio_is_valid(wls_dev->usb_int_gpio))
		gpio_free(wls_dev->usb_int_gpio);
	if (!gpio_is_valid(wls_dev->ext_pwr_en_gpio))
		gpio_free(wls_dev->ext_pwr_en_gpio);
	if (!gpio_is_valid(wls_dev->wrx_en_gpio))
		gpio_free(wls_dev->wrx_en_gpio);
	if (wls_dev->support_fastchg)
		oplus_chg_wls_fast_remove(wls_dev);
	oplus_chg_wls_nor_remove(wls_dev);
	oplus_chg_wls_rx_remove(wls_dev);
	destroy_workqueue(wls_dev->wls_wq);
#ifdef MAYBE_DELETE
	oplus_chg_unreg_changed_notifier(&wls_dev->wls_changed_nb);
	oplus_chg_unreg_event_notifier(&wls_dev->wls_event_nb);
	oplus_chg_unreg_mod_notifier(wls_dev->wls_ocm, &wls_dev->wls_mod_nb);
	oplus_chg_mod_unregister(wls_dev->wls_ocm);
#endif
	devm_kfree(&pdev->dev, wls_dev);
	return 0;
}

static const struct of_device_id oplus_chg_wls_match[] = {
	{ .compatible = "oplus,chg_wls" },
	{},
};

static struct platform_driver oplus_chg_wls_driver = {
	.driver = {
		.name = "oplus-chg_wls",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_chg_wls_match),
	},
	.probe		= oplus_chg_wls_driver_probe,
	.remove		= oplus_chg_wls_driver_remove,
};

static __init int oplus_chg_wls_driver_init(void)
{
	return platform_driver_register(&oplus_chg_wls_driver);
}

static __exit void oplus_chg_wls_driver_exit(void)
{
	platform_driver_unregister(&oplus_chg_wls_driver);
}

oplus_chg_module_register(oplus_chg_wls_driver);
