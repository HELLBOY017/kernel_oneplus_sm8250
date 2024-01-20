// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[SC8547A]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/proc_fs.h>

#include <trace/events/sched.h>
#include <linux/ktime.h>
#include <uapi/linux/sched/types.h>
#include <linux/pm_qos.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/sched/clock.h>
#include <linux/rtc.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/oplus_project.h>
#endif
#include <ufcs_class.h>

#include <oplus_chg_ic.h>
#include <oplus_chg_module.h>
#include <oplus_chg.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_impedance_check.h>
#include <oplus_chg_monitor.h>
#include "../voocphy/oplus_voocphy.h"

#include "../voocphy/phy/oplus_sc8547.h"
#include "oplus_hal_sc8547a.h"

#define DEFUALT_VBUS_LOW 100
#define DEFUALT_VBUS_HIGH 200
#define I2C_ERR_NUM 10
#define MAIN_I2C_ERROR (1 << 0)
#define OPLUS_SC8547A_UCP_100MS 0x30
#define OPLUS_SC8547A_UCP_5MS 0x10
static bool ic_sc8547a = false;

struct sc8547a_device {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
	struct oplus_voocphy_manager *voocphy;
	struct ufcs_dev *ufcs;

	struct oplus_chg_ic_dev *cp_ic;
	struct oplus_impedance_node *input_imp_node;
	struct oplus_impedance_node *output_imp_node;

	struct oplus_mms *err_topic;
	struct mms_subscribe *err_subs;

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

	struct work_struct ufcs_regdump_work;
	u8 ufcs_reg_dump[SC8547A_FLAG_NUM];
};

static enum oplus_cp_work_mode g_cp_support_work_mode[] = {
	CP_WORK_MODE_BYPASS,
	CP_WORK_MODE_2_TO_1,
};

static int sc8547_voocphy_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data);

#ifndef I2C_ERR_MAX
#define I2C_ERR_MAX 2
#endif

#define ERR_MSG_BUF	PAGE_SIZE
__printf(3, 4)
static int sc8547a_publish_ic_err_msg(struct sc8547a_device *chip, int sub_type, const char *format, ...)
{
	struct mms_msg *topic_msg;
	va_list args;
	char *buf;
	int rc;

	buf = kzalloc(ERR_MSG_BUF, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	va_start(args, format);
	vsnprintf(buf, ERR_MSG_BUF, format, args);
	va_end(args);

	topic_msg =
		oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH, ERR_ITEM_IC,
					"[%s]-[%d]-[%d]:%s", "cp-sc8547a",
					OPLUS_IC_ERR_UFCS, sub_type,
					buf);
	kfree(buf);
	if (topic_msg == NULL) {
		chg_err("alloc topic msg error\n");
		return -ENOMEM;
	}

	rc = oplus_mms_publish_msg(chip->err_topic, topic_msg);
	if (rc < 0) {
		chg_err("publish error topic msg error, rc=%d\n", rc);
		kfree(topic_msg);
	}

	return rc;
}

static void sc8547_i2c_error(struct oplus_voocphy_manager *voocphy, bool happen, bool read)
{
	struct sc8547a_device *chip;

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
			if (chip->err_topic != NULL)
				sc8547a_publish_ic_err_msg(
					chip, read ? UFCS_ERR_READ : UFCS_ERR_WRITE,
					"%s error", read ? "read" : "write");
#ifdef OPLUS_CHG_UNDEF /* TODO */
			oplus_chg_sc8547_error(report_flag, NULL, 0);
#endif
			chip->error_reported = true;
		}
	} else {
		voocphy->voocphy_iic_err_num = 0;
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
		sc8547_i2c_error(chip, true, true);
		chg_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}
	sc8547_i2c_error(chip, false, true);
	*data = (u8)ret;

	return 0;
}

static int __sc8547_write_byte(struct i2c_client *client, u8 reg, u8 val)
{
	s32 ret;
	struct oplus_voocphy_manager *chip = i2c_get_clientdata(client);

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		sc8547_i2c_error(chip, true, false);
		chg_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
			val, reg, ret);
		return ret;
	}
	sc8547_i2c_error(chip, false, false);
	return 0;
}

static int sc8547_read_byte(struct i2c_client *client, u8 reg, u8 *data)
{
	struct sc8547a_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;

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
	ret = __sc8547_read_byte(client, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sc8547_write_byte(struct i2c_client *client, u8 reg, u8 data)
{
	struct sc8547a_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;

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
	ret = __sc8547_write_byte(client, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sc8547_update_bits(struct i2c_client *client, u8 reg, u8 mask,
			      u8 data)
{
	struct sc8547a_device *chip;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	int ret;
	u8 tmp;

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
	ret = __sc8547_read_byte(client, reg, &tmp);
	if (ret) {
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sc8547_write_byte(client, reg, tmp);
	if (ret)
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);
out:
	mutex_unlock(&chip->i2c_rw_lock);
	return ret;
}

static s32 sc8547_read_word(struct i2c_client *client, u8 reg)
{
	s32 ret;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	struct sc8547a_device *chip;

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
	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0) {
		sc8547_i2c_error(voocphy, true, true);
		chg_err("i2c read word fail: can't read reg:0x%02X \n", reg);
		mutex_unlock(&chip->i2c_rw_lock);
		return ret;
	}
	sc8547_i2c_error(voocphy, false, true);
	mutex_unlock(&chip->i2c_rw_lock);
	return ret;
}

static s32 sc8547_write_word(struct i2c_client *client, u8 reg, u16 val)
{
	s32 ret;
	struct oplus_voocphy_manager *voocphy = i2c_get_clientdata(client);
	struct sc8547a_device *chip;

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
	ret = i2c_smbus_write_word_data(client, reg, val);
	if (ret < 0) {
		sc8547_i2c_error(voocphy, true, false);
		chg_err("i2c write word fail: can't write 0x%02X to reg:0x%02X \n", val, reg);
		mutex_unlock(&chip->i2c_rw_lock);
		return ret;
	}
	sc8547_i2c_error(voocphy, false, false);
	mutex_unlock(&chip->i2c_rw_lock);
	return 0;
}

static void sc8547a_awake_init(struct sc8547a_device *chip)
{
	chip->chip_ws = wakeup_source_register(chip->dev, "sc8547a_ws");
}

__maybe_unused static void sc8547a_set_awake(struct sc8547a_device *chip, bool awake)
{
	static bool pm_flag = false;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->chip_ws);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->chip_ws);
		pm_flag = false;
	}
}

static int sc8547a_read_byte(struct sc8547a_device *chip, u8 addr, u8 *data)
{
	u8 addr_buf = addr & 0xff;
	int rc = 0;

	mutex_lock(&chip->i2c_rw_lock);
	rc = i2c_master_send(chip->client, &addr_buf, 1);
	if (rc < 1) {
		chg_err("write 0x%04x error, rc = %d \n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}

	rc = i2c_master_recv(chip->client, data, 1);
	if (rc < 1) {
		chg_err("read 0x%04x error, rc = %d \n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}
	mutex_unlock(&chip->i2c_rw_lock);
	return 0;
error:
	mutex_unlock(&chip->i2c_rw_lock);
	return rc;
}

static int sc8547a_read_data(struct sc8547a_device *chip, u8 addr, u8 *buf,
			     int len)
{
	u8 addr_buf = addr & 0xff;
	int rc = 0;

	mutex_lock(&chip->i2c_rw_lock);
	rc = i2c_master_send(chip->client, &addr_buf, 1);
	if (rc < 1) {
		chg_err("read 0x%04x error, rc=%d\n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}

	rc = i2c_master_recv(chip->client, buf, len);
	if (rc < len) {
		chg_err("read 0x%04x error, rc=%d\n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}
	mutex_unlock(&chip->i2c_rw_lock);
	return rc;

error:
	mutex_unlock(&chip->i2c_rw_lock);
	return rc;
}

static int sc8547a_write_byte(struct sc8547a_device *chip, u8 addr, u8 data)
{
	u8 buf[2] = { addr & 0xff, data };
	int rc = 0;

	mutex_lock(&chip->i2c_rw_lock);
	rc = i2c_master_send(chip->client, buf, 2);
	if (rc < 2) {
		chg_err("write 0x%04x error, rc = %d \n", addr, rc);
		mutex_unlock(&chip->i2c_rw_lock);
		rc = rc < 0 ? rc : -EIO;
		return rc;
	}
	mutex_unlock(&chip->i2c_rw_lock);
	return 0;
}

static int sc8547a_write_data(struct sc8547a_device *chip, u8 addr,
			      u16 length, u8 *data)
{
	u8 *buf;
	int rc = 0;

	buf = kzalloc(length + 1, GFP_KERNEL);
	if (!buf) {
		chg_err("alloc memorry for i2c buffer error\n");
		return -ENOMEM;
	}

	buf[0] = addr & 0xff;
	memcpy(&buf[1], data, length);

	mutex_lock(&chip->i2c_rw_lock);
	rc = i2c_master_send(chip->client, buf, length + 1);
	if (rc < length + 1) {
		chg_err("write 0x%04x error, ret = %d \n", addr, rc);
		mutex_unlock(&chip->i2c_rw_lock);
		kfree(buf);
		rc = rc < 0 ? rc : -EIO;
		return rc;
	}
	mutex_unlock(&chip->i2c_rw_lock);
	kfree(buf);
	return rc;
}

static int sc8547a_write_bit_mask(struct sc8547a_device *chip, u8 reg,
				  u8 mask, u8 data)
{
	u8 temp = 0;
	int rc = 0;

	rc = sc8547a_read_byte(chip, reg, &temp);
	if (rc < 0)
		return rc;

	temp = (data & mask) | (temp & (~mask));

	rc = sc8547a_write_byte(chip, reg, temp);
	if (rc < 0)
		return rc;

	return 0;
}

static int sc8547_voocphy_set_predata(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;
	if (!chip) {
		chg_err("failed: chip is null\n");
		return -1;
	}

	ret = sc8547_write_word(chip->client, SC8547_REG_31, val);
	if (ret < 0) {
		chg_err("failed: write predata\n");
		return -1;
	}
	chg_info("write predata 0x%0x\n", val);
	return ret;
}

static int sc8547_voocphy_set_txbuff(struct oplus_voocphy_manager *chip, u16 val)
{
	s32 ret;
	if (!chip) {
		chg_err("failed: chip is null\n");
		return -1;
	}

	ret = sc8547_write_word(chip->client, SC8547_REG_2C, val);
	if (ret < 0) {
		chg_err("write txbuff\n");
		return -1;
	}

	return ret;
}

static int sc8547_voocphy_get_adapter_info(struct oplus_voocphy_manager *chip)
{
	s32 data;
	if (!chip) {
		chg_err("chip is null\n");
		return -1;
	}

	data = sc8547_read_word(chip->client, SC8547_REG_2E);

	if (data < 0) {
		chg_err("sc8547_read_word faile\n");
		return -1;
	}

	VOOCPHY_DATA16_SPLIT(data, chip->voocphy_rx_buff, chip->vooc_flag);
	chg_info("data: 0x%0x, vooc_flag: 0x%0x, vooc_rxdata: 0x%0x\n", data,
		chip->vooc_flag, chip->voocphy_rx_buff);

	return 0;
}

static void sc8547_voocphy_update_data(struct oplus_voocphy_manager *chip)
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
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547_update_data read vsys vbat error \n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}
	for (i = 0; i < 4; i++) {
		chg_info("read vsys vbat data_block[%d] = %u\n", i,
			data_block[i]);
	}

	chip->cp_vsys = ((data_block[0] << 8) | data_block[1]) * SC8547_VOUT_ADC_LSB;
	chip->cp_vbat = ((data_block[2] << 8) | data_block[3]) * SC8547_VOUT_ADC_LSB;

	memset(data_block, 0, sizeof(u8) * 4);

	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_13, 4,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547_update_data read vsys vbat error \n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}
	for (i = 0; i < 4; i++) {
		chg_info("read ichg vbus data_block[%d] = %u\n", i,
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
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547_update_data read vac error\n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}
	for (i = 0; i < 2; i++) {
		chg_info("read vac data_block[%d] = %u\n", i, data_block[i]);
	}

	chip->cp_vac = (((data_block[0] & SC8547_VAC_POL_H_MASK) << 8) |
			data_block[1]) *
		       SC8547_VAC_ADC_LSB;

	chg_info("cp_ichg = %d cp_vbus = %d, cp_vsys = %d cp_vbat = %d cp_vac = "
		"%d int_flag = %d",
		chip->cp_ichg, chip->cp_vbus, chip->cp_vsys, chip->cp_vbat,
		chip->cp_vac, chip->interrupt_flag);
}

static int sc8547_voocphy_get_cp_ichg(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	int cp_ichg = 0;
	u8 cp_enable = 0;
	s32 ret = 0;

	sc8547_voocphy_get_chg_enable(chip, &cp_enable);

	if (cp_enable == 0)
		return 0;
	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_13, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547 read ichg error \n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}

	cp_ichg = ((data_block[0] << 8) | data_block[1]) * SC8547_IBUS_ADC_LSB;

	return cp_ichg;
}

static int sc8547_get_cp_ichg(struct sc8547a_device *chip)
{
	u8 data_block[2] = { 0 };
	int cp_ichg = 0;
	u8 cp_enable = 0;
	s32 ret = 0;

	sc8547_voocphy_get_chg_enable(chip->voocphy, &cp_enable);

	if (cp_enable == 0)
		return 0;
	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_13, 2,
					    data_block);
	if (ret < 0) {
		chg_err("sc8547 read ichg error \n");
		return 0;
	}

	cp_ichg = ((data_block[0] << 8) | data_block[1]) * SC8547_IBUS_ADC_LSB;

	return cp_ichg;
}

static int sc8547_voocphy_get_cp_vbat(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_1B, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547 read vbat error \n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}

	chip->cp_vbat = ((data_block[0] << 8) | data_block[1]) * SC8547_VBAT_ADC_LSB;

	return chip->cp_vbat;
}

static int sc8547_get_cp_vbat(struct sc8547a_device *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_1B, 2,
					    data_block);
	if (ret < 0) {
		chg_err("sc8547 read vbat error \n");
		return 0;
	}

	return ((data_block[0] << 8) | data_block[1]) * SC8547_VBAT_ADC_LSB;
}

static int sc8547_voocphy_get_cp_vbus(struct oplus_voocphy_manager *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/* parse data_block for improving time of interrupt */
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_15, 2,
					    data_block);
	if (ret < 0) {
		sc8547_i2c_error(chip, true, true);
		chg_err("sc8547 read vbat error \n");
	} else {
		sc8547_i2c_error(chip, false, true);
	}

	chip->cp_vbus = (((data_block[0] & SC8547_VBUS_POL_H_MASK) << 8) | data_block[1]) * SC8547_VBUS_ADC_LSB;

	return chip->cp_vbus;
}

static int sc8547_get_cp_vbus(struct sc8547a_device *chip)
{
	u8 data_block[2] = { 0 };
	s32 ret = 0;

	/* parse data_block for improving time of interrupt */
	ret = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_15, 2,
					    data_block);
	if (ret < 0) {
		chg_err("sc8547 read vbat error \n");
		return 0;
	}

	return (((data_block[0] & SC8547_VBUS_POL_H_MASK) << 8) | data_block[1]) * SC8547_VBUS_ADC_LSB;
}

/*********************************************************************/
static int sc8547_reg_reset(struct sc8547a_device *chip, bool enable)
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

static bool sc8547a_hw_version_check(struct sc8547a_device *chip)
{
	int ret;
	u8 val;
	ret = sc8547_read_byte(chip->client, SC8547_REG_36, &val);
	if (val == SC8547A_DEVICE_ID) {
		ic_sc8547a = true;
		return true;
	} else {
		ic_sc8547a = false;
		return false;
	}
}

static int sc8547_voocphy_get_chg_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_07, data);
	if (ret < 0) {
		chg_err("SC8547_REG_07\n");
		return -1;
	}

	*data = *data >> 7;

	return ret;
}

static int sc8547_voocphy_get_voocphy_enable(
	struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_2B, data);
	if (ret < 0) {
		chg_err("SC8547_REG_2B\n");
		return -1;
	}

	return ret;
}

static void sc8547_voocphy_dump_reg_in_err_issue(struct oplus_voocphy_manager *chip)
{
	int i = 0, p = 0;
	if (!chip) {
		chg_err("!!!!! oplus_voocphy_manager chip NULL");
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
	chg_err("p[%d], ", p);

	return;
}

static int sc8547_voocphy_get_adc_enable(struct oplus_voocphy_manager *chip, u8 *data)
{
	int ret = 0;

	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_11, data);
	if (ret < 0) {
		chg_err("SC8547_REG_11\n");
		return -1;
	}

	*data = *data >> 7;

	return ret;
}

static u8 sc8547_voocphy_get_int_value(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;
	u8 state = 0;

	if (!chip) {
		chg_err("%s: chip null\n", __func__);
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_0F, &data);
	if (ret < 0) {
		chg_err(" read SC8547_REG_0F failed\n");
		return -1;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_06, &state);
	if (ret < 0) {
		chg_err(" read SC8547_REG_06 failed\n");
		return -1;
	}
	chg_info("REG06 = 0x%x REG0F = 0x%x\n", state, data);

	return data;
}

static int sc8547_voocphy_set_chg_enable(struct oplus_voocphy_manager *chip,
				 bool enable)
{
	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}
	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
}

static int sc8547_set_chg_enable(struct sc8547a_device *chip, bool enable)
{
	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
}

static void sc8547_voocphy_set_pd_svooc_config(
	struct oplus_voocphy_manager *chip, bool enable)
{
	int ret = 0;
	u8 reg_data = 0;
	if (!chip) {
		chg_err("Failed\n");
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
		chg_err("SC8547_REG_05\n");
		return;
	}
	chg_err("pd_svooc config SC8547_REG_05 = %d\n", reg_data);
}

static bool sc8547_voocphy_get_pd_svooc_config(struct oplus_voocphy_manager *chip)
{
	int ret = 0;
	u8 data = 0;

	if (!chip) {
		chg_err("Failed\n");
		return false;
	}

	ret = sc8547_read_byte(chip->client, SC8547_REG_05, &data);
	if (ret < 0) {
		chg_err("SC8547_REG_05\n");
		return false;
	}

	chg_err("SC8547_REG_05 = 0x%0x\n", data);

	data = data >> 7;
	if (data == 1)
		return true;
	else
		return false;
}

static int sc8547_voocphy_set_adc_enable(struct oplus_voocphy_manager *chip,
				 bool enable)
{
	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}

	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x00);
}

static int sc8547_set_adc_enable(struct sc8547a_device *chip, bool enable)
{
	if (!chip) {
		chg_err("Failed\n");
		return -1;
	}

	if (enable)
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x80);
	else
		return sc8547_write_byte(chip->client, SC8547_REG_11, 0x00);
}

static void sc8547_voocphy_send_handshake(struct oplus_voocphy_manager *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x81);
}

static int sc8547_voocphy_reset_voocphy(struct oplus_voocphy_manager *chip)
{
	u8 reg_data;

	/* turn off mos */
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	if(ic_sc8547a) {
		/* sc8547b need disable WDT when exit charging, to avoid after WDT time out.
		IF time out, sc8547b will trigger interrupt frequently.
		in addition, sc8547 and sc8547b WDT will disable when disable CP */
		sc8547_update_bits(chip->client, SC8547_REG_09,
				   SC8547_WATCHDOG_MASK, SC8547_WATCHDOG_DIS);
	}
	/* hwic config with plugout */
	reg_data = 0x20 | (chip->ovp_reg & 0x1f);
	sc8547_write_byte(chip->client, SC8547_REG_00, reg_data);
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x07);
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
	chg_err("oplus_vooc_reset_voocphy done");

	return VOOCPHY_SUCCESS;
}

static int sc8547_voocphy_reactive_voocphy(struct oplus_voocphy_manager *chip)
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
	sc8547_voocphy_send_handshake(chip);

	chg_info("oplus_vooc_reactive_voocphy done");

	return VOOCPHY_SUCCESS;
}

static int sc8547_init_device(struct sc8547a_device *chip)
{
	u8 reg_data;

	sc8547_write_byte(chip->client, SC8547_REG_11, 0x0); /* ADC_CTRL:disable */
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x7);
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /* VBUS_OVP:10 2:1 or 1:1V */
	reg_data = 0x20 | (chip->ovp_reg & 0x1f);
	sc8547_write_byte(chip->client, SC8547_REG_00, reg_data); /* VBAT_OVP:4.65V */
	reg_data = 0x10 | (chip->ocp_reg & 0xf);
	sc8547_write_byte(chip->client, SC8547_REG_05, reg_data); /* IBUS_OCP_UCP:3.6A */
	sc8547_write_byte(chip->client, SC8547_REG_01, 0xbf);
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00); /* OOC_CTRL:disable */
	sc8547_write_byte(chip->client, SC8547_REG_3A, 0x60);
	sc8547_update_bits(chip->client, SC8547_REG_09, SC8547_IBUS_UCP_RISE_MASK_MASK,
			   (1 << SC8547_IBUS_UCP_RISE_MASK_SHIFT));
	sc8547_write_byte(chip->client, SC8547_REG_10, 0x02); /* mask insert irq */
	sc8547_write_byte(chip->client, SC8547A_ADDR_OTG_EN, 0x04); /* OTG EN */

	return 0;
}

static int sc8547_voocphy_init_vooc(struct oplus_voocphy_manager *voocphy)
{
	u8 value;
	struct sc8547a_device *chip;

	chg_info(" >>>>start init vooc\n");
	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8547a chip is NULL\n");
		return -ENODEV;
	}

	sc8547_reg_reset(chip, true);
	sc8547_init_device(chip);

	/*to avoid cmd of adjust current(0x01)return error, add voocphy bit0 hold time to 800us */
	/*SET PREDATA */
	sc8547_write_word(chip->client, SC8547_REG_31, 0x0);
	/*sc8547_set_predata(0x0);*/
	sc8547_read_byte(chip->client, SC8547_REG_3A, &value);
	value = value | (1 << 6);
	sc8547_write_byte(chip->client, SC8547_REG_3A, value);
	chg_err("read value %d\n", value);

	/*dpdm */
	sc8547_write_byte(chip->client, SC8547_REG_21, 0x21);
	sc8547_write_byte(chip->client, SC8547_REG_22, 0x00);
	sc8547_write_byte(chip->client, SC8547_REG_33, 0xD1);

	/*vooc */
	sc8547_write_byte(chip->client, SC8547_REG_30, 0x05);

	return 0;
}

static int sc8547_svooc_hw_setting(struct sc8547a_device *chip)
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

static int sc8547_vooc_hw_setting(struct sc8547a_device *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x07); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_05, 0x1c); /*IBUS_OCP_UCP:*/
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x93); /*WD:1000ms*/
	sc8547_write_byte(chip->client, SC8547_REG_11, 0x80); /*ADC_CTRL:*/
	sc8547_write_byte(chip->client, SC8547_REG_33, 0xd1); /*Loose_det*/
	sc8547_write_byte(chip->client, SC8547_REG_3A, 0x60);
	return 0;
}

static int sc8547_5v2a_hw_setting(struct sc8547a_device *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x07); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x10); /*WD:DISABLE*/

	sc8547_write_byte(chip->client, SC8547_REG_11, 0x00); /*ADC_CTRL:*/
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00); /*VOOC_CTRL*/
	return 0;
}

static int sc8547_pdqc_hw_setting(struct sc8547a_device *chip)
{
	sc8547_write_byte(chip->client, SC8547_REG_00, 0x2E); /*VAC_OV:*/
	sc8547_write_byte(chip->client, SC8547_REG_02, 0x01); /*VAC_OVP:*/
	sc8547_write_byte(chip->client, SC8547_REG_04, 0x50); /*VBUS_OVP*/
	sc8547_write_byte(chip->client, SC8547_REG_07, 0x0);
	sc8547_write_byte(chip->client, SC8547_REG_09, 0x10); /*WD:DISABLE*/
	sc8547_write_byte(chip->client, SC8547_REG_11, 0x00); /*ADC_CTRL*/
	sc8547_write_byte(chip->client, SC8547_REG_2B, 0x00); /*VOOC_CTRL*/
	chg_err("sc8547_pdqc_hw_setting done");
	return 0;
}

static int sc8547_voocphy_hw_setting(struct oplus_voocphy_manager *voocphy, int reason)
{
	struct sc8547a_device *chip;

	if (!voocphy) {
		chg_err("voocphy is null exit\n");
		return -EINVAL;
	}

	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8547a chip is NULL\n");
		return -ENODEV;
	}

	switch (reason) {
	case SETTING_REASON_PROBE:
	case SETTING_REASON_RESET:
		sc8547_init_device(chip);
		chg_info("SETTING_REASON_RESET OR PROBE\n");
		break;
	case SETTING_REASON_SVOOC:
		sc8547_svooc_hw_setting(chip);
		chg_info("SETTING_REASON_SVOOC\n");
		break;
	case SETTING_REASON_VOOC:
		sc8547_vooc_hw_setting(chip);
		chg_info("SETTING_REASON_VOOC\n");
		break;
	case SETTING_REASON_5V2A:
		sc8547_5v2a_hw_setting(chip);
		chg_info("SETTING_REASON_5V2A\n");
		break;
	case SETTING_REASON_PDQC:
		sc8547_pdqc_hw_setting(chip);
		chg_info("SETTING_REASON_PDQC\n");
		break;
	default:
		chg_err("do nothing\n");
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

static void sc8547_hardware_init(struct sc8547a_device *chip)
{
	sc8547_reg_reset(chip, true);
	sc8547_init_device(chip);
}

static void sc8547_voocphy_hardware_init(struct oplus_voocphy_manager *voocphy)
{
	struct sc8547a_device *chip;

	chip = voocphy->priv_data;
	if (chip == NULL) {
		chg_err("sc8547a chip is NULL\n");
		return;
	}

	sc8547_reg_reset(chip, true);
	sc8547_init_device(chip);
}

static bool sc8547_voocphy_check_cp_int_happened(
	struct oplus_voocphy_manager *chip, bool *dump_reg, bool *send_info)
{
	int i = 0;

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

static int sc8547a_retrieve_reg_flags(struct sc8547a_device *chip)
{
	unsigned int err_flag = 0;
	int rc = 0;
	u8 flag_buf[SC8547A_FLAG_NUM] = { 0 };

	rc = sc8547a_read_data(chip, SC8547A_ADDR_GENERAL_INT_FLAG1, flag_buf,
			       SC8547A_FLAG_NUM);
	if (rc < 0) {
		chg_err("failed to read flag register\n");
		return -EBUSY;
	}
	memcpy(chip->ufcs_reg_dump, flag_buf, SC8547A_FLAG_NUM);

	if (flag_buf[1] & SC8547A_FLAG_ACK_RECEIVE_TIMEOUT)
		err_flag |= BIT(UFCS_RECV_ERR_ACK_TIMEOUT);
	if (flag_buf[1] & SC8547A_FLAG_HARD_RESET)
		err_flag |= BIT(UFCS_HW_ERR_HARD_RESET);
	if (flag_buf[1] & SC8547A_FLAG_DATA_READY)
		err_flag |= BIT(UFCS_RECV_ERR_DATA_READY);
	if (flag_buf[1] & SC8547A_FLAG_SENT_PACKET_COMPLETE)
		err_flag |= BIT(UFCS_RECV_ERR_SENT_CMP);
	if (flag_buf[1] & SC8547A_FLAG_CRC_ERROR)
		err_flag |= BIT(UFCS_COMM_ERR_CRC_ERR);
	if (flag_buf[1] & SC8547A_FLAG_BAUD_RATE_ERROR)
		err_flag |= BIT(UFCS_COMM_ERR_BAUD_RATE_ERR);

	if (flag_buf[2] & SC8547A_FLAG_TRAINING_BYTE_ERROR)
		err_flag |= BIT(UFCS_COMM_ERR_TRAINING_ERR);
	if (flag_buf[2] & SC8547A_FLAG_MSG_TRANS_FAIL)
		err_flag |= BIT(UFCS_RECV_ERR_TRANS_FAIL);
	if (flag_buf[2] & SC8547A_FLAG_DATA_BYTE_TIMEOUT)
		err_flag |= BIT(UFCS_COMM_ERR_BYTE_TIMEOUT);
	if (flag_buf[2] & SC8547A_FLAG_BAUD_RATE_CHANGE)
		err_flag |= BIT(UFCS_COMM_ERR_BAUD_RATE_CHANGE);
	if (flag_buf[2] & SC8547A_FLAG_LENGTH_ERROR)
		err_flag |= BIT(UFCS_COMM_ERR_RX_LEN_ERR);
	if (flag_buf[2] & SC8547A_FLAG_RX_OVERFLOW)
		err_flag |= BIT(UFCS_COMM_ERR_RX_OVERFLOW);
	if (flag_buf[2] & SC8547A_FLAG_BUS_CONFLICT)
		err_flag |= BIT(UFCS_COMM_ERR_BUS_CONFLICT);
	chip->ufcs->dev_err_flag |= err_flag;

	if (chip->ufcs->handshake_state == UFCS_HS_WAIT) {
		if ((flag_buf[1] & SC8547A_FLAG_HANDSHAKE_SUCCESS) &&
		    !(flag_buf[1] & SC8547A_FLAG_HANDSHAKE_FAIL)) {
			chip->ufcs->handshake_state = UFCS_HS_SUCCESS;
		 } else if (flag_buf[1] & SC8547A_FLAG_HANDSHAKE_FAIL) {
			chip->ufcs->handshake_state = UFCS_HS_FAIL;
		 }
	}
	chg_info("[0x%x, 0x%x, 0x%x], err_flag=0x%x\n", flag_buf[0], flag_buf[1], flag_buf[2],
		 chip->ufcs->dev_err_flag);

	return 0;
}

static int sc8547a_ufcs_init(struct ufcs_dev *ufcs)
{
	return 0;
}

static int sc8547a_ufcs_write_msg(struct ufcs_dev *ufcs, unsigned char *buf, int len)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	int rc;

	rc = sc8547a_write_byte(chip, SC8547A_ADDR_TX_LENGTH, len);
	if (rc < 0) {
		chg_err("write tx buf len error, rc=%d\n", rc);
		return rc;
	}
	rc = sc8547a_write_data(chip, SC8547A_ADDR_TX_BUFFER0, len, buf);
	if (rc < 0) {
		chg_err("write tx buf error, rc=%d\n", rc);
		return rc;
	}
	rc = sc8547a_write_bit_mask(chip, SC8547A_ADDR_UFCS_CTRL0,
		SC8547A_MASK_SND_CMP, SC8547A_CMD_SND_CMP);
	if (rc < 0) {
		chg_err("write tx buf send cmd error, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int sc8547a_ufcs_read_msg(struct ufcs_dev *ufcs, unsigned char *buf, int len)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	u8 rx_buf_len;
	int rc;

	rc = sc8547a_read_byte(chip, SC8547A_ADDR_RX_LENGTH, &rx_buf_len);
	if (rc < 0) {
		chg_err("can't read rx buf len, rc=%d\n", rc);
		return rc;
	}
	if (rx_buf_len > len) {
		chg_err("rx_buf_len = %d, limit to %d\n", rx_buf_len, len);
		rx_buf_len = len;
	}

	rc = sc8547a_read_data(chip, SC8547A_ADDR_RX_BUFFER0, buf, rx_buf_len);
	if (rc < 0) {
		chg_err("can't read rx buf, rc=%d\n", rc);
		return rc;
	}

	return (int)rx_buf_len;
}

static int sc8547a_ufcs_handshake(struct ufcs_dev *ufcs)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	int rc;

	chg_info("ufcs handshake\n");
	rc = sc8547a_write_bit_mask(chip, SC8547A_ADDR_UFCS_CTRL0,
				    SC8547A_MASK_EN_HANDSHAKE,
				    SC8547A_CMD_EN_HANDSHAKE);
	if (rc < 0)
		chg_err("send handshake error, rc=%d\n", rc);

	return rc;
}

static int sc8547a_ufcs_source_hard_reset(struct ufcs_dev *ufcs)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	int rc;
	int retry_count = 0;

retry:
	retry_count++;
	if (retry_count > UFCS_HARDRESET_RETRY_CNTS) {
		chg_err("send hard reset, retry count over!\n");
		return -EBUSY;
	}

	rc = sc8547a_write_bit_mask(chip, SC8547A_ADDR_UFCS_CTRL0,
				SEND_SOURCE_HARDRESET,
				SEND_SOURCE_HARDRESET);
	if (rc < 0) {
		chg_err("I2c send handshake error\n");
		goto retry;
	}

	msleep(100);
	return 0;
}

static int sc8547a_ufcs_cable_hard_reset(struct ufcs_dev *ufcs)
{
	return 0;
}

static int sc8547a_ufcs_set_baud_rate(struct ufcs_dev *ufcs, enum ufcs_baud_rate baud)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	int rc;

	rc = sc8547a_write_bit_mask(chip, SC8547A_ADDR_UFCS_CTRL0,
				FLAG_BAUD_RATE_VALUE,
				(baud << FLAG_BAUD_NUM_SHIFT));
	if (rc < 0)
		chg_err("set baud rate error, rc=%d\n", rc);

	return rc;
}

static int sc8547a_ufcs_enable(struct ufcs_dev *ufcs)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	u8 addr_buf[SC8547A_ENABLE_REG_NUM] = { SC8547A_ADDR_DPDM_CTRL,
						SC8547A_ADDR_UFCS_CTRL0,
						SC8547A_ADDR_TXRX_BUFFER_CTRL,
						SC8547A_ADDR_GENERAL_INT_FLAG1,
						SC8547A_ADDR_UFCS_INT_MASK0,
						SC8547A_ADDR_UFCS_INT_MASK1 };
	u8 cmd_buf[SC8547A_ENABLE_REG_NUM] = {
		SC8547A_CMD_DPDM_EN,	  SC8547A_CMD_EN_CHIP,
		SC8547A_CMD_CLR_TX_RX,	SC8547A_CMD_MASK_ACK_DISCARD,
		SC8547A_CMD_MASK_ACK_TIMEOUT, SC8547A_MASK_TRANING_BYTE_ERROR
	};
	int i, rc;

	chip->rested = true;
	sc8547_reg_reset(chip, true);
	msleep(10);
	sc8547_init_device(chip);
	for (i = 0; i < SC8547A_ENABLE_REG_NUM; i++) {
		rc = sc8547a_write_byte(chip, addr_buf[i], cmd_buf[i]);
		if (rc < 0) {
			chg_err("write i2c failed!\n");
			return rc;
		}
	}
	chip->ufcs_enable = true;

	return 0;
}

static int sc8547a_ufcs_disable(struct ufcs_dev *ufcs)
{
	struct sc8547a_device *chip = ufcs->drv_data;
	int rc;

	chip->ufcs_enable = false;
	chip->rested = false;
	rc = sc8547a_write_byte(chip, SC8547A_ADDR_UFCS_CTRL0,
				SC8547A_CMD_DIS_CHIP);
	if (rc < 0) {
		chg_err("write i2c failed\n");
		return rc;
	}

	return 0;
}

static void sc8547_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
}

static struct oplus_voocphy_operations oplus_sc8547_ops = {
	.hardware_init = sc8547_voocphy_hardware_init,
	.hw_setting = sc8547_voocphy_hw_setting,
	.init_vooc = sc8547_voocphy_init_vooc,
	.set_predata = sc8547_voocphy_set_predata,
	.set_txbuff = sc8547_voocphy_set_txbuff,
	.get_adapter_info = sc8547_voocphy_get_adapter_info,
	.update_data = sc8547_voocphy_update_data,
	.get_chg_enable = sc8547_voocphy_get_chg_enable,
	.set_chg_enable = sc8547_voocphy_set_chg_enable,
	.reset_voocphy = sc8547_voocphy_reset_voocphy,
	.reactive_voocphy = sc8547_voocphy_reactive_voocphy,
	.send_handshake = sc8547_voocphy_send_handshake,
	.get_cp_vbat = sc8547_voocphy_get_cp_vbat,
	.get_cp_vbus = sc8547_voocphy_get_cp_vbus,
	.get_int_value = sc8547_voocphy_get_int_value,
	.get_adc_enable = sc8547_voocphy_get_adc_enable,
	.set_adc_enable = sc8547_voocphy_set_adc_enable,
	.get_ichg = sc8547_voocphy_get_cp_ichg,
	.set_pd_svooc_config = sc8547_voocphy_set_pd_svooc_config,
	.get_pd_svooc_config = sc8547_voocphy_get_pd_svooc_config,
	.get_voocphy_enable = sc8547_voocphy_get_voocphy_enable,
	.dump_voocphy_reg = sc8547_voocphy_dump_reg_in_err_issue,
	.check_cp_int_happened = sc8547_voocphy_check_cp_int_happened,
	.dual_chan_buck_set_ucp = sc8547_dual_chan_buck_set_ucp,
};

static struct ufcs_dev_ops ufcs_ops = {
	.init = sc8547a_ufcs_init,
	.write_msg = sc8547a_ufcs_write_msg,
	.read_msg = sc8547a_ufcs_read_msg,
	.handshake = sc8547a_ufcs_handshake,
	.source_hard_reset = sc8547a_ufcs_source_hard_reset,
	.cable_hard_reset = sc8547a_ufcs_cable_hard_reset,
	.set_baud_rate = sc8547a_ufcs_set_baud_rate,
	.enable = sc8547a_ufcs_enable,
	.disable = sc8547a_ufcs_disable,
};

static int sc8547_charger_choose(struct sc8547a_device *chip)
{
	int ret;

	if (!oplus_voocphy_chip_is_null()) {
		chg_err("oplus_voocphy_chip already exists!");
		return 0;
	} else {
		ret = i2c_smbus_read_byte_data(chip->client, 0x07);
		chg_info("0x07 = %d\n", ret);
		if (ret < 0) {
			chg_err("i2c communication fail");
			return -EPROBE_DEFER;
		}

		return 1;
	}
}

static void sc8547a_ufcs_event_handler(struct sc8547a_device *chip)
{
	/* set awake */
	ufcs_clr_error_flag(chip->ufcs);
	sc8547a_retrieve_reg_flags(chip);
	ufcs_msg_handler(chip->ufcs);
}

static void sc8547a_check_fault_into(struct sc8547a_device *chip)
{
	int rc = 0;
	u8 buf[2] = { 0 };

	rc = sc8547a_read_data(chip, 0x0e, buf, 2);
	if (rc < 0) {
		chg_err("failed to read fault register\n");
		return;
	}
	chg_info("0x0e=0x%02x, 0x0f=0x%02x\n", buf[0], buf[1]);
}

static irqreturn_t sc8547_interrupt_handler(int irq, void *dev_id)
{
	struct sc8547a_device *chip = dev_id;
	struct oplus_voocphy_manager *voocphy = chip->voocphy;

	sc8547a_check_fault_into(chip);
	if (chip->use_ufcs_phy && chip->ufcs_enable) {
		sc8547a_ufcs_event_handler(chip);
		return IRQ_HANDLED;
	} else if (chip->use_vooc_phy) {
		return oplus_voocphy_interrupt_handler(voocphy);
	}

	return IRQ_HANDLED;
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

static int sc8547_irq_register(struct sc8547a_device *chip)
{
	struct oplus_voocphy_manager *voocphy = chip->voocphy;
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
					   "voocphy_irq", chip);
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
		free_irq(voocphy->irq, chip);
		chg_err("%s desc null\n", __func__);
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

static bool sc8547a_is_volatile_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static void sc8547a_ufcs_regdump_work(struct work_struct *work)
{
	struct sc8547a_device *chip =
		container_of(work, struct sc8547a_device, ufcs_regdump_work);
	struct mms_msg *topic_msg;
	char *buf;
	int rc;
	int i;
	size_t index = 0;

	buf = kzalloc(ERR_MSG_BUF, GFP_KERNEL);
	if (buf == NULL)
		return;

	for (i = 0; i < SC8547A_FLAG_NUM; i++)
		index += snprintf(buf + index, ERR_MSG_BUF, "0x%04x=%02x,",
			(SC8547A_ADDR_GENERAL_INT_FLAG1 + i), chip->ufcs_reg_dump[i]);
	if (index > 0)
		buf[index - 1] = 0;

	topic_msg =
		oplus_mms_alloc_str_msg(MSG_TYPE_ITEM, MSG_PRIO_HIGH, ERR_ITEM_IC,
					"[%s]-[%d]-[%d]:$$reg_info@@%s",
					"ufcs-sc8547a",
					OPLUS_IC_ERR_UFCS, UFCS_ERR_REG_DUMP,
					buf);
	kfree(buf);
	if (topic_msg == NULL) {
		chg_err("alloc topic msg error\n");
		return;
	}

	rc = oplus_mms_publish_msg(chip->err_topic, topic_msg);
	if (rc < 0) {
		chg_err("publish error topic msg error, rc=%d\n", rc);
		kfree(topic_msg);
	}
}

static void sc8547a_err_subs_callback(struct mms_subscribe *subs,
				     enum mms_msg_type type, u32 id)
{
	struct sc8547a_device *chip = subs->priv_data;

	switch (type) {
	case MSG_TYPE_ITEM:
		switch (id) {
		case ERR_ITEM_UFCS:
			schedule_work(&chip->ufcs_regdump_work);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void sc8547a_subscribe_error_topic(struct oplus_mms *topic, void *prv_data)
{
	struct sc8547a_device *chip = prv_data;

	chip->err_topic = topic;
	chip->err_subs =
		oplus_mms_subscribe(chip->err_topic, chip,
				    sc8547a_err_subs_callback, "sc8547a");
	if (IS_ERR_OR_NULL(chip->err_subs)) {
		chg_err("subscribe error topic error, rc=%ld\n",
			PTR_ERR(chip->err_subs));
		return;
	}
}

static struct regmap_config sc8547a_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = SC8547A_MAX_REG,
	.cache_type = REGCACHE_RBTREE,
	.volatile_reg = sc8547a_is_volatile_reg,
};

static struct ufcs_config sc8547a_ufcs_config = {
	.check_crc = false,
	.reply_ack = false,
	.msg_resend = false,
	.handshake_hard_retry = true,
	.ic_vendor_id = SC8547A_VENDOR_ID,
};

static bool sc8547a_check_work_mode_support(enum oplus_cp_work_mode mode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_cp_support_work_mode); i++) {
		if (g_cp_support_work_mode[i] == mode)
			return true;
	}
	return false;
}

static int sc8547a_cp_init(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	ic_dev->online = true;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_ONLINE);

	return 0;
}

static int sc8547a_cp_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	ic_dev->online = false;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_OFFLINE);

	return 0;
}

static int sc8547a_cp_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	struct sc8547a_device *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	sc8547_voocphy_dump_reg_in_err_issue(chip->voocphy);
	return 0;
}

static int sc8547a_cp_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static int sc8547a_cp_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	return 0;
}

static int sc8547a_cp_hw_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct sc8547a_device *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	if (chip->rested)
		return 0;

	sc8547_hardware_init(chip);
	return 0;
}

static int sc8547a_cp_set_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	struct sc8547a_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	if (!sc8547a_check_work_mode_support(mode)) {
		chg_err("not supported work mode, mode=%d\n", mode);
		return -EINVAL;
	}

	if (mode == CP_WORK_MODE_BYPASS)
		rc = sc8547_vooc_hw_setting(chip);
	else
		rc = sc8547_svooc_hw_setting(chip);

	if (rc < 0)
		chg_err("set work mode to %d error\n", mode);

	return rc;
}

static int sc8547a_cp_get_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode *mode)
{
	struct sc8547a_device *chip;
	u8 data;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8547_read_byte(chip->client, SC8547_REG_09, &data);
	if (rc < 0) {
		chg_err("read SC8547_REG_07 error, rc=%d\n", rc);
		return rc;
	}

	if (data & BIT(7))
		*mode = CP_WORK_MODE_BYPASS;
	else
		*mode = CP_WORK_MODE_2_TO_1;

	return 0;
}

static int sc8547a_cp_check_work_mode_support(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	return sc8547a_check_work_mode_support(mode);
}

static int sc8547a_cp_set_iin(struct oplus_chg_ic_dev *ic_dev, int iin)
{
	return 0;
}

static int sc8547a_cp_get_vin(struct oplus_chg_ic_dev *ic_dev, int *vin)
{
	struct sc8547a_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8547_get_cp_vbus(chip);
	if (rc < 0) {
		chg_err("can't get cp vin, rc=%d\n", rc);
		return rc;
	}
	*vin = rc;

	return 0;
}

static int sc8547a_cp_get_iin(struct oplus_chg_ic_dev *ic_dev, int *iin)
{
	struct sc8547a_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8547_get_cp_ichg(chip);
	if (rc < 0) {
		chg_err("can't get cp iin, rc=%d\n", rc);
		return rc;
	}
	*iin = rc;

	return 0;
}

static int sc8547a_cp_get_vout(struct oplus_chg_ic_dev *ic_dev, int *vout)
{
	struct sc8547a_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8547_get_cp_vbat(chip);
	if (rc < 0) {
		chg_err("can't get cp vout, rc=%d\n", rc);
		return rc;
	}
	*vout = rc;

	return 0;
}

static int sc8547a_cp_get_iout(struct oplus_chg_ic_dev *ic_dev, int *iout)
{
	struct sc8547a_device *chip;
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

static int sc8547a_cp_get_vac(struct oplus_chg_ic_dev *ic_dev, int *vac)
{
	struct sc8547a_device *chip;
	u8 data_block[2] = { 0 };
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);
	if (!chip->vac_support)
		return -ENOTSUPP;

	rc = i2c_smbus_read_i2c_block_data(chip->client, SC8547_REG_17, 2, data_block);
	if (rc < 0) {
		sc8547_i2c_error(chip->voocphy, true, true);
		chg_err("sc8547 read vac error, rc=%d\n", rc);
		return rc;
	} else {
		sc8547_i2c_error(chip->voocphy, false, true);
	}

	*vac = (((data_block[0] & SC8547_VAC_POL_H_MASK) << 8) | data_block[1]) * SC8547_VAC_ADC_LSB;

	return 0;
}

static int sc8547a_cp_set_work_start(struct oplus_chg_ic_dev *ic_dev, bool start)
{
	struct sc8547a_device *chip;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	chg_info("%s work %s\n", chip->dev->of_node->name, start ? "start" : "stop");

	rc = sc8547_set_chg_enable(chip, start);
	if (rc < 0)
		return rc;
	oplus_imp_node_set_active(chip->input_imp_node, start);
	oplus_imp_node_set_active(chip->output_imp_node, start);

	return 0;
}

static int sc8547a_cp_get_work_status(struct oplus_chg_ic_dev *ic_dev, bool *start)
{
	struct sc8547a_device *chip;
	u8 data;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	rc = sc8547_read_byte(chip->client, SC8547_REG_07, &data);
	if (rc < 0) {
		chg_err("read SC8547_REG_07 error, rc=%d\n", rc);
		return rc;
	}

	*start = data & BIT(7);

	return 0;
}

static int sc8547a_cp_adc_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	struct sc8547a_device *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_priv_data(ic_dev);

	return sc8547_set_adc_enable(chip, en);

	return 0;
}

static void *sc8547a_cp_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
{
	void *func = NULL;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT)) {
		chg_err("%s is offline\n", ic_dev->name);
		return NULL;
	}

	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT, sc8547a_cp_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT, sc8547a_cp_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP, sc8547a_cp_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST, sc8547a_cp_smt_test);
		break;
	case OPLUS_IC_FUNC_CP_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_ENABLE, sc8547a_cp_enable);
		break;
	case OPLUS_IC_FUNC_CP_HW_INTI:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_HW_INTI, sc8547a_cp_hw_init);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_MODE, sc8547a_cp_set_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_MODE, sc8547a_cp_get_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT,
			sc8547a_cp_check_work_mode_support);
		break;
	case OPLUS_IC_FUNC_CP_SET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_IIN, sc8547a_cp_set_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VIN, sc8547a_cp_get_vin);
		break;
	case OPLUS_IC_FUNC_CP_GET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IIN, sc8547a_cp_get_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VOUT, sc8547a_cp_get_vout);
		break;
	case OPLUS_IC_FUNC_CP_GET_IOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IOUT, sc8547a_cp_get_iout);
		break;
	case OPLUS_IC_FUNC_CP_GET_VAC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VAC, sc8547a_cp_get_vac);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_START:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_START, sc8547a_cp_set_work_start);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_STATUS, sc8547a_cp_get_work_status);
		break;
	case OPLUS_IC_FUNC_CP_SET_ADC_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_ADC_ENABLE, sc8547a_cp_adc_enable);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

struct oplus_chg_ic_virq sc8547a_cp_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static int sc8547a_get_input_node_impedance(void *data)
{
	struct sc8547a_device *chip;
	int vac, vin, iin;
	int r_mohm;
	int rc;

	if (data == NULL)
		return -EINVAL;
	chip = data;

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VIN, &vin);
	if (rc < 0) {
		chg_err("can't read cp vin, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IIN, &iin);
	if (rc < 0) {
		chg_err("can't read cp iin, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VAC, &vac);
	if (rc < 0 && rc != -ENOTSUPP) {
		chg_err("can't read cp vac, rc=%d\n", rc);
		return rc;
	} else if (rc == -ENOTSUPP) {
		/* If the current IC does not support it, try to get it from the parent IC */
		rc = oplus_chg_ic_func(chip->cp_ic->parent, OPLUS_IC_FUNC_CP_GET_VAC, &vac);
		if (rc < 0) {
			chg_err("can't read parent cp vac, rc=%d\n", rc);
			return rc;
		}
	}

	r_mohm = (vac - vin) * 1000 / iin;
	if (r_mohm < 0) {
		chg_err("input_node: r_mohm=%d\n", r_mohm);
		r_mohm = 0;
	}

	return r_mohm;
}

static int sc8547a_get_output_node_impedance(void *data)
{
	struct sc8547a_device *chip;
	struct oplus_mms *gauge_topic;
	union mms_msg_data mms_data = { 0 };
	int vout, iout, vbat;
	int r_mohm;
	int rc;

	if (data == NULL)
		return -EINVAL;
	chip = data;

	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_VOUT, &vout);
	if (rc < 0) {
		chg_err("can't read cp vout, rc=%d\n", rc);
		return rc;
	}
	rc = oplus_chg_ic_func(chip->cp_ic, OPLUS_IC_FUNC_CP_GET_IOUT, &iout);
	if (rc < 0) {
		chg_err("can't read cp iout, rc=%d\n", rc);
		return rc;
	}

	gauge_topic = oplus_mms_get_by_name("gauge");
	if (gauge_topic == NULL) {
		chg_err("gauge topic not found\n");
		return -ENODEV;
	}
	rc = oplus_mms_get_item_data(gauge_topic, GAUGE_ITEM_VOL_MAX, &mms_data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}
	vbat = mms_data.intval;

	r_mohm = (vout - vbat * oplus_gauge_get_batt_num()) * 1000 / iout;
	if (r_mohm < 0) {
		chg_err("output_node: r_mohm=%d\n", r_mohm);
		r_mohm = 0;
	}

	return r_mohm;
}

static int sc8547a_init_imp_node(struct sc8547a_device *chip, struct device_node *of_node)
{
	struct device_node *imp_node;
	struct device_node *child;
	const char *name;
	int rc;

	imp_node = of_get_child_by_name(of_node, "oplus,impedance_node");
	if (imp_node == NULL)
		return 0;

	for_each_child_of_node(imp_node, child) {
		rc = of_property_read_string(child, "node_name", &name);
		if (rc < 0) {
			chg_err("can't read %s node_name, rc=%d\n", child->name, rc);
			continue;
		}
		if (!strcmp(name, "cp_input")) {
			chip->input_imp_node =
				oplus_imp_node_register(child, chip->dev, chip, sc8547a_get_input_node_impedance);
			if (IS_ERR_OR_NULL(chip->input_imp_node)) {
				chg_err("%s register error, rc=%ld\n", child->name, PTR_ERR(chip->input_imp_node));
				chip->input_imp_node = NULL;
				continue;
			}
		} else if (!strcmp(name, "cp_output")) {
			chip->output_imp_node =
				oplus_imp_node_register(child, chip->dev, chip, sc8547a_get_output_node_impedance);
			if (IS_ERR_OR_NULL(chip->output_imp_node)) {
				chg_err("%s register error, rc=%ld\n", child->name, PTR_ERR(chip->output_imp_node));
				chip->output_imp_node = NULL;
				continue;
			}
		} else {
			chg_err("unknown node_name: %s\n", name);
		}
	}

	return 0;
}

static int sc8547a_ic_register(struct sc8547a_device *chip)
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
			(void)sc8547a_init_imp_node(chip, child);
			snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "cp-sc8547a:%d", ic_index);
			snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
			ic_cfg.get_func = sc8547a_cp_get_func;
			ic_cfg.virq_data = sc8547a_cp_virq_table;
			ic_cfg.virq_num = ARRAY_SIZE(sc8547a_cp_virq_table);
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

static int sc8547a_parse_dt(struct sc8547a_device *chip)
{
	int rc;
	struct device_node *node = chip->dev->of_node;

	rc = of_property_read_u32(node, "ovp_reg", &chip->ovp_reg);
	if (rc)
		chip->ovp_reg = 0xe;
	chg_err("ovp_reg=0x%2x\n", chip->ovp_reg);

	rc = of_property_read_u32(node, "ocp_reg", &chip->ocp_reg);
	if (rc)
		chip->ocp_reg = 0x8;
	chg_err("ocp_reg=0x%2x\n", chip->ocp_reg);

	return 0;
}

static int sc8547a_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct sc8547a_device *chip;
	struct oplus_voocphy_manager *voocphy;
	int rc;

	chip = devm_kzalloc(&client->dev, sizeof(struct sc8547a_device), GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc sc8547a device buf error\n");
		return -ENOMEM;
	}

	voocphy = devm_kzalloc(&client->dev, sizeof(struct oplus_voocphy_manager), GFP_KERNEL);
	if (voocphy == NULL) {
		chg_err("alloc voocphy buf error\n");
		rc = -ENOMEM;
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
	INIT_WORK(&chip->ufcs_regdump_work, sc8547a_ufcs_regdump_work);

	rc = sc8547a_parse_dt(chip);
	if (rc < 0)
		goto parse_dt_err;

	chip->regmap = devm_regmap_init_i2c(client, &sc8547a_regmap_config);
	if (chip->regmap == NULL) {
		rc = -ENODEV;
		goto regmap_init_err;
	}
	sc8547a_awake_init(chip);

	rc = sc8547_charger_choose(chip);
	if (rc <= 0) {
		chg_err("choose error, rc=%d\n", rc);
		goto regmap_init_err;
	}

	sc8547_create_device_node(&(client->dev));

	chip->use_vooc_phy = of_property_read_bool(chip->dev->of_node, "oplus,use_vooc_phy");
	chip->use_ufcs_phy = of_property_read_bool(chip->dev->of_node, "oplus,use_ufcs_phy");
	chip->vac_support = of_property_read_bool(chip->dev->of_node, "oplus,vac_support");
	chg_info("use_vooc_phy=%d, use_ufcs_phy=%d, vac_support=%d\n",
		 chip->use_vooc_phy, chip->use_ufcs_phy, chip->vac_support);

	if (chip->use_vooc_phy) {
		voocphy->ops = &oplus_sc8547_ops;

		rc = oplus_register_voocphy(voocphy);
		if (rc < 0) {
			chg_err("failed to register voocphy, ret = %d", rc);
			goto reg_voocphy_err;
		}
	}

	if (chip->use_ufcs_phy) {
		if (!sc8547a_hw_version_check(chip)) {
			chg_err("sc8547 dont use ufcs\n");
			chip->use_ufcs_phy = false;
			goto skip_ufcs_reg;
		}
		chip->ufcs = ufcs_device_register(chip->dev, &ufcs_ops, chip, &sc8547a_ufcs_config);
		if (IS_ERR_OR_NULL(chip->ufcs)) {
			chg_err("ufcs device register error\n");
			rc = -ENODEV;
			goto reg_ufcs_err;
		}
	}

skip_ufcs_reg:
	rc = sc8547_irq_register(chip);
	if (rc < 0) {
		if (chip->use_vooc_phy || chip->use_ufcs_phy) {
			chg_err("irq register error\n");
			goto irq_reg_err;
		}
	}

	rc = sc8547a_ic_register(chip);
	if (rc < 0) {
		chg_err("cp ic register error\n");
		goto cp_reg_err;
	}

	chip->ufcs_enable = false;

	sc8547a_cp_init(chip->cp_ic);
	oplus_mms_wait_topic("error", sc8547a_subscribe_error_topic, chip);

	chg_info("sc8547a(%s) probe successfully\n", chip->dev->of_node->name);

	return 0;

cp_reg_err:
	if (chip->input_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->input_imp_node);
	if (chip->output_imp_node != NULL)
		oplus_imp_node_unregister(chip->dev, chip->output_imp_node);
irq_reg_err:
	if (chip->use_ufcs_phy)
		ufcs_device_unregister(chip->ufcs);
reg_ufcs_err:
reg_voocphy_err:
regmap_init_err:
parse_dt_err:
	devm_kfree(&client->dev, voocphy);
chg_err:
	devm_kfree(&client->dev, chip);
	return rc;
}

static void sc8547a_shutdown(struct i2c_client *client)
{
	sc8547_write_byte(client, SC8547_REG_11, 0x00);
	sc8547_write_byte(client, SC8547_REG_21, 0x00);
	return;
}

static const struct of_device_id sc8547a_match[] = {
	{.compatible = "oplus,sc8547a" },
	{},
};

static const struct i2c_device_id sc8547a_id[] = {
	{ "oplus,sc8547a", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sc8547a_id);

static struct i2c_driver sc8547a_i2c_driver = {
	.driver =
		{
			.name = "sc8547a",
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
