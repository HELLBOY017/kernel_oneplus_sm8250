// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[P9415]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/sched/clock.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_hal_wls.h>
#include <oplus_chg_wls.h>
#include "oplus_hal_p9415_fw.h"
#include "oplus_hal_p9415.h"
#include "monitor/oplus_chg_track.h"

#ifndef I2C_ERR_MAX
#define I2C_ERR_MAX 6
#endif

#define LDO_ON_MV	4200

#define P9415_CHECK_LDO_ON_DELAY round_jiffies_relative(msecs_to_jiffies(2000))

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#ifdef MODULE
#undef printk_ratelimit
#define printk_ratelimit() 0
#endif
#endif

enum wls_ic_err_reason {
	WLS_IC_ERR_NONE,
	WLS_IC_ERR_RXAC,
	WLS_IC_ERR_OCP,
	WLS_IC_ERR_OVP,
	WLS_IC_ERR_LVP,
	WLS_IC_ERR_FOD,
	WLS_IC_ERR_RXEPT,
	WLS_IC_ERR_OTP,
	WLS_IC_ERR_I2C,
	WLS_IC_ERR_CRC,
	WLS_IC_ERR_OTHER,
};

static const char * const wls_ic_err_reason_text[] = {
	[WLS_IC_ERR_NONE] = "none",
	[WLS_IC_ERR_RXAC] = "rxac",
	[WLS_IC_ERR_OCP] = "ocp",
	[WLS_IC_ERR_OVP] = "ovp",
	[WLS_IC_ERR_LVP] = "lvp",
	[WLS_IC_ERR_FOD] = "fod",
	[WLS_IC_ERR_RXEPT] = "rxept",
	[WLS_IC_ERR_OTP] = "otp",
	[WLS_IC_ERR_I2C] = "i2c_err",
	[WLS_IC_ERR_CRC] = "crc_err",
	[WLS_IC_ERR_OTHER] = "other",
};

struct rx_msg_struct {
	void (*msg_call_back)(void *dev_data, u8 data[]);
	void *dev_data;
};

struct oplus_p9415 {
	struct i2c_client *client;
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	struct oplus_chg_ic_dev *nor_ic;
	struct oplus_mms *err_topic;

	int rx_con_gpio;
	int rx_con_irq;
	int rx_event_gpio;
	int rx_event_irq;
	int rx_en_gpio;
	int mode_sw_gpio;

	bool fw_updating;
	bool check_fw_update;
	bool connected_ldo_on;

	bool rx_connected;
	int adapter_type;
	int rx_pwr_cap;
	bool support_epp_11w;

	unsigned char *fw_data;
	int fw_length;
	u32 fw_boot_version;
	u32 fw_rx_version;
	u32 fw_tx_version;
	int event_code;

	struct mutex i2c_lock;

	struct pinctrl *pinctrl;
	struct pinctrl_state *rx_con_default;
	struct pinctrl_state *rx_event_default;
	struct pinctrl_state *rx_en_active;
	struct pinctrl_state *rx_en_sleep;
	struct pinctrl_state *mode_sw_active;
	struct pinctrl_state *mode_sw_sleep;

	struct delayed_work update_work;
	struct delayed_work event_work;
	struct delayed_work connect_work;
	struct delayed_work check_ldo_on_work;
	struct delayed_work check_event_work;

	struct regmap *regmap;
	struct rx_msg_struct rx_msg;
	struct completion ldo_on;

	atomic_t i2c_err_count;

	u32 debug_force_ic_err;
};

static struct oplus_p9415 *g_p9415_chip = NULL;

__maybe_unused static bool is_nor_ic_available(struct oplus_p9415 *chip)
{
	struct device_node *node = NULL;

	if (!chip) {
		chg_err("chip is NULL!\n");
		return false;
	}
	node = chip->dev->of_node;
	if (!chip->nor_ic)
		chip->nor_ic = of_get_oplus_chg_ic(node, "oplus,nor_ic", 0);
	return !!chip->nor_ic;
}

__maybe_unused static bool is_err_topic_available(struct oplus_p9415 *chip)
{
	if (!chip->err_topic)
		chip->err_topic = oplus_mms_get_by_name("error");
	return !!chip->err_topic;
}

static int p9415_rx_is_connected(struct oplus_chg_ic_dev *dev, bool *connected)
{
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		*connected = false;
		return 0;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (!gpio_is_valid(chip->rx_con_gpio)) {
		chg_err("rx_con_gpio invalid\n");
		*connected = false;
		return 0;
	}

	if (!!gpio_get_value(chip->rx_con_gpio))
		*connected = true;
	else
		*connected = false;
	return 0;
}

static int p9415_get_wls_type(struct oplus_p9415 *chip)
{
	struct oplus_mms *wls_topic = NULL;
	union mms_msg_data data = { 0 };

	wls_topic = oplus_mms_get_by_name("wireless");
	if (wls_topic) {
		oplus_mms_get_item_data(wls_topic, WLS_ITEM_WLS_TYPE, &data, true);
		return data.intval;
	}
	return OPLUS_CHG_WLS_UNKNOWN;
}

static __inline__ void p9415_i2c_err_inc(struct oplus_p9415 *chip)
{
	bool connected = false;

	p9415_rx_is_connected(chip->ic_dev, &connected);
	if (connected && (atomic_inc_return(&chip->i2c_err_count) > I2C_ERR_MAX)) {
		atomic_set(&chip->i2c_err_count, 0);
		chg_err("read i2c error\n");
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
		/*oplus_chg_anon_mod_event(chip->wls_ocm, OPLUS_CHG_EVENT_RX_IIC_ERR);*/
		/*todo, add i2c error callback*/
	}
}

static __inline__ void p9415_i2c_err_clr(struct oplus_p9415 *chip)
{
	atomic_set(&chip->i2c_err_count, 0);
}

static int p9415_read_byte(struct oplus_p9415 *chip, u16 addr, u8 *data)
{
	char cmd_buf[2] = { addr >> 8, addr & 0xff };
	int rc = 0;

	mutex_lock(&chip->i2c_lock);
	rc = i2c_master_send(chip->client, cmd_buf, 2);
	if (rc < 2) {
		chg_err("read 0x%04x error, rc=%d\n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}

	rc = i2c_master_recv(chip->client, data, 1);
	if (rc < 1) {
		chg_err("read 0x%04x error, rc=%d\n", addr, rc);
		rc = rc < 0 ? rc : -EIO;
		goto error;
	}

	mutex_unlock(&chip->i2c_lock);
	p9415_i2c_err_clr(chip);

	return 0;

error:
	mutex_unlock(&chip->i2c_lock);
	p9415_i2c_err_inc(chip);
	return rc;
}

static int p9415_read_data(struct oplus_p9415 *chip, u16 addr, u8 *buf, int len)
{
	char cmd_buf[2] = { addr >> 8, addr & 0xff };
	int rc = 0;

	mutex_lock(&chip->i2c_lock);
	rc = i2c_master_send(chip->client, cmd_buf, 2);
	if (rc < 2) {
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

	mutex_unlock(&chip->i2c_lock);
	p9415_i2c_err_clr(chip);

	return 0;

error:
	mutex_unlock(&chip->i2c_lock);
	p9415_i2c_err_inc(chip);
	return rc;
}

static int p9415_write_byte(struct oplus_p9415 *chip, u16 addr, u8 data)
{
	u8 buf_temp[3] = { addr >> 8, addr & 0xff, data };
	int rc;

	mutex_lock(&chip->i2c_lock);
	rc = i2c_master_send(chip->client, buf_temp, 3);
	if (rc < 3) {
		chg_err("write 0x%04x error, rc=%d\n", addr, rc);
		mutex_unlock(&chip->i2c_lock);
		p9415_i2c_err_inc(chip);
		rc = rc < 0 ? rc : -EIO;
		return rc;
	}
	mutex_unlock(&chip->i2c_lock);
	p9415_i2c_err_clr(chip);

	return 0;
}

static int p9415_write_data(struct oplus_p9415 *chip, u16 addr, u8 *buf, int len)
{
	u8 *buf_temp;
	int i;
	int rc;

	buf_temp = kzalloc(len + 2, GFP_KERNEL);
	if (!buf_temp) {
		chg_err("alloc memary error\n");
		return -ENOMEM;
	}

	buf_temp[0] = addr >> 8;
	buf_temp[1] = addr & 0xff;
	for (i = 0; i < len; i++)
		buf_temp[i + 2] = buf[i];

	mutex_lock(&chip->i2c_lock);
	rc = i2c_master_send(chip->client, buf_temp, len + 2);
	if (rc < 3) {
		chg_err("write 0x%04x error, rc=%d\n", addr, rc);
		mutex_unlock(&chip->i2c_lock);
		kfree(buf_temp);
		rc = rc < 0 ? rc : -EIO;
		return rc;
	}
	mutex_unlock(&chip->i2c_lock);
	kfree(buf_temp);

	return 0;
}

__maybe_unused static int p9415_read_byte_mask(struct oplus_p9415 *chip, u16 addr, u8 mask, u8 *data)
{
	u8 temp;
	int rc;

	rc = p9415_read_byte(chip, addr, &temp);
	if (rc < 0)
		return rc;

	*data = mask & temp;

	return 0;
}

static int p9415_write_byte_mask(struct oplus_p9415 *chip, u16 addr, u8 mask, u8 data)
{
	u8 temp;
	int rc;

	rc = p9415_read_byte(chip, addr, &temp);
	if (rc < 0)
		return rc;
	temp = (data & mask) | (temp & (~mask));
	rc = p9415_write_byte(chip, addr, temp);
	if (rc < 0)
		return rc;

	return 0;
}

#ifdef WLS_QI_DEBUG
static int p9415_write_reg = 0;
static int p9415_read_reg = 0;
static ssize_t p9415_write_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	u8 reg_val = 0;
	struct oplus_p9415 *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -EINVAL;
	}
	p9415_read_byte(chip, (u16)p9415_write_reg, &reg_val);
	count += snprintf(buf+count, PAGE_SIZE - count, "reg[0x%02x]=0x%02x\n", p9415_write_reg, reg_val);

	return count;
}

static ssize_t p9415_write_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int databuf[2] = {0, 0};
	struct oplus_p9415 *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -EINVAL;
	}
	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		chg_err("reg[0x%02x]=0x%02x\n", databuf[0], databuf[1]);
		p9415_write_reg = databuf[0];
		p9415_write_byte(chip, (u16)databuf[0], (u8)databuf[1]);
	}

	return count;
}

static ssize_t p9415_read_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	u8 reg_val = 0;
	struct oplus_p9415 *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -EINVAL;
	}

	p9415_read_byte(chip, (u16)p9415_read_reg, &reg_val);
	count += snprintf(buf+count, PAGE_SIZE - count, "reg[0x%02x]=0x%02x\n", p9415_read_reg, reg_val);

	return count;
}

static ssize_t p9415_read_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int databuf[2] = {0, 0};
	struct oplus_p9415 *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%x", &databuf[0]) == 1)
		p9415_read_reg = databuf[0];

	return count;
}

static DEVICE_ATTR(write_reg, S_IWUSR | S_IRUGO, p9415_write_reg_show, p9415_write_reg_store);
static DEVICE_ATTR(read_reg, S_IWUSR | S_IRUGO, p9415_read_reg_show, p9415_read_reg_store);

static struct attribute *p9415_attributes[] = {
	&dev_attr_write_reg.attr,
	&dev_attr_read_reg.attr,
	NULL
};

static struct attribute_group p9415_attribute_group = {
	.attrs = p9415_attributes
};
#endif /*WLS_QI_DEBUG*/

#define TRACK_LOCAL_T_NS_TO_S_THD		1000000000
#define TRACK_UPLOAD_COUNT_MAX		10
#define TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD	(24 * 3600)
static int p9415_track_get_local_time_s(void)
{
	int local_time_s;

	local_time_s = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;
	chg_info("local_time_s:%d\n", local_time_s);

	return local_time_s;
}

static int p9415_track_upload_wls_ic_err_info(struct oplus_p9415 *chip,
	enum wls_err_scene scene_type, enum wls_ic_err_reason reason_type)
{
	int curr_time;
	static int upload_count = 0;
	static int pre_upload_time = 0;

	if (!is_err_topic_available(chip)) {
		chg_err("error topic not found\n");
		return -ENODEV;
	}

	if (scene_type >= ARRAY_SIZE(wls_err_scene_text) || scene_type < 0) {
		chg_err("wls err scene inval\n");
		return -EINVAL;
	}

	if (reason_type >= ARRAY_SIZE(wls_ic_err_reason_text) || reason_type < 0) {
		chg_err("wls ic err reason inval\n");
		return -EINVAL;
	}

	chg_info("start\n");
	curr_time = p9415_track_get_local_time_s();
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (upload_count > TRACK_UPLOAD_COUNT_MAX)
		return 0;

	upload_count++;
	pre_upload_time = p9415_track_get_local_time_s();

	oplus_chg_ic_creat_err_msg(chip->ic_dev, OPLUS_IC_ERR_WLS_RX, 0,
		"$$err_scene@@%s$$err_reason@@%s",
		wls_err_scene_text[scene_type], wls_ic_err_reason_text[reason_type]);
	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);

	chg_info("success\n");

	return 0;
}

static int p9415_track_debugfs_init(struct oplus_p9415 *chip)
{
	int ret = 0;
	struct dentry *debugfs_root;
	struct dentry *debugfs_p9415;

	debugfs_root = oplus_chg_track_get_debugfs_root();
	if (!debugfs_root) {
		ret = -ENOENT;
		return ret;
	}

	debugfs_p9415 = debugfs_create_dir("p9415", debugfs_root);
	if (!debugfs_p9415) {
		ret = -ENOENT;
		return ret;
	}

	chip->debug_force_ic_err = WLS_IC_ERR_NONE;
	debugfs_create_u32("debug_force_ic_err", 0644, debugfs_p9415, &(chip->debug_force_ic_err));

	return ret;
}

static int p9415_set_trx_boost_enable(struct oplus_p9415 *chip, bool en)
{
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return  -ENODEV;
	}
	if (!is_nor_ic_available(chip)) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}
	return oplus_chg_wls_nor_set_boost_en(chip->nor_ic, en);
}

static int p9415_set_trx_boost_vol(struct oplus_p9415 *chip, int vol_mv)
{
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return  -ENODEV;
	}
	if (!is_nor_ic_available(chip)) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}
	return oplus_chg_wls_nor_set_boost_vol(chip->nor_ic, vol_mv);
}

__maybe_unused static int p9415_set_trx_boost_curr_limit(struct oplus_p9415 *chip, int curr_ma)
{
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return  -ENODEV;
	}
	if (!is_nor_ic_available(chip)) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}
	return oplus_chg_wls_nor_set_boost_curr_limit(chip->nor_ic, curr_ma);
}

__maybe_unused static int p9415_get_rx_event_gpio_val(struct oplus_p9415 *chip)
{
	if (!gpio_is_valid(chip->rx_event_gpio)) {
		chg_err("rx_event_gpio invalid\n");
		return -ENODEV;
	}

	return gpio_get_value(chip->rx_event_gpio);
}

static int p9415_set_rx_enable(struct oplus_chg_ic_dev *dev, bool en)
{
	struct oplus_p9415 *chip;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (IS_ERR_OR_NULL(chip->pinctrl) ||
	    IS_ERR_OR_NULL(chip->rx_en_active) ||
	    IS_ERR_OR_NULL(chip->rx_en_sleep)) {
		chg_err("rx_en pinctrl error\n");
		return -ENODEV;
	}

	rc = pinctrl_select_state(chip->pinctrl,
		en ? chip->rx_en_active : chip->rx_en_sleep);
	if (rc < 0)
		chg_err("can't %s rx\n", en ? "enable" : "disable");
	else
		chg_info("set rx %s\n", en ? "enable" : "disable");

	chg_info("vt_sleep: set value:%d, gpio_val:%d\n", !en, gpio_get_value(chip->rx_en_gpio));

	return rc;
}

static int p9415_rx_is_enable(struct oplus_chg_ic_dev *dev, bool *enable)
{
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		*enable = false;
		return 0;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (!gpio_is_valid(chip->rx_en_gpio)) {
		chg_err("rx_en_gpio invalid\n");
		*enable = false;
		return 0;
	}
	/* When rx_en_gpio is low, RX is enabled;
	 * when rx_en_gpio is high, RX is sleeps;
	 * the "negation" operation to obtain the appropriate value;
	 */
	if (!gpio_get_value(chip->rx_en_gpio))
		*enable = true;
	else
		*enable = false;
	return 0;
}

static int p9415_get_vout(struct oplus_chg_ic_dev *dev, int *vout)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int temp;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_VOUT_R, val_buf, 2);
	if (rc < 0) {
		chg_err("read vout err, rc=%d\n", rc);
		return rc;
	}
	temp = val_buf[0] | val_buf[1] << 8;
	*vout = temp * 21000 / 4095;

	return 0;
}

static int p9415_set_vout(struct oplus_chg_ic_dev *dev, int vout)
{
	struct oplus_p9415 *chip;
	char val_buf[2];
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	val_buf[0] = vout & 0x00FF;
	val_buf[1] = (vout & 0xFF00) >> 8;

	rc = p9415_write_data(chip, P9415_REG_VOUT_W, val_buf, 2);
	if (rc < 0) {
		chg_err("set vout err, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int p9415_get_vrect(struct oplus_chg_ic_dev *dev, int *vrect)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int temp;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_VRECT, val_buf, 2);
	if (rc < 0) {
		chg_err("read vrect err, rc=%d\n", rc);
		return rc;
	}
	temp = val_buf[0] | val_buf[1] << 8;
	*vrect = temp * 26250 / 4095;

	return 0;
}

static int p9415_get_iout(struct oplus_chg_ic_dev *dev, int *iout)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_TOUT, val_buf, 2);
	if (rc < 0) {
		chg_err("read iout err, rc=%d\n", rc);
		return rc;
	}
	*iout = val_buf[0] | val_buf[1] << 8;

	return 0;
}

static int p9415_get_tx_vout(struct oplus_chg_ic_dev *dev, int *vout)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_TRX_VOL, val_buf, 2);
	if (rc < 0) {
		chg_err("read trx vol err, rc=%d\n", rc);
		return rc;
	}
	*vout = val_buf[0] | val_buf[1] << 8;

	return 0;
}

static int p9415_get_tx_iout(struct oplus_chg_ic_dev *dev, int *iout)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_TRX_CURR, val_buf, 2);
	if (rc < 0) {
		chg_err("read trx current err, rc=%d\n", rc);
		return rc;
	}
	*iout = val_buf[0] | val_buf[1] << 8;

	return 0;
}

static int p9415_get_cep_count(struct oplus_chg_ic_dev *dev, int *count)
{
	struct oplus_p9415 *chip;
	char val_buf[2] = { 0, 0 };
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_CEP_COUNT, val_buf, 2);
	if (rc < 0) {
		chg_err("Couldn't read cep change status, rc=%d\n", rc);
		return rc;
	}
	*count = val_buf[0] | val_buf[1] << 8;

	return 0;
}

static int p9415_get_cep_val(struct oplus_chg_ic_dev *dev, int *val)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp = 0;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_CEP, &temp, 1);
	if (rc < 0) {
		chg_err("Couldn't read CEP, rc = %x\n", rc);
		return rc;
	}
	*val = (signed char)temp;

	return rc;
}


static int p9415_get_work_freq(struct oplus_chg_ic_dev *dev, int *val)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_data(chip, P9415_REG_FREQ, &temp, 1);
	if (rc < 0) {
		chg_err("Couldn't read rx freq val, rc = %d\n", rc);
		return rc;
	}
	*val = (int)temp;
	return rc;
}

static int p9415_get_rx_mode(struct oplus_chg_ic_dev *dev, enum oplus_chg_wls_rx_mode *rx_mode)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (chip->adapter_type == 0) {
		rc = p9415_read_data(chip, P9415_REG_RX_MODE, &temp, 1);
		if (rc < 0) {
			chg_err("Couldn't read rx mode, rc=%d\n", rc);
			return rc;
		}
		if (temp == P9415_RX_MODE_EPP) {
			chg_info("rx running in EPP!\n");
			chip->adapter_type = P9415_RX_MODE_EPP;
		} else if (temp == P9415_RX_MODE_BPP) {
			chg_info("rx running in BPP!\n");
			chip->adapter_type = P9415_RX_MODE_BPP;
		} else {
			chg_info("rx running in Others!\n");
			chip->adapter_type = 0;
		}
	}
	if (chip->adapter_type == P9415_RX_MODE_EPP) {
		if (chip->rx_pwr_cap == 0) {
			rc = p9415_read_data(chip, P9415_REG_RX_PWR, &temp, 1);
			if (rc < 0) {
				chg_err("Couldn't read rx pwr, rc = %d\n", rc);
				return rc;
			}
			chg_info("running mode epp-%d/2w\n", temp);
			if (!chip->support_epp_11w && temp >= P9415_RX_PWR_15W)
				chip->rx_pwr_cap = P9415_RX_PWR_15W;
			else if (chip->support_epp_11w && temp >= P9415_RX_PWR_11W)
				chip->rx_pwr_cap = P9415_RX_PWR_11W;
			else if (temp >= P9415_RX_PWR_10W)
				chip->rx_pwr_cap = P9415_RX_PWR_10W;
			else
				chip->rx_pwr_cap = P9415_RX_PWR_5W;
		}
		if (chip->rx_pwr_cap == P9415_RX_PWR_15W ||
		    chip->rx_pwr_cap == P9415_RX_PWR_11W)
			*rx_mode = OPLUS_CHG_WLS_RX_MODE_EPP_PLUS;
		else if (chip->rx_pwr_cap == P9415_RX_PWR_10W)
			*rx_mode = OPLUS_CHG_WLS_RX_MODE_EPP;
		else
			*rx_mode = OPLUS_CHG_WLS_RX_MODE_EPP_5W;
	} else if (chip->adapter_type == P9415_RX_MODE_BPP) {
		*rx_mode = OPLUS_CHG_WLS_RX_MODE_BPP;
	} else {
		*rx_mode = OPLUS_CHG_WLS_RX_MODE_UNKNOWN;
	}
	chg_debug("rx_mode=%d\n", *rx_mode);

	return 0;
}

__maybe_unused static int p9415_set_rx_mode(struct oplus_chg_ic_dev *dev, enum oplus_chg_wls_rx_mode rx_mode)
{
	struct oplus_p9415 *chip;
	int rc = 0;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (IS_ERR_OR_NULL(chip->pinctrl) ||
	    IS_ERR_OR_NULL(chip->mode_sw_active) ||
	    IS_ERR_OR_NULL(chip->mode_sw_sleep)) {
		chg_err("mode_sw pinctrl error\n");
		return -ENODEV;
	}

	rc = pinctrl_select_state(chip->pinctrl, chip->mode_sw_active);
	if (rc < 0)
		chg_err("can't set mode_sw active, rc=%d\n", rc);
	else
		chg_info("set mode_sw active\n");

	chg_info("mode_sw: gpio_val:%d\n", gpio_get_value(chip->mode_sw_gpio));

	rc = p9415_set_rx_enable(dev, false);
	if (rc < 0)
		chg_err("set rx disable failed, rc=%d\n", rc);

	msleep(100);

	rc = p9415_set_rx_enable(dev, true);
	if (rc < 0)
		chg_err("set rx enable failed, rc=%d\n", rc);

	return rc;
}

__maybe_unused static bool p9415_get_mode_sw_active(struct oplus_p9415 *chip)
{
	if (!chip) {
		chg_err("oplus_p9415 chip is null!\n");
		return false;
	}

	if (!gpio_is_valid(chip->mode_sw_gpio)) {
		chg_err("mode_sw_gpio not specified\n");
		return false;
	}

	return gpio_get_value(chip->mode_sw_gpio);
}

__maybe_unused static int p9415_set_mode_sw_default(struct oplus_p9415 *chip)
{
	int rc = 0;

	if (!chip) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	if (IS_ERR_OR_NULL(chip->pinctrl) ||
	    IS_ERR_OR_NULL(chip->mode_sw_active) ||
	    IS_ERR_OR_NULL(chip->mode_sw_sleep)) {
		chg_err("mode_sw pinctrl error\n");
		return -ENODEV;
	}

	rc = pinctrl_select_state(chip->pinctrl, chip->mode_sw_sleep);
	if (rc < 0)
		chg_err("can't set mode_sw active, rc=%d\n", rc);
	else
		chg_info("set mode_sw default\n");

	chg_info("mode_sw: gpio_val:%d\n", gpio_get_value(chip->mode_sw_gpio));

	return rc;
}

static int p9415_set_dcdc_enable(struct oplus_chg_ic_dev *dev, bool en)
{
	struct oplus_p9415 *chip;
	int rc;

	if (en == false)
		return 0;
	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_write_byte_mask(chip, P9415_REG_PWR_CTRL,
		P9415_DCDC_EN, P9415_DCDC_EN);
	if (rc < 0)
		chg_err("set dcdc enable error, rc=%d\n", rc);

	return rc;
}

static int p9415_set_tx_enable(struct oplus_chg_ic_dev *dev, bool en)
{
	struct oplus_p9415 *chip;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_write_byte(chip, P9415_REG_TRX_CTRL,
		en ? P9415_TRX_EN : 0x00);
	if (rc < 0)
		chg_err("can't %s trx, rc=%d\n", en ? "enable" : "disable", rc);

	return rc;
}

static int p9415_set_tx_start(struct oplus_chg_ic_dev *dev)
{
	return 0;
}

static int p9415_get_tx_status(struct oplus_chg_ic_dev *dev, u8 *status)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp;
	u8 trx_state = 0;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_byte(chip, P9415_REG_TRX_STATUS, &temp);
	if (rc) {
		chg_err("Couldn't read trx status, rc = %d\n", rc);
		return rc;
	}

	if (temp & P9415_TRX_STATUS_READY)
		trx_state |= WLS_TRX_STATUS_READY;
	if (temp & P9415_TRX_STATUS_DIGITALPING)
		trx_state |= WLS_TRX_STATUS_DIGITALPING;
	if (temp & P9415_TRX_STATUS_ANALOGPING)
		trx_state |= WLS_TRX_STATUS_ANALOGPING;
	if (temp & P9415_TRX_STATUS_TRANSFER)
		trx_state |= WLS_TRX_STATUS_TRANSFER;
	*status = trx_state;

	return rc;
}

static int p9415_get_tx_err(struct oplus_chg_ic_dev *dev, u8 *err)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp = 0;
	u8 trx_err = 0;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_byte(chip, P9415_REG_TRX_ERR, &temp);
	if (rc) {
		chg_err("Couldn't read trx err code, rc = %d\n", rc);
		return rc;
	}

	if (temp & P9415_TRX_ERR_RXAC)
		trx_err |= WLS_TRX_ERR_RXAC;
	if (temp & P9415_TRX_ERR_OCP)
		trx_err |= WLS_TRX_ERR_OCP;
	if (temp & P9415_TRX_ERR_OVP)
		trx_err |= WLS_TRX_ERR_OVP;
	if (temp & P9415_TRX_ERR_LVP)
		trx_err |= WLS_TRX_ERR_LVP;
	if (temp & P9415_TRX_ERR_FOD)
		trx_err |= WLS_TRX_ERR_FOD;
	if (temp & P9415_TRX_ERR_OTP)
		trx_err |= WLS_TRX_ERR_OTP;
	if (temp & P9415_TRX_ERR_CEPTIMEOUT)
		trx_err |= WLS_TRX_ERR_CEPTIMEOUT;
	if (temp & P9415_TRX_ERR_RXEPT)
		trx_err |= WLS_TRX_ERR_RXEPT;
	*err = trx_err;

	return rc;
}

static void p9415_rx_track_info(struct oplus_p9415 *chip)
{
	int rc = 0;
	char temp = 0;
	enum wls_ic_err_reason rx_err_reason = WLS_IC_ERR_NONE;

	rc = p9415_read_byte(chip, P9415_REG_TRX_ERR, &temp);
	if (rc) {
		chg_err("Couldn't read trx err code, rc = %d\n", rc);
		return;
	}
	if (temp & P9415_TRX_ERR_RXAC)
		rx_err_reason = WLS_IC_ERR_RXAC;
	if (temp & P9415_TRX_ERR_OCP)
		rx_err_reason = WLS_IC_ERR_OCP;
	if (temp & P9415_TRX_ERR_OVP)
		rx_err_reason = WLS_IC_ERR_OVP;
	if (temp & P9415_TRX_ERR_LVP)
		rx_err_reason = WLS_IC_ERR_LVP;
	if (temp & P9415_TRX_ERR_FOD)
		rx_err_reason = WLS_IC_ERR_FOD;
	if (temp & P9415_TRX_ERR_OTP)
		rx_err_reason = WLS_IC_ERR_OTP;
	if (temp & P9415_TRX_ERR_RXEPT)
		rx_err_reason = WLS_IC_ERR_RXEPT;

	if (chip->debug_force_ic_err) {
		rx_err_reason = chip->debug_force_ic_err;
		chip->debug_force_ic_err = WLS_IC_ERR_NONE;
	}
	if (rx_err_reason != WLS_IC_ERR_NONE)
		p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_RX, rx_err_reason);

	return;
}

static void p9415_tx_track_info(struct oplus_p9415 *chip)
{
	int rc = 0;
	char temp = 0;
	enum wls_ic_err_reason tx_err_reason = WLS_IC_ERR_NONE;

	rc = p9415_read_byte(chip, P9415_REG_TRX_ERR, &temp);
	if (rc) {
		chg_err("Couldn't read trx err code, rc = %d\n", rc);
		return;
	}
	if (temp & P9415_TRX_ERR_RXAC)
		tx_err_reason = WLS_IC_ERR_RXAC;
	if (temp & P9415_TRX_ERR_OCP)
		tx_err_reason = WLS_IC_ERR_OCP;
	if (temp & P9415_TRX_ERR_OVP)
		tx_err_reason = WLS_IC_ERR_OVP;
	if (temp & P9415_TRX_ERR_LVP)
		tx_err_reason = WLS_IC_ERR_LVP;
	if (temp & P9415_TRX_ERR_FOD)
		tx_err_reason = WLS_IC_ERR_FOD;
	if (temp & P9415_TRX_ERR_OTP)
		tx_err_reason = WLS_IC_ERR_OTP;
	if (temp & P9415_TRX_ERR_RXEPT)
		tx_err_reason = WLS_IC_ERR_RXEPT;

	if (chip->debug_force_ic_err) {
		tx_err_reason = chip->debug_force_ic_err;
		chip->debug_force_ic_err = WLS_IC_ERR_NONE;
	}
	if (tx_err_reason != WLS_IC_ERR_NONE)
		p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_TX, tx_err_reason);

	return;
}

static int p9415_get_headroom(struct oplus_chg_ic_dev *dev, int *val)
{
	struct oplus_p9415 *chip;
	int rc;
	char temp;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_read_byte(chip, P9415_REG_HEADROOM_R, &temp);
	if (rc) {
		chg_err("Couldn't read headroom, rc = %d\n", rc);
		return rc;
	}
	*val = (int)temp;

	return rc;
}

static int p9415_set_headroom(struct oplus_chg_ic_dev *dev, int val)
{
	struct oplus_p9415 *chip;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_write_byte(chip, P9415_REG_HEADROOM_W, val);
	if (rc)
		chg_err("can't set headroom, rc=%d\n", rc);

	return rc;
}

static u8 p9415_calculate_checksum(const u8 *data, int len)
{
	u8 temp = 0;

	while (len--)
		temp ^= *data++;

	chg_debug("checksum = %d\n", temp);
	return temp;
}

static int p9415_send_match_q(struct oplus_chg_ic_dev *dev, u8 data)
{
	struct oplus_p9415 *chip;
	u8 buf[4] = { 0x38, 0x48, 0x00, data };
	u8 checksum;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	checksum = p9415_calculate_checksum(buf, 4);
	p9415_write_byte(chip, 0x0050, buf[0]);
	p9415_write_byte(chip, 0x0051, buf[1]);
	p9415_write_byte(chip, 0x0052, buf[2]);
	p9415_write_byte(chip, 0x0053, buf[3]);
	p9415_write_byte(chip, 0x0054, checksum);

	p9415_write_byte_mask(chip, 0x004E, 0x01, 0x01);

	return 0;
}

static int p9415_set_fod_parm(struct oplus_chg_ic_dev *dev, u8 data[], int len)
{
	struct oplus_p9415 *chip;
	int rc;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	rc = p9415_write_data(chip, 0x0068, data, len);
	if (rc < 0)
		chg_err("set fod parameter error, rc=%d\n", rc);

	return rc;
}

static int p9415_send_msg(struct oplus_chg_ic_dev *dev, unsigned char msg[], int len)
{
	struct oplus_p9415 *chip;
	char write_data[2] = { 0, 0 };

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	if (len != 4) {
		chg_err("data length error\n");
		return -EINVAL;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (msg[0] == WLS_CMD_INDENTIFY_ADAPTER) {
		write_data[0] = 0x10;
		write_data[1] = 0x00;
		p9415_write_data(chip, 0x0038, write_data, 2);

		write_data[0] = 0x10;
		write_data[1] = 0x00;
		p9415_write_data(chip, 0x0056, write_data, 2);

		write_data[0] = 0x20;
		write_data[1] = 0x00;
		p9415_write_data(chip, 0x004E, write_data, 2);
	}

	if ((msg[0] != WLS_CMD_GET_TX_ID) && (msg[0] != WLS_CMD_GET_TX_PWR)) {
		p9415_write_byte(chip, 0x0050, 0x48);
		p9415_write_byte(chip, 0x0051, msg[0]);
		p9415_write_byte(chip, 0x0052, msg[1]);
		p9415_write_byte(chip, 0x0053, msg[2]);
		p9415_write_byte(chip, 0x0054, msg[3]);
	} else {
		p9415_write_byte(chip, 0x0050, 0x18);
		p9415_write_byte(chip, 0x0051, msg[0]);
	}

	p9415_write_byte_mask(chip, 0x004E, 0x01, 0x01);

	return 0;
}

static int p9415_register_msg_callback(struct oplus_chg_ic_dev *dev, void *data, void (*call_back)(void *, u8[]))
{
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	chip->rx_msg.dev_data = data;
	chip->rx_msg.msg_call_back = call_back;

	return 0;
}

static int p9415_load_bootloader(struct oplus_p9415 *chip)
{
	int rc = 0;

	/*configure the system*/
	rc = p9415_write_byte(chip, 0x3000, 0x5a); /*write key*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3000 reg error!\n");
		return rc;
	}

	rc = p9415_write_byte(chip, 0x3004, 0x00); /*set HS clock*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3004 reg error!\n");
		return rc;
	}

	rc = p9415_write_byte(chip, 0x3008, 0x09); /*set AHB clock*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3008 reg error!\n");
		return rc;
	}

	rc = p9415_write_byte(chip, 0x300C, 0x05); /*configure 1us pulse*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x300c reg error!\n");
		return rc;
	}

	rc = p9415_write_byte(chip, 0x300D, 0x1d); /*configure 500ns pulse*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x300d reg error!\n");
		return rc;
	}

	rc = p9415_write_byte(chip, 0x3040, 0x11); /*Enable MTP access via I2C*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3040 reg error!\n");
		return rc;
	}

	msleep(10);

	chg_info("<IDT UPDATE>-b-2--!\n");
	rc = p9415_write_byte(chip, 0x3040, 0x10); /*halt microcontroller M0*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3040 reg error!\n");
		return rc;
	}

	msleep(10);

	chg_info("<IDT UPDATE>-b-3--!\n");
	rc = p9415_write_data(chip, 0x0800, mtpbootloader9320,
		sizeof(mtpbootloader9320)); /*load provided by IDT array*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x1c00 reg error!\n");
		return rc;
	}

	chg_info("<IDT UPDATE>-b-4--!\n");
	rc = p9415_write_byte(chip, 0x400, 0); /*initialize buffer*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x400 reg error!\n");
		return rc;
	}

	chg_info("<IDT UPDATE>-b-5--!\n");
	rc = p9415_write_byte(chip, 0x3048, 0xD0); /*map RAM address 0x1c00 to OTP 0x0000*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3048 reg error!\n");
		return rc;
	}

	chg_info("<IDT UPDATE>-b-6--!\n");
	rc = p9415_write_byte(chip, 0x3040, 0x80); /*run M0*/

	return 0;
}

static int p9415_load_fw(struct oplus_p9415 *chip, unsigned char *fw_data, int code_length)
{
	unsigned char write_ack = 0;
	int rc = 0;

	rc = p9415_write_data(chip, 0x400, fw_data, ((code_length + 8 + 15) / 16) * 16);
	if (rc != 0) {
		chg_err("<IDT UPDATE>ERROR: write multi byte data error!\n");
		goto LOAD_ERR;
	}
	rc = p9415_write_byte(chip, 0x400, 0x01);
	if (rc != 0) {
		chg_err("<IDT UPDATE>ERROR: on OTP buffer validation\n");
		goto LOAD_ERR;
	}

	do {
		msleep(20);
		rc = p9415_read_byte(chip, 0x401, &write_ack);
		if (rc != 0) {
			chg_err("<IDT UPDATE>ERROR: on reading OTP buffer status\n");
			goto LOAD_ERR;
		}
	} while ((write_ack & 0x01) != 0);

	/*check status*/
	if (write_ack != 2) { /*not OK*/
		if (write_ack == 4)
			chg_err("<IDT UPDATE>ERROR: WRITE ERR\n");
		else if (write_ack == 8)
			chg_err("<IDT UPDATE>ERROR: CHECK SUM ERR\n");
		else
			chg_err("<IDT UPDATE>ERROR: UNKNOWN ERR\n");

		rc = -1;
	}
LOAD_ERR:
	return rc;
}

static int p9415_mtp(struct oplus_p9415 *chip, unsigned char *fw_buf, int fw_size)
{
	int rc;
	int i, j;
	unsigned char *fw_data;
	unsigned char write_ack;
	unsigned short int start_addr;
	unsigned short int checksum;
	unsigned short int code_length;
	/*pure fw size not contains last 128 bytes fw version.*/
	int pure_fw_size = fw_size - 128;

	chg_info("<IDT UPDATE>--1--!\n");

	rc = p9415_load_bootloader(chip);
	if (rc != 0) {
		chg_err("<IDT UPDATE>Update bootloader 1 error!\n");
		p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_UPDATE, WLS_IC_ERR_I2C);
		return rc;
	}

	msleep(100);

	chg_info("<IDT UPDATE>The idt firmware size: %d!\n", fw_size);

	/*program pages of 128 bytes*/
	/*8-bytes header, 128-bytes data, 8-bytes padding to round to 16-byte boundary*/
	fw_data = kzalloc(144, GFP_KERNEL);
	if (!fw_data) {
		chg_err("<IDT UPDATE>can't alloc memory!\n");
		return -ENOMEM;
	}

	/*ERASE FW VERSION(the last 128 byte of the MTP)*/
	memset(fw_data, 0x00, 144);
	start_addr = pure_fw_size;
	checksum = start_addr;
	code_length = 128;
	for (j = 127; j >= 0; j--)
		checksum += fw_data[j + 8]; /*add the non zero values.*/

	checksum += code_length; /*finish calculation of the check sum*/
	memcpy(fw_data + 2, (char *)&start_addr, 2);
	memcpy(fw_data + 4, (char *)&code_length, 2);
	memcpy(fw_data + 6, (char *)&checksum, 2);
	rc = p9415_load_fw(chip, fw_data, code_length);
	if (rc < 0) { /*not OK*/
		chg_err("<IDT UPDATE>ERROR: erase fw version ERR\n");
		goto MTP_ERROR;
	}

	/*upgrade fw*/
	memset(fw_data, 0x00, 144);
	for (i = 0; i < pure_fw_size; i += 128) {
		chg_info("<IDT UPDATE>Begin to write chunk %d!\n", i);

		start_addr = i;
		checksum = start_addr;
		code_length = 128;

		memcpy(fw_data + 8, fw_buf + i, 128);

		j = pure_fw_size - i;
		if (j < 128) {
			j = ((j + 15) / 16) * 16;
			code_length = (unsigned short int)j;
		} else {
			j = 128;
		}

		j -= 1;
		for (; j >= 0; j--)
			checksum += fw_data[j + 8]; /*add the non zero values*/

		checksum += code_length; /*finish calculation of the check sum*/

		memcpy(fw_data + 2, (char *)&start_addr, 2);
		memcpy(fw_data + 4, (char *)&code_length, 2);
		memcpy(fw_data + 6, (char *)&checksum, 2);

		/*typedef struct { //write to structure at address 0x400
		u16 Status;
		u16 start_addr;
		u16 code_length;
		u16 DataChksum;
		u8 DataBuf[128];
		} P9220PgmStrType;*/
		/*read status is guaranteed to be != 1 at this point*/

		rc = p9415_load_fw(chip, fw_data, code_length);
		if (rc < 0) { /*not OK*/
			chg_err("<IDT UPDATE>ERROR: write chunk %d ERR\n", i);
			goto MTP_ERROR;
		}
	}

	msleep(100);
	chg_info("<IDT UPDATE>disable trx boost\n");
	rc = p9415_set_trx_boost_enable(chip, false);
	if (rc < 0) {
		chg_err("disable trx power error, rc=%d\n", rc);
		goto MTP_ERROR;
	}
	msleep(3000);
	chg_info("<IDT UPDATE>enable trx boost\n");
	rc = p9415_set_trx_boost_vol(chip, P9415_MTP_VOL_MV);
	if (rc < 0) {
		chg_err("set trx power vol(=%d), rc=%d\n", P9415_MTP_VOL_MV, rc);
		goto MTP_ERROR;
	}
	rc = p9415_set_trx_boost_enable(chip, true);
	if (rc < 0) {
		chg_err("enable trx power error, rc=%d\n", rc);
		goto MTP_ERROR;
	}
	msleep(500);

	/*Verify*/
	rc = p9415_load_bootloader(chip);
	if (rc != 0) {
		chg_err("<IDT UPDATE>Update bootloader 2 error!\n");
		goto I2C_ERROR;
	}
	msleep(100);
	rc = p9415_write_byte(chip, 0x402, 0x00); /*write start address*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x402 reg error!\n");
		goto I2C_ERROR;
	}

	rc = p9415_write_byte(chip, 0x403, 0x00); /*write start address*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x403 reg error!\n");
		goto I2C_ERROR;
	}

	rc = p9415_write_byte(chip, 0x404, pure_fw_size & 0xff); /*write FW length low byte*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x404 reg error!\n");
		goto I2C_ERROR;
	}
	rc = p9415_write_byte(chip, 0x405, (pure_fw_size >> 8) & 0xff); /*write FW length high byte*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x405 reg error!\n");
		goto I2C_ERROR;
	}

	/*write CRC from FW release package*/
	fw_data[0] = fw_buf[pure_fw_size + 0x08];
	fw_data[1] = fw_buf[pure_fw_size + 0x09];
	p9415_write_data(chip, 0x406, fw_data, 2);

	rc = p9415_write_byte(chip, 0x400, 0x11);
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x406 reg error!\n");
		goto I2C_ERROR;
	}
	do {
		msleep(20);
		rc = p9415_read_byte(chip, 0x401, &write_ack);
		if (rc != 0) {
			chg_err("<IDT UPDATE>ERROR: on reading OTP buffer status\n");
			goto I2C_ERROR;
		}
	} while ((write_ack & 0x01) != 0);
	/*check status*/
	if (write_ack != 2) { /*not OK*/
		if (write_ack == 4)
			chg_err("<IDT UPDATE>ERROR: CRC WRITE ERR\n");
		else if (write_ack == 8)
			chg_err("<IDT UPDATE>ERROR: CRC CHECK SUM ERR\n");
		else
			chg_err("<IDT UPDATE>ERROR: CRC UNKNOWN ERR\n");
		goto CRC_ERROR;
	}

	memset(fw_data, 0x00, 144);
	start_addr = pure_fw_size;
	checksum = start_addr;
	code_length = 128;
	memcpy(fw_data + 8, fw_buf + start_addr, 128);
	j = 127;
	for (; j >= 0; j--)
		checksum += fw_data[j + 8]; /*add the non zero values.*/

	checksum += code_length; /*finish calculation of the check sum*/
	memcpy(fw_data + 2, (char *)&start_addr, 2);
	memcpy(fw_data + 4, (char *)&code_length, 2);
	memcpy(fw_data + 6, (char *)&checksum, 2);

	rc = p9415_load_fw(chip, fw_data, code_length);
	if (rc < 0) { /*not OK*/
		chg_err("<IDT UPDATE>ERROR: erase fw version ERR\n");
		goto MTP_ERROR;
	}

	/*restore system*/
	rc = p9415_write_byte(chip, 0x3000, 0x5a); /*write key*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3000 reg error!\n");
		goto I2C_ERROR;
	}

	rc = p9415_write_byte(chip, 0x3048, 0x00); /*remove code remapping*/
	if (rc != 0) {
		chg_err("<IDT UPDATE>Write 0x3048 reg error!\n");
		goto I2C_ERROR;
	}

	chg_info("<IDT UPDATE>OTP Programming finished\n");

	kfree(fw_data);
	return 0;

MTP_ERROR:
	p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_UPDATE, WLS_IC_ERR_OTHER);
	kfree(fw_data);
	return -EINVAL;
CRC_ERROR:
	p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_UPDATE, WLS_IC_ERR_CRC);
	kfree(fw_data);
	return -EINVAL;
I2C_ERROR:
	p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_UPDATE, WLS_IC_ERR_I2C);
	kfree(fw_data);
	return -EINVAL;
}

static int p9415_get_rx_version(struct oplus_chg_ic_dev *dev, u32 *version)
{
	struct oplus_p9415 *chip;

	*version = 0;
	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return false;
	}
	chip = oplus_chg_ic_get_drvdata(dev);
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -ENODEV;
	}

	*version = chip->fw_rx_version;

	return 0;
}

static int p9415_get_tx_version(struct oplus_chg_ic_dev *dev, u32 *version)
{
	struct oplus_p9415 *chip;

	*version = 0;
	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return false;
	}
	chip = oplus_chg_ic_get_drvdata(dev);
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -ENODEV;
	}

	*version = chip->fw_tx_version;

	return 0;
}

static u32 p9415_get_fw_version_by_chip(struct oplus_p9415 *chip)
{
	u32 version = 0;
	char temp[4];
	int rc = 0;

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return 0;
	}

	rc = p9415_read_data(chip, 0x001C, temp, 4);
	if (rc < 0) {
		chg_err("read rx version error!\n");
		return 0;
	}
	version |= temp[3] << 24;
	version |= temp[2] << 16;
	version |= temp[1] << 8;
	version |= temp[0];

	return version;
}

static int p9415_upgrade_firmware_by_buf(struct oplus_chg_ic_dev *dev, unsigned char *fw_buf, int fw_size)
{
	struct oplus_p9415 *chip;
	int rc = 0;
	int fw_ver_start_addr;
	u32 version = 0;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return false;
	}
	chip = oplus_chg_ic_get_drvdata(dev);

	if (fw_buf == NULL) {
		chg_err("fw_buf is NULL\n");
		return -EINVAL;
	}

	fw_ver_start_addr = fw_size - 128;
	version = 0;
	version |= fw_buf[fw_ver_start_addr + 0x07] << 24;
	version |= fw_buf[fw_ver_start_addr + 0x06] << 16;
	version |= fw_buf[fw_ver_start_addr + 0x05] << 8;
	version |= fw_buf[fw_ver_start_addr + 0x04];

	disable_irq(chip->rx_con_irq);
	disable_irq(chip->rx_event_irq);

	rc = p9415_set_trx_boost_vol(chip, P9415_MTP_VOL_MV);
	if (rc < 0) {
		chg_err("set trx power vol(=%d), rc=%d\n", P9415_MTP_VOL_MV, rc);
		goto exit_enable_irq;
	}
	rc = p9415_set_trx_boost_enable(chip, true);
	if (rc < 0) {
		chg_err("enable trx power error, rc=%d\n", rc);
		goto exit_enable_irq;
	}
	msleep(50);

	if (chip->debug_force_ic_err == WLS_IC_ERR_I2C) {
		chg_err("<FW UPDATE> debug i2c error!\n");
		chip->debug_force_ic_err = WLS_IC_ERR_NONE;
		p9415_track_upload_wls_ic_err_info(chip, WLS_ERR_SCENE_UPDATE, WLS_IC_ERR_I2C);
	}

	if (p9415_get_fw_version_by_chip(chip) == version) {
		chg_info("<IDT UPDATE>fw is the same, fw[0x%x]!\n", version);
		goto no_need_update;
	}

	rc = p9415_mtp(chip, fw_buf, fw_size);
	if (rc != 0) {
		chg_err("<IDT UPDATE>update error, rc=%d\n", rc);
		version = 0;
	} else {
		version = p9415_get_fw_version_by_chip(chip);
	}

	msleep(100);
no_need_update:
	(void)p9415_set_trx_boost_enable(chip, false);
	msleep(20);
	chip->fw_rx_version = version;
	chip->fw_tx_version = version;

exit_enable_irq:
	enable_irq(chip->rx_con_irq);
	enable_irq(chip->rx_event_irq);

	return rc;
}

static int p9415_upgrade_firmware_by_img(struct oplus_chg_ic_dev *dev)
{
	int rc = 0;
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);
	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return -ENODEV;
	}
	if (!chip->fw_data || chip->fw_length <= 0) {
		chg_err("fw data lfailed\n");
		return -EINVAL;
	}
	rc = p9415_upgrade_firmware_by_buf(dev, chip->fw_data, chip->fw_length);

	return rc;
}

static void p9415_event_process(struct oplus_p9415 *chip)
{
	int rc = -1;
	char temp[2] = { 0, 0 };
	char val_buf[6] = { 0, 0, 0, 0, 0, 0};
	bool enable = false;

	if (chip == NULL) {
		chg_err("oplus_p9415 is NULL\n");
		return;
	}

	p9415_rx_is_enable(chip->ic_dev, &enable);
	if (!enable) {
		chg_info("RX is sleep, Ignore events\n");
		return;
	}

	rc = p9415_read_data(chip, P9415_REG_STATUS, temp, 2);
	if (rc) {
		chg_err("Couldn't read 0x%04x rc = %x\n", P9415_REG_STATUS, rc);
		temp[0] = 0;
	} else {
		chg_info("read P9415_REG_STATUS = 0x%02x 0x%02x\n", temp[0], temp[1]);
	}

	if (temp[0] & P9415_LDO_ON) {
		chg_info("LDO is on, connected.\n");
		complete(&chip->ldo_on);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ONLINE);
		chip->connected_ldo_on = true;
	}
	if (temp[0] & P9415_VOUT_ERR)
		chg_err("Vout residual voltage is too high\n");
	if (temp[0] & P9415_EVENT) {
		rc = p9415_read_data(chip, 0x0058, val_buf, 6);
		if (rc) {
			chg_err("Couldn't read 0x%04x, rc=%d\n", 0x0058, rc);
		} else {
			chg_info("Received TX data: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
				val_buf[0], val_buf[1], val_buf[2], val_buf[3], val_buf[4], val_buf[5]);
			if (chip->rx_msg.msg_call_back != NULL)
				chip->rx_msg.msg_call_back(chip->rx_msg.dev_data, val_buf);
			p9415_rx_track_info(chip);
		}
	}
	if (temp[0] & P9415_TRX_EVENT) {
		chg_info("trx event\n");
		p9415_tx_track_info(chip);
		chip->event_code = WLS_EVENT_TRX_CHECK;
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_EVENT_CHANGED);
	}
	p9415_write_byte_mask(chip, 0x0036, 0xFF, 0x00);
	p9415_write_byte_mask(chip, 0x0037, 0xFF, 0x00);
	p9415_write_byte_mask(chip, 0x0056, 0x30, 0x30);
	p9415_write_byte_mask(chip, 0x004E, 0x20, 0x20);
	schedule_delayed_work(&chip->check_event_work, msecs_to_jiffies(100));
}

static int p9415_connect_check(struct oplus_chg_ic_dev *dev)
{
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);
	schedule_delayed_work(&chip->connect_work, 0);

	return 0;
}

static int p9415_get_event_code(struct oplus_chg_ic_dev *dev, enum oplus_chg_wls_event_code *code)
{
	struct oplus_p9415 *chip;

	if (dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(dev);
	*code = chip->event_code;
	return 0;
}

static void p9415_check_event_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_p9415 *chip =
		container_of(dwork, struct oplus_p9415, check_event_work);

	bool od2_state;

	od2_state = p9415_get_rx_event_gpio_val(chip);

	chg_info("od2_state = %s\n", od2_state ? "true" : "false");
	if (od2_state == false) {
		msleep(50);
		od2_state = p9415_get_rx_event_gpio_val(chip);
		if (od2_state == false) {
			chg_err("OD2 is low, reread the event.\n");
			p9415_event_process(chip);
			schedule_delayed_work(&chip->check_event_work, msecs_to_jiffies(1000));
		}
	}
}

static void p9415_check_ldo_on_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_p9415 *chip =
		container_of(dwork, struct oplus_p9415, check_ldo_on_work);
	int vout = 0;
	char buf[1] = {0};
	int rc;
	bool connected = false;

	chg_info("connected_ldo_on = %s\n", chip->connected_ldo_on ? "true" : "false");
	p9415_rx_is_connected(chip->ic_dev, &connected);
	if ((!chip->connected_ldo_on) && connected) {
		chg_err("Connect but no ldo on event irq, check again.\n");
		p9415_event_process(chip);
		if (!chip->connected_ldo_on) {
			rc = p9415_read_data(chip, P9415_REG_FLAG, buf, 1);
			if (rc) {
				chg_err("Couldn't read 0x%04x, rc=%d\n", P9415_REG_FLAG, rc);
			} else {
				p9415_get_vout(chip->ic_dev, &vout);
				if ((buf[0] & P9415_LDO_FLAG) && (vout > LDO_ON_MV)) {
					oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ONLINE);
					chip->connected_ldo_on = true;
				}
			}
		}
	}
}

static void p9415_event_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_p9415 *chip =
		container_of(dwork, struct oplus_p9415, event_work);

	if (!chip->ic_dev->online) {
		chg_info("ic is not online\n");
		return;
	}
	p9415_event_process(chip);
}

static void p9415_connect_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oplus_p9415 *chip =
		container_of(dwork, struct oplus_p9415, connect_work);
	bool connected = false;
	bool pre_connected = false;

	if (!chip->ic_dev->online) {
		chg_info("ic is not online\n");
		return;
	}
	p9415_rx_is_connected(chip->ic_dev, &pre_connected);
retry:
	reinit_completion(&chip->ldo_on);
	(void)wait_for_completion_timeout(&chip->ldo_on, msecs_to_jiffies(50));
	p9415_rx_is_connected(chip->ic_dev, &connected);
	if (connected != pre_connected) {
		pre_connected = connected;
		chg_err("retry to check connect\n");
		goto retry;
	}
	if (chip->rx_connected != connected)
		chg_err("!!!!! rx_connected[%d] -> con_gpio[%d]\n", chip->rx_connected, connected);
	if (connected && chip->rx_connected == false) {
		chip->rx_connected = true;
		chip->connected_ldo_on = false;
		cancel_delayed_work_sync(&chip->check_ldo_on_work);
		schedule_delayed_work(&chip->check_ldo_on_work, P9415_CHECK_LDO_ON_DELAY);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_PRESENT);
	} else if (!connected) {
		chip->rx_connected = false;
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_PRESENT);
		chip->adapter_type = 0;
		chip->rx_pwr_cap = 0;
		chip->connected_ldo_on = false;
		chip->event_code = WLS_EVENT_RX_UNKNOWN;
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_OFFLINE);
	}
}

static irqreturn_t p9415_event_handler(int irq, void *dev_id)
{
	struct oplus_p9415 *chip = dev_id;

	chg_info("event irq\n");
	schedule_delayed_work(&chip->event_work, 0);
	return IRQ_HANDLED;
}

static irqreturn_t p9415_connect_handler(int irq, void *dev_id)
{
	struct oplus_p9415 *chip = dev_id;

	chg_info("connect irq\n");
	schedule_delayed_work(&chip->connect_work, 0);
	return IRQ_HANDLED;
}

static int p9415_gpio_init(struct oplus_p9415 *chip)
{
	int rc = 0;
	struct device_node *node = chip->dev->of_node;

	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -ENODEV;
	}

	chip->rx_event_gpio = of_get_named_gpio(node, "oplus,rx_event-gpio", 0);
	if (!gpio_is_valid(chip->rx_event_gpio)) {
		chg_err("rx_event_gpio not specified\n");
		return -ENODEV;
	}
	rc = gpio_request(chip->rx_event_gpio, "rx_event-gpio");
	if (rc < 0) {
		chg_err("rx_event_gpio request error, rc=%d\n", rc);
		return rc;
	}
	chip->rx_event_default = pinctrl_lookup_state(chip->pinctrl, "rx_event_default");
	if (IS_ERR_OR_NULL(chip->rx_event_default)) {
		chg_err("get rx_event_default fail\n");
		goto free_event_gpio;
	}
	gpio_direction_input(chip->rx_event_gpio);
	pinctrl_select_state(chip->pinctrl, chip->rx_event_default);
	chip->rx_event_irq = gpio_to_irq(chip->rx_event_gpio);
	rc = devm_request_irq(chip->dev, chip->rx_event_irq, p9415_event_handler,
			IRQF_TRIGGER_FALLING, "rx_event_irq", chip);
	if (rc < 0) {
		chg_err("rx_event_irq request error, rc=%d\n", rc);
		goto free_event_gpio;
	}
	enable_irq_wake(chip->rx_event_irq);

	chip->rx_con_gpio = of_get_named_gpio(node, "oplus,rx_con-gpio", 0);
	if (!gpio_is_valid(chip->rx_con_gpio)) {
		chg_err("rx_con_gpio not specified\n");
		goto disable_event_irq;
	}
	rc = gpio_request(chip->rx_con_gpio, "rx_con-gpio");
	if (rc < 0) {
		chg_err("rx_con_gpio request error, rc=%d\n", rc);
		goto disable_event_irq;
	}
	chip->rx_con_default = pinctrl_lookup_state(chip->pinctrl, "rx_con_default");
	if (IS_ERR_OR_NULL(chip->rx_con_default)) {
		chg_err("get idt_con_default fail\n");
		goto free_con_gpio;
	}
	gpio_direction_input(chip->rx_con_gpio);
	pinctrl_select_state(chip->pinctrl, chip->rx_con_default);
	chip->rx_con_irq = gpio_to_irq(chip->rx_con_gpio);
	rc = devm_request_irq(chip->dev, chip->rx_con_irq, p9415_connect_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"rx_con_irq", chip);
	if (rc < 0) {
		chg_err("rx_con_irq request error, rc=%d\n", rc);
		goto free_con_gpio;
	}
	enable_irq_wake(chip->rx_con_irq);

	chip->rx_en_gpio = of_get_named_gpio(node, "oplus,rx_en-gpio", 0);
	if (!gpio_is_valid(chip->rx_en_gpio)) {
		chg_err("rx_en_gpio not specified\n");
		goto disable_con_irq;
	}
	rc = gpio_request(chip->rx_en_gpio, "rx_en-gpio");
	if (rc < 0) {
		chg_err("rx_en_gpio request error, rc=%d\n", rc);
		goto disable_con_irq;
	}
	chip->rx_en_active = pinctrl_lookup_state(chip->pinctrl, "rx_en_active");
	if (IS_ERR_OR_NULL(chip->rx_en_active)) {
		chg_err("get rx_en_active fail\n");
		goto free_en_gpio;
	}
	chip->rx_en_sleep = pinctrl_lookup_state(chip->pinctrl, "rx_en_sleep");
	if (IS_ERR_OR_NULL(chip->rx_en_sleep)) {
		chg_err("get rx_en_sleep fail\n");
		goto free_en_gpio;
	}
	gpio_direction_output(chip->rx_en_gpio, 0);
	pinctrl_select_state(chip->pinctrl, chip->rx_en_active);

	chip->mode_sw_gpio = of_get_named_gpio(node, "oplus,mode_sw-gpio", 0);
	if (!gpio_is_valid(chip->mode_sw_gpio)) {
		chg_err("mode_sw_gpio not specified\n");
		goto exit;
	}
	rc = gpio_request(chip->mode_sw_gpio, "mode_sw-gpio");
	if (rc < 0) {
		chg_err("mode_sw_gpio request error, rc=%d\n", rc);
		goto free_en_gpio;
	}
	chip->mode_sw_active = pinctrl_lookup_state(chip->pinctrl, "mode_sw_active");
	if (IS_ERR_OR_NULL(chip->mode_sw_active)) {
		chg_err("get mode_sw_active fail\n");
		goto free_mode_sw_gpio;
	}
	chip->mode_sw_sleep = pinctrl_lookup_state(chip->pinctrl, "mode_sw_sleep");
	if (IS_ERR_OR_NULL(chip->mode_sw_sleep)) {
		pr_err("get mode_sw_sleep fail\n");
		goto free_mode_sw_gpio;
	}
	gpio_direction_output(chip->mode_sw_gpio, 0);
	pinctrl_select_state(chip->pinctrl, chip->mode_sw_sleep);

exit:
	return 0;

free_mode_sw_gpio:
	if (!gpio_is_valid(chip->mode_sw_gpio))
		gpio_free(chip->mode_sw_gpio);
free_en_gpio:
	if (!gpio_is_valid(chip->rx_en_gpio))
		gpio_free(chip->rx_en_gpio);
disable_con_irq:
	disable_irq(chip->rx_con_irq);
free_con_gpio:
	if (!gpio_is_valid(chip->rx_con_gpio))
		gpio_free(chip->rx_con_gpio);
disable_event_irq:
	disable_irq(chip->rx_event_irq);
free_event_gpio:
	if (!gpio_is_valid(chip->rx_event_gpio))
		gpio_free(chip->rx_event_gpio);
	return rc;
}

static void p9415_shutdown(struct i2c_client *client)
{
	struct oplus_p9415 *chip = i2c_get_clientdata(client);
	bool is_connected = false;
	int wait_wpc_disconn_cnt = 0;

	disable_irq(chip->rx_con_irq);
	disable_irq(chip->rx_event_irq);

	p9415_rx_is_connected(chip->ic_dev, &is_connected);
	if (is_connected &&
	    (p9415_get_wls_type(chip) == OPLUS_CHG_WLS_VOOC ||
	     p9415_get_wls_type(chip) == OPLUS_CHG_WLS_SVOOC ||
	     p9415_get_wls_type(chip) == OPLUS_CHG_WLS_PD_65W)) {
		p9415_set_rx_enable(chip->ic_dev, false);
		msleep(100);
		while (wait_wpc_disconn_cnt < 10) {
			p9415_rx_is_connected(chip->ic_dev, &is_connected);
			if (!is_connected)
				break;
			msleep(150);
			wait_wpc_disconn_cnt++;
		}
	}
	return;
}

static struct regmap_config p9415_regmap_config = {
	.reg_bits	= 16,
	.val_bits	= 8,
	.max_register	= 0xFFFF,
};

static int p9415_init(struct oplus_chg_ic_dev *ic_dev)
{
	ic_dev->online = true;
	if (g_p9415_chip) {
		schedule_delayed_work(&g_p9415_chip->connect_work, 0);
		schedule_delayed_work(&g_p9415_chip->event_work, msecs_to_jiffies(500));
	}
	return 0;
}

static int p9415_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (!ic_dev->online)
		return 0;
	ic_dev->online = false;

	return 0;
}

static int p9415_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int p9415_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static void *oplus_chg_get_func(struct oplus_chg_ic_dev *ic_dev,
				enum oplus_chg_ic_func func_id)
{
	void *func = NULL;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT)) {
		chg_err("%s is offline\n", ic_dev->name);
		return NULL;
	}

	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT,
			p9415_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
			p9415_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP,
			p9415_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST,
			p9415_smt_test);
		break;
	case OPLUS_IC_FUNC_RX_SET_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_ENABLE,
			p9415_set_rx_enable);
		break;
	case OPLUS_IC_FUNC_RX_IS_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_IS_ENABLE,
			p9415_rx_is_enable);
		break;
	case OPLUS_IC_FUNC_RX_IS_CONNECTED:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_IS_CONNECTED,
			p9415_rx_is_connected);
		break;
	case OPLUS_IC_FUNC_RX_GET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_VOUT,
			p9415_get_vout);
		break;
	case OPLUS_IC_FUNC_RX_SET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_VOUT,
			p9415_set_vout);
		break;
	case OPLUS_IC_FUNC_RX_GET_VRECT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_VRECT,
			p9415_get_vrect);
		break;
	case OPLUS_IC_FUNC_RX_GET_IOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_IOUT,
			p9415_get_iout);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_VOL:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_VOL,
			p9415_get_tx_vout);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_CURR:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_CURR,
			p9415_get_tx_iout);
		break;
	case OPLUS_IC_FUNC_RX_GET_CEP_COUNT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_CEP_COUNT,
			p9415_get_cep_count);
		break;
	case OPLUS_IC_FUNC_RX_GET_CEP_VAL:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_CEP_VAL,
			p9415_get_cep_val);
		break;
	case OPLUS_IC_FUNC_RX_GET_WORK_FREQ:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_WORK_FREQ,
			p9415_get_work_freq);
		break;
	case OPLUS_IC_FUNC_RX_GET_RX_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_RX_MODE,
			p9415_get_rx_mode);
		break;
	case OPLUS_IC_FUNC_RX_SET_RX_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_RX_MODE,
			p9415_set_rx_mode);
		break;
	case OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE,
			p9415_set_dcdc_enable);
		break;
	case OPLUS_IC_FUNC_RX_SET_TRX_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_TRX_ENABLE,
			p9415_set_tx_enable);
		break;
	case OPLUS_IC_FUNC_RX_SET_TRX_START:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_TRX_START,
			p9415_set_tx_start);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_STATUS,
			p9415_get_tx_status);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_ERR:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_ERR,
			p9415_get_tx_err);
		break;
	case OPLUS_IC_FUNC_RX_GET_HEADROOM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_HEADROOM,
			p9415_get_headroom);
		break;
	case OPLUS_IC_FUNC_RX_SET_HEADROOM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_HEADROOM,
			p9415_set_headroom);
		break;
	case OPLUS_IC_FUNC_RX_SEND_MATCH_Q:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SEND_MATCH_Q,
			p9415_send_match_q);
		break;
	case OPLUS_IC_FUNC_RX_SET_FOD_PARM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_FOD_PARM,
			p9415_set_fod_parm);
		break;
	case OPLUS_IC_FUNC_RX_SEND_MSG:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SEND_MSG,
			p9415_send_msg);
		break;
	case OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK,
			p9415_register_msg_callback);
		break;
	case OPLUS_IC_FUNC_RX_GET_RX_VERSION:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_RX_VERSION,
			p9415_get_rx_version);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_VERSION:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_VERSION,
			p9415_get_tx_version);
		break;
	case OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF,
			p9415_upgrade_firmware_by_buf);
		break;
	case OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG,
			p9415_upgrade_firmware_by_img);
		break;
	case OPLUS_IC_FUNC_RX_CHECK_CONNECT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_CHECK_CONNECT,
			p9415_connect_check);
		break;
	case OPLUS_IC_FUNC_RX_GET_EVENT_CODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_EVENT_CODE,
			p9415_get_event_code);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

struct oplus_chg_ic_virq p9415_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_PRESENT },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
	{ .virq_id = OPLUS_IC_VIRQ_EVENT_CHANGED },
};

static int p9415_driver_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct oplus_p9415 *chip;
	struct device_node *node = client->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = {0};
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	int rc = 0;

	chg_info("call !\n");
	chip = devm_kzalloc(&client->dev, sizeof(struct oplus_p9415), GFP_KERNEL);
	if (!chip) {
		chg_err(" kzalloc() failed\n");
		return -ENOMEM;
	}

	chip->dev = &client->dev;
	chip->regmap = devm_regmap_init_i2c(client, &p9415_regmap_config);
	if (!chip->regmap)
		return -ENODEV;
	chip->client = client;
	i2c_set_clientdata(client, chip);

	chip->support_epp_11w = of_property_read_bool(node, "oplus,support_epp_11w");

	chip->fw_data = (char *)of_get_property(node, "oplus,fw_data", &chip->fw_length);
	if (!chip->fw_data) {
		chg_err("parse fw data failed\n");
		chip->fw_data = p9415_fw_data;
		chip->fw_length = sizeof(p9415_fw_data);
	} else {
		chg_info("parse fw data length[%d]\n", chip->fw_length);
	}

	rc = of_property_read_u32(node, "oplus,ic_type", &ic_type);
	if (rc < 0) {
		chg_err("can't get ic type, rc=%d\n", rc);
		goto reg_ic_err;
	}
	rc = of_property_read_u32(node, "oplus,ic_index", &ic_index);
	if (rc < 0) {
		chg_err("can't get ic index, rc=%d\n", rc);
		goto reg_ic_err;
	}
	ic_cfg.name = node->name;
	ic_cfg.type = ic_type;
	ic_cfg.index = ic_index;
	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "rx-p9415");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.virq_data = p9415_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(p9415_virq_table);
	ic_cfg.get_func = oplus_chg_get_func;
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}
	chip->ic_dev->type = ic_type;

	rc = p9415_gpio_init(chip);
	if (rc < 0) {
		chg_err("p9415 gpio init error.\n");
		goto gpio_init_err;
	}

	p9415_track_debugfs_init(chip);

	device_init_wakeup(chip->dev, true);

	INIT_DELAYED_WORK(&chip->event_work, p9415_event_work);
	INIT_DELAYED_WORK(&chip->connect_work, p9415_connect_work);
	INIT_DELAYED_WORK(&chip->check_ldo_on_work, p9415_check_ldo_on_work);
	INIT_DELAYED_WORK(&chip->check_event_work, p9415_check_event_work);
	mutex_init(&chip->i2c_lock);
	init_completion(&chip->ldo_on);
	schedule_delayed_work(&chip->connect_work, 0);
#ifdef WLS_QI_DEBUG
	rc = sysfs_create_group(&chip->dev->kobj, &p9415_attribute_group);
	if (rc < 0)
		chg_err("sysfs_create_group error fail\n");
#endif
	g_p9415_chip = chip;
	chg_info("call end!\n");
	return 0;

gpio_init_err:
	devm_oplus_chg_ic_unregister(chip->dev, chip->ic_dev);
reg_ic_err:
	i2c_set_clientdata(client, NULL);
	return rc;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
static int p9415_driver_remove(struct i2c_client *client)
#else
static void p9415_driver_remove(struct i2c_client *client)
#endif
{
	struct oplus_p9415 *chip = i2c_get_clientdata(client);

	if (!gpio_is_valid(chip->rx_en_gpio))
		gpio_free(chip->rx_en_gpio);
	disable_irq(chip->rx_con_irq);
	if (!gpio_is_valid(chip->rx_con_gpio))
		gpio_free(chip->rx_con_gpio);
	disable_irq(chip->rx_event_irq);
	if (!gpio_is_valid(chip->rx_event_gpio))
		gpio_free(chip->rx_event_gpio);
	devm_oplus_chg_ic_unregister(chip->dev, chip->ic_dev);
	i2c_set_clientdata(client, NULL);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
	return 0;
#else
	return;
#endif
}

static const struct of_device_id p9415_match[] = {
	{ .compatible = "oplus,rx-p9415" },
	{},
};

static const struct i2c_device_id p9415_id_table[] = {
	{ "oplus,rx-p9415", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, p9415_id_table);

static struct i2c_driver p9415_driver = {
	.driver = {
		.name = "rx-p9415",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(p9415_match),
	},
	.probe = p9415_driver_probe,
	.remove = p9415_driver_remove,
	.shutdown = p9415_shutdown,
	.id_table = p9415_id_table,
};

static __init int p9415_driver_init(void)
{
	return i2c_add_driver(&p9415_driver);
}

static __exit void p9415_driver_exit(void)
{
	i2c_del_driver(&p9415_driver);
}

oplus_chg_module_register(p9415_driver);
