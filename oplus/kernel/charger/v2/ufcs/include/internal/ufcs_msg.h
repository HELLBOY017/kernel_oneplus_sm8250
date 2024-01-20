// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_MSG_H__
#define __OPLUS_UFCS_MSG_H__

#include <linux/kernel.h>

#define UFCS_MSG_SIZE_MAX		64
#define UFCS_DATA_MSG_DATA_SIZE_MAX	59
#define UFCS_VENDOR_MSG_DATA_SIZE_MAX	58
#define UFCS_MSG_MAGIC			0x10666

enum ufcs_msg_type {
	UFCS_CTRL_MSG = 0,
	UFCS_DATA_MSG,
	UFCS_VENDOR_MSG,
	UFCS_INVALID_MSG,
};

enum ufcs_device_addr {
	UFCS_DEVICE_SOURCE = 1,
	UFCS_DEVICE_SINK,
	UFCS_DEVICE_CABLE,
	UFCS_DEVICE_INVALID,
};

enum ufcs_version {
	UFCS_VER_1_0 = 1,
	UFCS_VER_CURR = UFCS_VER_1_0,
	UFCS_VER_INVALID,
};

struct ufcs_msg_head {
	u8 type;
	u8 version;
	u8 index;
	u8 addr;
} __attribute__((packed));

enum ufcs_ctrl_msg_type {
	CTRL_MSG_PING = 0x00,
	CTRL_MSG_ACK,
	CTRL_MSG_NCK,
	CTRL_MSG_ACCEPT,
	CTRL_MSG_SOFT_RESET,
	CTRL_MSG_POWER_READY,
	CTRL_MSG_GET_OUTPUT_CAPABILITIES,
	CTRL_MSG_GET_SOURCE_INFO,
	CTRL_MSG_GET_SINK_INFO,
	CTRL_MSG_GET_CABLE_INFO,
	CTRL_MSG_GET_DEVICE_INFO,
	CTRL_MSG_GET_ERROR_INFO,
	CTRL_MSG_DETECT_CABLE_INFO,
	CTRL_MSG_START_CABLE_DETECT,
	CTRL_MSG_END_CABLE_DETECT,
	CTRL_MSG_EXIT_UFCS_MODE,
};

struct ufcs_ctrl_msg {
	u8 command;
} __attribute__((packed));

enum ufcs_data_msg_type {
	DATA_MSG_OUTPUT_CAPABILITIES = 0x01,
	DATA_MSG_REQUEST,
	DATA_MSG_SOURCE_INFO,
	DATA_MSG_SINK_INFO,
	DATA_MSG_CABLE_INFO,
	DATA_MSG_DEVICE_INFO,
	DATA_MSG_ERROR_INFO,
	DATA_MSG_CONFIG_WATCHDOG,
	DATA_MSG_REFUSE,
	DATA_MSG_VERIFY_REQUEST,
	DATA_MSG_VERIFY_RESPONSE,
	DATA_MSG_POWER_CHANGE,
	DATA_MSG_TEST_REQUEST = 0xff,
};

#define UFCS_OUTPUT_MODE_MAX			7
#define UFCS_OUTPUT_MODE_INDEX(mode)		(((mode) >> 60) & 0xf)
#define UFCS_OUTPUT_MODE_CURR_STEP(mode)	(((((mode) >> 57) & 0x7) + 1) * 10)
#define UFCS_OUTPUT_MODE_VOL_STEP(mode)		(((((mode) >> 56) & 0x1) + 1) * 10)
#define UFCS_OUTPUT_MODE_VOL_MAX(mode)		((((mode) >> 40) & 0xffff) * 10)
#define UFCS_OUTPUT_MODE_VOL_MIN(mode)		((((mode) >> 24) & 0xffff) * 10)
#define UFCS_OUTPUT_MODE_CURR_MAX(mode)		((((mode) >> 8) & 0xffff) * 10)
#define UFCS_OUTPUT_MODE_CURR_MIN(mode)		(((mode) & 0xff) * 10)
#define UFCS_OUTPUT_MODE_DATA(index, curr_step, vol_step, vol_max, vol_min, curr_max, curr_min)		\
	((((u64)(index) & 0xf) << 60) | ((((u64)(curr_step) / 10 - 1) & 0x7) << 57) |			\
	 ((((u64)(vol_step) / 10 - 1) & 0x1) << 56) | ((((u64)(vol_max) / 10) & 0xffff) << 40) |	\
	 ((((u64)(vol_min) / 10) & 0xffff) << 24) | ((((u64)(curr_max) / 10) & 0xffff) << 8) |		\
	 (((u64)(curr_min) / 10) & 0xff))
struct ufcs_data_msg_output_capabilities {
	u64 output_mode[UFCS_OUTPUT_MODE_MAX];
} __attribute__((packed));

#define UFCS_REQUEST_OUTPUT_MODE_INDEX(req)	(((req) >> 60) & 0xf)
#define UFCS_REQUEST_RESERVE(req)		(((req) >> 32) & 0xfffffff)
#define UFCS_REQUEST_VOL(req)			((((req) >> 16) & 0xffff) * 10)
#define UFCS_REQUEST_CURR(req)			(((req) & 0xffff) * 10)
#define UFCS_REQUEST_DATA(index, vol, curr)					\
	((((u64)(index) & 0xf) << 60) | ((((u64)(vol) / 10) & 0xffff) << 16) |	\
	 (((u64)(curr) / 10) & 0xffff))
struct ufcs_data_msg_request {
	u64 request;
} __attribute__((packed));

#define UFCS_SOURCE_INFO_RESERVE(info)		(((info) >> 48) & 0xffff)
#define UFCS_SOURCE_INFO_DEV_TEMP(info)		((int)(((info) >> 40) & 0xff) - 50)
#define UFCS_SOURCE_INFO_USB_TEMP(info)		((int)(((info) >> 32) & 0xff) - 50)
#define UFCS_SOURCE_INFO_VOL(info)		((((info) >> 16) & 0xffff) * 10)
#define UFCS_SOURCE_INFO_CURR(info)		(((info) & 0xffff) * 10)
#define UFCS_SOURCE_INFO_INVALID_TEMP		(-50)
#define UFCS_SOURCE_INFO_DATA(dev_temp, usb_temp, vol, curr)	\
	(((((u64)(dev_temp + 50)) & 0xff) << 40) |		\
	 ((((u64)(usb_temp + 50)) & 0xff) << 32) |		\
	 ((((u64)(vol) / 10) & 0xffff) << 16) |			\
	 (((u64)(curr) / 10) & 0xffff))
struct ufcs_data_msg_source_info {
	u64 info;
} __attribute__((packed));

#define UFCS_SINK_INFO_RESERVE(info)		(((info) >> 48) & 0xffff)
#define UFCS_SINK_INFO_TEMP(info)		((int)(((info) >> 40) & 0xff) - 50)
#define UFCS_SINK_INFO_USB_TEMP(info)		((int)(((info) >> 32) & 0xff) - 50)
#define UFCS_SINK_INFO_VOL(info)		((((info) >> 16) & 0xffff) * 10)
#define UFCS_SINK_INFO_CURR(info)		(((info) & 0xffff) * 10)
#define UFCS_SINK_INFO_INVALID_TEMP		(-50)
#define UFCS_SINK_INFO_DATA(batt_temp, usb_temp, vol, curr)	\
	(((((u64)(batt_temp + 50)) & 0xff) << 40) |		\
	 ((((u64)(usb_temp + 50)) & 0xff) << 32) |		\
	 ((((u64)(vol) / 10) & 0xffff) << 16) |			\
	 (((u64)(curr) / 10) & 0xffff))
struct ufcs_data_msg_sink_info {
	u64 info;
} __attribute__((packed));

#define UFCS_CABLE_INFO_RESERVE0(info)		(((info) >> 48) & 0xffff)
#define UFCS_CABLE_INFO_RESERVE1(info)		(((info) >> 32) & 0xffff)
#define UFCS_CABLE_INFO_IMP(info)		(((info) >> 16) & 0xffff)	/* mΩ */
#define UFCS_CABLE_INFO_VOL_MAX(info)		((((info) >> 8) & 0xff) * 1000) /* mV */
#define UFCS_CABLE_INFO_CURR_MAX(info)		(((info) & 0xff) * 1000)	/* mA */
#define UFCS_CABLE_INFO_DATA(imp, vol_max, curr_max)					\
	((((u64)(imp) & 0xffff) << 16) | (((u64)(vol_max / 1000) & 0xff) << 8) |	\
	 ((u64)(curr_max / 1000) & 0xff))
struct ufcs_data_msg_cable_info {
	u64 info;
} __attribute__((packed));

#define UFCS_DEVICE_INFO_DEV_VENDOR(info)	(((info) >> 48) & 0xffff)
#define UFCS_DEVICE_INFO_IC_VENDOR(info)	(((info) >> 32) & 0xffff)
#define UFCS_DEVICE_INFO_HW_VER(info)		(((info) >> 16) & 0xffff)
#define UFCS_DEVICE_INFO_SW_VER(info)		((info) & 0xffff)
#define UFCS_DEVICE_INFO_DATA(dev_vnd, ic_vnd, hw_ver, sw_ver)			\
	((((u64)(dev_vnd) & 0xffff) << 48) | (((u64)(ic_vnd) & 0xffff) << 32) |	\
	 (((u64)(hw_ver) & 0xffff) << 16) | ((u64)(sw_ver) & 0xffff))
struct ufcs_data_msg_device_info {
	u64 info;
} __attribute__((packed));

#define UFCS_ERROR_INFO_RESERVE(info)		(((info) >> 15) & 0x1ff)
#define UFCS_ERROR_INFO_WATCHDOG(info)		(((info) >> 14) & 0x1)
#define UFCS_ERROR_INFO_CRC_ERR(info)		(((info) >> 13) & 0x1)
#define UFCS_ERROR_INFO_INPUT_ONOFF(info)	(((info) >> 12) & 0x1)
#define UFCS_ERROR_INFO_LEAKAGE(info)		(((info) >> 11) & 0x1)
#define UFCS_ERROR_INFO_INPUT_UVP(info)		(((info) >> 10) & 0x1)
#define UFCS_ERROR_INFO_INPUT_OVP(info)		(((info) >> 9) & 0x1)
#define UFCS_ERROR_INFO_DP_OVP(info)		(((info) >> 8) & 0x1)
#define UFCS_ERROR_INFO_DM_OVP(info)		(((info) >> 7) & 0x1)
#define UFCS_ERROR_INFO_CC_OVP(info)		(((info) >> 6) & 0x1)
#define UFCS_ERROR_INFO_BATT_OTP(info)		(((info) >> 5) & 0x1)
#define UFCS_ERROR_INFO_DEV_OTP(info)		(((info) >> 5) & 0x1)
#define UFCS_ERROR_INFO_USB_OTP(info)		(((info) >> 4) & 0x1)
#define UFCS_ERROR_INFO_OUTPUT_SCP(info)	(((info) >> 3) & 0x1)
#define UFCS_ERROR_INFO_OUTPUT_OCP(info)	(((info) >> 2) & 0x1)
#define UFCS_ERROR_INFO_OUTPUT_UVP(info)	(((info) >> 1) & 0x1)
#define UFCS_ERROR_INFO_OUTPUT_OVP(info)	((info) & 0x1)

#define UFCS_ERROR_INFO_WATCHDOG_BIT		14
#define UFCS_ERROR_INFO_CRC_ERR_BIT		13
#define UFCS_ERROR_INFO_INPUT_ONOFF_BIT		12
#define UFCS_ERROR_INFO_LEAKAGE_BIT		11
#define UFCS_ERROR_INFO_INPUT_UVP_BIT		10
#define UFCS_ERROR_INFO_INPUT_OVP_BIT		9
#define UFCS_ERROR_INFO_DP_OVP_BIT		8
#define UFCS_ERROR_INFO_DM_OVP_BIT		7
#define UFCS_ERROR_INFO_CC_OVP_BIT		6
#define UFCS_ERROR_INFO_BATT_OTP_BIT		5
#define UFCS_ERROR_INFO_DEV_OTP_BIT		5
#define UFCS_ERROR_INFO_USB_OTP_BIT		4
#define UFCS_ERROR_INFO_OUTPUT_SCP_BIT		3
#define UFCS_ERROR_INFO_OUTPUT_OCP_BIT		2
#define UFCS_ERROR_INFO_OUTPUT_UVP_BIT		1
#define UFCS_ERROR_INFO_OUTPUT_OVP_BIT		0

#define UFCS_ERROR_INFO_RESERVE_MASK		0x1ff
#define UFCS_ERROR_INFO_WATCHDOG_MASK		BIT(UFCS_ERROR_INFO_WATCHDOG_BIT)
#define UFCS_ERROR_INFO_CRC_ERR_MASK		BIT(UFCS_ERROR_INFO_CRC_ERR_BIT)
#define UFCS_ERROR_INFO_INPUT_ONOFF_MASK	BIT(UFCS_ERROR_INFO_INPUT_ONOFF_BIT)
#define UFCS_ERROR_INFO_LEAKAGE_MASK		BIT(UFCS_ERROR_INFO_LEAKAGE_BIT)
#define UFCS_ERROR_INFO_INPUT_UVP_MASK		BIT(UFCS_ERROR_INFO_INPUT_UVP_BIT)
#define UFCS_ERROR_INFO_INPUT_OVP_MASK		BIT(UFCS_ERROR_INFO_INPUT_OVP_BIT)
#define UFCS_ERROR_INFO_DP_OVP_MASK		BIT(UFCS_ERROR_INFO_DP_OVP_BIT)
#define UFCS_ERROR_INFO_DM_OVP_MASK		BIT(UFCS_ERROR_INFO_DM_OVP_BIT)
#define UFCS_ERROR_INFO_CC_OVP_MASK		BIT(UFCS_ERROR_INFO_CC_OVP_BIT)
#define UFCS_ERROR_INFO_BATT_OTP_MASK		BIT(UFCS_ERROR_INFO_BATT_OTP_BIT)
#define UFCS_ERROR_INFO_DEV_OTP_MASK		BIT(UFCS_ERROR_INFO_DEV_OTP_BIT)
#define UFCS_ERROR_INFO_USB_OTP_MASK		BIT(UFCS_ERROR_INFO_USB_OTP_BIT)
#define UFCS_ERROR_INFO_OUTPUT_SCP_MASK		BIT(UFCS_ERROR_INFO_OUTPUT_SCP_BIT)
#define UFCS_ERROR_INFO_OUTPUT_OCP_MASK		BIT(UFCS_ERROR_INFO_OUTPUT_OCP_BIT)
#define UFCS_ERROR_INFO_OUTPUT_UVP_MASK		BIT(UFCS_ERROR_INFO_OUTPUT_UVP_BIT)
#define UFCS_ERROR_INFO_OUTPUT_OVP_MASK		BIT(UFCS_ERROR_INFO_OUTPUT_OVP_BIT)
struct ufcs_data_msg_error_info {
	u32 info;
} __attribute__((packed));

struct ufcs_data_msg_config_watchdog {
	u16 over_time_ms;
} __attribute__((packed));

#define UFCS_REFUSE_INFO_RESERVE0(data)		(((data) >> 28) & 0xf)
#define UFCS_REFUSE_INFO_MSG_INDEX(data)	(((data) >> 24) & 0xf)
#define UFCS_REFUSE_INFO_RESERVE1(data)		(((data) >> 19) & 0x1f)
#define UFCS_REFUSE_INFO_MSG_TYPE(data)		(((data) >> 16) & 0x7)
#define UFCS_REFUSE_INFO_MSG_CMD(data)		(((data) >> 8) & 0xf)
#define UFCS_REFUSE_INFO_REASON(data)		((data) & 0xf)
#define UFCS_REFUSE_INFO_DATA(msg_index, msg_type, msg_cmd, reason)		\
	((((u32)(msg_index) & 0xf) << 24) | (((u32)(msg_type) & 0x7) << 16) |	\
	 (((u32)(msg_cmd) & 0xf) << 8) | ((u32)(reason) & 0xf))

enum ufcs_refuse_reason {
	REFUSE_UNKNOWN_CMD = 0x1,
	REFUSE_NOT_SUPPORT_CMD,
	REFUSE_DEVICE_BUSY,
	REFUSE_OUT_OF_RANGE,
	REFUSE_OTHER,
};

struct ufcs_data_msg_refuse {
	u32 data;
} __attribute__((packed));

#define UFCS_VERIFY_RANDOM_DATA_SIZE		16
#define UFCS_VERIFY_ENCRYPTED_DATA_SIZE		32
#define UFCS_VERIFY_AUTH_DATA_SIZE		16
struct ufcs_data_msg_verify_request {
	u8 index;
	u8 random_data[UFCS_VERIFY_RANDOM_DATA_SIZE];
} __attribute__((packed));

struct ufcs_data_msg_verify_response {
	u8 encrypted_data[UFCS_VERIFY_ENCRYPTED_DATA_SIZE];
	u8 random_data[UFCS_VERIFY_RANDOM_DATA_SIZE];
} __attribute__((packed));

#define UFCS_POWER_CHANGE_INDEX(data)		(((data) >> 20) & 0xf)
#define UFCS_POWER_CHANGE_RESERVE(data)		(((data) >> 16) & 0xf)
#define UFCS_POWER_CHANGE_CURR_MAX(data)	(((data) & 0xffff) * 10)
#define UFCS_POWER_CHANGE_DATA(index, curr_max)	\
	((((u32)(index) & 0xf) << 20) | ((u32)(curr_max) & 0xffff))
struct ufcs_data_msg_power_change {
	u32 data[UFCS_OUTPUT_MODE_MAX];
} __attribute__((packed));

#define UFCS_TEST_REQUEST_ENABLE_TEST_MODE(data)		(((data) >> 15) & 0x1)
#define UFCS_TEST_REQUEST_ENABLE_VOL_ACC_TEST_MODE(data)	(((data) >> 14) & 0x1)
#define UFCS_TEST_REQUEST_DEV_ADDR(data)			(((data) >> 11) & 0x7)
#define UFCS_TEST_REQUEST_MSG_TYPE(data)			(((data) >> 8) & 0x7)
#define UFCS_TEST_REQUEST_MSG_CMD(data)				((data) & 0xf)
#define UFCS_TEST_REQUEST_DATA(test_mode, vol_acc_test_mode, dev_addr, msg_type, msg_cmd)		\
	((((u16)(test_mode) & 0x1) << 15) | (((u16)(vol_acc_test_mode) & 0x1) << 14) |			\
	 (((u16)(dev_addr) & 0x7) << 11) | (((u16)(msg_type) & 0x7) << 8) | ((u16)(msg_cmd) & 0xf))
#define UFCS_IS_TEST_CTRL_MSG(data) ({		\
	(((data) & 0x3fff) == 0x3fff) ? 1 : 0;	\
})

struct ufcs_data_msg_test_request {
	u16 data;
} __attribute__((packed));

struct ufcs_data_msg {
	u8 command;
	u8 length;
	union {
		u8 data[UFCS_DATA_MSG_DATA_SIZE_MAX];
		struct ufcs_data_msg_output_capabilities output_cap;
		struct ufcs_data_msg_request request;
		struct ufcs_data_msg_source_info src_info;
		struct ufcs_data_msg_sink_info sink_info;
		struct ufcs_data_msg_cable_info cable_info;
		struct ufcs_data_msg_device_info dev_info;
		struct ufcs_data_msg_error_info err_info;
		struct ufcs_data_msg_config_watchdog config_wd;
		struct ufcs_data_msg_refuse refuse;
		struct ufcs_data_msg_verify_request verify_request;
		struct ufcs_data_msg_verify_response verify_response;
		struct ufcs_data_msg_power_change power_change;
		struct ufcs_data_msg_test_request test_request;
	};
} __attribute__((packed));

/* vendor msg */

enum ufcs_oplus_vnd_msg_type {
	OPLUS_MSG_GET_EMARK_INFO = 0x01,
	OPLUS_MSG_RESP_EMARK_INFO,
	OPLUS_MSG_GET_POWER_INFO_EXT,
	OPLUS_MSG_RESP_POWER_INFO_EXT,
};

enum ufcs_oplus_vnd_emark_info {
	UFCS_OPLUS_EMARK_INFO_ERR = 0,
	UFCS_OPLUS_EMARK_INFO_OTHER_CABLE,
	UFCS_OPLUS_EMARK_INFO_OPLUS_CABLE,
	UFCS_OPLUS_EMARK_INFO_OTHER,
};

#define UFCS_OPLUS_VND_EMARK_RESERVE(data)	(((data) >> 42) & 0xfffc)
#define UFCS_OPLUS_VND_EMARK_INFO(data)		(((data) >> 40) & 0x3)
#define UFCS_OPLUS_VND_EMARK_HW_VER(data)	(((data) >> 32) & 0xff)
#define UFCS_OPLUS_VND_EMARK_VOL_MAX(data)	((((data) >> 16) & 0xffff) * 10)
#define UFCS_OPLUS_VND_EMARK_CURR_MAX(data)	(((data) & 0xffff) * 10)
#define UFCS_OPLUS_VND_EMARK_DATA(info, hw_ver, vol_max, curr_max)			\
	((((u64)(info) & 0x3) << 40) | (((u64)(hw_ver) & 0xff) << 32) |			\
	 (((u64)(vol_max / 10) & 0xffff) << 16) | ((u64)(curr_max / 10) & 0xffff))
struct ufcs_oplus_data_msg_resp_emark_info {
	u64 data;
};

enum ufcs_oplus_vnd_power_info_type {
	UFCS_OPLUS_POWER_INFO_UFCS = 0,
	UFCS_OPLUS_POWER_INFO_PPS,
	UFCS_OPLUS_POWER_INFO_SVOOC
};

#define UFCS_OPLUS_VND_POWER_INFO_MAX	7

#define UFCS_OPLUS_VND_POWER_INFO_RESERVE(data)		(((data) >> 40) & 0x3fffff)
#define UFCS_OPLUS_VND_POWER_INFO_INDEX(data)		(((data) >> 35) & 0x1f)
#define UFCS_OPLUS_VND_POWER_INFO_TYPE(data)		(((data) >> 32) & 0x7)
#define UFCS_OPLUS_VND_POWER_INFO_VOL_MAX(data)		((((data) >> 16) & 0xffff) * 10)
#define UFCS_OPLUS_VND_POWER_INFO_CURR_MAX(data)	(((data) & 0xffff) * 10)
#define UFCS_OPLUS_VND_POWER_INFO_DATA(index, type, vol_max, curr_max)	\
	((((u64)(index) & 0x1f) << 35) |				\
	 (((u64)(type) & 0x7) << 32) |					\
	 (((u64)(vol_max / 10) & 0xffff) << 16) |			\
	 ((u64)(curr_max / 10) & 0xffff))

struct ufcs_oplus_data_msg_resp_power_info_ext {
	u64 data[UFCS_OPLUS_VND_POWER_INFO_MAX];
} __attribute__((packed));

struct ufcs_oplus_vnd_data_msg {
	union {
		struct ufcs_oplus_data_msg_resp_emark_info emark_info;
		struct ufcs_oplus_data_msg_resp_power_info_ext power_info;
	};
} __attribute__((packed));

struct ufcs_oplus_vnd_msg {
	u8 type;
	struct ufcs_oplus_vnd_data_msg data_msg;
} __attribute__((packed));

struct ufcs_vendor_msg {
	u16 vnd_id;
	u8 length;
	union {
		u8 data[UFCS_VENDOR_MSG_DATA_SIZE_MAX];
		struct ufcs_oplus_vnd_msg vnd_msg;
	};
} __attribute__((packed));

struct ufcs_msg {
	u32 magic;
	struct ufcs_msg_head head;
	union {
		struct ufcs_ctrl_msg ctrl_msg;
		struct ufcs_data_msg data_msg;
		struct ufcs_vendor_msg vendor_msg;
	};
	u8 crc;
	u8 crc_good;
} __attribute__((packed));

#define MSG_RETRY_COUNT_MAX	3
#define MSG_NUMBER_COUNT_MAX	15

#if IS_ENABLED(CONFIG_OPLUS_UFCS_CLASS)
const char *ufcs_get_oplus_emark_info_str(enum ufcs_oplus_vnd_emark_info info);
const char *ufcs_get_oplus_power_info_type_str(enum ufcs_oplus_vnd_power_info_type type);
#else

__maybe_unused
static inline const char *ufcs_get_oplus_emark_info_str(enum ufcs_oplus_vnd_emark_info info)
{
	return "Invalid";
}

__maybe_unused
static inline const char *ufcs_get_oplus_power_info_type_str(enum ufcs_oplus_vnd_power_info_type type)
{
	return "Invalid";
}

#endif /* CONFIG_OPLUS_UFCS_CLASS */

#endif /* __OPLUS_UFCS_MSG_H__ */
