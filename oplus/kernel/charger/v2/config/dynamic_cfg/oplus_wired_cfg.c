// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */

#define LOG_BUF_SIZE	4096
static char *g_log_buf;

static void oplus_wired_update_pd_qc_config(struct oplus_param_head *param_head, struct oplus_wired_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	u32 buf;
	int rc;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,vbatt_pdqc_to_5v_thr");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,vbatt_pdqc_to_5v_thr data error, rc=%d\n", rc);
		} else {
			spec->vbatt_pdqc_to_5v_thr = le32_to_cpu(buf);
			chg_info("[TEST]:vbatt_pdqc_to_5v_thr=%d\n", spec->vbatt_pdqc_to_5v_thr);
		}
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,vbatt_pdqc_to_9v_thr");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,vbatt_pdqc_to_9v_thr data error, rc=%d\n", rc);
		} else {
			spec->vbatt_pdqc_to_9v_thr = le32_to_cpu(buf);
			chg_info("[TEST]:vbatt_pdqc_to_9v_thr=%d\n", spec->vbatt_pdqc_to_9v_thr);
		}
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,pd-iclmax-ma");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,pd-iclmax-ma data error, rc=%d\n", rc);
		} else {
			spec->pd_iclmax_ma = le32_to_cpu(buf);
			chg_info("[TEST]:pd_iclmax_ma=%d\n", spec->pd_iclmax_ma);
		}
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,qc-iclmax-ma");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,qc-iclmax-ma data error, rc=%d\n", rc);
		} else {
			spec->qc_iclmax_ma = le32_to_cpu(buf);
			chg_info("[TEST]:qc_iclmax_ma=%d\n", spec->qc_iclmax_ma);
		}
	}
}

static void oplus_wired_update_input_power_config(
	struct oplus_param_head *param_head, struct oplus_wired_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	u32 buf[OPLUS_WIRED_CHG_MODE_MAX];
	int i;
	int rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,input-power-mw");
	if (data_head == NULL)
		return;

	rc = oplus_cfg_get_data(data_head,  (u8 *)buf, sizeof(buf));
	if (rc < 0) {
		chg_err("get oplus_spec,input-power-mw data buf error, rc=%d\n", rc);
		return;
	}
	for (i = 0; i < OPLUS_WIRED_CHG_MODE_MAX; i++) {
		spec->input_power_mw[i] = le32_to_cpu(buf[i]);
		if (g_log_buf) {
			index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
				(i == 0) ? "" : ", ", spec->input_power_mw[i]);
			g_log_buf[index] = 0;
		}
	}
	if (g_log_buf)
		chg_info("[TEST]:input-power-mw = { %s }\n", g_log_buf);
}

static void oplus_wired_update_fcc_config(
	struct oplus_param_head *param_head, struct oplus_wired_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	u32 buf[OPLUS_WIRED_CHG_MODE_MAX][TEMP_REGION_MAX];
	int i, j;
	int rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,led_on-fccmax-ma");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)buf[0], sizeof(u32) * TEMP_REGION_MAX);
		if (rc < 0) {
			chg_err("get oplus_spec,led_on-fccmax-ma buf error, rc=%d\n", rc);
		} else {
			for (i = 0; i < TEMP_REGION_MAX; i++) {
				spec->led_on_fcc_max_ma[i] = le32_to_cpu(buf[0][i]);
				if (g_log_buf) {
					index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
						(i == 0) ? "" : ", ", spec->led_on_fcc_max_ma[i]);
					g_log_buf[index] = 0;
				}
			}
			if (g_log_buf)
				chg_info("[TEST]:led_on-fccmax-ma = { %s }\n", g_log_buf);
		}
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,fccmax-ma-lv");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,fccmax-ma-lv buf error, rc=%d\n", rc);
		} else {
			index = 0;
			for (i = 0; i < OPLUS_WIRED_CHG_MODE_MAX; i++) {
				for (j = 0; j < TEMP_REGION_MAX; j++) {
					spec->fcc_ma[0][i][j] = le32_to_cpu(buf[i][j]);
					if (g_log_buf) {
						index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
							(j == 0) ? "" : ", ", spec->fcc_ma[0][i][j]);
						g_log_buf[index] = 0;
					}
				}
				if (i < (OPLUS_WIRED_CHG_MODE_MAX - 1)) {
					if (g_log_buf) {
						index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, " |");
						g_log_buf[index] = 0;
					}
				}
			}
			if (g_log_buf)
				chg_info("[TEST]:fccmax-ma-lv = { %s }\n", g_log_buf);
		}
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,fccmax-ma-hv");
	if (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,fccmax-ma-hv buf error, rc=%d\n", rc);
		} else {
			index = 0;
			for (i = 0; i < OPLUS_WIRED_CHG_MODE_MAX; i++) {
				for (j = 0; j < TEMP_REGION_MAX; j++) {
					spec->fcc_ma[1][i][j] = le32_to_cpu(buf[i][j]);
					if (g_log_buf) {
						index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
							(j == 0) ? "" : ", ", spec->fcc_ma[1][i][j]);
						g_log_buf[index] = 0;
					}
				}
				if (i < (OPLUS_WIRED_CHG_MODE_MAX - 1)) {
					if (g_log_buf) {
						index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, " |");
						g_log_buf[index] = 0;
					}
				}
			}
			if (g_log_buf)
				chg_info("[TEST]:fccmax-ma-hv = { %s }\n", g_log_buf);
		}
	}
}

static void oplus_wired_update_cool_down_config(
	struct oplus_param_head *param_head, struct oplus_wired_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	u32 buf[WIRED_COOL_DOWN_LEVEL_MAX];
	ssize_t data_len;
	int i, count;
	int rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,cool_down_pdqc_vol_mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,cool_down_pdqc_vol_mv data buf error, rc=%d\n", rc);
			break;
		}
		count = 0;
		for (i = 0; i < WIRED_COOL_DOWN_LEVEL_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->cool_down_pdqc_vol_mv[i] = le32_to_cpu(buf[i]);
			if (spec->cool_down_pdqc_vol_mv[i] > 0) {
				if (count >= 0)
					count++;
			} else {
				spec->cool_down_pdqc_level_max = count;
				count = -EINVAL;
			}
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->cool_down_pdqc_vol_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:cool_down_pdqc_vol_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,cool_down_pdqc_curr_ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,cool_down_pdqc_curr_ma data buf error, rc=%d\n", rc);
			break;
		}
		count = 0;
		index = 0;
		for (i = 0; i < WIRED_COOL_DOWN_LEVEL_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->cool_down_pdqc_curr_ma[i] = le32_to_cpu(buf[i]);
			if (spec->cool_down_pdqc_curr_ma[i] > 0) {
				if (count >= 0)
					count++;
			} else {
				spec->cool_down_pdqc_level_max = count;
				count = -EINVAL;
			}
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->cool_down_pdqc_curr_ma[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:cool_down_pdqc_curr_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,cool_down_vooc_curr_ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,cool_down_vooc_curr_ma data buf error, rc=%d\n", rc);
			break;
		}
		count = 0;
		index = 0;
		for (i = 0; i < WIRED_COOL_DOWN_LEVEL_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->cool_down_vooc_curr_ma[i] = le32_to_cpu(buf[i]);
			if (spec->cool_down_vooc_curr_ma[i] > 0) {
				if (count >= 0)
					count++;
			} else {
				spec->cool_down_vooc_level_max = count;
				count = -EINVAL;
			}
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->cool_down_vooc_curr_ma[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:cool_down_vooc_curr_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,cool_down_normal_curr_ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,cool_down_normal_curr_ma data buf error, rc=%d\n", rc);
			break;
		}
		count = 0;
		index = 0;
		for (i = 0; i < WIRED_COOL_DOWN_LEVEL_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->cool_down_normal_curr_ma[i] = le32_to_cpu(buf[i]);
			if (spec->cool_down_normal_curr_ma[i] > 0) {
				if (count >= 0)
					count++;
			} else {
				spec->cool_down_normal_level_max = count;
				count = -EINVAL;
			}
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->cool_down_normal_curr_ma[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:cool_down_normal_curr_ma = { %s }\n", g_log_buf);
		break;
	}
}

static void oplus_wired_update_vbus_ov_uv_config(
	struct oplus_param_head *param_head, struct oplus_wired_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	u32 buf[OPLUS_VBUS_MAX];
	ssize_t data_len;
	int i;
	int rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,vbus_ov_thr_mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,vbus_ov_thr_mv data buf error, rc=%d\n", rc);
			break;
		}
		for (i = 0; i < OPLUS_VBUS_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->vbus_ov_thr_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->vbus_ov_thr_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:vbus_ov_thr_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,vbus_uv_thr_mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len > sizeof(buf)) {
			chg_err("Too much configuration data\n");
			break;
		}

		rc = oplus_cfg_get_data(data_head,  (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,vbus_uv_thr_mv data buf error, rc=%d\n", rc);
			break;
		}
		index = 0;
		for (i = 0; i < OPLUS_VBUS_MAX; i++) {
			if (i >= (data_len / sizeof(u32)))
				buf[i] = 0;
			spec->vbus_uv_thr_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->vbus_uv_thr_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:vbus_uv_thr_mv = { %s }\n", g_log_buf);
		break;
	}
}

static int oplus_wired_update_spec_config(void *data, struct oplus_param_head *param_head)
{
	struct oplus_wired_spec_config *spec;

	if (data == NULL) {
		chg_err("data is NULL\n");
		return -EINVAL;
	}
	if (param_head == NULL) {
		chg_err("param_head is NULL\n");
		return -EINVAL;
	}
	spec = (struct oplus_wired_spec_config *)data;

	g_log_buf = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	oplus_wired_update_pd_qc_config(param_head, spec);
	oplus_wired_update_input_power_config(param_head, spec);
	oplus_wired_update_fcc_config(param_head, spec);
	oplus_wired_update_cool_down_config(param_head, spec);
	oplus_wired_update_vbus_ov_uv_config(param_head, spec);

	if (g_log_buf != NULL) {
		kfree(g_log_buf);
		g_log_buf = NULL;
	}

	return 0;
}

#define STRATEGY_NAME_LEN	128
static void oplus_wired_update_vooc_strategy_config(
	struct oplus_param_head *param_head, struct oplus_wired_config *config)
{
	struct oplus_chg_wired *chip;
	struct oplus_cfg_data_head *data_head;
	struct oplus_chg_strategy *vooc_strategy;
	char name[STRATEGY_NAME_LEN] = { 0 };
	u8 *data_buf;
	ssize_t data_len;
	int i, index;
	int rc;

	chip = container_of(config, struct oplus_chg_wired, config);

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,vooc_strategy_name");
	if (data_head == NULL)
		return;
	data_len = oplus_cfg_get_data_size(data_head);
	if (data_len >= STRATEGY_NAME_LEN) {
		chg_err("strategy name is too long\n");
		return;
	}
	rc = oplus_cfg_get_data(data_head, name, data_len);
	if (rc < 0) {
		chg_err("get oplus,vooc_strategy_name error, rc=%d\n", rc);
		return;
	}
	name[data_len] = 0;
	chg_info("[TEST]:vooc_strategy_name = %s\n", name);

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,vooc_strategy_data");
	if (data_head == NULL) {
		chg_err("oplus,vooc_strategy_data not found\n");
		return;
	}
	data_len = oplus_cfg_get_data_size(data_head);
	data_buf = devm_kzalloc(chip->dev, data_len, GFP_KERNEL);
	if (data_buf == NULL) {
		chg_err("alloc strategy data buf error\n");
		return;
	}

	rc = oplus_cfg_get_data(data_head, data_buf, data_len);
	if (rc < 0) {
		chg_err("get oplus,vooc_strategy_data error, rc=%d\n", rc);
		goto out;
	}
	vooc_strategy = oplus_chg_strategy_alloc(name, data_buf, data_len);
	if (vooc_strategy == NULL)
		chg_err("vooc strategy alloc error");
	else
		chip->vooc_strategy = vooc_strategy;

	if (g_log_buf) {
		index = 0;
		for (i = 0; i < (data_len / sizeof(u32)); i++) {
			index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
				(i == 0) ? "" : ", ", le32_to_cpu(*((u32 *)(data_buf + i * sizeof(u32)))));
			g_log_buf[index] = 0;
		}
		chg_info("[TEST]:vooc_strategy_data = { %s }\n", g_log_buf);
	}

out:
	devm_kfree(chip->dev, data_buf);
}

static int oplus_wired_update_normal_config(void *data, struct oplus_param_head *param_head)
{
	struct oplus_wired_config *config;

	if (data == NULL) {
		chg_err("data is NULL\n");
		return -EINVAL;
	}
	if (param_head == NULL) {
		chg_err("param_head is NULL\n");
		return -EINVAL;
	}
	config = (struct oplus_wired_config *)data;

	g_log_buf = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	oplus_wired_update_vooc_strategy_config(param_head, config);

	if (g_log_buf != NULL) {
		kfree(g_log_buf);
		g_log_buf = NULL;
	}

	return 0;
}

static int oplus_wired_reg_debug_config(struct oplus_chg_wired *chip)
{
	int rc;

	chip->spec_debug_cfg.type = OPLUS_CHG_WIRED_SPEC_PARAM;
	chip->spec_debug_cfg.update = oplus_wired_update_spec_config;
	chip->spec_debug_cfg.priv_data = &chip->spec;
	rc = oplus_cfg_register(&chip->spec_debug_cfg);
	if (rc < 0)
		chg_err("spec cfg register error, rc=%d\n", rc);

	chip->normal_debug_cfg.type = OPLUS_CHG_WIRED_NORMAL_PARAM;
	chip->normal_debug_cfg.update = oplus_wired_update_normal_config;
	chip->normal_debug_cfg.priv_data = &chip->config;
	rc = oplus_cfg_register(&chip->normal_debug_cfg);
	if (rc < 0)
		chg_err("normal cfg register error, rc=%d\n", rc);

	return 0;
}

static void oplus_wired_unreg_debug_config(struct oplus_chg_wired *chip)
{
	oplus_cfg_unregister(&chip->spec_debug_cfg);
	oplus_cfg_unregister(&chip->normal_debug_cfg);
}
