// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2021 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[STRATEGY_PPS_UFCS]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <oplus_chg.h>
#include <oplus_mms.h>
#include <oplus_mms_gauge.h>
#include <oplus_chg_comm.h>
#include <oplus_strategy.h>

enum puc_soc_range {
	PUC_BATT_CURVE_SOC_RANGE_MIN = 0,
	PUC_BATT_CURVE_SOC_RANGE_LOW,
	PUC_BATT_CURVE_SOC_RANGE_MID_LOW,
	PUC_BATT_CURVE_SOC_RANGE_MID,
	PUC_BATT_CURVE_SOC_RANGE_MID_HIGH,
	PUC_BATT_CURVE_SOC_RANGE_HIGH,
	PUC_BATT_CURVE_SOC_RANGE_MAX,
	PUC_BATT_CURVE_SOC_RANGE_INVALID = PUC_BATT_CURVE_SOC_RANGE_MAX,
};

enum puc_temp_range {
	PUC_BATT_CURVE_TEMP_RANGE_LITTLE_COLD = 0,
	PUC_BATT_CURVE_TEMP_RANGE_COOL,
	PUC_BATT_CURVE_TEMP_RANGE_LITTLE_COOL,
	PUC_BATT_CURVE_TEMP_RANGE_NORMAL_LOW,
	PUC_BATT_CURVE_TEMP_RANGE_NORMAL_HIGH,
	PUC_BATT_CURVE_TEMP_RANGE_WARM,
	PUC_BATT_CURVE_TEMP_RANGE_MAX,
	PUC_BATT_CURVE_TEMP_RANGE_INVALID = PUC_BATT_CURVE_TEMP_RANGE_MAX,
};

struct puc_strategy_soc_curves {
	struct puc_strategy_temp_curves temp_curves[PUC_BATT_CURVE_TEMP_RANGE_MAX];
};

struct puc_strategy {
	struct oplus_chg_strategy strategy;
	struct puc_strategy_soc_curves soc_curves[PUC_BATT_CURVE_SOC_RANGE_MAX];
	uint32_t soc_range_data[PUC_BATT_CURVE_SOC_RANGE_MAX + 1];
	int32_t temp_range_data[PUC_BATT_CURVE_TEMP_RANGE_MAX + 1];
	uint32_t temp_type;

	struct puc_strategy_temp_curves *curve;
	int curr_level;
	unsigned long timeout;
	unsigned long over_time;
};

#define PUC_DATA_SIZE	sizeof(struct puc_strategy_data)

static const char * const puc_strategy_soc[] = {
	[PUC_BATT_CURVE_SOC_RANGE_MIN]		= "strategy_soc_range_min",
	[PUC_BATT_CURVE_SOC_RANGE_LOW]		= "strategy_soc_range_low",
	[PUC_BATT_CURVE_SOC_RANGE_MID_LOW]	= "strategy_soc_range_mid_low",
	[PUC_BATT_CURVE_SOC_RANGE_MID]		= "strategy_soc_range_mid",
	[PUC_BATT_CURVE_SOC_RANGE_MID_HIGH]	= "strategy_soc_range_mid_high",
	[PUC_BATT_CURVE_SOC_RANGE_HIGH]		= "strategy_soc_range_high",
};

static const char * const puc_strategy_temp[] = {
	[PUC_BATT_CURVE_TEMP_RANGE_LITTLE_COLD]	= "strategy_temp_little_cold",
	[PUC_BATT_CURVE_TEMP_RANGE_COOL]	= "strategy_temp_cool",
	[PUC_BATT_CURVE_TEMP_RANGE_LITTLE_COOL]	= "strategy_temp_little_cool",
	[PUC_BATT_CURVE_TEMP_RANGE_NORMAL_LOW]	= "strategy_temp_normal_low",
	[PUC_BATT_CURVE_TEMP_RANGE_NORMAL_HIGH]	= "strategy_temp_normal_high",
	[PUC_BATT_CURVE_TEMP_RANGE_WARM]	= "strategy_temp_warm",
};

static struct oplus_mms *comm_topic;
static struct oplus_mms *gauge_topic;

__maybe_unused static bool is_comm_topic_available(void)
{
	if (!comm_topic)
		comm_topic = oplus_mms_get_by_name("common");
	return !!comm_topic;
}

__maybe_unused static bool is_gauge_topic_available(void)
{
	if (!gauge_topic)
		gauge_topic = oplus_mms_get_by_name("gauge");
	return !!gauge_topic;
}

static int __read_signed_data_from_node(struct device_node *node,
					const char *prop_str,
					s32 *addr, int len_max)
{
	int rc = 0, length;

	if (!node || !prop_str || !addr) {
		chg_err("Invalid parameters passed\n");
		return -EINVAL;
	}

	rc = of_property_count_elems_of_size(node, prop_str, sizeof(s32));
	if (rc < 0) {
		chg_err("Count %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	length = rc;

	if (length != len_max) {
		chg_err("entries(%d) num error, only %d allowed\n", length,
			len_max);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(node, prop_str, (u32 *)addr, length);
	if (rc) {
		chg_err("Read %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	return rc;
}

static int __read_unsigned_data_from_node(struct device_node *node,
					  const char *prop_str, u32 *addr,
					  int len_max)
{
	int rc = 0, length;

	if (!node || !prop_str || !addr) {
		chg_err("Invalid parameters passed\n");
		return -EINVAL;
	}

	rc = of_property_count_elems_of_size(node, prop_str, sizeof(u32));
	if (rc < 0) {
		chg_err("Count %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	length = rc;

	if (length != len_max) {
		chg_err("entries(%d) num error, only %d allowed\n", length,
			len_max);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(node, prop_str, (u32 *)addr, length);
	if (rc < 0) {
		chg_err("Read %s failed, rc=%d\n", prop_str, rc);
		return rc;
	}

	return length;
}

static int puc_strategy_get_soc(struct puc_strategy *puc, int *soc)
{
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_comm_topic_available()) {
		chg_err("common topic not found\n");
		return -ENODEV;
	}
	rc = oplus_mms_get_item_data(comm_topic, COMM_ITEM_UI_SOC,
				     &data, false);
	if (rc < 0) {
		chg_err("can't get ui soc, rc=%d\n", rc);
		return rc;
	}
	*soc = data.intval;

	return 0;
}

static int puc_strategy_get_vbat(struct puc_strategy *puc, int *vbat)
{
	union mms_msg_data data = { 0 };
	int rc;

	if (!is_gauge_topic_available()) {
		chg_err("gauge topic not found\n");
		return -ENODEV;
	}
	rc = oplus_mms_get_item_data(gauge_topic, GAUGE_ITEM_VOL_MAX,
				     &data, false);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}
	*vbat = data.intval;

	return 0;
}

static int puc_strategy_get_temp(struct puc_strategy *puc, int *temp)
{
	union mms_msg_data data = { 0 };
	int rc;

	switch (puc->temp_type) {
	case STRATEGY_USE_BATT_TEMP:
		if (!is_gauge_topic_available()) {
			chg_err("gauge topic not found\n");
			return -ENODEV;
		}
		rc = oplus_mms_get_item_data(gauge_topic, GAUGE_ITEM_TEMP,
					     &data, false);
		if (rc < 0) {
			chg_err("can't get battery temp, rc=%d\n", rc);
			return rc;
		}

		*temp = data.intval;
		break;
	case STRATEGY_USE_SHELL_TEMP:
		if (!is_comm_topic_available()) {
			chg_err("common topic not found\n");
			return -ENODEV;
		}
		rc = oplus_mms_get_item_data(comm_topic, COMM_ITEM_SHELL_TEMP,
					     &data, false);
		if (rc < 0) {
			chg_err("can't get shell temp, rc=%d\n", rc);
			return rc;
		}

		*temp = data.intval;
		break;
	default:
		chg_err("not support temp type, type=%d\n", puc->temp_type);
		return -EINVAL;
	}

	return 0;
}

static enum puc_soc_range
puc_get_soc_region(struct puc_strategy *puc)
{
	int soc;
	enum puc_soc_range soc_region = PUC_BATT_CURVE_SOC_RANGE_INVALID;
	int i;
	int rc;

	rc = puc_strategy_get_soc(puc, &soc);
	if (rc < 0) {
		chg_err("can't get soc, rc=%d\n", rc);
		return PUC_BATT_CURVE_SOC_RANGE_INVALID;
	}

	for (i = 1; i < PUC_BATT_CURVE_SOC_RANGE_MAX + 1; i++) {
		if (soc < puc->soc_range_data[i]) {
			soc_region = i - 1;
			break;
		}
	}

	return soc_region;
}

static enum puc_temp_range
puc_get_temp_region(struct puc_strategy *puc)
{
	int temp;
	enum puc_temp_range temp_region = PUC_BATT_CURVE_TEMP_RANGE_INVALID;
	int i;
	int rc;

	rc = puc_strategy_get_temp(puc, &temp);
	if (rc < 0) {
		chg_err("can't get temp, rc=%d\n", rc);
		return PUC_BATT_CURVE_TEMP_RANGE_INVALID;
	}

	for (i = 0; i <= PUC_BATT_CURVE_TEMP_RANGE_MAX; i++) {
		if (temp < puc->temp_range_data[i]) {
			if (i != 0)
				temp_region = i - 1;
			break;
		}
	}

	return temp_region;
}

static struct oplus_chg_strategy *
puc_strategy_alloc(unsigned char *buf, size_t size)
{
	return ERR_PTR(-ENOTSUPP);
}

static struct oplus_chg_strategy *
puc_strategy_alloc_by_node(struct device_node *node)
{
	struct puc_strategy *puc;
	u32 data;
	int rc;
	int i, j;
	int length;
	struct device_node *soc_node;

	if (node == NULL) {
		chg_err("node is NULL\n");
		return ERR_PTR(-EINVAL);
	}

	puc = kzalloc(sizeof(struct puc_strategy), GFP_KERNEL);
	if (puc == NULL) {
		chg_err("alloc strategy memory error\n");
		return ERR_PTR(-ENOMEM);
	}

	rc = of_property_read_u32(node, "oplus,temp_type", &data);
	if (rc < 0) {
		chg_err("oplus,temp_type reading failed, rc=%d\n", rc);
		puc->temp_type = STRATEGY_USE_SHELL_TEMP;
	} else {
		puc->temp_type = (uint32_t)data;
	}
	rc = __read_unsigned_data_from_node(node, "oplus,soc_range",
					    (u32 *)puc->soc_range_data,
					    PUC_BATT_CURVE_SOC_RANGE_MAX + 1);
	if (rc < 0) {
		chg_err("get oplus,soc_range property error, rc=%d\n", rc);
		goto base_info_err;
	}
	rc = __read_signed_data_from_node(node, "oplus,temp_range",
					  (s32 *)puc->temp_range_data,
					  PUC_BATT_CURVE_TEMP_RANGE_MAX + 1);
	if (rc < 0) {
		chg_err("get oplus,temp_range property error, rc=%d\n", rc);
		goto base_info_err;
	}

	for (i = 0; i < PUC_BATT_CURVE_SOC_RANGE_MAX; i++) {
		soc_node = of_get_child_by_name(node, puc_strategy_soc[i]);
		if (!soc_node) {
			chg_err("can't find %s node\n", puc_strategy_soc[i]);
			rc = -ENODEV;
			goto data_err;
		}

		for (j = 0; j < PUC_BATT_CURVE_TEMP_RANGE_MAX; j++) {
			length = of_property_count_elems_of_size(
				soc_node, puc_strategy_temp[j], sizeof(u32));
			if (length < 0) {
				chg_err("can't find %s property, rc=%d\n",
					puc_strategy_temp[j], length);
				goto data_err;
			}
			rc = length * sizeof(u32);
			if (rc % PUC_DATA_SIZE != 0) {
				chg_err("buf size does not meet the requirements, size=%d\n", rc);
				rc = -EINVAL;
				goto data_err;
			}

			puc->soc_curves[i].temp_curves[j].num = rc / PUC_DATA_SIZE;
			puc->soc_curves[i].temp_curves[j].data = kzalloc(rc , GFP_KERNEL);
			if (puc->soc_curves[i].temp_curves[j].data == NULL) {
				chg_err("alloc strategy data memory error\n");
				rc = -ENOMEM;
				goto data_err;
			}

			rc = of_property_read_u32_array(
					soc_node, puc_strategy_temp[j],
					(u32 *)puc->soc_curves[i].temp_curves[j].data,
					length);
			if (rc < 0) {
				chg_err("read %s property error, rc=%d\n",
					puc_strategy_temp[j], rc);
				goto data_err;
			}
		}
	}

	return (struct oplus_chg_strategy *)puc;

data_err:
	for (i = 0; i < PUC_BATT_CURVE_SOC_RANGE_MAX; i++) {
		for (j = 0; j < PUC_BATT_CURVE_TEMP_RANGE_MAX; j++) {
			if (puc->soc_curves[i].temp_curves[j].data != NULL) {
				kfree(puc->soc_curves[i].temp_curves[j].data);
				puc->soc_curves[i].temp_curves[j].data = NULL;
			}
		}
	}
base_info_err:
	kfree(puc);
	return ERR_PTR(rc);
}

static int puc_strategy_release(struct oplus_chg_strategy *strategy)
{
	struct puc_strategy *puc;
	int i, j;

	if (strategy == NULL) {
		chg_err("strategy is NULL\n");
		return -EINVAL;
	}
	puc = (struct puc_strategy *)strategy;

	for (i = 0; i < PUC_BATT_CURVE_SOC_RANGE_MAX; i++) {
		for (j = 0; j < PUC_BATT_CURVE_TEMP_RANGE_MAX; j++) {
			if (puc->soc_curves[i].temp_curves[j].data != NULL) {
				kfree(puc->soc_curves[i].temp_curves[j].data);
				puc->soc_curves[i].temp_curves[j].data = NULL;
			}
		}
	}
	kfree(puc);

	return 0;
}

static int puc_strategy_init(struct oplus_chg_strategy *strategy)
{
	struct puc_strategy *puc;
	enum puc_temp_range temp_range;
	enum puc_soc_range soc_range;
	int vbat;
	int i;
	int rc;

	if (strategy == NULL) {
		chg_err("strategy is NULL\n");
		return -EINVAL;
	}
	puc = (struct puc_strategy *)strategy;

	soc_range = puc_get_soc_region(puc);
	if (soc_range == PUC_BATT_CURVE_SOC_RANGE_INVALID)
		return -EFAULT;
	temp_range = puc_get_temp_region(puc);
	if (temp_range == PUC_BATT_CURVE_SOC_RANGE_INVALID)
		return -EFAULT;

	rc = puc_strategy_get_vbat(puc, &vbat);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}

	chg_info("uss %s:%s curve\n", puc_strategy_soc[soc_range], puc_strategy_temp[temp_range]);
	puc->curve = &puc->soc_curves[soc_range].temp_curves[temp_range];
	for (i = 0; i < puc->curve->num; i++) {
		if (vbat < puc->curve->data[i].target_vbat) {
			puc->curr_level = i;
			break;
		}
	}
	if (i >= puc->curve->num) {
		chg_err("The battery voltage is too high, there is no suitable range, vbat=%d\n", vbat);
		return -EINVAL;
	}
	if (puc->curve->data[puc->curr_level].target_time > 0)
		puc->timeout = jiffies + msecs_to_jiffies(puc->curve->data[puc->curr_level].target_time * 1000);
	else
		puc->timeout = 0;
	puc->over_time = 0;

	return 0;
}

static int puc_strategy_get_data(struct oplus_chg_strategy *strategy, void *ret)
{
	struct puc_strategy *puc;
	struct puc_strategy_ret_data *ret_data;
	struct puc_strategy_data *data;
	int vbat;
	int rc;
	bool curve_level_update = false;

#define VBAT_OVER_TIME_MS	2500

	if (strategy == NULL) {
		chg_err("strategy is NULL\n");
		return -EINVAL;
	}
	if (ret == NULL) {
		chg_err("ret is NULL\n");
		return -EINVAL;
	}
	puc = (struct puc_strategy *)strategy;

	if (puc->curr_level >= puc->curve->num)
		goto out;

	rc = puc_strategy_get_vbat(puc, &vbat);
	if (rc < 0) {
		chg_err("can't get vbat, rc=%d\n", rc);
		return rc;
	}

	data = &puc->curve->data[puc->curr_level];
	if (puc->timeout > 0 && time_is_before_jiffies(puc->timeout)) {
		puc->curr_level++;
		curve_level_update = true;
		chg_info("timeout, switch to next level(=%d)\n", puc->curr_level);
		goto out;
	}
	if (vbat > data->target_vbat) {
		if (puc->over_time == 0) {
			puc->over_time = msecs_to_jiffies(VBAT_OVER_TIME_MS) + jiffies;
		} else if (time_is_before_jiffies(puc->over_time)) {
			puc->over_time = 0;
			puc->curr_level++;
			curve_level_update = true;
			chg_info("switch to next level(=%d)\n", puc->curr_level);
			goto out;
		}
	} else {
		puc->over_time = 0;
	}

out:
	ret_data = (struct puc_strategy_ret_data *)ret;
	if (puc->curr_level >= puc->curve->num) {
		ret_data->target_vbus = 0;
		ret_data->target_vbat = 0;
		ret_data->target_ibus = 0;
		ret_data->index = 0;
		ret_data->last_gear = false;
		ret_data->exit = true;
		chg_info("curve exit\n");
		return 0;
	}

	if (curve_level_update) {
		data = &puc->curve->data[puc->curr_level];
		if (data->target_time > 0)
			puc->timeout = jiffies + msecs_to_jiffies(data->target_time * 1000);
		else
			puc->timeout = 0;
		puc->over_time = 0;
		chg_info("level[%d]: %d %d %d %d %d\n", puc->curr_level,
			 data->target_vbus, data->target_vbat,
			 data->target_ibus, data->exit, data->target_time);
	}

	ret_data->target_vbus = data->target_vbus;
	ret_data->target_vbat = data->target_vbat;
	ret_data->target_ibus = data->target_ibus;
	ret_data->index = puc->curr_level;
	ret_data->last_gear = !!data->exit;
	ret_data->exit = false;

	return 0;
}

static struct oplus_chg_strategy_desc puc_strategy_desc = {
	.name = "pps_ufcs_curve",
	.strategy_init = puc_strategy_init,
	.strategy_release = puc_strategy_release,
	.strategy_alloc = puc_strategy_alloc,
	.strategy_alloc_by_node = puc_strategy_alloc_by_node,
	.strategy_get_data = puc_strategy_get_data,
};

int puc_strategy_register(void)
{
	return oplus_chg_strategy_register(&puc_strategy_desc);
}

struct puc_strategy_temp_curves *puc_strategy_get_curr_curve_data(struct oplus_chg_strategy *strategy)
{
	struct puc_strategy *puc;

	if (strategy == NULL) {
		chg_err("strategy is NULL\n");
		return NULL;
	}
	puc = (struct puc_strategy *)strategy;

	return puc->curve;
}
