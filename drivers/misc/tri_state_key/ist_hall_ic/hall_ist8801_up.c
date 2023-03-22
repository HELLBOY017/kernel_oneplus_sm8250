// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 * File: oplus_tri_key.c
 *
 * Description:
 *      Definitions for m1120 tri_state_key data process.
 *
 * Version: 1.0
 */
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include "hall_ist8801.h"
#include "../oplus_tri_key.h"

#define IST8801_I2C_BUF_SIZE				(17)

#define HALL_IST8801_UP "oplus,hall-ist8801,up"
#define TRI_KEY_TAG                  "[tri_state_key] "
#define TRI_KEY_ERR(fmt, args...)\
	pr_info(TRI_KEY_TAG" %s : "fmt, __func__, ##args)
#define TRI_KEY_LOG(fmt, args...)\
	pr_info(TRI_KEY_TAG" %s : "fmt, __func__, ##args)

static struct ist8801_data_t *g_ist8801_data;

static struct hall_srs ist8801_ranges_1[] = {
	{"40mT", GAIN_2_TIME, false, 0},
	{"35mT", GAIN_2_TIME, false, 10},
	{"20mT", GAIN_2_TIME, false, 28},
	{"15mT", GAIN_4_TIME, false, 17},
	{"10mT", GAIN_8_TIME, false, 0},
};

static struct hall_srs ist8801_ranges_2[] = {
	{"40mT", GAIN_2_TIME, false, 0},
	{"35mT", GAIN_2_TIME, false, 6},
	{"20mT", GAIN_2_TIME, false, 24},
	{"15mT", GAIN_4_TIME, false, 13},
	{"10mT", GAIN_8_TIME, false, 0},
};


static DEFINE_MUTEX(ist8801_i2c_mutex);
__attribute__((weak)) void ist8801_reconfig(struct ist8801_data_t *chip) { return; }

static int ist8801_i2c_read_block(struct ist8801_data_t *ist8801_data,
		u8 addr, u8 *data, u8 len)
{
	u8 reg_addr = addr;
	int err = 0;
	struct i2c_client *client = ist8801_data->client;
	struct i2c_msg msgs[2] = {{0}, {0}};

	if (!client) {
		TRI_KEY_ERR("client null\n");
		return -EINVAL;
	} else if (len >= IST8801_I2C_BUF_SIZE) {
		TRI_KEY_ERR(" length %d exceeds %d\n",
				len, IST8801_I2C_BUF_SIZE);
		return -EINVAL;
	}
	mutex_lock(&ist8801_i2c_mutex);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg_addr;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	err = i2c_transfer(client->adapter, msgs,
				ARRAY_SIZE(msgs));

	if (err < 0) {
		TRI_KEY_ERR("i2c_transfer error: (%d %p %d) %d\n",
				addr, data, len, err);
		err = -EIO;
	} else
		err = 0;
	mutex_unlock(&ist8801_i2c_mutex);

	return err;
}

static int ist8801_i2c_write_block(struct ist8801_data_t *ist8801_data,
			u8 addr, u8 *data, u8 len)
{
	int err = 0;
	int idx = 0;
	int num = 0;
	char buf[IST8801_I2C_BUF_SIZE] = {0};
	struct i2c_client *client = ist8801_data->client;

	if (!client) {
		TRI_KEY_ERR("client null\n");
		return -EINVAL;
	} else if (len >= IST8801_I2C_BUF_SIZE) {
		TRI_KEY_ERR(" length %d exceeds %d\n",
				len, IST8801_I2C_BUF_SIZE);
		return -EINVAL;
	}

	mutex_lock(&ist8801_i2c_mutex);

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0)
		TRI_KEY_ERR("send command error!! %d\n", err);

	if (len == 1) {
		switch (addr) {
		case IST8801_REG_PERSINT:
			ist8801_data->reg.map.persint = data[0];
			break;
		case IST8801_REG_INTSRS:
			ist8801_data->reg.map.intsrs = data[0];
			break;
		case IST8801_REG_LTHL:
			ist8801_data->reg.map.lthl = data[0];
			break;
		case IST8801_REG_LTHH:
			ist8801_data->reg.map.lthh = data[0];
			break;
		case IST8801_REG_HTHL:
			ist8801_data->reg.map.hthl = data[0];
			break;
		case IST8801_REG_HTHH:
			ist8801_data->reg.map.hthh = data[0];
			break;
		case IST8801_REG_I2CDIS:
			ist8801_data->reg.map.i2cdis = data[0];
			break;
		case IST8801_REG_SRST:
			ist8801_data->reg.map.srst = data[0];
			break;
		case IST8801_REG_OPF:
			ist8801_data->reg.map.opf = data[0];
			break;
		}
	}

	mutex_unlock(&ist8801_i2c_mutex);
	return err;
}

static void ist8801_short_to_2byte(struct ist8801_data_t *ist8801_data,
			short x, u8 *hbyte, u8 *lbyte)
{
	unsigned short temp;

	if (x >= 0)
		temp  = x;
	else
		temp = 65536 + x;

	*lbyte = temp & 0x00ff;
	*hbyte = (temp & 0xff00) >> 8;
}

static short ist8801_2byte_to_short(struct ist8801_data_t *ist8801_data,
				u8 hbyte, u8 lbyte)
{
	short x = 0;

	x = (short) ((hbyte << 8) | lbyte);

	return x;
}
#if ENABLE_FILTER
static void moving_average_1(u8 *data_hi, u8 *data_lo, u8 mode)
{
	static int first_1;
	int x, y;
	static int temp_1;

	x = 0;
	y = 0;
	x = (int) ist8801_2byte_to_short(NULL, *data_hi, *data_lo);

	if (!first_1) {
		if (mode == 0) {
			y = x;
			temp_1 = 4*x;
		} else {
			y = x;
			temp_1 = 2*x;
		}
	} else {
		if (mode == 0) {
			temp_1 = (temp_1>>2) + 3*x;
			y = temp_1 >> 2;
		} else {
			temp_1 = 2*x + temp_1;
			y = temp_1 >> 2;
			temp_1 =  temp_1 >> 1;
		}
	}

	first_1 = 1;

	if (y > 32767)
		y = 32767;
	else if (y <= -32768)
		y = -32768;

	ist8801_short_to_2byte(NULL, (short) y, data_hi, data_lo);
}

#endif

static int ist8801_get_id(struct ist8801_data_t *ist8801_data)
{
	u8 data = 0;

	ist8801_i2c_read_block(ist8801_data, IST8801_REG_DID, &data, 1);
	TRI_KEY_LOG("id = 0x%x\n", data);

	return data;
}

static int ist8801_get_data(short *data)
{
	int err = 0;
	u8 buf[3] = {0};
	short value = 0;
	static short pre_value;

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	err = ist8801_i2c_read_block(g_ist8801_data,
			IST8801_REG_ST1, buf, sizeof(buf));
	if (err < 0) {
		TRI_KEY_LOG("fail %d\n", err);
		return err;
	}

	if ((buf[0] & 0x01) | (buf[0] & 0x4)) {
#if ENABLE_FILTER
		moving_average_1(&buf[2], &buf[1], FILTER_MODE);
#endif

		value = ist8801_2byte_to_short(g_ist8801_data, buf[2], buf[1]);
	} else {
		*data = pre_value;
		return err;
	}

	*data = value;
	pre_value = value;

	return 0;
}

static void ist8801_dump_reg(u8 *buf)
{
	int i, err;
	u8 val;
	u8 buffer[512] = {0};
	u8 _buf[20] = {0};

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return;
	}

	for (i = 0; i <= 0x12; i++) {
		memset(_buf, 0, sizeof(_buf));

		err = ist8801_i2c_read_block(g_ist8801_data, i, &val, 1);
		if (err < 0) {
			sprintf(buf, "read reg error!\n");
			return;
		}

		sprintf(_buf,  "reg 0x%x:0x%x\n", i, val);
		strcat(buffer, _buf);
	}

	err = ist8801_i2c_read_block(g_ist8801_data, 0x54, &val, 1);
	if (err < 0) {
		sprintf(buf, "read reg error!\n");
		return;
	}
	sprintf(_buf,  "reg 0x%x:0x%x\n", 0x54, val);
	strcat(buffer, _buf);

	sprintf(buf, "%s\n", (char*)buffer);
	TRI_KEY_LOG("%s\n", buf);
}

static int ist8801_set_reg(int reg, int val)
{
	u8 data = (u8)val;

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	ist8801_i2c_write_block(g_ist8801_data, (u8)reg, &data, 1);

	return 0;
}


static bool ist8801_is_power_on(void)
{
	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return false;
	}

	return g_ist8801_data->is_power_on;
}

static int ist8801_set_power(struct ist8801_data_t *ist8801_data, bool on)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(ist8801_data->power_2v8)) {
		TRI_KEY_ERR("vdd_2v8 invalid\n");
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(ist8801_data->power_1v8)) {
		TRI_KEY_ERR("vdd1v8 invalid\n");
		return -EINVAL;
	}

	if (on) {
		if (regulator_count_voltages(ist8801_data->power_2v8) > 0) {
			ret = regulator_set_voltage(ist8801_data->power_2v8,
					2856000, 3104000);
			if (ret) {
				TRI_KEY_LOG("Regulator failed vdd\n", ret);
				return ret;
			}

			ret = regulator_set_load(ist8801_data->power_2v8,
					CURRENT_LOAD_UA);
			if (ret) {
				TRI_KEY_LOG("Regulator failed vdd\n");
				return ret;
			}
		}
		if (regulator_count_voltages(ist8801_data->power_1v8) > 0) {
			ret = regulator_set_voltage(ist8801_data->power_1v8,
					1800000, 1800000);
			if (ret) {
				TRI_KEY_LOG("Regulator failed vcc_i2c\n");
				return ret;
			}
		}
		ret = regulator_enable(ist8801_data->power_2v8);
		if (ret) {
			TRI_KEY_LOG("Regulator vdd enable failed\n");
			return ret;
		}

		msleep(20);

		ret = regulator_enable(ist8801_data->power_1v8);
		if (ret) {
			TRI_KEY_LOG("Regulator vcc_i2c enable failed\n");
			regulator_disable(ist8801_data->power_2v8);
			return ret;
		}

		ist8801_data->is_power_on = true;

	} else {
		ret = regulator_disable(ist8801_data->power_1v8);
		if (ret) {
			TRI_KEY_LOG("Regulator vcc_i2c disable failed\n");
			ret = regulator_enable(ist8801_data->power_2v8);
			return ret;
		}

		msleep(20);
		ret = regulator_disable(ist8801_data->power_2v8);
		if (ret) {
			TRI_KEY_LOG("Regulator vdd disable failed\n");
			return ret;
		}
	}

	return 0;
}

static int ist8801_clear_interrupt(struct ist8801_data_t *ist8801_data)
{
	int ret = 0;

	u8 data = ist8801_data->reg.map.persint | 0x01;

	ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_PERSINT, &data, 1);

	ist8801_data->reg.map.persint = ist8801_data->reg.map.persint & 0xfe;
	data = ist8801_data->reg.map.persint;

	ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_PERSINT, &data, 1);

	return ret;
}

/*
 *IST8801_ADC_BIT_NUM
 *8-bit:0x10 : threshold range: 127~-128
 *9-bit:0x0e : threshold range: 255~-256
 *10-bit:0x0c : threshold range: 511~-512
 *11-bit:0x0a : threshold range: 1023~-1024
 *12-bit:0x08 : threshold range: 2047~-2048
 *13-bit:0x06 : threshold range: 4095~-4096
 *14-bit:0x04 : threshold range: 8191~-8192
 *15-bit:0x02 : threshold range: 16383~-16384
 *16-bit: other : threshold range: 32767~-32768
 */
static bool ist8801_up_update_threshold(int position,
				short lowthd, short highthd)
{
	u8 lthh, lthl, hthh, hthl;
	int err = 0;

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	TRI_KEY_LOG("low:%d, high:%d\n", lowthd, highthd);

	err = ist8801_clear_interrupt(g_ist8801_data);

	if (g_ist8801_data->reg.map.intsrs & IST8801_DETECTION_MODE_INTERRUPT) {
		ist8801_short_to_2byte(g_ist8801_data, highthd, &hthh, &hthl);
		ist8801_short_to_2byte(g_ist8801_data, lowthd, &lthh, &lthl);

		err |= ist8801_i2c_write_block(g_ist8801_data,
				IST8801_REG_HTHH, &hthh, 1);
		err |= ist8801_i2c_write_block(g_ist8801_data,
				IST8801_REG_HTHL, &hthl, 1);
		err |= ist8801_i2c_write_block(g_ist8801_data,
				IST8801_REG_LTHH, &lthh, 1);
		err |= ist8801_i2c_write_block(g_ist8801_data,
				IST8801_REG_LTHL, &lthl, 1);
	}

	if (err < 0) {
		TRI_KEY_ERR("fail %d\n", err);
		return false;
	} else {
		return true;
	}
}

static int ist8801_set_operation_mode(struct ist8801_data_t *ist8801_data,
						int mode)
{
	u8 opf = 0;
	int ret = 0;
	u8 ifcntl = 0;

	switch (mode) {
	case OPERATION_MODE_POWERDOWN:
		opf = 0;
		ifcntl = 0;
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_OPF, &opf, 1);

		ret = ist8801_i2c_read_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);
		ifcntl |= 0x04;
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);
		TRI_KEY_ERR("operation ERATION_MODE_POWERDOWN\n");
		break;
	case OPERATION_MODE_MEASUREMENT:
		opf = 0x00;
		TRI_KEY_ERR("opf = 0x%x\n", opf);

		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_ACTION, &opf, 1);

		usleep_range(5000, 5100);

		opf = 0x00;
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_OPF, &opf, 1);

		ret = ist8801_i2c_read_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);
		ifcntl |= 0x04;
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);

		opf = FREQUENCY;
		TRI_KEY_ERR("opf = 0x%x\n", opf);
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_OPF, &opf, 1);
		TRI_KEY_ERR("=OPERATION_MODE_MEASUREMENT\n");
		break;
	case OPERATION_MODE_LOWPOWER_MEASUREMENT:
		opf = 0x00;
		TRI_KEY_ERR("opf = 0x%x\n", opf);

		ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_ACTION, &opf, 1);

		usleep_range(5000, 5100);

		opf = 0x00;
		ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_OPF, &opf, 1);

		ret = ist8801_i2c_read_block(ist8801_data,
			IST8801_REG_IFCNTL, &ifcntl, 1);
		ifcntl |= 0x04;
		ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_IFCNTL, &ifcntl, 1);

		opf = LOWPOWER_FREQUENCY;
		TRI_KEY_ERR("opf = 0x%x\n", opf);
		ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_OPF, &opf, 1);
		TRI_KEY_ERR("operation mode LOWPOWER_MEASUREMENT\n");
		break;

	case OPERATION_MODE_SUSPEND:
		opf = 0x00;
		TRI_KEY_ERR("opf = 0x%x\n", opf);
		ret = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_OPF, &opf, 1);

		ret = ist8801_i2c_read_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);
		ifcntl |= 0x04;
		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_IFCNTL, &ifcntl, 1);

		opf = 0x02;
		TRI_KEY_ERR("opf = 0x%x\n", opf);

		ret = ist8801_i2c_write_block(ist8801_data,
				IST8801_REG_ACTION, &opf, 1);
		TRI_KEY_ERR("operation mode :OPERATION_MODE_SUSPEND\n");

		usleep_range(5000, 5100);
		break;
	}

	TRI_KEY_ERR("opf = 0x%x\n", opf);

	return ret;
}


/* functions for interrupt handler */
static irqreturn_t ist8801_up_irq_handler(int irq, void *dev_id)
{
	TRI_KEY_LOG("call\n");

	if (!g_ist8801_data) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	disable_irq_nosync(g_ist8801_data->irq);
	__pm_wakeup_event(&g_ist8801_data->source, 2000);
	oplus_hall_irq_handler(0);

	return IRQ_HANDLED;
}

static int ist8801_setup_eint(struct ist8801_data_t *ist8801_data)
{
	int ret = 0;

	if (gpio_is_valid(ist8801_data->irq_gpio)) {
		ret = gpio_request(ist8801_data->irq_gpio, "ist8801_up_irq");
		if (ret)
			TRI_KEY_LOG("unable to request gpio [%d]\n",
			ist8801_data->irq_gpio);
		else {
			ret = gpio_direction_input(ist8801_data->irq_gpio);
			msleep(50);
			ist8801_data->irq = gpio_to_irq(ist8801_data->irq_gpio);
		}
	}
	TRI_KEY_ERR("GPIO %d irq:%d\n", ist8801_data->irq_gpio,
			ist8801_data->irq);

	return 0;
}

static int ist8801_set_detection_mode(u8 mode)
{
	u8 data = 0;
	int err = 0;

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	TRI_KEY_LOG("ist8801 detection mode : %s\n",
				(mode == 0) ? "POLLING":"INTERRUPT");

	if (mode & DETECTION_MODE_INTERRUPT) {
		if (!g_ist8801_data->irq_enabled) {
			data = g_ist8801_data->reg.map.intsrs |
				IST8801_DETECTION_MODE_INTERRUPT;
			err = ist8801_i2c_write_block(g_ist8801_data,
				IST8801_REG_INTSRS, &data, 1);
			if (err < 0) {
				TRI_KEY_ERR("config interrupt fail %d\n", err);
				return err;
			}

			err = ist8801_clear_interrupt(g_ist8801_data);
			if (err < 0) {
				TRI_KEY_ERR("clear interrupt fail %d\n", err);
				return err;
			}

			err = request_threaded_irq(g_ist8801_data->irq, NULL,
			&ist8801_up_irq_handler,
			IRQ_TYPE_LEVEL_LOW | IRQF_ONESHOT,
				"ist8801_up", (void *)g_ist8801_data->client);
			if (err < 0) {
				TRI_KEY_ERR("IRQ LINE NOT AVAILABLE!!\n");
				return -EINVAL;
			}
			irq_set_irq_wake(g_ist8801_data->irq, 1);

			g_ist8801_data->irq_enabled = 1;
		}
	} else {
		if (g_ist8801_data->irq_enabled) {
			data = g_ist8801_data->reg.map.intsrs &
				(0xFF - IST8801_DETECTION_MODE_INTERRUPT);

			err = ist8801_i2c_write_block(g_ist8801_data,
					IST8801_REG_INTSRS, &data, 1);
			if (err < 0) {
				TRI_KEY_ERR("config interrupt fail %d\n", err);
				return err;
			}

			disable_irq(g_ist8801_data->irq);
			free_irq(g_ist8801_data->irq, NULL);

			g_ist8801_data->irq_enabled = 0;
		}
	}

	return 0;
}

static int ist8801_enable_irq(bool enable)
{
	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return -EINVAL;
	}

	if (enable)
		enable_irq(g_ist8801_data->irq);
	else
		disable_irq_nosync(g_ist8801_data->irq);
	return 0;
}

static int ist8801_clear_irq(void)
{
	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL =\n");
		return -EINVAL;
	}

	ist8801_clear_interrupt(g_ist8801_data);

	return 0;
}

static int ist8801_get_irq_state(void)
{
	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL =\n");
		return -EINVAL;
	}

	return ((g_ist8801_data->reg.map.intsrs &
			IST8801_DETECTION_MODE_INTERRUPT) ? 1 : 0);
}

static void ist8801_set_sensitivity(char *value)
{
	int i = 0;
	uint8_t rwdata;
	struct hall_srs *srs = NULL, *ist8801_ranges = NULL;
	int len1 = 0, len2 = 0, len = 0;
	uint8_t temp_opf, err;

	if (g_ist8801_data == NULL) {
		TRI_KEY_LOG("g_ist8801_data NULL\n");
		return;
	}


	len1 = sizeof(ist8801_ranges_1)/sizeof(struct hall_srs);
	len2 = sizeof(ist8801_ranges_2)/sizeof(struct hall_srs);

	if (g_ist8801_data->origin_info == 0x01) {
		len = len1;
		ist8801_ranges = ist8801_ranges_1;
	} else {
		len = len2;
		ist8801_ranges = ist8801_ranges_2;
	}

	for (i = 0; i < len; i++) {
		srs = &ist8801_ranges[i];
		if (!strncmp(srs->name, value, strlen(srs->name)))
			break;
	}
	if (i == len)
		srs = NULL;

	if (!srs) {
		TRI_KEY_ERR("%s not match\n", value);
		return;
	}

	temp_opf = 0x00;
	err = ist8801_i2c_read_block(g_ist8801_data,
			IST8801_REG_OPF, &temp_opf, 1);

	rwdata = 0x00;
	ist8801_i2c_write_block(g_ist8801_data, IST8801_REG_OPF, &rwdata, 1);

	err = ist8801_i2c_read_block(g_ist8801_data,
		IST8801_REG_IFCNTL, &rwdata, 1);
	rwdata |= 0x04;
	ist8801_i2c_write_block(g_ist8801_data, IST8801_REG_IFCNTL, &rwdata, 1);

	rwdata = ((DYNAMIC_GAIN_ADC_BIT & 0x1E) | srs->value);
	ist8801_i2c_write_block(g_ist8801_data, IST8801_REG_CNTL2, &rwdata, 1);

	TRI_KEY_LOG("set sensitivity IST8801_REG_CNTL2 = 0x%x\n", rwdata);

	rwdata = 0;
	ist8801_i2c_read_block(g_ist8801_data, IST8801_REG_CNTL2, &rwdata, 1);

	TRI_KEY_LOG("get sensitivity IST8801_REG_CNTL2 = 0x%x\n", rwdata);

	rwdata = ((uint8_t) srs->ratio) + g_ist8801_data->origin_gain;
	TRI_KEY_LOG("set sensitivity IST8801_REG_GAINCNTL = %d\n", rwdata);

	ist8801_i2c_write_block(g_ist8801_data,
		IST8801_REG_GAINCNTL, &rwdata, 1);

	rwdata = 0;
	err = ist8801_i2c_read_block(g_ist8801_data,
		IST8801_REG_GAINCNTL, &rwdata, 1);

	TRI_KEY_LOG("get sensitivity IST8801_REG_GAINCNTL = %d\n", rwdata);

	rwdata = temp_opf;
	ist8801_i2c_write_block(g_ist8801_data, IST8801_REG_OPF, &rwdata, 1);
}
/*
 *IST8801_ADC_BIT_NUM
 *8-bit:0x10
 *9-bit:0x0e
 *10-bit:0x0c
 *11-bit:0x0a
 *12-bit:0x08
 *13-bit:0x06
 *14-bit:0x04
 *15-bit:0x02
 *16-bit: other
 */
static int ist8801_reset_device(struct ist8801_data_t *ist8801_data)
{
	int err = 0;
	u8 data = 0;

	data = IST8801_VAL_SRST_RESET;
	err = ist8801_i2c_write_block(ist8801_data, IST8801_REG_SRST, &data, 1);

	if (err < 0) {
		TRI_KEY_ERR("sw-reset failed(%d)", err);
		return err;
	}
	msleep(20);

	err = ist8801_i2c_read_block(ist8801_data, IST8801_REG_DID, &data, 1);
	if (err < 0) {
		TRI_KEY_ERR("read IST8801_REG_DID failed(%d)", err);
		return err;
	}
	if (data != IST8801_VAL_DID) {
		TRI_KEY_ERR("current device id(0x%02X)", data,
			"is not IST8801 device id(0x%02X)",
			 IST8801_VAL_DID);
	}

	data = 0x04;
	err = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_TSTCNTL, &data, 1);

	data = 0x05;
	err = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_OSRCNTL, &data, 1);

	data = 0x00;
	ist8801_i2c_read_block(ist8801_data, IST8801_REG_GAINCNTL, &data, 1);
	ist8801_data->origin_gain = data;

	TRI_KEY_LOG("ist8801_data->origin_gain = %d\n",
			ist8801_data->origin_gain);

	data = 0x00;
	ist8801_i2c_read_block(ist8801_data, IST8801_REG_OSRCNTL, &data, 1);
	ist8801_data->origin_osr = data;

	TRI_KEY_LOG("ist8801_data->origin_osr = %d\n",
			ist8801_data->origin_osr);

	data = 0x00;
	ist8801_i2c_read_block(ist8801_data, IST8801_REG_INFO, &data, 1);
	ist8801_data->origin_info = data;

	TRI_KEY_LOG("ist8801_data->origin_info = %d\n",
			ist8801_data->origin_info);

	ist8801_data->reg.map.persint = IST8801_PERSISTENCE_COUNT;
	data = ist8801_data->reg.map.persint;
	err = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_PERSINT, &data, 1);


	ist8801_data->reg.map.intsrs = IST8801_DETECTION_MODE |
				ist8801_data->reg.map.range;
	if (ist8801_data->reg.map.intsrs & IST8801_DETECTION_MODE_INTERRUPT)
		ist8801_data->reg.map.intsrs |= IST8801_INTERRUPT_TYPE;

	data = ist8801_data->reg.map.intsrs;
	err = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_INTSRS, &data, 1);

#if DISABLE_TEMP_CONPEN
	data = 0x01;
	err = ist8801_i2c_write_block(ist8801_data,
			IST8801_REG_CHIP_TEST, &data, 1);
	if (err < 0) {
		TRI_KEY_ERR("IST8801_REG_CHIP_TEST failed(%d)", err);
		return err;
	}
#endif
	err = ist8801_set_operation_mode(ist8801_data,
			OPERATION_MODE_MEASUREMENT);
	if (err < 0) {
		TRI_KEY_ERR("ist8801_set_operation_mode was failed(%d)", err);
		return err;
	}

	return err;
}

static int ist8801_parse_dts(struct device *dev, struct ist8801_data_t *p_data)
{
	struct device_node *np = dev->of_node;
	int rc = 0;
	uint32_t data_range;
	uint32_t value;

	rc = of_property_read_u32(np, "data-range", &data_range);
	if (rc) {
		p_data->reg.map.range = IST8801_SENSITIVITY_TYPE;
		TRI_KEY_LOG("use default value:0x%x\n",
				p_data->reg.map.range);
	} else {
		p_data->reg.map.range = (uint8_t)data_range;
		TRI_KEY_LOG("data-range is 0x%x\n", p_data->reg.map.range);
	}

	p_data->irq_gpio = of_get_named_gpio(np, "dhall,irq-gpio", 0);

	p_data->power_2v8 = regulator_get(&p_data->client->dev, "vdd");
	if (IS_ERR_OR_NULL(p_data->power_2v8)) {
		TRI_KEY_ERR("Regulator get failed vdd_2v8\n");
		goto err;
	}

	p_data->power_1v8 = regulator_get(&p_data->client->dev, "vio");
	if (IS_ERR_OR_NULL(p_data->power_1v8)) {
		TRI_KEY_ERR("Regulator get failed vcc_1v8\n");
		goto err;
	}

	p_data->pctrl = devm_pinctrl_get(&p_data->client->dev);
	if (IS_ERR_OR_NULL(p_data->pctrl)) {
		TRI_KEY_ERR("failed to get pinctrl\n");
		goto err;
	}

	p_data->irq_state = pinctrl_lookup_state(p_data->pctrl,
			"ist8801_hall_up_active");
	if (IS_ERR_OR_NULL(p_data->irq_state)) {
		rc = PTR_ERR(p_data->irq_state);
		TRI_KEY_ERR("pinctrl_lookup_state, err:%d\n", rc);
		goto err;
	} else {
		pinctrl_select_state(p_data->pctrl, p_data->irq_state);
	}

	p_data->enable_hidden = of_property_read_bool(np, "hall,bias_support");
	if (p_data->enable_hidden) {
		rc = of_property_read_u32(np, "hall,bias-ratio", &value);
		if (rc)
			p_data->bias_ratio = 100;
		else
			p_data->bias_ratio = value;
	}
	return 0;
err:
	return rc;
};

struct dhall_operations  ist8801_up_ops = {
	.get_data  = ist8801_get_data,
	.enable_irq = ist8801_enable_irq,
	.clear_irq = ist8801_clear_irq,
	.get_irq_state = ist8801_get_irq_state,
	.set_detection_mode = ist8801_set_detection_mode,
	.update_threshold = ist8801_up_update_threshold,
	.dump_regs = ist8801_dump_reg,
	.set_reg = ist8801_set_reg,
	.is_power_on = ist8801_is_power_on,
	.set_sensitivity = ist8801_set_sensitivity,
};

static int ist8801_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ist8801_data_t *p_data = NULL;
	struct extcon_dev_data *hall_dev = NULL;
	u8 dev_id = 0xFF;
	int err = 0;

	TRI_KEY_LOG("call\n");

	p_data = devm_kzalloc(&client->dev, sizeof(struct ist8801_data_t),
				GFP_KERNEL);
	if (!p_data) {
		TRI_KEY_ERR("kernel memory alocation was failed\n");
		return -ENOMEM;
	}

	hall_dev = kzalloc(sizeof(struct extcon_dev_data), GFP_KERNEL);
	if (!hall_dev) {
		TRI_KEY_ERR("kernel memory alocation was failed\n");
		devm_kfree(&client->dev, p_data);
		return -ENOMEM;
	}
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		TRI_KEY_LOG("i2c unsupported\n");
		devm_kfree(&client->dev, p_data);
		kfree(hall_dev);
		return -EOPNOTSUPP;
	}
	p_data->client = client;
	p_data->dev = &client->dev;
	g_ist8801_data = p_data;
	hall_dev->client = client;
	i2c_set_clientdata(client, hall_dev);
	hall_dev->dev = &client->dev;
	if (client->dev.of_node) {
		err = ist8801_parse_dts(&client->dev, p_data);
		if (err)
			TRI_KEY_ERR("failed to parse device tree\n");
	}
	ist8801_reconfig(p_data);

	err = ist8801_set_power(p_data, 1);

	dev_id = ist8801_get_id(p_data);
	if (dev_id != IST8801_VAL_DID) {
		TRI_KEY_ERR("current device id(0x%02x)", dev_id,
			"is not ist8801 device id(0x%02x)\n",
			IST8801_VAL_DID);
		goto fail;
	}

	err = ist8801_reset_device(p_data);
	if (err < 0) {
		TRI_KEY_ERR("ist8801_reset_device fail\n");
		goto fail;
	}

	err = ist8801_setup_eint(p_data);

	wakeup_source_add(&g_ist8801_data->source);

	ist8801_set_sensitivity("40mT");

	oplus_register_hall("hall_up", &ist8801_up_ops, hall_dev);

	TRI_KEY_LOG("success.\n");
	return 0;
fail:
	TRI_KEY_LOG("fail.\n");
	if (gpio_is_valid(p_data->irq_gpio))
		gpio_free(p_data->irq_gpio);

	devm_kfree(&client->dev, p_data);
	kfree(hall_dev);
	return -ENXIO;
}

static int ist8801_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct of_device_id ist8801_match[] = {
	{ .compatible = HALL_IST8801_UP, },
	{},
};

static const struct i2c_device_id ist8801_id[] = {
	{"hall_ist8801_up", 0 },
	{},
};

/*
 *static const struct dev_pm_ops ist8801_pm_ops = {
 *.suspend = ist8801_i2c_suspend,
 *.resume = ist8801_i2c_resume,
 *};
 */
struct i2c_driver ist8801_i2c_up_driver = {
	.driver = {
		.name	= "hall-ist8801-up",
		.of_match_table =  ist8801_match,
	},
	.probe		= ist8801_i2c_probe,
	.remove		= ist8801_i2c_remove,
	.id_table	= ist8801_id,
};

static int __init ist8801_up_init(void)
{
	TRI_KEY_LOG("call\n");
	i2c_add_driver(&ist8801_i2c_up_driver);
	return 0;
}
late_initcall(ist8801_up_init);

static void __exit ist8801_up_exit(void)
{
	TRI_KEY_LOG("call\n");
	i2c_del_driver(&ist8801_i2c_up_driver);
}

module_exit(ist8801_up_exit);

MODULE_DESCRIPTION("ist8801 hallswitch driver");
MODULE_LICENSE("GPL");


