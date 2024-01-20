// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[HAL_WLS]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/list.h>
#include <linux/kthread.h>

#include <oplus_chg.h>
#include <oplus_chg_module.h>
#include <oplus_chg_ic.h>
#include <oplus_chg_monitor.h>
#include <oplus_mms.h>
#include <oplus_mms_wired.h>

/**********************************************************************
* wls rx APIs:
**********************************************************************/
void oplus_chg_wls_rx_dump_regs(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;

	if (!rx_ic) {
		chg_err("rx_ic is NULL!\n");
		return;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_REG_DUMP);
	if (rc < 0)
		chg_err("can't dump rx reg, rc=%d\n", rc);
}

#ifdef USE_DEFAULT_SMT_TEST
int oplus_chg_wls_rx_smt_test(struct oplus_chg_ic_dev *rx_ic, char buf[], int len)
{
	int rc;

	if (!rx_ic) {
		chg_err("rx_ic is NULL!\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_SMT_TEST, buf, len);

	return rc;
}
#endif

int oplus_chg_wls_rx_enable(struct oplus_chg_ic_dev *rx_ic, bool en)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_ENABLE, en);
	if (rc < 0)
		chg_err("can't set rx enable, rc=%d\n", rc);

	return rc;
}

bool oplus_chg_wls_rx_is_enable(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;
	bool enable;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return false;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_IS_ENABLE, &enable);
	if (rc < 0) {
		chg_err("can't get rx_is_enable, rc=%d\n", rc);
		return false;
	}

	return enable;
}

bool oplus_chg_wls_rx_is_connected(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;
	bool connected;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return false;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_IS_CONNECTED, &connected);
	if (rc < 0) {
		chg_err("can't get rx_is_connected, rc=%d\n", rc);
		return false;
	}

	return connected;
}

int oplus_chg_wls_rx_get_vout(struct oplus_chg_ic_dev *rx_ic, int *vol_mv)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_VOUT, vol_mv);
	if (rc < 0)
		chg_err("can't get vout, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_vout_raw(struct oplus_chg_ic_dev *rx_ic, int vol_mv)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_VOUT, vol_mv);
	if (rc < 0)
		chg_err("can't set vout to %dmV, rc=%d\n", vol_mv, rc);

	return rc;
}

int oplus_chg_wls_rx_get_vrect(struct oplus_chg_ic_dev *rx_ic, int *vol_mv)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_VRECT, vol_mv);
	if (rc < 0)
		chg_err("can't get vrect, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_iout(struct oplus_chg_ic_dev *rx_ic, int *curr_ma)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_IOUT, curr_ma);
	if (rc < 0)
		chg_err("can't get iout, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_trx_vol(struct oplus_chg_ic_dev *rx_ic, int *vol_mv)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_TRX_VOL, vol_mv);
	if (rc < 0)
		chg_err("can't get tx vout, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_trx_curr(struct oplus_chg_ic_dev *rx_ic, int *curr_ma)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_TRX_CURR, curr_ma);
	if (rc < 0)
		chg_err("can't get tx iout, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_get_cep(struct oplus_chg_ic_dev *rx_ic, int *cep)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_CEP_VAL, cep);
	if (rc < 0)
		chg_err("can't get cep val, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_get_cep_check_update(struct oplus_chg_ic_dev *rx_ic, int *cep)
{
	static int cep_count;
	int cep_count_temp;
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_CEP_COUNT, &cep_count_temp);
	if (rc < 0) {
		chg_err("can't get cep count, rc=%d\n", rc);
		return rc;
	}
	if (cep_count == cep_count_temp) {
		chg_info("cep not update\n");
		return -EINVAL;
	}
	cep_count = cep_count_temp;
	rc = oplus_chg_wls_get_cep(rx_ic, cep);
	if (rc < 0)
		chg_err("can't get cep val, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_work_freq(struct oplus_chg_ic_dev *rx_ic, int *freq)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_WORK_FREQ, freq);
	if (rc < 0)
		chg_err("can't get work freq, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_rx_mode(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_rx_mode *rx_mode)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_RX_MODE, rx_mode);
	if (rc < 0)
		chg_err("can't get rx mode, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_rx_mode(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_rx_mode rx_mode)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_RX_MODE, rx_mode);
	if (rc < 0)
		chg_err("can't set rx mode, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_dcdc_enable(struct oplus_chg_ic_dev *rx_ic, bool en)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_DCDC_ENABLE, en);
	if (rc < 0)
		chg_err("can't set dcdc enable, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_trx_enable(struct oplus_chg_ic_dev *rx_ic, bool en)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_TRX_ENABLE, en);
	if (rc < 0)
		chg_err("can't set tx enable, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_trx_start(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_TRX_START);
	if (rc < 0)
		chg_err("can't set tx start, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_trx_status(struct oplus_chg_ic_dev *rx_ic, u8 *status)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_TRX_STATUS, status);
	if (rc < 0)
		chg_err("can't get tx status, rc=%d\n", rc);
	return rc;
}

int oplus_chg_wls_rx_get_trx_err(struct oplus_chg_ic_dev *rx_ic, u8 *err)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_TRX_ERR, err);
	if (rc < 0)
		chg_err("can't get tx err, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_get_headroom(struct oplus_chg_ic_dev *rx_ic, int *val)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_HEADROOM, val);
	if (rc < 0)
		chg_err("can't get headroom, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_set_headroom(struct oplus_chg_ic_dev *rx_ic, int val)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_HEADROOM, val);
	if (rc < 0)
		chg_err("can't set headroom, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_send_match_q(struct oplus_chg_ic_dev *rx_ic, u8 data)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}
	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SEND_MATCH_Q, data);
	if (rc < 0)
		chg_err("can't send match q, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_set_fod_parm(struct oplus_chg_ic_dev *rx_ic, u8 buf[], int len)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SET_FOD_PARM, buf, len);
	if (rc < 0)
		chg_err("can't set fod parm, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_send_msg_raw(struct oplus_chg_ic_dev *rx_ic, u8 buf[], int len)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_SEND_MSG, buf, len);
	if (rc < 0)
		chg_err("can't send msg, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_register_msg_callback(struct oplus_chg_ic_dev *rx_ic, void *data, void (*call_back)(void *, u8[]))
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_REG_MSG_CALLBACK, data, call_back);
	if (rc < 0)
		chg_err("can't register msg callback, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_rx_version(struct oplus_chg_ic_dev *rx_ic, u32 *version)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_RX_VERSION, version);
	if (rc < 0)
		chg_err("can't get rx version, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_trx_version(struct oplus_chg_ic_dev *rx_ic, u32 *version)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_TRX_VERSION, version);
	if (rc < 0)
		chg_err("can't get tx version, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_upgrade_firmware_by_buf_raw(struct oplus_chg_ic_dev *rx_ic, unsigned char *buf, int len)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_BUF, buf, len);
	if (rc < 0)
		chg_err("can't upgrade fw by buf, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_upgrade_firmware_by_img_raw(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_UPGRADE_FW_BY_IMG);
	if (rc < 0)
		chg_err("can't upgrade fw by img, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_connect_check(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_CHECK_CONNECT);
	if (rc < 0)
		chg_err("can't start rx connect check, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_get_event_code(struct oplus_chg_ic_dev *rx_ic, enum oplus_chg_wls_event_code *code)
{
	int rc;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(rx_ic, OPLUS_IC_FUNC_RX_GET_EVENT_CODE, code);
	if (rc < 0)
		chg_err("can't get event code, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_rx_smt_test(struct oplus_chg_ic_dev *rx_ic)
{
	int rc;
	u32 err_code = 0;
	u32 version;

	if (rx_ic == NULL) {
		chg_err("rx_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_wls_rx_get_rx_version(rx_ic, &version);
	if (rc < 0) {
		chg_err("can't get rx version, rc=%d\n", rc);
		err_code |= BIT(rx_ic->index);
	}

	return err_code;
}

/**********************************************************************
* wls nor APIs:
**********************************************************************/
__maybe_unused static bool oplus_chg_is_usb_present(void)
{
	struct oplus_mms *wired_topic;
	union mms_msg_data data = { 0 };
	int rc;

	wired_topic = oplus_mms_get_by_name("wired");
	if (!wired_topic)
		return false;

	rc = oplus_mms_get_item_data(wired_topic, WIRED_ITEM_PRESENT, &data, true);
	if (rc < 0)
		return false;

	return !!data.intval;
}

int oplus_chg_wls_nor_set_input_enable(struct oplus_chg_ic_dev *nor_ic, bool en)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_WLS_INPUT_SUSPEND, !en);
	if (rc < 0)
		chg_err("can't %s wls nor input, rc=%d\n", en ? "enable" : "disable", rc);
	else
		chg_info("%s wls nor input\n", en ? "enable" : "disable");

	return rc;
}

int oplus_chg_wls_nor_set_output_enable(struct oplus_chg_ic_dev *nor_ic, bool en)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}
	if (oplus_chg_is_usb_present()) {
		chg_debug("usb present, exit\n");
		return -EPERM;
	}
	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_OUTPUT_SUSPEND, !en);
	if (rc < 0)
		chg_err("can't %s wls nor output, rc=%d\n", en ? "enable" : "disable", rc);
	else
		chg_info("%s wls nor output\n", en ? "enable" : "disable");

	return rc;
}

int oplus_chg_wls_nor_set_icl_raw(struct oplus_chg_ic_dev *nor_ic, int icl_ma)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_SET_WLS_ICL, icl_ma);
	if (rc < 0)
		chg_err("set icl to %d mA error, rc=%d\n", icl_ma, rc);

	return rc;
}

int oplus_chg_wls_nor_set_fcc_raw(struct oplus_chg_ic_dev *nor_ic, int fcc_ma)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_SET_FCC, fcc_ma);
	if (rc < 0)
		chg_err("set fcc to %d mA error, rc=%d\n", fcc_ma, rc);

	return rc;
}

int oplus_chg_wls_nor_set_fv(struct oplus_chg_ic_dev *nor_ic, int fv_mv)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_SET_FV, fv_mv);
	if (rc < 0) {
		chg_err("set fv to %d mV error, rc=%d\n", fv_mv, rc);
		return rc;
	} else {
		chg_info("set fv to %d mV\n", fv_mv);
	}

	return 0;
}

int oplus_chg_wls_nor_set_rechg_vol(struct oplus_chg_ic_dev *nor_ic, int rechg_vol_mv)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_SET_RECHG_VOL, rechg_vol_mv);
	if (rc < 0) {
		chg_err("set rechg vol to %d mV error, rc=%d\n", rechg_vol_mv, rc);
		return rc;
	} else {
		chg_info("set rechg vol to %d mV\n", rechg_vol_mv);
	}

	return 0;
}

int oplus_chg_wls_nor_get_icl(struct oplus_chg_ic_dev *nor_ic, int *icl_ma)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_GET_WLS_ICL, icl_ma);
	if (rc < 0) {
		chg_err("get icl error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_get_input_curr(struct oplus_chg_ic_dev *nor_ic, int *curr_ma)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_GET_WLS_INPUT_CURR, curr_ma);
	if (rc < 0) {
		chg_err("get input current error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_get_input_vol(struct oplus_chg_ic_dev *nor_ic, int *vol_mv)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_GET_WLS_INPUT_VOL, vol_mv);
	if (rc < 0) {
		chg_err("get input voltage error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_boost_en(struct oplus_chg_ic_dev *nor_ic, bool en)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL!\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_WLS_BOOST_ENABLE, en);
	if (rc < 0) {
		chg_err("set boost %s error, rc=%d\n", en ? "enable" : "disable", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_boost_vol(struct oplus_chg_ic_dev *nor_ic, int vol_mv)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL!\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_SET_WLS_BOOST_VOL, vol_mv);
	if (rc < 0) {
		chg_err("set boost vol to %d mV, rc=%d\n", vol_mv, rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_boost_curr_limit(struct oplus_chg_ic_dev *nor_ic, int curr_ma)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL!\n");
		return -ENODEV;
	}


	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_SET_WLS_BOOST_CURR_LIMIT, curr_ma);
	if (rc < 0) {
		chg_err("set boost curr limit to %d mA, rc=%d\n", curr_ma, rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_aicl_enable(struct oplus_chg_ic_dev *nor_ic, bool en)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_WLS_AICL_ENABLE, en);
	if (rc < 0) {
		chg_err("can't %s aicl, rc=%d\n", en ? "enable" : "disable", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_aicl_rerun(struct oplus_chg_ic_dev *nor_ic)
{
	int rc;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_WLS_AICL_RERUN);
	if (rc < 0) {
		chg_err("can't rerun aicl, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int oplus_chg_wls_nor_set_vindpm(struct oplus_chg_ic_dev *nor_ic, int vindpm_mv)
{
	int rc = 0;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_SET_VINDPM, vindpm_mv);

	return rc;
}

int oplus_chg_wls_nor_set_usb_drv(struct oplus_chg_ic_dev *nor_ic, bool en)
{
	int rc = 0;

	if (nor_ic == NULL) {
		chg_err("nor_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(nor_ic, OPLUS_IC_FUNC_BUCK_DIS_INSERT_DETECT, en);

	return rc;
}

/**********************************************************************
* wls fast APIs:
**********************************************************************/
int oplus_chg_wls_fast_set_enable(struct oplus_chg_ic_dev *fast_ic, bool en)
{
	int rc;

	if (fast_ic == NULL) {
		chg_err("fast_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(fast_ic, OPLUS_IC_FUNC_CP_ENABLE, en);
	if (rc < 0)
		chg_err("can't set cp %s, rc=%d\n", en ? "enable" : "disable", rc);

	return rc;
}

int oplus_chg_wls_fast_start(struct oplus_chg_ic_dev *fast_ic)
{
	int rc;

	if (fast_ic == NULL) {
		chg_err("fast_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(fast_ic, OPLUS_IC_FUNC_CP_SET_WORK_START, true);
	if (rc < 0)
		chg_err("can't set cp start, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_fast_smt_test(struct oplus_chg_ic_dev *fast_ic)
{
	int rc;

	if (fast_ic == NULL) {
		chg_err("fast_ic is NULL\n");
		return -ENODEV;
	}

	rc = oplus_chg_ic_func(fast_ic, OPLUS_IC_FUNC_CP_SET_WORK_START, true);
	if (rc < 0)
		chg_err("can't set cp start, rc=%d\n", rc);

	return rc;
}

int oplus_chg_wls_fast_get_fault(struct oplus_chg_ic_dev *fast_ic, char *fault)
{
	int rc = 0;

	if (fast_ic == NULL) {
		chg_err("fast_ic is NULL\n");
		return -ENODEV;
	}

	if (fault == NULL) {
		chg_err("fault is NULL");
		return -ENODEV;
	}
	/*TODO*/

	return rc;
}
