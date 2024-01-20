// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022-2023 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SC8547A_H_
#define _OPLUS_SC8547A_H_

#define UFCS_BTB_TEMP_MAX 80
#define UFCS_USB_TEMP_MAX 80
#define UFCS_BTB_USB_OVER_CNTS 9

#define UFCS_UCT_POWER_VOLT 5000
#define UFCS_UCT_POWER_CURR 1000

#define UFCS_HARDRESET_RETRY_CNTS 3
#define UFCS_HANDSHAKE_RETRY_CNTS 1

/************************Timer***********************************/
#define UFCS_HANDSHAKE_TIMEOUT (300)
#define UFCS_PING_TIMEOUT (300)
#define UFCS_ACK_TIMEOUT (50)
#define UFCS_MSG_TIMEOUT (300)
#define UFCS_POWER_RDY_TIMEOUT (550)
/********************** I2C Slave Addr **************************/
#define SC8547A_I2C_ADDR (0x6F)
#define SC8547A_MAX_REG (0x0264)
#define SC8547A_FLAG_NUM (3)

/********************** SC2201 I2C reg map **********************/
#define SC8547A_ENABLE_REG_NUM (6)

#define SC8547A_ADDR_DEVICE_ID (0x00)
#define SC8547A_ADDR_DEVICE_ID1 (0x01)

#define SC8547A_ADDR_DPDM_CTRL (0x21)
#define SC8547A_CMD_DPDM_EN (0x01)

#define SC8547A_ADDR_OTG_EN (0x3B)

#define SC8547A_ADDR_UFCS_CTRL0 (0x40)
#define SEND_SOURCE_HARDRESET BIT(0)
#define SEND_CABLE_HARDRESET BIT(1)
#define FLAG_BAUD_RATE_VALUE (BIT(4) | BIT(3))
#define FLAG_BAUD_NUM_SHIFT (3)
#define SC8547A_CMD_EN_CHIP (0x80)
#define SC8547A_CMD_DIS_CHIP (0X00)
#define SC8547A_MASK_EN_HANDSHAKE BIT(5)
#define SC8547A_CMD_EN_HANDSHAKE BIT(5)

#define SC8547A_ADDR_GENERAL_INT_FLAG1 (0x41)
#define SC8547A_CMD_MASK_ACK_DISCARD (0x80)

#define SC8547A_ADDR_GENERAL_INT_FLAG2 (0x42)
#define SC8547A_FLAG_ACK_RECEIVE_TIMEOUT BIT(0)
#define SC8547A_FLAG_HARD_RESET BIT(1)
#define SC8547A_FLAG_DATA_READY BIT(2)
#define SC8547A_FLAG_SENT_PACKET_COMPLETE BIT(3)
#define SC8547A_FLAG_CRC_ERROR BIT(4)
#define SC8547A_FLAG_BAUD_RATE_ERROR BIT(5)
#define SC8547A_FLAG_HANDSHAKE_SUCCESS BIT(6)
#define SC8547A_FLAG_HANDSHAKE_FAIL BIT(7)

#define SC8547A_ADDR_GENERAL_INT_FLAG3 (0x43)
#define SC8547A_FLAG_TRAINING_BYTE_ERROR BIT(0)
#define SC8547A_FLAG_MSG_TRANS_FAIL BIT(1)
#define SC8547A_FLAG_DATA_BYTE_TIMEOUT BIT(3)
#define SC8547A_FLAG_BAUD_RATE_CHANGE BIT(4)
#define SC8547A_FLAG_LENGTH_ERROR BIT(5)
#define SC8547A_FLAG_RX_OVERFLOW BIT(6)
#define SC8547A_FLAG_BUS_CONFLICT BIT(7)

#define SC8547A_ADDR_UFCS_INT_MASK0 (0x44)
#define SC8547A_CMD_MASK_ACK_TIMEOUT (0x01)

#define SC8547A_ADDR_UFCS_INT_MASK1 (0x45)
#define SC8547A_MASK_TRANING_BYTE_ERROR (0x01)

/*tx_buffer*/
#define SC8547A_ADDR_TX_LENGTH (0x47)

#define SC8547A_ADDR_TX_BUFFER0 (0x48)

#define SC8547A_ADDR_TX_BUFFER35 (0x6B)

/*rx_buffer*/
#define SC8547A_ADDR_RX_LENGTH (0x6C)
#define SC8547A_ADDR_RX_BUFFER0 (0x6D)
#define SC8547A_ADDR_RX_BUFFER124 (0xE9)
#define SC8547A_LEN_MAX 64

#define SC8547A_ADDR_TXRX_BUFFER_CTRL (0xEA)
#define SC8547A_CMD_CLR_TX_RX (0xC0)

#define SC8547A_DEVICE_ID 0x67

/*********************command buffer**************/
/*avoid to rewrite the baudrate flags*/
#define SC8547A_CMD_SND_CMP BIT(2)
#define SC8547A_MASK_SND_CMP BIT(2)

/****************Message Construction Helper*********/
#define UFCS_HEADER_REV 0x01
#define UFCS_KEY_VER 0x01
#define OPLUS_DEV_VENDOR 0x22d9
#define OPLUS_IC_VENDOR 0x0

#define SC8547A_VENDOR_ID 0x0006
#define SC8547A_HARDWARE_VER 0x0001
#define SC8547A_SOFTWARE_VER 0x0001
#define DATA_LENGTH_BLANK 0x0
#define UCT_MODE_WATCHDOG 3000
#define UCT_MODE_IC_VEND 0xFFFF
#define UCT_MODE_DEV_VEND 0xFFFF

#define ABNORMAL_HARDWARE_VER_A_MIN 0x1006
#define ABNORMAL_HARDWARE_VER_A_MAX 0x1008
#define ABNORMAL_SOFTWARE_VER_A 0x0006
#define ABNORMAL_IC_VEND 0x0
#define ABNORMAL_DEV_VEND 0x0

#define VND_LENGTH_BLANK 0x2

#define UFCS_HEADER_DEV_TYPE_SHIFT 13
#define UFCS_HEADER_DEV_TYPE_MASK 0x7
#define UFCS_HEADER_MSG_ID_SHIFT 9
#define UFCS_HEADER_MSG_ID_MASK 0xF
#define UFCS_HEADER_REV_SHIFT 3
#define UFCS_HEADER_REV_MASK 0x3F
#define UFCS_HEADER_MSG_TYPE_SHIFT 0
#define UFCS_HEADER_MSG_TYPE_MASK 0x7

#define UFCS_HEADER(dev_type, msg_seq, msg_type)                               \
	((u16)(((dev_type)&UFCS_HEADER_DEV_TYPE_MASK)                          \
	       << UFCS_HEADER_DEV_TYPE_SHIFT) |                                \
	 (((msg_seq)&UFCS_HEADER_MSG_ID_MASK) << UFCS_HEADER_MSG_ID_SHIFT) |   \
	 (((UFCS_HEADER_REV)&UFCS_HEADER_REV_MASK) << UFCS_HEADER_REV_SHIFT) | \
	 (((msg_type)&UFCS_HEADER_MSG_TYPE_MASK)                               \
	  << UFCS_HEADER_MSG_TYPE_SHIFT))

#define UFCS_HEADER_ADDR(header) (header >> 13)
#define UFCS_HEADER_ID(header) ((header >> 9) & 0x0F)
#define UFCS_HEADER_VER(header) ((header >> 3) & 0x3F)
#define UFCS_HEADER_TYPE(header) (header & 0x07)

/*Protocol Data Length*/
#define COM_DATA_HEADER_POS 0
#define COM_DATA_CMD_TYPE_POS 2
#define COM_DATA_DATA_SIZE_POS 3
#define COM_DATA_DATA_POS 4

#define COM_HEADER_LEN 2
#define COM_CMD_ID_LEN 1
#define COM_VENDER_ID_LEN 2
#define COM_DATA_SIZE_LEN 1

#define COM_FIXED_FIELD_LEN                                                    \
	(COM_HEADER_LEN + COM_CMD_ID_LEN + COM_DATA_SIZE_LEN)

#define WRT_DATA_BGN_POS 4

#define READ_DATA_LEN_POS 3
#define READ_DATA_BGN_POS 4

#define READ_VENDER_LEN_POS 4
#define READ_VENDER_BGN_POS 5

#define MAX_RCV_BUFFER_SIZE 124
#define MAX_TX_BUFFER_SIZE 35
#define MAX_TX_MSG_SIZE (MAX_TX_BUFFER_SIZE - COM_FIXED_FIELD_LEN)

#define REQUEST_PDO_LENGTH 8
#define SINK_INFO_LENGTH 8
#define CONFIG_WTD_LENGTH 2
#define REFUSE_MSG_LENGTH 4
#define DEVICE_INFO_LENGTH 8
#define VERIFY_REQUEST_LENGTH 17
#define PHONE_RANDOM_LENGTH 16
#define VERIFY_RESPONSE_LENGTH 48
#define ADAPTER_RANDOM_LENGTH 16
#define ADAPTER_ENCRYPT_LENGTH 32
#define REQUEST_POWER_LENGTH 3

#define ATTRIBUTE_NOT_SUPPORTED 0
/***************************message parse helper**************************/
#define SWAP64(val)                                                            \
	((((val)&0xff00000000000000) >> 56) |                                  \
	 (((val)&0x00ff000000000000) >> 40) |                                  \
	 (((val)&0x0000ff0000000000) >> 24) |                                  \
	 (((val)&0x000000ff00000000) >> 8) |                                   \
	 (((val)&0x00000000ff000000) << 8) |                                   \
	 (((val)&0x0000000000ff0000) << 24) |                                  \
	 (((val)&0x000000000000ff00) << 40) |                                  \
	 (((val)&0x00000000000000ff) << 56))

#define SWAP32(val)                                                            \
	((((val)&0xff000000) >> 24) | (((val)&0x00ff0000) >> 8) |              \
	 (((val)&0x0000ff00) << 8) | (((val)&0x000000ff) << 24))

#define SWAP16(val) ((((val)&0xff00) >> 8) | (((val)&0x00ff) << 8))

#define POWER_EXCHANGE_GET_VOLT(val)                                           \
	(((val)&0x0000ff0000) >> 8) | (((val)&0x00ff000000) >> 24)

#define POWER_EXCHANGE_GET_CURR(val)                                           \
	(((val)&0x00000000ff) << 8) | (((val)&0x000000ff00) >> 8)

#define PDO_LENGTH 8
#define POWER_CHANGE_LENGTH 3
#define MAX_PDO_NUM 7
#define MIN_PDO_NUM 1
#define ERROR_INFO_NUM 16
#define ERROR_INFO_LEN 4
#define VALID_ERROR_INFO_BITS 0x7fff

const char *adapter_error_info[16] = {
	"adapter output OVP!",
	"adapter outout UVP!",
	"adapter output OCP!",
	"adapter output SCP!",
	"adapter USB OTP!",
	"adapter inside OTP!",
	"adapter CCOVP!",
	"adapter D-OVP!",
	"adapter D+OVP!",
	"adapter input OVP!",
	"adapter input UVP!",
	"adapter drain over current!",
	"adapter input current loss!",
	"adapter CRC error!",
	"adapter watchdog timeout!",
	"invalid msg!",
};

#endif /*_OPLUS_SC8547A_H_*/
