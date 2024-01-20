// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "OPLUS_CHG[8547a]: %s[%d]: " fmt, __func__, __LINE__

#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

#include <trace/events/sched.h>
#include <linux/ktime.h>
#include <uapi/linux/sched/types.h>
#include <linux/pm_qos.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/sched/clock.h>
#include <soc/oplus/system/oplus_project.h>
#include <linux/rtc.h>
#include <linux/device.h>
#include <linux/of_platform.h>

#include <oplus_chg_ic.h>
#include <oplus_chg_module.h>
#include <oplus_chg.h>
#include "../oplus_voocphy.h"

#include "oplus_sc8547.h"
#include "oplus_sc8547a.h"
#include "../chglib/oplus_chglib.h"

static struct mutex i2c_rw_lock;

static bool error_reported = false;

#define DEFUALT_VBUS_LOW 100
#define DEFUALT_VBUS_HIGH 200
#define I2C_ERR_NUM 10
#define MAIN_I2C_ERROR (1 << 0)
#define OPLUS_SC8547A_UCP_100MS 0x30
#define OPLUS_SC8547A_UCP_5MS 0x10

static int sc8547_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data);

static void sc8547_i2c_error(struct oplus_voocphy_manager *chip, bool happen)
{
	int report_flag = 0;
	if (!chip || error_reported)
		return;

	if (happen) {
		chip->voocphy_iic_err = true;
		chip->voocphy_iic_err_num++;
		if (chip->voocphy_iic_err_num >= I2C_ERR_NUM) {
			report_flag |= MAIN_I2C_ERROR;
#ifdef OPLUS_CHG_UNDEF /* TODO */
			oplus_chg_sc8547_error(report_flag, NULL, 0);
#endif
			error_reported = true;
		}
	} else {
		chip->voocphy_iic_err_num = 0;
#ifdef OPLUS_CHG_UNDEF /* TODO */
		oplus_chg_sc8547_error(0, NULL, 0);
#endif
	}
}

/************************************************************************/
static int __sc8547_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}
	sc8547_i2c_error(chip, false);
	*data = (u8)ret;

	return 0;
}

static int __sc8547_write_byte(struct i2c_client *client, u8 reg, u8 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
		       val, reg, ret);
		return ret;
	}
	sc8547_i2c_error(chip, false);
	return 0;
}

static int sc8547_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = __sc8547_read_byte(client, reg, data);
	mutex_unlock(&i2c_rw_lock);

	return ret;
}

static int sc8547_write_byte(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&i2c_rw_lock);
	ret = __sc8547_write_byte(client, reg, data);
	mutex_unlock(&i2c_rw_lock);

	return ret;
}

static int sc8547_update_bits(struct i2c_client *client, u8 reg, u8 mask,
			      u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&i2c_rw_lock);
	ret = __sc8547_read_byte(client, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sc8547_write_byte(client, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
out:
	mutex_unlock(&i2c_rw_lock);
	return ret;
}

static s32 sc8547_read_word(struct i2c_client *client, u8 reg)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	mutex_lock(&i2c_rw_lock);
	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("i2c read word fail: can't read reg:0x%02X \n", reg);
		mutex_unlock(&i2c_rw_lock);
		return ret;
	}
	sc8547_i2c_error(chip, false);
	mutex_unlock(&i2c_rw_lock);
	return ret;
}

static s32 sc8547_write_word(struct i2c_client *client, u8 reg, u16 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	mutex_lock(&i2c_rw_lock);
	ret = i2c_smbus_write_word_data(client, reg, val);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("i2c write word fail: can't write 0x%02X to reg:0x%02X \n", val, reg);
		mutex_unlock(&i2c_rw_lock);
		return ret;
	}
	sc8547_i2c_error(chip, false);
	mutex_unlock(&i2c_rw_lock);
	return 0;
}

static int sc8547_set_predata(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;
	if (!chip) {
		pr_err("failed: chip is null\n");
		return -1;
	}

	ret = sc8547_write_word(chip->client, SC8547_REG_31, val);
	if (ret < 0) {
		pr_err("failed: write predata\n");
		return -1;
	}
	pr_info("write predata 0x%0x\n", val);
	return ret;
}

static int sc8547_set_txbuff(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;
	if (!chip) {
		pr_err("failed: chip is null\n");
		return -1;
	}

	ret = sc8547_write_word(chip->client, SC8547_REG_2C, val);
	if (ret < 0) {
		pr_err("write txbuff\n");
		return -1;
	}

	return ret;
}

static int sc8547_get_adapter_info(struct oplus_voocphy_manager *chip)
{
	s32 data;
	if (!chip) {
		pr_err("chip is null\n");
		return -1;
	}

	data = sc8547_read_word(chip->client, SC8547_REG_2E);

	if (data < 0) {
		pr_err("sc8547_read_word faile\n");
		return -1;
	}

	VOOCPHY_DATA16_SPLIT(data, chip->voocphy_rx_buff, chip->vooc_flag);
	pr_info("data: 0x%0x, vooc_flag: 0x%0x, vooc_rxdata: 0x%0x\n", data,
		chip->vooc_flag, chip->voocphy_rx_buff);

	return 0;
}

static void sc8547_update_data(struct oplus_voocphy_manager *chip)
{
	u8 data_block[4] = { 0 };
	int i = 0;
	u8 data = 0;
	s32 ret = 0;

	/*int_flag*/
	sc8547_read_byte(chip->client, SC8547_REG_0F, &data);
	chip->interrupt_flag = data;

	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_19, 4,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547_update_data read vsys vbat error \n");
	} else {
		sc8547_i2c_error(chip, false);
	}
	for (i = 0; i < 4; i++) {
		pr_info("read vsys vbat data_block[%d] = %u\n", i,
			data_block[i]);
	}

	chip->cp_vsys = ((data_block[0] << 8) | data_block[1]) * SC8547_VOUT_ADC_LSB;
	chip->cp_vbat = ((data_block[2] << 8) | data_block[3]) * SC8547_VOUT_ADC_LSB;

	memset(data_block, 0, sizeof(u8) * 4);

	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_13, 4,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547_update_data read vsys vbat error \n");
	} else {
		sc8547_i2c_error(chip, false);
	}
	for (i = 0; i < 4; i++) {
		pr_info("read ichg vbus data_block[%d] = %u\n", i,
			data_block[i]);
	}

	chip->cp_ichg =
		((data_block[0] << 8) | data_block[1]) * SC8547_IBUS_ADC_LSB;
	chip->cp_vbus = (((data_block[2] & SC8547_VBUS_POL_H_MASK) << 8) |
			 data_block[3]) *
			SC8547_VBUS_ADC_LSB;

	memset(data_block, 0, sizeof(u8) * 4);

	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_17, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547_update_data read vac error\n");
	} else {
		sc8547_i2c_error(chip, false);
	}
	for (i = 0; i < 2; i++) {
		pr_info("read vac data_block[%d] = %u\n", i, data_block[i]);
	}

	chip->cp_vac = (((data_block[0] & SC8547_VAC_POL_H_MASK) << 8) |
			data_block[1]) *
		       SC8547_VAC_ADC_LSB;

	pr_info("cp_ichg = %d cp_vbus = %d, cp_vsys = %d cp_vbat = %d cp_vac = "
		"%d int_flag = %d",
		chip->cp_ichg, chip->cp_vbus, chip->cp_vsys, chip->cp_vbat,
		chip->cp_vac, chip->interrupt_flag);
}

static int sc8547_get_cp_ichg(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	int cp_ichg = 0;
	u8 cp_enable = 0;
	s32 ret = 0;

	sc8547_get_chg_enable(chip, &cp_enable);

	if (cp_enable == 0)
		return 0;
	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_13, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547 read ichg error \n");
	} else {
		sc8547_i2c_error(chip, false);
	}

	cp_ichg = ((data_block[0] << 8) | data_block[1]) * SC8547_IBUS_ADC_LSB;

	return cp_ichg;
}

static int sc8547_get_cp_vbat(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_1B, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547 read vbat error \n");
	} else {
		sc8547_i2c_error(chip, false);
	}

	chip->cp_vbat = ((data_block[0] << 8) | data_block[1]) * SC8547_VBAT_ADC_LSB;

	return chip->cp_vbat;
}

static int sc8547_get_cp_vbus(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/* parse data_block for improving time of interrupt */
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_15, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true);
		pr_err("sc8547 read vbat error \n");
	} else {
		sc8547_i2c_error(chip, false);
	}

	chip->cp_vbus = (((data_block[0] & SC8547_VBUS_POL_H_MASK) << 8) |
			 data_block[1]) *
			SC8547_VBUS_ADC_LSB;

	return chip->cp_vbus;
}

/*********************************************************************/
static int sc8547_reg_reset(struct oplus_voocphy_manager *chip, bool enable)
{
	int ret;
	u8 val;
	if (enable)
		val = SC8547_RESET_REG;
	else
		val = SC8547_NO_REG_RESET;

	val <<= SC8547_REG_RESET_SHIFT;

	ret = sc8547_update_bits(chip->client, SC8547_REG_07,
				 SC8547_REG_RESET_MASK, val);

	return ret;
}

static bool sc8547a_hw_version_check(struct oplus_voocphy_manager *chip)
{
	int ret;
	u8 val;
	ret = sc8547_read_byte(chip->client, SC8547_REG_36, &val);
	if (val == SC8547A_DEVICE_ID)
		return true;
	else
		return false;
}

static int sc8547_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_07, data);
	if (ret < 0) {
		pr_err("SC8547_REG_07\n");
		return -1;
	}

	*data = *data >> 7;

	return ret;
}

static int sc8547_get_voocphy_enable(struct oplus_voocphy_manager *chip,
				     u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_2B, data);
	if (ret < 0) {
		pr_err("SC8547_REG_2B\n");
		return -1;
	}

	return ret;
}

static void sc8547_dump_reg_in_err_issue(struct oplus_voocphy_manager *chip)
{
	int i = 0, p = 0;
	if (!chip) {
		pr_err("!!!!! oplus_voocphy_manager chip NULL");
		return;
	}

	for (i = 0; i < 37; i++) {
		p = p + 1;
		sc8547_read_byte(chip->client, i, &chip->reg_dump[p]);
	}
	for (i = 0; i < 9; i++) {
		p = p + 1;
		sc8547_read_byte(chip->client, 43 + i, &chip->reg_dump[p]);
	}
	p = p + 1;
	sc8547_read_byte(chip->client, SC8547_REG_36, &chip->reg_dump[p]);
	p = p + 1;
	sc8547_read_byte(chip->client, SC8547_REG_3A, &chip->reg_dump[p]);
	pr_err("p[%d], ", p);

	return;
}

static int sc8547_get_adc_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_11, data);
	if (ret < 0) {
		pr_err("SC8547_REG_11\n");
		return -1;
	}

	*data = *data >> 7;

	return ret;
}

static u8 sc8547_get_int_value(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;
	u8 state = 0;

	if (!chip) {
		pr_err("%s: chip null\n", __func__);
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_0F, &data);
	if (ret < 0) {
		pr_err(" read SC8547_REG_0F failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_06, &state);
	if (ret < 0) {
		pr_err(" read SC8547_REG_06 failed\n");
		return -1;
	}
	chg_info("REG06 = 0x%x REG0F = 0x%x\n", state, data);

	return data;
}

static int sc8547_set_chg_enable(struct oplus_voocphy_manager *chip,
				 bool enable)
{
	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}
	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
}

static void sc8547_set_pd_svooc_config(struct oplus_voocphy_manager *chip,
				       bool enable)
{
	int ret = 0;
	u8 reg_data = 0;
	if (!chip) {
		pr_err("Failed\n");
		return;
	}

	if (enable) {
		reg_data = 0x90 | (chip->ocp_reg & 0xf);
		sc8547_write_byte(chip->client, SC8547_REG_05, reg_data);
		sc8547_write_byte(chip->client, SC8547_REG_09, 0x13);
	} else {
		reg_data = 0x10 | (chip->ocp_reg & 0xf);
		sc8547_write_byte(chip->client, SC8547_REG_05, reg_data);
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_05, &reg_data);
	if (ret < 0) {
		pr_err("SC8547_REG_05\n");
		return;
	}
	pr_err("pd_svooc config SC8547_REG_05 = %d\n", reg_data);
}

static bool sc8547_get_pd_svooc_config(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;

	if (!chip) {
		pr_err("Failed\n");
		return false;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_05, &data);
	if (ret < 0) {
		pr_err("SC8547_REG_05\n");
		return false;
	}

	pr_err("SC8547_REG_05 = 0x%0x\n", data);

	data = data >> 7;
	if (data == 1)
		return true;
	else
		return false;
}

static int sc8547_set_adc_enable(struct oplus_voocphy_manager *chip,
				 bool enable)
{
	if (!chip) {
		pr_err("Failed\n");
		return -1;
	}

	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x00);
}

static void sc8547_send_handshake(struct oplus_voocphy_manager *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x81);
}

static int sc8547_reset_voocphy(struct oplus_voocphy_manager *chip)
{
	u8 reg_data;

	/* turn off mos */
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	/* sc8547b need disable WDT when exit charging, to avoid after WDT time out.
	IF time out, sc8547b will trigger interrupt frequently.
	in addition, sc8547 and sc8547b WDT will disable when disable CP */
	sc8547_update_bits(chip->client, SC8547_REG_09,
			   SC8547_WATCHDOG_MASK, SC8547_WATCHDOG_DIS);
	/* hwic config with plugout */
	reg_data = 0x20 | (chip->ovp_reg & 0x1f);
	sc8547_write_byte(chip->client, SC8547_REG_00, reg_data);
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01);
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50);
	reg_data = 0x10 | (chip->ocp_reg & 0xf);

	sc8547_write_byte(chip->client, SC8547_REG_05, reg_data);
	sc8547_write_byte(chip->client, SC8547_REG_11, 0x00);

	/* clear tx data */
	sc8547_write_byte(chip->client, SC8547_REG_2C, 0x00);
	sc8547_write_byte(chip->client, SC8547_REG_2D, 0x00);

	/* disable vooc phy irq */
	sc8547_write_byte(chip->client, SC8547_REG_30, 0x7f);

	/* set D+ HiZ */
	sc8547_write_byte(chip->client, SC8547_REG_21, 0xc0);

	/* select big bang mode */

	/* disable vooc */
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00);

	/* set predata */
	sc8547_write_word(chip->client, SC8547_REG_31, 0x0);

	/* mask insert irq */
	sc8547_write_byte(chip->client, SC8547_REG_10, 0x02);
	sc8547_write_byte(chip->client, SC8547A_ADDR_OTG_EN, 0x04);
	pr_err("oplus_vooc_reset_voocphy done");

	return VOOCPHY_SUCCESS;
}

static int sc8547_reactive_voocphy(struct oplus_voocphy_manager *chip)
{
	u8 value;

	/*set predata to avoid cmd of adjust current(0x01)return error, add voocphy bit0 hold time to 800us*/
	sc8547_write_word(chip->client, SC8547_REG_31, 0x0);
	sc8547_read_byte(chip->client, SC8547_REG_3A, &value);
	value = value | (3 << 5);
	sc8547_write_byte(chip->client, SC8547_REG_3A, value);

	/*dpdm*/
	sc8547_write_byte(chip->client, SC8547_REG_21, 0x21);
	sc8547_write_byte(chip->client, SC8547_REG_22, 0x00);
	sc8547_write_byte(chip->client, SC8547_REG_33, 0xD1);

	/*clear tx data*/
	sc8547_write_byte(chip->client, SC8547_REG_2C, 0x00);
	sc8547_write_byte(chip->client, SC8547_REG_2D, 0x00);

	/*vooc*/
	sc8547_write_byte(chip->client, SC8547_REG_30, 0x05);
	sc8547_send_handshake(chip);

	pr_info("oplus_vooc_reactive_voocphy done");

	return VOOCPHY_SUCCESS;
}

static int sc8547_init_device(struct oplus_voocphy_manager *chip)
{
	u8 reg_data;
	sc8547_write_byte(chip->client, SC8547_REG_11,
			  0x0); /* ADC_CTRL:disable */
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x1);
	sc8547_write_byte(chip->client, SC8547_REG_04,
			  0x50); /* VBUS_OVP:10 2:1 or 1:1V */
	reg_data = 0x20 | (chip->ovp_reg & 0x1f);
	sc8547_write_byte(chip->client, SC8547_REG_00,
			  reg_data); /* VBAT_OVP:4.65V */
	reg_data = 0x10 | (chip->ocp_reg & 0xf);
	sc8547_write_byte(chip->client, SC8547_REG_05,
			  reg_data); /* IBUS_OCP_UCP:3.6A */
	sc8547_write_byte(chip->client, SC8547_REG_01, 0xbf);
	sc8547_write_byte(chip->client, SC8547_REG_2B,
			  0x00); /* OOC_CTRL:disable */
	sc8547_write_byte(chip->client, SC8547_REG_3A, 0x60);
	sc8547_update_bits(chip->client, SC8547_REG_09,
			   SC8547_IBUS_UCP_RISE_MASK_MASK,
			   (1 << SC8547_IBUS_UCP_RISE_MASK_SHIFT));
	sc8547_write_byte(chip->client, SC8547_REG_10,
			  0x02); /* mask insert irq */
	sc8547_write_byte(chip->client, SC8547A_ADDR_OTG_EN,
			  0x04); /* OTG EN */
	pr_err("sc8547_init_device done");

	return 0;
}

static int sc8547_init_vooc(struct oplus_voocphy_manager *chip)
{
	u8 value;
	pr_err(" >>>>start init vooc\n");

	sc8547_reg_reset(chip, true);
	sc8547_init_device(chip);

	/*to avoid cmd of adjust current(0x01)return error, add voocphy bit0 hold time to 800us */
	/*SET PREDATA */
	sc8547_write_word(chip->client, SC8547_REG_31, 0x0);
	/*sc8547_set_predata(0x0);*/
	sc8547_read_byte(chip->client, SC8547_REG_3A, &value);
	value = value | (1 << 6);
	sc8547_write_byte(chip->client, SC8547_REG_3A, value);
	pr_err("read value %d\n", value);

	/*dpdm */
	sc8547_write_byte(chip->client, SC8547_REG_21, 0x21);
	sc8547_write_byte(chip->client, SC8547_REG_22, 0x00);
	sc8547_write_byte(chip->client, SC8547_REG_33, 0xD1);

	/*vooc */
	sc8547_write_byte(chip->client, SC8547_REG_30, 0x05);

	return 0;
}

static int sc8547_svooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	u8 reg_data;
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01); /*VAC_OVP:12v*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP:10v*/
	reg_data = 0x10 | (chip->ocp_reg & 0xf);
	sc8547_write_byte(chip->client, SC8547_REG_05,
			  reg_data); /*IBUS_OCP_UCP:3.6A*/
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x13); /*WD:1000ms*/
	sc8547_write_byte(chip->client, SC8547_REG_11,
			  0x80); /*ADC_CTRL:ADC_EN*/
	sc8547_write_byte(chip->client, SC8547_REG_0D, 0x70);
	/*sc8547_write_byte(chip->client, SC8547_REG_2B, 0x81);*/ /*VOOC_CTRL,send handshake*/

	sc8547_write_byte(chip->client, SC8547_REG_33, 0xd1); /*Loose_det=1*/
	sc8547_write_byte(chip->client, SC8547_REG_3A, 0x60);
	return 0;
}

static int sc8547_vooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_05, 0x1c); /*IBUS_OCP_UCP:*/
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x93); /*WD:1000ms*/
	sc8547_write_byte(chip->client, SC8547_REG_11, 0x80); /*ADC_CTRL:*/
	sc8547_write_byte(chip->client, SC8547_REG_33, 0xd1); /*Loose_det*/
	sc8547_write_byte(chip->client, SC8547_REG_3A, 0x60);
	return 0;
}

static int sc8547_5v2a_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x10); /*WD:DISABLE*/

	sc8547_write_byte(chip->client, SC8547_REG_11, 0x00); /*ADC_CTRL:*/
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00); /*VOOC_CTRL*/
	return 0;
}

static int sc8547_pdqc_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_00, 0x2E); /*VAC_OV:*/
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP*/
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x10); /*WD:DISABLE*/
	sc8547_write_byte(chip->client, SC8547_REG_11, 0x00); /*ADC_CTRL*/
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00); /*VOOC_CTRL*/
	pr_err("sc8547_pdqc_hw_setting done");
	return 0;
}

static int sc8547_hw_setting(struct oplus_voocphy_manager *chip, int reason)
{
	if (!chip) {
		pr_err("chip is null exit\n");
		return -1;
	}
	switch (reason) {
	case SETTING_REASON_PROBE:
	case SETTING_REASON_RESET:
		sc8547_init_device(chip);
		pr_info("SETTING_REASON_RESET OR PROBE\n");
		break;
	case SETTING_REASON_SVOOC:
		sc8547_svooc_hw_setting(chip);
		pr_info("SETTING_REASON_SVOOC\n");
		break;
	case SETTING_REASON_VOOC:
		sc8547_vooc_hw_setting(chip);
		pr_info("SETTING_REASON_VOOC\n");
		break;
	case SETTING_REASON_5V2A:
		sc8547_5v2a_hw_setting(chip);
		pr_info("SETTING_REASON_5V2A\n");
		break;
	case SETTING_REASON_PDQC:
		sc8547_pdqc_hw_setting(chip);
		pr_info("SETTING_REASON_PDQC\n");
		break;
	default:
		pr_err("do nothing\n");
		break;
	}
	return 0;
}

static void sc8547_dual_chan_buck_set_ucp(struct oplus_voocphy_manager *chip, int ucp_value)
{
	u8 value;

	if (ucp_value) {
		/* to avoid cp ucp cause buck chg, change ucp as 100ms */
		sc8547_read_byte(chip->client, SC8547_REG_05, &value);
		value = value | OPLUS_SC8547A_UCP_100MS;
		chg_info("set 05 reg = 0x%x\n", value);
		sc8547_write_byte(chip->client, SC8547_REG_05, value);
	} else {
		sc8547_read_byte(chip->client, SC8547_REG_05, &value);
		value = (value & (~OPLUS_SC8547A_UCP_100MS)) | OPLUS_SC8547A_UCP_5MS;
		chg_info("set 05 reg = 0x%x\n", value);
		sc8547_write_byte(chip->client, SC8547_REG_05, value);
	}
}

static void sc8547_hardware_init(struct oplus_voocphy_manager *chip)
{
	sc8547_reg_reset(chip, true);
	sc8547_init_device(chip);
}

static bool sc8547_check_cp_int_happened(struct oplus_voocphy_manager *chip,
					 bool *dump_reg, bool *send_info)
{
	int i = 0;
	struct vphy_chip *v_chip;

	for (i = 0; i < IRQ_EVNET_NUM; i++) {
		if ((int_flag[i].mask & chip->interrupt_flag) && int_flag[i].mark_except) {
			chg_err("cp int happened %s\n", int_flag[i].except_info);
			if (int_flag[i].mask != VOUT_OVP_FLAG_MASK &&
			    int_flag[i].mask != ADAPTER_INSERT_FLAG_MASK &&
			    int_flag[i].mask != VBAT_INSERT_FLAG_MASK)
				*dump_reg = true;
			return true;
		}
	}
	v_chip = oplus_chglib_get_vphy_chip(chip->dev);
	if (chip->interrupt_flag & IBUS_OCP_FLAG_MASK ||
	    (v_chip && v_chip->debug_cp_err))
		oplus_chglib_creat_ic_err(chip->dev, 0);

	return false;
}

static ssize_t sc8547_show_registers(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[300];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc8547");
	for (addr = 0x0; addr <= 0x3C; addr++) {
		if ((addr < 0x24) || (addr > 0x2B && addr < 0x33) ||
		    addr == 0x36 || addr == 0x3C) {
			ret = sc8547_read_byte(chip->client, addr, &val);
			if (ret == 0) {
				len = snprintf(tmpbuf, PAGE_SIZE - idx,
				               "Reg[%.2X] = 0x%.2x\n", addr, val);
				memcpy(&buf[idx], tmpbuf, len);
				idx += len;
			}
		}
	}

	return idx;
}

static ssize_t sc8547_store_register(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg <= 0x3C)
		sc8547_write_byte(chip->client, (unsigned char)reg,
				  (unsigned char)val);

	return count;
}

static DEVICE_ATTR(registers, 0660, sc8547_show_registers,
		   sc8547_store_register);

static void sc8547_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
}

static struct oplus_voocphy_operations oplus_sc8547_ops = {
	.hardware_init = sc8547_hardware_init,
	.hw_setting = sc8547_hw_setting,
	.init_vooc = sc8547_init_vooc,
	.set_predata = sc8547_set_predata,
	.set_txbuff = sc8547_set_txbuff,
	.get_adapter_info = sc8547_get_adapter_info,
	.update_data = sc8547_update_data,
	.get_chg_enable = sc8547_get_chg_enable,
	.set_chg_enable = sc8547_set_chg_enable,
	.reset_voocphy = sc8547_reset_voocphy,
	.reactive_voocphy = sc8547_reactive_voocphy,
	.send_handshake = sc8547_send_handshake,
	.get_cp_vbat = sc8547_get_cp_vbat,
	.get_cp_vbus = sc8547_get_cp_vbus,
	.get_int_value = sc8547_get_int_value,
	.get_adc_enable = sc8547_get_adc_enable,
	.set_adc_enable = sc8547_set_adc_enable,
	.get_ichg = sc8547_get_cp_ichg,
	.set_pd_svooc_config = sc8547_set_pd_svooc_config,
	.get_pd_svooc_config = sc8547_get_pd_svooc_config,
	.get_voocphy_enable = sc8547_get_voocphy_enable,
	.dump_voocphy_reg = sc8547_dump_reg_in_err_issue,
	.check_cp_int_happened = sc8547_check_cp_int_happened,
	.dual_chan_buck_set_ucp = sc8547_dual_chan_buck_set_ucp,
};

static int sc8547_charger_choose(struct oplus_voocphy_manager *chip)
{
	int ret;

	if (!oplus_voocphy_chip_is_null()) {
		pr_err("oplus_voocphy_chip already exists!");
		return 0;
	} else {
		ret = i2c_smbus_read_byte_data(chip->client, 0x07);
		pr_err("0x07 = %d\n", ret);
		if (ret < 0) {
			pr_err("i2c communication fail");
			return -EPROBE_DEFER;
		} else
			return 1;
	}
}

static irqreturn_t sc8547_interrupt_handler(int irq, void *dev_id)
{
	struct oplus_voocphy_manager *voocphy = dev_id;

	return oplus_voocphy_interrupt_handler(voocphy);
}

static int sc8547_irq_gpio_init(struct oplus_voocphy_manager *chip)
{
	int rc;
	struct device_node *node = chip->dev->of_node;

	chip->irq_gpio = of_get_named_gpio(node, "oplus,irq_gpio", 0);
	if (!gpio_is_valid(chip->irq_gpio)) {
		chip->irq_gpio = of_get_named_gpio(node, "oplus_spec,irq_gpio", 0);
		if (!gpio_is_valid(chip->irq_gpio)) {
			chg_err("irq_gpio not specified, rc=%d\n", chip->irq_gpio);
			return chip->irq_gpio;
		}
	}
	rc = gpio_request(chip->irq_gpio, "irq_gpio");
	if (rc) {
		chg_err("unable to request gpio[%d]\n", chip->irq_gpio);
		return rc;
	}
	chg_info("irq_gpio = %d\n", chip->irq_gpio);

	chip->irq = gpio_to_irq(chip->irq_gpio);
	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -EINVAL;
	}

	chip->charging_inter_active =
	    pinctrl_lookup_state(chip->pinctrl, "charging_inter_active");
	if (IS_ERR_OR_NULL(chip->charging_inter_active)) {
		chg_err("failed to get the pinctrl state(%d)\n", __LINE__);
		return -EINVAL;
	}

	chip->charging_inter_sleep =
	    pinctrl_lookup_state(chip->pinctrl, "charging_inter_sleep");
	if (IS_ERR_OR_NULL(chip->charging_inter_sleep)) {
		chg_err("Failed to get the pinctrl state(%d)\n", __LINE__);
		return -EINVAL;
	}

	gpio_direction_input(chip->irq_gpio);
	pinctrl_select_state(chip->pinctrl, chip->charging_inter_active); /* no_PULL */
	rc = gpio_get_value(chip->irq_gpio);
	chg_info("irq_gpio = %d, irq_gpio_stat = %d\n", chip->irq_gpio, rc);

	return 0;
}

static int sc8547_irq_register(struct oplus_voocphy_manager *voocphy)
{
	struct irq_desc *desc;
	struct cpumask current_mask;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	cpumask_var_t cpu_highcap_mask;
#endif
	int ret;

	ret = sc8547_irq_gpio_init(voocphy);
	if (ret < 0) {
		chg_err("failed to irq gpio init(%d)\n", ret);
		return ret;
	}

	if (voocphy->irq) {
		ret = request_threaded_irq(voocphy->irq, NULL,
					   sc8547_interrupt_handler,
					   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					   "voocphy_irq", voocphy);
		if (ret < 0) {
			chg_err("request irq for irq=%d failed, ret =%d\n",
				voocphy->irq, ret);
			return ret;
		}
		enable_irq_wake(voocphy->irq);
		chg_debug("request irq ok\n");
	}

	desc = irq_to_desc(voocphy->irq);
	if (desc == NULL) {
		free_irq(voocphy->irq, voocphy);
		chg_err("desc null\n");
		return ret;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	update_highcap_mask(cpu_highcap_mask);
	cpumask_and(&current_mask, cpu_online_mask, cpu_highcap_mask);
#else
	cpumask_setall(&current_mask);
	cpumask_and(&current_mask, cpu_online_mask, &current_mask);
#endif
	ret = set_cpus_allowed_ptr(desc->action->thread, &current_mask);

	return 0;
}

static int sc8547a_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct oplus_voocphy_manager *chip;
	int ret;

	client->addr = SC8547A_I2C_ADDR;
	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&client->dev, "Couldn't allocate memory\n");
		return -ENOMEM;
	}

	chip->client = client;
	chip->dev = &client->dev;
	mutex_init(&i2c_rw_lock);

	i2c_set_clientdata(client, chip);
	if (!sc8547a_hw_version_check(chip)) {
		pr_err("sc8547 dont use ufcs\n");
		return -ENOMEM;
	}

	ret = sc8547_charger_choose(chip);
	if (ret <= 0)
		return ret;

	sc8547_create_device_node(&(client->dev));

	chip->ops = &oplus_sc8547_ops;

	ret = oplus_register_voocphy(chip);
	if (ret < 0) {
		chg_err("failed to register voocphy, ret = %d", ret);
		return ret;
	}

	ret = sc8547_irq_register(chip);
	if (ret < 0) {
		chg_err("irq register error, rc=%d\n", ret);
		return ret;
	}

	chg_info("sc8547a_parse_dt successfully!\n");

	return 0;
}

static void sc8547a_shutdown(struct i2c_client *client)
{
	sc8547_write_byte(client, SC8547_REG_11, 0x00);
	sc8547_write_byte(client, SC8547_REG_21, 0x00);
	return;
}

static const struct of_device_id sc8547a_match[] = {
	{.compatible = "oplus,sc8547a-ufcs" },
	{},
};

static const struct i2c_device_id sc8547a_id[] = {
	{ "oplus,sc8547a-ufcs", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sc8547a_id);

static struct i2c_driver sc8547a_i2c_driver = {
	.driver =
		{
			.name = "sc8547a-ufcs",
			.owner = THIS_MODULE,
			.of_match_table = sc8547a_match,
		},
	.probe = sc8547a_driver_probe,
	.id_table = sc8547a_id,
	.shutdown = sc8547a_shutdown,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
static int __init sc8547a_i2c_driver_init(void)
{
	int ret = 0;
	chg_info(" init start\n");

	if (i2c_add_driver(&sc8547a_i2c_driver) != 0) {
		chg_err(" failed to register sc8574a i2c driver.\n");
	} else {
		chg_err(" Success to register sc8574a i2c driver.\n");
	}

	return ret;
}

subsys_initcall(sc8547a_i2c_driver_init);
#else
static __init int sc8547a_i2c_driver_init(void)
{
	return i2c_add_driver(&sc8547a_i2c_driver);
}

static __exit void sc8547a_i2c_driver_exit(void)
{
	i2c_del_driver(&sc8547a_i2c_driver);
}

oplus_chg_module_register(sc8547a_i2c_driver);
#endif /*LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)*/

MODULE_DESCRIPTION("SC SC8547A VOOCPHY&UFCS Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("JJ Kong");
