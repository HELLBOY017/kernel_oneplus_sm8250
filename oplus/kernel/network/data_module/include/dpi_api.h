/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: dpi_api.h
** Description: Add dpi interface
**
** Version: 1.0
** Date : 2022/6/24
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/6/24 1.0 build this module
****************************************************************/

#ifndef __DPI_API_H__
#define __DPI_API_H__


#define DPI_ID_APP_MASK 0xFFFF0000
#define DPI_ID_FUNC_MASK 0xFFFFFF00
#define DPI_IP_STREAM_MASK 0xFFFFFFFF
#define DPI_ID_UID_MASK 0xFFFFFFFF00000000

#define DPI_ID_UID_BIT_OFFSET 32


#define DPI_ID_TMGP_SGAME_APP  0x10000
#define DPI_ID_TMGP_SGAME_FUNC_GAME  0x10100
#define DPI_ID_TMGP_SGAME_STREAM_GAME_DATA  0x10101

#define DPI_ID_HEYTAP_MARKET_APP	0x20000
#define DPI_ID_HEYTAP_MARKET_FUNC_DOWNLOAD	0x20100
#define DPI_ID_HEYTAP_MARKET_STREAM_DOWNLOAD_DATA	0x20101

#define DPI_ID_SYSTEM_APP 0x30000
#define DPI_ID_LOG_KIT_FUNC 0x30100
#define DPI_ID_LOG_KIT_STREAM_DATA 0x30101

enum dpi_type_e {
	DPI_TYPE_UNSPEC,
	DPI_TYPE_UID,
	DPI_TYPE_APP,
	DPI_TYPE_FUNC,
	DPI_TYPE_STREAM,
	DPI_TYPE_MAX,
};



typedef int (*dpi_notify_fun)(u64 dpi_id, int startStop);


int dpi_register_result_notify(u64 dpi_id, dpi_notify_fun fun);
int dpi_unregister_result_notify(u64 dpi_id);

u64 get_skb_dpi_id(struct sk_buff *skb, int dir, int in_dev);

#endif /* __DPI_API_H__ */
