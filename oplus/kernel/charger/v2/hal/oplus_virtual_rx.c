// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[VIRTUAL_RX]([%s][%d]): " fmt, __func__, __LINE__

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
#include <linux/of_irq.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>

struct oplus_virtual_rx_child {
	struct oplus_chg_ic_dev *ic_dev;
	int index;
	enum oplus_chg_ic_func *funcs;
	int func_num;
	enum oplus_chg_ic_virq_id *virqs;
	int virq_num;

	bool initialized;
};

struct oplus_virtual_rx_ic {
	struct device *dev;
	struct oplus_chg_ic_dev *ic_dev;
	int child_num;
	struct oplus_virtual_rx_child *child_list;

	struct work_struct data_handler_work;
};

static int oplus_chg_vr_virq_register(struct oplus_virtual_rx_ic *chip);

static inline bool func_is_support(struct oplus_virtual_rx_child *ic,
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

static inline bool virq_is_support(struct oplus_virtual_rx_child *ic,
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

static int oplus_chg_vr_child_funcs_init(struct oplus_virtual_rx_ic *chip,
					 int child_num)
{
	struct device_node *node = chip->dev->of_node;
	struct device_node *func_node = NULL;
	int i, m;
	int rc = 0;

	for (i = 0; i < child_num; i++) {
		func_node = of_parse_phandle(node, "oplus,rx_ic_func_group", i);
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

static int oplus_chg_vr_child_virqs_init(struct oplus_virtual_rx_ic *chip,
					 int child_num)
{
	struct device_node *node = chip->dev->of_node;
	struct device_node *virq_node = NULL;
	int i, m;
	int rc = 0;

	for (i = 0; i < child_num; i++) {
		virq_node = of_parse_phandle(node, "oplus,rx_ic_func_group", i);
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

static int oplus_chg_vr_child_init(struct oplus_virtual_rx_ic *chip)
{
	struct device_node *node = chip->dev->of_node;
	int i;
	int rc = 0;

	rc = of_property_count_elems_of_size(node, "oplus,rx_ic", sizeof(u32));
	if (rc < 0) {
		chg_err("can't get rx ic number, rc=%d\n", rc);
		return rc;
	}
	chip->child_num = rc;
	chip->child_list = devm_kzalloc(
		chip->dev,
		sizeof(struct oplus_virtual_rx_child) * chip->child_num,
		GFP_KERNEL);
	if (chip->child_list == NULL) {
		rc = -ENOMEM;
		chg_err("alloc child ic memory error\n");
		return rc;
	}

	for (i = 0; i < chip->child_num; i++) {
		chip->child_list[i].ic_dev = of_get_oplus_chg_ic(node, "oplus,rx_ic", i);
		if (chip->child_list[i].ic_dev == NULL) {
			chg_debug("not find rx ic %d\n", i);
			rc = -EAGAIN;
			goto read_property_err;
		}
	}

	rc = oplus_chg_vr_child_funcs_init(chip, chip->child_num);
	if (rc < 0)
		goto child_funcs_init_err;
	rc = oplus_chg_vr_child_virqs_init(chip, chip->child_num);
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

static int oplus_chg_vr_init(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i, m;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	vr = oplus_chg_ic_get_drvdata(ic_dev);

	if (ic_dev->online)
		return 0;

	rc = oplus_chg_vr_child_init(vr);
	if (rc < 0) {
		if (rc != -EAGAIN)
			chg_err("child list init error, rc=%d\n", rc);
		goto child_list_init_err;
	}

	rc = oplus_chg_vr_virq_register(vr);
	if (rc < 0) {
		chg_err("virq register error, rc=%d\n", rc);
		goto virq_register_err;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev, OPLUS_IC_FUNC_INIT);
		if (rc < 0) {
			chg_err("child ic[%d] init error, rc=%d\n", i, rc);
			goto child_init_err;
		}
		oplus_chg_ic_set_parent(vr->child_list[i].ic_dev, ic_dev);
	}

	ic_dev->online = true;

	return 0;

child_init_err:
	for (m = i + 1; m > 0; m--)
		oplus_chg_ic_func(vr->child_list[m - 1].ic_dev, OPLUS_IC_FUNC_EXIT);
virq_register_err:
child_list_init_err:

	return rc;
}

static int oplus_chg_vr_exit(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}
	if (!ic_dev->online)
		return 0;

	ic_dev->online = false;

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev, OPLUS_IC_FUNC_EXIT);
		if (rc < 0)
			chg_err("child ic[%d] exit error, rc=%d\n", i, rc);
	}
	for (i = 0; i < vr->child_num; i++) {
		if (virq_is_support(&vr->child_list[i], OPLUS_IC_VIRQ_ERR)) {
			oplus_chg_ic_virq_release(vr->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_ERR, vr);
		}
		if (virq_is_support(&vr->child_list[i], OPLUS_IC_VIRQ_PRESENT)) {
			oplus_chg_ic_virq_release(vr->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_PRESENT, vr);
		}
		if (virq_is_support(&vr->child_list[i], OPLUS_IC_VIRQ_ONLINE)) {
			oplus_chg_ic_virq_release(vr->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_ONLINE, vr);
		}
		if (virq_is_support(&vr->child_list[i], OPLUS_IC_VIRQ_OFFLINE)) {
			oplus_chg_ic_virq_release(vr->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_OFFLINE, vr);
		}
		if (virq_is_support(&vr->child_list[i], OPLUS_IC_VIRQ_EVENT_CHANGED)) {
			oplus_chg_ic_virq_release(vr->child_list[i].ic_dev,
				OPLUS_IC_VIRQ_EVENT_CHANGED, vr);
		}
	}
	for (i = 0; i < vr->child_num; i++) {
		if (vr->child_list[i].virqs != NULL)
			devm_kfree(vr->dev, vr->child_list[i].virqs);
	}
	for (i = 0; i < vr->child_num; i++)
		devm_kfree(vr->dev, vr->child_list[i].funcs);

	return 0;
}

static int oplus_chg_vr_reg_dump(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i], OPLUS_IC_FUNC_REG_DUMP)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev, OPLUS_IC_FUNC_REG_DUMP);
		if (rc < 0)
			chg_err("child ic[%d] reg_dump error, rc=%d\n", i, rc);
	}

	return rc;
}

static int oplus_chg_vr_smt_test(struct oplus_chg_ic_dev *ic_dev, char buf[], int len)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int index = 0;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (index >= len)
			return len;
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_SMT_TEST, buf + index, len - index);
		if (rc < 0) {
			if (rc != -ENOTSUPP) {
				chg_err("child ic[%d] smt test error, rc=%d\n", i, rc);
				rc = snprintf(buf + index, len - index,
					"[%s]-[%s]:%d\n",
					vr->child_list[i].ic_dev->manu_name,
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

static int oplus_chg_vr_set_rx_enable(struct oplus_chg_ic_dev *ic_dev, bool en)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_ENABLE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_ENABLE, en);
		if (rc < 0)
			chg_err("child ic[%d] set_rx_enable error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_rx_is_enable(struct oplus_chg_ic_dev *ic_dev, bool *enable)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_IS_ENABLE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_IS_ENABLE, enable);
		if (rc < 0)
			chg_err("child ic[%d] rx_is_enable error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_rx_is_connected(struct oplus_chg_ic_dev *ic_dev, bool *connected)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_IS_CONNECTED)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}

		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_IS_CONNECTED, connected);
		if (rc < 0)
			chg_err("child ic[%d] rx_is_connected error, rc=%d\n", i, rc);
		break;
	}
	return rc;
}

static int oplus_chg_vr_get_vout(struct oplus_chg_ic_dev *ic_dev, int *vout)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_VOUT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_VOUT, vout);
		if (rc < 0)
			chg_err("child ic[%d] get_vout error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_vout(struct oplus_chg_ic_dev *ic_dev, int vout)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_VOUT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_VOUT, vout);
		if (rc < 0)
			chg_err("child ic[%d] set_vout error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_vrect(struct oplus_chg_ic_dev *ic_dev, int *vrect)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_VRECT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_VRECT, vrect);
		if (rc < 0)
			chg_err("child ic[%d] get_vrect error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_iout(struct oplus_chg_ic_dev *ic_dev, int *iout)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_IOUT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_IOUT, iout);
		if (rc < 0)
			chg_err("child ic[%d] get_iout error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_tx_vout(struct oplus_chg_ic_dev *ic_dev, int *vout)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_TRX_VOL)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_TRX_VOL, vout);
		if (rc < 0)
			chg_err("child ic[%d] get_tx_vout error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_tx_iout(struct oplus_chg_ic_dev *ic_dev, int *iout)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_TRX_CURR)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_TRX_CURR, iout);
		if (rc < 0)
			chg_err("child ic[%d] get_tx_iout error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_cep_count(struct oplus_chg_ic_dev *ic_dev, int *count)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_CEP_COUNT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_CEP_COUNT, count);
		if (rc < 0)
			chg_err("child ic[%d] get_cep_count error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_cep_val(struct oplus_chg_ic_dev *ic_dev, int *val)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_CEP_VAL)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_CEP_VAL, val);
		if (rc < 0)
			chg_err("child ic[%d] get_cep_val error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_work_freq(struct oplus_chg_ic_dev *ic_dev, int *freq)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_WORK_FREQ)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_WORK_FREQ, freq);
		if (rc < 0)
			chg_err("child ic[%d] get_work_freq error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_rx_mode(struct oplus_chg_ic_dev *ic_dev,
				enum oplus_chg_wls_rx_mode *mode)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_RX_MODE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_RX_MODE, mode);
		if (rc < 0)
			chg_err("child ic[%d] get_rx_mode error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_rx_mode(struct oplus_chg_ic_dev *ic_dev,
				enum oplus_chg_wls_rx_mode mode)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_RX_MODE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_RX_MODE, mode);
		if (rc < 0)
			chg_err("child ic[%d] set_rx_mode error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_dcdc_enable(struct oplus_chg_ic_dev *ic_dev, bool enable)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE, enable);
		if (rc < 0)
			chg_err("child ic[%d] set_dcdc_enable error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_tx_enable(struct oplus_chg_ic_dev *ic_dev, bool enable)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_TRX_ENABLE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_TRX_ENABLE, enable);
		if (rc < 0)
			chg_err("child ic[%d] set_tx_enable error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_tx_start(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_TRX_START)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_TRX_START);
		if (rc < 0)
			chg_err("child ic[%d] set_tx_start error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_tx_status(struct oplus_chg_ic_dev *ic_dev, u8 *status)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_TRX_STATUS)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_TRX_STATUS, status);
		if (rc < 0)
			chg_err("child ic[%d] get_tx_status error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_tx_err(struct oplus_chg_ic_dev *ic_dev, u8 *err)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_TRX_ERR)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_TRX_ERR, err);
		if (rc < 0)
			chg_err("child ic[%d] get_tx_err error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_headroom(struct oplus_chg_ic_dev *ic_dev, int *val)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_HEADROOM)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_HEADROOM, val);
		if (rc < 0)
			chg_err("child ic[%d] get_headroom error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_headroom(struct oplus_chg_ic_dev *ic_dev, int val)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_HEADROOM)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_HEADROOM, val);
		if (rc < 0)
			chg_err("child ic[%d] set_headroom error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_send_match_q(struct oplus_chg_ic_dev *ic_dev, u8 data)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SEND_MATCH_Q)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SEND_MATCH_Q, data);
		if (rc < 0)
			chg_err("child ic[%d] send_match_q error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_set_fod_parm(struct oplus_chg_ic_dev *ic_dev, u8 data[], int len)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SET_FOD_PARM)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SET_FOD_PARM, data, len);
		if (rc < 0)
			chg_err("child ic[%d] set_fod_parm error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_send_msg(struct oplus_chg_ic_dev *ic_dev, unsigned char msg[], int len)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_SEND_MSG)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_SEND_MSG, msg, len);
		if (rc < 0)
			chg_err("child ic[%d] send_msg error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_register_msg_callback(struct oplus_chg_ic_dev *ic_dev,
					      void *data, void (*call_back)(void *, u8[]))
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK, data, call_back);
		if (rc < 0)
			chg_err("child ic[%d] register_msg_callback error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_rx_version(struct oplus_chg_ic_dev *ic_dev, u32 *version)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_RX_VERSION)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_RX_VERSION, version);
		if (rc < 0)
			chg_err("child ic[%d] get_rx_version error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_tx_version(struct oplus_chg_ic_dev *ic_dev, u32 *version)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_TRX_VERSION)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_TRX_VERSION, version);
		if (rc < 0)
			chg_err("child ic[%d] tx_version error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_upgrade_firmware_by_buf(struct oplus_chg_ic_dev *ic_dev,
				unsigned char *fw_buf, int fw_size)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF, fw_buf, fw_size);
		if (rc < 0)
			chg_err("child ic[%d] upgrade_firmware_by_buf error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_upgrade_firmware_by_img(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev, OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG);
		if (rc < 0)
			chg_err("child ic[%d] upgrade_firmware_by_img error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_connect_check(struct oplus_chg_ic_dev *ic_dev)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_CHECK_CONNECT)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_CHECK_CONNECT);
		if (rc < 0)
			chg_err("child ic[%d] connect_check error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static int oplus_chg_vr_get_event_code(struct oplus_chg_ic_dev *ic_dev,  enum oplus_chg_wls_event_code *code)
{
	struct oplus_virtual_rx_ic *vr;
	int i;
	int rc = 0;

	if (ic_dev == NULL) {
		chg_err("oplus_chg_ic_dev is NULL\n");
		return -ENODEV;
	}

	vr = oplus_chg_ic_get_drvdata(ic_dev);
	for (i = 0; i < vr->child_num; i++) {
		if (!func_is_support(&vr->child_list[i],
				     OPLUS_IC_FUNC_RX_GET_EVENT_CODE)) {
			rc = (rc == 0) ? -ENOTSUPP : rc;
			continue;
		}
		rc = oplus_chg_ic_func(vr->child_list[i].ic_dev,
			OPLUS_IC_FUNC_RX_GET_EVENT_CODE, code);
		if (rc < 0)
			chg_err("child ic[%d] get_event_code error, rc=%d\n", i, rc);
		break;
	}

	return rc;
}

static void *oplus_chg_vr_get_func(struct oplus_chg_ic_dev *ic_dev,
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
			    oplus_chg_vr_init);
		break;
	case OPLUS_IC_FUNC_EXIT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_EXIT,
			    oplus_chg_vr_exit);
		break;
	case OPLUS_IC_FUNC_REG_DUMP:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_REG_DUMP,
			    oplus_chg_vr_reg_dump);
		break;
	case OPLUS_IC_FUNC_SMT_TEST:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_SMT_TEST,
			    oplus_chg_vr_smt_test);
		break;
	case OPLUS_IC_FUNC_RX_SET_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_ENABLE,
			    oplus_chg_vr_set_rx_enable);
		break;
	case OPLUS_IC_FUNC_RX_IS_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_IS_ENABLE,
			    oplus_chg_vr_rx_is_enable);
		break;
	case OPLUS_IC_FUNC_RX_IS_CONNECTED:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_IS_CONNECTED,
			    oplus_chg_vr_rx_is_connected);
		break;
	case OPLUS_IC_FUNC_RX_GET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_VOUT,
			    oplus_chg_vr_get_vout);
		break;
	case OPLUS_IC_FUNC_RX_SET_VOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_VOUT,
			    oplus_chg_vr_set_vout);
		break;
	case OPLUS_IC_FUNC_RX_GET_VRECT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_VRECT,
			    oplus_chg_vr_get_vrect);
		break;
	case OPLUS_IC_FUNC_RX_GET_IOUT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_IOUT,
			    oplus_chg_vr_get_iout);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_VOL:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_VOL,
			    oplus_chg_vr_get_tx_vout);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_CURR:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_CURR,
			    oplus_chg_vr_get_tx_iout);
		break;
	case OPLUS_IC_FUNC_RX_GET_CEP_COUNT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_CEP_COUNT,
			    oplus_chg_vr_get_cep_count);
		break;
	case OPLUS_IC_FUNC_RX_GET_CEP_VAL:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_CEP_VAL,
			    oplus_chg_vr_get_cep_val);
		break;
	case OPLUS_IC_FUNC_RX_GET_WORK_FREQ:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_WORK_FREQ,
			    oplus_chg_vr_get_work_freq);
		break;
	case OPLUS_IC_FUNC_RX_GET_RX_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_RX_MODE,
			    oplus_chg_vr_get_rx_mode);
		break;
	case OPLUS_IC_FUNC_RX_SET_RX_MODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_RX_MODE,
			    oplus_chg_vr_set_rx_mode);
		break;
	case OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE,
			    oplus_chg_vr_set_dcdc_enable);
		break;
	case OPLUS_IC_FUNC_RX_SET_TRX_ENABLE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_TRX_ENABLE,
			    oplus_chg_vr_set_tx_enable);
		break;
	case OPLUS_IC_FUNC_RX_SET_TRX_START:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_TRX_START,
			    oplus_chg_vr_set_tx_start);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_STATUS:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_STATUS,
			    oplus_chg_vr_get_tx_status);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_ERR:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_ERR,
			    oplus_chg_vr_get_tx_err);
		break;
	case OPLUS_IC_FUNC_RX_GET_HEADROOM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_HEADROOM,
			    oplus_chg_vr_get_headroom);
		break;
	case OPLUS_IC_FUNC_RX_SET_HEADROOM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_HEADROOM,
			    oplus_chg_vr_set_headroom);
		break;
	case OPLUS_IC_FUNC_RX_SEND_MATCH_Q:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SEND_MATCH_Q,
			    oplus_chg_vr_send_match_q);
		break;
	case OPLUS_IC_FUNC_RX_SET_FOD_PARM:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SET_FOD_PARM,
			    oplus_chg_vr_set_fod_parm);
		break;
	case OPLUS_IC_FUNC_RX_SEND_MSG:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_SEND_MSG,
			    oplus_chg_vr_send_msg);
		break;
	case OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK,
			    oplus_chg_vr_register_msg_callback);
		break;
	case OPLUS_IC_FUNC_RX_GET_RX_VERSION:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_RX_VERSION,
			    oplus_chg_vr_get_rx_version);
		break;
	case OPLUS_IC_FUNC_RX_GET_TRX_VERSION:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_TRX_VERSION,
			    oplus_chg_vr_get_tx_version);
		break;
	case OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF,
			    oplus_chg_vr_upgrade_firmware_by_buf);
		break;
	case OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG,
			    oplus_chg_vr_upgrade_firmware_by_img);
		break;
	case OPLUS_IC_FUNC_RX_CHECK_CONNECT:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_CHECK_CONNECT,
			    oplus_chg_vr_connect_check);
		break;
	case OPLUS_IC_FUNC_RX_GET_EVENT_CODE:
		func = OPLUS_CHG_IC_FUNC_CHECK(OPLUS_IC_FUNC_RX_GET_EVENT_CODE,
			    oplus_chg_vr_get_event_code);
		break;
	default:
		chg_err("this func(=%d) is not supported\n", func_id);
		func = NULL;
		break;
	}

	return func;
}

static struct oplus_chg_ic_virq oplus_chg_vr_virq_table[] = {
	{ .virq_id = OPLUS_IC_VIRQ_ERR },
	{ .virq_id = OPLUS_IC_VIRQ_PRESENT },
	{ .virq_id = OPLUS_IC_VIRQ_ONLINE },
	{ .virq_id = OPLUS_IC_VIRQ_OFFLINE },
	{ .virq_id = OPLUS_IC_VIRQ_EVENT_CHANGED },
};

static void oplus_chg_vr_err_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_rx_ic *chip = virq_data;

	oplus_chg_ic_move_err_msg(chip->ic_dev, ic_dev);
	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ERR);
}

static void oplus_chg_vr_present_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_rx_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_PRESENT);
}

static void oplus_chg_vr_online_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_rx_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_ONLINE);
}

static void oplus_chg_vr_offline_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_rx_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_OFFLINE);
}

static void oplus_chg_vr_event_changed_handler(struct oplus_chg_ic_dev *ic_dev, void *virq_data)
{
	struct oplus_virtual_rx_ic *chip = virq_data;

	oplus_chg_ic_virq_trigger(chip->ic_dev, OPLUS_IC_VIRQ_EVENT_CHANGED);
}

static int oplus_chg_vr_virq_register(struct oplus_virtual_rx_ic *chip)
{
	int i;
	int rc;

	for (i = 0; i < chip->child_num; i++) {
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_ERR)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_ERR,
				oplus_chg_vr_err_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_ERR error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_PRESENT)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_PRESENT,
				oplus_chg_vr_present_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_PRESENT error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_ONLINE)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_ONLINE,
				oplus_chg_vr_online_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_ONLINE error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_OFFLINE)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_OFFLINE,
				oplus_chg_vr_offline_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_OFFLINE error, rc=%d", rc);
		}
		if (virq_is_support(&chip->child_list[i], OPLUS_IC_VIRQ_EVENT_CHANGED)) {
			rc = oplus_chg_ic_virq_register(
				chip->child_list[i].ic_dev, OPLUS_IC_VIRQ_EVENT_CHANGED,
				oplus_chg_vr_event_changed_handler, chip);
			if (rc < 0)
				chg_err("register OPLUS_IC_VIRQ_EVENT_CHANGED error, rc=%d", rc);
		}
	}

	return 0;
}

static int oplus_virtual_rx_probe(struct platform_device *pdev)
{
	struct oplus_virtual_rx_ic *chip;
	struct device_node *node = pdev->dev.of_node;
	struct oplus_chg_ic_cfg ic_cfg = { 0 };
	enum oplus_chg_ic_type ic_type;
	int ic_index;
	int rc = 0;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_virtual_rx_ic), GFP_KERNEL);
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
	snprintf(ic_cfg.manu_name, OPLUS_CHG_IC_MANU_NAME_MAX - 1, "rx-virtual");
	snprintf(ic_cfg.fw_id, OPLUS_CHG_IC_FW_ID_MAX - 1, "0x00");
	ic_cfg.type = ic_type;
	ic_cfg.get_func = oplus_chg_vr_get_func;
	ic_cfg.virq_data = oplus_chg_vr_virq_table;
	ic_cfg.virq_num = ARRAY_SIZE(oplus_chg_vr_virq_table);
	ic_cfg.of_node = node;
	chip->ic_dev = devm_oplus_chg_ic_register(chip->dev, &ic_cfg);
	if (!chip->ic_dev) {
		rc = -ENODEV;
		chg_err("register %s error\n", node->name);
		goto reg_ic_err;
	}

	chg_info("probe success\n");
	return 0;

reg_ic_err:
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	chg_err("probe error\n");
	return rc;
}

static int oplus_virtual_rx_remove(struct platform_device *pdev)
{
	struct oplus_virtual_rx_ic *chip = platform_get_drvdata(pdev);

	if (chip == NULL)
		return -ENODEV;

	if (chip->ic_dev->online)
		oplus_chg_vr_exit(chip->ic_dev);
	devm_oplus_chg_ic_unregister(&pdev->dev, chip->ic_dev);
	devm_kfree(&pdev->dev, chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id oplus_virtual_rx_match[] = {
	{ .compatible = "oplus,virtual_rx" },
	{},
};

static struct platform_driver oplus_virtual_rx_driver = {
	.driver		= {
		.name = "oplus-virtual_rx",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_virtual_rx_match),
	},
	.probe		= oplus_virtual_rx_probe,
	.remove		= oplus_virtual_rx_remove,
};

static __init int oplus_virtual_rx_init(void)
{
	return platform_driver_register(&oplus_virtual_rx_driver);
}

static __exit void oplus_virtual_rx_exit(void)
{
	platform_driver_unregister(&oplus_virtual_rx_driver);
}

oplus_chg_module_register(oplus_virtual_rx);
