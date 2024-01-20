// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[TPS6128XD]: %s[%d]: " fmt, __func__, __LINE__

#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/ktime.h>

#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_chg.h>

#include "test-kit.h"

static int tps6128xd_debug_track = 0;
module_param(tps6128xd_debug_track, int, 0644);
MODULE_PARM_DESC(tps6128xd_debug_track, "debug track");

#define CONFIG_REG			0x01
#define VOUTFLOORSET_REG		0x02
#define VOUTROOFSET_REG			0x03
#define ILIMSET_REG			0x04
#define STATUS_REG			0x05
#define E2PROMCTRL_REG			0xFF
#define TPS6128XD_REG_CNT		6

#define VOUT_MV_MIN			2850
#define VOUT_MV_MAX			4400
#define VOUT_MV_STEP			50
#define VOUT_REG_MIN			0x00
#define VOUT_REG_MAX			0x1F

#define DEFAULT_VOUT_MV			3000
#define HIGH_VOUT_MV			4400

#define DEFAULT_VOUTFLOORSET_VAL	0x03
#define DEFAULT_ILIMSET_VAL		0x1F

#define STATUS_MASK			0x12
#define BOOST_STATUS_NORMAL		0x10
#define BYPASS_STATUS_NORMAL		0x00

enum {
	BYB_STATUS_FAULT = 0,
	BYB_STATUS_BOOST,
	BYB_STATUS_BYPASS,
};

static const char *byb_status_name[] = {
	[BYB_STATUS_FAULT] = "fault",
	[BYB_STATUS_BOOST] = "boost",
	[BYB_STATUS_BYPASS] = "bypass",
};

enum {
	GPIO_STATUS_NC,
	GPIO_STATUS_PD,
	GPIO_STATUS_PU,
	GPIO_STATUS_NOT_SUPPORT,
	GPIO_STATUS_MAX = GPIO_STATUS_NOT_SUPPORT,
};

static const char *gpio_status_name[] = {
	[GPIO_STATUS_NC] = "not connect",
	[GPIO_STATUS_PD] = "pull down",
	[GPIO_STATUS_PU] = "pull up",
	[GPIO_STATUS_NOT_SUPPORT] = "not support",
};

struct chip_tps6128xd {
	struct device *dev;
	struct i2c_client *client;
	struct oplus_chg_ic_dev *ic_dev;
	struct regmap *regmap;
	struct mutex pinctrl_lock;
	struct pinctrl *pinctrl;
	struct pinctrl_state *id_not_pull;
	struct pinctrl_state *id_pull_up;
	struct pinctrl_state *id_pull_down;
	int id_gpio;
	int id_match_status;
	int vout_mv;
	bool i2c_success;
	int probe_gpio_status;

	atomic_t suspended;
#if IS_ENABLED(CONFIG_OPLUS_CHG_TEST_KIT)
	struct test_feature *boost_id_gpio_test;
#endif
};

static bool tps6128xd_is_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CONFIG_REG:
	case VOUTFLOORSET_REG:
	case VOUTROOFSET_REG:
	case ILIMSET_REG:
	case E2PROMCTRL_REG:
		return true;
	default:
		return false;
	}
}

static bool tps6128xd_is_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CONFIG_REG:
	case VOUTFLOORSET_REG:
	case VOUTROOFSET_REG:
	case ILIMSET_REG:
	case STATUS_REG:
	case E2PROMCTRL_REG:
		return true;
	default:
		return false;
	}
}

static struct regmap_config tps6128xd_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.writeable_reg = tps6128xd_is_writeable_reg,
	.readable_reg = tps6128xd_is_readable_reg,
	.max_register = E2PROMCTRL_REG,
};

static int vout_mv_to_reg(int mv)
{
	if (mv < VOUT_MV_MIN)
		return VOUT_REG_MIN;
	if (mv > VOUT_MV_MAX)
		return VOUT_REG_MAX;
	return (mv - VOUT_MV_MIN) / VOUT_MV_STEP + VOUT_REG_MIN;
}

static int reg_to_vout_mv(int reg)
{
	int data = reg & VOUT_REG_MAX;

	return (data - VOUT_REG_MIN) * VOUT_MV_STEP + VOUT_MV_MIN;
}

static int tps6128xd_parse_dt(struct chip_tps6128xd *chip)
{
	struct device_node *node = chip->dev->of_node;
	int rc;

	rc = of_property_read_u32(node, "oplus,vout-mv", &chip->vout_mv);
	if (rc < 0) {
		chg_err("oplus,vout-mv read failed, rc=%d\n", rc);
		chip->vout_mv = DEFAULT_VOUT_MV;
	}

	rc = of_property_read_u32(node, "oplus,id-match-status", &chip->id_match_status);
	if (rc < 0) {
		chg_err("oplus,id-match-status read failed, rc=%d\n", rc);
		chip->id_match_status = GPIO_STATUS_NOT_SUPPORT;
	}

	chg_info("vout_mv=%d,id_match_status=%d\n", chip->vout_mv, chip->id_match_status);
	return 0;
}

static int tps6128xd_get_id_status(struct chip_tps6128xd *chip)
{
	int value_up = 0, value_down = 0;
	int gpio_value = GPIO_STATUS_NOT_SUPPORT;

	if (!chip || !gpio_is_valid(chip->id_gpio) ||
	    IS_ERR_OR_NULL(chip->pinctrl) || IS_ERR_OR_NULL(chip->id_not_pull) ||
	    IS_ERR_OR_NULL(chip->id_pull_down) || IS_ERR_OR_NULL(chip->id_pull_up)) {
		return GPIO_STATUS_NOT_SUPPORT;
	}

	mutex_lock(&chip->pinctrl_lock);
	gpio_direction_input(chip->id_gpio);
	pinctrl_select_state(chip->pinctrl, chip->id_pull_up);
	usleep_range(10000, 10000);
	value_up = gpio_get_value(chip->id_gpio);

	pinctrl_select_state(chip->pinctrl, chip->id_pull_down);
	usleep_range(10000, 10000);
	value_down = gpio_get_value(chip->id_gpio);

	pinctrl_select_state(chip->pinctrl, chip->id_not_pull);

	if (value_up == 1 && value_down == 0)
		gpio_value = GPIO_STATUS_NC;
	else if (value_up == 0 && value_down == 0)
		gpio_value = GPIO_STATUS_PD;
	else if (value_up == 1 && value_down == 1)
		gpio_value = GPIO_STATUS_PU;
	chg_info("value_up=%d value_down=%d\n", value_up, value_down);
	mutex_unlock(&chip->pinctrl_lock);

	return gpio_value;
}

static int tps6128xd_gpio_init(struct chip_tps6128xd *chip)
{
	struct device_node *node = chip->dev->of_node;
	int rc = 0;

	chip->id_gpio = of_get_named_gpio(node, "oplus,id-gpio", 0);
	if (!gpio_is_valid(chip->id_gpio)) {
		chg_err("id gpio not specified\n");
		return -EINVAL;
	}
	rc = gpio_request(chip->id_gpio, "tps6128xd-id-gpio");
	if (rc < 0) {
		chg_err("tps6128xd-id gpio request error, rc=%d\n", rc);
		return rc;
	}

	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -ENODEV;
	}

	chip->id_not_pull = pinctrl_lookup_state(chip->pinctrl, "id_not_pull");
	if (IS_ERR_OR_NULL(chip->id_not_pull)) {
		chg_err("get id_not_pull fail\n");
		goto free_id_gpio;
	}

	chip->id_pull_up = pinctrl_lookup_state(chip->pinctrl, "id_pull_up");
	if (IS_ERR_OR_NULL(chip->id_pull_up)) {
		chg_err("get id_pull_up fail\n");
		goto free_id_gpio;
	}
	chip->id_pull_down = pinctrl_lookup_state(chip->pinctrl, "id_pull_down");
	if (IS_ERR_OR_NULL(chip->id_pull_down)) {
		chg_err("get id_pull_down fail\n");
		goto free_id_gpio;
	}

	chip->probe_gpio_status = tps6128xd_get_id_status(chip);
	chg_info("id gpio value %d %s\n", chip->probe_gpio_status, gpio_status_name[chip->probe_gpio_status]);
	return 0;

free_id_gpio:
	if (!gpio_is_valid(chip->id_gpio))
		gpio_free(chip->id_gpio);
	return rc;
}

static int tps6128xd_hardware_init(struct chip_tps6128xd *chip)
{
	int rc = 0;
	u8 buf[TPS6128XD_REG_CNT + 3];

	rc = regmap_write(chip->regmap, VOUTFLOORSET_REG, DEFAULT_VOUTFLOORSET_VAL);
	if (rc < 0)
		chg_err("write voutfloor fail, rc=%d\n", rc);

	rc = regmap_write(chip->regmap, VOUTROOFSET_REG, vout_mv_to_reg(chip->vout_mv));
	if (rc < 0)
		chg_err("write voutroof fail, rc=%d\n", rc);

	rc = regmap_write(chip->regmap, ILIMSET_REG, DEFAULT_ILIMSET_VAL);
	if (rc < 0)
		chg_err("write ilim fail, rc=%d\n", rc);

	rc = regmap_read(chip->regmap, CONFIG_REG, (unsigned int *)&buf[0]);
	if (rc < 0)
		chg_err("read config register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, VOUTFLOORSET_REG, (unsigned int *)&buf[1]);
	if (rc < 0)
		chg_err("read voutfloor register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, VOUTROOFSET_REG, (unsigned int *)&buf[2]);
	if (rc < 0)
		chg_err("read voutroof register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, ILIMSET_REG, (unsigned int *)&buf[3]);
	if (rc < 0)
		chg_err("read ilim register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, STATUS_REG, (unsigned int *)&buf[4]);
	if (rc < 0)
		chg_err("read status register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, E2PROMCTRL_REG, (unsigned int *)&buf[5]);
	if (rc < 0)
		chg_err("read e2promctrl register fail, rc=%d", rc);


	if (rc >= 0 && (buf[1] == DEFAULT_VOUTFLOORSET_VAL) &&
	    (buf[2] == vout_mv_to_reg(chip->vout_mv)) &&
	    (buf[3] == DEFAULT_ILIMSET_VAL))
		chip->i2c_success = true;
	else
		chip->i2c_success = false;
	chg_err("i2c %s reg=%*ph\n", chip->i2c_success ? "success" : "fail", TPS6128XD_REG_CNT, buf);
	return 0;
}

struct oplus_chg_ic_virq tps6128xd_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
};

#define TRACK_UPLOAD_COUNT_MAX 10
#define TRACK_LOCAL_T_NS_TO_S_THD 1000000000
#define TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD (24 * 3600)
static int tps6128xd_push_err(struct oplus_chg_ic_dev *ic_dev,
				   bool i2c_error, int err_code, char *reg, bool fault)
{
	static int upload_count = 0;
	static int pre_upload_time = 0;
	int curr_time;

	curr_time = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;
	if (curr_time - pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		upload_count = 0;

	if (upload_count >= TRACK_UPLOAD_COUNT_MAX)
		return 0;

	pre_upload_time = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;

	if (i2c_error)
		oplus_chg_ic_creat_err_msg(ic_dev, OPLUS_IC_ERR_BUCK_BOOST, 0,
			"$$err_scene@@i2c_err$$err_reason@@%d", err_code);
	else
		oplus_chg_ic_creat_err_msg(ic_dev, OPLUS_IC_ERR_BUCK_BOOST, 0,
			"$$err_scene@@byb_work_err$$err_reason@@%s$$reg_info@@%s",
			fault ? "fault" : "power_not_good", reg);

	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_ERR);
	upload_count++;
	return 0;
}

static int tps6128xd_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct chip_tps6128xd *chip;
	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(ic_dev);

	if (chip->probe_gpio_status == chip->id_match_status || chip->i2c_success)
		ic_dev->online = true;
	else
		ic_dev->online = false;
	return 0;
}

static int tps6128xd_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;
	return 0;
}

#define REG_INFO_LEN 128
static int tps6128xd_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	struct chip_tps6128xd *chip;
	u8 buf[TPS6128XD_REG_CNT + 3];
	int rc;
	char reg_info[REG_INFO_LEN] = { 0 };

	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL\n");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(ic_dev);

	if (!ic_dev->online)
		return 0;

	if(atomic_read(&chip->suspended) == 1) {
		chg_err("in suspended\n");
		return 0;
	}

	rc = regmap_read(chip->regmap, CONFIG_REG, (unsigned int *)&buf[0]);
	if (rc < 0)
		chg_err("read config register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, VOUTFLOORSET_REG, (unsigned int *)&buf[1]);
	if (rc < 0)
		chg_err("read voutfloor register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, VOUTROOFSET_REG, (unsigned int *)&buf[2]);
	if (rc < 0)
		chg_err("read voutroof register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, ILIMSET_REG, (unsigned int *)&buf[3]);
	if (rc < 0)
		chg_err("read ilim register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, STATUS_REG, (unsigned int *)&buf[4]);
	if (rc < 0)
		chg_err("read status register fail, rc=%d", rc);
	rc = regmap_read(chip->regmap, E2PROMCTRL_REG, (unsigned int *)&buf[5]);
	if (rc < 0)
		chg_err("read e2promctrl register fail, rc=%d", rc);

	chg_err("%*ph\n", TPS6128XD_REG_CNT, buf);

	if (rc < 0 || (buf[4] & 0x3) != 0x1 || tps6128xd_debug_track) {
		snprintf(reg_info, REG_INFO_LEN, "reg01~05,ff:[%*ph]", TPS6128XD_REG_CNT, buf);
		tps6128xd_push_err(ic_dev, rc < 0, rc, reg_info, buf[4] & 2);
	}
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
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT, tps6128xd_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT, tps6128xd_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP, tps6128xd_reg_dump);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

#if IS_ENABLED(CONFIG_OPLUS_CHG_TEST_KIT)
static bool test_kit_boost_id_gpio_test(struct test_feature *feature, char *buf, size_t len)
{
	struct chip_tps6128xd *chip;
	int index = 0;
	int gpio_status = GPIO_STATUS_NOT_SUPPORT;

	if (buf == NULL) {
		pr_err("buf is NULL\n");
		return false;
	}
	if (feature == NULL) {
		pr_err("feature is NULL\n");
		index += snprintf(buf + index, len - index, "feature is NULL");
		return false;
	}

	chip = feature->private_data;
	gpio_status = tps6128xd_get_id_status(chip);
	index += snprintf(buf + index, len - index, "%s\n", gpio_status_name[gpio_status]);
	return true;
}

static const struct test_feature_cfg boost_id_gpio_test_cfg = {
	.name = "boost_id_gpio_test",
	.test_func = test_kit_boost_id_gpio_test,
};
#endif

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
static ssize_t byb_status_show(struct device *dev, struct device_attribute *attr,
				    char *buf)
{
	struct oplus_chg_ic_dev *ic_dev = dev_get_drvdata(dev);
	struct chip_tps6128xd *chip = oplus_chg_ic_get_drvdata(ic_dev);
	int reg_val = 0;
	int size = 0;
	int rc = 0;
	int status = BYB_STATUS_FAULT;
	int gpio_status = GPIO_STATUS_NOT_SUPPORT;

	if (!ic_dev->online)
		return -ENOTSUPP;

	if(atomic_read(&chip->suspended) == 1) {
		chg_err("in suspended\n");
		size += snprintf(buf + size, PAGE_SIZE - size, "in suspended|in suspended\n");
		return size;
	}

	gpio_status = tps6128xd_get_id_status(chip);
	if (gpio_status != chip->id_match_status) {
		chg_err("id not match %d %d,", gpio_status, chip->id_match_status);
		size += snprintf(buf + size, PAGE_SIZE - size,
			"id not match %d %d,\n", gpio_status, chip->id_match_status);
	}

	rc = regmap_read(chip->regmap, STATUS_REG, &reg_val);
	if (rc < 0) {
		chg_err("can't read 0x%02x register, rc=%d", STATUS_REG, rc);
		size += snprintf(buf + size, PAGE_SIZE - size,
				"0x%02x read fail:%d|0x%02x read fail:%d\n",
				STATUS_REG, rc, STATUS_REG, rc);
		return size;
	}

	if ((reg_val & STATUS_MASK) == BOOST_STATUS_NORMAL)
		status = BYB_STATUS_BOOST;
	else if ((reg_val & STATUS_MASK) == BYPASS_STATUS_NORMAL)
		status = BYB_STATUS_BYPASS;

	size += snprintf(buf + size, PAGE_SIZE - size,
			"%s|0x%02x:0x%02x", byb_status_name[status], STATUS_REG, reg_val);

	rc = regmap_read(chip->regmap, VOUTROOFSET_REG, &reg_val);
	if (rc < 0) {
		chg_err("can't read 0x%02x register, rc=%d", VOUTROOFSET_REG, rc);
		size += snprintf(buf + size, PAGE_SIZE - size, "0x%02x read fail:%d\n", VOUTROOFSET_REG, rc);
		return size;
	}

	size += snprintf(buf + size, PAGE_SIZE - size, ",vout:%dmv\n", reg_to_vout_mv(reg_val));

	return size;
}
static DEVICE_ATTR_RO(byb_status);

static ssize_t byb_vout_show(struct device *dev, struct device_attribute *attr,
				    char *buf)
{
	struct oplus_chg_ic_dev *ic_dev = dev_get_drvdata(dev);
	struct chip_tps6128xd *chip = oplus_chg_ic_get_drvdata(ic_dev);
	int reg_val = 0;
	int size = 0;
	int rc = 0;
	int vout = 0;

	if (!ic_dev->online)
		return -ENOTSUPP;

	if(atomic_read(&chip->suspended) == 1) {
		chg_err("in suspended\n");
		size += snprintf(buf + size, PAGE_SIZE - size, "in suspended\n");
		return size;
	}

	rc = regmap_read(chip->regmap, VOUTROOFSET_REG, &reg_val);
	if (rc < 0) {
		chg_err("can't read 0x%02x register, rc=%d", VOUTROOFSET_REG, rc);
		size += snprintf(buf + size, PAGE_SIZE - size, "0x%02x read fail:%d\n", VOUTROOFSET_REG, rc);
		return size;
	}

	vout = reg_to_vout_mv(reg_val);
	size += snprintf(buf + size, PAGE_SIZE - size, "%d\n", vout);
	return size;
}

static ssize_t byb_vout_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct oplus_chg_ic_dev *ic_dev = dev_get_drvdata(dev);
	struct chip_tps6128xd *chip = oplus_chg_ic_get_drvdata(ic_dev);
	int val = 0;
	int rc = 0;

	if (!ic_dev->online)
		return -ENOTSUPP;

	if (kstrtos32(buf, 0, &val)) {
		chg_err("buf error\n");
		return -EINVAL;
	}

	if(atomic_read(&chip->suspended) == 1) {
		chg_err("in suspended\n");
		return -EAGAIN;
	}

	if (val == 1)
		rc = regmap_write(chip->regmap, VOUTROOFSET_REG, vout_mv_to_reg(HIGH_VOUT_MV));
	else if (val <= 0)
		rc = regmap_write(chip->regmap, VOUTROOFSET_REG, vout_mv_to_reg(chip->vout_mv));
	else
		rc = regmap_write(chip->regmap, VOUTROOFSET_REG, vout_mv_to_reg(val));
	if (rc < 0)
		chg_err("write voutroof fail, rc=%d\n", rc);

	return count;
}

static struct device_attribute dev_attr_byb_vout = {
	.attr = {
		.name = __stringify(byb_vout),
		.mode = 0666
	},
	.show = byb_vout_show,
	.store = byb_vout_store,
};

static struct device_attribute *tps6128xd_attributes[] = {
	&dev_attr_byb_status,
	&dev_attr_byb_vout,
	NULL
};
#endif

static int tps6128xd_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct chip_tps6128xd *chip;
	struct device_node *node = client->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	int rc;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	chip = devm_kzalloc(&client->dev, sizeof(struct chip_tps6128xd), GFP_KERNEL);
	if (!chip) {
		chg_err("failed to allocate chip_tps6128xd\n");
		return -ENOMEM;
	}

	chip->regmap = devm_regmap_init_i2c(client, &tps6128xd_regmap_config);
	if (!chip->regmap) {
		rc = -ENODEV;
		goto regmap_init_err;
	}

	chip->dev = &client->dev;
	chip->client = client;
	i2c_set_clientdata(client, chip);
	mutex_init(&chip->pinctrl_lock);
	atomic_set(&chip->suspended, 0);

	tps6128xd_parse_dt(chip);

	rc = tps6128xd_gpio_init(chip);
	if (rc < 0) {
		chg_err("tps6128xd gpio init failed, rc = %d!\n", rc);
	}

	rc = tps6128xd_hardware_init(chip);
	if (rc < 0) {
		chg_err("tps6128xd ic init failed, rc = %d!\n", rc);
		goto gpio_init_err;
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
	ic_cfg.index = ic_index;
	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "buck-BYB/TPS6128xD");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.type = ic_type;
	ic_cfg.get_func = oplus_chg_get_func;
	ic_cfg.virq_data = tps6128xd_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(tps6128xd_virq_table);
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	attrs = tps6128xd_attributes;
	while ((attr = *attrs++)) {
		rc = device_create_file(chip->ic_dev->debug_dev, attr);
		if (rc) {
			chg_err("device_create_file fail!\n");
			goto reg_ic_err;
		}
	}
#endif

#if IS_ENABLED(CONFIG_OPLUS_CHG_TEST_KIT)
	if (gpio_is_valid(chip->id_gpio) && (chip->probe_gpio_status == chip->id_match_status || chip->i2c_success)) {
		chip->boost_id_gpio_test = test_feature_register(&boost_id_gpio_test_cfg, chip);
		if (IS_ERR_OR_NULL(chip->boost_id_gpio_test))
			chg_err("boost_id_gpio_test register error");
		else
			chg_info("boost_id_gpio_test register success");
	}
#endif
	chg_info("success!\n");
	return 0;

reg_ic_err:
gpio_init_err:
	if (gpio_is_valid(chip->id_gpio))
		gpio_free(chip->id_gpio);
regmap_init_err:
	devm_kfree(&client->dev, chip);
	return rc;
}

static int tps6128xd_pm_resume(struct device *dev_chip)
{
	struct i2c_client *client  = container_of(dev_chip, struct i2c_client, dev);
	struct chip_tps6128xd *chip = i2c_get_clientdata(client);

	if (chip == NULL)
		return 0;

	atomic_set(&chip->suspended, 0);

	return 0;
}

static int tps6128xd_pm_suspend(struct device *dev_chip)
{
	struct i2c_client *client  = container_of(dev_chip, struct i2c_client, dev);
	struct chip_tps6128xd *chip = i2c_get_clientdata(client);

	if (chip == NULL)
		return 0;

	atomic_set(&chip->suspended, 1);

	return 0;
}

static const struct dev_pm_ops tps6128xd_pm_ops = {
	.resume = tps6128xd_pm_resume,
	.suspend = tps6128xd_pm_suspend,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
static int tps6128xd_driver_remove(struct i2c_client *client)
{
	struct chip_tps6128xd *chip = i2c_get_clientdata(client);

	if(chip == NULL)
		return 0;

#if IS_ENABLED(CONFIG_OPLUS_CHG_TEST_KIT)
	if (!IS_ERR_OR_NULL(chip->boost_id_gpio_test))
		test_feature_unregister(chip->boost_id_gpio_test);
#endif

	if (!gpio_is_valid(chip->id_gpio))
		gpio_free(chip->id_gpio);
	devm_kfree(&client->dev, chip);
	return 0;
}
#else
static void tps6128xd_driver_remove(struct i2c_client *client)
{
	struct chip_tps6128xd *chip = i2c_get_clientdata(client);

	if(chip == NULL)
		return;

	if (!gpio_is_valid(chip->id_gpio))
		gpio_free(chip->id_gpio);
	devm_kfree(&client->dev, chip);
}
#endif

static void tps6128xd_shutdown(struct i2c_client *chip_client)
{
	struct chip_tps6128xd *chip = i2c_get_clientdata(chip_client);

	if(chip == NULL)
		return;

	return;
}

static const struct of_device_id tps6128xd_match[] = {
	{.compatible = "oplus,byb-tps6128xd"},
	{},
};

static const struct i2c_device_id tps6128xd_id[] = {
	{"oplus,byb-tps6128xd", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, tps6128xd_id);


static struct i2c_driver tps6128xd_i2c_driver = {
	.driver		= {
		.name = "byb-tps6128xd",
		.owner	= THIS_MODULE,
		.of_match_table = tps6128xd_match,
		.pm = &tps6128xd_pm_ops,
	},
	.probe		= tps6128xd_driver_probe,
	.remove		= tps6128xd_driver_remove,
	.id_table	= tps6128xd_id,
	.shutdown	= tps6128xd_shutdown,
};

static __init int tps6128xd_i2c_driver_init(void)
{
#if __and(IS_BUILTIN(CONFIG_OPLUS_CHG), IS_BUILTIN(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return 0;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return 0;
#endif /* CONFIG_OPLUS_CHG_V2 */
	return i2c_add_driver(&tps6128xd_i2c_driver);
}

static __exit void tps6128xd_i2c_driver_exit(void)
{
#if (IS_ENABLED(CONFIG_OPLUS_CHG) && IS_ENABLED(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return;
#endif /* CONFIG_OPLUS_CHG_V2 */
	i2c_del_driver(&tps6128xd_i2c_driver);
}

oplus_chg_module_register(tps6128xd_i2c_driver);

MODULE_DESCRIPTION("TI Boost/Bypass Driver");
MODULE_LICENSE("GPL v2");
