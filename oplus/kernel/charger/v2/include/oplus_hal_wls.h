// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#ifndef __OPLUS_HAL_WLS_H__
#define __OPLUS_HAL_WLS_H__

#include <oplus_chg_ic.h>
#include <oplus_mms.h>
#include <oplus_chg_wls.h>

/*wls rx APIs*/
void oplus_chg_wls_rx_dump_regs(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_smt_test(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_enable(struct oplus_chg_ic_dev *rx_ic, bool en);
bool oplus_chg_wls_rx_is_enable(struct oplus_chg_ic_dev *rx_ic);
bool oplus_chg_wls_rx_is_connected(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_get_vout(struct oplus_chg_ic_dev *rx_ic, int *vol_mv);
int oplus_chg_wls_rx_set_vout_raw(struct oplus_chg_ic_dev *rx_ic, int vol_mv);
int oplus_chg_wls_rx_get_vrect(struct oplus_chg_ic_dev *rx_ic, int *vol_mv);
int oplus_chg_wls_rx_get_iout(struct oplus_chg_ic_dev *rx_ic, int *curr_ma);
int oplus_chg_wls_rx_get_trx_vol(struct oplus_chg_ic_dev *rx_ic, int *vol_mv);
int oplus_chg_wls_rx_get_trx_curr(struct oplus_chg_ic_dev *rx_ic, int *curr_ma);
int oplus_chg_wls_get_cep(struct oplus_chg_ic_dev *rx_ic, int *cep);
int oplus_chg_wls_get_cep_check_update(struct oplus_chg_ic_dev *rx_ic, int *cep);
int oplus_chg_wls_rx_get_work_freq(struct oplus_chg_ic_dev *rx_ic, int *freq);
int oplus_chg_wls_rx_get_rx_mode(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_rx_mode *rx_mode);
int oplus_chg_wls_rx_set_rx_mode(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_rx_mode rx_mode);
int oplus_chg_wls_rx_set_dcdc_enable(struct oplus_chg_ic_dev *rx_ic, bool en);
int oplus_chg_wls_rx_set_trx_enable(struct oplus_chg_ic_dev *rx_ic, bool en);
int oplus_chg_wls_rx_set_trx_start(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_get_trx_status(struct oplus_chg_ic_dev *rx_ic, u8 *status);
int oplus_chg_wls_rx_get_trx_err(struct oplus_chg_ic_dev *rx_ic, u8 *err);
int oplus_chg_wls_get_headroom(struct oplus_chg_ic_dev *rx_ic, int *val);
int oplus_chg_wls_set_headroom(struct oplus_chg_ic_dev *rx_ic, int val);
int oplus_chg_wls_rx_send_match_q(struct oplus_chg_ic_dev *rx_ic, u8 data);
int oplus_chg_wls_rx_set_fod_parm(struct oplus_chg_ic_dev *rx_ic, u8 buf[], int len);
int oplus_chg_wls_rx_send_msg_raw(struct oplus_chg_ic_dev *rx_ic, u8 buf[], int len);
int oplus_chg_wls_rx_register_msg_callback(struct oplus_chg_ic_dev *rx_ic, void *data, void (*call_back)(void *, u8[]));
int oplus_chg_wls_rx_get_rx_version(struct oplus_chg_ic_dev *rx_ic, u32 *version);
int oplus_chg_wls_rx_get_trx_version(struct oplus_chg_ic_dev *rx_ic, u32 *version);
int oplus_chg_wls_rx_upgrade_firmware_by_buf_raw(struct oplus_chg_ic_dev *rx_ic, unsigned char *buf, int len);
int oplus_chg_wls_rx_upgrade_firmware_by_img_raw(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_connect_check(struct oplus_chg_ic_dev *rx_ic);
int oplus_chg_wls_rx_get_event_code(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_event_code *code);

/*wls nor APIs*/
int oplus_chg_wls_nor_set_input_enable(struct oplus_chg_ic_dev *nor_ic, bool en);
int oplus_chg_wls_nor_set_output_enable(struct oplus_chg_ic_dev *nor_ic, bool en);
int oplus_chg_wls_nor_set_icl_raw(struct oplus_chg_ic_dev *nor_ic, int icl_ma);
int oplus_chg_wls_nor_set_fcc_raw(struct oplus_chg_ic_dev *nor_ic, int fcc_ma);
int oplus_chg_wls_nor_set_fv(struct oplus_chg_ic_dev *nor_ic, int fv_mv);
int oplus_chg_wls_nor_set_rechg_vol(struct oplus_chg_ic_dev *nor_ic, int rechg_vol_mv);
int oplus_chg_wls_nor_get_icl(struct oplus_chg_ic_dev *nor_ic, int *icl_ma);
int oplus_chg_wls_nor_get_input_curr(struct oplus_chg_ic_dev *nor_ic, int *curr_ma);
int oplus_chg_wls_nor_get_input_vol(struct oplus_chg_ic_dev *nor_ic, int *vol_mv);
int oplus_chg_wls_nor_set_boost_en(struct oplus_chg_ic_dev *nor_ic, bool en);
int oplus_chg_wls_nor_set_boost_vol(struct oplus_chg_ic_dev *nor_ic, int vol_mv);
int oplus_chg_wls_nor_set_boost_curr_limit(struct oplus_chg_ic_dev *nor_ic, int curr_ma);
int oplus_chg_wls_nor_set_aicl_enable(struct oplus_chg_ic_dev *nor_ic, bool en);
int oplus_chg_wls_nor_set_aicl_rerun(struct oplus_chg_ic_dev *nor_ic);
int oplus_chg_wls_nor_set_vindpm(struct oplus_chg_ic_dev *nor_ic, int vindpm_mv);
int oplus_chg_wls_nor_set_usb_drv(struct oplus_chg_ic_dev *nor_ic, bool en);

/*wls fast APIs*/
int oplus_chg_wls_fast_set_enable(struct oplus_chg_ic_dev *fast_ic, bool en);
int oplus_chg_wls_fast_start(struct oplus_chg_ic_dev *fast_ic);
int oplus_chg_wls_fast_smt_test(struct oplus_chg_ic_dev *fast_ic);
int oplus_chg_wls_fast_get_fault(struct oplus_chg_ic_dev *fast_ic, char *fault);
#endif /* __OPLUS_HAL_WLS_H__ */
