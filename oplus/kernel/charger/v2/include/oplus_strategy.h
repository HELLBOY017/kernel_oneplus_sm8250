// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2021 Oplus. All rights reserved.
 */

#ifndef __OPLUS_STRATEGY_H__
#define __OPLUS_STRATEGY_H__

#include <linux/list.h>
#include <linux/device.h>

struct oplus_chg_strategy;

enum {
	STRATEGY_USE_BATT_TEMP = 0,
	STRATEGY_USE_SHELL_TEMP,
};

struct oplus_chg_strategy_desc {
	const char *name;
	struct list_head list;

	struct oplus_chg_strategy *(*strategy_alloc)(unsigned char *buf,
						     size_t size);
	struct oplus_chg_strategy *(*strategy_alloc_by_node)(struct device_node *node);
	int (*strategy_release)(struct oplus_chg_strategy *strategy);
	int (*strategy_init)(struct oplus_chg_strategy *strategy);
	int (*strategy_get_data)(struct oplus_chg_strategy *strategy, void *ret);
};

struct oplus_chg_strategy {
	struct oplus_chg_strategy_desc *desc;
	bool initialized;
};

struct oplus_chg_strategy *
oplus_chg_strategy_alloc(const char *name, unsigned char *buf, size_t size);
struct oplus_chg_strategy *
oplus_chg_strategy_alloc_by_node(const char *name, struct device_node *node);
int oplus_chg_strategy_init(struct oplus_chg_strategy *strategy);
int oplus_chg_strategy_release(struct oplus_chg_strategy *strategy);
int oplus_chg_strategy_get_data(struct oplus_chg_strategy *strategy, void *ret);
int oplus_chg_strategy_register(struct oplus_chg_strategy_desc *desc);
int oplus_chg_strategy_read_data(struct device *dev,
				 const char *prop_str, uint8_t **buf);

/* pps ufcs curve strategy*/

struct puc_strategy_data {
	int32_t target_vbus;
	int32_t target_vbat;
	int32_t target_ibus;
	int32_t exit;
	int32_t target_time;
} __attribute__((packed));

struct puc_strategy_temp_curves {
	struct puc_strategy_data *data;
	int num;
};

struct puc_strategy_ret_data {
	int32_t target_vbus;
	int32_t target_vbat;
	int32_t target_ibus;
	int index;
	bool last_gear;
	bool exit;
};

struct puc_strategy_temp_curves *puc_strategy_get_curr_curve_data(struct oplus_chg_strategy *strategy);

#endif /* __OPLUS_STRATEGY_H__ */
