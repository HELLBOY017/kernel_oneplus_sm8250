/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  Copyright (c) [2020], MediaTek Inc. All rights reserved.
*  This software/firmware and related documentation ("MediaTek Software") are
*  protected under relevant copyright laws.
*
*  The information contained herein is confidential and proprietary to
*  MediaTek Inc. and/or its licensors. Except as otherwise provided in the
*  applicable licensing terms with MediaTek Inc. and/or its licensors, any
*  reproduction, modification, use or disclosure of MediaTek Software, and
*  information contained herein, in whole or in part, shall be strictly
*  prohibited.
*****************************************************************************/

#ifndef _MIPC_LIST_H_
#define _MIPC_LIST_H_

#include "platform.h"

typedef struct mipc_list {
    struct mipc_list *next_ptr;
    struct mipc_list *prev_ptr;
} mipc_list_t;

#define MIPC_LIST_INIT(name) {&(name), &(name)}

void mipc_list_init(mipc_list_t *head_ptr);
void mipc_list_add_tail(mipc_list_t *head_ptr, mipc_list_t *list_ptr);
void mipc_list_add_head(mipc_list_t *head_ptr, mipc_list_t *list_ptr);
void mipc_list_del(mipc_list_t *list_ptr);

//
#define mipc_list_entry(ptr, type, member) ((type *)((unsigned char *)(ptr)-POINTER_TO_UINT(&((type *)0)->member)))

#endif
