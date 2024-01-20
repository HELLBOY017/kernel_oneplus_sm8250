/*******************************************************************************************
* Copyright (c) 2023 - 2029 OPLUS Mobile Comm Corp., Ltd.
*
* File: fp_netlink.c
* Description: fix non-normalized fp compile fail, add netlink module to uff-fp-driver
* Version: 1.0
* Date : 2023-10-20
* Author:  tengyuhang
** -----------------------------Revision History: -----------------------
**  <author>      <date>            <desc>
**  Yuhang.teng   2023/10/20        create the file
*********************************************************************************************/

#ifndef __FP_NETLINK_H
#define __FP_NETLINK_H

// netlink funciton
int  fp_netlink_init(void);
void fp_netlink_exit(void);
void fp_sendnlmsg(int module, int event, void *data,
                  unsigned int size);

#endif /* __FP_FAULT_INJECT_H */
