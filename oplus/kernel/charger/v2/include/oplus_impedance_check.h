// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#ifndef __OPLUS_IMPEDANCE_CHECK_H__
#define __OPLUS_IMPEDANCE_CHECK_H__

#include <linux/list.h>
#include <linux/of.h>
#include <linux/types.h>

#define IMPEDANCE_AVG_NUM	10

struct oplus_impedance_curr_drop_table {
	int32_t impedance_thr_mohm;
	int32_t curr_ma;
} __attribute__((packed));

struct oplus_impedance_node {
	const char *name;
	struct device_node *of_node;
	void *priv_data;
	int def_impedance_mohm;
	int def_curr_ma;
	bool active;
	int (*get_impedance)(void *data);
	struct list_head list;
	int imp_save[IMPEDANCE_AVG_NUM];

	int curr_drop_table_num;
	struct oplus_impedance_curr_drop_table curr_drop_table[0];
};

struct oplus_impedance_unit {
	const char *name;
	struct device_node *of_node;
	void *priv_data;
	int def_curr_ma;
	struct list_head list;

	int node_num;
	struct oplus_impedance_node *node_list[0];
};

static inline void *oplus_imp_unit_get_priv_data(struct oplus_impedance_unit *unit)
{
	return unit->priv_data;
}

static inline void *oplus_imp_node_get_priv_data(struct oplus_impedance_node *node)
{
	return node->priv_data;
}

static inline void oplus_imp_node_set_active(struct oplus_impedance_node *node, bool active)
{
	if (node)
		node->active = active;
}

int oplus_imp_uint_reset(struct oplus_impedance_unit *imp_uint);
struct oplus_impedance_unit *of_find_imp_uint(struct device_node *of_node, const char *prop_name);
struct oplus_impedance_node *oplus_imp_node_register(
	struct device_node *of_node, struct device *dev, void *priv_data, int (*get_imp_func)(void *));
void oplus_imp_node_unregister(struct device *dev, struct oplus_impedance_node *node);
int oplus_imp_uint_get_allow_curr(struct oplus_impedance_unit *uint);

#endif /* __OPLUS_IMPEDANCE_CHECK_H__ */
