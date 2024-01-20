// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#ifndef __OPLUS_UFCS_CLASS_H__
#define __OPLUS_UFCS_CLASS_H__

#include <linux/kernel.h>
#include <linux/device.h>

struct ufcs_class;

#include "internal/ufcs_notify.h"
#include "internal/ufcs_msg.h"

#define UFCS_OPLUS_DEV_ID	0x22d9

#define UFCS_SW_VERSION		0x1
#define UFCS_HW_VERSION		0x1
#define UFCS_VND_DEV_ID		UFCS_OPLUS_DEV_ID

#define UFCS_TEST_MODE_POWER_VOLT	5000
#define UFCS_TEST_MODE_POWER_CURR	1000
#define UFCS_TEST_MODE_WATCHDOG		3000

#define UFCS_TEST_MODE_IC_VND		0xffff
#define UFCS_TEST_MODE_DEV_VND		0xffff

enum ufcs_baud_rate {
	UFCS_BAUD_115200 = 0,
	UFCS_BAUD_57600,
	UFCS_BAUD_38400,
	UFCS_BAUD_19200,
	UFCS_BAUD_MAX,
};

enum ufcs_handshake_state {
	UFCS_HS_IDLE = 0,
	UFCS_HS_WAIT,
	UFCS_HS_SUCCESS,
	UFCS_HS_FAIL,
};

enum ufcs_err_type {
	UFCS_HW_ERR = 0,
	UFCS_HW_ERR_DP_OVP = UFCS_HW_ERR,
	UFCS_HW_ERR_DM_OVP,
	UFCS_HW_ERR_TEMP_SHUTDOWN,
	UFCS_HW_ERR_WTD_TIMEOUT,
	UFCS_HW_ERR_HARD_RESET,

	UFCS_RECV_ERR,
	UFCS_RECV_ERR_SENT_CMP = UFCS_RECV_ERR,
	UFCS_RECV_ERR_TRANS_FAIL,
	UFCS_RECV_ERR_ACK_TIMEOUT,
	UFCS_RECV_ERR_DATA_READY,

	UFCS_COMM_ERR,
	UFCS_COMM_ERR_BAUD_RATE_ERR = UFCS_COMM_ERR,
	UFCS_COMM_ERR_TRAINING_ERR,
	UFCS_COMM_ERR_START_FAIL,
	UFCS_COMM_ERR_BYTE_TIMEOUT,
	UFCS_COMM_ERR_RX_LEN_ERR,
	UFCS_COMM_ERR_RX_OVERFLOW,
	UFCS_COMM_ERR_CRC_ERR,
	UFCS_COMM_ERR_STOP_ERR,
	UFCS_COMM_ERR_BAUD_RATE_CHANGE,
	UFCS_COMM_ERR_BUS_CONFLICT,
};

struct ufcs_config {
	bool check_crc;
	bool reply_ack;
	bool msg_resend;
	bool handshake_hard_retry;
	u16 ic_vendor_id;
};

struct ufcs_dev_ops;

struct ufcs_dev {
	struct device dev;
	struct ufcs_dev_ops *ops;
	void *drv_data;
	struct ufcs_class *class;

	enum ufcs_handshake_state handshake_state;
	unsigned int dev_err_flag;
};

struct ufcs_dev_ops {
	int (*init)(struct ufcs_dev *ufcs);
	int (*write_msg)(struct ufcs_dev *ufcs, unsigned char *buf, int len);
	int (*read_msg)(struct ufcs_dev *ufcs, unsigned char *buf, int len);
	int (*handshake)(struct ufcs_dev *ufcs);
	int (*source_hard_reset)(struct ufcs_dev *ufcs);
	int (*cable_hard_reset)(struct ufcs_dev *ufcs);
	int (*set_baud_rate)(struct ufcs_dev *ufcs, enum ufcs_baud_rate baud);
	int (*enable)(struct ufcs_dev *ufcs);
	int (*disable)(struct ufcs_dev *ufcs);
};

#if IS_ENABLED(CONFIG_OPLUS_UFCS_CLASS)
struct ufcs_dev *__must_check
ufcs_device_register(struct device *parent, struct ufcs_dev_ops *ops,
	void *drv_data, struct ufcs_config *config);
void ufcs_device_unregister(struct ufcs_dev *ufcs);
struct ufcs_dev *ufcs_get_ufcs_device(void);

int ufcs_msg_handler(struct ufcs_dev *ufcs);
int ufcs_intf_pdo_set(struct ufcs_dev *ufcs, int vol_mv, int curr_ma);
int ufcs_handshake(struct ufcs_dev *ufcs);
int ufcs_source_hard_reset(struct ufcs_dev *ufcs);
int ufcs_cable_hard_reset(struct ufcs_dev *ufcs);
int ufcs_force_exit(struct ufcs_dev *ufcs);
int ufcs_intf_config_watchdog(struct ufcs_dev *ufcs, u16 time_ms);
void ufcs_clr_error_flag(struct ufcs_dev *ufcs);
int ufcs_intf_get_device_info(struct ufcs_dev *ufcs, u64 *dev_info);
int ufcs_intf_get_error_info(struct ufcs_dev *ufcs, u64 *err_info);
int ufcs_intf_get_source_info(struct ufcs_dev *ufcs, u64 *src_info);
int ufcs_intf_get_cable_info(struct ufcs_dev *ufcs, u64 *cable_info);
int ufcs_intf_get_pdo_info(struct ufcs_dev *ufcs, u64 *pdo, int num);
int ufcs_intf_verify_adapter(struct ufcs_dev *ufcs, u8 key_index, u8 *auth_data, u8 data_len);
int ufcs_intf_get_power_change_info(struct ufcs_dev *ufcs, u32 *pwr_change_info, int num);
int ufcs_intf_get_emark_info(struct ufcs_dev *ufcs, u64 *info);
int ufcs_intf_get_power_info_ext(struct ufcs_dev *ufcs, u64 *pie, int num);
bool ufcs_is_test_mode(struct ufcs_dev *ufcs);
bool ufcs_is_vol_acc_test_mode(struct ufcs_dev *ufcs);

#else /* CONFIG_OPLUS_UFCS_CLASS */

__maybe_unused
static inline struct ufcs_dev *__must_check
ufcs_device_register(struct device *parent, struct ufcs_dev_ops *ops,
	void *drv_data, struct ufcs_config *config)
{
	return NULL;
}

__maybe_unused
static inline void ufcs_device_unregister(struct ufcs_dev *ufcs)
{
	return;
}

__maybe_unused
static struct ufcs_dev *ufcs_get_ufcs_device(void)
{
	return NULL;
}

__maybe_unused
static inline int ufcs_msg_handler(struct ufcs_dev *ufcs)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_pdo_set(struct ufcs_dev *ufcs, int vol_mv, int curr_ma)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_handshake(struct ufcs_dev *ufcs)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_source_hard_reset(struct ufcs_dev *ufcs)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_cable_hard_reset(struct ufcs_dev *ufcs)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_force_exit(struct ufcs_dev *ufcs)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_config_watchdog(struct ufcs_dev *ufcs, u16 time_ms)
{
	return -EINVAL;
}

__maybe_unused
static inline void ufcs_clr_error_flag(struct ufcs_dev *ufcs)
{
	return;
}

__maybe_unused
static inline int ufcs_intf_get_device_info(struct ufcs_dev *ufcs, u64 *dev_info)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_error_info(struct ufcs_dev *ufcs, u64 *err_info)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_source_info(struct ufcs_dev *ufcs, u64 *src_info)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_cable_info(struct ufcs_dev *ufcs, u64 *cable_info)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_pdo_info(struct ufcs_dev *ufcs, u64 *pdo, int num)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_verify_adapter(struct ufcs_dev *ufcs, u8 key_index, u8 *auth_data, u8 data_len)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_power_change_info(struct ufcs_dev *ufcs, u32 *pwr_change_info, int num)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_emark_info(struct ufcs_dev *ufcs, u64 *info)
{
	return -EINVAL;
}

__maybe_unused
static inline int ufcs_intf_get_power_info_ext(struct ufcs_dev *ufcs, u64 *pie, int num)
{
	return -EINVAL;
}

__maybe_unused
static inline bool ufcs_is_test_mode(struct ufcs_dev *ufcs)
{
	return false;
}

__maybe_unused
static inline bool ufcs_is_vol_acc_test_mode(struct ufcs_dev *ufcs)
{
	return false;
}

#endif /* CONFIG_OPLUS_UFCS_CLASS */

#endif /* __OPLUS_UFCS_CLASS_H__ */
