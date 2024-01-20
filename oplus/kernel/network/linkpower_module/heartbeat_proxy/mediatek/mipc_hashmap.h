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

#ifndef _MIPC_HASHMAP_H_
#define _MIPC_HASHMAP_H_

#include "platform.h"
#include "mipc_list.h"

#define MAX_MIPC_HASHMAP_SIZE (32)

typedef uint8_t (*mipc_hashmap_hash_func)(const void *key_ptr, const uint16_t key_len);

typedef struct {
    uint8_t size;
    uint16_t count;
    mipc_hashmap_hash_func hash_func;
} mipc_hashmap_t;
#define MIPC_HASHMAP_T_SIZE (MIPC_ALIGN(sizeof(mipc_hashmap_t)))

typedef struct {
    mipc_list_t list;
    mipc_hashmap_t *hashmap_ptr;
    uint8_t *key_ptr;
    uint8_t *val_ptr;
    uint16_t key_len;
    uint16_t val_len;
} mipc_hashmap_element_t;
#define MIPC_HASHMAP_ELEMENT_T_SIZE (MIPC_ALIGN(sizeof(mipc_hashmap_element_t)))

typedef void (*MIPC_HASHMAP_ITERATE_CB)(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr);

//create a new hashmap
mipc_hashmap_t *mipc_hashmap_init(uint8_t size, mipc_hashmap_hash_func hash_func);

//clean the hashmap
void mipc_hashmap_clean(mipc_hashmap_t *hashmap_ptr);

//release the hashmap
void mipc_hashmap_deinit(mipc_hashmap_t *hashmap_ptr);

void mipc_hashmap_iterate(mipc_hashmap_t *hashmap_ptr, MIPC_HASHMAP_ITERATE_CB cb, void *cb_priv_ptr);

//add a key and value pair into the hashmap
mipc_hashmap_element_t *mipc_hashmap_add(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len, const void *val_ptr, const uint16_t val_len);
mipc_hashmap_element_t *mipc_hashmap_add2(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len, const void *t_val_ptr, const uint16_t t_val_len, const void *b_val_ptr, const uint16_t b_val_len);

//delete hashmap_element
int32_t mipc_hashmap_del(mipc_hashmap_element_t *hashmap_element_ptr);

//get the value by a key from the hashmap
mipc_hashmap_element_t *mipc_hashmap_get(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len);

#define MIPC_HASHMAP_GET_KEY_LEN(e) ((e) ? (e)->key_len : 0)
#define MIPC_HASHMAP_GET_KEY_PTR(e) ((e) ? (e)->key_ptr : NULL)
#define MIPC_HASHMAP_GET_VAL_LEN(e) ((e) ? (e)->val_len : 0)
#define MIPC_HASHMAP_GET_VAL_PTR(e) ((e) ? (e)->val_ptr : NULL)

#endif
