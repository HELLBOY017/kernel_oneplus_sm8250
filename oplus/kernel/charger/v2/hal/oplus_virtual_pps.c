// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[VIRTUAL_PPS]([%s][%d]): " fmt, __func__, __LINE__

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
#include <linux/regmap.h>
#include <linux/list.h>
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_chg_pps.h>

#define PPS_REG_TIMEOUT_MS	120000

struct oplus_virtual_pps_child {
	struct oplus_chg_ic_dev *parent;
	struct oplus_chg_ic_dev *ic_dev;
	int index;
};

struct oplus_virtual_pps_ic {
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	struct pps_dev *pps;
	bool online;
	int child_num;
	struct oplus_virtual_pps_child *child_list;
};

static void oplus_vpps_err_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_pps_ic *chip = virq_data;

	oplus_chg_ic_move_err_msg(chip->ic_dev, ic_dev);
	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
}

static int oplus_vpps_base_virq_register(struct oplus_virtual_pps_ic *chip, int index)
{
	int rc;

	rc = oplus_chg_ic_virq_register(chip->child_list[index].ic_dev,
		OPLUS_IC_VIRQ_ERR, oplus_vpps_err_handler, chip);
	if (rc < 0 && rc != -ENOTSUPP)
		chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);
	chg_info("%s virq register success\n", chip->child_list[index].ic_dev->name);

	return 0;
}

static void oplus_vpps_child_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_virtual_pps_child *child;
	struct oplus_chg_ic_dev *parent;
	struct oplus_virtual_pps_ic *chip;
	int rc;

	if (ic == NULL) {
		chg_err("ic is NULL\n");
		return;
	}
	if (data == NULL) {
		chg_err("ic(%s) data is NULL\n", ic->name);
		return;
	}
	child = data;
	parent = child->parent;
	chip = oplus_chg_ic_get_drvdata(parent);

	if (timeout) {
		return;
	}

	child->ic_dev = ic;
	oplus_chg_ic_set_parent(ic, parent);

	rc = oplus_vpps_base_virq_register(chip, child->index);
	if (rc < 0) {
		chg_err("%s virq register error, rc=%d\n", ic->name, rc);
		return;
	}

	if (!parent->online) {
		parent->online = true;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_INIT);
	}
}

static int oplus_vpps_child_init(struct oplus_virtual_pps_ic *chip)
{
	struct device_node *node = chip->dev->of_node;
	int i;
	int rc = 0;
	const char *name;

	rc = of_property_count_elems_of_size(node, "oplus,pps_ic",
					     sizeof(u32));
	if (rc < 0) {
		chg_err("can't get cp ic number, rc=%d\n", rc);
		return rc;
	}
	chip->child_num = rc;
	chip->child_list = devm_kzalloc(
		chip->dev,
		sizeof(struct oplus_virtual_pps_child) * chip->child_num,
		GFP_KERNEL);
	if (chip->child_list == NULL) {
		rc = -ENOMEM;
		chg_err("alloc child ic memory error\n");
		return rc;
	}

	for (i = 0; i < chip->child_num; i++) {
		chip->child_list[i].index = i;
		chip->child_list[i].parent = chip->ic_dev;
		name = of_get_oplus_chg_ic_name(node, "oplus,pps_ic", i);
		rc = oplus_chg_ic_wait_ic_timeout(name, oplus_vpps_child_reg_callback, &chip->child_list[i],
						  msecs_to_jiffies(PPS_REG_TIMEOUT_MS));
		if (rc < 0) {
			chg_err("can't wait ic[%d](%s), rc=%d\n", i, name, rc);
			goto read_property_err;
		}
	}

	return 0;

read_property_err:
	for (; i >=0; i--)
		chip->child_list[i].ic_dev = NULL;
	devm_kfree(chip->dev, chip->child_list);
	return rc;
}

static int oplus_chg_pps_init(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	ic_dev->online = true;

	return 0;
}

static int oplus_chg_pps_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;

	chg_info("unregister success\n");

	return 0;
}

static int oplus_chg_pps_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int oplus_chg_pps_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static int oplus_chg_pps_pdo_set(struct oplus_chg_ic_dev *ic_dev, int vol_mv, int curr_ma)
{
	struct oplus_virtual_pps_ic *vpps;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpps = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpps->child_num; i++) {
		rc = oplus_chg_ic_func(vpps->child_list[i].ic_dev, OPLUS_IC_FUNC_PPS_PDO_SET, vol_mv, curr_ma);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] pdo_set error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_fixed_pdo_set(struct oplus_chg_ic_dev *ic_dev, int vol_mv, int curr_ma)
{
	struct oplus_virtual_pps_ic *vpps;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpps = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpps->child_num; i++) {
		rc = oplus_chg_ic_func(vpps->child_list[i].ic_dev, OPLUS_IC_FUNC_FIXED_PDO_SET, vol_mv, curr_ma);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] pdo_set error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_pps_hard_reset(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int oplus_chg_pps_exit_pps_mode(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int oplus_chg_pps_config_wd(struct oplus_chg_ic_dev *ic_dev, u16 time_ms)
{
	return 0;
}

static int oplus_chg_pps_get_dev_info(struct oplus_chg_ic_dev *ic_dev, u64 *dev_info)
{
	return 0;
}

static int oplus_chg_pps_get_err_info(struct oplus_chg_ic_dev *ic_dev, u64 *err_info)
{
	return 0;
}

static int oplus_chg_pps_get_cable_info(struct oplus_chg_ic_dev *ic_dev, u64 *cable_info)
{
	return 0;
}

static int oplus_chg_pps_get_pdo_info(struct oplus_chg_ic_dev *ic_dev, u32 *pdo, int num)
{
	struct oplus_virtual_pps_ic *vpps;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpps = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpps->child_num; i++) {
		rc = oplus_chg_ic_func(vpps->child_list[i].ic_dev, OPLUS_IC_FUNC_PPS_GET_PDO_INFO, pdo, num);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] get_pdo_info error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_get_pps_status(struct oplus_chg_ic_dev *ic_dev, u32 *src_info)
{
	struct oplus_virtual_pps_ic *vpps;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}


	vpps = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpps->child_num; i++) {
		rc = oplus_chg_ic_func(vpps->child_list[i].ic_dev, OPLUS_IC_FUNC_GET_PPS_STATUS, src_info);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] get_pps_status error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}
static int oplus_chg_pps_verify_adapter(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_pps_ic *vpps;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}


	vpps = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpps->child_num; i++) {
		rc = oplus_chg_ic_func(vpps->child_list[i].ic_dev, OPLUS_IC_FUNC_PPS_VERIFY_ADAPTER);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] pps_verify_adapter error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_pps_get_power_change_info(
	struct oplus_chg_ic_dev *ic_dev, u32 *pwr_change_info, int num)
{
	return 0;
}

static void *oplus_chg_pps_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
{
	void *func = NULL;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT)) {
		chg_err("%s is offline\n", ic_dev->name);
		return NULL;
	}
	if (!oplus_chg_ic_func_is_support(ic_dev, func_id)) {
		chg_info("%s: this func(=%d) is not supported\n",  ic_dev->name, func_id);
		return NULL;
	}

	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT,
			oplus_chg_pps_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
			oplus_chg_pps_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP,
			oplus_chg_pps_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST,
			oplus_chg_pps_smt_test);
		break;
	case OPLUS_IC_FUNC_PPS_PDO_SET:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_PDO_SET,
			oplus_chg_pps_pdo_set);
		break;
	case OPLUS_IC_FUNC_FIXED_PDO_SET:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_FIXED_PDO_SET,
			oplus_chg_fixed_pdo_set);
		break;
	case OPLUS_IC_FUNC_PPS_HARD_RESET:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_HARD_RESET,
			oplus_chg_pps_hard_reset);
		break;
	case OPLUS_IC_FUNC_PPS_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_EXIT,
			oplus_chg_pps_exit_pps_mode);
		break;
	case OPLUS_IC_FUNC_PPS_CONFIG_WD:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_CONFIG_WD,
			oplus_chg_pps_config_wd);
		break;
	case OPLUS_IC_FUNC_PPS_GET_DEV_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_GET_DEV_INFO,
			oplus_chg_pps_get_dev_info);
		break;
	case OPLUS_IC_FUNC_PPS_GET_ERR_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_GET_ERR_INFO,
			oplus_chg_pps_get_err_info);
		break;
	case OPLUS_IC_FUNC_GET_PPS_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GET_PPS_STATUS,
			oplus_chg_get_pps_status);
		break;
	case OPLUS_IC_FUNC_PPS_GET_CABLE_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_GET_CABLE_INFO,
			oplus_chg_pps_get_cable_info);
		break;
	case OPLUS_IC_FUNC_PPS_GET_PDO_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_GET_PDO_INFO,
			oplus_chg_pps_get_pdo_info);
		break;
	case OPLUS_IC_FUNC_PPS_VERIFY_ADAPTER:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_VERIFY_ADAPTER,
			oplus_chg_pps_verify_adapter);
		break;
	case OPLUS_IC_FUNC_PPS_GET_POWER_CHANGE_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_PPS_GET_POWER_CHANGE_INFO,
			oplus_chg_pps_get_power_change_info);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
static int oplus_chg_pps_set_func_data(struct oplus_chg_ic_dev *ic_dev,
				      enum oplus_chg_ic_func func_id,
				      const void *buf, size_t buf_len)
{
	int rc = 0;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT))
		return -EINVAL;

	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_init(ic_dev);
		break;
	case OPLUS_IC_FUNC_EXIT:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_exit(ic_dev);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_reg_dump(ic_dev);
		break;
	case OPLUS_IC_FUNC_PPS_PDO_SET:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_pdo_set(ic_dev,
			oplus_chg_ic_get_item_data(buf, 0),
			oplus_chg_ic_get_item_data(buf, 1));
		break;
	case OPLUS_IC_FUNC_FIXED_PDO_SET:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_fixed_pdo_set(ic_dev,
			oplus_chg_ic_get_item_data(buf, 0),
			oplus_chg_ic_get_item_data(buf, 1));
		break;
	case OPLUS_IC_FUNC_PPS_HARD_RESET:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_hard_reset(ic_dev);
		break;
	case OPLUS_IC_FUNC_PPS_EXIT:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_exit(ic_dev);
		break;
	case OPLUS_IC_FUNC_PPS_CONFIG_WD:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_pps_config_wd(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	default:
		chg_err("this func(=%d) is not supported to set\n", func_id);
		return -ENOTSUPP;
		break;
	}

	return rc;
}

static ssize_t oplus_chg_pps_get_func_data(struct oplus_chg_ic_dev *ic_dev,
					   enum oplus_chg_ic_func func_id,
					   void *buf)
{
	ssize_t rc = 0;
	int len;
	char *tmp_buf;

	if (!ic_dev->online && (func_id != OPLUS_IC_FUNC_INIT) &&
	    (func_id != OPLUS_IC_FUNC_EXIT))
		return -EINVAL;

	switch (func_id) {
	case OPLUS_IC_FUNC_SMT_TEST:
		tmp_buf = (char *)get_zeroed_page(GFP_KERNEL);
		if (!tmp_buf) {
			rc = -ENOMEM;
			break;
		}
		rc = oplus_chg_pps_smt_test(ic_dev, tmp_buf, PAGE_SIZE);
		if (rc < 0) {
			free_page((unsigned long)tmp_buf);
			break;
		}
		len = oplus_chg_ic_debug_str_data_init(buf, rc);
		memcpy(oplus_chg_ic_get_item_data_addr(buf, 0), tmp_buf, rc);
		free_page((unsigned long)tmp_buf);
		rc = len;
		break;
	default:
		chg_err("this func(=%d) is not supported to get\n", func_id);
		return -ENOTSUPP;
		break;
	}

	return rc;
}

enum oplus_chg_ic_func oplus_pps_overwrite_funcs[] = {
	OPLUS_IC_FUNC_PPS_PDO_SET,
	OPLUS_IC_FUNC_PPS_CONFIG_WD,
};

#endif /* CONFIG_OPLUS_CHG_IC_DEBUG */

struct oplus_chg_ic_virq oplus_pps_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static int oplus_virtual_pps_probe(struct platform_device *pdev)
{
	struct oplus_virtual_pps_ic *chip;
	struct device_node *node = pdev->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	int rc = 0;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_virtual_pps_ic),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "pps-virtual");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x01");
	ic_cfg.get_func = oplus_chg_pps_get_func;
	ic_cfg.virq_data = oplus_pps_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(oplus_pps_virq_table);
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}
	oplus_chg_pps_init(chip->ic_dev);
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	chip->ic_dev->debug.get_func_data = oplus_chg_pps_get_func_data;
	chip->ic_dev->debug.set_func_data = oplus_chg_pps_set_func_data;
	chip->ic_dev->debug.overwrite_funcs = oplus_pps_overwrite_funcs;
	chip->ic_dev->debug.func_num = ARRAY_SIZE(oplus_pps_overwrite_funcs);
#endif

	rc = oplus_vpps_child_init(chip);
	if (rc < 0) {
		chg_err("child ic init error, rc=%d\n", rc);
		goto reg_ic_err;
	}

	chg_err("probe success\n");
	return 0;

reg_ic_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	chg_err("probe error, rc=%d\n", rc);
	return rc;
}

static int oplus_virtual_pps_remove(struct platform_device *pdev)
{
	struct oplus_virtual_pps_ic *chip = platform_get_drvdata(pdev);

	if(chip == NULL)
		return -ENODEV;

	if (chip->ic_dev->online)
		oplus_chg_pps_exit(chip->ic_dev);

	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_virtual_pps_match[] = {
	{ .compatible = "oplus,virtual_pps" },
	{},
};

static struct platform_driver oplus_virtual_pps_driver = {
	.driver		= {
		.name = "oplus-virtual_pps",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_virtual_pps_match),
	},
	.probe		= oplus_virtual_pps_probe,
	.remove		= oplus_virtual_pps_remove,
};

static __init int oplus_virtual_pps_init(void)
{
	return platform_driver_register(&oplus_virtual_pps_driver);
}

static __exit void oplus_virtual_pps_exit(void)
{
	platform_driver_unregister(&oplus_virtual_pps_driver);
}

oplus_chg_module_register(oplus_virtual_pps);
