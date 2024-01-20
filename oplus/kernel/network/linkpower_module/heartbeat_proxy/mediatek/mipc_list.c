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

#include "mipc_list.h"

void mipc_list_init(mipc_list_t *head_ptr)
{
    head_ptr->prev_ptr = head_ptr;
    head_ptr->next_ptr = head_ptr;
}

void mipc_list_add_tail(mipc_list_t *head_ptr, mipc_list_t *list_ptr)
{
    head_ptr->prev_ptr->next_ptr = list_ptr;
    list_ptr->next_ptr = head_ptr;
    list_ptr->prev_ptr = head_ptr->prev_ptr;
    head_ptr->prev_ptr = list_ptr;
}

void mipc_list_add_head(mipc_list_t *head_ptr, mipc_list_t *list_ptr)
{
    head_ptr->next_ptr->prev_ptr = list_ptr;
    list_ptr->next_ptr = head_ptr->next_ptr;
    list_ptr->prev_ptr = head_ptr;
    head_ptr->next_ptr = list_ptr;
}

void mipc_list_del(mipc_list_t *list_ptr)
{
    list_ptr->next_ptr->prev_ptr = list_ptr->prev_ptr;
    list_ptr->prev_ptr->next_ptr = list_ptr->next_ptr;
}
