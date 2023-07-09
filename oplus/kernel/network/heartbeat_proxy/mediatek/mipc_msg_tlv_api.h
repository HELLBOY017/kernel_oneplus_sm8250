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

#ifndef __MIPC_MSG_TLV_API_H__
#define __MIPC_MSG_TLV_API_H__

#include "mipc_msg_tlv_const.h"
#include "mipc_msg.h"


static inline mipc_result_enum mipc_get_result(mipc_msg_t *msg_ptr)
{
    if (!msg_ptr) return MIPC_RESULT_FAILURE;
    return (mipc_result_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_T_RESULT, MIPC_RESULT_FAILURE);
}

static inline mipc_sys_cap_cellular_class_const_enum mipc_sys_get_info_cnf_get_sys_cellular_class(mipc_msg_t *msg_ptr, mipc_sys_cap_cellular_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_cellular_class_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SYS_CELLULAR_CLASS, def_val);
}

static inline mipc_sys_voice_class_const_enum mipc_sys_get_info_cnf_get_voice_class(mipc_msg_t *msg_ptr, mipc_sys_voice_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_voice_class_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_VOICE_CLASS, def_val);
}

static inline mipc_sys_sim_class_const_enum mipc_sys_get_info_cnf_get_sim_class(mipc_msg_t *msg_ptr, mipc_sys_sim_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_sim_class_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SIM_CLASS, def_val);
}

static inline mipc_sys_cap_data_const_enum mipc_sys_get_info_cnf_get_data_class(mipc_msg_t *msg_ptr, mipc_sys_cap_data_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_data_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_DATA_CLASS, def_val);
}

static inline mipc_sys_cap_gsm_band_const_enum mipc_sys_get_info_cnf_get_gsm_band_class(mipc_msg_t *msg_ptr, mipc_sys_cap_gsm_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_gsm_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_GSM_BAND_CLASS, def_val);
}

static inline mipc_sys_cap_umts_band_const_enum mipc_sys_get_info_cnf_get_umts_band_class(mipc_msg_t *msg_ptr, mipc_sys_cap_umts_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_umts_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_UMTS_BAND_CLASS, def_val);
}

static inline mipc_sys_cap_tds_band_const_enum mipc_sys_get_info_cnf_get_tds_band_class(mipc_msg_t *msg_ptr, mipc_sys_cap_tds_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_tds_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_TDS_BAND_CLASS, def_val);
}

static inline mipc_sys_cap_cdma_band_const_enum mipc_sys_get_info_cnf_get_c2k_band_class(mipc_msg_t *msg_ptr, mipc_sys_cap_cdma_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_cdma_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_C2K_BAND_CLASS, def_val);
}

static inline mipc_sys_lte_band_struct4* mipc_sys_get_info_cnf_get_lte_band_class(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_lte_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_LTE_BAND_CLASS, val_len_ptr);
}

static inline mipc_sys_nr_band_struct4* mipc_sys_get_info_cnf_get_nr_band_class(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_nr_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_NR_BAND_CLASS, val_len_ptr);
}

static inline mipc_sys_cap_sms_const_enum mipc_sys_get_info_cnf_get_sms_caps(mipc_msg_t *msg_ptr, mipc_sys_cap_sms_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_sms_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SMS_CAPS, def_val);
}

static inline mipc_sys_cap_ctrl_const_enum mipc_sys_get_info_cnf_get_ctrl_caps(mipc_msg_t *msg_ptr, mipc_sys_cap_ctrl_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_ctrl_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_CTRL_CAPS, def_val);
}

static inline mipc_sys_auth_algo_const_enum mipc_sys_get_info_cnf_get_auth_algo_caps(mipc_msg_t *msg_ptr, mipc_sys_auth_algo_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_auth_algo_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_AUTH_ALGO_CAPS, def_val);
}

static inline mipc_sys_cap_service_const_enum mipc_sys_get_info_cnf_get_service_caps(mipc_msg_t *msg_ptr, mipc_sys_cap_service_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_service_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SERVICE_CAPS, def_val);
}

static inline mipc_sys_modem_struct8* mipc_sys_get_info_cnf_get_multi_md(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_modem_struct8*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_MULTI_MD, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_device_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_DEVICE_ID, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_manufctr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_MANUFCTR, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_firmware(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_FIRMWARE, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_hardware(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_HARDWARE, val_len_ptr);
}

static inline uint16_t mipc_sys_get_info_cnf_get_max_active_ctxt(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_MAX_ACTIVE_CTXT, def_val);
}

static inline uint16_t mipc_sys_get_info_cnf_get_executor_idx(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_EXECUTOR_IDX, def_val);
}

static inline char * mipc_sys_get_info_cnf_get_custom_class_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_CUSTOM_CLASS_NAME, val_len_ptr);
}

static inline mipc_sys_product_type_const_enum mipc_sys_get_info_cnf_get_product_type(mipc_msg_t *msg_ptr, mipc_sys_product_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_product_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_PRODUCT_TYPE, def_val);
}

static inline char * mipc_sys_get_info_cnf_get_esn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_ESN, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_imeisv(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_IMEISV, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_meid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_MEID, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_firmware_custom(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_FIRMWARE_CUSTOM, val_len_ptr);
}

static inline void * mipc_sys_get_info_cnf_get_hardware_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_HARDWARE_ID, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_serial_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SERIAL_NUMBER, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_project_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_PROJECT_NAME, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_flavor_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_FLAVOR_NAME, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_esn_v1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_ESN_V1, val_len_ptr);
}

static inline char * mipc_sys_get_info_cnf_get_meid_v1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_MEID_V1, val_len_ptr);
}

static inline mipc_sys_cap_cellular_class_const_enum mipc_sys_get_info_cnf_get_sys_cellular_class_v1(mipc_msg_t *msg_ptr, mipc_sys_cap_cellular_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_cellular_class_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_INFO_CNF_T_SYS_CELLULAR_CLASS_V1, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_at_req_add_atcmd(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_AT_CMD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_AT_REQ_T_ATCMD, len, (const void *)value);
}

static inline char * mipc_sys_at_cnf_get_atcmd(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_AT_CNF_T_ATCMD, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_reboot_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sys_reboot_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_REBOOT_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_reboot_req_add_timeout(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_REBOOT_REQ_T_TIMEOUT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_mapping_req_add_msg(mipc_msg_t *msg_ptr, enum mipc_sys_sim_ps_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_GET_MAPPING_REQ_T_MSG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sys_get_mapping_cnf_get_mapping_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_MAPPING_CNF_T_MAPPING_COUNT, def_val);
}

static inline mipc_sys_mapping_struct4* mipc_sys_get_mapping_cnf_get_mapping_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_mapping_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_MAPPING_CNF_T_MAPPING_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_mapping_req_add_mapping_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_MAPPING_REQ_T_MAPPING_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_mapping_req_add_mapping_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_mapping_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_MAPPING_REQ_T_MAPPING_LIST, len, (const void *)value);
}

static inline uint8_t mipc_sys_set_mapping_cnf_get_mapping_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_SET_MAPPING_CNF_T_MAPPING_COUNT, def_val);
}

static inline mipc_sys_mapping_struct4* mipc_sys_set_mapping_cnf_get_mapping_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_mapping_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_MAPPING_CNF_T_MAPPING_LIST, val_len_ptr);
}

static inline uint32_t mipc_sys_get_thermal_sensor_num_cnf_get_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_NUM_CNF_T_NUM, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_sensor_info_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_thermal_sensor_info_cnf_get_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_ID, def_val);
}

static inline char * mipc_sys_get_thermal_sensor_info_cnf_get_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_NAME, val_len_ptr);
}

static inline uint8_t mipc_sys_get_thermal_sensor_info_cnf_get_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_TYPE, def_val);
}

static inline uint8_t mipc_sys_get_thermal_sensor_info_cnf_get_meas_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_MEAS_TYPE, def_val);
}

static inline int32_t mipc_sys_get_thermal_sensor_info_cnf_get_min_tempature(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_MIN_TEMPATURE, def_val);
}

static inline uint32_t mipc_sys_get_thermal_sensor_info_cnf_get_max_tempature(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_MAX_TEMPATURE, def_val);
}

static inline uint32_t mipc_sys_get_thermal_sensor_info_cnf_get_accuracy(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_ACCURACY, def_val);
}

static inline uint32_t mipc_sys_get_thermal_sensor_info_cnf_get_resolution(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_RESOLUTION, def_val);
}

static inline int32_t mipc_sys_get_thermal_sensor_info_cnf_get_warn_tempature(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_WARN_TEMPATURE, def_val);
}

static inline int32_t mipc_sys_get_thermal_sensor_info_cnf_get_hw_shutdown_temperature(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_HW_SHUTDOWN_TEMPERATURE, def_val);
}

static inline uint32_t mipc_sys_get_thermal_sensor_info_cnf_get_min_sampling_period(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_INFO_CNF_T_MIN_SAMPLING_PERIOD, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_sensor_req_add_config_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_THERMAL_SENSOR_REQ_T_CONFIG_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_sensor_req_add_config_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_thermal_sensor_config_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_THERMAL_SENSOR_REQ_T_CONFIG_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_sensor_req_add_config_e(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_sys_thermal_sensor_config_e_struct4 *value)
{
    if (index >= MIPC_MAX_SYS_THERMAL_SENSOR_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline uint8_t mipc_sys_set_thermal_sensor_cnf_get_config_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_SET_THERMAL_SENSOR_CNF_T_CONFIG_COUNT, def_val);
}

static inline mipc_sys_thermal_sensor_config_struct4* mipc_sys_set_thermal_sensor_cnf_get_config_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_config_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_THERMAL_SENSOR_CNF_T_CONFIG_LIST, val_len_ptr);
}

static inline mipc_sys_thermal_sensor_config_e_struct4* mipc_sys_set_thermal_sensor_cnf_get_config_e(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SYS_THERMAL_SENSOR_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_config_e_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SYS_SET_THERMAL_SENSOR_CNF_T_CONFIG_E, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_sensor_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline int32_t mipc_sys_get_thermal_sensor_cnf_get_temperature(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_CNF_T_TEMPERATURE, def_val);
}

static inline uint32_t mipc_sys_get_thermal_actuator_num_cnf_get_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_NUM_CNF_T_NUM, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_actuator_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_THERMAL_ACTUATOR_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_actuator_req_add_level(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_THERMAL_ACTUATOR_REQ_T_LEVEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_actuator_req_add_state(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_THERMAL_ACTUATOR_REQ_T_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_actuator_info_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_thermal_actuator_info_cnf_get_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_ID, def_val);
}

static inline char * mipc_sys_get_thermal_actuator_info_cnf_get_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_NAME, val_len_ptr);
}

static inline uint32_t mipc_sys_get_thermal_actuator_info_cnf_get_total_level(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_TOTAL_LEVEL, def_val);
}

static inline uint32_t mipc_sys_get_thermal_actuator_info_cnf_get_current_level(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_CURRENT_LEVEL, def_val);
}

static inline uint32_t mipc_sys_get_thermal_actuator_info_cnf_get_user_impact(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_USER_IMPACT, def_val);
}

static inline uint32_t mipc_sys_get_thermal_actuator_info_cnf_get_efficiency(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_INFO_CNF_T_EFFICIENCY, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_config_req_add_class(mipc_msg_t *msg_ptr, enum mipc_sys_config_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_CONFIG_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_config_req_add_type(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_CONFIG_REQ_T_TYPE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_config_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  60000) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_CONFIG_REQ_T_DATA, len, (const void *)value);
}

static inline void * mipc_sys_set_config_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_CONFIG_CNF_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_get_config_req_add_class(mipc_msg_t *msg_ptr, enum mipc_sys_config_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_CONFIG_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_config_req_add_type(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_GET_CONFIG_REQ_T_TYPE, len, (const void *)value);
}

static inline void * mipc_sys_get_config_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_CONFIG_CNF_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_reg_config_req_add_class(mipc_msg_t *msg_ptr, enum mipc_sys_config_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_REG_CONFIG_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_reg_config_req_add_type(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_REG_CONFIG_REQ_T_TYPE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_adpclk_req_add_state(mipc_msg_t *msg_ptr, enum mipc_sys_adpclk_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_ADPCLK_REQ_T_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sys_get_adpclk_cnf_get_freq_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_ADPCLK_CNF_T_FREQ_INFO_COUNT, def_val);
}

static inline mipc_sys_adpclk_freq_info_struct8* mipc_sys_get_adpclk_cnf_get_freq_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_adpclk_freq_info_struct8*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_ADPCLK_CNF_T_FREQ_INFO_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_md_log_mode_req_add_mode(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_MD_LOG_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_md_log_mode_cnf_get_mode(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_MD_LOG_MODE_CNF_T_MODE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_md_log_level_req_add_level(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_MD_LOG_LEVEL_REQ_T_LEVEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_md_log_level_cnf_get_level(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_MD_LOG_LEVEL_CNF_T_LEVEL, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_md_log_location_req_add_enable(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_MD_LOG_LOCATION_REQ_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_md_log_location_cnf_get_enable(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_MD_LOG_LOCATION_CNF_T_ENABLE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_write_nvram_req_add_file_idx(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_WRITE_NVRAM_REQ_T_FILE_IDX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_write_nvram_req_add_record_idx(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_WRITE_NVRAM_REQ_T_RECORD_IDX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_write_nvram_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  60000) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_WRITE_NVRAM_REQ_T_DATA, len, (const void *)value);
}

static inline uint32_t mipc_sys_write_nvram_cnf_get_data_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_WRITE_NVRAM_CNF_T_DATA_LEN, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_read_nvram_req_add_file_idx(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_READ_NVRAM_REQ_T_FILE_IDX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_read_nvram_req_add_record_idx(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_READ_NVRAM_REQ_T_RECORD_IDX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline void * mipc_sys_read_nvram_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_READ_NVRAM_CNF_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_auth_req_add_op(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_AUTH_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_auth_req_add_encdata(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  256) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_AUTH_REQ_T_ENCDATA, len, (const void *)value);
}

static inline void * mipc_sys_auth_cnf_get_rand(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_AUTH_CNF_T_RAND, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_dat_req_add_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_DAT_REQ_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_dat_cnf_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_DAT_CNF_T_INDEX, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_op(mipc_msg_t *msg_ptr, enum mipc_sys_mcf_op_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_config_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_CONFIG_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_dump_lids(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DUMP_LIDS_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_MCF_REQ_T_DUMP_LIDS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_path_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_PATH_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_trigger_dsbp(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_TRIGGER_DSBP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_action(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_format(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_num(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_MCF_REQ_T_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_rec_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_MCF_REQ_T_REC_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_value(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_MCF_VALUE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_MCF_REQ_T_VALUE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_config1(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_MCF_CFG_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_MCF_REQ_T_CONFIG1, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_config(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_MCF_CFG_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_MCF_REQ_T_CONFIG, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_mcf_req_add_is_reset(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MCF_REQ_T_IS_RESET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_mcf_cnf_get_mcf_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_MCF_CNF_T_MCF_RESULT, def_val);
}

static inline uint32_t mipc_sys_mcf_cnf_get_dsbp_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_MCF_CNF_T_DSBP_RESULT, def_val);
}

static inline mipc_sys_mcf_op_const_enum mipc_sys_mcf_cnf_get_op(mipc_msg_t *msg_ptr, mipc_sys_mcf_op_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_mcf_op_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_OP, def_val);
}

static inline uint8_t mipc_sys_mcf_cnf_get_config_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_CONFIG_TYPE, def_val);
}

static inline uint8_t mipc_sys_mcf_cnf_get_path_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_PATH_TYPE, def_val);
}

static inline uint8_t mipc_sys_mcf_cnf_get_action(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_ACTION, def_val);
}

static inline uint8_t mipc_sys_mcf_cnf_get_format(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_FORMAT, def_val);
}

static inline uint8_t mipc_sys_mcf_cnf_get_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_CNF_T_LEN, def_val);
}

static inline void * mipc_sys_mcf_cnf_get_value(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_MCF_CNF_T_VALUE, val_len_ptr);
}

static inline char * mipc_sys_mcf_cnf_get_config1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_MCF_CNF_T_CONFIG1, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_fcc_lock_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_FCC_LOCK_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_year(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_YEAR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_month(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_MONTH, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_day(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_DAY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_hour(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_HOUR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_minute(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_MINUTE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_time_req_add_second(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_TIME_REQ_T_SECOND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_time_cnf_get_year(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_YEAR, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_month(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_MONTH, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_day(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_DAY, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_hour(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_HOUR, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_minute(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_MINUTE, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_second(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_SECOND, def_val);
}

static inline uint32_t mipc_sys_get_time_cnf_get_timestamp(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_TIME_CNF_T_TIMESTAMP, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_sar_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sys_sar_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_SAR_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_sar_req_add_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_SAR_REQ_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sys_sar_mode_const_enum mipc_sys_set_sar_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_sys_sar_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_sar_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_SET_SAR_CNF_T_MODE, def_val);
}

static inline uint32_t mipc_sys_set_sar_cnf_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_SET_SAR_CNF_T_INDEX, def_val);
}

static inline mipc_sys_sar_mode_const_enum mipc_sys_get_sar_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_sys_sar_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_sar_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_SAR_CNF_T_MODE, def_val);
}

static inline uint32_t mipc_sys_get_sar_cnf_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_SAR_CNF_T_INDEX, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_set_power_saving_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_POWER_SAVING_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_connectivity_statistics_req_add_read_flag(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_REQ_T_READ_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_connectivity_statistics_req_add_start(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_REQ_T_START, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_connectivity_statistics_req_add_stop(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_REQ_T_STOP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_connectivity_statistics_req_add_period_value(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_REQ_T_PERIOD_VALUE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_sms_tx_counter(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_SMS_TX_COUNTER, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_sms_rx_counter(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_SMS_RX_COUNTER, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_tx_data(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_TX_DATA, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_rx_data(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_RX_DATA, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_max_message_size(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_MAX_MESSAGE_SIZE, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_average_message_size(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_AVERAGE_MESSAGE_SIZE, def_val);
}

static inline uint32_t mipc_sys_connectivity_statistics_cnf_get_period_value(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_PERIOD_VALUE, def_val);
}

static inline void * mipc_sys_connectivity_statistics_cnf_get_tx_data_ext(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_TX_DATA_EXT, val_len_ptr);
}

static inline void * mipc_sys_connectivity_statistics_cnf_get_rx_data_ext(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_CONNECTIVITY_STATISTICS_CNF_T_RX_DATA_EXT, val_len_ptr);
}

static inline uint32_t mipc_sys_query_sbp_cnf_get_sbp_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_QUERY_SBP_CNF_T_SBP_ID, def_val);
}

static inline uint32_t mipc_sys_query_sbp_cnf_get_sim_sbp_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_QUERY_SBP_CNF_T_SIM_SBP_ID, def_val);
}

static inline char * mipc_sys_query_sbp_cnf_get_sbp_feature_byte(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_QUERY_SBP_CNF_T_SBP_FEATURE_BYTE, val_len_ptr);
}

static inline char * mipc_sys_query_sbp_cnf_get_sbp_data_byte(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_QUERY_SBP_CNF_T_SBP_DATA_BYTE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_tx_ind_interval_req_add_interval(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_TX_IND_INTERVAL_REQ_T_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_account_id(mipc_msg_t *msg_ptr, enum mipc_sys_account_id_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ACCOUNT_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_broadcast_flag(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_BROADCAST_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_latitude(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_LATITUDE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_longitude(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_LONGITUDE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_accuracy(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ACCURACY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_method(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_METHOD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_city(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_CITY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_state(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_STATE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_zip(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ZIP, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_country_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_COUNTRY_CODE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_ue_wlan_mac(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_UE_WLAN_MAC, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_confidence(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_CONFIDENCE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_altitude(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ALTITUDE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_accuracy_semi_major_axis(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ACCURACY_SEMI_MAJOR_AXIS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_accuracy_semi_minor_axis(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ACCURACY_SEMI_MINOR_AXIS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_geo_location_req_add_accuracy_vertical_axis(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SET_LOCATION_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_GEO_LOCATION_REQ_T_ACCURACY_VERTICAL_AXIS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_dsbp_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sys_dsbp_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_DSBP_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_send_sar_ind_req_add_cmd_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SEND_SAR_IND_REQ_T_CMD_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_send_sar_ind_req_add_cmd_param(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SYS_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SEND_SAR_IND_REQ_T_CMD_PARAM, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_silent_reboot_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SILENT_REBOOT_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_multi_sim_config_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_MULTI_SIM_CONFIG_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_reboot_set_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_REBOOT_SET_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_actuator_req_add_get_actuator_num(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_REQ_T_GET_ACTUATOR_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_actuator_req_add_actuator_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_REQ_T_ACTUATOR_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_get_thermal_actuator_cnf_get_actuator_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_CNF_T_ACTUATOR_NUM, def_val);
}

static inline mipc_sys_thermal_actuator_state_info_struct4* mipc_sys_get_thermal_actuator_cnf_get_actuator_state_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_actuator_state_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_THERMAL_ACTUATOR_CNF_T_ACTUATOR_STATE_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_sensor_runtime_req_add_get_md_auto_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_RUNTIME_REQ_T_GET_MD_AUTO_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_get_thermal_sensor_runtime_req_add_sensor_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_RUNTIME_REQ_T_SENSOR_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_boolean_const_enum mipc_sys_get_thermal_sensor_runtime_cnf_get_md_auto_enable(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_RUNTIME_CNF_T_MD_AUTO_ENABLE, def_val);
}

static inline mipc_sys_thermal_trip_map_struct4* mipc_sys_get_thermal_sensor_runtime_cnf_get_trip(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SYS_THERMAL_TRIP_POINT_NUM_IN_ONE_SENSOR) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_trip_map_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SYS_GET_THERMAL_SENSOR_RUNTIME_CNF_T_TRIP, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_runtime_req_add_reset_cfg_from_nv(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_THERMAL_RUNTIME_REQ_T_RESET_CFG_FROM_NV, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_runtime_req_add_md_auto_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_THERMAL_RUNTIME_REQ_T_MD_AUTO_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_runtime_req_add_trip_change(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_thermal_trip_change_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_THERMAL_RUNTIME_REQ_T_TRIP_CHANGE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_thermal_runtime_req_add_save_cfg_to_nv(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_THERMAL_RUNTIME_REQ_T_SAVE_CFG_TO_NV, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_or_get_sbp_info_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sys_sbp_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_or_get_sbp_info_req_add_feature_int(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_REQ_T_FEATURE_INT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_or_get_sbp_info_req_add_feature_str(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_REQ_T_FEATURE_STR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_set_or_get_sbp_info_req_add_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_REQ_T_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_set_or_get_sbp_info_req_add_param(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_REQ_T_PARAM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sys_set_or_get_sbp_info_cnf_get_data(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_SET_OR_GET_SBP_INFO_CNF_T_DATA, def_val);
}

static inline mipc_boolean_const_enum mipc_sys_get_all_thermal_info_cnf_get_auto_flag(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GET_ALL_THERMAL_INFO_CNF_T_AUTO_FLAG, def_val);
}

static inline mipc_sys_thermal_sensor_info_e_struct4* mipc_sys_get_all_thermal_info_cnf_get_thermal_temp_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SYS_THERMAL_SENSOR_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_info_e_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SYS_GET_ALL_THERMAL_INFO_CNF_T_THERMAL_TEMP_INFO, index, val_len_ptr);
}

static inline mipc_sys_thermal_actuator_state_info_struct4* mipc_sys_get_all_thermal_info_cnf_get_actuator_state_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SYS_THERMAL_ACTUATOR_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_actuator_state_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SYS_GET_ALL_THERMAL_INFO_CNF_T_ACTUATOR_STATE_INFO, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_md_log_flush_interval_req_add_flush_interval(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_SET_MD_LOG_FLUSH_INTERVAL_REQ_T_FLUSH_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sys_set_md_log_flush_interval_cnf_get_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_SET_MD_LOG_FLUSH_INTERVAL_CNF_T_STATUS, def_val);
}

static inline uint32_t mipc_sys_get_md_log_flush_interval_cnf_get_flush_interval(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_MD_LOG_FLUSH_INTERVAL_CNF_T_FLUSH_INTERVAL, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_proxy_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_PROXY_KEY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_if_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IF_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_hitchhike_interval(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_HITCHHIKE_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_hb_pattern(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_HB_PATTERN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_hb_ack_pattern(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_HB_ACK_PATTERN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_allow_dynamic_cycle(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_ALLOW_DYNAMIC_CYCLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_cycle_value(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_CYCLE_VALUE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_max_cycle(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_MAX_CYCLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_cycle_step(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_CYCLE_STEP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_cycle_step_success_num(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_CYCLE_STEP_SUCCESS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv4_tos(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV4_TOS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv4_ttl(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV4_TTL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv4_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV4_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv6_flow_label(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV6_FLOW_LABEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv6_hop_limit(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV6_HOP_LIMIT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_ipv6_tclass(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IPV6_TCLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_rcv_wscale(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_RCV_WSCALE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_snd_wscale(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_SND_WSCALE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_wscale_ok(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_WSCALE_OK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_snd_window(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_SND_WINDOW, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_rcv_window(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_RCV_WINDOW, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_snd_nxt_seq(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_SND_NXT_SEQ, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_tcp_rcv_nxt_seq(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_TCP_RCV_NXT_SEQ, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_v4_src_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_v4_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_V4_SRC_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_v4_dest_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_v4_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_V4_DEST_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_v6_src_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_v6_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_V6_SRC_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_v6_dest_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_v6_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_V6_DEST_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_is_ipv6(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_IS_IPV6, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_protocol(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_PROTOCOL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_src_port(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_SRC_PORT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_dest_port(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_DEST_PORT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_establish_req_add_max_retry_times(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_ESTABLISH_REQ_T_MAX_RETRY_TIMES, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_sys_hba_establish_cnf_get_proxy_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_HBA_ESTABLISH_CNF_T_PROXY_KEY, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_ctrl_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sys_hba_ctrl_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_ctrl_req_add_proxy_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_CTRL_REQ_T_PROXY_KEY, len, (const void *)value);
}

static inline mipc_sys_hba_ctrl_mode_const_enum mipc_sys_hba_ctrl_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_sys_hba_ctrl_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_hba_ctrl_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_MODE, def_val);
}

static inline char * mipc_sys_hba_ctrl_cnf_get_proxy_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_PROXY_KEY, val_len_ptr);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_ipv4_tos(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV4_TOS, def_val);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_ipv4_ttl(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV4_TTL, def_val);
}

static inline uint16_t mipc_sys_hba_ctrl_cnf_get_ipv4_id(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV4_ID, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_ipv6_flow_label(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV6_FLOW_LABEL, def_val);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_ipv6_hop_limit(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV6_HOP_LIMIT, def_val);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_ipv6_tclass(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_IPV6_TCLASS, def_val);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_tcp_rcv_wscale(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_RCV_WSCALE, def_val);
}

static inline uint8_t mipc_sys_hba_ctrl_cnf_get_tcp_snd_wscale(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_SND_WSCALE, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_tcp_wscale_ok(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_WSCALE_OK, def_val);
}

static inline uint16_t mipc_sys_hba_ctrl_cnf_get_tcp_snd_window(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_SND_WINDOW, def_val);
}

static inline uint16_t mipc_sys_hba_ctrl_cnf_get_tcp_rcv_window(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_RCV_WINDOW, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_tcp_snd_nxt_seq(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_SND_NXT_SEQ, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_tcp_rcv_nxt_seq(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_TCP_RCV_NXT_SEQ, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_send_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_SEND_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_recv_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_RECV_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_short_rrc_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_SHORT_RRC_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_ctrl_cnf_get_hitchhike_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_HITCHHIKE_COUNT, def_val);
}

static inline uint16_t mipc_sys_hba_ctrl_cnf_get_current_cycle(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_CTRL_CNF_T_CURRENT_CYCLE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_send_now_req_add_proxy_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_SEND_NOW_REQ_T_PROXY_KEY, len, (const void *)value);
}

static inline char * mipc_sys_hba_send_now_cnf_get_proxy_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_HBA_SEND_NOW_CNF_T_PROXY_KEY, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_proxy_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_PROXY_KEY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv4_tos(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV4_TOS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv4_ttl(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV4_TTL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv4_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV4_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv6_flow_label(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV6_FLOW_LABEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv6_hop_limit(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV6_HOP_LIMIT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_ipv6_tclass(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_IPV6_TCLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_rcv_wscale(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_RCV_WSCALE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_snd_wscale(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_SND_WSCALE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_wscale_ok(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_WSCALE_OK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_snd_window(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_SND_WINDOW, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_rcv_window(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_RCV_WINDOW, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_snd_nxt_seq(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_SND_NXT_SEQ, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sys_hba_resume_req_add_tcp_rcv_nxt_seq(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SYS_HBA_RESUME_REQ_T_TCP_RCV_NXT_SEQ, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_sys_hba_resume_cnf_get_proxy_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_HBA_RESUME_CNF_T_PROXY_KEY, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_SET_IA_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_pdp_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_IA_REQ_T_PDP_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_roaming_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_IA_REQ_T_ROAMING_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_auth_type(mipc_msg_t *msg_ptr, enum mipc_apn_auth_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_IA_REQ_T_AUTH_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_userid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_USERID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_SET_IA_REQ_T_USERID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_password(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PASSWORD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_SET_IA_REQ_T_PASSWORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_bearer_bitmask(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_SET_IA_REQ_T_BEARER_BITMASK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_req_add_compression(mipc_msg_t *msg_ptr, enum mipc_apn_compression_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_IA_REQ_T_COMPRESSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_apn_set_ia_cnf_get_ia_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_SET_IA_CNF_T_IA_COUNT, def_val);
}

static inline mipc_apn_ia_struct4* mipc_apn_set_ia_cnf_get_ia_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_ia_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_SET_IA_CNF_T_IA_LIST, val_len_ptr);
}

static inline uint8_t mipc_apn_get_ia_cnf_get_ia_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_GET_IA_CNF_T_IA_COUNT, def_val);
}

static inline mipc_apn_ia_struct4* mipc_apn_get_ia_cnf_get_ia_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_ia_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_GET_IA_CNF_T_IA_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PLMN_ID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_apn_type(mipc_msg_t *msg_ptr, enum mipc_apn_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_APN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_pdp_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_PDP_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_roaming_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_ROAMING_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_auth_type(mipc_msg_t *msg_ptr, enum mipc_apn_auth_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_AUTH_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_userid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_USERID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_USERID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_password(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PASSWORD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_PASSWORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_bearer_bitmask(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_BEARER_BITMASK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_compression(mipc_msg_t *msg_ptr, enum mipc_apn_compression_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_COMPRESSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_req_add_enabled(mipc_msg_t *msg_ptr, enum mipc_apn_enabled_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_REQ_T_ENABLED, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_apn_add_profile_cnf_get_apn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_CNF_T_APN_COUNT, def_val);
}

static inline mipc_apn_profile_struct4* mipc_apn_add_profile_cnf_get_apn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_profile_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_ADD_PROFILE_CNF_T_APN_LIST, val_len_ptr);
}

static inline uint8_t mipc_apn_list_profile_cnf_get_apn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_LIST_PROFILE_CNF_T_APN_COUNT, def_val);
}

static inline mipc_apn_profile_struct4* mipc_apn_list_profile_cnf_get_apn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_profile_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_LIST_PROFILE_CNF_T_APN_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_del_profile_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_DEL_PROFILE_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_apn_del_profile_cnf_get_apn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_DEL_PROFILE_CNF_T_APN_COUNT, def_val);
}

static inline mipc_apn_profile_struct4* mipc_apn_del_profile_cnf_get_apn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_profile_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_DEL_PROFILE_CNF_T_APN_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_set_profile_status_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_apn_profile_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_PROFILE_STATUS_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_apn_list_md_profile_cnf_get_apn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_LIST_MD_PROFILE_CNF_T_APN_COUNT, def_val);
}

static inline mipc_md_apn_profile_struct4* mipc_apn_list_md_profile_cnf_get_apn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_md_apn_profile_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_LIST_MD_PROFILE_CNF_T_APN_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_apn_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_APN_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_class(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_network_identifier(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_NETWORK_IDENTIFIER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_pdp_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_PDP_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_apn_bearer(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_BEARER_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_APN_BEARER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_enabled(mipc_msg_t *msg_ptr, enum mipc_apn_enabled_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_ENABLED, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_req_add_apn_timer(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_SET_VZW_APN_REQ_T_APN_TIMER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_timer_req_add_apn_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_TIMER_REQ_T_APN_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_timer_req_add_max_conn(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_TIMER_REQ_T_MAX_CONN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_timer_req_add_max_conn_t(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_VZW_APN_TIMER_REQ_T_MAX_CONN_T, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_timer_req_add_wait_time(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_SET_VZW_APN_TIMER_REQ_T_WAIT_TIME, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_set_vzw_apn_timer_req_add_throttle_time(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_APN_SET_VZW_APN_TIMER_REQ_T_THROTTLE_TIME, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_list_req_add_profile_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_LIST_REQ_T_PROFILE_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_apn_add_profile_list_req_add_profile_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_apn_profile_v2_struct4 *value)
{
    if (index >= MIPC_MAX_PROFILE_LIST_COUNT) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline uint8_t mipc_apn_add_profile_list_cnf_get_profile_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_ADD_PROFILE_LIST_CNF_T_PROFILE_COUNT, def_val);
}

static inline mipc_apn_profile_v2_struct4* mipc_apn_add_profile_list_cnf_get_profile_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_apn_profile_v2_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_ADD_PROFILE_LIST_CNF_T_PROFILE_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_apn_set_ia_md_prefer_req_add_ia_md_prefer(mipc_msg_t *msg_ptr, enum mipc_ia_md_prefer_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_APN_SET_IA_MD_PREFER_REQ_T_IA_MD_PREFER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_apn_type(mipc_msg_t *msg_ptr, enum mipc_apn_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_APN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_pdp_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_PDP_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_roaming_type(mipc_msg_t *msg_ptr, enum mipc_apn_pdp_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_ROAMING_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_auth_type(mipc_msg_t *msg_ptr, enum mipc_apn_auth_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_AUTH_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_userid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_USERID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_USERID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_password(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PASSWORD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_PASSWORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_ipv4v6_fallback(mipc_msg_t *msg_ptr, enum mipc_data_fallback_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_IPV4V6_FALLBACK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_bearer_bitmask(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_BEARER_BITMASK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_reuse_flag(mipc_msg_t *msg_ptr, enum mipc_data_reuse_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_REUSE_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_if_type(mipc_msg_t *msg_ptr, enum mipc_data_bind_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_IF_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_ursp_traffic_desc(mipc_msg_t *msg_ptr, uint16_t len, mipc_ursp_traffic_desc_struct_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_URSP_TRAFFIC_DESC, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_ursp_ue_local_conf(mipc_msg_t *msg_ptr, uint16_t len, mipc_ursp_ue_local_conf_struct_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_URSP_UE_LOCAL_CONF, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_ursp_eval_flag(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_URSP_EVAL_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_req_add_if_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_REQ_T_IF_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_data_act_call_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_ID, def_val);
}

static inline char * mipc_data_act_call_cnf_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_APN, val_len_ptr);
}

static inline mipc_apn_pdp_type_const_enum mipc_data_act_call_cnf_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PDP_TYPE, def_val);
}

static inline uint8_t mipc_data_act_call_cnf_get_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_cnf_get_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_cnf_get_dns_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_dns_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_dns_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_dns_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_dns_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_cnf_get_dns_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_dns_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_dns_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_dns_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_dns_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_DNS_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_cnf_get_pcscf_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V4_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_cnf_get_pcscf_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_pcscf_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_PCSCF_V6_3, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_cnf_get_gw_v4(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_GW_V4, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_cnf_get_gw_v6(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_GW_V6, val_len_ptr);
}

static inline uint32_t mipc_data_act_call_cnf_get_mtu_v4(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_MTU_V4, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_mtu_v6(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_MTU_V6, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_INTERFACE_ID, def_val);
}

static inline uint8_t mipc_data_act_call_cnf_get_p_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_P_ID, def_val);
}

static inline uint8_t mipc_data_act_call_cnf_get_fb_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_FB_ID, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_ipv4_netmask(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_IPv4_NETMASK, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_ipv6_netmask(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_IPv6_NETMASK, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_trans_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_TRANS_ID, def_val);
}

static inline mipc_ran_const_enum mipc_data_act_call_cnf_get_ran_info(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_RAN_INFO, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_bearer_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_BEARER_ID, def_val);
}

static inline uint8_t mipc_data_act_call_cnf_get_im_cn_signalling_flag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_IM_CN_SIGNALLING_FLAG, def_val);
}

static inline uint32_t mipc_data_act_call_cnf_get_mtu_ethernet(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_CNF_T_MTU_ETHERNET, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_deact_call_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_DEACT_CALL_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_deact_call_req_add_deact_reason(mipc_msg_t *msg_ptr, enum mipc_deact_reason_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_DEACT_CALL_REQ_T_DEACT_REASON, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_data_deact_call_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DEACT_CALL_CNF_T_ID, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_get_call_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_CALL_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_data_get_call_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_ID, def_val);
}

static inline char * mipc_data_get_call_cnf_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_APN, val_len_ptr);
}

static inline mipc_apn_pdp_type_const_enum mipc_data_get_call_cnf_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PDP_TYPE, def_val);
}

static inline uint8_t mipc_data_get_call_cnf_get_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_cnf_get_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_cnf_get_dns_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_dns_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_dns_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_dns_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_dns_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_cnf_get_dns_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_dns_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_dns_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_dns_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_dns_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_DNS_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_cnf_get_pcscf_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_cnf_get_pcscf_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_pcscf_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_PCSCF_V6_3, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_get_call_cnf_get_gw_v4(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_GW_V4, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_get_call_cnf_get_gw_v6(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_GW_V6, val_len_ptr);
}

static inline uint32_t mipc_data_get_call_cnf_get_mtu_v4(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_MTU_V4, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_mtu_v6(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_MTU_V6, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_INTERFACE_ID, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_ipv4_netmask(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_IPv4_NETMASK, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_ipv6_netmask(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_IPv6_NETMASK, def_val);
}

static inline mipc_apn_type_const_enum mipc_data_get_call_cnf_get_apn_type(mipc_msg_t *msg_ptr, mipc_apn_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_APN_TYPE, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_trans_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_TRANS_ID, def_val);
}

static inline mipc_ran_const_enum mipc_data_get_call_cnf_get_ran_info(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_RAN_INFO, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_bearer_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_BEARER_ID, def_val);
}

static inline uint8_t mipc_data_get_call_cnf_get_im_cn_signalling_flag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_IM_CN_SIGNALLING_FLAG, def_val);
}

static inline uint8_t mipc_data_get_call_cnf_get_qfi(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_QFI, def_val);
}

static inline uint32_t mipc_data_get_call_cnf_get_mtu_ethernet(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_CNF_T_MTU_ETHERNET, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_set_packet_filter_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_packet_filter_req_add_filter_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_REQ_T_FILTER_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_packet_filter_req_add_filter_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_packet_filter_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_REQ_T_FILTER_LIST, len, (const void *)value);
}

static inline uint8_t mipc_data_set_packet_filter_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_CNF_T_ID, def_val);
}

static inline uint8_t mipc_data_set_packet_filter_cnf_get_filter_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_CNF_T_FILTER_COUNT, def_val);
}

static inline mipc_data_packet_filter_struct4* mipc_data_set_packet_filter_cnf_get_filter_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_packet_filter_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_SET_PACKET_FILTER_CNF_T_FILTER_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_get_packet_filter_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_PACKET_FILTER_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_data_get_packet_filter_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PACKET_FILTER_CNF_T_ID, def_val);
}

static inline uint8_t mipc_data_get_packet_filter_cnf_get_filter_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PACKET_FILTER_CNF_T_FILTER_COUNT, def_val);
}

static inline mipc_data_packet_filter_struct4* mipc_data_get_packet_filter_cnf_get_filter_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_packet_filter_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_PACKET_FILTER_CNF_T_FILTER_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_get_pco_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_PCO_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_get_pco_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_GET_PCO_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_get_pco_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_GET_PCO_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_get_pco_req_add_pco_ie(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  5) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_GET_PCO_REQ_T_PCO_IE, len, (const void *)value);
}

static inline uint8_t mipc_data_get_pco_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PCO_CNF_T_ID, def_val);
}

static inline uint8_t mipc_data_get_pco_cnf_get_pco_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PCO_CNF_T_PCO_COUNT, def_val);
}

static inline mipc_data_pco_ie_struct4* mipc_data_get_pco_cnf_get_pco_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_pco_ie_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_PCO_CNF_T_PCO_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_data_allow_req_add_clear(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_DATA_ALLOW_REQ_T_CLEAR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline void * mipc_data_get_md_data_call_list_cnf_get_cid_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_MD_DATA_CALL_LIST_CNF_T_CID_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_mobile_data(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_MOBILE_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_data_roaming(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_DATA_ROAMING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_volte(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_VOLTE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_ims_test_mode(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_IMS_TEST_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_data_domestic_roaming(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_DATA_DOMESTIC_ROAMING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_data_international_roaming(mipc_msg_t *msg_ptr, enum mipc_data_config_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_DATA_INTERNATIONAL_ROAMING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_config_req_add_default_data_sim_card(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_CONFIG_REQ_T_DEFAULT_DATA_SIM_CARD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_mobile_data(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_MOBILE_DATA, def_val);
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_data_roaming(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_DATA_ROAMING, def_val);
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_volte(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_VOLTE, def_val);
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_ims_test_mode(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_IMS_TEST_MODE, def_val);
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_data_domestic_roaming(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_DATA_DOMESTIC_ROAMING, def_val);
}

static inline mipc_data_config_type_const_enum mipc_data_get_config_cnf_get_data_international_roaming(mipc_msg_t *msg_ptr, mipc_data_config_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_config_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CONFIG_CNF_T_DATA_INTERNATIONAL_ROAMING, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_abort_call_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_ABORT_CALL_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_get_call_info_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_GET_CALL_INFO_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_get_call_info_req_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_CALL_INFO_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_data_get_call_info_cnf_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_INFO_CNF_T_APN, val_len_ptr);
}

static inline uint8_t mipc_data_get_call_info_cnf_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_CALL_INFO_CNF_T_ID, def_val);
}

static inline mipc_data_nitz_info_struct4* mipc_data_get_call_info_cnf_get_estblished_time(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_nitz_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_INFO_CNF_T_ESTBLISHED_TIME, val_len_ptr);
}

static inline mipc_data_nitz_info_struct4* mipc_data_get_call_info_cnf_get_end_time(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_nitz_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_CALL_INFO_CNF_T_END_TIME, val_len_ptr);
}

static inline uint32_t mipc_data_get_call_info_cnf_get_reject_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_CALL_INFO_CNF_T_REJECT_CAUSE, def_val);
}

static inline uint8_t mipc_data_get_pdp_cid_cnf_get_min_cid(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PDP_CID_CNF_T_MIN_CID, def_val);
}

static inline uint8_t mipc_data_get_pdp_cid_cnf_get_max_cid(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_PDP_CID_CNF_T_MAX_CID, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_retry_timer_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_data_retry_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_RETRY_TIMER_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_retry_timer_req_add_apn_name(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_RETRY_TIMER_REQ_T_APN_NAME, len, (const void *)value);
}

static inline mipc_data_retry_type_const_enum mipc_data_retry_timer_cnf_get_retry_type(mipc_msg_t *msg_ptr, mipc_data_retry_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_retry_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_RETRY_TIMER_CNF_T_RETRY_TYPE, def_val);
}

static inline uint32_t mipc_data_retry_timer_cnf_get_retry_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_RETRY_TIMER_CNF_T_RETRY_TIME, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_data_lce_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_hysteresis_ms(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_HYSTERESIS_MS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_hysteresis_dl_kbps(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_HYSTERESIS_DL_KBPS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_hysteresis_ul_kbps(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_HYSTERESIS_UL_KBPS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_threshold_dl_kbps_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_THRESHOLD_DL_KBPS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_threshold_dl_kbps_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_LCE_THRESHOLD_NUMBER) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_THRESHOLD_DL_KBPS_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_threshold_ul_kbps_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_THRESHOLD_UL_KBPS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_threshold_ul_kbps_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_LCE_THRESHOLD_NUMBER) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_THRESHOLD_UL_KBPS_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_link_capacity_reporting_criteria_req_add_access_network(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_LINK_CAPACITY_REPORTING_CRITERIA_REQ_T_ACCESS_NETWORK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_get_dedicate_bearer_info_req_add_cid(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_DEDICATE_BEARER_INFO_REQ_T_CID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_data_secondary_pdp_context_info_struct4* mipc_data_get_dedicate_bearer_info_cnf_get_context_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CID_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_secondary_pdp_context_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_DEDICATE_BEARER_INFO_CNF_T_CONTEXT_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_get_qos_req_add_cid(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_QOS_REQ_T_CID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_data_qos_info_struct4* mipc_data_get_qos_cnf_get_qos_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CID_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_qos_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_QOS_CNF_T_QOS_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_get_tft_req_add_cid(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_TFT_REQ_T_CID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_data_tft_info_struct4* mipc_data_get_tft_cnf_get_tft_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CID_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_tft_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_TFT_CNF_T_TFT_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_lgdcont_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_LGDCONT_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_lgdcont_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_LGDCONT_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_lgdcont_req_add_request_type(mipc_msg_t *msg_ptr, enum mipc_data_lgdcont_req_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_LGDCONT_REQ_T_REQUEST_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_lgdcont_req_add_rat_type(mipc_msg_t *msg_ptr, enum mipc_data_lgdcont_rat_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_LGDCONT_REQ_T_RAT_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_nssai_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_NSSAI_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_nssai_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_NSSAI_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_nssai_req_add_snssai(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DATA_SNSSAI_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_NSSAI_REQ_T_SNSSAI, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_keepalive_req_add_stop_keepalive(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_KEEPALIVE_REQ_T_STOP_KEEPALIVE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_keepalive_req_add_start_keepalive(mipc_msg_t *msg_ptr, uint16_t len, mipc_data_start_keepalive_request_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_KEEPALIVE_REQ_T_START_KEEPALIVE, len, (const void *)value);
}

static inline uint32_t mipc_data_keepalive_cnf_get_session_handle(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_KEEPALIVE_CNF_T_SESSION_HANDLE, def_val);
}

static inline mipc_data_keepalive_status_code_const_enum mipc_data_keepalive_cnf_get_status_code(mipc_msg_t *msg_ptr, mipc_data_keepalive_status_code_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_keepalive_status_code_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_KEEPALIVE_CNF_T_STATUS_CODE, def_val);
}

static inline mipc_dsda_allowed_type_const_enum mipc_data_get_dsda_state_cnf_get_dsda_allowed(mipc_msg_t *msg_ptr, mipc_dsda_allowed_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dsda_allowed_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_DSDA_STATE_CNF_T_DSDA_ALLOWED, def_val);
}

static inline mipc_dsda_state_type_const_enum mipc_data_get_dsda_state_cnf_get_dsda_state(mipc_msg_t *msg_ptr, mipc_dsda_state_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dsda_state_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_DSDA_STATE_CNF_T_DSDA_STATE, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_get_5gqos_req_add_cid(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_5GQOS_REQ_T_CID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_data_5gqos_info_struct4* mipc_data_get_5gqos_cnf_get_qos_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CID_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_5gqos_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_5GQOS_CNF_T_QOS_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_psi_req_add_action(mipc_msg_t *msg_ptr, enum mipc_psi_action_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_PSI_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_psi_req_add_psi(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_PSI_REQ_T_PSI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_psi_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_PSI_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_psi_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_PSI_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_data_set_psi_cnf_get_psi(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_SET_PSI_CNF_T_PSI, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_get_nssai_req_add_nssai_type(mipc_msg_t *msg_ptr, enum mipc_nssai_type_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_REQ_T_NSSAI_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_get_nssai_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  6) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_GET_NSSAI_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_default_configured_nssai_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_DEFAULT_CONFIGURED_NSSAI_NUM, def_val);
}

static inline mipc_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_default_configured_nssai_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_DEFAULT_CONFIGURED_NSSAI_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_rejected_nssai_3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_REJECTED_NSSAI_3GPP_NUM, def_val);
}

static inline mipc_rejected_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_rejected_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_rejected_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_REJECTED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_rejected_nssai_non3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_REJECTED_NSSAI_NON3GPP_NUM, def_val);
}

static inline mipc_rejected_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_rejected_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_rejected_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_REJECTED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_configured_nssai_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_CONFIGURED_NSSAI_NUM, def_val);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_configured_nssai_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_CONFIGURED_NSSAI_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_allowed_nssai_3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_ALLOWED_NSSAI_3GPP_NUM, def_val);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_allowed_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_ALLOWED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_allowed_nssai_non3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_ALLOWED_NSSAI_NON3GPP_NUM, def_val);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_allowed_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_ALLOWED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_preferred_nssai_3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_PREFERRED_NSSAI_3GPP_NUM, def_val);
}

static inline mipc_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_preferred_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_PREFERRED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline uint8_t mipc_data_get_nssai_cnf_get_preferred_nssai_non3gpp_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_PREFERRED_NSSAI_NON3GPP_NUM, def_val);
}

static inline mipc_s_nssai_struct_struct4* mipc_data_get_nssai_cnf_get_preferred_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_NSSAI_CNF_T_PREFERRED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_get_ursp_route_profile_req_add_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_REQ_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_data_get_ursp_route_profile_cnf_get_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_CNF_T_ID, def_val);
}

static inline mipc_ursp_ue_local_conf_struct_struct4* mipc_data_get_ursp_route_profile_cnf_get_est_req_param(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ursp_ue_local_conf_struct_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_CNF_T_EST_REQ_PARAM, val_len_ptr);
}

static inline uint32_t mipc_data_get_ursp_route_profile_cnf_get_attr(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_CNF_T_ATTR, def_val);
}

static inline uint32_t mipc_data_get_ursp_route_profile_cnf_get_route_supp_profile_list_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_CNF_T_ROUTE_SUPP_PROFILE_LIST_NUM, def_val);
}

static inline mipc_ursp_get_route_supp_profile_ind_struct_struct4* mipc_data_get_ursp_route_profile_cnf_get_route_supp_profile_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CID_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ursp_get_route_supp_profile_ind_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_URSP_ROUTE_PROFILE_CNF_T_ROUTE_SUPP_PROFILE_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_ursp_preconf_ue_policy_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  6) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_URSP_PRECONF_UE_POLICY_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_ursp_preconf_ue_policy_req_add_rule_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_SET_URSP_PRECONF_UE_POLICY_REQ_T_RULE_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_ursp_preconf_ue_policy_req_add_rule_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_ursp_rule_struct_struct4 *value)
{
    if (index >= MIPC_MAX_RULE_LIST_LEN) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_get_ursp_ue_policy_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  6) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_GET_URSP_UE_POLICY_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline void * mipc_data_get_ursp_ue_policy_cnf_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_GET_URSP_UE_POLICY_CNF_T_PLMN_ID, val_len_ptr);
}

static inline uint8_t mipc_data_get_ursp_ue_policy_cnf_get_rule_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_GET_URSP_UE_POLICY_CNF_T_RULE_NUM, def_val);
}

static inline mipc_ursp_rule_struct_struct4* mipc_data_get_ursp_ue_policy_cnf_get_rule_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_RULE_LIST_LEN) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ursp_rule_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_DATA_GET_URSP_UE_POLICY_CNF_T_RULE_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_data_set_reserved_if_id_req_add_if_id_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  20) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_RESERVED_IF_ID_REQ_T_IF_ID_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_pco_req_add_apn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_APN_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_DATA_SET_PCO_REQ_T_APN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_data_set_pco_req_add_apn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_DATA_SET_PCO_REQ_T_APN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_set_pco_req_add_pco_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_data_pco_ie_struct4 *value)
{
    if (index >= MIPC_MAX_PCO_COUNT) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_open_req_add_version(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_OPEN_REQ_T_VERSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_open_req_add_client_name(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CLIENT_NAME_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_OPEN_REQ_T_CLIENT_NAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_open_req_add_usir_support(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_OPEN_REQ_T_USIR_SUPPORT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_internal_open_cnf_get_version(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_OPEN_CNF_T_VERSION, def_val);
}

static inline uint32_t mipc_internal_open_cnf_get_timeout(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_OPEN_CNF_T_TIMEOUT, def_val);
}

static inline uint32_t mipc_internal_test_cnf_get_test(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_TEST_CNF_T_TEST, def_val);
}

static inline mipc_msg_api_result_enum mipc_internal_register_ind_req_add_msg_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_INTERNAL_REGISTER_IND_REQ_T_MSG_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_register_ind_req_add_msg_id_group(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t value)
{
    if (index >= MIPC_MAX_REG_UNREG_NUM_IN_ONE_MIPC_REQ) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint16(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_unregister_ind_req_add_msg_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_INTERNAL_UNREGISTER_IND_REQ_T_MSG_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_unregister_ind_req_add_msg_id_group(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t value)
{
    if (index >= MIPC_MAX_REG_UNREG_NUM_IN_ONE_MIPC_REQ) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint16(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_unregister_ind_req_add_unreg_all(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_UNREGISTER_IND_REQ_T_UNREG_ALL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_register_cmd_req_add_msg_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_INTERNAL_REGISTER_CMD_REQ_T_MSG_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_unregister_cmd_req_add_msg_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_INTERNAL_UNREGISTER_CMD_REQ_T_MSG_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_filter_req_add_struct(mipc_msg_t *msg_ptr, uint16_t len, mipc_internal_set_filter_req_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_SET_FILTER_REQ_T_STRUCT, len, (const void *)value);
}

static inline mipc_internal_set_filter_cnf_struct4* mipc_internal_set_filter_cnf_get_struct(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_internal_set_filter_cnf_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_SET_FILTER_CNF_T_STRUCT, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_internal_reset_filter_req_add_struct(mipc_msg_t *msg_ptr, uint16_t len, mipc_internal_reset_filter_req_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_RESET_FILTER_REQ_T_STRUCT, len, (const void *)value);
}

static inline mipc_internal_reset_filter_cnf_struct4* mipc_internal_reset_filter_cnf_get_struct(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_internal_reset_filter_cnf_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_RESET_FILTER_CNF_T_STRUCT, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_internal_eif_req_add_transid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_EIF_REQ_T_TRANSID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eif_req_add_cmd(mipc_msg_t *msg_ptr, enum mipc_internal_eif_req_cmd_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_EIF_REQ_T_CMD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eif_req_add_new_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_full_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_EIF_REQ_T_NEW_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_eif_req_add_old_addr(mipc_msg_t *msg_ptr, uint16_t len, mipc_full_addr_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_EIF_REQ_T_OLD_ADDR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_eif_req_add_address_type(mipc_msg_t *msg_ptr, enum mipc_eif_address_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_EIF_REQ_T_ADDRESS_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_time_req_add_minute(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_TIME_REQ_T_MINUTE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_time_req_add_second(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_TIME_REQ_T_SECOND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_warning_ratio_req_add_m(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_WARNING_RATIO_REQ_T_M, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_warning_ratio_req_add_x(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_WARNING_RATIO_REQ_T_X, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_warning_ratio_req_add_y(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_WARNING_RATIO_REQ_T_Y, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_set_long_standby_monitor_warning_ratio_req_add_z(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_SET_LONG_STANDBY_MONITOR_WARNING_RATIO_REQ_T_Z, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_radio_state_const_enum mipc_nw_get_radio_state_cnf_get_sw_state(mipc_msg_t *msg_ptr, mipc_nw_radio_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_radio_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RADIO_STATE_CNF_T_SW_STATE, def_val);
}

static inline mipc_nw_radio_state_const_enum mipc_nw_get_radio_state_cnf_get_hw_state(mipc_msg_t *msg_ptr, mipc_nw_radio_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_radio_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RADIO_STATE_CNF_T_HW_STATE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_radio_state_req_add_sw_state(mipc_msg_t *msg_ptr, enum mipc_nw_radio_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RADIO_STATE_REQ_T_SW_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_radio_state_req_add_cause(mipc_msg_t *msg_ptr, enum mipc_nw_radio_state_cause_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RADIO_STATE_REQ_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_radio_state_req_add_hw_state(mipc_msg_t *msg_ptr, enum mipc_nw_radio_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RADIO_STATE_REQ_T_HW_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_radio_state_const_enum mipc_nw_set_radio_state_cnf_get_sw_state(mipc_msg_t *msg_ptr, mipc_nw_radio_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_radio_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_RADIO_STATE_CNF_T_SW_STATE, def_val);
}

static inline mipc_nw_radio_state_const_enum mipc_nw_set_radio_state_cnf_get_hw_state(mipc_msg_t *msg_ptr, mipc_nw_radio_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_radio_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_RADIO_STATE_CNF_T_HW_STATE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_nw_register_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_format(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_oper(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_OPER_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_OPER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_act(mipc_msg_t *msg_ptr, enum mipc_nw_act_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_ACT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_arfcn(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_ARFCN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_rat_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_RAT_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_block(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_BLOCK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_ctrl_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_CTRL_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_search_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_SEARCH_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_register_state_req_add_rsrp(mipc_msg_t *msg_ptr, int32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int32(msg_ptr, MIPC_NW_SET_REGISTER_STATE_REQ_T_RSRP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_reg_state_struct4* mipc_nw_set_register_state_cnf_get_state(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_reg_state_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_STATE, val_len_ptr);
}

static inline uint16_t mipc_nw_set_register_state_cnf_get_nw_err(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_NW_ERR, def_val);
}

static inline mipc_nw_register_mode_const_enum mipc_nw_set_register_state_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_nw_register_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_register_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_MODE, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_set_register_state_cnf_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_DATA_SPEED, def_val);
}

static inline char * mipc_nw_set_register_state_cnf_get_nw_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_NW_NAME, val_len_ptr);
}

static inline char * mipc_nw_set_register_state_cnf_get_roaming_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_ROAMING_TEXT, val_len_ptr);
}

static inline uint16_t mipc_nw_set_register_state_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_FAIL_CAUSE, def_val);
}

static inline char * mipc_nw_set_register_state_cnf_get_nw_long_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_REGISTER_STATE_CNF_T_NW_LONG_NAME, val_len_ptr);
}

static inline mipc_nw_reg_state_struct4* mipc_nw_get_register_state_cnf_get_state(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_reg_state_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_STATE, val_len_ptr);
}

static inline uint16_t mipc_nw_get_register_state_cnf_get_nw_err(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_NW_ERR, def_val);
}

static inline mipc_nw_register_mode_const_enum mipc_nw_get_register_state_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_nw_register_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_register_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_MODE, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_get_register_state_cnf_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_DATA_SPEED, def_val);
}

static inline char * mipc_nw_get_register_state_cnf_get_nw_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_NW_NAME, val_len_ptr);
}

static inline char * mipc_nw_get_register_state_cnf_get_roaming_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_ROAMING_TEXT, val_len_ptr);
}

static inline uint16_t mipc_nw_get_register_state_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_FAIL_CAUSE, def_val);
}

static inline char * mipc_nw_get_register_state_cnf_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_PLMN_ID, val_len_ptr);
}

static inline char * mipc_nw_get_register_state_cnf_get_nw_long_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_REGISTER_STATE_CNF_T_NW_LONG_NAME, val_len_ptr);
}

static inline uint8_t mipc_nw_get_plmn_list_cnf_get_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PLMN_LIST_CNF_T_INFO_COUNT, def_val);
}

static inline mipc_nw_provider_struct4* mipc_nw_get_plmn_list_cnf_get_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PLMN_LIST_CNF_T_INFO_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_plmn_list_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_PLMN_LIST_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_extend_provider_struct4* mipc_nw_get_plmn_list_cnf_get_extend_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_extend_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PLMN_LIST_CNF_T_EXTEND_INFO_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_ps_req_add_action(mipc_msg_t *msg_ptr, enum mipc_nw_ps_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PS_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ps_req_add_ctrl_mode(mipc_msg_t *msg_ptr, enum mipc_nw_ps_ctrl_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PS_REQ_T_CTRL_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_ps_const_enum mipc_nw_set_ps_cnf_get_tach(mipc_msg_t *msg_ptr, mipc_nw_ps_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_ps_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_PS_CNF_T_TACH, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_set_ps_cnf_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_PS_CNF_T_DATA_SPEED, def_val);
}

static inline uint8_t mipc_nw_set_ps_cnf_get_nw_frequency(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_PS_CNF_T_NW_FREQUENCY, def_val);
}

static inline uint16_t mipc_nw_set_ps_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_PS_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_ps_const_enum mipc_nw_get_ps_cnf_get_tach(mipc_msg_t *msg_ptr, mipc_nw_ps_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_ps_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PS_CNF_T_TACH, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_get_ps_cnf_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_PS_CNF_T_DATA_SPEED, def_val);
}

static inline uint8_t mipc_nw_get_ps_cnf_get_nw_frequency(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PS_CNF_T_NW_FREQUENCY, def_val);
}

static inline uint16_t mipc_nw_get_ps_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_PS_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_ps_reg_info_struct4* mipc_nw_get_ps_cnf_get_reg_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ps_reg_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PS_CNF_T_REG_INFO, val_len_ptr);
}

static inline mipc_nw_cell_type_const_enum mipc_nw_get_ps_cnf_get_cell_type(mipc_msg_t *msg_ptr, mipc_nw_cell_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_cell_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PS_CNF_T_CELL_TYPE, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_get_ps_cnf_get_cell_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PS_CNF_T_CELL_INFO, val_len_ptr);
}

static inline mipc_nw_nr_cell_struct4* mipc_nw_get_ps_cnf_get_nsa_ext_cell_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PS_CNF_T_NSA_EXT_CELL_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_signal_strength_interval(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_SIGNAL_STRENGTH_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_rssi_threshold(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_RSSI_THRESHOLD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_err_rate_threshold(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_ERR_RATE_THRESHOLD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_rsrp_threshold(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_RSRP_THRESHOLD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_snr_threshold(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_SNR_THRESHOLD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_req_add_threshold_mode(mipc_msg_t *msg_ptr, enum mipc_nw_signal_threshold_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_REQ_T_THRESHOLD_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_nw_set_signal_cnf_get_signal_strength_interval(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_SIGNAL_STRENGTH_INTERVAL, def_val);
}

static inline uint32_t mipc_nw_set_signal_cnf_get_rssi_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_RSSI_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_set_signal_cnf_get_err_rate_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_ERR_RATE_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_set_signal_cnf_get_rsrp_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_RSRP_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_set_signal_cnf_get_snr_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_SNR_THRESHOLD, def_val);
}

static inline uint16_t mipc_nw_set_signal_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_signal_type_const_enum mipc_nw_set_signal_cnf_get_signal_type(mipc_msg_t *msg_ptr, mipc_nw_signal_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_signal_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_SIGNAL_TYPE, def_val);
}

static inline mipc_nw_gsm_signal_strength_struct4* mipc_nw_set_signal_cnf_get_gsm_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_GSM_SIGNAL, val_len_ptr);
}

static inline mipc_nw_umts_signal_strength_struct4* mipc_nw_set_signal_cnf_get_umts_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_umts_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_UMTS_SIGNAL, val_len_ptr);
}

static inline mipc_nw_lte_signal_strength_struct4* mipc_nw_set_signal_cnf_get_lte_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_LTE_SIGNAL, val_len_ptr);
}

static inline mipc_nw_nr_signal_strength_struct4* mipc_nw_set_signal_cnf_get_nr_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_NR_SIGNAL, val_len_ptr);
}

static inline mipc_nw_raw_signal_info_struct4* mipc_nw_set_signal_cnf_get_raw_signal_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_raw_signal_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_RAW_SIGNAL_INFO_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_set_signal_cnf_get_raw_signal_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_RAW_SIGNAL_INFO_COUNT, def_val);
}

static inline mipc_nw_cdma_signal_strength_struct4* mipc_nw_set_signal_cnf_get_cdma_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cdma_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_SIGNAL_CNF_T_CDMA_SIGNAL, val_len_ptr);
}

static inline uint32_t mipc_nw_get_signal_cnf_get_signal_strength_interval(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_SIGNAL_STRENGTH_INTERVAL, def_val);
}

static inline uint32_t mipc_nw_get_signal_cnf_get_rssi_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_RSSI_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_get_signal_cnf_get_err_rate_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_ERR_RATE_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_get_signal_cnf_get_rsrp_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_RSRP_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_get_signal_cnf_get_snr_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_SNR_THRESHOLD, def_val);
}

static inline uint16_t mipc_nw_get_signal_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_signal_type_const_enum mipc_nw_get_signal_cnf_get_signal_type(mipc_msg_t *msg_ptr, mipc_nw_signal_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_signal_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_SIGNAL_TYPE, def_val);
}

static inline mipc_nw_gsm_signal_strength_struct4* mipc_nw_get_signal_cnf_get_gsm_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_GSM_SIGNAL, val_len_ptr);
}

static inline mipc_nw_umts_signal_strength_struct4* mipc_nw_get_signal_cnf_get_umts_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_umts_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_UMTS_SIGNAL, val_len_ptr);
}

static inline mipc_nw_lte_signal_strength_struct4* mipc_nw_get_signal_cnf_get_lte_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_LTE_SIGNAL, val_len_ptr);
}

static inline mipc_nw_nr_signal_strength_struct4* mipc_nw_get_signal_cnf_get_nr_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_NR_SIGNAL, val_len_ptr);
}

static inline mipc_nw_raw_signal_info_struct4* mipc_nw_get_signal_cnf_get_raw_signal_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_raw_signal_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_RAW_SIGNAL_INFO_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_get_signal_cnf_get_raw_signal_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_RAW_SIGNAL_INFO_COUNT, def_val);
}

static inline mipc_nw_cdma_signal_strength_struct4* mipc_nw_get_signal_cnf_get_cdma_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cdma_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SIGNAL_CNF_T_CDMA_SIGNAL, val_len_ptr);
}

static inline uint8_t mipc_nw_get_preferred_provider_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PREFERRED_PROVIDER_CNF_T_COUNT, def_val);
}

static inline mipc_nw_provider_struct4* mipc_nw_get_preferred_provider_cnf_get_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PREFERRED_PROVIDER_CNF_T_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_preferred_provider_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_PREFERRED_PROVIDER_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_preferred_provider_req_add_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PREFERRED_PROVIDER_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_preferred_provider_req_add_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_provider_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_PREFERRED_PROVIDER_REQ_T_LIST, len, (const void *)value);
}

static inline uint8_t mipc_nw_set_preferred_provider_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_PREFERRED_PROVIDER_CNF_T_COUNT, def_val);
}

static inline mipc_nw_provider_struct4* mipc_nw_set_preferred_provider_cnf_get_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_PREFERRED_PROVIDER_CNF_T_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_set_preferred_provider_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_PREFERRED_PROVIDER_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_home_provider_req_add_provider(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_provider_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_HOME_PROVIDER_REQ_T_PROVIDER, len, (const void *)value);
}

static inline mipc_nw_provider_struct4* mipc_nw_set_home_provider_cnf_get_provider(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SET_HOME_PROVIDER_CNF_T_PROVIDER, val_len_ptr);
}

static inline uint16_t mipc_nw_set_home_provider_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_HOME_PROVIDER_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_provider_struct4* mipc_nw_get_home_provider_cnf_get_provider(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_HOME_PROVIDER_CNF_T_PROVIDER, val_len_ptr);
}

static inline uint16_t mipc_nw_get_home_provider_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_HOME_PROVIDER_CNF_T_FAIL_CAUSE, def_val);
}

static inline char * mipc_nw_get_ia_status_cnf_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_APN, val_len_ptr);
}

static inline uint8_t mipc_nw_get_ia_status_cnf_get_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_RAT, def_val);
}

static inline mipc_apn_pdp_type_const_enum mipc_nw_get_ia_status_cnf_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_PDP_TYPE, def_val);
}

static inline mipc_apn_auth_type_const_enum mipc_nw_get_ia_status_cnf_get_auth_type(mipc_msg_t *msg_ptr, mipc_apn_auth_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_auth_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_AUTH_TYPE, def_val);
}

static inline char * mipc_nw_get_ia_status_cnf_get_userid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_USERID, val_len_ptr);
}

static inline char * mipc_nw_get_ia_status_cnf_get_password(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_IA_STATUS_CNF_T_PASSWORD, val_len_ptr);
}

static inline mipc_nw_nitz_info_struct4* mipc_nw_get_nitz_cnf_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nitz_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_NITZ_CNF_T_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_idle_hint_req_add_status(mipc_msg_t *msg_ptr, enum mipc_nw_fast_dormancy_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_IDLE_HINT_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_idle_hint_req_add_param_1(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_IDLE_HINT_REQ_T_PARAM_1, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_idle_hint_req_add_param_2(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_IDLE_HINT_REQ_T_PARAM_2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_fast_dormancy_const_enum mipc_nw_set_idle_hint_cnf_get_status(mipc_msg_t *msg_ptr, mipc_nw_fast_dormancy_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_fast_dormancy_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SET_IDLE_HINT_CNF_T_STATUS, def_val);
}

static inline uint16_t mipc_nw_set_idle_hint_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_SET_IDLE_HINT_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_fast_dormancy_const_enum mipc_nw_get_idle_hint_cnf_get_status(mipc_msg_t *msg_ptr, mipc_nw_fast_dormancy_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_fast_dormancy_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_IDLE_HINT_CNF_T_STATUS, def_val);
}

static inline uint16_t mipc_nw_get_idle_hint_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_IDLE_HINT_CNF_T_FAIL_CAUSE, def_val);
}

static inline uint8_t mipc_nw_get_idle_hint_cnf_get_r8_fd_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_IDLE_HINT_CNF_T_R8_FD_STATUS, def_val);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_gsm_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_GSM_CELL_COUNT, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_get_base_stations_cnf_get_gsm_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_GSM_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_umts_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_UMTS_CELL_COUNT, def_val);
}

static inline mipc_nw_umts_cell_struct4* mipc_nw_get_base_stations_cnf_get_umts_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_umts_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_UMTS_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_tdscdma_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_TDSCDMA_CELL_COUNT, def_val);
}

static inline mipc_nw_tdscdma_cell_struct4* mipc_nw_get_base_stations_cnf_get_tdscdma_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_tdscdma_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_TDSCDMA_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_lte_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_LTE_CELL_COUNT, def_val);
}

static inline mipc_nw_lte_cell_struct4* mipc_nw_get_base_stations_cnf_get_lte_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_LTE_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_cdma_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_CDMA_CELL_COUNT, def_val);
}

static inline mipc_nw_cdma_cell_struct4* mipc_nw_get_base_stations_cnf_get_cdma_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cdma_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_CDMA_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_get_base_stations_cnf_get_nr_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_NR_CELL_COUNT, def_val);
}

static inline mipc_nw_nr_cell_struct4* mipc_nw_get_base_stations_cnf_get_nr_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_NR_CELL_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_base_stations_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_BASE_STATIONS_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_location_info_struct4* mipc_nw_get_location_info_cnf_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_location_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_LOCATION_INFO_CNF_T_INFO, val_len_ptr);
}

static inline uint16_t mipc_nw_get_location_info_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_LOCATION_INFO_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_rat_req_add_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RAT_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_rat_req_add_prefer_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RAT_REQ_T_PREFER_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_rat_req_add_bit_rat(mipc_msg_t *msg_ptr, enum mipc_nw_bit_rat_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_RAT_REQ_T_BIT_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_provider_name_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PLMN_ID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_get_provider_name_req_add_lac(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_REQ_T_LAC, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_provider_name_req_add_option(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_REQ_T_OPTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_nw_get_provider_name_cnf_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_PLMN_ID, val_len_ptr);
}

static inline uint32_t mipc_nw_get_provider_name_cnf_get_lac(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_LAC, def_val);
}

static inline char * mipc_nw_get_provider_name_cnf_get_nw_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_NW_NAME, val_len_ptr);
}

static inline char * mipc_nw_get_provider_name_cnf_get_nw_name_long(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_NW_NAME_LONG, val_len_ptr);
}

static inline mipc_nw_name_pair_struct4* mipc_nw_get_provider_name_cnf_get_eons_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_name_pair_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_EONS_NAME, val_len_ptr);
}

static inline mipc_nw_name_pair_struct4* mipc_nw_get_provider_name_cnf_get_nitz_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_name_pair_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_NITZ_NAME, val_len_ptr);
}

static inline mipc_nw_name_pair_struct4* mipc_nw_get_provider_name_cnf_get_ts25_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_name_pair_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PROVIDER_NAME_CNF_T_TS25_NAME, val_len_ptr);
}

static inline uint8_t mipc_nw_get_rat_cnf_get_act(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RAT_CNF_T_ACT, def_val);
}

static inline uint8_t mipc_nw_get_rat_cnf_get_gprs_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RAT_CNF_T_GPRS_STATUS, def_val);
}

static inline uint8_t mipc_nw_get_rat_cnf_get_rat_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RAT_CNF_T_RAT_MODE, def_val);
}

static inline uint8_t mipc_nw_get_rat_cnf_get_prefer_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RAT_CNF_T_PREFER_RAT, def_val);
}

static inline uint8_t mipc_nw_get_rat_cnf_get_lock(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_RAT_CNF_T_LOCK, def_val);
}

static inline mipc_nw_bit_rat_const_enum mipc_nw_get_rat_cnf_get_bit_rat(mipc_msg_t *msg_ptr, mipc_nw_bit_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_bit_rat_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_RAT_CNF_T_BIT_RAT, def_val);
}

static inline uint32_t mipc_nw_get_rat_cnf_get_radio_capability(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_RAT_CNF_T_RADIO_CAPABILITY, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_nr_req_add_nr_opt(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_NR_REQ_T_NR_OPT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_nr_req_add_act_operation(mipc_msg_t *msg_ptr, enum mipc_nw_vg_option_operation_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_NR_REQ_T_ACT_OPERATION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_cs_reg_info_struct4* mipc_nw_get_cs_cnf_get_reg_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cs_reg_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CS_CNF_T_REG_INFO, val_len_ptr);
}

static inline mipc_nw_cell_type_const_enum mipc_nw_get_cs_cnf_get_cell_type(mipc_msg_t *msg_ptr, mipc_nw_cell_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_cell_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CS_CNF_T_CELL_TYPE, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_get_cs_cnf_get_cell_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CS_CNF_T_CELL_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_get_band_mode_req_add_option(mipc_msg_t *msg_ptr, enum mipc_nw_band_option_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_BAND_MODE_REQ_T_OPTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sys_cap_gsm_band_const_enum mipc_nw_get_band_mode_cnf_get_gsm_band(mipc_msg_t *msg_ptr, mipc_sys_cap_gsm_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_gsm_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BAND_MODE_CNF_T_GSM_BAND, def_val);
}

static inline mipc_sys_cap_umts_band_const_enum mipc_nw_get_band_mode_cnf_get_umts_band(mipc_msg_t *msg_ptr, mipc_sys_cap_umts_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_cap_umts_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_BAND_MODE_CNF_T_UMTS_BAND, def_val);
}

static inline mipc_sys_lte_band_struct4* mipc_nw_get_band_mode_cnf_get_lte_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_lte_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BAND_MODE_CNF_T_LTE_BAND, val_len_ptr);
}

static inline mipc_sys_nr_band_struct4* mipc_nw_get_band_mode_cnf_get_nr_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_nr_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BAND_MODE_CNF_T_NR_BAND, val_len_ptr);
}

static inline mipc_sys_nr_band_struct4* mipc_nw_get_band_mode_cnf_get_nr_nsa_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_nr_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BAND_MODE_CNF_T_NR_NSA_BAND, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_band_mode_req_add_gsm_band(mipc_msg_t *msg_ptr, enum mipc_sys_cap_gsm_band_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_BAND_MODE_REQ_T_GSM_BAND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_band_mode_req_add_umts_band(mipc_msg_t *msg_ptr, enum mipc_sys_cap_umts_band_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_BAND_MODE_REQ_T_UMTS_BAND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_band_mode_req_add_lte_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_lte_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_BAND_MODE_REQ_T_LTE_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_band_mode_req_add_nr_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_BAND_MODE_REQ_T_NR_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_band_mode_req_add_nr_nsa_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_BAND_MODE_REQ_T_NR_NSA_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_channel_lock_req_add_ch_lock_info_list_count(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_NW_SET_CHANNEL_LOCK_REQ_T_CH_LOCK_INFO_LIST_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_channel_lock_req_add_ch_lock_info_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_channel_lock_info_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_CHANNEL_LOCK_REQ_T_CH_LOCK_INFO_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_channel_lock_req_add_ch_lock_info_list_v1(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_nw_channel_lock_info_v1_struct4 *value)
{
    if (index >= MIPC_MAX_CHANNEL_LOCK_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_nw_channel_lock_info_v1_struct4* mipc_nw_get_channel_lock_cnf_get_ch_lock_info_list_v1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_channel_lock_info_v1_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CHANNEL_LOCK_CNF_T_CH_LOCK_INFO_LIST_V1, val_len_ptr);
}

static inline mipc_nw_pol_info_struct4* mipc_nw_get_pol_capability_cnf_get_pol_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_pol_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_POL_CAPABILITY_CNF_T_POL_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_prefer_rat_req_add_rat_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PREFER_RAT_REQ_T_RAT_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_prefer_rat_req_add_rat_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  4) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_PREFER_RAT_REQ_T_RAT_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_lte_carrier_aggregation_switch_req_add_status(mipc_msg_t *msg_ptr, enum mipc_nw_lte_carrier_arrregation_switch_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_LTE_CARRIER_AGGREGATION_SWITCH_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ps_cs_registration_state_roaming_type_req_add_ps_cs_reg_roaming_info(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_ps_cs_reg_roaming_info_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_PS_CS_REGISTRATION_STATE_ROAMING_TYPE_REQ_T_PS_CS_REG_ROAMING_INFO, len, (const void *)value);
}

static inline mipc_nw_lte_carrier_arrregation_switch_const_enum mipc_nw_get_lte_carrier_aggregation_switch_cnf_get_status(mipc_msg_t *msg_ptr, mipc_nw_lte_carrier_arrregation_switch_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_lte_carrier_arrregation_switch_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_LTE_CARRIER_AGGREGATION_SWITCH_CNF_T_STATUS, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_measurement_req_add_action(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_CELL_MEASUREMENT_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_cell_measurement_req_add_lte_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_lte_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_MEASUREMENT_REQ_T_LTE_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_measurement_req_add_nr_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_MEASUREMENT_REQ_T_NR_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_measurement_req_add_scan_type(mipc_msg_t *msg_ptr, enum mipc_nw_cellmeasurement_scan_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_CELL_MEASUREMENT_REQ_T_SCAN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_cell_measurement_cnf_get_lte_cell_list_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_LTE_CELL_LIST_COUNT, def_val);
}

static inline mipc_nw_cellmeasurement_info_struct4* mipc_nw_cell_measurement_cnf_get_lte_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cellmeasurement_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_LTE_CELL_LIST, val_len_ptr);
}

static inline mipc_cell_plmn_struct4* mipc_nw_cell_measurement_cnf_get_lte_plmn_array(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_PLMN_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_cell_plmn_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_LTE_PLMN_ARRAY, index, val_len_ptr);
}

static inline uint8_t mipc_nw_cell_measurement_cnf_get_nr_cell_list_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_NR_CELL_LIST_COUNT, def_val);
}

static inline mipc_nw_cellmeasurement_info_struct4* mipc_nw_cell_measurement_cnf_get_nr_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cellmeasurement_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_NR_CELL_LIST, val_len_ptr);
}

static inline mipc_cell_plmn_struct4* mipc_nw_cell_measurement_cnf_get_nr_plmn_array(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_PLMN_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_cell_plmn_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_NR_PLMN_ARRAY, index, val_len_ptr);
}

static inline mipc_nw_cell_band_bandwidth_struct4* mipc_nw_cell_measurement_cnf_get_lte_cell_band_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cell_band_bandwidth_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_LTE_CELL_BAND_LIST, val_len_ptr);
}

static inline mipc_nw_cell_band_bandwidth_struct4* mipc_nw_cell_measurement_cnf_get_nr_cell_band_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cell_band_bandwidth_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CELL_MEASUREMENT_CNF_T_NR_CELL_BAND_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_lte_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_lte_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_LTE_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_nr_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_NR_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_lte_cell_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_LTE_CELL_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_lte_cell_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_lte_cell_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_LTE_CELL_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_nr_cell_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_NR_CELL_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_nr_cell_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_nr_cell_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_NR_CELL_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_cell_band_white_list_lock_req_add_nr_nsa_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_CELL_BAND_WHITE_LIST_LOCK_REQ_T_NR_NSA_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_get_cell_band_bandwidth_req_add_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_cell_band_bandwidth_cnf_get_num_serving_cell(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NUM_SERVING_CELL, def_val);
}

static inline mipc_nw_cell_band_bandwidth_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_lte_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cell_band_bandwidth_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_LTE_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_cell_band_bandwidth_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_nr_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cell_band_bandwidth_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NR_SERVING_CELL_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_get_cell_band_bandwidth_cnf_get_lte_dl_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_LTE_DL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_get_cell_band_bandwidth_cnf_get_lte_ul_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_LTE_UL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_get_cell_band_bandwidth_cnf_get_nr_dl_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NR_DL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_get_cell_band_bandwidth_cnf_get_nr_ul_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NR_UL_SERVING_CELL_COUNT, def_val);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_lte_dl_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_LTE_DL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_lte_ul_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_LTE_UL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_nr_dl_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NR_DL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_get_cell_band_bandwidth_cnf_get_nr_ul_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CELL_BAND_BANDWIDTH_CNF_T_NR_UL_SERVING_CELL_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_get_nr_cnf_get_nr_opt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_NR_CNF_T_NR_OPT, def_val);
}

static inline mipc_nw_srxlev_info_struct4* mipc_nw_get_srxlev_cnf_get_lte_srxlev_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_srxlev_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SRXLEV_CNF_T_LTE_SRXLEV_INFO, val_len_ptr);
}

static inline mipc_nw_srxlev_info_struct4* mipc_nw_get_srxlev_cnf_get_nr_srxlev_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_srxlev_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SRXLEV_CNF_T_NR_SRXLEV_INFO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_mode_req_add_roaming_mode(mipc_msg_t *msg_ptr, enum mipc_nw_roaming_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_MODE_REQ_T_ROAMING_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_roaming_mode_const_enum mipc_nw_get_roaming_mode_cnf_get_roaming_mode(mipc_msg_t *msg_ptr, mipc_nw_roaming_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_roaming_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_MODE_CNF_T_ROAMING_MODE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_urc_enable_req_add_type(mipc_msg_t *msg_ptr, enum mipc_nw_ind_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_URC_ENABLE_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_urc_enable_req_add_enable(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_URC_ENABLE_REQ_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_suggested_plmn_list_req_add_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_suggested_plmn_list_req_add_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_REQ_T_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_suggested_plmn_list_req_add_timer(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_REQ_T_TIMER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_suggested_plmn_list_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_CNF_T_COUNT, def_val);
}

static inline mipc_nw_suggested_struct4* mipc_nw_get_suggested_plmn_list_cnf_get_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_suggested_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_CNF_T_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_suggested_plmn_list_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_SUGGESTED_PLMN_LIST_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_search_store_frequency_info_req_add_oper(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SEARCH_STORE_FREQUENCY_INFO_REQ_T_OPER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_search_store_frequency_info_req_add_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SEARCH_STORE_FREQUENCY_INFO_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_search_store_frequency_info_req_add_plmn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PLMN_ID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_SEARCH_STORE_FREQUENCY_INFO_REQ_T_PLMN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_search_store_frequency_info_req_add_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SEARCH_STORE_FREQUENCY_INFO_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_search_store_frequency_info_req_add_arfcn(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_arfcn_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_SEARCH_STORE_FREQUENCY_INFO_REQ_T_ARFCN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_select_femtocell_req_add_plmn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PLMN_ID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_SELECT_FEMTOCELL_REQ_T_PLMN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_select_femtocell_req_add_act(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SELECT_FEMTOCELL_REQ_T_ACT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_select_femtocell_req_add_csg_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  10) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_SELECT_FEMTOCELL_REQ_T_CSG_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_config_a2_offset_req_add_offset(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CONFIG_A2_OFFSET_REQ_T_OFFSET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_config_a2_offset_req_add_thresh_bound(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CONFIG_A2_OFFSET_REQ_T_THRESH_BOUND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_config_b1_offset_req_add_offset(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CONFIG_B1_OFFSET_REQ_T_OFFSET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_config_b1_offset_req_add_thresh_bound(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CONFIG_B1_OFFSET_REQ_T_THRESH_BOUND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_ebi(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_EBI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_is_ul(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_IS_UL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_beare_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_BEARE_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_bitrate(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_BITRATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_pdu_session_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_PDU_SESSION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_report_anbr_req_add_ext_param(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_REPORT_ANBR_REQ_T_EXT_PARAM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_network_event_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_NETWORK_EVENT_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_enable_ca_plus_filter_req_add_enable(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ENABLE_CA_PLUS_FILTER_REQ_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_femtocell_list_cnf_get_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_FEMTOCELL_LIST_CNF_T_CELL_COUNT, def_val);
}

static inline mipc_nw_femtocell_info_struct4* mipc_nw_get_femtocell_list_cnf_get_femtocell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_femtocell_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_FEMTOCELL_LIST_CNF_T_FEMTOCELL_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_femtocell_list_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_FEMTOCELL_LIST_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_pseudo_cell_mode_req_add_apc_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PSEUDO_CELL_MODE_REQ_T_APC_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_pseudo_cell_mode_req_add_urc_enable(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PSEUDO_CELL_MODE_REQ_T_URC_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_pseudo_cell_mode_req_add_timer(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_NW_SET_PSEUDO_CELL_MODE_REQ_T_TIMER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_pseudo_cell_info_cnf_get_apc_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_APC_MODE, def_val);
}

static inline uint8_t mipc_nw_get_pseudo_cell_info_cnf_get_urc_enable(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_URC_ENABLE, def_val);
}

static inline uint16_t mipc_nw_get_pseudo_cell_info_cnf_get_timer(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_TIMER, def_val);
}

static inline uint8_t mipc_nw_get_pseudo_cell_info_cnf_get_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_CELL_COUNT, def_val);
}

static inline mipc_nw_pseudocell_info_struct4* mipc_nw_get_pseudo_cell_info_cnf_get_pseudocell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_pseudocell_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_PSEUDOCELL_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_get_pseudo_cell_info_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_PSEUDO_CELL_INFO_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_protocol_index(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_PROTOCOL_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_dom_voice(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_DOM_VOICE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_dom_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_DOM_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_int_voice(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_INT_VOICE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_int_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_INT_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_roaming_enable_req_add_lte_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ROAMING_ENABLE_REQ_T_LTE_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_protocol_index(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_PROTOCOL_INDEX, def_val);
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_dom_voice(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_DOM_VOICE, def_val);
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_dom_data(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_DOM_DATA, def_val);
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_int_voice(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_INT_VOICE, def_val);
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_int_data(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_INT_DATA, def_val);
}

static inline uint8_t mipc_nw_get_roaming_enable_cnf_get_lte_data(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_LTE_DATA, def_val);
}

static inline uint16_t mipc_nw_get_roaming_enable_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_ROAMING_ENABLE_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_femtocell_system_selection_mode_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_FEMTOCELL_SYSTEM_SELECTION_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_query_femtocell_system_selection_mode_cnf_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_QUERY_FEMTOCELL_SYSTEM_SELECTION_MODE_CNF_T_MODE, def_val);
}

static inline uint16_t mipc_nw_query_femtocell_system_selection_mode_cnf_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_QUERY_FEMTOCELL_SYSTEM_SELECTION_MODE_CNF_T_FAIL_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_nw_ind_report_level_req_add_ps_state_level(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_NW_IND_REPORT_LEVEL_REQ_T_PS_STATE_LEVEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_nw_ind_report_level_req_add_cs_state_level(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_NW_IND_REPORT_LEVEL_REQ_T_CS_STATE_LEVEL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_disable_2g_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_DISABLE_2G_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_disable_2g_cnf_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_DISABLE_2G_CNF_T_MODE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_smart_rat_switch_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_nw_rat_switch_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SMART_RAT_SWITCH_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_smart_rat_switch_req_add_rat(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SMART_RAT_SWITCH_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_smart_rat_switch_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_nw_rat_switch_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_SMART_RAT_SWITCH_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_smart_rat_switch_cnf_get_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_SMART_RAT_SWITCH_CNF_T_STATE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_vss_antenna_conf_req_add_antenna_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_VSS_ANTENNA_CONF_REQ_T_ANTENNA_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_vss_antenna_conf_cnf_get_antenna_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_VSS_ANTENNA_CONF_CNF_T_ANTENNA_TYPE, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_primary_antenna_rssi(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_PRIMARY_ANTENNA_RSSI, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_relative_phase(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_RELATIVE_PHASE, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_secondary_antenna_rssi(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_SECONDARY_ANTENNA_RSSI, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_phase1(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_PHASE1, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_rx_state_0(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_RX_STATE_0, def_val);
}

static inline int32_t mipc_nw_vss_antenna_info_cnf_get_rx_state_1(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_VSS_ANTENNA_INFO_CNF_T_RX_STATE_1, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_radio_capability_req_add_radio_capability(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_RADIO_CAPABILITY_REQ_T_RADIO_CAPABILITY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_radio_capability_req_add_switch_mode_by_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_RADIO_CAPABILITY_REQ_T_SWITCH_MODE_BY_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cdma_roaming_preference_req_add_roaming_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CDMA_ROAMING_PREFERENCE_REQ_T_ROAMING_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_cdma_roaming_preference_cnf_get_roaming_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CDMA_ROAMING_PREFERENCE_CNF_T_ROAMING_TYPE, def_val);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_MODE, def_val);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_COUNT, def_val);
}

static inline mipc_nw_barring_info_struct4* mipc_nw_get_barring_info_cnf_get_barring_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_barring_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_BARRING_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_RAT, def_val);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_count_umts(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_COUNT_UMTS, def_val);
}

static inline mipc_nw_barring_info_struct4* mipc_nw_get_barring_info_cnf_get_barring_list_umts(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_barring_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_BARRING_LIST_UMTS, val_len_ptr);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_count_lte(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_COUNT_LTE, def_val);
}

static inline mipc_nw_barring_info_struct4* mipc_nw_get_barring_info_cnf_get_barring_list_lte(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_barring_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_BARRING_LIST_LTE, val_len_ptr);
}

static inline uint8_t mipc_nw_get_barring_info_cnf_get_count_nr(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_COUNT_NR, def_val);
}

static inline mipc_nw_barring_info_struct4* mipc_nw_get_barring_info_cnf_get_barring_list_nr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_barring_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_BARRING_INFO_CNF_T_BARRING_LIST_NR, val_len_ptr);
}

static inline uint8_t mipc_nw_get_ehrpd_info_cnf_get_rev(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_REV, def_val);
}

static inline uint16_t mipc_nw_get_ehrpd_info_cnf_get_mcc(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_MCC, def_val);
}

static inline uint16_t mipc_nw_get_ehrpd_info_cnf_get_mnc(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_MNC, def_val);
}

static inline uint16_t mipc_nw_get_ehrpd_info_cnf_get_nid(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_NID, def_val);
}

static inline uint16_t mipc_nw_get_ehrpd_info_cnf_get_sid(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_SID, def_val);
}

static inline uint16_t mipc_nw_get_ehrpd_info_cnf_get_bs_id(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_BS_ID, def_val);
}

static inline uint32_t mipc_nw_get_ehrpd_info_cnf_get_bs_lat(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_BS_LAT, def_val);
}

static inline uint32_t mipc_nw_get_ehrpd_info_cnf_get_bs_long(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_BS_LONG, def_val);
}

static inline char * mipc_nw_get_ehrpd_info_cnf_get_sector_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_SECTOR_ID, val_len_ptr);
}

static inline char * mipc_nw_get_ehrpd_info_cnf_get_subnet_mask(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_EHRPD_INFO_CNF_T_SUBNET_MASK, val_len_ptr);
}

static inline uint32_t mipc_nw_get_egmss_cnf_get_rat(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EGMSS_CNF_T_RAT, def_val);
}

static inline uint32_t mipc_nw_get_egmss_cnf_get_mcc(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EGMSS_CNF_T_MCC, def_val);
}

static inline uint32_t mipc_nw_get_egmss_cnf_get_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EGMSS_CNF_T_STATUS, def_val);
}

static inline uint32_t mipc_nw_get_egmss_cnf_get_cur_reported_rat(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EGMSS_CNF_T_CUR_REPORTED_RAT, def_val);
}

static inline uint32_t mipc_nw_get_egmss_cnf_get_is_home_country(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_EGMSS_CNF_T_IS_HOME_COUNTRY, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_cache_endc_connect_mode_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CACHE_ENDC_CONNECT_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cache_endc_connect_mode_req_add_timer1(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CACHE_ENDC_CONNECT_MODE_REQ_T_TIMER1, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cache_endc_connect_mode_req_add_timer2(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CACHE_ENDC_CONNECT_MODE_REQ_T_TIMER2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ps_test_mode_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_nw_ps_test_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_PS_TEST_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ps_test_mode_req_add_profile(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_PS_TEST_MODE_REQ_T_PROFILE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_ps_test_mode_const_enum mipc_nw_get_ps_test_mode_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_nw_ps_test_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_ps_test_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_PS_TEST_MODE_CNF_T_MODE, def_val);
}

static inline uint32_t mipc_nw_get_ps_test_mode_cnf_get_profile(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_PS_TEST_MODE_CNF_T_PROFILE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_rat(mipc_msg_t *msg_ptr, enum mipc_nw_signal_rat_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_signal_measurement(mipc_msg_t *msg_ptr, enum mipc_nw_signal_measurement_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_SIGNAL_MEASUREMENT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_hysteresis_ms(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_HYSTERESIS_MS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_hysteresis_db(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_HYSTERESIS_DB, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_signal_report_criteria_req_add_threshold(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_threshold_array_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_SIGNAL_REPORT_CRITERIA_REQ_T_THRESHOLD, len, (const void *)value);
}

static inline mipc_nw_ecainfo_struct4* mipc_nw_get_ecainfo_cnf_get_ecainfo(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ecainfo_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_ECAINFO_CNF_T_ECAINFO, val_len_ptr);
}

static inline mipc_nw_tx_struct4* mipc_nw_get_activity_info_cnf_get_tx(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_tx_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_ACTIVITY_INFO_CNF_T_TX, val_len_ptr);
}

static inline uint32_t mipc_nw_get_activity_info_cnf_get_rx(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_ACTIVITY_INFO_CNF_T_RX, def_val);
}

static inline uint32_t mipc_nw_get_activity_info_cnf_get_sleep_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_ACTIVITY_INFO_CNF_T_SLEEP_TIME, def_val);
}

static inline uint32_t mipc_nw_get_activity_info_cnf_get_idle_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_ACTIVITY_INFO_CNF_T_IDLE_TIME, def_val);
}

static inline uint8_t mipc_nw_get_activity_info_cnf_get_num_tx_levels(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_ACTIVITY_INFO_CNF_T_NUM_TX_LEVELS, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_ca_req_add_rat(mipc_msg_t *msg_ptr, enum mipc_nw_set_ca_rat_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CA_REQ_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ca_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_nw_ca_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CA_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_lte_rrc_state_cnf_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_LTE_RRC_STATE_CNF_T_MODE, def_val);
}

static inline mipc_lte_rrc_state_const_enum mipc_nw_get_lte_rrc_state_cnf_get_state(mipc_msg_t *msg_ptr, mipc_lte_rrc_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_lte_rrc_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_LTE_RRC_STATE_CNF_T_STATE, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_get_lte_1xrtt_cell_list_req_add_type(mipc_msg_t *msg_ptr, enum mipc_cell_list_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_LTE_1XRTT_CELL_LIST_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_lte_1xrtt_cell_list_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_LTE_1XRTT_CELL_LIST_CNF_T_COUNT, def_val);
}

static inline mipc_1xrtt_cell_info_struct4* mipc_nw_get_lte_1xrtt_cell_list_cnf_get_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_1xrtt_cell_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_LTE_1XRTT_CELL_LIST_CNF_T_CELL_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_get_ca_link_capability_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_ca_link_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_CA_LINK_CAPABILITY_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_ca_link_capability_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CA_LINK_CAPABILITY_CNF_T_COUNT, def_val);
}

static inline mipc_band_combo_info_struct4* mipc_nw_get_ca_link_capability_cnf_get_band_combo(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_band_combo_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CA_LINK_CAPABILITY_CNF_T_BAND_COMBO, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_get_ca_link_enable_status_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_ca_link_enable_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_CA_LINK_ENABLE_STATUS_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_ca_link_enable_status_req_add_band_combo(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_BAND_COMBO_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_GET_CA_LINK_ENABLE_STATUS_REQ_T_BAND_COMBO, len, (const void *)value);
}

static inline mipc_band_combo_enable_status_const_enum mipc_nw_get_ca_link_enable_status_cnf_get_status(mipc_msg_t *msg_ptr, mipc_band_combo_enable_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_band_combo_enable_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_CA_LINK_ENABLE_STATUS_CNF_T_STATUS, def_val);
}

static inline mipc_tm9_enable_status_const_enum mipc_nw_get_tm9_enable_status_cnf_get_tm9_fdd_setting(mipc_msg_t *msg_ptr, mipc_tm9_enable_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_tm9_enable_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_TM9_ENABLE_STATUS_CNF_T_TM9_FDD_SETTING, def_val);
}

static inline mipc_tm9_enable_status_const_enum mipc_nw_get_tm9_enable_status_cnf_get_tm9_tdd_setting(mipc_msg_t *msg_ptr, mipc_tm9_enable_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_tm9_enable_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_TM9_ENABLE_STATUS_CNF_T_TM9_TDD_SETTING, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_tm9_enable_status_req_add_type(mipc_msg_t *msg_ptr, enum mipc_tm9_setting_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_TM9_ENABLE_STATUS_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_tm9_enable_status_req_add_status(mipc_msg_t *msg_ptr, enum mipc_tm9_enable_status_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_TM9_ENABLE_STATUS_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_omadm_conf_req_add_type(mipc_msg_t *msg_ptr, enum mipc_omadm_node_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_OMADM_CONF_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_nw_get_omadm_conf_cnf_get_node_value(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_OMADM_CONF_CNF_T_NODE_VALUE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_omadm_conf_req_add_type(mipc_msg_t *msg_ptr, enum mipc_omadm_node_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_OMADM_CONF_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_omadm_conf_req_add_node_value(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_OMADM_VALUE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_OMADM_CONF_REQ_T_NODE_VALUE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_get_ca_band_mode_req_add_primary_id(mipc_msg_t *msg_ptr, int32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int32(msg_ptr, MIPC_NW_GET_CA_BAND_MODE_REQ_T_PRIMARY_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_nw_ca_band_struct4* mipc_nw_get_ca_band_mode_cnf_get_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ca_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_CA_BAND_MODE_CNF_T_BAND, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_ca_link_enable_status_req_add_list_type(mipc_msg_t *msg_ptr, enum mipc_ca_comb_list_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CA_LINK_ENABLE_STATUS_REQ_T_LIST_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_ca_link_enable_status_req_add_ca_comb_list(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  1632) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_CA_LINK_ENABLE_STATUS_REQ_T_CA_COMB_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_ca_link_enable_status_req_add_link_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CA_LINK_ENABLE_STATUS_REQ_T_LINK_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_reg_state(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_REG_STATE, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_mcc(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_MCC, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_mnc(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_MNC, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_cell_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_CELL_ID, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_band(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_BAND, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_bandwidth(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_BANDWIDTH, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_ul_channel(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_UL_CHANNEL, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_dl_channel(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_DL_CHANNEL, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_rssi(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_RSSI, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_rsrp(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_RSRP, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_rsrq(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_RSRQ, def_val);
}

static inline uint32_t mipc_nw_get_lte_data_cnf_get_tx_power(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_LTE_DATA_CNF_T_TX_POWER, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_tuw_timer_length_req_add_tuw_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_TUW_TIMER_LENGTH_REQ_T_TUW_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_tuw_timer_length_req_add_tuw(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_tuw_info_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_TUW_TIMER_LENGTH_REQ_T_TUW, len, (const void *)value);
}

static inline uint32_t mipc_nw_get_tuw_timer_length_cnf_get_tuw1(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_TUW_TIMER_LENGTH_CNF_T_TUW1, def_val);
}

static inline uint32_t mipc_nw_get_tuw_timer_length_cnf_get_tuw2(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_TUW_TIMER_LENGTH_CNF_T_TUW2, def_val);
}

static inline uint32_t mipc_nw_get_tuw_timer_length_cnf_get_tuw3(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_TUW_TIMER_LENGTH_CNF_T_TUW3, def_val);
}

static inline uint8_t mipc_nw_get_5guw_info_cnf_get_display_5guw(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_5GUW_INFO_CNF_T_DISPLAY_5GUW, def_val);
}

static inline uint8_t mipc_nw_get_5guw_info_cnf_get_on_n77_band(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_5GUW_INFO_CNF_T_ON_N77_BAND, def_val);
}

static inline uint8_t mipc_nw_get_5guw_info_cnf_get_on_fr2_band(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_5GUW_INFO_CNF_T_ON_FR2_BAND, def_val);
}

static inline uint8_t mipc_nw_get_5guw_info_cnf_get_5guw_allowed(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_5GUW_INFO_CNF_T_5GUW_ALLOWED, def_val);
}

static inline int32_t mipc_nw_get_nr_ca_band_cnf_get_is_endc(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_GET_NR_CA_BAND_CNF_T_IS_ENDC, def_val);
}

static inline uint8_t mipc_nw_get_nr_ca_band_cnf_get_band_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_NR_CA_BAND_CNF_T_BAND_NUM, def_val);
}

static inline mipc_nr_ca_band_struct4* mipc_nw_get_nr_ca_band_cnf_get_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nr_ca_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_NR_CA_BAND_CNF_T_BAND, val_len_ptr);
}

static inline int32_t mipc_nw_get_nr_scs_cnf_get_scs(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_GET_NR_SCS_CNF_T_SCS, def_val);
}

static inline mipc_physical_channel_info_v1_struct4* mipc_nw_get_physical_channel_configs_cnf_get_physical_channel_configs_list_v1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_PHYSICAL_CHANNEL_CONFIGS_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_physical_channel_info_v1_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_PHYSICAL_CHANNEL_CONFIGS_CNF_T_PHYSICAL_CHANNEL_CONFIGS_LIST_V1, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_allowed_mcc_list_req_add_action(mipc_msg_t *msg_ptr, enum mipc_nw_allowed_mcc_list_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_ALLOWED_MCC_LIST_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_allowed_mcc_list_req_add_allowed_mcc_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_nw_allowed_mcc_list_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_ALLOWED_MCC_LIST_REQ_T_ALLOWED_MCC_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_scan_type(mipc_msg_t *msg_ptr, enum mipc_nw_scan_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_START_NETWORK_SCAN_REQ_T_SCAN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_interval(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_START_NETWORK_SCAN_REQ_T_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_max_search_time(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_START_NETWORK_SCAN_REQ_T_MAX_SEARCH_TIME, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_incremental_results(mipc_msg_t *msg_ptr, enum mipc_nw_incremental_results_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_START_NETWORK_SCAN_REQ_T_INCREMENTAL_RESULTS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_incremental_result_periodicity(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_START_NETWORK_SCAN_REQ_T_INCREMENTAL_RESULT_PERIODICITY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_record_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_nw_record_info_struct4 *value)
{
    if (index >= MIPC_MAX_NWSCN_RECORD_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_start_network_scan_req_add_plmn_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_nw_plmn_info_struct4 *value)
{
    if (index >= MIPC_MAX_NWSCN_PLMN_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_pref_nssai_req_add_preferred_nssai_3gpp_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_s_nssai_struct_struct4 *value)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_pref_nssai_req_add_preferred_nssai_non3gpp_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_s_nssai_struct_struct4 *value)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_set_pref_nssai_cnf_get_preferred_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_SET_PREF_NSSAI_CNF_T_PREFERRED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_set_pref_nssai_cnf_get_preferred_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_SET_PREF_NSSAI_CNF_T_PREFERRED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_default_nssai_req_add_default_configured_nssai_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_s_nssai_struct_struct4 *value)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_set_default_nssai_cnf_get_default_configured_nssai_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_SET_DEFAULT_NSSAI_CNF_T_DEFAULT_CONFIGURED_NSSAI_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_get_nssai_req_add_nssai_type(mipc_msg_t *msg_ptr, enum mipc_nssai_type_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_GET_NSSAI_REQ_T_NSSAI_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_get_nssai_req_add_plmn_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  6) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_GET_NSSAI_REQ_T_PLMN_ID, len, (const void *)value);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_default_configured_nssai_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_DEFAULT_CONFIGURED_NSSAI_LIST, index, val_len_ptr);
}

static inline mipc_rejected_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_rejected_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_rejected_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_REJECTED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline mipc_rejected_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_rejected_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_rejected_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_REJECTED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_configured_nssai_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_CONFIGURED_NSSAI_LIST, index, val_len_ptr);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_allowed_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_ALLOWED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline mipc_plmn_specific_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_allowed_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_specific_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_ALLOWED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_preferred_nssai_3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_PREFERRED_NSSAI_3GPP_LIST, index, val_len_ptr);
}

static inline mipc_s_nssai_struct_struct4* mipc_nw_get_nssai_cnf_get_preferred_nssai_non3gpp_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_S_NSSAI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_s_nssai_struct_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_NSSAI_CNF_T_PREFERRED_NSSAI_NON3GPP_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_5guc_req_add_refresh_timer_length(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_5GUC_REQ_T_REFRESH_TIMER_LENGTH, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_5guc_req_add_nsa_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_5GUC_REQ_T_NSA_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_5guc_req_add_sa_band(mipc_msg_t *msg_ptr, uint16_t len, mipc_sys_nr_band_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_5GUC_REQ_T_SA_BAND, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_5guc_req_add_bw_check_enable(mipc_msg_t *msg_ptr, enum mipc_nw_bw_check_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_5GUC_REQ_T_BW_CHECK_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_5guc_req_add_bw_check_threshold(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_5GUC_REQ_T_BW_CHECK_THRESHOLD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_nw_get_5guc_setting_cnf_get_refresh_timer_length(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_SETTING_CNF_T_REFRESH_TIMER_LENGTH, def_val);
}

static inline mipc_sys_nr_band_struct4* mipc_nw_get_5guc_setting_cnf_get_nsa_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_nr_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_5GUC_SETTING_CNF_T_NSA_BAND, val_len_ptr);
}

static inline mipc_sys_nr_band_struct4* mipc_nw_get_5guc_setting_cnf_get_sa_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_nr_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_5GUC_SETTING_CNF_T_SA_BAND, val_len_ptr);
}

static inline mipc_nw_bw_check_const_enum mipc_nw_get_5guc_setting_cnf_get_bw_check_enable(mipc_msg_t *msg_ptr, mipc_nw_bw_check_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_bw_check_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_SETTING_CNF_T_BW_CHECK_ENABLE, def_val);
}

static inline uint32_t mipc_nw_get_5guc_setting_cnf_get_bw_check_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_SETTING_CNF_T_BW_CHECK_THRESHOLD, def_val);
}

static inline mipc_nw_5guc_state_const_enum mipc_nw_get_5guc_info_cnf_get_display_5guc(mipc_msg_t *msg_ptr, mipc_nw_5guc_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_5guc_state_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_INFO_CNF_T_DISPLAY_5GUC, def_val);
}

static inline mipc_nw_uc_band_const_enum mipc_nw_get_5guc_info_cnf_get_on_uc_band(mipc_msg_t *msg_ptr, mipc_nw_uc_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_uc_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_INFO_CNF_T_ON_UC_BAND, def_val);
}

static inline uint32_t mipc_nw_get_5guc_info_cnf_get_agg_bw(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_GET_5GUC_INFO_CNF_T_AGG_BW, def_val);
}

static inline char * mipc_nw_get_first_plmn_cnf_get_mcc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_FIRST_PLMN_CNF_T_MCC, val_len_ptr);
}

static inline char * mipc_nw_get_first_plmn_cnf_get_mnc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_GET_FIRST_PLMN_CNF_T_MNC, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_set_ue_usage_setting_req_add_usage_setting(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_UE_USAGE_SETTING_REQ_T_USAGE_SETTING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_nw_get_ue_usage_setting_cnf_get_usage_setting(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_GET_UE_USAGE_SETTING_CNF_T_USAGE_SETTING, def_val);
}

static inline mipc_msg_api_result_enum mipc_nw_set_cag_status_req_add_status(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CAG_STATUS_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cag_select_mode_req_add_select_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CAG_SELECT_MODE_REQ_T_SELECT_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cag_select_mode_req_add_plmn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_PLMN_ID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_NW_SET_CAG_SELECT_MODE_REQ_T_PLMN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_nw_set_cag_select_mode_req_add_cag_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_NW_SET_CAG_SELECT_MODE_REQ_T_CAG_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_nw_set_cag_select_mode_req_add_act(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_NW_SET_CAG_SELECT_MODE_REQ_T_ACT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_plmn_cag_info_struct4* mipc_nw_get_cag_list_cnf_get_plmn_cag_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_PLMN_CAG_INFO_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_plmn_cag_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_GET_CAG_LIST_CNF_T_PLMN_CAG_LIST, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_nw_os_id_update_req_add_os_id_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, mipc_os_id_info_struct4 *value)
{
    if (index >= MIPC_MAX_UE_OS_ID_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_pin_protect_req_add_pin_type(mipc_msg_t *msg_ptr, enum mipc_sim_pin_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_PIN_PROTECT_REQ_T_PIN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_pin_protect_req_add_pin_op(mipc_msg_t *msg_ptr, enum mipc_sim_pin_protection_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_PIN_PROTECT_REQ_T_PIN_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_pin_protect_req_add_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_PIN_PROTECT_REQ_T_PIN_CODE, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_pin_protect_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_PROTECT_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_pin_protect_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_PROTECT_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_pin_protect_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_PIN_PROTECT_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_change_pin_req_add_pin_type(mipc_msg_t *msg_ptr, enum mipc_sim_pin_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANGE_PIN_REQ_T_PIN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_change_pin_req_add_old_pin(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_CHANGE_PIN_REQ_T_OLD_PIN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_change_pin_req_add_new_pin(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_CHANGE_PIN_REQ_T_NEW_PIN, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_change_pin_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CHANGE_PIN_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_change_pin_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CHANGE_PIN_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_change_pin_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_CHANGE_PIN_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_verify_pin_req_add_pin_type(mipc_msg_t *msg_ptr, enum mipc_sim_pin_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_VERIFY_PIN_REQ_T_PIN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_verify_pin_req_add_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_VERIFY_PIN_REQ_T_PIN_CODE, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_verify_pin_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_VERIFY_PIN_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_verify_pin_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_VERIFY_PIN_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_verify_pin_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VERIFY_PIN_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_unblock_pin_req_add_pin_type(mipc_msg_t *msg_ptr, enum mipc_sim_pin_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_UNBLOCK_PIN_REQ_T_PIN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_unblock_pin_req_add_puk_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PUK_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UNBLOCK_PIN_REQ_T_PUK_CODE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_unblock_pin_req_add_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UNBLOCK_PIN_REQ_T_PIN_CODE, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_unblock_pin_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_UNBLOCK_PIN_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_unblock_pin_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_UNBLOCK_PIN_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_unblock_pin_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_UNBLOCK_PIN_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_get_pin_info_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_PIN_INFO_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_get_pin_info_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_PIN_INFO_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_get_pin_info_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_GET_PIN_INFO_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_pin1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_PIN1, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_pin2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_PIN2, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_nw_pin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_NW_PIN, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_sub_nw_pin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_SUB_NW_PIN, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_sp_pin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_SP_PIN, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_corp_pin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_CORP_PIN, val_len_ptr);
}

static inline mipc_sim_pin_desc_struct4* mipc_sim_get_pin_list_cnf_get_sim_pin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_pin_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_PIN_LIST_CNF_T_SIM_PIN, val_len_ptr);
}

static inline mipc_sim_state_const_enum mipc_sim_state_cnf_get_state(mipc_msg_t *msg_ptr, mipc_sim_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATE_CNF_T_STATE, def_val);
}

static inline uint32_t mipc_sim_state_cnf_get_sim_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATE_CNF_T_SIM_ID, def_val);
}

static inline uint32_t mipc_sim_state_cnf_get_ps_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATE_CNF_T_PS_ID, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_status_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_STATUS_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sim_status_const_enum mipc_sim_status_cnf_get_status(mipc_msg_t *msg_ptr, mipc_sim_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_STATUS, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_sim_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_SIM_ID, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_ps_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_PS_ID, def_val);
}

static inline mipc_sim_card_present_state_const_enum mipc_sim_status_cnf_get_card_present_state(mipc_msg_t *msg_ptr, mipc_sim_card_present_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_card_present_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_CARD_PRESENT_STATE, def_val);
}

static inline mipc_sim_enabled_const_enum mipc_sim_status_cnf_get_upin_status(mipc_msg_t *msg_ptr, mipc_sim_enabled_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_enabled_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_UPIN_STATUS, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_test_sim(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_TEST_SIM, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_gsm_app_idx(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_GSM_APP_IDX, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_cdma_app_idx(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_CDMA_APP_IDX, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_isim_app_idx(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_ISIM_APP_IDX, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_app_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_APP_COUNT, def_val);
}

static inline mipc_app_status_desc_struct4* mipc_sim_status_cnf_get_app_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_app_status_desc_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_STATUS_CNF_T_APP_LIST, val_len_ptr);
}

static inline char * mipc_sim_status_cnf_get_eid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_STATUS_CNF_T_EID, val_len_ptr);
}

static inline char * mipc_sim_status_cnf_get_iccid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_STATUS_CNF_T_ICCID, val_len_ptr);
}

static inline char * mipc_sim_status_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_STATUS_CNF_T_ATR, val_len_ptr);
}

static inline mipc_boolean_const_enum mipc_sim_status_cnf_get_msisdn_ready(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_MSISDN_READY, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_pin1(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_PIN1, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_pin2(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_PIN2, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_puk1(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_PUK1, def_val);
}

static inline uint8_t mipc_sim_status_cnf_get_puk2(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CNF_T_PUK2, def_val);
}

static inline char * mipc_sim_iccid_cnf_get_iccid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ICCID_CNF_T_ICCID, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_imsi_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_IMSI_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_imsi_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_ex_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_IMSI_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_sim_imsi_cnf_get_imsi(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_IMSI_CNF_T_IMSI, val_len_ptr);
}

static inline uint8_t mipc_sim_imsi_cnf_get_mnc_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_IMSI_CNF_T_MNC_LEN, def_val);
}

static inline uint8_t mipc_sim_msisdn_cnf_get_msisdn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_MSISDN_CNF_T_MSISDN_COUNT, def_val);
}

static inline mipc_sim_msisdn_struct4* mipc_sim_msisdn_cnf_get_msisdn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_msisdn_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_MSISDN_CNF_T_MSISDN_LIST, val_len_ptr);
}

static inline uint32_t mipc_sim_get_atr_info_cnf_get_atr_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_GET_ATR_INFO_CNF_T_ATR_LEN, def_val);
}

static inline char * mipc_sim_get_atr_info_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_ATR_INFO_CNF_T_ATR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_open_channel_req_add_app_id_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_OPEN_CHANNEL_REQ_T_APP_ID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_open_channel_req_add_app_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_OPEN_CHANNEL_REQ_T_APP_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_open_channel_req_add_p2(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_OPEN_CHANNEL_REQ_T_P2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_open_channel_req_add_channel_group(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_OPEN_CHANNEL_REQ_T_CHANNEL_GROUP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_sim_open_channel_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_OPEN_CHANNEL_CNF_T_SW, def_val);
}

static inline uint8_t mipc_sim_open_channel_cnf_get_channel(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_OPEN_CHANNEL_CNF_T_CHANNEL, def_val);
}

static inline void * mipc_sim_open_channel_cnf_get_resp(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_OPEN_CHANNEL_CNF_T_RESP, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_close_channel_req_add_channel_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CLOSE_CHANNEL_REQ_T_CHANNEL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_close_channel_req_add_channel_group(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CLOSE_CHANNEL_REQ_T_CHANNEL_GROUP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_sim_close_channel_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_CLOSE_CHANNEL_CNF_T_SW, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_session_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_SESSION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_cmd(mipc_msg_t *msg_ptr, enum mipc_sim_access_command_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_CMD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_file_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_FILE_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_p1(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_P1, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_p2(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_P2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_p3(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_P3, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_data_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_DATA_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_CMD_EXTENDED_DATA_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_path(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PATH_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_PATH, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_p3_ex(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_P3_EX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_restricted_access_req_add_fcp_convert(mipc_msg_t *msg_ptr, enum mipc_sim_fcp_convert_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_REQ_T_FCP_CONVERT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_sim_channel_restricted_access_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_channel_restricted_access_cnf_get_resp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_CNF_T_RESP_LEN, def_val);
}

static inline void * mipc_sim_channel_restricted_access_cnf_get_resp_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CHANNEL_RESTRICTED_ACCESS_CNF_T_RESP_APDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_channel_generic_access_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_generic_access_req_add_channel_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_REQ_T_CHANNEL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_generic_access_req_add_apdu_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_REQ_T_APDU_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_channel_generic_access_req_add_apdu(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_APDU_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_REQ_T_APDU, len, (const void *)value);
}

static inline uint16_t mipc_sim_channel_generic_access_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_channel_generic_access_cnf_get_resp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_CNF_T_RESP_LEN, def_val);
}

static inline void * mipc_sim_channel_generic_access_cnf_get_resp_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CHANNEL_GENERIC_ACCESS_CNF_T_RESP_APDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_version(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_VERSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_app_id_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_APP_ID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_app_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_APP_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_path_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PATH_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_PATH_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_file_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_FILE_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_file_offset(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_FILE_OFFSET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_number_of_bytes(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_NUMBER_OF_BYTES, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_local_pin(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_LOCAL_PIN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_binary_data_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_BINARY_DATA_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_long_apdu_access_req_add_binary_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_LONG_BIN_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_REQ_T_BINARY_DATA, len, (const void *)value);
}

static inline uint8_t mipc_sim_long_apdu_access_cnf_get_version(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_CNF_T_VERSION, def_val);
}

static inline uint16_t mipc_sim_long_apdu_access_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_long_apdu_access_cnf_get_data_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_CNF_T_DATA_LEN, def_val);
}

static inline void * mipc_sim_long_apdu_access_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_LONG_APDU_ACCESS_CNF_T_DATA, val_len_ptr);
}

static inline uint8_t mipc_sim_app_list_cnf_get_version(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_APP_LIST_CNF_T_VERSION, def_val);
}

static inline uint8_t mipc_sim_app_list_cnf_get_app_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_APP_LIST_CNF_T_APP_COUNT, def_val);
}

static inline uint8_t mipc_sim_app_list_cnf_get_active_app_idx(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_APP_LIST_CNF_T_ACTIVE_APP_idx, def_val);
}

static inline mipc_sim_app_info_struct4* mipc_sim_app_list_cnf_get_app_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_app_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_APP_LIST_CNF_T_APP_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_file_status_req_add_version(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_REQ_T_VERSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_file_status_req_add_aid_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_REQ_T_AID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_file_status_req_add_aid(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_FILE_STATUS_REQ_T_AID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_file_status_req_add_file_path_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_REQ_T_FILE_PATH_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_file_status_req_add_file_path(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PATH_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_FILE_STATUS_REQ_T_FILE_PATH, len, (const void *)value);
}

static inline uint8_t mipc_sim_file_status_cnf_get_version(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_VERSION, def_val);
}

static inline uint16_t mipc_sim_file_status_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_SW, def_val);
}

static inline mipc_sim_file_accessibility_const_enum mipc_sim_file_status_cnf_get_file_accessibility(mipc_msg_t *msg_ptr, mipc_sim_file_accessibility_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_file_accessibility_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_FILE_ACCESSIBILITY, def_val);
}

static inline mipc_sim_file_type_const_enum mipc_sim_file_status_cnf_get_file_type(mipc_msg_t *msg_ptr, mipc_sim_file_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_file_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_FILE_TYPE, def_val);
}

static inline mipc_sim_file_structure_const_enum mipc_sim_file_status_cnf_get_file(mipc_msg_t *msg_ptr, mipc_sim_file_structure_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_file_structure_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_FILE, def_val);
}

static inline uint8_t mipc_sim_file_status_cnf_get_item_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_ITEM_COUNT, def_val);
}

static inline uint8_t mipc_sim_file_status_cnf_get_size(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_SIZE, def_val);
}

static inline void * mipc_sim_file_status_cnf_get_lock_status(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_FILE_STATUS_CNF_T_LOCK_STATUS, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_get_reset_req_add_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_RESET_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sim_get_reset_cnf_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_RESET_CNF_T_MODE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_set_reset_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sim_pass_through_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_RESET_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sim_get_terminal_capability_cnf_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_TERMINAL_CAPABILITY_CNF_T_COUNT, def_val);
}

static inline uint16_t mipc_sim_get_terminal_capability_cnf_get_tc_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_TERMINAL_CAPABILITY_CNF_T_TC_LEN, def_val);
}

static inline void * mipc_sim_get_terminal_capability_cnf_get_tc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_TERMINAL_CAPABILITY_CNF_T_TC, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_set_terminal_capability_req_add_count(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_TERMINAL_CAPABILITY_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_terminal_capability_req_add_tc_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_SET_TERMINAL_CAPABILITY_REQ_T_TC_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_terminal_capability_req_add_tc(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_TERMINAL_CAPABILITY_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_TERMINAL_CAPABILITY_REQ_T_TC, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_pin_type(mipc_msg_t *msg_ptr, enum mipc_sim_pin_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_PIN_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_op(mipc_msg_t *msg_ptr, enum mipc_sim_pin_operation_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PUK_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_PIN_CODE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_new_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_NEW_PIN_CODE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_aid_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_AID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_pin_ex_req_add_aid(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_PIN_EX_REQ_T_AID, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_set_pin_ex_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SET_PIN_EX_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_set_pin_ex_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SET_PIN_EX_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_set_pin_ex_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SET_PIN_EX_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_get_pin_ex_req_add_version(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_PIN_EX_REQ_T_VERSION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_pin_ex_req_add_aid_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_PIN_EX_REQ_T_AID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_pin_ex_req_add_aid(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_PIN_EX_REQ_T_AID, len, (const void *)value);
}

static inline mipc_sim_pin_type_const_enum mipc_sim_get_pin_ex_cnf_get_pin_type(mipc_msg_t *msg_ptr, mipc_sim_pin_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_PIN_EX_CNF_T_PIN_TYPE, def_val);
}

static inline mipc_sim_pin_state_const_enum mipc_sim_get_pin_ex_cnf_get_pin_state(mipc_msg_t *msg_ptr, mipc_sim_pin_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_pin_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_PIN_EX_CNF_T_PIN_STATE, def_val);
}

static inline uint32_t mipc_sim_get_pin_ex_cnf_get_remaining_attempts(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_GET_PIN_EX_CNF_T_REMAINING_ATTEMPTS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_get_gsm_auth_req_add_rand1(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_GSM_AUTH_RAND_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_GSM_AUTH_REQ_T_RAND1, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_get_gsm_auth_req_add_rand2(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_GSM_AUTH_RAND_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_GSM_AUTH_REQ_T_RAND2, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_get_gsm_auth_req_add_rand3(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_GSM_AUTH_RAND_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_GSM_AUTH_REQ_T_RAND3, len, (const void *)value);
}

static inline uint16_t mipc_sim_get_gsm_auth_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_SW, def_val);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_sres1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_SRES1, val_len_ptr);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_kc1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_KC1, val_len_ptr);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_sres2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_SRES2, val_len_ptr);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_kc2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_KC2, val_len_ptr);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_sres3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_SRES3, val_len_ptr);
}

static inline void * mipc_sim_get_gsm_auth_cnf_get_kc3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_GSM_AUTH_CNF_T_KC3, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_get_ext_auth_req_add_ch(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_EXT_AUTH_REQ_T_CH, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_ext_auth_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_EXT_AUTH_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_ext_auth_req_add_cmd_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_GET_EXT_AUTH_REQ_T_CMD_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_ext_auth_req_add_cmd_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_EXT_AUTH_CMD_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_EXT_AUTH_REQ_T_CMD_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_get_ext_auth_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_EXT_AUTH_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_sim_get_ext_auth_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_EXT_AUTH_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_get_ext_auth_cnf_get_rsp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_EXT_AUTH_CNF_T_RSP_LEN, def_val);
}

static inline void * mipc_sim_get_ext_auth_cnf_get_rsp_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_EXT_AUTH_CNF_T_RSP_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_get_facility_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_FACILITY_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_facility_req_add_facility(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_FACILITY_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_FACILITY_REQ_T_FACILITY, len, (const void *)value);
}

static inline uint8_t mipc_sim_get_facility_cnf_get_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_FACILITY_CNF_T_STATUS, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_set_facility_req_add_app_id(mipc_msg_t *msg_ptr, enum mipc_sim_app_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_FACILITY_REQ_T_APP_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_facility_req_add_facility(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_FACILITY_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_FACILITY_REQ_T_FACILITY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_set_facility_req_add_pass_word(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_FACILITY_REQ_T_PASS_WORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_set_facility_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_FACILITY_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sim_set_facility_cnf_get_retry_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SET_FACILITY_CNF_T_RETRY_COUNT, def_val);
}

static inline uint8_t mipc_sim_get_euicc_slots_status_cnf_get_slots_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_SLOTS_INFO_COUNT, def_val);
}

static inline mipc_sim_slots_info_struct4* mipc_sim_get_euicc_slots_status_cnf_get_slots_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_slots_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_SLOTS_INFO_LIST, val_len_ptr);
}

static inline mipc_sim_card_present_state_const_enum mipc_sim_get_euicc_slots_status_cnf_get_card_state(mipc_msg_t *msg_ptr, mipc_sim_card_present_state_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_sim_card_present_state_const_enum)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_CARD_STATE, index, def_val);
}

static inline uint8_t mipc_sim_get_euicc_slots_status_cnf_get_slots_state(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_SLOTS_STATE, index, def_val);
}

static inline uint8_t mipc_sim_get_euicc_slots_status_cnf_get_logical_idx(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_LOGICAL_IDX, index, def_val);
}

static inline char * mipc_sim_get_euicc_slots_status_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_ATR, index, val_len_ptr);
}

static inline char * mipc_sim_get_euicc_slots_status_cnf_get_eid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_EID, index, val_len_ptr);
}

static inline char * mipc_sim_get_euicc_slots_status_cnf_get_iccid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_GET_EUICC_SLOTS_STATUS_CNF_T_ICCID, index, val_len_ptr);
}

static inline uint8_t mipc_sim_access_profile_connect_cnf_get_cur_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_CONNECT_CNF_T_CUR_TYPE, def_val);
}

static inline uint8_t mipc_sim_access_profile_connect_cnf_get_support_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_CONNECT_CNF_T_SUPPORT_TYPE, def_val);
}

static inline char * mipc_sim_access_profile_connect_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ACCESS_PROFILE_CONNECT_CNF_T_ATR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_access_profile_power_on_req_add_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_POWER_ON_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sim_access_profile_power_on_cnf_get_cur_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_POWER_ON_CNF_T_CUR_TYPE, def_val);
}

static inline char * mipc_sim_access_profile_power_on_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ACCESS_PROFILE_POWER_ON_CNF_T_ATR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_access_profile_reset_req_add_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_RESET_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_sim_access_profile_reset_cnf_get_cur_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_RESET_CNF_T_CUR_TYPE, def_val);
}

static inline char * mipc_sim_access_profile_reset_cnf_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ACCESS_PROFILE_RESET_CNF_T_ATR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_access_profile_apdu_req_add_type(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_ACCESS_PROFILE_APDU_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_access_profile_apdu_req_add_apdu(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_APDU_STRING_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_ACCESS_PROFILE_APDU_REQ_T_APDU, len, (const void *)value);
}

static inline char * mipc_sim_access_profile_apdu_cnf_get_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ACCESS_PROFILE_APDU_CNF_T_APDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_set_sim_power_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_SIM_POWER_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_sim_power_req_add_sim_power(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_SIM_POWER_REQ_T_SIM_POWER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_physical_slots_mapping_req_add_slots_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SET_PHYSICAL_SLOTS_MAPPING_REQ_T_SLOTS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_set_physical_slots_mapping_req_add_slots_mapping_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_NUM) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SET_PHYSICAL_SLOTS_MAPPING_REQ_T_SLOTS_MAPPING_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_session_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_SESSION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_cla(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_CLA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_ins(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_INS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_p1(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_P1, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_p2(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_P2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_p3(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_P3, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_data_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_DATA_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_extended_channel_generic_access_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_CMD_EXTENDED_DATA_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_REQ_T_DATA, len, (const void *)value);
}

static inline uint16_t mipc_sim_extended_channel_generic_access_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_extended_channel_generic_access_cnf_get_resp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_CNF_T_RESP_LEN, def_val);
}

static inline void * mipc_sim_extended_channel_generic_access_cnf_get_resp_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_EXTENDED_CHANNEL_GENERIC_ACCESS_CNF_T_RESP_APDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_app_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  16) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_APP_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_app_id_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_APP_ID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_file_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_FILE_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_record_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_RECORD_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_data_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_DATA_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_CMD_DATA_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_path(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PATH_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_PATH, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_record_req_add_pin2(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_REQ_T_PIN2, len, (const void *)value);
}

static inline uint16_t mipc_sim_uicc_file_access_record_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_uicc_file_access_record_cnf_get_resp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_CNF_T_RESP_LEN, def_val);
}

static inline void * mipc_sim_uicc_file_access_record_cnf_get_resp_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_RECORD_CNF_T_RESP_APDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_app_id(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  16) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_APP_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_app_id_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_APP_ID_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_file_id(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_FILE_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_offset(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_OFFSET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_data_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_DATA_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_CMD_DATA_BYTE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_path(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PATH_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_PATH, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_uicc_file_access_binary_req_add_pin2(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_PIN_CODE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_REQ_T_PIN2, len, (const void *)value);
}

static inline uint16_t mipc_sim_uicc_file_access_binary_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_uicc_file_access_binary_cnf_get_resp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_CNF_T_RESP_LEN, def_val);
}

static inline void * mipc_sim_uicc_file_access_binary_cnf_get_resp_apdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_UICC_FILE_ACCESS_BINARY_CNF_T_RESP_APDU, val_len_ptr);
}

static inline uint8_t mipc_sim_get_physical_slots_mapping_cnf_get_active_physical_slot_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_GET_PHYSICAL_SLOTS_MAPPING_CNF_T_ACTIVE_PHYSICAL_SLOT_ID, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_get_sim_auth_req_add_p2(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_GET_SIM_AUTH_REQ_T_P2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_sim_auth_req_add_aid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_AID_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_SIM_AUTH_REQ_T_AID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_get_sim_auth_req_add_cmd_len(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SIM_GET_SIM_AUTH_REQ_T_CMD_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_get_sim_auth_req_add_cmd_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_EXT_AUTH_CMD_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_GET_SIM_AUTH_REQ_T_CMD_DATA, len, (const void *)value);
}

static inline uint16_t mipc_sim_get_sim_auth_cnf_get_sw(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_SIM_AUTH_CNF_T_SW, def_val);
}

static inline uint16_t mipc_sim_get_sim_auth_cnf_get_rsp_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_GET_SIM_AUTH_CNF_T_RSP_LEN, def_val);
}

static inline void * mipc_sim_get_sim_auth_cnf_get_rsp_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_GET_SIM_AUTH_CNF_T_RSP_DATA, val_len_ptr);
}

static inline mipc_sim_crrst_state_const_enum mipc_sim_sml_get_allowed_carriers_cnf_get_state(mipc_msg_t *msg_ptr, mipc_sim_crrst_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_crrst_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_STATE, def_val);
}

static inline uint8_t mipc_sim_sml_get_allowed_carriers_cnf_get_multi_sim_policy(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_MULTI_SIM_POLICY, def_val);
}

static inline mipc_boolean_const_enum mipc_sim_sml_get_allowed_carriers_cnf_get_allowed_carriers_prioritized(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_ALLOWED_CARRIERS_PRIORITIZED, def_val);
}

static inline uint8_t mipc_sim_sml_get_allowed_carriers_cnf_get_allowed_carriers_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_ALLOWED_CARRIERS_NUM, def_val);
}

static inline mipc_sim_carrier_struct_struct4* mipc_sim_sml_get_allowed_carriers_cnf_get_allowed_carriers(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_carrier_struct_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_ALLOWED_CARRIERS, val_len_ptr);
}

static inline uint8_t mipc_sim_sml_get_allowed_carriers_cnf_get_excluded_carriers_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_EXCLUDED_CARRIERS_NUM, def_val);
}

static inline mipc_sim_carrier_struct_struct4* mipc_sim_sml_get_allowed_carriers_cnf_get_excluded_carriers(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_carrier_struct_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_GET_ALLOWED_CARRIERS_CNF_T_EXCLUDED_CARRIERS, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_multi_sim_policy(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_MULTI_SIM_POLICY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_allowed_carriers_prioritized(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_ALLOWED_CARRIERS_PRIORITIZED, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_allowed_carriers_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_ALLOWED_CARRIERS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_allowed_carriers(mipc_msg_t *msg_ptr, uint16_t len, mipc_sim_carrier_struct_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_ALLOWED_CARRIERS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_excluded_carriers_num(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_EXCLUDED_CARRIERS_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_allowed_carriers_req_add_excluded_carriers(mipc_msg_t *msg_ptr, uint16_t len, mipc_sim_carrier_struct_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_REQ_T_EXCLUDED_CARRIERS, len, (const void *)value);
}

static inline uint8_t mipc_sim_sml_set_allowed_carriers_cnf_get_allowed_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_CNF_T_ALLOWED_COUNT, def_val);
}

static inline uint8_t mipc_sim_sml_set_allowed_carriers_cnf_get_excluded_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_SET_ALLOWED_CARRIERS_CNF_T_EXCLUDED_COUNT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_enter_sim_depersonalization_req_add_category(mipc_msg_t *msg_ptr, enum mipc_sim_sml_category_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_ENTER_SIM_DEPERSONALIZATION_REQ_T_CATEGORY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_enter_sim_depersonalization_req_add_pin_code(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_SML_HCK_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_ENTER_SIM_DEPERSONALIZATION_REQ_T_PIN_CODE, len, (const void *)value);
}

static inline mipc_sim_sml_category_const_enum mipc_sim_sml_enter_sim_depersonalization_cnf_get_category(mipc_msg_t *msg_ptr, mipc_sim_sml_category_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_sml_category_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_ENTER_SIM_DEPERSONALIZATION_CNF_T_CATEGORY, def_val);
}

static inline uint8_t mipc_sim_sml_enter_sim_depersonalization_cnf_get_remain_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_ENTER_SIM_DEPERSONALIZATION_CNF_T_REMAIN_COUNT, def_val);
}

static inline void * mipc_sim_sml_enter_sim_depersonalization_cnf_get_remain_count_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_ENTER_SIM_DEPERSONALIZATION_CNF_T_REMAIN_COUNT_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_get_lock_req_add_category(mipc_msg_t *msg_ptr, enum mipc_sim_sml_category_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_REQ_T_CATEGORY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sim_sml_category_const_enum mipc_sim_sml_get_lock_cnf_get_category(mipc_msg_t *msg_ptr, mipc_sim_sml_category_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_sml_category_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_CATEGORY, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_STATE, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_retry_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_RETRY_CNT, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_autolock_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_AUTOLOCK_CNT, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_num_set(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_NUM_SET, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_total_set(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_TOTAL_SET, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_key_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_KEY_STATE, def_val);
}

static inline uint8_t mipc_sim_sml_get_lock_cnf_get_max_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_MAX_CNT, def_val);
}

static inline mipc_boolean_const_enum mipc_sim_sml_get_lock_cnf_get_rsu_enable(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_LOCK_CNF_T_RSU_ENABLE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_category(mipc_msg_t *msg_ptr, enum mipc_sim_sml_category_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_CATEGORY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_op(mipc_msg_t *msg_ptr, enum mipc_sim_sml_operation_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_SML_HCK_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_KEY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_data_imsi(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_IMSI_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_DATA_IMSI, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_gid1(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  3) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_GID1, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_set_lock_req_add_gid2(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  3) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_SET_LOCK_REQ_T_GID2, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_enter_device_depersonalization_req_add_key(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_SML_HCK_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_ENTER_DEVICE_DEPERSONALIZATION_REQ_T_KEY, len, (const void *)value);
}

static inline uint8_t mipc_sim_sml_enter_device_depersonalization_cnf_get_remain_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_ENTER_DEVICE_DEPERSONALIZATION_CNF_T_REMAIN_COUNT, def_val);
}

static inline uint8_t mipc_sim_sml_get_dev_lock_cnf_get_lock_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_DEV_LOCK_CNF_T_LOCK_STATE, def_val);
}

static inline uint8_t mipc_sim_sml_get_dev_lock_cnf_get_algo(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_DEV_LOCK_CNF_T_ALGO, def_val);
}

static inline uint8_t mipc_sim_sml_get_dev_lock_cnf_get_max_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_DEV_LOCK_CNF_T_MAX_CNT, def_val);
}

static inline uint8_t mipc_sim_sml_get_dev_lock_cnf_get_remain_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_DEV_LOCK_CNF_T_REMAIN_CNT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_operator_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_OPERATOR_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_request_id(mipc_msg_t *msg_ptr, enum mipc_sim_sml_rsu_operation_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_REQUEST_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_request_type(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_REQUEST_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  65535) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_rsv1(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_RSV1, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_rsv2(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_RSV2, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_sml_rsu_req_add_rsv_string(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  65535) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_SML_RSU_REQ_T_RSV_STRING, len, (const void *)value);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_operator_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_OPERATOR_ID, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_request_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_REQUEST_ID, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_request_type(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_REQUEST_TYPE, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_error_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_ERROR_CODE, def_val);
}

static inline char * mipc_sim_sml_rsu_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_DATA, val_len_ptr);
}

static inline void * mipc_sim_sml_rsu_cnf_get_time(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_TIME, val_len_ptr);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_version(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_VERSION, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_STATUS, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_rsv1(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_RSV1, def_val);
}

static inline uint32_t mipc_sim_sml_rsu_cnf_get_rsv2(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_RSV2, def_val);
}

static inline char * mipc_sim_sml_rsu_cnf_get_rsv_string(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_RSU_CNF_T_RSV_STRING, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_set_aka_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_VSIM_SET_AKA_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sim_vsim_set_aka_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_SET_AKA_CNF_T_RESULT, def_val);
}

static inline uint32_t mipc_sim_vsim_enable_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_ENABLE_CNF_T_RESULT, def_val);
}

static inline uint32_t mipc_sim_vsim_disable_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_DISABLE_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_plug_req_add_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_VSIM_PLUG_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_plug_req_add_sim_type(mipc_msg_t *msg_ptr, enum mipc_sim_vsim_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_VSIM_PLUG_REQ_T_SIM_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sim_vsim_plug_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_PLUG_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_set_timer_req_add_timer(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_VSIM_SET_TIMER_REQ_T_TIMER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sim_vsim_set_timer_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_SET_TIMER_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_reset_req_add_result(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_VSIM_RESET_REQ_T_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_reset_req_add_length(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_VSIM_RESET_REQ_T_LENGTH, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_reset_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_APDU_STRING_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_VSIM_RESET_REQ_T_DATA, len, (const void *)value);
}

static inline uint32_t mipc_sim_vsim_reset_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_RESET_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_apdu_req_add_length(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SIM_VSIM_APDU_REQ_T_LENGTH, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_apdu_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SIM_APDU_STRING_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SIM_VSIM_APDU_REQ_T_DATA, len, (const void *)value);
}

static inline uint32_t mipc_sim_vsim_apdu_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_APDU_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_sim_vsim_auth_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_sim_vsim_auth_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SIM_VSIM_AUTH_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_sim_vsim_auth_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_VSIM_AUTH_CNF_T_RESULT, def_val);
}

static inline uint32_t mipc_sim_cdma_subscription_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_RESULT, def_val);
}

static inline char * mipc_sim_cdma_subscription_cnf_get_msisdn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_MSISDN, val_len_ptr);
}

static inline uint8_t mipc_sim_cdma_subscription_cnf_get_sid_nid_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_SID_NID_COUNT, def_val);
}

static inline mipc_sid_nid_list_struct4* mipc_sim_cdma_subscription_cnf_get_sid_nid_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sid_nid_list_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_SID_NID_LIST, val_len_ptr);
}

static inline char * mipc_sim_cdma_subscription_cnf_get_vmin(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_VMIN, val_len_ptr);
}

static inline char * mipc_sim_cdma_subscription_cnf_get_vprlid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_CDMA_SUBSCRIPTION_CNF_T_VPRLID, val_len_ptr);
}

static inline uint32_t mipc_sim_cdma_get_subscription_source_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_CDMA_GET_SUBSCRIPTION_SOURCE_CNF_T_RESULT, def_val);
}

static inline uint8_t mipc_sim_cdma_get_subscription_source_cnf_get_uim_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CDMA_GET_SUBSCRIPTION_SOURCE_CNF_T_UIM_STATE, def_val);
}

static inline uint32_t mipc_sim_pin_count_query_cnf_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_PIN_COUNT_QUERY_CNF_T_RESULT, def_val);
}

static inline uint8_t mipc_sim_pin_count_query_cnf_get_pin1(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_COUNT_QUERY_CNF_T_PIN1, def_val);
}

static inline uint8_t mipc_sim_pin_count_query_cnf_get_pin2(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_COUNT_QUERY_CNF_T_PIN2, def_val);
}

static inline uint8_t mipc_sim_pin_count_query_cnf_get_puk1(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_COUNT_QUERY_CNF_T_PUK1, def_val);
}

static inline uint8_t mipc_sim_pin_count_query_cnf_get_puk2(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_PIN_COUNT_QUERY_CNF_T_PUK2, def_val);
}

static inline uint32_t mipc_sim_sml_get_network_lock_cnf_get_carrier_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_CARRIER_ID, def_val);
}

static inline uint32_t mipc_sim_sml_get_network_lock_cnf_get_supported_cats(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_SUPPORTED_CATS, def_val);
}

static inline uint32_t mipc_sim_sml_get_network_lock_cnf_get_version(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_VERSION, def_val);
}

static inline uint8_t mipc_sim_sml_get_network_lock_cnf_get_lock_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_LOCK_STATUS, def_val);
}

static inline mipc_boolean_const_enum mipc_sim_sml_get_network_lock_cnf_get_lock_fuse(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_LOCK_FUSE, def_val);
}

static inline mipc_boolean_const_enum mipc_sim_sml_get_network_lock_cnf_get_rsu_enable(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_RSU_ENABLE, def_val);
}

static inline uint16_t mipc_sim_sml_get_network_lock_cnf_get_rule(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_RULE, def_val);
}

static inline uint16_t mipc_sim_sml_get_network_lock_cnf_get_subrule(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_SML_GET_NETWORK_LOCK_CNF_T_SUBRULE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_set_sca(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_SCA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_CFG_REQ_T_SET_SCA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_get_sca(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_GET_SCA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_get_sms_state(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_GET_SMS_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_get_store_status(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_GET_STORE_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_prefer_ack(mipc_msg_t *msg_ptr, enum mipc_sms_ack_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_PREFER_ACK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_prefer_storage(mipc_msg_t *msg_ptr, enum mipc_sms_storage_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_PREFER_STORAGE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_get_all_can_get(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_GET_ALL_CAN_GET, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_set_host_mem_available(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_SET_HOST_MEM_AVAILABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_text_mode_param_action(mipc_msg_t *msg_ptr, enum mipc_sms_text_mode_param_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_TEXT_MODE_PARAM_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_text_mode_fo(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_TEXT_MODE_FO, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_text_mode_vp(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_TEXT_MODE_VP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_text_mode_pid(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_TEXT_MODE_PID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_text_mode_dcs(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_TEXT_MODE_DCS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_prefer_storage_c2k(mipc_msg_t *msg_ptr, enum mipc_sms_storage_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_PREFER_STORAGE_C2K, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_w_r_d_storage(mipc_msg_t *msg_ptr, enum mipc_sms_storage_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_W_R_D_STORAGE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cfg_req_add_save_setting(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CFG_REQ_T_SAVE_SETTING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sms_format_const_enum mipc_sms_cfg_cnf_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_FORMAT, def_val);
}

static inline char * mipc_sms_cfg_cnf_get_sca(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_CFG_CNF_T_SCA, val_len_ptr);
}

static inline mipc_sms_state_const_enum mipc_sms_cfg_cnf_get_sms_state(mipc_msg_t *msg_ptr, mipc_sms_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_SMS_STATE, def_val);
}

static inline uint16_t mipc_sms_cfg_cnf_get_max_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_CNF_T_MAX_MESSAGE, def_val);
}

static inline mipc_sms_ack_const_enum mipc_sms_cfg_cnf_get_prefer_ack(mipc_msg_t *msg_ptr, mipc_sms_ack_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_ack_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_PREFER_ACK, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_cnf_get_prefer_storage(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_PREFER_STORAGE, def_val);
}

static inline uint16_t mipc_sms_cfg_cnf_get_used_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_CNF_T_USED_MESSAGE, def_val);
}

static inline uint16_t mipc_sms_cfg_cnf_get_total_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_CNF_T_TOTAL_MESSAGE, def_val);
}

static inline uint8_t mipc_sms_cfg_cnf_get_text_mode_fo(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_TEXT_MODE_FO, def_val);
}

static inline uint8_t mipc_sms_cfg_cnf_get_text_mode_vp(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_TEXT_MODE_VP, def_val);
}

static inline uint8_t mipc_sms_cfg_cnf_get_text_mode_pid(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_TEXT_MODE_PID, def_val);
}

static inline uint8_t mipc_sms_cfg_cnf_get_text_mode_dcs(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_TEXT_MODE_DCS, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_cnf_get_prefer_storage_c2k(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_PREFER_STORAGE_C2K, def_val);
}

static inline uint16_t mipc_sms_cfg_cnf_get_used_message_c2k(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_CNF_T_USED_MESSAGE_C2K, def_val);
}

static inline uint16_t mipc_sms_cfg_cnf_get_max_message_c2k(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_CNF_T_MAX_MESSAGE_C2K, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_cnf_get_w_r_d_storage(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_CNF_T_W_R_D_STORAGE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_SEND_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_pdu(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_SEND_REQ_T_PDU, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_pdu_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_SEND_REQ_T_PDU_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_save(mipc_msg_t *msg_ptr, enum mipc_sms_send_save_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_SEND_REQ_T_SAVE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_more_msg_to_send(mipc_msg_t *msg_ptr, enum mipc_sms_more_msg_to_send_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_SEND_REQ_T_MORE_MSG_TO_SEND, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_SEND_REQ_T_PDU_C2K, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sms_send_req_add_num_c2k(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_NUM_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_SEND_REQ_T_NUM_C2K, len, (const void *)value);
}

static inline uint8_t mipc_sms_send_cnf_get_mr(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_SEND_CNF_T_MR, def_val);
}

static inline uint16_t mipc_sms_send_cnf_get_message_index(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_SEND_CNF_T_MESSAGE_INDEX, def_val);
}

static inline mipc_sms_c2k_err_class_const_enum mipc_sms_send_cnf_get_err_class_c2k(mipc_msg_t *msg_ptr, mipc_sms_c2k_err_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_c2k_err_class_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_SEND_CNF_T_ERR_CLASS_C2K, def_val);
}

static inline mipc_sms_format_const_enum mipc_sms_send_cnf_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_SEND_CNF_T_FORMAT, def_val);
}

static inline mipc_sms_c2k_err_code_const_enum mipc_sms_send_cnf_get_err_code_c2k(mipc_msg_t *msg_ptr, mipc_sms_c2k_err_code_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_c2k_err_code_const_enum)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_SEND_CNF_T_ERR_CODE_C2K, def_val);
}

static inline void * mipc_sms_send_cnf_get_ack_pdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_SEND_CNF_T_ACK_PDU, val_len_ptr);
}

static inline uint16_t mipc_sms_send_cnf_get_msgid_c2k(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_SEND_CNF_T_MSGID_C2K, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_read_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_READ_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_read_req_add_flag(mipc_msg_t *msg_ptr, enum mipc_sms_flag_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_READ_REQ_T_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_read_req_add_message_index(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SMS_READ_REQ_T_MESSAGE_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_read_req_add_status_unchange(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_READ_REQ_T_STATUS_UNCHANGE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sms_format_const_enum mipc_sms_read_cnf_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_READ_CNF_T_FORMAT, def_val);
}

static inline uint16_t mipc_sms_read_cnf_get_pdu_count(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_READ_CNF_T_PDU_COUNT, def_val);
}

static inline mipc_sms_pdu_struct4* mipc_sms_read_cnf_get_pdu_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sms_pdu_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_READ_CNF_T_PDU_LIST, val_len_ptr);
}

static inline void * mipc_sms_read_cnf_get_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_READ_CNF_T_PDU_C2K, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sms_delete_req_add_flag(mipc_msg_t *msg_ptr, enum mipc_sms_flag_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_DELETE_REQ_T_FLAG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_delete_req_add_message_index(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SMS_DELETE_REQ_T_MESSAGE_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_delete_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_DELETE_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_get_store_status_req_add_storage(mipc_msg_t *msg_ptr, enum mipc_sms_storage_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_GET_STORE_STATUS_REQ_T_STORAGE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_get_store_status_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_GET_STORE_STATUS_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sms_store_flag_const_enum mipc_sms_get_store_status_cnf_get_flag(mipc_msg_t *msg_ptr, mipc_sms_store_flag_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_store_flag_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SMS_GET_STORE_STATUS_CNF_T_FLAG, def_val);
}

static inline uint16_t mipc_sms_get_store_status_cnf_get_message_index(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_GET_STORE_STATUS_CNF_T_MESSAGE_INDEX, def_val);
}

static inline uint16_t mipc_sms_get_store_status_cnf_get_max_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_GET_STORE_STATUS_CNF_T_MAX_MESSAGE, def_val);
}

static inline uint16_t mipc_sms_get_store_status_cnf_get_used_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_GET_STORE_STATUS_CNF_T_USED_MESSAGE, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_get_store_status_cnf_get_storage(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_GET_STORE_STATUS_CNF_T_STORAGE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_WRITE_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_pdu(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_WRITE_REQ_T_PDU, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_pdu_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_WRITE_REQ_T_PDU_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_storage(mipc_msg_t *msg_ptr, enum mipc_sms_storage_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_WRITE_REQ_T_STORAGE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_status(mipc_msg_t *msg_ptr, enum mipc_sms_status_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_WRITE_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_write_req_add_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_WRITE_REQ_T_PDU_C2K, len, (const void *)value);
}

static inline uint16_t mipc_sms_write_cnf_get_message_index(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_WRITE_CNF_T_MESSAGE_INDEX, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_open_cbm_type(mipc_msg_t *msg_ptr, enum mipc_sms_cbm_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_OPEN_CBM_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_msg_id_cfg_type(mipc_msg_t *msg_ptr, enum mipc_sms_cbm_cfg_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_MSG_ID_CFG_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_msg_id_range(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint32_t value)
{
    if (index >= MIPC_MAX_SMS_CBM_MSGID_RANGE_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint32(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_msg_id_single(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t value)
{
    if (index >= MIPC_MAX_SMS_CBM_MSGID_SINGLE_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint16(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_dcs_cfg_type(mipc_msg_t *msg_ptr, enum mipc_sms_cbm_cfg_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_DCS_CFG_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_dcs_range(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t value)
{
    if (index >= MIPC_MAX_SMS_CBM_DCS_RANGE_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint16(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_dcs_single(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint8_t value)
{
    if (index >= MIPC_MAX_SMS_CBM_DCS_SINGLE_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint8(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_etws_primary_cfg(mipc_msg_t *msg_ptr, enum mipc_sms_etws_primary_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_ETWS_PRIMARY_CFG, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_c2k_cbm_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_C2K_CBM_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_cbm_cfg_req_add_pdu_3gpp_seg_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_CBM_CFG_REQ_T_PDU_3GPP_SEG_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_sms_cbm_type_const_enum mipc_sms_cbm_cfg_cnf_get_open_cbm_type(mipc_msg_t *msg_ptr, mipc_sms_cbm_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_cbm_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_OPEN_CBM_TYPE, def_val);
}

static inline mipc_sms_cbm_cfg_type_const_enum mipc_sms_cbm_cfg_cnf_get_msg_id_cfg_type(mipc_msg_t *msg_ptr, mipc_sms_cbm_cfg_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_cbm_cfg_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_MSG_ID_CFG_TYPE, def_val);
}

static inline uint32_t mipc_sms_cbm_cfg_cnf_get_msg_id_range(mipc_msg_t *msg_ptr, uint32_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_CBM_MSGID_RANGE_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_idx_uint32(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_MSG_ID_RANGE, index, def_val);
}

static inline uint16_t mipc_sms_cbm_cfg_cnf_get_msg_id_single(mipc_msg_t *msg_ptr, uint16_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_CBM_MSGID_SINGLE_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_idx_uint16(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_MSG_ID_SINGLE, index, def_val);
}

static inline mipc_sms_cbm_cfg_type_const_enum mipc_sms_cbm_cfg_cnf_get_dcs_cfg_type(mipc_msg_t *msg_ptr, mipc_sms_cbm_cfg_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_cbm_cfg_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_DCS_CFG_TYPE, def_val);
}

static inline uint16_t mipc_sms_cbm_cfg_cnf_get_dcs_range(mipc_msg_t *msg_ptr, uint16_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_CBM_DCS_RANGE_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_idx_uint16(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_DCS_RANGE, index, def_val);
}

static inline uint8_t mipc_sms_cbm_cfg_cnf_get_dcs_single(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_CBM_DCS_SINGLE_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_DCS_SINGLE, index, def_val);
}

static inline uint8_t mipc_sms_cbm_cfg_cnf_get_language_single(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_CBM_LANGUAGE_SINGLE_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_LANGUAGE_SINGLE, index, def_val);
}

static inline mipc_sms_format_const_enum mipc_sms_cbm_cfg_cnf_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_FORMAT, def_val);
}

static inline mipc_boolean_const_enum mipc_sms_cbm_cfg_cnf_get_c2k_cbm_enable(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CBM_CFG_CNF_T_C2K_CBM_ENABLE, def_val);
}

static inline mipc_msg_api_result_enum mipc_sms_scbm_req_add_quit_scbm_mode(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_SCBM_REQ_T_QUIT_SCBM_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_send_ussd_req_add_dcs(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SS_SEND_USSD_REQ_T_DCS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_send_ussd_req_add_payload_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SEND_USSD_REQ_T_PAYLOAD_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_send_ussd_req_add_payload(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_USSD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SEND_USSD_REQ_T_PAYLOAD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_send_ussd_req_add_lang(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SS_USSD_LANG_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SEND_USSD_REQ_T_LANG, len, (const void *)value);
}

static inline mipc_ss_ussd_const_enum mipc_ss_send_ussd_cnf_get_ussd_response(mipc_msg_t *msg_ptr, mipc_ss_ussd_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_ussd_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_SEND_USSD_CNF_T_USSD_RESPONSE, def_val);
}

static inline mipc_ss_session_const_enum mipc_ss_send_ussd_cnf_get_ussd_session_state(mipc_msg_t *msg_ptr, mipc_ss_session_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_session_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_SEND_USSD_CNF_T_USSD_SESSION_STATE, def_val);
}

static inline uint32_t mipc_ss_send_ussd_cnf_get_dcs(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_SEND_USSD_CNF_T_DCS, def_val);
}

static inline uint8_t mipc_ss_send_ussd_cnf_get_payload_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_SEND_USSD_CNF_T_PAYLOAD_LEN, def_val);
}

static inline void * mipc_ss_send_ussd_cnf_get_payload(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SEND_USSD_CNF_T_PAYLOAD, val_len_ptr);
}

static inline mipc_ss_ussd_const_enum mipc_ss_cancel_ussd_cnf_get_ussd_response(mipc_msg_t *msg_ptr, mipc_ss_ussd_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_ussd_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_CANCEL_USSD_CNF_T_USSD_RESPONSE, def_val);
}

static inline mipc_ss_session_const_enum mipc_ss_cancel_ussd_cnf_get_ussd_session_state(mipc_msg_t *msg_ptr, mipc_ss_session_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_session_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_CANCEL_USSD_CNF_T_USSD_SESSION_STATE, def_val);
}

static inline uint32_t mipc_ss_cancel_ussd_cnf_get_dcs(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_CANCEL_USSD_CNF_T_DCS, def_val);
}

static inline uint8_t mipc_ss_cancel_ussd_cnf_get_payload_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_CANCEL_USSD_CNF_T_PAYLOAD_LEN, def_val);
}

static inline void * mipc_ss_cancel_ussd_cnf_get_payload(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_CANCEL_USSD_CNF_T_PAYLOAD, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_clir_req_add_n_value(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CLIR_REQ_T_N_VALUE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_clir_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_CLIR_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline uint8_t mipc_ss_get_clir_cnf_get_clir_n(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_GET_CLIR_CNF_T_CLIR_N, def_val);
}

static inline uint8_t mipc_ss_get_clir_cnf_get_clir_m(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_GET_CLIR_CNF_T_CLIR_M, def_val);
}

static inline char * mipc_ss_get_clir_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_GET_CLIR_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_waiting_req_add_cw_enable_disable(mipc_msg_t *msg_ptr, enum mipc_ss_call_waiting_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_WAITING_REQ_T_CW_ENABLE_DISABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_waiting_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_SET_CALL_WAITING_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_call_waiting_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_CALL_WAITING_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_query_call_waiting_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_ss_active_status_const_enum mipc_ss_query_call_waiting_cnf_get_call_waiting_status(mipc_msg_t *msg_ptr, mipc_ss_active_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_active_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_CNF_T_CALL_WAITING_STATUS, def_val);
}

static inline mipc_ss_service_class_const_enum mipc_ss_query_call_waiting_cnf_get_service_class(mipc_msg_t *msg_ptr, mipc_ss_service_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_service_class_const_enum)mipc_msg_get_val_uint16(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_CNF_T_SERVICE_CLASS, def_val);
}

static inline uint8_t mipc_ss_query_call_waiting_cnf_get_cw_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_CNF_T_CW_COUNT, def_val);
}

static inline mipc_ss_call_waiting_struct4* mipc_ss_query_call_waiting_cnf_get_cw_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ss_call_waiting_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_CNF_T_CW_LIST, val_len_ptr);
}

static inline char * mipc_ss_query_call_waiting_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CALL_WAITING_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_ss_operation(mipc_msg_t *msg_ptr, enum mipc_ss_set_call_forward_operation_code_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_SS_OPERATION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_call_forward_reason(mipc_msg_t *msg_ptr, enum mipc_ss_call_forward_reason_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_CALL_FORWARD_REASON, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_dial_number(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SS_DIAL_NUMBER_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_DIAL_NUMBER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_toa(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_TOA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_timer_seconds(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_TIMER_SECONDS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_time_slot_begin(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SS_COMM_INFO_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_TIME_SLOT_BEGIN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_forward_req_add_time_slot_end(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SS_COMM_INFO_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_CALL_FORWARD_REQ_T_TIME_SLOT_END, len, (const void *)value);
}

static inline char * mipc_ss_set_call_forward_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_CALL_FORWARD_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_query_call_forward_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_QUERY_CALL_FORWARD_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_query_call_forward_req_add_call_forward_reason(mipc_msg_t *msg_ptr, enum mipc_ss_call_forward_reason_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_QUERY_CALL_FORWARD_REQ_T_CALL_FORWARD_REASON, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_ss_query_call_forward_cnf_get_call_forward_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_QUERY_CALL_FORWARD_CNF_T_CALL_FORWARD_COUNT, def_val);
}

static inline mipc_ss_call_forward_struct4* mipc_ss_query_call_forward_cnf_get_call_forward_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ss_call_forward_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CALL_FORWARD_CNF_T_CALL_FORWARD_LIST, val_len_ptr);
}

static inline char * mipc_ss_query_call_forward_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CALL_FORWARD_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_barring_req_add_lock(mipc_msg_t *msg_ptr, enum mipc_ss_call_barring_lock_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_BARRING_REQ_T_LOCK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_barring_req_add_facility(mipc_msg_t *msg_ptr, enum mipc_ss_call_barring_fac_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CALL_BARRING_REQ_T_FACILITY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_barring_req_add_password(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_CALL_BARRING_PASSWORD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_CALL_BARRING_REQ_T_PASSWORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_call_barring_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_SET_CALL_BARRING_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_call_barring_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_CALL_BARRING_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_query_call_barring_req_add_facility(mipc_msg_t *msg_ptr, enum mipc_ss_call_barring_fac_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_QUERY_CALL_BARRING_REQ_T_FACILITY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_query_call_barring_req_add_service_class(mipc_msg_t *msg_ptr, enum mipc_ss_service_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SS_QUERY_CALL_BARRING_REQ_T_SERVICE_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_ss_query_call_barring_cnf_get_call_barring_status(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SS_QUERY_CALL_BARRING_CNF_T_CALL_BARRING_STATUS, def_val);
}

static inline char * mipc_ss_query_call_barring_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CALL_BARRING_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_change_barring_password_req_add_facility(mipc_msg_t *msg_ptr, enum mipc_ss_call_barring_fac_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_CHANGE_BARRING_PASSWORD_REQ_T_FACILITY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_change_barring_password_req_add_old_pwd(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CHANGE_BARRING_PWD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_CHANGE_BARRING_PASSWORD_REQ_T_OLD_PWD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_change_barring_password_req_add_new_pwd(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CHANGE_BARRING_PWD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_CHANGE_BARRING_PASSWORD_REQ_T_NEW_PWD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_change_barring_password_req_add_new_pwd_confirm(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CHANGE_BARRING_PWD_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_CHANGE_BARRING_PASSWORD_REQ_T_NEW_PWD_CONFIRM, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_supp_svc_notification_req_add_status_i(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_SUPP_SVC_NOTIFICATION_REQ_T_STATUS_I, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_set_supp_svc_notification_req_add_status_u(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_SUPP_SVC_NOTIFICATION_REQ_T_STATUS_U, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_ss_query_clip_cnf_get_code_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_QUERY_CLIP_CNF_T_CODE_STATUS, def_val);
}

static inline uint8_t mipc_ss_query_clip_cnf_get_nw_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_QUERY_CLIP_CNF_T_NW_STATUS, def_val);
}

static inline char * mipc_ss_query_clip_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_QUERY_CLIP_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_clip_req_add_status(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_CLIP_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_clip_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_CLIP_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_run_gba_req_add_naf_fqdn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SYS_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_RUN_GBA_REQ_T_NAF_FQDN, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_run_gba_req_add_naf_secure_protocol_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SYS_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_RUN_GBA_REQ_T_NAF_SECURE_PROTOCOL_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_run_gba_req_add_force_run(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_RUN_GBA_REQ_T_FORCE_RUN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ss_run_gba_req_add_net_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_SS_RUN_GBA_REQ_T_NET_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_run_gba_cnf_get_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_RUN_GBA_CNF_T_KEY, val_len_ptr);
}

static inline uint8_t mipc_ss_run_gba_cnf_get_key_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_RUN_GBA_CNF_T_KEY_LEN, def_val);
}

static inline char * mipc_ss_run_gba_cnf_get_bit_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_RUN_GBA_CNF_T_BIT_ID, val_len_ptr);
}

static inline char * mipc_ss_run_gba_cnf_get_key_lifetime(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_RUN_GBA_CNF_T_KEY_LIFETIME, val_len_ptr);
}

static inline uint8_t mipc_ss_get_colp_cnf_get_colp_n(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_GET_COLP_CNF_T_COLP_N, def_val);
}

static inline uint8_t mipc_ss_get_colp_cnf_get_colp_m(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_GET_COLP_CNF_T_COLP_M, def_val);
}

static inline char * mipc_ss_get_colp_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_GET_COLP_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_set_colp_req_add_n_value(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_COLP_REQ_T_N_VALUE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_colp_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_COLP_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline uint8_t mipc_ss_get_colr_cnf_get_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_GET_COLR_CNF_T_STATUS, def_val);
}

static inline char * mipc_ss_get_colr_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_GET_COLR_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_ss_cnap_state_const_enum mipc_ss_send_cnap_cnf_get_cnap_n(mipc_msg_t *msg_ptr, mipc_ss_cnap_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_cnap_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_SEND_CNAP_CNF_T_CNAP_N, def_val);
}

static inline uint8_t mipc_ss_send_cnap_cnf_get_cnap_m(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_SEND_CNAP_CNF_T_CNAP_M, def_val);
}

static inline mipc_msg_api_result_enum mipc_ss_set_colr_req_add_n_value(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SS_SET_COLR_REQ_T_N_VALUE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_ss_set_colr_cnf_get_errmessage(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_SET_COLR_CNF_T_ERRMESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ss_setup_xcap_user_agent_req_add_str(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_XCAP_USER_AGENT_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SETUP_XCAP_USER_AGENT_REQ_T_STR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_xcap_cfg_req_add_cfg_name(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_XCAP_USER_AGENT_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_XCAP_CFG_REQ_T_CFG_NAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ss_set_xcap_cfg_req_add_value(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_XCAP_USER_AGENT_STR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SS_SET_XCAP_CFG_REQ_T_VALUE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_stk_set_pac_req_add_pac_bitmask_ptr(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_STK_SET_PAC_REQ_T_PAC_BITMASK_PTR, len, (const void *)value);
}

static inline void * mipc_stk_set_pac_cnf_get_pac_profile(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_SET_PAC_CNF_T_PAC_PROFILE, val_len_ptr);
}

static inline void * mipc_stk_get_pac_cnf_get_pac_profile(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_GET_PAC_CNF_T_PAC_PROFILE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_stk_send_terminal_response_req_add_tr_len(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_STK_SEND_TERMINAL_RESPONSE_REQ_T_TR_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_stk_send_terminal_response_req_add_tr_ptr(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_TERMINAL_RESPONSE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_STK_SEND_TERMINAL_RESPONSE_REQ_T_TR_PTR, len, (const void *)value);
}

static inline uint16_t mipc_stk_send_terminal_response_cnf_get_status_words(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_STK_SEND_TERMINAL_RESPONSE_CNF_T_STATUS_WORDS, def_val);
}

static inline uint32_t mipc_stk_send_terminal_response_cnf_get_tr_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_STK_SEND_TERMINAL_RESPONSE_CNF_T_TR_LEN, def_val);
}

static inline void * mipc_stk_send_terminal_response_cnf_get_tr_ptr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_SEND_TERMINAL_RESPONSE_CNF_T_TR_PTR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_stk_send_envelope_req_add_envelope_len(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_STK_SEND_ENVELOPE_REQ_T_ENVELOPE_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_stk_send_envelope_req_add_envelope_ptr(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_ENVELOP_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_STK_SEND_ENVELOPE_REQ_T_ENVELOPE_PTR, len, (const void *)value);
}

static inline uint16_t mipc_stk_send_envelope_cnf_get_status_words(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_STK_SEND_ENVELOPE_CNF_T_STATUS_WORDS, def_val);
}

static inline char * mipc_stk_send_envelope_cnf_get_envelope_response(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_SEND_ENVELOPE_CNF_T_ENVELOPE_RESPONSE, val_len_ptr);
}

static inline void * mipc_stk_get_envelope_info_cnf_get_envelope_bitmask(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_GET_ENVELOPE_INFO_CNF_T_ENVELOPE_BITMASK, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_stk_handle_call_setup_from_sim_req_add_data(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_STK_HANDLE_CALL_SETUP_FROM_SIM_REQ_T_DATA, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_stk_send_bipconf_req_add_cmd_num(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_STK_SEND_BIPCONF_REQ_T_CMD_NUM, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_stk_send_bipconf_req_add_result(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_STK_SEND_BIPCONF_REQ_T_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_dial_address(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DIAL_ADDRESS_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_DIAL_REQ_T_DIAL_ADDRESS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_dial_address_type(mipc_msg_t *msg_ptr, enum mipc_call_dial_address_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_DIAL_REQ_T_DIAL_ADDRESS_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_type(mipc_msg_t *msg_ptr, enum mipc_call_dial_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_DIAL_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_domain(mipc_msg_t *msg_ptr, enum mipc_call_dial_domain_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_DIAL_REQ_T_DOMAIN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_ecc_retry_domain(mipc_msg_t *msg_ptr, enum mipc_call_dial_domain_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_DIAL_REQ_T_ECC_RETRY_DOMAIN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_ecc_category(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_CALL_DIAL_REQ_T_ECC_CATEGORY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_clir(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_DIAL_REQ_T_CLIR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_is_ecc_testing(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_DIAL_REQ_T_IS_ECC_TESTING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dial_req_add_clir_ext(mipc_msg_t *msg_ptr, enum mipc_call_clir_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_DIAL_REQ_T_CLIR_EXT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ss_req_add_action(mipc_msg_t *msg_ptr, enum mipc_call_ss_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SS_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ss_req_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SS_REQ_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ss_req_add_ect_type(mipc_msg_t *msg_ptr, enum mipc_call_ect_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SS_REQ_T_ECT_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ss_req_add_ect_number(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_NUMBER_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SS_REQ_T_ECT_NUMBER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_hangup_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_call_hangup_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_HANGUP_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_hangup_req_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_HANGUP_REQ_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_hangup_req_add_cause(mipc_msg_t *msg_ptr, enum mipc_call_hangup_cause_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_HANGUP_REQ_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_answer_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_call_answer_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_ANSWER_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_answer_req_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_ANSWER_REQ_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_get_call_status_req_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_REQ_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_call_get_call_status_cnf_get_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_COUNT, def_val);
}

static inline uint32_t mipc_call_get_call_status_cnf_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_CALLID, index, def_val);
}

static inline mipc_call_direction_const_enum mipc_call_get_call_status_cnf_get_direction(mipc_msg_t *msg_ptr, mipc_call_direction_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_direction_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_DIRECTION, index, def_val);
}

static inline mipc_call_mode_const_enum mipc_call_get_call_status_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_call_mode_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_mode_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_MODE, index, def_val);
}

static inline mipc_call_clcc_state_const_enum mipc_call_get_call_status_cnf_get_call_clcc_state(mipc_msg_t *msg_ptr, mipc_call_clcc_state_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_clcc_state_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_CALL_CLCC_STATE, index, def_val);
}

static inline mipc_call_dial_address_type_const_enum mipc_call_get_call_status_cnf_get_number_type(mipc_msg_t *msg_ptr, mipc_call_dial_address_type_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_dial_address_type_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_NUMBER_TYPE, index, def_val);
}

static inline uint32_t mipc_call_get_call_status_cnf_get_ton(mipc_msg_t *msg_ptr, uint32_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_TON, index, def_val);
}

static inline char * mipc_call_get_call_status_cnf_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_NUMBER, index, val_len_ptr);
}

static inline mipc_call_rat_const_enum mipc_call_get_call_status_cnf_get_rat(mipc_msg_t *msg_ptr, mipc_call_rat_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_rat_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_RAT, index, def_val);
}

static inline mipc_call_type_const_enum mipc_call_get_call_status_cnf_get_type(mipc_msg_t *msg_ptr, mipc_call_type_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_type_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_TYPE, index, def_val);
}

static inline mipc_call_detail_info_struct4* mipc_call_get_call_status_cnf_get_detail_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_call_detail_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_DETAIL_INFO, index, val_len_ptr);
}

static inline mipc_call_video_cap_struct4* mipc_call_get_call_status_cnf_get_video_cap(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_call_video_cap_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CALL_STATUS_CNF_T_VIDEO_CAP, index, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_call_conference_req_add_conf_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_CONFERENCE_REQ_T_CONF_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_req_add_action(mipc_msg_t *msg_ptr, enum mipc_call_conf_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_CONFERENCE_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_req_add_number(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_NUMBER_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_CONFERENCE_REQ_T_NUMBER, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_conference_req_add_target_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_CONFERENCE_REQ_T_TARGET_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_call_reject_cause_const_enum mipc_call_conference_cnf_get_cause(mipc_msg_t *msg_ptr, mipc_call_reject_cause_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_reject_cause_const_enum)mipc_msg_get_val_uint16(msg_ptr, MIPC_CALL_CONFERENCE_CNF_T_CAUSE, def_val);
}

static inline mipc_msg_api_result_enum mipc_call_get_conference_info_req_add_conf_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_REQ_T_CONF_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_call_get_conference_info_cnf_get_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_COUNT, def_val);
}

static inline mipc_call_direction_const_enum mipc_call_get_conference_info_cnf_get_direction(mipc_msg_t *msg_ptr, mipc_call_direction_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_direction_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_DIRECTION, index, def_val);
}

static inline char * mipc_call_get_conference_info_cnf_get_participant_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_PARTICIPANT_NUMBER, index, val_len_ptr);
}

static inline char * mipc_call_get_conference_info_cnf_get_participant_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_PARTICIPANT_NAME, index, val_len_ptr);
}

static inline mipc_conf_participant_status_const_enum mipc_call_get_conference_info_cnf_get_participant_status(mipc_msg_t *msg_ptr, mipc_conf_participant_status_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_conf_participant_status_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_PARTICIPANT_STATUS, index, def_val);
}

static inline char * mipc_call_get_conference_info_cnf_get_participant_user_entity(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_PARTICIPANT_USER_ENTITY, index, val_len_ptr);
}

static inline char * mipc_call_get_conference_info_cnf_get_participant_endpoint_entity(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_GET_CONFERENCE_INFO_CNF_T_PARTICIPANT_ENDPOINT_ENTITY, index, val_len_ptr);
}

static inline uint32_t mipc_call_get_finish_reason_cnf_get_reason(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GET_FINISH_REASON_CNF_T_REASON, def_val);
}

static inline char * mipc_call_get_finish_reason_cnf_get_reason_str(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_GET_FINISH_REASON_CNF_T_REASON_STR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_call_dtmf_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_dtmf_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_DTMF_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_dtmf_req_add_digit(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DTMF_DIGIT_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_DTMF_REQ_T_DIGIT, len, (const void *)value);
}

static inline uint32_t mipc_call_get_ecc_list_cnf_get_info_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GET_ECC_LIST_CNF_T_INFO_COUNT, def_val);
}

static inline mipc_ecc_info_struct4* mipc_call_get_ecc_list_cnf_get_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_ecc_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_GET_ECC_LIST_CNF_T_INFO_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_call_set_ecc_list_req_add_info_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_ECC_LIST_REQ_T_INFO_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_ecc_list_req_add_info_list(mipc_msg_t *msg_ptr, uint16_t len, mipc_ecc_info_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_ECC_LIST_REQ_T_INFO_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_set_flight_mode_ecc_session_req_add_is_flight_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SET_FLIGHT_MODE_ECC_SESSION_REQ_T_IS_FLIGHT_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ivs_onekey_ecall_req_add_type(mipc_msg_t *msg_ptr, enum mipc_ecall_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_IVS_ONEKEY_ECALL_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ivs_onekey_ecall_req_add_msd_format(mipc_msg_t *msg_ptr, enum mipc_ecall_msd_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_IVS_ONEKEY_ECALL_REQ_T_MSD_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ivs_onekey_ecall_req_add_msd(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_ECALL_MSD_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_IVS_ONEKEY_ECALL_REQ_T_MSD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_set_sip_header_req_add_total(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_SIP_HEADER_REQ_T_TOTAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_sip_header_req_add_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_SIP_HEADER_REQ_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_sip_header_req_add_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_SIP_HEADER_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_sip_header_req_add_value_pair(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_SIP_HEADER_VALUE_PAIR_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_SIP_HEADER_REQ_T_VALUE_PAIR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_enable_ims_sip_header_report_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_ENABLE_IMS_SIP_HEADER_REPORT_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_enable_ims_sip_header_report_req_add_header_type(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_ENABLE_IMS_SIP_HEADER_REPORT_REQ_T_HEADER_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_call_additional_info_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_type(mipc_msg_t *msg_ptr, enum mipc_call_additional_info_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_total(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_TOTAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_call_additional_info_req_add_additional_info(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_ADDITIONAL_INFO_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_CALL_ADDITIONAL_INFO_REQ_T_ADDITIONAL_INFO, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_peer_rtt_modify_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_PEER_RTT_MODIFY_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_peer_rtt_modify_req_add_result(mipc_msg_t *msg_ptr, enum mipc_call_peer_rtt_modify_result_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_PEER_RTT_MODIFY_REQ_T_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_local_rtt_modify_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_local_rtt_modify_req_add_op(mipc_msg_t *msg_ptr, enum mipc_call_local_rtt_modify_op_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_call_local_rtt_modify_cnf_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_CNF_T_CALL_ID, def_val);
}

static inline uint8_t mipc_call_local_rtt_modify_cnf_get_result(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_CNF_T_RESULT, def_val);
}

static inline mipc_msg_api_result_enum mipc_call_rtt_text_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_RTT_TEXT_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_rtt_text_req_add_len(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_RTT_TEXT_REQ_T_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_rtt_text_req_add_text(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_RTT_TEXT_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_RTT_TEXT_REQ_T_TEXT, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_rtt_mode_req_add_op(mipc_msg_t *msg_ptr, enum mipc_call_rtt_mode_op_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_RTT_MODE_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_rtt_audio_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_RTT_AUDIO_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_rtt_audio_req_add_enable(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_RTT_AUDIO_REQ_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_rcs_state_and_feature_req_add_state(mipc_msg_t *msg_ptr, enum mipc_call_rcs_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_RCS_STATE_AND_FEATURE_REQ_T_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_rcs_state_and_feature_req_add_feature(mipc_msg_t *msg_ptr, enum mipc_call_rcs_feature_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_RCS_STATE_AND_FEATURE_REQ_T_FEATURE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_update_rcs_session_info_req_add_status(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_UPDATE_RCS_SESSION_INFO_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_call_voice_domain_const_enum mipc_call_get_voice_domain_preference_cnf_get_setting(mipc_msg_t *msg_ptr, mipc_call_voice_domain_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_voice_domain_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GET_VOICE_DOMAIN_PREFERENCE_CNF_T_SETTING, def_val);
}

static inline mipc_msg_api_result_enum mipc_call_pull_req_add_uri(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_CALL_PULL_URI_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_PULL_REQ_T_URI, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_pull_req_add_type(mipc_msg_t *msg_ptr, enum mipc_call_pull_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_PULL_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_call_tty_mode_const_enum mipc_call_get_tty_mode_cnf_get_mode(mipc_msg_t *msg_ptr, mipc_call_tty_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_tty_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_GET_TTY_MODE_CNF_T_MODE, def_val);
}

static inline mipc_msg_api_result_enum mipc_call_set_ims_call_mode_req_add_op(mipc_msg_t *msg_ptr, enum mipc_call_ims_call_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SET_IMS_CALL_MODE_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_tty_mode_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_call_tty_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_SET_TTY_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_voice_domain_preference_req_add_setting(mipc_msg_t *msg_ptr, enum mipc_call_voice_domain_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_VOICE_DOMAIN_PREFERENCE_REQ_T_SETTING, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_dial_req_add_type(mipc_msg_t *msg_ptr, enum mipc_call_conference_dial_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_CONFERENCE_DIAL_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_dial_req_add_clir(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_CONFERENCE_DIAL_REQ_T_CLIR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_dial_req_add_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_CONFERENCE_DIAL_REQ_T_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_conference_dial_req_add_dial_address(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, char  *value)
{
    if (index >= MIPC_MAX_CALL_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DIAL_ADDRESS_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_conference_dial_req_add_clir_ext(mipc_msg_t *msg_ptr, enum mipc_call_clir_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_CONFERENCE_DIAL_REQ_T_CLIR_EXT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_mode_req_add_action(mipc_msg_t *msg_ptr, enum mipc_set_gwsd_mode_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_GWSD_MODE_REQ_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_mode_req_add_mode(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_GWSD_MODE_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_mode_req_add_ka_mode(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_GWSD_MODE_REQ_T_KA_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_mode_req_add_ka_cycle(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  16) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_GWSD_MODE_REQ_T_KA_CYCLE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_approve_csfb_req_add_is_approve(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_APPROVE_CSFB_REQ_T_IS_APPROVE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_call_valid_req_add_timer(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_GWSD_CALL_VALID_REQ_T_TIMER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_ignore_call_interval_req_add_interval(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_SET_GWSD_IGNORE_CALL_INTERVAL_REQ_T_INTERVAL, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_ka_pdcp_req_add_pdata(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  16) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_GWSD_KA_PDCP_REQ_T_PDATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_set_gwsd_ka_ipdata_req_add_pdata(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  16) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_SET_GWSD_KA_IPDATA_REQ_T_PDATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_uis_info_req_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_UIS_INFO_REQ_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_uis_info_req_add_type(mipc_msg_t *msg_ptr, enum mipc_call_uis_info_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_UIS_INFO_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_uis_info_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_CALL_UIS_INFO_REQ_T_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_call_auto_answer_req_add_enable(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_AUTO_ANSWER_REQ_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_data_prefer_set_req_add_state(mipc_msg_t *msg_ptr, enum mipc_call_prefer_set_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_DATA_PREFER_SET_REQ_T_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_data_prefer_set_req_add_mode(mipc_msg_t *msg_ptr, enum mipc_call_prefer_set_monitor_mode_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_DATA_PREFER_SET_REQ_T_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ecc_redial_approve_req_add_approve(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_ECC_REDIAL_APPROVE_REQ_T_APPROVE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_ecc_redial_approve_req_add_call_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_ECC_REDIAL_APPROVE_REQ_T_CALL_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_config_req_add_class(mipc_msg_t *msg_ptr, enum mipc_ims_config_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_IMS_SET_CONFIG_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_config_req_add_type(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_CONFIG_REQ_T_TYPE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ims_set_config_req_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  60000) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_CONFIG_REQ_T_DATA, len, (const void *)value);
}

static inline void * mipc_ims_set_config_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SET_CONFIG_CNF_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ims_get_config_req_add_class(mipc_msg_t *msg_ptr, enum mipc_ims_config_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_IMS_GET_CONFIG_REQ_T_CLASS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_get_config_req_add_type(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_GET_CONFIG_REQ_T_TYPE, len, (const void *)value);
}

static inline void * mipc_ims_get_config_cnf_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_GET_CONFIG_CNF_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ims_get_state_req_add_event(mipc_msg_t *msg_ptr, enum mipc_ims_state_ind_event_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_GET_STATE_REQ_T_EVENT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_ims_state_ind_event_const_enum mipc_ims_get_state_cnf_get_event(mipc_msg_t *msg_ptr, mipc_ims_state_ind_event_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_state_ind_event_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_EVENT, def_val);
}

static inline mipc_ims_state_const_enum mipc_ims_get_state_cnf_get_reg_state(mipc_msg_t *msg_ptr, mipc_ims_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_REG_STATE, def_val);
}

static inline uint32_t mipc_ims_get_state_cnf_get_ext_info(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_EXT_INFO, def_val);
}

static inline uint8_t mipc_ims_get_state_cnf_get_wfc(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_WFC, def_val);
}

static inline uint32_t mipc_ims_get_state_cnf_get_account_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_ACCOUNT_ID, def_val);
}

static inline void * mipc_ims_get_state_cnf_get_uri(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_URI, val_len_ptr);
}

static inline uint32_t mipc_ims_get_state_cnf_get_expire_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_EXPIRE_TIME, def_val);
}

static inline uint32_t mipc_ims_get_state_cnf_get_error_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_ERROR_CODE, def_val);
}

static inline char * mipc_ims_get_state_cnf_get_error_message(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_GET_STATE_CNF_T_ERROR_MESSAGE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ims_set_pdis_req_add_transaction_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_PDIS_REQ_T_TRANSACTION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_pdis_req_add_method(mipc_msg_t *msg_ptr, enum mipc_pdis_method_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_PDIS_REQ_T_METHOD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_transaction_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_TRANSACTION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_mod_id(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_MOD_ID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_result(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_order(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_ORDER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_pref(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_PREF, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_flags(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  100) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_FLAGS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_service(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  100) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_SERVICE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_regexp(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  100) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_REGEXP, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ims_set_naptr_req_add_fqdn(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  256) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_IMS_SET_NAPTR_REQ_T_FQDN, len, (const void *)value);
}

static inline mipc_ims_report_status_const_enum mipc_ims_get_nw_rpt_cnf_get_report_status(mipc_msg_t *msg_ptr, mipc_ims_report_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_report_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_GET_NW_RPT_CNF_T_REPORT_STATUS, def_val);
}

static inline mipc_nw_ims_vops_status_const_enum mipc_ims_get_nw_rpt_cnf_get_nw_ims_vops(mipc_msg_t *msg_ptr, mipc_nw_ims_vops_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_ims_vops_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_GET_NW_RPT_CNF_T_NW_IMS_VOPS, def_val);
}

static inline mipc_msg_api_result_enum mipc_ims_set_test_mode_req_add_test_mode(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_TEST_MODE_REQ_T_TEST_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_reg_state(mipc_msg_t *msg_ptr, enum mipc_ims_reg_state_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_REG_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_reg_type(mipc_msg_t *msg_ptr, enum mipc_ims_reg_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_REG_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_ext_info(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_EXT_INFO, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_dereg_cause(mipc_msg_t *msg_ptr, enum mipc_ims_dereg_cause_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_DEREG_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_ims_rat(mipc_msg_t *msg_ptr, enum mipc_ims_rat_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_IMS_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_sip_uri_type(mipc_msg_t *msg_ptr, enum mipc_ims_uri_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_SIP_URI_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_eireg_req_add_ims_retry(mipc_msg_t *msg_ptr, enum mipc_ims_retry_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EIREG_REQ_T_IMS_RETRY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_scm_req_add_application(mipc_msg_t *msg_ptr, enum mipc_scm_application_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_SCM_REQ_T_APPLICATION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_scm_req_add_indication(mipc_msg_t *msg_ptr, enum mipc_scm_indication_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_SCM_REQ_T_INDICATION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_service_session_req_add_service_type(mipc_msg_t *msg_ptr, enum mipc_ims_service_type_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_SERVICE_SESSION_REQ_T_SERVICE_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_service_session_req_add_service_status(mipc_msg_t *msg_ptr, enum mipc_ims_service_status_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_SERVICE_SESSION_REQ_T_SERVICE_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_uac_req_add_service_type(mipc_msg_t *msg_ptr, enum mipc_ims_service_type_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_UAC_REQ_T_SERVICE_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_uac_req_add_service_status(mipc_msg_t *msg_ptr, enum mipc_ims_service_status_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_UAC_REQ_T_SERVICE_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_evodata_req_add_en(mipc_msg_t *msg_ptr, enum mipc_ims_evodata_en_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EVODATA_REQ_T_EN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_evodata_req_add_data_mode(mipc_msg_t *msg_ptr, enum mipc_ims_evodata_mode_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EVODATA_REQ_T_DATA_MODE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_evodata_req_add_allow_rat(mipc_msg_t *msg_ptr, enum mipc_ims_evodata_allowrat_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_SET_EVODATA_REQ_T_ALLOW_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_set_evodata_req_add_report_metric(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_IMS_SET_EVODATA_REQ_T_REPORT_METRIC, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_signal_type(mipc_msg_t *msg_ptr, enum mipc_wfc_signal_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_REGISTER_CELL_SIGNAL_IND_REQ_T_SIGNAL_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_enable_rpt(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_REGISTER_CELL_SIGNAL_IND_REQ_T_ENABLE_RPT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_time(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_WFC_REGISTER_CELL_SIGNAL_IND_REQ_T_TIME, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_threshold_in(mipc_msg_t *msg_ptr, int16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int16(msg_ptr, MIPC_WFC_REGISTER_CELL_SIGNAL_IND_REQ_T_THRESHOLD_IN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_threshold_out(mipc_msg_t *msg_ptr, int16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int16(msg_ptr, MIPC_WFC_REGISTER_CELL_SIGNAL_IND_REQ_T_THRESHOLD_OUT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_register_cell_signal_ind_req_add_threshold_ext(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, int16_t value)
{
    if (index >= MIPC_MAX_WFC_THRESHOLD_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_int16(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint16_t mipc_wfc_cssac_cnf_get_bf_voice(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_CSSAC_CNF_T_BF_VOICE, def_val);
}

static inline uint16_t mipc_wfc_cssac_cnf_get_bf_video(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_CSSAC_CNF_T_BF_VIDEO, def_val);
}

static inline uint16_t mipc_wfc_cssac_cnf_get_bt_voice(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_CSSAC_CNF_T_BT_VOICE, def_val);
}

static inline uint16_t mipc_wfc_cssac_cnf_get_bt_video(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_CSSAC_CNF_T_BT_VIDEO, def_val);
}

static inline mipc_msg_api_result_enum mipc_wfc_set_emc_aid_req_add_aid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_EMC_AID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_SET_EMC_AID_REQ_T_AID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_cfg_req_add_location_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_CFG_REQ_T_LOCATION_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_cfg_req_add_wfc_prefer(mipc_msg_t *msg_ptr, enum mipc_wfc_prefer_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_CFG_REQ_T_WFC_PREFER, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_op(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_OP, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_type(mipc_msg_t *msg_ptr, enum mipc_phb_ef_file_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_adn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_ADN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_ef_entry_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_EF_ENTRY_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_ton(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_TON, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_aas_id(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_AAS_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_grp_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_GRP_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_grp_id_list(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint32_t value)
{
    if (index >= MIPC_MAX_GRP_ID_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_idx_uint32(msg_ptr, array, index, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_line(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  248) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_LINE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_phb_set_upb_entry_req_add_encode_method(mipc_msg_t *msg_ptr, enum mipc_phb_encode_method_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_UPB_ENTRY_REQ_T_ENCODE_METHOD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_anr_email_sne_entry_req_add_type(mipc_msg_t *msg_ptr, enum mipc_phb_ef_file_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_anr_email_sne_entry_req_add_adn_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_REQ_T_ADN_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_anr_email_sne_entry_req_add_ef_entry_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_REQ_T_EF_ENTRY_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_phb_anr_entry_struct4* mipc_phb_get_upb_anr_email_sne_entry_cnf_get_phb_entry(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_anr_entry_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_CNF_T_PHB_ENTRY, val_len_ptr);
}

static inline mipc_phb_email_str_struct4* mipc_phb_get_upb_anr_email_sne_entry_cnf_get_email(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_email_str_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_CNF_T_EMAIL, val_len_ptr);
}

static inline mipc_phb_name_str_struct4* mipc_phb_get_upb_anr_email_sne_entry_cnf_get_snestr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_name_str_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_UPB_ANR_EMAIL_SNE_ENTRY_CNF_T_SNESTR, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_aas_gas_grp_list_entry_req_add_type(mipc_msg_t *msg_ptr, enum mipc_phb_ef_file_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_aas_gas_grp_list_entry_req_add_bindex(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_REQ_T_BINDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_upb_aas_gas_grp_list_entry_req_add_eindex(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_REQ_T_EINDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_aas_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_AAS_COUNT, def_val);
}

static inline mipc_phb_name_str_struct4* mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_aas_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_name_str_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_AAS_LIST, val_len_ptr);
}

static inline uint32_t mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_gas_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_GAS_COUNT, def_val);
}

static inline mipc_phb_name_str_struct4* mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_gas_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_name_str_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_GAS_LIST, val_len_ptr);
}

static inline uint32_t mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_grp_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_GRP_COUNT, def_val);
}

static inline uint32_t mipc_phb_get_upb_aas_gas_grp_list_entry_cnf_get_grp_id_list(mipc_msg_t *msg_ptr, uint32_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_GRP_ID_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_idx_uint32(msg_ptr, MIPC_PHB_GET_UPB_AAS_GAS_GRP_LIST_ENTRY_CNF_T_GRP_ID_LIST, index, def_val);
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_storage_info_req_add_storage_type(mipc_msg_t *msg_ptr, enum mipc_phb_stroage_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_REQ_T_STORAGE_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_phb_get_phb_storage_info_cnf_get_used(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_CNF_T_USED, def_val);
}

static inline uint32_t mipc_phb_get_phb_storage_info_cnf_get_total(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_CNF_T_TOTAL, def_val);
}

static inline uint32_t mipc_phb_get_phb_storage_info_cnf_get_nlength(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_CNF_T_NLENGTH, def_val);
}

static inline uint32_t mipc_phb_get_phb_storage_info_cnf_get_tlength(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_CNF_T_TLENGTH, def_val);
}

static inline mipc_phb_stroage_type_const_enum mipc_phb_get_phb_storage_info_cnf_get_storage_type(mipc_msg_t *msg_ptr, mipc_phb_stroage_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_phb_stroage_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STORAGE_INFO_CNF_T_STORAGE_TYPE, def_val);
}

static inline mipc_msg_api_result_enum mipc_phb_set_phb_mem_storage_req_add_storage_type(mipc_msg_t *msg_ptr, enum mipc_phb_stroage_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_PHB_MEM_STORAGE_REQ_T_STORAGE_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_phb_mem_storage_req_add_password(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  50) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_PHB_SET_PHB_MEM_STORAGE_REQ_T_PASSWORD, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_entry_req_add_bindex(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_REQ_T_BINDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_entry_req_add_eindex(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_REQ_T_EINDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_entry_req_add_storage_type(mipc_msg_t *msg_ptr, enum mipc_phb_stroage_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_REQ_T_STORAGE_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_entry_req_add_ext(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_REQ_T_EXT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_phb_get_phb_entry_cnf_get_entry_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_CNF_T_ENTRY_COUNT, def_val);
}

static inline mipc_phb_entry_struct4* mipc_phb_get_phb_entry_cnf_get_entry_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_phb_entry_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_PHB_GET_PHB_ENTRY_CNF_T_ENTRY_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_phb_set_phb_entry_req_add_type(mipc_msg_t *msg_ptr, enum mipc_phb_stroage_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_SET_PHB_ENTRY_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_phb_entry_req_add_ext(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_PHB_SET_PHB_ENTRY_REQ_T_EXT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_set_phb_entry_req_add_entry(mipc_msg_t *msg_ptr, uint16_t len, mipc_phb_entry_struct4 *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_PHB_SET_PHB_ENTRY_REQ_T_ENTRY, len, (const void *)value);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_num_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_NUM_LEN, def_val);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_alpha_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_ALPHA_LEN, def_val);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_aas_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_AAS_LEN, def_val);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_gas_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_GAS_LEN, def_val);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_sne_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_SNE_LEN, def_val);
}

static inline uint32_t mipc_phb_get_phb_stringslength_cnf_get_max_email_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_STRINGSLENGTH_CNF_T_MAX_EMAIL_LEN, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_anr(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_ANR, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_email(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_EMAIL, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_sne(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_SNE, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_aas(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_AAS, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_len_aas(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_LEN_AAS, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_gas(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_GAS, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_len_gas(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_LEN_GAS, def_val);
}

static inline uint32_t mipc_phb_get_upb_capability_cnf_get_num_grp(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_UPB_CAPABILITY_CNF_T_NUM_GRP, def_val);
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_available_req_add_type(mipc_msg_t *msg_ptr, enum mipc_phb_ef_file_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_AVAILABLE_REQ_T_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_phb_get_phb_available_req_add_index(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_PHB_GET_PHB_AVAILABLE_REQ_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_phb_get_phb_available_cnf_get_max_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_AVAILABLE_CNF_T_MAX_NUM, def_val);
}

static inline uint32_t mipc_phb_get_phb_available_cnf_get_available_num(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_AVAILABLE_CNF_T_AVAILABLE_NUM, def_val);
}

static inline uint32_t mipc_phb_get_phb_available_cnf_get_max_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_PHB_GET_PHB_AVAILABLE_CNF_T_MAX_LEN, def_val);
}

static inline uint8_t mipc_embms_emslu_cnf_get_is_enabled(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_EMSLU_CNF_T_IS_ENABLED, def_val);
}

static inline uint16_t mipc_embms_emslu_cnf_get_session_count(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_EMBMS_EMSLU_CNF_T_SESSION_COUNT, def_val);
}

static inline mipc_embms_session_info_struct4* mipc_embms_emslu_cnf_get_session_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_session_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_EMSLU_CNF_T_SESSION_LIST, val_len_ptr);
}

static inline uint8_t mipc_embms_get_sai_list_cnf_get_is_enabled(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_GET_SAI_LIST_CNF_T_IS_ENABLED, def_val);
}

static inline mipc_embms_sai_cf_info_struct4* mipc_embms_get_sai_list_cnf_get_sai_cf_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_sai_cf_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_GET_SAI_LIST_CNF_T_SAI_CF_INFO, val_len_ptr);
}

static inline uint8_t mipc_embms_get_sai_list_cnf_get_sai_nf_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_GET_SAI_LIST_CNF_T_SAI_NF_COUNT, def_val);
}

static inline mipc_embms_sai_nf_info_struct4* mipc_embms_get_sai_list_cnf_get_sai_nf_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_sai_nf_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_GET_SAI_LIST_CNF_T_SAI_NF_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_embms_notify_hvolte_status_req_add_status(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_EMBMS_NOTIFY_HVOLTE_STATUS_REQ_T_STATUS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_update_msd_req_add_msd_format(mipc_msg_t *msg_ptr, enum mipc_ecall_msd_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_ECALL_IVS_UPDATE_MSD_REQ_T_MSD_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_update_msd_req_add_msd_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_ECALL_MSD_DATA_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_ECALL_IVS_UPDATE_MSD_REQ_T_MSD_DATA, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_test_addr_req_add_addr_type(mipc_msg_t *msg_ptr, enum mipc_call_dial_address_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_ECALL_IVS_SET_TEST_ADDR_REQ_T_ADDR_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_test_addr_req_add_address(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DIAL_ADDRESS_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_ECALL_IVS_SET_TEST_ADDR_REQ_T_ADDRESS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_reconf_addr_req_add_addr_type(mipc_msg_t *msg_ptr, enum mipc_call_dial_address_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_ECALL_IVS_SET_RECONF_ADDR_REQ_T_ADDR_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_reconf_addr_req_add_address(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DIAL_ADDRESS_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_ECALL_IVS_SET_RECONF_ADDR_REQ_T_ADDRESS, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_addr_pri_req_add_first_pri(mipc_msg_t *msg_ptr, enum mipc_ecall_address_priority_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_ECALL_IVS_SET_ADDR_PRI_REQ_T_FIRST_PRI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_addr_pri_req_add_second_pri(mipc_msg_t *msg_ptr, enum mipc_ecall_address_priority_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_ECALL_IVS_SET_ADDR_PRI_REQ_T_SECOND_PRI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_addr_pri_req_add_third_pri(mipc_msg_t *msg_ptr, enum mipc_ecall_address_priority_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_ECALL_IVS_SET_ADDR_PRI_REQ_T_THIRD_PRI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ecall_ivs_set_addr_pri_req_add_fourth_pri(mipc_msg_t *msg_ptr, enum mipc_ecall_address_priority_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_ECALL_IVS_SET_ADDR_PRI_REQ_T_FOURTH_PRI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_ecall_sim_type_const_enum mipc_ecall_ivs_get_sim_info_cnf_get_sim_type(mipc_msg_t *msg_ptr, mipc_ecall_sim_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ecall_sim_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_ECALL_IVS_GET_SIM_INFO_CNF_T_SIM_TYPE, def_val);
}

static inline char * mipc_ecall_ivs_get_sim_info_cnf_get_test_ecall_uri(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_ECALL_IVS_GET_SIM_INFO_CNF_T_TEST_ECALL_URI, val_len_ptr);
}

static inline char * mipc_ecall_ivs_get_sim_info_cnf_get_test_ecall_num(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_ECALL_IVS_GET_SIM_INFO_CNF_T_TEST_ECALL_NUM, val_len_ptr);
}

static inline char * mipc_ecall_ivs_get_sim_info_cnf_get_reconf_ecall_uri(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_ECALL_IVS_GET_SIM_INFO_CNF_T_RECONF_ECALL_URI, val_len_ptr);
}

static inline char * mipc_ecall_ivs_get_sim_info_cnf_get_reconf_ecall_num(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_ECALL_IVS_GET_SIM_INFO_CNF_T_RECONF_ECALL_NUM, val_len_ptr);
}

static inline char * mipc_sys_at_ind_get_atcmd(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_AT_IND_T_ATCMD, val_len_ptr);
}

static inline int32_t mipc_sys_thermal_sensor_ind_get_temperature(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_TEMPERATURE, def_val);
}

static inline mipc_sys_thermal_sensor_config_struct4* mipc_sys_thermal_sensor_ind_get_threshold(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_config_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_THRESHOLD, val_len_ptr);
}

static inline uint8_t mipc_sys_thermal_sensor_ind_get_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_INFO_COUNT, def_val);
}

static inline mipc_sys_thermal_sensor_info_struct4* mipc_sys_thermal_sensor_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_INFO, val_len_ptr);
}

static inline mipc_sys_thermal_sensor_config_e_struct4* mipc_sys_thermal_sensor_ind_get_config_e(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_config_e_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_CONFIG_E, val_len_ptr);
}

static inline mipc_sys_thermal_sensor_info_e_struct4* mipc_sys_thermal_sensor_ind_get_info_e(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SYS_THERMAL_SENSOR_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_thermal_sensor_info_e_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SYS_THERMAL_SENSOR_IND_T_INFO_E, index, val_len_ptr);
}

static inline mipc_sys_config_change_reason_const_enum mipc_sys_config_ind_get_reason(mipc_msg_t *msg_ptr, mipc_sys_config_change_reason_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_config_change_reason_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONFIG_IND_T_REASON, def_val);
}

static inline mipc_sys_config_class_const_enum mipc_sys_config_ind_get_class(mipc_msg_t *msg_ptr, mipc_sys_config_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_config_class_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONFIG_IND_T_CLASS, def_val);
}

static inline uint8_t mipc_sys_adpclk_ind_get_freq_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_ADPCLK_IND_T_FREQ_INFO_COUNT, def_val);
}

static inline mipc_sys_adpclk_freq_info_struct8* mipc_sys_adpclk_ind_get_freq_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sys_adpclk_freq_info_struct8*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_ADPCLK_IND_T_FREQ_INFO_LIST, val_len_ptr);
}

static inline uint8_t mipc_sys_mcf_ind_get_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_IND_T_TYPE, def_val);
}

static inline uint8_t mipc_sys_mcf_ind_get_result(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MCF_IND_T_RESULT, def_val);
}

static inline uint32_t mipc_sys_sbp_ind_get_sbp_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_SBP_IND_T_SBP_ID, def_val);
}

static inline uint32_t mipc_sys_sbp_ind_get_sim_sbp_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_SBP_IND_T_SIM_SBP_ID, def_val);
}

static inline uint32_t mipc_sys_el2_ip_ul_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_IP_UL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_el2_ip_dl_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_IP_DL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_el2_mac_ul_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_MAC_UL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_el2_mac_dl_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_MAC_DL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_el2_pdcp_ul_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_PDCP_UL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_el2_pdcp_dl_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_EL2_PDCP_DL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_nl2_mac_ul_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_NL2_MAC_UL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_nl2_mac_dl_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_NL2_MAC_DL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_nl2_pdcp_ul_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_NL2_PDCP_UL_IND_T_TX_BPS, def_val);
}

static inline uint32_t mipc_sys_nl2_pdcp_dl_ind_get_tx_bps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_NL2_PDCP_DL_IND_T_TX_BPS, def_val);
}

static inline mipc_sys_account_id_const_enum mipc_sys_geo_location_ind_get_account_id(mipc_msg_t *msg_ptr, mipc_sys_account_id_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_account_id_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_ACCOUNT_ID, def_val);
}

static inline mipc_boolean_const_enum mipc_sys_geo_location_ind_get_broadcast_flag(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_BROADCAST_FLAG, def_val);
}

static inline char * mipc_sys_geo_location_ind_get_latitude(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_LATITUDE, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_longitude(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_LONGITUDE, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_accuracy(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_ACCURACY, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_method(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_METHOD, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_city(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_CITY, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_state(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_STATE, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_zip(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_ZIP, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_country_code(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_COUNTRY_CODE, val_len_ptr);
}

static inline char * mipc_sys_geo_location_ind_get_ue_wlan_mac(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_UE_WLAN_MAC, val_len_ptr);
}

static inline uint32_t mipc_sys_geo_location_ind_get_confidence(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GEO_LOCATION_IND_T_CONFIDENCE, def_val);
}

static inline mipc_md_init_id_const_enum mipc_sys_md_init_ind_get_init_id(mipc_msg_t *msg_ptr, mipc_md_init_id_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_md_init_id_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_MD_INIT_IND_T_INIT_ID, def_val);
}

static inline char * mipc_sys_warning_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_WARNING_IND_T_INFO, val_len_ptr);
}

static inline uint8_t mipc_sys_nv_sig_err_ind_get_error_code(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_NV_SIG_ERR_IND_T_ERROR_CODE, def_val);
}

static inline uint8_t mipc_sys_vodata_statistics_ind_get_sim_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_VODATA_STATISTICS_IND_T_SIM_ID, def_val);
}

static inline uint32_t mipc_sys_vodata_statistics_ind_get_tx_bytes(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_VODATA_STATISTICS_IND_T_TX_BYTES, def_val);
}

static inline uint32_t mipc_sys_vodata_statistics_ind_get_rx_bytes(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_VODATA_STATISTICS_IND_T_RX_BYTES, def_val);
}

static inline uint32_t mipc_sys_vodata_statistics_ind_get_tx_pkt(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_VODATA_STATISTICS_IND_T_TX_PKT, def_val);
}

static inline uint32_t mipc_sys_vodata_statistics_ind_get_rx_pkt(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_VODATA_STATISTICS_IND_T_RX_PKT, def_val);
}

static inline mipc_boolean_const_enum mipc_sys_thermal_actuator_ind_get_ims_only_ind(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_THERMAL_ACTUATOR_IND_T_IMS_ONLY_IND, def_val);
}

static inline mipc_boolean_const_enum mipc_sys_thermal_actuator_ind_get_flight_mode_ind(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_THERMAL_ACTUATOR_IND_T_FLIGHT_MODE_IND, def_val);
}

static inline mipc_boolean_const_enum mipc_sys_thermal_actuator_ind_get_charge_ind(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_THERMAL_ACTUATOR_IND_T_CHARGE_IND, def_val);
}

static inline char * mipc_sys_hba_timeout_ind_get_proxy_key(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_PROXY_KEY, val_len_ptr);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_ipv4_tos(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV4_TOS, def_val);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_ipv4_ttl(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV4_TTL, def_val);
}

static inline uint16_t mipc_sys_hba_timeout_ind_get_ipv4_id(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV4_ID, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_ipv6_flow_label(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV6_FLOW_LABEL, def_val);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_ipv6_hop_limit(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV6_HOP_LIMIT, def_val);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_ipv6_tclass(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_IPV6_TCLASS, def_val);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_tcp_rcv_wscale(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_RCV_WSCALE, def_val);
}

static inline uint8_t mipc_sys_hba_timeout_ind_get_tcp_snd_wscale(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_SND_WSCALE, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_tcp_wscale_ok(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_WSCALE_OK, def_val);
}

static inline uint16_t mipc_sys_hba_timeout_ind_get_tcp_snd_window(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_SND_WINDOW, def_val);
}

static inline uint16_t mipc_sys_hba_timeout_ind_get_tcp_rcv_window(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_RCV_WINDOW, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_tcp_snd_nxt_seq(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_SND_NXT_SEQ, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_tcp_rcv_nxt_seq(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_TCP_RCV_NXT_SEQ, def_val);
}

static inline uint16_t mipc_sys_hba_timeout_ind_get_current_cycle(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_CURRENT_CYCLE, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_send_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_SEND_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_recv_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_RECV_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_short_rrc_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_SHORT_RRC_COUNT, def_val);
}

static inline uint32_t mipc_sys_hba_timeout_ind_get_hitchhike_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_HBA_TIMEOUT_IND_T_HITCHHIKE_COUNT, def_val);
}

static inline uint8_t mipc_sys_hba_hw_filter_src_state_ind_get_hw_filter_src_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_HBA_HW_FILTER_SRC_STATE_IND_T_HW_FILTER_SRC_STATE, def_val);
}

static inline uint8_t mipc_apn_vzw_chg_ind_get_apn_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_APN_VZW_CHG_IND_T_APN_COUNT, def_val);
}

static inline mipc_vzw_apn_profile_struct4* mipc_apn_vzw_chg_ind_get_apn_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_vzw_apn_profile_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_APN_VZW_CHG_IND_T_APN_LIST, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_ID, def_val);
}

static inline char * mipc_data_act_call_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_APN, val_len_ptr);
}

static inline mipc_apn_pdp_type_const_enum mipc_data_act_call_ind_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PDP_TYPE, def_val);
}

static inline uint8_t mipc_data_act_call_ind_get_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_dns_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_dns_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_dns_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_dns_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_dns_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_dns_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_dns_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_dns_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_dns_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_dns_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_DNS_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_pcscf_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V4_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_pcscf_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_pcscf_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_pcscf_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_act_call_ind_get_pcscf_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_act_call_ind_get_pcscf_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_pcscf_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_pcscf_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_pcscf_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_pcscf_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_PCSCF_V6_3, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_gw_v4(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_GW_V4, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_act_call_ind_get_gw_v6(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_GW_V6, val_len_ptr);
}

static inline uint32_t mipc_data_act_call_ind_get_mtu_v4(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_MTU_V4, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_mtu_v6(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_MTU_V6, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_INTERFACE_ID, def_val);
}

static inline uint8_t mipc_data_act_call_ind_get_p_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_P_ID, def_val);
}

static inline uint8_t mipc_data_act_call_ind_get_fb_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_FB_ID, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_trans_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_TRANS_ID, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_ipv4_netmask(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_IPv4_NETMASK, def_val);
}

static inline mipc_ran_const_enum mipc_data_act_call_ind_get_ran_info(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_RAN_INFO, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_bearer_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_BEARER_ID, def_val);
}

static inline uint8_t mipc_data_act_call_ind_get_im_cn_signalling_flag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_IM_CN_SIGNALLING_FLAG, def_val);
}

static inline uint32_t mipc_data_act_call_ind_get_mtu_ethernet(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_ACT_CALL_IND_T_MTU_ETHERNET, def_val);
}

static inline uint8_t mipc_data_deact_call_ind_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DEACT_CALL_IND_T_ID, def_val);
}

static inline uint8_t mipc_data_deact_call_ind_get_res(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DEACT_CALL_IND_T_RES, def_val);
}

static inline uint32_t mipc_data_deact_call_ind_get_new_res(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_DEACT_CALL_IND_T_NEW_RES, def_val);
}

static inline uint8_t mipc_data_mod_call_ind_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_ID, def_val);
}

static inline char * mipc_data_mod_call_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_APN, val_len_ptr);
}

static inline mipc_apn_pdp_type_const_enum mipc_data_mod_call_ind_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PDP_TYPE, def_val);
}

static inline uint8_t mipc_data_mod_call_ind_get_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_mod_call_ind_get_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_mod_call_ind_get_dns_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_dns_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_dns_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_dns_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_dns_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_mod_call_ind_get_dns_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_dns_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_dns_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_dns_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_dns_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_DNS_V6_3, val_len_ptr);
}

static inline uint8_t mipc_data_mod_call_ind_get_pcscf_v4_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V4_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v4_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V4_0, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v4_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V4_1, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v4_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V4_2, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v4_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V4_3, val_len_ptr);
}

static inline uint8_t mipc_data_mod_call_ind_get_pcscf_v6_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V6_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v6_0(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V6_0, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v6_1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V6_1, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v6_2(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V6_2, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_pcscf_v6_3(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_PCSCF_V6_3, val_len_ptr);
}

static inline mipc_data_v4_addr_struct4* mipc_data_mod_call_ind_get_gw_v4(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_GW_V4, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_data_mod_call_ind_get_gw_v6(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_GW_V6, val_len_ptr);
}

static inline uint32_t mipc_data_mod_call_ind_get_mtu_v4(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_MTU_V4, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_mtu_v6(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_MTU_V6, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_INTERFACE_ID, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_trans_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_TRANS_ID, def_val);
}

static inline mipc_ran_const_enum mipc_data_mod_call_ind_get_ran_info(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_RAN_INFO, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_bearer_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_BEARER_ID, def_val);
}

static inline uint8_t mipc_data_mod_call_ind_get_im_cn_signalling_flag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_IM_CN_SIGNALLING_FLAG, def_val);
}

static inline uint8_t mipc_data_mod_call_ind_get_p_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_P_ID, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_mtu_ethernet(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_MTU_ETHERNET, def_val);
}

static inline uint32_t mipc_data_mod_call_ind_get_change_reason(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_CHANGE_REASON, def_val);
}

static inline mipc_data_mod_event_type_const_enum mipc_data_mod_call_ind_get_event_type(mipc_msg_t *msg_ptr, mipc_data_mod_event_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_mod_event_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_CALL_IND_T_EVENT_TYPE, def_val);
}

static inline uint8_t mipc_data_mod_pco_ind_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_PCO_IND_T_ID, def_val);
}

static inline uint8_t mipc_data_mod_pco_ind_get_pco_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_MOD_PCO_IND_T_PCO_COUNT, def_val);
}

static inline mipc_data_pco_ie_struct4* mipc_data_mod_pco_ind_get_pco_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_pco_ie_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MOD_PCO_IND_T_PCO_LIST, val_len_ptr);
}

static inline uint32_t mipc_data_wwan_act_call_ind_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_INTERFACE_ID, def_val);
}

static inline uint32_t mipc_data_wwan_act_call_ind_get_cid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_CID, def_val);
}

static inline char * mipc_data_wwan_act_call_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_APN, val_len_ptr);
}

static inline mipc_apn_type_const_enum mipc_data_wwan_act_call_ind_get_apn_type(mipc_msg_t *msg_ptr, mipc_apn_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_APN_TYPE, def_val);
}

static inline mipc_apn_pdp_type_const_enum mipc_data_wwan_act_call_ind_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_PDP_TYPE, def_val);
}

static inline uint32_t mipc_data_wwan_act_call_ind_get_v4_mtu(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V4_MTU, def_val);
}

static inline uint32_t mipc_data_wwan_act_call_ind_get_v6_mtu(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V6_MTU, def_val);
}

static inline uint8_t mipc_data_wwan_act_call_ind_get_v4_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V4_ADDR_COUNT, def_val);
}

static inline mipc_addr_struct4* mipc_data_wwan_act_call_ind_get_v4_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V4_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_data_wwan_act_call_ind_get_v6_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V6_ADDR_COUNT, def_val);
}

static inline mipc_addr_struct4* mipc_data_wwan_act_call_ind_get_v6_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_V6_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_data_wwan_act_call_ind_get_dns_v4_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_DNS_V4_ADDR_COUNT, def_val);
}

static inline mipc_addr_struct4* mipc_data_wwan_act_call_ind_get_dns_v4_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_DNS_V4_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_data_wwan_act_call_ind_get_dns_v6_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_DNS_V6_ADDR_COUNT, def_val);
}

static inline mipc_addr_struct4* mipc_data_wwan_act_call_ind_get_dns_v6_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_ACT_CALL_IND_T_DNS_V6_ADDR_LIST, val_len_ptr);
}

static inline uint32_t mipc_data_wwan_deact_call_ind_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_DEACT_CALL_IND_T_INTERFACE_ID, def_val);
}

static inline uint32_t mipc_data_wwan_deact_call_ind_get_cid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_DEACT_CALL_IND_T_CID, def_val);
}

static inline char * mipc_data_wwan_deact_call_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_WWAN_DEACT_CALL_IND_T_APN, val_len_ptr);
}

static inline mipc_apn_type_const_enum mipc_data_wwan_deact_call_ind_get_apn_type(mipc_msg_t *msg_ptr, mipc_apn_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_WWAN_DEACT_CALL_IND_T_APN_TYPE, def_val);
}

static inline uint32_t mipc_data_md_act_call_ind_get_cid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MD_ACT_CALL_IND_T_CID, def_val);
}

static inline char * mipc_data_md_act_call_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_MD_ACT_CALL_IND_T_APN, val_len_ptr);
}

static inline uint32_t mipc_data_md_act_call_ind_get_apn_idx(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MD_ACT_CALL_IND_T_APN_IDX, def_val);
}

static inline uint32_t mipc_data_md_deact_call_ind_get_cid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MD_DEACT_CALL_IND_T_CID, def_val);
}

static inline char * mipc_data_iwlan_priority_list_ind_get_cmd(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_CMD, val_len_ptr);
}

static inline char * mipc_data_iwlan_priority_list_ind_get_type(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_TYPE, val_len_ptr);
}

static inline mipc_mipc_eiwlpl_priority_type_const_enum mipc_data_iwlan_priority_list_ind_get_setup_priority(mipc_msg_t *msg_ptr, mipc_mipc_eiwlpl_priority_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_mipc_eiwlpl_priority_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_SETUP_PRIORITY, def_val);
}

static inline mipc_mipc_eiwlpl_priority_type_const_enum mipc_data_iwlan_priority_list_ind_get_cellular_priority(mipc_msg_t *msg_ptr, mipc_mipc_eiwlpl_priority_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_mipc_eiwlpl_priority_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_CELLULAR_PRIORITY, def_val);
}

static inline mipc_mipc_eiwlpl_priority_type_const_enum mipc_data_iwlan_priority_list_ind_get_wifi_priority(mipc_msg_t *msg_ptr, mipc_mipc_eiwlpl_priority_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_mipc_eiwlpl_priority_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_WIFI_PRIORITY, def_val);
}

static inline char * mipc_data_iwlan_priority_list_ind_get_description(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_DESCRIPTION, val_len_ptr);
}

static inline uint8_t mipc_data_iwlan_priority_list_ind_get_rat_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_RAT_NUM, def_val);
}

static inline void * mipc_data_iwlan_priority_list_ind_get_rat_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_IWLAN_PRIORITY_LIST_IND_T_RAT_LIST, val_len_ptr);
}

static inline uint32_t mipc_data_link_capacity_estimate_ind_get_dl_kbps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_LINK_CAPACITY_ESTIMATE_IND_T_DL_KBPS, def_val);
}

static inline uint32_t mipc_data_link_capacity_estimate_ind_get_ul_kbps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_LINK_CAPACITY_ESTIMATE_IND_T_UL_KBPS, def_val);
}

static inline uint32_t mipc_data_link_capacity_estimate_ind_get_second_dl_kbps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_LINK_CAPACITY_ESTIMATE_IND_T_SECOND_DL_KBPS, def_val);
}

static inline uint32_t mipc_data_link_capacity_estimate_ind_get_second_ul_kbps(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_LINK_CAPACITY_ESTIMATE_IND_T_SECOND_UL_KBPS, def_val);
}

static inline uint32_t mipc_data_nw_limit_ind_get_state(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_NW_LIMIT_IND_T_STATE, def_val);
}

static inline char * mipc_data_timer_ind_get_src_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_TIMER_IND_T_SRC_ID, val_len_ptr);
}

static inline char * mipc_data_timer_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_TIMER_IND_T_APN, val_len_ptr);
}

static inline uint32_t mipc_data_timer_ind_get_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_TIMER_IND_T_CAUSE, def_val);
}

static inline uint32_t mipc_data_timer_ind_get_timer_state(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_TIMER_IND_T_TIMER_STATE, def_val);
}

static inline uint32_t mipc_data_timer_ind_get_expire_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_TIMER_IND_T_EXPIRE_TIME, def_val);
}

static inline uint32_t mipc_data_keepalive_status_ind_get_session_handle(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_KEEPALIVE_STATUS_IND_T_SESSION_HANDLE, def_val);
}

static inline mipc_data_keepalive_status_code_const_enum mipc_data_keepalive_status_ind_get_status_code(mipc_msg_t *msg_ptr, mipc_data_keepalive_status_code_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_data_keepalive_status_code_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_KEEPALIVE_STATUS_IND_T_STATUS_CODE, def_val);
}

static inline uint32_t mipc_data_mobile_data_usage_ind_get_tx_bytes(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOBILE_DATA_USAGE_IND_T_TX_BYTES, def_val);
}

static inline uint32_t mipc_data_mobile_data_usage_ind_get_tx_packets(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOBILE_DATA_USAGE_IND_T_TX_PACKETS, def_val);
}

static inline uint32_t mipc_data_mobile_data_usage_ind_get_rx_bytes(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOBILE_DATA_USAGE_IND_T_RX_BYTES, def_val);
}

static inline uint32_t mipc_data_mobile_data_usage_ind_get_rx_packets(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_MOBILE_DATA_USAGE_IND_T_RX_PACKETS, def_val);
}

static inline uint16_t mipc_data_network_reject_cause_ind_get_emm_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_DATA_NETWORK_REJECT_CAUSE_IND_T_EMM_CAUSE, def_val);
}

static inline uint16_t mipc_data_network_reject_cause_ind_get_esm_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_DATA_NETWORK_REJECT_CAUSE_IND_T_ESM_CAUSE, def_val);
}

static inline uint16_t mipc_data_network_reject_cause_ind_get_event(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_DATA_NETWORK_REJECT_CAUSE_IND_T_EVENT, def_val);
}

static inline mipc_dsda_allowed_type_const_enum mipc_data_dsda_state_ind_get_dsda_allowed(mipc_msg_t *msg_ptr, mipc_dsda_allowed_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dsda_allowed_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DSDA_STATE_IND_T_DSDA_ALLOWED, def_val);
}

static inline mipc_dsda_state_type_const_enum mipc_data_dsda_state_ind_get_dsda_state(mipc_msg_t *msg_ptr, mipc_dsda_state_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dsda_state_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DSDA_STATE_IND_T_DSDA_STATE, def_val);
}

static inline mipc_dr_dsda_type_const_enum mipc_data_dsda_state_ind_get_is_dr_dsda(mipc_msg_t *msg_ptr, mipc_dr_dsda_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dr_dsda_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DSDA_STATE_IND_T_IS_DR_DSDA, def_val);
}

static inline mipc_dr_dsds_type_const_enum mipc_data_dsda_state_ind_get_is_dr_dsds(mipc_msg_t *msg_ptr, mipc_dr_dsds_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_dr_dsds_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_DSDA_STATE_IND_T_IS_DR_DSDS, def_val);
}

static inline uint8_t mipc_data_umts_ps_state_ind_get_conn_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_UMTS_PS_STATE_IND_T_CONN_STATUS, def_val);
}

static inline char * mipc_data_retry_timer_ind_get_apn_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_RETRY_TIMER_IND_T_APN_NAME, val_len_ptr);
}

static inline uint8_t mipc_data_ursp_reeval_ind_get_id_list_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_DATA_URSP_REEVAL_IND_T_ID_LIST_LEN, def_val);
}

static inline void * mipc_data_ursp_reeval_ind_get_id_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_URSP_REEVAL_IND_T_ID_LIST, val_len_ptr);
}

static inline mipc_ursp_reeval_type_enum_const_enum mipc_data_ursp_reeval_ind_get_event(mipc_msg_t *msg_ptr, mipc_ursp_reeval_type_enum_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ursp_reeval_type_enum_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_URSP_REEVAL_IND_T_EVENT, def_val);
}

static inline void * mipc_data_ursp_ue_policy_chg_ind_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_URSP_UE_POLICY_CHG_IND_T_PLMN_ID, val_len_ptr);
}

static inline mipc_ursp_reeval_type_enum_const_enum mipc_data_ursp_ue_policy_chg_ind_get_event(mipc_msg_t *msg_ptr, mipc_ursp_reeval_type_enum_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ursp_reeval_type_enum_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_URSP_UE_POLICY_CHG_IND_T_EVENT, def_val);
}

static inline uint32_t mipc_data_pdn_nw_cause_ind_get_emm_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_PDN_NW_CAUSE_IND_T_EMM_CAUSE, def_val);
}

static inline uint32_t mipc_data_pdn_nw_cause_ind_get_esm_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_DATA_PDN_NW_CAUSE_IND_T_ESM_CAUSE, def_val);
}

static inline char * mipc_data_pdn_nw_cause_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_DATA_PDN_NW_CAUSE_IND_T_APN, val_len_ptr);
}

static inline uint32_t mipc_internal_eif_ind_get_transid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIF_IND_T_TRANSID, def_val);
}

static inline mipc_internal_eif_ind_cmd_const_enum mipc_internal_eif_ind_get_cmd(mipc_msg_t *msg_ptr, mipc_internal_eif_ind_cmd_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_internal_eif_ind_cmd_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIF_IND_T_CMD, def_val);
}

static inline uint32_t mipc_internal_eif_ind_get_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIF_IND_T_CAUSE, def_val);
}

static inline uint32_t mipc_internal_eif_ind_get_mtu(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIF_IND_T_MTU, def_val);
}

static inline uint8_t mipc_internal_eif_ind_get_net_v4_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIF_IND_T_NET_V4_ADDR_COUNT, def_val);
}

static inline mipc_v4_full_addr_struct4* mipc_internal_eif_ind_get_net_v4_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_v4_full_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_EIF_IND_T_NET_V4_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_internal_eif_ind_get_net_v6_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIF_IND_T_NET_V6_ADDR_COUNT, def_val);
}

static inline mipc_v6_full_addr_struct4* mipc_internal_eif_ind_get_net_v6_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_v6_full_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_EIF_IND_T_NET_V6_ADDR_LIST, val_len_ptr);
}

static inline mipc_apn_pdp_type_const_enum mipc_internal_eif_ind_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIF_IND_T_PDP_TYPE, def_val);
}

static inline uint8_t mipc_internal_ho_ind_get_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_HO_IND_T_ID, def_val);
}

static inline mipc_internal_ho_progress_const_enum mipc_internal_ho_ind_get_progress(mipc_msg_t *msg_ptr, mipc_internal_ho_progress_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_internal_ho_progress_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_HO_IND_T_PROGRESS, def_val);
}

static inline mipc_ran_const_enum mipc_internal_ho_ind_get_src_ran(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_HO_IND_T_SRC_RAN, def_val);
}

static inline mipc_ran_const_enum mipc_internal_ho_ind_get_dst_ran(mipc_msg_t *msg_ptr, mipc_ran_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ran_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_HO_IND_T_DST_RAN, def_val);
}

static inline uint8_t mipc_internal_ho_ind_get_is_succ(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_HO_IND_T_IS_SUCC, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_internal_ho_ind_get_v4_addr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_HO_IND_T_V4_ADDR, val_len_ptr);
}

static inline mipc_data_v6_addr_struct4* mipc_internal_ho_ind_get_v6_addr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_HO_IND_T_V6_ADDR, val_len_ptr);
}

static inline uint32_t mipc_internal_ho_ind_get_trans_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_HO_IND_T_TRANS_ID, def_val);
}

static inline mipc_nw_reg_state_struct4* mipc_nw_register_ind_get_state(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_reg_state_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_REGISTER_IND_T_STATE, val_len_ptr);
}

static inline uint16_t mipc_nw_register_ind_get_nw_err(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_REGISTER_IND_T_NW_ERR, def_val);
}

static inline mipc_nw_register_mode_const_enum mipc_nw_register_ind_get_mode(mipc_msg_t *msg_ptr, mipc_nw_register_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_register_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_REGISTER_IND_T_MODE, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_register_ind_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_REGISTER_IND_T_DATA_SPEED, def_val);
}

static inline char * mipc_nw_register_ind_get_nw_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_REGISTER_IND_T_NW_NAME, val_len_ptr);
}

static inline char * mipc_nw_register_ind_get_roaming_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_REGISTER_IND_T_ROAMING_TEXT, val_len_ptr);
}

static inline uint16_t mipc_nw_register_ind_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_REGISTER_IND_T_FAIL_CAUSE, def_val);
}

static inline char * mipc_nw_register_ind_get_nw_long_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_REGISTER_IND_T_NW_LONG_NAME, val_len_ptr);
}

static inline uint32_t mipc_nw_signal_ind_get_signal_strength_interval(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SIGNAL_IND_T_SIGNAL_STRENGTH_INTERVAL, def_val);
}

static inline uint32_t mipc_nw_signal_ind_get_rssi_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SIGNAL_IND_T_RSSI_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_signal_ind_get_err_rate_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SIGNAL_IND_T_ERR_RATE_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_signal_ind_get_rsrp_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SIGNAL_IND_T_RSRP_THRESHOLD, def_val);
}

static inline uint32_t mipc_nw_signal_ind_get_snr_threshold(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_SIGNAL_IND_T_SNR_THRESHOLD, def_val);
}

static inline mipc_nw_signal_type_const_enum mipc_nw_signal_ind_get_signal_type(mipc_msg_t *msg_ptr, mipc_nw_signal_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_signal_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SIGNAL_IND_T_SIGNAL_TYPE, def_val);
}

static inline mipc_nw_gsm_signal_strength_struct4* mipc_nw_signal_ind_get_gsm_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_GSM_SIGNAL, val_len_ptr);
}

static inline mipc_nw_umts_signal_strength_struct4* mipc_nw_signal_ind_get_umts_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_umts_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_UMTS_SIGNAL, val_len_ptr);
}

static inline mipc_nw_lte_signal_strength_struct4* mipc_nw_signal_ind_get_lte_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_LTE_SIGNAL, val_len_ptr);
}

static inline mipc_nw_nr_signal_strength_struct4* mipc_nw_signal_ind_get_nr_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_NR_SIGNAL, val_len_ptr);
}

static inline mipc_nw_raw_signal_info_struct4* mipc_nw_signal_ind_get_raw_signal_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_raw_signal_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_RAW_SIGNAL_INFO_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_signal_ind_get_raw_signal_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_SIGNAL_IND_T_RAW_SIGNAL_INFO_COUNT, def_val);
}

static inline mipc_nw_cdma_signal_strength_struct4* mipc_nw_signal_ind_get_cdma_signal(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cdma_signal_strength_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_SIGNAL_IND_T_CDMA_SIGNAL, val_len_ptr);
}

static inline uint8_t mipc_nw_ps_ind_get_tach(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PS_IND_T_TACH, def_val);
}

static inline mipc_nw_data_speed_const_enum mipc_nw_ps_ind_get_data_speed(mipc_msg_t *msg_ptr, mipc_nw_data_speed_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_data_speed_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_PS_IND_T_DATA_SPEED, def_val);
}

static inline uint8_t mipc_nw_ps_ind_get_nw_frequency(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PS_IND_T_NW_FREQUENCY, def_val);
}

static inline mipc_nw_ps_reg_info_struct4* mipc_nw_ps_ind_get_reg_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ps_reg_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_PS_IND_T_REG_INFO, val_len_ptr);
}

static inline mipc_nw_cell_type_const_enum mipc_nw_ps_ind_get_cell_type(mipc_msg_t *msg_ptr, mipc_nw_cell_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_cell_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PS_IND_T_CELL_TYPE, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_ps_ind_get_cell_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_PS_IND_T_CELL_INFO, val_len_ptr);
}

static inline uint8_t mipc_nw_radio_ind_get_sw_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_RADIO_IND_T_SW_STATE, def_val);
}

static inline uint8_t mipc_nw_radio_ind_get_hw_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_RADIO_IND_T_HW_STATE, def_val);
}

static inline char * mipc_nw_ia_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_IA_IND_T_APN, val_len_ptr);
}

static inline uint8_t mipc_nw_ia_ind_get_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_IA_IND_T_RAT, def_val);
}

static inline mipc_apn_pdp_type_const_enum mipc_nw_ia_ind_get_pdp_type(mipc_msg_t *msg_ptr, mipc_apn_pdp_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_pdp_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_IA_IND_T_PDP_TYPE, def_val);
}

static inline mipc_apn_auth_type_const_enum mipc_nw_ia_ind_get_auth_type(mipc_msg_t *msg_ptr, mipc_apn_auth_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_auth_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_IA_IND_T_AUTH_TYPE, def_val);
}

static inline char * mipc_nw_ia_ind_get_userid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_IA_IND_T_USERID, val_len_ptr);
}

static inline char * mipc_nw_ia_ind_get_password(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_IA_IND_T_PASSWORD, val_len_ptr);
}

static inline uint32_t mipc_nw_ia_ind_get_apn_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_IA_IND_T_APN_INDEX, def_val);
}

static inline mipc_nw_nitz_info_struct4* mipc_nw_nitz_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nitz_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_NITZ_IND_T_INFO, val_len_ptr);
}

static inline mipc_nitz_ind_type_const_enum mipc_nw_nitz_ind_get_type(mipc_msg_t *msg_ptr, mipc_nitz_ind_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nitz_ind_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NITZ_IND_T_TYPE, def_val);
}

static inline mipc_nw_location_info_struct4* mipc_nw_location_info_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_location_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_LOCATION_INFO_IND_T_INFO, val_len_ptr);
}

static inline mipc_nw_reg_change_info_struct4* mipc_nw_cs_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_reg_change_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CS_IND_T_INFO, val_len_ptr);
}

static inline mipc_nw_cell_type_const_enum mipc_nw_cs_ind_get_cell_type(mipc_msg_t *msg_ptr, mipc_nw_cell_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_cell_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CS_IND_T_CELL_TYPE, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_cs_ind_get_cell_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CS_IND_T_CELL_INFO, val_len_ptr);
}

static inline mipc_nw_cscon_status_struct4* mipc_nw_cscon_ind_get_status(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cscon_status_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CSCON_IND_T_STATUS, val_len_ptr);
}

static inline uint8_t mipc_nw_preferred_provider_ind_get_provider_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PREFERRED_PROVIDER_IND_T_PROVIDER_COUNT, def_val);
}

static inline mipc_nw_provider_struct4* mipc_nw_preferred_provider_ind_get_provider_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_provider_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_PREFERRED_PROVIDER_IND_T_PROVIDER_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_cainfo_ind_get_lte_dl_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CAINFO_IND_T_LTE_DL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_cainfo_ind_get_lte_ul_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CAINFO_IND_T_LTE_UL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_cainfo_ind_get_nr_dl_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CAINFO_IND_T_NR_DL_SERVING_CELL_COUNT, def_val);
}

static inline uint8_t mipc_nw_cainfo_ind_get_nr_ul_serving_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CAINFO_IND_T_NR_UL_SERVING_CELL_COUNT, def_val);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_cainfo_ind_get_lte_dl_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAINFO_IND_T_LTE_DL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_cainfo_ind_get_lte_ul_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAINFO_IND_T_LTE_UL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_cainfo_ind_get_nr_dl_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAINFO_IND_T_NR_DL_SERVING_CELL_LIST, val_len_ptr);
}

static inline mipc_nw_lte_nr_ca_info_struct4* mipc_nw_cainfo_ind_get_nr_ul_serving_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_nr_ca_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAINFO_IND_T_NR_UL_SERVING_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_eons_ind_get_pnn(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EONS_IND_T_PNN, def_val);
}

static inline uint32_t mipc_nw_eons_ind_get_opl(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EONS_IND_T_OPL, def_val);
}

static inline uint32_t mipc_nw_ciev_ind_get_ciev_type(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_CIEV_IND_T_CIEV_TYPE, def_val);
}

static inline uint32_t mipc_nw_ciev_ind_get_ecbm_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_CIEV_IND_T_ECBM_STATUS, def_val);
}

static inline char * mipc_nw_ciev_ind_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CIEV_IND_T_PLMN_ID, val_len_ptr);
}

static inline char * mipc_nw_ciev_ind_get_nw_name_long(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CIEV_IND_T_NW_NAME_LONG, val_len_ptr);
}

static inline char * mipc_nw_ciev_ind_get_nw_name_short(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CIEV_IND_T_NW_NAME_SHORT, val_len_ptr);
}

static inline uint32_t mipc_nw_ciev_ind_get_prl_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_CIEV_IND_T_PRL_ID, def_val);
}

static inline uint32_t mipc_nw_egmss_ind_get_rat(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EGMSS_IND_T_RAT, def_val);
}

static inline uint32_t mipc_nw_egmss_ind_get_mcc(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EGMSS_IND_T_MCC, def_val);
}

static inline uint32_t mipc_nw_egmss_ind_get_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EGMSS_IND_T_STATUS, def_val);
}

static inline uint32_t mipc_nw_egmss_ind_get_cur_reported_rat(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EGMSS_IND_T_CUR_REPORTED_RAT, def_val);
}

static inline uint32_t mipc_nw_egmss_ind_get_is_home_country(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_EGMSS_IND_T_IS_HOME_COUNTRY, def_val);
}

static inline uint32_t mipc_nw_psbearer_ind_get_cell_data_speed_support(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_PSBEARER_IND_T_CELL_DATA_SPEED_SUPPORT, def_val);
}

static inline uint32_t mipc_nw_psbearer_ind_get_max_data_bearer_capability(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_PSBEARER_IND_T_MAX_DATA_BEARER_CAPABILITY, def_val);
}

static inline uint32_t mipc_nw_psbearer_ind_get_sec_cell_num_in_dl(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_PSBEARER_IND_T_SEC_CELL_NUM_IN_DL, def_val);
}

static inline uint32_t mipc_nw_psbearer_ind_get_sec_cell_num_in_ul(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_PSBEARER_IND_T_SEC_CELL_NUM_IN_UL, def_val);
}

static inline uint32_t mipc_nw_ecell_ind_get_gsm_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_GSM_CELL_COUNT, def_val);
}

static inline mipc_nw_gsm_cell_struct4* mipc_nw_ecell_ind_get_gsm_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_gsm_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_GSM_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_ecell_ind_get_umts_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_UMTS_CELL_COUNT, def_val);
}

static inline mipc_nw_umts_cell_struct4* mipc_nw_ecell_ind_get_umts_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_umts_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_UMTS_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_ecell_ind_get_tdscdma_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_TDSCDMA_CELL_COUNT, def_val);
}

static inline mipc_nw_tdscdma_cell_struct4* mipc_nw_ecell_ind_get_tdscdma_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_tdscdma_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_TDSCDMA_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_ecell_ind_get_lte_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_LTE_CELL_COUNT, def_val);
}

static inline mipc_nw_lte_cell_struct4* mipc_nw_ecell_ind_get_lte_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_lte_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_LTE_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_ecell_ind_get_cdma_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_CDMA_CELL_COUNT, def_val);
}

static inline mipc_nw_cdma_cell_struct4* mipc_nw_ecell_ind_get_cdma_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_cdma_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_CDMA_CELL_LIST, val_len_ptr);
}

static inline uint32_t mipc_nw_ecell_ind_get_nr_cell_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ECELL_IND_T_NR_CELL_COUNT, def_val);
}

static inline mipc_nw_nr_cell_struct4* mipc_nw_ecell_ind_get_nr_cell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nr_cell_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ECELL_IND_T_NR_CELL_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_ecell_ind_get_fail_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_ECELL_IND_T_FAIL_CAUSE, def_val);
}

static inline mipc_nw_anbr_info_struct4* mipc_nw_anbr_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_anbr_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_ANBR_IND_T_INFO, val_len_ptr);
}

static inline mipc_nw_irat_info_struct4* mipc_nw_irat_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_irat_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_IRAT_IND_T_INFO, val_len_ptr);
}

static inline uint8_t mipc_nw_ereginfo_ind_get_act(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_EREGINFO_IND_T_ACT, def_val);
}

static inline uint8_t mipc_nw_ereginfo_ind_get_event_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_EREGINFO_IND_T_EVENT_TYPE, def_val);
}

static inline uint8_t mipc_nw_emodcfg_ind_get_modulation(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_EMODCFG_IND_T_MODULATION, def_val);
}

static inline uint8_t mipc_nw_epcellinfo_ind_get_lte_band(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_EPCELLINFO_IND_T_LTE_BAND, def_val);
}

static inline uint8_t mipc_nw_pseudo_cell_ind_get_cell_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PSEUDO_CELL_IND_T_CELL_COUNT, def_val);
}

static inline mipc_nw_pseudocell_info_struct4* mipc_nw_pseudo_cell_ind_get_pseudocell_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_pseudocell_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_PSEUDO_CELL_IND_T_PSEUDOCELL_LIST, val_len_ptr);
}

static inline uint16_t mipc_nw_network_info_ind_get_type(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_NETWORK_INFO_IND_T_TYPE, def_val);
}

static inline char * mipc_nw_network_info_ind_get_nw_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_NETWORK_INFO_IND_T_NW_INFO, val_len_ptr);
}

static inline char * mipc_nw_mccmnc_ind_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_MCCMNC_IND_T_PLMN_ID, val_len_ptr);
}

static inline uint8_t mipc_nw_physical_channel_configs_ind_get_physical_ch_info_list_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_PHYSICAL_CHANNEL_CONFIGS_IND_T_PHYSICAL_CH_INFO_LIST_COUNT, def_val);
}

static inline mipc_physical_channel_info_struct4* mipc_nw_physical_channel_configs_ind_get_physical_ch_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_physical_channel_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_PHYSICAL_CHANNEL_CONFIGS_IND_T_PHYSICAL_CH_INFO_LIST, val_len_ptr);
}

static inline mipc_physical_channel_info_v1_struct4* mipc_nw_physical_channel_configs_ind_get_physical_ch_info_list_v1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_PHYSICAL_CHANNEL_CONFIGS_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_physical_channel_info_v1_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_PHYSICAL_CHANNEL_CONFIGS_IND_T_PHYSICAL_CH_INFO_LIST_V1, index, val_len_ptr);
}

static inline uint8_t mipc_nw_otacmsg_ind_get_ota_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_OTACMSG_IND_T_OTA_STATE, def_val);
}

static inline uint8_t mipc_nw_barring_info_ind_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_BARRING_INFO_IND_T_COUNT, def_val);
}

static inline mipc_nw_barring_info_struct4* mipc_nw_barring_info_ind_get_barring_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_barring_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_BARRING_INFO_IND_T_BARRING_LIST, val_len_ptr);
}

static inline uint8_t mipc_nw_barring_info_ind_get_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_BARRING_INFO_IND_T_RAT, def_val);
}

static inline uint32_t mipc_nw_radio_capability_ind_get_radio_capability(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_RADIO_CAPABILITY_IND_T_RADIO_CAPABILITY, def_val);
}

static inline mipc_nw_bit_rat_const_enum mipc_nw_current_rat_ind_get_current_bit_rat(mipc_msg_t *msg_ptr, mipc_nw_bit_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_bit_rat_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_CURRENT_RAT_IND_T_CURRENT_BIT_RAT, def_val);
}

static inline uint8_t mipc_nw_current_rat_ind_get_current_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CURRENT_RAT_IND_T_CURRENT_RAT, def_val);
}

static inline uint8_t mipc_nw_current_rat_ind_get_prefer_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_CURRENT_RAT_IND_T_PREFER_RAT, def_val);
}

static inline mipc_nw_ps_reg_info_struct4* mipc_nw_camp_state_ind_get_reg_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ps_reg_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAMP_STATE_IND_T_REG_INFO, val_len_ptr);
}

static inline char * mipc_nw_camp_state_ind_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAMP_STATE_IND_T_PLMN_ID, val_len_ptr);
}

static inline char * mipc_nw_camp_state_ind_get_nw_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CAMP_STATE_IND_T_NW_NAME, val_len_ptr);
}

static inline uint8_t mipc_nw_nr_switch_ind_get_nr_sim(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NR_SWITCH_IND_T_NR_SIM, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_TYPE, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_is_1x_femtocell(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_IS_1X_FEMTOCELL, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_is_evdo_femtocell(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_IS_EVDO_FEMTOCELL, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_is_femtocell(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_IS_FEMTOCELL, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_domain(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_DOMAIN, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_STATE, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_act(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_ACT, def_val);
}

static inline uint8_t mipc_nw_femtocell_info_ind_get_is_csg_cell(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_IS_CSG_CELL, def_val);
}

static inline uint32_t mipc_nw_femtocell_info_ind_get_csg_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_CSG_ID, def_val);
}

static inline uint16_t mipc_nw_femtocell_info_ind_get_csg_icon_type(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_CSG_ICON_TYPE, def_val);
}

static inline uint16_t mipc_nw_femtocell_info_ind_get_cause(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_CAUSE, def_val);
}

static inline char * mipc_nw_femtocell_info_ind_get_plmn_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_PLMN_ID, val_len_ptr);
}

static inline char * mipc_nw_femtocell_info_ind_get_oper_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_OPER_NAME, val_len_ptr);
}

static inline char * mipc_nw_femtocell_info_ind_get_hnbname(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_FEMTOCELL_INFO_IND_T_HNBNAME, val_len_ptr);
}

static inline uint32_t mipc_nw_etxpwr_ind_get_act(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_ETXPWR_IND_T_ACT, def_val);
}

static inline int32_t mipc_nw_etxpwr_ind_get_tx_power(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_ETXPWR_IND_T_TX_POWER, def_val);
}

static inline uint16_t mipc_nw_etxpwrstus_ind_get_event(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_NW_ETXPWRSTUS_IND_T_EVENT, def_val);
}

static inline int16_t mipc_nw_etxpwrstus_ind_get_sar_scenario_index(mipc_msg_t *msg_ptr, int16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int16_t)mipc_msg_get_val_int16(msg_ptr, MIPC_NW_ETXPWRSTUS_IND_T_SAR_SCENARIO_INDEX, def_val);
}

static inline mipc_nw_iwlan_status_const_enum mipc_nw_iwlan_ind_get_status(mipc_msg_t *msg_ptr, mipc_nw_iwlan_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_iwlan_status_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_IWLAN_IND_T_STATUS, def_val);
}

static inline mipc_nw_ch_info_rat_const_enum mipc_nw_ch_info_ind_get_rat(mipc_msg_t *msg_ptr, mipc_nw_ch_info_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_ch_info_rat_const_enum)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_CH_INFO_IND_T_RAT, def_val);
}

static inline int32_t mipc_nw_ch_info_ind_get_band(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_CH_INFO_IND_T_BAND, def_val);
}

static inline int32_t mipc_nw_ch_info_ind_get_channel(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_CH_INFO_IND_T_CHANNEL, def_val);
}

static inline int32_t mipc_nw_ch_info_ind_get_is_endc(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_CH_INFO_IND_T_IS_ENDC, def_val);
}

static inline uint8_t mipc_nw_nruw_info_ind_get_display_5guw(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NRUW_INFO_IND_T_DISPLAY_5GUW, def_val);
}

static inline uint8_t mipc_nw_nruw_info_ind_get_on_n77_band(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NRUW_INFO_IND_T_ON_N77_BAND, def_val);
}

static inline uint8_t mipc_nw_nruw_info_ind_get_on_fr2_band(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NRUW_INFO_IND_T_ON_FR2_BAND, def_val);
}

static inline uint8_t mipc_nw_nruw_info_ind_get_5guw_allowed(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NRUW_INFO_IND_T_5GUW_ALLOWED, def_val);
}

static inline int32_t mipc_nw_nr_ca_band_ind_get_is_endc(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_NR_CA_BAND_IND_T_IS_ENDC, def_val);
}

static inline uint8_t mipc_nw_nr_ca_band_ind_get_band_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_NW_NR_CA_BAND_IND_T_BAND_NUM, def_val);
}

static inline mipc_nr_ca_band_struct4* mipc_nw_nr_ca_band_ind_get_band(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nr_ca_band_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_NR_CA_BAND_IND_T_BAND, val_len_ptr);
}

static inline int32_t mipc_nw_nr_scs_ind_get_scs(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_NW_NR_SCS_IND_T_SCS, def_val);
}

static inline mipc_nw_scan_status_const_enum mipc_nw_network_scan_ind_get_scan_status(mipc_msg_t *msg_ptr, mipc_nw_scan_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_scan_status_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_NETWORK_SCAN_IND_T_SCAN_STATUS, def_val);
}

static inline mipc_nw_scan_info_struct4* mipc_nw_network_scan_ind_get_record_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_NWSCN_RESULT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_scan_info_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_NW_NETWORK_SCAN_IND_T_RECORD_LIST, index, val_len_ptr);
}

static inline mipc_nw_ecainfo_struct4* mipc_nw_ca_info_ind_get_cainfo(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_ecainfo_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_CA_INFO_IND_T_CAINFO, val_len_ptr);
}

static inline mipc_nw_5guc_state_const_enum mipc_nw_nruc_info_ind_get_display_5guc(mipc_msg_t *msg_ptr, mipc_nw_5guc_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_5guc_state_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_NRUC_INFO_IND_T_DISPLAY_5GUC, def_val);
}

static inline mipc_nw_uc_band_const_enum mipc_nw_nruc_info_ind_get_on_uc_band(mipc_msg_t *msg_ptr, mipc_nw_uc_band_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_nw_uc_band_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_NRUC_INFO_IND_T_ON_UC_BAND, def_val);
}

static inline uint32_t mipc_nw_nruc_info_ind_get_agg_bw(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_NW_NRUC_INFO_IND_T_AGG_BW, def_val);
}

static inline char * mipc_nw_first_plmn_ind_get_mcc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_FIRST_PLMN_IND_T_MCC, val_len_ptr);
}

static inline char * mipc_nw_first_plmn_ind_get_mnc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_NW_FIRST_PLMN_IND_T_MNC, val_len_ptr);
}

static inline mipc_sim_state_const_enum mipc_sim_state_ind_get_state(mipc_msg_t *msg_ptr, mipc_sim_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATE_IND_T_STATE, def_val);
}

static inline uint32_t mipc_sim_state_ind_get_sim_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATE_IND_T_SIM_ID, def_val);
}

static inline uint32_t mipc_sim_state_ind_get_ps_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATE_IND_T_PS_ID, def_val);
}

static inline uint32_t mipc_sim_state_ind_get_is_present(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATE_IND_T_IS_PRESENT, def_val);
}

static inline mipc_sim_sub_state_const_enum mipc_sim_state_ind_get_sub_state(mipc_msg_t *msg_ptr, mipc_sim_sub_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_sub_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATE_IND_T_SUB_STATE, def_val);
}

static inline mipc_sim_status_const_enum mipc_sim_status_ind_get_status(mipc_msg_t *msg_ptr, mipc_sim_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_IND_T_STATUS, def_val);
}

static inline uint32_t mipc_sim_status_ind_get_sim_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATUS_IND_T_SIM_ID, def_val);
}

static inline uint32_t mipc_sim_status_ind_get_ps_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SIM_STATUS_IND_T_PS_ID, def_val);
}

static inline uint8_t mipc_sim_euicc_slots_status_ind_get_slots_info_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_SLOTS_INFO_COUNT, def_val);
}

static inline mipc_sim_slots_info_struct4* mipc_sim_euicc_slots_status_ind_get_slots_info_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sim_slots_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_SLOTS_INFO_LIST, val_len_ptr);
}

static inline mipc_sim_card_present_state_const_enum mipc_sim_euicc_slots_status_ind_get_card_state(mipc_msg_t *msg_ptr, mipc_sim_card_present_state_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_sim_card_present_state_const_enum)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_CARD_STATE, index, def_val);
}

static inline uint8_t mipc_sim_euicc_slots_status_ind_get_slots_state(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_SLOTS_STATE, index, def_val);
}

static inline uint8_t mipc_sim_euicc_slots_status_ind_get_logical_idx(mipc_msg_t *msg_ptr, uint8_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_idx_uint8(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_LOGICAL_IDX, index, def_val);
}

static inline char * mipc_sim_euicc_slots_status_ind_get_atr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_ATR, index, val_len_ptr);
}

static inline char * mipc_sim_euicc_slots_status_ind_get_eid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_EID, index, val_len_ptr);
}

static inline char * mipc_sim_euicc_slots_status_ind_get_iccid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SIM_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SIM_EUICC_SLOTS_STATUS_IND_T_ICCID, index, val_len_ptr);
}

static inline char * mipc_sim_iccid_ind_get_iccid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_ICCID_IND_T_ICCID, val_len_ptr);
}

static inline uint8_t mipc_sim_status_change_with_cause_ind_get_is_sim_inserted(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CHANGE_WITH_CAUSE_IND_T_IS_SIM_INSERTED, def_val);
}

static inline mipc_sim_cause_const_enum mipc_sim_status_change_with_cause_ind_get_cause(mipc_msg_t *msg_ptr, mipc_sim_cause_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_cause_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CHANGE_WITH_CAUSE_IND_T_CAUSE, def_val);
}

static inline mipc_sim_additional_cause_const_enum mipc_sim_status_change_with_cause_ind_get_additional_cause(mipc_msg_t *msg_ptr, mipc_sim_additional_cause_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_additional_cause_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_STATUS_CHANGE_WITH_CAUSE_IND_T_ADDITIONAL_CAUSE, def_val);
}

static inline uint8_t mipc_sim_csim_imsi_change_ind_get_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CSIM_IMSI_CHANGE_IND_T_STATUS, def_val);
}

static inline uint8_t mipc_sim_sml_status_ind_get_lock_rule(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_LOCK_RULE, def_val);
}

static inline uint16_t mipc_sim_sml_status_ind_get_lock_sub_rule(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_LOCK_SUB_RULE, def_val);
}

static inline uint8_t mipc_sim_sml_status_ind_get_device_lock_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_DEVICE_LOCK_STATE, def_val);
}

static inline uint8_t mipc_sim_sml_status_ind_get_rule_policy(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_RULE_POLICY, def_val);
}

static inline uint8_t mipc_sim_sml_status_ind_get_sim_validity(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_SIM_VALIDITY, def_val);
}

static inline uint8_t mipc_sim_sml_status_ind_get_device_lock_remain_cnt(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_STATUS_IND_T_DEVICE_LOCK_REMAIN_CNT, def_val);
}

static inline uint8_t mipc_sim_sml_rsu_ind_get_operator_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SML_RSU_IND_T_OPERATOR_ID, def_val);
}

static inline uint16_t mipc_sim_sml_rsu_ind_get_event_id(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SIM_SML_RSU_IND_T_EVENT_ID, def_val);
}

static inline char * mipc_sim_sml_rsu_ind_get_event_string(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SML_RSU_IND_T_EVENT_STRING, val_len_ptr);
}

static inline char * mipc_sim_vsim_apdu_ind_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_VSIM_APDU_IND_T_DATA, val_len_ptr);
}

static inline uint8_t mipc_sim_vsim_event_ind_get_event(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_VSIM_EVENT_IND_T_EVENT, def_val);
}

static inline char * mipc_sim_vsim_event_ind_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_VSIM_EVENT_IND_T_DATA, val_len_ptr);
}

static inline mipc_sim_app_type_ex_const_enum mipc_sim_simapp_ind_get_app_id(mipc_msg_t *msg_ptr, mipc_sim_app_type_ex_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_app_type_ex_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SIMAPP_IND_T_APP_ID, def_val);
}

static inline uint8_t mipc_sim_simapp_ind_get_ch_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SIMAPP_IND_T_CH_ID, def_val);
}

static inline char * mipc_sim_simapp_ind_get_mcc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMAPP_IND_T_MCC, val_len_ptr);
}

static inline char * mipc_sim_simapp_ind_get_mnc(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMAPP_IND_T_MNC, val_len_ptr);
}

static inline uint8_t mipc_sim_test_sim_ind_get_test_sim(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_TEST_SIM_IND_T_TEST_SIM, def_val);
}

static inline uint8_t mipc_sim_ct3g_ind_get_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CT3G_IND_T_TYPE, def_val);
}

static inline uint8_t mipc_sim_card_type_ind_get_usim_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CARD_TYPE_IND_T_USIM_STATE, def_val);
}

static inline uint8_t mipc_sim_card_type_ind_get_csim_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CARD_TYPE_IND_T_CSIM_STATE, def_val);
}

static inline uint8_t mipc_sim_card_type_ind_get_isim_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_CARD_TYPE_IND_T_ISIM_STATE, def_val);
}

static inline uint8_t mipc_sim_simind_ind_get_event(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SIMIND_IND_T_EVENT, def_val);
}

static inline uint8_t mipc_sim_simind_ind_get_app_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SIMIND_IND_T_APP_ID, def_val);
}

static inline char * mipc_sim_simind_ind_get_spn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_SPN, val_len_ptr);
}

static inline char * mipc_sim_simind_ind_get_imsi(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_IMSI, val_len_ptr);
}

static inline char * mipc_sim_simind_ind_get_gid1(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_GID1, val_len_ptr);
}

static inline char * mipc_sim_simind_ind_get_pnn_full_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_PNN_FULL_NAME, val_len_ptr);
}

static inline char * mipc_sim_simind_ind_get_impi(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_IMPI, val_len_ptr);
}

static inline uint8_t mipc_sim_simind_ind_get_file_num(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SIM_SIMIND_IND_T_FILE_NUM, def_val);
}

static inline mipc_file_list_struct4* mipc_sim_simind_ind_get_file_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_file_list_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SIM_SIMIND_IND_T_FILE_LIST, val_len_ptr);
}

static inline mipc_sms_format_const_enum mipc_sms_cfg_ind_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_FORMAT, def_val);
}

static inline char * mipc_sms_cfg_ind_get_sca(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_CFG_IND_T_SCA, val_len_ptr);
}

static inline mipc_sms_state_const_enum mipc_sms_cfg_ind_get_sms_state(mipc_msg_t *msg_ptr, mipc_sms_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_SMS_STATE, def_val);
}

static inline uint16_t mipc_sms_cfg_ind_get_max_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_IND_T_MAX_MESSAGE, def_val);
}

static inline mipc_sms_ack_const_enum mipc_sms_cfg_ind_get_prefer_ack(mipc_msg_t *msg_ptr, mipc_sms_ack_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_ack_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_PREFER_ACK, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_ind_get_prefer_storage(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_PREFER_STORAGE, def_val);
}

static inline uint16_t mipc_sms_cfg_ind_get_used_message(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_IND_T_USED_MESSAGE, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_ind_get_prefer_storage_c2k(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_PREFER_STORAGE_C2K, def_val);
}

static inline uint16_t mipc_sms_cfg_ind_get_used_message_c2k(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_IND_T_USED_MESSAGE_C2K, def_val);
}

static inline uint16_t mipc_sms_cfg_ind_get_max_message_c2k(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_CFG_IND_T_MAX_MESSAGE_C2K, def_val);
}

static inline mipc_sms_storage_const_enum mipc_sms_cfg_ind_get_w_r_d_storage(mipc_msg_t *msg_ptr, mipc_sms_storage_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_storage_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_CFG_IND_T_W_R_D_STORAGE, def_val);
}

static inline mipc_sms_format_const_enum mipc_sms_new_sms_ind_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_NEW_SMS_IND_T_FORMAT, def_val);
}

static inline uint16_t mipc_sms_new_sms_ind_get_pdu_count(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_NEW_SMS_IND_T_PDU_COUNT, def_val);
}

static inline mipc_sms_pdu_struct4* mipc_sms_new_sms_ind_get_pdu_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sms_pdu_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_SMS_IND_T_PDU_LIST, val_len_ptr);
}

static inline void * mipc_sms_new_sms_ind_get_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_SMS_IND_T_PDU_C2K, val_len_ptr);
}

static inline mipc_sms_store_flag_const_enum mipc_sms_store_status_ind_get_flag(mipc_msg_t *msg_ptr, mipc_sms_store_flag_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_store_flag_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SMS_STORE_STATUS_IND_T_FLAG, def_val);
}

static inline uint16_t mipc_sms_store_status_ind_get_message_index(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_STORE_STATUS_IND_T_MESSAGE_INDEX, def_val);
}

static inline void * mipc_sms_new_status_report_ind_get_pdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_STATUS_REPORT_IND_T_PDU, val_len_ptr);
}

static inline mipc_sms_cbm_type_const_enum mipc_sms_new_cbm_ind_get_cbm_type(mipc_msg_t *msg_ptr, mipc_sms_cbm_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_cbm_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_CBM_TYPE, def_val);
}

static inline uint16_t mipc_sms_new_cbm_ind_get_warning_type(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_WARNING_TYPE, def_val);
}

static inline uint16_t mipc_sms_new_cbm_ind_get_message_id(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_MESSAGE_ID, def_val);
}

static inline uint16_t mipc_sms_new_cbm_ind_get_serial_number(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_SERIAL_NUMBER, def_val);
}

static inline uint8_t mipc_sms_new_cbm_ind_get_dcs(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_DCS, def_val);
}

static inline void * mipc_sms_new_cbm_ind_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_DATA, val_len_ptr);
}

static inline void * mipc_sms_new_cbm_ind_get_secur_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_SECUR_INFO, val_len_ptr);
}

static inline mipc_sms_format_const_enum mipc_sms_new_cbm_ind_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_FORMAT, def_val);
}

static inline void * mipc_sms_new_cbm_ind_get_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_PDU_C2K, val_len_ptr);
}

static inline void * mipc_sms_new_cbm_ind_get_pdu_3gpp_seg(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_SMS_SEG_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_PDU_3GPP_SEG, index, val_len_ptr);
}

static inline void * mipc_sms_new_cbm_ind_get_wac_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_CBM_IND_T_WAC_INFO, val_len_ptr);
}

static inline mipc_sms_scbm_status_const_enum mipc_sms_scbm_ind_get_status_update(mipc_msg_t *msg_ptr, mipc_sms_scbm_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_scbm_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_SCBM_IND_T_STATUS_UPDATE, def_val);
}

static inline char * mipc_sms_ext_info_ind_get_epsi(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_EXT_INFO_IND_T_EPSI, val_len_ptr);
}

static inline void * mipc_sms_ext_info_ind_get_esn_old(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_EXT_INFO_IND_T_ESN_OLD, val_len_ptr);
}

static inline void * mipc_sms_ext_info_ind_get_esn_new(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_EXT_INFO_IND_T_ESN_NEW, val_len_ptr);
}

static inline mipc_sms_format_const_enum mipc_sms_dup_new_sms_ind_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_DUP_NEW_SMS_IND_T_FORMAT, def_val);
}

static inline void * mipc_sms_dup_new_sms_ind_get_pdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_DUP_NEW_SMS_IND_T_PDU, val_len_ptr);
}

static inline mipc_ss_ussd_const_enum mipc_ss_ussd_ind_get_ussd_response(mipc_msg_t *msg_ptr, mipc_ss_ussd_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_ussd_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_USSD_IND_T_USSD_RESPONSE, def_val);
}

static inline mipc_ss_session_const_enum mipc_ss_ussd_ind_get_ussd_session_state(mipc_msg_t *msg_ptr, mipc_ss_session_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_session_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_USSD_IND_T_USSD_SESSION_STATE, def_val);
}

static inline uint32_t mipc_ss_ussd_ind_get_dcs(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_USSD_IND_T_DCS, def_val);
}

static inline uint8_t mipc_ss_ussd_ind_get_payload_len(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_USSD_IND_T_PAYLOAD_LEN, def_val);
}

static inline void * mipc_ss_ussd_ind_get_payload(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_USSD_IND_T_PAYLOAD, val_len_ptr);
}

static inline uint32_t mipc_ss_ecmccss_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_ECMCCSS_IND_T_CALL_ID, def_val);
}

static inline uint32_t mipc_ss_ecmccss_ind_get_service(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_ECMCCSS_IND_T_SERVICE, def_val);
}

static inline char * mipc_ss_ecmccss_ind_get_raw_string(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_ECMCCSS_IND_T_RAW_STRING, val_len_ptr);
}

static inline mipc_ss_ecfu_icon_status_const_enum mipc_ss_cfu_ind_get_status(mipc_msg_t *msg_ptr, mipc_ss_ecfu_icon_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ss_ecfu_icon_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_CFU_IND_T_STATUS, def_val);
}

static inline uint8_t mipc_ss_cfu_ind_get_line(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_CFU_IND_T_LINE, def_val);
}

static inline uint32_t mipc_ss_xcap_rcn_ind_get_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SS_XCAP_RCN_IND_T_CODE, def_val);
}

static inline uint8_t mipc_ss_xcap_rcn_ind_get_response(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_XCAP_RCN_IND_T_RESPONSE, def_val);
}

static inline uint8_t mipc_ss_ims_xui_ind_get_account_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_IMS_XUI_IND_T_ACCOUNT_ID, def_val);
}

static inline uint8_t mipc_ss_ims_xui_ind_get_broadcast_flag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_SS_IMS_XUI_IND_T_BROADCAST_FLAG, def_val);
}

static inline char * mipc_ss_ims_xui_ind_get_xui_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SS_IMS_XUI_IND_T_XUI_INFO, val_len_ptr);
}

static inline mipc_stk_pac_type_const_enum mipc_stk_pac_ind_get_pac_type(mipc_msg_t *msg_ptr, mipc_stk_pac_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_stk_pac_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_STK_PAC_IND_T_PAC_TYPE, def_val);
}

static inline uint16_t mipc_stk_pac_ind_get_pac_len(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_STK_PAC_IND_T_PAC_LEN, def_val);
}

static inline void * mipc_stk_pac_ind_get_pac(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_PAC_IND_T_PAC, val_len_ptr);
}

static inline mipc_sim_refresh_result_type_const_enum mipc_stk_sim_refresh_ind_get_sim_refresh_result(mipc_msg_t *msg_ptr, mipc_sim_refresh_result_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sim_refresh_result_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_STK_SIM_REFRESH_IND_T_SIM_REFRESH_RESULT, def_val);
}

static inline uint32_t mipc_stk_sim_refresh_ind_get_ef_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_STK_SIM_REFRESH_IND_T_EF_ID, def_val);
}

static inline char * mipc_stk_sim_refresh_ind_get_aid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_SIM_REFRESH_IND_T_AID, val_len_ptr);
}

static inline char * mipc_stk_bip_event_notify_ind_get_cmd_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_STK_BIP_EVENT_NOTIFY_IND_T_CMD_DATA, val_len_ptr);
}

static inline uint32_t mipc_call_status_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_CALLID, def_val);
}

static inline mipc_call_direction_const_enum mipc_call_status_ind_get_direction(mipc_msg_t *msg_ptr, mipc_call_direction_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_direction_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_DIRECTION, def_val);
}

static inline mipc_call_mode_const_enum mipc_call_status_ind_get_mode(mipc_msg_t *msg_ptr, mipc_call_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_mode_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_MODE, def_val);
}

static inline uint32_t mipc_call_status_ind_get_ton(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_TON, def_val);
}

static inline char * mipc_call_status_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_STATUS_IND_T_NUMBER, val_len_ptr);
}

static inline mipc_call_type_const_enum mipc_call_status_ind_get_type(mipc_msg_t *msg_ptr, mipc_call_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_TYPE, def_val);
}

static inline mipc_call_detail_info_struct4* mipc_call_status_ind_get_detail_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_call_detail_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_STATUS_IND_T_DETAIL_INFO, val_len_ptr);
}

static inline mipc_call_video_cap_struct4* mipc_call_status_ind_get_video_cap(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_call_video_cap_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_STATUS_IND_T_VIDEO_CAP, val_len_ptr);
}

static inline mipc_call_msg_type_const_enum mipc_call_status_ind_get_msg_type(mipc_msg_t *msg_ptr, mipc_call_msg_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_msg_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_MSG_TYPE, def_val);
}

static inline uint32_t mipc_call_status_ind_get_disc_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_STATUS_IND_T_DISC_CAUSE, def_val);
}

static inline char * mipc_call_status_ind_get_pau(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_STATUS_IND_T_PAU, val_len_ptr);
}

static inline mipc_call_event_const_enum mipc_call_event_ind_get_event(mipc_msg_t *msg_ptr, mipc_call_event_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_event_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_EVENT, def_val);
}

static inline mipc_call_reject_reason_const_enum mipc_call_event_ind_get_reject_reason(mipc_msg_t *msg_ptr, mipc_call_reject_reason_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_reject_reason_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_REJECT_REASON, def_val);
}

static inline uint32_t mipc_call_event_ind_get_srvcch(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_SRVCCH, def_val);
}

static inline char * mipc_call_event_ind_get_redirect_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_EVENT_IND_T_REDIRECT_NUMBER, val_len_ptr);
}

static inline mipc_call_audio_codec_const_enum mipc_call_event_ind_get_audio_codec(mipc_msg_t *msg_ptr, mipc_call_audio_codec_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_audio_codec_const_enum)mipc_msg_get_val_uint16(msg_ptr, MIPC_CALL_EVENT_IND_T_AUDIO_CODEC, def_val);
}

static inline mipc_boolean_const_enum mipc_call_event_ind_get_speech_on(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EVENT_IND_T_SPEECH_ON, def_val);
}

static inline mipc_speech_rat_const_enum mipc_call_event_ind_get_speech_rat(mipc_msg_t *msg_ptr, mipc_speech_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_speech_rat_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_SPEECH_RAT, def_val);
}

static inline mipc_boolean_const_enum mipc_call_event_ind_get_speech_irho_on(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EVENT_IND_T_SPEECH_IRHO_ON, def_val);
}

static inline mipc_call_rat_const_enum mipc_call_event_ind_get_rat(mipc_msg_t *msg_ptr, mipc_call_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EVENT_IND_T_RAT, def_val);
}

static inline uint32_t mipc_call_event_ind_get_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_COUNT, def_val);
}

static inline uint32_t mipc_call_event_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_EVENT_IND_T_CALL_ID, index, def_val);
}

static inline uint32_t mipc_call_mode_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_MODE_IND_T_CALLID, def_val);
}

static inline mipc_call_mode_const_enum mipc_call_mode_ind_get_mode(mipc_msg_t *msg_ptr, mipc_call_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_mode_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_MODE_IND_T_MODE, def_val);
}

static inline mipc_sdp_direction_const_enum mipc_call_mode_ind_get_sdp_camera_direction(mipc_msg_t *msg_ptr, mipc_sdp_direction_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sdp_direction_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_MODE_IND_T_SDP_CAMERA_DIRECTION, def_val);
}

static inline mipc_sdp_direction_const_enum mipc_call_mode_ind_get_sdp_audio_direction(mipc_msg_t *msg_ptr, mipc_sdp_direction_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sdp_direction_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_MODE_IND_T_SDP_AUDIO_DIRECTION, def_val);
}

static inline mipc_sdp_audio_codec_const_enum mipc_call_mode_ind_get_sdp_audio_codec(mipc_msg_t *msg_ptr, mipc_sdp_audio_codec_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sdp_audio_codec_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_MODE_IND_T_SDP_AUDIO_CODEC, def_val);
}

static inline uint32_t mipc_call_sip_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SIP_IND_T_CALLID, def_val);
}

static inline mipc_sip_direction_const_enum mipc_call_sip_ind_get_direction(mipc_msg_t *msg_ptr, mipc_sip_direction_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sip_direction_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SIP_IND_T_DIRECTION, def_val);
}

static inline mipc_sip_msg_type_const_enum mipc_call_sip_ind_get_msg_type(mipc_msg_t *msg_ptr, mipc_sip_msg_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sip_msg_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SIP_IND_T_MSG_TYPE, def_val);
}

static inline mipc_sip_method_const_enum mipc_call_sip_ind_get_method(mipc_msg_t *msg_ptr, mipc_sip_method_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sip_method_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SIP_IND_T_METHOD, def_val);
}

static inline uint32_t mipc_call_sip_ind_get_response_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SIP_IND_T_RESPONSE_CODE, def_val);
}

static inline char * mipc_call_sip_ind_get_reason_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_SIP_IND_T_REASON_TEXT, val_len_ptr);
}

static inline uint32_t mipc_call_conference_ind_get_conf_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_CONF_CALLID, def_val);
}

static inline uint32_t mipc_call_conference_ind_get_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_COUNT, def_val);
}

static inline mipc_call_direction_const_enum mipc_call_conference_ind_get_direction(mipc_msg_t *msg_ptr, mipc_call_direction_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_call_direction_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_DIRECTION, index, def_val);
}

static inline char * mipc_call_conference_ind_get_participant_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_PARTICIPANT_NUMBER, index, val_len_ptr);
}

static inline char * mipc_call_conference_ind_get_participant_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_PARTICIPANT_NAME, index, val_len_ptr);
}

static inline mipc_conf_participant_status_const_enum mipc_call_conference_ind_get_participant_status(mipc_msg_t *msg_ptr, mipc_conf_participant_status_const_enum def_val, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (mipc_conf_participant_status_const_enum)mipc_msg_get_idx_uint32(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_PARTICIPANT_STATUS, index, def_val);
}

static inline char * mipc_call_conference_ind_get_participant_user_entity(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_PARTICIPANT_USER_ENTITY, index, val_len_ptr);
}

static inline char * mipc_call_conference_ind_get_participant_endpoint_entity(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_CALL_CONFERENCE_PARTICIPANT_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_idx_ptr(msg_ptr, MIPC_CALL_CONFERENCE_IND_T_PARTICIPANT_ENDPOINT_ENTITY, index, val_len_ptr);
}

static inline uint32_t mipc_call_ims_event_package_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_EVENT_PACKAGE_IND_T_CALLID, def_val);
}

static inline mipc_ims_event_package_type_const_enum mipc_call_ims_event_package_ind_get_type(mipc_msg_t *msg_ptr, mipc_ims_event_package_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_event_package_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_EVENT_PACKAGE_IND_T_TYPE, def_val);
}

static inline char * mipc_call_ims_event_package_ind_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_IMS_EVENT_PACKAGE_IND_T_DATA, val_len_ptr);
}

static inline mipc_call_ss_code1_const_enum mipc_call_ss_ind_get_code1(mipc_msg_t *msg_ptr, mipc_call_ss_code1_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_ss_code1_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SS_IND_T_CODE1, def_val);
}

static inline mipc_call_ss_code2_const_enum mipc_call_ss_ind_get_code2(mipc_msg_t *msg_ptr, mipc_call_ss_code2_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_ss_code2_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SS_IND_T_CODE2, def_val);
}

static inline uint32_t mipc_call_ss_ind_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SS_IND_T_INDEX, def_val);
}

static inline char * mipc_call_ss_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_SS_IND_T_NUMBER, val_len_ptr);
}

static inline uint32_t mipc_call_ss_ind_get_toa(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SS_IND_T_TOA, def_val);
}

static inline char * mipc_call_ss_ind_get_subaddr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_SS_IND_T_SUBADDR, val_len_ptr);
}

static inline uint32_t mipc_call_ss_ind_get_satype(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_SS_IND_T_SATYPE, def_val);
}

static inline char * mipc_call_ss_ind_get_raw_string(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_SS_IND_T_RAW_STRING, val_len_ptr);
}

static inline mipc_call_ecbm_mode_const_enum mipc_call_ecbm_change_ind_get_mode(mipc_msg_t *msg_ptr, mipc_call_ecbm_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_ecbm_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_ECBM_CHANGE_IND_T_MODE, def_val);
}

static inline mipc_crss_type_const_enum mipc_call_crss_ind_get_crss_type(mipc_msg_t *msg_ptr, mipc_crss_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_crss_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CRSS_IND_T_CRSS_TYPE, def_val);
}

static inline char * mipc_call_crss_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_CRSS_IND_T_NUMBER, val_len_ptr);
}

static inline mipc_number_type_const_enum mipc_call_crss_ind_get_call_number_type(mipc_msg_t *msg_ptr, mipc_number_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_number_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CRSS_IND_T_CALL_NUMBER_TYPE, def_val);
}

static inline mipc_number_presentation_const_enum mipc_call_crss_ind_get_number_presentation(mipc_msg_t *msg_ptr, mipc_number_presentation_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_number_presentation_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CRSS_IND_T_NUMBER_PRESENTATION, def_val);
}

static inline char * mipc_call_crss_ind_get_sub_address(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_CRSS_IND_T_SUB_ADDRESS, val_len_ptr);
}

static inline uint8_t mipc_call_crss_ind_get_sa_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CRSS_IND_T_SA_TYPE, def_val);
}

static inline char * mipc_call_crss_ind_get_alphaid(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_CRSS_IND_T_ALPHAID, val_len_ptr);
}

static inline char * mipc_call_crss_ind_get_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_CRSS_IND_T_NAME, val_len_ptr);
}

static inline uint32_t mipc_call_ect_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECT_IND_T_CALL_ID, def_val);
}

static inline mipc_boolean_const_enum mipc_call_ect_ind_get_ect_result(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_ECT_IND_T_ECT_RESULT, def_val);
}

static inline uint32_t mipc_call_ect_ind_get_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECT_IND_T_CAUSE, def_val);
}

static inline mipc_boolean_const_enum mipc_call_cipher_ind_get_sim_cipher_ind(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CIPHER_IND_T_SIM_CIPHER_IND, def_val);
}

static inline mipc_boolean_const_enum mipc_call_cipher_ind_get_mm_connection(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CIPHER_IND_T_MM_CONNECTION, def_val);
}

static inline mipc_call_cipher_on_status_const_enum mipc_call_cipher_ind_get_cs_cipher_on(mipc_msg_t *msg_ptr, mipc_call_cipher_on_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_cipher_on_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CIPHER_IND_T_CS_CIPHER_ON, def_val);
}

static inline mipc_call_cipher_on_status_const_enum mipc_call_cipher_ind_get_ps_cipher_on(mipc_msg_t *msg_ptr, mipc_call_cipher_on_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_cipher_on_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CIPHER_IND_T_PS_CIPHER_ON, def_val);
}

static inline uint32_t mipc_call_rtt_audio_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_RTT_AUDIO_IND_T_CALL_ID, def_val);
}

static inline mipc_call_rtt_audio_type_const_enum mipc_call_rtt_audio_ind_get_type(mipc_msg_t *msg_ptr, mipc_call_rtt_audio_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_rtt_audio_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_RTT_AUDIO_IND_T_TYPE, def_val);
}

static inline uint32_t mipc_call_rtt_capability_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_RTT_CAPABILITY_IND_T_CALL_ID, def_val);
}

static inline uint8_t mipc_call_rtt_capability_ind_get_local_text_capability(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_RTT_CAPABILITY_IND_T_LOCAL_TEXT_CAPABILITY, def_val);
}

static inline uint8_t mipc_call_rtt_capability_ind_get_remote_text_capability(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_RTT_CAPABILITY_IND_T_REMOTE_TEXT_CAPABILITY, def_val);
}

static inline uint8_t mipc_call_rtt_capability_ind_get_local_text_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_RTT_CAPABILITY_IND_T_LOCAL_TEXT_STATUS, def_val);
}

static inline uint8_t mipc_call_rtt_capability_ind_get_real_remote_text_capability(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_RTT_CAPABILITY_IND_T_REAL_REMOTE_TEXT_CAPABILITY, def_val);
}

static inline uint32_t mipc_call_local_rtt_modify_result_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_RESULT_IND_T_CALL_ID, def_val);
}

static inline uint8_t mipc_call_local_rtt_modify_result_ind_get_result(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_LOCAL_RTT_MODIFY_RESULT_IND_T_RESULT, def_val);
}

static inline uint32_t mipc_call_peer_rtt_modify_result_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_PEER_RTT_MODIFY_RESULT_IND_T_CALL_ID, def_val);
}

static inline mipc_call_local_rtt_modify_op_const_enum mipc_call_peer_rtt_modify_result_ind_get_op(mipc_msg_t *msg_ptr, mipc_call_local_rtt_modify_op_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_local_rtt_modify_op_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_PEER_RTT_MODIFY_RESULT_IND_T_OP, def_val);
}

static inline uint32_t mipc_call_rtt_text_receive_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_RTT_TEXT_RECEIVE_IND_T_CALL_ID, def_val);
}

static inline uint32_t mipc_call_rtt_text_receive_ind_get_len(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_RTT_TEXT_RECEIVE_IND_T_LEN, def_val);
}

static inline char * mipc_call_rtt_text_receive_ind_get_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_RTT_TEXT_RECEIVE_IND_T_TEXT, val_len_ptr);
}

static inline char * mipc_call_rcs_digits_line_ind_get_digits_line(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_RCS_DIGITS_LINE_IND_T_DIGITS_LINE, val_len_ptr);
}

static inline char * mipc_call_display_and_signals_info_ind_get_display(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_DISPLAY_AND_SIGNALS_INFO_IND_T_DISPLAY, val_len_ptr);
}

static inline uint32_t mipc_call_display_and_signals_info_ind_get_signal_type(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_DISPLAY_AND_SIGNALS_INFO_IND_T_SIGNAL_TYPE, def_val);
}

static inline uint32_t mipc_call_display_and_signals_info_ind_get_alert_pitch(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_DISPLAY_AND_SIGNALS_INFO_IND_T_ALERT_PITCH, def_val);
}

static inline uint32_t mipc_call_display_and_signals_info_ind_get_signal(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_DISPLAY_AND_SIGNALS_INFO_IND_T_SIGNAL, def_val);
}

static inline uint32_t mipc_call_extended_display_info_ind_get_display_tag(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_EXTENDED_DISPLAY_INFO_IND_T_DISPLAY_TAG, def_val);
}

static inline char * mipc_call_extended_display_info_ind_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_EXTENDED_DISPLAY_INFO_IND_T_INFO, val_len_ptr);
}

static inline uint32_t mipc_call_line_control_info_ind_get_polarity_included(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LINE_CONTROL_INFO_IND_T_POLARITY_INCLUDED, def_val);
}

static inline uint32_t mipc_call_line_control_info_ind_get_toggle_mode(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LINE_CONTROL_INFO_IND_T_TOGGLE_MODE, def_val);
}

static inline uint32_t mipc_call_line_control_info_ind_get_reverse_polarity(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LINE_CONTROL_INFO_IND_T_REVERSE_POLARITY, def_val);
}

static inline uint32_t mipc_call_line_control_info_ind_get_power_denial_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_LINE_CONTROL_INFO_IND_T_POWER_DENIAL_TIME, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_ext_bit_1(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_EXT_BIT_1, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_number_type(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_NUMBER_TYPE, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_number_plan(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_NUMBER_PLAN, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_ext_bit_2(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_EXT_BIT_2, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_pi(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_PI, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_si(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_SI, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_ext_bit_3(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_EXT_BIT_3, def_val);
}

static inline uint32_t mipc_call_redirecting_number_info_ind_get_redirection_reason(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_REDIRECTION_REASON, def_val);
}

static inline char * mipc_call_redirecting_number_info_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_REDIRECTING_NUMBER_INFO_IND_T_NUMBER, val_len_ptr);
}

static inline mipc_gwsd_event_const_enum mipc_call_gwsd_event_ind_get_event(mipc_msg_t *msg_ptr, mipc_gwsd_event_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_gwsd_event_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_EVENT, def_val);
}

static inline uint32_t mipc_call_gwsd_event_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_CALL_ID, def_val);
}

static inline uint32_t mipc_call_gwsd_event_ind_get_update_status(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_UPDATE_STATUS, def_val);
}

static inline uint32_t mipc_call_gwsd_event_ind_get_ton(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_TON, def_val);
}

static inline char * mipc_call_gwsd_event_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_NUMBER, val_len_ptr);
}

static inline uint32_t mipc_call_gwsd_event_ind_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_GWSD_EVENT_IND_T_RESULT, def_val);
}

static inline uint32_t mipc_call_econf_ind_get_conf_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECONF_IND_T_CONF_CALL_ID, def_val);
}

static inline uint32_t mipc_call_econf_ind_get_operation(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECONF_IND_T_OPERATION, def_val);
}

static inline char * mipc_call_econf_ind_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_ECONF_IND_T_NUMBER, val_len_ptr);
}

static inline uint32_t mipc_call_econf_ind_get_result(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECONF_IND_T_RESULT, def_val);
}

static inline uint32_t mipc_call_econf_ind_get_cause(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECONF_IND_T_CAUSE, def_val);
}

static inline uint32_t mipc_call_econf_ind_get_joined_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_ECONF_IND_T_JOINED_CALL_ID, def_val);
}

static inline uint32_t mipc_call_ims_sip_header_ind_get_call_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_SIP_HEADER_IND_T_CALL_ID, def_val);
}

static inline uint32_t mipc_call_ims_sip_header_ind_get_header_type(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_SIP_HEADER_IND_T_HEADER_TYPE, def_val);
}

static inline uint32_t mipc_call_ims_sip_header_ind_get_total_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_SIP_HEADER_IND_T_TOTAL_COUNT, def_val);
}

static inline uint32_t mipc_call_ims_sip_header_ind_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_IMS_SIP_HEADER_IND_T_INDEX, def_val);
}

static inline char * mipc_call_ims_sip_header_ind_get_value(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_IMS_SIP_HEADER_IND_T_VALUE, val_len_ptr);
}

static inline uint8_t mipc_call_ecc_redial_ind_get_call_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_ECC_REDIAL_IND_T_CALL_ID, def_val);
}

static inline mipc_emergency_call_s1_support_const_enum mipc_call_emergency_bearer_support_ind_get_s1_support(mipc_msg_t *msg_ptr, mipc_emergency_call_s1_support_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_s1_support_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_S1_SUPPORT, def_val);
}

static inline mipc_emergency_call_rat_const_enum mipc_call_emergency_bearer_support_ind_get_rat(mipc_msg_t *msg_ptr, mipc_emergency_call_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_RAT, def_val);
}

static inline mipc_emergency_call_support_emc_const_enum mipc_call_emergency_bearer_support_ind_get_support_emc(mipc_msg_t *msg_ptr, mipc_emergency_call_support_emc_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_support_emc_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_SUPPORT_EMC, def_val);
}

static inline mipc_emergency_call_emb_iu_supp_const_enum mipc_call_emergency_bearer_support_ind_get_emb_iu_supp(mipc_msg_t *msg_ptr, mipc_emergency_call_emb_iu_supp_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_emb_iu_supp_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_EMB_IU_SUPP, def_val);
}

static inline mipc_emergency_call_ems_5g_supp_const_enum mipc_call_emergency_bearer_support_ind_get_ems_5g_supp(mipc_msg_t *msg_ptr, mipc_emergency_call_ems_5g_supp_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_ems_5g_supp_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_EMS_5G_SUPP, def_val);
}

static inline mipc_emergency_call_emf_5g_supp_const_enum mipc_call_emergency_bearer_support_ind_get_emf_5g_supp(mipc_msg_t *msg_ptr, mipc_emergency_call_emf_5g_supp_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_emergency_call_emf_5g_supp_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_EMERGENCY_BEARER_SUPPORT_IND_T_EMF_5G_SUPP, def_val);
}

static inline uint32_t mipc_call_uis_info_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_UIS_INFO_IND_T_CALLID, def_val);
}

static inline mipc_call_uis_info_type_const_enum mipc_call_uis_info_ind_get_type(mipc_msg_t *msg_ptr, mipc_call_uis_info_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_uis_info_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_UIS_INFO_IND_T_TYPE, def_val);
}

static inline char * mipc_call_uis_info_ind_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_UIS_INFO_IND_T_DATA, val_len_ptr);
}

static inline mipc_call_uis_info_result_const_enum mipc_call_uis_info_ind_get_result(mipc_msg_t *msg_ptr, mipc_call_uis_info_result_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_uis_info_result_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_UIS_INFO_IND_T_RESULT, def_val);
}

static inline mipc_call_uis_info_cause_const_enum mipc_call_uis_info_ind_get_cause(mipc_msg_t *msg_ptr, mipc_call_uis_info_cause_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_uis_info_cause_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_UIS_INFO_IND_T_CAUSE, def_val);
}

static inline uint32_t mipc_call_call_additional_info_ind_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_CALLID, def_val);
}

static inline mipc_call_additional_info_mode_const_enum mipc_call_call_additional_info_ind_get_mode(mipc_msg_t *msg_ptr, mipc_call_additional_info_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_additional_info_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_MODE, def_val);
}

static inline mipc_call_additional_info_type_const_enum mipc_call_call_additional_info_ind_get_type(mipc_msg_t *msg_ptr, mipc_call_additional_info_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_additional_info_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_TYPE, def_val);
}

static inline uint32_t mipc_call_call_additional_info_ind_get_total(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_TOTAL, def_val);
}

static inline uint32_t mipc_call_call_additional_info_ind_get_index(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_INDEX, def_val);
}

static inline uint32_t mipc_call_call_additional_info_ind_get_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_COUNT, def_val);
}

static inline char * mipc_call_call_additional_info_ind_get_additional_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_CALL_ADDITIONAL_INFO_IND_T_ADDITIONAL_INFO, val_len_ptr);
}

static inline char * mipc_call_mt_sip_invite_ind_get_from_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_MT_SIP_INVITE_IND_T_FROM_NUMBER, val_len_ptr);
}

static inline uint8_t mipc_call_mt_sip_invite_ind_get_total_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_MT_SIP_INVITE_IND_T_TOTAL_COUNT, def_val);
}

static inline uint8_t mipc_call_mt_sip_invite_ind_get_index(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_MT_SIP_INVITE_IND_T_INDEX, def_val);
}

static inline char * mipc_call_mt_sip_invite_ind_get_mt_sip_invite(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_MT_SIP_INVITE_IND_T_MT_SIP_INVITE, val_len_ptr);
}

static inline mipc_ims_config_ind_reason_const_enum mipc_ims_config_ind_get_reason(mipc_msg_t *msg_ptr, mipc_ims_config_ind_reason_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_config_ind_reason_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_CONFIG_IND_T_REASON, def_val);
}

static inline void * mipc_ims_config_ind_get_config_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_CONFIG_IND_T_CONFIG_DATA, val_len_ptr);
}

static inline mipc_ims_state_ind_event_const_enum mipc_ims_state_ind_get_event(mipc_msg_t *msg_ptr, mipc_ims_state_ind_event_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_state_ind_event_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_STATE_IND_T_EVENT, def_val);
}

static inline uint8_t mipc_ims_state_ind_get_reg_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_STATE_IND_T_REG_STATE, def_val);
}

static inline uint32_t mipc_ims_state_ind_get_ext_info(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_STATE_IND_T_EXT_INFO, def_val);
}

static inline uint8_t mipc_ims_state_ind_get_wfc(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_STATE_IND_T_WFC, def_val);
}

static inline uint32_t mipc_ims_state_ind_get_account_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_STATE_IND_T_ACCOUNT_ID, def_val);
}

static inline void * mipc_ims_state_ind_get_uri(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_STATE_IND_T_URI, val_len_ptr);
}

static inline uint32_t mipc_ims_state_ind_get_expire_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_STATE_IND_T_EXPIRE_TIME, def_val);
}

static inline uint32_t mipc_ims_state_ind_get_error_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_STATE_IND_T_ERROR_CODE, def_val);
}

static inline char * mipc_ims_state_ind_get_error_message(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_STATE_IND_T_ERROR_MESSAGE, val_len_ptr);
}

static inline mipc_ims_ecc_ind_rat_const_enum mipc_ims_support_ecc_ind_get_rat(mipc_msg_t *msg_ptr, mipc_ims_ecc_ind_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_ecc_ind_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_SUPPORT_ECC_IND_T_RAT, def_val);
}

static inline mipc_boolean_const_enum mipc_ims_support_ecc_ind_get_support_emc(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_SUPPORT_ECC_IND_T_SUPPORT_EMC, def_val);
}

static inline mipc_ims_ind_type_const_enum mipc_ims_pdn_ind_get_ind_type(mipc_msg_t *msg_ptr, mipc_ims_ind_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_ind_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDN_IND_T_IND_TYPE, def_val);
}

static inline uint32_t mipc_ims_pdn_ind_get_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_PDN_IND_T_ID, def_val);
}

static inline mipc_mipc_cid_act_state_const_enum mipc_ims_pdn_ind_get_pdn_state(mipc_msg_t *msg_ptr, mipc_mipc_cid_act_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_mipc_cid_act_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDN_IND_T_PDN_STATE, def_val);
}

static inline mipc_apn_type_const_enum mipc_ims_pdn_ind_get_apn_type(mipc_msg_t *msg_ptr, mipc_apn_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_apn_type_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_PDN_IND_T_APN_TYPE, def_val);
}

static inline uint32_t mipc_ims_pdn_ind_get_interface_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_PDN_IND_T_INTERFACE_ID, def_val);
}

static inline uint8_t mipc_ims_pdn_ind_get_v4_dns_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDN_IND_T_V4_DNS_ADDR_COUNT, def_val);
}

static inline mipc_data_v4_addr_struct4* mipc_ims_pdn_ind_get_v4_dns_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v4_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_PDN_IND_T_V4_DNS_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_ims_pdn_ind_get_v6_dns_addr_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDN_IND_T_V6_DNS_ADDR_COUNT, def_val);
}

static inline mipc_data_v6_addr_struct4* mipc_ims_pdn_ind_get_v6_dns_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_data_v6_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_PDN_IND_T_V6_DNS_ADDR_LIST, val_len_ptr);
}

static inline uint8_t mipc_ims_naptr_ind_get_transaction_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_NAPTR_IND_T_TRANSACTION_ID, def_val);
}

static inline char * mipc_ims_naptr_ind_get_mod_id(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_NAPTR_IND_T_MOD_ID, val_len_ptr);
}

static inline char * mipc_ims_naptr_ind_get_fqdn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_NAPTR_IND_T_FQDN, val_len_ptr);
}

static inline uint8_t mipc_ims_reg_ind_get_reg_state(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_REG_STATE, def_val);
}

static inline uint8_t mipc_ims_reg_ind_get_reg_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_REG_TYPE, def_val);
}

static inline uint32_t mipc_ims_reg_ind_get_ext_info(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_REG_IND_T_EXT_INFO, def_val);
}

static inline uint8_t mipc_ims_reg_ind_get_dereg_cause(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_DEREG_CAUSE, def_val);
}

static inline uint8_t mipc_ims_reg_ind_get_ims_retry(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_IMS_RETRY, def_val);
}

static inline uint8_t mipc_ims_reg_ind_get_rat(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_RAT, def_val);
}

static inline uint8_t mipc_ims_reg_ind_get_sip_uri_type(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_SIP_URI_TYPE, def_val);
}

static inline mipc_ims_state_const_enum mipc_ims_reg_ind_get_reg_sub_state(mipc_msg_t *msg_ptr, mipc_ims_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ims_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_REG_IND_T_REG_SUB_STATE, def_val);
}

static inline uint32_t mipc_ims_sip_reg_info_ind_get_account_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_ACCOUNT_ID, def_val);
}

static inline char * mipc_ims_sip_reg_info_ind_get_direction(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_DIRECTION, val_len_ptr);
}

static inline char * mipc_ims_sip_reg_info_ind_get_sip_msg_type(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_SIP_MSG_TYPE, val_len_ptr);
}

static inline char * mipc_ims_sip_reg_info_ind_get_method(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_METHOD, val_len_ptr);
}

static inline uint32_t mipc_ims_sip_reg_info_ind_get_response_code(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_RESPONSE_CODE, def_val);
}

static inline char * mipc_ims_sip_reg_info_ind_get_reason_phrase(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_REASON_PHRASE, val_len_ptr);
}

static inline char * mipc_ims_sip_reg_info_ind_get_warn_text(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_SIP_REG_INFO_IND_T_WARN_TEXT, val_len_ptr);
}

static inline uint8_t mipc_ims_vops_ind_get_nwimsvops(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_VOPS_IND_T_NWIMSVOPS, def_val);
}

static inline uint32_t mipc_ims_reg_remain_time_ind_get_reg_remain_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_REG_REMAIN_TIME_IND_T_REG_REMAIN_TIME, def_val);
}

static inline uint32_t mipc_ims_reg_remain_time_ind_get_sub_remain_time(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_IMS_REG_REMAIN_TIME_IND_T_SUB_REMAIN_TIME, def_val);
}

static inline uint8_t mipc_ims_ui_ind_get_iconflag(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_UI_IND_T_ICONFLAG, def_val);
}

static inline mipc_wfc_signal_type_const_enum mipc_wfc_cell_signal_ind_get_signal_type(mipc_msg_t *msg_ptr, mipc_wfc_signal_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_signal_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_CELL_SIGNAL_IND_T_SIGNAL_TYPE, def_val);
}

static inline int16_t mipc_wfc_cell_signal_ind_get_value(mipc_msg_t *msg_ptr, int16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int16_t)mipc_msg_get_val_int16(msg_ptr, MIPC_WFC_CELL_SIGNAL_IND_T_VALUE, def_val);
}

static inline uint8_t mipc_wfc_wifi_pdn_count_ind_get_count(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WIFI_PDN_COUNT_IND_T_COUNT, def_val);
}

static inline mipc_wfc_pdn_type_const_enum mipc_wfc_pdn_ho_ind_get_pdn_type(mipc_msg_t *msg_ptr, mipc_wfc_pdn_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_pdn_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_PDN_HO_IND_T_PDN_TYPE, def_val);
}

static inline mipc_wfc_pdn_ho_status_const_enum mipc_wfc_pdn_ho_ind_get_status(mipc_msg_t *msg_ptr, mipc_wfc_pdn_ho_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_pdn_ho_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_PDN_HO_IND_T_STATUS, def_val);
}

static inline mipc_wfc_rat_const_enum mipc_wfc_pdn_ho_ind_get_src_rat(mipc_msg_t *msg_ptr, mipc_wfc_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_PDN_HO_IND_T_SRC_RAT, def_val);
}

static inline mipc_wfc_rat_const_enum mipc_wfc_pdn_ho_ind_get_dst_rat(mipc_msg_t *msg_ptr, mipc_wfc_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_PDN_HO_IND_T_DST_RAT, def_val);
}

static inline char * mipc_wfc_rove_out_ind_get_ifname(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_ROVE_OUT_IND_T_IFNAME, val_len_ptr);
}

static inline mipc_boolean_const_enum mipc_wfc_rove_out_ind_get_rvout(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_ROVE_OUT_IND_T_RVOUT, def_val);
}

static inline mipc_boolean_const_enum mipc_wfc_rove_out_ind_get_mobike_ind(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_ROVE_OUT_IND_T_MOBIKE_IND, def_val);
}

static inline uint16_t mipc_wfc_ssac_ind_get_bf_voice(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_SSAC_IND_T_BF_VOICE, def_val);
}

static inline uint16_t mipc_wfc_ssac_ind_get_bf_video(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_SSAC_IND_T_BF_VIDEO, def_val);
}

static inline uint16_t mipc_wfc_ssac_ind_get_bt_voice(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_SSAC_IND_T_BT_VOICE, def_val);
}

static inline uint16_t mipc_wfc_ssac_ind_get_bt_video(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_SSAC_IND_T_BT_VIDEO, def_val);
}

static inline int32_t mipc_wfc_wifi_pdn_err_ind_get_cause(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_WFC_WIFI_PDN_ERR_IND_T_CAUSE, def_val);
}

static inline int32_t mipc_wfc_wifi_pdn_err_ind_get_sub_cause(mipc_msg_t *msg_ptr, int32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (int32_t)mipc_msg_get_val_int32(msg_ptr, MIPC_WFC_WIFI_PDN_ERR_IND_T_SUB_CAUSE, def_val);
}

static inline mipc_boolean_const_enum mipc_wfc_wifi_pdn_err_ind_get_last_retry(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WIFI_PDN_ERR_IND_T_LAST_RETRY, def_val);
}

static inline char * mipc_wfc_wifi_pdn_oos_ind_get_apn(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_WIFI_PDN_OOS_IND_T_APN, val_len_ptr);
}

static inline uint8_t mipc_wfc_wifi_pdn_oos_ind_get_cid(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WIFI_PDN_OOS_IND_T_CID, def_val);
}

static inline mipc_wfc_pdn_oos_state_const_enum mipc_wfc_wifi_pdn_oos_ind_get_oos_state(mipc_msg_t *msg_ptr, mipc_wfc_pdn_oos_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_pdn_oos_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WIFI_PDN_OOS_IND_T_OOS_STATE, def_val);
}

static inline mipc_wfc_wifi_pdn_state_const_enum mipc_wfc_wfc_ind_get_wifi_pdn_state(mipc_msg_t *msg_ptr, mipc_wfc_wifi_pdn_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_wifi_pdn_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WFC_IND_T_WIFI_PDN_STATE, def_val);
}

static inline uint8_t mipc_wfc_wfc_ind_get_data_sim(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WFC_IND_T_DATA_SIM, def_val);
}

static inline char * mipc_wfc_wfc_ind_get_ifname(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_WFC_IND_T_IFNAME, val_len_ptr);
}

static inline mipc_boolean_const_enum mipc_wfc_wfc_ind_get_lock(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WFC_IND_T_LOCK, def_val);
}

static inline mipc_wfc_wifi_pdn_state_const_enum mipc_wfc_wfc_ind_get_wifi_ims_pdn_state(mipc_msg_t *msg_ptr, mipc_wfc_wifi_pdn_state_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_wifi_pdn_state_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_WFC_IND_T_WIFI_IMS_PDN_STATE, def_val);
}

static inline uint8_t mipc_phb_ready_state_ind_get_ready(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_PHB_READY_STATE_IND_T_READY, def_val);
}

static inline uint8_t mipc_embms_emsrv_ind_get_status(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_EMSRV_IND_T_STATUS, def_val);
}

static inline mipc_embms_area_id_info_struct4* mipc_embms_emsrv_ind_get_area_id_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_area_id_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_EMSRV_IND_T_AREA_ID_INFO, val_len_ptr);
}

static inline uint16_t mipc_embms_emslui_ind_get_num_sessions(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_EMBMS_EMSLUI_IND_T_NUM_SESSIONS, def_val);
}

static inline mipc_embms_session_info_struct4* mipc_embms_emslui_ind_get_sessions_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_session_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_EMSLUI_IND_T_SESSIONS_INFO, val_len_ptr);
}

static inline mipc_embms_nb_req_info_struct4* mipc_embms_emsailnf_ind_get_mbms_nb_freq_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_nb_req_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_EMSAILNF_IND_T_MBMS_NB_FREQ_INFO, val_len_ptr);
}

static inline uint16_t mipc_embms_emsess_ind_get_num_sessions(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_EMBMS_EMSESS_IND_T_NUM_SESSIONS, def_val);
}

static inline mipc_embms_session_info_struct4* mipc_embms_emsess_ind_get_mbms_session_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_embms_session_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_EMBMS_EMSESS_IND_T_MBMS_SESSION_INFO, val_len_ptr);
}

static inline uint8_t mipc_embms_emsess_ind_get_cause(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_EMSESS_IND_T_CAUSE, def_val);
}

static inline uint8_t mipc_embms_emsess_ind_get_sub_cause(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_EMSESS_IND_T_SUB_CAUSE, def_val);
}

static inline uint8_t mipc_embms_ehvolte_ind_get_mode(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_EMBMS_EHVOLTE_IND_T_MODE, def_val);
}

static inline mipc_ecall_status_const_enum mipc_ecall_status_ind_get_state(mipc_msg_t *msg_ptr, mipc_ecall_status_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ecall_status_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_ECALL_STATUS_IND_T_STATE, def_val);
}

static inline uint8_t mipc_ecall_status_ind_get_call_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_ECALL_STATUS_IND_T_CALL_ID, def_val);
}

static inline mipc_sys_reboot_mode_const_enum mipc_sys_reboot_cmd_get_mode(mipc_msg_t *msg_ptr, mipc_sys_reboot_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_reboot_mode_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SYS_REBOOT_CMD_T_MODE, def_val);
}

static inline uint32_t mipc_sys_reboot_cmd_get_timeout(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_REBOOT_CMD_T_TIMEOUT, def_val);
}

static inline uint32_t mipc_sys_reboot_cmd_get_sbp_id(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_REBOOT_CMD_T_SBP_ID, def_val);
}

static inline mipc_sys_config_class_const_enum mipc_sys_set_config_dipc_cmd_get_class(mipc_msg_t *msg_ptr, mipc_sys_config_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_config_class_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_SET_CONFIG_DIPC_CMD_T_CLASS, def_val);
}

static inline char * mipc_sys_set_config_dipc_cmd_get_type(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_CONFIG_DIPC_CMD_T_TYPE, val_len_ptr);
}

static inline void * mipc_sys_set_config_dipc_cmd_get_data(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_CONFIG_DIPC_CMD_T_DATA, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_set_config_dipc_rsp_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  60000) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_SET_CONFIG_DIPC_RSP_T_DATA, len, (const void *)value);
}

static inline mipc_sys_config_class_const_enum mipc_sys_config_needed_to_update_cmd_get_class(mipc_msg_t *msg_ptr, mipc_sys_config_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_config_class_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_CONFIG_NEEDED_TO_UPDATE_CMD_T_CLASS, def_val);
}

static inline char * mipc_sys_config_needed_to_update_cmd_get_type(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_CONFIG_NEEDED_TO_UPDATE_CMD_T_TYPE, val_len_ptr);
}

static inline mipc_sys_config_class_const_enum mipc_sys_get_config_dipc_cmd_get_class(mipc_msg_t *msg_ptr, mipc_sys_config_class_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sys_config_class_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_SYS_GET_CONFIG_DIPC_CMD_T_CLASS, def_val);
}

static inline char * mipc_sys_get_config_dipc_cmd_get_type(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_GET_CONFIG_DIPC_CMD_T_TYPE, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sys_get_config_dipc_rsp_add_data(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  60000) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SYS_GET_CONFIG_DIPC_RSP_T_DATA, len, (const void *)value);
}

static inline mipc_nw_nitz_info_struct4* mipc_sys_set_time_cmd_get_info(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_nw_nitz_info_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SYS_SET_TIME_CMD_T_INFO, val_len_ptr);
}

static inline uint32_t mipc_internal_eipport_cmd_get_transid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_TRANSID, def_val);
}

static inline mipc_internal_eipport_action_const_enum mipc_internal_eipport_cmd_get_action(mipc_msg_t *msg_ptr, mipc_internal_eipport_action_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_internal_eipport_action_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_ACTION, def_val);
}

static inline uint8_t mipc_internal_eipport_cmd_get_ifid(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_IFID, def_val);
}

static inline mipc_addr_struct4* mipc_internal_eipport_cmd_get_addr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_ADDR, val_len_ptr);
}

static inline uint8_t mipc_internal_eipport_cmd_get_proto(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_PROTO, def_val);
}

static inline uint16_t mipc_internal_eipport_cmd_get_port(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_INTERNAL_EIPPORT_CMD_T_PORT, def_val);
}

static inline mipc_msg_api_result_enum mipc_internal_eipport_rsp_add_transid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_EIPPORT_RSP_T_TRANSID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eipport_rsp_add_action(mipc_msg_t *msg_ptr, enum mipc_internal_eipport_action_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_EIPPORT_RSP_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eipport_rsp_add_result(mipc_msg_t *msg_ptr, enum mipc_internal_eipport_result_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_EIPPORT_RSP_T_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_internal_eipspi_cmd_get_transid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_TRANSID, def_val);
}

static inline mipc_internal_eipspi_action_const_enum mipc_internal_eipspi_cmd_get_action(mipc_msg_t *msg_ptr, mipc_internal_eipspi_action_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_internal_eipspi_action_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_ACTION, def_val);
}

static inline mipc_addr_struct4* mipc_internal_eipspi_cmd_get_src_addr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_SRC_ADDR, val_len_ptr);
}

static inline mipc_addr_struct4* mipc_internal_eipspi_cmd_get_dst_addr(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_DST_ADDR, val_len_ptr);
}

static inline mipc_ipsec_proto_enum_const_enum mipc_internal_eipspi_cmd_get_proto(mipc_msg_t *msg_ptr, mipc_ipsec_proto_enum_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ipsec_proto_enum_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_PROTO, def_val);
}

static inline mipc_ipsec_mode_enum_const_enum mipc_internal_eipspi_cmd_get_mode(mipc_msg_t *msg_ptr, mipc_ipsec_mode_enum_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_ipsec_mode_enum_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_MODE, def_val);
}

static inline uint32_t mipc_internal_eipspi_cmd_get_min_spi(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_MIN_SPI, def_val);
}

static inline uint32_t mipc_internal_eipspi_cmd_get_max_spi(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_MAX_SPI, def_val);
}

static inline uint32_t mipc_internal_eipspi_cmd_get_spi(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_CMD_T_SPI, def_val);
}

static inline mipc_msg_api_result_enum mipc_internal_eipspi_rsp_add_transid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_RSP_T_TRANSID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eipspi_rsp_add_action(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_EIPSPI_RSP_T_ACTION, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_eipspi_rsp_add_spi(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_EIPSPI_RSP_T_SPI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint32_t mipc_internal_multi_eipspi_free_cmd_get_transid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_TRANSID, def_val);
}

static inline uint32_t mipc_internal_multi_eipspi_free_cmd_get_multi_free_count(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_MULTI_FREE_COUNT, def_val);
}

static inline mipc_addr_struct4* mipc_internal_multi_eipspi_free_cmd_get_src_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_DEL_SPI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_SRC_ADDR_LIST, index, val_len_ptr);
}

static inline mipc_addr_struct4* mipc_internal_multi_eipspi_free_cmd_get_dst_addr_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr, uint32_t index)
{
    if (index >= MIPC_MAX_DEL_SPI_NUM) return 0;
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_addr_struct4*)mipc_msg_get_idx_ptr(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_DST_ADDR_LIST, index, val_len_ptr);
}

static inline void * mipc_internal_multi_eipspi_free_cmd_get_proto_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_PROTO_LIST, val_len_ptr);
}

static inline void * mipc_internal_multi_eipspi_free_cmd_get_spi_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_CMD_T_SPI_LIST, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_internal_multi_eipspi_free_rsp_add_transid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_RSP_T_TRANSID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_multi_eipspi_free_rsp_add_multi_free_count(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_RSP_T_MULTI_FREE_COUNT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_multi_eipspi_free_rsp_add_spi_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DEL_SPI_NUM) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_RSP_T_SPI_LIST, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_multi_eipspi_free_rsp_add_status_list(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_DEL_SPI_NUM) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_MULTI_EIPSPI_FREE_RSP_T_STATUS_LIST, len, (const void *)value);
}

static inline mipc_sms_format_const_enum mipc_sms_new_sms_cmd_get_format(mipc_msg_t *msg_ptr, mipc_sms_format_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_sms_format_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_SMS_NEW_SMS_CMD_T_FORMAT, def_val);
}

static inline uint16_t mipc_sms_new_sms_cmd_get_pdu_count(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_SMS_NEW_SMS_CMD_T_PDU_COUNT, def_val);
}

static inline mipc_sms_pdu_struct4* mipc_sms_new_sms_cmd_get_pdu_list(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (mipc_sms_pdu_struct4*)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_SMS_CMD_T_PDU_LIST, val_len_ptr);
}

static inline void * mipc_sms_new_sms_cmd_get_pdu_c2k(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_SMS_CMD_T_PDU_C2K, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_new_sms_ack(mipc_msg_t *msg_ptr, enum mipc_new_sms_ack_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_NEW_SMS_ACK, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_cause(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_format(mipc_msg_t *msg_ptr, enum mipc_sms_format_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_FORMAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_err_class_c2k(mipc_msg_t *msg_ptr, enum mipc_sms_c2k_err_class_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_ERR_CLASS_C2K, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_err_code_c2k(mipc_msg_t *msg_ptr, enum mipc_sms_c2k_err_code_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_ERR_CODE_C2K, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_sms_rsp_add_ack_pdu(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_NEW_SMS_RSP_T_ACK_PDU, len, (const void *)value);
}

static inline void * mipc_sms_new_status_report_cmd_get_pdu(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_SMS_NEW_STATUS_REPORT_CMD_T_PDU, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_sms_new_status_report_rsp_add_ack_type(mipc_msg_t *msg_ptr, enum mipc_new_sms_ack_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_STATUS_REPORT_RSP_T_ACK_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_status_report_rsp_add_cause(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_SMS_NEW_STATUS_REPORT_RSP_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_sms_new_status_report_rsp_add_ack_pdu(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_SMS_PDU_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_SMS_NEW_STATUS_REPORT_RSP_T_ACK_PDU, len, (const void *)value);
}

static inline uint32_t mipc_call_approve_incoming_cmd_get_callid(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_CALLID, def_val);
}

static inline char * mipc_call_approve_incoming_cmd_get_number(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_NUMBER, val_len_ptr);
}

static inline uint32_t mipc_call_approve_incoming_cmd_get_toa(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_TOA, def_val);
}

static inline uint32_t mipc_call_approve_incoming_cmd_get_seq_no(mipc_msg_t *msg_ptr, uint32_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint32_t)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_SEQ_NO, def_val);
}

static inline mipc_call_mode_const_enum mipc_call_approve_incoming_cmd_get_mode(mipc_msg_t *msg_ptr, mipc_call_mode_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_call_mode_const_enum)mipc_msg_get_val_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_MODE, def_val);
}

static inline uint8_t mipc_call_approve_incoming_cmd_get_evoltesi_flow(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_CALL_APPROVE_INCOMING_CMD_T_EVOLTESI_FLOW, def_val);
}

static inline mipc_msg_api_result_enum mipc_call_approve_incoming_rsp_add_is_approve(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_CALL_APPROVE_INCOMING_RSP_T_IS_APPROVE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_approve_incoming_rsp_add_cause(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_RSP_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_approve_incoming_rsp_add_callid(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_RSP_T_CALLID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_call_approve_incoming_rsp_add_seq_no(mipc_msg_t *msg_ptr, uint32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint32(msg_ptr, MIPC_CALL_APPROVE_INCOMING_RSP_T_SEQ_NO, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline uint8_t mipc_ims_pdis_cmd_get_transaction_id(mipc_msg_t *msg_ptr, uint8_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint8_t)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDIS_CMD_T_TRANSACTION_ID, def_val);
}

static inline mipc_em_id_type_const_enum mipc_ims_pdis_cmd_get_em_id(mipc_msg_t *msg_ptr, mipc_em_id_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_em_id_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDIS_CMD_T_EM_ID, def_val);
}

static inline mipc_pdis_method_type_const_enum mipc_ims_pdis_cmd_get_method(mipc_msg_t *msg_ptr, mipc_pdis_method_type_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_pdis_method_type_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_IMS_PDIS_CMD_T_METHOD, def_val);
}

static inline char * mipc_ims_pdis_cmd_get_nw_interface_name(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_IMS_PDIS_CMD_T_NW_INTERFACE_NAME, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_ims_pdis_rsp_add_transaction_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_PDIS_RSP_T_TRANSACTION_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_pdis_rsp_add_method(mipc_msg_t *msg_ptr, enum mipc_pdis_method_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_PDIS_RSP_T_METHOD, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_ims_pdis_rsp_add_is_success(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_IMS_PDIS_RSP_T_IS_SUCCESS, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_wfc_rat_const_enum mipc_wfc_ping_cmd_get_rat(mipc_msg_t *msg_ptr, mipc_wfc_rat_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_wfc_rat_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_PING_CMD_T_RAT, def_val);
}

static inline mipc_msg_api_result_enum mipc_wfc_ping_rsp_add_rat(mipc_msg_t *msg_ptr, enum mipc_wfc_rat_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_PING_RSP_T_RAT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_ping_rsp_add_ave_latency(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_WFC_PING_RSP_T_AVE_LATENCY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_ping_rsp_add_loss_rate(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_PING_RSP_T_LOSS_RATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline char * mipc_wfc_get_mac_cmd_get_ifname(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (char *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_GET_MAC_CMD_T_IFNAME, val_len_ptr);
}

static inline void * mipc_wfc_get_mac_cmd_get_ip(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_GET_MAC_CMD_T_IP, val_len_ptr);
}

static inline mipc_msg_api_result_enum mipc_wfc_get_mac_rsp_add_get_result(mipc_msg_t *msg_ptr, int32_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int32(msg_ptr, MIPC_WFC_GET_MAC_RSP_T_GET_RESULT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_get_mac_rsp_add_ifname(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IFNAME_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_GET_MAC_RSP_T_IFNAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_get_mac_rsp_add_ip(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IP_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_GET_MAC_RSP_T_IP, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_get_mac_rsp_add_mac(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_MAC_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_GET_MAC_RSP_T_MAC, len, (const void *)value);
}

static inline mipc_boolean_const_enum mipc_wfc_natt_keep_alive_cmd_get_enable(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_ENABLE, def_val);
}

static inline uint16_t mipc_wfc_natt_keep_alive_cmd_get_interval(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_INTERVAL, def_val);
}

static inline void * mipc_wfc_natt_keep_alive_cmd_get_src_ip(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_SRC_IP, val_len_ptr);
}

static inline uint16_t mipc_wfc_natt_keep_alive_cmd_get_src_port(mipc_msg_t *msg_ptr, uint16_t def_val)
{
    if (!msg_ptr) return def_val;
    return (uint16_t)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_SRC_PORT, def_val);
}

static inline void * mipc_wfc_natt_keep_alive_cmd_get_dst_ip(mipc_msg_t *msg_ptr, uint16_t *val_len_ptr)
{
    if (!msg_ptr) { if (val_len_ptr) {*val_len_ptr = 0;} return 0;}
    return (void *)mipc_msg_get_val_ptr(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_DST_IP, val_len_ptr);
}

static inline mipc_boolean_const_enum mipc_wfc_natt_keep_alive_cmd_get_dst_port(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint16(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_CMD_T_DST_PORT, def_val);
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_ifname(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IFNAME_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_IFNAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_src_ip(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IP_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_SRC_IP, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_src_port(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_SRC_PORT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_dst_ip(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IP_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_DST_IP, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_natt_keep_alive_rsp_add_dst_port(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_WFC_NATT_KEEP_ALIVE_RSP_T_DST_PORT, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_boolean_const_enum mipc_wfc_register_wifi_signal_ntf_cmd_get_enable_ntf(mipc_msg_t *msg_ptr, mipc_boolean_const_enum def_val)
{
    if (!msg_ptr) return def_val;
    return (mipc_boolean_const_enum)mipc_msg_get_val_uint8(msg_ptr, MIPC_WFC_REGISTER_WIFI_SIGNAL_NTF_CMD_T_ENABLE_NTF, def_val);
}

static inline int16_t mipc_wfc_register_wifi_signal_ntf_cmd_get_rssi_threshold(mipc_msg_t *msg_ptr, int16_t def_val, uint32_t index)
{
    if (index >= MIPC_MAX_WFC_THRESHOLD_NUM) return def_val;
    if (!msg_ptr) return def_val;
    return (int16_t)mipc_msg_get_idx_int16(msg_ptr, MIPC_WFC_REGISTER_WIFI_SIGNAL_NTF_CMD_T_RSSI_THRESHOLD, index, def_val);
}

static inline mipc_msg_api_result_enum mipc_data_act_call_ntf_add_id(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_NTF_T_ID, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_data_act_call_ntf_add_response(mipc_msg_t *msg_ptr, enum mipc_act_call_ntf_enum_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_DATA_ACT_CALL_NTF_T_RESPONSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_inject_tst_ntf_add_module(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  32) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_INJECT_TST_NTF_T_MODULE, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_internal_inject_tst_ntf_add_index(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_INTERNAL_INJECT_TST_NTF_T_INDEX, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_internal_inject_tst_ntf_add_inject_string(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  128) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_INTERNAL_INJECT_TST_NTF_T_INJECT_STRING, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_signal_ntf_add_ifname(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IFNAME_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_SIGNAL_NTF_T_IFNAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_signal_ntf_add_rssi(mipc_msg_t *msg_ptr, int16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int16(msg_ptr, MIPC_WFC_WIFI_SIGNAL_NTF_T_RSSI, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_signal_ntf_add_snr(mipc_msg_t *msg_ptr, int16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_int16(msg_ptr, MIPC_WFC_WIFI_SIGNAL_NTF_T_SNR, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_epdg_screen_state_ntf_add_state(mipc_msg_t *msg_ptr, enum mipc_epdg_screen_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_EPDG_SCREEN_STATE_NTF_T_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ifname(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IFNAME_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IFNAME, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_enable(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_ENABLE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_cause(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_CAUSE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_associated(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_ASSOCIATED, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ssid(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_SSID_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_SSID, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ap_mac(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_MAC_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_AP_MAC, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_wifi_type(mipc_msg_t *msg_ptr, enum mipc_wifi_type_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_WIFI_TYPE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_mtu(mipc_msg_t *msg_ptr, uint16_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint16(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_MTU, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ue_mac(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_MAC_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_UE_MAC, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv4(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_IPV4_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV4, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv4_prefix_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV4_PREFIX_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv4_gateway(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_IPV4_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV4_GATEWAY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv6(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_IPV6_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV6, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv6_prefix_len(mipc_msg_t *msg_ptr, uint8_t value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV6_PREFIX_LEN, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ipv6_gateway(mipc_msg_t *msg_ptr, uint16_t len, const void  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_FIX_WFC_IPV6_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IPV6_GATEWAY, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_dns(mipc_msg_t *msg_ptr, mipc_msg_tlv_array_t *array, uint16_t index, uint16_t len, const void  *value)
{
    if (index >= MIPC_MAX_WFC_DNS_NUM) return MIPC_MSG_API_RESULT_FAIL;
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_IP_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_idx(msg_ptr, array, index, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_ip_update(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_IP_UPDATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_wifi_type_str(mipc_msg_t *msg_ptr, uint16_t len, char  *value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    if (!value) return MIPC_MSG_API_RESULT_FAIL;
    if (len >  MIPC_MAX_WFC_WIFI_TYPE_LEN) return MIPC_MSG_API_RESULT_FAIL;
    return mipc_msg_add_tlv(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_WIFI_TYPE_STR, len, (const void *)value);
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_wifi_extend_state(mipc_msg_t *msg_ptr, enum mipc_wifi_extend_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_WIFI_EXTEND_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_conn_state(mipc_msg_t *msg_ptr, enum mipc_wifi_conn_state_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_CONN_STATE, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}

static inline mipc_msg_api_result_enum mipc_wfc_wifi_info_ntf_add_conn_ready(mipc_msg_t *msg_ptr, enum mipc_boolean_const_enum value)
{
    if (!msg_ptr) return MIPC_MSG_API_RESULT_FAIL;
    mipc_msg_add_tlv_uint8(msg_ptr, MIPC_WFC_WIFI_INFO_NTF_T_CONN_READY, value);
    return MIPC_MSG_API_RESULT_SUCCESS;
}


#endif /* __MIPC_MSG_TLV_API_H__ */
