// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */


#ifndef SM5602_FG_H
#define SM5602_FG_H

#define FG_INIT_MARK				0xA000

#define FG_PARAM_UNLOCK_CODE	  	0x3700
#define FG_PARAM_LOCK_CODE	  		0x0000
#define FG_TABLE_LEN				0x18/*real table length -1*/
#define FG_ADD_TABLE_LEN			0x8/*real table length -1*/
#define FG_INIT_B_LEN		    	0x7/*real table length -1*/

#define ENABLE_EN_TEMP_IN           0x0200
#define ENABLE_EN_TEMP_EX           0x0400
#define ENABLE_EN_BATT_DET          0x0800
#define ENABLE_IOCV_MAN_MODE        0x1000
#define ENABLE_FORCED_SLEEP         0x2000
#define ENABLE_SLEEPMODE_EN         0x4000
#define ENABLE_SHUTDOWN             0x8000

/* REG */
#define FG_REG_SOC_CYCLE			0x0B
#define FG_REG_SOC_CYCLE_CFG		0x15
#define FG_REG_ALPHA             	0x20
#define FG_REG_BETA              	0x21
#define FG_REG_RS                	0x24
#define FG_REG_RS_1     			0x25
#define FG_REG_RS_2            		0x26
#define FG_REG_RS_3            		0x27
#define FG_REG_RS_0            		0x29
#define FG_REG_END_V_IDX			0x2F
#define FG_REG_START_LB_V			0x30
#define FG_REG_START_CB_V			0x38
#define FG_REG_START_LB_I			0x40
#define FG_REG_START_CB_I			0x48
#define FG_REG_VOLT_CAL				0x70
#define FG_REG_CURR_IN_OFFSET		0x75
#define FG_REG_CURR_IN_SLOPE		0x76

#define FG_REG_SRADDR				0x8C
#define FG_REG_SRDATA				0x8D
#define FG_REG_SWADDR				0x8E
#define FG_REG_SWDATA				0x8F

#define FG_REG_AGING_CTRL			0x9C

#define FG_TEMP_TABLE_CNT_MAX       0x65

#define I2C_ERROR_COUNT_MAX			0x5

#define FG_PARAM_VERION       		0x1E

#define INIT_CHECK_MASK         	0x0010
#define DISABLE_RE_INIT         	0x0010

#define FG_REMOVE_IRQ 1

#define SM5602_DEFAULT_MVOLTS_2CELL_MAX			4000
#define SM5602_DEFAULT_MVOLTS_2CELL_MIN			3300
#define SM5602_FG_READ_ANYTHING_VAL				36
#define SM5602_DEFAULT_MVOLTS         3800
#define SM5602_DEFAULT_TEMP           250

int sm5602_driver_init(void);
void sm5602_driver_exit(void);

#endif /* SM5602_FG_H */

