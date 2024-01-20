// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[OPLUS_MOS_CTRL]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/rtc.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_parallel.h>

struct chip_mos_ctl {
	struct platform_device *pdev;
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;

	struct oplus_parallel_chip *parallel_chip;
	int mos_en_gpio;
	struct pinctrl *mos_en_pinctrl;
	struct pinctrl_state *mos_en_gpio_active;
	struct pinctrl_state *mos_en_gpio_sleep;
};
static struct chip_mos_ctl *g_mos_ic;

static int oplus_mos_en_gpio_init(struct chip_mos_ctl *dev)
{
	if (!dev) {
		chg_err("oplus_mos_dev null!\n");
		return -EINVAL;
	}

	dev->mos_en_pinctrl = devm_pinctrl_get(dev->dev);
	if (IS_ERR_OR_NULL(dev->mos_en_pinctrl)) {
		chg_err("get mos_en_pinctrl fail\n");
		return -EINVAL;
	}

	dev->mos_en_gpio_active =
		pinctrl_lookup_state(dev->mos_en_pinctrl, "mos_en_gpio_active");
	if (IS_ERR_OR_NULL(dev->mos_en_gpio_active)) {
		chg_err("get mos_en_gpio_active fail\n");
		return -EINVAL;
	}

	dev->mos_en_gpio_sleep =
		pinctrl_lookup_state(dev->mos_en_pinctrl, "mos_en_gpio_sleep");
	if (IS_ERR_OR_NULL(dev->mos_en_gpio_sleep)) {
		chg_err("get mos_en_gpio_sleep fail\n");
		return -EINVAL;
	}

	pinctrl_select_state(dev->mos_en_pinctrl, dev->mos_en_gpio_sleep);

	return 0;
}

static int oplus_mos_ctrl_parse_dt(struct chip_mos_ctl *dev)
{
	int rc = 0;
	struct device_node *node = NULL;

	if (!dev) {
		chg_err("oplus_mos_dev null!\n");
		return -1;
	}

	node = dev->dev->of_node;

	dev->mos_en_gpio = of_get_named_gpio(node, "oplus,mos-en-gpio", 0);
	if (dev->mos_en_gpio <= 0) {
		chg_err("Couldn't read oplus,mos-en-gpio\n");
	} else {
		if (gpio_is_valid(dev->mos_en_gpio)) {
			rc = gpio_request(dev->mos_en_gpio, "mos_en-gpio");
			if (rc) {
				chg_err("unable request mos-en-gpio:%d\n", dev->mos_en_gpio);
			} else {
				rc = oplus_mos_en_gpio_init(dev);
				if (rc)
					chg_err("unable init mos-en-gpio, rc = %d\n", rc);
				else
					chg_err("mos_en_gpio level[%d]\n", gpio_get_value(dev->mos_en_gpio));
			}
		}
		chg_err("mos_en-gpio:%d\n", dev->mos_en_gpio);
	}

	return 0;
}

static int oplus_mos_ctrl_set_enable(struct oplus_chg_ic_dev *ic_dev, bool enable)
{
	struct chip_mos_ctl *mos_ctl;

	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL");
		return -ENODEV;
	}
	mos_ctl = oplus_chg_ic_get_drvdata(ic_dev);

	if (!mos_ctl) {
		chg_err("mos_ctl not specified!\n");
		return -EINVAL;
	}

	if (!mos_ctl->mos_en_pinctrl
	    || !mos_ctl->mos_en_gpio_active
	    || !mos_ctl->mos_en_gpio_sleep) {
		chg_err("pinctrl or gpio_active or gpio_sleep not specified!\n");
		return -EINVAL;
	}

	if (enable)
		pinctrl_select_state(mos_ctl->mos_en_pinctrl, mos_ctl->mos_en_gpio_active);
	else
		pinctrl_select_state(mos_ctl->mos_en_pinctrl, mos_ctl->mos_en_gpio_sleep);

	chg_err("enable %d, level %d\n", enable, gpio_get_value(mos_ctl->mos_en_gpio));

	return 0;
}

static int oplus_mos_ctrl_get_enable(struct oplus_chg_ic_dev *ic_dev, bool *enable)
{
	struct chip_mos_ctl *mos_ctl;

	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL");
		return -ENODEV;
	}
	mos_ctl = oplus_chg_ic_get_drvdata(ic_dev);

	if (!mos_ctl) {
		chg_err("mos_ctl not specified!\n");
		return -EINVAL;
	}

	if (!gpio_is_valid(mos_ctl->mos_en_gpio)) {
		chg_err("mos_en_gpio not specified!\n");
		return -EINVAL;
	}

	*enable = gpio_get_value(mos_ctl->mos_en_gpio);

	return 0;
}

struct oplus_chg_ic_virq mos_ctl_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
};

static int oplus_mos_ctl_init(struct oplus_chg_ic_dev *ic_dev)
{
	ic_dev->online = true;

	return PARALLEL_MOS_CTRL;
}

static int oplus_mos_ctl_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;

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
					       oplus_mos_ctl_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
					       oplus_mos_ctl_exit);
		break;
	case OPLUS_IC_FUNC_SET_HW_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SET_HW_ENABLE,
					       oplus_mos_ctrl_set_enable);
		break;
	case OPLUS_IC_FUNC_GET_HW_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GET_HW_ENABLE,
					       oplus_mos_ctrl_get_enable);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

static int oplus_mos_ctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	struct chip_mos_ctl *mos_ic;
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	int rc;

	mos_ic = devm_kzalloc(dev, sizeof(*mos_ic), GFP_KERNEL);
	if (!mos_ic) {
		chg_err("failed to allocate device info data\n");
		return -ENOMEM;
	}

	mos_ic->dev = dev;

	platform_set_drvdata(pdev, mos_ic);

	oplus_mos_ctrl_parse_dt(mos_ic);
	rc = of_property_read_u32(mos_ic->dev->of_node, "oplus,ic_type",
				  &ic_type);
	if (rc < 0) {
		chg_err("can't get ic type, rc=%d\n", rc);
		goto error;
	}
	rc = of_property_read_u32(mos_ic->dev->of_node, "oplus,ic_index",
				  &ic_index);
	if (rc < 0) {
		chg_err("can't get ic index, rc=%d\n", rc);
		goto error;
	}
	ic_cfg.name = dev->of_node->name;
	ic_cfg.index = ic_index;
	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "parallel-mos_ctl");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.type = ic_type;
	ic_cfg.get_func = oplus_chg_get_func;
	ic_cfg.virq_data = mos_ctl_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(mos_ctl_virq_table);
	ic_cfg.of_node = mos_ic->dev->of_node;
	mos_ic->ic_dev = devm_oplus_chg_ic_register(mos_ic->dev, &ic_cfg);
	if (!mos_ic->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", mos_ic->dev->of_node->name);
		goto error;
	}
	mos_ic->pdev = pdev;
	g_mos_ic = mos_ic;

	chg_info("register %s\n", mos_ic->dev->of_node->name);
	oplus_mos_ctl_init(mos_ic->ic_dev);

	return 0;
error:
	return rc;
}

static int oplus_mos_ctrl_remove(struct platform_device *pdev)
{
	struct chip_mos_ctl *dev = platform_get_drvdata(pdev);

	pinctrl_select_state(dev->mos_en_pinctrl, dev->mos_en_gpio_sleep);

	return 0;
}

static void oplus_mos_ctrl_shutdown(struct platform_device *pdev)
{
	struct chip_mos_ctl *dev = platform_get_drvdata(pdev);

	pinctrl_select_state(dev->mos_en_pinctrl, dev->mos_en_gpio_sleep);
}

static const struct of_device_id oplus_mos_match_table[] = {
	{ .compatible = "oplus,mos-ctrl" },
	{},
};

static struct platform_driver oplus_mos_ctrl_driver = {
	.driver = {
		.name = "oplus_mos_ctrl",
		.of_match_table = oplus_mos_match_table,
	},
	.probe = oplus_mos_ctrl_probe,
	.remove = oplus_mos_ctrl_remove,
	.shutdown = oplus_mos_ctrl_shutdown,
};

static __init int oplus_mos_ctrl_init(void)
{
	return platform_driver_register(&oplus_mos_ctrl_driver);
}

static __exit void oplus_mos_ctrl_exit(void)
{
	platform_driver_unregister(&oplus_mos_ctrl_driver);
}

oplus_chg_module_register(oplus_mos_ctrl);

MODULE_DESCRIPTION("Oplus mos ctrl driver");
MODULE_LICENSE("GPL v2");
