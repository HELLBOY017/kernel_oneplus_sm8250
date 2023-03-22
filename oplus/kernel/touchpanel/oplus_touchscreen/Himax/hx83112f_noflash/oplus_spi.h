/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __MTK_SPI_H__
#define __MTK_SPI_H__


#include <linux/types.h>
#include <linux/io.h>
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
//#include <mobicore_driver_api.h>
//#include <tlspi_Api.h>
#endif
/*******************************************************************************
* define struct for spi driver
********************************************************************************/
enum spi_sample_sel {
	POSEDGE,
	NEGEDGE
};
enum spi_cs_pol {
	ACTIVE_LOW,
	ACTIVE_HIGH
};

enum spi_cpol {
	SPI_CPOL_0,
	SPI_CPOL_1
};

enum spi_cpha {
	SPI_CPHA_0,
	SPI_CPHA_1
};

enum spi_mlsb {
	SPI_LSB,
	SPI_MSB
};

enum spi_endian {
	SPI_LENDIAN,
	SPI_BENDIAN
};

enum spi_transfer_mode {
	FIFO_TRANSFER,
	DMA_TRANSFER,
	OTHER1,
	OTHER2,
};

enum spi_pause_mode {
	PAUSE_MODE_DISABLE,
	PAUSE_MODE_ENABLE
};
enum spi_finish_intr {
	FINISH_INTR_DIS,
	FINISH_INTR_EN,
};

enum spi_deassert_mode {
	DEASSERT_DISABLE,
	DEASSERT_ENABLE
};

enum spi_ulthigh {
	ULTRA_HIGH_DISABLE,
	ULTRA_HIGH_ENABLE
};

enum spi_tckdly {
	TICK_DLY0,
	TICK_DLY1,
	TICK_DLY2,
	TICK_DLY3
};

struct mt_chip_conf {
	u32 setuptime;
	u32 holdtime;
	u32 high_time;
	u32 low_time;
	u32 cs_idletime;
	u32 ulthgh_thrsh;
	enum spi_sample_sel sample_sel;
	enum spi_cs_pol cs_pol;
	enum spi_cpol cpol;
	enum spi_cpha cpha;
	enum spi_mlsb tx_mlsb;
	enum spi_mlsb rx_mlsb;
	enum spi_endian tx_endian;
	enum spi_endian rx_endian;
	enum spi_transfer_mode com_mod;
	enum spi_pause_mode pause;
	enum spi_finish_intr finish_intr;
	enum spi_deassert_mode deassert;
	enum spi_ulthigh ulthigh;
	enum spi_tckdly tckdly;
};

//void secspi_enable_clk(struct spi_device *spidev);
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
//int secspi_session_open(u32 spinum);
//int secspi_execute(u32 cmd, tciSpiMessage_t *param);
#endif

#endif
