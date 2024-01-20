// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[MT6375_GAUGE]([%s][%d]): " fmt, __func__, __LINE__

#ifdef CONFIG_OPLUS_CHARGER_MTK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>

/* #include <mt-plat/battery_common.h> */
#include <soc/oplus/device_info.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#else
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <soc/oplus/device_info.h>
#include <linux/proc_fs.h>
#include <linux/soc/qcom/smem.h>
#endif
#include <linux/version.h>
#include<linux/gfp.h>

#ifdef OPLUS_SHA1_HMAC
#include <linux/random.h>
#endif
#include <oplus_chg_module.h>
#include <oplus_chg_comm.h>
#include <oplus_chg_vooc.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_mms_wired.h>

#include "oplus_hal_mtk_platform_gauge.h"
#include "../../oplus_gauge.h"

extern struct oplus_gauge_chip *g_gauge_chip;

static int oplus_mt6375_guage_init(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -EAGAIN;
	}

	ic_dev->online = false;

	if (g_gauge_chip == NULL) {
		ic_dev->online = false;
		chg_err("ic_dev->online = %d\n", ic_dev->online);
		return -EAGAIN;
	}

	if(g_gauge_chip->gauge_ops->get_battery_soc() < 0) {
		chg_err("soc abnormal");
		return -EAGAIN;
	}

	ic_dev->online = true;

	chg_info("oplus_mt6375_guage_init, ic_dev->online = %d\n", ic_dev->online);
	return 0;
}

static int oplus_mt6375_guage_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;
	chg_info("oplus_mt6375_guage_exit, ic_dev->online = %d\n", ic_dev->online);

	return 0;
}

static int oplus_mt6375_guage_get_batt_vol(struct oplus_chg_ic_dev *ic_dev, int index, int *vol_mv)
{
	*vol_mv = g_gauge_chip->gauge_ops->get_battery_mvolts();

	return 0;
}

static int oplus_mt6375_guage_get_batt_max(struct oplus_chg_ic_dev *ic_dev, int *vol_mv)
{
	*vol_mv = g_gauge_chip->gauge_ops->get_battery_mvolts();

	return 0;
}
static int oplus_mt6375_guage_get_batt_min(struct oplus_chg_ic_dev *ic_dev, int *vol_mv)
{
	*vol_mv = g_gauge_chip->gauge_ops->get_battery_mvolts();

	return 0;
}

static int oplus_mt6375_guage_get_batt_curr(struct oplus_chg_ic_dev *ic_dev, int *curr_ma)
{
	*curr_ma = g_gauge_chip->gauge_ops->get_average_current();

	return 0;
}

static int oplus_mt6375_guage_get_batt_temp(struct oplus_chg_ic_dev *ic_dev, int *temp)
{
	int soc;
	soc = g_gauge_chip->gauge_ops->get_battery_soc();
	if (soc < 0) {
		return -1;
	}
	*temp = g_gauge_chip->gauge_ops->get_battery_temperature();

	return 0;
}

static int oplus_mt6375_guage_get_batt_soc(struct oplus_chg_ic_dev *ic_dev, int *soc)
{
	*soc = g_gauge_chip->gauge_ops->get_battery_soc();
	if (*soc < 0) {
		return -1;
	}

	return 0;
}

static int oplus_mt6375_guage_get_batt_fcc(struct oplus_chg_ic_dev *ic_dev, int *fcc)
{
	*fcc = g_gauge_chip->gauge_ops->get_battery_fcc();

	return 0;
}

static int oplus_mt6375_guage_get_batt_cc(struct oplus_chg_ic_dev *ic_dev, int *cc)
{
	*cc = g_gauge_chip->gauge_ops->get_battery_cc();

	return 0;
}

static int oplus_mt6375_guage_get_batt_rm(struct oplus_chg_ic_dev *ic_dev, int *rm)
{
	*rm = g_gauge_chip->gauge_ops->get_batt_remaining_capacity();

	return 0;
}

static int oplus_mt6375_guage_get_batt_soh(struct oplus_chg_ic_dev *ic_dev, int *soh)
{
	*soh = g_gauge_chip->gauge_ops->get_battery_soh();

	return 0;
}

static int oplus_mt6375_guage_get_batt_auth(struct oplus_chg_ic_dev *ic_dev, bool *pass)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	if(g_gauge_chip == NULL) {
		chg_err("g_gauge_chip == NULL\n");
		return -ENODEV;
	}

	*pass = g_gauge_chip->gauge_ops->get_battery_authenticate();
	chg_info("*pass = %d\n", *pass);

	return 0;
}

static int oplus_mt6375_guage_get_afi_update_done(struct oplus_chg_ic_dev *ic_dev, bool *status)
{
	*status = true;
	return 0;
}

static int oplus_mt6375_guage_get_batt_hmac(struct oplus_chg_ic_dev *ic_dev, bool *pass)
{
	*pass = true;
	chg_info("*pass = %d\n", *pass);
	return 0;
}

static int oplus_mt6375_guage_set_batt_full(struct oplus_chg_ic_dev *ic_dev, bool full)
{
	g_gauge_chip->gauge_ops->set_battery_full(full);
	chg_info("full = %d\n", full);

	return 0;
}

static int oplus_mt6375_guage_update_dod0(struct oplus_chg_ic_dev *ic_dev)
{
	return g_gauge_chip->gauge_ops->update_battery_dod0();
}

static int oplus_mt6375_guage_update_soc_smooth_parameter(struct oplus_chg_ic_dev *ic_dev)
{
	return g_gauge_chip->gauge_ops->update_soc_smooth_parameter();
}

static int oplus_mt6375_guage_get_batt_num(struct oplus_chg_ic_dev *ic_dev, int *num)
{
	struct chip_mt6375_gauge *chip;

	chip = oplus_chg_ic_get_drvdata(ic_dev);
	*num = chip->batt_num;

	return 0;
}

static int oplus_mt6375_guage_get_batt_exist(struct oplus_chg_ic_dev *ic_dev, bool *exist)
{
	*exist = true;

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
					       oplus_mt6375_guage_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
					       oplus_mt6375_guage_exit);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_VOL:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_VOL,
					       oplus_mt6375_guage_get_batt_vol);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_MAX,
					       oplus_mt6375_guage_get_batt_max);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_MIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_MIN,
					       oplus_mt6375_guage_get_batt_min);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_GET_BATT_CURR,
			oplus_mt6375_guage_get_batt_curr);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_TEMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_GET_BATT_TEMP,
			oplus_mt6375_guage_get_batt_temp);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_SOC,
					       oplus_mt6375_guage_get_batt_soc);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_FCC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_FCC,
					       oplus_mt6375_guage_get_batt_fcc);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_CC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_CC,
					       oplus_mt6375_guage_get_batt_cc);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_RM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_RM,
					       oplus_mt6375_guage_get_batt_rm);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_SOH:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_SOH,
					       oplus_mt6375_guage_get_batt_soh);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_AUTH:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_GET_BATT_AUTH,
			oplus_mt6375_guage_get_batt_auth);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_HMAC:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_GET_BATT_HMAC,
			oplus_mt6375_guage_get_batt_hmac);
		break;
	case OPLUS_IC_FUNC_GAUGE_SET_BATT_FULL:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_SET_BATT_FULL,
			oplus_mt6375_guage_set_batt_full);
		break;
	case OPLUS_IC_FUNC_GAUGE_UPDATE_DOD0:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_UPDATE_DOD0,
					       oplus_mt6375_guage_update_dod0);
		break;
	case OPLUS_IC_FUNC_GAUGE_UPDATE_SOC_SMOOTH:
		func = OPLUS_CHG_IC_FUNC_CHECK(
			OPLUS_IC_FUNC_GAUGE_UPDATE_SOC_SMOOTH,
			oplus_mt6375_guage_update_soc_smooth_parameter);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_NUM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_NUM,
					       oplus_mt6375_guage_get_batt_num);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_AFI_UPDATE_DONE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_AFI_UPDATE_DONE,
					       oplus_mt6375_guage_get_afi_update_done);
		break;
	case OPLUS_IC_FUNC_GAUGE_GET_BATT_EXIST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_GAUGE_GET_BATT_EXIST,
					       oplus_mt6375_guage_get_batt_exist);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

struct oplus_chg_ic_virq mt6375_guage_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
	{ .virq_id = OPLUS_IC_VIRQ_RESUME },
};

static void oplus_mt6375_guage_parse_dt(struct chip_mt6375_gauge *chip)
{
	int rc = 0;

	atomic_set(&chip->locked, 0);
	atomic_set(&chip->suspended, 0);
	rc = of_property_read_u32(chip->dev->of_node, "oplus,batt_num",
				  &chip->batt_num);

	chg_info("batt_num = %d\n", chip->batt_num);
	if (rc < 0) {
		chg_err("can't get oplus,batt_num, rc = %d\n", rc);
		chip->batt_num = 1;
	}
}

static int mt6375_guage_driver_probe(struct platform_device *pdev)
{
	struct chip_mt6375_gauge *chip;
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	int rc = 0;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&pdev->dev, "failed to allocate device info data\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);
	atomic_set(&chip->suspended, 0);
	mutex_init(&chip->chip_mutex);
	oplus_mt6375_guage_parse_dt(chip);

	chip->soc_pre = 50;
	chip->batt_vol_pre = 3800;
	chip->fc_pre = 0;
	chip->qm_pre = 0;
	chip->pd_pre = 0;
	chip->rcu_pre = 0;
	chip->rcf_pre = 0;
	chip->fcu_pre = 0;
	chip->fcf_pre = 0;
	chip->sou_pre = 0;
	chip->do0_pre = 0;
	chip->doe_pre = 0;
	chip->trm_pre = 0;
	chip->pc_pre = 0;
	chip->qs_pre = 0;
	chip->max_vol_pre = 3800;
	chip->min_vol_pre = 3800;
	chip->current_pre = 999;
	chip->protect_check_done = false;
	chip->afi_update_done = true;
	chip->disabled = false;
	chip->error_occured = false;
	chip->need_check = true;
	chip->protect_check_done = true;

	atomic_set(&chip->locked, 0);
	rc = of_property_read_u32(chip->dev->of_node, "oplus,ic_type",
				  &ic_type);
	if (rc < 0) {
		chg_err("can't get ic type, rc=%d\n", rc);
		goto error;
	}
	rc = of_property_read_u32(chip->dev->of_node, "oplus,ic_index",
				  &ic_index);
	if (rc < 0) {
		chg_err("can't get ic index, rc=%d\n", rc);
		goto error;
	}
	ic_cfg.name = chip->dev->of_node->name;
	ic_cfg.index = ic_index;
	chip->device_type = 0;

	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "gauge-mt6375");

	ic_cfg.type = ic_type;
	ic_cfg.get_func = oplus_chg_get_func;
	ic_cfg.virq_data = mt6375_guage_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(mt6375_guage_virq_table);
	ic_cfg.of_node = chip->dev->of_node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", chip->dev->of_node->name);
		goto error;
	}
	chg_info("register %s\n", chip->dev->of_node->name);

#ifndef CONFIG_OPLUS_CHARGER_MTK
	oplus_vooc_get_fastchg_started_pfunc(&oplus_vooc_get_fastchg_started);
	oplus_vooc_get_fastchg_ing_pfunc(&oplus_vooc_get_fastchg_ing);
#endif

	oplus_mt6375_guage_init(chip->ic_dev);
	chg_info("mt6375_guage_driver_probe success\n");
	return 0;

error:
	return rc;
}

static int mt6375_guage_driver_remove(struct platform_device *pdev)
{
	struct mt6375_device *chip = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, chip);

	return 0;
}
/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/

static const struct of_device_id mt6375_gauge_match[] = {
	{ .compatible = "oplus,hal_mt6375_gauge" },
	{},
};

static struct platform_driver mt6375_gauge_driver = {
	.driver = {
		.name = "oplus_mt6375_gauge",
		.of_match_table = mt6375_gauge_match,
	},
	.probe = mt6375_guage_driver_probe,
	.remove = mt6375_guage_driver_remove,
};

static __init int oplus_mt6375_gauge_driver_init(void)
{
	int rc;

	rc = platform_driver_register(&mt6375_gauge_driver);
	if (rc < 0)
		chg_err("failed to register mt6375 debug driver, rc = %d\n", rc);

	return rc;
}

static __exit void oplus_mt6375_gauge_driver_exit(void)
{
	platform_driver_unregister(&mt6375_gauge_driver);
}

oplus_chg_module_register(oplus_mt6375_gauge_driver);

MODULE_DESCRIPTION("Driver for mt6375 gauge");
MODULE_LICENSE("GPL v2");

