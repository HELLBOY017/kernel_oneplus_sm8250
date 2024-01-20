// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/power_supply.h>
#include <linux/iio/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/pm_wakeup.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/reboot.h>
#include "../oplus_charger.h"
#include "../oplus_gauge.h"
#include "../oplus_vooc.h"
#include "../oplus_short.h"
#include "../oplus_adapter.h"
#include "../charger_ic/oplus_short_ic.h"
#include "../gauge_ic/oplus_bq27541.h"
#include "../oplus_chg_module.h"
#include "../oplus_chg_ops_manager.h"
#include "../voocphy/oplus_voocphy.h"
#include "oplus_sc6607_reg.h"
#ifndef CONFIG_OPLUS_CHARGER_MTK
#include <soc/oplus/system/boot_mode.h>
#include "oplus_discrete_charger.h"
#endif

#ifdef CONFIG_OPLUS_CHARGER_MTK
extern void mt_usb_connect_v1(void);
extern void mt_usb_disconnect_v1(void);
extern void oplus_chg_pullup_dp_set(bool is_on);
extern void oplus_chg_choose_gauge_curve(int index_curve);
extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
#else
extern int oplus_chg_set_pd_config(void);
extern void oplus_set_usb_props_type(enum power_supply_type type);
extern int get_boot_mode(void);
#endif

extern int smbchg_get_boot_reason(void);
extern int qpnp_get_battery_voltage(void);
extern int oplus_chg_get_shutdown_soc(void);
extern int oplus_chg_backup_soc(int backup_soc);
extern int oplus_get_rtc_ui_soc(void);
extern int oplus_set_rtc_ui_soc(int value);
extern int oplus_sm8150_get_pd_type(void);
extern void oplus_get_usbtemp_volt(struct oplus_chg_chip *chip);
extern void oplus_set_typec_sinkonly(void);
extern void oplus_set_typec_cc_open(void);
extern bool oplus_chg_check_qchv_condition(void);
extern void oplus_wake_up_usbtemp_thread(void);
extern bool oplus_chg_wake_update_work(void);
extern void oplus_chg_sc8547_error(int report_flag, int *buf, int len);
extern void oplus_voocphy_adapter_plugout_handler(void);
extern void set_charger_ic(int sel);
extern int oplus_check_pd_usb_type(void);

void oplus_voocphy_set_pdqc_config(void);
static int oplus_sc6607_charger_unsuspend(void);
static int sc6607_voocphy_get_tsbat(void);
static int sc6607_voocphy_get_tsbus(void);
static void sc6607_check_ic_suspend(void);
static void oplus_sc6607_set_mivr_by_battery_vol(void);
static void sc6607_dump_regs(struct sc6607 *chip);
static int oplus_sc6607_set_ichg(int cur);
static int oplus_sc6607_get_vbus(void);
static int oplus_sc6607_get_pd_type(void);
static int oplus_sc6607_charger_suspend(void);
static int sc6607_request_dpdm(struct sc6607 *chip, bool enable);
static int sc6607_dpdm_irq_handle(struct sc6607 *chip);
static int sc6607_hk_set_acdrv(struct sc6607 *chip, bool en);
static int oplus_sc6607_enable_otg(void);
static int oplus_sc6607_disable_otg(void);
static int bc12_detect_run(struct sc6607 *chip);
static int sc6607_get_otg_status(struct sc6607 *chip);
static int sc6607_voocphy_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data);
static int sc6607_voocphy_track_upload_i2c_err_info(struct oplus_voocphy_manager *chip, int err_type, u8 reg);
static int sc6607_voocphy_track_upload_cp_err_info(struct oplus_voocphy_manager *chip, int err_type,
						   struct sc6607_track_check_reg *p_check_reg, int check_reg_len);
static int sc6607_track_match_hk_err(struct sc6607 *chip, u8 data);
static u8 sc6607_voocphy_match_err_value(struct oplus_voocphy_manager *chip, u8 reg_state, u8 reg_flag);
static int sc6607_init_device(struct sc6607 *chip);
static int sc6607_force_dpdm(struct sc6607 *chip, bool enable);
static int sc6607_get_voocphy_enable(struct oplus_voocphy_manager *chip, u8 *data);
static void sc6607_aicr_setting_work_callback(struct work_struct *work);

#ifdef CONFIG_OPLUS_CHARGER_MTK
static const struct charger_properties  sc6607_chg_props = {
	.alias_name = "sc6607",
};
#endif
#undef pr_fmt
#define pr_fmt(fmt) "[sc6607]:%s: " fmt, __func__
#define SC6607_ADC_REG_STEP 2
#define SC6607_ADC_IDTE_THD 440
#define SC6607_ADC_TSBAT_DEFAULT (4966 * SC6607_UV_PER_MV)
#define SC6607_ADC_TSBUS_DEFAULT (4966 * SC6607_UV_PER_MV)
#define SC6607_WAIT_RESUME_TIME 200
#define SC6607_DEFAULT_IBUS_MA 500
#define SC6607_DEFAULT_VBUS_MV 5000

#ifdef CONFIG_OPLUS_CHARGER_MTK
#define OPLUS_BC12_MAX_TRY_COUNT 3
#else
#define OPLUS_BC12_MAX_TRY_COUNT 2
#endif

#define AICL_DELAY_MS 90
#define AICL_DELAY2_MS 120

#define TRACK_LOCAL_T_NS_TO_S_THD 1000000000
#define TRACK_UPLOAD_COUNT_MAX 10
#define TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD (24 * 3600)

#define SC6607_1P1_CHIP_ID 0x67
#define SC6607_1P0_CHIP_ID 0x66

#define PORT_ERROR 0
#define PORT_A 1
#define PORT_PD_WITH_USB 2
#define PORT_PD_WITHOUT_USB 3

#define DECL_ALERT_HANDLER(xbit, xhandler) { \
	.bit_mask = (1 << xbit), \
	.handler = xhandler, \
}

static int sc6607_chg_dbg_enable = 0;
static bool error_reported = false;
static int sc6607_voocphy_dump_reg = 0;
static bool oplus_is_prswap = 0;
static int bc12_try_count = 0;
static struct oplus_voocphy_manager *oplus_voocphy_mg;
static struct mutex i2c_rw_lock;
static struct sc6607 *g_chip;
static struct dentry *debugfs_sc6607;

module_param(sc6607_chg_dbg_enable, int, 0644);
MODULE_PARM_DESC(sc6607_chg_dbg_enable, "debug charger sc6607");
module_param(sc6607_voocphy_dump_reg, int, 0644);
MODULE_PARM_DESC(sc6607_voocphy_dump_reg, "dump sc6607 reg");

static const u32 sy6607_adc_step[] = {
	2500, 3750, 5000, 1250, 1250, 1220, 1250, 9766, 9766, 5, 156,
};

static int usb_icl[] = {
	100, 500, 900, 1200, 1500, 1750, 2000, 3000,
};

static struct sc6607_temp_param pst_temp_table[] = {
	{34, 4966},
	{40, 4043},
	{45, 3396},
	{50, 2847},
	{55, 2386},
	{60, 2000},
	{65, 1678},
	{70, 1411},
	{75, 1189},
	{76, 1149},
	{77, 1111},
	{78, 1074},
	{79, 1039},
	{80, 1005},
	{81, 971},
	{82, 940},
	{83, 909},
	{84, 879},
	{85, 851},
	{90, 723},
	{95, 616},
	{100, 527},
	{110, 389},
	{120, 291},
	{125, 253},
};

static const struct reg_field sc6607_reg_fields[] = {
	[F_VAC_OVP] = REG_FIELD(SC6607_REG_VAC_VBUS_OVP, 4, 7),
	[F_VBUS_OVP] = REG_FIELD(SC6607_REG_VAC_VBUS_OVP, 0, 2),
	[F_TSBUS_FLT] = REG_FIELD(SC6607_REG_TSBUS_FAULT, 0, 7),
	[F_TSBAT_FLT] = REG_FIELD(SC6607_REG_TSBAT_FAULT, 0, 7),
	[F_ACDRV_MANUAL_PRE] = REG_FIELD(SC6607_REG_HK_CTRL1, 6, 7),
	[F_ACDRV_EN] = REG_FIELD(SC6607_REG_HK_CTRL1, 5, 5),
	[F_ACDRV_MANUAL_EN] = REG_FIELD(SC6607_REG_HK_CTRL1, 4, 4),
	[F_WD_TIME_RST] = REG_FIELD(SC6607_REG_HK_CTRL1, 3, 3),
	[F_WD_TIMER] = REG_FIELD(SC6607_REG_HK_CTRL1, 0, 2),
	[F_REG_RST] = REG_FIELD(SC6607_REG_HK_CTRL2, 7, 7),
	[F_VBUS_PD] = REG_FIELD(SC6607_REG_HK_CTRL2, 6, 6),
	[F_VAC_PD] = REG_FIELD(SC6607_REG_HK_CTRL2, 5, 5),
	[F_TSBUS_TSBAT_FLT_DIS] = REG_FIELD(SC6607_REG_HK_CTRL2, 0, 0),
	[F_EDL_TSBUS_SEL] = REG_FIELD(SC6607_REG_EDL_BUFFER, 0, 0),
	[F_ADC_EN] = REG_FIELD(SC6607_REG_HK_ADC_CTRL, 7, 7),
	[F_ADC_FREEZE] = REG_FIELD(SC6607_REG_HK_ADC_CTRL, 5, 5),
	[F_BATSNS_EN] = REG_FIELD(SC6607_REG_VBAT, 7, 7),
	[F_VBAT] = REG_FIELD(SC6607_REG_VBAT, 0, 6),
	[F_ICHG_CC] = REG_FIELD(SC6607_REG_ICHG_CC, 0, 6),
	[F_VINDPM_VBAT] = REG_FIELD(SC6607_REG_VINDPM, 5, 6),
	[F_VINDPM_DIS] = REG_FIELD(SC6607_REG_VINDPM, 4, 4),
	[F_VINDPM] = REG_FIELD(SC6607_REG_VINDPM, 0, 3),
	[F_IINDPM_DIS] = REG_FIELD(SC6607_REG_IINDPM, 7, 7),
	[F_IINDPM] = REG_FIELD(SC6607_REG_IINDPM, 0, 5),
	[F_FORCE_ICO] = REG_FIELD(SC6607_REG_ICO_CTRL, 7, 7),
	[F_ICO_EN] = REG_FIELD(SC6607_REG_ICO_CTRL, 6, 6),
	[F_IINDPM_ICO] = REG_FIELD(SC6607_REG_ICO_CTRL, 0, 5),
	[F_VPRECHG] = REG_FIELD(SC6607_REG_PRECHARGE_CTRL, 7, 7),
	[F_IPRECHG] = REG_FIELD(SC6607_REG_PRECHARGE_CTRL, 0, 3),
	[F_TERM_EN] = REG_FIELD(SC6607_REG_TERMINATION_CTRL, 7, 7),
	[F_ITERM] = REG_FIELD(SC6607_REG_TERMINATION_CTRL, 0, 4),
	[F_RECHG_DIS] = REG_FIELD(SC6607_REG_RECHARGE_CTRL, 4, 4),
	[F_RECHG_DG] = REG_FIELD(SC6607_REG_RECHARGE_CTRL, 2, 3),
	[F_VRECHG] = REG_FIELD(SC6607_REG_RECHARGE_CTRL, 0, 1),
	[F_VBOOST] = REG_FIELD(SC6607_REG_VBOOST_CTRL, 3, 7),
	[F_IBOOST] = REG_FIELD(SC6607_REG_VBOOST_CTRL, 0, 2),
	[F_CONV_OCP_DIS] = REG_FIELD(SC6607_REG_PROTECTION_DIS, 4, 4),
	[F_TSBAT_JEITA_DIS] = REG_FIELD(SC6607_REG_PROTECTION_DIS, 3, 3),
	[F_IBAT_OCP_DIS] = REG_FIELD(SC6607_REG_PROTECTION_DIS, 2, 2),
	[F_VPMID_OVP_OTG_DIS] = REG_FIELD(SC6607_REG_PROTECTION_DIS, 1, 1),
	[F_VBAT_OVP_BUCK_DIS] = REG_FIELD(SC6607_REG_PROTECTION_DIS, 0, 0),
	[F_T_BATFET_RST] = REG_FIELD(SC6607_REG_RESET_CTRL, 5, 5),
	[F_T_PD_nRST] = REG_FIELD(SC6607_REG_RESET_CTRL, 4, 4),
	[F_BATFET_RST_EN] = REG_FIELD(SC6607_REG_RESET_CTRL, 3, 3),
	[F_BATFET_DLY] = REG_FIELD(SC6607_REG_RESET_CTRL, 2, 2),
	[F_BATFET_DIS] = REG_FIELD(SC6607_REG_RESET_CTRL, 1, 1),
	[F_nRST_SHIPMODE_DIS] = REG_FIELD(SC6607_REG_RESET_CTRL, 0, 0),
	[F_HIZ_EN] = REG_FIELD(SC6607_REG_CHG_CTRL1, 7, 7),
	[F_PERFORMANCE_EN] = REG_FIELD(SC6607_REG_CHG_CTRL1, 6, 6),
	[F_DIS_BUCKCHG_PATH] = REG_FIELD(SC6607_REG_CHG_CTRL1, 5, 5),
	[F_DIS_SLEEP_FOR_OTG] = REG_FIELD(SC6607_REG_CHG_CTRL1, 4, 4),
	[F_QB_EN] = REG_FIELD(SC6607_REG_CHG_CTRL1, 2, 2),
	[F_BOOST_EN] = REG_FIELD(SC6607_REG_CHG_CTRL1, 1, 1),
	[F_CHG_EN] = REG_FIELD(SC6607_REG_CHG_CTRL1, 0, 0),
	[F_VBAT_TRACK] = REG_FIELD(SC6607_REG_CHG_CTRL2, 5, 5),
	[F_IBATOCP] = REG_FIELD(SC6607_REG_CHG_CTRL2, 3, 4),
	[F_VSYSOVP_DIS] = REG_FIELD(SC6607_REG_CHG_CTRL2, 2, 2),
	[F_VSYSOVP_TH] = REG_FIELD(SC6607_REG_CHG_CTRL2, 0, 1),
	[F_JEITA_ISET_COOL] = REG_FIELD(SC6607_REG_CHG_CTRL3, 1, 1),
	[F_JEITA_VSET_WARM] = REG_FIELD(SC6607_REG_CHG_CTRL3, 0, 0),
	[F_TMR2X_EN] = REG_FIELD(SC6607_REG_CHG_CTRL4, 7, 7),
	[F_CHG_TIMER_EN] = REG_FIELD(SC6607_REG_CHG_CTRL4, 6, 6),
	[F_CHG_TIMER] = REG_FIELD(SC6607_REG_CHG_CTRL4, 4, 5),
	[F_TDIE_REG_DIS] = REG_FIELD(SC6607_REG_CHG_CTRL4, 3, 3),
	[F_TDIE_REG] = REG_FIELD(SC6607_REG_CHG_CTRL4, 1, 2),
	[F_PFM_DIS] = REG_FIELD(SC6607_REG_CHG_CTRL4, 0, 0),
	[F_BOOST_FREQ] = REG_FIELD(SC6607_REG_CHG_CTRL5, 3, 4),
	[F_BUCK_FREQ] = REG_FIELD(SC6607_REG_CHG_CTRL5, 1, 2),
	[F_BAT_LOAD_EN] = REG_FIELD(SC6607_REG_CHG_CTRL5, 0, 0),
	[F_CHG_STAT] = REG_FIELD(SC6607_REG_CHG_INT_STAT2, 5, 7),
	[F_BOOST_GOOD] = REG_FIELD(SC6607_REG_CHG_INT_STAT2, 4, 4),
	[F_JEITA_COOL_TEMP] = REG_FIELD(SC6607_REG_JEITA_TEMP, 6, 7),
	[F_JEITA_WARM_TEMP] = REG_FIELD(SC6607_REG_JEITA_TEMP, 4, 5),
	[F_BOOST_NTC_HOT_TEMP] = REG_FIELD(SC6607_REG_JEITA_TEMP, 2, 3),
	[F_BOOST_NTC_COLD_TEMP] = REG_FIELD(SC6607_REG_JEITA_TEMP, 0, 0),
	[F_TESTM_EN] = REG_FIELD(SC6607_REG_INTERNAL1, 0, 0),
	[F_KEY_EN_OWN] = REG_FIELD(SC6607_REG_INTERNAL2, 0, 0),
	[F_VBATSNS_OVP_DIS] = REG_FIELD(SC6607_REG_VBATSNS_OVP, 7, 7),
	[F_VBATSNS_DIS] = REG_FIELD(SC6607_REG_VBATSNS_OVP, 6, 6),
	[F_VBATSNS_OVP] = REG_FIELD(SC6607_REG_VBATSNS_OVP, 0, 5),
	[F_IBUS_UCP_FALL_DG_SET] = REG_FIELD(SC6607_REG_IBUS_OCP_UCP, 5, 6),
	[F_IBUS_OCP] = REG_FIELD(SC6607_REG_IBUS_OCP_UCP, 0, 4),
	[F_PMID2OUT_OVP_DIS] = REG_FIELD(SC6607_REG_PMID2OUT_OVP, 7, 7),
	[F_PMID2OUT_OVP_BLK] = REG_FIELD(SC6607_REG_PMID2OUT_OVP, 3, 4),
	[F_PMID2OUT_OVP] = REG_FIELD(SC6607_REG_PMID2OUT_OVP, 0, 2),
	[F_PMID2OUT_UVP_DIS] = REG_FIELD(SC6607_REG_PMID2OUT_UVP, 7, 7),
	[F_PMID2OUT_UVP_BLK] = REG_FIELD(SC6607_REG_PMID2OUT_UVP, 3, 4),
	[F_PMID2OUT_UVP] = REG_FIELD(SC6607_REG_PMID2OUT_UVP, 0, 2),
	[F_FSW_SET] = REG_FIELD(SC6607_REG_CP_CTRL, 5, 7),
	[F_FREQ_SHIFT] = REG_FIELD(SC6607_REG_CP_CTRL, 3, 4),
	[F_FSW_SET_Ratio] = REG_FIELD(SC6607_REG_CP_CTRL, 2, 2),
	[F_MODE] = REG_FIELD(SC6607_REG_CP_CTRL, 1, 1),
	[F_CP_EN] = REG_FIELD(SC6607_REG_CP_CTRL, 0, 0),
	[F_SS_TIMEOUT] = REG_FIELD(SC6607_REG_CP_CTRL_2, 5, 7),
	[F_IBUS_UCP_DIS] = REG_FIELD(SC6607_REG_CP_CTRL_2, 3, 3),
	[F_IBUS_OCP_DIS] = REG_FIELD(SC6607_REG_CP_CTRL_2, 2, 2),
	[F_VBUS_PD_100MA_1S_DIS] = REG_FIELD(SC6607_REG_CP_CTRL_2, 1, 1),
	[F_VBUS_IN_RANGE_DIS] = REG_FIELD(SC6607_REG_CP_CTRL_2, 0, 0),
	[F_FORCE_INDET] = REG_FIELD(SC6607_REG_DPDM_EN, 7, 7),
	[F_AUTO_INDET_EN] = REG_FIELD(SC6607_REG_DPDM_EN, 6, 6),
	[F_HVDCP_EN] = REG_FIELD(SC6607_REG_DPDM_EN, 5, 5),
	[F_QC_EN] = REG_FIELD(SC6607_REG_DPDM_EN, 0, 0),
	[F_DP_DRIV] = REG_FIELD(SC6607_REG_DPDM_CTRL, 5, 7),
	[F_DM_DRIV] = REG_FIELD(SC6607_REG_DPDM_CTRL, 2, 4),
	[F_BC1_2_VDAT_REF_SET] = REG_FIELD(SC6607_REG_DPDM_CTRL, 1, 1),
	[F_BC1_2_DP_DM_SINK_CAP] = REG_FIELD(SC6607_REG_DPDM_CTRL, 0, 0),
	[F_VBUS_STAT] = REG_FIELD(SC6607_REG_DPDM_INT_FLAG, 5, 7),
	[F_BC1_2_DONE] = REG_FIELD(SC6607_REG_DPDM_INT_FLAG, 2, 2),
	[F_DP_OVP] = REG_FIELD(SC6607_REG_DPDM_INT_FLAG, 1, 1),
	[F_DM_OVP] = REG_FIELD(SC6607_REG_DPDM_INT_FLAG, 0, 0),
	[F_DM_500K_PD_EN] = REG_FIELD(SC6607_REG_DPDM_CTRL_2, 7, 7),
	[F_DP_500K_PD_EN] = REG_FIELD(SC6607_REG_DPDM_CTRL_2, 6, 6),
	[F_DM_SINK_EN] = REG_FIELD(SC6607_REG_DPDM_CTRL_2, 5, 5),
	[F_DP_SINK_EN] = REG_FIELD(SC6607_REG_DPDM_CTRL_2, 4, 4),
	[F_DP_SRC_10UA] = REG_FIELD(SC6607_REG_DPDM_CTRL_2, 3, 3),
	[F_DPDM_3P3_EN] = REG_FIELD(SC6607_REG_DPDM_INTERNAL_3, 6, 6),
	[F_SS_TIMEOUT_MASK] = REG_FIELD(SC6607_REG_CP_FLT_MASK, 4, 4),
	[F_PHY_EN] = REG_FIELD(SC6607_REG_PHY_CTRL, 7, 7),
	[F_PHY_RST] = REG_FIELD(SC6607_REG_PHY_CTRL, 1, 1),
};

static const struct regmap_config sc6607_regmap_cfg = {
	.reg_bits = 8,
	.val_bits = 8,
};

static const char *const state_str[] = {
	"bc1.2 detect Init",
	"non-standard adapter detection",
	"floating Detection",
	"bc1.2 Primary Detection",
	"hiz set",
	"bc1.2 Secondary Detection",
	"hvdcp hanke",
};

static const char *const dpdm_str[] = {
	"0v to 0.325v",
	"0.325v to 1v",
	"1v to 1.35v",
	"1.35v to 2.2v",
	"2.2v to 3v",
	"higher than 3v",
};

void __attribute__((weak)) oplus_start_usb_peripheral(void)
{
}

void __attribute__((weak)) oplus_enable_device_mode(bool enable)
{
}

bool __attribute__((weak)) oplus_pd_without_usb(void)
{
	return 0;
}

int __attribute__((weak)) oplus_check_pd_usb_type()
{
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
void __attribute__((weak)) oplus_chg_pullup_dp_set(bool is_on);
#else
void __attribute__((weak)) oplus_chg_pullup_dp_set(bool is_on)
{
}
#endif

static int sc6607_field_read(struct sc6607 *chip, enum sc6607_fields field_id, u8 *data)
{
	int ret = 0;
	int retry = SC6607_I2C_RETRY_READ_MAX_COUNT;
	int val;

	if (ARRAY_SIZE(sc6607_reg_fields) <= field_id)
		return ret;

	mutex_lock(&i2c_rw_lock);
	ret = regmap_field_read(chip->regmap_fields[field_id], &val);
	mutex_unlock(&i2c_rw_lock);
	if (ret < 0) {
		while (retry > 0 && atomic_read(&chip->driver_suspended) == 0) {
			usleep_range(SC6607_I2C_RETRY_DELAY_US, SC6607_I2C_RETRY_DELAY_US);
			mutex_lock(&i2c_rw_lock);
			ret = regmap_field_read(chip->regmap_fields[field_id], &val);
			mutex_unlock(&i2c_rw_lock);
			if (ret < 0)
				retry--;
			else
				break;
		}
	}

	if (ret < 0)
		chg_err("i2c read fail: can't read field %d, %d\n", field_id, ret);
	else
		*data = val & 0xff;

	return ret;
}

static int sc6607_field_write(struct sc6607 *chip, enum sc6607_fields field_id, u8 val)
{
	int ret = 0;
	int retry = SC6607_I2C_RETRY_WRITE_MAX_COUNT;

	if (ARRAY_SIZE(sc6607_reg_fields) <= field_id)
		return ret;

	mutex_lock(&i2c_rw_lock);
	ret = regmap_field_write(chip->regmap_fields[field_id], val);
	mutex_unlock(&i2c_rw_lock);
	if (ret < 0) {
		while (retry > 0 && atomic_read(&chip->driver_suspended) == 0) {
			usleep_range(SC6607_I2C_RETRY_DELAY_US, SC6607_I2C_RETRY_DELAY_US);
			mutex_lock(&i2c_rw_lock);
			ret = regmap_field_write(chip->regmap_fields[field_id], val);
			mutex_unlock(&i2c_rw_lock);
			if (ret < 0)
				retry--;
			else
				break;
		}
	}

	if (ret < 0)
		chg_err("i2c read fail: can't write field %d, %d\n", field_id, ret);

	return ret;
}

static int sc6607_bulk_read(struct sc6607 *chip, u8 reg, u8 *val, size_t count)
{
	int ret;
	int retry = SC6607_I2C_RETRY_READ_MAX_COUNT;

	ret = regmap_bulk_read(chip->regmap, reg, val, count);
	if (ret < 0) {
		while (retry > 0 && atomic_read(&chip->driver_suspended) == 0) {
			usleep_range(SC6607_I2C_RETRY_DELAY_US, SC6607_I2C_RETRY_DELAY_US);
			ret = regmap_bulk_read(chip->regmap, reg, val, count);
			if (ret < 0)
				retry--;
			else
				break;
		}
	}

	if (ret < 0)
		chg_err("i2c bulk read failed: can't read 0x%0x, ret:%d\n", reg, ret);

	return ret;
}

static int sc6607_bulk_write(struct sc6607 *chip, u8 reg, u8 *val, size_t count)
{
	int ret;
	int retry = SC6607_I2C_RETRY_WRITE_MAX_COUNT;

	ret = regmap_bulk_write(chip->regmap, reg, val, count);
	if (ret < 0) {
		while (retry > 0 && atomic_read(&chip->driver_suspended) == 0) {
			usleep_range(SC6607_I2C_RETRY_DELAY_US, SC6607_I2C_RETRY_DELAY_US);
			ret = regmap_bulk_write(chip->regmap, reg, val, count);
			if (ret < 0)
				retry--;
			else
				break;
		}
	}

	if (ret < 0)
		chg_err("i2c bulk write failed: can't write 0x%0x, ret:%d\n", reg, ret);

	return ret;
}

__maybe_unused static int sc6607_read_byte(struct sc6607 *chip, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = sc6607_bulk_read(chip, reg, data, 1);
	mutex_unlock(&i2c_rw_lock);

	return ret;
}

__maybe_unused static int sc6607_write_byte(struct sc6607 *chip, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = sc6607_bulk_write(chip, reg, &data, 1);
	mutex_unlock(&i2c_rw_lock);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}

__maybe_unused static int sc6607_read_data(struct sc6607 *chip, u8 addr, u8 *buf, int len)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = sc6607_bulk_read(chip, addr, buf, len);
	mutex_unlock(&i2c_rw_lock);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", addr, ret);

	return ret;
}

__maybe_unused static int sc6607_write_data(struct sc6607 *chip, u8 addr, u8 *buf, int len)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = sc6607_bulk_write(chip, addr, buf, len);
	mutex_unlock(&i2c_rw_lock);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", addr, ret);

	return ret;
}

#ifndef CONFIG_OPLUS_CHARGER_MTK
static void Charger_Detect_Init(void)
{
	pr_info("enter\n");
	if (g_chip)
		sc6607_request_dpdm(g_chip, true);
}

static void Charger_Detect_Release(void)
{
	if (g_chip)
		sc6607_request_dpdm(g_chip, false);
}
#endif

static int bc12_update_dpdm_state(struct sc6607 *chip)
{
	u8 dp;
	u8 dm;
	int ret = -EINVAL;
	struct soft_bc12 *bc;

	if (!chip)
		return ret;

	bc = &(chip->bc12);
	ret = sc6607_write_byte(chip, SC6607_REG_DPDM_INTERNAL, 0xa0);
	if (ret < 0)
		return ret;

	ret = sc6607_read_byte(chip, SC6607_REG_DP_STAT, &dp);
	switch (dp) {
	case 0x00:
		bc->dp_state = DPDM_V0_TO_V0_325;
		break;
	case 0x01:
		bc->dp_state = DPDM_V0_325_TO_V1;
		break;
	case 0x03:
		bc->dp_state = DPDM_V1_TO_V1_35;
		break;
	case 0x07:
		bc->dp_state = DPDM_V1_35_TO_V22;
		break;
	case 0x0F:
		bc->dp_state = DPDM_V2_2_TO_V3;
		break;
	case 0x1F:
		bc->dp_state = DPDM_V3;
		break;
	default:
		break;
	}

	ret = sc6607_read_byte(chip, SC6607_REG_DM_STAT, &dm);
	switch (dm) {
	case 0x00:
		bc->dm_state = DPDM_V0_TO_V0_325;
		break;
	case 0x01:
		bc->dm_state = DPDM_V0_325_TO_V1;
		break;
	case 0x03:
		bc->dm_state = DPDM_V1_TO_V1_35;
		break;
	case 0x07:
		bc->dm_state = DPDM_V1_35_TO_V22;
		break;
	case 0x0F:
		bc->dm_state = DPDM_V2_2_TO_V3;
		break;
	case 0x1F:
		bc->dm_state = DPDM_V3;
		break;
	default:
		break;
	}

	return 0;
}

static int bc12_set_dp_state(struct sc6607 *chip, enum DPDM_SET_STATE state)
{
	int ret;

	if (!chip)
		return -EINVAL;

	switch (state) {
	case DPDM_DOWN_500K:
		ret = sc6607_field_write(chip, F_DP_DRIV, 0);
		ret |= sc6607_field_write(chip, F_DP_500K_PD_EN, 1);
		break;
	default:
		ret = sc6607_field_write(chip, F_DP_DRIV, state);
		break;
	}

	return ret;
}

static int bc12_set_dm_state(struct sc6607 *chip, enum DPDM_SET_STATE state)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_write_byte(chip, SC6607_REG_DPDM_INTERNAL + 1, 0x00);

	switch (state) {
	case DPDM_DOWN_500K:
		ret |= sc6607_field_write(chip, F_DM_DRIV, 0);
		ret |= sc6607_field_write(chip, F_DM_500K_PD_EN, 1);
		break;
	case DPDM_V1_8:
		ret |= sc6607_write_byte(chip, SC6607_REG_DPDM_INTERNAL + 2, 0x2a);
		ret |= sc6607_write_byte(chip, SC6607_REG_DPDM_INTERNAL + 1, 0x0a);
		break;
	default:
		ret = sc6607_field_write(chip, F_DM_DRIV, state);
		break;
	}

	return ret;
}

static int bc12_set_dp_cap(struct sc6607 *chip, enum DPDM_CAP cap)
{
	if (!chip)
		return -EINVAL;

	switch (cap) {
	case DPDM_CAP_SNK_50UA:
		sc6607_field_write(chip, F_DP_SINK_EN, 1);
		sc6607_field_write(chip, F_BC1_2_DP_DM_SINK_CAP, 0);
		break;
	case DPDM_CAP_SNK_100UA:
		sc6607_field_write(chip, F_DP_SINK_EN, 1);
		sc6607_field_write(chip, F_BC1_2_DP_DM_SINK_CAP, 1);
		break;
	case DPDM_CAP_SRC_10UA:
		sc6607_field_write(chip, F_DP_SINK_EN, 0);
		sc6607_field_write(chip, F_DP_SRC_10UA, 1);
		break;
	case DPDM_CAP_SRC_250UA:
		sc6607_field_write(chip, F_DP_SINK_EN, 0);
		sc6607_field_write(chip, F_DP_SRC_10UA, 0);
		break;
	default:
		break;
	}

	return 0;
}

static int bc12_set_dm_cap(struct sc6607 *chip, enum DPDM_CAP cap)
{
	if (!chip)
		return -EINVAL;

	switch (cap) {
	case DPDM_CAP_SNK_50UA:
		sc6607_field_write(chip, F_DM_SINK_EN, 1);
		sc6607_field_write(chip, F_BC1_2_DP_DM_SINK_CAP, 0);
		break;
	case DPDM_CAP_SNK_100UA:
		sc6607_field_write(chip, F_DM_SINK_EN, 1);
		sc6607_field_write(chip, F_BC1_2_DP_DM_SINK_CAP, 1);
		break;
	default:
		break;
	}

	return 0;
}

static int bc12_init(struct sc6607 *chip)
{
	int ret;
	u8 data = 0;

	ret = sc6607_field_read(chip, F_AUTO_INDET_EN, &data);
	if (data > 0) {
		sc6607_field_write(chip, F_AUTO_INDET_EN, false);
		msleep(10);
	}
	return 0;
}

static inline void bc12_transfer_state(struct soft_bc12 *bc, u8 state, int time)
{
	bc->bc12_state = state;
	bc->next_run_time = time;
}

static inline void bc_set_result(struct soft_bc12 *bc, enum BC12_RESULT result)
{
	struct sc6607 *chip = g_chip;

	if (!chip)
		return;

	bc->result = result | (bc->flag << 3);
	bc->detect_done = true;

	switch (result) {
	case UNKNOWN_DETECED:
		chip->soft_bc12_type = SC6607_VBUS_TYPE_DCP;
		break;
	case SDP_DETECED:
		if (bc->first_noti_sdp)
			chip->soft_bc12_type = SC6607_VBUS_TYPE_NONE;
		else
			chip->soft_bc12_type = SC6607_VBUS_TYPE_SDP;
		break;
	case CDP_DETECED:
		if (bc->first_noti_sdp)
			chip->soft_bc12_type = SC6607_VBUS_TYPE_NONE;
		else
			chip->soft_bc12_type = SC6607_VBUS_TYPE_CDP;
		break;
	case DCP_DETECED:
	case NON_STANDARD_DETECTED:
	case APPLE_3A_DETECTED:
	case APPLE_2_1A_DETECTED:
	case SS_2A_DETECTED:
	case APPLE_1A_DETECTED:
	case APPLE_2_4A_DETECTED:
	case OCP_DETECED:
		chip->soft_bc12_type = SC6607_VBUS_TYPE_DCP;
		break;
	case HVDCP_DETECED:
		chip->soft_bc12_type = SC6607_VBUS_TYPE_DCP;
		break;
	default:
		chip->soft_bc12_type = SC6607_VBUS_TYPE_DCP;
		break;
	}
}

static int bc12_detect_init(struct soft_bc12 *bc)
{
	bc->detect_done = false;

	bc12_init(g_chip);
	bc12_set_dm_state(g_chip, DPDM_HIZ);
	bc12_set_dp_state(g_chip, DPDM_HIZ);
	bc12_transfer_state(bc, NON_STANDARD_ADAPTER_DETECTION, 40);

	return 0;
}

static int bc12_nostand_adapter_detect_entry(struct soft_bc12 *bc)
{
	if (bc->dp_state == DPDM_V2_2_TO_V3 && bc->dm_state == DPDM_V3)
		bc->flag = 1;
	else if (bc->dp_state == DPDM_V2_2_TO_V3 && bc->dm_state == DPDM_V1_35_TO_V22)
		bc->flag = 2;
	else if (bc->dp_state == DPDM_V1_TO_V1_35 && bc->dm_state == DPDM_V1_TO_V1_35)
		bc->flag = 3;
	else if (bc->dp_state == DPDM_V1_35_TO_V22 && bc->dm_state == DPDM_V2_2_TO_V3)
		bc->flag = 4;
	else if (bc->dp_state == DPDM_V2_2_TO_V3 && bc->dm_state == DPDM_V2_2_TO_V3)
		bc->flag = 5;
	else
		bc->flag = 0;

	bc12_set_dp_state(g_chip, DPDM_V2_7);
	bc12_set_dm_state(g_chip, DPDM_DOWN_20K);
	bc12_set_dp_cap(g_chip, DPDM_CAP_SRC_10UA);
	bc12_transfer_state(bc, FLOAT_DETECTION, 15);
	return 0;
}

static int bc12_float_detection_entry(struct soft_bc12 *bc)
{
	if (bc->dp_state >= DPDM_V1_TO_V1_35 && bc->flag == 0)
		bc_set_result(bc, OCP_DETECED);
	else {
		bc12_set_dp_state(g_chip, DPDM_V0_6);
		bc12_set_dp_cap(g_chip, DPDM_CAP_SRC_250UA);

		bc12_set_dm_state(g_chip, DPDM_HIZ);
		bc12_set_dm_cap(g_chip, DPDM_CAP_SNK_50UA);

		bc12_transfer_state(bc, BC12_PRIMARY_DETECTION, 100);
	}

	return 0;
}

static int bc12_primary_detect_entry(struct soft_bc12 *bc)
{
	if (bc->dm_state == DPDM_V0_TO_V0_325 && bc->flag == 0) {
		bc_set_result(bc, SDP_DETECED);
	} else if (bc->dm_state == DPDM_V0_325_TO_V1) {
		bc12_set_dp_state(g_chip, DPDM_HIZ);
		bc12_set_dm_state(g_chip, DPDM_HIZ);
		bc12_transfer_state(bc, HIZ_SET, 20);
	} else {
		if (bc->flag == 0)
			bc_set_result(bc, UNKNOWN_DETECED);
		else
			bc_set_result(bc, NON_STANDARD_DETECTED);
	}
	return 0;
}

static int bc12_hiz_set_entry(struct soft_bc12 *bc)
{
	bc12_set_dp_cap(g_chip, DPDM_CAP_SNK_50UA);
	bc12_set_dm_state(g_chip, DPDM_V1_8);
	bc12_transfer_state(bc, BC12_SECONDARY_DETECTION, 40);
	return 0;
}

static int bc12_secondary_detect_entry(struct soft_bc12 *bc)
{
	if (bc->dp_state < DPDM_V1_35_TO_V22)
		bc_set_result(bc, CDP_DETECED);
	else if (bc->dp_state == DPDM_V1_35_TO_V22) {
		bc12_set_dm_cap(g_chip, DPDM_CAP_SNK_100UA);
		bc12_set_dm_state(g_chip, DPDM_HIZ);

		bc12_set_dp_state(g_chip, DPDM_V0_6);
		bc12_transfer_state(bc, HVDCP_HANKE, 2000);
		bc_set_result(bc, DCP_DETECED);
	} else {
		if (bc->flag == 0)
			bc_set_result(bc, UNKNOWN_DETECED);
		else
			bc_set_result(bc, NON_STANDARD_DETECTED);
	}
	return 0;
}

static int bc12_hvdcp_hanke_entry(struct soft_bc12 *bc)
{
	if (bc->dm_state == DPDM_V0_TO_V0_325)
		bc_set_result(bc, HVDCP_DETECED);
	bc->next_run_time = -1;
	return 0;
}

static bool bc12_should_run(struct sc6607 *chip)
{
	bool ret = true;

	mutex_lock(&chip->bc12.running_lock);
	if (chip->power_good) {
		chip->bc12.detect_ing = true;
	} else {
		chip->bc12.detect_ing = false;
		ret = false;
	}
	mutex_unlock(&chip->bc12.running_lock);

	return ret;
}

static void sc6607_soft_bc12_work_func(struct work_struct *work)
{
	struct soft_bc12 *bc;

	if (!g_chip || !bc12_should_run(g_chip))
		return;

	bc = &(g_chip->bc12);
	bc12_update_dpdm_state(g_chip);
	pr_info("dp volt range %s, dm volt range %s, state : %s\n",
		dpdm_str[bc->dp_state], dpdm_str[bc->dm_state],
		state_str[bc->bc12_state]);

	switch (bc->bc12_state) {
	case BC12_DETECT_INIT:
		bc12_detect_init(bc);
		break;
	case NON_STANDARD_ADAPTER_DETECTION:
		bc12_nostand_adapter_detect_entry(bc);
		break;
	case FLOAT_DETECTION:
		bc12_float_detection_entry(bc);
		break;
	case BC12_PRIMARY_DETECTION:
		bc12_primary_detect_entry(bc);
		break;
	case HIZ_SET:
		bc12_hiz_set_entry(bc);
		break;
	case BC12_SECONDARY_DETECTION:
		bc12_secondary_detect_entry(bc);
		break;
	case HVDCP_HANKE:
		bc12_hvdcp_hanke_entry(bc);
		break;
	default:
		break;
	}

	if (bc->detect_done) {
		bc->detect_ing = false;
		if (bc->first_noti_sdp && (bc->result == SDP_DETECED || bc->result == CDP_DETECED)) {
			bc12_set_dp_state(g_chip, DPDM_HIZ);
			bc12_set_dm_state(g_chip, DPDM_HIZ);
			sc6607_field_write(g_chip, F_DPDM_3P3_EN, true);
			bc12_detect_run(g_chip);
		} else {
			pr_info("set hiz\n");
			bc12_set_dp_state(g_chip, DPDM_HIZ);
			bc12_set_dm_state(g_chip, DPDM_HIZ);
			sc6607_field_write(g_chip, F_DPDM_3P3_EN, true);
			if (bc->result == SDP_DETECED || bc->result == CDP_DETECED)
				g_chip->usb_connect_start = true;
			sc6607_dpdm_irq_handle(g_chip);
			sc6607_check_ic_suspend();
		}
		bc->first_noti_sdp = false;
	} else if (bc->next_run_time >= 0) {
		schedule_delayed_work(&bc->detect_work, msecs_to_jiffies(bc->next_run_time));
	}
}

static int bc12_detect_run(struct sc6607 *chip)
{
	struct soft_bc12 *bc;

	if (!chip)
		return -EINVAL;

	pr_info("start\n");
	bc = &(chip->bc12);
	if (bc->detect_ing) {
		pr_info("detect_ing, should return\n");
		return 0;
	}
	bc->bc12_state = BC12_DETECT_INIT;
	schedule_delayed_work(&bc->detect_work, msecs_to_jiffies(300));

	return 0;
}

static int sc6607_charger_get_soft_bc12_type(struct sc6607 *chip)
{
	if (chip)
		return chip->soft_bc12_type;

	return SC6607_VBUS_TYPE_NONE;
}

static void sc6607_hw_bc12_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct sc6607 *chip = container_of(dwork, struct sc6607, hw_bc12_detect_work);

	if (!chip)
		return;
	Charger_Detect_Init();
	bc12_try_count = 0;
	chip->bc12_done = false;
	sc6607_force_dpdm(chip, true);
}

static int sc6607_check_device_id(struct sc6607 *chip)
{
	int ret;
	u8 chip_id;

	if (!chip)
		return -EINVAL;

	ret = sc6607_read_byte(chip, SC6607_REG_DEVICE_ID, &chip_id);
	if (ret < 0) {
		chip->chip_id = SC6607_1P0_CHIP_ID;
		return ret;
	}
	chip->chip_id = chip_id;
	pr_info("chip_id:%d\n", chip->chip_id);

	return 0;
}

static int sc6607_enable_otg(struct sc6607 *chip)
{
	int ret = 0;

	if (!chip)
		return -EINVAL;

	pr_info("enter\n");
	ret = sc6607_field_write(chip, F_BOOST_EN, true);

	return ret;
}

static int sc6607_disable_otg(struct sc6607 *chip)
{
	int ret = 0;

	if (!chip)
		return -EINVAL;

	pr_err("enter\n");
	ret = sc6607_field_write(chip, F_BOOST_EN, false);

	return ret;
}

static int sc6607_get_otg_status(struct sc6607 *chip)
{
	u8 data = 0;
	if (!chip)
		return 0;

	sc6607_field_read(chip, F_BOOST_EN, &data);
	pr_info("status = %d\n", data);

	return data;
}

static int sc6607_disable_hvdcp(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_HVDCP_EN, false);
	return ret;
}

static int sc6607_enable_charger(struct sc6607 *chip)
{
	int ret;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!chip || !chg_chip)
		return -EINVAL;

	pr_info("enable\n");
	if (atomic_read(&chip->driver_suspended) || chip->request_otg) {
		pr_err("suspend or camera, ignore\n");
		return 0;
	}

	ret = sc6607_field_write(chip, F_CHG_EN, true);

	return ret;
}

static int sc6607_disable_charger(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	pr_info("disable\n");
	ret = sc6607_field_write(chip, F_CHG_EN, false);

	return ret;
}

static int sc6607_hk_get_adc(struct sc6607 *chip, enum SC6607_ADC_MODULE id)
{
	u32 reg = SC6607_REG_HK_IBUS_ADC + id * SC6607_ADC_REG_STEP;
	u8 val[2] = { 0 };
	u32 ret;
	u8 adc_open = 0;

	sc6607_field_read(chip, F_ADC_EN, &adc_open);
	if (!adc_open) {
		if (id == SC6607_ADC_TSBAT)
			return SC6607_ADC_TSBAT_DEFAULT;
		else if (id == SC6607_ADC_TSBUS)
			return SC6607_ADC_TSBUS_DEFAULT;
		else
			return 0;
	}
	mutex_lock(&chip->adc_read_lock);
	sc6607_field_write(chip, F_ADC_FREEZE, 1);
	ret = sc6607_bulk_read(chip, reg, val, sizeof(val));
	sc6607_field_write(chip, F_ADC_FREEZE, 0);
	mutex_unlock(&chip->adc_read_lock);
	if (ret < 0) {
		return 0;
	}
	ret = val[1] + (val[0] << 8);
	if (id == SC6607_ADC_TDIE)
		ret = (SC6607_ADC_IDTE_THD - ret) / 2;
	else
		ret *= sy6607_adc_step[id];
	return ret;
}

static int sc6607_adc_read_ibus(struct sc6607 *chip)
{
	int ibus = 0;

	if (!chip)
		return -EINVAL;
	if (oplus_voocphy_mg != NULL && oplus_voocphy_get_fastchg_commu_ing()) {
		pr_err("svooc in communication, ignore.\n");
		return oplus_voocphy_mg->cp_ichg;
	}
	ibus = sc6607_hk_get_adc(chip, SC6607_ADC_IBUS);
	ibus /= SC6607_UA_PER_MA;

	return ibus;
}

static int sc6607_adc_read_vbus_volt(struct sc6607 *chip)
{
	int vbus_vol = 0;

	if (!chip)
		return -EINVAL;
	if (oplus_voocphy_mg != NULL && oplus_voocphy_get_fastchg_commu_ing()) {
		pr_err("svooc in communication, ignore.\n");
		return oplus_voocphy_mg->cp_vbus;
	}
	vbus_vol = sc6607_hk_get_adc(chip, SC6607_ADC_VBUS);
	vbus_vol /= SC6607_UV_PER_MV;

	return vbus_vol;
}

static int sc6607_adc_read_vac(struct sc6607 *chip)
{
	int vac_vol = 0;

	if (!chip)
		return -EINVAL;
	if (oplus_voocphy_mg != NULL && oplus_voocphy_get_fastchg_commu_ing()) {
		pr_err("svooc in communication, ignore.\n");
		return oplus_voocphy_mg->cp_vac;
	}
	vac_vol = sc6607_hk_get_adc(chip, SC6607_ADC_VAC);
	vac_vol /= SC6607_UV_PER_MV;

	return vac_vol;
}

static int sc6607_adc_read_vbat(struct sc6607 *chip)
{
	int vbat = 0;

	if (!chip)
		return -EINVAL;
	if (oplus_voocphy_mg != NULL && oplus_voocphy_get_fastchg_commu_ing()) {
		pr_err("svooc in communication, ignore.\n");
		return oplus_voocphy_mg->cp_vbat;
	}
	vbat = sc6607_hk_get_adc(chip, SC6607_ADC_VBATSNS);
	vbat /= SC6607_UV_PER_MV;

	return vbat;
}

static int sc6607_adc_read_vsys(struct sc6607 *chip)
{
	int vsys = 0;

	if (!chip)
		return -EINVAL;
	if (oplus_voocphy_mg != NULL && oplus_voocphy_get_fastchg_commu_ing()) {
		pr_err("svooc in communication, ignore.\n");
		return oplus_voocphy_mg->cp_vsys;
	}
	vsys = sc6607_hk_get_adc(chip, SC6607_ADC_VSYS);
	vsys /= SC6607_UV_PER_MV;

	return vsys;
}

static int sc6607_adc_read_tsbus(struct sc6607 *chip)
{
	int tsbus = 0;

	if (!chip)
		return -EINVAL;

	tsbus = sc6607_hk_get_adc(chip, SC6607_ADC_TSBUS);
	tsbus /= SC6607_UV_PER_MV;

	return tsbus;
}

static int sc6607_adc_read_tsbat(struct sc6607 *chip)
{
	int tsbat = 0;

	if (!chip)
		return -EINVAL;

	tsbat = sc6607_hk_get_adc(chip, SC6607_ADC_TSBAT);
	tsbat /= SC6607_UV_PER_MV;

	return tsbat;
}
int oplus_sc6607_read_vbus(void)
{
	int vbus = 0;

	if (!g_chip)
		return -EINVAL;

	vbus = sc6607_adc_read_vbus_volt(g_chip);

	return vbus;
}

int oplus_sc6607_read_ibus(void)
{
	int ibus = 0;

	if (!g_chip)
		return -EINVAL;

	ibus = sc6607_adc_read_ibus(g_chip);

	return ibus;
}

int oplus_sc6607_read_vac(void)
{
	int vac = 0;

	if (!g_chip)
		return -EINVAL;

	vac = sc6607_adc_read_vac(g_chip);

	return vac;
}

int oplus_sc6607_read_vsys(void)
{
	int vsys = 0;

	if (!g_chip)
		return -EINVAL;

	vsys = sc6607_adc_read_vsys(g_chip);

	return vsys;
}

static int oplus_sc6607_set_ichg(int curr)
{
	int ret;
	int val;

	if (!g_chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		pr_err("suspend,ignore set cur = %d mA\n", curr);
		return 0;
	}

	if (curr > SC6607_CHG_CURRENT_MAX_MA)
		curr = SC6607_CHG_CURRENT_MAX_MA;

	val = (curr - SC6607_BUCK_ICHG_OFFSET) / SC6607_BUCK_ICHG_STEP;
	ret = sc6607_field_write(g_chip, F_ICHG_CC, val);
	pr_info("current = %d, val=0x%0x\n", curr, val);

	return ret;
}

static int sc6607_set_term_current(struct sc6607 *chip, int curr)
{
	u8 iterm;
	int ret;

	if (!g_chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		pr_err("suspend,ignore set cur = %d mA\n", curr);
		return 0;
	}

	iterm = (curr - SC6607_BUCK_ITERM_OFFSET) / SC6607_BUCK_ITERM_STEP;
	ret = sc6607_field_write(g_chip, F_ITERM, iterm);
	pr_info("iterm = %d, val=0x%0x\n", curr, iterm);

	return ret;
}

static int sc6607_set_prechg_current(struct sc6607 *chip, int curr)
{
	u8 iprechg;
	int ret;

	if (!g_chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		pr_err("suspend,ignore set cur = %d mA\n", curr);
		return 0;
	}

	iprechg = (curr - SC6607_BUCK_IPRECHG_OFFSET) / SC6607_BUCK_IPRECHG_STEP;
	ret = sc6607_field_write(g_chip, F_IPRECHG, iprechg);
	pr_info("iprechg = %d, val=0x%0x\n", curr, iprechg);

	return ret;
}

static int sc6607_set_chargevolt(struct sc6607 *chip, int volt)
{
	u8 val = 0;
	int ret;

	if (!g_chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		pr_err("suspend,ignore set volt = %d mv\n", volt);
		return 0;
	}

	val = (volt - SC6607_BUCK_VBAT_OFFSET) / SC6607_BUCK_VBAT_STEP;
	ret = sc6607_field_write(g_chip, F_VBAT, val);
	pr_info("volt = %d, val=0x%0x\n", volt, val);

	return ret;
}

static int sc6607_set_input_volt_limit(struct sc6607 *chip, int volt)
{
	u8 val = 0;
	int ret;

	if (!chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		pr_err("suspend,ignore set volt_limit = %d mv\n", volt);
		return 0;
	}

	if (volt <= SC6607_VINDPM_VOL_MV(4000))
		val = SC6607_VINDPM_4000;
	else if (volt <= SC6607_VINDPM_VOL_MV(4100))
		val = SC6607_VINDPM_4100;
	else if (volt <= SC6607_VINDPM_VOL_MV(4200))
		val = SC6607_VINDPM_4200;
	else if (volt <= SC6607_VINDPM_VOL_MV(4300))
		val = SC6607_VINDPM_4300;
	else if (volt <= SC6607_VINDPM_VOL_MV(4400))
		val = SC6607_VINDPM_4400;
	else if (volt <= SC6607_VINDPM_VOL_MV(4500))
		val = SC6607_VINDPM_4500;
	else if (volt <= SC6607_VINDPM_VOL_MV(4600))
		val = SC6607_VINDPM_4600;
	else if (volt <= SC6607_VINDPM_VOL_MV(4700))
		val = SC6607_VINDPM_4700;
	else if (volt <= SC6607_VINDPM_VOL_MV(7600))
		val = SC6607_VINDPM_7600;
	else if (volt <= SC6607_VINDPM_VOL_MV(8200))
		val = SC6607_VINDPM_8200;
	else if (volt <= SC6607_VINDPM_VOL_MV(8400))
		val = SC6607_VINDPM_8400;
	else if (volt <= SC6607_VINDPM_VOL_MV(8600))
		val = SC6607_VINDPM_8600;
	else if (volt <= SC6607_VINDPM_VOL_MV(10000))
		val = SC6607_VINDPM_10000;
	else if (volt <= SC6607_VINDPM_VOL_MV(10500))
		val = SC6607_VINDPM_10500;
	else
		val = SC6607_VINDPM_10700;

	ret = sc6607_field_write(chip, F_VINDPM, val);
	pr_info("volt = %d, val=0x%0x\n", volt, val);

	return ret;
}

static int sc6607_set_input_current_limit(struct sc6607 *chip, int curr)
{
	int val;
	int ret;

	if (!chip)
		return -EINVAL;

	if (curr < SC6607_BUCK_IINDPM_OFFSET)
		curr = SC6607_BUCK_IINDPM_OFFSET;

	val = (curr - SC6607_BUCK_IINDPM_OFFSET) / SC6607_BUCK_IINDPM_STEP;
	ret = sc6607_field_write(chip, F_IINDPM, val);
	pr_info("curr:%d, val=0x%x\n", curr, val);

	return ret;
}

static int sc6607_set_watchdog_timer(struct sc6607 *chip, u32 timeout)
{
	u8 val = 0;
	int ret;

	if (!chip)
		return -EINVAL;

	if (timeout <= SC6607_WD_TIMEOUT_S(0))
		val = SC6607_WD_DISABLE;
	else if (timeout <= SC6607_WD_TIMEOUT_S(500))
		val = SC6607_WD_0_5_S;
	else if (timeout <= SC6607_WD_TIMEOUT_S(1000))
		val = SC6607_WD_1_S;
	else if (timeout <= SC6607_WD_TIMEOUT_S(2000))
		val = SC6607_WD_2_S;
	else if (timeout <= SC6607_WD_TIMEOUT_S(20000))
		val = SC6607_WD_20_S;
	else if (timeout <= SC6607_WD_TIMEOUT_S(40000))
		val = SC6607_WD_40_S;
	else if (timeout <= SC6607_WD_TIMEOUT_S(80000))
		val = SC6607_WD_80_S;
	else
		val = SC6607_WD_160_S;

	ret = sc6607_field_write(chip, F_WD_TIMER, val);
	pr_info("timeout:%d, val=0x%x\n", timeout, val);

	return ret;
}

static int sc6607_disable_watchdog_timer(struct sc6607 *chip)
{
	if (!chip)
		return -EINVAL;

	return sc6607_set_watchdog_timer(chip, SC6607_WD_TIMEOUT_S(0));
}

static int sc6607_reset_watchdog_timer(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	pr_info("enter\n");
	ret = sc6607_field_write(chip, F_WD_TIME_RST, true);

	return ret;
}

static int sc6607_force_dpdm(struct sc6607 *chip, bool enable)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_FORCE_INDET, enable);
	pr_info("force dpdm %s, enable=%d\n", !ret ? "successfully" : "failed", enable);
	return ret;
}

static int sc6607_reset_chip(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_REG_RST, true);
	ret = sc6607_field_write(chip, F_BATFET_RST_EN, false);
	pr_info("reset chip %s\n", !ret ? "successfully" : "failed");
	return ret;
}

static int sc6607_enable_enlim(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_IINDPM_DIS, false);
	pr_info("enable ilim %s\n", !ret ? "successfully" : "failed");
	return ret;
}

static int sc6607_enter_hiz_mode(struct sc6607 *chip)
{
	int ret;
	int boot_mode = get_boot_mode();

	if (!chip)
		return -EINVAL;

	pr_info("enter\n");
#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (oplus_is_rf_ftm_mode())
		ret = sc6607_field_write(chip, F_HIZ_EN, false);
	else
		ret = sc6607_field_write(chip, F_HIZ_EN, true);

#else
	if (boot_mode == MSM_BOOT_MODE__RF || boot_mode == MSM_BOOT_MODE__WLAN ||
	    boot_mode == MSM_BOOT_MODE__FACTORY)
		ret = sc6607_field_write(chip, F_HIZ_EN, false);
	else
		ret = sc6607_field_write(chip, F_HIZ_EN, true);
#endif
	ret |= sc6607_disable_charger(chip);
	oplus_sc6607_charger_suspend();

	return ret;
}

static int sc6607_exit_hiz_mode(struct sc6607 *chip)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_HIZ_EN, false);
	pr_info("exit_hiz_mode %s\n", !ret ? "successfully" : "failed");

	return ret;
}

static int sc6607_enable_term(struct sc6607 *chip, bool enable)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_TERM_EN, enable);
	pr_info("%s\n", enable ? "enable" : "disable");

	return ret;
}

int sc6607_set_boost_current(struct sc6607 *chip, int curr)
{
	int val;
	int ret;

	if (!chip)
		return -EINVAL;

	if (curr <= SC6607_BOOST_CURR_MA(500))
		val = SC6607_BOOST_CURR_500;
	else if (curr <= SC6607_BOOST_CURR_MA(900))
		val = SC6607_BOOST_CURR_900;
	else if (curr <= SC6607_BOOST_CURR_MA(1300))
		val = SC6607_BOOST_CURR_1300;
	else if (curr <= SC6607_BOOST_CURR_MA(1500))
		val = SC6607_BOOST_CURR_1500;
	else if (curr <= SC6607_BOOST_CURR_MA(2100))
		val = SC6607_BOOST_CURR_2100;
	else if (curr <= SC6607_BOOST_CURR_MA(2500))
		val = SC6607_BOOST_CURR_2500;
	else if (curr <= SC6607_BOOST_CURR_MA(2900))
		val = SC6607_BOOST_CURR_2900;
	else
		val = SC6607_BOOST_CURR_32500;

	ret = sc6607_field_write(chip, F_IBOOST, val);
	pr_info("boost current %d mA, val=0x%x\n", curr, val);

	return ret;
}

static int sc6607_vmin_limit(struct sc6607 *chip, u32 volt)
{
	int val;
	int ret;

	if (!chip)
		return -EINVAL;

	if (volt < SC6607_VSYSMIN_VOL_MV(2800))
		val = SC6607_VSYSMIN_2600;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3000))
		val = SC6607_VSYSMIN_2800;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3200))
		val = SC6607_VSYSMIN_3000;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3400))
		val = SC6607_VSYSMIN_3200;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3500))
		val = SC6607_VSYSMIN_3400;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3600))
		val = SC6607_VSYSMIN_3500;
	else if (volt < SC6607_VSYSMIN_VOL_MV(3700))
		val = SC6607_VSYSMIN_3600;
	else
		val = SC6607_VSYSMIN_3700;

	ret = sc6607_write_byte(chip, SC6607_REG_VSYS_MIN, val);
	pr_info("vsys_min %d mv, val=0x%x\n", volt, val);

	return ret;
}

static int sc6607_enable_auto_dpdm(struct sc6607 *chip, bool enable)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_AUTO_INDET_EN, enable);
	pr_info("%s\n", enable ? "enable" : "disable");

	return ret;
}

static int sc6607_set_boost_voltage(struct sc6607 *chip, int volt)
{
	int ret;
	u8 val;

	if (!chip)
		return -EINVAL;

	if (volt < SC6607_BUCK_VBOOST_OFFSET)
		volt = SC6607_BUCK_VBOOST_OFFSET;

	val = (volt - SC6607_BUCK_VBOOST_OFFSET) / SC6607_BUCK_VBOOST_STEP;
	ret = sc6607_field_write(chip, F_VBOOST, val);
	pr_info("volt:%d, val=0x%x\n", volt, val);

	return ret;
}

static int sc6607_enable_ico(struct sc6607 *chip, bool enable)
{
	int ret;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_write(chip, F_ICO_EN, enable);
	pr_info("enable:%d\n", enable);

	return ret;
}

static int sc6607_read_idpm_limit(struct sc6607 *chip, int *icl)
{
	int ret;
	u8 val;

	if (!chip)
		return -EINVAL;

	ret = sc6607_field_read(chip, F_IINDPM_ICO, &val);
	if (ret < 0)
		return ret;

	*icl = (val * SC6607_BUCK_IINDPM_ICO_STEP) + SC6607_BUCK_IINDPM_ICO_OFFSET;

	return ret;
}

static int sc6607_check_charge_done(struct sc6607 *chip, bool *done)
{
	int ret;
	u8 val;

	if (!chip)
		return -EINVAL;

	*done = false;
	ret = sc6607_field_read(chip, F_CHG_STAT, &val);
	if (ret < 0)
		return ret;

	if (val == SC6607_TERM_CHARGE)
		*done = true;
	pr_info("done:%d, val:0x%x\n", *done, val);

	return ret;
}

static struct sc6607_platform_data *sc6607_parse_dt(struct device_node *np, struct sc6607 *chip)
{
	int ret;
	struct sc6607_platform_data *pdata;

	pdata = devm_kzalloc(chip->dev, sizeof(struct sc6607_platform_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

	if (of_property_read_string(np, "charger_name", &chip->chg_dev_name) < 0) {
		chip->chg_dev_name = "primary_chg";
		pr_warn("no charger name\n");
	}

	if (of_property_read_string(np, "eint_name", &chip->eint_name) < 0) {
		chip->eint_name = "chr_stat";
		pr_warn("no eint name\n");
	}

	ret = of_property_read_u32(np, "sc,vsys-limit", &pdata->vsyslim);
	if (ret) {
		pdata->vsyslim = 3500;
		pr_err("Failed to read node of sc,vsys-limit\n");
	}

	ret = of_property_read_u32(np, "sc,batsnc-enable", &pdata->batsns_en);
	if (ret) {
		pdata->batsns_en = 0;
		pr_err("Failed to read node of sc,batsnc-enable\n");
	}

	ret = of_property_read_u32(np, "sc,vbat", &pdata->vbat);
	if (ret) {
		pdata->vbat = 4450;
		pr_err("Failed to read node of sc,vbat\n");
	}

	ret = of_property_read_u32(np, "sc,charge-curr", &pdata->ichg);
	if (ret) {
		pdata->ichg = 2000;
		pr_err("Failed to read node of sc,charge-curr\n");
	}

	ret = of_property_read_u32(np, "sc,iindpm-disable", &pdata->iindpm_dis);
	if (ret) {
		pdata->iindpm_dis = 0;
		pr_err("Failed to read node of sc,iindpm-disable\n");
	}

	ret = of_property_read_u32(np, "sc,input-curr-limit", &pdata->iindpm);
	if (ret) {
		pdata->iindpm = 500;
		pr_err("Failed to read node of sc,input-curr-limit\n");
	}

	ret = of_property_read_u32(np, "sc,ico-enable", &pdata->ico_enable);
	if (ret) {
		pdata->ico_enable = 0;
		pr_err("Failed to read node of sc,ico-enable\n");
	}

	ret = of_property_read_u32(np, "sc,iindpm-ico", &pdata->iindpm_ico);
	if (ret) {
		pdata->iindpm_ico = 100;
		pr_err("Failed to read node of sc,iindpm-ico\n");
	}

	ret = of_property_read_u32(np, "sc,precharge-volt", &pdata->vprechg);
	if (ret) {
		pdata->vprechg = 0;
		pr_err("Failed to read node of sc,precharge-volt\n");
	}

	ret = of_property_read_u32(np, "sc,precharge-volt", &pdata->vprechg);
	if (ret) {
		pdata->vprechg = 0;
		pr_err("Failed to read node of sc,precharge-volt\n");
	}

	ret = of_property_read_u32(np, "sc,precharge-curr", &pdata->iprechg);
	if (ret) {
		pdata->iprechg = 500;
		pr_err("Failed to read node of sc,precharge-curr\n");
	}

	ret = of_property_read_u32(np, "sc,term-en", &pdata->iterm_en);
	if (ret) {
		pdata->iterm_en = 0;
		pr_err("Failed to read node of sc,term-en\n");
	}

	ret = of_property_read_u32(np, "sc,term-curr", &pdata->iterm);
	if (ret) {
		pdata->iterm = 0;
		pr_err("Failed to read node of sc,sc,term-curr\n");
	}

	ret = of_property_read_u32(np, "sc,rechg-dis", &pdata->rechg_dis);
	if (ret) {
		pdata->rechg_dis = 0;
		pr_err("Failed to read node of sc,rechg-dis\n");
	}

	ret = of_property_read_u32(np, "sc,rechg-dg", &pdata->rechg_dg);
	if (ret) {
		pdata->rechg_dg = 0;
		pr_err("Failed to read node of sc,rechg-dg\n");
	}

	ret = of_property_read_u32(np, "sc,rechg-volt", &pdata->rechg_volt);
	if (ret) {
		pdata->rechg_volt = 0;
		pr_err("Failed to read node of sc,rechg-volt\n");
	}

	ret = of_property_read_u32(np, "sc,boost-voltage", &pdata->vboost);
	if (ret) {
		pdata->vboost = 5000;
		pr_err("Failed to read node of sc,boost-voltage\n");
	}

	ret = of_property_read_u32(np, "sc,boost-max-current", &pdata->iboost);
	if (ret) {
		pdata->iboost = 1500;
		pr_err("Failed to read node of sc,boost-max-current\n");
	}

	ret = of_property_read_u32(np, "sc,conv-ocp-dis", &pdata->conv_ocp_dis);
	if (ret) {
		pdata->conv_ocp_dis = 0;
		pr_err("Failed to read node of sc,conv-ocp-dis\n");
	}

	ret = of_property_read_u32(np, "sc,tsbat-jeita-dis", &pdata->tsbat_jeita_dis);
	if (ret) {
		pdata->tsbat_jeita_dis = 1;
		pr_err("Failed to read node of sc,tsbat-jeita-dis\n");
	}

	ret = of_property_read_u32(np, "sc,ibat-ocp-dis", &pdata->ibat_ocp_dis);
	if (ret) {
		pdata->ibat_ocp_dis = 0;
		pr_err("Failed to read node of sc,ibat-ocp-dis\n");
	}

	ret = of_property_read_u32(np, "sc,vpmid-ovp-otg-dis", &pdata->vpmid_ovp_otg_dis);
	if (ret) {
		pdata->vpmid_ovp_otg_dis = 0;
		pr_err("Failed to read node of sc,vpmid-ovp-otg-dis\n");
	}

	ret = of_property_read_u32(np, "sc,vbat-ovp-buck-dis", &pdata->vbat_ovp_buck_dis);
	if (ret) {
		pdata->vbat_ovp_buck_dis = 0;
		pr_err("Failed to read node of sc,vbat-ovp-buck-dis\n");
	}

	ret = of_property_read_u32(np, "sc,ibat-ocp", &pdata->ibat_ocp);
	if (ret) {
		pdata->ibat_ocp = 1;
		pr_err("Failed to read node of sc,ibat-ocp\n");
	}

#ifdef OPLUS_FEATURE_CHG_BASIC
/********* workaround: Octavian needs to enable adc start *********/
	pdata->enable_adc = of_property_read_bool(np, "sc,enable-adc");
	pr_err("sc,enable-adc = %d\n", pdata->enable_adc);
/********* workaround: Octavian needs to enable adc end *********/
#endif
	return pdata;
}

static bool sc6607_check_rerun_detect_chg_type(struct sc6607 *chip, u8 type)
{
	if (!chip)
		return false;

	if (bc12_try_count == OPLUS_BC12_MAX_TRY_COUNT)
		chip->bc12.first_noti_sdp = false;

	if (chip->bc12.first_noti_sdp && (type == SC6607_VBUS_TYPE_SDP || type == SC6607_VBUS_TYPE_CDP)) {
		Charger_Detect_Init();
		sc6607_disable_hvdcp(chip);
		sc6607_force_dpdm(chip, true);
		pr_info("hw rerun bc12\n");
		return true;
	}
	chip->bc12_done = true;
	return false;
}


void sc6607_set_usb_props_type(enum power_supply_type type)
{
#ifndef CONFIG_OPLUS_CHARGER_MTK
	oplus_set_usb_props_type(type);
#endif
}

static int sc6607_get_charger_type(
	struct sc6607 *chip, enum power_supply_type *type)
{
	int ret;
	u8 vbus_stat;
	u8 hw_bc12_done = 0;
	enum power_supply_type oplus_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;

	if (!chip)
		return -EINVAL;

	if (!chip->soft_bc12) {
		ret = sc6607_field_read(chip, F_BC1_2_DONE, &hw_bc12_done);
		if (ret)
			return ret;

		if (hw_bc12_done) {
			ret = sc6607_field_read(chip, F_VBUS_STAT, &vbus_stat);
			if (ret)
				return ret;
			chip->vbus_type = vbus_stat;
			bc12_try_count++;
			pr_info("bc12_try_count[%d] reg_type:0x%x\n", bc12_try_count, vbus_stat);
		} else {
			pr_err("hw_bc12_done not complete\n");
			return ret;
		}

		if (sc6607_check_rerun_detect_chg_type(chip, vbus_stat))
			return ret;

		chip->usb_connect_start = true;
	} else {
		chip->vbus_type = sc6607_charger_get_soft_bc12_type(chip);
		pr_info("soft_bc12_type:0x%x\n", chip->vbus_type);
	}

	switch (chip->vbus_type) {
	case SC6607_VBUS_TYPE_OTG:
	case SC6607_VBUS_TYPE_NONE:
		chip->chg_type = CHARGER_UNKNOWN;
		oplus_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case SC6607_VBUS_TYPE_SDP:
		if (oplus_check_pd_usb_type() == PORT_PD_WITHOUT_USB) {
			pr_err("pd without usb_comm,force sdp to dcp\n");
			chip->chg_type = STANDARD_CHARGER;
			oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		} else {
			chip->chg_type = STANDARD_HOST;
			oplus_chg_type = POWER_SUPPLY_TYPE_USB;
		}
		break;
	case SC6607_VBUS_TYPE_CDP:
		if (oplus_check_pd_usb_type() == PORT_PD_WITHOUT_USB) {
			pr_err("pd without usb_comm,force cdp to dcp\n");
			chip->chg_type = STANDARD_CHARGER;
			oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		} else {
			chip->chg_type = CHARGING_HOST;
			oplus_chg_type = POWER_SUPPLY_TYPE_USB_CDP;
		}
		break;
	case SC6607_VBUS_TYPE_DCP:
		chip->chg_type = STANDARD_CHARGER;
		oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	case SC6607_VBUS_TYPE_HVDCP:
		chip->chg_type = STANDARD_CHARGER;
		oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	case SC6607_VBUS_TYPE_UNKNOWN:
		chip->chg_type = STANDARD_CHARGER;
		oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	case SC6607_VBUS_TYPE_NON_STD:
		chip->chg_type = STANDARD_CHARGER;
		oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	default:
		chip->chg_type = STANDARD_CHARGER;
		oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	}

	*type = oplus_chg_type;
	sc6607_set_usb_props_type(chip->oplus_chg_type);

	return 0;
}

static int sc6607_inform_charger_type(struct sc6607 *chip)
{
	int ret = 0;
	union power_supply_propval propval;

	if (!chip->psy) {
		chip->psy = power_supply_get_by_name("charger");
		if (!chip->psy)
			return -ENODEV;
	}

	propval.intval = chip->power_good;

	ret = power_supply_set_property(chip->psy, POWER_SUPPLY_PROP_ONLINE,
			&propval);

	if (ret < 0)
		pr_err("inform power supply online failed:%d\n", ret);

	propval.intval = chip->chg_type;

	ret = power_supply_set_property(chip->psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE,
			&propval);

	if (ret < 0)
		pr_err("inform power supply charge type failed:%d\n", ret);

	oplus_chg_wake_update_work();
	power_supply_changed(chip->psy);
	return ret;
}

static int sc6607_request_dpdm(struct sc6607 *chip, bool enable)
{
	int ret = 0;

	if (!chip) {
		pr_err(" chip is null");
		return -EINVAL;
	}

	if (!chip->dpdm_reg && of_get_property(chip->dev->of_node, "dpdm-supply", NULL)) {
		chip->dpdm_reg = devm_regulator_get(chip->dev, "dpdm");
		if (IS_ERR(chip->dpdm_reg)) {
			ret = PTR_ERR(chip->dpdm_reg);
			pr_err("fail get dpdm regulator ret=%d\n", ret);
			chip->dpdm_reg = NULL;
			return ret;
		}
	}

	mutex_lock(&chip->dpdm_lock);
	if (enable) {
		if (chip->dpdm_reg && !chip->dpdm_enabled) {
			pr_err("enabling dpdm regulator\n");
			ret = regulator_enable(chip->dpdm_reg);
			if (ret < 0)
				pr_err("success enable dpdm regulator ret=%d\n", ret);
			else
				chip->dpdm_enabled = true;
		}
	} else {
		if (chip->dpdm_reg && chip->dpdm_enabled) {
			pr_err("disabling dpdm regulator\n");
			ret = regulator_disable(chip->dpdm_reg);
			if (ret < 0)
				pr_err("fail disable dpdm regulator ret=%d\n", ret);
			else
				chip->dpdm_enabled = false;
		}
	}
	mutex_unlock(&chip->dpdm_lock);

	pr_info("dpdm regulator: enable= %d, ret=%d\n", enable, ret);
	return ret;
}

static void oplus_chg_awake_init(struct sc6607 *chip)
{
	if (!chip) {
		pr_err("chip is null\n");
		return;
	}

	chip->suspend_ws = wakeup_source_register(NULL, "split chg wakelock");
}

static void oplus_chg_wakelock(struct sc6607 *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->suspend_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->suspend_ws);
		pr_info("true\n");
	} else if (!awake && pm_flag) {
		__pm_relax(chip->suspend_ws);
		pm_flag = false;
		pr_info("false\n");
	}
}

static void oplus_keep_resume_awake_init(struct sc6607 *chip)
{
	if (!chip) {
		pr_err("chip is null\n");
		return;
	}

	chip->keep_resume_ws = wakeup_source_register(NULL, "split_chg_keep_resume");
}

static void oplus_keep_resume_wakelock(struct sc6607 *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->keep_resume_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->keep_resume_ws);
		pr_info("true\n", __func__);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->keep_resume_ws);
		pm_flag = false;
		pr_info("false\n", __func__);
	}
}

static void oplus_sc6607_set_mivr_by_battery_vol(void)
{
	u32 mV = 0;
	int vbatt = 0;
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();

	if (chip)
		vbatt = chip->batt_volt;

	if (vbatt > SC6607_VINDPM_VBAT_PHASE1)
		mV = vbatt + SC6607_VINDPM_THRES_PHASE1;
	else if (vbatt > SC6607_VINDPM_VBAT_PHASE2)
		mV = vbatt + SC6607_VINDPM_THRES_PHASE2;
	else
		mV = vbatt + SC6607_VINDPM_THRES_PHASE3;

	if (mV < SC6607_VINDPM_THRES_MIN)
		mV = SC6607_VINDPM_THRES_MIN;

	sc6607_set_input_volt_limit(g_chip, mV);
}

static int sc6607_hk_irq_handle(struct sc6607 *chip)
{
	int ret;
	u8 val[2];
	bool prev_pg = false;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!chip || !chg_chip)
		return -EINVAL;

	pr_info("!!!enter\n");
	if (atomic_read(&chip->driver_suspended) == 1) {
		pr_err("suspended and wait %d ms\n", SC6607_WAIT_RESUME_TIME);
	}

	ret = sc6607_read_byte(chip, SC6607_REG_HK_INT_STAT, &val[0]);
	if (ret) {
		pr_err("read hk int stat reg failed\n");
		return -EINVAL;
	}

	ret = sc6607_read_byte(chip, SC6607_REG_CHG_INT_STAT, &val[1]);
	if (ret) {
		pr_err("read chg int stat reg failed\n");
		return -EINVAL;
	}

	prev_pg = chip->power_good;
	chip->power_good = (!!(val[0] & SC6607_HK_VAC_PRESENT_MASK)) && (!!(val[0] & SC6607_HK_VBUS_PRESENT_MASK));
	pr_info("prev_pg:%d, now_pg:%d, val[0]=0x%x, val[1]=0x%x\n", prev_pg, chip->power_good, val[0], val[1]);
	if (chip->power_good)
		oplus_chg_wakelock(chip, true);

	oplus_chg_check_break(chip->power_good);
	oplus_sc6607_set_mivr_by_battery_vol();

	if ((oplus_vooc_get_fastchg_started() && oplus_vooc_get_adapter_update_status() != 1) || chg_chip->camera_on) {
		pr_err("fastchg_started or camera_on\n");
		goto out;
	}

	sc6607_dump_regs(chip);

	if ((!prev_pg && chip->power_good) || chip->wd_rerun_detect) {
		pr_info("!!!plugin, is_prswap[%d], camera_on[%d]\n", oplus_is_prswap, chg_chip->camera_on);
		if (!chg_chip->camera_on)
			chip->request_otg = 0;
		chip->wd_rerun_detect = false;
		oplus_chg_track_check_wired_charging_break(chip->power_good);
		oplus_chg_wakelock(chip, true);
		oplus_chg_set_charger_type_unknown();

		sc6607_field_write(chip, F_IBATOCP, chip->platform_data->ibat_ocp);
		sc6607_field_write(chip, F_CONV_OCP_DIS, chip->platform_data->conv_ocp_dis);
		sc6607_field_write(chip, F_RECHG_DIS, chip->platform_data->rechg_dis);
		sc6607_field_write(chip, F_CHG_TIMER, 0x03);
		sc6607_field_write(chip, F_ACDRV_MANUAL_PRE, 3);
		if (chip->chip_id == SC6607_1P0_CHIP_ID) {
			sc6607_field_write(chip, F_TSBUS_TSBAT_FLT_DIS, true);
			sc6607_field_write(chip, F_TSBAT_JEITA_DIS, true);
		} else {
			sc6607_field_write(chip, F_TSBUS_TSBAT_FLT_DIS, false);
			sc6607_field_write(chip, F_TSBAT_JEITA_DIS, false);
		}
		sc6607_field_write(chip, F_ADC_EN, 1);
		sc6607_field_write(chip, F_VBUS_PD, 0);
		sc6607_enable_enlim(chip);
		/*sc6607_field_write(chip, F_ACDRV_MANUAL_EN, 0);*/
		sc6607_set_input_current_limit(chip, SC6607_DEFAULT_IBUS_MA);
		if (atomic_read(&chip->charger_suspended))
			oplus_sc6607_charger_suspend();
		sc6607_inform_charger_type(chip);
		if (oplus_is_prswap) {
			chip->chg_type = CHARGER_UNKNOWN;
			chip->oplus_chg_type = POWER_SUPPLY_TYPE_USB;
			sc6607_set_usb_props_type(chip->oplus_chg_type);
			chip->pd_sdp_port = true;
			oplus_is_prswap = false;
			return 0;
		}

		if (chip->is_force_dpdm) {
			Charger_Detect_Init();
			chip->is_force_dpdm = false;
			sc6607_force_dpdm(chip, false);
		} else {
			if (chip->oplus_chg_type == POWER_SUPPLY_TYPE_UNKNOWN) {
				Charger_Detect_Init();
				sc6607_disable_hvdcp(chip);
				chip->bc12.first_noti_sdp = true;
				chip->bc12_done = false;
				bc12_try_count = 0;
				if (chip->soft_bc12)
					bc12_detect_run(chip);
				else
					schedule_delayed_work(&chip->hw_bc12_detect_work, msecs_to_jiffies(300));
			}
		}
		oplus_wake_up_usbtemp_thread();
	} else if (prev_pg && !chip->power_good) {
#ifdef OPLUS_FEATURE_CHG_BASIC
/********* workaround: Octavian needs to enable adc start *********/
		if ((chip->platform_data->enable_adc == true) && (oplus_is_rf_ftm_mode() == false))
			pr_debug("Do not disable adc\n");
		else
/********* workaround: Octavian needs to enable adc end *********/
#endif
			sc6607_field_write(chip, F_ADC_EN, 0);
		sc6607_field_write(chip, F_VBUS_PD, 1);
		if (chip->soft_bc12) {
			bc12_set_dp_state(chip, DPDM_HIZ);
			bc12_set_dm_state(chip, DPDM_HIZ);
		}
		oplus_chg_track_check_wired_charging_break(chip->power_good);
		sc6607_set_input_current_limit(chip, SC6607_DEFAULT_IBUS_MA);
		oplus_wake_up_usbtemp_thread();
		if (oplus_chg_get_voocphy_support() == AP_SINGLE_CP_VOOCPHY ||
		    oplus_chg_get_voocphy_support() == AP_DUAL_CP_VOOCPHY) {
			oplus_voocphy_adapter_plugout_handler();
		}

		mutex_lock(&chip->bc12.running_lock);
		chip->bc12.detect_ing = false;
		mutex_unlock(&chip->bc12.running_lock);
		chip->usb_connect_start = false;
		chip->is_force_dpdm = false;
		chip->soft_bc12_type = SC6607_VBUS_TYPE_NONE;
		chip->chg_type = CHARGER_UNKNOWN;
		chip->oplus_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
		chip->pd_sdp_port = false;
		sc6607_set_usb_props_type(chip->oplus_chg_type);
#ifdef CONFIG_OPLUS_CHARGER_MTK
		oplus_chg_pullup_dp_set(false);
#endif
		if (chg_chip)
			chg_chip->pd_chging = false;

		if (!oplus_vooc_get_fastchg_started())
			sc6607_inform_charger_type(chip);
		sc6607_enable_enlim(chip);
		sc6607_field_write(chip, F_ACDRV_MANUAL_PRE, 3);
		/*sc6607_field_write(chip, F_ACDRV_MANUAL_EN, 0);*/
		sc6607_disable_hvdcp(chip);
		Charger_Detect_Release();
		oplus_enable_device_mode(false);
		if (chip->soft_bc12)
			cancel_delayed_work_sync(&chip->bc12.detect_work);
		else
			cancel_delayed_work_sync(&chip->hw_bc12_detect_work);
		pr_info("adapter/usb removed.");
		oplus_chg_wakelock(chip, false);
	} else {
		pr_info("!!!not handle: prev_pg and now_pg same \n");
	}

out:
	return 0;
}

static int sc6607_dpdm_irq_handle(struct sc6607 *chip)
{
	int ret;
	static int prev_vbus_type = SC6607_VBUS_TYPE_NONE;
	enum power_supply_type prev_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
	enum power_supply_type cur_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;

	if (!chip)
		return -EINVAL;

	prev_chg_type = chip->oplus_chg_type;
	ret = sc6607_get_charger_type(chip, &cur_chg_type);

	if (!chip->soft_bc12)
		pr_info("bc12_try_count [%d] : prev_vbus_type[%d] --> cur_vbus_type[%d]\n",
				bc12_try_count, prev_vbus_type, chip->vbus_type);
	if (!chip->bc12_done) {
		prev_vbus_type = chip->vbus_type;
		return 0;
	}
	prev_vbus_type = SC6607_VBUS_TYPE_NONE;
	pr_info("prev_chg_type[%d] --> cur_chg_type[%d]\n", prev_chg_type, cur_chg_type);

	if ((prev_chg_type == POWER_SUPPLY_TYPE_USB_DCP) &&
	    ((cur_chg_type == POWER_SUPPLY_TYPE_USB) || (cur_chg_type == POWER_SUPPLY_TYPE_USB_CDP))) {
		pr_info("!!!keep prev chg type\n");
		cur_chg_type = prev_chg_type;
	}

	if ((cur_chg_type == POWER_SUPPLY_TYPE_USB) || (cur_chg_type == POWER_SUPPLY_TYPE_USB_CDP)) {
		pr_info("usb_connect_start:%d\n", chip->usb_connect_start);
		chip->oplus_chg_type = cur_chg_type;
		sc6607_set_usb_props_type(chip->oplus_chg_type);
		bc12_set_dm_state(chip, DPDM_HIZ);
		bc12_set_dp_state(chip, DPDM_HIZ);
		oplus_enable_device_mode(true);
		if (chip->usb_connect_start) {
			sc6607_inform_charger_type(chip);
		}
	} else if (chip->oplus_chg_type == POWER_SUPPLY_TYPE_UNKNOWN && cur_chg_type != POWER_SUPPLY_TYPE_UNKNOWN) {
		pr_info("cur_chg_type = %d, vbus_type = %d\n", cur_chg_type, chip->vbus_type);
		chip->oplus_chg_type = cur_chg_type;
		sc6607_set_usb_props_type(chip->oplus_chg_type);
		sc6607_inform_charger_type(chip);
	} else {
		pr_info("oplus_chg_type = %d, vbus_type = %d", chip->oplus_chg_type, chip->vbus_type);
	}

#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (cur_chg_type == POWER_SUPPLY_TYPE_USB_CDP) {
		Charger_Detect_Release();
		oplus_chg_pullup_dp_set(true);
		return 0;
	} else {
		oplus_chg_pullup_dp_set(false);
	}
	Charger_Detect_Release();
#endif
	return 0;
}

__maybe_unused static int sc6607_mask_hk_irq(struct sc6607 *chip, int irq_channel)
{
	int ret = 0;
	u8 val = 0;

	ret = sc6607_read_byte(chip, SC6607_REG_HK_INT_MASK, &val);
	if (ret < 0)
		return ret;

	val |= irq_channel;
	return sc6607_write_byte(chip, SC6607_REG_HK_INT_MASK, val);
}

__maybe_unused static int sc6607_unmask_hk_irq(struct sc6607 *chip, int irq_channel)
{
	int ret;
	u8 val = 0;

	ret = sc6607_read_byte(chip, SC6607_REG_HK_INT_MASK, &val);
	if (ret < 0)
		return ret;

	val &= ~irq_channel;
	return sc6607_write_byte(chip, SC6607_REG_HK_INT_MASK, val);
}

__maybe_unused static int sc6607_mask_buck_irq(struct sc6607 *chip, int irq_channel)
{
	int ret;
	u8 val[3] = { 0 };

	ret = sc6607_bulk_read(chip, SC6607_REG_CHG_INT_MASK, val, 3);
	if (ret < 0)
		return ret;

	val[0] |= irq_channel;
	val[1] |= irq_channel >> 8;
	val[2] |= irq_channel >> 16;

	return sc6607_bulk_write(chip, SC6607_REG_CHG_INT_MASK, val, 3);
}

__maybe_unused static int sc6607_unmask_buck_irq(struct sc6607 *chip, int irq_channel)
{
	int ret;
	u8 val[3] = { 0 };

	ret = sc6607_bulk_read(chip, SC6607_REG_CHG_INT_MASK, val, 3);
	if (ret < 0)
		return ret;

	val[0] &= ~(irq_channel);
	val[1] &= ~(irq_channel >> 8);
	val[2] &= ~(irq_channel >> 16);

	return sc6607_bulk_write(chip, SC6607_REG_CHG_INT_MASK, val, 3);
}

static int sc6607_vooc_irq_handle(struct sc6607 *chip)
{
	if (!oplus_voocphy_mg)
		return 0;

	oplus_voocphy_interrupt_handler(oplus_voocphy_mg);

	return 0;
}

static int sc6607_ufcs_irq_handle(struct sc6607 *chip)
{
	return 0;
}

static int sc6607_buck_irq_handle(struct sc6607 *chip)
{
	return 0;
}

static int sc6607_check_wd_timeout_fault(struct sc6607 *chip)
{
	int ret;
	u8 val;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!chg_chip)
		return -EINVAL;

	ret = sc6607_read_byte(chip, SC6607_REG_HK_FLT_FLG, &val);
	if (ret < 0) {
		pr_err("read reg 0x%x failed\n", SC6607_REG_HK_FLT_FLG);
		return ret;
	}

	if (val & SC6607_HK_WD_TIMEOUT_MASK) {
		pr_err("wd timeout happened\n");
		chip->wd_rerun_detect = true;
		sc6607_init_device(chip);
		chg_chip->charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sc6607_hk_irq_handle(chip);
	}
	sc6607_track_match_hk_err(chip, val);

	return 0;
}

static int sc6607_cp_irq_handle(struct sc6607 *chip)
{
	return 0;
}

static int sc6607_led_irq_handle(struct sc6607 *chip)
{
	return 0;
}

static const struct sc6607_alert_handler alert_handler[] = {
	DECL_ALERT_HANDLER(UFCS_FLAG, sc6607_ufcs_irq_handle),
	DECL_ALERT_HANDLER(VOOC_FLAG, sc6607_vooc_irq_handle),
	DECL_ALERT_HANDLER(HK_FLAG, sc6607_hk_irq_handle),
	DECL_ALERT_HANDLER(BUCK_CHARGER_FLAG, sc6607_buck_irq_handle),
	DECL_ALERT_HANDLER(CHARGER_PUMP_FLAG, sc6607_cp_irq_handle),
	DECL_ALERT_HANDLER(DPDM_FLAG, sc6607_dpdm_irq_handle),
	DECL_ALERT_HANDLER(LED_FLAG, sc6607_led_irq_handle),
};

static irqreturn_t sc6607_irq_handler(int irq, void *data)
{
	int ret;
	u8 val;
	int i;
	struct sc6607 *chip = (struct sc6607 *)data;

	if (!chip) {
		pr_err("!!!chip null\n");
		return IRQ_HANDLED;
	}

	oplus_keep_resume_wakelock(chip, true);

	ret = sc6607_read_byte(chip, SC6607_REG_HK_GEN_FLG, &val);
	if (ret < 0) {
		pr_err("read reg 0x%x failed\n", SC6607_REG_HK_GEN_FLG);
		goto irq_out;
	}

	pr_info("!!!hk_gen_flg:0x%x\n", val);
	val |= BIT(HK_FLAG);

	for (i = 0; i < ARRAY_SIZE(alert_handler); i++) {
		if ((alert_handler[i].bit_mask & val) && alert_handler[i].handler != NULL) {
			alert_handler[i].handler(chip);
		}
	}

	sc6607_check_wd_timeout_fault(chip);

irq_out:
	oplus_keep_resume_wakelock(chip, false);
	return IRQ_HANDLED;
}

static int sc6607_check_chg_plugin(void)
{
	return sc6607_irq_handler(0, g_chip);
}

static int oplus_chg_irq_gpio_init(struct sc6607 *chip)
{
	int rc = 0;
	struct device_node *node = chip->dev->of_node;

	if (!node) {
		pr_err("device tree node missing\n");
		return -EINVAL;
	}

	chip->irq_gpio = of_get_named_gpio(node, "qcom,chg_irq_gpio", 0);
	if (!gpio_is_valid(chip->irq_gpio)) {
		pr_err("sy->irq_gpio not specified\n");
		return -EINVAL;
	}

	rc = gpio_request(chip->irq_gpio, "chg_irq_gpio");
	if (rc) {
		pr_err("unable to request gpio [%d]\n", chip->irq_gpio);
		return -EINVAL;
	}

	chip->irq = gpio_to_irq(chip->irq_gpio);

	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		pr_err("get pinctrl fail\n");
		return -EINVAL;
	}

	chip->splitchg_inter_active = pinctrl_lookup_state(chip->pinctrl, "splitchg_inter_active");
	if (IS_ERR_OR_NULL(chip->splitchg_inter_active)) {
		pr_err(": Failed to get the active state pinctrl handle\n");
		return -EINVAL;
	}

	chip->splitchg_inter_sleep = pinctrl_lookup_state(chip->pinctrl, "splitchg_inter_sleep");
	if (IS_ERR_OR_NULL(chip->splitchg_inter_sleep)) {
		pr_err(" Failed to get the sleep state pinctrl handle\n");
		return -EINVAL;
	}

	gpio_direction_input(chip->irq_gpio);
	pinctrl_select_state(chip->pinctrl, chip->splitchg_inter_active);
	rc = gpio_get_value(chip->irq_gpio);

	pr_info("irq_gpio:%d, level = %d\n", chip->irq_gpio, rc);
	return 0;
}

static int sc6607_register_interrupt(struct device_node *np, struct sc6607 *chip)
{
	int ret = 0;

	ret = oplus_chg_irq_gpio_init(chip);
	if (ret) {
		return ret;
	}

	ret = devm_request_threaded_irq(chip->dev, chip->irq, NULL, sc6607_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT, chip->eint_name, chip);
	if (ret < 0) {
		pr_err("request thread irq failed:%d\n", ret);
		return ret;
	}

	enable_irq_wake(chip->irq);

	return 0;
}

static int sc6607_hk_set_acdrv(struct sc6607 *chip, bool en)
{
	int ret;
	int cnt = 0;
	u8 from_ic;
	do {
		if (cnt++ > 3)
			pr_err("set acdrv failed\n");

		ret = sc6607_field_read(chip, F_ACDRV_EN, &from_ic);
		if (ret < 0)
			continue;

		ret = sc6607_field_write(chip, F_ACDRV_EN, en);
		if (ret < 0)
			continue;

		ret = sc6607_field_read(chip, F_ACDRV_EN, &from_ic);
		if (ret < 0)
			continue;
	} while (en != from_ic);

	return 0;
}

static int oplus_sc6607_hk_set_acdrv(bool en)
{
	int ret = 0;

	if (!g_chip)
		return -EINVAL;

	if (0) {
		pr_info("en = %d\n", en);
		sc6607_hk_set_acdrv(g_chip, 1);
	}
	return ret;
}

#ifdef CONFIG_OPLUS_CHARGER_MTK
static int sc6607_plug_in(struct charger_device *chg_dev)
{
	int ret = 0;
	struct sc6607 *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc6607_enable_charger(chip);
	if (ret) {
		chg_err("Failed to enable charging:%d", ret);
	}
	return ret;
}

static int sc6607_plug_out(struct charger_device *chg_dev)
{
	int ret = 0;
	struct sc6607 *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc6607_disable_charger(chip);

	if (ret) {
		chg_err("Failed to disable charging:%d", ret);
	}
	return ret;
}

static int sc6607_charge_enable(struct charger_device *chg_dev, bool en)
{
	struct sc6607 *chip = dev_get_drvdata(&chg_dev->dev);

	if (en)
		return sc6607_enable_charger(chip);
	else
		return sc6607_disable_charger(chip);
}

static int sc6607_kick_wdt(struct charger_device *chg_dev)
{
	struct sc6607 *chip = dev_get_drvdata(&chg_dev->dev);

	return sc6607_reset_watchdog_timer(chip);
}
#endif

static int sc6607_init_default(struct sc6607 *chip)
{
	u8 val[3] = { 0 };
	int ret;

	if (!chip)
		return -EINVAL;

	if (chip->chip_id == SC6607_1P0_CHIP_ID)
		ret = sc6607_field_write(chip, F_VAC_OVP, 0x02);
	else
		ret = sc6607_field_write(chip, F_VAC_OVP, 0x00);
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 0x02);
	ret |= sc6607_field_write(chip, F_CHG_TIMER, 0x03);
	ret |= sc6607_field_write(chip, F_ACDRV_MANUAL_PRE, 3);

	oplus_sc6607_hk_set_acdrv(true);

	val[0] = 0;
	if (chip->chip_id == SC6607_1P0_CHIP_ID)
		val[1] = 0x0;
	else
		val[1] = 0x04;
	ret |= sc6607_bulk_write(chip, SC6607_REG_HK_ADC_CTRL, val, 2);

	ret |= sc6607_enable_ico(chip, chip->platform_data->ico_enable);
	ret |= sc6607_field_write(chip, F_EDL_TSBUS_SEL, true);
	ret |= sc6607_field_write(chip, F_RECHG_DIS, chip->platform_data->rechg_dis);
	ret |= sc6607_field_write(chip, F_TERM_EN, chip->platform_data->iterm_en);
	ret |= sc6607_field_write(chip, F_CONV_OCP_DIS, chip->platform_data->conv_ocp_dis);
	if (chip->chip_id == SC6607_1P0_CHIP_ID) {
		ret |= sc6607_field_write(chip, F_TSBUS_TSBAT_FLT_DIS, true);
		ret |= sc6607_field_write(chip, F_TSBAT_JEITA_DIS, true);
	} else {
		ret |= sc6607_field_write(chip, F_TSBUS_TSBAT_FLT_DIS, false);
		ret |= sc6607_field_write(chip, F_TSBAT_JEITA_DIS, false);
	}
	ret |= sc6607_field_write(chip, F_VPMID_OVP_OTG_DIS, chip->platform_data->vpmid_ovp_otg_dis);
	ret |= sc6607_field_write(chip, F_VBAT_OVP_BUCK_DIS, chip->platform_data->vbat_ovp_buck_dis);
	ret |= sc6607_field_write(chip, F_IBATOCP, chip->platform_data->ibat_ocp);
#ifdef OPLUS_FEATURE_CHG_BASIC
/********* workaround: Octavian needs to enable adc start *********/
	if ((chip->platform_data->enable_adc == true) && (oplus_is_rf_ftm_mode() == false))
		ret |= sc6607_field_write(chip, F_ADC_EN, 1);
/********* workaround: Octavian needs to enable adc end *********/
#endif
	if (ret < 0)
		pr_info("fail\n");

	return ret;
}

static int sc6607_init_device(struct sc6607 *chip)
{
	int ret = 0;

	chip->is_force_dpdm = false;

	if (chip->chip_id != SC6607_1P1_CHIP_ID)
		chip->soft_bc12 = true;
	else
		chip->soft_bc12 = false;

	chip->bc12.first_noti_sdp = true;
	chip->bc12_done = false;
	bc12_try_count = 0;

	sc6607_disable_watchdog_timer(chip);

	ret = sc6607_set_prechg_current(chip, chip->platform_data->iprechg);
	if (ret)
		pr_err("Failed to set prechg current, ret = %d\n", ret);

	ret = sc6607_set_chargevolt(chip, chip->platform_data->vbat);
	if (ret)
		pr_err("Failed to set default cv, ret = %d\n", ret);

	ret = sc6607_set_term_current(chip, chip->platform_data->iterm);
	if (ret)
		pr_err("Failed to set termination current, ret = %d\n", ret);

	ret = sc6607_set_boost_voltage(chip, chip->platform_data->vboost);
	if (ret)
		pr_err("Failed to set boost voltage, ret = %d\n", ret);

	ret = sc6607_set_boost_current(chip, chip->platform_data->iboost);
	if (ret)
		pr_err("Failed to set boost current, ret = %d\n", ret);

	ret = sc6607_enable_enlim(chip);
	if (ret)
		pr_err("Failed to set enlim, ret = %d\n", ret);

	ret = sc6607_enable_auto_dpdm(chip, false);
	if (ret)
		pr_err("Failed to set auto dpdm, ret = %d\n", ret);

	ret = sc6607_vmin_limit(chip, chip->platform_data->vsyslim);
	if (ret)
		pr_err("Failed to set vmin limit, ret = %d\n", ret);

	ret = sc6607_set_input_volt_limit(chip, SC6607_HW_AICL_POINT_VOL_5V_PHASE1);
	if (ret)
		pr_err("Failed to set input volt limit, ret = %d\n", ret);

	ret = sc6607_set_input_current_limit(chip, chip->platform_data->iindpm);
	if (ret)
		pr_err("Failed to set input current limit, ret = %d\n", ret);

	ret = oplus_sc6607_set_ichg(chip->platform_data->ichg);
	if (ret)
		pr_err("Failed to set input current limit, ret = %d\n", ret);

	ret |= sc6607_mask_hk_irq(chip, SC6607_HK_RESET_MASK | SC6607_HK_ADC_DONE_MASK | SC6607_HK_REGN_OK_MASK |
						SC6607_HK_VAC_PRESENT_MASK);

	ret |= sc6607_mask_buck_irq(chip, SC6607_BUCK_ICO_MASK | SC6607_BUCK_IINDPM_MASK | SC6607_BUCK_VINDPM_MASK |
						  SC6607_BUCK_CHG_MASK | SC6607_BUCK_QB_ON_MASK |
						  SC6607_BUCK_VSYSMIN_MASK);

	ret |= sc6607_init_default(chip);

	return ret;
}

static void sc6607_dump_regs(struct sc6607 *chip)
{
	int addr;
	u8 val[128];
	int ret;
	char buf[1024];
	char *s;
	int index;

	if (!chip || !sc6607_chg_dbg_enable)
		return;

	memset(buf, 0, sizeof(buf));
	s = buf;
	index = 0;
	ret = sc6607_read_data(chip, SC6607_REG_DEVICE_ID, val, (SC6607_REG_HK_TSBAT_ADC - SC6607_REG_DEVICE_ID));
	if (ret < 0)
		return;
	s += sprintf(s, "hk_regs:");
	for (addr = SC6607_REG_DEVICE_ID; addr < SC6607_REG_HK_TSBAT_ADC; addr++) {
		s += sprintf(s, "[0x%.2x,0x%.2x]", addr, val[index]);
		index++;
		if (index == sizeof(val))
			break;
	}
	s += sprintf(s, "\n");
	pr_info("%s \n", buf);

	memset(buf, 0, sizeof(buf));
	s = buf;
	index = 0;
	ret = sc6607_read_data(chip, SC6607_REG_VSYS_MIN, val, (SC6607_REG_BUCK_MASK - SC6607_REG_VSYS_MIN));
	if (ret < 0)
		return;
	s += sprintf(s, "buck_regs:");
	for (addr = SC6607_REG_VSYS_MIN; addr < SC6607_REG_BUCK_MASK; addr++) {
		s += sprintf(s, "[0x%.2x,0x%.2x]", addr, val[index]);
		index++;
		if (index == sizeof(val))
			break;
	}
	s += sprintf(s, "\n");
	pr_info("%s \n", buf);

	memset(buf, 0, sizeof(buf));
	s = buf;
	index = 0;
	ret = sc6607_read_data(chip, SC6607_REG_DPDM_EN, val, (SC6607_REG_DPDM_NONSTD_STAT - SC6607_REG_DPDM_EN));
	if (ret < 0)
		return;
	s += sprintf(s, "dpdm_regs:");
	for (addr = SC6607_REG_DPDM_EN; addr < SC6607_REG_DPDM_NONSTD_STAT; addr++) {
		s += sprintf(s, "[0x%.2x,0x%.2x]", addr, val[index]);
		index++;
		if (index == sizeof(val))
			break;
	}
	s += sprintf(s, "\n");
	pr_info("%s \n", buf);

	memset(buf, 0, sizeof(buf));
	s = buf;
	index = 0;
	ret = sc6607_read_data(chip, 0x60, val, (0x6f - 0x60));
	if (ret < 0)
		return;
	s += sprintf(s, "cp_regs:");
	for (addr = 0x60; addr < 0x6f; addr++) {
		s += sprintf(s, "[0x%.2x,0x%.2x]", addr, val[index]);
		index++;
		if (index == sizeof(val))
			break;
	}
	s += sprintf(s, "\n");
	pr_info("%s \n", buf);
}

static ssize_t sc6607_charger_show_registers(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sc6607 *chip = g_chip;
	int addr;
	u8 val[128];
	int ret;
	u8 tmpbuf[1024];
	int idx = 0;
	int index;
	int len;

	if (!chip)
		return idx;

	ret = sc6607_read_data(chip, SC6607_REG_DEVICE_ID, val, (SC6607_REG_HK_TSBAT_ADC - SC6607_REG_DEVICE_ID));
	if (ret < 0)
		return idx;
	index = 0;
	for (addr = SC6607_REG_DEVICE_ID; addr < SC6607_REG_HK_TSBAT_ADC; addr++) {
		len = snprintf(tmpbuf, PAGE_SIZE - idx, "[%.2X]=0x%.2x\n", addr, val[index]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
		index++;
		if (index >= sizeof(val))
			break;
	}

	ret = sc6607_read_data(chip, SC6607_REG_VSYS_MIN, val, (SC6607_REG_BUCK_MASK - SC6607_REG_VSYS_MIN));
	if (ret < 0)
		return idx;
	index = 0;
	for (addr = SC6607_REG_VSYS_MIN; addr < SC6607_REG_BUCK_MASK; addr++) {
		len = snprintf(tmpbuf, PAGE_SIZE - idx, "[%.2X]=0x%.2x\n", addr, val[index]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
		index++;
		if (index >= sizeof(val))
			break;
	}

	ret = sc6607_read_data(chip, SC6607_REG_VBATSNS_OVP, val, (SC6607_REG_CP_FLT_DIS - SC6607_REG_VBATSNS_OVP));
	if (ret < 0)
		return idx;
	index = 0;
	for (addr = SC6607_REG_VBATSNS_OVP; addr < SC6607_REG_CP_FLT_DIS; addr++) {
		len = snprintf(tmpbuf, PAGE_SIZE - idx, "[%.2X]=0x%.2x\n", addr, val[index]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
		index++;
		if (index >= sizeof(val))
			break;
	}

	ret = sc6607_read_data(chip, SC6607_REG_DPDM_EN, val, (SC6607_REG_DPDM_NONSTD_STAT - SC6607_REG_DPDM_EN));
	if (ret < 0)
		return idx;
	index = 0;
	for (addr = SC6607_REG_DPDM_EN; addr < SC6607_REG_DPDM_NONSTD_STAT; addr++) {
		len = snprintf(tmpbuf, PAGE_SIZE - idx, "[%.2X]=0x%.2x\n", addr, val[index]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
		index++;
		if (index >= sizeof(val))
			break;
	}
	ret = sc6607_read_data(chip, SC6607_REG_PHY_CTRL, val, (SC6607_REG_DP_HOLD_TIME - SC6607_REG_PHY_CTRL));
	if (ret < 0)
		return idx;
	index = 0;
	for (addr = SC6607_REG_PHY_CTRL; addr < SC6607_REG_DP_HOLD_TIME; addr++) {
		len = snprintf(tmpbuf, PAGE_SIZE - idx, "[%.2X]=0x%.2x\n", addr, val[index]);
		memcpy(&buf[idx], tmpbuf, len);
		idx += len;
		index++;
		if (index >= sizeof(val))
			break;
	}
	pr_info(" SC6607_REG_PHY_CTRL  idx=%d, buf=%s \n", idx, buf);

	return idx;
}

static int disable_wdt = 0;
static ssize_t sc6607_charger_store_register(struct device *dev, struct device_attribute *attr, const char *buf,
					     size_t count)
{
	struct sc6607 *chip = g_chip;
	int ret;
	unsigned int reg;
	unsigned int val;

	if (!chip)
		return count;

	ret = sscanf(buf, "%x %x", &reg, &val);
	pr_info("reg[0x%2x], val[0x%2x]\n", reg, val);
	if (ret == 2 && reg < SC6607_REG_MAX)
		sc6607_write_byte(chip, (u8)reg, (u8)val);

	if (reg == 0x00 && val == 1)
		disable_wdt = 1;
	else
		disable_wdt = 0;
	return count;
}

static DEVICE_ATTR(charger_registers, 0660, sc6607_charger_show_registers, sc6607_charger_store_register);

static void sc6607_charger_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_charger_registers);
}

static int sc6607_enter_ship_mode(struct sc6607 *chip, bool en)
{
	int ret = 0;

	if (!chip)
		return -EINVAL;

	if (en) {
		ret = sc6607_field_write(chip, F_BATFET_DLY, true);
		ret |= sc6607_field_write(chip, F_BATFET_DIS, true);
	} else {
		ret |= sc6607_field_write(chip, F_BATFET_DIS, false);
	}

	return ret;
}

static int sc6607_enable_shipmode(bool en)
{
	int ret;

	if (!g_chip)
		return -EINVAL;

	ret = sc6607_enter_ship_mode(g_chip, en);

	return ret;
}

static int sc6607_set_hz_mode(bool en)
{
	int ret;

	if (!g_chip)
		return -EINVAL;

	if (en)
		ret = sc6607_enter_hiz_mode(g_chip);
	else
		ret = sc6607_exit_hiz_mode(g_chip);

	return ret;
}

static bool oplus_usbtemp_condition(void)
{
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return -EINVAL;

	if ((g_chip && g_chip->power_good) || (chg_chip && chg_chip->charger_exist))
		return true;
	else
		return false;
}

void oplus_sc6607_dump_registers(void)
{
	if (!g_chip)
		return;

	sc6607_dump_regs(g_chip);
}

int oplus_sc6607_kick_wdt(void)
{
	if (!g_chip)
		return -EINVAL;

	return sc6607_reset_watchdog_timer(g_chip);
}

void oplus_sc6607_set_mivr(int vbatt)
{
	if (!g_chip)
		return;

	if (g_chip->hw_aicl_point == SC6607_HW_AICL_POINT_VOL_5V_PHASE1 && vbatt > SC6607_AICL_POINT_VOL_5V_HIGH) {
		g_chip->hw_aicl_point = SC6607_HW_AICL_POINT_VOL_5V_PHASE2;
	} else if (g_chip->hw_aicl_point == SC6607_HW_AICL_POINT_VOL_5V_PHASE2 &&
		   vbatt < SC6607_AICL_POINT_VOL_5V_MID) {
		g_chip->hw_aicl_point = SC6607_HW_AICL_POINT_VOL_5V_PHASE1;
	}

	sc6607_set_input_volt_limit(g_chip, g_chip->hw_aicl_point);
}

static int oplus_sc6607_set_aicr(int current_ma)
{
	int rc = 0, i = 0;
	int chg_vol = 0;
	int aicl_point = 0;
	int aicl_point_temp = 0;
	int main_cur = 0;
	int slave_cur = 0;
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();

	if (!chip || !g_chip)
		return 0;

	if (atomic_read(&g_chip->driver_suspended) == 1)
		return 0;

	if (atomic_read(&g_chip->charger_suspended) == 1) {
		pr_err("suspend,ignore set current=%dmA\n", current_ma);
		return 0;
	}

	if (chip->batt_volt > SC6607_AICL_POINT_VOL_5V_LOW)
		aicl_point_temp = aicl_point = SC6607_SW_AICL_POINT_VOL_5V_PHASE2;
	else
		aicl_point_temp = aicl_point = SC6607_SW_AICL_POINT_VOL_5V_PHASE1;

	pr_info("usb input max current limit=%d, aicl_point_temp=%d, em_mode=%d\n", current_ma, aicl_point_temp,
		chip->em_mode);

	if (current_ma < 500) {
		i = 0;
		goto aicl_end;
	}

	i = 1; /* 500 */
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY_MS);

	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		pr_debug("use 500 here\n");
		goto aicl_end;
	} else if (current_ma < 900)
		goto aicl_end;

	i = 2; /* 900 */
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY_MS);
	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		i = i - 1;
		goto aicl_pre_step;
	} else if (current_ma < 1200)
		goto aicl_end;

	i = 3; /* 1200 */
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY_MS);
	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		i = i - 1;
		goto aicl_pre_step;
	}

	i = 4; /* 1500 */
	aicl_point_temp = aicl_point + 50;
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY2_MS);
	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		i = i - 2;
		goto aicl_pre_step;
	} else if (current_ma < 1500) {
		i = i - 1;
		goto aicl_end;
	} else if (current_ma < 2000)
		goto aicl_end;

	i = 5; /* 1750 */
	aicl_point_temp = aicl_point + 50;
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY2_MS);
	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		i = i - 2;
		goto aicl_pre_step;
	}

	i = 6; /* 2000 */
	aicl_point_temp = aicl_point;
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY_MS);
	if (chg_vol < aicl_point_temp) {
		i = i - 2;
		goto aicl_pre_step;
	} else if (current_ma < 3000)
		goto aicl_end;

	i = 7; /* 3000 */
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	msleep(AICL_DELAY_MS);
	chg_vol = sc6607_adc_read_vbus_volt(g_chip);
	if (chg_vol < aicl_point_temp) {
		i = i - 1;
		goto aicl_pre_step;
	} else if (current_ma >= 3000)
		goto aicl_end;

aicl_pre_step:
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	pr_info("aicl_pre_step: current limit aicl chg_vol = %d j[%d] = %d sw_aicl_point:%d, \
		main %d mA, slave %d mA, slave_charger_enable:%d\n",
		chg_vol, i, usb_icl[i], aicl_point_temp, main_cur, slave_cur, chip->slave_charger_enable);
	return rc;
aicl_end:
	sc6607_set_input_current_limit(g_chip, usb_icl[i]);
	pr_info("aicl_end: current limit aicl chg_vol = %d j[%d] = %d sw_aicl_point:%d, \
		main %d mA, slave %d mA, slave_charger_enable:%d\n",
		chg_vol, i, usb_icl[i], aicl_point_temp, main_cur, slave_cur, chip->slave_charger_enable);
	return rc;
}

static int oplus_sc6607_set_input_current_limit(int current_ma)
{
	int ret;

	if (!g_chip)
		return -EINVAL;

	chg_err("current = %d\n", current_ma);
	g_chip->aicr = current_ma;
	ret = oplus_sc6607_set_aicr(current_ma);

	return ret;
}

static int oplus_sc6607_set_cv(int cv)
{
	if (!g_chip)
		return -EINVAL;

	return sc6607_set_chargevolt(g_chip, cv);
}

static int oplus_sc6607_set_ieoc(int cur)
{
	if (!g_chip)
		return -EINVAL;

	return sc6607_set_term_current(g_chip, cur);
}

static int oplus_sc6607_charging_enable(void)
{
	if (!g_chip)
		return -EINVAL;

	return sc6607_enable_charger(g_chip);
}

static int oplus_sc6607_charging_disable(void)
{
	if (!g_chip)
		return -EINVAL;

	pr_info(" disable");
	sc6607_disable_watchdog_timer(g_chip);
	g_chip->hw_aicl_point = SC6607_HW_AICL_POINT_VOL_5V_PHASE1;
	sc6607_set_input_volt_limit(g_chip, g_chip->hw_aicl_point);

	return sc6607_disable_charger(g_chip);
}

static int oplus_sc6607_hardware_init(void)
{
	int ret = 0;

	if (!g_chip)
		return -EINVAL;

	pr_info("start\n");

	g_chip->hw_aicl_point = SC6607_HW_AICL_POINT_VOL_5V_PHASE1;
	sc6607_set_input_volt_limit(g_chip, g_chip->hw_aicl_point);
	if (!strcmp(g_chip->chg_dev_name, "primary_chg")) {
		if (oplus_is_rf_ftm_mode()) {
			sc6607_disable_charger(g_chip);
			oplus_sc6607_charger_suspend();
		} else {
			oplus_sc6607_charger_unsuspend();
			sc6607_enable_charger(g_chip);
		}

		if (atomic_read(&g_chip->charger_suspended) == 1)
			chg_err("suspend,ignore set current=500mA\n");
		else
			sc6607_set_input_current_limit(g_chip, 500);
	}

	return ret;
}

static int oplus_sc6607_is_charging_enabled(void)
{
	int ret;
	u8 val = 0;
	struct sc6607 *chip = g_chip;

	if (!chip)
		return 0;

	ret = sc6607_field_read(chip, F_CHG_EN, &val);
	pr_info("enabled:%d\n", val);

	return val;
}

static int oplus_sc6607_is_charging_done(void)
{
	bool done;

	if (!g_chip)
		return 0;

	sc6607_check_charge_done(g_chip, &done);

	return done;
}

static void oplus_set_prswap(bool swap)
{
	pr_info("%s set prswap %d\n", __func__, swap);

	if (swap)
		oplus_is_prswap = true;
	else
		oplus_is_prswap = false;
}

static int oplus_sc6607_request_otg_on(int index)
{
	int ret = 0;
	int try_count = 10;
	u8 boost_good = false;

	if (!g_chip)
		return -EINVAL;

	oplus_is_prswap = false;

	if (!index)
		set_bit(BOOST_ON_OTG, &g_chip->request_otg);
	else
		set_bit(BOOST_ON_CAMERA, &g_chip->request_otg);

	ret = sc6607_get_otg_status(g_chip);
	if (ret) {
		pr_info(" already enable, index=%d, ret=%d, request_otg=%d\n", index, ret, g_chip->request_otg);
		return ret;
	}

	sc6607_disable_charger(g_chip);
	ret = sc6607_enable_otg(g_chip);
	if (ret < 0)
		pr_err("enable otg fail:%d\n", ret);

	do {
		msleep(10);
		ret = sc6607_field_read(g_chip, F_BOOST_GOOD, &boost_good);
		if (ret < 0) {
			pr_err("read boost good fail:%d\n", ret);
		}
	} while ((try_count--) && (!boost_good));

	if (!boost_good) {
		sc6607_enable_charger(g_chip);
		if (!index)
			clear_bit(BOOST_ON_OTG, &g_chip->request_otg);
		else
			clear_bit(BOOST_ON_CAMERA, &g_chip->request_otg);
		ret = -EINVAL;
	} else {
		ret = true;
	}
	pr_info(" index=%d, ret=0x%x, request_otg=%d\n", index, ret, g_chip->request_otg);

	return ret;
}

static int oplus_sc6607_request_otg_off(int index)
{
	int ret = 0;

	if (!g_chip)
		return -EINVAL;

	if (!index)
		clear_bit(BOOST_ON_OTG, &g_chip->request_otg);
	else
		clear_bit(BOOST_ON_CAMERA, &g_chip->request_otg);

	if (!g_chip->request_otg) {
		ret = sc6607_disable_otg(g_chip);
		if (ret < 0) {
			if (!index)
				set_bit(BOOST_ON_OTG, &g_chip->request_otg);
			else
				set_bit(BOOST_ON_CAMERA, &g_chip->request_otg);
			pr_err("disable otg fail:%d\n", ret);
		} else {
			sc6607_enable_charger(g_chip);
			ret = true;
		}
	}
	pr_info(" index=%d, ret=%d, request_otg=%d\n", index, ret, g_chip->request_otg);

	return ret;
}

static int oplus_sc6607_enable_otg(void)
{
	int ret = 0;

	if (!g_chip)
		return -EINVAL;

	if (atomic_read(&g_chip->driver_suspended) == 1) {
		return 0;
	}

	ret = oplus_sc6607_request_otg_on(BOOST_ON_OTG);
	if (ret > 0) {
		sc6607_field_write(g_chip, F_QB_EN, 1);
		sc6607_field_write(g_chip, F_ACDRV_MANUAL_EN, 1);
		sc6607_field_write(g_chip, F_ACDRV_EN, 1);
		sc6607_disable_watchdog_timer(g_chip);
	}
	pr_info("request_otg=%d ret=%d\n", g_chip->request_otg, ret);

	return ret;
}

static int oplus_sc6607_disable_otg(void)
{
	int ret = 0;

	if (!g_chip)
		return -EINVAL;
	if (atomic_read(&g_chip->driver_suspended) == 1) {
		return 0;
	}

	ret = oplus_sc6607_request_otg_off(BOOST_ON_OTG);
	if (ret > 0) {
		sc6607_field_write(g_chip, F_QB_EN, 0);
		sc6607_field_write(g_chip, F_ACDRV_EN, 0);
		sc6607_field_write(g_chip, F_ACDRV_MANUAL_EN, 0);
	}
	pr_info("request_otg=%d ret=%d\n", g_chip->request_otg, ret);

	return ret;
}

static int oplus_sc6607_disable_te(void)
{
	if (!g_chip)
		return 0;

	return sc6607_enable_term(g_chip, false);
}

static int oplus_sc6607_get_chg_current_step(void)
{
	return SC6607_BUCK_ICHG_STEP;
}

static int oplus_sc6607_get_charger_type(void)
{
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return POWER_SUPPLY_TYPE_UNKNOWN;

	if (g_chip->pd_sdp_port && chg_chip->usb_psy) {
		power_supply_changed(chg_chip->usb_psy);
		return POWER_SUPPLY_TYPE_USB_PD_SDP;
	}

	if (oplus_check_pd_usb_type() == PORT_PD_WITH_USB)
		return POWER_SUPPLY_TYPE_USB_PD_SDP;

	if (g_chip->oplus_chg_type != chg_chip->charger_type && chg_chip->usb_psy)
		power_supply_changed(chg_chip->usb_psy);

	return g_chip->oplus_chg_type;
}

void sc6607_really_suspend_charger(bool en)
{
	sc6607_set_hz_mode(true);
}

static int oplus_sc6607_charger_suspend(void)
{
	int ret;

	if (!g_chip)
		return -EINVAL;

	chg_err(" [%d, %d]\n", atomic_read(&g_chip->driver_suspended),
		atomic_read(&g_chip->charger_suspended));

	if (atomic_read(&g_chip->driver_suspended))
		return 0;

	oplus_sc6607_set_aicr(SC6607_SUSPEND_AICR);
	ret = sc6607_field_write(g_chip, F_PERFORMANCE_EN, true);
	ret = sc6607_field_write(g_chip, F_DIS_BUCKCHG_PATH, 1);
	if (oplus_vooc_get_fastchg_to_normal() == false && oplus_vooc_get_fastchg_to_warm() == false)
		sc6607_disable_charger(g_chip);

	atomic_set(&g_chip->charger_suspended, 1);

	return ret;
}

static int oplus_sc6607_charger_unsuspend(void)
{
	int rc = 0;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return -EINVAL;

	chg_err(" [%d, %d]\n", atomic_read(&g_chip->driver_suspended),
		atomic_read(&g_chip->charger_suspended));

	if (atomic_read(&g_chip->driver_suspended))
		return 0;

	atomic_set(&g_chip->charger_suspended, 0);
	sc6607_field_write(g_chip, F_DIS_BUCKCHG_PATH, 0);
	sc6607_field_write(g_chip, F_PERFORMANCE_EN, false);

	schedule_delayed_work(&g_chip->sc6607_aicr_setting_work, msecs_to_jiffies(SC6607_UNSUSPEND_JIFFIES));

	if (chg_chip) {
		if (!oplus_vooc_get_fastchg_to_normal() && !oplus_vooc_get_fastchg_to_warm() &&
		    chg_chip->authenticate && chg_chip->mmi_chg && !chg_chip->balancing_bat_stop_chg &&
		    (chg_chip->charging_state != CHARGING_STATUS_FAIL) && oplus_vooc_get_allow_reading() &&
		    !oplus_is_rf_ftm_mode())
			sc6607_enable_charger(g_chip);
	} else {
		sc6607_enable_charger(g_chip);
	}

	return rc;
}

static void sc6607_check_ic_suspend(void)
{
	u8 val = 0;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip || (chg_chip->mmi_chg && chg_chip->stop_chg))
		return;

	if (atomic_read(&g_chip->driver_suspended))
		return;

	if (atomic_read(&g_chip->charger_suspended)) {
		sc6607_field_read(g_chip, F_CHG_EN, &val);
		if (val)
			sc6607_field_write(g_chip, F_CHG_EN, false);

		sc6607_field_read(g_chip, F_DIS_BUCKCHG_PATH, &val);
		if (!val) {
			sc6607_field_write(g_chip, F_PERFORMANCE_EN, true);
			sc6607_field_write(g_chip, F_DIS_BUCKCHG_PATH, true);
		}
	}
}

static void sc6607_vooc_timeout_callback(bool vbus_rising)
{
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return;

	g_chip->power_good = vbus_rising;
	if (!vbus_rising) {
		if (chg_chip->mmi_chg)
			oplus_sc6607_charger_unsuspend();
		sc6607_request_dpdm(g_chip, false);
		g_chip->chg_type = CHARGER_UNKNOWN;
		g_chip->oplus_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sc6607_set_usb_props_type(g_chip->oplus_chg_type);
		oplus_chg_wakelock(g_chip, false);
		sc6607_disable_watchdog_timer(g_chip);
	}
	sc6607_dump_regs(g_chip);
}

static void sc6607_aicr_setting_work_callback(struct work_struct *work)
{
	if (!g_chip)
		return;

	oplus_sc6607_set_aicr(g_chip->aicr);
}

static int oplus_sc6607_set_rechg_vol(int vol)
{
	return 0;
}

static int oplus_sc6607_reset_charger(void)
{
	return 0;
}

static bool oplus_sc6607_check_charger_resume(void)
{
	return true;
}

static bool oplus_sc6607_check_chrdet_status(void)
{
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return 0;

	if (oplus_vooc_get_fastchg_started()|| oplus_vooc_get_fastchg_dummy_started() ||
        oplus_vooc_get_fastchg_to_warm() || oplus_vooc_get_fastchg_to_normal() ||
        chg_chip->camera_on)
		return true;

	return g_chip->power_good;
}

static int oplus_sc6607_get_charger_subtype(void)
{
	if (!g_chip)
		return 0;

	if (oplus_sc6607_get_pd_type())
		return CHARGER_SUBTYPE_PD;

	return CHARGER_SUBTYPE_DEFAULT;
}

static bool oplus_sc6607_need_to_check_ibatt(void)
{
	return false;
}

static int oplus_sc6607_get_dyna_aicl_result(void)
{
	int ma = 0;

	if (!g_chip)
		return 0;

	sc6607_read_idpm_limit(g_chip, &ma);
	return ma;
}

#ifdef CONFIG_OPLUS_SHORT_HW_CHECK
static bool oplus_shortc_check_is_gpio(struct oplus_chg_chip *chip)
{
	if (!chip) {
		pr_info(": oplus_chip not ready!\n");
		return false;
	}

	if (gpio_is_valid(chip->normalchg_gpio.shortc_gpio))
		return true;

	return false;
}

static bool oplus_sc6607_get_shortc_hw_gpio_status(void)
{
	bool shortc_hw_status = true;
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();

	if (!chip) {
		pr_info(": oplus_chip not ready!\n");
		return shortc_hw_status;
	}

	if (oplus_shortc_check_is_gpio(chip) == true) {
		shortc_hw_status = !!(gpio_get_value(chip->normalchg_gpio.shortc_gpio));
	}
	return shortc_hw_status;
}
#else /* CONFIG_OPLUS_SHORT_HW_CHECK */
static bool oplus_sc6607_get_shortc_hw_gpio_status(void)
{
	bool shortc_hw_status = true;

	return shortc_hw_status;
}
#endif /* CONFIG_OPLUS_SHORT_HW_CHECK */

static void sc6607_force_pd_to_dcp(void)
{
	if (!g_chip)
		return;

	g_chip->chg_type = CHARGING_HOST;
	g_chip->oplus_chg_type = POWER_SUPPLY_TYPE_USB_DCP;
	sc6607_set_usb_props_type(g_chip->oplus_chg_type);
}

#ifdef CONFIG_OPLUS_CHARGER_MTK
#define VBUS_9V	9000
#define VBUS_5V	5000
#define IBUS_2A	2000
#define IBUS_3A	3000

static void battery_update(void)
{
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();

	if (!chip || !chip->batt_psy) {
		chg_err("chip or chip->batt_psy is null,return\n");
		return;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
	power_supply_changed(chip->batt_psy);
#else
	power_supply_changed(&chip->batt_psy);
#endif
}

static int pd_tcp_notifier_call(struct notifier_block *pnb,
				unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	struct sc6607 *pinfo = NULL;
	uint8_t old_state = TYPEC_UNATTACHED;
	uint8_t new_state = TYPEC_UNATTACHED;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();
	int ret = 0;
	pinfo = container_of(pnb, struct sc6607, pd_nb);

	pr_err("PD charger event:%d %d\n", (int)event, (int)noti->pd_state.connected);
	if (pinfo == NULL || g_chip == NULL || chg_chip == NULL)
		return NOTIFY_BAD;
	switch (event) {
	case TCP_NOTIFY_PD_STATE:
		switch (noti->pd_state.connected) {
		case  PD_CONNECT_NONE:
			mutex_lock(&pinfo->charger_pd_lock);
			pr_err("PD Notify Detach\n");
			pinfo->pd_type = PD_CONNECT_NONE;
			mutex_unlock(&pinfo->charger_pd_lock);
			break;
		case PD_CONNECT_HARD_RESET:
			mutex_lock(&pinfo->charger_pd_lock);
			pr_err("PD Notify HardReset\n");
			pinfo->pd_type = PD_CONNECT_NONE;
			mutex_unlock(&pinfo->charger_pd_lock);
			break;
		case PD_CONNECT_PE_READY_SNK:
			mutex_lock(&pinfo->charger_pd_lock);
			pinfo->pd_type = PD_CONNECT_PE_READY_SNK;
			mutex_unlock(&pinfo->charger_pd_lock);
			ret = tcpm_inquire_dpm_flags(pinfo->tcpc);
			if ((g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB_CDP ||
			     g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB) &&
			     ((ret & DPM_FLAGS_PARTNER_USB_COMM) == 0)) {
				pr_err("pd without usb_comm, force sdp/cdp to dcp\n");
				oplus_chg_set_charger_type_unknown();
				sc6607_force_pd_to_dcp();
			}
			sc6607_inform_charger_type(g_chip);
			pr_err("PD Notify fixe voltage ready\n");
			break;

		case PD_CONNECT_PE_READY_SNK_PD30:
			mutex_lock(&pinfo->charger_pd_lock);
			pinfo->pd_type = PD_CONNECT_PE_READY_SNK_PD30;
			mutex_unlock(&pinfo->charger_pd_lock);
			ret = tcpm_inquire_dpm_flags(pinfo->tcpc);
			if ((g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB_CDP ||
			     g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB) &&
			     ((ret & DPM_FLAGS_PARTNER_USB_COMM) == 0)) {
				pr_err("pd without usb_comm, force sdp/cdp to dcp\n");
				oplus_chg_set_charger_type_unknown();
				sc6607_force_pd_to_dcp();
			}
			sc6607_inform_charger_type(g_chip);
			pr_err("PD Notify PD30 ready\r\n");
			break;
		case PD_CONNECT_PE_READY_SNK_APDO:
			mutex_lock(&pinfo->charger_pd_lock);
			pinfo->pd_type = PD_CONNECT_PE_READY_SNK_APDO;
			mutex_unlock(&pinfo->charger_pd_lock);
			ret = tcpm_inquire_dpm_flags(pinfo->tcpc);
			if ((g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB_CDP ||
			     g_chip->oplus_chg_type == POWER_SUPPLY_TYPE_USB) &&
			     ((ret & DPM_FLAGS_PARTNER_USB_COMM) == 0)) {
				pr_err("pd without usb_comm, force sdp/cdp to dcp\n");
				oplus_chg_set_charger_type_unknown();
				sc6607_force_pd_to_dcp();
			}
			sc6607_inform_charger_type(g_chip);
			pr_err("PD Notify APDO Ready\n");
			break;
		case PD_CONNECT_TYPEC_ONLY_SNK:
			mutex_lock(&pinfo->charger_pd_lock);
			pr_err("PD Notify Type-C Ready\n");
			pinfo->pd_type = PD_CONNECT_TYPEC_ONLY_SNK;
			mutex_unlock(&pinfo->charger_pd_lock);
			break;
		}

	case TCP_NOTIFY_TYPEC_STATE:
		old_state = noti->typec_state.old_state;
		new_state = noti->typec_state.new_state;
		if (old_state == TYPEC_UNATTACHED &&
		    (new_state == TYPEC_ATTACHED_SNK ||
		     new_state == TYPEC_ATTACHED_NORP_SRC ||
		     new_state == TYPEC_ATTACHED_CUSTOM_SRC ||
		     new_state == TYPEC_ATTACHED_DBGACC_SNK)) {
			pr_info("Charger plug in, polarity = %d\n", noti->typec_state.polarity);
			if (!chg_chip->authenticate || chg_chip->balancing_bat_stop_chg)
				sc6607_disable_charger(g_chip);
		} else if ((old_state == TYPEC_ATTACHED_SNK ||
			    old_state == TYPEC_ATTACHED_NORP_SRC ||
			    old_state == TYPEC_ATTACHED_CUSTOM_SRC ||
			    old_state == TYPEC_ATTACHED_DBGACC_SNK) &&
			    new_state == TYPEC_UNATTACHED) {
			pr_info("Charger plug out\n");

		} else if (old_state == TYPEC_UNATTACHED &&
			   (new_state == TYPEC_ATTACHED_SRC ||
			    new_state == TYPEC_ATTACHED_DEBUG)) {
			pr_info("OTG plug in, polarity = %d\n", noti->typec_state.polarity);
			battery_update();
		} else if ((old_state == TYPEC_ATTACHED_SRC ||
			    old_state == TYPEC_ATTACHED_DEBUG) &&
			    new_state == TYPEC_UNATTACHED) {
			pr_info("OTG plug out\n");
			battery_update();
		} else if (old_state == TYPEC_UNATTACHED &&
			   new_state == TYPEC_ATTACHED_AUDIO) {
			pr_info("Audio plug in\n");
			/* enable AudioAccessory connection */
		} else if (old_state == TYPEC_ATTACHED_AUDIO &&
			   new_state == TYPEC_UNATTACHED) {
			pr_info("Audio plug out\n");
			/* disable AudioAccessory connection */
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int oplus_mtk_get_pd_type(void)
{
	if (g_chip) {
		pr_info("pd_type=%d\n", g_chip->pd_type);
		if (g_chip->pd_type == PD_CONNECT_PE_READY_SNK ||
		    g_chip->pd_type == PD_CONNECT_PE_READY_SNK_PD30)
			return PD_ACTIVE;
		else if (g_chip->pd_type == PD_CONNECT_PE_READY_SNK_APDO)
			return PD_ACTIVE;
		else
			return PD_INACTIVE;
	}

	return PD_INACTIVE;
}

int oplus_mtk_pd_setup(void)
{
	int i;
	int ret = -1;
	int vbus_mv = VBUS_5V;
	int ibus_ma = IBUS_2A;
	struct adapter_power_cap cap;
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();

	if (g_chip && (!g_chip->pd_adapter))
		g_chip->pd_adapter = get_adapter_by_name("pd_adapter");

	if (!g_chip || !chip || !g_chip->pd_adapter) {
		pr_err("null\n");
		return ret;
	}

	cap.nr = 0;
	cap.pdp = 0;
	for (i = 0; i < ADAPTER_CAP_MAX_NR; i++) {
		cap.max_mv[i] = 0;
		cap.min_mv[i] = 0;
		cap.ma[i] = 0;
		cap.type[i] = 0;
		cap.pwr_limit[i] = 0;
	}

	pr_info("pd_type: %d\n", g_chip->pd_type);
	if (!chip->calling_on && !chip->camera_on && chip->charger_volt < 6500 &&
	     chip->soc < 90 && chip->temperature <= 420 && chip->cool_down_force_5v == false) {
		if (g_chip->pd_type == PD_CONNECT_PE_READY_SNK_APDO) {
			adapter_dev_get_cap(g_chip->pd_adapter, MTK_PD_APDO, &cap);
			for (i = 0; i < cap.nr; i++)
				pr_info("PD APDO cap %d: mV:%d,%d mA:%d type:%d pwr_limit:%d pdp:%d\n",
					i, cap.max_mv[i], cap.min_mv[i], cap.ma[i],
					cap.type[i], cap.pwr_limit[i], cap.pdp);

			for (i = 0; i < cap.nr; i++) {
				if (cap.min_mv[i] <= VBUS_9V && VBUS_9V <= cap.max_mv[i]) {
					vbus_mv = VBUS_9V;
					ibus_ma = cap.ma[i];
					if (ibus_ma > IBUS_2A)
						ibus_ma = IBUS_2A;
					break;
				}
			}
		} else if (g_chip->pd_type == PD_CONNECT_PE_READY_SNK ||
			g_chip->pd_type == PD_CONNECT_PE_READY_SNK_PD30) {
			adapter_dev_get_cap(g_chip->pd_adapter, MTK_PD, &cap);
			for (i = 0; i < cap.nr; i++)
				pr_info("PD cap %d: mV:%d,%d mA:%d type:%d\n",
					i, cap.max_mv[i], cap.min_mv[i], cap.ma[i], cap.type[i]);

			for (i = 0; i < cap.nr; i++) {
				if (VBUS_9V <= cap.max_mv[i]) {
					vbus_mv = cap.max_mv[i];
					ibus_ma = cap.ma[i];
					if (ibus_ma > IBUS_2A)
						ibus_ma = IBUS_2A;
					break;
				}
			}
		} else {
			vbus_mv = VBUS_5V;
			ibus_ma = IBUS_2A;
		}
		pr_info("PD request: %dmV, %dmA\n", vbus_mv, ibus_ma);
		ret = oplus_pdc_setup(&vbus_mv, &ibus_ma);
	} else {
		if (chip->charger_volt > 7500 && (chip->calling_on || chip->camera_on ||
		    chip->soc >= 90 || chip->batt_volt >= 4450 || chip->temperature > 420 ||
		    chip->cool_down_force_5v == true)) {
			vbus_mv = VBUS_5V;
			ibus_ma = IBUS_3A;
			pr_info("PD request: %dmV, %dmA\n", vbus_mv, ibus_ma);
			ret = oplus_pdc_setup(&vbus_mv, &ibus_ma);
		}
	}

	return ret;
}
#endif

static int oplus_sc6607_get_pd_type(void)
{
	if (!g_chip) {
		pr_err("disable_pd\n");
		return CHARGER_SUBTYPE_DEFAULT;
	}

#ifdef CONFIG_OPLUS_CHARGER_MTK
	return oplus_mtk_get_pd_type();
#else
	return oplus_sm8150_get_pd_type();
#endif
}

static int oplus_sc6607_set_pd_config(void)
{
	if (!g_chip)
		return 0;

#ifdef CONFIG_OPLUS_CHARGER_MTK
	return oplus_mtk_pd_setup();
#else
	return oplus_chg_set_pd_config();
#endif
}

static int oplus_sc6607_get_vbus(void)
{
	struct oplus_chg_chip *chip = oplus_chg_get_chg_struct();
#ifndef CONFIG_OPLUS_CHARGER_MTK
	struct smb_charger *chg = NULL;
#endif
	if (!chip || !g_chip)
		return 0;

#ifndef CONFIG_OPLUS_CHARGER_MTK
	chg = &chip->pmic_spmi.smb5_chip->chg;
	if (qpnp_is_power_off_charging()) {
		if (chg->pd_hard_reset || chg->keep_vbus_5v) {
			pr_info("pd hardreset,return 5000\n");
			return SC6607_DEFAULT_VBUS_MV;
		}
	}
#endif /* CONFIG_OPLUS_CHARGER_MTK */

	return sc6607_adc_read_vbus_volt(g_chip);
}

static int oplus_sc6607_get_ibus(void)
{
	if (!g_chip)
		return 0;
	else
		return sc6607_adc_read_ibus(g_chip);
}

static int oplus_sc6607_get_vbat(struct oplus_voocphy_manager *chip)
{
	if (!g_chip)
		return 0;
	else
		return sc6607_adc_read_vbat(g_chip);
}

static void sc6607_init_status_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct sc6607 *chip = container_of(dwork, struct sc6607, init_status_work);
	if (chip == NULL) {
		pr_err("g_chip is NULL\n");
		return;
	}
	if (chip->oplus_chg_type != POWER_SUPPLY_TYPE_UNKNOWN)
		return;

	chip->wd_rerun_detect = true;	/* Rerun detect for META mode */
	sc6607_hk_irq_handle(chip);
	pr_info("%s enter\n", __func__);
}

#ifdef CONFIG_OPLUS_CHARGER_MTK
static struct charger_ops sc6607_chg_ops = {
	/* Normal charging */
	.plug_in = sc6607_plug_in,
	.plug_out = sc6607_plug_out,
	.kick_wdt = sc6607_kick_wdt,
	.enable = sc6607_charge_enable,
};
#endif

#ifdef CONFIG_OPLUS_CHARGER_MTK
static void oplus_mt_power_off(void)
{
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!chg_chip) {
		chg_err("oplus_chip not ready!\n");
		return;
	}

	if (chg_chip->ac_online != true) {
		if (!oplus_sc6607_check_chrdet_status())
			kernel_power_off();
	} else {
		chg_err("ac_online is true, return!\n");
	}
}

static int oplus_sc6607_get_rtc_spare_oplus_fg_value(void)
{
	return 0;
}

static int oplus_sc6607_set_rtc_spare_oplus_fg_value(int soc)
{
	return 0;
}
#endif

struct oplus_chg_operations oplus_chg_sc6607_ops = {
	.dump_registers = oplus_sc6607_dump_registers,
	.kick_wdt = oplus_sc6607_kick_wdt,
	.hardware_init = oplus_sc6607_hardware_init,
	.charging_current_write_fast = oplus_sc6607_set_ichg,
	.set_aicl_point = oplus_sc6607_set_mivr,
	.input_current_write = oplus_sc6607_set_input_current_limit,
	.float_voltage_write = oplus_sc6607_set_cv,
	.term_current_set = oplus_sc6607_set_ieoc,
	.charging_enable = oplus_sc6607_charging_enable,
	.charging_disable = oplus_sc6607_charging_disable,
	.get_charging_enable = oplus_sc6607_is_charging_enabled,
	.charger_suspend = oplus_sc6607_charger_suspend,
	.charger_unsuspend = oplus_sc6607_charger_unsuspend,
	.set_rechg_vol = oplus_sc6607_set_rechg_vol,
	.reset_charger = oplus_sc6607_reset_charger,
	.read_full = oplus_sc6607_is_charging_done,
	.otg_enable = oplus_sc6607_enable_otg,
	.otg_disable = oplus_sc6607_disable_otg,
	.set_charging_term_disable = oplus_sc6607_disable_te,
	.check_charger_resume = oplus_sc6607_check_charger_resume,
	.get_charger_type = oplus_sc6607_get_charger_type,
#ifdef CONFIG_OPLUS_CHARGER_MTK
	.get_boot_mode = (int (*)(void))get_boot_mode,
	.get_boot_reason = (int (*)(void))get_boot_reason,
	.get_rtc_soc = oplus_sc6607_get_rtc_spare_oplus_fg_value,
	.set_rtc_soc = oplus_sc6607_set_rtc_spare_oplus_fg_value,
	.set_power_off = oplus_mt_power_off,
	.get_platform_gauge_curve = oplus_chg_choose_gauge_curve,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	.usb_connect = mt_usb_connect,
	.usb_disconnect = mt_usb_disconnect,
#else
	.usb_connect = mt_usb_connect_v1,
	.usb_disconnect = mt_usb_disconnect_v1,
#endif
#else
	.get_boot_mode = get_boot_mode,
	.get_boot_reason = smbchg_get_boot_reason,
	.get_rtc_soc = oplus_chg_get_shutdown_soc,
	.set_rtc_soc = oplus_chg_backup_soc,
	.get_instant_vbatt = qpnp_get_battery_voltage,
	.get_subboard_temp = oplus_get_subboard_temp,
	.usb_connect = NULL,
	.usb_disconnect = NULL,
#endif /* CONFIG_OPLUS_CHARGER_MTK */
	.check_chrdet_status = oplus_sc6607_check_chrdet_status,
	.get_charger_volt = oplus_sc6607_get_vbus,
	.get_chg_current_step = oplus_sc6607_get_chg_current_step,
	.need_to_check_ibatt = oplus_sc6607_need_to_check_ibatt,
	.get_dyna_aicl_result = oplus_sc6607_get_dyna_aicl_result,
	.get_shortc_hw_gpio_status = oplus_sc6607_get_shortc_hw_gpio_status,
	.oplus_chg_get_pd_type = oplus_sc6607_get_pd_type,
	.oplus_chg_pd_setup = oplus_sc6607_set_pd_config,
	.get_charger_subtype = oplus_sc6607_get_charger_subtype,
	.oplus_chg_set_high_vbus = NULL,
	.enable_shipmode = sc6607_enable_shipmode,
	.oplus_chg_set_hz_mode = sc6607_set_hz_mode,
	.oplus_usbtemp_monitor_condition = oplus_usbtemp_condition,
	.get_usbtemp_volt = oplus_get_usbtemp_volt,
	.set_typec_sinkonly = oplus_set_typec_sinkonly,
	.set_typec_cc_open = oplus_set_typec_cc_open,
	.check_qchv_condition = oplus_chg_check_qchv_condition,
	.vooc_timeout_callback = sc6607_vooc_timeout_callback,
	.force_pd_to_dcp = sc6607_force_pd_to_dcp,
	/*.really_suspend_charger = sc6607_really_suspend_charger,*/
	.set_prswap = oplus_set_prswap,
	.check_chg_plugin = sc6607_check_chg_plugin,
	.get_charger_current = oplus_sc6607_get_ibus,
	.get_cp_tsbus = sc6607_voocphy_get_tsbus,
	.get_cp_tsbat = sc6607_voocphy_get_tsbat,
};

/***********************************voocphy part*************************************/
static void sc6607_voocphy_i2c_error(bool happen)
{
	int report_flag = 0;
	if (!oplus_voocphy_mg || error_reported)
		return;

	if (happen) {
		oplus_voocphy_mg->voocphy_iic_err = true;
		oplus_voocphy_mg->voocphy_iic_err_num++;
		if (oplus_voocphy_mg->voocphy_iic_err_num >= SC6607_I2C_ERR_NUM) {
			report_flag |= SC6607_MAIN_I2C_ERROR;
			oplus_chg_sc8547_error(report_flag, NULL, 0);
			error_reported = true;
		}
	} else {
		oplus_voocphy_mg->voocphy_iic_err_num = 0;
		oplus_chg_sc8547_error(0, NULL, 0);
	}
}

static int __sc6607_voocphy_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	s32 ret;
	struct oplus_voocphy_manager *chip;

	chip = i2c_get_clientdata(client);
	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		sc6607_voocphy_i2c_error(true);
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		sc6607_voocphy_track_upload_i2c_err_info(chip, ret, reg);
		return ret;
	}
	sc6607_voocphy_i2c_error(false);
	*data = (u8)ret;

	return 0;
}

static int __sc6607_voocphy_write_byte(struct i2c_client *client, u8 reg, u8 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip;

	chip = i2c_get_clientdata(client);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		sc6607_voocphy_i2c_error(true);
		pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n", val, reg, ret);
		sc6607_voocphy_track_upload_i2c_err_info(chip, ret, reg);
		return ret;
	}
	sc6607_voocphy_i2c_error(false);
	return 0;
}

static int sc6607_voocphy_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = __sc6607_voocphy_read_byte(client, reg, data);
	mutex_unlock(&i2c_rw_lock);

	return ret;
}

static int sc6607_voocphy_write_byte(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = __sc6607_voocphy_write_byte(client, reg, data);
	mutex_unlock(&i2c_rw_lock);

	return ret;
}

static s32 sc6607_voocphy_read_word(struct i2c_client *client, u8 reg)
{
	s32 ret;
	struct oplus_voocphy_manager *chip;

	chip = i2c_get_clientdata(client);
	mutex_lock(&i2c_rw_lock);
	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0) {
		sc6607_voocphy_i2c_error(true);
		pr_err("i2c read word fail: can't read reg:0x%02X \n", reg);
		sc6607_voocphy_track_upload_i2c_err_info(chip, ret, reg);
		mutex_unlock(&i2c_rw_lock);
		return ret;
	}
	sc6607_voocphy_i2c_error(false);
	mutex_unlock(&i2c_rw_lock);
	return ret;
}

static s32 sc6607_voocphy_write_word(struct i2c_client *client, u8 reg, u16 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip;

	chip = i2c_get_clientdata(client);
	mutex_lock(&i2c_rw_lock);
	ret = i2c_smbus_write_word_data(client, reg, val);
	if (ret < 0) {
		sc6607_voocphy_i2c_error(true);
		pr_err("i2c write word fail: can't write 0x%02X to reg:0x%02X \n", val, reg);
		sc6607_voocphy_track_upload_i2c_err_info(chip, ret, reg);
		mutex_unlock(&i2c_rw_lock);
		return ret;
	}
	sc6607_voocphy_i2c_error(false);
	mutex_unlock(&i2c_rw_lock);
	return 0;
}

static int sc6607_voocphy_track_get_local_time_s(void)
{
	struct timespec ts;

	getnstimeofday(&ts);

	return ts.tv_sec;
}

static int sc6607_track_get_current_time_s(void)
{
	struct timespec ts;

	getnstimeofday(&ts);

	return ts.tv_sec;
}

static int sc6607_voocphy_track_upload_i2c_err_info(struct oplus_voocphy_manager *chip, int err_type, u8 reg)
{
	int index = 0;
	int curr_time;
	static int upload_count = 0;
	static int pre_upload_time = 0;

	if (!chip || !chip->track_init_done)
		return -EINVAL;

	mutex_lock(&chip->track_upload_lock);
	memset(chip->chg_power_info, 0, sizeof(chip->chg_power_info));
	memset(chip->err_reason, 0, sizeof(chip->err_reason));
	curr_time = sc6607_voocphy_track_get_local_time_s();
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (upload_count > TRACK_UPLOAD_COUNT_MAX) {
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (chip->debug_force_i2c_err)
		err_type = -chip->debug_force_i2c_err;

	mutex_lock(&chip->track_i2c_err_lock);
	if (chip->i2c_err_uploading) {
		pr_info("i2c_err_uploading, should return\n");
		mutex_unlock(&chip->track_i2c_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (chip->i2c_err_load_trigger)
		kfree(chip->i2c_err_load_trigger);
	chip->i2c_err_load_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->i2c_err_load_trigger) {
		pr_err("i2c_err_load_trigger memery alloc fail\n");
		mutex_unlock(&chip->track_i2c_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return -ENOMEM;
	}
	chip->i2c_err_load_trigger->type_reason = TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL;
	chip->i2c_err_load_trigger->flag_reason = TRACK_NOTIFY_FLAG_CP_ABNORMAL;
	chip->i2c_err_uploading = true;
	upload_count++;
	pre_upload_time = sc6607_voocphy_track_get_local_time_s();
	mutex_unlock(&chip->track_i2c_err_lock);

	index += snprintf(&(chip->i2c_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$device_id@@%s", "sc6607");
	index += snprintf(&(chip->i2c_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_scene@@%s", OPLUS_CHG_TRACK_SCENE_I2C_ERR);

	oplus_chg_track_get_i2c_err_reason(err_type, chip->err_reason, sizeof(chip->err_reason));
	index += snprintf(&(chip->i2c_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_reason@@%s", chip->err_reason);

	oplus_chg_track_obtain_power_info(chip->chg_power_info, sizeof(chip->chg_power_info));
	index += snprintf(&(chip->i2c_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s",
			  chip->chg_power_info);
	index += snprintf(&(chip->i2c_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$access_reg@@0x%02x", reg);
	schedule_delayed_work(&chip->i2c_err_load_trigger_work, 0);
	mutex_unlock(&chip->track_upload_lock);
	pr_info("success\n");

	return 0;
}

static void sc6607_voocphy_track_i2c_err_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_voocphy_manager *chip =
		container_of(dwork, struct oplus_voocphy_manager, i2c_err_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(*(chip->i2c_err_load_trigger));
	if (chip->i2c_err_load_trigger) {
		kfree(chip->i2c_err_load_trigger);
		chip->i2c_err_load_trigger = NULL;
	}
	chip->i2c_err_uploading = false;
}

static int sc6607_dump_reg_info(char *dump_info, int len, struct sc6607_track_check_reg *p_check_reg, int check_reg_len)
{
	int i;
	int ret;
	u8 data;
	int index = 0;
	struct sc6607 *chip = g_chip;
	u8 track_dump_reg[] = {
		SC6607_REG_HK_GEN_FLG,
		SC6607_REG_VAC_VBUS_OVP,
		SC6607_REG_HK_INT_STAT,
		SC6607_REG_HK_INT_FLG,
		SC6607_REG_PROTECTION_DIS,
		SC6607_REG_CHG_CTRL1,
		SC6607_REG_CHG_INT_FLG,
		SC6607_REG_CHG_INT_FLG + 1,
		SC6607_REG_CHG_INT_FLG3,
		SC6607_REG_CHG_FLT_FLG,
		SC6607_REG_CHG_FLT_FLG2,
		SC6607_REG_CP_CTRL,
		SC6607_REG_CP_INT_STAT,
		SC6607_REG_DPDM_INT_FLAG,
		SC6607_REG_VOOCPHY_FLAG,
	};

	if (!chip || !dump_info || !p_check_reg)
		return 0;

	for (i = 0; i < check_reg_len; i++)
		index += snprintf(&(dump_info[index]), len - index, "0x%02x=0x%02x,", p_check_reg[i].addr,
				  p_check_reg[i].data);

	for (i = 0; i < ARRAY_SIZE(track_dump_reg); i++) {
		ret = sc6607_read_byte(chip, track_dump_reg[i], &data);
		if (ret < 0) {
			pr_err("read 0x%x failed\n", track_dump_reg[i]);
			return -EINVAL;
		}
		index += snprintf(&(dump_info[index]), len - index, "0x%02x=0x%02x,", track_dump_reg[i], data);
	}

	return 0;
}

static int sc6607_track_upload_hk_err_info(struct sc6607 *chip, int err_type,
					   struct sc6607_track_check_reg *p_check_reg, int check_reg_len)
{
	int index = 0;
	int curr_time;
	static int upload_count = 0;
	static int pre_upload_time = 0;

	if (!chip || !chip->track_init_done || !p_check_reg)
		return -EINVAL;

	mutex_lock(&chip->track_upload_lock);
	memset(chip->chg_power_info, 0, sizeof(chip->chg_power_info));
	memset(chip->err_reason, 0, sizeof(chip->err_reason));
	memset(chip->dump_info, 0, sizeof(chip->dump_info));
	curr_time = sc6607_track_get_current_time_s();
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (err_type == TRACK_HK_ERR_DEFAULT) {
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (upload_count > TRACK_UPLOAD_COUNT_MAX) {
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	mutex_lock(&chip->track_hk_err_lock);
	if (chip->hk_err_uploading) {
		pr_info("hk_err_uploading, should return\n");
		mutex_unlock(&chip->track_hk_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (chip->hk_err_load_trigger)
		kfree(chip->hk_err_load_trigger);
	chip->hk_err_load_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->hk_err_load_trigger) {
		pr_err("hk_err_load_trigger memery alloc fail\n");
		mutex_unlock(&chip->track_hk_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return -ENOMEM;
	}
	chip->hk_err_load_trigger->type_reason = TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL;
	chip->hk_err_load_trigger->flag_reason = TRACK_NOTIFY_FLAG_HK_ABNORMAL;
	chip->hk_err_uploading = true;
	upload_count++;
	pre_upload_time = sc6607_track_get_current_time_s();
	mutex_unlock(&chip->track_hk_err_lock);

	index += snprintf(&(chip->hk_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$device_id@@%s", "sc6607");
	index += snprintf(&(chip->hk_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_scene@@%s", OPLUS_CHG_TRACK_SCENE_HK_ERR);

	oplus_chg_track_get_hk_err_reason(err_type, chip->err_reason, sizeof(chip->err_reason));
	index += snprintf(&(chip->hk_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_reason@@%s", chip->err_reason);

	oplus_chg_track_obtain_power_info(chip->chg_power_info, sizeof(chip->chg_power_info));
	index += snprintf(&(chip->hk_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s",
			  chip->chg_power_info);
	sc6607_dump_reg_info(chip->dump_info, sizeof(chip->dump_info), p_check_reg, check_reg_len);
	index += snprintf(&(chip->hk_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$reg_info@@%s", chip->dump_info);
	schedule_delayed_work(&chip->hk_err_load_trigger_work, 0);
	mutex_unlock(&chip->track_upload_lock);
	pr_info("success\n");

	return 0;
}

static void sc6607_track_hk_err_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct sc6607 *chip = container_of(dwork, struct sc6607, hk_err_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(*(chip->hk_err_load_trigger));
	if (chip->hk_err_load_trigger) {
		kfree(chip->hk_err_load_trigger);
		chip->hk_err_load_trigger = NULL;
	}
	chip->hk_err_uploading = false;
}

static int sc6607_voocphy_track_upload_cp_err_info(struct oplus_voocphy_manager *chip, int err_type,
						   struct sc6607_track_check_reg *p_check_reg, int check_reg_len)
{
	int index = 0;
	int curr_time;
	static int upload_count = 0;
	static int pre_upload_time = 0;

	if (!chip || !chip->track_init_done || !p_check_reg)
		return -EINVAL;

	mutex_lock(&chip->track_upload_lock);
	memset(chip->chg_power_info, 0, sizeof(chip->chg_power_info));
	memset(chip->err_reason, 0, sizeof(chip->err_reason));
	memset(chip->dump_info, 0, sizeof(chip->dump_info));
	curr_time = sc6607_voocphy_track_get_local_time_s();
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (err_type == TRACK_CP_ERR_DEFAULT) {
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (upload_count > TRACK_UPLOAD_COUNT_MAX) {
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	mutex_lock(&chip->track_cp_err_lock);
	if (chip->cp_err_uploading) {
		pr_info("cp_err_uploading, should return\n");
		mutex_unlock(&chip->track_cp_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return 0;
	}

	if (chip->cp_err_load_trigger)
		kfree(chip->cp_err_load_trigger);
	chip->cp_err_load_trigger = kzalloc(sizeof(oplus_chg_track_trigger), GFP_KERNEL);
	if (!chip->cp_err_load_trigger) {
		pr_err("cp_err_load_trigger memery alloc fail\n");
		mutex_unlock(&chip->track_cp_err_lock);
		mutex_unlock(&chip->track_upload_lock);
		return -ENOMEM;
	}
	chip->cp_err_load_trigger->type_reason = TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL;
	chip->cp_err_load_trigger->flag_reason = TRACK_NOTIFY_FLAG_CP_ABNORMAL;
	chip->cp_err_uploading = true;
	upload_count++;
	pre_upload_time = sc6607_voocphy_track_get_local_time_s();
	mutex_unlock(&chip->track_cp_err_lock);

	index += snprintf(&(chip->cp_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$device_id@@%s", "sc6607");
	index += snprintf(&(chip->cp_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_scene@@%s", OPLUS_CHG_TRACK_SCENE_CP_ERR);

	oplus_chg_track_get_cp_err_reason(err_type, chip->err_reason, sizeof(chip->err_reason));
	index += snprintf(&(chip->cp_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$err_reason@@%s", chip->err_reason);

	oplus_chg_track_obtain_power_info(chip->chg_power_info, sizeof(chip->chg_power_info));
	index += snprintf(&(chip->cp_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index, "%s",
			  chip->chg_power_info);
	sc6607_dump_reg_info(chip->dump_info, sizeof(chip->dump_info), p_check_reg, check_reg_len);
	index += snprintf(&(chip->cp_err_load_trigger->crux_info[index]), OPLUS_CHG_TRACK_CURX_INFO_LEN - index,
			  "$$reg_info@@%s", chip->dump_info);
	schedule_delayed_work(&chip->cp_err_load_trigger_work, 0);
	mutex_unlock(&chip->track_upload_lock);
	pr_info("success\n");

	return 0;
}

static void sc6607_voocphy_track_cp_err_load_trigger_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_voocphy_manager *chip =
		container_of(dwork, struct oplus_voocphy_manager, cp_err_load_trigger_work);

	if (!chip)
		return;

	oplus_chg_track_upload_trigger_data(*(chip->cp_err_load_trigger));
	if (chip->cp_err_load_trigger) {
		kfree(chip->cp_err_load_trigger);
		chip->cp_err_load_trigger = NULL;
	}
	chip->cp_err_uploading = false;
}

static int sc6607_buck_track_debugfs_init(struct sc6607 *chip)
{
	int ret = 0;
	struct dentry *debugfs_root;

	debugfs_root = oplus_chg_track_get_debugfs_root();
	if (!debugfs_root) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_sc6607 = debugfs_create_dir("sc6607", debugfs_root);
	if (!debugfs_sc6607) {
		ret = -ENOENT;
		return ret;
	}

	chip->debug_force_hk_err = TRACK_HK_ERR_DEFAULT;
	debugfs_create_u32("debug_force_hk_err", 0644, debugfs_sc6607, &(chip->debug_force_hk_err));

	return ret;
}

static int sc6607_voocphy_track_debugfs_init(struct oplus_voocphy_manager *chip)
{
	int ret = 0;

	if (!debugfs_sc6607) {
		ret = -ENOENT;
		return ret;
	}

	chip->debug_force_i2c_err = false;
	chip->debug_force_cp_err = TRACK_WLS_TRX_ERR_DEFAULT;
	debugfs_create_u32("debug_force_i2c_err", 0644, debugfs_sc6607, &(chip->debug_force_i2c_err));
	debugfs_create_u32("debug_force_cp_err", 0644, debugfs_sc6607, &(chip->debug_force_cp_err));

	return ret;
}

static int sc6607_buck_track_init(struct sc6607 *chip)
{
	int rc;

	if (!chip)
		return -EINVAL;

	mutex_init(&chip->track_hk_err_lock);
	mutex_init(&chip->track_upload_lock);
	chip->hk_err_uploading = false;
	chip->hk_err_load_trigger = NULL;

	INIT_DELAYED_WORK(&chip->hk_err_load_trigger_work, sc6607_track_hk_err_load_trigger_work);
	rc = sc6607_buck_track_debugfs_init(chip);
	if (rc < 0) {
		pr_err("sc6607 debugfs init error, rc=%d\n", rc);
		return rc;
	}
	chip->track_init_done = true;

	return rc;
}

static int sc6607_voocphy_track_init(struct oplus_voocphy_manager *chip)
{
	int rc;

	if (!chip)
		return -EINVAL;

	mutex_init(&chip->track_i2c_err_lock);
	mutex_init(&chip->track_cp_err_lock);
	mutex_init(&chip->track_upload_lock);
	chip->i2c_err_uploading = false;
	chip->i2c_err_load_trigger = NULL;
	chip->cp_err_uploading = false;
	chip->cp_err_load_trigger = NULL;

	INIT_DELAYED_WORK(&chip->i2c_err_load_trigger_work, sc6607_voocphy_track_i2c_err_load_trigger_work);
	INIT_DELAYED_WORK(&chip->cp_err_load_trigger_work, sc6607_voocphy_track_cp_err_load_trigger_work);
	rc = sc6607_voocphy_track_debugfs_init(chip);
	if (rc < 0) {
		pr_err("sc6607 debugfs init error, rc=%d\n", rc);
		return rc;
	}
	chip->track_init_done = true;

	return rc;
}

void oplus_chg_set_camera_on(bool val)
{
	int ret = 0;
	struct oplus_chg_chip *chg_chip = oplus_chg_get_chg_struct();

	if (!g_chip || !chg_chip)
		return;
	if (chg_chip->camera_on == val)
		return;

	if (val) {
		chg_chip->camera_on = true;
		if (chg_chip->charger_exist && !chg_chip->pd_svooc && chg_chip->chg_ops->get_charger_subtype() == CHARGER_SUBTYPE_PD)
			oplus_sc6607_set_pd_config();
		else
			oplus_chg_set_flash_led_status(true);
		ret = oplus_sc6607_request_otg_on(BOOST_ON_CAMERA);
	} else {
		if (chg_chip->charger_exist && !chg_chip->pd_svooc && chg_chip->chg_ops->get_charger_subtype() == CHARGER_SUBTYPE_PD)
			oplus_sc6607_set_pd_config();
		else
			oplus_chg_set_flash_led_status(false);
		ret = oplus_sc6607_request_otg_off(BOOST_ON_CAMERA);
		msleep(SC6607_CAMERA_ON_DELAY);
		chg_chip->camera_on = false;
		sc6607_enable_charger(g_chip);
	}
	pr_info("val: %d", val);
}
EXPORT_SYMBOL(oplus_chg_set_camera_on);

static int sc6607_voocphy_set_predata(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;

	if (!chip) {
		pr_err("failed: chip is null\n");
		return -1;
	}

	ret = sc6607_voocphy_write_word(chip->client, SC6607_REG_PREDATA_VALUE, val);
	if (ret < 0) {
		pr_err("failed: write predata\n");
		return -1;
	}
	pr_info("write predata 0x%0x\n", val);
	return ret;
}

static int sc6607_voocphy_set_txbuff(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;

	if (!chip) {
		pr_err("failed: chip is null\n");
		return -1;
	}

	ret = sc6607_voocphy_write_word(chip->client, SC6607_REG_TXBUF_DATA0, val);
	if (ret < 0) {
		pr_err("write txbuff\n");
		return -1;
	}

	return ret;
}

static int sc6607_voocphy_get_adapter_info(struct oplus_voocphy_manager *chip)
{
	s32 data;
	u8 reg_data[6] = { 0 };

	if (!chip) {
		pr_err("chip is null\n");
		return -1;
	}

	data = sc6607_voocphy_read_word(chip->client, SC6607_REG_ADAPTER_INFO);

	if (data < 0) {
		pr_err("sc6607_voocphy_read_word faile\n");
		return -1;
	}

	VOOCPHY_DATA16_SPLIT(data, chip->voocphy_rx_buff, chip->vooc_flag);
	pr_info("data: 0x%0x, vooc_flag: 0x%0x, vooc_rxdata: 0x%0x\n", data, chip->vooc_flag, chip->voocphy_rx_buff);

	if (sc6607_voocphy_dump_reg) {
		sc6607_voocphy_read_byte(chip->client, SC6607_REG_VAC_VBUS_OVP, &reg_data[0]); /*Vac Vbus ovp*/
		sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_INT_FLG, &reg_data[1]); /*Vbus error hi or low*/
		sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_FLT_FLG, &reg_data[2]); /*UCP*/
		sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_PMID2OUT_FLG, &reg_data[3]); /*PMID2OUT*/
		pr_info("0x04=0x%02x,0x67=0x%02x,0x6B=0x%02x,0x6C=0x%02x\n", reg_data[0], reg_data[1], reg_data[2],
			reg_data[3]);
	}

	return 0;
}

static void sc6607_voocphy_update_data(struct oplus_voocphy_manager *chip)
{
	u8 data_block[8] = { 0 };
	u8 data = 0;
	u8 state = 0;

	s32 ret = 0;

	if (!g_chip) {
		pr_err("g_chip null\n");
		return;
	}
	if (!disable_wdt)
		sc6607_reset_watchdog_timer(g_chip);

	/*int_flag*/
	sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_FLT_FLG, &data);
	chip->int_flag = data;

	/*int_flag*/
	sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_PMID2OUT_FLG, &state);

	sc6607_voocphy_match_err_value(chip, state, data);

	/*parse data_block for improving time of interrupt*/
	mutex_lock(&g_chip->adc_read_lock);
	sc6607_field_write(g_chip, F_ADC_FREEZE, 1);
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC6607_REG_HK_IBUS_ADC, 8, data_block);
	sc6607_field_write(g_chip, F_ADC_FREEZE, 0);
	mutex_unlock(&g_chip->adc_read_lock);
	if (ret < 0) {
		sc6607_voocphy_i2c_error(true);
		pr_err("read vsys vbat error \n");
	} else {
		sc6607_voocphy_i2c_error(false);
	}
	chip->cp_ichg = (((data_block[0] & SC6607_VOOCPHY_IBUS_POL_H_MASK) << SC6607_VOOCPHY_IBUS_POL_H_SHIFT) |
			 data_block[1]) *
			SC6607_VOOCPHY_IBUS_ADC_LSB;
	chip->cp_vbus = (((data_block[2] & SC6607_VOOCPHY_VBUS_POL_H_MASK) << SC6607_VOOCPHY_VBUS_POL_H_SHIFT) |
			 data_block[3]) *
			SC6607_VOOCPHY_VBUS_ADC_LSB;
	chip->cp_vac =
		(((data_block[4] & SC6607_VOOCPHY_VAC_POL_H_MASK) << SC6607_VOOCPHY_VAC_POL_H_SHIFT) | data_block[5]) *
		SC6607_VOOCPHY_VAC_ADC_LSB;
	chip->cp_vbat = (((data_block[6] & SC6607_VOOCPHY_VBAT_POL_H_MASK) << SC6607_VOOCPHY_VBAT_POL_H_SHIFT) |
			 data_block[7]) *
			SC6607_VOOCPHY_VBAT_ADC_LSB;

	chip->cp_vsys = sc6607_hk_get_adc(g_chip, SC6607_ADC_VSYS);
	chip->cp_vsys /= SC6607_UV_PER_MV;

	pr_info(" [%d, %d, %d, %d, %d, %d]", chip->cp_ichg,
		chip->cp_vbus, chip->cp_vac, chip->cp_vbat, chip->cp_vsys, chip->int_flag);
}

static int sc6607_voocphy_get_cp_ichg(struct oplus_voocphy_manager *chip)
{
	int cp_ichg = 0;
	u8 cp_enable = 0;

	if (!g_chip)
		return 0;

	sc6607_voocphy_get_chg_enable(chip, &cp_enable);

	if (cp_enable == 0)
		return 0;

	cp_ichg = sc6607_adc_read_ibus(g_chip);

	return cp_ichg;
}

static s32 sc6607_voocphy_thermistor_conver_temp(s32 res)
{
	int i = 0;
	int asize = 0;
	s32 res1 = 0, res2 = 0;
	s32 tap_value = -2000, tmp1 = 0, tmp2 = 0;
	asize = sizeof(pst_temp_table) / sizeof(struct sc6607_temp_param);

	if (res >= pst_temp_table[0].temperature_r) {
		tap_value = pst_temp_table[0].bts_temp; /* min */
	} else if (res <= pst_temp_table[asize - 1].temperature_r) {
		tap_value = pst_temp_table[asize - 1].bts_temp; /* max */
	} else {
		res1 = pst_temp_table[0].temperature_r;
		tmp1 = pst_temp_table[0].bts_temp;

		for (i = 0; i < asize; i++) {
			if (res >= pst_temp_table[i].temperature_r) {
				res2 = pst_temp_table[i].temperature_r;
				tmp2 = pst_temp_table[i].bts_temp;
				break;
			}
			res1 = pst_temp_table[i].temperature_r;
			tmp1 = pst_temp_table[i].bts_temp;
		}
		tap_value = (((res - res2) * tmp1) + ((res1 - res) * tmp2)) / (res1 - res2);
	}

	return tap_value;
}

int sc6607_voocphy_get_tsbus(void)
{
	int ret = 0;

	if (!oplus_voocphy_mg)
		return 0;

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return 0;
	}

	ret = sc6607_adc_read_tsbus(g_chip);
	ret = sc6607_voocphy_thermistor_conver_temp(ret);

	return ret;
}

int sc6607_voocphy_get_tsbat(void)
{
	s32 ret = 0;

	if (!oplus_voocphy_mg)
		return ret;

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return ret;
	}

	ret = sc6607_adc_read_tsbat(g_chip);
	ret = sc6607_voocphy_thermistor_conver_temp(ret);

	return ret;
}

static int sc6607_voocphy_get_cp_vbus(struct oplus_voocphy_manager *chip)
{
	if (!g_chip)
		return 0;
	else
		return sc6607_adc_read_vbus_volt(g_chip);
}

static int sc6607_voocphy_reg_reset(struct oplus_voocphy_manager *chip, bool enable)
{
	int ret;

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}
	ret = sc6607_field_write(g_chip, F_PHY_EN, enable);

	return ret;
}

static int sc6607_voocphy_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}
	ret = sc6607_field_read(g_chip, F_CP_EN, data);

	if (ret < 0) {
		pr_err("F_CP_EN err\n");
		return -1;
	}

	return ret;
}

static int sc6607_get_voocphy_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}
	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}
	ret = sc6607_field_read(g_chip, F_PHY_EN, data);

	if (ret < 0) {
		pr_err("SC6607_REG_PHY_CTRL\n");
		return -1;
	}
	pr_info("data = %d\n", *data);

	return ret;
}

static void sc6607_voocphy_dump_reg_in_err_issue(struct oplus_voocphy_manager *chip)
{
	int i = 0, p = 0;

	if (!chip) {
		pr_err("!!!!! oplus_voocphy_manager chip NULL");
		return;
	}

	for (i = 0x0; i < 0X0F; i++) {
		sc6607_voocphy_read_byte(chip->client, i, &chip->reg_dump[p]);
		p = p + 1;
	}
	for (i = 0x0; i < 0X0F; i++) {
		p = p + 1;
		sc6607_voocphy_read_byte(chip->client, 0x60 + i, &chip->reg_dump[p]);
	}
	for (i = 0x0; i < 0X0F; i++) {
		p = p + 1;
		sc6607_voocphy_read_byte(chip->client, 0xA0 + i, &chip->reg_dump[p]);
	}
	pr_info("p[%d], ", p);

	return;
}

static int sc6607_voopchy_get_adc_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}
	ret = sc6607_field_read(g_chip, F_ADC_EN, data);

	if (ret < 0) {
		pr_err("F_ADC_EN\n");
		return -1;
	}

	return ret;
}

static int sc6607_track_match_hk_err(struct sc6607 *chip, u8 data)
{
	int i;
	int ret;
	int err_type = TRACK_HK_ERR_DEFAULT;
	struct sc6607_track_check_reg track_check_reg[] = {
		{ SC6607_REG_HK_FLT_FLG, data }, { SC6607_REG_CP_FLT_FLG, 0 }, { SC6607_REG_CP_PMID2OUT_FLG, 0 },
	};

	if (!chip) {
		pr_err("%s: chip null\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(track_check_reg); i++) {
		if (track_check_reg[i].addr != SC6607_REG_HK_FLT_FLG)
			ret = sc6607_read_byte(chip, track_check_reg[i].addr, &track_check_reg[i].data);

		pr_err("read reg[0x%0x] = 0x%x \n", track_check_reg[i].addr, track_check_reg[i].data);
		if (track_check_reg[i].addr == SC6607_REG_HK_FLT_FLG) {
			if (track_check_reg[i].data & SC6607_HK_WD_TIMEOUT_MASK)
				err_type = TRACK_HK_ERR_WDOG_TIMEOUT;
			else if (track_check_reg[i].data & SC6607_HK_VAC_OVP_MASK)
				err_type = TRACK_HK_ERR_VAC_OVP;

			if (err_type == TRACK_HK_ERR_DEFAULT)
				break;
		}
	}

	if (chip->debug_force_hk_err)
		err_type = chip->debug_force_hk_err;
	if (err_type != TRACK_HK_ERR_DEFAULT)
		sc6607_track_upload_hk_err_info(chip, err_type, track_check_reg, ARRAY_SIZE(track_check_reg));

	return 0;
}

static u8 sc6607_voocphy_match_err_value(struct oplus_voocphy_manager *chip, u8 reg_state, u8 reg_flag)
{
	int err_type = TRACK_CP_ERR_DEFAULT;
	struct sc6607_track_check_reg track_check_reg[] = {
		{ SC6607_REG_HK_FLT_FLG, 0 },
		{ SC6607_REG_CP_FLT_FLG, reg_flag },
		{ SC6607_REG_CP_PMID2OUT_FLG, reg_state },
	};

	if (!chip) {
		pr_err("%s: chip null\n", __func__);
		return -EINVAL;
	}

	if (reg_state & SC6607_VOOCPHY_PIN_DIAG_FALL_FLAG_MASK)
		err_type = TRACK_CP_ERR_CFLY_CDRV_FAULT;
	else if (reg_flag & SC6607_VOOCPHY_SS_TIMEOUT_FLAG_MASK)
		err_type = TRACK_CP_ERR_SS_TIMEOUT;
	else if (reg_flag & SC6607_VOOCPHY_VBAT_OVP_FLAG_MASK)
		err_type = TRACK_CP_ERR_VBAT_OVP;
	else if (reg_flag & SC6607_VOOCPHY_VBATSNS_OVP_FLAG_MASK)
		err_type = TRACK_CP_ERR_VBATSNS_OVP;
	else if (reg_flag & SC6607_VOOCPHY_IBUS_OCP_FLAG_MASK)
		err_type = TRACK_CP_ERR_IBUS_OCP;
	else if (reg_state & SC6607_VOOCPHY_PMID2OUT_OVP_FLAG_MASK)
		err_type = TRACK_CP_ERR_PMID2OUT_OVP;

	if (chip->debug_force_cp_err)
		err_type = chip->debug_force_cp_err;
	if (err_type != TRACK_CP_ERR_DEFAULT) {
		sc6607_voocphy_read_byte(chip->client, track_check_reg[0].addr, &track_check_reg[0].data);
		sc6607_voocphy_track_upload_cp_err_info(chip, err_type, track_check_reg, ARRAY_SIZE(track_check_reg));
	}

	return 0;
}

static u8 sc6607_voocphy_get_int_value(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;
	u8 state = 0;

	if (!chip) {
		pr_err("%s: chip null\n", __func__);
		return -1;
	}

	ret = sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_FLT_FLG, &data); /*ibus ucp register*/
	if (ret < 0) {
		pr_err(" read SC6607_REG_CP_FLT_FLG failed\n");
		return -1;
	}

	ret = sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_PMID2OUT_FLG, &state); /*pmid2out protection*/
	if (ret < 0) {
		pr_err(" read SC6607_REG_CP_PMID2OUT_FLG failed\n");
		return -1;
	}

	sc6607_voocphy_match_err_value(chip, state, data);

	return data;
}

static int sc6607_voocphy_set_chg_enable(struct oplus_voocphy_manager *chip, bool enable)
{
	u8 data = 0;
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	if (enable)
		ret = sc6607_field_write(g_chip, F_CP_EN, true);
	else
		ret = sc6607_field_write(g_chip, F_CP_EN, false);
	sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_CTRL, &data); /*performance mode , CP mode*/

	return 0;
}

/* init ucp deglitch 160ms ,which can fix the bug */
static void sc6607_voopchy_set_pd_svooc_config(struct oplus_voocphy_manager *chip, bool enable)
{
	pr_info("enter\n");
}

static bool sc6607_voopchy_get_pd_svooc_config(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;

	if (!chip) {
		pr_err("Failed\n");
		return false;
	}

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}
	ret = sc6607_field_read(g_chip, F_IBUS_UCP_DIS, &data);
	if (ret < 0) {
		pr_err(" F_IBUS_UCP_DIS\n");
		return false;
	}

	if (data)
		return true;
	else
		return false;
}

static int sc6607_voocphy_set_adc_enable(struct oplus_voocphy_manager *chip, bool enable)
{
	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}
	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}

	if (enable)
		return sc6607_field_write(g_chip, F_ADC_EN, true);
	else
		return true;
}

static void sc6607_voocphy_send_handshake(struct oplus_voocphy_manager *chip)
{
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_PHY_CTRL, 0x81);
}

static int sc6607_voocphy_reset_voocphy(struct oplus_voocphy_manager *chip)
{
	if (!g_chip)
		return 0;
	pr_info(" reset\n");
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VOOCPHY_IRQ, 0x7F);
	/* close CP */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_CP_CTRL, 0x80);

	/* hwic config with plugout */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VBATSNS_OVP, 0x2E); /*Vbat ovp 4.65V*/
	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		sc6607_voocphy_write_byte(chip->client, SC6607_REG_VAC_VBUS_OVP, 0x21); /*Vac ovp 6.5V,Vbus OVP 10V*/
	else
		sc6607_voocphy_write_byte(chip->client, SC6607_REG_VAC_VBUS_OVP, 0x01); /*Vac ovp 12V,Vbus OVP 10V*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_IBUS_OCP_UCP, 0x6B); /*ucp deglitch1 160ms,IBUS OCP 3.75A*/

	/* clear tx data */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_TXBUF_DATA0, 0x00);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_TXBUF_DATA1, 0x00);

	/* set D+ HiZ */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_DPDM_CTRL, 0x00);

	/* select big bang mode */

	/* disable vooc */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_PHY_CTRL, 0x00);

	/* set predata */
	sc6607_voocphy_write_word(chip->client, SC6607_REG_PREDATA_VALUE, 0x0);
	sc6607_set_watchdog_timer(g_chip, 0);
	sc6607_field_write(g_chip, F_PERFORMANCE_EN, 0);

	return VOOCPHY_SUCCESS;
}

static int sc6607_voocphy_reactive_voocphy(struct oplus_voocphy_manager *chip)
{
	/*no test, need to check*/
	sc6607_voocphy_write_word(chip->client, SC6607_REG_PHY_CTRL, 0x0);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_DP_HOLD_TIME, 0x60); /*dp hold time to endtime*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_DPDM_CTRL, 0x24); /*dp dm pull down 20k*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_T5_T7_SETTING, 0xD1); /*T5~T7 setting*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_TXBUF_DATA0, 0x00);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_TXBUF_DATA1, 0x00);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VOOCPHY_IRQ, 0x05); /*mask rx start and txdone flag*/
	sc6607_voocphy_send_handshake(chip);

	return VOOCPHY_SUCCESS;
}

static int sc6607_voocphy_init_device(struct oplus_voocphy_manager *chip)
{
	if (!g_chip)
		return 0;

	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		sc6607_voocphy_write_byte(chip->client, SC6607_REG_VAC_VBUS_OVP, 0x11); /*Vac ovp 6.5V	 Vbus_ovp 10V*/
	else
		sc6607_voocphy_write_byte(chip->client, SC6607_REG_VAC_VBUS_OVP, 0x01); /*Vac ovp 12V	 Vbus_ovp 10V*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VBATSNS_OVP, 0x2E); /*VBAT_OVP:4.65V */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_IBUS_OCP_UCP, 0x6B); /* IBUS_OCP_UCP:3.75A */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_PHY_CTRL, 0x00); /*VOOC_CTRL:disable */
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_DP_HOLD_TIME, 0x60); /*dp hold time to endtime*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_CP_INT_MASK,
				  0x21); /*close ucp rising int,change to bit5 1 mask ucp rising int*/

	return 0;
}

static int sc6607_voocphy_init_vooc(struct oplus_voocphy_manager *chip)
{
	u8 data = 0;

	if (!g_chip) {
		pr_err("%s: g_chip null\n", __func__);
		return -1;
	}

	pr_info(" >>>>start init vooc\n");
	sc6607_voocphy_reg_reset(chip, true);
	sc6607_voocphy_init_device(chip);

	sc6607_voocphy_write_word(chip->client, SC6607_REG_PREDATA_VALUE, 0x0);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_DPDM_CTRL, 0x24); /*dp dm pull down 20k*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_T5_T7_SETTING, 0xD1); /*T5~T7 setting*/
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VOOCPHY_IRQ, 0x05); /*mask rx start and txdone flag*/

	if (g_chip) {
		sc6607_field_read(g_chip, F_DPDM_3P3_EN, &data);
		pr_info("data:%d\n", data);
		sc6607_field_write(g_chip, F_DPDM_3P3_EN, true);
	}
	return 0;
}

static int sc6607_voocphy_svooc_ovp_hw_setting(struct oplus_voocphy_manager *chip)
{
	int ret = 0;

	if (!g_chip)
		return 0;

	ret = sc6607_field_write(g_chip, F_VAC_OVP, 0x00);
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 0x01);

	return 0;
}

static int sc6607_voocphy_svooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	u8 data = 0;
	int ret = 0;

	if (!g_chip)
		return 0;

	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 0x02); /*VAC_OVP:12v VBUS_OVP:10v*/
	else
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 0x00); /*VAC_OVP:12v VBUS_OVP:10v*/
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 0x01);
	ret = sc6607_field_write(g_chip, F_IBUS_OCP, 0x0D); /*IBUS_OCP_UCP:4.25A*/
	ret = sc6607_set_watchdog_timer(g_chip, 1000);
	ret = sc6607_field_write(g_chip, F_MODE, 0x0);
	ret = sc6607_field_write(g_chip, F_PMID2OUT_OVP, 0x05);
	ret = sc6607_field_write(g_chip, F_CHG_EN, true);
	ret = sc6607_field_write(g_chip, F_PERFORMANCE_EN, true);
	sc6607_voocphy_read_byte(chip->client, SC6607_REG_CP_CTRL, &data);
	pr_info("data:0x%x\n", data);

	return 0;
}

static int sc6607_voocphy_vooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	int ret = 0;

	if (!g_chip)
		return 0;

	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 0); /*VAC_OVP:6.5v VBUS_OVP:6v*/
	else
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 2); /*VAC_OVP:6.5v VBUS_OVP:6v*/
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 0);
	ret = sc6607_field_write(g_chip, F_IBUS_OCP, 0x0F); /*IBUS_OCP_UCP:4.8A,160ms UCP*/
	ret = sc6607_set_watchdog_timer(g_chip, 1000);
	/*need to set bit1 bp:1 default 0; mode bit1 sc:0 bp:1 default 0  mos bit0, enable:1*/
	ret = sc6607_field_write(g_chip, F_MODE, 0x1);
	ret = sc6607_field_write(g_chip, F_CHG_EN, true);
	ret = sc6607_field_write(g_chip, F_PERFORMANCE_EN, true);

	return 0;
}

static int sc6607_voocphy_5v2a_hw_setting(struct oplus_voocphy_manager *chip)
{
	int ret = 0;

	if (!g_chip)
		return 0;

	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 0); /*VAC_OVP:6.5v VBUS_OVP:6v*/
	else
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 2); /*VAC_OVP:6.5v VBUS_OVP:6v*/
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 0);
	ret = sc6607_field_write(g_chip, F_MODE, 0x0); /*close CPx*/
	ret = sc6607_field_write(g_chip, F_CP_EN, 0x0);
	ret = sc6607_set_watchdog_timer(g_chip, 0);
	ret = sc6607_field_write(g_chip, F_PHY_EN, 0x0); /*VOOC_disable*/
	ret = sc6607_field_write(g_chip, F_CHG_EN, true); /*performance mode disable , CP mode*/
	ret = sc6607_field_write(g_chip, F_PERFORMANCE_EN, false);
	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VBATSNS_OVP, 0x2E); /* VBAT_OVP:4.65V */
	return 0;
}

static int sc6607_voocphy_pdqc_hw_setting(struct oplus_voocphy_manager *chip)
{
	int ret = 0;

	if (!g_chip)
		return 0;

	sc6607_voocphy_write_byte(chip->client, SC6607_REG_VBATSNS_OVP, 0x2E); /* VBAT_OVP:4.65V */
	if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 2); /*VAC_OVP:12v VBUS_OVP:10v*/
	else
		ret = sc6607_field_write(g_chip, F_VAC_OVP, 0); /*VAC_OVP:12v VBUS_OVP:10v*/
	ret = sc6607_field_write(g_chip, F_VBUS_OVP, 1);
	ret = sc6607_field_write(g_chip, F_PHY_EN, 0x0); /* close CP*/
	ret = sc6607_field_write(g_chip, F_MODE, 0x0);
	ret = sc6607_field_write(g_chip, F_CP_EN, 0x0);
	ret = sc6607_set_watchdog_timer(g_chip, 0);

	sc6607_voocphy_write_byte(chip->client, SC6607_REG_PHY_CTRL, 0x00); /*VOOC_disable*/
	ret = sc6607_field_write(g_chip, F_CHG_EN, true); /*performance mode disable , CP mode*/
	ret = sc6607_field_write(g_chip, F_PERFORMANCE_EN, false);
	return 0;
}

static int sc6607_voocphy_hw_setting(struct oplus_voocphy_manager *chip, int reason)
{
	if (!chip) {
		pr_err("chip is null exit\n");
		return -1;
	}

	switch (reason) {
	case SETTING_REASON_PROBE:
	case SETTING_REASON_RESET:
		sc6607_voocphy_init_device(chip);
		pr_info("SETTING_REASON_RESET OR PROBE\n");
		break;
	case SETTING_REASON_COPYCAT_SVOOC:
		sc6607_voocphy_svooc_ovp_hw_setting(chip);
		pr_info("SETTING_REASON_COPYCAT_SVOOC\n");
		break;
	case SETTING_REASON_SVOOC:
		sc6607_voocphy_svooc_hw_setting(chip);
		pr_info("SETTING_REASON_SVOOC\n");
		break;
	case SETTING_REASON_VOOC:
		sc6607_voocphy_vooc_hw_setting(chip);
		pr_info("SETTING_REASON_VOOC\n");
		break;
	case SETTING_REASON_5V2A:
		sc6607_voocphy_5v2a_hw_setting(chip);
		pr_info("SETTING_REASON_5V2A\n");
		break;
	case SETTING_REASON_PDQC:
		sc6607_voocphy_pdqc_hw_setting(chip);
		pr_info("SETTING_REASON_PDQC\n");
		break;
	default:
		pr_err("do nothing\n");
		break;
	}

	return 0;
}

static int sc6607_voocphy_dump_registers(struct oplus_voocphy_manager *chip)
{
	int rc = 0;
	u8 addr;
	u8 val_buf[16] = { 0x0 };

	if (atomic_read(&chip->suspended) == 1) {
		pr_err("sc6607 is suspend!\n");
		return -ENODEV;
	}

	for (addr = SC6607_REG_VBATSNS_OVP; addr <= SC6607_REG_CP_PMID2OUT_FLG; addr++) {
		rc = sc6607_voocphy_read_byte(chip->client, addr, &val_buf[addr]);
		if (rc < 0) {
			pr_err("Couldn't read 0x%02x, rc = %d\n", addr, rc);
			break;
		}
	}

	pr_info(":[0~5][0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", val_buf[0], val_buf[1], val_buf[2], val_buf[3],
		val_buf[4], val_buf[5]);
	pr_info(":[6~c][0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", val_buf[6], val_buf[7], val_buf[8], val_buf[9],
		val_buf[0xa], val_buf[0xb], val_buf[0xc]);

	return 0;
}

static int sc6607_voocphy_parse_dt(struct oplus_voocphy_manager *chip)
{
	int rc;
	struct device_node *node = NULL;

	if (!chip) {
		pr_debug("chip null\n");
		return -1;
	}

	node = chip->dev->of_node;
	chip->switch1_gpio = -1;

	rc = of_property_read_u32(node, "ovp_reg", &chip->ovp_reg);
	if (rc)
		chip->ovp_reg = 0xE;
	else
		pr_info("ovp_reg is %d\n", chip->ovp_reg);

	rc = of_property_read_u32(node, "ocp_reg", &chip->ocp_reg);
	if (rc)
		chip->ocp_reg = 0x8;
	else
		pr_info("ocp_reg is %d\n", chip->ocp_reg);

	rc = of_property_read_u32(node, "qcom,voocphy_vbus_low", &chip->voocphy_vbus_low);
	if (rc)
		chip->voocphy_vbus_low = SC6607_DEFUALT_VBUS_LOW;
	pr_info("voocphy_vbus_low is %d\n", chip->voocphy_vbus_low);

	rc = of_property_read_u32(node, "qcom,voocphy_vbus_high", &chip->voocphy_vbus_high);
	if (rc)
		chip->voocphy_vbus_high = SC6607_DEFUALT_VBUS_HIGH;
	pr_info("voocphy_vbus_high is %d\n", chip->voocphy_vbus_high);

	return 0;
}

static void sc6607_voocphy_set_switch_fast_charger(struct oplus_voocphy_manager *chip)
{
	pr_info("return\n");
	return;
}

static void sc6607_voocphy_set_switch_normal_charger(struct oplus_voocphy_manager *chip)
{
	pr_info("return\n");
	return;
}

static void sc6607_voocphy_set_switch_mode(struct oplus_voocphy_manager *chip, int mode)
{
	switch (mode) {
	case VOOC_CHARGER_MODE:
		sc6607_voocphy_set_switch_fast_charger(chip);
		break;
	case NORMAL_CHARGER_MODE:
	default:
		sc6607_voocphy_set_switch_normal_charger(chip);
		break;
	}

	return;
}

static struct oplus_voocphy_operations sc6607_voocphy_ops = {
	.hw_setting = sc6607_voocphy_hw_setting,
	.init_vooc = sc6607_voocphy_init_vooc,
	.set_predata = sc6607_voocphy_set_predata,
	.set_txbuff = sc6607_voocphy_set_txbuff,
	.get_adapter_info = sc6607_voocphy_get_adapter_info,
	.update_data = sc6607_voocphy_update_data,
	.get_chg_enable = sc6607_voocphy_get_chg_enable,
	.set_chg_enable = sc6607_voocphy_set_chg_enable,
	.reset_voocphy = sc6607_voocphy_reset_voocphy,
	.reactive_voocphy = sc6607_voocphy_reactive_voocphy,
	.set_switch_mode = sc6607_voocphy_set_switch_mode,
	.send_handshake = sc6607_voocphy_send_handshake,
	.get_cp_vbat = oplus_sc6607_get_vbat,
	.get_cp_vbus = sc6607_voocphy_get_cp_vbus,
	.get_int_value = sc6607_voocphy_get_int_value,
	.get_adc_enable = sc6607_voopchy_get_adc_enable,
	.set_adc_enable = sc6607_voocphy_set_adc_enable,
	.get_ichg = sc6607_voocphy_get_cp_ichg,
	.set_pd_svooc_config = sc6607_voopchy_set_pd_svooc_config,
	.get_pd_svooc_config = sc6607_voopchy_get_pd_svooc_config,
	.get_voocphy_enable = sc6607_get_voocphy_enable,
	.dump_voocphy_reg = sc6607_voocphy_dump_reg_in_err_issue,
};

static int sc6607_voocphy_charger_choose(struct oplus_voocphy_manager *chip)
{
	int ret;

	if (!oplus_voocphy_chip_is_null()) {
		pr_err("oplus_voocphy_chip already exists!");
		return 0;
	} else {
		ret = i2c_smbus_read_byte_data(chip->client, 0x07);
		pr_info("0x07 = %d\n", ret);
		if (ret < 0) {
			pr_err("i2c communication fail");
			return -EPROBE_DEFER;
		} else
			return 1;
	}
}

static struct of_device_id sc6607_charger_match_table[] = {
	{.compatible = "oplus,sc6607-charger", },
	{},
};
MODULE_DEVICE_TABLE(of, sc6607_charger_match_table);

static const struct i2c_device_id sc6607_i2c_device_id[] = {
	{ "sc6607", 0x66 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sc6607_i2c_device_id);

static int sc6607_charger_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sc6607 *chip;
	struct oplus_voocphy_manager *chip_voocphy;
	struct device_node *node = client->dev.of_node;
	int ret = 0;
	int reg_i = 0;

	chip = devm_kzalloc(&client->dev, sizeof(struct sc6607), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &client->dev;
	chip->client = client;
	g_chip = chip;

	i2c_set_clientdata(client, chip);
	mutex_init(&i2c_rw_lock);
	mutex_init(&chip->dpdm_lock);
	mutex_init(&chip->bc12.running_lock);
	mutex_init(&chip->adc_read_lock);

	chip->regmap = devm_regmap_init_i2c(client, &sc6607_regmap_cfg);
	if (IS_ERR(chip->regmap)) {
		pr_err("failed to allocate register map\n");
		ret = -EINVAL;
		goto err_regmap;
	}
	for (reg_i = 0; reg_i < ARRAY_SIZE(sc6607_reg_fields); reg_i++) {
		chip->regmap_fields[reg_i] = devm_regmap_field_alloc(chip->dev, chip->regmap, sc6607_reg_fields[reg_i]);
		if (IS_ERR(chip->regmap_fields[reg_i])) {
			pr_err("cannot allocate regmap field\n");
			ret = -EINVAL;
			goto err_regmap_fields;
		}
	}

	chip->platform_data = sc6607_parse_dt(node, chip);
	if (!chip->platform_data) {
		pr_err("No platform data provided.\n");
		ret = -EINVAL;
		goto err_parse_dt;
	}

	ret = sc6607_reset_chip(chip);
	ret |= sc6607_check_device_id(chip);
	ret |= sc6607_init_device(chip);
	if (ret) {
		pr_err("Failed to init device\n");
		goto err_init;
	}

	atomic_set(&chip->driver_suspended, 0);
	atomic_set(&chip->charger_suspended, 0);
	oplus_chg_awake_init(chip);
	init_waitqueue_head(&chip->wait);
	oplus_keep_resume_awake_init(chip);

	chip->chg_type = CHARGER_UNKNOWN;
	chip->oplus_chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
	sc6607_set_usb_props_type(chip->oplus_chg_type);
	chip->usb_connect_start = false;
	chip->bc12.detect_ing = false;

	sc6607_disable_hvdcp(chip);
	INIT_DELAYED_WORK(&(chip->bc12.detect_work), sc6607_soft_bc12_work_func);
	INIT_DELAYED_WORK(&chip->hw_bc12_detect_work, sc6607_hw_bc12_work_func);
	INIT_DELAYED_WORK(&chip->init_status_work, sc6607_init_status_work);
	INIT_DELAYED_WORK(&chip->sc6607_aicr_setting_work, sc6607_aicr_setting_work_callback);

	ret = sc6607_register_interrupt(node, chip);
	if (ret) {
		pr_err("Failed to register irq ret=%d\n", ret);
		goto err_irq;
	}
#ifdef CONFIG_OPLUS_CHARGER_MTK
	g_chip->chg_dev = charger_device_register(g_chip->chg_dev_name,
						&client->dev, g_chip,
						&sc6607_chg_ops,
						&sc6607_chg_props);
		if (IS_ERR_OR_NULL(g_chip->chg_dev)) {
		ret = PTR_ERR(g_chip->chg_dev);
		goto err_device_register;
	}

	if (ret) {
		dev_err(g_chip->dev, "failed to register sysfs. err: %d\n", ret);
		goto err_sysfs_create;
	}
#endif

	sc6607_charger_create_device_node(chip->dev);
	if (oplus_is_rf_ftm_mode()) {
		pr_info("disable_charger for ftm mode.\n");
		sc6607_enter_hiz_mode(chip);
	}

	set_charger_ic(SC6607);
	atomic_set(&chip->otg_enable_cnt, 0);
	chip->request_otg = 0;
	sc6607_buck_track_init(chip);
	pr_err("sc6607_charger_probe success\n");

	chip_voocphy = devm_kzalloc(&client->dev, sizeof(*chip_voocphy), GFP_KERNEL);
	if (!chip_voocphy) {
		dev_err(&client->dev, "Couldn't allocate memory\n");
		goto err_irq;
	}

	chip_voocphy->client = client;
	chip_voocphy->dev = &client->dev;
	i2c_set_clientdata(client, chip_voocphy);

	ret = sc6607_voocphy_charger_choose(chip_voocphy);
	if (ret <= 0)
		goto err_irq;

	sc6607_voocphy_parse_dt(chip_voocphy);
	sc6607_voocphy_dump_registers(chip_voocphy);
	sc6607_voocphy_reg_reset(chip_voocphy, true);
	sc6607_voocphy_init_device(chip_voocphy);

	chip_voocphy->irq = chip->irq;
	chip_voocphy->ops = &sc6607_voocphy_ops;

	oplus_voocphy_init(chip_voocphy);
	oplus_voocphy_mg = chip_voocphy;

	sc6607_voocphy_track_init(chip_voocphy);
	init_proc_voocphy_debug();

	sc6607_irq_handler(0, chip);
#ifdef CONFIG_OPLUS_CHARGER_MTK
	mutex_init(&chip->charger_pd_lock);
	chip->pd_type = PD_CONNECT_NONE;
	chip->tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (chip->tcpc != NULL) {
		chip->pd_nb.notifier_call = pd_tcp_notifier_call;
		ret = register_tcp_dev_notifier(chip->tcpc, &chip->pd_nb,
			TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC);
	} else {
		pr_err("get tcpc dev fail\n");
	}
	chip->pd_adapter = get_adapter_by_name("pd_adapter");
	if (chip->pd_adapter)
		pr_info("Found PD adapter [%s]\n", chip->pd_adapter->props.alias_name);
#endif
	if (oplus_is_rf_ftm_mode())
		schedule_delayed_work(&chip->init_status_work,
				msecs_to_jiffies(INIT_STATUS_TIME_5S));
	pr_err("sc6607_voocphy_parse_dt successfully!\n");

	return 0;

#ifdef CONFIG_OPLUS_CHARGER_MTK
err_sysfs_create:
	charger_device_unregister(g_chip->chg_dev);
err_device_register:
#endif
err_irq:
err_init:
err_parse_dt:
err_regmap_fields:
err_regmap:
	mutex_destroy(&chip->bc12.running_lock);
	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&i2c_rw_lock);
	mutex_destroy(&chip->adc_read_lock);
	devm_kfree(chip->dev, chip);
	g_chip = NULL;
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static unsigned long suspend_tm_sec = 0;
static int get_rtc_time(unsigned long *rtc_time)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("Failed to open rtc device (%s)\n", CONFIG_RTC_HCTOSYS_DEVICE);
		return -EINVAL;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Failed to read rtc time (%s) : %d\n", CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n", CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}
	rtc_tm_to_time(&tm, rtc_time);

close_time:
	rtc_class_close(rtc);
	return rc;
}

static int sc6607_pm_resume(struct device *dev)
{
	struct sc6607 *chip = NULL;
	unsigned long resume_tm_sec = 0;
	unsigned long sleep_time = 0;
	int rc = 0;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			pr_info("set driver_suspended\n");
			atomic_set(&chip->driver_suspended, 0);
			/*wake_up_interruptible(&chip->wait);*/
			rc = get_rtc_time(&resume_tm_sec);
			if (rc || suspend_tm_sec == -1) {
				pr_err("RTC read failed\n");
				sleep_time = 0;
			} else {
				sleep_time = resume_tm_sec - suspend_tm_sec;
			}
			if ((resume_tm_sec > suspend_tm_sec) && (sleep_time > 60))
				oplus_chg_soc_update_when_resume(sleep_time);
		}
	}

	return 0;
}

static int sc6607_pm_suspend(struct device *dev)
{
	struct sc6607 *chip = NULL;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			pr_info(" set driver_suspended\n");
			atomic_set(&chip->driver_suspended, 1);
			if (get_rtc_time(&suspend_tm_sec)) {
				pr_err("RTC read failed\n");
				suspend_tm_sec = -1;
			}
		}
	}

	return 0;
}

static const struct dev_pm_ops sc6607_pm_ops = {
	.resume = sc6607_pm_resume,
	.suspend = sc6607_pm_suspend,
};
#else
static int sc6607_resume(struct i2c_client *client)
{
	struct sc6607 *chip = i2c_get_clientdata(client);

	if (chip)
		atomic_set(&chip->driver_suspended, 0);

	return 0;
}

static int sc6607_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct sc6607 *chip = i2c_get_clientdata(client);

	if (!chip)
		atomic_set(&chip->driver_suspended, 1);

	return 0;
}
#endif

static int sc6607_charger_remove(struct i2c_client *client)
{
	struct sc6607 *chip = i2c_get_clientdata(client);

	if (chip) {
		mutex_destroy(&i2c_rw_lock);
		cancel_delayed_work_sync(&chip->bc12.detect_work);
	}

	return 0;
}

static void sc6607_charger_shutdown(struct i2c_client *client)
{
	if (g_chip) {
		if (g_chip->chip_id == SC6607_1P0_CHIP_ID)
			sc6607_field_write(g_chip, F_TSBAT_JEITA_DIS, true);
		else
			sc6607_field_write(g_chip, F_TSBAT_JEITA_DIS, false);
		sc6607_field_write(g_chip, F_ADC_EN, 0);
		sc6607_field_write(g_chip, F_ACDRV_MANUAL_PRE, 3);
		/*sc6607_field_write(g_chip, F_ACDRV_MANUAL_EN, 0);*/
	}
}

static struct i2c_driver sc6607_charger_driver = {
	.driver =
		{
			.name = "sc6607-charger",
			.owner = THIS_MODULE,
			.of_match_table = sc6607_charger_match_table,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
			.pm = &sc6607_pm_ops,
#endif
		},

	.probe = sc6607_charger_probe,
	.remove = sc6607_charger_remove,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	.resume = sc6607_resume,
	.suspend = sc6607_suspend,
#endif
	.shutdown = sc6607_charger_shutdown,
	.id_table = sc6607_i2c_device_id,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
module_i2c_driver(sc6607_charger_driver);
#else
void sc6607_charger_exit(void)
{
	i2c_del_driver(&sc6607_charger_driver);
}

int sc6607_charger_init(void)
{
	int ret = 0;

	oplus_chg_ops_register("ext-sc6607", &oplus_chg_sc6607_ops);
	if (i2c_add_driver(&sc6607_charger_driver) != 0)
		pr_err("failed to register sc6607 i2c driver.\n");
	else
		pr_info("Success to register sc6607 i2c driver.\n");

	return ret;
}
#endif

MODULE_DESCRIPTION("SC6607 Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("LiZhiJie");
