/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: log_stream.c
** Description: add logkit stream identify
**
** Version: 1.0
** Date : 2022/10/13
** Author: Zhangpeng
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** Zhangpeng 2022/10/13 1.0 build this module
****************************************************************/

#ifndef OPLUS_KERNEL_NET_LOG_STREAM_H
#define OPLUS_KERNEL_NET_LOG_STREAM_H

int set_system_uid(u32 uid);

int log_stream_init(void);
void log_stream_fini(void);

#endif  /* OPLUS_KERNEL_NET_LOG_STREAM_H */
