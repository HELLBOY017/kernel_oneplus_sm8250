// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[IMP]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <oplus_chg_module.h>
#include <oplus_chg.h>
#include <oplus_impedance_check.h>

static LIST_HEAD(g_node_list);
static DEFINE_SPINLOCK(g_node_list_lock);
static LIST_HEAD(g_uint_list);
static DEFINE_SPINLOCK(g_uint_list_lock);

static int oplus_imp_node_get_allow_curr(struct oplus_impedance_node *node)
{
	int i, m;
	int r_mohm;
	int curr;

	r_mohm = node->get_impedance(node->priv_data);
	if (r_mohm < 0) {
		chg_err("can't get %s node impedance, rc=%d\n", node->of_node->name, r_mohm);
		return r_mohm;
	}

	for (i = 0; i < IMPEDANCE_AVG_NUM - 1; i++)
		node->imp_save[i] = node->imp_save[i + 1];
	node->imp_save[i] = r_mohm;
	r_mohm = 0;

	for (m = 0; m <= i; m++)
		r_mohm += node->imp_save[m];
	r_mohm = r_mohm / (i + 1);

	for (i = 0; i < node->curr_drop_table_num; i++) {
		if (r_mohm < node->def_impedance_mohm + node->curr_drop_table[i].impedance_thr_mohm)
			break;
	}
	if (i == 0)
		curr = node->def_curr_ma;
	else
		curr = node->curr_drop_table[i - 1].curr_ma;
	if (curr == 0)
		chg_err("%s: r_mohm=%d, curr=0", node->of_node->name, r_mohm);

	return curr;
}

int oplus_imp_uint_get_allow_curr(struct oplus_impedance_unit *imp_uint)
{
	struct oplus_impedance_node *node;
	int i;
	int curr;
	int tmp;

	if (imp_uint == NULL) {
		chg_err("imp_uint is NULL\n");
		return -EINVAL;
	}
	curr = imp_uint->def_curr_ma;

	for (i = 0; i < imp_uint->node_num; i++) {
		node = imp_uint->node_list[i];
		if (node == NULL || !node->active)
			continue;
		tmp = oplus_imp_node_get_allow_curr(node);
		if (tmp < 0)
			continue;
		if (tmp < curr)
			curr = tmp;
	}

	return curr;
}

static struct oplus_impedance_unit *oplus_imp_uint_find_by_name(const char *name)
{
	struct oplus_impedance_unit *unit;

	spin_lock(&g_uint_list_lock);
	list_for_each_entry(unit, &g_uint_list, list) {
		if (unit->of_node == NULL)
			continue;
		if (strcmp(unit->of_node->name, name) == 0) {
			spin_unlock(&g_uint_list_lock);
			return unit;
		}
	}
	spin_unlock(&g_uint_list_lock);

	return NULL;
}

struct oplus_impedance_unit *of_find_imp_uint(struct device_node *of_node, const char *prop_name)
{
	struct device_node *unit_node;

	if (of_node == NULL) {
		chg_err("of_node is NULL\n");
		return NULL;
	}
	if (prop_name == NULL) {
		chg_err("of_node is NULL\n");
		return NULL;
	}

	unit_node = of_parse_phandle(of_node, prop_name, 0);
	if (!unit_node)
		return NULL;

	chg_debug("search %s\n", unit_node->name);
	return oplus_imp_uint_find_by_name(unit_node->name);
}

static struct oplus_impedance_node *oplus_imp_node_find_by_name(const char *name)
{
	struct oplus_impedance_node *node;

	spin_lock(&g_node_list_lock);
	list_for_each_entry(node, &g_node_list, list) {
		if (node->of_node == NULL)
			continue;
		if (strcmp(node->of_node->name, name) == 0) {
			spin_unlock(&g_node_list_lock);
			return node;
		}
	}
	spin_unlock(&g_node_list_lock);

	return NULL;
}

static struct oplus_impedance_node *of_find_imp_node(struct device_node *of_node, const char *prop_name, int index)
{
	struct device_node *imp_node;

	imp_node = of_parse_phandle(of_node, prop_name, index);
	if (!imp_node)
		return NULL;

	chg_debug("search %s\n", imp_node->name);
	return oplus_imp_node_find_by_name(imp_node->name);
}

static int oplus_imp_node_reset(struct oplus_impedance_node *node)
{
	int i;

	for (i = 0; i < IMPEDANCE_AVG_NUM; i++)
		node->imp_save[i] = node->def_impedance_mohm;

	return 0;
}

int oplus_imp_uint_reset(struct oplus_impedance_unit *imp_uint)
{
	int i;

	if (imp_uint == NULL) {
		chg_err("imp_uint is NULL\n");
		return -EINVAL;
	}

	for (i = 0; i < imp_uint->node_num; i++) {
		if (imp_uint->node_list[i] != NULL)
			(void)oplus_imp_node_reset(imp_uint->node_list[i]);
	}

	return 0;
}

static int __oplus_imp_node_register(struct oplus_impedance_node *node)
{
	struct device_node *of_node = node->of_node;
	int rc;

	rc = of_property_read_string(of_node, "node_name", &node->name);
	if (rc < 0) {
		chg_err("can't read node_name, rc=%d\n", rc);
		return rc;
	}
	rc = of_property_read_u32(of_node, "default_impedance_mohm", &node->def_impedance_mohm);
	if (rc < 0) {
		chg_err("can't read default_impedance_mohm, rc=%d\n", rc);
		return rc;
	}
	rc = of_property_read_u32(of_node, "default_curr_ma", &node->def_curr_ma);
	if (rc < 0) {
		chg_err("can't read default_curr_ma, rc=%d\n", rc);
		return rc;
	}

	rc = of_property_read_u32_array(of_node, "current_drop_table", (u32 *)node->curr_drop_table,
					node->curr_drop_table_num *
						(sizeof(struct oplus_impedance_curr_drop_table) / sizeof(s32)));
	if (rc < 0) {
		chg_err("can't read current_drop_table, rc=%d\n", rc);
		return rc;
	}

	spin_lock(&g_node_list_lock);
	list_add(&node->list, &g_node_list);
	spin_unlock(&g_node_list_lock);

	return 0;
}

struct oplus_impedance_node *oplus_imp_node_register(
	struct device_node *of_node, struct device *dev, void *priv_data, int (*get_imp_func)(void *))
{
	struct oplus_impedance_node *node;
	int rc;

	if (of_node == NULL) {
		chg_err("of_node is NULL\n");
		return ERR_PTR(-EINVAL);
	}
	if (dev == NULL) {
		chg_err("dev is NULL\n");
		return ERR_PTR(-EINVAL);
	}
	if (priv_data == NULL) {
		chg_err("priv_data is NULL\n");
		return ERR_PTR(-EINVAL);
	}
	if (get_imp_func == NULL) {
		chg_err("get_imp_func is NULL\n");
		return ERR_PTR(-EINVAL);
	}

	rc = of_property_count_elems_of_size(of_node, "current_drop_table", sizeof(s32));
	if (rc < 0) {
		chg_err("can't read current_drop_table, rc=%d\n", rc);
		return ERR_PTR(rc);
	}
	if (rc % (sizeof(struct oplus_impedance_curr_drop_table) / sizeof(s32))) {
		chg_err("buf size does not meet the requirements, size=%d\n", rc);
		return ERR_PTR(-EINVAL);
	}
	node = devm_kzalloc(dev,
			    sizeof(struct oplus_impedance_node) + rc * sizeof(s32),
			    GFP_KERNEL);
	if (node == NULL) {
		chg_err("alloc node buf error\n");
		return ERR_PTR(-ENOMEM);
	}

	node->of_node = of_node;
	node->priv_data = priv_data;
	node->get_impedance = get_imp_func;
	node->curr_drop_table_num = rc / (sizeof(struct oplus_impedance_curr_drop_table) / sizeof(s32));
	chg_info("%s: curr_drop_table_num=%d\n", node->of_node->name, node->curr_drop_table_num);

	rc = __oplus_imp_node_register(node);
	if (rc < 0)
		goto err;
	node->active = false;
	chg_info("%s(%s) node register\n", node->name, node->of_node->name);

	return node;

err:
	devm_kfree(dev, node);
	return ERR_PTR(rc);
}

void oplus_imp_node_unregister(struct device *dev, struct oplus_impedance_node *node)
{
	struct oplus_impedance_unit *imp_uint;
	int i;

	if (dev == NULL) {
		chg_err("dev is NULL\n");
		return;
	}
	if (node == NULL) {
		chg_err("node is NULL\n");
		return;
	}

	spin_lock(&g_node_list_lock);
	list_del(&node->list);
	spin_unlock(&g_node_list_lock);

	spin_lock(&g_uint_list_lock);
	list_for_each_entry(imp_uint, &g_uint_list, list) {
		for (i = 0; i < imp_uint->node_num; i++) {
			if (imp_uint->node_list[i] == node) {
				imp_uint->node_list[i] = NULL;
				break;
			}
		}
	}
	spin_unlock(&g_uint_list_lock);

	devm_kfree(dev, node);
}

static void oplus_imp_uint_unregister(struct device *dev)
{
	struct oplus_impedance_unit *imp_uint, *tmp;
	LIST_HEAD(tmp_list);

	spin_lock(&g_uint_list_lock);
	list_for_each_entry_safe(imp_uint, tmp, &g_uint_list, list) {
		list_del(&imp_uint->list);
		list_add(&imp_uint->list, &tmp_list);
	}
	spin_unlock(&g_uint_list_lock);

	list_for_each_entry_safe(imp_uint, tmp, &tmp_list, list) {
		list_del(&imp_uint->list);
		devm_kfree(dev, imp_uint);
	}
}

static int __oplus_imp_uint_register(struct device_node *of_node, struct device *dev)
{
	struct oplus_impedance_unit *imp_uint;
	struct oplus_impedance_node *node;
	int rc;
	int node_num;
	int i;

	node_num = of_count_phandle_with_args(of_node, "impedance_node", NULL);
	if (node_num < 0) {
		chg_err("can't read impedance_node, rc=%d\n", node_num);
		return node_num;
	}
	if (node_num == 0)
		chg_err("impedance_node number is 0\n");

	imp_uint = devm_kzalloc(dev, sizeof(struct oplus_impedance_unit) + node_num * sizeof(struct oplus_impedance_node *),
			    GFP_KERNEL);
	if (imp_uint == NULL) {
		chg_err("alloc imp_uint buf error\n");
		return -ENOMEM;
	}
	imp_uint->of_node = of_node;
	imp_uint->node_num = node_num;

	rc = of_property_read_string(of_node, "uint_name", &imp_uint->name);
	if (rc < 0) {
		chg_err("can't read uint_name, rc=%d\n", rc);
		goto property_err;
	}
	rc = of_property_read_u32(of_node, "default_curr_ma", &imp_uint->def_curr_ma);
	if (rc < 0) {
		chg_err("can't read default_curr_ma, rc=%d\n", rc);
		goto property_err;
	}

	for (i = 0; i < node_num; i++) {
		node = of_find_imp_node(of_node, "impedance_node", i);
		if (node == NULL) {
			chg_err("node[%d] not found, retry\n", i);
			rc = -EPROBE_DEFER;
			goto node_not_found;
		}
		imp_uint->node_list[i] = node;
	}
	rc = oplus_imp_uint_reset(imp_uint);
	if (rc < 0) {
		chg_err("imp uint reset error, rc=%d\n", rc);
		goto uint_reset_err;
	}

	spin_lock(&g_uint_list_lock);
	list_add(&imp_uint->list, &g_uint_list);
	spin_unlock(&g_uint_list_lock);

	return 0;

uint_reset_err:
node_not_found:
property_err:
	devm_kfree(dev, imp_uint);
	return rc;
}

static int oplus_imp_uint_register(struct device *dev)
{
	struct device_node *node;
	struct device_node *child;
	int rc;

	node = of_get_child_by_name(dev->of_node, "oplus,impedance_unit");
	if (node == NULL) {
		chg_err("oplus,impedance_unit node not found\n");
		return -ENODEV;
	}

	for_each_child_of_node(node, child) {
		rc = __oplus_imp_uint_register(child, dev);
		if (rc < 0) {
			chg_err("%s register error, rc=%d\n", child->name, rc);
			goto err;
		}
	}

	return 0;

err:
	oplus_imp_uint_unregister(dev);
	return rc;
}

static int oplus_imp_check_probe(struct platform_device *pdev)
{
	int rc;

	rc = oplus_imp_uint_register(&pdev->dev);
	if (rc < 0)
		return rc;

	return 0;
}

static int oplus_imp_check_remove(struct platform_device *pdev)
{
	oplus_imp_uint_unregister(&pdev->dev);
	return 0;
}

static const struct of_device_id oplus_imp_check_match[] = {
	{ .compatible = "oplus,impedance_check" },
	{},
};

static struct platform_driver oplus_imp_check_driver = {
	.driver		= {
		.name = "oplus-impedance_check",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(oplus_imp_check_match),
	},
	.probe		= oplus_imp_check_probe,
	.remove		= oplus_imp_check_remove,
};

static __init int oplus_imp_check_init(void)
{
#if __and(IS_BUILTIN(CONFIG_OPLUS_CHG), IS_BUILTIN(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return 0;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return 0;
#endif /* CONFIG_OPLUS_CHG_V2 */
	return platform_driver_register(&oplus_imp_check_driver);
}

static __exit void oplus_imp_check_exit(void)
{
#if (IS_ENABLED(CONFIG_OPLUS_CHG) && IS_ENABLED(CONFIG_OPLUS_CHG_V2))
	struct device_node *node;

	node = of_find_node_by_path("/soc/oplus_chg_core");
	if (node == NULL)
		return;
	if (!of_property_read_bool(node, "oplus,chg_framework_v2"))
		return;
#endif /* CONFIG_OPLUS_CHG_V2 */
	platform_driver_unregister(&oplus_imp_check_driver);
}

oplus_chg_module_register(oplus_imp_check);
