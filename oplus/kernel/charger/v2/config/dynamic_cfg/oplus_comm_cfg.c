// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */

#define LOG_BUF_SIZE	4096
static char *g_log_buf;

static void oplus_comm_update_temp_region_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	int32_t buf[TEMP_REGION_MAX - 1];
	int i, rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,batt-them-thr");
	if (data_head == NULL)
		return;
	rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
	if (rc < 0) {
		chg_err("get oplus_spec,batt-them-thr data error, rc=%d\n", rc);
		return;
	}
	for (i = 0; i < ARRAY_SIZE(buf); i++) {
		spec->batt_temp_thr[i] = le32_to_cpu(buf[i]);
		if (g_log_buf) {
			index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
				(i == 0) ? "" : ", ", spec->batt_temp_thr[i]);
			g_log_buf[index] = 0;
		}
	}
	if (g_log_buf)
		chg_info("[TEST]:batt_temp_thr = { %s }\n", g_log_buf);
}

static void oplus_comm_update_iterm_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	int32_t buf;
	int rc;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,iterm-ma");
	if (data_head == NULL)
		return;
	rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
	if (rc < 0) {
		chg_err("get oplus_spec,iterm-ma data error, rc=%d\n", rc);
		return;
	}
	spec->iterm_ma = le32_to_cpu(buf);
	chg_info("[TEST]:iterm_ma = %d\n", spec->iterm_ma);
}

static void oplus_comm_update_fv_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	int32_t buf[TEMP_REGION_MAX];
	int i, rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,fv-mv data error, rc=%d\n", rc);
			break;
		}
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->fv_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->fv_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:fv_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,sw-fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,sw-fv-mv data error, rc=%d\n", rc);
			break;
		}
		index = 0;
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->sw_fv_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->sw_fv_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:sw_fv_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,hw-fv-inc-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,hw-fv-inc-mv data error, rc=%d\n", rc);
			break;
		}
		index = 0;
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->hw_fv_inc_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->hw_fv_inc_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:hw_fv_inc_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,sw-over-fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,sw-over-fv-mv data error, rc=%d\n", rc);
			break;
		}
		index = 0;
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->sw_over_fv_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->sw_over_fv_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:sw_over_fv_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,sw-over-fv-dec-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,sw-over-fv-dec-mv data error, rc=%d\n", rc);
			break;
		}
		spec->sw_over_fv_dec_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:sw_over_fv_dec_mv = %d\n", spec->sw_over_fv_dec_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,non-standard-sw-fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,non-standard-sw-fv-mv data error, rc=%d\n", rc);
			break;
		}
		spec->non_standard_sw_fv_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:non_standard_sw_fv_mv = %d\n", spec->non_standard_sw_fv_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,non-standard-fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,non-standard-fv-mv data error, rc=%d\n", rc);
			break;
		}
		spec->non_standard_fv_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:non_standard_fv_mv = %d\n", spec->non_standard_fv_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,non-standard-hw-fv-inc-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,non-standard-hw-fv-inc-mv data error, rc=%d\n", rc);
			break;
		}
		spec->non_standard_hw_fv_inc_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:non_standard_hw_fv_inc_mv = %d\n", spec->non_standard_hw_fv_inc_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,non-standard-sw-over-fv-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,non-standard-sw-over-fv-mv data error, rc=%d\n", rc);
			break;
		}
		spec->non_standard_sw_over_fv_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:non_standard_sw_over_fv_mv = %d\n", spec->non_standard_sw_over_fv_mv);
		break;
	}
}

static void oplus_comm_update_rechg_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	int32_t buf[TEMP_REGION_MAX];
	int i, rc;
	int index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-vbatdet-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,wired-vbatdet-mv data error, rc=%d\n", rc);
			break;
		}
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->wired_vbatdet_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->wired_vbatdet_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_vbatdet_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-vbatdet-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,wls-vbatdet-mv data error, rc=%d\n", rc);
			break;
		}
		index = 0;
		for (i = 0; i < ARRAY_SIZE(buf); i++) {
			spec->wls_vbatdet_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				index += snprintf(g_log_buf + index, LOG_BUF_SIZE - index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->wls_vbatdet_mv[i]);
				g_log_buf[index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_vbatdet_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,non-standard-vbatdet-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,non-standard-vbatdet-mv data error, rc=%d\n", rc);
			break;
		}
		spec->non_standard_vbatdet_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:non_standard_vbatdet_mv = %d\n", spec->non_standard_vbatdet_mv);
		break;
	}
}

static void oplus_comm_update_misc_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
	struct oplus_cfg_data_head *data_head;
	int32_t buf;
	int rc;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,fcc-gear-thr-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,fcc-gear-thr-mv data error, rc=%d\n", rc);
			break;
		}
		spec->fcc_gear_thr_mv = le32_to_cpu(buf);
		chg_info("[TEST]:fcc_gear_thr_mv = %d\n", spec->fcc_gear_thr_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,vbatt-ov-thr-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,vbatt-ov-thr-mv data error, rc=%d\n", rc);
			break;
		}
		spec->vbatt_ov_thr_mv = le32_to_cpu(buf);
		chg_info("[TEST]:vbatt_ov_thr_mv = %d\n", spec->vbatt_ov_thr_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,max_chg_time_sec");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus_spec,max_chg_time_sec data error, rc=%d\n", rc);
			break;
		}
		spec->max_chg_time_sec = le32_to_cpu(buf);
		chg_info("[TEST]:max_chg_time_sec = %d\n", spec->max_chg_time_sec);
		break;
	}
}

static void oplus_comm_update_ffc_config(
	struct oplus_param_head *param_head, struct oplus_comm_spec_config *spec)
{
#define FFC_BUF_MAX (FFC_CHG_STEP_MAX * max(AGAIN_FFC_CYCLY_THR_COUNT, (FFC_TEMP_REGION_MAX - 2)))
	struct oplus_cfg_data_head *data_head;
	ssize_t data_len;
	u8 enable;
	int32_t buf[FFC_CHG_STEP_MAX * (FFC_TEMP_REGION_MAX - 2)];
	int i, j, index, rc;
	int log_index = 0;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,full_pre_ffc_judge");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&enable, sizeof(enable));
		if (rc < 0) {
			chg_err("get oplus_spec,full_pre_ffc_judge data error, rc=%d\n", rc);
			break;
		}
		spec->full_pre_ffc_judge = !!enable;
		chg_info("[TEST]:full_pre_ffc_judge = %s\n", spec->full_pre_ffc_judge ? "true" : "false");
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,full-pre-ffc-mv");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,full-pre-ffc-mv data error, rc=%d\n", rc);
			break;
		}
		spec->full_pre_ffc_mv = le32_to_cpu(buf[0]);
		chg_info("[TEST]:full_pre_ffc_mv = %d\n", spec->full_pre_ffc_mv);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-ffc-step-max");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,wired-ffc-step-max data error, rc=%d\n", rc);
			break;
		}
		spec->wired_ffc_step_max = le32_to_cpu(buf[0]);
		chg_info("[TEST]:wired_ffc_step_max = %d\n", spec->wired_ffc_step_max);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-ffc-fv-mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_CHG_STEP_MAX) {
			chg_err("Too much configuration data, data_len=%ld\n", data_len / sizeof(buf[0]));
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-ffc-fv-mv data error, rc=%d\n", rc);
			break;
		}
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			if (i >= (data_len / sizeof(buf[0])))
				buf[i] = 0;
			spec->wired_ffc_fv_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				log_index += snprintf(g_log_buf + log_index,
					LOG_BUF_SIZE - log_index - 1, "%s%d",
					(i == 0) ? " " : ", ", spec->wired_ffc_fv_mv[i]);
				g_log_buf[log_index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_ffc_fv_mv = {%s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-ffc-fv-cutoff-mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-ffc-fv-cutoff-mv data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wired_ffc_fv_cutoff_mv[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wired_ffc_fv_cutoff_mv[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_ffc_fv_cutoff_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-ffc-fcc-ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-ffc-fcc-ma data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wired_ffc_fcc_ma[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wired_ffc_fcc_ma[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_ffc_fcc_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-ffc-fcc-cutoff-ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-ffc-fcc-cutoff-ma data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wired_ffc_fcc_cutoff_ma[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wired_ffc_fcc_cutoff_ma[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_ffc_fcc_cutoff_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-aging-ffc-version");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,wired-aging-ffc-version data error, rc=%d\n", rc);
			break;
		}
		spec->wired_aging_ffc_version = le32_to_cpu(buf[0]);
		chg_info("[TEST]:wired_aging_ffc_version = %d\n", spec->wired_aging_ffc_version);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-aging-ffc-offset-mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-aging-ffc-offset-mv data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < AGAIN_FFC_CYCLY_THR_COUNT; j++) {
				index = i * AGAIN_FFC_CYCLY_THR_COUNT + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wired_aging_ffc_offset_mv[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wired_aging_ffc_offset_mv[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_aging_ffc_offset_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wired-aging-ffc-cycle-thr");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > AGAIN_FFC_CYCLY_THR_COUNT) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wired-aging-ffc-cycle-thr data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < AGAIN_FFC_CYCLY_THR_COUNT; i++) {
			if (i >= (data_len / sizeof(buf[0])))
				buf[i] = 0;
			spec->wired_aging_ffc_cycle_thr[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				log_index += snprintf(g_log_buf + log_index, LOG_BUF_SIZE - log_index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->wired_aging_ffc_cycle_thr[i]);
				g_log_buf[log_index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wired_aging_ffc_cycle_thr = { %s }\n", g_log_buf);
		break;
	}

	/* wireless */
	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-step-max");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, sizeof(buf[0]));
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-step-max data error, rc=%d\n", rc);
			break;
		}
		spec->wls_ffc_step_max = le32_to_cpu(buf[0]);
		chg_info("[TEST]:wls_ffc_step_max = %d\n", spec->wls_ffc_step_max);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-fv-mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_CHG_STEP_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-fv-mv data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			if (i >= (data_len / sizeof(buf[0])))
				buf[i] = 0;
			spec->wls_ffc_fv_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				log_index += snprintf(g_log_buf + log_index, LOG_BUF_SIZE - log_index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->wls_ffc_fv_mv[i]);
				g_log_buf[log_index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_ffc_fv_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-fv-cutoff-mv");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_CHG_STEP_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-fv-cutoff-mv data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			if (i >= (data_len / sizeof(buf[0])))
				buf[i] = 0;
			spec->wls_ffc_fv_cutoff_mv[i] = le32_to_cpu(buf[i]);
			if (g_log_buf) {
				log_index += snprintf(g_log_buf + log_index, LOG_BUF_SIZE - log_index - 1, "%s%d",
					(i == 0) ? "" : ", ", spec->wls_ffc_fv_cutoff_mv[i]);
				g_log_buf[log_index] = 0;
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_ffc_fv_cutoff_mv = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-icl-ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-icl-ma data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wls_ffc_icl_ma[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wls_ffc_icl_ma[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_ffc_icl_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-fcc-ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-fcc-ma data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wls_ffc_fcc_ma[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wls_ffc_fcc_ma[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_ffc_fcc_ma = { %s }\n", g_log_buf);
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus_spec,wls-ffc-fcc-cutoff-ma");
	while (data_head != NULL) {
		data_len = oplus_cfg_get_data_size(data_head);
		if (data_len / sizeof(buf[0]) > FFC_BUF_MAX) {
			chg_err("Too much configuration data\n");
			break;
		}
		rc = oplus_cfg_get_data(data_head, (u8 *)buf, data_len);
		if (rc < 0) {
			chg_err("get oplus_spec,wls-ffc-fcc-cutoff-ma data error, rc=%d\n", rc);
			break;
		}
		log_index = 0;
		for (i = 0; i < FFC_CHG_STEP_MAX; i++) {
			for (j = 0; j < (FFC_TEMP_REGION_MAX - 2); j++) {
				index = i * (FFC_TEMP_REGION_MAX - 2) + j;
				if (index >= (data_len / sizeof(buf[0])))
					buf[index] = 0;
				spec->wls_ffc_fcc_cutoff_ma[i][j] = le32_to_cpu(buf[index]);
				if (g_log_buf) {
					log_index += snprintf(g_log_buf + log_index,
						LOG_BUF_SIZE - log_index - 1, "%s%d",
						(((i == 0) && (j == 0)) ? "" : ", "),
						spec->wls_ffc_fcc_cutoff_ma[i][j]);
					g_log_buf[log_index] = 0;
				}
			}
		}
		if (g_log_buf)
			chg_info("[TEST]:wls_ffc_fcc_cutoff_ma = { %s }\n", g_log_buf);
		break;
	}
}

static int oplus_comm_update_spec_config(void *data, struct oplus_param_head *param_head)
{
	struct oplus_comm_spec_config *spec;

	if (data == NULL) {
		chg_err("data is NULL\n");
		return -EINVAL;
	}
	if (param_head == NULL) {
		chg_err("param_head is NULL\n");
		return -EINVAL;
	}
	spec = (struct oplus_comm_spec_config *)data;


	g_log_buf = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	oplus_comm_update_temp_region_config(param_head, spec);
	oplus_comm_update_iterm_config(param_head, spec);
	oplus_comm_update_fv_config(param_head, spec);
	oplus_comm_update_rechg_config(param_head, spec);
	oplus_comm_update_misc_config(param_head, spec);
	oplus_comm_update_ffc_config(param_head, spec);

	if (g_log_buf != NULL) {
		kfree(g_log_buf);
		g_log_buf = NULL;
	}

	return 0;
}

static void oplus_comm_update_ui_soc_config(
	struct oplus_param_head *param_head, struct oplus_comm_config *config)
{
	struct oplus_cfg_data_head *data_head;
	u8 enable;
	int32_t buf;
	int rc;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,vooc_show_ui_soc_decimal");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&enable, sizeof(enable));
		if (rc < 0) {
			chg_err("get oplus,vooc_show_ui_soc_decimal data error, rc=%d\n", rc);
			break;
		}
		config->vooc_show_ui_soc_decimal = !!enable;
		chg_info("[TEST]:vooc_show_ui_soc_decimal = %s\n", config->vooc_show_ui_soc_decimal ? "true" : "false");
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,ui_soc_decimal_speedmin");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus,ui_soc_decimal_speedmin data error, rc=%d\n", rc);
			break;
		}
		config->ui_soc_decimal_speedmin = le32_to_cpu(buf);
		chg_info("[TEST]:ui_soc_decimal_speedmin = %d\n", config->ui_soc_decimal_speedmin);
		break;
	}
}

static void oplus_comm_update_smooth_soc_config(
	struct oplus_param_head *param_head, struct oplus_comm_config *config)
{
		struct oplus_cfg_data_head *data_head;
	u8 enable;
	int32_t buf;
	int rc;

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,smooth_switch");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&enable, sizeof(enable));
		if (rc < 0) {
			chg_err("get oplus,smooth_switch data error, rc=%d\n", rc);
			break;
		}
		config->smooth_switch = !!enable;
		chg_info("[TEST]:smooth_switch = %s\n", config->smooth_switch ? "true" : "false");
		break;
	}

	data_head = oplus_cfg_find_param_by_name(param_head, "oplus,reserve_chg_soc");
	while (data_head != NULL) {
		rc = oplus_cfg_get_data(data_head, (u8 *)&buf, sizeof(buf));
		if (rc < 0) {
			chg_err("get oplus,reserve_chg_soc data error, rc=%d\n", rc);
			break;
		}
		config->reserve_soc = le32_to_cpu(buf);
		chg_info("[TEST]:reserve_soc = %d\n", config->reserve_soc);
		break;
	}
}

static int oplus_comm_update_normal_config(void *data, struct oplus_param_head *param_head)
{
	struct oplus_comm_config *config;

	if (data == NULL) {
		chg_err("data is NULL\n");
		return -EINVAL;
	}
	if (param_head == NULL) {
		chg_err("param_head is NULL\n");
		return -EINVAL;
	}
	config = (struct oplus_comm_config *)data;

	g_log_buf = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	oplus_comm_update_ui_soc_config(param_head, config);
	oplus_comm_update_smooth_soc_config(param_head, config);

	if (g_log_buf != NULL) {
		kfree(g_log_buf);
		g_log_buf = NULL;
	}

	return 0;
}

static int oplus_comm_reg_debug_config(struct oplus_chg_comm *chip)
{
	int rc;

	chip->spec_debug_cfg.type = OPLUS_CHG_COMM_SPEC_PARAM;
	chip->spec_debug_cfg.update = oplus_comm_update_spec_config;
	chip->spec_debug_cfg.priv_data = &chip->spec;
	rc = oplus_cfg_register(&chip->spec_debug_cfg);
	if (rc < 0)
		chg_err("spec cfg register error, rc=%d\n", rc);

	chip->normal_debug_cfg.type = OPLUS_CHG_COMM_NORMAL_PARAM;
	chip->normal_debug_cfg.update = oplus_comm_update_normal_config;
	chip->normal_debug_cfg.priv_data = &chip->config;
	rc = oplus_cfg_register(&chip->normal_debug_cfg);
	if (rc < 0)
		chg_err("normal cfg register error, rc=%d\n", rc);

	return 0;
}

static void oplus_comm_unreg_debug_config(struct oplus_chg_comm *chip)
{
	oplus_cfg_unregister(&chip->spec_debug_cfg);
	oplus_cfg_unregister(&chip->normal_debug_cfg);
}
