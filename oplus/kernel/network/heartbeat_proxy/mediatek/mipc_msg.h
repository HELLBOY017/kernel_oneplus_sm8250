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

#ifndef _MIPC_MSG_H_
#define _MIPC_MSG_H_

#include "platform.h"
#include "mipc.h"
#include "mipc_hashmap.h"

typedef enum MIPC_MSG_enum mipc_msg_id_enum;
typedef enum mipc_result_const_enum mipc_result_enum;

#define MIPC_VERSION (0x00000002)

#define MIPC_SIM0   (1 << 0)
#define MIPC_SIM1   (1 << 1)
#define MIPC_SIM2   (1 << 2)
#define MIPC_SIM3   (1 << 3)
#define MIPC_PS_START (4)
#define MIPC_PS0    (1 << 4)
#define MIPC_PS1    (1 << 5)
#define MIPC_PS2    (1 << 6)
#define MIPC_PS3    (1 << 7)
#define MIPC_ALL    (0xff)

#define MIPC_MSG_TYPE_MASK              (0xC000)
#define MIPC_MSG_TYPE_REQ_CNF           (0x0000)
#define MIPC_MSG_TYPE_IND               (0x4000)
#define MIPC_MSG_TYPE_CMD_RSP           (0x8000)
#define MIPC_MSG_TYPE_NTF               (0xC000)


/*!
    \brief MIPC MSG API result
*/

#define MAX_T_NUM (0x7fff)
#define MIPC_USIR_BIT (0x8000)

typedef enum {
    MIPC_MSG_API_RESULT_SUCCESS = 0,    /*!< API result is SUCCESS */
    MIPC_MSG_API_RESULT_FAIL = 1        /*!< API result is FAIL */
} mipc_msg_api_result_enum;

typedef enum {
    MIPC_MSG_SIM0 = MIPC_SIM0,
    MIPC_MSG_SIM1 = MIPC_SIM1,
    MIPC_MSG_SIM2 = MIPC_SIM2,
    MIPC_MSG_SIM3 = MIPC_SIM3,
    MIPC_MSG_PS0 = MIPC_PS0,
    MIPC_MSG_PS1 = MIPC_PS1,
    MIPC_MSG_PS2 = MIPC_PS2,
    MIPC_MSG_PS3 = MIPC_PS3,
    MIPC_MSG_ALL = MIPC_ALL
} mipc_msg_sim_ps_id_enum;

#pragma pack (push)
#pragma pack (1)

#define MIPC_MSG_HDR_FLAG_RESERVE0  (1 << 0)
#define MIPC_MSG_HDR_FLAG_RESERVE1  (1 << 1)
#define MIPC_MSG_HDR_FLAG_RESERVE2  (1 << 2)
#define MIPC_MSG_HDR_FLAG_RESERVE3  (1 << 3)
#define MIPC_MSG_HDR_FLAG_RESERVE4  (1 << 4)
#define MIPC_MSG_HDR_FLAG_RESERVE5  (1 << 5)
#define MIPC_MSG_HDR_FLAG_RESERVE6  (1 << 6)
#define MIPC_MSG_HDR_FLAG_MORE      (1 << 7)

typedef struct {
    uint32_t magic;
    uint16_t padding[2];
    uint8_t msg_sim_ps_id; //sim_ps_id (bit-wise)
    uint8_t msg_flag;
    uint16_t msg_id; //type[2] + id[14]
    uint16_t msg_txid;
    uint16_t msg_len;
} mipc_msg_hdr_t;

#define MIPC_MSG_TLV_HDR \
    uint16_t type; \
    uint16_t len

typedef struct {
    MIPC_MSG_TLV_HDR;
} mipc_msg_tlv_t;

#define MIPC_MSG_ARRAY_VAL \
    uint32_t reserved; /* always 0 (for futrue use)*/ \
    uint32_t reserved2; /* always 0 (for futrue use)*/ \
    uint16_t array_base_t; \
    uint16_t array_size

typedef struct {
    MIPC_MSG_TLV_HDR;
    MIPC_MSG_ARRAY_VAL;
} mipc_msg_tlv_array_t;
#pragma pack (pop)

typedef struct {
    /** @brief message header*/
    mipc_msg_hdr_t hdr;

    /** @brief message payload len*/
    uint16_t tlvs_len;
    /** @brief message payload (TLVs)*/
    mipc_hashmap_t *tlvs_ptr;

    //for array
    uint16_t array_free_t;
} mipc_msg_t;

typedef void (*MIPC_MSG_ITERATE_CB)(mipc_msg_tlv_t *tlv_ptr, void *cb_priv_ptr);

#define mipc_msg_add_tlv8(m, t, v_ptr) mipc_msg_add_tlv(m, t, 1, v_ptr)
#define mipc_msg_add_tlv16(m, t, v_ptr) mipc_msg_add_tlv(m, t, 2, v_ptr)
#define mipc_msg_add_tlv32(m, t, v_ptr) mipc_msg_add_tlv(m, t, 4, v_ptr)
#define mipc_msg_add_tlv_int8(m, t, v) {int8_t tmp = v; mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}
#define mipc_msg_add_tlv_int16(m, t, v) {int16_t tmp = HTOMS(v); mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}
#define mipc_msg_add_tlv_int32(m, t, v) {int32_t tmp = HTOML(v); mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}
#define mipc_msg_add_tlv_uint8(m, t, v) {uint8_t tmp = v; mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}
#define mipc_msg_add_tlv_uint16(m, t, v) {uint16_t tmp = HTOMS(v); mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}
#define mipc_msg_add_tlv_uint32(m, t, v) {uint32_t tmp = HTOML(v); mipc_msg_add_tlv(m, t, sizeof(tmp), &tmp);}

#define mipc_msg_add_idx8(m, a, i, v_ptr) mipc_msg_add_idx(m, a, i, 1, v_ptr)
#define mipc_msg_add_idx16(m, a, i, v_ptr) mipc_msg_add_idx(m, a, i, 2, v_ptr)
#define mipc_msg_add_idx32(m, a, i, v_ptr) mipc_msg_add_idx(m, a, i, 4, v_ptr)
#define mipc_msg_add_idx_int8(m, a, i, v) {int8_t tmp = v; mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}
#define mipc_msg_add_idx_int16(m, a, i, v) {int16_t tmp = HTOMS(v); mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}
#define mipc_msg_add_idx_int32(m, a, i, v) {int32_t tmp = HTOML(v); mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}
#define mipc_msg_add_idx_uint8(m, a, i, v) {uint8_t tmp = v; mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}
#define mipc_msg_add_idx_uint16(m, a, i, v) {uint16_t tmp = HTOMS(v); mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}
#define mipc_msg_add_idx_uint32(m, a, i, v) {uint32_t tmp = HTOML(v); mipc_msg_add_idx(m, a, i, sizeof(tmp), &tmp);}

#define MIPC_MSG_GET_TLV_TYPE(tlv_ptr) ((tlv_ptr) ? (tlv_ptr)->type : 0)
#define MIPC_MSG_GET_TLV_LEN(tlv_ptr) ((tlv_ptr) ? (tlv_ptr)->len : 0)
#define MIPC_MSG_GET_TLV_VAL_PTR(tlv_ptr) ((void *)(((uint8_t *)tlv_ptr) + sizeof(mipc_msg_tlv_t)))

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT mipc_msg_t *mipc_msg_init(
    uint16_t msg_id,
    mipc_msg_sim_ps_id_enum sim_ps_id
);

DLL_EXPORT mipc_msg_t *mipc_msg_init_with_txid(
    uint16_t msg_id,
    mipc_msg_sim_ps_id_enum sim_ps_id,
    uint16_t msg_txid
);

//only copy message header
mipc_msg_t *mipc_msg_copy_hdr(
    mipc_msg_t *msg_ptr
);

//init rsp message with CMD
DLL_EXPORT mipc_msg_t *mipc_msg_init_rsp_with_msgid(
    mipc_msg_t *msg_ptr,
    uint16_t msg_id
);

//only copy message TLVs
mipc_msg_api_result_enum mipc_msg_copy_tlv(
    mipc_msg_t *src_msg_ptr,
    mipc_msg_t *dst_msg_ptr
);

uint8_t is_valid_mipc_msg_txid(uint16_t txid);

DLL_EXPORT void mipc_msg_deinit(
    mipc_msg_t *msg_ptr
);

DLL_EXPORT mipc_msg_api_result_enum mipc_msg_add_tlv(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    uint16_t len,
    const void *val_ptr
);

//for 1st level array
#define mipc_msg_add_tlv_array(m, t, s) mipc_msg_add_array(m, NULL, 0, t, s)

DLL_EXPORT mipc_msg_tlv_array_t *mipc_msg_add_array(
    mipc_msg_t *msg_ptr,
    mipc_msg_tlv_array_t *array_tlv_ptr,  //can be NULL if it's a first dimension
    uint16_t idx,  //shouild be 0 if array_tlv_ptr is NULL
    uint16_t type, //only valid if array_tlv_ptr is NULL
    uint16_t array_size
);

DLL_EXPORT mipc_msg_api_result_enum mipc_msg_add_idx(
    mipc_msg_t *msg_ptr,
    mipc_msg_tlv_array_t *array_tlv_ptr,
    uint16_t idx,
    uint16_t len,
    const void *val_ptr
);

mipc_msg_tlv_t *mipc_msg_get_tlv(
    mipc_msg_t *msg_ptr,
    uint16_t type
);

uint16_t mipc_msg_get_tlv_num(
    mipc_msg_t *msg_ptr,
    uint16_t type
);

mipc_msg_tlv_t *mipc_msg_get_idx(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    uint16_t idx
);

DLL_EXPORT void *mipc_msg_get_val_ptr(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    uint16_t *val_len_ptr
);

DLL_EXPORT char *mipc_msg_get_val_str(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    uint16_t *val_len_ptr
);

DLL_EXPORT void *mipc_msg_get_val_struct(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    int size,
    int count
);

DLL_EXPORT char *mipc_msg_cpy_str_ptr(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    char *buf,
    uint16_t buf_len
);

DLL_EXPORT void *mipc_msg_cpy_val_ptr(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    void *buf,
    uint16_t buf_len
);

DLL_EXPORT void *mipc_msg_get_idx_ptr(
    mipc_msg_t *msg_ptr,
    uint16_t type,
    uint16_t idx,
    uint16_t *val_len_ptr
);

#define mipc_msg_get_val_int8(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? *(int8_t *)mipc_msg_get_val_ptr(m, t, NULL) : def_val)
#define mipc_msg_get_val_int16(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? MTOHS(*(int16_t *)mipc_msg_get_val_ptr(m, t, NULL)) : def_val)
#define mipc_msg_get_val_int32(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? MTOHL(*(int32_t *)mipc_msg_get_val_ptr(m, t, NULL)) : def_val)
#define mipc_msg_get_val_uint8(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? *(uint8_t *)mipc_msg_get_val_ptr(m, t, NULL) : def_val)
#define mipc_msg_get_val_uint16(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? MTOHS(*(uint16_t *)mipc_msg_get_val_ptr(m, t, NULL)) : def_val)
#define mipc_msg_get_val_uint32(m, t, def_val) ((mipc_msg_get_val_ptr(m, t, NULL)) ? MTOHL(*(uint32_t *)mipc_msg_get_val_ptr(m, t, NULL)) : def_val)
//
#define mipc_msg_get_idx_int8(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? *(int8_t *)mipc_msg_get_idx_ptr(m, t, i, NULL) : def_val)
#define mipc_msg_get_idx_int16(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? MTOHS(*(int16_t *)mipc_msg_get_idx_ptr(m, t, i, NULL)) : def_val)
#define mipc_msg_get_idx_int32(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? MTOHL(*(int32_t *)mipc_msg_get_idx_ptr(m, t, i, NULL)) : def_val)
#define mipc_msg_get_idx_uint8(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? *(uint8_t *)mipc_msg_get_idx_ptr(m, t, i, NULL) : def_val)
#define mipc_msg_get_idx_uint16(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? MTOHS(*(uint16_t *)mipc_msg_get_idx_ptr(m, t, i, NULL)) : def_val)
#define mipc_msg_get_idx_uint32(m, t, i, def_val) ((mipc_msg_get_idx_ptr(m, t, i, NULL)) ? MTOHL(*(uint32_t *)mipc_msg_get_idx_ptr(m, t, i, NULL)) : def_val)

//
DLL_EXPORT void mipc_msg_iterate(mipc_msg_t *msg_ptr, MIPC_MSG_ITERATE_CB cb, void *cb_priv_ptr);
//
DLL_EXPORT uint8_t *mipc_msg_serialize(mipc_msg_t *msg_ptr, uint16_t *msg_len_ptr);
DLL_EXPORT uint8_t *mipc_msg_usir_serialize(mipc_msg_t *msg_ptr, uint16_t *msg_len_ptr, uint8_t context_usir_support);
DLL_EXPORT mipc_msg_t *mipc_msg_deserialize(uint8_t *msg_raw_ptr, uint16_t msg_raw_len);
//
DLL_EXPORT int32_t mipc_msg_compare(mipc_msg_t *msg_ptr1, mipc_msg_t *msg_ptr2);
//
int32_t ind_msg_id_to_bit(mipc_msg_id_enum msg_id);
int32_t cmd_msg_id_to_bit(mipc_msg_id_enum msg_id);

#ifdef __cplusplus
}
#endif

#endif
