// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#define VOOC_ASIC_OP10

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#ifdef CONFIG_OPLUS_CHARGER_MTK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

//#include <linux/xlog.h>
//#include <upmu_common.h>
//#include <mt-plat/mtk_gpio.h>
#include <linux/dma-mapping.h>

//#include <mt-plat/battery_meter.h>
#include <linux/module.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/device_info.h>
#endif

#else
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
#include <soc/oplus/device_info.h>
#endif
#endif
#include "oplus_vooc_fw.h"
#include "../oplus_pps.h"
#include "../oplus_pps_ops_manager.h"

extern int charger_abnormal_log;

static int op10_get_fw_old_version(struct oplus_vooc_chip *chip, u8 version_info[]);

#ifdef CONFIG_OPLUS_CHARGER_MTK
#define I2C_MASK_FLAG	(0x00ff)
#define I2C_ENEXT_FLAG	(0x0200)
#define I2C_DMA_FLAG	(0xdead2000)
#endif

#define GTP_DMA_MAX_TRANSACTION_LENGTH	255 /* for DMA mode */

#define ERASE_COUNT			959 /*0x0000-0x3BFF*/

#define BYTE_OFFSET			2
#define BYTES_TO_WRITE		16
#define FW_CHECK_FAIL		0
#define FW_CHECK_SUCCESS	1

#define REG_SYS0			0xC0
#define REG_STATE			0xC4
#define REG_HOST			0xC8

#define POLYNOMIAL				0x04C11DB7
#define INITIAL_REMAINDER		0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF

#define WIDTH		(8 * sizeof(u32))
#define TOPBIT		(1U << (WIDTH - 1))
#define REFLECT_DATA(X)			(X)
#define REFLECT_REMAINDER(X)	(X)

#define CMD_SET_ADDR			0x01
#define CMD_XFER_W_DAT		0x02
#define CMD_XFER_R_DATA		0x03
#define CMD_PRG_START			0x05
#define CMD_USER_BOOT			0x06
#define CMD_CHIP_ERASE			0x07
#define CMD_GET_VERSION			0x08
#define CMD_GET_CRC32			0x09
#define CMD_SET_CKSM_LEN		0x0A
#define CMD_DEV_STATUS			0x0B

#define I2C_RW_LEN_MAX			32
#define ONE_WRITE_LEN_MAX		256
#define FW_VERSION_LEN			11

#define ENABLE_OVP_AND_WDT_FLAG		0xa5a50000
#define DISABLE_OVP_AND_WDT_FLAG	0x0000ffff
static struct oplus_vooc_chip *the_chip = NULL;
struct wakeup_source *op10_update_wake_lock = NULL;

#ifdef CONFIG_OPLUS_CHARGER_MTK
#define GTP_SUPPORT_I2C_DMA		0
#define I2C_MASTER_CLOCK			300

DEFINE_MUTEX(dma_wr_access_op10);

/*static char gpDMABuf_pa[GTP_DMA_MAX_TRANSACTION_LENGTH] = {0};*/

#if GTP_SUPPORT_I2C_DMA
static int i2c_dma_write(struct i2c_client *client, u8 addr, s32 len, u8 *txbuf);
static int i2c_dma_read(struct i2c_client *client, u8 addr, s32 len, u8 *txbuf);
static u8 *gpDMABuf_va = NULL;
static dma_addr_t gpDMABuf_pa = 0;
#endif

#if GTP_SUPPORT_I2C_DMA
static int i2c_dma_read(struct i2c_client *client, u8 addr, s32 len, u8 *rxbuf)
{
	int ret;
	s32 retry = 0;
	u8 buffer[1];

	struct i2c_msg msg[2] = {
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.flags = 0,
			.buf = buffer,
			.len = 1,
			.timing = I2C_MASTER_CLOCK
		},
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			.flags = I2C_M_RD,
			.buf = (__u8 *)gpDMABuf_pa, /*modified by PengNan*/
			.len = len,
			.timing = I2C_MASTER_CLOCK
		},
	};

	mutex_lock(&dma_wr_access_op10);
	/*buffer[0] = (addr >> 8) & 0xFF;*/
	buffer[0] = addr & 0xFF;
	if (rxbuf == NULL) {
		mutex_unlock(&dma_wr_access_op10);
		return -1;
	}
	/*chg_err("vooc dma i2c read: 0x%x, %d bytes(s)\n", addr, len);*/
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0) {
			continue;
		}
		memcpy(rxbuf, gpDMABuf_va, len);
		mutex_unlock(&dma_wr_access_op10);
		return 0;
	}
	/*chg_err(" Error: 0x%04X, %d byte(s), err-code: %d\n", addr, len, ret);*/
	mutex_unlock(&dma_wr_access_op10);
	return ret;
}

static int i2c_dma_write(struct i2c_client *client, u8 addr, s32 len, u8 const *txbuf)
{
	int ret = 0;
	s32 retry = 0;
	u8 *wr_buf = gpDMABuf_va;
	struct i2c_msg msg = {
		.addr = (client->addr & I2C_MASK_FLAG),
		.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
		.flags = 0,
		.buf = (__u8 *)gpDMABuf_pa, /*modified by PengNan*/
		.len = 1 + len,
		.timing = I2C_MASTER_CLOCK
	};

	mutex_lock(&dma_wr_access_op10);
	wr_buf[0] = (u8)(addr & 0xFF);
	if (txbuf == NULL) {
		mutex_unlock(&dma_wr_access_op10);
		return -1;
	}
	memcpy(wr_buf + 1, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0) {
			continue;
		}
		mutex_unlock(&dma_wr_access_op10);
		return 0;
	}
	/*chg_err(" Error: 0x%04X, %d byte(s), err-code: %d\n", addr, len, ret);*/
	mutex_unlock(&dma_wr_access_op10);
	return ret;
}
#endif /*GTP_SUPPORT_I2C_DMA*/
#endif /*CONFIG_OPLUS_CHARGER_MTK*/

static int oplus_vooc_i2c_read(struct i2c_client *client, u8 addr, s32 len, u8 *rxbuf)
{
#ifdef CONFIG_OPLUS_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
	return i2c_dma_read(client, addr, len, rxbuf);
#else
	return i2c_smbus_read_i2c_block_data(client, addr, len, rxbuf);
#endif
#else
	return i2c_smbus_read_i2c_block_data(client, addr, len, rxbuf);
#endif
}

static int oplus_vooc_i2c_write(struct i2c_client *client, u8 addr, s32 len, u8 const *txbuf)
{
#ifdef CONFIG_OPLUS_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
	return i2c_dma_write(client, addr, len, txbuf);
#else
	return i2c_smbus_write_i2c_block_data(client, addr, len, txbuf);
#endif
#else
	return i2c_smbus_write_i2c_block_data(client, addr, len, txbuf);
#endif
}

static int check_flash_idle(struct oplus_vooc_chip *chip, u32 try_count)
{
	u8 rx_buf;
	int rc = 0;

	do {
		rx_buf = 0xff;
		rc = oplus_vooc_i2c_read(chip->client, CMD_DEV_STATUS,1, &rx_buf);
		if (rc < 0) {
			chg_debug("read CMD_DEV_STATUS error:%0x\n", rx_buf);
			goto i2c_err;
		}
		//chg_debug("the rx_buf=%0x\n", rx_buf);
		if ((rx_buf & 0x01) == 0x0) {// check OP10 flash is idle
			return 0;
		}
		try_count--;
		msleep(20);
	} while (try_count);

i2c_err:
	return -1;
}

static int check_crc32_available(struct oplus_vooc_chip *chip, u32 try_count)
{
	u8 rx_buf;
	int rc = 0;

	do {
		rx_buf = 0x0;
		rc = oplus_vooc_i2c_read(chip->client, CMD_DEV_STATUS, 1, &rx_buf);
		if (rc < 0) {
			chg_debug("read CMD_DEV_STATUS error:%0x\n", rx_buf);
			goto i2c_err;
		}
		//chg_debug("the rx_buf=%0x\n", rx_buf);
		if ((rx_buf & 0x02) == 0x2) {
			return 0;
		}
		try_count--;
		msleep(20);
	} while (try_count);

i2c_err:
	return -1;
}

u32 crc32_sram(struct oplus_vooc_chip *chip)
{
	u32 remainder = INITIAL_REMAINDER;
	u32 byte;
	u8 bit;

	/* Perform modulo-2 division, a byte at a time. */
	for (byte = 0; byte < chip->fw_data_count; ++byte) {
		/* Bring the next byte into the remainder. */
		remainder ^= (REFLECT_DATA(chip->firmware_data[byte]) << (WIDTH - 8));

		/* Perform modulo-2 division, a bit at a time.*/
		for (bit = 8; bit > 0; --bit) {
			/* Try to divide the current data bit. */
			if (remainder & TOPBIT) {
				remainder = (remainder << 1) ^ POLYNOMIAL;
			} else {
				remainder = (remainder << 1);
			}
		}
	}
	/* The final remainder is the CRC result. */
	return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);
}

static bool op10_fw_update_check(struct oplus_vooc_chip *chip)
{
	int i = 0;
	int ret = 0;
	u8 fw_version[FW_VERSION_LEN] = {0};
	u8 rx_buf[4] = {0};
	u32 check_status_try_count = 100;//try 2s
	u32 fw_status_address = 0x4000 - 0x10;
	u32 new_fw_crc32 = 0;

	ret = op10_get_fw_old_version(chip, fw_version);
	if (ret == 1)
		return false;

	/* chip version */
	ret = oplus_vooc_i2c_read(chip->client, CMD_GET_VERSION, 1, rx_buf);
	if (ret < 0) {
		chg_err("read CMD_GET_VERSION error:%d\n", ret);
	} else {
		switch (rx_buf[0]) {
		case 0x01:
		case 0x02:
			chg_debug("chip is sy6610:0x%02x\n", rx_buf[0]);
			break;
		case 0x11:
			chg_debug("chip is sy6610c:0x%02x\n", rx_buf[0]);
			break;
		default:
			chg_debug("invalid chip version:0x%02x\n", rx_buf[0]);
		}
	}

	/*read fw status*/
	rx_buf[0] = fw_status_address & 0xFF;
	rx_buf[1] = (fw_status_address >> 8) & 0xFF;
	oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
	msleep(1);
	memset(rx_buf, 0, 4);
	oplus_vooc_i2c_read(chip->client, CMD_XFER_R_DATA, 4, rx_buf);
	chg_debug("fw crc32 status:0x%08x\n", *((u32 *)rx_buf));

	chip->fw_mcu_version = fw_version[FW_VERSION_LEN-4];

	for (i = 0; i < FW_VERSION_LEN; i++) {
		chg_debug("the old version: %0x, the fw version: %0x\n", fw_version[i], chip->firmware_data[chip->fw_data_count - FW_VERSION_LEN + i]);
		if (fw_version[i] != chip->firmware_data[chip->fw_data_count - FW_VERSION_LEN + i])
			return false;
	}

	/*noticefy OP10 to update the CRC32 and check it*/
	*((u32 *)rx_buf) = chip->fw_data_count;
	oplus_vooc_i2c_write(chip->client, CMD_SET_CKSM_LEN, 4, rx_buf);
	msleep(5);
	if (check_crc32_available(chip, check_status_try_count) == -1) {
		chg_debug("crc32 is not available, timeout!\n");
		return false;
	}

	/* check crc32 is correct */
	memset(rx_buf, 0, 4);
	oplus_vooc_i2c_read(chip->client, CMD_GET_CRC32, 4, rx_buf);
	new_fw_crc32 = crc32_sram(chip);
	chg_debug("fw_data_crc:0x%0x, the read data_crc32:0x%0x\n", new_fw_crc32, *((u32 *)rx_buf));
	if (*((u32 *)rx_buf) != new_fw_crc32) {
		chg_debug("crc32 compare fail!\n");
		return false;
	}

	/* fw update success,jump to new fw */
	/*oplus_vooc_i2c_read(chip->client, CMD_USER_BOOT, 1, rx_buf);*/

	return true;
}

int op10_read_input_voltage(void)
{
	int ret = 0;
	u8 read_buf[4] = {0};

	if (!the_chip) {
		printk("op10_read_input_voltage fail\n");
		return -1;
	}

	//ret = oplus_i2c_dma_read(the_chip->client, REG_SYS0, 4, read_buf);
	ret = oplus_vooc_i2c_read(the_chip->client, REG_SYS0, 4, read_buf);
	if (ret < 0) {
		printk("op10 read REG_SYS0 fail\n");
		return -1;
	}
	printk("op10_read_input_voltage the data: %x, %x, %x, %x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

	ret = ((read_buf[3] << 8) | read_buf[2]);
	printk("op10_read_input_voltage ret = %d\n", ret);

	return ret;
}

int op10_read_vbat0_voltage(void)
{
	int ret = 0;
	u8 read_buf[4] = {0};

	if (!the_chip) {
		printk("op10_read_vbat0_voltage fail\n");
		return -1;
	}

	//ret = oplus_i2c_dma_read(the_chip->client, REG_SYS0, 4, read_buf);
	ret = oplus_vooc_i2c_read(the_chip->client, REG_SYS0, 4, read_buf);
	if (ret < 0) {
		printk("op10 read REG_SYS0 fail\n");
		return -1;
	}
	printk("op10_read_vbat0_voltage the data: %x, %x, %x, %x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

	ret = ((read_buf[1] << 8) | read_buf[0]);
	printk("op10_read_vbat0_voltage ret = %d\n", ret);

	return ret;
}

#define protect_temp_80 211
int op10_check_btb_temp(void)
{
	int ret = 0;
	int usb_btb, bat_btb = -1;
	u8 read_buf[4] = {0};

	if (!the_chip) {
		printk("op10_check_btb_temp fail\n");
		return 1;
	}

	//ret = oplus_i2c_dma_read(the_chip->client, REG_STATE, 4, read_buf);
	ret = oplus_vooc_i2c_read(the_chip->client, REG_STATE, 4, read_buf);
	if (ret < 0) {
		printk("op10_check_btb_temp read REG_STATE fail\n");
		return 1;
	}
	printk("op10_check_btb_temp the data: %x, %x, %x, %x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

	usb_btb = ((read_buf[3] << 8) | read_buf[2]);
	bat_btb = ((read_buf[1] << 8) | read_buf[0]);
	printk("op10_check_btb_temp usb_btb, bat_btb = %d, %d\n", usb_btb, bat_btb);

	if (usb_btb < protect_temp_80 || bat_btb < protect_temp_80) {
		printk("usb_btb or bat_btb over 80\n");
		return 0;
	}

	return 1;
}


int op10_pps_mos_ctrl(int on)
{
	int ret = 0;
	int i = 0;
	u32 ovp_flag = 0;

	if (!the_chip) {
		printk("op10_pps_mos_ctrl fail\n");
		return -1;
	}

	if (on == 1){
		ovp_flag =  ENABLE_OVP_AND_WDT_FLAG;
		//ret = oplus_i2c_dma_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
		ret = oplus_vooc_i2c_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
		if (ret < 0) {
			printk("op10_pps_mos_ctrl write REG_HOST fail\n");
			return -1;
		}
		printk("op10_pps_mos_ctrl enable flags:0x%x, ret:%d\n", ovp_flag, ret);
	} else {
		ovp_flag =  DISABLE_OVP_AND_WDT_FLAG;
		//ret = oplus_i2c_dma_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
		ret = oplus_vooc_i2c_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
		printk("op10_pps_mos_ctrl disable flags:0x%x, ret:%d\n", ovp_flag, ret);
		if (ret < 0) {
			for (i=0; i<3; i++) {
				msleep(50);
				//ret = oplus_i2c_dma_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
				ret = oplus_vooc_i2c_write(the_chip->client, REG_HOST, 4, (u8 *)(&ovp_flag));
				if (ret >= 0)
					break;
			}
		}
		if(ret >= 0){
			printk("op10_pps_mos_ctrl disable success\n");
		} else {
			printk("op10_pps_mos_ctrl write REG_HOST fail\n");
		}
	}

	return ret;
}

static int op10_fw_update(struct oplus_vooc_chip *chip)
{
	u32 check_status_try_count = 100;//try 2s
	u32 write_done_try_count = 500;//max try 10s
	u8 rx_buf[4] = {0};
	u32 fw_len = 0, fw_offset = 0;
	u32 write_len = 0, write_len_temp = 0, chunk_index = 0, chunk_len = 0;
	u32 new_fw_crc32 = 0;
	int rc = 0;
	u32 fw_status_address = 0x4000 - 0x10;

	chg_debug("start op_fw_update now, fw length is: %d\n", chip->fw_data_count);

	/* chip erase */
	rc = oplus_vooc_i2c_read(chip->client, CMD_CHIP_ERASE, 1, rx_buf);
	if (rc < 0) {
		chg_debug("read CMD_CHIP_ERASE error:%d\n", rc);
		goto update_fw_err;
	}
	msleep(100);

	/* check device status */
	if (check_flash_idle(chip, check_status_try_count) == -1) {
		chg_debug("device is always busy, timeout!\n");
		goto update_fw_err;
	}

	/* start write the fw array */
	fw_len = chip->fw_data_count;
	fw_offset = 0;
	while (fw_len) {
		write_len = (fw_len < ONE_WRITE_LEN_MAX) ? fw_len : ONE_WRITE_LEN_MAX;

		/* set flash start address */
		*((u32 *)rx_buf) = fw_offset;
		oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
		msleep(1);

		/* send data which will be written in future */
		chunk_index = 0;
		write_len_temp = write_len;
		while (write_len_temp) {
			chunk_len = (write_len_temp < I2C_RW_LEN_MAX) ? write_len_temp : I2C_RW_LEN_MAX;
			oplus_vooc_i2c_write(chip->client, CMD_XFER_W_DAT, chunk_len, chip->firmware_data + fw_offset + chunk_index * I2C_RW_LEN_MAX);
			msleep(1);

			write_len_temp -= chunk_len;
			chunk_index++;
		}
		oplus_vooc_i2c_read(chip->client, CMD_PRG_START, 1, rx_buf);
		msleep(5);
		if (check_flash_idle(chip, write_done_try_count) == -1) {
			chg_debug("cannot wait flash write done, timeout!\n");
			goto update_fw_err;
		}

		//chg_debug("current write address: %d,to bw write length:%d\n", fw_offset, write_len);
		fw_offset += write_len;
		fw_len -= write_len;
	}

	/*noticefy OP10 to update the CRC32 and check it*/
	*((u32 *)rx_buf) = chip->fw_data_count;
	oplus_vooc_i2c_write(chip->client, CMD_SET_CKSM_LEN, 4, rx_buf);
	msleep(5);
	if (check_crc32_available(chip, check_status_try_count) == -1) {
		chg_debug("crc32 is not available after flash write done, timeout!\n");
		goto update_fw_err;
	}

	/* check crc32 is correct */
	memset(rx_buf, 0, 4);
	oplus_vooc_i2c_read(chip->client, CMD_GET_CRC32, 4, rx_buf);
	new_fw_crc32 = crc32_sram(chip);
	if (*((u32 *)rx_buf) != new_fw_crc32) {
		chg_debug("fw_data_crc:0x%0x, the read data_crc32:0x%0x\n", new_fw_crc32, *((u32 *)rx_buf));
		chg_debug("crc32 compare fail!\n");

		/*write FAIL*/
		rx_buf[0] = fw_status_address & 0xFF;
		rx_buf[1] = (fw_status_address >> 8) & 0xFF;
		oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
		msleep(1);
		*((u32 *)rx_buf) = 0x4641494C;/*FAIL*/
		oplus_vooc_i2c_write(chip->client, CMD_XFER_W_DAT, 4, rx_buf);
		msleep(1);
		oplus_vooc_i2c_read(chip->client, CMD_PRG_START, 1, rx_buf);
		msleep(10);
		if (check_flash_idle(chip, write_done_try_count) == -1) {
			chg_debug("cannot wait flash write fail done, timeout!\n");
		}
		goto update_fw_err;
	}

	/*write SUCC*/
	rx_buf[0] = fw_status_address & 0xFF;
	rx_buf[1] = (fw_status_address >> 8) & 0xFF;
	oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
	msleep(1);
	*((u32 *)rx_buf) = 0x53554343;/*SUCC*/
	oplus_vooc_i2c_write(chip->client, CMD_XFER_W_DAT, 4, rx_buf);
	msleep(1);
	oplus_vooc_i2c_read(chip->client, CMD_PRG_START, 1, rx_buf);
	msleep(10);
	if (check_flash_idle(chip, write_done_try_count) == -1) {
		chg_debug("cannot wait flash write succ done, timeout!\n");
	}

	/* fw update success,jump to new fw */
	oplus_vooc_i2c_read(chip->client, CMD_USER_BOOT, 1, rx_buf);
	chip->fw_mcu_version = chip->fw_data_version;
	chg_debug("success!\n");
	return 0;

update_fw_err:
	charger_abnormal_log = CRITICAL_LOG_VOOC_FW_UPDATE_ERR;
	chg_err("fail\n");
	return 1;
}

static int op10_get_fw_old_version(struct oplus_vooc_chip *chip, u8 version_info[])
{
	u8 rx_buf[4] = {0};//i = 0;
	u32 fw_version_address = 0;
	u32 check_status_try_count = 100;//try 2s
	u32 fw_len_address = 0x4000 - 8;

	memset(version_info, 0xFF, FW_VERSION_LEN);//clear version info at first

	if (check_flash_idle(chip, check_status_try_count) == -1) {
		chg_debug("cannot get the fw old version because of the device is always busy!\n");
		return 1;
	}

	rx_buf[0] = fw_len_address & 0xFF;
	rx_buf[1] = (fw_len_address >> 8) & 0xFF;
	oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
	msleep(1);
	oplus_vooc_i2c_read(chip->client, CMD_XFER_R_DATA, 4, rx_buf);
	if (*((u32 *)rx_buf) < fw_len_address) {
		fw_version_address = *((u32 *)rx_buf) - FW_VERSION_LEN;
		rx_buf[0] = fw_version_address & 0xFF;
		rx_buf[1] = (fw_version_address >> 8) & 0xFF;
		oplus_vooc_i2c_write(chip->client, CMD_SET_ADDR, 2, rx_buf);
		msleep(1);
		oplus_vooc_i2c_read(chip->client, CMD_XFER_R_DATA, FW_VERSION_LEN, version_info);
	} else {
		chg_debug("warning:fw length is invalid\n");
	}


	/* below code is used for debug log,pls comment it after this interface test pass */
	/*chg_debug("the fw old version is:\n");
	for (i = 0; i < FW_VERSION_LEN; i++) {
		chg_debug("0x%x,", version_info[i]);
	}
	chg_debug("\n");*/

	return 0;
}

static int op10_get_fw_verion_from_ic(struct oplus_vooc_chip *chip)
{
	unsigned char addr_buf[2] = {0x3B, 0xF0};
	unsigned char data_buf[4] = {0};
	int rc = 0;
	int update_result = 0;

	if (oplus_is_power_off_charging(chip) == true || oplus_is_charger_reboot(chip) == true) {
		chip->mcu_update_ing = true;
		update_result = op10_fw_update(chip);
		chip->mcu_update_ing = false;
		if (update_result) {
			msleep(30);
			opchg_set_clock_sleep(chip);
			opchg_set_reset_active(chip);
		}
	} else {
		opchg_set_clock_active(chip);
		chip->mcu_boot_by_gpio = true;
		msleep(10);
		opchg_set_reset_active(chip);
		chip->mcu_update_ing = true;
		msleep(2500);
		chip->mcu_boot_by_gpio = false;
		opchg_set_clock_sleep(chip);

		//first:set address
		rc = oplus_vooc_i2c_write(chip->client, 0x01, 2, &addr_buf[0]);
		if (rc < 0) {
			chg_err(" i2c_write 0x01 error\n" );
			return FW_CHECK_FAIL;
		}
		msleep(2);
		oplus_vooc_i2c_read(chip->client, 0x03, 4, data_buf);
		//strcpy(ver,&data_buf[0]);
		chg_err("data:%x %x %x %x, fw_ver:%x\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[0]);

		msleep(5);
		chip->mcu_update_ing = false;
		opchg_set_reset_active(chip);
	}
	return data_buf[0];
}

static int op10_fw_check_then_recover(struct oplus_vooc_chip *chip)
{
	int update_result = 0;
	int try_count = 5;
	int ret = 0;

	if (!chip->firmware_data) {
		chg_err("op10_fw_data Null, Return\n");
		return FW_ERROR_DATA_MODE;
	} else {
		chg_debug("begin\n");
	}

	if (oplus_is_power_off_charging(chip) == true || oplus_is_charger_reboot(chip) == true) {
		chip->mcu_update_ing = false;
		opchg_set_clock_sleep(chip);
		opchg_set_reset_sleep(chip);
		ret = FW_NO_CHECK_MODE;
	} else {
		opchg_set_clock_active(chip);
		chip->mcu_boot_by_gpio = true;
		msleep(10);
		opchg_set_reset_active_force(chip);
		chip->mcu_update_ing = true;
		msleep(2500);
		chip->mcu_boot_by_gpio = false;
		opchg_set_clock_sleep(chip);
		__pm_stay_awake(op10_update_wake_lock);
		if (op10_fw_update_check(chip) == FW_CHECK_FAIL) {
			chg_debug("firmware update start\n");
			do {
				update_result = op10_fw_update(chip);
				if (!update_result)
					break;
				opchg_set_clock_active(chip);
				chip->mcu_boot_by_gpio = true;
				msleep(10);
				//chip->mcu_update_ing = false;
				opchg_set_reset_active_force(chip);
				//chip->mcu_update_ing = true;
				msleep(2500);
				chip->mcu_boot_by_gpio = false;
				opchg_set_clock_sleep(chip);
			} while ((update_result) && (--try_count > 0));
			chg_debug("firmware update end, retry times %d\n", 5 - try_count);
		} else {
			chip->vooc_fw_check = true;
			chg_debug("fw check ok\n");
		}
		__pm_relax(op10_update_wake_lock);
		msleep(5);
		chip->mcu_update_ing = false;
		opchg_set_reset_active(chip);
		ret = FW_CHECK_MODE;
	}
	if (!oplus_vooc_get_fastchg_allow())
		opchg_set_reset_sleep(chip);

	return ret;
}

int op10_set_battery_temperature_soc(int temp_bat, int soc_bat)
{
	int ret = 0;
	u8 read_buf[2] = { 0 };
	chg_err("kilody write op10:temp_bat=%d,soc_bat=%d\n", temp_bat, soc_bat);

	read_buf[0] = temp_bat & 0xff;
	read_buf[1] = (temp_bat >> 8) & 0xff;

	ret = oplus_vooc_i2c_write(the_chip->client, (u8)0xE, 2, read_buf);
	if (ret < 0) {
			chg_err("op10 write slave ack fail");
			return -1;
		}
	return 0;
}

void op10_update_temperature_soc(void)
{
	int temp = 0;
	int soc = 0;

	temp = oplus_chg_match_temp_for_chging();
	op10_set_battery_temperature_soc(temp, soc);

	chg_err("kilody in! soc = %d,temp = %d,chging = %d\n", soc, temp, oplus_vooc_get_fastchg_ing());
}


struct oplus_vooc_operations oplus_op10_ops = {
	.fw_update = op10_fw_update,
	.fw_check_then_recover = op10_fw_check_then_recover,
	.set_switch_mode = opchg_set_switch_mode,
	.eint_regist = oplus_vooc_eint_register,
	.eint_unregist = oplus_vooc_eint_unregister,
	.set_data_active = opchg_set_data_active,
	.set_data_sleep = opchg_set_data_sleep,
	.set_clock_active = opchg_set_clock_active,
	.set_clock_sleep = opchg_set_clock_sleep,
	.get_gpio_ap_data = opchg_get_gpio_ap_data,
	.read_ap_data = opchg_read_ap_data,
	.reply_mcu_data = opchg_reply_mcu_data,
	.reply_mcu_data_4bits = opchg_reply_mcu_data_4bits,
	.reset_fastchg_after_usbout = reset_fastchg_after_usbout,
	.switch_fast_chg = switch_fast_chg,
	.reset_mcu = opchg_set_reset_active,
	.set_mcu_sleep = opchg_set_reset_sleep,
	.set_vooc_chargerid_switch_val = opchg_set_vooc_chargerid_switch_val,
	.is_power_off_charging = oplus_is_power_off_charging,
	.get_reset_gpio_val = oplus_vooc_get_reset_gpio_val,
	.get_switch_gpio_val = oplus_vooc_get_switch_gpio_val,
	.get_ap_clk_gpio_val = oplus_vooc_get_ap_clk_gpio_val,
	.get_fw_version = op10_get_fw_verion_from_ic,
	.get_clk_gpio_num = opchg_get_clk_gpio_num,
	.get_data_gpio_num = opchg_get_data_gpio_num,
	.update_temperature_soc = op10_update_temperature_soc,
};

static int oplus_sm8350_pps_get_authentiate(void)
{
	return 1;
}


void oplus_op10_hardware_init(void)
{
	pps_err(" end\n");
}

void oplus_op10_cp_reset(void)
{
	pps_err(" end\n");
}

int oplus_op10_master_get_vbus(void)
{
	return op10_read_input_voltage() & 0xffff;
}

int oplus_op10_slave_get_vbus(void)
{
	return 0;
}

int oplus_op10_master_get_ibus(void)
{
	return 0;
}

int oplus_op10_slave_get_ibus(void)
{
	return 0;
}

int oplus_op10_slave_cp_enable(int enable)
{
	return 0;
}

int oplus_op10_master_get_ucp_flag(void)
{
	return 0;
}

int oplus_op10_cfg_mode_init(int mode)
{
	return 0;
}
int oplus_op10_master_get_vac(void)
{
	return 0;
}

int oplus_op10_slave_get_vac(void)
{
	return 0;
}

int oplus_op10_master_get_vout(void)
{
	return 0;
}

int oplus_op10_slave_get_vout(void)
{
	return 0;
}

void oplus_op10_set_mcu_pps_mode(bool pps)
{
	if (!the_chip) {
		printk("op10_read_vbat0_voltage fail\n");
		return;
	}
	oplus_vooc_set_mcu_pps_mode(the_chip, pps);
}

int oplus_op10_get_mcu_pps_mode(void)
{
	int ret = 0;

	if (!the_chip) {
		printk("oplus_op10_get_mcu_pps_mode fail\n");
		return -1;
	}
	ret = oplus_vooc_get_mcu_pps_mode(the_chip);

	return ret;
}


struct oplus_pps_operations oplus_op10_pps_ops = {
	.set_mcu_pps_mode = oplus_op10_set_mcu_pps_mode,
	.get_mcu_pps_mode = oplus_op10_get_mcu_pps_mode,
	.get_vbat0_volt = op10_read_vbat0_voltage,
	.check_btb_temp = op10_check_btb_temp,

	.pps_get_authentiate = oplus_sm8350_pps_get_authentiate,
	.pps_pdo_select = oplus_chg_set_pps_config,
	.get_pps_status = oplus_chg_get_pps_status,
	.get_pps_max_cur = oplus_chg_pps_get_max_cur,

	.pps_cp_hardware_init = oplus_op10_hardware_init,
	.pps_cp_reset = oplus_op10_cp_reset,
	.pps_cp_mode_init = oplus_op10_cfg_mode_init,

	.pps_mos_ctrl = op10_pps_mos_ctrl,
	.pps_get_cp_master_vbus = oplus_op10_master_get_vbus,
	.pps_get_cp_master_ibus = oplus_op10_master_get_ibus,
	.pps_get_ucp_flag = oplus_op10_master_get_ucp_flag,
	.pps_get_cp_master_vac = oplus_op10_master_get_vac,
	.pps_get_cp_master_vout = oplus_op10_master_get_vout,

	.pps_get_cp_slave_vbus = oplus_op10_slave_get_vbus,
	.pps_get_cp_slave_ibus = oplus_op10_slave_get_ibus,
	.pps_mos_slave_ctrl = oplus_op10_slave_cp_enable,
	.pps_get_cp_slave_vac = oplus_op10_slave_get_vac,
	.pps_get_cp_slave_vout = oplus_op10_slave_get_vout,
};

static void register_vooc_devinfo(void)
{
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int ret = 0;
	char *version;
	char *manufacture;

	version = "op10";
	manufacture = "SILERGY";

	ret = register_device_proc("vooc", version, manufacture);
	if (ret) {
		chg_err(" fail\n");
	}
#endif
}

static int op10_parse_fw_from_dt(struct oplus_vooc_chip *chip)
{
	struct device_node *node = chip->dev->of_node;
	const char *data;
	int len = 0;

	if (!node) {
		pr_err("device tree info. missing\n");
		return -ENOMEM;
	}

	data = of_get_property(node, "vooc,firmware_data", &len);
	if (!data) {
		pr_err("%s: parse vooc fw failed\n", __func__);
		return -ENOMEM;
	}
	pr_err("%s: parse vooc fw len: %d\n", __func__, len);

	chip->firmware_data = data;
	chip->fw_data_count = len;
	chip->fw_data_version = data[len - 4];
	pr_err("%s: version: 0x%x, count: %d\n", __func__, chip->fw_data_version, chip->fw_data_count);

	return 0;
}

static int op10_parse_fw_from_array(struct oplus_vooc_chip *chip)
{
	if (chip->batt_type_4400mv) {
		chip->firmware_data = op10_fw_data_4400_vooc_ffc_15c;
		chip->fw_data_count = sizeof(op10_fw_data_4400_vooc_ffc_15c);
		chip->fw_data_version = op10_fw_data_4400_vooc_ffc_15c[chip->fw_data_count - 4];
	} else {
		chip->firmware_data = op10_fw_data_4400_vooc_ffc_15c;
		chip->fw_data_count = sizeof(op10_fw_data_4400_vooc_ffc_15c);
		chip->fw_data_version = op10_fw_data_4400_vooc_ffc_15c[chip->fw_data_count - 4];
	}

	if (chip->vooc_fw_type == VOOC_FW_TYPE_OP10_4400_VOOC_FFC_15C) {
		chip->firmware_data = op10_fw_data_4400_vooc_ffc_15c;
		chip->fw_data_count = sizeof(op10_fw_data_4400_vooc_ffc_15c);
		chip->fw_data_version = op10_fw_data_4400_vooc_ffc_15c[chip->fw_data_count - 4];
	}

	switch (chip->vooc_fw_type) {
	case VOOC_FW_TYPE_OP10_4400_VOOC_FFC_15C:
		chip->firmware_data = op10_fw_data_4400_vooc_ffc_15c;
		chip->fw_data_count = sizeof(op10_fw_data_4400_vooc_ffc_15c);
		chip->fw_data_version = op10_fw_data_4400_vooc_ffc_15c[chip->fw_data_count - 4];
		break;
	case VOOC_FW_TYPE_OP10_4450_VOOC_FFC_5V4A_4BIT:
		chip->firmware_data = op10_fw_data_4450_vooc_ffc_5v4a_4bit;
		chip->fw_data_count = sizeof(op10_fw_data_4450_vooc_ffc_5v4a_4bit);
		chip->fw_data_version = op10_fw_data_4450_vooc_ffc_5v4a_4bit[chip->fw_data_count - 4];
		break;
	case VOOC_FW_TYPE_OP10_4250_VOOC_FFC_10V6A_4BIT:
		chip->firmware_data = op10_fw_data_4250_vooc_ffc_10v6a_4bit;
		chip->fw_data_count = sizeof(op10_fw_data_4250_vooc_ffc_10v6a_4bit);
		chip->fw_data_version = op10_fw_data_4250_vooc_ffc_10v6a_4bit[chip->fw_data_count - 4];
		break;
	case VOOC_FW_TYPE_OP10_4450_VOOC_FFC_5V6A_4BIT:
		chip->firmware_data = op10_fw_data_4450_vooc_ffc_5v6a_4bit;
		chip->fw_data_count = sizeof(op10_fw_data_4450_vooc_ffc_5v6a_4bit);
		chip->fw_data_version = op10_fw_data_4450_vooc_ffc_5v6a_4bit[chip->fw_data_count - 4];
	default:
		break;
	}

	return 0;
}

static void op10_shutdown(struct i2c_client *client)
{
	if (!the_chip) {
		return;
	}
	opchg_set_switch_mode(the_chip, NORMAL_CHARGER_MODE);
	msleep(10);
	if (oplus_vooc_get_fastchg_started() == true) {
		opchg_set_clock_sleep(the_chip);
		msleep(10);
		opchg_set_reset_active(the_chip);
	}
	msleep(80);
	return;
}

static ssize_t vooc_fw_check_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	char page[256] = {0};
	char read_data[32] = {0};
	int len = 0;

	if (the_chip && the_chip->vooc_fw_check == true) {
		read_data[0] = '1';
	} else {
		read_data[0] = '0';
	}
	len = sprintf(page, "%s", read_data);
	if (len > *off) {
		len -= *off;
	} else {
		len = 0;
	}
	if (copy_to_user(buff, page, (len < count ? len : count))) {
		return -EFAULT;
	}
	*off += len < count ? len : count;
	return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static const struct file_operations vooc_fw_check_proc_fops = {
	.read = vooc_fw_check_read,
	.llseek = noop_llseek,
};
#else
static const struct proc_ops vooc_fw_check_proc_fops = {
	.proc_read = vooc_fw_check_read,
	.proc_lseek = noop_llseek,
};
#endif
static int init_proc_vooc_fw_check(void)
{
	struct proc_dir_entry *p = NULL;

	p = proc_create("vooc_fw_check", 0444, NULL, &vooc_fw_check_proc_fops);
	if (!p) {
		chg_err("proc_create vooc_fw_check_proc_fops fail!\n");
	}
	return 0;
}

static int op10_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct oplus_vooc_chip *chip;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&client->dev, "Couldn't allocate memory\n");
		return -ENOMEM;
	}

	chip->client = client;
	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

#ifdef CONFIG_OPLUS_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
	client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	gpDMABuf_va = (u8 *)dma_alloc_coherent(&client->dev, GTP_DMA_MAX_TRANSACTION_LENGTH, &gpDMABuf_pa, GFP_KERNEL);
	if (!gpDMABuf_va) {
		chg_err("[Error] Allocate DMA I2C Buffer failed!\n");
	} else {
		chg_debug(" ppp dma_alloc_coherent success\n");
	}
	memset(gpDMABuf_va, 0, GTP_DMA_MAX_TRANSACTION_LENGTH);
#endif
#endif
	if (get_vooc_mcu_type(chip) != OPLUS_VOOC_ASIC_HWID_OP10) {
		chg_err("It is not op10\n");
		return -ENOMEM;
	}

	chip->pcb_version = g_hw_version;
	chip->vooc_fw_check = false;
	mutex_init(&chip->pinctrl_mutex);

	oplus_vooc_fw_type_dt(chip);

	if (chip->parse_fw_from_dt)
		op10_parse_fw_from_dt(chip);
	else
		op10_parse_fw_from_array(chip);

	oplus_pps_ops_register("mcu-op10", &oplus_op10_pps_ops);

	chip->vops = &oplus_op10_ops;
	chip->fw_mcu_version = 0;

	oplus_vooc_gpio_dt_init(chip);

	opchg_set_clock_sleep(chip);
	oplus_vooc_delay_reset_mcu_init(chip);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	op10_update_wake_lock = wakeup_source_register("op10_update_wake_lock");
#else
	op10_update_wake_lock = wakeup_source_register(NULL, "op10_update_wake_lock");
#endif
	if (chip->vooc_fw_update_newmethod) {
		if (oplus_is_rf_ftm_mode()) {
			oplus_vooc_fw_update_work_init(chip);
		}
	} else {
		oplus_vooc_fw_update_work_init(chip);
	}

	oplus_vooc_init(chip);

	register_vooc_devinfo();

	init_proc_vooc_fw_check();

	oplus_vooc_bcc_curves_init(chip);

	the_chip = chip;
	chg_debug("op10 success\n");
	return 0;
}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
static const struct of_device_id op10_match[] = {
	{ .compatible = "oplus,op10-fastcg"},
	{ .compatible = "oplus,sy6610-fastcg"},
	{},
};

static const struct i2c_device_id op10_id[] = {
	{"op10-fastcg", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, op10_id);

struct i2c_driver op10_i2c_driver = {
	.driver = {
		.name = "op10-fastcg",
		.owner = THIS_MODULE,
		.of_match_table = op10_match,
	},
	.probe = op10_driver_probe,
	.shutdown = op10_shutdown,
	.id_table = op10_id,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
static int __init op10_subsys_init(void)
#else
void op10_subsys_exit(void)
{
	i2c_del_driver(&op10_i2c_driver);
}

int op10_subsys_init(void)
#endif
{
	int ret = 0;
	chg_debug(" init start\n");
	init_hw_version();

	if (i2c_add_driver(&op10_i2c_driver) != 0) {
		chg_err(" failed to register op10 i2c driver.\n");
	} else {
		chg_debug(" Success to register op10 i2c driver.\n");
	}
	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
subsys_initcall(op10_subsys_init);
#endif
MODULE_DESCRIPTION("Driver for oplus vooc op10 fast mcu");
MODULE_LICENSE("GPL v2");

