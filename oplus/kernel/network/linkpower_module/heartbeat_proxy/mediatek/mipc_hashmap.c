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

#include "mipc_hashmap.h"

static uint8_t _mipc_hashmap_idx(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len)
{
    int i;
    uint8_t idx;

    //compute hash value
    if (hashmap_ptr->hash_func == NULL) {
        uint32_t tmp = 0;
        for (i = 0; i < key_len; i++) {
            tmp += *((uint8_t *)key_ptr + i) + i;
        }
        idx = tmp % hashmap_ptr->size;
    } else {
        idx = hashmap_ptr->hash_func(key_ptr, key_len) % hashmap_ptr->size;
    }

    return idx;
}

mipc_hashmap_t *mipc_hashmap_init(uint8_t size, mipc_hashmap_hash_func hash_func)
{
    int32_t i;
    uint8_t *ptr;
    mipc_hashmap_t *hashmap_ptr;
    mipc_list_t *head_ptr;

    if (size == 0 || size > MAX_MIPC_HASHMAP_SIZE) {
        return NULL;
    }

    //allocate mipc_hashmap_t + padding (aligned) + (mipc_list_t * size)
    if ((ptr = (uint8_t *)ALLOC(MIPC_HASHMAP_T_SIZE + (sizeof(mipc_list_t) * size))) == NULL) {
        return NULL;
    }

    hashmap_ptr = (mipc_hashmap_t *)ptr;
    head_ptr = (mipc_list_t *)(ptr + MIPC_HASHMAP_T_SIZE);
    for (i = 0; i < size; i++) {
        mipc_list_init(&head_ptr[i]);
    }
    hashmap_ptr->size = size;
    hashmap_ptr->count = 0;
    hashmap_ptr->hash_func = hash_func;

    return hashmap_ptr;
}

void mipc_hashmap_clean_cb(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr)
{
    mipc_hashmap_del(hashmap_element_ptr);
}

void mipc_hashmap_clean(mipc_hashmap_t *hashmap_ptr)
{
    mipc_hashmap_iterate(hashmap_ptr, mipc_hashmap_clean_cb, NULL);
}

void mipc_hashmap_deinit(mipc_hashmap_t *hashmap_ptr)
{
    mipc_hashmap_clean(hashmap_ptr);

    FREE(hashmap_ptr);
}

void mipc_hashmap_iterate(mipc_hashmap_t *hashmap_ptr, MIPC_HASHMAP_ITERATE_CB cb, void *cb_priv_ptr)
{
    int32_t i;
    uint8_t *ptr;
    mipc_list_t *head_ptr;
    mipc_list_t *list_ptr;
    mipc_hashmap_element_t *hashmap_element_ptr;

    ptr = (uint8_t *)hashmap_ptr;
    head_ptr = (mipc_list_t *)(ptr + MIPC_HASHMAP_T_SIZE);
    for (i = 0; i < hashmap_ptr->size; i++) {
        list_ptr = head_ptr[i].next_ptr;
        while (list_ptr != &head_ptr[i]) {
            hashmap_element_ptr = mipc_list_entry(list_ptr, mipc_hashmap_element_t, list);
            list_ptr = list_ptr->next_ptr;
            cb(hashmap_element_ptr, cb_priv_ptr);
        }
    }
}

mipc_hashmap_element_t *mipc_hashmap_add(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len, const void *val_ptr, const uint16_t val_len)
{
    return mipc_hashmap_add2(hashmap_ptr, key_ptr, key_len, NULL, 0, val_ptr, val_len);
}

mipc_hashmap_element_t *mipc_hashmap_add2(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len, const void *t_val_ptr, const uint16_t t_val_len, const void *b_val_ptr, const uint16_t b_val_len)
{
    uint8_t *ptr;
    mipc_list_t *head_ptr;
    mipc_hashmap_element_t *hashmap_element_ptr;

    //if b_val_ptr == NULL or b_val_len == 0, no hashmap entry is created
    if (hashmap_ptr == NULL || key_ptr == NULL || key_len == 0 || b_val_ptr == NULL || b_val_len == 0) {
        return NULL;
    }
    if (t_val_len != 0 && t_val_ptr == NULL) {
        return NULL;
    }

    ptr = (uint8_t *)hashmap_ptr;
    head_ptr = &((mipc_list_t *)(ptr + MIPC_HASHMAP_T_SIZE))[_mipc_hashmap_idx(hashmap_ptr, key_ptr, key_len)];

    //check if it is already exist
    if (mipc_hashmap_get(hashmap_ptr, key_ptr, key_len)) {
        return NULL;
    }

    //allocate mipc_hashmap_element_t + padding (aligned) + key + padding (aligned) + val
    if ((ptr = (uint8_t *)ALLOC(MIPC_HASHMAP_ELEMENT_T_SIZE + MIPC_ALIGN(key_len) + t_val_len + b_val_len)) == NULL) {
        return NULL;
    }

    hashmap_element_ptr = (mipc_hashmap_element_t *)ptr;
    mipc_list_init(&hashmap_element_ptr->list);
    hashmap_element_ptr->hashmap_ptr = hashmap_ptr;
    hashmap_element_ptr->key_ptr = (uint8_t *)(ptr + MIPC_HASHMAP_ELEMENT_T_SIZE);
    //val_ptr is always not NULL
    hashmap_element_ptr->val_ptr = (uint8_t *)(hashmap_element_ptr->key_ptr + MIPC_ALIGN(key_len));
    hashmap_element_ptr->key_len = key_len;
    hashmap_element_ptr->val_len = t_val_len + b_val_len;
    MEMCPY(hashmap_element_ptr->key_ptr, key_ptr, key_len);
    if (t_val_len) {
        MEMCPY(hashmap_element_ptr->val_ptr, t_val_ptr, t_val_len);
    }
    MEMCPY(hashmap_element_ptr->val_ptr + t_val_len, b_val_ptr, b_val_len);

    mipc_list_add_tail(head_ptr, &hashmap_element_ptr->list);

    hashmap_ptr->count++;

    return hashmap_element_ptr;
}

int32_t mipc_hashmap_del(mipc_hashmap_element_t *hashmap_element_ptr)
{
    if (hashmap_element_ptr == NULL || hashmap_element_ptr->hashmap_ptr == NULL) {
        return -1;
    }

    mipc_list_del(&hashmap_element_ptr->list);
    hashmap_element_ptr->hashmap_ptr->count--;

    FREE(hashmap_element_ptr);

    return 0;
}

mipc_hashmap_element_t *mipc_hashmap_get(mipc_hashmap_t *hashmap_ptr, const void *key_ptr, const uint16_t key_len)
{
    uint8_t *p;
    mipc_list_t *head_ptr;
    mipc_hashmap_element_t *hashmap_element_ptr;
    mipc_list_t *list_ptr;

    if (hashmap_ptr == NULL || key_ptr == NULL || key_len == 0) {
        return NULL;
    }

    p = (uint8_t *)hashmap_ptr;
    head_ptr = &((mipc_list_t *)(p + MIPC_HASHMAP_T_SIZE))[_mipc_hashmap_idx(hashmap_ptr, key_ptr, key_len)];

    list_ptr = head_ptr->next_ptr;
    while (list_ptr != head_ptr) {
        hashmap_element_ptr = mipc_list_entry(list_ptr, mipc_hashmap_element_t, list);
        if (hashmap_element_ptr->key_len == key_len && MEMCMP(hashmap_element_ptr->key_ptr, key_ptr, hashmap_element_ptr->key_len) == 0) {
            return hashmap_element_ptr;
        }
        list_ptr = list_ptr->next_ptr;
    }

    return NULL;
}
