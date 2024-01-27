/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef SYNAPTICS_H_S4322
#define SYNAPTICS_H_S4322

/*********PART1:Head files**********************/
#include <linux/i2c.h>
#ifdef CONFIG_FB
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#include "../synaptics_common.h"
#ifdef CONFIG_SYNAPTIC_RED
#include "../synaptics_touch_panel_remote.h"
#endif

/*********PART2:Define Area**********************/
#define ENABLE_UNICODE  0x40
#define ENABLE_VEE      0x20
#define ENABLE_CIRCLE   0x08
#define ENABLE_SWIPE    0x02
#define ENABLE_DTAP     0x01

#define UNICODE_DETECT  0x0b
#define VEE_DETECT      0x0a
#define CIRCLE_DETECT   0x08
#define SWIPE_DETECT    0x07
#define DTAP_DETECT     0x03

#define RESET_TO_NORMAL_TIME 80    /*Sleep time after reset*/

#define SPURIOUS_FP_LIMIT 100
#define SPURIOUS_FP_RX_NUM 8
#define SPURIOUS_FP_TX_NUM 9
#define SPURIOUS_FP_BASE_DATA_RETRY 10

#define I2C_ERROR_MAX_TIME 5

#define EXTEND_EE_SHORT_RESET_DUR  60
#define EXTEND_EE_SHORT_INT_DUR  150
#define EXTEND_EE_SHORT_TX_ON_COUNT  146
#define EXTEND_EE_SHORT_RX_ON_COUNT  146
#define EXTEND_EE_SHORT_TEST_LIMIT_PART1  160
#define EXTEND_EE_SHORT_TEST_LIMIT_PART2  80         // ( unit = ratio )

/* tddi f54 test reporting - */
#define ELEC_OPEN_TEST_TX_ON_COUNT 2
#define ELEC_OPEN_TEST_RX_ON_COUNT 2
#define ELEC_OPEN_INT_DUR_ONE 15
#define ELEC_OPEN_INT_DUR_TWO 25
#define ELEC_OPEN_TEST_LIMIT_ONE 500
#define ELEC_OPEN_TEST_LIMIT_TWO 50

#define COMMAND_FORCE_UPDATE 4

//#define UPDATE_DISPLAY_CONFIG

//Self Capacitance key test limite
#define MENU_LOW_LIMITE  1630
#define MENU_HIGH_LIMITE 3803

#define BACK_LOW_LIMITE 3016
#define BACK_HIGH_LIMITE 7039

#define TEST_FAIL	1
#define TEST_PASS	0

#define LIMIT_DOZE_LOW  50
#define LIMIT_DOZE_HIGH 975

struct f55_control_43 {
	union {
		struct {
			unsigned char swap_sensor_side:1;
			unsigned char f55_ctrl43_b1__7:7;
			unsigned char afe_l_mux_size:4;
			unsigned char afe_r_mux_size:4;
		} __packed;
		unsigned char data[2];
	};
};

struct f54_control_91 {
	union {
		struct {
			unsigned char reflo_transcap_capacitance;
			unsigned char refhi_transcap_capacitance;
			unsigned char receiver_feedback_capacitance;
			unsigned char reference_receiver_feedback_capacitance;
			unsigned char gain_ctrl;
		} __packed;
		struct {
			unsigned char data[5];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_99 {
	union {
		struct {
			unsigned char integration_duration_lsb;
			unsigned char integration_duration_msb;
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_182 {
	union {
		struct {
			unsigned char cbc_timing_ctrl_tx_lsb;
			unsigned char cbc_timing_ctrl_tx_msb;
			unsigned char cbc_timing_ctrl_rx_lsb;
			unsigned char cbc_timing_ctrl_rx_msb;
		} __packed;
		struct {
			unsigned char data[4];
			unsigned short address;
		} __packed;
	};
};

/*********PART3:Struct Area**********************/
struct synaptics_register
{
    uint8_t F01_RMI_QUERY_BASE;
    uint8_t F01_RMI_CMD_BASE;
    uint8_t F01_RMI_CTRL_BASE;
    uint8_t F01_RMI_DATA_BASE;

    uint8_t F01_RMI_QUERY11;
    uint8_t F01_RMI_DATA01;
    uint8_t F01_RMI_CMD00;
    uint8_t F01_RMI_CTRL00;
    uint8_t F01_RMI_CTRL01;
    uint8_t F01_RMI_CTRL02;

    uint8_t F12_2D_QUERY_BASE;
    uint8_t F12_2D_CMD_BASE;
    uint8_t F12_2D_CTRL_BASE;
    uint8_t F12_2D_DATA_BASE;

    uint8_t F12_2D_CTRL08;
    uint8_t F12_2D_CTRL20;
    uint8_t F12_2D_CTRL23;
    uint8_t F12_2D_CTRL27;
    uint8_t F12_2D_CTRL32;
    uint8_t F12_2D_DATA04;
    uint8_t F12_2D_DATA15;
    uint8_t F12_2D_DATA38;
    uint8_t F12_2D_DATA39;
    uint8_t F12_2D_CMD00;

    uint8_t F34_FLASH_QUERY_BASE;
    uint8_t F34_FLASH_CMD_BASE;
    uint8_t F34_FLASH_CTRL_BASE;
    uint8_t F34_FLASH_DATA_BASE;

    uint8_t SynaF34_FlashControl;
    uint8_t SynaF34_FlashStatus;
    uint8_t SynaF34Reflash_BlockNum;
    uint8_t SynaF34Reflash_BlockData;
    uint8_t SynaF34ReflashQuery_BootID;
    uint8_t SynaF34ReflashQuery_FlashPropertyQuery;
    uint8_t SynaF34ReflashQuery_FirmwareBlockSize;
    uint8_t SynaF34ReflashQuery_FirmwareBlockCount;
    uint8_t SynaF34ReflashQuery_ConfigBlockSize;
    uint8_t SynaF34ReflashQuery_ConfigBlockCount;

    uint8_t F51_CUSTOM_QUERY_BASE;
    uint8_t F51_CUSTOM_CMD_BASE;
    uint8_t F51_CUSTOM_CTRL_BASE;
    uint8_t F51_CUSTOM_DATA_BASE;

    uint8_t F51_CUSTOM_CTRL00;
    uint8_t F51_CUSTOM_CTRL31;
    uint8_t F51_CUSTOM_CTRL42;
    uint8_t F51_CUSTOM_CTRL48;   //ESD Mode Ctrl, bit2:1 ps near , bit2:0 ps far
    uint8_t F51_CUSTOM_GRIP_CTRL;//edge function register,the enable bit of Grip suppression
    uint8_t F51_CUSTOM_DATA;

    uint8_t F54_ANALOG_QUERY_BASE;
    uint8_t F54_ANALOG_COMMAND_BASE;
    uint8_t F54_ANALOG_CONTROL_BASE;
    uint8_t F54_ANALOG_DATA_BASE;
    uint8_t F54_ANALOG_CTRL123;
    uint8_t F54_ANALOG_CTRL91;
    uint8_t F54_ANALOG_CTRL99;
    uint8_t F54_ANALOG_CTRL182;
    uint8_t F54_ANALOG_CTRL225;

    uint8_t F55_SENSOR_QUERY_BASE;
    uint8_t F55_SENSOR_CMD_BASE;
    uint8_t F55_SENSOR_CTRL_BASE;
    uint8_t F55_SENSOR_DATA_BASE;

    uint8_t F55_SENSOR_CTRL01;
    uint8_t F55_SENSOR_CTRL02;
    uint8_t F55_SENSOR_CTRL43;
};

struct chip_data_s4322 {
    uint32_t *p_tp_fw;
    tp_dev tp_type;
    struct i2c_client *client;
    struct synaptics_proc_operations *syna_ops; /*synaptics func provide to synaptics common driver*/
#ifdef CONFIG_SYNAPTIC_RED
    int    enable_remote;                       /*Redremote connect state*/
    struct remotepanel_data *premote_data;
#endif/*CONFIG_SYNAPTIC_RED*/
    struct synaptics_register     reg_info;
    struct hw_resource          *hw_res;
    int16_t *spuri_fp_data;
    struct spurious_fp_touch *p_spuri_fp_touch;
};

#endif
