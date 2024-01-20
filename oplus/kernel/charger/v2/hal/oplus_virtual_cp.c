// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[VIRTUAL_CP]([%s][%d]): " fmt, __func__, __LINE__

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
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/device_info.h>
#include <soc/oplus/system/oplus_project.h>
#endif
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>

#define CP_REG_TIMEOUT_MS	120000
#define PROC_DATA_BUF_SIZE	256
#define CP_NAME_BUF_MAX		128

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0))
#define pde_data(inode) PDE_DATA(inode)
#endif

struct oplus_virtual_cp_child {
	struct oplus_chg_ic_dev *parent;
	struct oplus_chg_ic_dev *ic_dev;
	int index;
	int max_curr_ma;
	struct work_struct online_work;
	struct work_struct offline_work;
};

struct oplus_virtual_cp_ic {
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	bool online;
	enum oplus_chg_ic_connect_type connect_type;
	int child_num;
	struct oplus_virtual_cp_child *child_list;

	/* parallel charge */
	int main_cp;
	int target_iin;

	bool reg_proc_node;
};

static void oplus_vc_online_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_cp_child *child = virq_data;
	schedule_work(&child->online_work);
}

static void oplus_vc_offline_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_cp_child *child = virq_data;
	schedule_work(&child->offline_work);
}

static void oplus_vc_err_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_cp_ic *chip = virq_data;

	oplus_chg_ic_move_err_msg(chip->ic_dev, ic_dev);
	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
}

static int oplus_vc_base_virq_register(struct oplus_virtual_cp_ic *chip, int index)
{
	int rc;

	rc = oplus_chg_ic_virq_register(chip->child_list[index].ic_dev,
			OPLUS_IC_VIRQ_ONLINE, oplus_vc_online_handler, &chip->child_list[index]);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(chip->child_list[index].ic_dev,
			OPLUS_IC_VIRQ_OFFLINE, oplus_vc_offline_handler, &chip->child_list[index]);
	if (rc < 0)
		chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);
	rc = oplus_chg_ic_virq_register(chip->child_list[index].ic_dev,
		OPLUS_IC_VIRQ_ERR, oplus_vc_err_handler, chip);
	if (rc < 0 && rc != -ENOTSUPP)
		chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);
	chg_info("%s virq register success\n", chip->child_list[index].ic_dev->name);

	return 0;
}

static void oplus_vc_online_work(struct work_struct *work)
{
	struct oplus_virtual_cp_child *child =
		container_of(work, struct oplus_virtual_cp_child, online_work);
	struct oplus_virtual_cp_ic *chip;

	chg_info("%s online\n", child->ic_dev->name);
	chip = oplus_chg_ic_get_drvdata(child->parent);

	if (!child->parent->online && child->index == chip->main_cp) {
		child->parent->online = true;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_INIT);
	}
}

static void oplus_vc_offline_work(struct work_struct *work)
{
	struct oplus_virtual_cp_child *child =
		container_of(work, struct oplus_virtual_cp_child, offline_work);
	struct oplus_virtual_cp_ic *chip;

	chg_info("%s offline\n", child->ic_dev->name);
	chip = oplus_chg_ic_get_drvdata(child->parent);

	if (child->parent->online && child->index == chip->main_cp) {
		child->parent->online = false;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_EXIT);
		oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_OFFLINE);
	}
}

static void oplus_vc_child_reg_callback(struct oplus_chg_ic_dev *ic, void *data, bool timeout)
{
	struct oplus_virtual_cp_child *child;
	struct oplus_chg_ic_dev *parent;
	struct oplus_virtual_cp_ic *chip;
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
		if (chip->main_cp == child->index) {
			/* TODO: Add the main cp switching function after IC registration timeout */
		}
		return;
	}

	child->ic_dev = ic;
	oplus_chg_ic_set_parent(ic, parent);

	rc = oplus_vc_base_virq_register(chip, child->index);
	if (rc < 0) {
		chg_err("%s virq register error, rc=%d\n", ic->name, rc);
		return;
	}

	if (!parent->online && child->index == chip->main_cp) {
		parent->online = true;
		oplus_chg_ic_func(child->parent, OPLUS_IC_FUNC_INIT);
	}
}

static int oplus_vc_child_init(struct oplus_virtual_cp_ic *chip)
{
	struct device_node *node = chip->dev->of_node;
	int i;
	int rc = 0;
	const char *name;

	rc = of_property_read_u32(node, "oplus,cp_ic_connect",
				  &chip->connect_type);
	if (rc < 0) {
		chg_err("can't get cp ic connect type, rc=%d\n", rc);
		return rc;
	}
	chip->main_cp = 0;
	rc = of_property_count_elems_of_size(node, "oplus,cp_ic",
					     sizeof(u32));
	if (rc < 0) {
		chg_err("can't get cp ic number, rc=%d\n", rc);
		return rc;
	}
	chip->child_num = rc;
	chip->child_list = devm_kzalloc(
		chip->dev,
		sizeof(struct oplus_virtual_cp_child) * chip->child_num,
		GFP_KERNEL);
	if (chip->child_list == NULL) {
		rc = -ENOMEM;
		chg_err("alloc child ic memory error\n");
		return rc;
	}

	for (i = 0; i < chip->child_num; i++) {
		rc = of_property_read_u32_index(
			node, "oplus,input_curr_max_ma", i,
			&chip->child_list[i].max_curr_ma);
		if (rc < 0) {
			chg_err("can't read ic[%d] oplus,input_curr_max_ma, rc=%d\n", i, rc);
			goto read_property_err;
		}

		chip->child_list[i].index = i;
		chip->child_list[i].parent = chip->ic_dev;
		INIT_WORK(&chip->child_list[i].online_work, oplus_vc_online_work);
		INIT_WORK(&chip->child_list[i].offline_work, oplus_vc_offline_work);
		name = of_get_oplus_chg_ic_name(node, "oplus,cp_ic", i);
		rc = oplus_chg_ic_wait_ic_timeout(name, oplus_vc_child_reg_callback, &chip->child_list[i],
						  msecs_to_jiffies(CP_REG_TIMEOUT_MS));
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

static int oplus_chg_vc_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_cp_ic *chip;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	chip = oplus_chg_ic_get_drvdata(ic_dev);

	if (ic_dev->online)
		return 0;
	ic_dev->online = true;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_ONLINE);

	return 0;
}

static int oplus_chg_vc_exit(struct oplus_chg_ic_dev *ic_dev)
{
	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;
	oplus_chg_ic_virq_trigger(ic_dev, OPLUS_IC_VIRQ_OFFLINE);

	chg_info("unregister success\n");
	return 0;
}

static int oplus_chg_vc_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev, OPLUS_IC_FUNC_REG_DUMP);
		if (rc < 0)
			chg_err("child ic[%d] exit error, rc=%d\n", i, rc);
	}

	return 0;
}

static int oplus_chg_vc_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	struct oplus_virtual_cp_ic *vc;
	int i, index;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	index = 0;
	for (i = 0; i < vc->child_num; i++) {
		if (index >= len)
			return len;
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				       OPLUS_IC_FUNC_SMT_TEST, buf + index,
				       len - index);
		if (rc < 0) {
			if (rc != -ENOTSUPP) {
				chg_err("child ic[%d] smt test error, rc=%d\n",
					i, rc);
				rc = snprintf(buf + index, len - index,
					"[%s]-[%s]:%d\n",
					vc->child_list[i].ic_dev->manu_name,
					"FUNC_ERR", rc);
			} else {
				rc = 0;
			}
		} else {
			if ((rc > 0) && buf[index + rc - 1] != '\n') {
				buf[index + rc] = '\n';
				index++;
			}
		}
		index += rc;
	}

	return index;
}

static int oplus_chg_vc_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_ENABLE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		en = oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev, OPLUS_IC_FUNC_CP_ENABLE, en);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] %s error, rc=%d\n", i, en ? "enable" : "disable", rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_vc_hw_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev, OPLUS_IC_FUNC_CP_HW_INTI);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] hw init error, rc=%d\n", i, rc);
			/* TODO: need push ic error msg, and offline child ic */
			return rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static int oplus_chg_vc_set_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_SET_WORK_MODE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		mode = oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev, OPLUS_IC_FUNC_CP_SET_WORK_MODE, mode);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] set work mode to %d error, rc=%d\n", i, mode, rc);
				/* TODO: need push ic error msg, and offline child ic */
				return rc;
			} else if (rc >= 0) {
				err = rc;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_get_work_mode(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode *mode)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
	enum oplus_cp_work_mode mode_backup;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_MODE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*mode = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_MODE, mode);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] get work mode error, rc=%d\n", i, rc);
				return rc;
			} else if (rc >= 0) {
				err = rc;
			}
			if (i == 0) {
				mode_backup = *mode;
			} else if (mode_backup != *mode) {
				chg_err("child ic[%d] work mode expected to be %d, but it was actually %d\n",
					i, mode_backup, *mode);
				return -EINVAL;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_check_work_mode_support(struct oplus_chg_ic_dev *ic_dev, enum oplus_cp_work_mode mode)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
	bool support = false;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		return !!oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT, mode);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] check work mode support error, rc=%d\n", i, rc);
				return rc;
			}
			if (rc == 0) {
				return 0;
			} else if (rc > 0) {
				support = true;
				err = rc;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err < 0 ? err : support;
}

static int oplus_chg_vc_set_iin(struct oplus_chg_ic_dev *ic_dev, int iin)
{
	struct oplus_virtual_cp_ic *vc;
	bool work_start;
	int curr_sum;
	int curr_min;
	int start_num;
	int i;
	int rc;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_SET_IIN);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		iin = oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	if (vc->connect_type != OPLUS_CHG_IC_CONNECT_PARALLEL)
		return -ENOTSUPP;

	if (vc->target_iin == iin)
		return 0;

	rc = oplus_chg_ic_func(ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_STATUS, &work_start);
	if (rc < 0) {
		chg_err("can't get cp work status, rc=%d\n", rc);
		return rc;
	}
	if (!work_start) {
		vc->target_iin = 0;
		return 0;
	}

	curr_min = vc->child_list[vc->main_cp].max_curr_ma;
	start_num = 1;

	for (i = 0; i < vc->child_num; i++) {
		if (i == vc->main_cp)
			continue;
		if (!vc->child_list[i].ic_dev->online) {
			chg_err("%s offline\n", vc->child_list[i].ic_dev->name);
			continue;
		}
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_WORK_STATUS, &work_start);
		if (rc < 0) {
			chg_err("can't get cp[%d] work status, rc=%d\n", i, rc);
			continue;
		}
		if (work_start) {
			if (vc->child_list[i].max_curr_ma < curr_min)
				curr_min = vc->child_list[i].max_curr_ma;
			start_num++;
		}
	}

	curr_sum = curr_min * start_num;
	if (curr_sum >= iin) {
		vc->target_iin = iin;
		return 0;
	}

	for (i = 0; i < vc->child_num; i++) {
		if (i == vc->main_cp)
			continue;
		if (!vc->child_list[i].ic_dev->online) {
			chg_err("%s offline\n", vc->child_list[i].ic_dev->name);
			continue;
		}
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_WORK_STATUS, &work_start);
		if (rc < 0) {
			chg_err("can't get cp[%d] work status, rc=%d\n", i, rc);
			continue;
		}
		if (work_start)
			continue;

		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_SET_WORK_START, true);
		if (rc < 0) {
			chg_err("can't set cp[%d] work start, rc=%d\n", i, rc);
			continue;
		}
		if (vc->child_list[i].max_curr_ma < curr_min)
			curr_min = vc->child_list[i].max_curr_ma;
		start_num++;
		curr_sum = curr_min * start_num;
		if (curr_sum >= iin) {
			vc->target_iin = iin;
			return 0;
		}
	}

	chg_err("input current is too large, iin=%d\n", iin);

	return -EINVAL;
}

static int oplus_chg_vc_get_vin(struct oplus_chg_ic_dev *ic_dev, int *vin)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_VIN);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*vin = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				OPLUS_IC_FUNC_CP_GET_VIN, vin);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] get vin error, rc=%d\n", i, rc);
				err = rc;
			} else if (rc >= 0) {
				return 0;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_get_iin(struct oplus_chg_ic_dev *ic_dev, int *iin)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
	int curr;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_IIN);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*iin = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	*iin = 0;
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				OPLUS_IC_FUNC_CP_GET_IIN, &curr);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] get iin error, rc=%d\n", i, rc);
				return rc;
			} else if (rc >= 0) {
				*iin += curr;
				err = rc;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_get_vout(struct oplus_chg_ic_dev *ic_dev, int *vout)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_VOUT);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*vout = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				OPLUS_IC_FUNC_CP_GET_VOUT, vout);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] get vout error, rc=%d\n", i, rc);
				err = rc;
			} else if (rc >= 0) {
				return 0;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_get_iout(struct oplus_chg_ic_dev *ic_dev, int *iout)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
	int curr;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_IOUT);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*iout = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	*iout = 0;
	for (i = 0; i < vc->child_num; i++) {
		if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
			rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
				OPLUS_IC_FUNC_CP_GET_IOUT, &curr);
			if (rc < 0 && rc != -ENOTSUPP) {
				chg_err("child ic[%d] get iout error, rc=%d\n", i, rc);
				return rc;
			} else if (rc >= 0) {
				*iout += curr;
				err = rc;
			}
		} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
			/* TODO */
			return -ENOTSUPP;
		} else {
			chg_err("Unknown connect type\n");
			return -EINVAL;
		}
	}

	return err;
}

static int oplus_chg_vc_get_vac(struct oplus_chg_ic_dev *ic_dev, int *vac)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc;
	int err = -ENOTSUPP;
	int vol;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_VAC);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*vac = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_VAC, &vol);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] get iout error, rc=%d\n", i, rc);
			err = rc;
		} else if (rc >= 0) {
			*vac = vol;
			return 0;
		}
	}

	return err;
}

static int oplus_chg_vc_set_work_start(struct oplus_chg_ic_dev *ic_dev, bool start)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_SET_WORK_START);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		start = !!oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	vc->target_iin = 0;
	if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
		if (start) {
			/* Only the main CP is allowed to be turned on here */
			if (vc->main_cp < 0 || vc->main_cp >= vc->child_num)
				return -EINVAL;
			rc = oplus_chg_ic_func(vc->child_list[vc->main_cp].ic_dev,
					OPLUS_IC_FUNC_CP_SET_WORK_START, true);
			if (rc < 0 && rc != -ENOTSUPP)
				chg_err("main cp set start work error, rc=%d\n", rc);
			return rc;
		} else {
			/* TODO */
			for (i = vc->child_num - 1; i >= 0; i--) {
				rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
					OPLUS_IC_FUNC_CP_SET_WORK_START, false);
				if (rc < 0 && rc != -ENOTSUPP) {
					chg_err("child ic[%d] set stop work error, rc=%d\n", i, rc);
					err = rc;
				} else if (rc >= 0) {
					err = rc;
				}
			}
		}
	} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
		/* TODO */
		return -ENOTSUPP;
	} else {
		chg_err("Unknown connect type\n");
		return -EINVAL;
	}

	return err;
}

static int oplus_chg_vc_get_work_status(struct oplus_chg_ic_dev *ic_dev, bool *start)
{
	struct oplus_virtual_cp_ic *vc;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_GET_WORK_STATUS);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		*start = oplus_chg_ic_get_item_data(buf, 0);
		return 0;
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	if (vc->connect_type == OPLUS_CHG_IC_CONNECT_PARALLEL) {
		/* After the main CP is turned on, it is considered to be turned on */
		if (vc->main_cp < 0 || vc->main_cp >= vc->child_num)
			return -EINVAL;
		rc = oplus_chg_ic_func(vc->child_list[vc->main_cp].ic_dev,
				OPLUS_IC_FUNC_CP_GET_WORK_STATUS, start);
		if (rc < 0 && rc != -ENOTSUPP)
			chg_err("main cp get work status error, rc=%d\n", rc);
		return rc;
	} else if (vc->connect_type == OPLUS_CHG_IC_CONNECT_SERIAL) {
		/* TODO */
		return -ENOTSUPP;
	} else {
		chg_err("Unknown connect type\n");
		return -EINVAL;
	}

	return err;
}

static int oplus_chg_vc_adc_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	struct oplus_virtual_cp_ic *vc;
	int i;
	int rc = 0;
	int err = -ENOTSUPP;
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	struct oplus_chg_ic_overwrite_data *data;
	const void *buf;
#endif

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL");
		return -ENODEV;
	}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	data = oplus_chg_ic_get_overwrite_data(ic_dev, OPLUS_IC_FUNC_CP_SET_ADC_ENABLE);
	if (unlikely(data != NULL)) {
		buf = (const void *)data->buf;
		if (!oplus_chg_ic_debug_data_check(buf, data->size))
			return -EINVAL;
		en = !!oplus_chg_ic_get_item_data(buf, 0);
	}
#endif

	vc = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vc->child_num; i++) {
		rc = oplus_chg_ic_func(vc->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_SET_ADC_ENABLE, en);
		if (rc < 0 && rc != -ENOTSUPP) {
			chg_err("child ic[%d] %s error, rc=%d\n", i, en ? "enable" : "disable", rc);
			err = rc;
		} else if (rc >= 0) {
			err = rc;
		}
	}

	return err;
}

static void *oplus_chg_vc_get_func(struct oplus_chg_ic_dev *ic_dev, enum oplus_chg_ic_func func_id)
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
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_INIT, oplus_chg_vc_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT, oplus_chg_vc_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP, oplus_chg_vc_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST, oplus_chg_vc_smt_test);
		break;
	case OPLUS_IC_FUNC_CP_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_ENABLE, oplus_chg_vc_enable);
		break;
	case OPLUS_IC_FUNC_CP_HW_INTI:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_HW_INTI, oplus_chg_vc_hw_init);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_MODE, oplus_chg_vc_set_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_MODE, oplus_chg_vc_get_work_mode);
		break;
	case OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT,
					       oplus_chg_vc_check_work_mode_support);
		break;
	case OPLUS_IC_FUNC_CP_SET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_IIN, oplus_chg_vc_set_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VIN, oplus_chg_vc_get_vin);
		break;
	case OPLUS_IC_FUNC_CP_GET_IIN:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IIN, oplus_chg_vc_get_iin);
		break;
	case OPLUS_IC_FUNC_CP_GET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VOUT, oplus_chg_vc_get_vout);
		break;
	case OPLUS_IC_FUNC_CP_GET_IOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_IOUT, oplus_chg_vc_get_iout);
		break;
	case OPLUS_IC_FUNC_CP_GET_VAC:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_VAC, oplus_chg_vc_get_vac);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_START:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_WORK_START, oplus_chg_vc_set_work_start);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_GET_WORK_STATUS, oplus_chg_vc_get_work_status);
		break;
	case OPLUS_IC_FUNC_CP_SET_ADC_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_CP_SET_ADC_ENABLE, oplus_chg_vc_adc_enable);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
static int oplus_chg_vc_set_func_data(struct oplus_chg_ic_dev *ic_dev,
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
		rc = oplus_chg_vc_init(ic_dev);
		break;
	case OPLUS_IC_FUNC_EXIT:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_exit(ic_dev);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_reg_dump(ic_dev);
		break;
	case OPLUS_IC_FUNC_CP_ENABLE:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_enable(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	case OPLUS_IC_FUNC_CP_HW_INTI:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_hw_init(ic_dev);
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_MODE:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_set_work_mode(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	case OPLUS_IC_FUNC_CP_SET_IIN:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_set_iin(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	case OPLUS_IC_FUNC_CP_SET_WORK_START:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_set_work_start(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	case OPLUS_IC_FUNC_CP_SET_ADC_ENABLE:
		if (!oplus_chg_ic_debug_data_check(buf, buf_len))
			return -EINVAL;
		rc = oplus_chg_vc_adc_enable(ic_dev, oplus_chg_ic_get_item_data(buf, 0));
		break;
	default:
		chg_err("this func(=%d) is not supported to set\n", func_id);
		return -ENOTSUPP;
		break;
	}

	return rc;
}

static ssize_t oplus_chg_vc_get_func_data(struct oplus_chg_ic_dev *ic_dev,
					  enum oplus_chg_ic_func func_id,
					  void *buf)
{
	bool temp;
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
		rc = oplus_chg_vc_smt_test(ic_dev, tmp_buf, PAGE_SIZE);
		if (rc < 0) {
			free_page((unsigned long)tmp_buf);
			break;
		}
		len = oplus_chg_ic_debug_str_data_init(buf, rc);
		memcpy(oplus_chg_ic_get_item_data_addr(buf, 0), tmp_buf, rc);
		free_page((unsigned long)tmp_buf);
		rc = len;
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_MODE:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_work_mode(ic_dev, (enum oplus_cp_work_mode *)item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_VIN:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_vin(ic_dev, item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_IIN:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_iin(ic_dev, item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_VOUT:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_vout(ic_dev, item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_IOUT:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_iout(ic_dev, item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_VAC:
		oplus_chg_ic_debug_data_init(buf, 1);
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		rc = oplus_chg_vc_get_vac(ic_dev, item_data);
		if (rc < 0)
			break;
		*item_data = cpu_to_le32(*item_data);
		rc = oplus_chg_ic_debug_data_size(1);
		break;
	case OPLUS_IC_FUNC_CP_GET_WORK_STATUS:
		oplus_chg_ic_debug_data_init(buf, 1);
		rc = oplus_chg_vc_get_work_status(ic_dev, &temp);
		if (rc < 0)
			break;
		item_data = oplus_chg_ic_get_item_data_addr(buf, 0);
		*item_data = temp;
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

enum oplus_chg_ic_func oplus_vc_overwrite_funcs[] = {
	OPLUS_IC_FUNC_CP_ENABLE,
	OPLUS_IC_FUNC_CP_HW_INTI,
	OPLUS_IC_FUNC_CP_SET_WORK_MODE,
	OPLUS_IC_FUNC_CP_GET_WORK_MODE,
	OPLUS_IC_FUNC_CP_CHECK_WORK_MODE_SUPPORT,
	OPLUS_IC_FUNC_CP_SET_IIN,
	OPLUS_IC_FUNC_CP_GET_VIN,
	OPLUS_IC_FUNC_CP_GET_IIN,
	OPLUS_IC_FUNC_CP_GET_VOUT,
	OPLUS_IC_FUNC_CP_GET_IOUT,
	OPLUS_IC_FUNC_CP_GET_VAC,
	OPLUS_IC_FUNC_CP_SET_WORK_START,
	OPLUS_IC_FUNC_CP_GET_WORK_STATUS,
	OPLUS_IC_FUNC_CP_SET_ADC_ENABLE,
};

#endif /* CONFIG_OPLUS_CHG_IC_DEBUG */

struct oplus_chg_ic_virq oplus_vc_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
};

static ssize_t oplus_vc_iin_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int curr;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_IIN, &curr);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "NULL");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "%d", curr);
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_iin_ops =
{
	.read = oplus_vc_iin_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_iin_ops =
{
	.proc_read  = oplus_vc_iin_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_vin_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int vol;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_VIN, &vol);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "NULL");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "%d", vol);
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_vin_ops =
{
	.read = oplus_vc_vin_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_vin_ops =
{
	.proc_read  = oplus_vc_vin_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_iout_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int curr;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_IOUT, &curr);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "NULL");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "%d", curr);
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_iout_ops =
{
	.read = oplus_vc_iout_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_iout_ops =
{
	.proc_read  = oplus_vc_iout_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_vout_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int vol;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_VOUT, &vol);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "NULL");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "%d", vol);
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_vout_ops =
{
	.read = oplus_vc_vout_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_vout_ops =
{
	.proc_read  = oplus_vc_vout_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_number_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int len;

	len = snprintf(buf, ARRAY_SIZE(buf), "%d\n", chip->child_num);
	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_number_ops =
{
	.read = oplus_vc_number_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_number_ops =
{
	.proc_read  = oplus_vc_number_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_work_mode_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	enum oplus_cp_work_mode mode;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_WORK_MODE, &mode);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Unknown");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		switch (mode) {
		case CP_WORK_MODE_AUTO:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Auto");
			break;
		case CP_WORK_MODE_BYPASS:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "1:1");
			break;
		case CP_WORK_MODE_2_TO_1:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "2:1");
			break;
		case CP_WORK_MODE_3_TO_1:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "3:1");
			break;
		case CP_WORK_MODE_4_TO_1:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "4:1");
			break;
		case CP_WORK_MODE_1_TO_2:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "1:2");
			break;
		case CP_WORK_MODE_1_TO_3:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "1:3");
			break;
		case CP_WORK_MODE_1_TO_4:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "1:4");
			break;
		default:
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Unknown");
			break;
		}
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_work_mode_ops =
{
	.read = oplus_vc_work_mode_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_work_mode_ops =
{
	.proc_read  = oplus_vc_work_mode_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_support_mode_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	return -ENOTSUPP;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_support_mode_ops =
{
	.read = oplus_vc_support_mode_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_support_mode_ops =
{
	.proc_read  = oplus_vc_support_mode_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_work_status_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	bool work_start;
	int i;
	int len = 0;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (i > 0)
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, ",");
		rc = oplus_chg_ic_func(chip->child_list[i].ic_dev,
			OPLUS_IC_FUNC_CP_GET_WORK_STATUS, &work_start);
		if (rc == -ENOTSUPP) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Unknown");
			continue;
		} else if (rc < 0) {
			len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "Error(%d)", rc);
			continue;
		}
		len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "%s", work_start ? "Open" : "Close");
	}
	len += snprintf(buf + len, ARRAY_SIZE(buf) - len, "\n");

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_work_status_ops =
{
	.read = oplus_vc_work_status_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_work_status_ops =
{
	.proc_read  = oplus_vc_work_status_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static ssize_t oplus_vc_connect_type_proc_read(struct file *file, char __user *buff, size_t count, loff_t *off)
{
	struct oplus_virtual_cp_ic *chip = pde_data(file_inode(file));
	char buf[PROC_DATA_BUF_SIZE];
	int len;

	switch(chip->connect_type) {
	case OPLUS_CHG_IC_CONNECT_PARALLEL:
		len = snprintf(buf, ARRAY_SIZE(buf), "Parallel\n");
		break;
	case OPLUS_CHG_IC_CONNECT_SERIAL:
		len = snprintf(buf, ARRAY_SIZE(buf), "Serial\n");
		break;
	default:
		len = snprintf(buf, ARRAY_SIZE(buf), "Unknown\n");
		break;
	}

	if (len > *off)
		len -= *off;
	else
		len = 0;
	if (copy_to_user(buff, buf, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations oplus_vc_connect_type_ops =
{
	.read = oplus_vc_connect_type_proc_read,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops oplus_vc_connect_type_ops =
{
	.proc_read  = oplus_vc_connect_type_proc_read,
	.proc_lseek = noop_llseek,
};
#endif

static int oplus_virtual_cp_proc_init(struct oplus_virtual_cp_ic *chip)
{
	struct proc_dir_entry *cp_entry;
	struct proc_dir_entry *pr_entry_tmp;
	char name_buf[CP_NAME_BUF_MAX] = { 0 };

	snprintf(name_buf, CP_NAME_BUF_MAX - 1, "charger/cp:%d", chip->ic_dev->index);
	cp_entry = proc_mkdir(name_buf, NULL);
	if (cp_entry == NULL) {
		chg_err("Couldn't create charger/cp proc entry\n");
		return -EFAULT;
	}

	pr_entry_tmp = proc_create_data("iin", 0644, cp_entry, &oplus_vc_iin_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create iin proc entry\n");
	pr_entry_tmp = proc_create_data("vin", 0644, cp_entry, &oplus_vc_vin_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create vin proc entry\n");
	pr_entry_tmp = proc_create_data("iout", 0644, cp_entry, &oplus_vc_iout_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create iout proc entry\n");
	pr_entry_tmp = proc_create_data("vout", 0644, cp_entry, &oplus_vc_vout_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create vout proc entry\n");
	pr_entry_tmp = proc_create_data("number", 0644, cp_entry, &oplus_vc_number_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create number proc entry\n");
	pr_entry_tmp = proc_create_data("work_mode", 0644, cp_entry, &oplus_vc_work_mode_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create work_mode proc entry\n");
	pr_entry_tmp = proc_create_data("support_mode", 0644, cp_entry, &oplus_vc_support_mode_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create support_mode proc entry\n");
	pr_entry_tmp = proc_create_data("work_status", 0644, cp_entry, &oplus_vc_work_status_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create work_status proc entry\n");
	pr_entry_tmp = proc_create_data("connect_type", 0644, cp_entry, &oplus_vc_connect_type_ops, chip);
	if (pr_entry_tmp == NULL)
		chg_err("Couldn't create connect_type proc entry\n");

	return 0;
}

static int oplus_virtual_cp_probe(struct platform_device *pdev)
{
	struct oplus_virtual_cp_ic *chip;
	struct device_node *node = pdev->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	char name_buf[CP_NAME_BUF_MAX] = { 0 };
	static int retry_count = 0;
	int rc;

#define PROBE_RETRY_MAX	300

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_virtual_cp_ic),
			    GFP_KERNEL);
	if (chip == NULL) {
		chg_err("alloc memory error\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "cp-virtual");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.get_func = oplus_chg_vc_get_func;
	ic_cfg.virq_data = oplus_vc_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(oplus_vc_virq_table);
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}
#ifdef CONFIG_OPLUS_CHG_IC_DEBUG
	chip->ic_dev->debug.get_func_data = oplus_chg_vc_get_func_data;
	chip->ic_dev->debug.set_func_data = oplus_chg_vc_set_func_data;
	oplus_chg_ic_func_table_sort(oplus_vc_overwrite_funcs, ARRAY_SIZE(oplus_vc_overwrite_funcs));
	chip->ic_dev->debug.overwrite_funcs = oplus_vc_overwrite_funcs;
	chip->ic_dev->debug.func_num = ARRAY_SIZE(oplus_vc_overwrite_funcs);
#endif

	chip->reg_proc_node = of_property_read_bool(node, "oplus,reg_proc_node");
	if (chip->reg_proc_node) {
		rc = oplus_virtual_cp_proc_init(chip);
		if (rc < 0) {
			rc = -EPROBE_DEFER;
			goto proc_init_err;
		}
	}

	rc = oplus_vc_child_init(chip);
	if (rc < 0) {
		chg_err("child ic init error, rc=%d\n", rc);
		goto child_init_err;
	}

	chg_err("probe success\n");
	return 0;

child_init_err:
	if (chip->reg_proc_node) {
		snprintf(name_buf, CP_NAME_BUF_MAX - 1, "charger/cp:%d", chip->ic_dev->index);
		remove_proc_entry(name_buf, NULL);
	}
proc_init_err:
	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
reg_ic_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	if (rc == -EPROBE_DEFER && retry_count < PROBE_RETRY_MAX) {
		retry_count++;
		return rc;
	}
	chg_err("probe error, retry=%d, rc=%d\n", retry_count, rc);
	return rc;
}

static int oplus_virtual_cp_remove(struct platform_device *pdev)
{
	struct oplus_virtual_cp_ic *chip = platform_get_drvdata(pdev);
	char name_buf[CP_NAME_BUF_MAX] = { 0 };

	if(chip == NULL)
		return -ENODEV;

	if (chip->ic_dev->online)
		oplus_chg_vc_exit(chip->ic_dev);

	if (chip->reg_proc_node) {
		snprintf(name_buf, CP_NAME_BUF_MAX - 1, "charger/cp:%d", chip->ic_dev->index);
		remove_proc_entry(name_buf, NULL);
	}
	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
	devm_kfree(&pdev->dev, chip->child_list);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_virtual_cp_match[] = {
	{ .compatible = "oplus,virtual_cp" },
	{},
};

static struct platform_driver oplus_virtual_cp_driver = {
	.driver		= {
		.name = "oplus-virtual_cp",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_virtual_cp_match),
	},
	.probe		= oplus_virtual_cp_probe,
	.remove		= oplus_virtual_cp_remove,
};

static __init int oplus_virtual_cp_init(void)
{
	return platform_driver_register(&oplus_virtual_cp_driver);
}

static __exit void oplus_virtual_cp_exit(void)
{
	platform_driver_unregister(&oplus_virtual_cp_driver);
}

oplus_chg_module_register(oplus_virtual_cp);
