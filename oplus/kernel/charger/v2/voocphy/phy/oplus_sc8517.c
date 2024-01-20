//SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[sc8517]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <soc/oplus/device_info.h>
#include <oplus_chg_ic.h>
#include <oplus_chg_module.h>
#include <oplus_chg.h>
#include <oplus_mms_wired.h>
#include <oplus_mms.h>
#include <oplus_impedance_check.h>
#include "../oplus_voocphy.h"
#include "oplus_sc8517.h"

struct sc8517_device {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
	struct oplus_voocphy_manager *voocphy;
	struct ufcs_dev *ufcs;

	struct oplus_chg_ic_dev *cp_ic;
	struct oplus_impedance_node *input_imp_node;
	struct oplus_impedance_node *output_imp_node;

	struct mutex i2c_rw_lock;
	struct mutex chip_lock;
	atomic_t suspended;
	atomic_t i2c_err_count;
	struct wakeup_source *chip_ws;

	int ovp_reg;
	int ocp_reg;

	bool ufcs_enable;

	enum oplus_cp_work_mode cp_work_mode;

	bool rested;
	bool error_reported;

	bool use_ufcs_phy;
	bool use_vooc_phy;
	bool vac_support;
};

static enum oplus_cp_work_mode g_cp_support_work_mode[] = {
	CP_WORK_MODE_BYPASS,
};

static bool sc8517_check_work_mode_support(enum oplus_cp_work_mode mode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_cp_support_work_mode); i++) {
		if (g_cp_support_work_mode[i] == mode)
			return true;
	}
	return false;
}

static void sc8517_i2c_error(struct oplus_voocphy_manager *voocphy, bool happen)
{
	struct sc8517_device *chip;
	int report_flag = 0;

	if (!voocphy)
		return;
	chip = voocphy->priv_data;

	if (!chip || chip->error_reported)
		return;

	if (happen) {
		voocphy->voocphy_iic_err = true;
		voocphy->voocphy_iic_err_num++;
		if (voocphy->voocphy_iic_err_num >= I2C_ERR_NUM) {
			report_flag |= MAIN_I2C_ERROR;
#ifdef OPLUS_CHG_UNDEF /* TODO */
			oplus_chg_sc8517_error(report_flag, NULL, 0);
#endif
			chip->error_reported = true;
		}
	} else {
		voocphy->voocphy_iic_err_num = 0;
#ifdef OPLUS_CHG_UNDEF /* TODO */
		oplus_chg_sc8517_error(0, NULL, 0);
#endif
	}
}

static int __sc8517_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		sc8517_i2c_error(chip, true);
		chg_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u8) ret;

	return 0;
}

static int __sc8517_write_byte(struct i2c_client *client, int reg, u8 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		sc8517_i2c_error(chip, true);
		chg_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
		       val, reg, ret);
		return ret;
	}

	return 0;
}

static int sc8517_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	struct sc8517_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8517 chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sc8517_read_byte(client, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sc8517_write_byte(struct i2c_client *client, u8 reg, u8 data)
{
	struct sc8517_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8517 chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sc8517_write_byte(client, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sc8517_update_bits(struct i2c_client *client, u8 reg,
                              u8 mask, u8 data)
{
	struct sc8517_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;
	u8 tmp;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8517 chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sc8517_read_byte(client, reg, &tmp);
	if (ret) {
		chg_err("failed to read %02X register, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sc8517_write_byte(client, reg, tmp);
	if (ret)
		chg_err("failed to write %02X register, ret=%d\n", reg, ret);
out:
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static s32 sc8517_read_word(struct i2c_client *client, u8 reg)
{
	s32 ret;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	struct sc8517_device *chip;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8517 chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0) {
		/* TODO
		sc8517_i2c_error(chip, true); */
		chg_err("i2c read word fail: can't read reg:0x%02X \n", reg);
		mutex_unlock(&chip->i2c_rw_lock);
		return ret;
	}

	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static s32 sc8517_write_word(struct i2c_client *client, u8 reg, u16 val)
{
	s32 ret;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	struct sc8517_device *chip;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8517 chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_smbus_write_word_data(client, reg, val);
	if (ret < 0) {
		/* TODO
		sc8517_i2c_error(chip, true); */
		chg_err("i2c write word fail: can't write 0x%02X to reg:0x%02X\n", val, reg);
		mutex_unlock(&chip->i2c_rw_lock);
		return ret;
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return 0;
}

static int sc8517_read_i2c_block(struct i2c_client *client, u8 reg, u8 length, u8 *returnData)
{
	struct sc8517_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int rc = 0;
	int retry = 3;

	if (voocphy == NULL) {
		chg_err("voocphy is NULL\n");
		return -ENODEV;
	}
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8547a chip is NULL\n");
		return -ENODEV;
	}

	mutex_lock(&chip->i2c_rw_lock);
	rc = i2c_smbus_read_i2c_block_data(client, reg, length, returnData);

	if (rc < 0) {
		while (retry > 0) {
			usleep_range(5000, 5000);
			rc = i2c_smbus_read_i2c_block_data(client, reg, length, returnData);
			if (rc < 0)
				retry--;
			else
				break;
		}
	}

	if (rc < 0)
		chg_err("read err, rc = %d,\n", rc);
	mutex_unlock(&chip->i2c_rw_lock);

	return rc;
}

static int sc8517_set_predata(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;
	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	/* predata */
	ret = sc8517_write_word(chip->client, SC8517_REG_2A, val);
	if (ret < 0) {
		chg_err("failed to write predata(%d)\n", ret);
		return ret;
	}

	chg_info("write predata 0x%0x\n", val);

	return ret;
}

static int sc8517_set_txbuff(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	/* txbuff */
	ret = sc8517_write_word(chip->client, SC8517_REG_25, val);
	if (ret < 0) {
		chg_err("failed to  write txbuff(%d)\n", ret);
		return ret;
	}

	return ret;
}

static int sc8517_get_adapter_info(struct oplus_voocphy_manager *chip)
{
	s32 data;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	data = sc8517_read_word(chip->client, SC8517_REG_27);

	if (data < 0) {
		chg_err("failed to get adapter info(%d)\n", data);
		return data;
	}

	VOOCPHY_DATA16_SPLIT(data, chip->voocphy_rx_buff, chip->vooc_flag);
	chg_info("data: 0x%0x, vooc_flag: 0x%0x, vooc_rxdata: 0x%0x\n",
		 data, chip->vooc_flag, chip->voocphy_rx_buff);

	return 0;
}

static void sc8517_update_data(struct oplus_voocphy_manager *chip)
{
	/* interrupt_flag */
	/* sc8517_read_byte(chip->client, SC8517_REG_09, &data); */
	chip->interrupt_flag = 0;
	chip->cp_vsys = 0;
	chip->cp_vbat = 0;
	chip->cp_ichg = 0;
	chip->cp_vbus = 0;

	chg_info("cp_ichg = %d cp_vbus = %d, cp_vsys = %d cp_vbat = %d interrupt_flag = %d",
		 chip->cp_ichg, chip->cp_vbus, chip->cp_vsys, chip->cp_vbat, chip->interrupt_flag);
}

static int sc8517_cp_vbus(struct oplus_voocphy_manager *chip)
{
	int vbus;
	struct oplus_mms *wired_topic;
	union mms_msg_data data = { 0 };

	wired_topic = oplus_mms_get_by_name("wired");
	if (!wired_topic)
		return 0;

	oplus_mms_get_item_data(wired_topic, WIRED_ITEM_VBUS, &data, true);
	vbus = data.intval;

	return vbus;
}

static int sc8517_get_cp_ichg(struct oplus_voocphy_manager *chip)
{
	return 0;
}

int sc8517_get_cp_vbat(struct oplus_voocphy_manager *chip)
{
	return 0;
}

static int sc8517_reg_reset(struct oplus_voocphy_manager *chip, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = SC8517_RESET_REG;
	else
		val = SC8517_NO_REG_RESET;

	val <<= SC8517_REG_RESET_SHIFT;

	ret = sc8517_update_bits(chip->client, SC8517_REG_06,
	                         SC8517_REG_RESET_MASK, val);

	chg_err("%s register reset\n", enable ? "enable" : "false");

	return ret;
}

static int sc8517_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	ret = sc8517_read_byte(chip->client, SC8517_REG_02, data);
	if (ret < 0) {
		chg_err("failed get chg enable status(%d)\n", ret);
		return ret;
	}

	*data = *data & SC8517_CHG_EN_MASK;

	return ret;
}

static int sc8517_set_chg_enable(struct oplus_voocphy_manager *chip, bool enable)
{
	int ret = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	/* vac range disable */
	sc8517_write_byte(chip->client, SC8517_REG_02, 0x7a);

	if (enable)
		ret = sc8517_write_byte(chip->client, SC8517_REG_02, ENABLE_MOS); /* enable mos */
	else
		ret = sc8517_write_byte(chip->client, SC8517_REG_02, DISENABLE_MOS); /* disable mos */

	if (ret < 0) {
		chg_err("failed to set chg enable(%d)\n", ret);
		return ret;
	}

	chg_err("%s\n", enable ? "enable" : "false");

	return ret;
}


static int sc8517_get_adc_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	return 0;
}

static int sc8517_set_adc_enable(struct oplus_voocphy_manager *chip, bool enable)
{
	return 0;
}

static u8 sc8517_get_vbus_status(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 value = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	ret = sc8517_read_byte(chip->client, SC8517_REG_0B, &value);
	if (ret < 0) {
		chg_err("failed to get vbus status(%d)\n", ret);
		return ret;
	}

	value = (value & VBUS_INRANGE_STATUS_MASK) >> VBUS_INRANGE_STATUS_SHIFT;
	chg_info("vbus status: %d\n", value);

	return value;
}

static int sc8517_get_voocphy_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	ret = sc8517_read_byte(chip->client, SC8517_REG_2B, data);
	if (ret < 0)
		chg_err("failed to get voocphy enable status(%d)\n", ret);

	return ret;
}

static void sc8517_dump_reg_in_err_issue(struct oplus_voocphy_manager *chip)
{
	if (!chip) {
		chg_err("chip is null\n");
		return;
	}

	chg_info("SC8517_REG_09 -->09~0E[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n",
		 chip->reg_dump[9], chip->reg_dump[10], chip->reg_dump[11],
		 chip->reg_dump[12], chip->reg_dump[13], chip->reg_dump[14]);

	return;
}

static u8 sc8517_get_int_value(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 int_column[6];

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	ret = sc8517_read_i2c_block(chip->client, SC8517_REG_09, 6, int_column);
	if (ret < 0) {
		sc8517_i2c_error(chip, true);
		chg_err("read SC8517_REG_09 6 bytes failed(%d)\n", ret);
		memset(chip->int_column, 0, sizeof(chip->int_column));
		/* set int_column[1]=1, otherwise the dcdc chg can't be enabled*/
		chip->int_column[1] = BIT(0);
		return chip->int_column[1];
	}

	memcpy(chip->int_column, int_column, sizeof(chip->int_column));
	chg_info("SC8517_REG_09 -->09~0E[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n",
		 chip->int_column[0], chip->int_column[1], chip->int_column[2],
		 chip->int_column[3], chip->int_column[4], chip->int_column[5]);

	return int_column[1];
}

static u8 sc8517_get_chg_auto_mode(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 value = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}
	ret = sc8517_read_byte(chip->client, SC8517_REG_15, &value);
	value = value & SC8517_CHG_MODE_MASK;
	chg_info("get auto mode value = %d\n", value);

	return value;
}

static int sc8517_set_chg_auto_mode(struct oplus_voocphy_manager *chip, bool enable)
{
	int ret = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	chg_info("enable = %d\n", enable);

	if (enable && (sc8517_get_chg_auto_mode(chip) == SC8517_CHG_FIX_MODE))
		ret = sc8517_update_bits(chip->client, SC8517_REG_15,
					 SC8517_CHG_MODE_MASK,
					 SC8517_CHG_AUTO_MODE);
	else if (!enable && (sc8517_get_chg_auto_mode(chip) == SC8517_CHG_AUTO_MODE))
		ret = sc8517_update_bits(chip->client, SC8517_REG_15,
					 SC8517_CHG_MODE_MASK,
					 SC8517_CHG_FIX_MODE);
	if (ret < 0)
		chg_err("failed to %s auto mode\n", enable ? "enable" : "disable");

	return ret;
}

static void sc8517_set_pd_svooc_config(struct oplus_voocphy_manager *chip, bool enable)
{
	if (!chip) {
		chg_err("chip is null\n");
		return;
	}

	sc8517_write_byte(chip->client, SC8517_REG_04, 0x36); /* WD:1000ms */
	sc8517_write_byte(chip->client, SC8517_REG_2C, 0xd1); /* Loose_det=1 */

	chg_info("pd svooc \n");
}

static bool sc8517_get_pd_svooc_config(struct oplus_voocphy_manager *chip)
{
	if (!chip) {
		chg_err("Failed\n");
		return false;
	}
	chg_info("no ucp flag,return true\n");

	return true;
}

void sc8517_send_handshake(struct oplus_voocphy_manager *chip)
{
	/* enable voocphy and handshake */
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x81);
}


static int sc8517_reset_voocphy(struct oplus_voocphy_manager *chip)
{
	/* turn off mos */
	sc8517_write_byte(chip->client, SC8517_REG_02, 0x78);
	/* clear tx data */
	sc8517_write_byte(chip->client, SC8517_REG_25, 0x00);
	sc8517_write_byte(chip->client, SC8517_REG_26, 0x00);
	/* disable vooc phy irq */
	sc8517_write_byte(chip->client, SC8517_REG_29, 0x7F); /* mask all flag */
	sc8517_write_byte(chip->client, SC8517_REG_10, 0x79); /* disable irq */
	/* set D+ HiZ */
	sc8517_write_byte(chip->client, SC8517_REG_20, 0xc0);

	/* select big bang mode */
	/* disable vooc */
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x00);
	sc8517_write_byte(chip->client, SC8517_REG_04, 0x06); /* disable wdt */

	/* set predata */
	/* sc8517_write_word(chip->client, SC8517_REG_2A, 0x0); */
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x02); /* reset voocphy */
	msleep(1);
	/* sc8517_set_predata(0x0); */
	chg_err("oplus_vooc_reset_voocphy done");

	return VOOCPHY_SUCCESS;
}


static int sc8517_reactive_voocphy(struct oplus_voocphy_manager *chip)
{
	/* set predata 0 */
	sc8517_write_word(chip->client, SC8517_REG_2A, 0x0);

	/* dpdm */
	sc8517_write_byte(chip->client, SC8517_REG_20, 0x21);
	sc8517_write_byte(chip->client, SC8517_REG_21, 0x80);
	sc8517_write_byte(chip->client, SC8517_REG_2C, 0xD1);

	/* clear tx data */
	sc8517_write_byte(chip->client, SC8517_REG_25, 0x00);
	sc8517_write_byte(chip->client, SC8517_REG_26, 0x00);

	/* vooc */
	sc8517_write_byte(chip->client, SC8517_REG_29, 0x05); /* diable vooc int flag mask */
	sc8517_send_handshake(chip);

	chg_info("oplus_vooc_reactive_voocphy done");

	return VOOCPHY_SUCCESS;
}

static int sc8517_init_device(struct oplus_voocphy_manager *chip)
{
	sc8517_reg_reset(chip, true);
	msleep(10);
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x00); /* VOOC_CTRL:disable,no handshake */
	chg_info("\n");

	return 0;
}

static int sc8517_init_vooc(struct oplus_voocphy_manager *chip)
{
	chg_info("start init vooc\n");

	sc8517_write_byte(chip->client, SC8517_REG_24, 0x02); /* reset voocphy */
	sc8517_write_word(chip->client, SC8517_REG_2A, 0x00); /* reset predata 00 */
	msleep(1);
	sc8517_write_byte(chip->client, SC8517_REG_20, 0x21); /* VOOC_CTRL:disable,no handshake */
	sc8517_write_byte(chip->client, SC8517_REG_21, 0x80); /* VOOC_CTRL:disable,no handshake */
	sc8517_write_byte(chip->client, SC8517_REG_2C, 0xD1); /* VOOC_CTRL:disable,no handshake */
	sc8517_write_byte(chip->client, SC8517_REG_29, 0x25); /* mask sedseq flag,rx start,tx done */
	/* sc8517_vac_inrange_enable(chip, SC8517_VAC_INRANGE_DISABLE); */

	return 0;
}

static void sc8517_hardware_init(struct oplus_voocphy_manager *chip)
{
	chg_info("\n");
	sc8517_reg_reset(chip, true);
	msleep(10);
	sc8517_write_byte(chip->client, SC8517_REG_01, 0x5e); /* disable v1x_scp */
	sc8517_write_byte(chip->client, SC8517_REG_06, 0x85); /* enable  audio mode */
	sc8517_write_byte(chip->client, SC8517_REG_08, 0xA6); /* REF_SKIP_R 40mv */
	sc8517_write_byte(chip->client, SC8517_REG_29, 0x05); /* Masked Pulse_filtered, RX_Start,Tx_Done,soft intflag */
	sc8517_write_byte(chip->client, SC8517_REG_10, 0x79); /* Masked Pulse_filtered, RX_Start,Tx_Done */
	sc8517_write_byte(chip->client, SC8517_REG_03, 0xFF); /* set rvs and fwd ocp */
}

static int sc8517_dump_registers(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 int_column[7];

	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	ret = sc8517_read_i2c_block(chip->client, SC8517_REG_00, 7, int_column);
	if (ret < 0) {
		chg_err("read SC8517_REG_00 7 bytes failed(%d)\n", ret);
		return ret;
	}
	chg_info("[00~06][0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
		 int_column[0], int_column[1], int_column[2],
		 int_column[3], int_column[4], int_column[5], int_column[6]);

	ret = sc8517_read_i2c_block(chip->client, SC8517_REG_07, 7, int_column);
	if (ret < 0) {
		chg_err("read SC8517_REG_07 7 bytes failed[%d]\n", ret);
		return ret;
	}
	chg_info("[07~0D][0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
		 int_column[0], int_column[1], int_column[2],
		 int_column[3], int_column[4], int_column[5], int_column[6]);

	ret = sc8517_read_i2c_block(chip->client, SC8517_REG_0E, 7, int_column);
	if (ret < 0) {
		chg_err("read SC8517_REG_0E 7 bytes failed[%d]\n", ret);
		return ret;
	}
	chg_info("[0E~14][0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
		 int_column[0], int_column[1], int_column[2],
		 int_column[3], int_column[4], int_column[5], int_column[6]);

	return ret;
}

static int sc8517_svooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8517_write_byte(chip->client, SC8517_REG_04, 0x36); /* WD:1000ms */
	sc8517_write_byte(chip->client, SC8517_REG_2C, 0xd1); /* Loose_det=1 */

	return 0;
}

static int sc8517_vooc_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8517_write_byte(chip->client, SC8517_REG_04, 0x46); /* WD:5000ms */
	sc8517_write_byte(chip->client, SC8517_REG_2C, 0xd1); /* Loose_det=1 */

	return 0;
}

static int sc8517_5v2a_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8517_write_byte(chip->client, SC8517_REG_02, 0x78); /* disable acdrv,close mos */
	sc8517_write_byte(chip->client, SC8517_REG_04, 0x06); /* dsiable wdt */
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x00); /* close voocphy */

	return 0;
}

static int sc8517_pdqc_hw_setting(struct oplus_voocphy_manager *chip)
{
	sc8517_write_byte(chip->client, SC8517_REG_02, 0x78); /* disable acdrv,close mos */
	sc8517_write_byte(chip->client, SC8517_REG_04, 0x06); /* dsiable wdt */
	sc8517_write_byte(chip->client, SC8517_REG_24, 0x00); /* close voocphy */

	return 0;
}

static int sc8517_hw_setting(struct oplus_voocphy_manager *chip, int reason)
{
	if (!chip) {
		chg_err("chip is null exit\n");
		return -1;
	}
	chg_info("reason = %d\n", reason);
	switch (reason) {
	case SETTING_REASON_PROBE:
	case SETTING_REASON_RESET:
		sc8517_init_device(chip);
		chg_info("SETTING_REASON_RESET OR PROBE\n");
		break;
	case SETTING_REASON_SVOOC:
		sc8517_svooc_hw_setting(chip);
		chg_info("SETTING_REASON_SVOOC\n");
		break;
	case SETTING_REASON_VOOC:
		sc8517_vooc_hw_setting(chip);
		chg_info("SETTING_REASON_VOOC\n");
		break;
	case SETTING_REASON_5V2A:
		sc8517_5v2a_hw_setting(chip);
		chg_info("SETTING_REASON_5V2A\n");
		break;
	case SETTING_REASON_PDQC:
		sc8517_pdqc_hw_setting(chip);
		chg_info("SETTING_REASON_PDQC\n");
		break;
	default:
		chg_info("do nothing\n");
		break;
	}
	return 0;
}

static bool sc8517_check_cp_int_happened(struct oplus_voocphy_manager *chip,
					 bool *dump_reg, bool *send_info)
{
	int i = 0;

	if (((bidirect_int_flag[0].mask & chip->int_column_pre[1]) == 0) && bidirect_int_flag[i].mark_except) {
		*dump_reg = true;
		*send_info = true;
		memcpy(&chip->reg_dump[9], chip->int_column_pre, sizeof(chip->int_column_pre));
		chg_info("cp int happened %s\n", bidirect_int_flag[i].except_info);
		return true;
	}

	for (i = 1; i < 7; i++) {
		if ((bidirect_int_flag[i].mask & chip->int_column_pre[3]) && bidirect_int_flag[i].mark_except) {
			*dump_reg = true;
			*send_info = true;
			memcpy(&chip->reg_dump[9], chip->int_column_pre, sizeof(chip->int_column_pre));
			chg_info("cp int happened %s\n", bidirect_int_flag[i].except_info);
			return true;
		}
	}

	for (i = 7; i < BIDIRECT_IRQ_EVNET_NUM; i++) {
		if ((bidirect_int_flag[i].mask & chip->int_column_pre[5]) && bidirect_int_flag[i].mark_except) {
			*dump_reg = true;
			*send_info = true;
			memcpy(&chip->reg_dump[9], chip->int_column_pre, sizeof(chip->int_column_pre));
			chg_info("cp int happened %s\n", bidirect_int_flag[i].except_info);
			return true;
		}
	}

	return false;
}

static ssize_t sc8517_show_registers(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[300];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc8517");
	for (addr = 0x0; addr <= 0x3C; addr++) {
		if ((addr < 0x24) || (addr > 0x2B && addr < 0x33)
		    || addr == 0x36 || addr == 0x3C) {
			ret = sc8517_read_byte(chip->client, addr, &val);
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

static ssize_t sc8517_store_register(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t count)
{
	struct oplus_voocphy_manager *chip = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg <= 0x3C)
		sc8517_write_byte(chip->client, (unsigned char)reg, (unsigned char)val);

	return count;
}


static DEVICE_ATTR(registers, 0660, sc8517_show_registers, sc8517_store_register);

static void sc8517_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
}


static struct of_device_id sc8517_charger_match_table[] = {
	{
		.compatible = "sc,sc8517-master",
	},
	{},
};

static void register_voocphy_devinfo(void)
{
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int ret = 0;
	char *version;
	char *manufacture;

	version = "sc8517";
	manufacture = "MP";

	ret = register_device_proc("voocphy", version, manufacture);
	if (ret)
		chg_err("register_voocphy_devinfo fail\n");
#endif
}

static struct oplus_voocphy_operations oplus_sc8517_ops = {
	.hardware_init		= sc8517_hardware_init,
	.hw_setting		= sc8517_hw_setting,
	.init_vooc		= sc8517_init_vooc,
	.set_predata		= sc8517_set_predata,
	.set_txbuff		= sc8517_set_txbuff,
	.get_adapter_info	= sc8517_get_adapter_info,
	.update_data		= sc8517_update_data,
	.get_chg_enable		= sc8517_get_chg_enable,
	.set_chg_enable		= sc8517_set_chg_enable,
	.reset_voocphy		= sc8517_reset_voocphy,
	.reactive_voocphy	= sc8517_reactive_voocphy,
	.send_handshake		= sc8517_send_handshake,
	.get_cp_vbat		= sc8517_get_cp_vbat,
	.get_int_value		= sc8517_get_int_value,
	.get_adc_enable		= sc8517_get_adc_enable,
	.set_adc_enable		= sc8517_set_adc_enable,
	.get_ichg		= sc8517_get_cp_ichg,
	.set_pd_svooc_config	= sc8517_set_pd_svooc_config,
	.get_pd_svooc_config	= sc8517_get_pd_svooc_config,
	.get_vbus_status	= sc8517_get_vbus_status,
	.set_chg_auto_mode	= sc8517_set_chg_auto_mode,
	.get_voocphy_enable	= sc8517_get_voocphy_enable,
	.dump_voocphy_reg	= sc8517_dump_reg_in_err_issue,
	.check_cp_int_happened	= sc8517_check_cp_int_happened,
};

static irqreturn_t sc8517_interrupt_handler(int irq, void *dev_id)
{
	struct oplus_voocphy_manager *voocphy = dev_id;

	return oplus_voocphy_interrupt_handler(voocphy);
}

static int sc8517_irq_gpio_init(struct oplus_voocphy_manager *chip)
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

static int sc8517_irq_register(struct sc8517_device *chip)
{
	struct oplus_voocphy_manager *voocphy = chip->voocphy;
	struct irq_desc *desc;
	struct cpumask current_mask;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	cpumask_var_t cpu_highcap_mask;
#endif
	int ret;

	ret = sc8517_irq_gpio_init(voocphy);
	if (ret < 0) {
		chg_err("failed to irq gpio init(%d)\n", ret);
		return ret;
	}

	if (voocphy->irq) {
		ret = request_threaded_irq(voocphy->irq, NULL,
					   sc8517_interrupt_handler,
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
static int sc8517_cp_init(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	ic_dev->online = true;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_ONLINE);

	return 0;
}

static int sc8517_cp_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	ic_dev->online = false;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_OFFLINE);

	return 0;
}

static int sc8517_cp_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	struct sc8517_device *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	sc8517_dump_reg_in_err_issue(chip->voocphy);
	return 0;
}

static int sc8517_cp_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static int sc8517_cp_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	return 0;
}

static int sc8517_cp_hw_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct sc8517_device *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	if (chip->rested)
		return 0;

	sc8517_hardware_init(chip->voocphy);
	return 0;
}

static int sc8517_cp_set_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	struct sc8517_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);
	rc = sc8517_svooc_hw_setting(chip->voocphy);
	if (rc < 0)
		chg_err("set work mode to %d error\n", mode);

	return rc;
}

static int sc8517_cp_get_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode *mode)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	*mode = CP_WORK_MODE_BYPASS;

	return 0;
}

static int sc8517_cp_check_work_mode_support(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	return sc8517_check_work_mode_support(mode);
}

static int sc8517_cp_set_iin(struct oplus_chg_ic_dev *ic_dev, int iin)
{
	return 0;
}

static int sc8517_cp_get_vin(struct oplus_chg_ic_dev *ic_dev, int *vin)
{
	struct sc8517_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8517_cp_vbus(chip->voocphy);
	if (rc < 0) {
		chg_err("can't get cp vin, rc=%d\n", rc);
		return rc;
	}
	*vin = rc;

	return 0;
}

static int sc8517_cp_get_iin(struct oplus_chg_ic_dev *ic_dev, int *iin)
{
	struct sc8517_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8517_get_cp_ichg(chip->voocphy);
	if (rc < 0) {
		chg_err("can't get cp iin, rc=%d\n", rc);
		return rc;
	}
	*iin = rc;

	return 0;
}

static int sc8517_cp_get_vout(struct oplus_chg_ic_dev *ic_dev, int *vout)
{
	struct sc8517_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8517_get_cp_vbat(chip->voocphy);
	if (rc < 0) {
		chg_err("can't get cp vout, rc=%d\n", rc);
		return rc;
	}
	*vout = rc;

	return 0;
}

static int sc8517_cp_get_iout(struct oplus_chg_ic_dev *ic_dev, int *iout)
{
	struct sc8517_device *chip;
	int iin;
	bool working;
	enum oplus_cp_work_mode work_mode;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	/*
	 * There is an exception in the iout adc of sc8537a, which is obtained
	 * indirectly through iin
	 */
	rc = oplus_chg_ic_func(ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_STATUS, &working);
	if (rc < 0)
		return rc;
	if (!working) {
		*iout = 0;
		return 0;
	}
	rc = oplus_chg_ic_func(ic_dev, OPLUS_IC_FUNC_CP_GET_IIN, &iin);
	if (rc < 0)
		return rc;
	rc = oplus_chg_ic_func(ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_MODE, &work_mode);
	if (rc < 0)
		return rc;
	switch (work_mode) {
	case CP_WORK_MODE_BYPASS:
		*iout = iin;
		break;
	case CP_WORK_MODE_2_TO_1:
		*iout = iin * 2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sc8517_cp_get_vac(struct oplus_chg_ic_dev *ic_dev, int *vac)
{
	struct sc8517_device *chip;
	/* u8 data_block[2] = { 0 }; */
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);
	if (!chip->vac_support)
		return -ENOTSUPP;

	/* rc = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_17, 2, data_block); */
	if (rc < 0) {
		/* sc8547_i2c_error(chip->voocphy, true); */
		chg_err("sc8547 read vac error, rc=%d\n", rc);
		return rc;
	} else {
		/* sc8547_i2c_error(chip->voocphy, false); */
	}

	/* *vac = (((data_block[0] & SC8547_VAC_POL_H_MASK) << 8) | data_block[1]) * SC8547_VAC_ADC_LSB; */

	return 0;
}

static int sc8517_cp_set_work_start(struct oplus_chg_ic_dev *ic_dev, bool start)
{
	struct sc8517_device *chip;
	int rc = 0;
	u8 data = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	chg_info("%s work %s\n", chip->dev->of_node->name, start ? "start" : "stop");
	sc8517_read_byte(chip->voocphy->client, SC8517_REG_02, &data);
	if (start && data == DISENABLE_MOS)
		rc = sc8517_set_chg_enable(chip->voocphy, start);
	else if (!start && data == ENABLE_MOS)
		rc = sc8517_set_chg_enable(chip->voocphy, start);
	if (rc < 0)
		return rc;

	oplus_imp_node_set_active(chip->input_imp_node, start);
	/*oplus_imp_node_set_active(chip->output_imp_node, start); */

	return 0;
}

static int sc8517_cp_get_work_status(struct oplus_chg_ic_dev *ic_dev, bool *start)
{
	struct sc8517_device *chip;
	u8 data;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8517_get_chg_enable(chip->voocphy, &data);
	if (rc < 0) {
		chg_err("read SC8517_chg_enable error, rc=%d\n", rc);
		return rc;
	}

	*start = data & 1;

	return 0;
}

static void *sc8517_cp_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
{
	void *func = NULL;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT)) {
		chg_err("%s is offline\n", ic_dev->name);
		return NULL;
	}

	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT, sc8517_cp_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT, sc8517_cp_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP, sc8517_cp_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST, sc8517_cp_smt_test);
		break;
	case OPLUS_IC_FUNC_CP_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_ENABLE, sc8517_cp_enable);
		break;
	case OPLUS_IC_FUNC_CP_HW_INTI:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_HW_INTI, sc8517_cp_hw_init);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_MODE, sc8517_cp_set_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_MODE, sc8517_cp_get_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT,
			sc8517_cp_check_work_mode_support);
		break;
	case OPLUS_IC_FUNC_CP_SET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_IIN, sc8517_cp_set_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VIN, sc8517_cp_get_vin);
		break;
	case OPLUS_IC_FUNC_CP_GET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IIN, sc8517_cp_get_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VOUT, sc8517_cp_get_vout);
		break;
	case OPLUS_IC_FUNC_CP_GET_IOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IOUT, sc8517_cp_get_iout);
		break;
	case OPLUS_IC_FUNC_CP_GET_VAC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VAC, sc8517_cp_get_vac);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_START:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_START, sc8517_cp_set_work_start);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_STATUS, sc8517_cp_get_work_status);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

struct oplus_chg_ic_virq sc8517_cp_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static int sc8517_ic_register(struct sc8517_device *chip)
{
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	struct device_node *child;
	struct oplus_chg_ic_dev *ic_dev = NULL;
	struct oplus_chg_ic_cfg ic_cfg;
	int rc;

	for_each_child_of_node(chip->dev->of_node, child) {
		rc = of_property_read_u32(child, "oplus,ic_type", &ic_type);
		if (rc < 0)
			continue;
		rc = of_property_read_u32(child, "oplus,ic_index", &ic_index);
		if (rc < 0)
			continue;
		ic_cfg.name = child->name;
		ic_cfg.index = ic_index;
		ic_cfg.type = ic_type;
		ic_cfg.priv_data = chip;
		ic_cfg.of_node = child;
		switch (ic_type) {
		case OPLUS_CHG_IC_CP:
			/* (void)sc8517_init_imp_node(chip, child); */
			snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "cp-sc8517:%d", ic_index);
			snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
			ic_cfg.get_func = sc8517_cp_get_func;
			ic_cfg.virq_data = sc8517_cp_virq_table;
			ic_cfg.virq_num = ARRAY_SIZE(sc8517_cp_virq_table);
			break;
		default:
			chg_err("not support ic_type(=%d)\n", ic_type);
			continue;
		}
		ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
		if (!ic_dev) {
			rc = -ENODEV;
			chg_err("register %s error\n", child->name);
			continue;
		}
		chg_info("register %s\n", child->name);

		switch (ic_dev->type) {
		case OPLUS_CHG_IC_CP:
			chip->cp_work_mode = CP_WORK_MODE_UNKNOWN;
			chip->cp_ic = ic_dev;
			break;
		default:
			chg_err("not support ic_type(=%d)\n", ic_dev->type);
			continue;
		}

		of_platform_populate(child, NULL, NULL, chip->dev);
	}

	return 0;
}

static int sc8517_charger_probe(struct i2c_client *client,
                                const struct i2c_device_id *id)
{
	int ret;
	struct sc8517_device *chip;
	struct oplus_voocphy_manager *voocphy;

	chg_info("start\n");

	chip = devm_kzalloc(&client->dev, sizeof(struct sc8517_device), GFP_KERNEL);
	if (!chip) {
		dev_err(&client->dev, "alloc sc8517 device buf error\n");
		return -ENOMEM;
	}

	voocphy = devm_kzalloc(&client->dev, sizeof(struct oplus_voocphy_manager), GFP_KERNEL);
	if (voocphy == NULL) {
		chg_err("alloc voocphy buf error\n");
		ret = -ENOMEM;
		goto chg_err;
	}

	chip->client = client;
	chip->dev = &client->dev;
	voocphy->client = client;
	voocphy->dev = &client->dev;
	voocphy->priv_data = chip;
	chip->voocphy = voocphy;
	mutex_init(&chip->i2c_rw_lock);
	mutex_init(&chip->chip_lock);
	i2c_set_clientdata(client, voocphy);

	sc8517_create_device_node(&(client->dev));
	voocphy->ops = &oplus_sc8517_ops;
	ret = oplus_register_voocphy(voocphy);
	if (ret < 0) {
		chg_err("failed to register voocphy, ret = %d", ret);
		goto reg_voocphy_err;
	}
	ret = sc8517_irq_register(chip);
	if (ret < 0) {
		chg_err("irq register error, rc=%d\n", ret);
		goto reg_irq_err;
	}

	ret = sc8517_ic_register(chip);
	if (ret < 0) {
		chg_err("cp ic register error\n");
		goto cp_reg_err;
	}

	register_voocphy_devinfo();
	sc8517_cp_init(chip->cp_ic);
	sc8517_dump_registers(voocphy);

	chg_info("sc8517(%s) probe successfully\n", chip->dev->of_node->name);

	return 0;

cp_reg_err:
reg_irq_err:
reg_voocphy_err:
	devm_kfree(&client->dev, voocphy);
chg_err:
	devm_kfree(&client->dev, chip);
	return ret;
}

static void sc8517_charger_shutdown(struct i2c_client *client)
{
	sc8517_update_bits(client, SC8517_REG_06, SC8517_REG_RESET_MASK,
			   SC8517_RESET_REG << SC8517_REG_RESET_SHIFT);
	msleep(10);
	/* disable v1x_scp */
	sc8517_write_byte(client, SC8517_REG_01, 0x5e);
	/* REF_SKIP_R 40mv */
	sc8517_write_byte(client, SC8517_REG_08, 0xA6);
	/* Masked Pulse_filtered, RX_Start,Tx_Done,bit soft intflag */
	sc8517_write_byte(client, SC8517_REG_29, 0x05);

	return;
}

static const struct i2c_device_id sc8517_charger_id[] = {
	{"sc8517-master", 0},
	{},
};

static struct i2c_driver sc8517_charger_driver = {
	.driver		= {
		.name	= "sc8517-charger",
		.owner	= THIS_MODULE,
		.of_match_table = sc8517_charger_match_table,
	},
	.id_table	= sc8517_charger_id,

	.probe		= sc8517_charger_probe,
	.shutdown	= sc8517_charger_shutdown,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
static int __init sc8517_subsys_init(void)
{
	int ret = 0;
	chg_debug(" init start\n");

	if (i2c_add_driver(&sc8517_charger_driver) != 0) {
		chg_err(" failed to register sc8517 i2c driver.\n");
	} else {
		chg_debug(" Success to register sc8517 i2c driver.\n");
	}

	return ret;
}

subsys_initcall(sc8517_subsys_init);
#else
static int sc8517_subsys_init(void)
{
	int ret = 0;
	chg_debug(" init start\n");

	if (i2c_add_driver(&sc8517_charger_driver) != 0) {
		chg_err(" failed to register sc8517 i2c driver.\n");
	} else {
		chg_debug(" Success to register sc8517 i2c driver.\n");
	}

	return ret;
}

static void sc8517_subsys_exit(void)
{
	i2c_del_driver(&sc8517_charger_driver);
}
oplus_chg_module_register(sc8517_subsys);
#endif /*LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)*/

MODULE_DESCRIPTION("SC SC8517 Charge Pump Driver");
MODULE_LICENSE("GPL v2");
