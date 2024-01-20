#define pr_fmt(fmt) "[PPS]([%s][%d]): " fmt, __func__, __LINE__

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
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_vooc.h>
#include <oplus_chg_voter.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_cpa.h>
#include <oplus_impedance_check.h>
#include <oplus_chg_vooc.h>
#include <oplus_mms_wired.h>
#include <oplus_chg_pps.h>

#define PPS_MONITOR_CYCLE_MS		1500
#define PPS_WATCHDOG_TIME_MS		3000
#define PPS_START_DEF_CURR_MA		1500
#define PPS_START_DEF_VOL_MV		5000
#define PPS_MONITOR_TIME_MS		500
#define OPLUS_FIXED_PDO_CURR_MA		3000

#define PPS_UPDATE_PDO_TIME		5
#define PPS_UPDATE_FASTCHG_TIME	1
#define PPS_UPDATE_TEMP_TIME		1
#define PPS_UPDATE_IBAT_TIME		1

#define PPS_TEMP_OVER_COUNTS		2
#define PPS_TEMP_WARM_RANGE_THD	10
#define PPS_TEMP_LOW_RANGE_THD		20

#define PPS_R_AVG_NUM			10
#define PPS_R_ROW_NUM			7

#define FULL_PPS_SYS_MAX		6
#define PPS_ERROR_COUNT_MAX		3

#define PPS_BTB_OVER_TEMP		80
#define BTB_TEMP_OVER_MAX_INPUT_CUR	1000

enum {
	PPS_BAT_TEMP_NATURAL = 0,
	PPS_BAT_TEMP_HIGH0,
	PPS_BAT_TEMP_HIGH1,
	PPS_BAT_TEMP_HIGH2,
	PPS_BAT_TEMP_HIGH3,
	PPS_BAT_TEMP_HIGH4,
	PPS_BAT_TEMP_HIGH5,
	PPS_BAT_TEMP_LOW0,
	PPS_BAT_TEMP_LOW1,
	PPS_BAT_TEMP_LOW2,
	PPS_BAT_TEMP_LITTLE_COOL,
	PPS_BAT_TEMP_LITTLE_COOL_LOW,
	PPS_BAT_TEMP_COOL,
	PPS_BAT_TEMP_NORMAL_LOW,
	PPS_BAT_TEMP_NORMAL_HIGH,
	PPS_BAT_TEMP_LITTLE_COLD,
	PPS_BAT_TEMP_WARM,
	PPS_BAT_TEMP_EXIT,
	PPS_BAT_TEMP_SWITCH_CURVE,
};

enum {
	PPS_TEMP_RANGE_INIT = 0,
	PPS_TEMP_RANGE_LITTLE_COLD, /* 0 ~ 5 */
	PPS_TEMP_RANGE_COOL, /* 5 ~ 12 */
	PPS_TEMP_RANGE_LITTLE_COOL, /* 12~16 */
	PPS_TEMP_RANGE_NORMAL_LOW, /* 16~25 */
	PPS_TEMP_RANGE_NORMAL_HIGH, /* 25~43 */
	PPS_TEMP_RANGE_WARM, /* 43-52 */
	PPS_TEMP_RANGE_NORMAL,
};

enum {
	PPS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL,
	PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW,
	PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH,
	PPS_LOW_CURR_FULL_CURVE_TEMP_MAX,
};

struct oplus_pps_config {
	unsigned int target_vbus_mv;
	int curr_max_ma;
};

struct oplus_pps_timer {
	struct timespec pdo_timer;
	struct timespec fastchg_timer;
	struct timespec temp_timer;
	struct timespec ibat_timer;
	int pps_max_time_ms;
	unsigned long monitor_jiffies;
};

struct pps_protection_counts {
	int cool_fw;
	int sw_full;
	int hw_full;
	int low_curr_full;
	int ibat_low;
	int ibat_high;
	int ibat_over;
	int ibat_abnormal;
	int btb_high;
	int tbatt_over;
	int tfg_over;
	int output_low;
	int ibus_over;
	int pps_recovery;
};

struct oplus_pps_limits {
	int default_pps_normal_high_temp;
	int default_pps_little_cool_temp;
	int default_pps_cool_temp;
	int default_pps_little_cold_temp;
	int default_pps_normal_low_temp;
	int pps_warm_allow_vol;
	int pps_warm_allow_soc;

	int pps_batt_over_low_temp;
	int pps_little_cold_temp;
	int pps_cool_temp;
	int pps_little_cool_temp;
	int pps_normal_low_temp;
	int pps_normal_high_temp;
	int pps_batt_over_high_temp;
	int pps_strategy_temp_num;

	int pps_strategy_soc_over_low;
	int pps_strategy_soc_min;
	int pps_strategy_soc_low;
	int pps_strategy_soc_mid_low;
	int pps_strategy_soc_mid;
	int pps_strategy_soc_mid_high;
	int pps_strategy_soc_high;
	int pps_strategy_soc_num;

	int pps_strategy_normal_current;
	int pps_strategy_batt_high_temp0;
	int pps_strategy_batt_high_temp1;
	int pps_strategy_batt_high_temp2;
	int pps_strategy_batt_low_temp2;
	int pps_strategy_batt_low_temp1;
	int pps_strategy_batt_low_temp0;
	int pps_strategy_high_current0;
	int pps_strategy_high_current1;
	int pps_strategy_high_current2;
	int pps_strategy_low_current2;
	int pps_strategy_low_current1;
	int pps_strategy_low_current0;

	int pps_low_curr_full_cool_temp;
	int pps_low_curr_full_little_cool_temp;
	int pps_low_curr_full_normal_low_temp;
	int pps_low_curr_full_normal_high_temp;

	int pps_over_high_or_low_current;
	int pps_strategy_change_count;
	int pps_full_cool_sw_vbat;
	int pps_full_normal_sw_vbat;
	int pps_full_normal_hw_vbat;
	int pps_full_warm_vbat;
	int pps_full_ffc_vbat;
	int pps_full_cool_sw_vbat_third;
	int pps_full_normal_sw_vbat_third;
	int pps_full_normal_hw_vbat_third;
	int pps_timeout_third;
	int pps_timeout_oplus;
	int pps_ibat_over_third;
	int pps_ibat_over_oplus;
};

struct pps_full_curve {
	unsigned int iterm;
	unsigned int vterm;
	bool exit;
};

struct pps_full_curves_temp {
	struct pps_full_curve full_curves[FULL_PPS_SYS_MAX];
	int full_curve_num;
};

#define PPS_CMD_BUF_SIZE		128
#define PPS_GET_AUTH_DATA_TIMEOUT_MS	1000
enum pps_dev_cmd_type {
	PPS_DEV_CMD_GET_AUTH_DATA,
};

struct pps_dev_cmd {
	unsigned int cmd;
	unsigned int data_size;
	unsigned char data_buf[PPS_CMD_BUF_SIZE];
};

enum pps_vbus_check_status {
	PPS_VBUS_CHECK_INVALID = 0,
	PPS_VBUS_NEED_CHECK_UP,
	PPS_VBUS_NEED_CHECK_DOWN,
	PPS_VBUS_CHECK_DONE,
};

struct oplus_pps {
	struct device *dev;
	struct oplus_mms *pps_topic;
	struct oplus_mms *cpa_topic;
	struct mms_subscribe *cpa_subs;
	struct oplus_mms *wired_topic;
	struct mms_subscribe *wired_subs;
	struct oplus_mms *comm_topic;
	struct mms_subscribe *comm_subs;
	struct oplus_mms *gauge_topic;
	struct mms_subscribe *gauge_subs;
	struct oplus_chg_ic_dev *pps_ic;
	struct oplus_chg_ic_dev *cp_ic;
	struct oplus_chg_ic_dev *dpdm_switch;

	struct notifier_block nb;

	struct votable *pps_curr_votable;
	struct votable *pps_disable_votable;
	struct votable *pps_not_allow_votable;
	struct votable *wired_suspend_votable;
	struct votable *pps_boot_votable;
	struct votable *wired_icl_votable;

	struct delayed_work switch_check_work;
	struct delayed_work monitor_work;
	struct delayed_work current_work;
	struct delayed_work imp_uint_init_work;

	struct work_struct wired_online_work;
	struct work_struct force_exit_work;
	struct work_struct soft_exit_work;
	struct work_struct gauge_update_work;

	wait_queue_head_t read_wq;
	struct miscdevice misc_dev;
	struct mutex read_lock;
	struct mutex cmd_data_lock;
	struct completion cmd_ack;
	struct pps_dev_cmd cmd;
	bool cmd_data_ok;

	struct oplus_chg_strategy *oplus_curve_strategy;
	struct oplus_chg_strategy *third_curve_strategy;
	struct oplus_chg_strategy *strategy;

	struct oplus_impedance_node *input_imp_node;
	struct oplus_impedance_unit *imp_uint;

	struct oplus_pps_config config;
	struct oplus_pps_timer timer;
	struct pps_protection_counts count;
	struct oplus_pps_limits limits;

	struct pps_full_curves_temp low_curr_full_curves_temp[PPS_LOW_CURR_FULL_CURVE_TEMP_MAX];

	u64 dev_info;
	u32 pps_status_info;
	pps_msg_data pdo[PPS_PDO_MAX];
	int pdo_num;
	bool pps_online;
	bool pps_charging;
	bool oplus_pps_adapter;
	bool pps_disable;
	bool pps_not_allow;
	enum oplus_cp_work_mode cp_work_mode;
	int target_curr_ma;
	int target_vbus_mv;
	int curr_set_ma;
	int vol_set_mv;
	int bcc_max_curr;
	int bcc_min_curr;
	int bcc_exit_curr;
	int pps_current_change_timeout;
	int rmos_mohm;
	int cool_down;
	u32 adapter_id;
	int error_count;
	int cp_ratio;

	bool wired_online;
	bool led_on;
	bool need_check_current;
	bool mos_on_check;

	int ui_soc;
	int shell_temp;
	enum oplus_temp_region batt_temp_region;
	bool shell_temp_ready;

	int pps_fastchg_batt_temp_status;
	int pps_temp_cur_range;
	int pps_low_curr_full_temp_status;
	bool quit_pps_protocol;
};

struct current_level {
	int level;
	int curr_ma;
};

static const struct current_level g_pps_current_table[] = {
	{ 1, 1000 },   { 2, 1000 },   { 3, 1200 },   { 4, 1500 },   { 5, 1700 },  { 6, 2000 },	 { 7, 2200 },
	{ 8, 2500 },   { 9, 2700 },   { 10, 3000 },  { 11, 3200 },  { 12, 3500 }, { 13, 3700 },	 { 14, 4000 },
	{ 15, 4500 },  { 16, 5000 },  { 17, 5500 },  { 18, 6000 },  { 19, 6300 }, { 20, 6500 },	 { 21, 7000 },
	{ 22, 7500 },  { 23, 8000 },  { 24, 8500 },  { 25, 9000 },  { 26, 9500 }, { 27, 10000 }, { 28, 10500 },
	{ 29, 11000 }, { 30, 11500 }, { 31, 12000 }, { 32, 12500 },
};

static const int pps_cool_down_oplus_curve[] = {
	15000, 1000,  1000,  1200,  1500,  1700,  2000,	 2200,	2500,  2700,
	3000,  3200,  3500,  3700,  4000,  4500,  5000,	 5500,	6000,  6300,
	6500,  7000,  7500,  8000,  8500,  9000,  9500,	 10000, 10500, 11000,
	11500, 12000, 12500, 13000, 13500, 14000, 14500, 15000
};

__maybe_unused static bool
is_wired_suspend_votable_available(struct oplus_pps *chip)
{
	if (!chip->wired_suspend_votable)
		chip->wired_suspend_votable = find_votable("WIRED_CHARGE_SUSPEND");
	return !!chip->wired_suspend_votable;
}

__maybe_unused static bool
is_wired_icl_votable_available(struct oplus_pps *chip)
{
	if (!chip->wired_icl_votable)
		chip->wired_icl_votable = find_votable("WIRED_ICL");
	return !!chip->wired_icl_votable;
}

__maybe_unused static bool
is_gauge_topic_available(struct oplus_pps *chip)
{
	if (!chip->gauge_topic)
		chip->gauge_topic = oplus_mms_get_by_name("gauge");
	return !!chip->gauge_topic;
}

__maybe_unused
static int pps_find_current_to_level(int val, const struct current_level * const table, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (table[i].level == val)
			return table[i].curr_ma;
	}
	return 0;
}

static int pps_find_level_to_current(int val, const struct current_level * const table, int len)
{
	int i;
	bool find_out_flag = false;

	for (i = 0; i < len; i++) {
		if (table[i].curr_ma > val) {
			find_out_flag = true;
			break;
		}
		find_out_flag = false;
	}
	if (find_out_flag == true && i != 0)
		return table[i - 1].level;
	return 0;
}

static enum oplus_cp_work_mode vbus_to_cp_work_mode(int vbus_mv)
{
	enum oplus_cp_work_mode mode;
	int batt_num;

#define BATT_VOL_MAX_MV	5000

	batt_num = oplus_gauge_get_batt_num();

	if (vbus_mv >= batt_num * BATT_VOL_MAX_MV) {
		switch (vbus_mv / (batt_num * BATT_VOL_MAX_MV)) {
		case 1:
			mode = CP_WORK_MODE_BYPASS;
			break;
		case 2:
			mode = CP_WORK_MODE_2_TO_1;
			break;
		case 3:
			mode = CP_WORK_MODE_3_TO_1;
			break;
		case 4:
			mode = CP_WORK_MODE_4_TO_1;
			break;
		default:
			mode = CP_WORK_MODE_UNKNOWN;
			break;
		}
	} else {
		/* TODO */
		mode = CP_WORK_MODE_UNKNOWN;
	}

	return mode;
}

__maybe_unused
static int32_t oplus_pps_get_curve_vbus(struct oplus_pps *chip)
{
	struct puc_strategy_ret_data data;
	int rc;

	if (chip->strategy == NULL)
		return -EINVAL;

	rc = oplus_chg_strategy_get_data(chip->strategy, &data);
	if (rc < 0) {
		chg_err("can't get curve vbus, rc=%d\n", rc);
		return rc;
	}

	return data.target_vbus;
}

__maybe_unused
static int32_t oplus_pps_get_curve_ibus(struct oplus_pps *chip)
{
	struct puc_strategy_ret_data data;
	int rc;

	if (chip->strategy == NULL)
		return -EINVAL;

	rc = oplus_chg_strategy_get_data(chip->strategy, &data);
	if (rc < 0) {
		chg_err("can't get curve ibus, rc=%d\n", rc);
		return rc;
	}

	return data.target_ibus;
}

__maybe_unused
static int oplus_pps_pdo_set(struct oplus_pps *chip, int vol_mv, int curr_ma)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_PDO_SET, vol_mv, curr_ma);
	if (rc == 0) {
		chip->curr_set_ma = curr_ma;
		chip->vol_set_mv = vol_mv;
	}

	return rc;
}

__maybe_unused
static int oplus_pps_hard_reset(struct oplus_pps *chip)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_HARD_RESET);

	return rc;
}

__maybe_unused
static int oplus_pps_exit_pps_mode(struct oplus_pps *chip)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_EXIT);
	if (rc < 0)
		chg_err("exit pps mode error, rc=%d", rc);

	return rc;
}

__maybe_unused
static int oplus_pps_get_err_info(struct oplus_pps *chip, u64 *err_info)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_GET_ERR_INFO, err_info);

	return rc;
}

__maybe_unused
static int oplus_pps_get_pps_status_info(struct oplus_pps *chip, u32 *pps_status_info)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_GET_PPS_STATUS, pps_status_info);
	if (rc < 0)
		chip->error_count++;
	else
		chip->error_count = 0;

	return rc;
}

__maybe_unused
static int oplus_pps_get_pdo_info(struct oplus_pps *chip, u32 *pdo, int num)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_GET_PDO_INFO, pdo, num);

	return rc;
}

__maybe_unused
static int oplus_pps_verify_adapter(struct oplus_pps *chip)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_VERIFY_ADAPTER);

	return rc;
}

__maybe_unused
static int oplus_pps_get_power_change_info(
	struct oplus_pps *chip, u32 *pwr_change_info, int num)
{
	int rc;

	if (chip->pps_ic == NULL) {
		chg_err("pps_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_PPS_GET_POWER_CHANGE_INFO,
		pwr_change_info, num);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_enable(struct oplus_pps *chip, bool en)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_ENABLE, en);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_hw_init(struct oplus_pps *chip)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_HW_INTI);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_set_work_mode(struct oplus_pps *chip, enum oplus_cp_work_mode mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_WORK_MODE, mode);

	if (rc >= 0) {
		switch (mode) {
		case CP_WORK_MODE_4_TO_1:
			chip->cp_ratio = 4;
			break;
		case CP_WORK_MODE_3_TO_1:
			chip->cp_ratio = 3;
			break;
		case CP_WORK_MODE_2_TO_1:
			chip->cp_ratio = 2;
			break;
		case CP_WORK_MODE_BYPASS:
			chip->cp_ratio = 1;
			break;
		default:
			chg_err("unsupported cp work mode, mode=%d\n", mode);
			rc = -ENOTSUPP;
			break;
		}
	}

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_work_mode(struct oplus_pps *chip, enum oplus_cp_work_mode *mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_WORK_MODE, mode);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_check_work_mode_support(struct oplus_pps *chip, enum oplus_cp_work_mode mode)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT, mode);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_set_iin(struct oplus_pps *chip, int iin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_IIN, iin);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_vin(struct oplus_pps *chip, int *vin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VIN, vin);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_iin(struct oplus_pps *chip, int *iin)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IIN, iin);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_vout(struct oplus_pps *chip, int *vout)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VOUT, vout);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_iout(struct oplus_pps *chip, int *iout)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IOUT, iout);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_vac(struct oplus_pps *chip, int *vac)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VAC, vac);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_set_work_start(struct oplus_pps *chip, bool start)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_WORK_START, start);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_get_work_status(struct oplus_pps *chip, bool *start)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_WORK_STATUS, start);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_adc_enable(struct oplus_pps *chip, bool en)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_SET_ADC_ENABLE, en);

	return rc;
}

__maybe_unused
static int oplus_pps_cp_reg_dump(struct oplus_pps *chip)
{
	int rc;

	if (chip->cp_ic == NULL) {
		chg_err("cp_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_REG_DUMP);

	return rc;
}

static int oplus_pps_switch_to_normal(struct oplus_pps *chip)
{
	int rc;

	if (chip->dpdm_switch == NULL) {
		chg_err("dpdm_switch ic is NULL\n");
		return -ENODEV;
	}

	/* switch to 5v when switch to normal */
	if (chip->wired_online)
		rc = oplus_chg_ic_func(chip->pps_ic, OPLUS_IC_FUNC_FIXED_PDO_SET,
				PPS_START_DEF_VOL_MV, OPLUS_FIXED_PDO_CURR_MA);

	rc = oplus_chg_ic_func(chip->dpdm_switch,
		OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE, DPDM_SWITCH_TO_AP);

	return rc;
}
static int oplus_pps_set_online(struct oplus_pps *chip, bool online)
{
	struct mms_msg *msg;
	int rc;

	if (chip->pps_online == online)
		return 0;

	chip->pps_online = online;
	chg_info("set pps_online=%s\n", online ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  PPS_ITEM_ONLINE);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->pps_topic, msg);
	if (rc < 0) {
		chg_err("publish pps online msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_pps_set_charging(struct oplus_pps *chip, bool charging)
{
	struct mms_msg *msg;
	int rc;

	if (chip->pps_charging == charging)
		return 0;

	chip->pps_charging = charging;
	chg_info("set pps_charging=%s\n", charging ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  PPS_ITEM_CHARGING);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->pps_topic, msg);
	if (rc < 0) {
		chg_err("publish pps charging msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_pps_set_adapter_id(struct oplus_pps *chip, u16 id)
{
	struct mms_msg *msg;
	int rc;

	if (chip->adapter_id == id)
		return 0;

	chip->adapter_id = id;
	chg_info("set adapter_id=%u\n", id);

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  PPS_ITEM_ADAPTER_ID);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->pps_topic, msg);
	if (rc < 0) {
		chg_err("publish pps adapter id msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_pps_set_oplus_adapter(struct oplus_pps *chip, bool oplus_adapter)
{
	struct mms_msg *msg;
	int rc;

	if (chip->oplus_pps_adapter == oplus_adapter)
		return 0;

	chip->oplus_pps_adapter = oplus_adapter;
	chg_info("set oplus_pps_adapter=%s\n", oplus_adapter ? "true" : "false");

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_MEDIUM,
				  PPS_ITEM_OPLUS_ADAPTER);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->pps_topic, msg);
	if (rc < 0) {
		chg_err("publish oplus adapter msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static void oplus_pps_charge_btb_allow_check(struct oplus_pps *chip)
{
#define PPS_BTB_CHECK_MAX_CNT 3
#define PPS_BTB_CHECK_TIME_US 10000

	int btb_check_cnt = PPS_BTB_CHECK_MAX_CNT;
	int batt_btb_temp;
	int usb_btb_temp;

	while (btb_check_cnt != 0) {
		batt_btb_temp = oplus_wired_get_batt_btb_temp();
		usb_btb_temp = oplus_wired_get_usb_btb_temp();
		chg_info("batt_btb_temp: %d, usb_btb_temp = %d", batt_btb_temp, usb_btb_temp);

		if (batt_btb_temp < PPS_BTB_OVER_TEMP && usb_btb_temp < PPS_BTB_OVER_TEMP)
			break;

		btb_check_cnt--;
		if (btb_check_cnt > 0)
			usleep_range(PPS_BTB_CHECK_TIME_US, PPS_BTB_CHECK_TIME_US);
	}
	if (btb_check_cnt == 0) {
		vote(chip->pps_not_allow_votable, BTB_TEMP_OVER_VOTER, true, 1, false);
		if (is_wired_icl_votable_available(chip))
			vote(chip->wired_icl_votable, BTB_TEMP_OVER_VOTER, true,
			     BTB_TEMP_OVER_MAX_INPUT_CUR, true);
	}
}

static bool oplus_pps_charge_allow_check(struct oplus_pps *chip)
{
	union mms_msg_data data = { 0 };
	int vbat_mv = 0;
	int chg_temp = 0;
	int rc = 0;

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (unlikely(rc < 0)) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vbat_mv = 3800;
	} else {
		vbat_mv = data.intval;
	}

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, true);
	if (unlikely(rc < 0)) {
		chg_err("can't get comm_item_shell_temp, rc=%d\n", rc);
		rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_TEMP, &data, false);
		if (unlikely(rc < 0)) {
			chg_err("can't get gauge_item_temp, rc=%d\n", rc);
			chg_temp = 250;
		} else {
			chg_temp = data.intval;
		}
	} else {
		chg_temp = data.intval;
	}

	if (chg_temp < chip->limits.pps_batt_over_low_temp ||
	    chg_temp > chip->limits.pps_batt_over_high_temp) {
		vote(chip->pps_not_allow_votable, BATT_TEMP_VOTER, true, 1, false);
	} else if (chg_temp > chip->limits.pps_normal_high_temp) {
		if (chip->ui_soc > chip->limits.pps_warm_allow_soc ||
		    vbat_mv > chip->limits.pps_warm_allow_vol)
			vote(chip->pps_not_allow_votable, BATT_TEMP_VOTER, true, 1, false);
		else
			vote(chip->pps_not_allow_votable, BATT_TEMP_VOTER, false, 0, false);
	} else {
		vote(chip->pps_not_allow_votable, BATT_TEMP_VOTER, false, 0, false);
	}

	if (chip->ui_soc < chip->limits.pps_strategy_soc_over_low ||
	    chip->ui_soc > chip->limits.pps_strategy_soc_high)
		vote(chip->pps_not_allow_votable, BATT_SOC_VOTER, true, 1, false);
	else
		vote(chip->pps_not_allow_votable, BATT_SOC_VOTER, false, 0, false);

	oplus_pps_charge_btb_allow_check(chip);

	return !chip->pps_not_allow;
}

static void oplus_pps_count_init(struct oplus_pps *chip)
{
	chip->count.cool_fw = 0;
	chip->count.sw_full = 0;
	chip->count.hw_full = 0;
	chip->count.low_curr_full = 0;
	chip->count.ibat_low = 0;
	chip->count.ibat_high = 0;
	chip->count.ibat_over = 0;
	chip->count.ibat_abnormal = 0;
	chip->count.btb_high = 0;
	chip->count.tbatt_over = 0;
	chip->count.tfg_over = 0;
	chip->count.output_low = 0;
	chip->count.ibus_over = 0;
	chip->count.pps_recovery = 0;
}

static void oplus_pps_votable_reset(struct oplus_pps *chip)
{
	vote(chip->pps_disable_votable, NO_DATA_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, CHG_FULL_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, IBAT_OVER_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, BTB_TEMP_OVER_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, BATT_TEMP_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, TFG_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, IMP_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, TIMEOUT_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, CONNECT_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, PPS_IBAT_ABNOR_VOTER, false, 0, false);
	vote(chip->pps_disable_votable, SWITCH_RANGE_VOTER, false, 0, false);
	vote(chip->pps_not_allow_votable, BTB_TEMP_OVER_VOTER, false, 0, false);

	vote(chip->pps_curr_votable, IMP_VOTER, false, 0, false);
	vote(chip->pps_curr_votable, STEP_VOTER, false, 0, false);
	vote(chip->pps_curr_votable, BATT_TEMP_VOTER, false, 0, false);
	vote(chip->pps_curr_votable, COOL_DOWN_VOTER, false, 0, false);
}

static int oplus_pps_temp_cur_range_init(struct oplus_pps *chip)
{
	int vbat_temp_cur;

	vbat_temp_cur = chip->shell_temp;
	if (vbat_temp_cur < chip->limits.pps_little_cold_temp) { /*0-5C*/
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COLD;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_LITTLE_COLD;
	} else if (vbat_temp_cur < chip->limits.pps_cool_temp) { /*5-12C*/
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_COOL;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_COOL;
	} else if (vbat_temp_cur < chip->limits.pps_little_cool_temp) { /*12-20C*/
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COOL;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_LITTLE_COOL;
	} else if (vbat_temp_cur < chip->limits.pps_normal_low_temp) { /*20-35C*/
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_LOW;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NORMAL_LOW;
	} else if (vbat_temp_cur < chip->limits.pps_normal_high_temp) { /*35C-43C*/
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_HIGH;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NORMAL_HIGH;
	} else {
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_WARM;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_WARM;
	}

	return 0;
}
static void oplus_pps_variables_init(struct oplus_pps *chip)
{
	oplus_pps_count_init(chip);

	chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NATURAL;
	chip->pps_temp_cur_range = PPS_TEMP_RANGE_INIT;
	chip->error_count = 0;
	chip->need_check_current = false;
	chip->quit_pps_protocol = false;
	chip->mos_on_check = false;

	chip->timer.fastchg_timer = oplus_current_kernel_time();
	chip->timer.temp_timer = oplus_current_kernel_time();
	chip->timer.pdo_timer = oplus_current_kernel_time();
	chip->timer.ibat_timer = oplus_current_kernel_time();
	if (!chip->oplus_pps_adapter)
		chip->timer.pps_max_time_ms = chip->limits.pps_timeout_third * 1000;
	else
		chip->timer.pps_max_time_ms = chip->limits.pps_timeout_oplus * 1000;

	chip->pps_low_curr_full_temp_status =
		PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW;

	(void)oplus_imp_uint_reset(chip->imp_uint);
}

static void oplus_pps_force_exit(struct oplus_pps *chip)
{
	oplus_pps_set_charging(chip, false);
	oplus_pps_set_oplus_adapter(chip, false);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
	chip->cp_ratio = 0;
	oplus_pps_exit_pps_mode(chip);
	oplus_pps_cp_set_work_start(chip, false);
	oplus_pps_cp_enable(chip, false);
	oplus_pps_cp_adc_enable(chip, false);
	oplus_pps_switch_to_normal(chip);
	oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_PPS);
	oplus_pps_set_adapter_id(chip, 0);
	oplus_pps_set_online(chip, false);
	vote(chip->pps_curr_votable, STEP_VOTER, false, 0, false);
	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, PPS_VOTER, false, 0, false);
}

static void oplus_pps_soft_exit(struct oplus_pps *chip)
{
	oplus_pps_set_charging(chip, false);
	oplus_pps_set_oplus_adapter(chip, false);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
	chip->cp_ratio = 0;
	oplus_pps_exit_pps_mode(chip);
	oplus_pps_cp_set_work_start(chip, false);
	oplus_pps_cp_enable(chip, false);
	oplus_pps_cp_adc_enable(chip, false);
	oplus_pps_switch_to_normal(chip);
	vote(chip->pps_curr_votable, STEP_VOTER, false, 0, false);
	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, PPS_VOTER, false, 0, false);
}

static void oplus_pps_switch_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_pps *chip =
		container_of(dwork, struct oplus_pps, switch_check_work);
	enum oplus_cp_work_mode cp_mode;
	union mms_msg_data data = { 0 };
	int rc;
	int i;
	bool pdo_ok = false;
	int max_curr = 0;
	int target_vbus_max = 0;
	int wired_type;

	oplus_cpa_switch_start(chip->cpa_topic, CHG_PROTOCOL_PPS);

	rc = oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_REAL_CHG_TYPE,
				&data, false);
	if (rc < 0)
		wired_type = OPLUS_CHG_USB_TYPE_UNKNOWN;
	else
		wired_type = data.intval;

	if (wired_type != OPLUS_CHG_USB_TYPE_PD_PPS) {
		chg_err("wired_type=%d, Not PPS adapter", wired_type);
		oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_PPS);
		return;
	}

	oplus_pps_switch_to_normal(chip);
	oplus_pps_set_charging(chip, false);
	oplus_pps_votable_reset(chip);
	if (chip->pps_disable) {
		oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_PPS);
		return;
	}
	if (chip->pps_ic == NULL || chip->cp_ic == NULL || !is_gauge_topic_available(chip)) {
		oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_PPS);
		return;
	}

	oplus_pps_set_online(chip, true);

	rc = oplus_pps_verify_adapter(chip);
	if (rc <= 0) {
		/* rc=0 if PPS_GET_AUTHENTICATE failed */
		chg_err("adapter verify fail, rc=%d\n", rc);
		chip->oplus_pps_adapter = false;
	} else {
		chg_info("adapter verify pass\n");
		chip->oplus_pps_adapter = true;
	}
	chg_info("oplus_pps_adapter=%s\n", chip->oplus_pps_adapter ? "true" : "false");

	rc = oplus_pps_get_pdo_info(chip, (u32*)chip->pdo, PPS_PDO_MAX);
	if (rc < 0) {
		chg_err("pps get pdo info error\n");
		goto err;
	}
	chip->pdo_num = PPS_PDO_MAX;

	for (i = 0; i < chip->pdo_num; i++) {
		target_vbus_max = chip->config.target_vbus_mv;

		if (chip->pdo[i].pdo_type != USBPD_PDMSG_PDOTYPE_AUGMENTED)
			continue;

		chg_err("pdo[%d]: voltage[%u-%umV,  max current[%umA]\n", i,
			 PPS_PDO_VOL_MIN(chip->pdo[i].min_voltage100mv),
			 PPS_PDO_VOL_MAX(chip->pdo[i].max_voltage100mv),
			 PPS_PDO_CURR_MAX(chip->pdo[i].max_current50ma));

		if (PPS_PDO_VOL_MIN(chip->pdo[i].min_voltage100mv) > PPS_START_DEF_VOL_MV) {
			chg_err("The output voltage does not support 5V\n");
			break;
		}

		if (target_vbus_max <= PPS_PDO_VOL_MAX(chip->pdo[i].max_voltage100mv) &&
		    target_vbus_max >= PPS_PDO_VOL_MIN(chip->pdo[i].min_voltage100mv)) {
			pdo_ok = true;
			if (PPS_PDO_CURR_MAX(chip->pdo[i].max_current50ma) > max_curr)
				max_curr = PPS_PDO_CURR_MAX(chip->pdo[i].max_current50ma);
		}
	}
	if (!pdo_ok) {
		chg_err("The pdo range of the current adapter does not support PPS charging\n");
		goto err;
	}
	vote(chip->pps_curr_votable, BASE_MAX_VOTER, true, max_curr, false);
	oplus_pps_variables_init(chip);
	if (!oplus_pps_charge_allow_check(chip)) {
		chg_info("pps charge not allow, exit pps mode\n");
		goto exit;
	}

	cp_mode = vbus_to_cp_work_mode(chip->config.target_vbus_mv);
	if (cp_mode == CP_WORK_MODE_UNKNOWN) {
		chg_err("can't find a suitable CP working mode\n");
		goto err;
	}
	rc = oplus_pps_cp_check_work_mode_support(chip, cp_mode);
	if (rc <= 0) {
		chg_err("cp not support %d mode, rc=%d\n", cp_mode, rc);
		goto err;
	}
	rc = oplus_pps_cp_set_work_mode(chip, cp_mode);
	if (rc < 0) {
		chg_err("cp set %d mode error, rc=%d\n", cp_mode, rc);
		goto err;
	}
	chip->cp_work_mode = cp_mode;

	oplus_pps_cp_adc_enable(chip, true);

	if (is_wired_suspend_votable_available(chip))
		vote(chip->wired_suspend_votable, PPS_VOTER, true, 1, false);

	rc = oplus_pps_pdo_set(chip, PPS_START_DEF_VOL_MV, PPS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("pdo set error, rc=%d\n", rc);
		goto err;
	}

	schedule_delayed_work(&chip->monitor_work, 0);

	return;
err:
	chg_err("error, pps exit\n");
	oplus_pps_force_exit(chip);
	return;

exit:
	oplus_pps_exit_pps_mode(chip);
	chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
	chip->cp_ratio = 0;
}

enum {
	OPLUS_PPS_VOLT_UPDATE_100MV = 100,
	OPLUS_PPS_VOLT_UPDATE_200MV = 200,
	OPLUS_PPS_VOLT_UPDATE_500MV = 500,
	OPLUS_PPS_VOLT_UPDATE_1V = 1000,
	OPLUS_PPS_VOLT_UPDATE_2V = 2000,
	OPLUS_PPS_VOLT_UPDATE_5V = 5000,
};

static int oplus_pps_charge_start(struct oplus_pps *chip)
{
	union mms_msg_data data = { 0 };
	int vbat_mv;
	int rc;
	int target_vbus, update_size, req_vol;
	int cp_vin, delta_vbus;
	static int retry_count = 0;
	int batt_num;

#define PPS_START_VOL_THR_4_TO_1_600MV	600
#define PPS_START_VOL_THR_3_TO_1_400MV	400
#define PPS_START_VOL_THR_2_TO_1_200MV	200
#define PPS_START_VOL_THR_1_TO_1_300MV	300
#define PPS_START_RETAY_MAX		3
#define PPS_START_CHECK_DELAY_MS	500
#define PPS_START_PDO_DELAY_MS		500

	batt_num = oplus_gauge_get_batt_num();
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, true);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}
	vbat_mv = data.intval * batt_num;

	switch (chip->cp_work_mode) {
	case CP_WORK_MODE_4_TO_1:
		delta_vbus = PPS_START_VOL_THR_4_TO_1_600MV;
		break;
	case CP_WORK_MODE_3_TO_1:
		delta_vbus = PPS_START_VOL_THR_3_TO_1_400MV;
		break;
	case CP_WORK_MODE_2_TO_1:
		delta_vbus = PPS_START_VOL_THR_2_TO_1_200MV;
		break;
	case CP_WORK_MODE_BYPASS:
		delta_vbus = PPS_START_VOL_THR_1_TO_1_300MV;
		break;
	default:
		chg_err("unsupported cp work mode, mode=%d\n", chip->cp_work_mode);
		return -ENOTSUPP;
	}
	target_vbus = ((vbat_mv * chip->cp_ratio) / 100) * 100 + delta_vbus;

	rc = oplus_pps_cp_get_vin(chip, &cp_vin);
	if (rc < 0) {
		chg_err("can't get cp input voltage, rc=%d\n", rc);
		return rc;
	}
	chg_err("cp_work_mode=%d, vbat=%d, cp_vin=%d\n", chip->cp_work_mode, vbat_mv, cp_vin);

	if ((cp_vin >= target_vbus && cp_vin <= (target_vbus + OPLUS_PPS_VOLT_UPDATE_200MV)) ||
	    chip->mos_on_check) {
		if (chip->mos_on_check) {
			bool work_start;

			rc = oplus_pps_cp_get_work_status(chip, &work_start);
			if (rc < 0) {
				chg_err("can't get cp work status, rc=%d\n", rc);
			} else {
				if (work_start) {
					if (chip->oplus_pps_adapter)
						chip->strategy = chip->oplus_curve_strategy;
					else
						chip->strategy = chip->third_curve_strategy;
					rc = oplus_chg_strategy_init(chip->strategy);
					if (rc < 0) {
						chg_err("strategy_init error, not support pps fast charge\n");
						return rc;
					}

					rc = oplus_pps_temp_cur_range_init(chip);
					if (rc < 0)
						return rc;

					retry_count = 0;
					chip->mos_on_check = false;
					oplus_pps_set_charging(chip, true);
					if (chip->oplus_pps_adapter)
						chip->target_vbus_mv = chip->config.target_vbus_mv;
					else
						chip->target_vbus_mv = chip->vol_set_mv;
					chip->timer.monitor_jiffies = jiffies;
					schedule_delayed_work(&chip->current_work, 0);
					oplus_pps_cp_reg_dump(chip);
					return 0;
				}
				chg_err("cp not work, retry=%d\n", retry_count);
			}
			if (retry_count >= PPS_START_RETAY_MAX) {
				retry_count = 0;
				chip->mos_on_check = false;
				oplus_pps_cp_set_work_start(chip, false);
				oplus_pps_cp_enable(chip, false);
				oplus_pps_cp_adc_enable(chip, false);
				oplus_pps_cp_reg_dump(chip);
				chg_err("cp not working\n");
				return -EFAULT;
			}
			retry_count++;
			return PPS_START_CHECK_DELAY_MS;
		}
		rc = oplus_pps_cp_enable(chip, true);
		if (rc < 0) {
			chg_err("set cp enable error, rc=%d\n", rc);
			return rc;
		}
		rc = oplus_pps_cp_set_work_start(chip, true);
		if (rc < 0) {
			chg_err("set cp work start error, rc=%d\n", rc);
			return rc;
		}
		retry_count = 0;
		chip->mos_on_check = true;

		return PPS_START_CHECK_DELAY_MS;
	}

	retry_count = 0;
	chip->mos_on_check = false;

	if (abs(cp_vin - target_vbus) >= OPLUS_PPS_VOLT_UPDATE_2V)
		update_size = OPLUS_PPS_VOLT_UPDATE_2V;
	else if (abs(cp_vin - target_vbus) >= OPLUS_PPS_VOLT_UPDATE_1V)
		update_size = OPLUS_PPS_VOLT_UPDATE_1V;
	else if (abs(cp_vin - target_vbus) >= OPLUS_PPS_VOLT_UPDATE_500MV)
		update_size = OPLUS_PPS_VOLT_UPDATE_500MV;
	else if (abs(cp_vin - target_vbus) >= OPLUS_PPS_VOLT_UPDATE_200MV)
		update_size = OPLUS_PPS_VOLT_UPDATE_200MV;
	else
		update_size = OPLUS_PPS_VOLT_UPDATE_100MV;

	req_vol = chip->vol_set_mv;
	if (cp_vin < target_vbus)
		req_vol += update_size;
	else
		req_vol -= update_size;
	chg_info("cp_vin=%d, target_vbus=%d, update_size=%d, req_vol=%d\n",
		 cp_vin, target_vbus, update_size, req_vol);

	rc = oplus_pps_cp_set_iin(chip, PPS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("set cp input current error, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_pps_pdo_set(chip, req_vol, PPS_START_DEF_CURR_MA);
	if (rc < 0) {
		chg_err("pdo set error, rc=%d\n", rc);
		return rc;
	}

	return PPS_START_PDO_DELAY_MS;
}

static void oplus_pps_reset_temp_range(struct oplus_pps *chip)
{
	chip->limits.pps_normal_high_temp =
		chip->limits.default_pps_normal_high_temp;
	chip->limits.pps_little_cold_temp =
		chip->limits.default_pps_little_cold_temp;
	chip->limits.pps_cool_temp = chip->limits.default_pps_cool_temp;
	chip->limits.pps_little_cool_temp =
		chip->limits.default_pps_little_cool_temp;
	chip->limits.pps_normal_low_temp =
		chip->limits.default_pps_normal_low_temp;
}

static int oplus_pps_set_current_warm_range(struct oplus_pps *chip,
					     int vbat_temp_cur)
{
	int ret = chip->limits.pps_strategy_normal_current;

	if (chip->limits.pps_batt_over_high_temp != -EINVAL &&
	    vbat_temp_cur > chip->limits.pps_batt_over_high_temp) {
		chip->limits.pps_strategy_change_count++;
		if (chip->limits.pps_strategy_change_count >=
		    PPS_TEMP_OVER_COUNTS) {
			chip->limits.pps_strategy_change_count = 0;
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_EXIT;
			ret = chip->limits.pps_over_high_or_low_current;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur < chip->limits.pps_normal_high_temp) {
		chip->limits.pps_strategy_change_count++;
		if (chip->limits.pps_strategy_change_count >=
		    PPS_TEMP_OVER_COUNTS) {
			chip->limits.pps_strategy_change_count = 0;
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_SWITCH_CURVE;
			chip->pps_temp_cur_range = PPS_TEMP_RANGE_INIT;
			ret = chip->limits.pps_strategy_normal_current;
			oplus_pps_reset_temp_range(chip);
			(void)oplus_chg_strategy_init(chip->strategy);
			chip->limits.pps_normal_high_temp +=
				PPS_TEMP_WARM_RANGE_THD;
			chg_err("switch temp range:%d", vbat_temp_cur);
		}

	} else {
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_WARM;
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_WARM;
		ret = chip->limits.pps_strategy_normal_current;
	}

	return ret;
}

static int
oplus_pps_set_current_temp_normal_range(struct oplus_pps *chip,
					 int vbat_temp_cur)
{
	int ret = chip->limits.pps_strategy_normal_current;
	int batt_soc, batt_vol;
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return -ENODEV;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return rc;
	}
	batt_vol = data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_SOC, &data, false);
	if (rc < 0) {
		chg_err("can't get soc, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return rc;
	}
	batt_soc = data.intval;

	switch (chip->pps_fastchg_batt_temp_status) {
	case PPS_BAT_TEMP_NORMAL_HIGH:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp0) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH0;
			ret = chip->limits.pps_strategy_high_current0;
		} else if (vbat_temp_cur >= chip->limits.pps_normal_low_temp) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.pps_strategy_normal_current;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_NORMAL_LOW;
			chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_LOW;
			ret = chip->limits.pps_strategy_normal_current;
			oplus_pps_reset_temp_range(chip);
			chip->limits.pps_normal_low_temp +=
				PPS_TEMP_WARM_RANGE_THD;
		}
		break;
	case PPS_BAT_TEMP_HIGH0:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp1) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH1;
			ret = chip->limits.pps_strategy_high_current1;
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp0) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW0;
			ret = chip->limits.pps_strategy_low_current0;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH0;
			ret = chip->limits.pps_strategy_high_current0;
		}
		break;
	case PPS_BAT_TEMP_HIGH1:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp2) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH2;
			ret = chip->limits.pps_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp1) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW1;
			ret = chip->limits.pps_strategy_low_current1;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH1;
			ret = chip->limits.pps_strategy_high_current1;
		}
		break;
	case PPS_BAT_TEMP_HIGH2:
		if (chip->limits.pps_normal_high_temp != -EINVAL &&
		    vbat_temp_cur > chip->limits.pps_normal_high_temp) {
			chip->limits.pps_strategy_change_count++;
			if (chip->limits.pps_strategy_change_count >=
			    PPS_TEMP_OVER_COUNTS) {
				chip->limits.pps_strategy_change_count = 0;
				if (batt_soc < chip->limits.pps_warm_allow_soc &&
				    batt_vol < chip->limits.pps_warm_allow_vol) {
					chip->pps_fastchg_batt_temp_status =
						PPS_BAT_TEMP_SWITCH_CURVE;
					chip->pps_temp_cur_range =
						PPS_TEMP_RANGE_INIT;
					ret = chip->limits.pps_strategy_high_current2;
					oplus_pps_reset_temp_range(chip);
					(void)oplus_chg_strategy_init(chip->strategy);
					chip->limits.pps_normal_high_temp -=
						PPS_TEMP_WARM_RANGE_THD;
					chg_err("switch temp range:%d", vbat_temp_cur);
				} else {
					chg_err("high temp_over:%d", vbat_temp_cur);
					chip->pps_fastchg_batt_temp_status =
						PPS_BAT_TEMP_EXIT;
					ret = chip->limits
						      .pps_over_high_or_low_current;
				}
			}
		} else if (chip->limits.pps_batt_over_high_temp != -EINVAL &&
			   vbat_temp_cur >
				   chip->limits.pps_batt_over_high_temp) {
			chip->limits.pps_strategy_change_count++;
			if (chip->limits.pps_strategy_change_count >=
			    PPS_TEMP_OVER_COUNTS) {
				chip->limits.pps_strategy_change_count = 0;
				chip->pps_fastchg_batt_temp_status =
					PPS_BAT_TEMP_EXIT;
				ret = chip->limits.pps_over_high_or_low_current;
				chg_err("over_high temp_over:%d", vbat_temp_cur);
			}
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp2) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW2;
			ret = chip->limits.pps_strategy_low_current2;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH2;
			ret = chip->limits.pps_strategy_high_current2;
		}
		break;
	case PPS_BAT_TEMP_LOW0:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp0) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH0;
			ret = chip->limits.pps_strategy_high_current0;
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp0) { /* T<39C */
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.pps_strategy_normal_current;
			oplus_pps_reset_temp_range(chip);
			chip->limits.pps_strategy_batt_low_temp0 +=
				PPS_TEMP_WARM_RANGE_THD;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW0;
			ret = chip->limits.pps_strategy_low_current0;
		}
		break;
	case PPS_BAT_TEMP_LOW1:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp1) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH1;
			ret = chip->limits.pps_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp0) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW0;
			ret = chip->limits.pps_strategy_low_current0;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW1;
			ret = chip->limits.pps_strategy_low_current1;
		}
		break;
	case PPS_BAT_TEMP_LOW2:
		if (vbat_temp_cur >
		    chip->limits.pps_strategy_batt_high_temp2) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_HIGH2;
			ret = chip->limits.pps_strategy_high_current2;
		} else if (vbat_temp_cur <
			   chip->limits.pps_strategy_batt_low_temp1) {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW1;
			ret = chip->limits.pps_strategy_low_current1;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LOW2;
			ret = chip->limits.pps_strategy_low_current2;
		}
		break;
	default:
		break;
	}
	chg_info("the ret: %d, the temp =%d, status = %d\r\n", ret,
		 vbat_temp_cur, chip->pps_fastchg_batt_temp_status);
	return ret;
}

static int
oplus_pps_set_current_temp_low_normal_range(struct oplus_pps *chip,
					     int vbat_temp_cur)
{
	int ret = chip->limits.pps_strategy_normal_current;

	if (vbat_temp_cur < chip->limits.pps_normal_low_temp &&
	    vbat_temp_cur >=
		    chip->limits.pps_little_cool_temp) { /* 20C<=T<35C */
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_LOW;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NORMAL_LOW;
		ret = chip->limits.pps_strategy_normal_current;
	} else {
		if (vbat_temp_cur >= chip->limits.pps_normal_low_temp) {
			chip->limits.pps_normal_low_temp -=
				PPS_TEMP_LOW_RANGE_THD;
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_NORMAL_HIGH;
			ret = chip->limits.pps_strategy_normal_current;
			chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_HIGH;
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_LITTLE_COOL;
			chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COOL;
			ret = chip->limits.pps_strategy_normal_current;
			oplus_pps_reset_temp_range(chip);
			chip->limits.pps_little_cool_temp +=
				PPS_TEMP_LOW_RANGE_THD;
		}
	}

	return ret;
}

static int
oplus_pps_set_current_temp_little_cool_range(struct oplus_pps *chip,
					      int vbat_temp_cur)
{
	int ret = 0;

	if (vbat_temp_cur < chip->limits.pps_little_cool_temp &&
	    vbat_temp_cur >= chip->limits.pps_cool_temp) { /* 12C<=T<20C */
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_LITTLE_COOL;
		ret = chip->limits.pps_strategy_normal_current;
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COOL;
	} else if (vbat_temp_cur >= chip->limits.pps_little_cool_temp) {
		chip->limits.pps_little_cool_temp -= PPS_TEMP_LOW_RANGE_THD;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NORMAL_LOW;
		ret = chip->limits.pps_strategy_normal_current;
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_NORMAL_LOW;
	} else {
		if (chip->ui_soc <= chip->limits.pps_strategy_soc_high) {
			chip->limits.pps_strategy_change_count++;
			if (chip->limits.pps_strategy_change_count >=
			    PPS_TEMP_OVER_COUNTS) {
				chip->limits.pps_strategy_change_count = 0;
				chip->pps_fastchg_batt_temp_status =
					PPS_BAT_TEMP_SWITCH_CURVE;
				chip->pps_temp_cur_range =
					PPS_TEMP_RANGE_INIT;
				(void)oplus_chg_strategy_init(chip->strategy);
				chg_err("switch temp range:%d", vbat_temp_cur);
			}
		} else {
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_COOL;
			chip->pps_temp_cur_range = PPS_TEMP_RANGE_COOL;
		}
		ret = chip->limits.pps_strategy_normal_current;
		oplus_pps_reset_temp_range(chip);
		chip->limits.pps_cool_temp += PPS_TEMP_LOW_RANGE_THD;
	}

	return ret;
}

static int oplus_pps_set_current_temp_cool_range(struct oplus_pps *chip,
						  int vbat_temp_cur)
{
	int ret = chip->limits.pps_strategy_normal_current;;
	if (chip->limits.pps_batt_over_low_temp != -EINVAL &&
	    vbat_temp_cur < chip->limits.pps_batt_over_low_temp) {
		chip->limits.pps_strategy_change_count++;
		if (chip->limits.pps_strategy_change_count >=
		    PPS_TEMP_OVER_COUNTS) {
			chip->limits.pps_strategy_change_count = 0;
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_EXIT;
			ret = chip->limits.pps_over_high_or_low_current;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur < chip->limits.pps_cool_temp &&
		   vbat_temp_cur >=
			   chip->limits.pps_little_cold_temp) { /* 5C <=T<12C */
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_COOL;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_COOL;
		ret = chip->limits.pps_strategy_normal_current;
	} else if (vbat_temp_cur >= chip->limits.pps_cool_temp) {
		chip->limits.pps_strategy_change_count++;
		if (chip->limits.pps_strategy_change_count >=
		    PPS_TEMP_OVER_COUNTS) {
			chip->limits.pps_strategy_change_count = 0;
			if (chip->ui_soc <= chip->limits.pps_strategy_soc_high) {
				chip->pps_fastchg_batt_temp_status =
					PPS_BAT_TEMP_SWITCH_CURVE;
				chip->pps_temp_cur_range =
					PPS_TEMP_RANGE_INIT;
				(void)oplus_chg_strategy_init(chip->strategy);
				chg_info("switch temp range:%d", vbat_temp_cur);
			} else {
				chip->pps_fastchg_batt_temp_status =
					PPS_BAT_TEMP_LITTLE_COOL;
				chip->pps_temp_cur_range =
					PPS_TEMP_RANGE_LITTLE_COOL;
			}
			oplus_pps_reset_temp_range(chip);
			chip->limits.pps_cool_temp -= PPS_TEMP_LOW_RANGE_THD;
		}

		ret = chip->limits.pps_strategy_normal_current;
	} else {
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_LITTLE_COLD;
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COLD;
		ret = chip->limits.pps_strategy_normal_current;
		oplus_pps_reset_temp_range(chip);
		chip->limits.pps_little_cold_temp += PPS_TEMP_LOW_RANGE_THD;
	}
	return ret;
}

static int
oplus_pps_set_current_temp_little_cold_range(struct oplus_pps *chip,
					      int vbat_temp_cur)
{
	int ret = chip->limits.pps_strategy_normal_current;

	if (chip->limits.pps_batt_over_low_temp != -EINVAL &&
	    vbat_temp_cur < chip->limits.pps_batt_over_low_temp) {
		chip->limits.pps_strategy_change_count++;
		if (chip->limits.pps_strategy_change_count >=
		    PPS_TEMP_OVER_COUNTS) {
			chip->limits.pps_strategy_change_count = 0;
			ret = chip->limits.pps_over_high_or_low_current;
			chip->pps_fastchg_batt_temp_status =
				PPS_BAT_TEMP_EXIT;
			chg_err("temp_over:%d", vbat_temp_cur);
		}
	} else if (vbat_temp_cur <
		   chip->limits.pps_little_cold_temp) { /* 0C<=T<5C */
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_LITTLE_COLD;
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_LITTLE_COLD;
		ret = chip->limits.pps_strategy_normal_current;
	} else {
		chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_COOL;
		ret = chip->limits.pps_strategy_normal_current;
		chip->pps_temp_cur_range = PPS_TEMP_RANGE_COOL;
		oplus_pps_reset_temp_range(chip);
		chip->limits.pps_little_cold_temp -= PPS_TEMP_LOW_RANGE_THD;
	}

	return ret;
}

static int oplus_pps_get_batt_temp_curr(struct oplus_pps *chip)

{
	int ret;
	int vbat_temp_cur;
	union mms_msg_data data = { 0 };

	if (!is_gauge_topic_available(chip) || !chip->comm_topic) {
		chg_err("comm or gauge topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return -ENODEV;
	}
	ret = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, false);
	if (ret < 0) {
		chg_err("can't get shell temp data, rc=%d", ret);
	} else {
		chip->shell_temp = data.intval;
	}

	vbat_temp_cur = chip->shell_temp;
	ret = chip->limits.pps_strategy_normal_current;
	switch (chip->pps_temp_cur_range) {
	case PPS_TEMP_RANGE_WARM:
		ret = oplus_pps_set_current_warm_range(chip, vbat_temp_cur);
		break;
	case PPS_TEMP_RANGE_NORMAL_HIGH:
		ret = oplus_pps_set_current_temp_normal_range(chip,
							       vbat_temp_cur);
		break;
	case PPS_TEMP_RANGE_NORMAL_LOW:
		ret = oplus_pps_set_current_temp_low_normal_range(
			chip, vbat_temp_cur);
		break;
	case PPS_TEMP_RANGE_LITTLE_COOL:
		ret = oplus_pps_set_current_temp_little_cool_range(
			chip, vbat_temp_cur);
		break;
	case PPS_TEMP_RANGE_COOL:
		ret = oplus_pps_set_current_temp_cool_range(chip,
							     vbat_temp_cur);
		break;
	case PPS_TEMP_RANGE_LITTLE_COLD:
		ret = oplus_pps_set_current_temp_little_cold_range(
			chip, vbat_temp_cur);
		break;
	default:
		break;
	}

	if (ret > 0)
		vote(chip->pps_curr_votable, BATT_TEMP_VOTER, true, ret, false);
	else if (ret == 0)
		ret = -EINVAL;

	return ret;
}

static void oplus_pps_check_low_curr_temp_status(struct oplus_pps *chip)
{
	static int t_cnts = 0;
	int vbat_temp_cur;
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_gauge_topic_available(chip) || !chip->comm_topic) {
		chg_err("gauge or comm topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, false);
	if (rc < 0) {
		chg_err("can't get battery temp, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	vbat_temp_cur = data.intval;

	if (((vbat_temp_cur >=
	      chip->limits.pps_low_curr_full_normal_high_temp) ||
	     (vbat_temp_cur < chip->limits.pps_low_curr_full_cool_temp)) &&
	    (chip->pps_low_curr_full_temp_status !=
	     PPS_LOW_CURR_FULL_CURVE_TEMP_MAX)) {
		t_cnts++;
		if (t_cnts >= PPS_TEMP_OVER_COUNTS) {
			chip->pps_low_curr_full_temp_status =
				PPS_LOW_CURR_FULL_CURVE_TEMP_MAX;
			t_cnts = 0;
		}
	} else if ((vbat_temp_cur >=
		    chip->limits.pps_low_curr_full_normal_low_temp) &&
		   (chip->pps_low_curr_full_temp_status !=
		    PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH)) {
		t_cnts++;
		if (t_cnts >= PPS_TEMP_OVER_COUNTS) {
			chip->pps_low_curr_full_temp_status =
				PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_HIGH;
			t_cnts = 0;
		}
	} else if ((vbat_temp_cur >=
		    chip->limits.pps_low_curr_full_little_cool_temp) &&
		   (chip->pps_low_curr_full_temp_status !=
		    PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW)) {
		t_cnts++;
		if (t_cnts >= PPS_TEMP_OVER_COUNTS) {
			chip->pps_low_curr_full_temp_status =
				PPS_LOW_CURR_FULL_CURVE_TEMP_NORMAL_LOW;
			t_cnts = 0;
		}
	} else if (chip->pps_low_curr_full_temp_status !=
		   PPS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL) {
		t_cnts++;
		if (t_cnts >= PPS_TEMP_OVER_COUNTS) {
			chip->pps_low_curr_full_temp_status =
				PPS_LOW_CURR_FULL_CURVE_TEMP_LITTLE_COOL;
			t_cnts = 0;
		}
	} else {
		t_cnts = 0;
	}
}

static void oplus_pps_check_sw_full(struct oplus_pps *chip, struct puc_strategy_ret_data *data)
{
	int cool_sw_vth, normal_sw_vth, normal_hw_vth;
	union mms_msg_data mms_data = { 0 };
	int batt_temp, vbat_mv;
	int rc;

#define PPS_FULL_COUNTS_COOL		6
#define PPS_FULL_COUNTS_SW		6
#define PPS_FULL_COUNTS_HW		3

	if (!chip->oplus_pps_adapter) {
		cool_sw_vth = chip->limits.pps_full_cool_sw_vbat_third;
		normal_sw_vth = chip->limits.pps_full_normal_sw_vbat_third;
		normal_hw_vth = chip->limits.pps_full_normal_hw_vbat_third;
	} else {
		cool_sw_vth = chip->limits.pps_full_cool_sw_vbat;
		normal_sw_vth = chip->limits.pps_full_normal_sw_vbat;
		normal_hw_vth = chip->limits.pps_full_normal_hw_vbat;
	}

	if (!is_gauge_topic_available(chip) || !chip->comm_topic) {
		chg_err("gauge or comm topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &mms_data, false);
	if (rc < 0) {
		chg_err("can't get battery temp, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	batt_temp = mms_data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &mms_data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	vbat_mv = mms_data.intval;

	if ((batt_temp < chip->limits.pps_cool_temp) && (vbat_mv > cool_sw_vth)) {
		chip->count.cool_fw++;
		if (chip->count.cool_fw >= PPS_FULL_COUNTS_COOL) {
			chip->count.cool_fw = 0;
			vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.cool_fw = 0;
	}

	if ((batt_temp >= chip->limits.pps_cool_temp) &&
	    (batt_temp < chip->limits.pps_batt_over_high_temp)) {
		if ((vbat_mv > normal_sw_vth) && data->last_gear) {
			chip->count.sw_full++;
			if (chip->count.sw_full >= PPS_FULL_COUNTS_SW) {
				chip->count.sw_full = 0;
				vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
				return;
			}
		}

		if ((vbat_mv > normal_hw_vth)) {
			chip->count.hw_full++;
			if (chip->count.hw_full >= PPS_FULL_COUNTS_HW) {
				chip->count.hw_full = 0;
				vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
				return;
			}
		}
	} else {
		chip->count.sw_full = 0;
		chip->count.hw_full = 0;
	}

	if ((chip->pps_fastchg_batt_temp_status == PPS_BAT_TEMP_WARM) &&
	    (vbat_mv > chip->limits.pps_full_warm_vbat)) {
		chip->count.cool_fw++;
		if (chip->count.cool_fw >= PPS_FULL_COUNTS_COOL) {
			chip->count.cool_fw = 0;
			vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.cool_fw = 0;
	}
}

static void oplus_pps_check_low_curr_full(struct oplus_pps *chip)
{
	int i, temp_current, temp_vbatt, temp_status, iterm, vterm;
	bool low_curr = false;
	union mms_msg_data data = { 0 };
	int rc;

#define PPS_FULL_COUNTS_LOW_CURR	6

	oplus_pps_check_low_curr_temp_status(chip);

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	temp_vbatt = data.intval;
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	temp_current = -data.intval;

	temp_status = chip->pps_low_curr_full_temp_status;
	if (temp_status == PPS_LOW_CURR_FULL_CURVE_TEMP_MAX)
		return;

	for (i = 0; i < chip->low_curr_full_curves_temp[temp_status].full_curve_num; i++) {
		iterm = chip->low_curr_full_curves_temp[temp_status].full_curves[i].iterm;
		vterm = chip->low_curr_full_curves_temp[temp_status].full_curves[i].vterm;

		if ((temp_current <= iterm) && (temp_vbatt >= vterm)) {
			low_curr = true;
			break;
		}
	}

	if (low_curr) {
		chip->count.low_curr_full++;
		if (chip->count.low_curr_full > PPS_FULL_COUNTS_LOW_CURR) {
			chip->count.low_curr_full = 0;
			vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
		}
	} else {
		chip->count.low_curr_full = 0;
	}
}

static void oplus_pps_check_timeout(struct oplus_pps *chip)
{
	unsigned long tmp_time;

	tmp_time = jiffies - chip->timer.monitor_jiffies;
	chip->timer.monitor_jiffies = jiffies;
	if (chip->timer.pps_max_time_ms <= jiffies_to_msecs(tmp_time)) {
		chip->timer.pps_max_time_ms = 0;
		vote(chip->pps_disable_votable, TIMEOUT_VOTER, true, 1, false);
		return;
	}
	chip->timer.pps_max_time_ms -= jiffies_to_msecs(tmp_time);
}

static void oplus_pps_check_full(struct oplus_pps *chip, struct puc_strategy_ret_data *data)
{
	oplus_pps_check_sw_full(chip, data);
	oplus_pps_check_low_curr_full(chip);
}

static bool oplus_pps_btb_temp_check(struct oplus_pps *chip)
{
	bool btb_status = true;
	int btb_temp, usb_temp;
	static unsigned char temp_over_count = 0;

#define PPS_BTB_USB_OVER_CNTS		9

	btb_temp = oplus_wired_get_batt_btb_temp();
	usb_temp = oplus_wired_get_usb_btb_temp();

	if (btb_temp >= PPS_BTB_OVER_TEMP || usb_temp >= PPS_BTB_OVER_TEMP) {
		temp_over_count++;
		if (temp_over_count > PPS_BTB_USB_OVER_CNTS) {
			btb_status = false;
			chg_err("btb or usb temp over");
		}
	} else {
		temp_over_count = 0;
	}

	return btb_status;
}

static void oplus_pps_check_ibat_safety(struct oplus_pps *chip)
{
	struct timespec ts_current;
	union mms_msg_data data = { 0 };
	int ibat;
	int ibat_hi_th;
	int rc;

#define PPS_IBAT_LOW_MIN	2000
#define PPS_IBAT_LOW_CNT	4
#define PPS_IBAT_HIGH_CNT	8

	ts_current = oplus_current_kernel_time();
	if ((ts_current.tv_sec - chip->timer.ibat_timer.tv_sec) < PPS_UPDATE_IBAT_TIME)
		return;
	chip->timer.ibat_timer = ts_current;

	if (!is_gauge_topic_available(chip)) {
		chg_err("gauge topic not found\n");
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &data, false);
	if (rc < 0) {
		chg_err("can't get ibat, rc=%d\n", rc);
		vote(chip->pps_disable_votable, NO_DATA_VOTER, true, 1, false);
		return;
	}
	ibat = data.intval;

	chg_err("ibat now :%d\n", ibat);
	if (ibat > PPS_IBAT_LOW_MIN) {
		chip->count.ibat_low++;
		if (chip->count.ibat_low >= PPS_IBAT_LOW_CNT) {
			vote(chip->pps_disable_votable, IBAT_OVER_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.ibat_low = 0;
	}

	if (!chip->oplus_pps_adapter)
		ibat_hi_th = -chip->limits.pps_ibat_over_third;
	else
		ibat_hi_th = -chip->limits.pps_ibat_over_oplus;
	if (ibat < ibat_hi_th) {
		chip->count.ibat_high++;
		if (chip->count.ibat_high >= PPS_IBAT_HIGH_CNT) {
			chip->quit_pps_protocol = true;
			vote(chip->pps_disable_votable, IBAT_OVER_VOTER, true, 1, false);
			return;
		}
	} else {
		chip->count.ibat_high = 0;
	}
}

static void oplus_pps_check_temp(struct oplus_pps *chip)
{
	struct timespec ts_current;
	union mms_msg_data data = { 0 };
	int batt_temp;
	int rc;

#define PPS_FG_TEMP_PROTECTION	800
#define PPS_TFG_OV_CNT		6
#define PPS_BTB_OV_CNT		8
#define PPS_TBATT_OV_CNT	1

	ts_current = oplus_current_kernel_time();
	if ((ts_current.tv_sec - chip->timer.temp_timer.tv_sec) < PPS_UPDATE_TEMP_TIME)
		return;

	chip->timer.temp_timer = ts_current;

	if (!oplus_pps_btb_temp_check(chip)) {
		chip->count.btb_high++;
		if (chip->count.btb_high >= PPS_BTB_OV_CNT) {
			chip->count.btb_high = 0;
			vote(chip->pps_disable_votable, BTB_TEMP_OVER_VOTER, true, 1, false);
			if (is_wired_icl_votable_available(chip))
				vote(chip->wired_icl_votable, BTB_TEMP_OVER_VOTER, true,
				     BTB_TEMP_OVER_MAX_INPUT_CUR, true);
		}
	} else {
		chip->count.btb_high = 0;
	}

	if (chip->pps_fastchg_batt_temp_status == PPS_BAT_TEMP_EXIT) {
		chg_err("pps battery temp out of range\n");
		chip->count.tbatt_over++;
		if (chip->count.tbatt_over >= PPS_TBATT_OV_CNT)
			vote(chip->pps_not_allow_votable, BATT_TEMP_VOTER, true, 1, false);
	} else {
		chip->count.tbatt_over = 0;
	}

	if (chip->pps_fastchg_batt_temp_status == PPS_BAT_TEMP_SWITCH_CURVE) {
		chg_err("pps battery temp switch curve range\n");
		vote(chip->pps_disable_votable, SWITCH_RANGE_VOTER, true, 1, false);
	}

	if (is_gauge_topic_available(chip)) {
		rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_REAL_TEMP, &data, false);
		if (rc >= 0) {
			batt_temp = data.intval;
			if (batt_temp >= PPS_FG_TEMP_PROTECTION) {
				chg_err("pps tfg > 80\n");
				chip->count.tfg_over++;
				if (chip->count.tfg_over >= PPS_TFG_OV_CNT) {
					chip->count.tfg_over = 0;
					vote(chip->pps_disable_votable, TFG_VOTER, true, 1, false);
				}
			} else {
				chip->count.tfg_over = 0;
			}
		}
	}
}

static void oplus_pps_set_cool_down_curr(struct oplus_pps *chip, int cool_down)
{
	int target_curr;

	if (cool_down >= ARRAY_SIZE(pps_cool_down_oplus_curve))
		cool_down = ARRAY_SIZE(pps_cool_down_oplus_curve) - 1;
	target_curr = pps_cool_down_oplus_curve[cool_down];

	if (chip->cp_ratio != 0) {
		target_curr /= chip->cp_ratio;
	} else {
		chg_err("unsupported cp work mode, mode=%d, ratio=%d\n", chip->cp_work_mode, chip->cp_ratio);
		return;
	}

	vote(chip->pps_curr_votable, COOL_DOWN_VOTER, true, target_curr, false);
}

static void oplus_pps_imp_check(struct oplus_pps *chip)
{
	int curr;
	int rc;

	if (!chip->imp_uint) {
		/* prevent triggering watchdog */
		rc = oplus_pps_get_pps_status_info(chip, &chip->pps_status_info);
		if (rc < 0)
			chg_err("pps get src info error\n");
		return;
	}

	curr = oplus_imp_uint_get_allow_curr(chip->imp_uint);
	if (curr < 0) {
		chg_err("can't get pps channel impedance current limite, exit pps charging\n");
		vote(chip->pps_disable_votable, IMP_VOTER, true, 1, false);
	} else if (curr == 0) {
		chg_err("pps channel impedance is too high, exit pps charging\n");
		vote(chip->pps_disable_votable, IMP_VOTER, true, 1, false);
	} else {
		vote(chip->pps_curr_votable, IMP_VOTER, true, curr, false);
	}
}

static void oplus_pps_watchdog(struct oplus_pps *chip)
{
	int rc;
	bool work_start;

	rc = oplus_pps_cp_get_work_status(chip, &work_start);
	if (rc < 0) {
		chg_err("can't get cp work status, rc=%d\n", rc);
		return;
	}
	if (!work_start)
		return;
	rc = oplus_pps_cp_set_work_start(chip, true);
	if (rc < 0)
		chg_err("set cp work start error, rc=%d\n", rc);
}

static void oplus_pps_protection_check(struct oplus_pps *chip, struct puc_strategy_ret_data *data)
{
	oplus_pps_check_timeout(chip);
	oplus_pps_check_full(chip, data);
	oplus_pps_check_ibat_safety(chip);
	oplus_pps_check_temp(chip);
	oplus_pps_imp_check(chip);
	oplus_pps_watchdog(chip);
}

enum oplus_ibat_check_result {
	OPLUS_IBAT_LOW,
	OPLUS_IBAT_OK,
	OPLUS_IBAT_HIGH,
	OPLUS_IBAT_OVER,
	OPLUS_IBAT_ABNOR,
};

static int oplus_third_pps_current_check(struct oplus_pps *chip)
{
	int rc;
	int ibat_ma;
	int ibat_ok_thd;
	int ibat_high_thd;
	int ibat_over_thd;
	union mms_msg_data msg_data = { 0 };

#define OPLUS_PPS_OK_CURRENT_DEBOUNCE (200)
#define OPLUS_PPS_OVER_CURRENT_THD (500)
#define OPLUS_PPS_IBAT_ABNORMAL_CURR (0)

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &msg_data, true);
	if (unlikely(rc < 0)) {
		rc = OPLUS_IBAT_ABNOR;
		chg_err("can't get ibat, rc=%d\n", rc);
	} else {
		ibat_ma = -msg_data.intval;
		ibat_ok_thd = chip->target_curr_ma - OPLUS_PPS_OK_CURRENT_DEBOUNCE;
		ibat_high_thd = chip->target_curr_ma + OPLUS_PPS_OK_CURRENT_DEBOUNCE;
		ibat_over_thd = chip->target_curr_ma + OPLUS_PPS_OVER_CURRENT_THD;

		if (ibat_ma > ibat_over_thd) {
			rc = OPLUS_IBAT_OVER;
			chg_err("ibat=%d, ibat_over_thd=%d", ibat_ma, ibat_over_thd);
		} else if (ibat_ma > ibat_high_thd) {
			rc = OPLUS_IBAT_HIGH;
			chg_err("ibat=%d, ibat_ok_thd=%d", ibat_ma, ibat_high_thd);
		} else if (ibat_ma > ibat_ok_thd) {
			rc = OPLUS_IBAT_OK;
		} else {
			rc = OPLUS_IBAT_LOW;
		}

		if (ibat_ma < OPLUS_PPS_IBAT_ABNORMAL_CURR)
			rc = OPLUS_IBAT_ABNOR;
	}
	return rc;
}

static int oplus_third_pps_target_voltage_check(struct oplus_pps *chip)
{
	int rc;
	int vtarget_mv;
	int batt_num;
	int allowed_vbus_min;
	int next_target_vbus_mv;
	union mms_msg_data msg_data = { 0 };

#define TARGET_VBUS_STEP (40)
#define TARGET_VBUS_STEP_IOVER (100)
#define TARGET_VBUS_DEBOUNCE (80)
#define IBAT_OVER_ITARGET_CNT (4)

	batt_num = oplus_gauge_get_batt_num();
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &msg_data, true);
	if (unlikely(rc < 0)) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	} else {
		allowed_vbus_min = msg_data.intval * batt_num * chip->cp_ratio;;
	}

	/* default common resistor 0.35ohm, calculate the best request-vbus */
	vtarget_mv = allowed_vbus_min + chip->target_curr_ma * 35 / 100;
	chg_info("last_vol_set_mv=%d, vtarget_mv=%d, vbus_min=%d, Itarget=%d", chip->vol_set_mv,
		vtarget_mv, allowed_vbus_min, chip->target_curr_ma);
	if (chip->vol_set_mv < vtarget_mv - TARGET_VBUS_DEBOUNCE) {
		chip->need_check_current = true;
		next_target_vbus_mv = chip->vol_set_mv + TARGET_VBUS_STEP;
	} else if (chip->vol_set_mv > vtarget_mv + TARGET_VBUS_DEBOUNCE) {
		chip->need_check_current = false;
		next_target_vbus_mv = chip->vol_set_mv - TARGET_VBUS_STEP;
	} else {
		chip->need_check_current = true;
		next_target_vbus_mv = chip->vol_set_mv;
	}

	if (chip->need_check_current) {
		rc = oplus_third_pps_current_check(chip);
		switch (rc) {
		case OPLUS_IBAT_OVER:
			chip->count.ibat_over++;
			if (chip->count.ibat_over >= IBAT_OVER_ITARGET_CNT) {
				chg_err("current too high continues, exit pps and not recovery");
				chip->quit_pps_protocol = true;
				vote(chip->pps_disable_votable, IBAT_OVER_VOTER, true, 1, false);
			} else {
				next_target_vbus_mv = chip->vol_set_mv - TARGET_VBUS_STEP_IOVER;
			}
			chip->count.ibat_abnormal = 0;
			break;
		case OPLUS_IBAT_HIGH:
			chip->count.ibat_over = 0;
			chip->count.ibat_abnormal = 0;
			next_target_vbus_mv = chip->vol_set_mv - TARGET_VBUS_STEP;
			break;
		case OPLUS_IBAT_OK:
			chip->count.ibat_over = 0;
			chip->count.ibat_abnormal = 0;
			next_target_vbus_mv = chip->vol_set_mv;
			break;
		case OPLUS_IBAT_ABNOR:
			chip->count.ibat_abnormal++;
			if (chip->count.ibat_abnormal >= IBAT_OVER_ITARGET_CNT) {
				chg_err("current abnormal continues, exit pps but allow recovery");
				chip->quit_pps_protocol = true;
				vote(chip->pps_disable_votable, PPS_IBAT_ABNOR_VOTER, true, 1, false);
				chip->count.pps_recovery = 0;
			}
			chg_err("count.ibat_abnormal=%d", chip->count.ibat_abnormal);
			break;
		case OPLUS_IBAT_LOW:
			/* request-vbus calculate by vtarget_mv, don't change it in this case */
			chip->count.ibat_over = 0;
			chip->count.ibat_abnormal = 0;
			break;
		default:
			chip->count.ibat_over = 0;
			chip->count.ibat_abnormal = 0;
			break;
		}
	}

	/* Check the request-vbus not too low or too high */
	if (next_target_vbus_mv < allowed_vbus_min) {
		if (next_target_vbus_mv < chip->vol_set_mv) {
			/* Not allow decrease request-vbus */
			next_target_vbus_mv = chip->vol_set_mv;
			if (chip->count.output_low > IBAT_OVER_ITARGET_CNT) {
				chg_err("output abnormal continues, exit pps and not recovery");
				chip->quit_pps_protocol = true;
				vote(chip->pps_disable_votable, IBAT_OVER_VOTER, true, 1, false);
				chip->count.pps_recovery = 0;
			}
			chip->count.output_low++;
		} else {
			/* Allow increase or keep request-vbus */
			chip->count.output_low = 0;
		}
	} else if (next_target_vbus_mv > chip->config.target_vbus_mv) {
		next_target_vbus_mv = chip->config.target_vbus_mv;
		chip->count.output_low = 0;
	} else {
		chip->count.output_low = 0;
	}

	chip->target_vbus_mv = next_target_vbus_mv;

	return 0;
}

static void oplus_pps_monitor_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_pps *chip =
		container_of(dwork, struct oplus_pps, monitor_work);
	struct puc_strategy_ret_data data;
	int rc;
	int delay = PPS_MONITOR_TIME_MS;
	bool switch_to_ffc = false;

	if (!chip->pps_charging) {
		if (is_wired_suspend_votable_available(chip))
			vote(chip->wired_suspend_votable, PPS_VOTER, true, 1, false);
		rc = oplus_pps_charge_start(chip);
		if (rc < 0)
			goto exit;
		delay = rc;
	} else {
		oplus_pps_cp_reg_dump(chip);
		rc = oplus_pps_get_batt_temp_curr(chip);
		if (rc < 0)
			goto exit;

		rc = oplus_chg_strategy_get_data(chip->strategy, &data);
		if (rc < 0) {
			chg_err("can't get strategy data, rc=%d", rc);
			goto exit;
		}
		if (data.exit) {
			chg_info("exit pps fast charge, start ffc\n");
			vote(chip->pps_disable_votable, CHG_FULL_VOTER, true, 1, false);
			switch_to_ffc = true;
			goto exit;
		}

		oplus_pps_set_cool_down_curr(chip, chip->cool_down);
		oplus_pps_protection_check(chip, &data);
		if (get_client_vote(chip->pps_disable_votable, CHG_FULL_VOTER) > 0) {
			chg_info("exit pps fast charge, start ffc\n");
			switch_to_ffc = true;
			goto exit;
		}
		if (chip->error_count > PPS_ERROR_COUNT_MAX) {
			chg_info("pps communication exception, exit pps mode\n");
			goto exit;
		}
		if (chip->pps_not_allow || chip->pps_disable) {
			chg_info("pps charge not allow or disable, exit pps mode\n");
			goto exit;
		}

		if (chip->oplus_pps_adapter) {
			chip->target_vbus_mv = data.target_vbus;
		} else {
			rc = oplus_third_pps_target_voltage_check(chip);
			if (rc < 0)
				chip->target_vbus_mv = data.target_vbus;
		}

		vote(chip->pps_curr_votable, STEP_VOTER, true, data.target_ibus, false);
	}

	if (chip->wired_online)
		schedule_delayed_work(&chip->monitor_work, msecs_to_jiffies(delay));
	return;

exit:
	if (unlikely(chip->quit_pps_protocol)) {
		oplus_pps_force_exit(chip);
		chip->quit_pps_protocol = false;
		oplus_cpa_request(chip->cpa_topic, CHG_PROTOCOL_PD);
	} else {
		oplus_pps_soft_exit(chip);
		if (switch_to_ffc)
			oplus_comm_switch_ffc(chip->comm_topic);
		if (chip->pps_fastchg_batt_temp_status == PPS_BAT_TEMP_SWITCH_CURVE) {
			chg_info("pps switch_curve, need retry start pps\n");
			chip->pps_fastchg_batt_temp_status = PPS_BAT_TEMP_NATURAL;
			schedule_delayed_work(&chip->switch_check_work, msecs_to_jiffies(100));
		}
	}
}

enum {
	OPLUS_PPS_CURR_UPDATE_50MA = 50,
	OPLUS_PPS_CURR_UPDATE_100MA = 100,
	OPLUS_PPS_CURR_UPDATE_200MA = 200,
	OPLUS_PPS_CURR_UPDATE_300MA = 300,
	OPLUS_PPS_CURR_UPDATE_400MA = 400,
	OPLUS_PPS_CURR_UPDATE_500MA = 500,
	OPLUS_PPS_CURR_UPDATE_1A = 1000,
	OPLUS_PPS_CURR_UPDATE_1P5A = 1500,
	OPLUS_PPS_CURR_UPDATE_2A = 2000,
	OPLUS_PPS_CURR_UPDATE_2P5A = 2500,
	OPLUS_PPS_CURR_UPDATE_3A = 3000,
};

static void oplus_pps_current_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_pps *chip =
		container_of(dwork, struct oplus_pps, current_work);
	int target_vbus;
	int curr_set, update_size;
	int rc;

	if (!chip->pps_charging)
		return;

#define PPS_CURR_CHANGE_UPDATE_DELAY		200
#define PPS_CURR_NO_CHANGE_UPDATE_DELAY	500

	if (chip->oplus_pps_adapter) {
		if (abs(chip->curr_set_ma - chip->target_curr_ma) >= OPLUS_PPS_CURR_UPDATE_300MA)
			update_size = OPLUS_PPS_CURR_UPDATE_300MA;
		else
			update_size = OPLUS_PPS_CURR_UPDATE_100MA;
		curr_set = chip->curr_set_ma;

		if ((curr_set > chip->target_curr_ma) && ((curr_set - chip->target_curr_ma) >= update_size))
			curr_set -= update_size;
		else if ((chip->target_curr_ma > curr_set) && ((chip->target_curr_ma - curr_set) >= update_size))
			curr_set += update_size;
		else
			curr_set = chip->target_curr_ma;
	} else {
		/* third pps adapter */
		curr_set = chip->target_curr_ma;
	}

	target_vbus = chip->target_vbus_mv;
	if ((target_vbus != chip->vol_set_mv) || (curr_set != chip->curr_set_ma)) {
		rc = oplus_pps_cp_set_iin(chip, curr_set);
		if (rc < 0) {
			chg_err("set cp input current error, rc=%d\n", rc);
		} else {
			rc = oplus_pps_pdo_set(chip, target_vbus, curr_set);
			if (rc < 0)
				chg_err("pdo set error, rc=%d\n", rc);
		}
	}

	if (chip->curr_set_ma != chip->target_curr_ma)
		schedule_delayed_work(&chip->current_work, msecs_to_jiffies(PPS_CURR_CHANGE_UPDATE_DELAY));
	else
		schedule_delayed_work(&chip->current_work, msecs_to_jiffies(PPS_CURR_NO_CHANGE_UPDATE_DELAY));
}

static void oplus_pps_wired_online_work(struct work_struct *work)
{
	struct oplus_pps *chip =
		container_of(work, struct oplus_pps, wired_online_work);

	if (!chip->wired_online) {
		oplus_pps_force_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);

		if (is_wired_icl_votable_available(chip))
			vote(chip->wired_icl_votable, BTB_TEMP_OVER_VOTER,
			     false, 0, true);
	}
}

static void oplus_pps_force_exit_work(struct work_struct *work)
{
	struct oplus_pps *chip =
		container_of(work, struct oplus_pps, force_exit_work);

	if (chip->pps_online) {
		oplus_pps_force_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);
	}
}

static void oplus_pps_soft_exit_work(struct work_struct *work)
{
	struct oplus_pps *chip =
		container_of(work, struct oplus_pps, soft_exit_work);

	if (chip->pps_charging) {
		oplus_pps_soft_exit(chip);
		cancel_delayed_work_sync(&chip->switch_check_work);
		cancel_delayed_work_sync(&chip->monitor_work);
		cancel_delayed_work_sync(&chip->current_work);
	}
}

static void oplus_pps_gauge_update_work(struct work_struct *work)
{
	int rc = 0;
	union mms_msg_data msg_data = { 0 };

	struct oplus_pps *chip =
		container_of(work, struct oplus_pps, gauge_update_work);

#define PPS_RECOVERY_IBAT_THD (-1000)
#define PPS_RECOVERY_IBAT_TIMES (10)

	if (chip->pps_not_allow)
		oplus_pps_charge_allow_check(chip);

	if (is_client_vote_enabled(chip->pps_disable_votable, PPS_IBAT_ABNOR_VOTER)) {
		rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_CURR, &msg_data, true);
		if (unlikely(rc < 0)) {
			chg_err("can't get ibat, rc=%d\n", rc);
		} else {
			if (msg_data.intval < PPS_RECOVERY_IBAT_THD) {
				if (chip->count.pps_recovery > PPS_RECOVERY_IBAT_TIMES) {
					vote(chip->pps_disable_votable, PPS_IBAT_ABNOR_VOTER, false, 0, false);
					oplus_cpa_request(chip->cpa_topic, CHG_PROTOCOL_PPS);
				}
				chip->count.pps_recovery++;
			} else {
				chip->count.pps_recovery = 0;
			}
		}
	}

	if (!chip->shell_temp_ready) {
		rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &msg_data, false);
		if (rc < 0) {
			chg_err("can't get battery temp, rc=%d\n", rc);
		} else {
			chip->shell_temp = msg_data.intval;
			if (chip->shell_temp != GAUGE_INVALID_TEMP) {
				chip->shell_temp_ready = true;
				vote(chip->pps_boot_votable, SHELL_TEMP_VOTER, false, 0, false);
			}
		}
	}
}

static void oplus_pps_comm_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_pps *chip = subs->priv_data;
	union mms_msg_data data = { 0 };
	int rc;

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_UI_SOC:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->ui_soc = data.intval;
			break;
		case COMM_ITEM_TEMP_REGION:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->batt_temp_region = data.intval;
			break;
		case COMM_ITEM_CHARGE_SUSPEND:
		case COMM_ITEM_CHARGING_DISABLE:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get charge suspend status, rc=%d", rc);
			else
				vote(chip->pps_not_allow_votable, USER_VOTER, !!data.intval, data.intval, false);
			break;
		case COMM_ITEM_COOL_DOWN:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get cool down data, rc=%d", rc);
			else
				chip->cool_down = data.intval;
			break;
		case COMM_ITEM_SHELL_TEMP:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0) {
				chg_err("can't get shell temp data, rc=%d", rc);
			} else {
				chip->shell_temp = data.intval;
				if (unlikely(!chip->shell_temp_ready && chip->shell_temp != GAUGE_INVALID_TEMP)) {
					chip->shell_temp_ready = true;
					vote(chip->pps_boot_votable, SHELL_TEMP_VOTER, false, 0, false);
				}
			}
			break;
		case COMM_ITEM_LED_ON:
			rc = oplus_mms_get_item_data(chip->comm_topic, id,
						     &data, false);
			if (rc < 0)
				chg_err("can't get led_on data, rc=%d", rc);
			else
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

static void oplus_pps_subscribe_comm_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_pps *chip = prv_data;
	union mms_msg_data data = { 0 };
	int rc;

	chip->comm_topic = topic;
	chip->comm_subs =
		oplus_mms_subscribe(topic, chip,
				    oplus_pps_comm_subs_callback, "pps");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe comm topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(topic, COMM_ITEM_UI_SOC, &data, true);
	chip->ui_soc = data.intval;
	oplus_mms_get_item_data(topic, COMM_ITEM_TEMP_REGION, &data, true);
	chip->batt_temp_region = data.intval;

	rc = oplus_mms_get_item_data(topic, COMM_ITEM_CHARGE_SUSPEND, &data, true);
	if (rc < 0)
		chg_err("can't get charge suspend status, rc=%d", rc);
	else
		vote(chip->pps_not_allow_votable, USER_VOTER, !!data.intval, data.intval, false);
	rc = oplus_mms_get_item_data(topic, COMM_ITEM_CHARGING_DISABLE, &data, true);
	if (rc < 0)
		chg_err("can't get charge disable status, rc=%d", rc);
	else
		vote(chip->pps_not_allow_votable, USER_VOTER, !!data.intval, data.intval, false);

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_COOL_DOWN, &data, true);
	if (rc < 0)
		chg_err("can't get cool down data, rc=%d", rc);
	else
		chip->cool_down = data.intval;
	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_LED_ON, &data, true);
	if (rc < 0)
		chg_err("can't get led on data, rc=%d", rc);
	else
		chip->led_on = !!data.intval;

	rc = oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_SHELL_TEMP, &data, true);
	if (rc < 0) {
		chg_err("can't get shell temp data, rc=%d", rc);
	} else {
		chip->shell_temp = data.intval;
		if (unlikely(!chip->shell_temp_ready && chip->shell_temp != GAUGE_INVALID_TEMP)) {
			chip->shell_temp_ready = true;
			vote(chip->pps_boot_votable, SHELL_TEMP_VOTER, false, 0, false);
		}
	}

	vote(chip->pps_boot_votable, COMM_TOPIC_VOTER, false, 0, false);
}

static void oplus_pps_wired_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_pps *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_online = !!data.intval;
			schedule_work(&chip->wired_online_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_pps_subscribe_wired_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_pps *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_pps_wired_subs_callback, "pps");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = !!data.intval;
	vote(chip->pps_boot_votable, WIRED_TOPIC_VOTER, false, 0, false);
}

static void oplus_pps_cpa_subs_callback(struct mms_subscribe *subs,
					 enum mms_msg_type type, u32 id)
{
	struct oplus_pps *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_TIMER:
		break;
	case MSG_TYPE_ITEM:
		switch (id) {
		case CPA_ITEM_ALLOW:
			oplus_mms_get_item_data(chip->cpa_topic, id, &data,
						false);
			chg_err("type=%d", data.intval);
			if (data.intval == CHG_PROTOCOL_PPS)
				schedule_delayed_work(&chip->switch_check_work, 0);
			break;
		case CPA_ITEM_TIMEOUT:
			oplus_mms_get_item_data(chip->cpa_topic, id, &data,
						false);
			if (data.intval == CHG_PROTOCOL_PPS) {
				oplus_pps_switch_to_normal(chip);
				oplus_cpa_switch_end(chip->cpa_topic, CHG_PROTOCOL_PPS);
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

static void oplus_pps_subscribe_cpa_topic(struct oplus_mms *topic,
					   void *prv_data)
{
	struct oplus_pps *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->cpa_topic = topic;
	chip->cpa_subs =
		oplus_mms_subscribe(chip->cpa_topic, chip,
				    oplus_pps_cpa_subs_callback, "pps");
	if (IS_ERR_OR_NULL(chip->cpa_subs)) {
		chg_err("subscribe cpa topic error, rc=%ld\n",
			PTR_ERR(chip->cpa_subs));
		return;
	}

	oplus_mms_get_item_data(chip->cpa_topic, CPA_ITEM_ALLOW, &data, true);
	if (data.intval == CHG_PROTOCOL_PPS)
		schedule_delayed_work(&chip->switch_check_work, msecs_to_jiffies(PPS_MONITOR_CYCLE_MS));

	vote(chip->pps_boot_votable, CPA_TOPIC_VOTER, false, 0, false);
}

static void oplus_pps_gauge_subs_callback(struct mms_subscribe *subs,
					   enum mms_msg_type type, u32 id)
{
	struct oplus_pps *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_TIMER:
		schedule_work(&chip->gauge_update_work);
		break;
	default:
		break;
	}
}

static void oplus_pps_subscribe_gauge_topic(struct oplus_mms *topic,
					     void *prv_data)
{
	struct oplus_pps *chip = prv_data;

	chip->gauge_topic = topic;
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_pps_gauge_subs_callback, "pps");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return;
	}

	vote(chip->pps_boot_votable, GAUGE_TOPIC_VOTER, false, 0, false);
}

static int oplus_pps_update_online(struct oplus_mms *mms,
				    union mms_msg_data *data)
{
	struct oplus_pps *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->pps_online;

	return 0;
}

static int oplus_pps_update_charging(struct oplus_mms *mms,
				      union mms_msg_data *data)
{
	struct oplus_pps *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->pps_charging;

	return 0;
}

static int oplus_pps_update_adapter_id(struct oplus_mms *mms,
					union mms_msg_data *data)
{
	struct oplus_pps *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = (int)chip->adapter_id;

	return 0;
}

static int oplus_pps_update_oplus_adapter(struct oplus_mms *mms,
					   union mms_msg_data *data)
{
	struct oplus_pps *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->oplus_pps_adapter;

	return 0;
}
static void oplus_pps_topic_update(struct oplus_mms *mms, bool publish)
{
}

static struct mms_item oplus_pps_item[] = {
	{
		.desc = {
			.item_id = PPS_ITEM_ONLINE,
			.update = oplus_pps_update_online,
		}
	},
	{
		.desc = {
			.item_id = PPS_ITEM_CHARGING,
			.update = oplus_pps_update_charging,
		}
	},
	{
		.desc = {
			.item_id = PPS_ITEM_ADAPTER_ID,
			.update = oplus_pps_update_adapter_id,
		}
	},
	{
		.desc = {
			.item_id = PPS_ITEM_OPLUS_ADAPTER,
			.update = oplus_pps_update_oplus_adapter,
		}
	},
};

static const struct oplus_mms_desc oplus_pps_desc = {
	.name = "pps",
	.type = OPLUS_MMS_TYPE_PPS,
	.item_table = oplus_pps_item,
	.item_num = ARRAY_SIZE(oplus_pps_item),
	.update_items = NULL,
	.update_items_num = 0,
	.update_interval = 0, /* ms */
	.update = oplus_pps_topic_update,
};

static int oplus_pps_topic_init(struct oplus_pps *chip)
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

	chip->pps_topic =
		devm_oplus_mms_register(chip->dev, &oplus_pps_desc, &mms_cfg);
	if (IS_ERR(chip->pps_topic)) {
		chg_err("Couldn't register pps topic\n");
		rc = PTR_ERR(chip->pps_topic);
		return rc;
	}

	return 0;
}

static int oplus_pps_virq_reg(struct oplus_pps *chip)
{
	return 0;
}

static void oplus_pps_virq_unreg(struct oplus_pps *chip)
{}

static void oplus_pps_ic_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_pps *chip;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	chip->pps_ic = ic;
	rc = oplus_pps_virq_reg(chip);
	if (rc < 0) {
		chg_err("virq register error, rc=%d\n", rc);
		chip->pps_ic = NULL;
		return;
	}

	rc = oplus_pps_topic_init(chip);
	if (rc < 0)
		goto topic_reg_err;

	oplus_mms_wait_topic("common", oplus_pps_subscribe_comm_topic, chip);
	oplus_mms_wait_topic("wired", oplus_pps_subscribe_wired_topic, chip);
	oplus_mms_wait_topic("cpa", oplus_pps_subscribe_cpa_topic, chip);
	oplus_mms_wait_topic("gauge", oplus_pps_subscribe_gauge_topic, chip);
	return;

topic_reg_err:
	oplus_pps_virq_unreg(chip);
}

static int oplus_pps_dpdm_switch_virq_reg(struct oplus_pps *chip)
{
	return 0;
}

static void oplus_pps_dpdm_switch_virq_unreg(struct oplus_pps *chip)
{}

static void oplus_pps_dpdm_switch_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_pps *chip;
	const char *name;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	rc = oplus_pps_dpdm_switch_virq_reg(chip);
	if (rc < 0) {
		chg_err("dpdm_switch virq register error, rc=%d\n", rc);
		return;
	}
	chip->dpdm_switch = ic;

	name = of_get_oplus_chg_ic_name(chip->dev->of_node, "oplus,pps_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_pps_ic_reg_callback, chip);
}

static int oplus_pps_cp_virq_reg(struct oplus_pps *chip)
{
	return 0;
}

static void oplus_pps_cp_virq_unreg(struct oplus_pps *chip)
{}

static void oplus_pps_cp_ic_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_pps *chip;
	const char *name;
	int rc;

	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	chip = data;

	rc = oplus_pps_cp_virq_reg(chip);
	if (rc < 0) {
		chg_err("cp virq register error, rc=%d\n", rc);
		return;
	}
	chip->cp_ic = ic;

	name = of_get_oplus_chg_ic_name(chip->dev->of_node, "oplus,dpdm_switch_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_pps_dpdm_switch_reg_callback, chip);
}

static int oplus_pps_curr_vote_callback(struct votable *votable, void *data,
					 int curr_ma, const char *client,
					 bool step)
{
	struct oplus_pps *chip = data;

	if (curr_ma < 0)
		return -EINVAL;

	chip->target_curr_ma = curr_ma;
	chg_info("%s set pps target current to %d mA\n", client, curr_ma);

	return 0;
}

static int oplus_pps_disable_vote_callback(struct votable *votable, void *data,
					    int disable, const char *client,
					    bool step)
{
	struct oplus_pps *chip = data;

	if (disable < 0)
		chip->pps_disable = false;
	else
		chip->pps_disable = !!disable;
	chg_info("%s set pps disable to %s\n", client, chip->pps_disable ? "true" : "false");

	return 0;
}

static int oplus_pps_not_allow_vote_callback(struct votable *votable, void *data,
					      int not_allow, const char *client,
					      bool step)
{
	struct oplus_pps *chip = data;
	union mms_msg_data mms_data = { 0 };
	int rc;

	if (not_allow < 0)
		chip->pps_not_allow = false;
	else
		chip->pps_not_allow = !!not_allow;
	chg_info("%s set pps not allow to %s\n", client, chip->pps_not_allow ? "true" : "false");

	if (!chip->pps_not_allow) {
		rc = oplus_mms_get_item_data(chip->cpa_topic, CPA_ITEM_ALLOW, &mms_data, true);
		if (rc < 0)
			return 0;
		if (mms_data.intval == CHG_PROTOCOL_PPS)
			schedule_delayed_work(&chip->switch_check_work, 0);
	}

	return 0;
}

static int oplus_pps_boot_vote_callback(struct votable *votable, void *data,
					 int boot, const char *client,
					 bool step)
{
	struct oplus_pps *chip = data;

	chg_info("pps_boot_vote change to %s by %s\n", (!!boot) ? "true" : "false", client);
	/* The PPS module startup has not been completed and will not be processed temporarily */
	if (!!boot)
		return 0;

	oplus_cpa_protocol_ready(chip->cpa_topic, CHG_PROTOCOL_PPS);
	return 0;
}

static int oplus_pps_parse_charge_strategy(struct oplus_pps *chip)
{
	int rc;
	int length = 0;
	int soc_tmp[7] = { 0, 15, 30, 50, 75, 85, 95 };
	int rang_temp_tmp[7] = { 0, 50, 120, 200, 350, 440, 510 };
	int high_temp_tmp[6] = { 425, 430, 435, 400, 415, 420 };
	int high_curr_tmp[6] = { 4000, 3000, 2000, 3000, 4000, 4000 };
	int low_curr_temp_tmp[4] = { 120, 200, 300, 440 };
	struct device_node *node = chip->dev->of_node;

	rc = of_property_read_u32(node, "oplus,pps_warm_allow_vol",
				  &chip->limits.pps_warm_allow_vol);
	if (rc)
		chip->limits.pps_warm_allow_vol = 4000;
	else
		chg_info("oplus,pps_warm_allow_vol is %d\n",
			 chip->limits.pps_warm_allow_vol);

	rc = of_property_read_u32(node, "oplus,pps_warm_allow_soc",
				  &chip->limits.pps_warm_allow_soc);
	if (rc)
		chip->limits.pps_warm_allow_soc = 50;
	else
		chg_info("oplus,pps_warm_allow_soc is %d\n",
			 chip->limits.pps_warm_allow_soc);

	rc = of_property_read_u32(node, "oplus,pps_strategy_normal_current",
				  &chip->limits.pps_strategy_normal_current);
	if (rc)
		chip->limits.pps_strategy_normal_current = 6000;
	else
		chg_info("oplus,pps_strategy_normal_current is %d\n",
			 chip->limits.pps_strategy_normal_current);

	rc = of_property_read_u32(node, "oplus,pps_over_high_or_low_current",
				  &chip->limits.pps_over_high_or_low_current);
	if (rc)
		chip->limits.pps_over_high_or_low_current = -EINVAL;
	else
		chg_info("oplus,pps_over_high_or_low_current is %d\n",
			 chip->limits.pps_over_high_or_low_current);

	rc = of_property_read_u32(node, "oplus,pps_full_cool_sw_vbat",
				  &chip->limits.pps_full_cool_sw_vbat);
	if (rc)
		chip->limits.pps_full_cool_sw_vbat = 4430;
	else
		chg_info("oplus,pps_full_cool_sw_vbat is %d\n",
			 chip->limits.pps_full_cool_sw_vbat);

	rc = of_property_read_u32(node, "oplus,pps_full_normal_sw_vbat",
				  &chip->limits.pps_full_normal_sw_vbat);
	if (rc)
		chip->limits.pps_full_normal_sw_vbat = 4495;
	else
		chg_info("oplus,pps_full_normal_sw_vbat %d\n",
			 chip->limits.pps_full_normal_sw_vbat);

	rc = of_property_read_u32(node, "oplus,pps_full_normal_hw_vbat",
				  &chip->limits.pps_full_normal_hw_vbat);
	if (rc)
		chip->limits.pps_full_normal_hw_vbat = 4500;
	else
		chg_info("oplus,pps_full_normal_hw_vbat is %d\n",
			 chip->limits.pps_full_normal_hw_vbat);

	rc = of_property_read_u32(node, "oplus,pps_full_cool_sw_vbat_third",
				  &chip->limits.pps_full_cool_sw_vbat_third);
	if (rc)
		chip->limits.pps_full_cool_sw_vbat_third = 4430;
	else
		chg_info("oplus,pps_full_cool_sw_vbat_third is %d\n",
			 chip->limits.pps_full_cool_sw_vbat_third);

	rc = of_property_read_u32(node, "oplus,pps_full_normal_sw_vbat_third",
				  &chip->limits.pps_full_normal_sw_vbat_third);
	if (rc)
		chip->limits.pps_full_normal_sw_vbat_third = 4430;
	else
		chg_info("oplus,pps_full_normal_sw_vbat_third is %d\n",
			 chip->limits.pps_full_normal_sw_vbat_third);

	rc = of_property_read_u32(node, "oplus,pps_full_normal_hw_vbat_third",
				  &chip->limits.pps_full_normal_hw_vbat_third);
	if (rc)
		chip->limits.pps_full_normal_hw_vbat_third = 4440;
	else
		chg_info("oplus,pps_full_normal_hw_vbat_third is %d\n",
			 chip->limits.pps_full_normal_hw_vbat_third);

	rc = of_property_read_u32(node, "oplus,pps_full_ffc_vbat",
				  &chip->limits.pps_full_ffc_vbat);
	if (rc)
		chip->limits.pps_full_ffc_vbat = 4420;
	else
		chg_info("oplus,pps_full_ffc_vbat is %d\n",
			 chip->limits.pps_full_ffc_vbat);

	rc = of_property_read_u32(node, "oplus,pps_full_warm_vbat",
				  &chip->limits.pps_full_warm_vbat);
	if (rc)
		chip->limits.pps_full_warm_vbat = 4150;
	else
		chg_info("oplus,pps_full_warm_vbat is %d\n",
			 chip->limits.pps_full_warm_vbat);

	rc = of_property_read_u32(node, "oplus,pps_timeout_third",
				  &chip->limits.pps_timeout_third);
	if (rc)
		chip->limits.pps_timeout_third = 9000;
	else
		chg_info("oplus,pps_timeout_third is %d\n",
			 chip->limits.pps_timeout_third);

	rc = of_property_read_u32(node, "oplus,pps_timeout_oplus",
				  &chip->limits.pps_timeout_oplus);
	if (rc)
		chip->limits.pps_timeout_oplus = 7200;
	else
		chg_info("oplus,pps_timeout_oplus is %d\n",
			 chip->limits.pps_timeout_oplus);

	rc = of_property_read_u32(node, "oplus,pps_ibat_over_third",
				  &chip->limits.pps_ibat_over_third);
	if (rc)
		chip->limits.pps_ibat_over_third = 4000;
	else
		chg_info("oplus,pps_ibat_over_third is %d\n",
			 chip->limits.pps_ibat_over_third);

	rc = of_property_read_u32(node, "oplus,pps_ibat_over_oplus",
				  &chip->limits.pps_ibat_over_oplus);
	if (rc)
		chip->limits.pps_ibat_over_oplus = 17000;
	else
		chg_info("oplus,pps_ibat_over_oplus is %d\n",
			 chip->limits.pps_ibat_over_oplus);

	rc = of_property_read_u32(node, "oplus,pps_current_change_timeout",
				  &chip->pps_current_change_timeout);
	if (rc)
		chip->pps_current_change_timeout = 100;
	chg_info("pps_current_change_timeout is %d\n",
		 chip->pps_current_change_timeout);

	rc = of_property_read_u32(node, "oplus,pps_bcc_max_curr",
				  &chip->bcc_max_curr);
	if (rc)
		chip->bcc_max_curr = 15000;
	else
		chg_info("oplus,pps_bcc_max_curr is %d\n", chip->bcc_max_curr);

	rc = of_property_read_u32(node, "oplus,pps_bcc_min_curr",
				  &chip->bcc_min_curr);
	if (rc)
		chip->bcc_min_curr = 1000;
	else
		chg_info("oplus,pps_bcc_min_curr is %d\n", chip->bcc_min_curr);

	rc = of_property_read_u32(node, "oplus,pps_bcc_exit_curr",
				  &chip->bcc_exit_curr);
	if (rc)
		chip->bcc_exit_curr = 1000;
	else
		chg_info("oplus,pps_bcc_exit_curr is %d\n",
			 chip->bcc_exit_curr);

	rc = of_property_count_elems_of_size(
		node, "oplus,pps_strategy_batt_high_temp", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 6)
			length = 6;
		rc = of_property_read_u32_array(node, "oplus,pps_strategy_batt_high_temp",
						(u32 *)high_temp_tmp, length);
		if (rc < 0)
			chg_err("parse pps_strategy_batt_high_temp failed, "
				 "rc=%d\n", rc);
	} else {
		length = 6;
		chg_err("parse pps_strategy_batt_high_temp error, rc=%d\n", rc);
	}

	chip->limits.pps_strategy_batt_high_temp0 = high_temp_tmp[0];
	chip->limits.pps_strategy_batt_high_temp1 = high_temp_tmp[1];
	chip->limits.pps_strategy_batt_high_temp2 = high_temp_tmp[2];
	chip->limits.pps_strategy_batt_low_temp0 = high_temp_tmp[3];
	chip->limits.pps_strategy_batt_low_temp1 = high_temp_tmp[4];
	chip->limits.pps_strategy_batt_low_temp2 = high_temp_tmp[5];
	chg_info("length = %d, high_temp[%d, %d, %d, %d, %d, %d]\n", length,
		 high_temp_tmp[0], high_temp_tmp[1], high_temp_tmp[2],
		 high_temp_tmp[3], high_temp_tmp[4], high_temp_tmp[5]);

	rc = of_property_count_elems_of_size(
		node, "oplus,pps_strategy_high_current", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 6)
			length = 6;
		rc = of_property_read_u32_array(node, "oplus,pps_strategy_high_current",
						(u32 *)high_curr_tmp, length);
		if (rc < 0) {
			chg_err("parse pps_strategy_high_current failed, "
				 "rc=%d\n", rc);
		}
	} else {
		length = 6;
		chg_err("parse pps_strategy_high_current error, rc=%d\n", rc);
	}
	chip->limits.pps_strategy_high_current0 = high_curr_tmp[0];
	chip->limits.pps_strategy_high_current1 = high_curr_tmp[1];
	chip->limits.pps_strategy_high_current2 = high_curr_tmp[2];
	chip->limits.pps_strategy_low_current0 = high_curr_tmp[3];
	chip->limits.pps_strategy_low_current1 = high_curr_tmp[4];
	chip->limits.pps_strategy_low_current2 = high_curr_tmp[5];
	chg_info("length = %d, high_current[%d, %d, %d, %d, %d, %d]\n", length,
		 high_curr_tmp[0], high_curr_tmp[1], high_curr_tmp[2],
		 high_curr_tmp[3], high_curr_tmp[4], high_curr_tmp[5]);

	rc = of_property_count_elems_of_size(
		node, "oplus,pps_charge_strategy_temp", sizeof(u32));
	if (rc >= 0) {
		chip->limits.pps_strategy_temp_num = rc;
		if (chip->limits.pps_strategy_temp_num > 7)
			chip->limits.pps_strategy_temp_num = 7;
		rc = of_property_read_u32_array(
			node, "oplus,pps_charge_strategy_temp",
			(u32 *)rang_temp_tmp,
			chip->limits.pps_strategy_temp_num);
		if (rc < 0)
			chg_err("parse pps_charge_strategy_temp failed, "
				"rc=%d\n", rc);
	} else {
		chip->limits.pps_strategy_temp_num = 7;
		chg_err("parse pps_charge_strategy_temp error, rc=%d\n", rc);
	}
	chip->limits.pps_batt_over_low_temp = rang_temp_tmp[0];
	chip->limits.pps_little_cold_temp = rang_temp_tmp[1];
	chip->limits.pps_cool_temp = rang_temp_tmp[2];
	chip->limits.pps_little_cool_temp = rang_temp_tmp[3];
	chip->limits.pps_normal_low_temp = rang_temp_tmp[4];
	chip->limits.pps_normal_high_temp = rang_temp_tmp[5];
	chip->limits.pps_batt_over_high_temp = rang_temp_tmp[6];
	chg_info("pps_charge_strategy_temp num = %d, [%d, %d, %d, %d, %d, "
		 "%d, %d]\n",
		 chip->limits.pps_strategy_temp_num, rang_temp_tmp[0],
		 rang_temp_tmp[1], rang_temp_tmp[2], rang_temp_tmp[3],
		 rang_temp_tmp[4], rang_temp_tmp[5], rang_temp_tmp[6]);
	chip->limits.default_pps_normal_high_temp =
		chip->limits.pps_normal_high_temp;
	chip->limits.default_pps_normal_low_temp =
		chip->limits.pps_normal_low_temp;
	chip->limits.default_pps_little_cool_temp =
		chip->limits.pps_little_cool_temp;
	chip->limits.default_pps_cool_temp = chip->limits.pps_cool_temp;
	chip->limits.default_pps_little_cold_temp =
		chip->limits.pps_little_cold_temp;

	rc = of_property_count_elems_of_size(
		node, "oplus,pps_charge_strategy_soc", sizeof(u32));
	if (rc >= 0) {
		chip->limits.pps_strategy_soc_num = rc;
		if (chip->limits.pps_strategy_soc_num > 7)
			chip->limits.pps_strategy_soc_num = 7;
		rc = of_property_read_u32_array(
			node, "oplus,pps_charge_strategy_soc", (u32 *)soc_tmp,
			chip->limits.pps_strategy_soc_num);
		if (rc < 0)
			chg_err("parse pps_charge_strategy_soc failed, "
				"rc=%d\n", rc);
	} else {
		chip->limits.pps_strategy_soc_num = 7;
		chg_err("parse pps_charge_strategy_soc error, rc=%d\n", rc);
	}

	chip->limits.pps_strategy_soc_over_low = soc_tmp[0];
	chip->limits.pps_strategy_soc_min = soc_tmp[1];
	chip->limits.pps_strategy_soc_low = soc_tmp[2];
	chip->limits.pps_strategy_soc_mid_low = soc_tmp[3];
	chip->limits.pps_strategy_soc_mid = soc_tmp[4];
	chip->limits.pps_strategy_soc_mid_high = soc_tmp[5];
	chip->limits.pps_strategy_soc_high = soc_tmp[6];
	chg_info("pps_charge_strategy_soc num = %d, [%d, %d, %d, %d, %d, %d, "
		 "%d]\n",
		 chip->limits.pps_strategy_soc_num, soc_tmp[0], soc_tmp[1],
		 soc_tmp[2], soc_tmp[3], soc_tmp[4], soc_tmp[5], soc_tmp[6]);

	rc = of_property_count_elems_of_size(
		node, "oplus,pps_low_curr_full_strategy_temp", sizeof(u32));
	if (rc >= 0) {
		length = rc;
		if (length > 4)
			length = 4;
		rc = of_property_read_u32_array(
			node, "oplus,pps_low_curr_full_strategy_temp",
			(u32 *)low_curr_temp_tmp, length);
		if (rc < 0)
			chg_err("parse pps_low_curr_full_strategy_temp "
				"failed, rc=%d\n", rc);
	} else {
		length = 4;
		chg_err("parse pps_low_curr_full_strategy_temp error, rc=%d\n", rc);
	}

	chip->limits.pps_low_curr_full_cool_temp = low_curr_temp_tmp[0];
	chip->limits.pps_low_curr_full_little_cool_temp = low_curr_temp_tmp[1];
	chip->limits.pps_low_curr_full_normal_low_temp = low_curr_temp_tmp[2];
	chip->limits.pps_low_curr_full_normal_high_temp = low_curr_temp_tmp[3];
	chg_info("length = %d, low_curr_temp[%d, %d, %d, %d]\n", length,
		 low_curr_temp_tmp[0], low_curr_temp_tmp[1],
		 low_curr_temp_tmp[2], low_curr_temp_tmp[3]);

	return rc;
}

static int oplus_pps_parse_dt(struct oplus_pps *chip)
{
	struct device_node *node = chip->dev->of_node;
	struct oplus_pps_config *config = &chip->config;
	int rc;

	rc = of_property_read_u32(node, "oplus,target_vbus_mv",
				  &config->target_vbus_mv);
	if (rc < 0) {
		chg_err("get oplus,target_vbus_mv property error, rc=%d\n",
			rc);
		config->target_vbus_mv = 11000;
	}

	rc = of_property_read_u32(node, "oplus,curr_max_ma",
				  &config->curr_max_ma);
	if (rc < 0) {
		chg_err("get oplus,curr_max_ma property error, rc=%d\n",
			rc);
		config->curr_max_ma = 3000;
	}

	(void)oplus_pps_parse_charge_strategy(chip);

	return 0;
}

static int oplus_pps_vote_init(struct oplus_pps *chip)
{
	int rc;

	chip->pps_curr_votable = create_votable("PPS_CURR", VOTE_MIN,
					   oplus_pps_curr_vote_callback, chip);
	if (IS_ERR(chip->pps_curr_votable)) {
		rc = PTR_ERR(chip->pps_curr_votable);
		chg_err("creat pps_curr_votable error, rc=%d\n", rc);
		chip->pps_curr_votable = NULL;
		return rc;
	}
	vote(chip->pps_curr_votable, MAX_VOTER, true, chip->config.curr_max_ma, false);

	chip->pps_disable_votable =
		create_votable("PPS_DISABLE", VOTE_SET_ANY, oplus_pps_disable_vote_callback, chip);
	if (IS_ERR(chip->pps_disable_votable)) {
		rc = PTR_ERR(chip->pps_disable_votable);
		chg_err("creat pps_disable_votable error, rc=%d\n", rc);
		chip->pps_disable_votable = NULL;
		goto creat_disable_votable_err;
	}

	chip->pps_not_allow_votable =
		create_votable("PPS_NOT_ALLOW", VOTE_MIN, oplus_pps_not_allow_vote_callback, chip);
	if (IS_ERR(chip->pps_not_allow_votable)) {
		rc = PTR_ERR(chip->pps_not_allow_votable);
		chg_err("creat pps_not_allow_votable error, rc=%d\n", rc);
		chip->pps_not_allow_votable = NULL;
		goto creat_not_allow_votable_err;
	}

	chip->pps_boot_votable =
		create_votable("PPS_BOOT", VOTE_SET_ANY, oplus_pps_boot_vote_callback, chip);
	if (IS_ERR(chip->pps_boot_votable)) {
		rc = PTR_ERR(chip->pps_boot_votable);
		chg_err("creat pps_boot_votable error, rc=%d\n", rc);
		chip->pps_boot_votable = NULL;
		goto creat_boot_votable_err;
	}
	vote(chip->pps_boot_votable, SHELL_TEMP_VOTER, true, 1, false);
	vote(chip->pps_boot_votable, COMM_TOPIC_VOTER, true, 1, false);
	vote(chip->pps_boot_votable, WIRED_TOPIC_VOTER, true, 1, false);
	vote(chip->pps_boot_votable, GAUGE_TOPIC_VOTER, true, 1, false);
	vote(chip->pps_boot_votable, CPA_TOPIC_VOTER, true, 1, false);
	chip->shell_temp_ready = false;

	return 0;

creat_boot_votable_err:
	destroy_votable(chip->pps_not_allow_votable);
creat_not_allow_votable_err:
	destroy_votable(chip->pps_disable_votable);
creat_disable_votable_err:
	destroy_votable(chip->pps_curr_votable);
	return rc;
}

static int oplus_pps_get_input_node_impedance(void *data)
{
	struct oplus_pps *chip;
	int vac, iin, vchg;
	int r_mohm;
	int rc;

	if (data == NULL)
		return -EINVAL;
	chip = data;

	if (!chip->pps_charging) {
		chg_err("pps_charging is false, can't read input node impedance\n");
		return -EINVAL;
	}

	rc = oplus_pps_cp_get_vac(chip, &vac);
	if (rc < 0) {
		chg_err("can't get cp vac, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_pps_cp_get_iin(chip, &iin);
	if (rc < 0) {
		chg_err("can't get cp iin, rc=%d\n", rc);
		return rc;
	}

	rc = oplus_pps_get_pps_status_info(chip, &chip->pps_status_info);
	if (rc < 0) {
		chg_err("pps get src info error\n");
		return rc;
	}
	vchg = PPS_STATUS_VOLT(chip->pps_status_info);

	r_mohm = (vchg - vac) * 1000 / iin;
	if (r_mohm < 0) {
		chg_err("pps_input_node: r_mohm=%d\n", r_mohm);
		r_mohm = 0;
	}

	return r_mohm;
}

static int oplus_pps_init_imp_node(struct oplus_pps *chip)
{
	struct device_node *imp_node;
	struct device_node *child;
	const char *name;
	int rc;

	imp_node = of_get_child_by_name(chip->dev->of_node, "oplus,impedance_node");
	if (imp_node == NULL)
		return 0;

	for_each_child_of_node(imp_node, child) {
		rc = of_property_read_string(child, "node_name", &name);
		if (rc < 0) {
			chg_err("can't read %s node_name, rc=%d\n", child->name, rc);
			continue;
		}
		if (!strcmp(name, "pps_input")) {
			chip->input_imp_node =
				oplus_imp_node_register(child, chip->dev, chip, oplus_pps_get_input_node_impedance);
			if (IS_ERR_OR_NULL(chip->input_imp_node)) {
				chg_err("%s register error, rc=%ld\n", child->name, PTR_ERR(chip->input_imp_node));
				chip->input_imp_node = NULL;
				continue;
			}
			oplus_imp_node_set_active(chip->input_imp_node, true);
		} else {
			chg_err("unknown node_name: %s\n", name);
		}
	}

	return 0;
}

static void oplus_pps_imp_uint_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_pps *chip =
		container_of(dwork, struct oplus_pps, imp_uint_init_work);
	static int retry_count = 0;
#define IMP_UINT_INIT_RETRY_MAX	500

	if (!of_property_read_bool(chip->dev->of_node, "oplus,impedance_unit")) {
		chg_err("oplus,impedance_unit not found\n");
		return;
	}

	chip->imp_uint = of_find_imp_uint(chip->dev->of_node, "oplus,impedance_unit");
	if (chip->imp_uint != NULL) {
		vote(chip->pps_not_allow_votable, IMP_VOTER, false, 0, false);
		return;
	}

	retry_count++;
	if (retry_count > IMP_UINT_INIT_RETRY_MAX) {
		chg_err("can't get pps imp uint\n");
		return;
	}

	/* Try once every 100ms, accumulatively try 50s */
	schedule_delayed_work(&chip->imp_uint_init_work, msecs_to_jiffies(100));
}

static int oplus_pps_dev_open(struct inode *inode, struct file *filp)
{
	struct oplus_pps *chip = container_of(filp->private_data,
		struct oplus_pps, misc_dev);

	filp->private_data = chip;
	chg_info("pps_dev: %d-%d\n", imajor(inode), iminor(inode));
	return 0;
}

static ssize_t oplus_pps_dev_read(struct file *filp, char __user *buf,
				   size_t count, loff_t *offset)
{
	struct oplus_pps *chip = filp->private_data;
	struct pps_dev_cmd cmd;
	int rc;

	mutex_lock(&chip->read_lock);
	rc = wait_event_interruptible(chip->read_wq, chip->cmd_data_ok);
	mutex_unlock(&chip->read_lock);
	if (rc)
		return rc;
	if (!chip->cmd_data_ok)
		chg_err("pps false wakeup, rc=%d\n", rc);

	mutex_lock(&chip->cmd_data_lock);
	chip->cmd_data_ok = false;
	memcpy(&cmd, &chip->cmd, sizeof(struct pps_dev_cmd));
	mutex_unlock(&chip->cmd_data_lock);
	if (copy_to_user(buf, &cmd, sizeof(struct pps_dev_cmd))) {
		chg_err("failed to copy to user space\n");
		return -EFAULT;
	}

	return sizeof(struct pps_dev_cmd);
}

#define PPS_IOC_MAGIC			0x66
#define PPS_NOTIFY_GET_AUTH_DATA	_IOW(PPS_IOC_MAGIC, 1, char)

static long oplus_pps_dev_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct oplus_pps *chip = filp->private_data;

	switch (cmd) {
	case PPS_NOTIFY_GET_AUTH_DATA:
		complete(&chip->cmd_ack);
		break;
		break;
	default:
		chg_err("bad ioctl %u\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t oplus_pps_dev_write(struct file *filp, const char __user *buf,
				    size_t count, loff_t *offset)
{
	return count;
}

static const struct file_operations oplus_pps_dev_fops = {
	.owner			= THIS_MODULE,
	.llseek			= no_llseek,
	.write			= oplus_pps_dev_write,
	.read			= oplus_pps_dev_read,
	.open			= oplus_pps_dev_open,
	.unlocked_ioctl		= oplus_pps_dev_ioctl,
};

static int oplus_pps_misc_dev_reg(struct oplus_pps *chip)
{
	int rc;

	mutex_init(&chip->read_lock);
	mutex_init(&chip->cmd_data_lock);
	init_waitqueue_head(&chip->read_wq);
	chip->cmd_data_ok = false;
	init_completion(&chip->cmd_ack);

	chip->misc_dev.minor = MISC_DYNAMIC_MINOR;
	chip->misc_dev.name = "pps_dev";
	chip->misc_dev.fops = &oplus_pps_dev_fops;
	rc = misc_register(&chip->misc_dev);
	if (rc)
		chg_err("misc_register failed, rc=%d\n", rc);

	return rc;
}

static int oplus_pps_probe(struct platform_device *pdev)
{
	struct oplus_pps *chip;
	const char *name;
	struct device_node *startegy_node;
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_pps), GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc oplus_pps struct buffer error\n");
		return 0;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	oplus_pps_parse_dt(chip);
	INIT_DELAYED_WORK(&chip->switch_check_work, oplus_pps_switch_check_work);
	INIT_DELAYED_WORK(&chip->monitor_work, oplus_pps_monitor_work);
	INIT_DELAYED_WORK(&chip->current_work, oplus_pps_current_work);
	INIT_DELAYED_WORK(&chip->imp_uint_init_work, oplus_pps_imp_uint_init_work);
	INIT_WORK(&chip->wired_online_work, oplus_pps_wired_online_work);
	INIT_WORK(&chip->force_exit_work, oplus_pps_force_exit_work);
	INIT_WORK(&chip->soft_exit_work, oplus_pps_soft_exit_work);
	INIT_WORK(&chip->gauge_update_work, oplus_pps_gauge_update_work);

	rc = oplus_pps_init_imp_node(chip);
	if (rc < 0)
		goto imp_node_init_err;

	rc = oplus_pps_vote_init(chip);
	if (rc < 0)
		goto vote_init_err;

	rc = oplus_pps_misc_dev_reg(chip);
	if (rc < 0)
		goto misc_dev_reg_err;

	startegy_node = of_get_child_by_name(pdev->dev.of_node, "pps_charge_third_strategy");
	if (startegy_node == NULL) {
		chg_err("pps_charge_third_strategy not found\n");
		rc = -ENODEV;
		goto third_startegy_err;
	}
	chip->third_curve_strategy = oplus_chg_strategy_alloc_by_node("pps_ufcs_curve", startegy_node);
	if (IS_ERR_OR_NULL(chip->third_curve_strategy)) {
		chg_err("alloc third curve startegy error, rc=%ld", PTR_ERR(chip->third_curve_strategy));
		chip->third_curve_strategy = NULL;
		rc = -EFAULT;
		goto third_startegy_err;
	}
	startegy_node = of_get_child_by_name(pdev->dev.of_node, "pps_charge_oplus_strategy");
	if (startegy_node == NULL) {
		chg_err("pps_charge_oplus_strategy not found\n");
		rc = -ENODEV;
		goto oplus_startegy_err;
	}
	chip->oplus_curve_strategy = oplus_chg_strategy_alloc_by_node("pps_ufcs_curve", startegy_node);
	if (IS_ERR_OR_NULL(chip->oplus_curve_strategy)) {
		chg_err("alloc oplus curve startegy error, rc=%ld", PTR_ERR(chip->oplus_curve_strategy));
		chip->oplus_curve_strategy = NULL;
		rc = -EFAULT;
		goto oplus_startegy_err;
	}

	name = of_get_oplus_chg_ic_name(pdev->dev.of_node, "oplus,cp_ic", 0);
	oplus_chg_ic_wait_ic(name, oplus_pps_cp_ic_reg_callback, chip);
	if (of_property_read_bool(chip->dev->of_node, "oplus,impedance_unit")) {
		vote(chip->pps_not_allow_votable, IMP_VOTER, true, 1, false);
		schedule_delayed_work(&chip->imp_uint_init_work, 0);
	} else {
		chg_err("oplus,impedance_unit not found\n");
	}

	return 0;

oplus_startegy_err:
	if (chip->third_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->third_curve_strategy);
third_startegy_err:
	misc_deregister(&chip->misc_dev);
misc_dev_reg_err:
	if (chip->pps_boot_votable != NULL)
		destroy_votable(chip->pps_boot_votable);
	if (chip->pps_not_allow_votable != NULL)
		destroy_votable(chip->pps_not_allow_votable);
	if (chip->pps_disable_votable != NULL)
		destroy_votable(chip->pps_disable_votable);
	if (chip->pps_curr_votable != NULL)
		destroy_votable(chip->pps_curr_votable);
vote_init_err:
	if (chip->input_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->input_imp_node);
imp_node_init_err:
	devm_kfree(&pdev->dev, chip);
	return rc;
}

static int oplus_pps_remove(struct platform_device *pdev)
{
	struct oplus_pps *chip = platform_get_drvdata(pdev);

	if (chip->pps_ic)
		oplus_pps_virq_unreg(chip);
	if (chip->dpdm_switch)
		oplus_pps_dpdm_switch_virq_unreg(chip);
	if (chip->cp_ic)
		oplus_pps_cp_virq_unreg(chip);
	if (!IS_ERR_OR_NULL(chip->cpa_subs))
		oplus_mms_unsubscribe(chip->cpa_subs);
	if (!IS_ERR_OR_NULL(chip->wired_subs))
		oplus_mms_unsubscribe(chip->wired_subs);
	if (!IS_ERR_OR_NULL(chip->comm_subs))
		oplus_mms_unsubscribe(chip->comm_subs);
	if (chip->oplus_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->oplus_curve_strategy);
	if (chip->third_curve_strategy != NULL)
		oplus_chg_strategy_release(chip->third_curve_strategy);
	misc_deregister(&chip->misc_dev);
	if (chip->pps_boot_votable != NULL)
		destroy_votable(chip->pps_boot_votable);
	if (chip->pps_not_allow_votable != NULL)
		destroy_votable(chip->pps_not_allow_votable);
	if (chip->pps_disable_votable != NULL)
		destroy_votable(chip->pps_disable_votable);
	if (chip->pps_curr_votable != NULL)
		destroy_votable(chip->pps_curr_votable);
	if (chip->input_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->input_imp_node);
	devm_kfree(&pdev->dev, chip);

	return 0;
}

static const struct of_device_id oplus_pps_match[] = {
	{ .compatible = "oplus,pps_charge" },
	{},
};

static struct platform_driver oplus_pps_driver = {
	.driver		= {
		.name = "oplus-pps_charge",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_pps_match),
	},
	.probe		= oplus_pps_probe,
	.remove		= oplus_pps_remove,
};

static __init int oplus_pps_init(void)
{
	return platform_driver_register(&oplus_pps_driver);
}

static __exit void oplus_pps_exit(void)
{
	platform_driver_unregister(&oplus_pps_driver);
}

oplus_chg_module_register(oplus_pps);
int oplus_pps_current_to_level(int curr)
{
	int level = 0;

	if (curr <= 0)
		return level;
	level = pps_find_level_to_current(curr, g_pps_current_table, ARRAY_SIZE(g_pps_current_table));
	return level;
}

enum fastchg_protocol_type oplus_pps_adapter_id_to_protocol_type(u32 id)
{
	switch (id) {
	case PPS_FASTCHG_TYPE_V1:
	case PPS_FASTCHG_TYPE_V2:
	case PPS_FASTCHG_TYPE_V3:
		return PROTOCOL_CHARGING_PPS_OPLUS;
	case PPS_FASTCHG_TYPE_THIRD:
		return PROTOCOL_CHARGING_PPS_THIRD;
	default:
		return PROTOCOL_CHARGING_UNKNOWN;
	}
}

int oplus_pps_adapter_id_to_power(u32 id)
{
	switch (id) {
	case PPS_FASTCHG_TYPE_V1:
		return PPS_POWER_TYPE_V1;
	case PPS_FASTCHG_TYPE_V2:
		return PPS_POWER_TYPE_V2;
	case PPS_FASTCHG_TYPE_V3:
		return PPS_POWER_TYPE_V3;
	case PPS_FASTCHG_TYPE_THIRD:
		return PPS_POWER_TYPE_THIRD;
	default:
		return PPS_POWER_TYPE_UNKOWN;
	}
}
