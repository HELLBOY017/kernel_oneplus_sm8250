#define pr_fmt(fmt) "[MMS_GAUGE]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/iio/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/power_supply.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#endif
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_voter.h>
#include <oplus_mms.h>
#include <oplus_chg_monitor.h>
#include <oplus_mms_wired.h>
#include <oplus_mms_gauge.h>
#include <oplus_chg_vooc.h>
#include <oplus_parallel.h>
#include <oplus_chg_wls.h>

#define GAUGE_IC_NUM_MAX 2
#define ERR_COUNT_MAX 3
#define GAUGE_PARALLEL_IC_NUM_MIN 2
#define GAUGE_DEFAULT_VOLT_MV		3800
#define DEFAULT_SOC 50

struct oplus_virtual_gauge_child {
	struct oplus_chg_ic_dev *ic_dev;
	int index;
	int capacity_ratio;
	enum oplus_chg_ic_func *funcs;
	int func_num;
	enum oplus_chg_ic_virq_id *virqs;
	int virq_num;
};

struct oplus_mms_gauge {
	struct device *dev;
	struct oplus_chg_ic_dev *gauge_ic;
	struct oplus_chg_ic_dev *gauge_ic_parallel[GAUGE_IC_NUM_MAX];
	struct oplus_chg_ic_dev *voocphy_ic;
	struct oplus_mms *gauge_topic;
	struct oplus_mms *gauge_topic_parallel[GAUGE_IC_NUM_MAX];
	struct oplus_mms *comm_topic;
	struct oplus_mms *wired_topic;
	struct oplus_mms *vooc_topic;
	struct oplus_mms *err_topic;
	struct oplus_mms *parallel_topic;
	struct oplus_mms *wls_topic;
	struct mms_subscribe *comm_subs;
	struct mms_subscribe *wired_subs;
	struct mms_subscribe *gauge_subs;
	struct mms_subscribe *vooc_subs;
	struct mms_subscribe *parallel_subs;
	struct mms_subscribe *wls_subs;

	struct delayed_work hal_gauge_init_work;
	struct work_struct err_handler_work;
	struct work_struct online_handler_work;
	struct work_struct offline_handler_work;
	struct work_struct resume_handler_work;
	struct work_struct update_change_work;
	struct work_struct gauge_update_work;
	struct work_struct gauge_set_curve_work;
	struct work_struct set_gauge_batt_full_work;
	struct delayed_work subboard_ntc_err_work;

	struct votable *gauge_update_votable;

	int device_type;
	int device_type_for_vooc;
	unsigned int vooc_sid;
	unsigned int err_code;
	int check_batt_vol_count;
	bool pd_svooc;
	bool bat_volt_different;

	bool factory_test_mode;
	bool wired_online;
	bool wls_online;
	bool hmac;
	bool support_subboard_ntc;
	bool check_subboard_ntc_err;
	bool batt_full;
	int batt_temp_region;
	int child_num;
	struct oplus_virtual_gauge_child *child_list;
	int main_gauge;
	int sub_gauge;
};

static struct oplus_mms_gauge *g_mms_gauge;

static int gauge_dbg_tbat = 0;
module_param(gauge_dbg_tbat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_tbat, "debug battery temperature");

static int gauge_dbg_vbat = 0;
module_param(gauge_dbg_vbat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_vbat, "debug battery voltage");

static int gauge_dbg_ibat = 0;
module_param(gauge_dbg_ibat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_ibat, "debug battery current");

__maybe_unused static bool
is_err_topic_available(struct oplus_mms_gauge *chip)
{
	if (!chip->err_topic)
		chip->err_topic = oplus_mms_get_by_name("error");
	return !!chip->err_topic;
}

static bool is_support_parallel(struct oplus_mms_gauge *chip)
{
	if (chip == NULL) {
		chg_err("chip is NULL\n");
		return false;
	}

	if (chip->child_num >= GAUGE_PARALLEL_IC_NUM_MIN)
		return true;
	else
		return false;
}

bool is_support_parallel_battery(struct oplus_mms *topic)
{
	struct oplus_mms_gauge *chip;

	if (topic == NULL) {
		chg_err("topic is NULL\n");
		return 0;
	}
	chip = oplus_mms_get_drvdata(topic);
	return is_support_parallel(chip);
}

int oplus_gauge_get_batt_mvolts(void)
{
	int rc;
	int vol_mv;

	if (!g_mms_gauge)
		return 3800;

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		return gauge_dbg_vbat;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX, &vol_mv);
	if (rc < 0) {
		chg_err("get battery voltage error, rc=%d\n", rc);
		return 3800;
	}

	return vol_mv;
}

int oplus_gauge_get_batt_fc(void)
{
	int rc;
	int fc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_FC, &fc);
	if (rc < 0) {
		chg_err("get battery fc error, rc=%d\n", rc);
		return 0;
	}

	return fc;
}

int oplus_gauge_get_batt_qm(void)
{
	int rc;
	int qm;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_QM, &qm);
	if (rc < 0) {
		chg_err("get battery qm error, rc=%d\n", rc);
		return 0;
	}

	return qm;
}

int oplus_gauge_get_batt_pd(void)
{
	int rc;
	int pd;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_PD, &pd);
	if (rc < 0) {
		chg_err("get battery pd error, rc=%d\n", rc);
		return 0;
	}

	return pd;
}

int oplus_gauge_get_batt_rcu(void)
{
	int rc;
	int rcu;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_RCU, &rcu);
	if (rc < 0) {
		chg_err("get battery rcu error, rc=%d\n", rc);
		return 0;
	}

	return rcu;
}

int oplus_gauge_get_batt_rcf(void)
{
	int rc;
	int rcf;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_RCF, &rcf);
	if (rc < 0) {
		chg_err("get battery rcf error, rc=%d\n", rc);
		return 0;
	}

	return rcf;
}

int oplus_gauge_get_batt_fcu(void)
{
	int rc;
	int fcu;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_FCU, &fcu);
	if (rc < 0) {
		chg_err("get battery fcu error, rc=%d\n", rc);
		return 0;
	}

	return fcu;
}

int oplus_gauge_get_batt_fcf(void)
{
	int rc;
	int fcf;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_FCF, &fcf);
	if (rc < 0) {
		chg_err("get battery fcf error, rc=%d\n", rc);
		return 0;
	}

	return fcf;
}

int oplus_gauge_get_batt_sou(void)
{
	int rc;
	int sou;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_SOU, &sou);
	if (rc < 0) {
		chg_err("get battery sou error, rc=%d\n", rc);
		return 0;
	}

	return sou;
}

int oplus_gauge_get_batt_do0(void)
{
	int rc;
	int do0;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_DO0, &do0);
	if (rc < 0) {
		chg_err("get battery do0 error, rc=%d\n", rc);
		return 0;
	}

	return do0;
}

int oplus_gauge_get_batt_doe(void)
{
	int rc;
	int doe;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_DOE, &doe);
	if (rc < 0) {
		chg_err("get battery doe error, rc=%d\n", rc);
		return 0;
	}

	return doe;
}

int oplus_gauge_get_batt_trm(void)
{
	int rc;
	int trm;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_TRM, &trm);
	if (rc < 0) {
		chg_err("get battery trm error, rc=%d\n", rc);
		return 0;
	}

	return trm;
}

int oplus_gauge_get_batt_pc(void)
{
	int rc;
	int pc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_PC, &pc);
	if (rc < 0) {
		chg_err("get battery pc error, rc=%d\n", rc);
		return 0;
	}

	return pc;
}

int oplus_gauge_get_batt_qs(void)
{
	int rc;
	int qs;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_QS, &qs);
	if (rc < 0) {
		chg_err("get battery qs error, rc=%d\n", rc);
		return 0;
	}

	return qs;
}

int oplus_gauge_get_batt_mvolts_2cell_max(void)
{
	int rc;
	int vol_mv;

	if (!g_mms_gauge)
		return 0;

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		return gauge_dbg_vbat;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX, &vol_mv);
	if (rc < 0) {
		chg_err("get battery voltage max error, rc=%d\n", rc);
		return 0;
	}

	return vol_mv;
}

int oplus_gauge_get_batt_mvolts_2cell_min(void)
{
	int rc;
	int vol_mv;

	if (!g_mms_gauge)
		return 0;

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		return gauge_dbg_vbat;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MIN, &vol_mv);
	if (rc < 0) {
		chg_err("get battery voltage min error, rc=%d\n", rc);
		return 0;
	}

	return vol_mv;
}

static int oplus_gauge_get_subboard_temp(struct oplus_mms_gauge *chip)
{
	int rc;
	int temp;

	if (gauge_dbg_tbat != 0) {
		chg_err("debug enabled, gauge_dbg_tbat[%d]\n", gauge_dbg_tbat);
		return gauge_dbg_tbat;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_SUBBOARD_TEMP, &temp);
	if (rc < 0) {
		if (rc != -ENOTSUPP)
			chg_err("get sub board temp error, rc=%d\n", rc);
		return GAUGE_INVALID_TEMP;
	}
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	if (get_eng_version() == HIGH_TEMP_AGING || oplus_is_ptcrb_version()) {
		chg_info("[OPLUS_CHG]CONFIG_HIGH_TEMP_VERSION enable here, "
			 "disable high tbat shutdown\n");
		if (temp > 690)
			temp = 690;
	}
#endif
	return temp;
}

#define TEMP_SELECT_POINT 320
static int oplus_gauge_get_batt_temperature(struct oplus_mms_gauge *chip)
{
	int rc;
	int temp;
	int main_temp, sub_temp;

	if (gauge_dbg_tbat != 0) {
		chg_err("debug enabled, gauge_dbg_tbat[%d]\n", gauge_dbg_tbat);
		return gauge_dbg_tbat;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_TEMP, &main_temp);
	if (rc < 0) {
		chg_err("get battery temp error, rc=%d\n", rc);
		main_temp = GAUGE_INVALID_TEMP;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_TEMP, &sub_temp);
		if (rc < 0) {
			chg_err("get sub battery temp error, rc=%d\n", rc);
			sub_temp = GAUGE_INVALID_TEMP;
		}
		if (sub_temp == GAUGE_INVALID_TEMP || main_temp == GAUGE_INVALID_TEMP)
			temp = main_temp;
		else if (main_temp >= TEMP_SELECT_POINT || sub_temp >= TEMP_SELECT_POINT)
			temp = main_temp > sub_temp ? main_temp : sub_temp;
		else
			temp = main_temp < sub_temp ? main_temp : sub_temp;
	} else {
		temp = main_temp;
	}

#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	if (get_eng_version() == HIGH_TEMP_AGING || oplus_is_ptcrb_version()) {
		chg_info("[OPLUS_CHG]CONFIG_HIGH_TEMP_VERSION enable here, "
			 "disable high tbat shutdown\n");
		if (temp > 690)
			temp = 690;
	}
#endif
	return temp;
}

int oplus_gauge_get_batt_soc(void)
{
	int rc;
	int soc;

	if (!g_mms_gauge)
		return DEFAULT_SOC;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC, &soc);
	if (rc < 0) {
		chg_err("get battery soc error, rc=%d\n", rc);
		return DEFAULT_SOC;
	}

	return soc;
}

int oplus_gauge_get_bcc_parameters(char *buf)
{
	int rc;
	bool gauge_locked;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return 0;
	}

	gauge_locked = oplus_gauge_is_locked();

	if (!gauge_locked) {
		rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BCC_PARMS, buf);
		if (rc < 0) {
			chg_err("update cc parms, rc=%d\n", rc);
			return 0;
		}
	}

	return 0;
}

int oplus_gauge_fastchg_update_bcc_parameters(char *buf)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return 0;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_FASTCHG_UPDATE_BCC_PARMS, buf);
	if (rc < 0) {
		chg_err("update cc parms, rc=%d\n", rc);
		return 0;
	}
	return 0;
}

int oplus_gauge_get_prev_bcc_parameters(char *buf)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return 0;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_PREV_BCC_PARMS, buf);
	if (rc < 0) {
		chg_err("update cc parms, rc=%d\n", rc);
		return 0;
	}
	return 0;
}

int oplus_gauge_set_bcc_parameters(const char *buf)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return 0;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_BCC_PARMS, buf);
	if (rc < 0) {
		chg_err("update cc parms, rc=%d\n", rc);
		return 0;
	}
	return 0;
}

int oplus_gauge_protect_check(void)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return 0;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_PROTECT_CHECK);
	if (rc < 0) {
		chg_err("protect_check, rc=%d\n", rc);
		return 0;
	}
	return 0;
}

bool oplus_gauge_afi_update_done(void)
{
	int rc;
	bool status = true;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return true;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_AFI_UPDATE_DONE, &status);
	if (rc < 0) {
		chg_err("afi_update_done, rc=%d\n", rc);
		return true;
	}
	return status;
}

bool oplus_gauge_check_reset_condition(void)
{
	int rc;
	bool need_reset = false;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return true;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_CHECK_RESET, &need_reset);
	if (rc < 0) {
		chg_err("need_reset, rc=%d\n", rc);
		return false;
	}
	return need_reset;
}

bool oplus_gauge_reset(void)
{
	int rc;
	bool reset_done = false;

	if (g_mms_gauge == NULL) {
		chg_err("g_gauge_ic is NULL\n");
		return true;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_RESET, &reset_done);
	if (rc < 0) {
		chg_err("reset_done, rc=%d\n", rc);
		return false;
	}
	chg_err("oplus_gauge_reset, reset_done=%d\n", reset_done);
	return reset_done;
}


int oplus_gauge_get_batt_current(void)
{
	int rc;
	int curr_ma;
	int main_curr = 0, sub_curr = 0;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic_parallel[g_mms_gauge->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR, &main_curr);
	if (rc < 0) {
		chg_err("get main battery current error, rc=%d\n", rc);
		main_curr = 0;
	}
	if (g_mms_gauge->sub_gauge) {
		rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic_parallel[__ffs(g_mms_gauge->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR, &sub_curr);
		if (rc < 0) {
			chg_err("get sub battery current error, rc=%d\n", rc);
			sub_curr = 0;
		}
	}
	curr_ma = main_curr + sub_curr;

	return curr_ma;
}

int oplus_gauge_get_remaining_capacity(void)
{
	int rc;
	int rm;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_RM, &rm);
	if (rc < 0) {
		chg_err("get battery remaining capacity error, rc=%d\n", rc);
		return 0;
	}

	return rm;
}

int oplus_gauge_get_device_type(void)
{
	if (!g_mms_gauge)
		return 0;
	return g_mms_gauge->device_type;
}

int oplus_gauge_get_device_type_for_vooc(void)
{
	if (!g_mms_gauge)
		return 0;
	return g_mms_gauge->device_type_for_vooc;
}

int oplus_gauge_get_batt_fcc(void)
{
	int rc;
	int fcc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_FCC, &fcc);
	if (rc < 0) {
		chg_err("get battery fcc error, rc=%d\n", rc);
		return 0;
	}

	return fcc;
}

int oplus_gauge_get_batt_cc(void)
{
	int rc;
	int cc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_CC, &cc);
	if (rc < 0) {
		chg_err("get battery cc error, rc=%d\n", rc);
		return 0;
	}

	return cc;
}

int oplus_gauge_get_batt_soh(void)
{
	int rc;
	int soh;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_SOH, &soh);
	if (rc < 0) {
		chg_err("get battery soh error, rc=%d\n", rc);
		return 0;
	}

	return soh;
}

bool oplus_gauge_get_batt_hmac(void)
{
	int rc;
	bool pass;

	if (!g_mms_gauge)
		return false;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_HMAC, &pass);
	if (rc < 0) {
		chg_err("get battery hmac status error, rc=%d\n", rc);
		return false;
	}

	return pass;
}

bool oplus_gauge_get_batt_authenticate(void)
{
	int rc;
	bool pass;

	if (!g_mms_gauge)
		return false;
	if (g_mms_gauge->bat_volt_different)
		return false;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_BATT_AUTH, &pass);
	if (rc < 0) {
		chg_err("get battery authenticate status error, rc=%d\n", rc);
		return false;
	}

	return pass;
}

bool oplus_gauge_is_exist(struct oplus_mms *topic)
{
	struct oplus_mms_gauge *chip;
	bool exist;
	int rc;

	if (topic == NULL) {
		chg_err("topic is NULL");
		return false;
	}
	chip = oplus_mms_get_drvdata(topic);

	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_BATT_EXIST, &exist);
	if (rc < 0) {
		chg_err("get battery exist status error, rc=%d\n", rc);
		exist = false;
	}

	return exist;
}

int oplus_gauge_get_batt_capacity_mah(struct oplus_mms *topic)
{
	struct oplus_mms_gauge *chip;
	int rc;
	int cap_mah = 0;
	int i;
	int cap_temp;

	if (topic == NULL) {
		chg_err("topic is NULL");
		return 2000;
	}
	chip = oplus_mms_get_drvdata(topic);

	for (i = 0; i < chip->child_num; i++) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[i],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_CAP, &cap_temp);
		if (rc < 0) {
			chg_err("get battery capacity_mah error, rc=%d\n", rc);
			cap_temp = 2000;
		}
		cap_mah += cap_temp;
	}

	return cap_mah;
}

void oplus_gauge_set_batt_full(bool full)
{
	int rc;

	if (!g_mms_gauge)
		return;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_BATT_FULL, full);
	if (rc < 0)
		chg_err("set battery full error, rc=%d\n", rc);
}

static void oplus_mms_gauge_set_batt_full_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip =
		container_of(work, struct oplus_mms_gauge, set_gauge_batt_full_work);
	union mms_msg_data data = { 0 };

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_CHG_FULL,
				&data, false);
	chip->batt_full = data.intval;
	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_TEMP_REGION,
				&data, false);
	chip->batt_temp_region = data.intval;

	if ((chip->batt_temp_region == TEMP_REGION_COLD ||
	     chip->batt_temp_region == TEMP_REGION_LITTLE_COLD ||
	     chip->batt_temp_region == TEMP_REGION_COOL ||
	     chip->batt_temp_region == TEMP_REGION_LITTLE_COOL ||
	     chip->batt_temp_region == TEMP_REGION_PRE_NORMAL ||
	     chip->batt_temp_region == TEMP_REGION_NORMAL) && chip->batt_full) {
		oplus_gauge_set_batt_full(true);
	} else {
		oplus_gauge_set_batt_full(false);
	}
}

bool oplus_gauge_check_chip_is_null(void)
{
	if (!g_mms_gauge) {
		return true;
	} else {
		return false;
	}
}

int oplus_gauge_update_battery_dod0(void)
{
	int rc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_UPDATE_DOD0);
	if (rc < 0)
		chg_err("update battery dod0 error, rc=%d\n", rc);

	return 0;
}

int oplus_gauge_update_soc_smooth_parameter(void)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_mms_gauge is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_UPDATE_SOC_SMOOTH);
	if (rc < 0)
		chg_err("update soc smooth parameter, rc=%d\n", rc);

	return rc;
}

int oplus_gauge_get_battery_cb_status(void)
{
	int rc;
	int cb_status;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_CB_STATUS, &cb_status);
	if (rc < 0) {
		chg_err("update soc smooth parameter, rc=%d\n", rc);
		return 0;
	}

	return cb_status;
}

int oplus_gauge_get_i2c_err(void)
{
	return 0; /* nick.hu TODO */
}

void oplus_gauge_clear_i2c_err(void)
{
	/* nick.hu TODO */
}

int oplus_gauge_get_passedchg(int *val)
{
	int rc;

	if (!g_mms_gauge)
		return 0;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_PASSEDCHG, val);
	if (rc < 0) {
		chg_err("get passedchg error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int oplus_gauge_get_dod0(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_DOD0, 0, val);
			if (rc < 0) {
				chg_err("get main battery dod0 error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_DOD0, 0, val);
			if (rc < 0) {
				chg_err("get sub battery dod0 error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_DOD0, index, val);
		if (rc < 0) {
			chg_err("get dod0 error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_get_dod0_passed_q(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_DOD0_PASSED_Q, val);
			if (rc < 0) {
				chg_err("get main battery _dod0_passed_q error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_DOD0_PASSED_Q, val);
			if (rc < 0) {
				chg_err("get sub battery _dod0_passed_q error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_DOD0_PASSED_Q, val);
		if (rc < 0) {
			chg_err("get _dod0_passed_q error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_get_qmax(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_QMAX, 0, val);
			if (rc < 0) {
				chg_err("get main battery qmax error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_QMAX, 0, val);
			if (rc < 0) {
				chg_err("get sub battery qmax error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_QMAX, index, val);
		if (rc < 0) {
			chg_err("get qmax error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_get_qmax_passed_q(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_QMAX_PASSED_Q, val);
			if (rc < 0) {
				chg_err("get main battery qmax_passed_q error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_QMAX_PASSED_Q, val);
			if (rc < 0) {
				chg_err("get sub battery qmax_passed_q error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_QMAX_PASSED_Q, val);
		if (rc < 0) {
			chg_err("get qmax_passed_q error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_get_volt(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_BATT_VOL, 0, val);
			if (rc < 0) {
				chg_err("get main battery volt error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_BATT_VOL, 0, val);
			if (rc < 0) {
				chg_err("get sub battery volt error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_VOL, index, val);
		if (rc < 0) {
			chg_err("get volt error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_get_gauge_type(struct oplus_mms *mms, int index, int *val)
{
	struct oplus_mms_gauge *chip;
	int rc;

	if ((val == NULL) || (mms == NULL))
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (!chip)
		return 0;

	if (is_support_parallel(chip)) {
		switch (index) {
		case 0:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
				OPLUS_IC_FUNC_GAUGE_GET_DEVICE_TYPE_FOR_BCC, val);
			if (rc < 0) {
				chg_err("get main battery gauge_type error, rc=%d\n", rc);
				return rc;
			}
			break;
		case 1:
			rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
				OPLUS_IC_FUNC_GAUGE_GET_DEVICE_TYPE_FOR_BCC, val);
			if (rc < 0) {
				chg_err("get sub battery gauge_type error, rc=%d\n", rc);
				return rc;
			}
			break;
		default:
			break;
		}
	} else {
		rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_DEVICE_TYPE_FOR_BCC, val);
		if (rc < 0) {
			chg_err("get gauge_type error, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

int oplus_gauge_dump_register(void)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_mms_gauge is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_REG_DUMP);
	if (rc == -ENOTSUPP)
		rc = 0;

	return rc;
}

int oplus_gauge_lock(void)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_mms_gauge is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_LOCK, true);
	if (rc == -ENOTSUPP)
		rc = 0;

	return rc;
}

int oplus_gauge_unlock(void)
{
	int rc;

	if (g_mms_gauge == NULL) {
		chg_err("g_mms_gauge is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_LOCK, false);
	if (rc == -ENOTSUPP)
		rc = 0;

	return rc;
}

bool oplus_gauge_is_locked(void)
{
	int rc;
	bool locked;

	if (g_mms_gauge == NULL) {
		chg_err("g_mms_gauge is NULL\n");
		return true;
	}

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_IS_LOCKED, &locked);
	if (rc == -ENOTSUPP)
		return false;
	else if (rc < 0)
		return true;

	return locked;
}

int oplus_gauge_get_batt_num(void)
{
	int rc;
	int num;

	if (!g_mms_gauge)
		return 1;

	rc = oplus_chg_ic_func(g_mms_gauge->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_NUM, &num);
	if (rc < 0) {
		chg_err("can't get battery num, rc=%d\n", rc);
		return 1;
	}

	return num;
}

static int oplus_mms_gauge_set_err_code(struct oplus_mms_gauge *chip,
					unsigned int err_code)
{
	struct mms_msg *msg;
	int rc;

	if (chip->err_code == err_code)
		return 0;

	chip->err_code = err_code;
	chg_info("set err_code=%08x\n", err_code);

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_ERR_CODE);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish notify code msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_mms_gauge_virq_register(struct oplus_mms_gauge *chip);
static int oplus_mms_gauge_topic_init(struct oplus_mms_gauge *chip);

static void oplus_mms_gauge_init_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_mms_gauge *chip = container_of(dwork,
		struct oplus_mms_gauge, hal_gauge_init_work);
	struct device_node *node = chip->dev->of_node;
	static int retry = OPLUS_CHG_IC_INIT_RETRY_MAX;
	int rc;
	int i = 0;
	int j;

	rc = of_property_count_elems_of_size(node, "oplus,gauge_ic",
					     sizeof(u32));
	if (rc < 0) {
		chg_err("can't get gauge ic number, rc=%d\n", rc);
		goto init_try_again;
	}
	chip->child_num = rc;
	chip->child_list = devm_kzalloc(
		chip->dev,
		sizeof(struct oplus_virtual_gauge_child) * chip->child_num,
		GFP_KERNEL);
	if (chip->child_list == NULL) {
		rc = -ENOMEM;
		chg_err("alloc child ic memory error\n");
		goto init_try_again;
	}
	for (i = 0; i < chip->child_num; i++) {
		chip->child_list[i].ic_dev = of_get_oplus_chg_ic(node, "oplus,gauge_ic", i);
		if (chip->child_list[i].ic_dev == NULL) {
			chg_err("not find gauge ic %d\n", i);
			goto init_try_again;
		}
		if (chip->child_num >= GAUGE_PARALLEL_IC_NUM_MIN) {
			rc = of_property_read_u32_index(
				node, "oplus,gauge_ic_capacity_ratio", i,
				&chip->child_list[i].capacity_ratio);
			if (rc < 0) {
				chg_err("can't read ic[%d] current ratio, rc=%d\n", i,
				       rc);
				chip->child_list[i].capacity_ratio = 50;
			}
		}

		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev, OPLUS_IC_FUNC_INIT);
		if (rc == -EAGAIN) {
			chg_err("gauge_ic init timeout\n");
			goto init_try_again;
		} else if (rc < 0) {
			chg_err("gauge_ic init error, rc=%d\n", rc);
			retry = 0;
			goto init_error;
		}
	}
	retry = 0;
	rc = of_property_read_u32(node, "oplus,main_gauge", &chip->main_gauge);
	if (rc < 0) {
		chg_err("can't get main charger index, rc=%d\n", rc);
		chip->main_gauge = 0;
	}
	chip->gauge_ic = chip->child_list[chip->main_gauge].ic_dev;
	chip->gauge_ic_parallel[chip->main_gauge] = chip->child_list[chip->main_gauge].ic_dev;
	if (chip->child_num >= GAUGE_PARALLEL_IC_NUM_MIN) {
		for (j = 0;j < chip->child_num; j++){
			if (j != chip->main_gauge) {
				chip->sub_gauge |= BIT(j);
			}
		}
		chg_err(" sub_gauge: %lu\n", __ffs(chip->sub_gauge));
		chip->gauge_ic_parallel[__ffs(chip->sub_gauge)] = chip->child_list[__ffs(chip->sub_gauge)].ic_dev;
	} else {
		chip->sub_gauge = 0;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_DEVICE_TYPE,
			       &chip->device_type);
	if (rc < 0) {
		chg_err("can't get device type, rc=%d\n", rc);
		chip->device_type = 0;
	}
	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_DEVICE_TYPE_FOR_VOOC,
			       &chip->device_type_for_vooc);
	if (rc < 0) {
		chg_err("can't get device type for vooc, rc=%d\n", rc);
		chip->device_type_for_vooc = 0;
	}
	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_BATT_HMAC,
			       &chip->hmac);
	if (rc < 0) {
		chg_err("get battery hmac status error, rc=%d\n", rc);
		chip->hmac = 0;
	} else {
		chg_err("hmac=%d\n", chip->hmac);
	}

	chip->support_subboard_ntc = of_property_read_bool(node, "oplus,support_subboard_ntc");
	chg_info("support_subboard_ntc=%d \n", chip->support_subboard_ntc);

	chip->check_subboard_ntc_err = false;

	oplus_mms_gauge_virq_register(chip);
	g_mms_gauge = chip;

	(void)oplus_mms_gauge_topic_init(chip);

	return;
init_try_again:
	if (retry > 0) {
		retry--;
		schedule_delayed_work(&chip->hal_gauge_init_work,
			msecs_to_jiffies(OPLUS_CHG_IC_INIT_RETRY_DELAY));
	} else {
		chg_err("oplus,gauge_ic not found\n");
	}
init_error:
	if (chip->child_list) {
		for (; i >=0; i--)
			chip->child_list[i].ic_dev = NULL;
		devm_kfree(chip->dev, chip->child_list);
	}
	return;
}

static void oplus_mms_gauge_err_analyze(struct oplus_mms_gauge *chip,
					struct oplus_chg_ic_err_msg *msg)
{
	oplus_mms_gauge_set_err_code(chip, chip->err_code | BIT(msg->type));
}

static void oplus_mms_gauge_err_handler_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip = container_of(work, struct oplus_mms_gauge,
						err_handler_work);
	struct oplus_chg_ic_err_msg *msg, *tmp;
	struct list_head msg_list;

	INIT_LIST_HEAD(&msg_list);
	spin_lock(&chip->gauge_ic->err_list_lock);
	if (!list_empty(&chip->gauge_ic->err_list))
		list_replace_init(&chip->gauge_ic->err_list, &msg_list);
	spin_unlock(&chip->gauge_ic->err_list_lock);

	list_for_each_entry_safe(msg, tmp, &msg_list, list) {
		if (is_err_topic_available(chip))
			oplus_mms_publish_ic_err_msg(chip->err_topic,
						     ERR_ITEM_IC, msg);
		oplus_mms_gauge_err_analyze(chip, msg);
		oplus_print_ic_err(msg);
		list_del(&msg->list);
		kfree(msg);
	}
}

static void oplus_mms_gauge_online_handler_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip = container_of(
		work, struct oplus_mms_gauge, online_handler_work);
	struct mms_msg *msg;
	unsigned int err_code;
	int rc;

	err_code = chip->err_code;
	if (err_code & BIT(OPLUS_ERR_CODE_I2C))
		err_code &= ~BIT(OPLUS_ERR_CODE_I2C);
	oplus_mms_gauge_set_err_code(chip, err_code);

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_BATT_EXIST);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish batt exist msg error, rc=%d\n", rc);
		kfree(msg);
	}

	chg_info("batt_exist=%d\n", oplus_gauge_is_exist(chip->gauge_topic));
}

static void oplus_mms_gauge_offline_handler_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip = container_of(
		work, struct oplus_mms_gauge, offline_handler_work);
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_BATT_EXIST);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish batt exist msg error, rc=%d\n", rc);
		kfree(msg);
	}

	chg_info("batt_exist=%d\n", oplus_gauge_is_exist(chip->gauge_topic));
}

static void oplus_mms_gauge_resume_handler_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip = container_of(
		work, struct oplus_mms_gauge, resume_handler_work);
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_int_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_RESUME, 1);
	if (msg == NULL) {
		chg_err("alloc msg error\n");
		return;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish gauge remuse msg error, rc=%d\n", rc);
		kfree(msg);
	}
}

static void oplus_mms_gauge_err_handler(struct oplus_chg_ic_dev *ic_dev,
					void *virq_data)
{
	struct oplus_mms_gauge *chip = virq_data;
	schedule_work(&chip->err_handler_work);
}

static void oplus_mms_gauge_online_handler(struct oplus_chg_ic_dev *ic_dev,
					    void *virq_data)
{
	struct oplus_mms_gauge *chip = virq_data;
	schedule_work(&chip->online_handler_work);
}

static void oplus_mms_gauge_offline_handler(struct oplus_chg_ic_dev *ic_dev,
					    void *virq_data)
{
	struct oplus_mms_gauge *chip = virq_data;
	schedule_work(&chip->offline_handler_work);
}

static void oplus_mms_gauge_resume_handler(struct oplus_chg_ic_dev *ic_dev,
					    void *virq_data)
{
	struct oplus_mms_gauge *chip = virq_data;
	schedule_work(&chip->resume_handler_work);
}

static int oplus_mms_gauge_virq_register(struct oplus_mms_gauge *chip)
{
	int i, rc;

	for (i = 0; i < chip->child_num; i++) {
		rc = oplus_chg_ic_virq_register(chip->child_list[i].ic_dev,
			OPLUS_IC_VIRQ_ERR,
			oplus_mms_gauge_err_handler, chip);
		if (rc < 0)
			chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);

		rc = oplus_chg_ic_virq_register(chip->child_list[i].ic_dev,
			OPLUS_IC_VIRQ_ONLINE,
			oplus_mms_gauge_online_handler, chip);
		if (rc < 0)
			chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);

		rc = oplus_chg_ic_virq_register(chip->child_list[i].ic_dev,
			OPLUS_IC_VIRQ_OFFLINE,
			oplus_mms_gauge_offline_handler, chip);
		if (rc < 0)
			chg_err("register OPLUS_IC_VIRQ_OFFLINE error, rc=%d", rc);

		rc = oplus_chg_ic_virq_register(chip->child_list[i].ic_dev,
			OPLUS_IC_VIRQ_RESUME,
			oplus_mms_gauge_resume_handler, chip);
		if (rc < 0)
			chg_err("register OPLUS_IC_VIRQ_RESUME error, rc=%d", rc);
	}

	return 0;
}

static int oplus_mms_gauge_update_soc(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int soc;
	int rc;
	int main_soc, sub_soc, soc_remainder;

	if (mms == NULL) {
		chg_err("mms is NULL");
		soc = DEFAULT_SOC;
		goto end;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC, &main_soc);
	if (rc < 0) {
		chg_err("get battery soc error, rc=%d\n", rc);
		soc = DEFAULT_SOC;
		goto end;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC, &sub_soc);
		if (rc < 0) {
			chg_err("get battery sub soc error, rc=%d\n", rc);
			soc = DEFAULT_SOC;
			goto end;
		}
		soc_remainder = (main_soc * chip->child_list[chip->main_gauge].capacity_ratio +
			sub_soc * chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio) % 100;
		soc = (main_soc * chip->child_list[chip->main_gauge].capacity_ratio +
			sub_soc * chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio) / 100;
		chg_info(" main_soc:%d, sub_soc:%d, main_ratio:%d, sub_ratio:%d, soc:%d, remainder:%d",
			main_soc, sub_soc, chip->child_list[chip->main_gauge].capacity_ratio,
			chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio, soc, soc_remainder);
		if (soc_remainder != 0) {
			soc = soc + 1;
		}
	} else {
		soc = main_soc;
	}

end:
	data->intval = soc;
	return 0;
}

static int oplus_mms_sub_gauge_update_soc(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int soc;
	int rc;
	unsigned long gauge_index;

	if (mms == NULL) {
		chg_err("mms is NULL");
		soc = DEFAULT_SOC;
		goto end;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}

	chip = oplus_mms_get_drvdata(mms);
	if (strcmp("gauge:0", mms->desc->name) == 0)
		gauge_index = chip->main_gauge;
	else
		gauge_index = __ffs(chip->sub_gauge);
	if (!chip->gauge_ic_parallel[gauge_index]) {
		chg_err("sub_gauge_ic is NULL");
		soc = 0;
		goto end;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[gauge_index],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC, &soc);
	if (rc < 0) {
		chg_err("get battery soc error, rc=%d\n", rc);
		soc = DEFAULT_SOC;
	}

end:
	data->intval = soc;
	return 0;
}

static bool is_voocphy_ic_available(struct oplus_mms_gauge *chip)
{
	if (!chip->voocphy_ic)
		chip->voocphy_ic = of_get_oplus_chg_ic(chip->dev->of_node,
						       "oplus,voocphy_ic", 0);

	return !!chip->voocphy_ic;
}

#define VBATT_OVER_THR_MV 4600
#define MAX_CP_GAUGE_VBATT_DIFF 800
static int oplus_mms_gauge_choice_fit_vol(struct oplus_mms_gauge *chip, int gauge_vol)
{
	int ret;
	int cp_vbat;
	int vbatt_ov_thr_mv;

	if (chip->comm_topic)
		vbatt_ov_thr_mv = oplus_comm_get_vbatt_over_threshold(chip->comm_topic);
	else
		vbatt_ov_thr_mv = VBATT_OVER_THR_MV;

	ret = oplus_chg_ic_func(chip->voocphy_ic,
				OPLUS_IC_FUNC_VOOCPHY_GET_CP_VBAT,
				&cp_vbat);
	if (ret < 0) {
		chg_err("can't get cp voltage, rc=%d\n", ret);
		return gauge_vol;
	}

	if (cp_vbat >= vbatt_ov_thr_mv || cp_vbat <= 0 ||
	    abs(cp_vbat - gauge_vol) > MAX_CP_GAUGE_VBATT_DIFF) {
		chg_info("choice gauge voltage as vbat [%d, %d]\n",
			 cp_vbat, gauge_vol);
		return gauge_vol;
	} else {
		chg_info("choice cp voltage as vbat [%d, %d]\n",
			 cp_vbat, gauge_vol);
		return cp_vbat;
	}
}

static int oplus_mms_gauge_update_vol_max(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int vol;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		data->intval = gauge_dbg_vbat;
		return 0;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX, &vol);
	if (rc < 0) {
		chg_err("get battery voltage max error, rc=%d\n", rc);
		vol = GAUGE_DEFAULT_VOLT_MV;
	}

	if (chip->wired_online && is_voocphy_ic_available(chip) &&
	    !is_support_parallel(chip))	/* parallel project don't use cp vol */
		vol = oplus_mms_gauge_choice_fit_vol(chip, vol);

	data->intval = vol;
	return 0;
}

static int oplus_mms_sub_gauge_update_vol_max(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int vol;
	int rc;
	unsigned long gauge_index;

	if (mms == NULL) {
		chg_err("mms is NULL");
		vol = 0;
		goto end;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	if (strcmp("gauge:0", mms->desc->name) == 0)
		gauge_index = chip->main_gauge;
	else
		gauge_index = __ffs(chip->sub_gauge);
	if (!chip->gauge_ic_parallel[gauge_index]) {
		chg_err("sub_gauge_ic is NULL");
		vol = 0;
		goto end;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[gauge_index],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX, &vol);
	if (rc < 0) {
		chg_err("get battery voltage max error, rc=%d\n", rc);
		vol = 0;
	}

end:
	data->intval = vol;
	return 0;
}

static int oplus_mms_gauge_update_vol_min(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int vol;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		data->intval = gauge_dbg_vbat;
		return 0;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MIN, &vol);
	if (rc < 0) {
		chg_err("get battery voltage min error, rc=%d\n", rc);
		vol = GAUGE_DEFAULT_VOLT_MV;
	}

	if (chip->wired_online && is_voocphy_ic_available(chip))
		vol = oplus_mms_gauge_choice_fit_vol(chip, vol);

	data->intval = vol;
	return 0;
}

static int oplus_mms_gauge_update_gauge_vbat(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int vol;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	if (gauge_dbg_vbat != 0) {
		chg_info("debug enabled, voltage gauge_dbg_vbat[%d]\n", gauge_dbg_vbat);
		data->intval = gauge_dbg_vbat;
		return 0;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX, &vol);
	if (rc < 0) {
		chg_err("get battery voltage min error, rc=%d\n", rc);
		vol = GAUGE_DEFAULT_VOLT_MV;
	}

	data->intval = vol;
	return 0;
}

static int oplus_mms_gauge_update_curr(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int curr = 0;
	int rc;
	int main_curr = 0, sub_curr = 0;

	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	if (mms == NULL) {
		chg_err("mms is NULL");
		curr = 0;
		goto end;
	}
	chip = oplus_mms_get_drvdata(mms);

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR, &main_curr);
	if (rc < 0)
		main_curr = 0;
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR, &sub_curr);
		if (rc < 0)
			sub_curr = 0;
	}
	curr = main_curr + sub_curr;
end:
	data->intval = curr;
	return 0;
}

static int oplus_mms_sub_gauge_update_curr(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int curr;
	int rc;
	unsigned long gauge_index;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	if (strcmp("gauge:0", mms->desc->name) == 0)
		gauge_index = chip->main_gauge;
	else
		gauge_index = __ffs(chip->sub_gauge);
	if (!chip->gauge_ic_parallel[gauge_index]) {
		chg_err("sub_gauge_ic is NULL");
		curr = 0;
		goto end;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[gauge_index],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR, &curr);
	if (rc < 0) {
		chg_err("get battery current error, rc=%d\n", rc);
		curr = 0;
	}
end:
	data->intval = curr;
	return 0;
}

static int oplus_mms_gauge_push_subboard_temp_err(struct oplus_mms_gauge *chip, bool err)
{
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_int_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH, GAUGE_ITEM_SUBBOARD_TEMP_ERR, err);
	if (msg == NULL) {
		chg_err("alloc battery subboard msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish battery subboard msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

#define SUBBOARD_LOW_ABNORMAL_TEMP (-300)
#define SUBBOARD_HIGH_ABNORMAL_TEMP 1000
#define GAUGE_LOW_ABNORMAL_TEMP (-200)

static void oplus_mms_subboard_ntc_err_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_mms_gauge *chip =
		container_of(dwork, struct oplus_mms_gauge, subboard_ntc_err_work);
	int subboard_temp, gauge_temp;
	int err_count = 0;

	do {
		subboard_temp = oplus_gauge_get_subboard_temp(chip);
		gauge_temp = oplus_gauge_get_batt_temperature(chip);
		chg_info("subboard_temp:%d, gauge_temp:%d, err_count:%d\n",
			 subboard_temp, gauge_temp, err_count);
		usleep_range(10000, 11000);
		if (((subboard_temp <= SUBBOARD_LOW_ABNORMAL_TEMP) &&
		     (gauge_temp >= GAUGE_LOW_ABNORMAL_TEMP)) ||
		    (subboard_temp >= SUBBOARD_HIGH_ABNORMAL_TEMP))
			err_count++;
		else
			break;
	} while (err_count <= ERR_COUNT_MAX);

	if (err_count >= ERR_COUNT_MAX) {
		chip->check_subboard_ntc_err = true;
		oplus_mms_gauge_push_subboard_temp_err(chip, true);
		chg_err("send subboard temp err msg!\n");
		oplus_chg_ic_creat_err_msg(
		chip->child_list[chip->main_gauge].ic_dev, OPLUS_IC_ERR_GAUGE, 0, "bad_subboard[%d]", subboard_temp);
		oplus_chg_ic_virq_trigger(chip->child_list[chip->main_gauge].ic_dev, OPLUS_IC_VIRQ_ERR);
	}
}
#define SUBBOARD_NTC_ERR_CHECK 100
static int oplus_mms_gauge_update_temp(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int temp, gauge_temp;
	static int last_temp;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}

	chip = oplus_mms_get_drvdata(mms);
	if (get_eng_version() == FACTORY && chip->support_subboard_ntc) {
		temp = oplus_gauge_get_subboard_temp(chip);
		chg_info("is factory mode, use subboard temp directly! temp = %d\n", temp);
		data->intval = temp;
		return 0;
	}

	if (chip->support_subboard_ntc && !chip->check_subboard_ntc_err) {
		temp = oplus_gauge_get_subboard_temp(chip);
		gauge_temp = oplus_gauge_get_batt_temperature(chip);
		if (gauge_temp <= GAUGE_INVALID_TEMP) {
			temp = gauge_temp;
		} else if (((temp <= SUBBOARD_LOW_ABNORMAL_TEMP) &&
			    (gauge_temp >= GAUGE_LOW_ABNORMAL_TEMP)) ||
			   (temp >= SUBBOARD_HIGH_ABNORMAL_TEMP)) {
			if (!chip->check_subboard_ntc_err) {
				if (!(work_busy(&chip->subboard_ntc_err_work.work) & (WORK_BUSY_RUNNING)))
					schedule_delayed_work(&chip->subboard_ntc_err_work,
						msecs_to_jiffies(SUBBOARD_NTC_ERR_CHECK));
				if (last_temp <= GAUGE_INVALID_TEMP)
					temp = gauge_temp;
				else
					temp = last_temp;
			}
		} else {
			last_temp = temp;
		}
	} else {
		temp = oplus_gauge_get_batt_temperature(chip);
	}

	data->intval = temp;

	chg_info("support_subboard_ntc = %d, temp = %d\n", chip->support_subboard_ntc, temp);
	return 0;
}

static int oplus_mms_subboard_temp_err(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}

	chip = oplus_mms_get_drvdata(mms);
	if (chip->support_subboard_ntc)
		data->intval = chip->check_subboard_ntc_err;
	else
		data->intval = 0;
	chg_debug("error_info = %d \n", data->intval);
	return 0;
}

static int oplus_mms_sub_gauge_update_temp(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int temp;
	int rc;
	unsigned long gauge_index;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	if (strcmp("gauge:0", mms->desc->name) == 0)
		gauge_index = chip->main_gauge;
	else
		gauge_index = __ffs(chip->sub_gauge);
	if (!chip->gauge_ic_parallel[gauge_index]) {
		chg_err("sub_gauge_ic is NULL");
		temp = GAUGE_INVALID_TEMP;
		goto end;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[gauge_index],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_TEMP, &temp);
	if (rc < 0) {
		chg_err("get battery current error, rc=%d\n", rc);
		temp = GAUGE_INVALID_TEMP;
	}
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	if (get_eng_version() == HIGH_TEMP_AGING || oplus_is_ptcrb_version()) {
		chg_info("[OPLUS_CHG]CONFIG_HIGH_TEMP_VERSION enable here, "
			 "disable high tbat shutdown\n");
		if (temp > 690)
			temp = 690;
	}
#endif

end:
	data->intval = temp;
	return 0;
}

static int oplus_mms_gauge_update_fcc(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int fcc = 0, main_fcc = 0, sub_fcc = 0;
	int rc;
	int batt_num = oplus_gauge_get_batt_num();

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_FCC, &main_fcc);
	if (rc < 0) {
		chg_err("get battery fcc error, rc=%d\n", rc);
		main_fcc = 0;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_FCC, &sub_fcc);
		if (rc < 0) {
			chg_err("get sub battery fcc error, rc=%d\n", rc);
			sub_fcc = 0;
		}
	}

	fcc = main_fcc + sub_fcc;
	chg_info(" main_fcc:%d, sub_fcc:%d, fcc:%d", main_fcc, sub_fcc, fcc);

	data->intval = fcc * batt_num;
	return 0;
}

static int oplus_mms_gauge_update_rm(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int rm = 0;
	int main_rm = 0;
	int sub_rm = 0;
	int rc;
	int batt_num = oplus_gauge_get_batt_num();

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_RM, &main_rm);
	if (rc < 0) {
		chg_err("get battery remaining capacity error, rc=%d\n", rc);
		main_rm = 0;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_RM, &sub_rm);
		if (rc < 0) {
			chg_err("get sub battery remaining capacity error, rc=%d\n", rc);
			sub_rm = 0;
		}
	}
	rm = main_rm + sub_rm;

	data->intval = rm * batt_num;
	return 0;
}

static int oplus_mms_gauge_update_cc(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int cc, main_cc, sub_cc;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_CC, &main_cc);
	if (rc < 0) {
		chg_err("get battery cc error, rc=%d\n", rc);
		main_cc = 0;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_CC, &sub_cc);
		if (rc < 0) {
			chg_err("get sub battery cc error, rc=%d\n", rc);
			sub_cc = 0;
		}
		cc = (main_cc * chip->child_list[chip->main_gauge].capacity_ratio +
			sub_cc * chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio) / 100;
		chg_info("main_cc:%d, sub_cc:%d, main_ratio:%d, sub_ratio:%d, cc:%d\n",
			 main_cc, sub_cc, chip->child_list[chip->main_gauge].capacity_ratio,
			 chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio, cc);
	} else {
		cc = main_cc;
	}

	data->intval = cc;
	return 0;
}

static int oplus_mms_gauge_get_zy_gauge_soh(struct oplus_mms *mms, int gauge_type)
{
	int batt_soh = 0;
	int batt_qmax = 0;
	int batt_qmax_1 = 0;
	int batt_qmax_2 = 0;
	int batt_capacity_mah = 0;
	struct oplus_mms_gauge *chip;

	if (mms == NULL)
		return 0;

	chip = oplus_mms_get_drvdata(mms);
	if (chip == NULL)
		return 0;

	batt_capacity_mah = oplus_gauge_get_batt_capacity_mah(mms);
	if (batt_capacity_mah <= 0)
		return 0;

	if (DEVICE_ZY0603 == gauge_type) {
		oplus_gauge_get_qmax(mms, 0, &batt_qmax_1);
		oplus_gauge_get_qmax(mms, 1, &batt_qmax_2);

		batt_qmax = batt_qmax_1 + batt_qmax_2;
		batt_soh = batt_qmax * 100 / batt_capacity_mah;
		chg_info("series qmax_1:%d, qmax_2:%d, capacity_mah:%d, soh:%d, gauge_type:%d\n",
			batt_qmax_1, batt_qmax_2, batt_capacity_mah, batt_soh, gauge_type);
	} else if (DEVICE_ZY0602 == gauge_type) {
		if (is_support_parallel(chip)) {
			oplus_gauge_get_qmax(mms, 0, &batt_qmax_1);
			oplus_gauge_get_qmax(mms, 1, &batt_qmax_2);

			batt_qmax = batt_qmax_1 + batt_qmax_2;
			batt_soh = batt_qmax * 100 / batt_capacity_mah;
			chg_info("parallel qmax_1:%d, qmax_2:%d, capacity_mah:%d, soh:%d, gauge_type:%d\n",
				batt_qmax_1, batt_qmax_2, batt_capacity_mah, batt_soh, gauge_type);
		} else {
			oplus_gauge_get_qmax(mms, 0, &batt_qmax);
			batt_soh = batt_qmax * 100 / batt_capacity_mah;
			chg_info("singal qmax:%d, capacity_mah:%d, soh:%d, gauge_type:%d\n",
				batt_qmax, batt_capacity_mah, batt_soh, gauge_type);
		}
	}

	if (batt_soh > 100)
		batt_soh = 100;

	return batt_soh;
}

static int oplus_mms_gauge_update_soh(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int soh, main_soh, sub_soh;
	int rc;
	int gauge_type = 0;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);
	if (chip == NULL)
		return  -EINVAL;

	oplus_gauge_get_gauge_type(mms, 0, &gauge_type);
	if (gauge_type == DEVICE_ZY0602 || gauge_type == DEVICE_ZY0603) {
		soh = oplus_mms_gauge_get_zy_gauge_soh(mms, gauge_type);
		data->intval = soh;
		return 0;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic_parallel[chip->main_gauge],
		OPLUS_IC_FUNC_GAUGE_GET_BATT_SOH, &main_soh);
	if (rc < 0) {
		chg_err("get main battery soh error, rc=%d\n", rc);
		main_soh = 0;
	}
	if (chip->sub_gauge) {
		rc = oplus_chg_ic_func(chip->gauge_ic_parallel[__ffs(chip->sub_gauge)],
			OPLUS_IC_FUNC_GAUGE_GET_BATT_SOH, &sub_soh);
		if (rc < 0) {
			chg_err("get sub battery soh error, rc=%d\n", rc);
			sub_soh = 0;
		}
		soh = (main_soh * chip->child_list[chip->main_gauge].capacity_ratio +
			sub_soh * chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio) / 100;
		chg_info("main_soh:%d, sub_soh:%d, main_ratio:%d, sub_ratio:%d, soh:%d\n",
			 main_soh, sub_soh, chip->child_list[chip->main_gauge].capacity_ratio,
			 chip->child_list[__ffs(chip->sub_gauge)].capacity_ratio, soh);
	} else {
		soh = main_soh;
	}

	chg_info("soh:%d, gauge_type:%d", soh, gauge_type);
	data->intval = soh;
	return 0;
}

static int oplus_mms_gauge_update_exist(struct oplus_mms *mms,
					union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	bool exist;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_BATT_EXIST, &exist);
	if (rc < 0) {
		chg_err("get battery exist status error, rc=%d\n", rc);
		exist = false;
	}

	data->intval = exist;
	return 0;
}

static int oplus_mms_gauge_update_err_code(struct oplus_mms *mms,
					union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	unsigned int err_code;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	err_code = chip->err_code;

	data->intval = err_code;
	return 0;
}

static int oplus_mms_gauge_update_hmac(struct oplus_mms *mms,
				       union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	data->intval = chip->hmac;

	return 0;
}

static int oplus_mms_gauge_update_auth(struct oplus_mms *mms,
				       union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	bool auth;
	int rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	if (chip->bat_volt_different) {
		auth = false;
		goto end;
	}

	if (is_support_parallel(chip)) {
		auth = true;
		goto end;
	}

	rc = oplus_chg_ic_func(chip->gauge_ic,
			       OPLUS_IC_FUNC_GAUGE_GET_BATT_AUTH, &auth);
	if (rc < 0) {
		chg_err("get battery authenticate status error, rc=%d\n", rc);
		auth = false;
	}

end:
	data->intval = auth;
	return 0;
}

static int oplus_mms_gauge_real_temp(struct oplus_mms *mms, union mms_msg_data *data)
{
	struct oplus_mms_gauge *chip;
	int temp;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return -EINVAL;
	}
	if (data == NULL) {
		chg_err("data is NULL");
		return -EINVAL;
	}
	chip = oplus_mms_get_drvdata(mms);

	temp = oplus_gauge_get_batt_temperature(chip);

	data->intval = temp;
	return 0;
}

static void oplus_mms_gauge_update(struct oplus_mms *mms, bool publish)
{
	struct oplus_mms_gauge *chip;
	struct mms_msg *msg;
	int i, rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return;
	}
	chip = oplus_mms_get_drvdata(mms);

	(void)oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_UPDATE);

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

static void oplus_mms_sub_gauge_update(struct oplus_mms *mms, bool publish)
{
	struct mms_msg *msg;
	int i, rc;

	if (mms == NULL) {
		chg_err("mms is NULL");
		return;
	}

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

static struct mms_item oplus_mms_gauge_item[] = {
	{
		.desc = {
			.item_id = GAUGE_ITEM_SOC,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_soc,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_VOL_MAX,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_vol_max,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_VOL_MIN,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_vol_min,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_GAUGE_VBAT,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_gauge_vbat,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_CURR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_curr,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_TEMP,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_temp,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_FCC,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_fcc,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_RM,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_rm,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_CC,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_cc,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_SOH,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_soh,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_BATT_EXIST,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_exist,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_ERR_CODE,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_err_code,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_RESUME,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = NULL,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_HMAC,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_hmac,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_AUTH,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_gauge_update_auth,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_REAL_TEMP,
			.update = oplus_mms_gauge_real_temp,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_SUBBOARD_TEMP_ERR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_subboard_temp_err,
		}
	}
};

static struct mms_item oplus_mms_sub_gauge_item[] = {
	{
		.desc = {
			.item_id = GAUGE_ITEM_SOC,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_sub_gauge_update_soc,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_VOL_MAX,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_sub_gauge_update_vol_max,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_CURR,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_sub_gauge_update_curr,
		}
	}, {
		.desc = {
			.item_id = GAUGE_ITEM_TEMP,
			.str_data = false,
			.up_thr_enable = false,
			.down_thr_enable = false,
			.dead_thr_enable = false,
			.update = oplus_mms_sub_gauge_update_temp,
		}
	}
};

static const u32 oplus_mms_gauge_update_item[] = {
	GAUGE_ITEM_SOC,
	GAUGE_ITEM_VOL_MAX,
	GAUGE_ITEM_VOL_MIN,
	GAUGE_ITEM_CURR,
	GAUGE_ITEM_TEMP,
	GAUGE_ITEM_FCC,
	GAUGE_ITEM_CC,
	GAUGE_ITEM_SOH,
	GAUGE_ITEM_RM,
	GAUGE_ITEM_REAL_TEMP,
};

static const u32 oplus_mms_main_sub_gauge_update_item[] = {
	GAUGE_ITEM_SOC,
	GAUGE_ITEM_VOL_MAX,
	GAUGE_ITEM_CURR,
	GAUGE_ITEM_TEMP,
};

static const struct oplus_mms_desc oplus_mms_gauge_desc = {
	.name = "gauge",
	.type = OPLUS_MMS_TYPE_GAUGE,
	.item_table = oplus_mms_gauge_item,
	.item_num = ARRAY_SIZE(oplus_mms_gauge_item),
	.update_items = oplus_mms_gauge_update_item,
	.update_items_num = ARRAY_SIZE(oplus_mms_gauge_update_item),
	.update_interval = 10000, /* ms */
	.update = oplus_mms_gauge_update,
};

#define CHARGE_UPDATE_INTERVAL		5000
static void oplus_mms_gauge_update_change_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip =
		container_of(work, struct oplus_mms_gauge, update_change_work);

/* TODO: factory mode app adaptation
	if (chip->factory_test_mode)
		vote(chip->gauge_update_votable, FACTORY_TEST_VOTER, true, 500, false);
	else
		vote(chip->gauge_update_votable, FACTORY_TEST_VOTER, false, 0, false);
*/

	if (chip->wired_online || chip->wls_online) {
		chip->check_subboard_ntc_err = false;
		oplus_mms_gauge_push_subboard_temp_err(chip, false);
		vote(chip->gauge_update_votable, USER_VOTER, true, CHARGE_UPDATE_INTERVAL, false);
	} else {
		vote(chip->gauge_update_votable, USER_VOTER, false, 0, false);
	}
}

static void oplus_mms_gauge_comm_subs_callback(struct mms_subscribe *subs,
					       enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case COMM_ITEM_FACTORY_TEST:
			oplus_mms_get_item_data(chip->comm_topic, id, &data,
						false);
			chip->factory_test_mode = data.intval;
			schedule_work(&chip->update_change_work);
			break;
		case COMM_ITEM_CHG_FULL:
			schedule_work(&chip->set_gauge_batt_full_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_mms_gauge_subscribe_comm_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_mms_gauge *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->comm_topic = topic;
	chip->comm_subs =
		oplus_mms_subscribe(chip->comm_topic, chip,
				    oplus_mms_gauge_comm_subs_callback,
				    "mms_gauge");
	if (IS_ERR_OR_NULL(chip->comm_subs)) {
		chg_err("subscribe common topic error, rc=%ld\n",
			PTR_ERR(chip->comm_subs));
		return;
	}

	oplus_mms_get_item_data(chip->comm_topic, COMM_ITEM_FACTORY_TEST, &data,
				true);
	chip->factory_test_mode = data.intval;
	schedule_work(&chip->update_change_work);
}

static void oplus_mms_gauge_set_curve_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip =
		container_of(work, struct oplus_mms_gauge, gauge_set_curve_work);
	int type;
	unsigned int adapter_id;

	type = oplus_wired_get_chg_type();
	if ((type != OPLUS_CHG_USB_TYPE_VOOC) &&
	    (type != OPLUS_CHG_USB_TYPE_SVOOC)) {
		if (sid_to_adapter_chg_type(chip->vooc_sid) == CHARGER_TYPE_VOOC)
			type = OPLUS_CHG_USB_TYPE_VOOC;
		else if (sid_to_adapter_chg_type(chip->vooc_sid) == CHARGER_TYPE_SVOOC)
			type = OPLUS_CHG_USB_TYPE_SVOOC;
	}

	switch (type) {
	case OPLUS_CHG_USB_TYPE_UNKNOWN:
		return;
	case OPLUS_CHG_USB_TYPE_QC2:
	case OPLUS_CHG_USB_TYPE_QC3:
		type = CHARGER_SUBTYPE_QC;
		break;
	case OPLUS_CHG_USB_TYPE_PD:
	case OPLUS_CHG_USB_TYPE_PD_DRP:
	case OPLUS_CHG_USB_TYPE_PD_PPS:
		type = CHARGER_SUBTYPE_PD;
		break;
	case OPLUS_CHG_USB_TYPE_VOOC:
		type = CHARGER_SUBTYPE_FASTCHG_VOOC;
		break;
	case OPLUS_CHG_USB_TYPE_SVOOC:
		type = CHARGER_SUBTYPE_FASTCHG_SVOOC;
		break;
	default:
		type = CHARGER_SUBTYPE_DEFAULT;
		break;
	}

	adapter_id = sid_to_adapter_id(chip->vooc_sid);
	(void)oplus_chg_ic_func(chip->gauge_ic, OPLUS_IC_FUNC_GAUGE_SET_BATTERY_CURVE,
				type, adapter_id, chip->pd_svooc);
}

static void oplus_mms_gauge_wired_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WIRED_ITEM_ONLINE:
			oplus_mms_get_item_data(chip->wired_topic, id, &data,
						false);
			chip->wired_online = data.intval;
			schedule_work(&chip->update_change_work);
			break;
		case WIRED_ITEM_CHG_TYPE:
			if (chip->wired_online && is_voocphy_ic_available(chip))
				schedule_work(&chip->gauge_set_curve_work);
			break;
		case WIRED_TIME_ABNORMAL_ADAPTER:
			oplus_mms_get_item_data(chip->wired_topic,
						WIRED_TIME_ABNORMAL_ADAPTER,
						&data, false);
			chip->pd_svooc = data.intval;
			if (chip->wired_online && is_voocphy_ic_available(chip))
				schedule_work(&chip->gauge_set_curve_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_mms_gauge_subscribe_wired_topic(struct oplus_mms *topic,
						  void *prv_data)
{
	struct oplus_mms_gauge *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wired_topic = topic;
	chip->wired_subs =
		oplus_mms_subscribe(chip->wired_topic, chip,
				    oplus_mms_gauge_wired_subs_callback,
				    "mms_gauge");
	if (IS_ERR_OR_NULL(chip->wired_subs)) {
		chg_err("subscribe wired topic error, rc=%ld\n",
			PTR_ERR(chip->wired_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wired_topic, WIRED_ITEM_ONLINE, &data,
				true);
	chip->wired_online = !!data.intval;
	schedule_work(&chip->update_change_work);
	if (chip->wired_online && is_voocphy_ic_available(chip)) {
		oplus_mms_get_item_data(chip->wired_topic,
					WIRED_TIME_ABNORMAL_ADAPTER,
					&data, false);
		chip->pd_svooc = data.intval;
		schedule_work(&chip->gauge_set_curve_work);
	}
}

static int oplus_mms_gauge_push_hmac(struct oplus_mms_gauge *chip)
{
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_HMAC);
	if (msg == NULL) {
		chg_err("alloc battery hmac msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish battery hmac msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

static int oplus_mms_gauge_push_auth(struct oplus_mms_gauge *chip)
{
	struct mms_msg *msg;
	int rc;

	msg = oplus_mms_alloc_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH,
				  GAUGE_ITEM_AUTH);
	if (msg == NULL) {
		chg_err("alloc battery auth msg error\n");
		return -ENOMEM;
	}
	rc = oplus_mms_publish_msg(chip->gauge_topic, msg);
	if (rc < 0) {
		chg_err("publish battery auth msg error, rc=%d\n", rc);
		kfree(msg);
	}

	return rc;
}

#define ALLOW_DIFF_VALUE_MV	1000
#define BATT_VOL_ERR_COUNT_MAX	5
static int oplus_mms_gauge_check_batt_vol_diff(struct oplus_mms_gauge *chip)
{
	int vbat_max;
	int vbat_min;
	int batt_num = oplus_gauge_get_batt_num();
	struct votable *chg_disable_votable;
	union mms_msg_data data = { 0 };
	int rc;

	/* just support 2S battery */
	if (batt_num != 2)
		return 0;

	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MAX, &data,
				     false);
	if (rc < 0) {
		chg_err("can't get vbat_max, rc=%d\n", rc);
		vbat_max = 0;
	} else {
		vbat_max = data.intval;
	}
	rc = oplus_mms_get_item_data(chip->gauge_topic, GAUGE_ITEM_VOL_MIN, &data,
				     false);
	if (rc < 0) {
		chg_err("can't get vbat_min, rc=%d\n", rc);
		vbat_min = 0;
	} else {
		vbat_min = data.intval;
	}

	if (abs(vbat_max - vbat_min) > ALLOW_DIFF_VALUE_MV) {
		chip->check_batt_vol_count++;
		if (chip->check_batt_vol_count > BATT_VOL_ERR_COUNT_MAX) {
			chg_disable_votable = find_votable("CHG_DISABLE");
			if (chg_disable_votable)
				vote(chg_disable_votable, VOL_DIFF_VOTER,
				     true, 1, false);
			if (!chip->bat_volt_different)
				oplus_mms_gauge_push_auth(chip);
			chip->bat_volt_different = true;
			chip->check_batt_vol_count = 0;
			chg_info("BATTERY_SOFT_DIFF_VOLTAGE disable chg\n");
		}
	} else {
		if (chip->bat_volt_different) {
			chip->check_batt_vol_count++;
			if (chip->check_batt_vol_count >
			    BATT_VOL_ERR_COUNT_MAX) {
				if (chip->bat_volt_different) {
					chip->bat_volt_different = false;
					oplus_mms_gauge_push_auth(chip);
				}
				chip->check_batt_vol_count = 0;
				chg_disable_votable = find_votable("CHG_DISABLE");
				if (chg_disable_votable)
					vote(chg_disable_votable,
					     VOL_DIFF_VOTER, false, 0, false);
				chg_info(
					"Recovery BATTERY_SOFT_DIFF_VOLTAGE\n");
			}
		} else {
			chip->check_batt_vol_count = 0;
		}
	}

	return 0;
}

static void oplus_mms_gauge_gauge_update_work(struct work_struct *work)
{
	struct oplus_mms_gauge *chip =
		container_of(work, struct oplus_mms_gauge, gauge_update_work);
	
	oplus_mms_gauge_check_batt_vol_diff(chip);
}

static void oplus_mms_gauge_gauge_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_TIMER:
		schedule_work(&chip->gauge_update_work);
		break;
	default:
		break;
	}
}

static int oplus_mms_gauge_subscribe_gauge_topic(struct oplus_mms_gauge *chip)
{
	chip->gauge_subs =
		oplus_mms_subscribe(chip->gauge_topic, chip,
				    oplus_mms_gauge_gauge_subs_callback,
				    "mms_gauge");
	if (IS_ERR_OR_NULL(chip->gauge_subs)) {
		chg_err("subscribe gauge topic error, rc=%ld\n",
			PTR_ERR(chip->gauge_subs));
		return PTR_ERR(chip->gauge_subs);
	}

	return 0;
}

static void oplus_mms_gauge_vooc_subs_callback(struct mms_subscribe *subs,
					       enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case VOOC_ITEM_SID:
			oplus_mms_get_item_data(chip->vooc_topic, id, &data,
						false);
			chip->vooc_sid = (unsigned int)data.intval;
			if (chip->vooc_sid && chip->wired_online &&
			    is_voocphy_ic_available(chip))
				schedule_work(&chip->gauge_set_curve_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
static void oplus_mms_gauge_subscribe_vooc_topic(struct oplus_mms *topic,
						 void *prv_data)
{
	struct oplus_mms_gauge *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->vooc_topic = topic;
	chip->vooc_subs =
		oplus_mms_subscribe(chip->vooc_topic, chip,
				    oplus_mms_gauge_vooc_subs_callback,
				    "mms_gauge");
	if (IS_ERR_OR_NULL(chip->vooc_subs)) {
		chg_err("subscribe vooc topic error, rc=%ld\n",
			PTR_ERR(chip->vooc_subs));
		return;
	}

	oplus_mms_get_item_data(chip->vooc_topic, VOOC_ITEM_SID, &data,
				true);
	chip->vooc_sid = (unsigned int)data.intval;
	if (chip->wired_online && is_voocphy_ic_available(chip))
		schedule_work(&chip->gauge_set_curve_work);
}

static void oplus_mms_gauge_parallel_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case SWITCH_ITEM_STATUS:
			oplus_mms_get_item_data(chip->parallel_topic, id, &data,
						false);
			if (data.intval == PARALLEL_BAT_BALANCE_ERROR_STATUS8) {
				chip->hmac = false;
				chg_err(" ERROR_STATUS8, hmac set false\n");
				oplus_mms_gauge_push_hmac(chip);
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

static void oplus_mms_gauge_subscribe_parallel_topic(struct oplus_mms *topic,
						  void *prv_data)
{
	struct oplus_mms_gauge *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->parallel_topic = topic;
	chip->parallel_subs =
		oplus_mms_subscribe(chip->parallel_topic, chip,
				    oplus_mms_gauge_parallel_subs_callback,
				    "mms_gauge");
	if (IS_ERR_OR_NULL(chip->parallel_subs)) {
		chg_err("subscribe switch topic error, rc=%ld\n",
			PTR_ERR(chip->parallel_subs));
		return;
	}

	oplus_mms_get_item_data(chip->parallel_topic, SWITCH_ITEM_STATUS, &data,
				 true);
	if (data.intval == PARALLEL_BAT_BALANCE_ERROR_STATUS8) {
		chip->hmac = false;
		chg_err(" ERROR_STATUS8, hmac set false\n");
		oplus_mms_gauge_push_hmac(chip);
	}
}

static void oplus_mms_gauge_wls_subs_callback(struct mms_subscribe *subs,
						enum mms_msg_type type, u32 id)
{
	struct oplus_mms_gauge *chip = subs->priv_data;
	union mms_msg_data data = { 0 };

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case WLS_ITEM_PRESENT:
			oplus_mms_get_item_data(chip->wls_topic, id, &data, false);
			chip->wls_online = !!data.intval;
			schedule_work(&chip->update_change_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void oplus_mms_gauge_subscribe_wls_topic(struct oplus_mms *topic, void *prv_data)
{
	struct oplus_mms_gauge *chip = prv_data;
	union mms_msg_data data = { 0 };

	chip->wls_topic = topic;
	chip->wls_subs = oplus_mms_subscribe(chip->wls_topic, chip, oplus_mms_gauge_wls_subs_callback, "mms_gauge");
	if (IS_ERR_OR_NULL(chip->wls_subs)) {
		chg_err("subscribe wls topic error, rc=%ld\n", PTR_ERR(chip->wls_subs));
		return;
	}

	oplus_mms_get_item_data(chip->wls_topic, WLS_ITEM_PRESENT, &data, true);
	chip->wls_online = !!data.intval;
	if (chip->wls_online)
		schedule_work(&chip->update_change_work);
}

#define GAUGE_NAME_LENGTH 10
static int oplus_mms_gauge_topic_init(struct oplus_mms_gauge *chip)
{
	struct oplus_mms_config mms_cfg = {};
	int rc;
	int i;
	struct oplus_mms_desc *mms_desc;
	char *name;
	struct mms_item *item;

	mms_cfg.drv_data = chip;
	mms_cfg.of_node = chip->dev->of_node;

	if (of_property_read_bool(mms_cfg.of_node,
				  "oplus,topic-update-interval")) {
		rc = of_property_read_u32(mms_cfg.of_node,
					  "oplus,topic-update-interval",
					  &mms_cfg.update_interval);
		if (rc < 0) {
			chg_err("can't read oplus,topic-update-interval, rc=%d\n",
				rc);
			mms_cfg.update_interval = 0;
		}
	}

	chip->gauge_topic = devm_oplus_mms_register(chip->dev, &oplus_mms_gauge_desc, &mms_cfg);
	if (IS_ERR(chip->gauge_topic)) {
		chg_err("Couldn't register gauge topic\n");
		rc = PTR_ERR(chip->gauge_topic);
		return rc;
	}
	vote(chip->gauge_update_votable, DEF_VOTER, true, oplus_mms_gauge_desc.update_interval, false);
	if (is_support_parallel(chip)) {
		mms_cfg.update_interval = 0;
		mms_desc = devm_kzalloc(chip->dev, sizeof(struct oplus_mms_desc) * chip->child_num,
					GFP_KERNEL);
		for (i = 0; i < chip->child_num; i++) {
			name = devm_kzalloc(chip->dev, sizeof(char) * GAUGE_NAME_LENGTH,
					GFP_KERNEL);
			if (i != 0) {
				item = devm_kzalloc(chip->dev,
					sizeof(struct mms_item) * ARRAY_SIZE(oplus_mms_sub_gauge_item), GFP_KERNEL);
				memcpy(item, oplus_mms_sub_gauge_item, sizeof(oplus_mms_sub_gauge_item));
			}
			snprintf(name, GAUGE_NAME_LENGTH, "gauge:%d", i);
			mms_desc[i].name = name;
			mms_desc[i].type = OPLUS_MMS_TYPE_GAUGE;
			if (i == 0)
				mms_desc[i].item_table = oplus_mms_sub_gauge_item;
			else
				mms_desc[i].item_table = item;
			mms_desc[i].item_num = ARRAY_SIZE(oplus_mms_sub_gauge_item);
			mms_desc[i].update_items = oplus_mms_main_sub_gauge_update_item;
			mms_desc[i].update_items_num = ARRAY_SIZE(oplus_mms_main_sub_gauge_update_item);
			mms_desc[i].update_interval = 0;
			mms_desc[i].update = oplus_mms_sub_gauge_update;
			if (strcmp("gauge:0", mms_desc[i].name) == 0) {
				chip->gauge_topic_parallel[chip->main_gauge] =
					devm_oplus_mms_register(chip->dev, &mms_desc[i], &mms_cfg);
				if (IS_ERR(chip->gauge_topic_parallel[chip->main_gauge]))
					chg_err(" error to register main_gauge topic\n");
				else
					chg_info(" register main_gauge topic success %s\n", mms_desc[i].name);
			} else {
				chip->gauge_topic_parallel[__ffs(chip->sub_gauge)] =
					devm_oplus_mms_register(chip->dev, &mms_desc[i], &mms_cfg);
				if (IS_ERR(chip->gauge_topic_parallel[__ffs(chip->sub_gauge)]))
					chg_err(" error to register sub_gauge topic\n");
				else
					chg_info(" register sub_gauge topic success %s\n", mms_desc[i].name);
			}
		}
	}

	oplus_mms_gauge_subscribe_gauge_topic(chip);
	oplus_mms_gauge_push_hmac(chip);
	oplus_mms_wait_topic("common", oplus_mms_gauge_subscribe_comm_topic, chip);
	oplus_mms_wait_topic("wired", oplus_mms_gauge_subscribe_wired_topic, chip);
	oplus_mms_wait_topic("vooc", oplus_mms_gauge_subscribe_vooc_topic, chip);
	oplus_mms_wait_topic("parallel", oplus_mms_gauge_subscribe_parallel_topic, chip);
	oplus_mms_wait_topic("wireless", oplus_mms_gauge_subscribe_wls_topic, chip);

	return 0;
}

static int oplus_gauge_update_vote_callback(struct votable *votable, void *data,
					    int time_ms, const char *client,
					    bool step)
{
	struct oplus_mms_gauge *chip = data;
	int rc;

	if (time_ms < 0) {
		chg_err("time_ms=%d, restore default publish interval\n", time_ms);
		oplus_mms_restore_publish(chip->gauge_topic);
		return 0;
	}

	rc = oplus_mms_set_publish_interval(chip->gauge_topic, time_ms);
	if (rc < 0)
		chg_err("can't set gauge publish interval to %d\n", time_ms);
	else
		chg_err("set gauge publish interval to %d\n", time_ms);

	return rc;
}

static int oplus_mms_gauge_probe(struct platform_device *pdev)
{
	struct oplus_mms_gauge *chip;
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_mms_gauge),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}
	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	of_platform_populate(chip->dev->of_node, NULL, NULL, chip->dev);

	chip->gauge_update_votable =
		create_votable("GAUGE_UPDATE", VOTE_MIN,
			       oplus_gauge_update_vote_callback, chip);
	if (IS_ERR(chip->gauge_update_votable)) {
		rc = PTR_ERR(chip->gauge_update_votable);
		chip->gauge_update_votable = NULL;
		goto create_vote_err;
	}

	INIT_DELAYED_WORK(&chip->hal_gauge_init_work, oplus_mms_gauge_init_work);
	INIT_WORK(&chip->err_handler_work, oplus_mms_gauge_err_handler_work);
	INIT_WORK(&chip->online_handler_work, oplus_mms_gauge_online_handler_work);
	INIT_WORK(&chip->offline_handler_work, oplus_mms_gauge_offline_handler_work);
	INIT_WORK(&chip->resume_handler_work, oplus_mms_gauge_resume_handler_work);
	INIT_WORK(&chip->update_change_work, oplus_mms_gauge_update_change_work);
	INIT_WORK(&chip->gauge_update_work, oplus_mms_gauge_gauge_update_work);
	INIT_WORK(&chip->gauge_set_curve_work, oplus_mms_gauge_set_curve_work);
	INIT_WORK(&chip->set_gauge_batt_full_work, oplus_mms_gauge_set_batt_full_work);
	INIT_DELAYED_WORK(&chip->subboard_ntc_err_work, oplus_mms_subboard_ntc_err_work);

	schedule_delayed_work(&chip->hal_gauge_init_work, 0);

	chg_info("probe success\n");
	return 0;

create_vote_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);
	return rc;
}

static int oplus_mms_gauge_remove(struct platform_device *pdev)
{
	struct oplus_mms_gauge *chip = platform_get_drvdata(pdev);

	if (!IS_ERR_OR_NULL(chip->comm_subs))
		oplus_mms_unsubscribe(chip->comm_subs);
	if (!IS_ERR_OR_NULL(chip->wired_subs))
		oplus_mms_unsubscribe(chip->wired_subs);
	if (!IS_ERR_OR_NULL(chip->gauge_subs))
		oplus_mms_unsubscribe(chip->gauge_subs);

	destroy_votable(chip->gauge_update_votable);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_mms_gauge_match[] = {
	{ .compatible = "oplus,mms_gauge" },
	{},
};

static struct platform_driver oplus_mms_gauge_driver = {
	.driver		= {
		.name = "oplus-mms_gauge",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_mms_gauge_match),
	},
	.probe		= oplus_mms_gauge_probe,
	.remove		= oplus_mms_gauge_remove,
};

static __init int oplus_mms_gauge_init(void)
{
	return platform_driver_register(&oplus_mms_gauge_driver);
}

static __exit void oplus_mms_gauge_exit(void)
{
	platform_driver_unregister(&oplus_mms_gauge_driver);
}

oplus_chg_module_register(oplus_mms_gauge);
