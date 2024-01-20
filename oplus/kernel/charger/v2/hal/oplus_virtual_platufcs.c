// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[VIRTUAL_PLATUFCS]([%s][%d]): " fmt, __func__, __LINE__

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
#include <ufcs_class.h>

#define UFCS_REG_TIMEOUT_MS	120000

struct oplus_virtual_ufcs_child {
	struct oplus_chg_ic_dev *parent;
	struct oplus_chg_ic_dev *ic_dev;
	int index;
	enum oplus_chg_ic_func *funcs;
	int func_num;
	enum oplus_chg_ic_virq_id *virqs;
	int virq_num;
	struct work_struct online_work;
	struct work_struct offline_work;
};

struct oplus_virtual_ufcs_ic {
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	struct ufcs_dev *ufcs;
	enum oplus_chg_ic_connect_type connect_type;
	int child_num;
	struct oplus_virtual_ufcs_child *child_list;
	bool online;
};

static void oplus_chg_vpu_err_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_ufcs_ic *chip = virq_data;

	oplus_chg_ic_move_err_msg(chip->ic_dev, ic_dev);
	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
}

static void oplus_chg_vpu_online_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_ufcs_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ONLINE);
}

static void oplus_chg_vpu_offline_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_ufcs_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_OFFLINE);
}

static inline bool func_is_support(struct oplus_virtual_ufcs_child *ic,
				   enum oplus_chg_ic_func func_id)
{
	switch (func_id) {
	case OPLUS_IC_FUNC_INIT:
	case OPLUS_IC_FUNC_EXIT:
		return true; /* must support */
	default:
		break;
	}

	if (ic->func_num > 0)
		return oplus_chg_ic_func_check_support_by_table(ic->funcs, ic->func_num, func_id);
	else
		return false;
}

static inline bool virq_is_support(struct oplus_virtual_ufcs_child *ic,
				   enum oplus_chg_ic_virq_id virq_id)
{
	switch (virq_id) {
	case OPLUS_IC_VIRQ_ERR:
	case OPLUS_IC_VIRQ_ONLINE:
	case OPLUS_IC_VIRQ_OFFLINE:
		return true; /* must support */
	default:
		break;
	}

	if (ic->virq_num > 0)
		return oplus_chg_ic_virq_check_support_by_table(ic->virqs, ic->virq_num, virq_id);
	else
		return false;
}

static int oplus_chg_vpu_virq_register(struct oplus_virtual_ufcs_ic *chip)
{
	int i;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_ERR)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_ERR,
				oplus_chg_vpu_err_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_ONLINE)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_ONLINE,
				oplus_chg_vpu_online_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_OFFLINE)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_OFFLINE,
				oplus_chg_vpu_offline_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_OFFLINE error, rc=%d", rc);
		}
	}

	return 0;
}

static void oplus_vpu_online_work(struct work_struct *work)
{
	struct oplus_virtual_ufcs_child *child =
		container_of(work, struct oplus_virtual_ufcs_child, online_work);
	struct oplus_virtual_ufcs_ic *chip;

	chg_info("%s online\n", child->ic_dev->name);
	chip = oplus_chg_ic_get_drvdata(child->parent);

	if (!child->parent->online) {
		child->parent->online = true;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_INIT);
	}
}

static void oplus_vpu_offline_work(struct work_struct *work)
{
	struct oplus_virtual_ufcs_child *child =
		container_of(work, struct oplus_virtual_ufcs_child, offline_work);
	struct oplus_virtual_ufcs_ic *chip;

	chg_info("%s offline\n", child->ic_dev->name);
	chip = oplus_chg_ic_get_drvdata(child->parent);

	if (child->parent->online) {
		child->parent->online = false;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_EXIT);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_OFFLINE);
	}
}

static int oplus_chg_vpu_child_funcs_init(struct oplus_virtual_ufcs_ic *chip,
					 int child_num)
{
	struct device_node *node = chip->dev->of_node;
	struct device_node *func_node = NULL;
	int i, m;
	int rc = 0;

	for (i = 0; i < child_num; i++) {
		func_node = of_parse_phandle(node, "oplus,ufcs_ic_func_group", i);
		if (func_node == NULL) {
			chg_err("can't get ic[%d] function group\n", i);
			rc = -ENODATA;
			goto err;
		}
		rc = of_property_count_elems_of_size(func_node, "functions", sizeof(u32));
		if (rc < 0) {
			chg_err("can't get ic[%d] functions size, rc=%d\n", i, rc);
			goto err;
		}
		chip->child_list[i].func_num = rc;
		chip->child_list[i].funcs = devm_kzalloc(
			chip->dev,
			sizeof(enum oplus_chg_ic_func) * chip->child_list[i].func_num,
			GFP_KERNEL);
		if (chip->child_list[i].funcs == NULL) {
			rc = -ENOMEM;
			chg_err("alloc child ic funcs memory error\n");
			goto err;
		}
		rc = of_property_read_u32_array(
			func_node, "functions",
			(u32 *)chip->child_list[i].funcs,
			chip->child_list[i].func_num);
		if (rc) {
			i++;
			chg_err("can't get ic[%d] functions, rc=%d\n", i, rc);
			goto err;
		}
		(void)oplus_chg_ic_func_table_sort(
			chip->child_list[i].funcs,
			chip->child_list[i].func_num);
	}

	return 0;

err:
	for (m = i; m > 0; m--)
		devm_kfree(chip->dev, chip->child_list[m - 1].funcs);
	return rc;
}

static int oplus_chg_vpu_child_virqs_init(struct oplus_virtual_ufcs_ic *chip,
					 int child_num)
{
	struct device_node *node = chip->dev->of_node;
	struct device_node *virq_node = NULL;
	int i, m;
	int rc = 0;

	for (i = 0; i < child_num; i++) {
		virq_node = of_parse_phandle(node, "oplus,ufcs_ic_func_group", i);
		if (virq_node == NULL) {
			chg_err("can't get ic[%d] function group\n", i);
			rc = -ENODATA;
			goto err;
		}
		rc = of_property_count_elems_of_size(virq_node, "virqs", sizeof(u32));
		if (rc <= 0) {
			chip->child_list[i].virq_num = 0;
			chip->child_list[i].virqs = NULL;
			continue;
		}
		chip->child_list[i].virq_num = rc;
		chip->child_list[i].virqs = devm_kzalloc(
			chip->dev,
			sizeof(enum oplus_chg_ic_func) * chip->child_list[i].virq_num,
			GFP_KERNEL);
		if (chip->child_list[i].virqs == NULL) {
			rc = -ENOMEM;
			chg_err("alloc child ic virqs memory error\n");
			goto err;
		}
		rc = of_property_read_u32_array(
			virq_node, "virqs", (u32 *)chip->child_list[i].virqs,
			chip->child_list[i].virq_num);
		if (rc) {
			i++;
			chg_err("can't get ic[%d] virqs, rc=%d\n", i, rc);
			goto err;
		}
		(void)oplus_chg_ic_irq_table_sort(chip->child_list[i].virqs, chip->child_list[i].virq_num);
	}

	return 0;

err:
	for (m = i; m > 0; m--) {
		if (chip->child_list[m - 1].virqs != NULL)
			devm_kfree(chip->dev, chip->child_list[m - 1].virqs);
	}
	return rc;
}

static void oplus_vpu_child_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_virtual_ufcs_child *child;
	struct oplus_chg_ic_dev *parent;
	struct oplus_virtual_ufcs_ic *chip;
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

	rc = oplus_chg_vpu_virq_register(chip);
	if (rc < 0) {
		chg_err("%s virq register error, rc=%d\n", ic->name, rc);
		return;
	}

	if (!parent->online) {
		parent->online = true;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_INIT);
	}
}

static int oplus_chg_vpu_child_init(struct oplus_virtual_ufcs_ic *chip)
{
	struct device_node *node = chip->dev->of_node;
	int i;
	int rc = 0;
	const char *name;

	rc = of_property_count_elems_of_size(node, "oplus,ufcs_ic", sizeof(u32));
	if (rc < 0) {
		chg_err("can't get ufcs ic number, rc=%d\n", rc);
		return rc;
	}
	chip->child_num = rc;
	chip->child_list = devm_kzalloc(
		chip->dev,
		sizeof(struct oplus_virtual_ufcs_child) * chip->child_num,
		GFP_KERNEL);
	if (chip->child_list == NULL) {
		rc = -ENOMEM;
		chg_err("alloc child ic memory error\n");
		return rc;
	}

	for (i = 0; i < chip->child_num; i++) {
		chip->child_list[i].index = i;
		chip->child_list[i].parent = chip->ic_dev;
		INIT_WORK(&chip->child_list[i].online_work, oplus_vpu_online_work);
		INIT_WORK(&chip->child_list[i].offline_work, oplus_vpu_offline_work);
		name = of_get_oplus_chg_ic_name(node, "oplus,ufcs_ic", i);
		rc = oplus_chg_ic_wait_ic_timeout(name, oplus_vpu_child_reg_callback, &chip->child_list[i],
						  msecs_to_jiffies(UFCS_REG_TIMEOUT_MS));
		if (rc < 0) {
			chg_err("can't wait ic[%d](%s), rc=%d\n", i, name, rc);
			goto read_property_err;
		}
	}

	rc = oplus_chg_vpu_child_funcs_init(chip, chip->child_num);
	if (rc < 0)
		goto child_funcs_init_err;
	rc = oplus_chg_vpu_child_virqs_init(chip, chip->child_num);
	if (rc < 0)
		goto child_virqs_init_err;

	return 0;

child_virqs_init_err:
	for (i = 0; i < chip->child_num; i++)
		devm_kfree(chip->dev, chip->child_list[i].funcs);
child_funcs_init_err:
read_property_err:
	for (; i >=0; i--)
		chip->child_list[i].ic_dev = NULL;
	devm_kfree(chip->dev, chip->child_list);
	return rc;
}

static int oplus_chg_vpu_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int i, m;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);

	if (ic_dev->online) {
		return 0;
	}

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev, OPLUS_IC_FUNC_INIT);
		if (rc < 0) {
			chg_err("child ic[%d] init error, rc=%d\n", i, rc);
			goto child_init_err;
		}
		oplus_chg_ic_set_parent(vpu->child_list[i].ic_dev, ic_dev);
	}

	ic_dev->online = true;

	return 0;

child_init_err:
	for (m = i + 1; m > 0; m--)
		oplus_chg_ic_func(vpu->child_list[m - 1].ic_dev, OPLUS_IC_FUNC_EXIT);

	return rc;
}

static int oplus_chg_vpu_exit(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int i;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev, OPLUS_IC_FUNC_EXIT);
		if (rc < 0)
			chg_err("child ic[%d] exit error, rc=%d\n", i, rc);
	}
	for (i = 0; i < vpu->child_num; i++) {
		if (virq_is_support(&vpu->child_list[i], OPLUS_IC_VIRQ_ERR)) {
			oplus_chg_ic_virq_release(vpu->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_ERR, vpu);
		}
		if (virq_is_support(&vpu->child_list[i], OPLUS_IC_VIRQ_ONLINE)) {
			oplus_chg_ic_virq_release(vpu->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_ONLINE, vpu);
		}
		if (virq_is_support(&vpu->child_list[i], OPLUS_IC_VIRQ_OFFLINE)) {
			oplus_chg_ic_virq_release(vpu->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_OFFLINE, vpu);
		}
	}
	for (i = 0; i < vpu->child_num; i++) {
		if (vpu->child_list[i].virqs != NULL)
			devm_kfree(vpu->dev, vpu->child_list[i].virqs);
	}
	for (i = 0; i < vpu->child_num; i++)
		devm_kfree(vpu->dev, vpu->child_list[i].funcs);

	return 0;
}

static int oplus_chg_vpu_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	return 0;
}

static int oplus_chg_vpu_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	return 0;
}

static int oplus_chg_vpu_handshake(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_HANDSHAKE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_HANDSHAKE);
		if (rc < 0)
			chg_err("child ic[%d] handshake error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_pdo_set(struct oplus_chg_ic_dev *ic_dev, int vol_mv, int curr_ma)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_PDO_SET)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_PDO_SET, vol_mv, curr_ma);
		if (rc < 0)
			chg_err("child ic[%d] pdo_set error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_hard_reset(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_HARD_RESET)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_HARD_RESET);
		if (rc < 0)
			chg_err("child ic[%d] hard_reset error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_exit_ufcs_mode(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_EXIT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_EXIT);
		if (rc < 0)
			chg_err("child ic[%d] exit_ufcs_mode error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_config_wd(struct oplus_chg_ic_dev *ic_dev, u16 time_ms)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_CONFIG_WD)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_CONFIG_WD, time_ms);
		if (rc < 0)
			chg_err("child ic[%d] config_wd error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_dev_info(struct oplus_chg_ic_dev *ic_dev, u64 *dev_info)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (dev_info == NULL) {
		chg_err("dev_info buf is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_DEV_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_DEV_INFO, dev_info);
		if (rc < 0)
			chg_err("child ic[%d] get_dev_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_err_info(struct oplus_chg_ic_dev *ic_dev, u64 *err_info)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (err_info == NULL) {
		chg_err("err_info buf is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_ERR_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_ERR_INFO, err_info);
		if (rc < 0)
			chg_err("child ic[%d] get_err_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_src_info(struct oplus_chg_ic_dev *ic_dev, u64 *src_info)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (src_info == NULL) {
		chg_err("src_info buf is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_SRC_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_SRC_INFO, src_info);
		if (rc < 0)
			chg_err("child ic[%d] get_src_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_cable_info(struct oplus_chg_ic_dev *ic_dev, u64 *cable_info)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (cable_info == NULL) {
		chg_err("cable_info buf is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_CABLE_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_CABLE_INFO, cable_info);
		if (rc < 0)
			chg_err("child ic[%d] get_cable_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_pdo_info(struct oplus_chg_ic_dev *ic_dev, u64 *pdo, int num)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (pdo == NULL) {
		chg_err("pdo buf is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_PDO_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_PDO_INFO, pdo, num);
		if (rc < 0)
			chg_err("child ic[%d] get_pdo_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_verify_adapter(struct oplus_chg_ic_dev *ic_dev,
	u8 key_index, u8 *auth_data, u8 data_len)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_VERIFY_ADAPTER)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_VERIFY_ADAPTER, key_index, auth_data, data_len);
		if (rc < 0)
			chg_err("child ic[%d] verify_adapter error, rc=%d\n", i, rc);
	}
	return rc;
}

static int oplus_chg_vpu_get_power_change_info(
	struct oplus_chg_ic_dev *ic_dev, u32 *pwr_change_info, int num)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (pwr_change_info == NULL) {
		chg_err("pwr_change_info is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_POWER_CHANGE_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_POWER_CHANGE_INFO, pwr_change_info, num);
		if (rc < 0)
			chg_err("child ic[%d] get_power_change_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_emark_info(struct oplus_chg_ic_dev *ic_dev, u64 *info)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (info == NULL) {
		chg_err("info is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_EMARK_INFO)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_EMARK_INFO, info);
		if (rc < 0)
			chg_err("child ic[%d] get_emark_info error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_get_power_info_ext(
	struct oplus_chg_ic_dev *ic_dev, u64 *info, int num)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (info == NULL) {
		chg_err("info is NULL");
		return -EINVAL;
	}
	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_GET_POWER_INFO_EXT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_GET_POWER_INFO_EXT, info, num);
		if (rc < 0)
			chg_err("child ic[%d] get_power_info_ext error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_is_test_mode(struct oplus_chg_ic_dev *ic_dev, bool *en)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_IS_TEST_MODE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_IS_TEST_MODE, en);
		if (rc < 0)
			chg_err("child ic[%d] get test mode error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vpu_is_vol_acc_test_mode(struct oplus_chg_ic_dev *ic_dev, bool *en)
{
	struct oplus_virtual_ufcs_ic *vpu;
	int rc = 0;
	int i;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vpu = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vpu->child_num; i++) {
		if (!func_is_support(&vpu->child_list[i],
				     OPLUS_IC_FUNC_UFCS_IS_VOL_ACC_TEST_MODE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vpu->child_list[i].ic_dev,
			OPLUS_IC_FUNC_UFCS_IS_VOL_ACC_TEST_MODE, en);
		if (rc < 0)
			chg_err("child ic[%d] get acc test mode error, rc=%d\n", i, rc);
	}

	return rc;
}

static void *oplus_chg_vpu_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
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
			oplus_chg_vpu_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
			oplus_chg_vpu_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP,
			oplus_chg_vpu_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST,
			oplus_chg_vpu_smt_test);
		break;
	case OPLUS_IC_FUNC_UFCS_HANDSHAKE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_HANDSHAKE,
			oplus_chg_vpu_handshake);
		break;
	case OPLUS_IC_FUNC_UFCS_PDO_SET:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_PDO_SET,
			oplus_chg_vpu_pdo_set);
		break;
	case OPLUS_IC_FUNC_UFCS_HARD_RESET:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_HARD_RESET,
			oplus_chg_vpu_hard_reset);
		break;
	case OPLUS_IC_FUNC_UFCS_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_EXIT,
			oplus_chg_vpu_exit_ufcs_mode);
		break;
	case OPLUS_IC_FUNC_UFCS_CONFIG_WD:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_CONFIG_WD,
			oplus_chg_vpu_config_wd);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_DEV_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_DEV_INFO,
			oplus_chg_vpu_get_dev_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_ERR_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_ERR_INFO,
			oplus_chg_vpu_get_err_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_SRC_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_SRC_INFO,
			oplus_chg_vpu_get_src_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_CABLE_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_CABLE_INFO,
			oplus_chg_vpu_get_cable_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_PDO_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_PDO_INFO,
			oplus_chg_vpu_get_pdo_info);
		break;
	case OPLUS_IC_FUNC_UFCS_VERIFY_ADAPTER:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_VERIFY_ADAPTER,
			oplus_chg_vpu_verify_adapter);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_POWER_CHANGE_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_POWER_CHANGE_INFO,
			oplus_chg_vpu_get_power_change_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_EMARK_INFO:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_EMARK_INFO,
			 oplus_chg_vpu_get_emark_info);
		break;
	case OPLUS_IC_FUNC_UFCS_GET_POWER_INFO_EXT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_GET_POWER_INFO_EXT,
			oplus_chg_vpu_get_power_info_ext);
		break;
	case OPLUS_IC_FUNC_UFCS_IS_TEST_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_IS_TEST_MODE,
			oplus_chg_vpu_is_test_mode);
		break;
	case OPLUS_IC_FUNC_UFCS_IS_VOL_ACC_TEST_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_UFCS_IS_VOL_ACC_TEST_MODE,
			oplus_chg_vpu_is_vol_acc_test_mode);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

struct oplus_chg_ic_virq oplus_vpu_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static int oplus_virtual_platufcs_probe(struct platform_device *pdev)
{
	struct oplus_virtual_ufcs_ic *chip;
	struct device_node *node = pdev->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	int rc = 0;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_virtual_ufcs_ic),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

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
	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "platufcs-virtual");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x01");
	ic_cfg.type = ic_type;
	ic_cfg.get_func = oplus_chg_vpu_get_func;
	ic_cfg.virq_data = oplus_vpu_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(oplus_vpu_virq_table);
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}
	rc = oplus_chg_vpu_child_init(chip);
	if (rc < 0) {
		chg_err("vpu child init error, rc = %d\n", rc);
		goto reg_ic_err;
	}

	chg_info("probe success\n");
	return 0;

reg_ic_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	chg_err("probe error, rc=%d\n", rc);
	return rc;
}

static int oplus_virtual_platufcs_remove(struct platform_device *pdev)
{
	struct oplus_virtual_ufcs_ic *chip = platform_get_drvdata(pdev);

	if(chip == NULL)
		return -ENODEV;

	if (chip->ic_dev->online)
		oplus_chg_vpu_exit(chip->ic_dev);

	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_virtual_platufcs_match[] = {
	{ .compatible = "oplus,virtual_platufcs" },
	{},
};

static struct platform_driver oplus_virtual_platufcs_driver = {
	.driver		= {
		.name = "oplus-virtual_platufcs",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_virtual_platufcs_match),
	},
	.probe		= oplus_virtual_platufcs_probe,
	.remove		= oplus_virtual_platufcs_remove,
};

static __init int oplus_virtual_platufcs_init(void)
{
#if __and(IS_BUILTIN(CONFIG_OPLUS_CHG), IS_BUILTIN(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return 0;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return 0;
#endif /* CONFIG_OPLUS_CHG_V2 */
	return platform_driver_register(&oplus_virtual_platufcs_driver);
}

static __exit void oplus_virtual_platufcs_exit(void)
{
#if __and(IS_BUILTIN(CONFIG_OPLUS_CHG), IS_BUILTIN(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return;
#endif /* CONFIG_OPLUS_CHG_V2 */
	platform_driver_unregister(&oplus_virtual_platufcs_driver);
}

oplus_chg_module_register(oplus_virtual_platufcs);
