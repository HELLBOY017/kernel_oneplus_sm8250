/***********************************************************
** Copyright (C), 2008-2021, oplus Mobile Comm Corp., Ltd.
** File: oplus_comm_def.h
** Description: common defines
**
** Version: 1.0
** Date : 2022/6/23
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/6/23 1.0 build this module
****************************************************************/


#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#define LOG(tag, fmt, args...) \
	do { \
		printk("[%-10s][%-5u] " fmt "\n", tag, __LINE__, ##args); \
	} while (0)


#endif  /* __COMMON_DEF_H__ */
