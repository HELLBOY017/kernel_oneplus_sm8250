// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_INTF_H__
#define __OPLUS_UFCS_INTF_H__

#include "ufcs_core.h"

struct ufcs_class;

int ufcs_pdo_set(struct ufcs_class *class, int vol_mv, int curr_ma);
int ufcs_config_watchdog(struct ufcs_class *class, u16 time_ms);
int ufcs_get_device_info(struct ufcs_class *class, u64 *dev_info);
int ufcs_get_error_info(struct ufcs_class *class, u64 *err_info);
int ufcs_get_source_info(struct ufcs_class *class, u64 *src_info);
int ufcs_get_cable_info(struct ufcs_class *class, u64 *cable_info);
int ufcs_get_pdo_info(struct ufcs_class *class, u64 *pdo, int num);
int ufcs_verify_adapter(struct ufcs_class *class, u8 key_index, u8 *auth_data, u8 data_len);
int ufcs_get_power_change_info(struct ufcs_class *class, u32 *pwr_change_info, int num);
int ufcs_get_emark_info(struct ufcs_class *class, u64 *info);
int ufcs_get_power_info_ext(struct ufcs_class *class, u64 *pie, int num);

#endif /* __OPLUS_UFCS_INTF_H__ */
