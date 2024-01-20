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

#include "platform.h"
#include "mipc_hashmap.h"
#include "mipc_msg.h"

//should be protected by MUTEX
static uint32_t internal_mipc_msg_txid(void)
{
#ifndef UNIT_TEST
    static uint32_t msg_txid = 0;
    return msg_txid++;
#else
    return 0x7788;
#endif
}

mipc_msg_t *internal_mipc_msg_init(void)
{
    mipc_msg_t *msg_ptr = (mipc_msg_t *)ALLOC(sizeof(mipc_msg_t));

    if (msg_ptr == NULL) {
        return NULL;
    }
    msg_ptr->tlvs_len = 0;
    msg_ptr->tlvs_ptr = mipc_hashmap_init(8, NULL);
    //for array
    msg_ptr->array_free_t = MAX_T_NUM;

    return msg_ptr;
}

mipc_msg_t *mipc_msg_init(uint16_t msg_id, mipc_msg_sim_ps_id_enum sim_ps_id)
{
    mipc_msg_t *msg_ptr = internal_mipc_msg_init();

    if (msg_ptr == NULL) {
        return NULL;
    }
    MEMSET(&msg_ptr->hdr, 0, sizeof(msg_ptr->hdr));
    msg_ptr->hdr.magic = HTOML(0x24541984);
    msg_ptr->hdr.msg_sim_ps_id = sim_ps_id;
    msg_ptr->hdr.msg_id = HTOMS(msg_id);
    if ((msg_id & MIPC_MSG_TYPE_MASK) == MIPC_MSG_TYPE_IND || (msg_id & MIPC_MSG_TYPE_MASK) == MIPC_MSG_TYPE_NTF) {
        msg_ptr->hdr.msg_txid = 0xFFFF;
    } else {
        msg_ptr->hdr.msg_txid = HTOMS(internal_mipc_msg_txid());
    }

    return msg_ptr;
}

mipc_msg_t *mipc_msg_init_with_txid(uint16_t msg_id, mipc_msg_sim_ps_id_enum sim_ps_id, uint16_t msg_txid)
{
    mipc_msg_t *msg_ptr = internal_mipc_msg_init();

    if (msg_ptr == NULL) {
        return NULL;
    }
    MEMSET(&msg_ptr->hdr, 0, sizeof(msg_ptr->hdr));
    msg_ptr->hdr.magic = HTOML(0x24541984);
    msg_ptr->hdr.msg_sim_ps_id = sim_ps_id;
    msg_ptr->hdr.msg_id = HTOMS(msg_id);
    msg_ptr->hdr.msg_txid = HTOMS(msg_txid);

    return msg_ptr;
}

mipc_msg_t *mipc_msg_init_rsp_with_msgid(mipc_msg_t *msg_ptr, uint16_t msg_id)
{
    mipc_msg_t *rsp_msg_ptr = mipc_msg_copy_hdr(msg_ptr);
    rsp_msg_ptr->hdr.msg_id = HTOMS(msg_id);

    return rsp_msg_ptr;
}

mipc_msg_t *mipc_msg_copy_hdr(mipc_msg_t *msg_ptr)
{
    mipc_msg_t *msg_ptr2;

    if (msg_ptr == NULL) {
        return NULL;
    }

    msg_ptr2 = internal_mipc_msg_init();

    if (msg_ptr2 == NULL) {
        return NULL;
    }
    msg_ptr2->hdr = msg_ptr->hdr;

    return msg_ptr2;
}

typedef struct {
    int32_t ret;
    mipc_msg_t *dst_msg_ptr;
} mipc_msg_copy_cb_info_t;

static void mipc_msg_copy_cb(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr)
{
    mipc_msg_copy_cb_info_t *info_ptr = (mipc_msg_copy_cb_info_t *)cb_priv_ptr;
    mipc_msg_tlv_t *tlv_ptr = (mipc_msg_tlv_t *)MIPC_HASHMAP_GET_VAL_PTR(hashmap_element_ptr);

    if (mipc_msg_add_tlv(info_ptr->dst_msg_ptr, MIPC_MSG_GET_TLV_TYPE(tlv_ptr), MIPC_MSG_GET_TLV_LEN(tlv_ptr), MIPC_MSG_GET_TLV_VAL_PTR(tlv_ptr)) != MIPC_MSG_API_RESULT_SUCCESS) {
        info_ptr->ret = 1;
    }
}

mipc_msg_api_result_enum mipc_msg_copy_tlv(mipc_msg_t *src_msg_ptr, mipc_msg_t *dst_msg_ptr)
{
    mipc_msg_copy_cb_info_t info;

    //compare TLV content
    info.ret = 0;
    info.dst_msg_ptr = dst_msg_ptr;
    mipc_hashmap_iterate(src_msg_ptr->tlvs_ptr, mipc_msg_copy_cb, &info);

    if (info.ret) {
        return MIPC_MSG_API_RESULT_FAIL;
    } else {
        return MIPC_MSG_API_RESULT_SUCCESS;
    }
}

void mipc_msg_deinit(mipc_msg_t *msg_ptr)
{
    if (msg_ptr == NULL) {
        return;
    }

    mipc_hashmap_deinit(msg_ptr->tlvs_ptr);

    FREE(msg_ptr);
}

mipc_msg_api_result_enum mipc_msg_add_tlv(mipc_msg_t *msg_ptr, uint16_t type, uint16_t len, const void *val_ptr)
{
    mipc_msg_tlv_t tlv;
    uint16_t val_len;
    uint16_t tlv_type;

    if (msg_ptr == NULL || val_ptr == NULL) {
        return MIPC_MSG_API_RESULT_FAIL;
    }

    tlv.type = type;
    if (len == 0) { //it's an array)
        tlv.len = 0;
        val_len = sizeof(mipc_msg_tlv_array_t) - sizeof(mipc_msg_tlv_t);
    } else {
        tlv.len = len;
        val_len = len;
    }

    tlv_type = type & ~MIPC_USIR_BIT; //clear USIR
    if (mipc_hashmap_add2(msg_ptr->tlvs_ptr, &tlv_type, sizeof(tlv_type), &tlv, sizeof(tlv), val_ptr, val_len)) {
        msg_ptr->tlvs_len += MIPC_ALIGN(sizeof(tlv) + val_len);
    }

    return MIPC_MSG_API_RESULT_SUCCESS;
}


mipc_msg_tlv_array_t *mipc_msg_add_array(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array_tlv_ptr, uint16_t idx, uint16_t type, uint16_t array_size)
{
    mipc_hashmap_element_t *hashmap_element_ptr;
    mipc_msg_tlv_array_t array_tlv;

    if (msg_ptr == NULL) return NULL;

    if (array_tlv_ptr == NULL) { //first dimension
        if (idx != 0 || array_size == 0) return NULL;

        array_tlv.type = type;
    } else { //other dimension
        if (type != 0 || array_tlv_ptr->len != 0 || array_tlv_ptr->reserved != 0 || array_tlv_ptr->reserved2 != 0 || idx >= array_tlv_ptr->array_size) return NULL;

        array_tlv.type = array_tlv_ptr->array_base_t + idx;
    }

    array_tlv.len = 0;
    array_tlv.reserved = 0;
    array_tlv.reserved2 = 0;
    if (array_tlv.type & MIPC_USIR_BIT) {
        array_tlv.array_base_t = (msg_ptr->array_free_t - (array_size - 1)) | MIPC_USIR_BIT;
    } else {
        array_tlv.array_base_t = msg_ptr->array_free_t - (array_size - 1);
    }
    array_tlv.array_size = array_size;

    msg_ptr->array_free_t = (array_tlv.array_base_t & ~MIPC_USIR_BIT) - 1;
    if ((hashmap_element_ptr = mipc_hashmap_add(msg_ptr->tlvs_ptr, &array_tlv.type, sizeof(array_tlv.type), &array_tlv, sizeof(array_tlv))) != NULL) {
        msg_ptr->tlvs_len += MIPC_ALIGN(sizeof(array_tlv));
        return (mipc_msg_tlv_array_t *)hashmap_element_ptr->val_ptr;
    }
    return NULL;
}

mipc_msg_api_result_enum mipc_msg_add_idx(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array_tlv_ptr, uint16_t idx, uint16_t len, const void *val_ptr)
{
    uint16_t type;

    if (msg_ptr == NULL || array_tlv_ptr == NULL || array_tlv_ptr->len != 0 || array_tlv_ptr->reserved != 0 || array_tlv_ptr->reserved2 != 0 || idx >= array_tlv_ptr->array_size) {
        return MIPC_MSG_API_RESULT_FAIL;
    }


    //compute type number
    type = array_tlv_ptr->array_base_t + idx;

    return mipc_msg_add_tlv(msg_ptr, type, len, val_ptr);
}

mipc_msg_tlv_t *mipc_msg_get_tlv(mipc_msg_t *msg_ptr, uint16_t type)
{
    mipc_hashmap_element_t *hashmap_element_ptr;
    type = type & ~MIPC_USIR_BIT; //clear USIR

    hashmap_element_ptr = mipc_hashmap_get(msg_ptr->tlvs_ptr, &type, sizeof(type));

    if (hashmap_element_ptr == NULL) {
        return NULL;
    }

    return (mipc_msg_tlv_t *)MIPC_HASHMAP_GET_VAL_PTR(hashmap_element_ptr);
}

uint16_t mipc_msg_get_tlv_num(mipc_msg_t *msg_ptr, uint16_t type)
{
    mipc_msg_tlv_array_t *array_tlv_ptr = (mipc_msg_tlv_array_t *)mipc_msg_get_tlv(msg_ptr, type);
    uint16_t i = 0;

    if(array_tlv_ptr == NULL) {
        return 0;
    }
    //for 1-item no_array_format case
    if(array_tlv_ptr->len != 0) {
        return 1;
    }
    if (array_tlv_ptr->reserved != 0 || array_tlv_ptr->reserved2 != 0) {
        return 0;
    }
    for(i=0; i<array_tlv_ptr->array_size;i++) {
        if(NULL == mipc_msg_get_tlv(msg_ptr, array_tlv_ptr->array_base_t + i)) {
            return i;
        }
    }
    return array_tlv_ptr->array_size;
}

mipc_msg_tlv_t *mipc_msg_get_idx(mipc_msg_t *msg_ptr, uint16_t type, uint16_t idx)
{
    mipc_msg_tlv_array_t *array_tlv_ptr = (mipc_msg_tlv_array_t *)mipc_msg_get_tlv(msg_ptr, type);

    if(array_tlv_ptr == NULL) {
        return NULL;
    }
    //for 1-item no_array_format case
    if(array_tlv_ptr->len != 0) {
        return (mipc_msg_tlv_t*)array_tlv_ptr;
    }
    if (array_tlv_ptr->reserved != 0 || array_tlv_ptr->reserved2 != 0 || idx >= array_tlv_ptr->array_size) {
        return NULL;
    }

    return mipc_msg_get_tlv(msg_ptr, array_tlv_ptr->array_base_t + idx);
}

void *mipc_msg_get_val_ptr(mipc_msg_t *msg_ptr, uint16_t type, uint16_t *val_len_ptr)
{
    mipc_msg_tlv_t *tlv_ptr;
    void *val_ptr = NULL;

    tlv_ptr = mipc_msg_get_tlv(msg_ptr, type);
    if (tlv_ptr) {
        val_ptr = ((uint8_t *)tlv_ptr) + sizeof(mipc_msg_tlv_t);
        if (val_len_ptr) {
            *val_len_ptr = tlv_ptr->len;
        }
    } else {
        if (val_len_ptr) {
            *val_len_ptr = 0;
        }
    }
    return val_ptr;
}

//val_len_ptr include '\0'
char *mipc_msg_get_val_str(mipc_msg_t *msg_ptr, uint16_t type, uint16_t *val_len_ptr)
{
    mipc_msg_tlv_t *tlv_ptr;
    char *val_ptr = NULL;

    tlv_ptr = mipc_msg_get_tlv(msg_ptr, type);
    if (tlv_ptr) {
        val_ptr = ((char *)tlv_ptr) + sizeof(mipc_msg_tlv_t);
        //workaround
        if (val_ptr[tlv_ptr->len - 1] != '\0') {
            //if (tlv_ptr->len & (MIPC_ALIGN_SIZE - 1)) {
            //    //have padding space can be used
            //    tlv_ptr->len++;
            //}
            val_ptr[tlv_ptr->len - 1] = '\0';
        }
        if (val_len_ptr) {
            *val_len_ptr = tlv_ptr->len;
        }
    } else {
        if (val_len_ptr) {
            *val_len_ptr = 0;
        }
    }
    return val_ptr;
}

void *mipc_msg_get_idx_ptr(mipc_msg_t *msg_ptr, uint16_t type, uint16_t idx, uint16_t *val_len_ptr)
{
    mipc_msg_tlv_t *tlv_ptr;
    void *val_ptr = NULL;

    tlv_ptr = mipc_msg_get_idx(msg_ptr, type, idx);
    if (tlv_ptr) {
        val_ptr = ((uint8_t *)tlv_ptr) + sizeof(mipc_msg_tlv_t);
        if (val_len_ptr) {
            *val_len_ptr = tlv_ptr->len;
        }
    } else {
        if (val_len_ptr) {
            *val_len_ptr = 0;
        }
    }
    return val_ptr;
}

typedef struct {
    MIPC_MSG_ITERATE_CB cb;
    void *cb_priv_ptr;
} mipc_msg_iterate_cb_info_t;

static void mipc_msg_iterate_cb(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr)
{
    mipc_msg_iterate_cb_info_t *info_ptr = (mipc_msg_iterate_cb_info_t *)cb_priv_ptr;
    mipc_msg_tlv_t *tlv_ptr = (mipc_msg_tlv_t *)MIPC_HASHMAP_GET_VAL_PTR(hashmap_element_ptr);

    info_ptr->cb(tlv_ptr, info_ptr->cb_priv_ptr);
}

void mipc_msg_iterate(mipc_msg_t *msg_ptr, MIPC_MSG_ITERATE_CB cb, void *cb_priv_ptr)
{
    mipc_msg_iterate_cb_info_t info;

    if (cb == NULL) {
        return;
    }

    info.cb = cb;
    info.cb_priv_ptr = cb_priv_ptr;

    mipc_hashmap_iterate(msg_ptr->tlvs_ptr, mipc_msg_iterate_cb, &info);
}

void mipc_msg_serialize_cb(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr)
{
    uint8_t **tail_pptr = (uint8_t **)cb_priv_ptr;
    uint16_t tlv_len;
    mipc_msg_tlv_t *tlv_ptr = (mipc_msg_tlv_t *)MIPC_HASHMAP_GET_VAL_PTR(hashmap_element_ptr);

    if (tlv_ptr == NULL || tail_pptr == NULL || *tail_pptr == NULL) return;

    if (tlv_ptr->len == 0) { //it's an array {
        tlv_len = sizeof(mipc_msg_tlv_array_t);
    } else {
        tlv_len = sizeof(mipc_msg_tlv_t) + tlv_ptr->len;
    }

    MEMCPY(*tail_pptr, tlv_ptr, tlv_len);

    *tail_pptr += MIPC_ALIGN(tlv_len);
}

uint8_t *mipc_msg_serialize(mipc_msg_t *msg_ptr, uint16_t *msg_len_ptr)
{
    uint8_t *head_ptr;
    uint8_t *tail_ptr;
    uint16_t msg_len;

    if (msg_ptr == NULL || msg_len_ptr == NULL) {
        return NULL;
    }

    msg_len = sizeof(mipc_msg_hdr_t) + msg_ptr->tlvs_len;

    if ((head_ptr = tail_ptr = (uint8_t *)ALLOC(msg_len)) == NULL) {
        return NULL;
    }
    memset(head_ptr, 0, msg_len);

    msg_ptr->hdr.msg_len = msg_ptr->tlvs_len;

    //fill msg header
    MEMCPY(tail_ptr, &msg_ptr->hdr, sizeof(msg_ptr->hdr));
    tail_ptr += sizeof(mipc_msg_hdr_t);
    //fill msg payload
    mipc_hashmap_iterate(msg_ptr->tlvs_ptr, mipc_msg_serialize_cb, &tail_ptr);

    *msg_len_ptr = tail_ptr - head_ptr;
    //check
    if (msg_len != *msg_len_ptr) {
        FREE(head_ptr);
        return NULL;
    }
    return head_ptr;
}

mipc_msg_t *mipc_msg_deserialize(uint8_t *msg_raw_ptr, uint16_t msg_raw_len)
{
    mipc_msg_t *msg_ptr;
    uint8_t *ptr;
    uint8_t *tail_ptr;

    if (msg_raw_ptr == NULL || msg_raw_len < sizeof(mipc_msg_hdr_t)) {
        return NULL;
    }

    if ((msg_ptr = internal_mipc_msg_init()) == NULL) {
        return NULL;
    }
    memcpy(&msg_ptr->hdr, msg_raw_ptr, sizeof(msg_ptr->hdr));

    ptr = (uint8_t *)msg_raw_ptr;
    tail_ptr = ptr + msg_raw_len;
    //skip header
    ptr += sizeof(mipc_msg_hdr_t);
    while (ptr < tail_ptr) {
        mipc_msg_tlv_t *tlv_ptr;
        uint16_t tlv_len;
        //process type and len
        if ((tail_ptr - ptr) < sizeof(mipc_msg_tlv_t)) {
            //something error
            mipc_msg_deinit(msg_ptr);
            return NULL;
        }
        tlv_ptr = (mipc_msg_tlv_t *)ptr;
        if (tlv_ptr->len == 0) { //it's an array
            mipc_msg_tlv_array_t *array_tlv_ptr = (mipc_msg_tlv_array_t *)tlv_ptr;
            tlv_len = sizeof(mipc_msg_tlv_array_t) - sizeof(mipc_msg_tlv_t);
            if (array_tlv_ptr->array_base_t <= msg_ptr->array_free_t) {
                msg_ptr->array_free_t = array_tlv_ptr->array_base_t - 1;
            }
        } else {
            tlv_len = tlv_ptr->len;
        }
        ptr += sizeof(mipc_msg_tlv_t);
        //process val
        if ((tail_ptr - ptr) < tlv_len) {
            //something error
            mipc_msg_deinit(msg_ptr);
            return NULL;
        }
        if (tlv_ptr->len == 0) { //it's an array
            mipc_msg_add_tlv(msg_ptr, tlv_ptr->type, 0, ptr);
        } else {
            mipc_msg_add_tlv(msg_ptr, tlv_ptr->type, tlv_len, ptr);
        }
        ptr = ((uint8_t *)tlv_ptr) + MIPC_ALIGN(sizeof(mipc_msg_tlv_t) + tlv_len);
    }

    return msg_ptr;
}

typedef struct {
    int32_t ret;
    mipc_msg_t *msg_ptr;
    mipc_msg_t *other_msg_ptr;
} mipc_msg_compare_cb_info_t;

static void mipc_msg_compare_cb(mipc_hashmap_element_t *hashmap_element_ptr, void *cb_priv_ptr)
{
    mipc_msg_compare_cb_info_t *info_ptr = (mipc_msg_compare_cb_info_t *)cb_priv_ptr;
    mipc_hashmap_element_t *other_hashmap_element_ptr;
    mipc_msg_tlv_t *tlv_ptr = (mipc_msg_tlv_t *)MIPC_HASHMAP_GET_VAL_PTR(hashmap_element_ptr);

    if (tlv_ptr == NULL) return;
    if (tlv_ptr->type > info_ptr->msg_ptr->array_free_t) return;

    other_hashmap_element_ptr = mipc_hashmap_get(info_ptr->other_msg_ptr->tlvs_ptr, hashmap_element_ptr->key_ptr, hashmap_element_ptr->key_len);

    if (other_hashmap_element_ptr != NULL) {
        if (tlv_ptr->len == 0) { //it's an array
            int i;
            mipc_msg_tlv_array_t *a_ptr1 = (mipc_msg_tlv_array_t *)tlv_ptr;
            mipc_msg_tlv_array_t *a_ptr2 = (mipc_msg_tlv_array_t *)MIPC_HASHMAP_GET_VAL_PTR(other_hashmap_element_ptr);
            if (a_ptr1->array_size != a_ptr2->array_size) {
                info_ptr->ret = -1;
                return;
            }
            for (i = 0; i < a_ptr1->array_size; i++) {
                mipc_msg_tlv_t *t_ptr1 = mipc_msg_get_tlv(info_ptr->msg_ptr, a_ptr1->array_base_t + i);
                mipc_msg_tlv_t *t_ptr2 = mipc_msg_get_tlv(info_ptr->other_msg_ptr, a_ptr2->array_base_t + i);

                if (t_ptr1 != NULL && t_ptr2 == NULL) {
                    info_ptr->ret = -1;
                    return;
                } else if (t_ptr1 == NULL && t_ptr2 != NULL) {
                    info_ptr->ret = -1;
                    return;
                } else if (t_ptr1 != NULL && t_ptr2 != NULL) {
                    if (t_ptr1->len != t_ptr2->len || memcmp(MIPC_MSG_GET_TLV_VAL_PTR(t_ptr1), MIPC_MSG_GET_TLV_VAL_PTR(t_ptr2), t_ptr1->len)) {
                        info_ptr->ret = -1;
                        return;
                    }
                }
            }
        } else {
            if (hashmap_element_ptr->val_len != other_hashmap_element_ptr->val_len || memcmp(hashmap_element_ptr->val_ptr, other_hashmap_element_ptr->val_ptr, hashmap_element_ptr->val_len)) {
                info_ptr->ret = -1;
                return;
            }
        }
    } else {
        info_ptr->ret = -1;
        return;
    }
}

int32_t mipc_msg_compare(mipc_msg_t *msg_ptr1, mipc_msg_t *msg_ptr2)
{
    mipc_msg_compare_cb_info_t info;

    if (msg_ptr1 == NULL || msg_ptr2 == NULL) {
        return -1;
    }

    //compare MSG header
    if (msg_ptr1->hdr.msg_len == 0 && msg_ptr1->tlvs_len != 0) {
        msg_ptr1->hdr.msg_len = msg_ptr1->tlvs_len;
    }
    if (msg_ptr2->hdr.msg_len == 0 && msg_ptr2->tlvs_len != 0) {
        msg_ptr2->hdr.msg_len = msg_ptr2->tlvs_len;
    }
    if (memcmp(&msg_ptr1->hdr, &msg_ptr2->hdr, sizeof(msg_ptr1->hdr))) {
        //printf("header diff\n");
        return -1;
    }

    //compare TLV count
    if (msg_ptr1->tlvs_ptr->count != msg_ptr2->tlvs_ptr->count) {
        //printf("TLV count diff\n");
        return -1;
    }

    //compare TLV content
    info.ret = 0;
    info.msg_ptr = msg_ptr1;
    info.other_msg_ptr = msg_ptr2;
    mipc_hashmap_iterate(msg_ptr1->tlvs_ptr, mipc_msg_compare_cb, &info);

    return info.ret;
}
