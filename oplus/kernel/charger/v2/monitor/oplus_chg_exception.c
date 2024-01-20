// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2023 Oplus. All rights reserved.
 */

#include <linux/err.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <asm/current.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>

#include <oplus_chg.h>
#include <oplus_chg_exception.h>
#include "oplus_monitor_internal.h"

enum olc_notify_type {
	OLC_NOTIFY_TYPE_SOC_JUMP,
	OLC_NOTIFY_TYPE_GENERAL_RECORD,
	OLC_NOTIFY_TYPE_NO_CHARGING,
	OLC_NOTIFY_TYPE_CHARGING_SLOW,
	OLC_NOTIFY_TYPE_CHARGING_BREAK,
	OLC_NOTIFY_TYPE_DEVICE_ABNORMAL,
	OLC_NOTIFY_TYPE_SOFTWARE_ABNORMAL,
};

struct chg_exception_table {
	int type_reason;
	int olc_type;
};

static inline void *chg_kzalloc(size_t size, gfp_t flags)
{
	void *p;

	p = kzalloc(size, flags);

	if (!p)
		chg_err("Failed to allocate memory\n");

	return p;
}

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_LOG_CORE)
#include <soc/oplus/system/olc.h>

#define CHG_MODULE_NAME         "charger"
#define CHG_LOG_PATH            "kernel/chg_debug_info"
#define CHG_MODULE_ID           0x2A
#define CHG_RESERVED_ID         0x100

static int chg_olc_raise_exception(unsigned int excep_chgye, void *summary, unsigned int summary_size)
{
	struct exception_info *exp_info = NULL;
	int retval = -1;
	int len  = 0;

	chg_info("enter,type:%d\n", excep_chgye);

	exp_info = chg_kzalloc(sizeof(struct exception_info), GFP_KERNEL);
	if (!exp_info) {
		chg_err("Failed to allocate chg memory\n");
		return -ENOMEM;
	}

	if (excep_chgye > 0xfff) {
		chg_err("excep_chgye:%d is beyond 0xfff\n", excep_chgye);
		goto free_exp;
	}
	exp_info->time = 0;
	exp_info->id = (CHG_RESERVED_ID << 20) | (CHG_MODULE_ID << 12) | excep_chgye;
	exp_info->pid = 0;
	exp_info->exceptionType = EXCEPTION_KERNEL;
	exp_info->faultLevel = 0;
	exp_info->logOption = LOG_KERNEL | LOG_MAIN;

	len = sizeof(CHG_MODULE_NAME);
	len = len > sizeof(exp_info->module) ? sizeof(exp_info->module) : len;
	memcpy(exp_info->module, CHG_MODULE_NAME, len);
	len = sizeof(CHG_LOG_PATH);
	len = len > sizeof(exp_info->module) ? sizeof(exp_info->module) : len;
	memcpy(exp_info->logPath, CHG_LOG_PATH, len);
	len = summary_size;
	len = len > sizeof(exp_info->module) ? sizeof(exp_info->module) : len;
	memcpy(exp_info->summary, summary, len);

	retval = olc_raise_exception(exp_info);
	if (retval) {
		chg_err("raise fail, ret:%d\n", retval);
	}

free_exp:
	if (exp_info != NULL)
		kfree(exp_info);
	return retval;
}

static int chg_olc_battery_log_raise_exception(void)
{
	return 0;
}
#elif IS_ENABLED(CONFIG_OPLUS_FEATURE_OLC)
#include <soc/oplus/dft/olc.h>

#define CHG_LOG_PATH            "kernel/chg_debug_info"
#define CHG_MODULE_ID           0x2A
#define CHG_RESERVED_ID         0x100
#define CHG_BATTERY_LOG_ID      0xfff

#ifndef LOG_CHARGER
#define LOG_CHARGER             (0x1L << 42)
#endif  /*add for olc project compatible*/

static int chg_olc_raise_exception(unsigned int excep_chgye, void *summary, unsigned int summary_size)
{
	struct exception_info *exp_info = NULL;
	int retval = -1;
	int len = 0;
	struct timespec64 time;

	chg_info("enter,type:%d\n", excep_chgye);

	exp_info = chg_kzalloc(sizeof(struct exception_info), GFP_KERNEL);
	if (!exp_info) {
		chg_err("Failed to allocate chg memory\n");
		return -ENOMEM;
	}

	if (excep_chgye > 0xfff) {
		chg_err("excep_chgye:%d is beyond 0xfff\n", excep_chgye);
		goto free_exp;
	}
	ktime_get_ts64(&time);
	exp_info->time = time.tv_sec;
	exp_info->exceptionId = (CHG_RESERVED_ID << 20) | (CHG_MODULE_ID << 12) | excep_chgye;
	exp_info->exceptionType = EXCEPTION_KERNEL;
	exp_info->level = 0;
	exp_info->atomicLogs = LOG_KERNEL | LOG_MAIN;

	len = sizeof(CHG_LOG_PATH);
	len = len > sizeof(exp_info->logParams) ? sizeof(exp_info->logParams) : len;
	memcpy(exp_info->logParams, CHG_LOG_PATH, len);

	retval = olc_raise_exception(exp_info);
	if (retval) {
		chg_err("raise fail, ret:%d\n", retval);
	}

free_exp:
	if (exp_info != NULL)
		kfree(exp_info);
	return retval;
}

static int chg_olc_battery_log_raise_exception(void)
{
	struct exception_info *exp_info = NULL;
	int retval = -1;
	int len = 0;
	struct timespec64 time;

	exp_info = chg_kzalloc(sizeof(struct exception_info), GFP_KERNEL);
	if (!exp_info) {
		chg_err("Failed to allocate chg memory\n");
		return -ENOMEM;
	}

	ktime_get_ts64(&time);
	exp_info->time = time.tv_sec;
	exp_info->exceptionId = (CHG_RESERVED_ID << 20) | (CHG_MODULE_ID << 12) | CHG_BATTERY_LOG_ID;
	exp_info->exceptionType = EXCEPTION_KERNEL;
	exp_info->level = 0;
	exp_info->atomicLogs = LOG_CHARGER;

	len = sizeof(CHG_LOG_PATH);
	len = len > sizeof(exp_info->logParams) ? sizeof(exp_info->logParams) : len;
	memcpy(exp_info->logParams, CHG_LOG_PATH, len);

	retval = olc_raise_exception(exp_info);
	if (retval)
		chg_err("raise fail, ret:%d\n", retval);

	if (exp_info != NULL)
		kfree(exp_info);
	return retval;
}

#else
static  int chg_olc_raise_exception(unsigned int excep_chgye, void *summary, unsigned int summary_size)
{
	return 0;
}

static int chg_olc_battery_log_raise_exception(void)
{
	return 0;
}
#endif /* CONFIG_OPLUS_KEVENT_UPLOAD_DELETE */

static struct chg_exception_table olc_table[] = {
	{ TRACK_NOTIFY_TYPE_SOC_JUMP, OLC_NOTIFY_TYPE_SOC_JUMP },
	{ TRACK_NOTIFY_TYPE_GENERAL_RECORD, OLC_NOTIFY_TYPE_GENERAL_RECORD },
	{ TRACK_NOTIFY_TYPE_NO_CHARGING, OLC_NOTIFY_TYPE_NO_CHARGING },
	{ TRACK_NOTIFY_TYPE_CHARGING_SLOW, OLC_NOTIFY_TYPE_CHARGING_SLOW },
	{ TRACK_NOTIFY_TYPE_CHARGING_BREAK, OLC_NOTIFY_TYPE_CHARGING_BREAK },
	{ TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL, OLC_NOTIFY_TYPE_DEVICE_ABNORMAL },
	{ TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL, OLC_NOTIFY_TYPE_SOFTWARE_ABNORMAL },
};

#define TRACK_LOCAL_T_NS_TO_S_THD		1000000000
#define TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD	(1 * 3600)
#define MAX_UPLOAD_COUNT	3
static int olc_track_get_local_time_s(void)
{
	int local_time_s;

	local_time_s = local_clock() / TRACK_LOCAL_T_NS_TO_S_THD;
	chg_info("local_time_s:%d\n", local_time_s);

	return local_time_s;
}

static bool oplus_chg_olc_check_type_reason(struct exception_data *excep_data, int type_reason, int excep_flag)
{
	int i;
	int olc_type;
	int olc_config = 1 << excep_flag;
	bool ret = false;

	for (i = 0; i < ARRAY_SIZE(olc_table); i++) {
		if(olc_table[i].type_reason == type_reason)
			break;
	}
	if (i >= ARRAY_SIZE(olc_table)) {
		chg_err("not support olc track type\n");
		return ret;
	}
	olc_type = olc_table[i].olc_type;
	if (olc_config & excep_data->olc_config[olc_type])
		ret = true;
	else
		ret = false;
	return ret;
}

static int oplus_chg_olc_get_excep_reason(int type_reason, int flag_reason, int *diff)
{
	int excep_value = -1;
	int flag_base_value = 0;
	int olc_base_value = 0;

	if (!diff)
		return excep_value;

	switch (type_reason) {
	case TRACK_NOTIFY_TYPE_SOC_JUMP:
		flag_base_value = TRACK_NOTIFY_FLAG_UI_SOC_LOAD_JUMP;
		olc_base_value = EXCEP_SOC_ERROR_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_GENERAL_RECORD:
		flag_base_value = TRACK_NOTIFY_FLAG_CHARGER_INFO;
		olc_base_value = EXCEP_GENERAL_RECORD_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_NO_CHARGING:
		flag_base_value = TRACK_NOTIFY_FLAG_NO_CHARGING;
		olc_base_value = EXCEP_NO_CHARGING_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_CHARGING_SLOW:
		flag_base_value = TRACK_NOTIFY_FLAG_CHG_SLOW_TBATT_WARM;
		olc_base_value = EXCEP_CHARGING_SLOW_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_CHARGING_BREAK:
		flag_base_value = TRACK_NOTIFY_FLAG_FAST_CHARGING_BREAK;
		olc_base_value = EXCEP_CHARGING_BREAK_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_DEVICE_ABNORMAL:
		flag_base_value = TRACK_NOTIFY_FLAG_WLS_ABNORMAL;
		olc_base_value = EXCEP_DEVICE_ABNORMAL_DEFAULT;
		break;
	case TRACK_NOTIFY_TYPE_SOFTWARE_ABNORMAL:
		flag_base_value = TRACK_NOTIFY_FLAG_SMART_CHG_ABNORMAL;
		olc_base_value = EXCEP_SOFTWARE_ABNORMAL_DEFAULT;
		break;
	default:
		break;
	}
	*diff = flag_reason - flag_base_value;
	if ((*diff < 0) ||(*diff > OLC_FLAG_NUM_MAX)) {
		chg_err("error olc flag reason\n");
		return excep_value;
	}
	excep_value = olc_base_value + *diff;
	chg_info("excep_value = %d, diff = %d\n", excep_value, *diff);
	return excep_value;
}

int chg_exception_report(void *chg_exception_data, int type_reason, int flag_reason, void *summary, unsigned int summary_size)
{
	int ret = -1;
	int type_diff = 0;
	int excep_chgye = 0;
	bool should_upload = false;
	struct exception_data *exception_data = (struct exception_data *)chg_exception_data;

	if ((!exception_data) || (!summary))
		return ret;

	chg_info("reason: %s, receive type:%d, flag:%d,for olc type\n", (char *)summary, type_reason, flag_reason);
	excep_chgye = oplus_chg_olc_get_excep_reason(type_reason, flag_reason, &type_diff);
	if (excep_chgye < 0) {
		chg_err("error olc exception tyep\n");
		return ret;
	}

	should_upload = oplus_chg_olc_check_type_reason(exception_data, type_reason, type_diff);

	if (should_upload == false) {
		chg_err("not allow upload olc type\n");
		return ret;
	}

	exception_data->excep[flag_reason].num++;
	exception_data->excep[flag_reason].curr_time = olc_track_get_local_time_s();
	if (exception_data->excep[flag_reason].num == 1) {
		exception_data->excep[flag_reason].pre_upload_time = olc_track_get_local_time_s();
	}
	if (exception_data->excep[flag_reason].curr_time -
		exception_data->excep[flag_reason].pre_upload_time > TRACK_DEVICE_ABNORMAL_UPLOAD_PERIOD)
		exception_data->excep[flag_reason].num = 1;
	if (exception_data->excep[flag_reason].num > MAX_UPLOAD_COUNT) {
		chg_err("not allow upload olc max times\n");
		return ret;
	}

	ret = chg_olc_raise_exception(excep_chgye, summary, summary_size);

	return ret;
}

#define COUNT_TIMELIMIT	60
#define SUCCESS_COUNT	100
int oplus_chg_batterylog_exception_push(void)
{
	int ret = -1;
	static int count = 0;
	static unsigned long count_time;

	if (time_after(jiffies, count_time)) {
		count_time = COUNT_TIMELIMIT * HZ;
		count_time += jiffies;
		ret = chg_olc_battery_log_raise_exception();
		if (ret > 0) {
			count++;
			chg_info("push battery log %d times\n", count);
		} else {
			chg_err("push battery log fail");
		}
	}

	if (count > SUCCESS_COUNT) {
		count = 0;
		chg_info("push battery log more than 100 times, count to 0\n");
	}

	return ret;
}

int oplus_chg_set_app_info(const char *buf)
{
	int ret = 0;

	if (!buf)
		return -1;

	ret = oplus_chg_track_set_app_info(buf);
	return ret;
}

int oplus_chg_olc_config_set(const char *buf)
{
	int ret = 0;

	if (!buf)
		return -1;

	ret = oplus_chg_track_olc_config_set(buf);
	return ret;
}

int oplus_chg_olc_config_get(char *buf)
{
	int ret = 0;

	ret = oplus_chg_track_olc_config_get(buf);
	return ret;
}
