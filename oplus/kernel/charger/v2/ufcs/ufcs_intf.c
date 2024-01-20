// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[INTF]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/power_supply.h>

#include "ufcs_class.h"
#include "ufcs_core.h"

int ufcs_msg_handler(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;

	kthread_queue_work(class->worker, &class->recv_work);

	return 0;
}
EXPORT_SYMBOL(ufcs_msg_handler);

static int ufcs_pdo_select(struct ufcs_class *class, int vol_mv, int curr_ma)
{
	int i;
	u64 pdo;
	u32 max_curr;

	if (class->pdo.num == 0) {
		ufcs_err("no pdo data\n");
		return -ENODATA;
	}

	for (i = 0; i < class->pdo.num; i++) {
		pdo = class->pdo.data[i];

		if (vol_mv < UFCS_OUTPUT_MODE_VOL_MIN(pdo))
			continue;
		if (vol_mv > UFCS_OUTPUT_MODE_VOL_MAX(pdo))
			continue;
		if (curr_ma < UFCS_OUTPUT_MODE_CURR_MIN(pdo))
			continue;
		max_curr = 0;
		if (class->power_changed)
			max_curr = UFCS_POWER_CHANGE_CURR_MAX(class->pwr_change_info[i]);
		if (max_curr == 0 || max_curr > UFCS_OUTPUT_MODE_CURR_MAX(pdo))
			max_curr = UFCS_OUTPUT_MODE_CURR_MAX(pdo);
		if (curr_ma > max_curr)
			continue;
		return ++i;
	}

	return -ENODATA;
}

static int __ufcs_pdo_set(struct ufcs_class *class, int index, int vol_mv, int curr_ma)
{
	int rc;
	struct ufcs_data_msg_request *request;
	struct ufcs_event *event;

	event = devm_kzalloc(&class->ufcs->dev,
		sizeof(struct ufcs_data_msg_request) + sizeof(struct ufcs_event),
		GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	request = (struct ufcs_data_msg_request *)((u8 *)event + sizeof(struct ufcs_event));
	event->data = request;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_REQUEST;
	INIT_LIST_HEAD(&event->list);
	request->request = UFCS_REQUEST_DATA(index, vol_mv, curr_ma);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push pdo request event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("set pdo error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int ufcs_pdo_set(struct ufcs_class *class, int vol_mv, int curr_ma)
{
	int rc;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	rc = ufcs_pdo_select(class, vol_mv, curr_ma);
	if (rc < 0)
		return rc;
	rc = __ufcs_pdo_set(class, rc, vol_mv, curr_ma);

	return rc;
}

int ufcs_intf_pdo_set(struct ufcs_dev *ufcs, int vol_mv, int curr_ma)
{
	struct ufcs_class *class;
	int rc;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	rc = ufcs_pdo_select(class, vol_mv, curr_ma);
	if (rc < 0)
		return rc;
	rc = __ufcs_pdo_set(class, rc, vol_mv, curr_ma);

	return rc;
}
EXPORT_SYMBOL(ufcs_intf_pdo_set);

int ufcs_handshake(struct ufcs_dev *ufcs)
{
	int rc;
	struct ufcs_event *event;
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	ufcs_info("start handshake\n");

	class->start_cable_detect = false;
	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_HANDSHAKE;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->handshake_lock);
	ufcs_event_reset(class);
	ufcs->ops->enable(ufcs);
	class->sm_task_wakeup = true;
	wake_up(&class->sm_wq);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->handshake_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push handshake event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	class->handshake_success = false;
	mutex_unlock(&class->pe_lock);
	mutex_unlock(&class->handshake_lock);
	wait_for_completion(&class->request_ack);
	mutex_unlock(&class->ext_req_lock);

	if (!READ_ONCE(class->handshake_success)) {
		ufcs->ops->disable(ufcs);
		ufcs_err("handshake fail");
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(ufcs_handshake);

int ufcs_source_hard_reset(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;

	ufcs_err("send source hard reset\n");
	ufcs_exit_sm_work(class);
	ufcs_clean_process_info(class);
	class->ufcs->ops->source_hard_reset(class->ufcs);
	class->handshake_success = false;
	return class->ufcs->ops->disable(class->ufcs);
}
EXPORT_SYMBOL(ufcs_source_hard_reset);

int ufcs_cable_hard_reset(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;

	ufcs_err("send cable hard reset\n");
	return class->ufcs->ops->cable_hard_reset(class->ufcs);
}
EXPORT_SYMBOL(ufcs_cable_hard_reset);

static int ufcs_send_exit_ufcs_mode(struct ufcs_class *class)
{
	int rc;
	struct ufcs_event *event;

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_EXIT_UFCS_MODE;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push exit ufcs mode event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("exit ufcs mode error, rc=%d\n", rc);
		return rc;
	}
	class->handshake_success = false;

	return 0;
}

int ufcs_force_exit(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;
	struct power_supply *usb_psy;
	union power_supply_propval pval = { 0 };
	bool usb_online;
	int rc;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;

	ufcs_info("ufcs exit\n");
	mutex_lock(&class->handshake_lock);
	usb_psy = power_supply_get_by_name("usb");
	if (!usb_psy) {
		ufcs_err("usb psy not found\n");
		usb_online = true;
	} else {
		rc = power_supply_get_property(usb_psy, POWER_SUPPLY_PROP_ONLINE, &pval);
		if (rc < 0) {
			ufcs_err("can't get usb online status, rc=%d\n", rc);
			usb_online = true;
		} else {
			usb_online = !!pval.intval;
		}
		power_supply_put(usb_psy);
	}

	if (READ_ONCE(class->handshake_success) && usb_online) {
		rc = ufcs_send_exit_ufcs_mode(class);
		if (rc < 0) {
			ufcs_err("send exit ufcs mode error, send hard reset\n");
			ufcs_source_hard_reset(ufcs);
		}
	}

	class->ufcs->ops->disable(class->ufcs);
	class->event.exit = true;
	class->test_mode = false;
	class->sm_task_wakeup = false;
	complete_all(&class->event.ack);
	ufcs_clean_process_info(class);
	ufcs_exit_sm_work(class);
	if (mutex_is_locked(&class->pe_lock))
		mutex_unlock(&class->pe_lock);
	mutex_unlock(&class->handshake_lock);

	return 0;
}
EXPORT_SYMBOL(ufcs_force_exit);

int ufcs_config_watchdog(struct ufcs_class *class, u16 time_ms)
{
	int rc;
	struct ufcs_event *event;
	struct ufcs_data_msg_config_watchdog *config_wd;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev,
		sizeof(struct ufcs_event) + sizeof(struct ufcs_data_msg_config_watchdog),
		GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	config_wd = (struct ufcs_data_msg_config_watchdog *)((u8 *)event + sizeof(struct ufcs_event));
	config_wd->over_time_ms = time_ms;
	event->data = config_wd;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_CONFIG_WD;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push config watch dog event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("config watch dog error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int ufcs_intf_config_watchdog(struct ufcs_dev *ufcs, u16 time_ms)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_config_watchdog(class, time_ms);
}
EXPORT_SYMBOL(ufcs_intf_config_watchdog);

void ufcs_clr_error_flag(struct ufcs_dev *ufcs)
{
	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return;
	}

	ufcs->dev_err_flag = 0;
}
EXPORT_SYMBOL(ufcs_clr_error_flag);

int ufcs_get_device_info(struct ufcs_class *class, u64 *dev_info)
{
	int rc;
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (dev_info == NULL) {
		ufcs_err("dev_info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_GET_DEVICE_INFO;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push get device info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get device info error, rc=%d\n", rc);
		return rc;
	}

	*dev_info = class->dev_info;

	return 0;
}

int ufcs_intf_get_device_info(struct ufcs_dev *ufcs, u64 *dev_info)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (dev_info == NULL) {
		ufcs_err("dev_info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_device_info(class, dev_info);
}
EXPORT_SYMBOL(ufcs_intf_get_device_info);

int ufcs_get_error_info(struct ufcs_class *class, u64 *err_info)
{
	int rc;
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (err_info == NULL) {
		ufcs_err("err_info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_GET_ERROR_INFO;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push get error info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get error info error, rc=%d\n", rc);
		return rc;
	}

	*err_info = class->err_info;

	return 0;
}

int ufcs_intf_get_error_info(struct ufcs_dev *ufcs, u64 *err_info)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (err_info == NULL) {
		ufcs_err("err_info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_error_info(class, err_info);
}
EXPORT_SYMBOL(ufcs_intf_get_error_info);

int ufcs_get_source_info(struct ufcs_class *class, u64 *src_info)
{
	int rc;
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (src_info == NULL) {
		ufcs_err("src_info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_GET_SOURCE_INFO;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push get source info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get source info error, rc=%d\n", rc);
		return rc;
	}

	*src_info = class->src_info;

	return 0;
}

int ufcs_intf_get_source_info(struct ufcs_dev *ufcs, u64 *src_info)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (src_info == NULL) {
		ufcs_err("src_info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_source_info(class, src_info);
}
EXPORT_SYMBOL(ufcs_intf_get_source_info);

int ufcs_get_cable_info(struct ufcs_class *class, u64 *cable_info)
{
	int rc;
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (cable_info == NULL) {
		ufcs_err("cable_info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_START_CABLE_DETECT;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push get cable info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get cable info error, rc=%d\n", rc);
		return rc;
	}

	*cable_info = class->cable_info;

	return 0;
}

int ufcs_intf_get_cable_info(struct ufcs_dev *ufcs, u64 *cable_info)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (cable_info == NULL) {
		ufcs_err("cable_info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_cable_info(class, cable_info);
}
EXPORT_SYMBOL(ufcs_intf_get_cable_info);

int ufcs_get_pdo_info(struct ufcs_class *class, u64 *pdo, int num)
{
	int rc;
	struct ufcs_event *event;
	int i;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (pdo == NULL) {
		ufcs_err("pdo buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_GET_OUTPUT_CAP;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push get output cap event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get pdo info error, rc=%d\n", rc);
		return rc;
	}

	if (num < class->pdo.num)
		ufcs_err("pdo buf too short\n");
	else
		num = class->pdo.num;
	for (i = 0; i < num; i++)
		pdo[i] = class->pdo.data[i];

	return num;
}

int ufcs_intf_get_pdo_info(struct ufcs_dev *ufcs, u64 *pdo, int num)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (pdo == NULL) {
		ufcs_err("pdo buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_pdo_info(class, pdo, num);
}
EXPORT_SYMBOL(ufcs_intf_get_pdo_info);

int ufcs_verify_adapter(struct ufcs_class *class, u8 key_index, u8 *auth_data, u8 data_len)
{
	int rc;
	struct ufcs_event *event;
	struct ufcs_data_msg_verify_request *verify_request;
	int i;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (auth_data == NULL) {
		ufcs_err("auth_data is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}
	if (data_len != UFCS_VERIFY_AUTH_DATA_SIZE) {
		ufcs_err("data_len is not equal to %d\n", UFCS_VERIFY_AUTH_DATA_SIZE);
		return -EINVAL;
	}
	memcpy(class->verify_info.auth_data, auth_data, UFCS_VERIFY_AUTH_DATA_SIZE);

	event = devm_kzalloc(&class->ufcs->dev,
		sizeof(struct ufcs_data_msg_verify_request) + sizeof(struct ufcs_event),
		GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	verify_request = (struct ufcs_data_msg_verify_request *)((u8 *)event + sizeof(struct ufcs_event));
	event->data = verify_request;
	event->msg = NULL;
	event->type = UFCS_EVENT_SEND_VERIFY_REQUEST;
	INIT_LIST_HEAD(&event->list);
	verify_request->index = key_index;
	for (i = 0; i < UFCS_VERIFY_RANDOM_DATA_SIZE; i++)
		get_random_bytes(&verify_request->random_data[i], 1);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	class->verify_pass = false;
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push verify request event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("adapter verify error, rc=%d\n", rc);
		return rc;
	}

	return class->verify_pass;
}

int ufcs_intf_verify_adapter(struct ufcs_dev *ufcs, u8 key_index, u8 *auth_data, u8 data_len)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (auth_data == NULL) {
		ufcs_err("auth_data is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_verify_adapter(class, key_index, auth_data, data_len);
}
EXPORT_SYMBOL(ufcs_intf_verify_adapter);

int ufcs_get_power_change_info(struct ufcs_class *class, u32 *pwr_change_info, int num)
{
	int i;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (pwr_change_info == NULL) {
		ufcs_err("pwr_change_info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	if (class->pdo.num == 0) {
		ufcs_err("need get pdo info first\n");
		return -EINVAL;
	}

	if (num < class->pdo.num)
		ufcs_err("pwr_change_info buf too short\n");
	else
		num = class->pdo.num;
	for (i = 0; i < num; i++)
		pwr_change_info[i] = class->pwr_change_info[i];

	return num;
}

int ufcs_intf_get_power_change_info(struct ufcs_dev *ufcs, u32 *pwr_change_info, int num)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (pwr_change_info == NULL) {
		ufcs_err("pwr_change_info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_power_change_info(class, pwr_change_info, num);
}
EXPORT_SYMBOL(ufcs_intf_get_power_change_info);

int ufcs_get_emark_info(struct ufcs_class *class, u64 *info)
{
	int rc;
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (info == NULL) {
		ufcs_err("info buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_OPLUS_SEND_GET_EMARK_INFO_MSG;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push oplus get emark info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get emark info error, rc=%d\n", rc);
		return rc;
	}
	*info = class->emark_info;

	return 0;
}

int ufcs_intf_get_emark_info(struct ufcs_dev *ufcs, u64 *info)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (info == NULL) {
		ufcs_err("info buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_emark_info(class, info);
}
EXPORT_SYMBOL(ufcs_intf_get_emark_info);

int ufcs_get_power_info_ext(struct ufcs_class *class, u64 *pie, int num)
{
	int rc;
	struct ufcs_event *event;
	int i;

	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}
	if (pie == NULL) {
		ufcs_err("pie buf is NULL\n");
		return -EINVAL;
	}
	if (!class->handshake_success) {
		ufcs_err("ufcs no handshake\n");
		return -EINVAL;
	}

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return -ENOMEM;
	}

	event->data = NULL;
	event->msg = NULL;
	event->type = UFCS_EVENT_OPLUS_SEND_GET_POWER_INFO_MSG;
	INIT_LIST_HEAD(&event->list);

	mutex_lock(&class->ext_req_lock);
	mutex_lock(&class->pe_lock);
	reinit_completion(&class->request_ack);
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		mutex_unlock(&class->pe_lock);
		mutex_unlock(&class->ext_req_lock);
		ufcs_err("push oplus get power info event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
		return rc;
	}
	mutex_unlock(&class->pe_lock);
	wait_for_completion(&class->request_ack);
	rc = READ_ONCE(class->state.err);
	mutex_unlock(&class->ext_req_lock);

	if (rc < 0) {
		ufcs_err("get power info error, rc=%d\n", rc);
		return rc;
	}

	if (num < class->pie.num)
		ufcs_err("pie buf too short\n");
	else
		num = class->pie.num;
	for (i = 0; i < num; i++)
		pie[i] = class->pie.data[i];

	return num;
}

int ufcs_intf_get_power_info_ext(struct ufcs_dev *ufcs, u64 *pie, int num)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return -EINVAL;
	}
	class = ufcs->class;
	if (pie == NULL) {
		ufcs_err("pie buf is NULL\n");
		return -EINVAL;
	}

	if (class->test_mode) {
		ufcs_err("test mode, unable to process external request\n");
		return -EBUSY;
	}

	return ufcs_get_power_info_ext(class, pie, num);
}
EXPORT_SYMBOL(ufcs_intf_get_power_info_ext);

bool ufcs_is_test_mode(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return false;
	}
	class = ufcs->class;

	return class->test_mode;
}
EXPORT_SYMBOL(ufcs_is_test_mode);

bool ufcs_is_vol_acc_test_mode(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL) {
		ufcs_err("ufcs is NULL\n");
		return false;
	}
	class = ufcs->class;

	return class->vol_acc_test;
}
EXPORT_SYMBOL(ufcs_is_vol_acc_test_mode);
