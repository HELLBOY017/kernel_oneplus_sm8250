// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[DPDM_SWITCH]([%s][%d]): " fmt, __func__, __LINE__

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
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#endif
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>

/*Add for audio switch */
#include "oplus_chg_audio_switch.c"

#if IS_ENABLED(CONFIG_OPLUS_AUDIO_SWITCH_GLINK)
int register_chg_glink_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&chg_glink_notifier, nb);
}
EXPORT_SYMBOL_GPL(register_chg_glink_notifier);

int unregister_chg_glink_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_unregister(&chg_glink_notifier, nb);
}
EXPORT_SYMBOL_GPL(unregister_chg_glink_notifier);
#endif

struct oplus_dpdm_switch_ic {
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;

	int switch_ctrl_gpio;
	int dpdm_switch1_gpio;
	int dpdm_switch2_gpio;
	struct pinctrl_state *gpio_switch_ctrl_ap;
	struct pinctrl_state *gpio_switch_ctrl_vooc;
	struct pinctrl_state *gpio_switch_ctrl_ufcs;
	struct pinctrl_state *gpio_dpdm_switch_ap;
	struct pinctrl_state *gpio_dpdm_switch_vooc;
	struct pinctrl_state *gpio_dpdm_switch_ufcs;
	bool use_audio_switch;

	struct pinctrl *pinctrl;
	struct mutex pinctrl_lock;

	enum oplus_dpdm_switch_mode mode;
};

#define AUDIO_SWITCH_RETRY_MAX	10

static int oplus_switch_to_ap(struct oplus_dpdm_switch_ic *chip)
{
	int rc;
	int retry;
	int status;

	mutex_lock(&chip->pinctrl_lock);
	if (!IS_ERR_OR_NULL(chip->pinctrl) && !IS_ERR_OR_NULL(chip->gpio_dpdm_switch_ap)) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_dpdm_switch_ap);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_dpdm_switch_ap error, rc=%d\n", rc);
			return rc;
		}
	}
	if (!chip->use_audio_switch) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_switch_ctrl_ap);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_switch_ctrl_ap error, rc=%d\n", rc);
			return rc;
		}
	} else {
		retry = AUDIO_SWITCH_RETRY_MAX;
		do {
			oplus_set_audio_switch_status(0);
			status = oplus_get_audio_switch_status();
			if ((status & TYPEC_AUDIO_SWITCH_STATE_MASK) | TYPEC_AUDIO_SWITCH_STATE_DPDM) {
				chg_err("set switch to ap failed , retry=%d, status = 0x%x\n", retry, status);
			} else {
				chg_info("set switch to ap success , retry=%d\n", retry);
				break;
			}
		} while (--retry > 0);
		if (retry <= 0) {
			mutex_unlock(&chip->pinctrl_lock);
			return -EFAULT;
		}
	}
	mutex_unlock(&chip->pinctrl_lock);

	return 0;
}

static int oplus_switch_to_vooc(struct oplus_dpdm_switch_ic *chip)
{
	int rc;
	int retry;
	int status;

	mutex_lock(&chip->pinctrl_lock);
	if (!IS_ERR_OR_NULL(chip->pinctrl) && !IS_ERR_OR_NULL(chip->gpio_dpdm_switch_vooc)) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_dpdm_switch_vooc);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_dpdm_switch_vooc error, rc=%d\n", rc);
			return rc;
		}
	}
	if (!chip->use_audio_switch) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_switch_ctrl_vooc);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_switch_ctrl_vooc error, rc=%d\n", rc);
			return rc;
		}
	} else {
		retry = AUDIO_SWITCH_RETRY_MAX;
		do {
			oplus_set_audio_switch_status(1);
			status = oplus_get_audio_switch_status();
			if (!(TYPEC_AUDIO_SWITCH_STATE_FAST_CHG & status)) {
				chg_err("set switch to vooc failed , retry=%d, status = 0x%x\n", retry, status);
			} else {
				chg_info("set switch to vooc success , retry=%d\n", retry);
				break;
			}
		} while (--retry > 0);
		if (retry <= 0) {
			mutex_unlock(&chip->pinctrl_lock);
			return -EFAULT;
		}
	}
	mutex_unlock(&chip->pinctrl_lock);

	return 0;
}

static int oplus_switch_to_ufcs(struct oplus_dpdm_switch_ic *chip)
{
	int rc;
	int retry;
	int status;

	mutex_lock(&chip->pinctrl_lock);
	if (!IS_ERR_OR_NULL(chip->pinctrl) && !IS_ERR_OR_NULL(chip->gpio_dpdm_switch_ufcs)) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_dpdm_switch_ufcs);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_dpdm_switch_ufcs error, rc=%d\n", rc);
			return rc;
		}
	}
	if (!chip->use_audio_switch) {
		rc = pinctrl_select_state(chip->pinctrl, chip->gpio_switch_ctrl_ufcs);
		if (rc < 0) {
			mutex_unlock(&chip->pinctrl_lock);
			chg_err("set gpio_switch_ctrl_ufcs error, rc=%d\n", rc);
			return rc;
		}
	} else {
		retry = AUDIO_SWITCH_RETRY_MAX;
		do {
			oplus_set_audio_switch_status(1);
			status = oplus_get_audio_switch_status();
			if (!(TYPEC_AUDIO_SWITCH_STATE_FAST_CHG & status)) {
				chg_err("set switch to ufcs failed , retry=%d, status = 0x%x\n", retry, status);
			} else {
				chg_info("set switch to ufcs success , retry=%d\n", retry);
				break;
			}
		} while (--retry > 0);
		if (retry <= 0) {
			mutex_unlock(&chip->pinctrl_lock);
			return -EFAULT;
		}
	}
	mutex_unlock(&chip->pinctrl_lock);

	return 0;
}

static int oplus_dpdm_switch_init(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	if (ic_dev->online)
		return 0;
	ic_dev->online = true;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_ONLINE);

	return 0;
}

static int oplus_dpdm_switch_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (!ic_dev->online)
		return 0;
	ic_dev->online = false;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_OFFLINE);

	return 0;
}

static int oplus_dpdm_switch_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int oplus_dpdm_switch_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static int oplus_dpdm_switch_set_switch_mode(struct oplus_chg_ic_dev *ic_dev,
	enum oplus_dpdm_switch_mode mode)
{
	struct oplus_dpdm_switch_ic *chip;
	int rc;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_ENABLE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		mode = oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	chip = oplus_chg_ic_get_drvdata(ic_dev);

	switch (mode) {
	case DPDM_SWITCH_TO_AP:
		chg_info("dpdm switch to ap\n");
		rc = oplus_switch_to_ap(chip);
		break;
	case DPDM_SWITCH_TO_VOOC:
		chg_info("dpdm switch to vooc\n");
		rc = oplus_switch_to_vooc(chip);
		break;
	case DPDM_SWITCH_TO_UFCS:
		chg_info("dpdm switch to ufcs\n");
		rc = oplus_switch_to_ufcs(chip);
		break;
	default:
		chg_err("not supported mode, mode=%d\n", mode);
		return -EINVAL;
	}
	chip->mode = mode;

	return rc;
}

static int oplus_dpdm_switch_get_switch_mode(struct oplus_chg_ic_dev *ic_dev,
	enum oplus_dpdm_switch_mode *mode)
{
	struct oplus_dpdm_switch_ic *chip;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_ENABLE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*mode = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	chip = oplus_chg_ic_get_drvdata(ic_dev);
	*mode = chip->mode;

	return 0;
}

static void *oplus_dpdm_switch_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
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
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT, oplus_dpdm_switch_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT, oplus_dpdm_switch_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP, oplus_dpdm_switch_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST, oplus_dpdm_switch_smt_test);
		break;
	case OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE,
			oplus_dpdm_switch_set_switch_mode);
		break;
	case OPLUS_IC_FUNC_GET_DPDM_SWITCH_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GET_DPDM_SWITCH_MODE,
			oplus_dpdm_switch_get_switch_mode);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
static int oplus_dpdm_switch_set_func_data(struct oplus_chg_ic_dev *ic_dev,
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
		rc = oplus_dpdm_switch_init(ic_dev);
		break;
	case OPLUS_IC_FUNC_EXIT:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_dpdm_switch_exit(ic_dev);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_dpdm_switch_reg_dump(ic_dev);
		break;
	case OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_dpdm_switch_set_switch_mode(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	default:
		chg_err("this func(=%d) is not supported to set\n", func_id);
		return -ENOTSUPP;
		break;
	}

	return rc;
}

static ssize_t oplus_dpdm_switch_get_func_data(struct oplus_chg_ic_dev *ic_dev,
					  enum oplus_chg_ic_func func_id,
					  void *buf)
{
	int *item_data;
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
		rc = oplus_dpdm_switch_smt_test(ic_dev, tmp_buf, PAGE_SIZE);
		if (rc < 0) {
			free_page((unsigned long)tmp_buf);
			break;
		}
		len = oplus_chg_ic_debug_str_data_init(buf, rc);
		memcpy(oplus_chg_ic_get_item_data_addr(buf, 0), tmp_buf, rc);
		free_page((unsigned long)tmp_buf);
		rc = len;
		break;
	case OPLUS_IC_FUNC_GET_DPDM_SWITCH_MODE:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_dpdm_switch_get_switch_mode(ic_dev, (enum oplus_dpdm_switch_mode *)item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	default:
		chg_err("this func(=%d) is not supported to get\n", func_id);
		return -ENOTSUPP;
		break;
	}

	return rc;
}

enum oplus_chg_ic_func oplus_dpdm_switch_overwrite_funcs[] = {
	OPLUS_IC_FUNC_SET_DPDM_SWITCH_MODE,
};

#endif /* CONFIG_OPLUS_CHG_IC_DEBUG */

struct oplus_chg_ic_virq oplus_dpdm_switch_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static int oplus_dpdm_switch_hw_init(struct oplus_dpdm_switch_ic *chip)
{
	struct device_node *node = chip->dev->of_node;
	int rc;

	chip->pinctrl = devm_pinctrl_get(chip->dev);

	chip->use_audio_switch = of_property_read_bool(node, "oplus,use_audio_switch");
	chg_info("use_audio_switch = %d\n", chip->use_audio_switch);

	if (!chip->use_audio_switch) {
		if (IS_ERR_OR_NULL(chip->pinctrl)) {
			chg_err("get pinctrl is null\n");
			return -ENODEV;
		}
		chip->switch_ctrl_gpio =
			of_get_named_gpio(node, "oplus,switch_ctrl-gpio", 0);
		if (!gpio_is_valid(chip->switch_ctrl_gpio)) {
			chg_err("Couldn't read oplus,switch_ctrl-gpio, %d\n",
				chip->switch_ctrl_gpio);
			return -ENODEV;
		}

		rc = gpio_request(chip->switch_ctrl_gpio, "switch_ctrl-gpio");
		if (rc != 0) {
			chg_err("unable to request switch_ctrl-gpio:%d\n",
				chip->switch_ctrl_gpio);
			return rc;
		}

		chip->gpio_switch_ctrl_ap =
			pinctrl_lookup_state(chip->pinctrl, "switch_ctrl_ap");
		if (IS_ERR_OR_NULL(chip->gpio_switch_ctrl_ap)) {
			chg_err("get switch_ctrl_ap fail\n");
			rc = -EINVAL;
			goto free_switch_ctrl_gpio;
		}
		chip->gpio_switch_ctrl_vooc =
			pinctrl_lookup_state(chip->pinctrl, "switch_ctrl_vooc");
		if (IS_ERR_OR_NULL(chip->gpio_switch_ctrl_vooc)) {
			chg_err("get switch_ctrl_vooc fail\n");
			rc = -EINVAL;
			goto free_switch_ctrl_gpio;
		}
		chip->gpio_switch_ctrl_ufcs =
			pinctrl_lookup_state(chip->pinctrl, "switch_ctrl_ufcs");
		if (IS_ERR_OR_NULL(chip->gpio_switch_ctrl_ufcs)) {
			chg_err("get switch_ctrl_ufcs fail\n");
			rc = -EINVAL;
			goto free_switch_ctrl_gpio;
		}
		rc = pinctrl_select_state(chip->pinctrl,
			chip->gpio_switch_ctrl_ap);
		if (rc < 0) {
			chg_err("set gpio switch_ctrl_ap error, rc=%d\n", rc);
			goto free_switch_ctrl_gpio;
		}
	} else {
		audio_switch_init();
		if (IS_ERR_OR_NULL(chip->pinctrl)) {
			chg_info("but use audio switch, maybe not use pinctrl\n");
			return 0;
		}
		/*TODO: need to check the default AUIDIO SWITCH state is DPDM and set to DPDM state.*/
	}

	chip->dpdm_switch1_gpio =
		of_get_named_gpio(node, "oplus,dpdm_switch1-gpio", 0);
	if (!gpio_is_valid(chip->dpdm_switch1_gpio)) {
		chg_err("Couldn't read oplus,dpdm_switch1-gpio, %d\n",
			chip->dpdm_switch1_gpio);
	} else {
		rc = gpio_request(chip->dpdm_switch1_gpio, "dpdm_switch1-gpio");
		if (rc != 0) {
			chg_err("unable to request dpdm_switch1-gpio:%d\n",
				chip->dpdm_switch1_gpio);
			goto free_switch_ctrl_gpio;
		}
	}
	chip->dpdm_switch2_gpio =
		of_get_named_gpio(node, "oplus,dpdm_switch2-gpio", 0);
	if (!gpio_is_valid(chip->dpdm_switch2_gpio)) {
		chg_info("Couldn't read oplus,dpdm_switch2-gpio, %d\n",
			chip->dpdm_switch2_gpio);
	} else {
		rc = gpio_request(chip->dpdm_switch2_gpio,
				"dpdm_switch2-gpio");
		if (rc != 0) {
			chg_err("unable to request dpdm_switch2-gpio:%d\n",
				chip->dpdm_switch2_gpio);
			goto free_dpdm_switch1_gpio;
		}
	}

	chip->gpio_dpdm_switch_ap =
		pinctrl_lookup_state(chip->pinctrl, "dpdm_switch_ap");
	if (IS_ERR_OR_NULL(chip->gpio_dpdm_switch_ap)) {
		chg_err("get dpdm_switch_ap fail\n");
		rc = -EINVAL;
		goto free_dpdm_switch2_gpio;
	}
	chip->gpio_dpdm_switch_vooc =
		pinctrl_lookup_state(chip->pinctrl, "dpdm_switch_vooc");
	if (IS_ERR_OR_NULL(chip->gpio_dpdm_switch_vooc)) {
		chg_err("get dpdm_switch_vooc fail\n");
		rc = -EINVAL;
		goto free_dpdm_switch2_gpio;
	}
	chip->gpio_dpdm_switch_ufcs =
		pinctrl_lookup_state(chip->pinctrl, "dpdm_switch_ufcs");
	if (IS_ERR_OR_NULL(chip->gpio_dpdm_switch_ufcs)) {
		chg_err("get dpdm_switch_ufcs fail\n");
		rc = -EINVAL;
		goto free_dpdm_switch2_gpio;
	}
	rc = pinctrl_select_state(chip->pinctrl,
		chip->gpio_dpdm_switch_ap);
	if (rc < 0) {
		chg_err("set gpio dpdm_switch_ap error, rc=%d\n", rc);
		goto free_dpdm_switch2_gpio;
	}

	return 0;

free_dpdm_switch2_gpio:
	if (gpio_is_valid(chip->dpdm_switch1_gpio))
		gpio_free(chip->dpdm_switch1_gpio);
free_dpdm_switch1_gpio:
	if (gpio_is_valid(chip->dpdm_switch1_gpio))
		gpio_free(chip->dpdm_switch1_gpio);
free_switch_ctrl_gpio:
	if (gpio_is_valid(chip->switch_ctrl_gpio))
		gpio_free(chip->switch_ctrl_gpio);

	return rc;
}

static int oplus_virtual_dpdm_switch_probe(struct platform_device *pdev)
{
	struct oplus_dpdm_switch_ic *chip;
	struct device_node *node = pdev->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_dpdm_switch_ic),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	mutex_init(&chip->pinctrl_lock);
	rc = oplus_dpdm_switch_hw_init(chip);
	if (rc < 0)
		goto hw_init_err;

	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "misc-dpdm_switch");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.get_func = oplus_dpdm_switch_get_func;
	ic_cfg.virq_data = oplus_dpdm_switch_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(oplus_dpdm_switch_virq_table);
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}
	oplus_dpdm_switch_init(chip->ic_dev);
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	chip->ic_dev->debug.get_func_data = oplus_dpdm_switch_get_func_data;
	chip->ic_dev->debug.set_func_data = oplus_dpdm_switch_set_func_data;
	oplus_chg_ic_func_table_sort(oplus_dpdm_switch_overwrite_funcs,
		ARRAY_SIZE(oplus_dpdm_switch_overwrite_funcs));
	chip->ic_dev->debug.overwrite_funcs = oplus_dpdm_switch_overwrite_funcs;
	chip->ic_dev->debug.func_num = ARRAY_SIZE(oplus_dpdm_switch_overwrite_funcs);
#endif

	chg_info("probe success\n");
	return 0;

reg_ic_err:
	if (gpio_is_valid(chip->switch_ctrl_gpio))
		gpio_free(chip->switch_ctrl_gpio);
hw_init_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	chg_err("probe error\n");
	return rc;
}

static int oplus_virtual_dpdm_switch_remove(struct platform_device *pdev)
{
	struct oplus_dpdm_switch_ic *chip = platform_get_drvdata(pdev);

	if(chip == NULL)
		return -ENODEV;

	if (chip->ic_dev->online)
		oplus_dpdm_switch_exit(chip->ic_dev);

	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
	if (gpio_is_valid(chip->switch_ctrl_gpio))
		gpio_free(chip->switch_ctrl_gpio);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_virtual_dpdm_switch_match[] = {
	{ .compatible = "oplus,virtual_dpdm_switch" },
	{},
};

static struct platform_driver oplus_virtual_dpdm_switch_driver = {
	.driver		= {
		.name = "oplus-virtual_dpdm_switch",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_virtual_dpdm_switch_match),
	},
	.probe		= oplus_virtual_dpdm_switch_probe,
	.remove		= oplus_virtual_dpdm_switch_remove,
};

static __init int oplus_virtual_dpdm_switch_init(void)
{
#if __and(IS_BUILTIN(CONFIG_OPLUS_CHG), IS_BUILTIN(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return 0;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return 0;
#endif /* CONFIG_OPLUS_CHG_V2 */
	return platform_driver_register(&oplus_virtual_dpdm_switch_driver);
}

static __exit void oplus_virtual_dpdm_switch_exit(void)
{
#if (IS_ENABLED(CONFIG_OPLUS_CHG) && IS_ENABLED(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return;
#endif /* CONFIG_OPLUS_CHG_V2 */
	platform_driver_unregister(&oplus_virtual_dpdm_switch_driver);
}

oplus_chg_module_register(oplus_virtual_dpdm_switch);
