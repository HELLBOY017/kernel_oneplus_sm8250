// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
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
#include <linux/iio/consumer.h>

#include "../oplus_charger.h"
#include "../oplus_gauge.h"
#include "../oplus_vooc.h"
#include "../oplus_short.h"
#include "../oplus_wireless.h"
#include "../oplus_adapter.h"
#include "../charger_ic/oplus_short_ic.h"
#include "../gauge_ic/oplus_bq27541.h"
#include <soc/oplus/system/boot_mode.h>
#include "../oplus_chg_ops_manager.h"
#include "../voocphy/oplus_voocphy.h"
#include "oplus_sgm41542.h"
#include <linux/pm_wakeup.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <tcpm.h>
#include <tcpci.h>
#include "../oplus_chg_track.h"

struct chip_sgm41542 {
	struct device		*dev;
	struct i2c_client	*client;
	int			irq_gpio;
	int			enable_gpio;
	struct pinctrl 		*pinctrl;
	struct pinctrl_state 	*enable_gpio_active;
	struct pinctrl_state 	*enable_gpio_sleep;
	enum power_supply_type	oplus_charger_type;
	atomic_t		charger_suspended;
	atomic_t		is_suspended;
	const char *chg_dev_name;
	struct charger_device *chg_dev;
	struct mutex		i2c_lock;
	struct mutex		dpdm_lock;
	struct regulator	*dpdm_reg;
	struct tcpc_device *tcpc;
	struct notifier_block pd_nb;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	bool			dpdm_enabled;
	bool			power_good;
	bool			is_sgm41542;
	bool			is_bq25601d;
	bool			bc12_done;
	char			bc12_delay_cnt;
	char			bc12_retried;
	int			hw_aicl_point;
	int			sw_aicl_point;
	int			reg_access;
	int			before_suspend_icl;
	int			before_unsuspend_icl;
	bool                    batfet_reset_disable;
	bool			use_voocphy;
	struct delayed_work	init_work;
	struct delayed_work	bc12_retry_work;
	struct delayed_work	qc_detect_work;
	bool support_hvdcp;
	bool is_hvdcp;
	bool pre_is_hvdcp;

	struct wakeup_source *suspend_ws;
	/*fix chgtype identify error*/
	struct wakeup_source *keep_resume_ws;
	wait_queue_head_t wait;
};

static struct chip_sgm41542 *charger_ic = NULL;
static int aicl_result = 500;
static bool btb_detect_over;
static bool dumpreg_by_irq = 0;

static const struct charger_properties sgm41542_chg_props = {
	.alias_name = "sgm41542",
};

int __attribute__((weak)) oplus_chg_enable_qc_detect(void)
{
	return 0;
}
int oplus_sgm41542_get_pd_type(void)
{
	return 0;
}
static int oplus_get_boot_reason(void)
{
	return 0;
}
static void oplus_notify_device_mode(bool enable)
{
}
bool oplus_sgm41542_get_bc12_done(void)
{
	if(!charger_ic)
	    return false;

	return charger_ic->bc12_done;
}
EXPORT_SYMBOL(oplus_sgm41542_get_bc12_done);

int oplus_sgm41542_get_charger_type(void)
{
	if(!charger_ic)
	    return -1;

	return charger_ic->oplus_charger_type;
}
EXPORT_SYMBOL(oplus_sgm41542_get_charger_type);

void oplus_sgm41542_set_ap_current(int value)
{
}
EXPORT_SYMBOL(oplus_sgm41542_set_ap_current);

bool oplus_sgm41542_get_slave_chg_en(void)
{
	return 0;
}
EXPORT_SYMBOL(oplus_sgm41542_get_slave_chg_en);

void oplus_sgm41542_set_chargerid_switch_val(int value)
{
	mt_set_chargerid_switch_val(value);
}

int oplus_sgm41542_get_chargerid_switch_val(void)
{
	return mt_get_chargerid_switch_val();
}

int oplus_sgm41542_get_chargerid_volt(void)
{
	return 0;
}

static int sgm41542_request_dpdm(struct chip_sgm41542 *chip, bool enable);
static int sgm41542_inform_charger_type(struct chip_sgm41542 *chip);
static void sgm41542_get_bc12(struct chip_sgm41542 *chip);
static void oplus_chg_wakelock(struct chip_sgm41542 *chip, bool awake);

#define READ_REG_CNT_MAX	20
#define USLEEP_5000MS	5000
static int __sgm41542_read_reg(struct chip_sgm41542 *chip, int reg, int *data)
{
	s32 ret = 0;
	int retry = READ_REG_CNT_MAX;

	ret = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0) {
		while(retry > 0 && atomic_read(&chip->is_suspended) == 0) {
			usleep_range(USLEEP_5000MS, USLEEP_5000MS);
			ret = i2c_smbus_read_byte_data(chip->client, reg);
			if (ret < 0) {
				retry--;
			} else {
				break;
			}
		}
	}

	if (ret < 0) {
		chg_err("i2c read fail: can't read from %02x: %d\n", reg, ret);
		return ret;
	} else {
		*data = ret;
	}

	return 0;
}

static int __sgm41542_write_reg(struct chip_sgm41542 *chip, int reg, int val)
{
	s32 ret = 0;
	int retry = 3;

	ret = i2c_smbus_write_byte_data(chip->client, reg, val);
	if (ret < 0) {
		while(retry > 0) {
			usleep_range(5000, 5000);
			ret = i2c_smbus_write_byte_data(chip->client, reg, val);
			if (ret < 0)
				retry--;
			else
				break;
		}
	}

	if (ret < 0) {
		chg_err("i2c write fail: can't write %02x to %02x: %d\n", val, reg, ret);
		return ret;
	}

	return 0;
}

static int sgm41542_read_reg(struct chip_sgm41542 *chip, int reg, int *data)
{
	int ret;

	mutex_lock(&chip->i2c_lock);
	ret = __sgm41542_read_reg(chip, reg, data);
	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static __maybe_unused int sgm41542_write_reg(struct chip_sgm41542 *chip, int reg, int data)
{
	int ret;

	mutex_lock(&chip->i2c_lock);
	ret = __sgm41542_write_reg(chip, reg, data);
	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static __maybe_unused int sgm41542_config_interface(struct chip_sgm41542 *chip, int reg, int data, int mask)
{
	int ret;
	int tmp;

	mutex_lock(&chip->i2c_lock);
	ret = __sgm41542_read_reg(chip, reg, &tmp);
	if (ret) {
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sgm41542_write_reg(chip, reg, tmp);
	if (ret)
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

int sgm41542_set_vindpm_vol(int vindpm)
{
	int rc = 0;
	int tmp = 0;
	int offset = 0, offset_val = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	chg_err("vindpm = %d\n", vindpm);

	if(vindpm < SGM41542_VINDPM_THRESHOLD_5900MV) {
		offset = VINDPM_OS_3900mV;
		offset_val = SGM41542_VINDPM_THRESHOLD_3900MV;
	} else if (vindpm < SGM41542_VINDPM_THRESHOLD_7500MV) {
		offset = VINDPM_OS_5900mV;
		offset_val = SGM41542_VINDPM_THRESHOLD_5900MV;
	} else if (vindpm < SGM41542_VINDPM_THRESHOLD_10500MV) {
		offset = VINDPM_OS_7500mV;
		offset_val = SGM41542_VINDPM_THRESHOLD_7500MV;
	} else if (vindpm <= SGM41542_VINDPM_THRESHOLD_MAX) {
		offset = VINDPM_OS_10500mV;
		offset_val = SGM41542_VINDPM_THRESHOLD_10500MV;
	}

	/*set input os*/
	rc = sgm41542_config_interface(chip, REG0F_SGM41542_ADDRESS,
			offset,
			REG0F_SGM41542_VINDPM_THRESHOLD_OFFSET_MASK);

	/*set input vindpm*/
	tmp = (vindpm - offset_val) / REG06_SGM41542_VINDPM_STEP_MV;
	rc = sgm41542_config_interface(chip, REG06_SGM41542_ADDRESS,
			tmp << REG06_SGM41542_VINDPM_SHIFT,
			REG06_SGM41542_VINDPM_MASK);

	return rc;
}

int sgm41542_usb_icl[] = {
	300, 500, 900, 1200, 1350, 1500, 1750, 2000, 3000,
};

static int sgm41542_get_usb_icl(void)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	rc = sgm41542_read_reg(chip, REG00_SGM41542_ADDRESS, &tmp);
	if (rc) {
		chg_err("Couldn't read REG00_SGM41542_ADDRESS rc = %d\n", rc);
		return 0;
	}
	tmp = (tmp & REG00_SGM41542_INPUT_CURRENT_LIMIT_MASK) >> REG00_SGM41542_INPUT_CURRENT_LIMIT_SHIFT;
	return (tmp * REG00_SGM41542_INPUT_CURRENT_LIMIT_STEP + REG00_SGM41542_INPUT_CURRENT_LIMIT_OFFSET);
}

int sgm41542_input_current_limit_without_aicl(int current_ma)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1) {
		chg_err("in suspend\n");
		return 0;
	}

	if (current_ma > REG00_SGM41542_INPUT_CURRENT_LIMIT_MAX)
		current_ma = REG00_SGM41542_INPUT_CURRENT_LIMIT_MAX;

	if (current_ma < REG00_SGM41542_INPUT_CURRENT_LIMIT_OFFSET)
		current_ma = REG00_SGM41542_INPUT_CURRENT_LIMIT_OFFSET;

	tmp = (current_ma - REG00_SGM41542_INPUT_CURRENT_LIMIT_OFFSET) / REG00_SGM41542_INPUT_CURRENT_LIMIT_STEP;
	chg_err("tmp current [%d]ma\n", current_ma);
	rc = sgm41542_config_interface(chip, REG00_SGM41542_ADDRESS,
			tmp << REG00_SGM41542_INPUT_CURRENT_LIMIT_SHIFT,
			REG00_SGM41542_INPUT_CURRENT_LIMIT_MASK);

	if (rc < 0)
		chg_err("Couldn't set aicl rc = %d\n", rc);

	return rc;
}

int sgm41542_chg_get_dyna_aicl_result(void)
{
	return aicl_result;
}

#define BATT_VOL_4V14	4140
#define SGM41542_INP_VOL_4V44	4440
#define SGM41542_INP_VOL_4V5	4500
#define SGM41542_INP_VOL_4V52	4520
#define SGM41542_INP_VOL_4V535	4535
void sgm41542_set_aicl_point(int vbatt)
{
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return;

	if (chip->hw_aicl_point == SGM41542_INP_VOL_4V44 && vbatt > BATT_VOL_4V14) {
		chip->hw_aicl_point = SGM41542_INP_VOL_4V52;
		chip->sw_aicl_point = SGM41542_INP_VOL_4V535;
		sgm41542_set_vindpm_vol(chip->hw_aicl_point);
	} else if (chip->hw_aicl_point == SGM41542_INP_VOL_4V52 && vbatt < BATT_VOL_4V14) {
		chip->hw_aicl_point = SGM41542_INP_VOL_4V44;
		chip->sw_aicl_point = SGM41542_INP_VOL_4V5;
		sgm41542_set_vindpm_vol(chip->hw_aicl_point);
	}
}

int sgm41542_get_charger_vol(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	int vol_vaule = 0;
	get_vbus_voltage(&vol_vaule);
	chg_err("sgm41542_get_charger_vol: %d\n", vol_vaule);
	return vol_vaule;
	#else
	return qpnp_get_prop_charger_voltage_now();
	#endif
}

int sgm41542_get_vbus_voltage(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	int vol_vaule = 0;
	get_vbus_voltage(&vol_vaule);
	chg_err("sgm41542_get_charger_vol: %d\n", vol_vaule);
	return vol_vaule;
	#else
	return qpnp_get_prop_charger_voltage_now();
	#endif
}

int sgm41542_get_ibus_current(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	return 0;
	#else
	return qpnp_get_prop_ibus_now();
	#endif
}

int sgm41542_input_current_limit_write(int current_ma)
{
	int i = 0, rc = 0;
	int chg_vol = 0;
	int sw_aicl_point = 0;
	struct chip_sgm41542 *chip = charger_ic;
	int pre_icl_index = 0, pre_icl = 0;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	if (atomic_read(&chip->is_suspended) == 1) {
		chg_err("suspend,ignore set current=%dmA\n", current_ma);
		return 0;
	}

	/*first: icl down to 500mA, step from pre icl*/
	pre_icl = sgm41542_get_usb_icl();
	for (pre_icl_index = ARRAY_SIZE(sgm41542_usb_icl) - 1; pre_icl_index >= 0; pre_icl_index--) {
		if (sgm41542_usb_icl[pre_icl_index] < pre_icl)
			break;
	}
	chg_err("icl_set: %d, pre_icl: %d, pre_icl_index: %d\n", current_ma, pre_icl, pre_icl_index);

	for (i = pre_icl_index; i > 1; i--) {
		rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
		if (rc)
			chg_err("icl_down: set icl to %d mA fail, rc=%d\n", sgm41542_usb_icl[i], rc);
		else
			chg_err("icl_down: set icl to %d mA\n", sgm41542_usb_icl[i]);
		msleep(50);
	}

	/*second: aicl process, step from 500ma*/
	if (current_ma < 500) {
		i = 0;
		goto aicl_end;
	}

	sw_aicl_point = chip->sw_aicl_point;

	i = 1; /* 500 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		chg_debug("use 500 here\n");
		goto aicl_end;
	} else if (current_ma < 900)
		goto aicl_end;
	i = 2; /* 900 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 1;
		goto aicl_pre_step;
	} else if (current_ma < 1200)
		goto aicl_end;
	i = 3; /* 1200 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 1;
		goto aicl_pre_step;
	}
	i = 4; /* 1350 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 2; /*We DO NOT use 1.2A here*/
		goto aicl_pre_step;
	} else if (current_ma < 1350) {
		i = i - 1; /*We use 1.2A here*/
		goto aicl_end;
	}
	i = 5; /* 1500 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 3; /*We DO NOT use 1.2A here*/
		goto aicl_pre_step;
	} else if (current_ma < 1500) {
		i = i - 2; /*We use 1.2A here*/
		goto aicl_end;
	} else if (current_ma < 2000) {
		goto aicl_end;
	}
	i = 6; /* 1750 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 3; /*1.2*/
		goto aicl_pre_step;
	}
	i = 7; /* 2000 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 2; /*1.5*/
		goto aicl_pre_step;
	} else if (current_ma < 3000) {
		goto aicl_end;
	}
	i = 8; /* 3000 */
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	usleep_range(90000, 91000);
	chg_vol = sgm41542_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i -1;
		goto aicl_pre_step;
	} else if (current_ma >= 3000) {
		goto aicl_end;
	}
aicl_pre_step:
	chg_debug("usb input max current limit aicl chg_vol=%d j[%d]=%d sw_aicl_point:%d aicl_pre_step\n", chg_vol, i, sgm41542_usb_icl[i], sw_aicl_point);
	goto aicl_rerun;
aicl_end:
	chg_debug("usb input max current limit aicl chg_vol=%d j[%d]=%d sw_aicl_point:%d aicl_end\n", chg_vol, i, sgm41542_usb_icl[i], sw_aicl_point);
	goto aicl_rerun;
aicl_rerun:
	aicl_result = sgm41542_usb_icl[i];
	rc = sgm41542_input_current_limit_without_aicl(sgm41542_usb_icl[i]);
	rc = sgm41542_set_vindpm_vol(chip->hw_aicl_point);
	return rc;
}

int sgm41542_input_current_limit_ctrl_by_vooc_write(int current_ma)
{
	int rc = 0;
	int cur_usb_icl  = 0;
	int temp_curr = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	if (atomic_read(&chip->is_suspended) == 1) {
		chg_err("suspend,ignore set current=%dmA\n", current_ma);
		return 0;
	}

	cur_usb_icl = sgm41542_get_usb_icl();
	chg_err(" get cur_usb_icl = %d\n", cur_usb_icl);

	if (current_ma > cur_usb_icl) {
		for (temp_curr = cur_usb_icl; temp_curr < current_ma; temp_curr += 500) {
			msleep(35);
			rc = sgm41542_input_current_limit_without_aicl(temp_curr);
			chg_err("[up] set input_current = %d\n", temp_curr);
		}
	} else {
		for (temp_curr = cur_usb_icl; temp_curr > current_ma; temp_curr -= 500) {
			msleep(35);
			rc = sgm41542_input_current_limit_without_aicl(temp_curr);
			chg_err("[down] set input_current = %d\n", temp_curr);
		}
	}

	rc = sgm41542_input_current_limit_without_aicl(current_ma);
	return rc;
}

static ssize_t sgm41542_access_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct chip_sgm41542 *chip = dev_get_drvdata(dev);
	if (!chip) {
		chg_err("chip is null\n");
		return 0;
	}
	return sprintf(buf, "0x%02x\n", chip->reg_access);
}

static ssize_t sgm41542_access_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct chip_sgm41542 *chip = dev_get_drvdata(dev);
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return 0;
	}

	if (buf != NULL && size != 0) {
		pr_info("[%s] buf is %s and size is %zu\n", __func__, buf, size);

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&chip->reg_access);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&chip->reg_access);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err("[%s] write sgm41542 reg 0x%02x with value 0x%02x !\n",
					__func__, (unsigned int) chip->reg_access, reg_value);
			ret = sgm41542_write_reg(chip, chip->reg_access, reg_value);
		} else {
			ret = sgm41542_read_reg(chip, chip->reg_access, &reg_value);
			pr_err("[%s] read sgm41542 reg 0x%02x with value 0x%02x !\n",
					__func__, (unsigned int) chip->reg_access, reg_value);
		}
	}
	return size;
}

static DEVICE_ATTR(sgm41542_access, 0660, sgm41542_access_show, sgm41542_access_store);

void sgm41542_dump_registers(void)
{
	int ret = 0;
	int addr = 0;
	int val_buf[SGM41542_REG_NUMBER] = {0x0};
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return;

	if (atomic_read(&chip->charger_suspended) == 1)
		return;

	for(addr = SGM41542_FIRST_REG; addr <= SGM41542_LAST_REG; addr++) {
		ret = sgm41542_read_reg(chip, addr, &val_buf[addr]);
		if (ret)
			chg_err("Couldn't read 0x%02x ret = %d\n", addr, ret);
	}

	chg_err("[0x%02x, 0x%02x, 0x%02x, 0x%02x], [0x%02x, 0x%02x, 0x%02x, 0x%02x], "
			"[0x%02x, 0x%02x, 0x%02x, 0x%02x], [0x%02x, 0x%02x, 0x%02x, 0x%02x]\n",
			val_buf[0], val_buf[1], val_buf[2], val_buf[3],
			val_buf[4], val_buf[5], val_buf[6], val_buf[7],
			val_buf[8], val_buf[9], val_buf[10], val_buf[11],
			val_buf[12], val_buf[13], val_buf[14], val_buf[15]);
}

int sgm41542_kick_wdt(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_WDT_TIMER_RESET,
			REG01_SGM41542_WDT_TIMER_RESET_MASK);
	if (rc)
		chg_err("Couldn't sgm41542 kick wdt rc = %d\n", rc);

	return rc;
}

int sgm41542_set_wdt_timer(int reg)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG05_SGM41542_ADDRESS,
			reg,
			REG05_SGM41542_WATCHDOG_TIMER_MASK);
	if (rc)
		chg_err("Couldn't set recharging threshold rc = %d\n", rc);

	return 0;
}

static void sgm41542_wdt_enable(bool wdt_enable)
{
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return;

	if (atomic_read(&chip->charger_suspended) == 1)
		return;

	if (wdt_enable)
		sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_40S);
	else
		sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_DISABLE);

	chg_err("sgm41542_wdt_enable[%d]\n", wdt_enable);
}

int sgm41542_set_stat_dis(bool enable)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG00_SGM41542_ADDRESS,
			enable ? REG00_SGM41542_STAT_DIS_ENABLE : REG00_SGM41542_STAT_DIS_DISABLE,
			REG00_SGM41542_STAT_DIS_MASK);
	if (rc)
		chg_err("Couldn't sgm41542 set_stat_dis rc = %d\n", rc);

	return rc;
}

int sgm41542_set_int_mask(int val)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG0A_SGM41542_ADDRESS,
			val,
			REG0A_SGM41542_VINDPM_INT_MASK | REG0A_SGM41542_IINDPM_INT_MASK);
	if (rc)
		chg_err("Couldn't sgm41542 set_int_mask rc = %d\n", rc);

	return rc;
}

int sgm41542_set_chg_timer(bool enable)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG05_SGM41542_ADDRESS,
			enable ? REG05_SGM41542_CHG_SAFETY_TIMER_ENABLE : REG05_SGM41542_CHG_SAFETY_TIMER_DISABLE,
			REG05_SGM41542_CHG_SAFETY_TIMER_MASK);
	if (rc)
		chg_err("Couldn't sgm41542 set_chg_timer rc = %d\n", rc);

	return rc;
}

bool sgm41542_get_bus_gd(void)
{
	int rc = 0;
	int reg_val = 0;
	bool bus_gd = false;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG0A_SGM41542_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read regeister, rc = %d\n", rc);
		return false;
	}

	bus_gd = ((reg_val & REG0A_SGM41542_BUS_GD_MASK) == REG0A_SGM41542_BUS_GD_YES) ? 1 : 0;
	return bus_gd;
}

bool sgm41542_get_power_gd(void)
{
	int rc = 0;
	int reg_val = 0;
	bool power_gd = false;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG08_SGM41542_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't get_power_gd rc = %d\n", rc);
		return false;
	}

	power_gd = ((reg_val & REG08_SGM41542_POWER_GOOD_STAT_MASK) == REG08_SGM41542_POWER_GOOD_STAT_GOOD) ? 1 : 0;
	return power_gd;
}

static bool sgm41542_chg_is_usb_present(void)
{
	if (oplus_get_otg_online_status()) {
		chg_err("otg,return false");
		return false;
	}

	if (oplus_vooc_get_fastchg_started() == true) {
		chg_err("[%s]:svooc/vooc already started!\n", __func__);
		return true;
	} else {
		return sgm41542_get_bus_gd();
	}
}

#ifdef CONFIG_OPLUS_CHARGER_MTK
static void sgm41542_mt_power_off(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!g_oplus_chip) {
		printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip not ready!\n", __func__);
		return;
	}

	if (g_oplus_chip->ac_online != true) {
		if(!sgm41542_chg_is_usb_present())
			kernel_power_off();
	} else {
		printk(KERN_ERR "[OPLUS_CHG][%s]: ac_online is true, return!\n", __func__);
	}
}
#endif

int sgm41542_charging_current_write_fast(int chg_cur)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	chg_err("set charge current = %d\n", chg_cur);

	tmp = chg_cur - REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_STEP;

	rc = sgm41542_config_interface(chip, REG02_SGM41542_ADDRESS,
			tmp << REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_SHIFT,
			REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_MASK);

	return rc;
}

int sgm41542_float_voltage_write(int vfloat_mv)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	chg_err("vfloat_mv = %d\n", vfloat_mv);

	if (chip->is_bq25601d)
		tmp = vfloat_mv - REG04_BQ25601D_CHG_VOL_LIMIT_OFFSET;
	else
		tmp = vfloat_mv - REG04_SGM41542_CHG_VOL_LIMIT_OFFSET;

	tmp = tmp / REG04_SGM41542_CHG_VOL_LIMIT_STEP;

	rc = sgm41542_config_interface(chip, REG04_SGM41542_ADDRESS,
			tmp << REG04_SGM41542_CHG_VOL_LIMIT_SHIFT,
			REG04_SGM41542_CHG_VOL_LIMIT_MASK);

	return rc;
}

int sgm41542_set_termchg_current(int term_curr)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	chg_err("term_current = %d\n", term_curr);
	tmp = term_curr - REG03_SGM41542_TERM_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG03_SGM41542_TERM_CHG_CURRENT_LIMIT_STEP;

	rc = sgm41542_config_interface(chip, REG03_SGM41542_ADDRESS,
			tmp << REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_SHIFT,
			REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_MASK);
	return 0;
}

int sgm41542_otg_ilim_set(int ilim)
{
	int rc;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG02_SGM41542_ADDRESS,
			ilim,
			REG02_SGM41542_OTG_CURRENT_LIMIT_MASK);
	if (rc < 0)
		chg_err("Couldn't sgm41542_otg_ilim_set  rc = %d\n", rc);

	return rc;
}

int sgm41542_otg_enable(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_DISABLE);

	rc = sgm41542_otg_ilim_set(REG02_SGM41542_OTG_CURRENT_LIMIT_1200MA);
	if (rc < 0)
		chg_err("Couldn't sgm41542_otg_ilim_set rc = %d\n", rc);

	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_OTG_ENABLE,
			REG01_SGM41542_OTG_MASK);
	if (rc < 0)
		chg_err("Couldn't sgm41542_otg_enable  rc = %d\n", rc);

	return rc;
}

int sgm41542_otg_disable(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_OTG_DISABLE,
			REG01_SGM41542_OTG_MASK);
	if (rc < 0)
		chg_err("Couldn't sgm41542_otg_disable rc = %d\n", rc);

	sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_DISABLE);

	return rc;
}

int oplus_sgm41542_otg_enable(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	sgm41542_otg_enable();
	if(g_oplus_chip && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);
	return 0;
}

int oplus_sgm41542_otg_disable(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	sgm41542_otg_disable();
	if(g_oplus_chip && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);
	return 0;
}

static int sgm41542_enable_gpio(struct chip_sgm41542 *chip, bool enable)
{
	if (enable == true) {
		if (chip->enable_gpio > 0)
			gpio_direction_output(chip->enable_gpio, 0);
	} else {
		if (chip->enable_gpio > 0)
			gpio_direction_output(chip->enable_gpio, 1);
	}
	return 0;
}
int sgm41542_enable_charging(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	sgm41542_enable_gpio(chip, true);
	sgm41542_otg_disable();
	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_CHARGING_ENABLE,
			REG01_SGM41542_CHARGING_MASK);
	if (rc < 0)
		chg_err("Couldn't sgm41542_enable_charging rc = %d\n", rc);

	chg_err("sgm41542_enable_charging \n");
	return rc;
}

int sgm41542_disable_charging(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	sgm41542_enable_gpio(chip, false);
	sgm41542_otg_disable();
	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_CHARGING_DISABLE,
			REG01_SGM41542_CHARGING_MASK);
	if (rc < 0)
		chg_err("Couldn't sgm41542_disable_charging rc = %d\n", rc);

	chg_err("sgm41542_disable_charging \n");
	return rc;
}

int sgm41542_check_charging_enable(void)
{
	int rc = 0;
	int reg_val = 0;
	struct chip_sgm41542 *chip = charger_ic;
	bool charging_enable = false;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG01_SGM41542_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read REG01_SGM41542_ADDRESS rc = %d\n", rc);
		return 0;
	}

	charging_enable = ((reg_val & REG01_SGM41542_CHARGING_MASK) == REG01_SGM41542_CHARGING_ENABLE) ? 1 : 0;

	return charging_enable;
}

int sgm41542_suspend_charger(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	atomic_set(&chip->is_suspended, 1);
	chip->before_suspend_icl = sgm41542_get_usb_icl();
	rc = sgm41542_disable_charging();
	rc = sgm41542_input_current_limit_without_aicl(100);


	chg_err("before_suspend_icl=%d\n", chip->before_suspend_icl);
	return rc;
}

int sgm41542_unsuspend_charger(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	atomic_set(&chip->is_suspended, 0);
	rc = sgm41542_enable_charging();
	chip->before_unsuspend_icl = sgm41542_get_usb_icl();
	if ((chip->before_unsuspend_icl == 0)
			|| (chip->before_suspend_icl == 0)
			|| (chip->before_unsuspend_icl != 100)
			|| (chip->before_unsuspend_icl == chip->before_suspend_icl)) {
		chg_err("ignore set icl [%d %d]\n", chip->before_suspend_icl, chip->before_unsuspend_icl);
	} else {
		if (g_oplus_chip->mmi_chg != 0) {
			rc = sgm41542_input_current_limit_without_aicl(chip->before_suspend_icl);
		} else {
			chg_err("set icl 100 when mmi_chg = %d\n", g_oplus_chip->mmi_chg);
			rc = sgm41542_input_current_limit_without_aicl(100);
		}
	}

	return rc;
}

bool sgm41542_check_suspend_charger(void)
{
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	return atomic_read(&chip->is_suspended);
}

void sgm41542_really_suspend_charger(bool en)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return;

	if (atomic_read(&chip->charger_suspended) == 1)
		return;

	rc = sgm41542_config_interface(chip, REG00_SGM41542_ADDRESS,
			en ? REG00_SGM41542_SUSPEND_MODE_ENABLE : REG00_SGM41542_SUSPEND_MODE_DISABLE,
			REG00_SGM41542_SUSPEND_MODE_MASK);

	if (rc < 0)
		chg_err("fail en=%d rc = %d\n", en, rc);
}

int sgm41542_set_rechg_voltage(int recharge_mv)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG04_SGM41542_ADDRESS,
			recharge_mv,
			REG04_SGM41542_RECHG_THRESHOLD_VOL_MASK);

	if (rc)
		chg_err("Couldn't set recharging threshold rc = %d\n", rc);

	return rc;
}

int sgm41542_reset_charger(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG0B_SGM41542_ADDRESS,
			REG0B_SGM41542_REG_RST_RESET,
			REG0B_SGM41542_REG_RST_MASK);

	if (rc)
		chg_err("Couldn't sgm41542_reset_charger rc = %d\n", rc);

	return rc;
}

int sgm41542_registers_read_full(void)
{
	int rc = 0;
	int reg_full = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG08_SGM41542_ADDRESS, &reg_full);
	if (rc) {
		chg_err("Couldn't read REG08_SGM41542_ADDRESS rc = %d\n", rc);
		return 0;
	}

	reg_full = ((reg_full & REG08_SGM41542_CHG_STAT_MASK) == REG08_SGM41542_CHG_STAT_CHG_TERMINATION) ? 1 : 0;
	if (reg_full) {
		chg_err("the sgm41542 is full");
		sgm41542_dump_registers();
	}

	return rc;
}

int sgm41542_set_chging_term_disable(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG05_SGM41542_ADDRESS,
			REG05_SGM41542_TERMINATION_DISABLE,
			REG05_SGM41542_TERMINATION_MASK);
	if (rc)
		chg_err("Couldn't set chging term disable rc = %d\n", rc);

	return rc;
}

bool sgm41542_check_charger_resume(void)
{
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return false;

	if(atomic_read(&chip->charger_suspended) == 1)
		return false;

	return true;
}

bool sgm41542_need_to_check_ibatt(void)
{
	return false;
}

int sgm41542_get_chg_current_step(void)
{
	return REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_STEP;
}

int sgm41542_set_prechg_voltage_threshold(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG01_SGM41542_ADDRESS,
			REG01_SGM41542_SYS_VOL_LIMIT_3400MV,
			REG01_SGM41542_SYS_VOL_LIMIT_MASK);

	return rc;
}

int sgm41542_set_prechg_current(int ipre_mA)
{
	int tmp = 0;
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	tmp = ipre_mA - REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_STEP;
	rc = sgm41542_config_interface(chip, REG03_SGM41542_ADDRESS,
			(tmp + 1) << REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_SHIFT,
			REG03_SGM41542_PRE_CHG_CURRENT_LIMIT_MASK);

	return 0;
}

int sgm41542_set_otg_voltage(void)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG06_SGM41542_ADDRESS,
			REG06_SGM41542_OTG_VLIM_5000MV,
			REG06_SGM41542_OTG_VLIM_MASK);

	return rc;
}

int sgm41542_set_ovp(int val)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG06_SGM41542_ADDRESS,
			val,
			REG06_SGM41542_OVP_MASK);

	return rc;
}

int sgm41542_get_vbus_stat(void)
{
	int rc = 0;
	int vbus_stat = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG08_SGM41542_ADDRESS, &vbus_stat);
	if (rc) {
		chg_err("Couldn't read REG08_SGM41542_ADDRESS rc = %d\n", rc);
		return 0;
	}

	vbus_stat = vbus_stat & REG08_SGM41542_VBUS_STAT_MASK;

	return vbus_stat;
}

int sgm41542_set_iindet(void)
{
	int rc;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_config_interface(chip, REG07_SGM41542_ADDRESS,
			REG07_SGM41542_IINDET_EN_MASK,
			REG07_SGM41542_IINDET_EN_FORCE_DET);
	if (rc < 0)
		chg_err("Couldn't set REG07_SGM41542_IINDET_EN_MASK rc = %d\n", rc);

	return rc;
}

int sgm41542_get_iindet(void)
{
	int rc = 0;
	int reg_val = 0;
	bool is_complete = false;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG07_SGM41542_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read REG07_SGM41542_ADDRESS rc = %d\n", rc);
		return false;
	}

	is_complete = ((reg_val & REG07_SGM41542_IINDET_EN_MASK) == REG07_SGM41542_IINDET_EN_DET_COMPLETE) ? 1 : 0;
	return is_complete;
}

#ifdef CONFIG_OPLUS_RTC_DET_SUPPORT
static int rtc_reset_check(void)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc = 0;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
				__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return 0;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Error reading rtc device (%s) : %d\n",
				CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n",
				CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	if ((tm.tm_year == 110) && (tm.tm_mon == 0) && (tm.tm_mday <= 1)) {
		chg_debug(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  @@@ wday: %d, yday: %d, isdst: %d\n",
				tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
				tm.tm_wday, tm.tm_yday, tm.tm_isdst);
		rtc_class_close(rtc);
		return 1;
	}

	chg_debug(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  ###  wday: %d, yday: %d, isdst: %d\n",
			tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
			tm.tm_wday, tm.tm_yday, tm.tm_isdst);

close_time:
	rtc_class_close(rtc);
	return 0;
}
#endif /* CONFIG_OPLUS_RTC_DET_SUPPORT */

void sgm41542_vooc_timeout_callback(bool vbus_rising)
{
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return;

	chip->power_good = vbus_rising;
	if (!vbus_rising) {
		sgm41542_request_dpdm(chip, false);
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sgm41542_inform_charger_type(chip);
		oplus_chg_wakelock(chip, false);
	}
	sgm41542_dump_registers();
}

static int sgm41542_batfet_reset_disable(struct chip_sgm41542 *chip, bool enable)
{
	int rc = 0;
	int val = 0;

	if(enable)
		val = SGM41542_BATFET_RST_DISABLE << REG07_SGM41542_BATFET_RST_EN_SHIFT;
	else
		val = SGM41542_BATFET_RST_ENABLE << REG07_SGM41542_BATFET_RST_EN_SHIFT;

	rc = sgm41542_config_interface(chip, REG07_SGM41542_ADDRESS, val, REG07_SGM41542_BATFET_RST_EN_MASK);

	return rc;
}

int sgm41542_set_shipmode(bool enable)
{
	int rc = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return rc;

	if (enable) {
		rc = sgm41542_config_interface(chip, REG07_SGM41542_ADDRESS,
			REG07_SGM41542_BATFET_DIS_ON,
			REG07_SGM41542_BATFET_DIS_MASK);
		if (rc < 0)
			chg_err("Couldn't set REG07_SGM41542_BATFET_DIS_ON rc = %d\n", rc);
	} else {
		rc = sgm41542_config_interface(chip, REG07_SGM41542_ADDRESS,
			REG07_SGM41542_BATFET_DIS_OFF,
			REG07_SGM41542_BATFET_DIS_MASK);
		if (rc < 0)
			chg_err("Couldn't set REG07_SGM41542_BATFET_DIS_OFF rc = %d\n", rc);
	}

	return rc;
}


bool sgm41542_get_deivce_online(void)
{
	int rc = 0;
	int reg_val = 0;
	int part_id = 0x00;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip)
		return 0;

	if(atomic_read(&chip->charger_suspended) == 1)
		return 0;

	rc = sgm41542_read_reg(chip, REG0B_SGM41542_ADDRESS, &reg_val);
	if (rc) {
		rc = sgm41542_read_reg(chip, REG0B_SGM41542_ADDRESS, &reg_val);
		if (rc) {
			chg_err("Couldn't read REG0B_SGM41542_ADDRESS rc = %d\n", rc);
			return false;
		}
	}

	part_id = (reg_val & REG0B_SGM41542_PN_MASK);
	chg_err("sgm41542 part_id=0x%02X\n", part_id);

	if (part_id == 0x0c || part_id == 0x0d) /*part id 1100=SGM41541; 1101=SGM41542*/
		return true;

	return false;
}

#define HW_AICL_POINT_OFFSET 4440
#define SW_AICL_POINT_OFFSET 4500
int sgm41542_hardware_init(void)
{
	struct chip_sgm41542 *chip = charger_ic;

	chg_err("init sgm41542 hardware! \n");

	if (!chip)
		return false;

	/*must be before set_vindpm_vol and set_input_current*/
	chip->hw_aicl_point = HW_AICL_POINT_OFFSET;
	chip->sw_aicl_point = SW_AICL_POINT_OFFSET;


	sgm41542_set_stat_dis(false);
	sgm41542_set_int_mask(REG0A_SGM41542_VINDPM_INT_NOT_ALLOW | REG0A_SGM41542_IINDPM_INT_NOT_ALLOW);

	sgm41542_set_chg_timer(false);

	sgm41542_disable_charging();

	sgm41542_set_ovp(REG06_SGM41542_OVP_14P0V);

	sgm41542_set_chging_term_disable();

	sgm41542_float_voltage_write(WPC_TERMINATION_VOLTAGE);

	sgm41542_otg_ilim_set(REG02_SGM41542_OTG_CURRENT_LIMIT_1200MA);

	sgm41542_set_prechg_voltage_threshold();

	sgm41542_set_prechg_current(WPC_PRECHARGE_CURRENT);

	sgm41542_charging_current_write_fast(REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_2000MA);

	sgm41542_set_termchg_current(WPC_TERMINATION_CURRENT);

	sgm41542_input_current_limit_without_aicl(REG00_SGM41542_INIT_INPUT_CURRENT_LIMIT_500MA);

	sgm41542_set_rechg_voltage(WPC_RECHARGE_VOLTAGE_OFFSET);

	sgm41542_set_vindpm_vol(chip->hw_aicl_point);

	sgm41542_set_otg_voltage();

	sgm41542_batfet_reset_disable(chip, chip->batfet_reset_disable);

	sgm41542_unsuspend_charger();

	sgm41542_enable_charging();

	sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_40S);

	return true;
}

static int sgm41542_get_charger_type(void)
{
	struct chip_sgm41542 *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip || !g_oplus_chip)
		return POWER_SUPPLY_TYPE_UNKNOWN;


	if (chip->oplus_charger_type != g_oplus_chip->charger_type && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);

	return chip->oplus_charger_type;
}

static int sgm41542_parse_dt(struct chip_sgm41542 *chip)
{
	int ret = 0;

	if (of_property_read_string(chip->client->dev.of_node, "charger_name", &chip->chg_dev_name) < 0) {
		chip->chg_dev_name = "primary_chg";
		chg_err("no charger name\n");
	}

	chip->irq_gpio = of_get_named_gpio(chip->client->dev.of_node, "sgm41542-irq-gpio", 0);
	if (!gpio_is_valid(chip->irq_gpio)) {
		chg_err("gpio_is_valid fail irq-gpio[%d]\n", chip->irq_gpio);
		return -EINVAL;
	}

	/*slaver charger not need bc1.2 and irq service.*/
	ret = devm_gpio_request(chip->dev, chip->irq_gpio, "sgm41542-irq-gpio");
	if (ret) {
		chg_err("unable to request irq-gpio[%d]\n", chip->irq_gpio);
		return -EINVAL;
	}

	chg_err("irq-gpio[%d]\n", chip->irq_gpio);

	/*Get the charger enable gpio.*/
	chip->enable_gpio = of_get_named_gpio(chip->client->dev.of_node, "qcom,charger_enable-gpio", 0);
	if (!gpio_is_valid(chip->enable_gpio)) {
                chg_err("gpio_is_valid fail enable-gpio[%d]\n", chip->enable_gpio);
        } else {
		chg_err("enable-gpio[%d]\n", chip->enable_gpio);
		ret = gpio_request(chip->enable_gpio,
			                  "sgm41542-enable-gpio");
			if (ret) {
				chg_err("unable to request gpio [%d]\n",
				         chip->enable_gpio);
			}
	}
	chip->use_voocphy = of_property_read_bool(chip->client->dev.of_node, "qcom,use_voocphy");
	chg_err("use_voocphy=%d\n", chip->use_voocphy);
	return ret;
}

void sgm41542_force_dpdm(void)
{
	int val;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return;

	val = 0;
	sgm41542_config_interface(chip, REG0D_SGM41542_ADDRESS,
				  val, REG0D_SGM41542_DPDM_VSEL_MASK); /*dp/dm Hiz*/
}

int sgm41542_enable_qc20_hvdcp_9v(void)
{
	int ret = 0;
	int val, dp_val;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return ret;

	/*dp and dm connected,dp 0.6V dm 0.6V*/
	dp_val = REG0D_SGM41542_DP_600MV << REG0D_SGM41542_DP_VSEL_SHIFT;
	ret = sgm41542_config_interface(chip, REG0D_SGM41542_ADDRESS,
				  dp_val, REG0D_SGM41542_DP_VSEL_MASK); /*dp 0.6V*/
	if (ret)
		return ret;

	msleep(1500);

	val = REG0D_SGM41542_DP_3300MV_DM_600MV << REG0D_SGM41542_DPDM_VSEL_SHIFT;
	ret = sgm41542_config_interface(chip, REG0D_SGM41542_ADDRESS,
				  val, REG0D_SGM41542_DPDM_VSEL_MASK); /*dp 3.3v dm 0.6v*/

	return ret;
}

static int sgm41542_disable_hvdcp(void)
{
	int ret = 0, val;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return 0;

	chg_debug("disable hvdcp\n");

	/*dp and dm connected,dp 0.6V dm Hiz*/
	val = REG0D_SGM41542_DP_600MV_DM_HIZ << REG0D_SGM41542_DPDM_VSEL_SHIFT;
	ret = sgm41542_config_interface(chip, REG0D_SGM41542_ADDRESS,
			val, REG0D_SGM41542_DPDM_VSEL_MASK);

	return ret;
}

static bool sgm41542_is_hvdcp(void)
{
	int ret = 0;
	bool hvdcp = false;
	int vol_mv = 0;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return ret;

	vol_mv = sgm41542_get_vbus_voltage();

	if (vol_mv > 7500)
		hvdcp = true;

	chg_err("finn vbus = %dmV, hvdcp = %d\n", vol_mv, hvdcp);

	chip->is_hvdcp = hvdcp;
	return hvdcp;
}

#define SGM4154x_FAST_CHARGER_VOLTAGE		9000
#define SGM4154x_NORMAL_CHARGER_VOLTAGE		5000
#define SGM41542_QC_RETRY_CNT	3
static int sgm41542_adjust_qc_voltage(u32 value)
{
	int i = 0, cnt = 0, ret = 0, retry_count = 10;
	int val = REG0D_SGM41542_DP_3300MV_DM_600MV << REG0D_SGM41542_DPDM_VSEL_SHIFT;
	struct chip_sgm41542 *chip = charger_ic;

	if (chip == NULL)
		return ret;

	if (value == SGM4154x_NORMAL_CHARGER_VOLTAGE) {
		ret = sgm41542_disable_hvdcp();
		chip->is_hvdcp = false;
		chg_err("adjust 5v success\n");
		return ret;
	} else if (value == SGM4154x_FAST_CHARGER_VOLTAGE) {
		for (i = 0; i < retry_count; i++) {
			if (sgm41542_is_hvdcp())
				break;
			msleep(100);
		}

		ret = sgm41542_config_interface(chip, REG0D_SGM41542_ADDRESS,
			  val, REG0D_SGM41542_DPDM_VSEL_MASK); /*dp 3.3v dm 0.6v*/

		if (!sgm41542_is_hvdcp()) {
			do {
				sgm41542_force_dpdm();
				msleep(100);
				ret = sgm41542_enable_qc20_hvdcp_9v();
				msleep(200);
			} while (!sgm41542_is_hvdcp() && cnt++ < SGM41542_QC_RETRY_CNT && chip->bc12_done);
		}

		if (chip->is_hvdcp)
			chg_err("adjust 9v success\n");
		else
			chg_err("adjust 9v failed\n");

		return ret;
	}

	return ret;
}

static void btb_temp_detect_over(void)
{
	int temp;

	temp = oplus_chg_get_battery_btb_temp_cal();

	if(!btb_detect_over && (temp >= 80))
		btb_detect_over = true;

	chg_err("[sgm41542_set_qc_config]btb:temp:%d, over:%d\n", temp, btb_detect_over);
}

#define SOC_90	90
#define TEMP_42	420
#define TEMP_0	0
#define BAT_VOLT_4V45 4450
#define MT6377_VBUS_VOL_7V5	7500
#define MT6377_VBUS_VOL_6V5	6500
static int sgm41542_set_qc_config(void)
{
	int ret = -1;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip || !g_oplus_chip) {
		chg_debug("oplus_chip is null\n");
		return ret;
	}

	btb_temp_detect_over();

	if (!g_oplus_chip->calling_on && !g_oplus_chip->camera_on && g_oplus_chip->charger_volt < MT6377_VBUS_VOL_6V5 && g_oplus_chip->soc < SOC_90
			&& g_oplus_chip->temperature <= TEMP_42 && g_oplus_chip->cool_down_force_5v == false && !btb_detect_over) {
		printk(KERN_ERR "%s: set qc to 9V", __func__);
		ret = sgm41542_adjust_qc_voltage(9000);
	} else {
		if (g_oplus_chip->charger_volt > MT6377_VBUS_VOL_7V5 &&
				(g_oplus_chip->calling_on || g_oplus_chip->camera_on || g_oplus_chip->soc >= SOC_90 || g_oplus_chip->batt_volt >= BAT_VOLT_4V45
				|| g_oplus_chip->temperature > TEMP_42 || g_oplus_chip->temperature < TEMP_0 || g_oplus_chip->cool_down_force_5v == true || btb_detect_over)) {
			printk(KERN_ERR "%s: set qc to 5V", __func__);
			ret = sgm41542_adjust_qc_voltage(5000);
		}
	}

	if((chip->pre_is_hvdcp != chip->is_hvdcp) && chip->is_hvdcp)
		ret = 0;
	chip->pre_is_hvdcp = chip->is_hvdcp;

	return ret;
}

static void sgm41542_qc_detect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct chip_sgm41542 *chip = container_of(dwork, struct chip_sgm41542, qc_detect_work);
	int count = 0, ret = 0;

	chg_err("fchg qc start detect, oplus_charger_type=%d\n", chip->oplus_charger_type);

	if (chip->oplus_charger_type != POWER_SUPPLY_TYPE_USB_DCP)
		return;

	if (!sgm41542_is_hvdcp()) {
		do {
			sgm41542_force_dpdm();
			msleep(100);
			ret = sgm41542_enable_qc20_hvdcp_9v();
			msleep(200);
		} while (!sgm41542_is_hvdcp() && count++ < SGM41542_QC_RETRY_CNT && chip->bc12_done);
	}

	chg_err("count=%d\n", count);

	if (chip->is_hvdcp) {
		chip->support_hvdcp = true;
		chg_err("detect 9v success\n");
	} else {
		chip->support_hvdcp = false;
		chg_err("detect 9v failed\n");
	}
}

static void sgm41542_start_qc_detect(struct chip_sgm41542 *chip)
{
	if (!chip)
		return;

	if (chip->is_sgm41542)
		schedule_delayed_work(&chip->qc_detect_work, round_jiffies_relative(msecs_to_jiffies(100)));
}

int sgm41542_get_charger_subtype(void)
{
	int charg_subtype = CHARGER_SUBTYPE_DEFAULT;
	struct chip_sgm41542 *chip = charger_ic;

	if (!chip) {
		return charg_subtype;
	}

	if (chip->support_hvdcp)
		return CHARGER_SUBTYPE_QC;

	return charg_subtype;
}
static int sgm41542_request_dpdm(struct chip_sgm41542 *chip, bool enable)
{
	if (!chip)
		return 0;

	if(enable)
		Charger_Detect_Init();
	else
		Charger_Detect_Release();

	chg_err("sgm41542_request_dpdm enable = %d\n", enable);

	return 0;
}

#define OPLUS_BC12_DELAY_CNT 	18
static void sgm41542_bc12_retry_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct chip_sgm41542 *chip = container_of(dwork, struct chip_sgm41542, bc12_retry_work);

	if (chip->is_sgm41542) {
		if (!sgm41542_chg_is_usb_present()) {
			chg_err("plugout during BC1.2, delay_cnt=%d,return\n", chip->bc12_delay_cnt);
			chip->bc12_delay_cnt = 0;
			return;
		}

		if (chip->bc12_delay_cnt >= OPLUS_BC12_DELAY_CNT) {
			chg_err("BC1.2 not complete delay_cnt to max\n");
			return;
		}
		chip->bc12_delay_cnt++;

		if (sgm41542_get_iindet()) {
			chg_err("BC1.2 complete, delay_cnt=%d\n", chip->bc12_delay_cnt);
			sgm41542_get_bc12(chip);
			sgm41542_request_dpdm(chip, false);
		} else {
			chg_err("BC1.2 not complete delay 50ms,delay_cnt=%d\n", chip->bc12_delay_cnt);
			schedule_delayed_work(&chip->bc12_retry_work, round_jiffies_relative(msecs_to_jiffies(50)));
		}
	}
}

static void sgm41542_start_bc12_retry(struct chip_sgm41542 *chip)
{
	if (!chip)
		return;

	sgm41542_request_dpdm(chip, true);
	sgm41542_set_iindet();

	if (chip->is_sgm41542)
		schedule_delayed_work(&chip->bc12_retry_work, round_jiffies_relative(msecs_to_jiffies(100)));
}

#define OPLUS_BC12_RETRY_CNT 	1
static void sgm41542_get_bc12(struct chip_sgm41542 *chip)
{
	int vbus_stat = 0;

	if (!chip)
		return;

	if (!chip->bc12_done) {
		vbus_stat = sgm41542_get_vbus_stat();
		chg_err("vbus_stat=%x\n", vbus_stat);
		switch (vbus_stat) {
		case REG08_SGM41542_VBUS_STAT_SDP:
				if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
					chip->bc12_retried++;
					chg_err("bc1.2 sdp retry cnt=%d\n", chip->bc12_retried);
					sgm41542_start_bc12_retry(chip);
					break;
				} else {
					oplus_notify_device_mode(true);
				}
				chip->bc12_done = true;
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB;
				sgm41542_inform_charger_type(chip);
				oplus_chg_wake_update_work();
				break;
		case REG08_SGM41542_VBUS_STAT_CDP:
				if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
					chip->bc12_retried++;
					chg_err("bc1.2 cdp retry cnt=%d\n", chip->bc12_retried);
					sgm41542_start_bc12_retry(chip);
					break;
				}
				chip->bc12_done = true;
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_CDP;
				sgm41542_inform_charger_type(chip);
				oplus_notify_device_mode(true);
				oplus_chg_wake_update_work();
				break;
		case REG08_SGM41542_VBUS_STAT_DCP:
		case REG08_SGM41542_VBUS_STAT_OCP:
		case REG08_SGM41542_VBUS_STAT_FLOAT:
                if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
					chip->bc12_retried++;
					chg_err("bc1.2 dcp retry cnt=%d\n", chip->bc12_retried);
					sgm41542_start_bc12_retry(chip);
					break;
				}
				chip->bc12_done = true;
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
				sgm41542_inform_charger_type(chip);
				oplus_chg_wake_update_work();
				sgm41542_start_qc_detect(chip);
				break;
		case REG08_SGM41542_VBUS_STAT_UNKNOWN:
				if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
					chip->bc12_retried++;
					chg_err("bc1.2 unknown retry cnt=%d\n", chip->bc12_retried);
					sgm41542_start_bc12_retry(chip);
					break;
				}
				break;
		case REG08_SGM41542_VBUS_STAT_OTG_MODE:
		default:
				chg_err("bc1.2 unknown\n");
			break;
		}
	}
}

static void oplus_chg_awake_init(struct chip_sgm41542 *chip)
{
	if (!chip) {
		pr_err("[%s]chip is null\n", __func__);
		return;
	}
	chip->suspend_ws = NULL;
	chip->suspend_ws = wakeup_source_register(NULL, "split chg wakelock");
	return;
}

static void oplus_chg_wakelock(struct chip_sgm41542 *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->suspend_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->suspend_ws);
		pr_err("[%s] true\n", __func__);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->suspend_ws);
		pm_flag = false;
		pr_err("[%s] false\n", __func__);
	}
	return;
}

static void oplus_keep_resume_awake_init(struct chip_sgm41542 *chip)
{
	if (!chip) {
		pr_err("[%s]chip is null\n", __func__);
		return;
	}
	chip->keep_resume_ws = NULL;
	chip->keep_resume_ws = wakeup_source_register(NULL, "split_chg_keep_resume");
	return;
}

static void oplus_keep_resume_wakelock(struct chip_sgm41542 *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->keep_resume_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->keep_resume_ws);
		pr_err("[%s] true\n", __func__);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->keep_resume_ws);
		pm_flag = false;
		pr_err("[%s] false\n", __func__);
	}
	return;
}

#define OPLUS_WAIT_RESUME_TIME	200
static irqreturn_t sgm41542_irq_handler(int irq, void *data)
{
	struct chip_sgm41542 *chip = (struct chip_sgm41542 *) data;
	bool prev_pg = false, curr_pg = false, bus_gd = false;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();
	int reg_val = 0;
	int ret = 0;

	if (!chip)
		return IRQ_HANDLED;

	if (oplus_get_otg_online_status()) {
		chg_err("otg,ignore\n");
		oplus_keep_resume_wakelock(chip, false);
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sgm41542_inform_charger_type(chip);
		return IRQ_HANDLED;
	}
	oplus_chg_wake_update_work();
	chg_err(" sgm41542_irq_handler:enter improve irq time\n");
	oplus_keep_resume_wakelock(chip, true);

	/*for check bus i2c/spi is ready or not*/
	if (atomic_read(&chip->charger_suspended) == 1) {
		chg_err(" sgm41542_irq_handler:suspended and wait_event_interruptible %d\n", OPLUS_WAIT_RESUME_TIME);
		wait_event_interruptible_timeout(chip->wait, atomic_read(&chip->charger_suspended) == 0, msecs_to_jiffies(OPLUS_WAIT_RESUME_TIME));
	}
	prev_pg = chip->power_good;
	ret = sgm41542_read_reg(chip, REG0A_SGM41542_ADDRESS, &reg_val);
	if (ret) {
		chg_err("[%s] SGM41542_REG_0A read failed ret[%d]\n", __func__, ret);
		oplus_keep_resume_wakelock(chip, false);
		return IRQ_HANDLED;
	}
	bus_gd = sgm41542_get_bus_gd();
	curr_pg = bus_gd;
	if(curr_pg) {
		oplus_chg_wakelock(chip, true);
	}
	sgm41542_dump_registers();
	oplus_chg_check_break(bus_gd);
	oplus_chg_track_check_wired_charging_break(curr_pg);
	if (oplus_vooc_get_fastchg_started() == true
			&& oplus_vooc_get_adapter_update_status() != 1) {
		chg_err("oplus_vooc_get_fastchg_started = true!\n", __func__);
		oplus_keep_resume_wakelock(chip, false);
		return IRQ_HANDLED;
	} else {
		chip->power_good = curr_pg;
	}
	chg_err("(%d,%d, %d, %d)\n", prev_pg, chip->power_good, curr_pg, bus_gd);


	if (!prev_pg && chip->power_good) {
		oplus_chg_wakelock(chip, true);
		sgm41542_request_dpdm(chip, true);
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		if (chip->use_voocphy) {
			oplus_voocphy_set_adc_enable(true);
		}
		sgm41542_set_wdt_timer(REG05_SGM41542_WATCHDOG_TIMER_40S);
		oplus_wake_up_usbtemp_thread();
		if (chip->oplus_charger_type == POWER_SUPPLY_TYPE_UNKNOWN) {
			sgm41542_get_bc12(chip);
		}

		if (g_oplus_chip) {
			if (oplus_vooc_get_fastchg_to_normal() == false
					&& oplus_vooc_get_fastchg_to_warm() == false) {
				if (g_oplus_chip->authenticate
						&& g_oplus_chip->mmi_chg
						&& !g_oplus_chip->balancing_bat_stop_chg
						&& oplus_vooc_get_allow_reading()
						&& !oplus_is_rf_ftm_mode()) {
					sgm41542_enable_charging();
				}
			}
		}
		goto POWER_CHANGE;
	} else if (prev_pg && !chip->power_good) {
		chip->pre_is_hvdcp = false;
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		chip->support_hvdcp = false;
		chip->is_hvdcp = false;
		btb_detect_over = false;
		sgm41542_inform_charger_type(chip);
		oplus_vooc_reset_fastchg_after_usbout();
		if (oplus_vooc_get_fastchg_started() == false) {
			oplus_chg_set_chargerid_switch_val(0);
			oplus_chg_clear_chargerid_info();
		}
		oplus_chg_set_charger_type_unknown();
		oplus_chg_wake_update_work();
		oplus_wake_up_usbtemp_thread();
		oplus_notify_device_mode(false);
		if (chip->use_voocphy) {
			oplus_voocphy_set_adc_enable(false);
		}
		oplus_chg_wakelock(chip, false);
		goto POWER_CHANGE;
	} else if (!prev_pg && !chip->power_good) {
		chg_err("prev_pg & now_pg is false\n");
		chip->pre_is_hvdcp = false;
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		chip->support_hvdcp = false;
		chip->is_hvdcp = false;
		goto POWER_CHANGE;
	}
POWER_CHANGE:
	if(dumpreg_by_irq)
		sgm41542_dump_registers();

	oplus_keep_resume_wakelock(chip, false);
	return IRQ_HANDLED;
}

static int sgm41542_enable_gpio_init(struct chip_sgm41542 *chip)
{
	if (NULL == chip)
		return -EINVAL;

	sgm41542_enable_gpio(chip, true);

	return 0;
}

static int sgm41542_irq_register(struct chip_sgm41542 *chip)
{
	int ret = 0;

	ret = devm_request_threaded_irq(chip->dev, gpio_to_irq(chip->irq_gpio), NULL,
			sgm41542_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"sgm41542-eint",
			chip);
	if (ret < 0) {
		chg_err("sgm41542 request_irq fail!");
		return -EFAULT;
	}

	ret = enable_irq_wake(gpio_to_irq(chip->irq_gpio));
	if (ret != 0) {
		chg_err("enable_irq_wake: irq_gpio failed %d\n", ret);
	}

	return ret;
}

static int sgm41542_inform_charger_type(struct chip_sgm41542 *chip)
{
	int ret = 0;
	union power_supply_propval propval;

	if (!chip->psy) {
		chip->psy = power_supply_get_by_name("mtk-master-charger");
		if (IS_ERR_OR_NULL(chip->psy)) {
			chg_err("Couldn't get chip->psy");
			return -EINVAL;
		}
	}

	if (chip->power_good)
		propval.intval = 1;
	else
		propval.intval = 0;

	chg_debug("inform power supply online status:%d", propval.intval);
	ret = power_supply_set_property(chip->psy, POWER_SUPPLY_PROP_ONLINE, &propval);

	if (ret < 0)
		chg_err("inform power supply online failed:%d", ret);

	power_supply_changed(chip->psy);
	return ret;
}

static void sgm41542_init_work_handler(struct work_struct *work)
{
	struct chip_sgm41542 *chip = NULL;

	if (charger_ic) {
		chip = charger_ic;

		sgm41542_irq_handler(0, chip);

		if (sgm41542_chg_is_usb_present())
			sgm41542_irq_handler(0, chip);
	}

	return;
}

static int sgm41542_dump_register(struct charger_device *chg_dev)
{
	sgm41542_dump_registers();

	return 0;
}
int oplus_sgm41542_kick_wdt(struct charger_device *chg_dev)
{
	return sgm41542_kick_wdt();
}

static int sgm41542_charging(struct charger_device *chg_dev, bool enable)
{
	int ret = 0;

	if (enable)
		ret = sgm41542_enable_charging();
	else
		ret = sgm41542_disable_charging();

	chg_debug(" %s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}

static int sgm41542_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	*en = sgm41542_check_charging_enable();

	return 0;
}

static int sgm41542_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	int rc = 0;

	rc = sgm41542_registers_read_full();

	return rc;
}

static int sgm41542_plug_in(struct charger_device *chg_dev)
{
	int ret;

	ret = sgm41542_charging(chg_dev, true);

	if (ret)
		chg_err("Failed to enable charging:%d", ret);

	return ret;
}

static int sgm41542_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = sgm41542_charging(chg_dev, false);

	if (ret)
		chg_err("Failed to disable charging:%d", ret);

	return ret;
}

__attribute__((unused)) static int sgm41542_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	struct chip_sgm41542 *chip = charger_ic;
	int ret;
	int reg_val;

	ret = sgm41542_read_reg(chip, REG05_SGM41542_ADDRESS, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG05_SGM41542_CHG_SAFETY_TIMER_MASK);

	return ret;
}

__attribute__((unused)) static int sgm41542_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	int ret;

	if (en)
		ret = sgm41542_set_chg_timer(true);
	else
		ret = sgm41542_set_chg_timer(false);

	return ret;
}

static int sgm41542_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	chg_debug("charge curr = %d", curr);

	return sgm41542_charging_current_write_fast(curr / 1000);
}

static int sgm41542_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	int min_ichg_ma = 60, unit = 1000;

	*curr = min_ichg_ma * unit;

	return 0;
}

__attribute__((unused)) static int sgm41542_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct chip_sgm41542 *chip = charger_ic;
	int reg_val;
	int ichg;
	int ret;

	ret = sgm41542_read_reg(chip, REG02_SGM41542_ADDRESS, &reg_val);
	if (!ret) {
		ichg = (reg_val & REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_MASK) >> REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_SHIFT;
		ichg = ichg * REG02_SGM41542_FAST_CHG_CURRENT_LIMIT_STEP;
		*curr = ichg * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sgm41542_set_chargevolt(u32 volt)
{
	struct chip_sgm41542 *chip = charger_ic;
	u8 val;

	if(!chip)
	return -1;

	if (volt < REG04_SGM41542_CHG_VOL_LIMIT_OFFSET)
		volt = REG04_SGM41542_CHG_VOL_LIMIT_OFFSET;

	val = (volt - REG04_SGM41542_CHG_VOL_LIMIT_OFFSET)/REG04_SGM41542_CHG_VOL_LIMIT_STEP;

	return sgm41542_config_interface(chip, REG04_SGM41542_ADDRESS,
				val << REG04_SGM41542_CHG_VOL_LIMIT_SHIFT,
				REG04_SGM41542_CHG_VOL_LIMIT_MASK);
}

__attribute__((unused)) static int sgm41542_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct chip_sgm41542 *chip = charger_ic;
	int reg_val;
	int vchg;
	int ret;

	ret = sgm41542_read_reg(chip, REG04_SGM41542_ADDRESS, &reg_val);
	if (!ret) {
		vchg = (reg_val & REG04_SGM41542_CHG_VOL_LIMIT_MASK) >> REG04_SGM41542_CHG_VOL_LIMIT_SHIFT;
		vchg = vchg * REG04_SGM41542_CHG_VOL_LIMIT_STEP + REG04_SGM41542_CHG_VOL_LIMIT_OFFSET;
		*volt = vchg * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sgm41542_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct chip_sgm41542 *chip = charger_ic;
	int reg_val;
	int icl;
	int ret;

	ret = sgm41542_read_reg(chip, REG00_SGM41542_ADDRESS, &reg_val);
	if (!ret) {
		icl = (reg_val & REG00_SGM41542_INPUT_CURRENT_LIMIT_MASK) >> REG00_SGM41542_INPUT_CURRENT_LIMIT_SHIFT;
		icl = icl * REG00_SGM41542_INPUT_CURRENT_LIMIT_STEP + REG00_SGM41542_INPUT_CURRENT_LIMIT_OFFSET;
		*curr = icl * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sgm41542_set_ivl(struct charger_device *chg_dev, u32 volt)
{
	return sgm41542_set_vindpm_vol(volt/1000);
}

static int sgm41542_set_vchg(struct charger_device *chg_dev, u32 volt)
{
	return sgm41542_set_chargevolt(volt/1000);
}

static int sgm41542_set_icl(struct charger_device *chg_dev, u32 curr)
{
	return sgm41542_input_current_limit_without_aicl(curr/1000);
}

static int sgm41542_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (en)
		ret = sgm41542_otg_enable();
	else
		ret = sgm41542_otg_disable();

	power_supply_changed(g_oplus_chip->usb_psy);
	return ret;
}

static int sgm41542_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	int ret = 0;
	u8 val = REG04_SGM41542_CHG_VOL_LIMIT_MASK;

	if ((curr/1000) >= REG02_SGM41542_BOOSTI_1200)
		val = REG02_SGM41542_OTG_CURRENT_LIMIT_1200MA;

	ret = sgm41542_otg_ilim_set(val);

	return ret;
}

static int sgm41542_chgdet_en(bool en)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if(g_oplus_chip == NULL || g_oplus_chip->chg_ops == NULL
		|| g_oplus_chip->chg_ops->get_charging_enable == NULL) {
		return -1;
	}

	if (en) {
		if (oplus_vooc_get_fastchg_to_normal() == false
			&& oplus_vooc_get_fastchg_to_warm() == false) {
			if (g_oplus_chip->authenticate
				&& g_oplus_chip->mmi_chg
				&& !g_oplus_chip->balancing_bat_stop_chg
				&& (g_oplus_chip->charging_state != CHARGING_STATUS_FAIL)
				&& oplus_vooc_get_allow_reading()
				&& !oplus_is_rf_ftm_mode()) {
				sgm41542_enable_charging();
			} else {
				chg_err("should not turn on charging here! \n");
			}
		}
	} else {
		sgm41542_disable_charging();
		Charger_Detect_Release();
		charger_ic->bc12_done = false;
		charger_ic->bc12_retried = 0;
		charger_ic->bc12_delay_cnt = 0;
		charger_ic->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sgm41542_inform_charger_type(charger_ic);
	}

	return 0;
}

static int oplus_sgm41542_pd_setup(void)
{
	return false;
}

static int sgm41542_enable_chgdet(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_notice("sgm41542_enable_chgdet:%d\n", en);
	ret = sgm41542_chgdet_en(en);

	return ret;
}

static struct charger_ops sgm41542_chg_ops = {
	/* Normal charging */
	.plug_in = sgm41542_plug_in,
	.plug_out = sgm41542_plug_out,
	.dump_registers = sgm41542_dump_register,
	.enable = sgm41542_charging,
	.is_enabled = sgm41542_is_charging_enable,
	.get_charging_current = sgm41542_get_ichg,
	.set_charging_current = sgm41542_set_ichg,
	.get_input_current = sgm41542_get_icl,
	.set_input_current = sgm41542_set_icl,
	.get_constant_voltage = sgm41542_get_vchg,
	.set_constant_voltage = sgm41542_set_vchg,
	.kick_wdt = oplus_sgm41542_kick_wdt,
	.set_mivr = sgm41542_set_ivl,
	.is_charging_done = sgm41542_is_charging_done,
	.get_min_charging_current = sgm41542_get_min_ichg,

	/* Safety timer */
	.enable_safety_timer = sgm41542_set_safety_timer,
	.is_safety_timer_enabled = sgm41542_is_safety_timer_enabled,

	/* Power path */
	.enable_powerpath = NULL,
	.is_powerpath_enabled = NULL,

	.enable_chg_type_det = sgm41542_enable_chgdet,
	/* OTG */
	.enable_otg = sgm41542_set_otg,
	.set_boost_current_limit = sgm41542_set_boost_ilmt,
	.enable_discharge = NULL,

	/* ADC */
	.get_tchg_adc = NULL,
};

struct oplus_chg_operations  oplus_chg_sgm41542_ops = {
	.dump_registers = sgm41542_dump_registers,
	.kick_wdt = sgm41542_kick_wdt,
	.hardware_init = sgm41542_hardware_init,
	.charging_current_write_fast = sgm41542_charging_current_write_fast,
	.set_aicl_point = sgm41542_set_aicl_point,
	.input_current_write = sgm41542_input_current_limit_write,
	.input_current_ctrl_by_vooc_write = sgm41542_input_current_limit_ctrl_by_vooc_write,
	.float_voltage_write = sgm41542_float_voltage_write,
	.term_current_set = sgm41542_set_termchg_current,
	.charging_enable = sgm41542_enable_charging,
	.charging_disable = sgm41542_disable_charging,
	.get_charging_enable = sgm41542_check_charging_enable,
	.charger_suspend = sgm41542_suspend_charger,
	.charger_unsuspend = sgm41542_unsuspend_charger,
	.charger_suspend_check = sgm41542_check_suspend_charger,
	.set_rechg_vol = sgm41542_set_rechg_voltage,
	.reset_charger = sgm41542_reset_charger,
	.read_full = sgm41542_registers_read_full,
	.otg_enable = oplus_sgm41542_otg_enable,
	.otg_disable = oplus_sgm41542_otg_disable,
	.set_charging_term_disable = sgm41542_set_chging_term_disable,
	.check_charger_resume = sgm41542_check_charger_resume,
	.get_chargerid_volt = oplus_sgm41542_get_chargerid_volt,
	.set_chargerid_switch_val = oplus_sgm41542_set_chargerid_switch_val,
	.get_chargerid_switch_val = oplus_sgm41542_get_chargerid_switch_val,
	.need_to_check_ibatt = sgm41542_need_to_check_ibatt,
	.get_chg_current_step = sgm41542_get_chg_current_step,
	.get_charger_type = sgm41542_get_charger_type,
	.get_real_charger_type = NULL,
	.get_charger_volt = sgm41542_get_vbus_voltage,
	.get_charger_current = sgm41542_get_ibus_current,
	.check_chrdet_status = sgm41542_chg_is_usb_present,
#ifdef CONFIG_OPLUS_CHARGER_MTK
	.set_power_off = sgm41542_mt_power_off,
#endif
	.get_instant_vbatt = oplus_battery_meter_get_battery_voltage,
	.get_boot_mode = get_boot_mode,
	.get_boot_reason = oplus_get_boot_reason,
	.get_rtc_soc = get_rtc_spare_oplus_fg_value,
	.set_rtc_soc = set_rtc_spare_oplus_fg_value,
#ifdef CONFIG_OPLUS_SHORT_C_BATT_CHECK
	.get_dyna_aicl_result = sgm41542_chg_get_dyna_aicl_result,
#endif
#ifdef CONFIG_OPLUS_RTC_DET_SUPPORT
	.check_rtc_reset = rtc_reset_check,
#endif
	.get_charger_subtype = sgm41542_get_charger_subtype,
	.set_qc_config = sgm41542_set_qc_config,
	.oplus_chg_pd_setup = oplus_sgm41542_pd_setup,
	.oplus_chg_get_pd_type = oplus_sgm41542_get_pd_type,
	.enable_qc_detect = oplus_chg_enable_qc_detect,
	.input_current_write_without_aicl = sgm41542_input_current_limit_without_aicl,
	.oplus_chg_wdt_enable = sgm41542_wdt_enable,
	.get_usbtemp_volt = oplus_get_usbtemp_volt,
	.set_typec_sinkonly = oplus_mt6789_usbtemp_set_typec_sinkonly,
	.set_typec_cc_open = oplus_mt6789_usbtemp_set_cc_open,
	.really_suspend_charger = sgm41542_really_suspend_charger,
	.oplus_usbtemp_monitor_condition = oplus_usbtemp_condition,
	.vooc_timeout_callback = sgm41542_vooc_timeout_callback,
	.enable_shipmode = sgm41542_set_shipmode,
};

static enum power_supply_usb_type sgm41542_charger_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_DRP,
	POWER_SUPPLY_USB_TYPE_APPLE_BRICK_ID
};

static enum power_supply_property sgm41542_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
};

static int sgm41542_charger_get_online(struct chip_sgm41542 *chip, bool *val)
{
	bool pwr_rdy = false;
	int ret = 0;
	int reg_val;

	ret = sgm41542_read_reg(chip, REG08_SGM41542_ADDRESS, &reg_val);

	if (0 == ret)
		pwr_rdy = !!(reg_val & REG08_SGM41542_POWER_GOOD_STAT_MASK);

	if ((oplus_vooc_get_fastchg_started() == TRUE) && (pwr_rdy == 0))
		pwr_rdy = 1;

	pr_info("reg_val:%x, online = %d\n", reg_val, pwr_rdy);
	*val = pwr_rdy;

	return 0;
}
static int sgm41542_charger_get_property(struct power_supply *psy,
						   enum power_supply_property psp,
						   union power_supply_propval *val)
{
	struct chip_sgm41542 *chip = power_supply_get_drvdata(psy);
	bool pwr_rdy = false;
	int ret = 0;
	int boot_mode = get_boot_mode();

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = sgm41542_charger_get_online(chip, &pwr_rdy);
		val->intval = pwr_rdy;
		break;
	case POWER_SUPPLY_PROP_TYPE:
	case POWER_SUPPLY_PROP_USB_TYPE:
		if (boot_mode == META_BOOT)
			val->intval = POWER_SUPPLY_TYPE_USB;
		else
			val->intval = charger_ic->oplus_charger_type;

		pr_info("sgm41542 get power_supply_type = %d\n", val->intval);
		break;
	default:
		ret = -ENODATA;
	}
	return ret;
}

static char *sgm41542_charger_supplied_to[] = {
	"battery",
	"mtk-master-charger"
};

static const struct power_supply_desc sgm41542_charger_desc = {
	.type			= POWER_SUPPLY_TYPE_USB,
	.usb_types      = sgm41542_charger_usb_types,
	.num_usb_types  = ARRAY_SIZE(sgm41542_charger_usb_types),
	.properties 	= sgm41542_charger_properties,
	.num_properties 	= ARRAY_SIZE(sgm41542_charger_properties),
	.get_property		= sgm41542_charger_get_property,
};
static int sgm41542_chg_init_psy(struct chip_sgm41542 *chip)
{
	struct power_supply_config cfg = {
		.drv_data = chip,
		.of_node = chip->dev->of_node,
		.supplied_to = sgm41542_charger_supplied_to,
		.num_supplicants = ARRAY_SIZE(sgm41542_charger_supplied_to),
	};

	pr_err("%s\n", __func__);
	memcpy(&chip->psy_desc, &sgm41542_charger_desc, sizeof(chip->psy_desc));
	chip->psy_desc.name = "sgm41542";
	chip->psy = devm_power_supply_register(chip->dev, &chip->psy_desc,
						&cfg);
	return IS_ERR(chip->psy) ? PTR_ERR(chip->psy) : 0;
}

static int pd_tcp_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	struct chip_sgm41542 * chip =
		(struct chip_sgm41542 *)container_of(nb,
		struct chip_sgm41542, pd_nb);

	switch (event) {
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.old_state == TYPEC_UNATTACHED &&
		    noti->typec_state.new_state == TYPEC_ATTACHED_SNK) {
			chg_debug("USB Plug in\n");
			power_supply_changed(chip->psy);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SNK
			&& noti->typec_state.new_state == TYPEC_UNATTACHED) {
			sgm41542_unsuspend_charger();
			chg_debug("USB Plug out\n");
			if (oplus_vooc_get_fast_chg_type() == CHARGER_SUBTYPE_FASTCHG_VOOC && oplus_chg_get_wait_for_ffc_flag() != true) {
				chip->power_good = 0;
				chip->bc12_done = false;
				chip->bc12_retried = 0;
				chip->bc12_delay_cnt = 0;
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
				sgm41542_disable_charging();
				oplus_chg_set_charger_type_unknown();
				oplus_vooc_set_fastchg_type_unknow();
				sgm41542_inform_charger_type(chip);
				oplus_vooc_reset_fastchg_after_usbout();
				oplus_chg_clear_chargerid_info();
				Charger_Detect_Release();
				oplus_chg_wake_update_work();
				chg_debug("usb real remove vooc fastchg clear flag!\n");
			}
		}
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

#define INIT_WORK_NORMAL_DELAY 8000
#define INIT_WORK_OTHER_DELAY 1000
static int sgm41542_charger_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct chip_sgm41542 *chip = NULL;
	int ret = 0;

	chg_err("sgm41542 probe enter\n");

	chip = devm_kzalloc(&client->dev, sizeof(struct chip_sgm41542), GFP_KERNEL);
	if (!chip) {
		return -ENOMEM;
	}

	charger_ic = chip;
	chip->dev = &client->dev;
	chip->client = client;

	i2c_set_clientdata(client, chip);
	mutex_init(&chip->i2c_lock);
	mutex_init(&chip->dpdm_lock);
	atomic_set(&chip->charger_suspended, 0);
	atomic_set(&chip->is_suspended, 0);
	oplus_chg_awake_init(chip);
	init_waitqueue_head(&chip->wait);
	oplus_keep_resume_awake_init(chip);
	chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->power_good = false;
	chip->before_suspend_icl = 0;
	chip->before_unsuspend_icl = 0;
	chip->is_sgm41542 = true;
	chip->is_bq25601d = false;
	chip->bc12_done = false;
	chip->bc12_retried = 0;
	chip->bc12_delay_cnt = 0;
	chip->batfet_reset_disable = true;
	chip->support_hvdcp = false;
	chip->is_hvdcp = false;
	btb_detect_over = false;
	ret = sgm41542_get_deivce_online();
	if (ret < 0) {
		chg_err("!!!sgm41542 is not detected\n");
		goto err_nodev;
	}

	if (!chip->is_sgm41542) {
		chg_err("not support sgm41542\n");
		ret = -ENOTSUPP;
		goto err_nodev;
	}

	ret = sgm41542_parse_dt(chip);
	if (ret) {
		chg_err("Couldn't parse device tree ret=%d\n", ret);
		goto err_parse_dt;
	}

	sgm41542_enable_gpio_init(chip);
	sgm41542_dump_registers();
	sgm41542_hardware_init();

	ret = sgm41542_chg_init_psy(chip);
	if (ret < 0) {
		pr_err("failed to init power supply\n");
		goto err_register_psy;
	}

	chip->chg_dev = charger_device_register(chip->chg_dev_name,
					      &client->dev, chip,
					      &sgm41542_chg_ops,
					      &sgm41542_chg_props);
	if (IS_ERR_OR_NULL(chip->chg_dev)) {
		ret = PTR_ERR(chip->chg_dev);
		chg_err("sgm41542 charger_device_register fail\n");
		goto err_device_register;
	}

	ret = sgm41542_irq_register(chip);
	if (ret) {
		chg_err("Failed to register irq ret=%d\n", ret);
		goto err_irq;
	}

	ret = device_create_file(chip->dev, &dev_attr_sgm41542_access);
	if (ret) {
		chg_err("create sgm41542_access file fail ret=%d\n", ret);
		goto err_create_file;
	}

	INIT_DELAYED_WORK(&chip->bc12_retry_work, sgm41542_bc12_retry_work);
	INIT_DELAYED_WORK(&chip->init_work, sgm41542_init_work_handler);
	INIT_DELAYED_WORK(&chip->qc_detect_work, sgm41542_qc_detect_work);

	if (strcmp(chip->chg_dev_name, "primary_chg") == 0) {
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(14000));
		chip->tcpc = tcpc_dev_get_by_name("type_c_port0");

		if (!chip->tcpc)
			chr_err("%s get tcpc device type_c_port0 fail\n", __func__);
	}

	chip->pd_nb.notifier_call = pd_tcp_notifier_call;
	ret = register_tcp_dev_notifier(chip->tcpc, &chip->pd_nb, TCP_NOTIFY_TYPE_ALL);
	if (ret < 0) {
		pr_notice("register tcpc notifer fail\n");
		ret = -EINVAL;
		goto err_register_tcp_notifier;
	}

#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (NORMAL_BOOT == get_boot_mode())
#else
	if (MSM_BOOT_MODE__NORMAL == get_boot_mode())
#endif
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(INIT_WORK_NORMAL_DELAY));
	else
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(INIT_WORK_OTHER_DELAY));

	set_charger_ic(SGM41542);

	chg_err("SGM41542 probe success.");

	return 0;

err_register_tcp_notifier:
err_irq:
err_create_file:
	device_remove_file(chip->dev, &dev_attr_sgm41542_access);
err_nodev:
err_device_register:
err_register_psy:
err_parse_dt:
	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&chip->i2c_lock);
	charger_ic = NULL;
	devm_kfree(chip->dev, chip);
	return ret;
}

static int sgm41542_charger_remove(struct i2c_client *client)
{
	struct chip_sgm41542 *chip = i2c_get_clientdata(client);

	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&chip->i2c_lock);
	cancel_delayed_work_sync(&chip->qc_detect_work);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static int sgm41542_pm_resume(struct device *dev)
{
	struct chip_sgm41542 *chip = NULL;
	struct i2c_client *client = to_i2c_client(dev);

	pr_err("+++ complete %s: enter +++\n", __func__);
	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			wake_up_interruptible(&charger_ic->wait);
			atomic_set(&chip->charger_suspended, 0);
		}
	}

	return 0;
}

static int sgm41542_pm_suspend(struct device *dev)
{
	struct chip_sgm41542 *chip = NULL;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip)
			atomic_set(&chip->charger_suspended, 1);
	}

	return 0;
}

static const struct dev_pm_ops sgm41542_pm_ops = {
	.resume			= sgm41542_pm_resume,
	.suspend		= sgm41542_pm_suspend,
};
#else
static int sgm41542_resume(struct i2c_client *client)
{
	struct chip_sgm41542 *chip = i2c_get_clientdata(client);

	if(!chip)
		return 0;

	atomic_set(&chip->charger_suspended, 0);

	return 0;
}

static int sgm41542_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct chip_sgm41542 *chip = i2c_get_clientdata(client);

	if(!chip)
		return 0;

	atomic_set(&chip->charger_suspended, 1);

	return 0;
}
#endif

static void sgm41542_charger_shutdown(struct i2c_client *client)
{
}

static struct of_device_id sgm41542_charger_match_table[] = {
	{.compatible = "oplus,sgm41542", },
	{},
};

MODULE_DEVICE_TABLE(of, sgm41542_charger_match_table);

static const struct i2c_device_id sgm41542_i2c_device_id[] = {
	{ "sgm41542", 0x3b },
	{ },
};

MODULE_DEVICE_TABLE(i2c, sgm41542_i2c_device_id);

static struct i2c_driver sgm41542_charger_driver = {
	.driver = {
		.name = "sgm41542-charger",
		.owner = THIS_MODULE,
		.of_match_table = sgm41542_charger_match_table,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
		.pm 	= &sgm41542_pm_ops,
#endif
	},

	.probe = sgm41542_charger_probe,
	.remove = sgm41542_charger_remove,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	.resume		= sgm41542_resume,
	.suspend	= sy6974_suspend,
#endif
	.shutdown = sgm41542_charger_shutdown,
	.id_table = sgm41542_i2c_device_id,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
module_i2c_driver(sgm41542_charger_driver);
#else
void sgm41542_charger_exit(void)
{
	i2c_del_driver(&sgm41542_charger_driver);
}

int sgm41542_charger_init(void)
{
	int ret = 0;
	chg_err(" init start\n");

	if (i2c_add_driver(&sgm41542_charger_driver) != 0) {
		chg_err(" failed to register sgm41542 i2c driver.\n");
	} else {
		chg_debug(" Success to register sgm41542 i2c driver.\n");
	}
	return ret;
}
#endif

MODULE_DESCRIPTION("Driver for charge IC.");
MODULE_LICENSE("GPL v2");
