// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[EVENT]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "ufcs_core.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"
#include "ufcs_notify.h"

#define MSG_HEAD_SIZE		2
#define MSG_HEAD_TYPE_MASK	0x7
#define MSG_HEAD_VER_MASK	0x1f8
#define MSG_HEAD_INDEX_MASK	0x1e00
#define MSG_HEAD_ADDR_MASK	0xe000
#define MSG_HEAD_TYPE_INDEX	0
#define MSG_HEAD_VER_INDEX	3
#define MSG_HEAD_INDEX_INDEX	9
#define MSG_HEAD_ADDR_INDEX	13
#define MSG_DATA_INDEX		2

static const char *const ufcs_msg_addr_name[] = {
	[UFCS_DEVICE_SOURCE]	= "source",
	[UFCS_DEVICE_SINK]	= "sink",
	[UFCS_DEVICE_CABLE]	= "cable",
};

static const char *const ufcs_msg_type_name[] = {
	[UFCS_CTRL_MSG]		= "ctrl",
	[UFCS_DATA_MSG]		= "data",
	[UFCS_VENDOR_MSG]	= "vendor",
};

static const char *const ufcs_msg_version_name[] = {
	[UFCS_VER_1_0]		= "V1.0",
};

static const char *const ufcs_event_name[] = {
	[UFCS_EVENT_SEND_HW_RESET] =			"send_hard_reset",
	[UFCS_EVENT_SEND_PING] =			"send_ping",
	[UFCS_EVENT_SEND_ACK] =				"send_ack",
	[UFCS_EVENT_SEND_NCK] =				"send_nck",
	[UFCS_EVENT_SEND_ACCEPT] =			"send_accept",
	[UFCS_EVENT_SEND_SOFT_RESET] =			"send_soft_reset",
	[UFCS_EVENT_SEND_POWER_READY] =			"send_power_ready",
	[UFCS_EVENT_SEND_GET_OUTPUT_CAP] =		"send_get_output_cap",
	[UFCS_EVENT_SEND_GET_SOURCE_INFO] =		"send_get_source_info",
	[UFCS_EVENT_SEND_GET_SINK_INFO] =		"send_get_sink_info",
	[UFCS_EVENT_SEND_GET_DEVICE_INFO] =		"send_get_device_info",
	[UFCS_EVENT_SEND_GET_ERROR_INFO] =		"send_get_error_info",
	[UFCS_EVENT_SEND_DETECT_CABLE_INFO] =		"send_detect_cable_info",
	[UFCS_EVENT_SEND_START_CABLE_DETECT] =		"send_start_cable_detect",
	[UFCS_EVENT_SEND_END_CABLE_DETECT] =		"send_end_cable_detect",
	[UFCS_EVENT_SEND_EXIT_UFCS_MODE] =		"send_exit_ufcs_mode",
	[UFCS_EVENT_RECV_HW_RESET] =			"recv_hard_reset",
	[UFCS_EVENT_RECV_PING] =			"recv_ping",
	[UFCS_EVENT_RECV_ACK] =				"recv_ack",
	[UFCS_EVENT_RECV_NCK] =				"recv_nck",
	[UFCS_EVENT_RECV_ACCEPT] =			"recv_accept",
	[UFCS_EVENT_RECV_SOFT_RESET] =			"recv_soft_reset",
	[UFCS_EVENT_RECV_POWER_READY] =			"recv_power_ready",
	[UFCS_EVENT_RECV_GET_OUTPUT_CAP] =		"recv_get_output_cap",
	[UFCS_EVENT_RECV_GET_SOURCE_INFO] =		"recv_get_source_info",
	[UFCS_EVENT_RECV_GET_SINK_INFO] =		"recv_get_sink_info",
	[UFCS_EVENT_RECV_GET_DEVICE_INFO] =		"recv_get_device_info",
	[UFCS_EVENT_RECV_GET_ERROR_INFO] =		"recv_get_error_info",
	[UFCS_EVENT_RECV_DETECT_CABLE_INFO] =		"recv_detect_cable_inf",
	[UFCS_EVENT_RECV_START_CABLE_DETECT] =		"recv_start_cable_dete",
	[UFCS_EVENT_RECV_END_CABLE_DETECT] =		"recv_end_cable_detect",
	[UFCS_EVENT_RECV_EXIT_UFCS_MODE] =		"recv_exit_ufcs_mode",
	[UFCS_EVENT_RECV_VERIFY_REQUEST] =		"recv_verify_request",
	[UFCS_EVENT_RECV_VERIFY_RESPONSE] =		"recv_verify_response",
	[UFCS_EVENT_RECV_OUTPUT_CAP] =			"recv_output_cap",
	[UFCS_EVENT_RECV_REQUEST] =			"recv_request",
	[UFCS_EVENT_RECV_SOURCE_INFO] =			"recv_source_info",
	[UFCS_EVENT_RECV_SINK_INFO] =			"recv_sink_info",
	[UFCS_EVENT_RECV_DEVICE_INFO] =			"recv_device_info",
	[UFCS_EVENT_RECV_CABLE_INFO] =			"recv_cable_info",
	[UFCS_EVENT_RECV_ERROR_INFO] =			"recv_error_info",
	[UFCS_EVENT_RECV_CONFIG_WD] =			"recv_config_watchdog",
	[UFCS_EVENT_RECV_REFUSE] =			"recv_refuse",
	[UFCS_EVENT_RECV_POWER_CHANGE] =		"recv_power_change",
	[UFCS_EVENT_RECV_TEST_REQUEST] =		"recv_test_request",
	[UFCS_EVENT_SEND_VERIFY_REQUEST] =		"send_verify_request",
	[UFCS_EVENT_SEND_VERIFY_RESPONSE] =		"send_verify_response",
	[UFCS_EVENT_SEND_OUTPUT_CAP] =			"send_output_cap",
	[UFCS_EVENT_SEND_REQUEST] =			"send_request",
	[UFCS_EVENT_SEND_SOURCE_INFO] =			"send_source_info",
	[UFCS_EVENT_SEND_SINK_INFO] =			"send_sink_info",
	[UFCS_EVENT_SEND_DEVICE_INFO] =			"send_device_info",
	[UFCS_EVENT_SEND_CABLE_INFO] =			"send_cable_info",
	[UFCS_EVENT_SEND_ERROR_INFO] =			"send_error_info",
	[UFCS_EVENT_SEND_CONFIG_WD] =			"send_config_watchdog",
	[UFCS_EVENT_SEND_REFUSE] =			"send_refuse",
	[UFCS_EVENT_SEND_POWER_CHANGE] =		"send_power_change",
	[UFCS_EVENT_SEND_TEST_REQUEST] =		"send_test_request",

	[UFCS_EVENT_OPLUS_SEND_GET_EMARK_INFO_MSG] =	"oplus_send_get_emark_info",
	[UFCS_EVENT_OPLUS_SEND_GET_POWER_INFO_MSG] =	"oplus_send_get_power_info",
	[UFCS_EVENT_OPLUS_RECV_EMARK_INFO_MSG] =	"oplus_recv_emark_info",
	[UFCS_EVENT_OPLUS_RECV_POWER_INFO_MSG] =	"oplus_recv_power_info",

	[UFCS_EVENT_HANDSHAKE] =			"handshake",
	[UFCS_EVENT_TIMEOUT] =				"timeout",
};

const char *ufcs_get_msg_addr_name(enum ufcs_device_addr addr)
{
	if (addr < 0 || addr >= UFCS_DEVICE_INVALID)
		return "invalid";
	return ufcs_msg_addr_name[addr];
}

const char *ufcs_get_msg_type_name(enum ufcs_msg_type type)
{
	if (type < 0 || type >= UFCS_INVALID_MSG)
		return "invalid";
	return ufcs_msg_type_name[type];
}

const char *ufcs_get_msg_version_name(enum ufcs_version ver)
{
	if (ver < 0 || ver >= UFCS_VER_INVALID)
		return "invalid";
	return ufcs_msg_version_name[ver];
}

const char *ufcs_get_event_name(struct ufcs_event *event)
{
	if (event == NULL)
		return "NULL";
	if (event->type < 0 || event->type >= UFCS_EVENT_MAX)
		return "invalid";
	return ufcs_event_name[event->type];
}

int ufcs_dump_msg_info(struct ufcs_msg *msg, const char *prefix)
{
	const char *tmp;

	if (msg == NULL)
		return -EINVAL;

	tmp = prefix ? prefix : "";

	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		ufcs_info("%s %s ctrl msg\n", tmp, ufcs_get_ctrl_msg_name(msg->ctrl_msg.command));
		break;
	case UFCS_DATA_MSG:
		ufcs_info("%s %s data msg\n", tmp, ufcs_get_data_msg_name(msg->data_msg.command));
		break;
	case UFCS_VENDOR_MSG:
		ufcs_info("%s vendor msg\n", tmp);
		break;
	default:
		ufcs_err("%s Unknown msg, type=%u\n", tmp, msg->head.type);
		return -EINVAL;
	}

	return 0;
}

#define CRC_8_POLYNOMIAL 0x29 /* X8 + X5 + X3 + 1 */
static unsigned char crc8_calculate(const unsigned char *data, unsigned char size)
{
	unsigned char i;
	unsigned char crc = 0;

	while(size--) {
		crc ^= *data++;
		for (i = 8; i > 0; --i) {
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC_8_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

static struct ufcs_msg *ufcs_unpack_msg(struct ufcs_class *class, const u8 *buf, int len)
{
	struct ufcs_msg *msg;
	struct ufcs_config *config;
	unsigned char crc;
	u16 head;
	int msg_size;

	config = &class->config;
	if (config->check_crc)
		msg_size = MSG_HEAD_SIZE + 2;
	else
		msg_size = MSG_HEAD_SIZE + 1;
	if (len < msg_size) {
		ufcs_err("message length too short\n");
		return NULL;
	} else if (len > UFCS_MSG_SIZE_MAX) {
		ufcs_err("message length too long\n");
		return NULL;
	}

	msg = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_msg), GFP_KERNEL);
	if (msg == NULL) {
		ufcs_err("alloc ufcs msg buf error\n");
		return NULL;
	}

	msg->magic = UFCS_MSG_MAGIC;
	head = be16_to_cpu(*((u16 *)buf));
	msg->head.type = head & MSG_HEAD_TYPE_MASK;
	msg->head.version = head & MSG_HEAD_VER_MASK;
	msg->head.index = head & MSG_HEAD_INDEX_MASK;
	msg->head.addr = head & MSG_HEAD_ADDR_MASK;

	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		msg->ctrl_msg.command = buf[MSG_DATA_INDEX];
		if (config->check_crc) {
			msg->crc = buf[MSG_DATA_INDEX + 1];
			crc = crc8_calculate(buf, MSG_DATA_INDEX + 1);
		}
		break;
	case UFCS_DATA_MSG:
		msg->data_msg.command = buf[MSG_DATA_INDEX];
		msg->data_msg.length = buf[MSG_DATA_INDEX + 1];
		memcpy(msg->data_msg.data, &buf[MSG_DATA_INDEX + 2], msg->data_msg.length);
		ufcs_data_msg_init(&msg->data_msg);
		if (config->check_crc) {
			msg->crc = buf[MSG_DATA_INDEX + 2 + msg->data_msg.length];
			crc = crc8_calculate(buf, MSG_DATA_INDEX + 1 + msg->data_msg.length);
			if (msg->crc != crc) {
				ufcs_err("data message crc error\n");
				goto err;
			}
		}
		break;
	case UFCS_VENDOR_MSG:
		if (len < (MSG_HEAD_SIZE + 3)) {
			ufcs_err("vendor message length too short\n");
			goto err;
		}
		msg->vendor_msg.vnd_id = be16_to_cpu((*((u16 *)(buf + MSG_DATA_INDEX))));
		msg->vendor_msg.length = buf[MSG_DATA_INDEX + 2];
		if (config->check_crc)
			msg_size = MSG_HEAD_SIZE + 3 + msg->vendor_msg.length + 1;
		else
			msg_size = MSG_HEAD_SIZE + 3 + msg->vendor_msg.length;
		if (len < msg_size) {
			ufcs_err("vendor message length too short, len=%d, msg_size=%d\n", len, msg_size);
			goto err;
		}
		memcpy(msg->vendor_msg.data, &buf[MSG_DATA_INDEX + 3], msg->vendor_msg.length);
		if (!ufcs_is_supported_oplus_data_msg(&msg->vendor_msg))
			ufcs_err("not supported vendor msg, type=%d\n", msg->vendor_msg.vnd_msg.type);
		ufcs_oplus_data_msg_init(&msg->vendor_msg);
		if (config->check_crc) {
			msg->crc = buf[MSG_DATA_INDEX + 3 + msg->vendor_msg.length];
			crc = crc8_calculate(buf, MSG_DATA_INDEX + 2 + msg->vendor_msg.length);
		}
		break;
	default:
		ufcs_err("not support msg type, type=%d\n", msg->head.type);
		goto err;
	}

	if (config->check_crc && msg->crc != crc) {
		ufcs_err("%s message crc error\n", ufcs_get_msg_type_name(msg->head.type));
		msg->crc_good = 0;
	} else {
		msg->crc_good = 1;
	}
	return msg;

err:
	devm_kfree(&class->ufcs->dev, msg);
	return NULL;
}

static int ufcs_pack_msg(struct ufcs_class *class, struct ufcs_msg *msg, u8 *buf, int len)
{
	struct ufcs_config *config;
	int index = 0;
	unsigned char crc;
	u16 head;
	int msg_size;

	config = &class->config;
	if (config->check_crc)
		msg_size = MSG_HEAD_SIZE + 2;
	else
		msg_size = MSG_HEAD_SIZE + 1;
	if (len < msg_size) {
		ufcs_err("message length too short\n");
		return -EINVAL;
	}

	head = (msg->head.addr << MSG_HEAD_ADDR_INDEX) & MSG_HEAD_ADDR_MASK;
	head |= (msg->head.index << MSG_HEAD_INDEX_INDEX) & MSG_HEAD_INDEX_MASK;
	head |= (msg->head.version << MSG_HEAD_VER_INDEX) & MSG_HEAD_VER_MASK;
	head |= (msg->head.type << MSG_HEAD_TYPE_INDEX) & MSG_HEAD_TYPE_MASK;
	*((u16 *)buf) = cpu_to_be16(head);
	index += MSG_DATA_INDEX;

	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		buf[index++] = msg->ctrl_msg.command;
		break;
	case UFCS_DATA_MSG:
		msg_size += 2 + msg->data_msg.length;
		if (len < msg_size) {
			ufcs_err("data message length too short\n");
			return -EINVAL;
		}
		buf[index++] = msg->data_msg.command;
		buf[index++] = msg->data_msg.length;
		ufcs_data_msg_pack(&msg->data_msg);
		memcpy(&buf[index], msg->data_msg.data, msg->data_msg.length);
		index += msg->data_msg.length;
		break;
	case UFCS_VENDOR_MSG:
		if (!ufcs_is_supported_oplus_ctrl_msg(&msg->vendor_msg)) {
			ufcs_err("not supported vendor msg, type=%d\n", msg->vendor_msg.vnd_msg.type);
			return -ENOTSUPP;
		}
		msg_size += 3 + msg->vendor_msg.length;
		if (len < msg_size) {
			ufcs_err("vendor message length too short\n");
			return -EINVAL;
		}
		*((u16 *)(buf + index)) = cpu_to_be16(msg->vendor_msg.vnd_id);
		index += 2;
		buf[index++] = msg->vendor_msg.length;
		memcpy(&buf[index], msg->vendor_msg.data, msg->vendor_msg.length);
		index += msg->vendor_msg.length;
		break;
	default:
		ufcs_err("not support msg type, type=%d\n", msg->head.type);
		return -ENOTSUPP;
	}

	if (config->check_crc) {
		crc = crc8_calculate(buf, index);
		buf[index++] = crc;
	}
	return index;
}

static int __ufcs_send_msg(struct ufcs_class *class, struct ufcs_msg *msg)
{
	u8 buf[UFCS_MSG_SIZE_MAX];
	int rc;

	rc = ufcs_pack_msg(class, msg, buf, sizeof(buf));
	if (rc < 0) {
		ufcs_err("pack ufcs msg error, rc=%d\n", rc);
		return rc;
	}

	if (ufcs_log_level >= LOG_LEVEL_DEBUG)
		print_hex_dump(KERN_INFO, "UFCS[SEND BUF]: ", DUMP_PREFIX_OFFSET, 32, 1, buf, rc, false);

	rc = class->ufcs->ops->write_msg(class->ufcs, buf, rc);
	if (rc < 0) {
		ufcs_err("write ufcs msg error, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

int ufcs_send_msg(struct ufcs_class *class, struct ufcs_msg *msg)
{
	struct ufcs_msg_sender *sender = &class->sender;
	int rc;

	mutex_lock(&sender->lock);
	sender->msg = msg;
	sender->msg_retry_count = 0;
	reinit_completion(&sender->ack);
	kthread_queue_work(class->worker, &class->msg_send_work);
	wait_for_completion(&sender->ack);
	kthread_cancel_work_sync(&class->msg_send_work);
	sender->msg = NULL;
	if (sender->status == MSG_SEND_OK) {
		rc = 0;
	} else {
		ufcs_err("msg send error, sender status=%d\n", sender->status);
		if (sender->status == MSG_RETRY_OVER)
			rc = -EAGAIN;
		else
			rc = -EINVAL;
	}
	sender->status = MSG_SEND_PENDIGN;
	start_msg_trans_delay_timer(class);
	sender->msg_number_counter++;
	if (sender->msg_number_counter > MSG_NUMBER_COUNT_MAX)
		sender->msg_number_counter = 0;
	mutex_unlock(&sender->lock);

	return rc;
}

static struct ufcs_event *ufcs_alloc_no_data_event(struct ufcs_class *class,
	struct ufcs_msg *msg, enum ufcs_event_type type)
{
	struct ufcs_event *event;

	event = devm_kzalloc(&class->ufcs->dev, sizeof(struct ufcs_event),
		GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return NULL;
	}

	event->data = NULL;
	event->msg = msg;
	event->type = type;
	INIT_LIST_HEAD(&event->list);

	return event;
}

/**
 * ufcs_check_handshake() - Check if the current notification is a handshake notification.
 * @class: ufcs_class struct
 *
 * Return: Returns true if it is a handshake notification, otherwise returns false.
 */
static bool ufcs_check_handshake(struct ufcs_class *class)
{
	struct ufcs_event *event;
	int rc;

	if (class->state.curr != PE_STATE_HANDSHAKE ||
	    (class->ufcs->handshake_state != UFCS_HS_SUCCESS &&
	     class->ufcs->handshake_state != UFCS_HS_FAIL))
		return false;

	if (class->ufcs->handshake_state == UFCS_HS_FAIL) {
		class->ufcs->handshake_state = UFCS_HS_IDLE;
		stop_handshake_timer(class);
		event = ufcs_alloc_no_data_event(class, NULL, UFCS_EVENT_TIMEOUT);
		if (event == NULL) {
			ufcs_err("alloc event buf error\n");
			return true;
		}
		rc = ufcs_push_event(class, event);
		if (rc < 0) {
			ufcs_err("push handshake timeout event error, rc=%d\n", rc);
			devm_kfree(&class->ufcs->dev, event);
		}
		ufcs_info("hardware retry done, handshake fail, send timeout and exit\n");
		return true;
	} /*hardware retry support, exit ufcs*/

	class->ufcs->handshake_state = UFCS_HS_IDLE;
	event = ufcs_alloc_no_data_event(class, NULL, UFCS_EVENT_RECV_ACK);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		return true;
	}
	rc = ufcs_push_event(class, event);
	if (rc < 0) {
		ufcs_err("push handshake ack event error, rc=%d\n", rc);
		devm_kfree(&class->ufcs->dev, event);
	}

	return true;
}

static bool ufcs_check_error_info(struct ufcs_class *class)
{
	unsigned int dev_err_flag = class->ufcs->dev_err_flag;
	struct ufcs_msg_sender *sender;

	sender = &class->sender;
	ufcs_send_state(UFCS_NOTIFY_ERR_FLAG, &class->ufcs->dev_err_flag);

	if (dev_err_flag & BIT(UFCS_HW_ERR_HARD_RESET)) {
		if (class->start_cable_detect) {
			ufcs_send_state(UFCS_NOTIFY_CABLE_HW_RESET, NULL);
			ufcs_err("cable hard reset\n");
		} else {
			class->handshake_success = false;
			ufcs_send_state(UFCS_NOTIFY_SOURCE_HW_RESET, NULL);
			ufcs_err("source hard reset\n");
		}
		sender->status = MSG_RETRY_OVER;
		complete(&sender->ack);
		goto err;
	}
	if (dev_err_flag & BIT(UFCS_COMM_ERR_BUS_CONFLICT)) {
		ufcs_err("ufcs bus conflict\n");
		stop_ack_receive_timer(class);
		sender->status = MSG_RETRY_OVER;
		complete(&sender->ack);
		goto err;
	}

	if (dev_err_flag & BIT(UFCS_RECV_ERR_SENT_CMP)) {
		if (dev_err_flag & BIT(UFCS_RECV_ERR_TRANS_FAIL)) {
			ufcs_err("sent packet complete, but msg trans fail\n");
			if (sender->status == MSG_WAIT_ACK) {
				stop_ack_receive_timer(class);
				sender->status = MSG_RETRY_OVER;
				complete(&sender->ack);
				if ((dev_err_flag & BIT(UFCS_RECV_ERR_SENT_CMP)) &&
				    (dev_err_flag & BIT(UFCS_RECV_ERR_DATA_READY))) {
					ufcs_err("ack interrupt read delayed! need to read msg!\n");
					return 0;
				}
			}
			goto err;
		}
		if (dev_err_flag & BIT(UFCS_RECV_ERR_ACK_TIMEOUT))
			ufcs_err("sent packet complete, but ack receive timeout\n");
		if (dev_err_flag & BIT(UFCS_RECV_ERR_DATA_READY))
			ufcs_err("sent packet complete = 1 && data ready = 1\n");
	} else {
		if (!(dev_err_flag & BIT(UFCS_RECV_ERR_DATA_READY))) {
			ufcs_err("sent packet complete = 0 && data ready = 0\n");
			goto err;
		}
	}

	if (sender->status == MSG_WAIT_ACK) {
		stop_ack_receive_timer(class);
		sender->status = MSG_SEND_OK;
		complete(&sender->ack);
		if ((dev_err_flag & BIT(UFCS_RECV_ERR_SENT_CMP)) &&
		    (dev_err_flag & BIT(UFCS_RECV_ERR_DATA_READY)))
			ufcs_err("ack interrupt read delayed! need to read msg!\n");
		else
			return 1;
	}

	return 0;
err:
	return -EINVAL;
}

static void ufcs_recv_work(struct kthread_work *work)
{
	struct ufcs_class *class = container_of(work, struct ufcs_class, recv_work);
	struct ufcs_dev *ufcs = class->ufcs;
	struct ufcs_msg *msg;
	u8 buf[UFCS_MSG_SIZE_MAX];
	int rc;

	if (ufcs_check_handshake(class))
		return;
	rc = ufcs_check_error_info(class);
	if (rc != 0)
		return;

	rc = ufcs->ops->read_msg(ufcs, buf, sizeof(buf));
	if (rc < 0) {
		ufcs_err("read ufcs msg error, rc=%d\n", rc);
		return;
	}
	if (rc == 0) {
		ufcs_err("msg buf size is 0\n");
		return;
	}

	if (ufcs_log_level >= LOG_LEVEL_DEBUG)
		print_hex_dump(KERN_INFO, "UFCS[RECV BUF]: ", DUMP_PREFIX_OFFSET, 32, 1, buf, rc, false);

	msg = ufcs_unpack_msg(class, buf, rc);
	if (msg == NULL) {
		ufcs_err("ufcs_unpack_msg error\n");
		return;
	}
	ufcs_dump_msg_info(msg, "recv");

	if (msg->head.type == UFCS_CTRL_MSG && ufcs_is_ack_nck_msg(&msg->ctrl_msg)) {
		/*
		 * ACK messages of non-message reply types need to be handed
		 * over to the state machine for processing, such as ACK
		 * messages when UFCS recognition is successful.
		 */
		if (ufcs_recv_ack_nck_msg(class, &msg->ctrl_msg)) {
			devm_kfree(&ufcs->dev, msg);
			return;
		}
	} else if (!class->config.reply_ack) {
		/*
		 * For chips that support automatic sending of ack, we need to
		 * actively reset the msg_trans_delay timer after receiving a
		 * non-ACK/NCK message to ensure that the message sending interval
		 * meets the requirements.
		 */
		mutex_lock(&class->sender.lock);
		class->sender.status = MSG_SEND_PENDIGN;
		stop_msg_trans_delay_timer(class);
		start_msg_trans_delay_timer(class);
		mutex_unlock(&class->sender.lock);
	}

	usleep_range(T_ACK_TRANSMIT_US, T_ACK_TRANSMIT_US + 1);
	class->recv_msg = msg;
	kthread_queue_work(class->worker, &class->event_work);
}

static int ufcs_process_ctrl_msg_event(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_ctrl_msg *ctrl_msg;

	ctrl_msg = &event->msg->ctrl_msg;
	switch (ctrl_msg->command) {
	case CTRL_MSG_PING:
		event->type = UFCS_EVENT_RECV_PING;
		break;
	case CTRL_MSG_ACK:
		event->type = UFCS_EVENT_RECV_ACK;
		break;
	case CTRL_MSG_NCK:
		event->type = UFCS_EVENT_RECV_NCK;
		break;
	case CTRL_MSG_ACCEPT:
		event->type = UFCS_EVENT_RECV_ACCEPT;
		break;
	case CTRL_MSG_SOFT_RESET:
		event->type = UFCS_EVENT_RECV_SOFT_RESET;
		break;
	case CTRL_MSG_POWER_READY:
		event->type = UFCS_EVENT_RECV_POWER_READY;
		break;
	case CTRL_MSG_GET_SINK_INFO:
		event->type = UFCS_EVENT_RECV_GET_SINK_INFO;
		break;
	case CTRL_MSG_GET_DEVICE_INFO:
		event->type = UFCS_EVENT_RECV_GET_DEVICE_INFO;
		break;
	case CTRL_MSG_GET_ERROR_INFO:
		event->type = UFCS_EVENT_RECV_GET_ERROR_INFO;
		break;
	case CTRL_MSG_DETECT_CABLE_INFO:
		event->type = UFCS_EVENT_RECV_DETECT_CABLE_INFO;
		break;
	case CTRL_MSG_START_CABLE_DETECT:
		event->type = UFCS_EVENT_RECV_START_CABLE_DETECT;
		break;
	case CTRL_MSG_END_CABLE_DETECT:
		event->type = UFCS_EVENT_RECV_END_CABLE_DETECT;
		break;
	case CTRL_MSG_EXIT_UFCS_MODE:
		event->type = UFCS_EVENT_RECV_EXIT_UFCS_MODE;
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int ufcs_process_data_msg_event(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_data_msg *data_msg;

	data_msg = &event->msg->data_msg;
	switch (data_msg->command) {
	case DATA_MSG_OUTPUT_CAPABILITIES:
		event->type = UFCS_EVENT_RECV_OUTPUT_CAP;
		break;
	case DATA_MSG_REQUEST:
		event->type = UFCS_EVENT_RECV_REQUEST;
		break;
	case DATA_MSG_SOURCE_INFO:
		event->type = UFCS_EVENT_RECV_SOURCE_INFO;
		break;
	case DATA_MSG_SINK_INFO:
		event->type = UFCS_EVENT_RECV_SINK_INFO;
		break;
	case DATA_MSG_CABLE_INFO:
		event->type = UFCS_EVENT_RECV_CABLE_INFO;
		break;
	case DATA_MSG_DEVICE_INFO:
		event->type = UFCS_EVENT_RECV_DEVICE_INFO;
		break;
	case DATA_MSG_ERROR_INFO:
		event->type = UFCS_EVENT_RECV_ERROR_INFO;
		break;
	case DATA_MSG_CONFIG_WATCHDOG:
		event->type = UFCS_EVENT_RECV_CONFIG_WD;
		break;
	case DATA_MSG_REFUSE:
		event->type = UFCS_EVENT_RECV_REFUSE;
		break;
	case DATA_MSG_VERIFY_REQUEST:
		event->type = UFCS_EVENT_RECV_VERIFY_REQUEST;
		break;
	case DATA_MSG_VERIFY_RESPONSE:
		event->type = UFCS_EVENT_RECV_VERIFY_RESPONSE;
		break;
	case DATA_MSG_POWER_CHANGE:
		event->type = UFCS_EVENT_RECV_POWER_CHANGE;
		break;
	case DATA_MSG_TEST_REQUEST:
		event->type = UFCS_EVENT_RECV_TEST_REQUEST;
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int ufcs_process_vendor_msg_event(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_vendor_msg *vnd_msg;

	vnd_msg = &event->msg->vendor_msg;
	switch (vnd_msg->vnd_msg.type) {
	case OPLUS_MSG_RESP_EMARK_INFO:
		event->type = UFCS_EVENT_OPLUS_RECV_EMARK_INFO_MSG;
		break;
	case OPLUS_MSG_RESP_POWER_INFO_EXT:
		event->type = UFCS_EVENT_OPLUS_RECV_POWER_INFO_MSG;
		break;
	default:
		ufcs_err("received unsupported oplus msg, type=%u\n", vnd_msg->vnd_msg.type);
		return -ENOTSUPP;
	}

	return 0;
}

int ufcs_push_event(struct ufcs_class *class, struct ufcs_event *event)
{
	bool event_pending = false;

	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return -EINVAL;
	}
	if (event == NULL) {
		ufcs_err("event is NULL\n");
		return -EINVAL;
	}

	ufcs_debug("push %s event\n", ufcs_get_event_name(event));
	spin_lock(&class->event.event_list_lock);
	if (!list_empty(&class->event.event_list))
		event_pending = true;
	list_add_tail(&event->list, &class->event.event_list);
	spin_unlock(&class->event.event_list_lock);
	if (event_pending)
		ufcs_err("event pending\n");
	complete(&class->event.ack);

	return 0;
}

static int ufcs_process_event(struct ufcs_class *class, struct ufcs_event *event)
{
	struct ufcs_msg *msg;
	int rc;

	msg = event->msg;
	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		rc = ufcs_process_ctrl_msg_event(class, event);
		if (rc < 0)
			return rc;
		break;
	case UFCS_DATA_MSG:
		rc = ufcs_process_data_msg_event(class, event);
		if (rc < 0)
			return rc;
		break;
	case UFCS_VENDOR_MSG:
		rc = ufcs_process_vendor_msg_event(class, event);
		if (rc < 0)
			return rc;
		break;
	default:
		return -ENOTSUPP;
	}

	return ufcs_push_event(class, event);
}

static void ufcs_event_work(struct kthread_work *work)
{
	struct ufcs_class *class = container_of(work, struct ufcs_class, event_work);
	struct ufcs_dev *ufcs = class->ufcs;
	struct ufcs_event *event;
	struct ufcs_msg *msg;
	struct ufcs_config *config;

	config = &class->config;
	msg = class->recv_msg;
	if (msg == NULL)
		return;

	switch (msg->head.type) {
	case UFCS_CTRL_MSG:
		if (ufcs_is_ack_nck_msg(&msg->ctrl_msg))
			break;
		if (config->check_crc && !msg->crc_good) {
			ufcs_err("%s ctrl msg crc error\n",
				 ufcs_get_ctrl_msg_name(msg->ctrl_msg.command));
			ufcs_send_ctrl_msg_nck(class);
			goto crc_err;
		}
		break;
	case UFCS_DATA_MSG:
		if (config->check_crc && !msg->crc_good) {
			ufcs_err("%s data msg crc error\n",
				 ufcs_get_data_msg_name(msg->data_msg.command));
			ufcs_send_ctrl_msg_nck(class);
			goto crc_err;
		}
		break;
	case UFCS_VENDOR_MSG:
		if (config->check_crc && !msg->crc_good) {
			ufcs_err("vendor msg crc error, vnd_id=0x%x, type=%u\n",
				 msg->vendor_msg.vnd_id, msg->vendor_msg.vnd_msg.type);
			ufcs_send_ctrl_msg_nck(class);
			goto crc_err;
		}
		break;
	default:
		ufcs_err("not support msg type, type=%d\n", msg->head.type);
		ufcs_source_hard_reset(class->ufcs);
		goto msg_type_err;
	}
	if (config->reply_ack)
		ufcs_send_ctrl_msg_ack(class);

	event = devm_kzalloc(&ufcs->dev, sizeof(struct ufcs_event), GFP_KERNEL);
	if (event == NULL) {
		ufcs_err("alloc event buf error\n");
		goto event_err;
	}
	event->msg = msg;
	event->data = NULL;
	INIT_LIST_HEAD(&event->list);
	ufcs_process_event(class, event);

	return;

event_err:
msg_type_err:
crc_err:
	devm_kfree(&ufcs->dev, msg);
}

static void ufcs_msg_send_work(struct kthread_work *work)
{
	struct ufcs_class *class = container_of(work, struct ufcs_class, msg_send_work);
	struct ufcs_msg_sender *sender = &class->sender;
	bool ack_nck_msg;
	int rc;
	int retry_max;

	if (sender->msg == NULL) {
		ufcs_err("sender->msg is NULL");
		return;
	}

	if (sender->msg->head.type == UFCS_CTRL_MSG &&
	    (sender->msg->ctrl_msg.command == CTRL_MSG_ACK ||
	     sender->msg->ctrl_msg.command == CTRL_MSG_NCK))
		ack_nck_msg = true;
	else
		ack_nck_msg = false;

	if (!ack_nck_msg) {
		switch (sender->status) {
		case MSG_ACK_TIMEOUT:
		case MSG_NCK:
			sender->msg_retry_count++;
			break;
		case MSG_RETRY_OVER:
		case MSG_SEND_OK:
		case MSG_SEND_FAIL:
			complete(&sender->ack);
			return;
		case MSG_SEND_PENDIGN:
		case MSG_WAIT_ACK:
			return;
		case MSG_SEND_START:
			break;
		default:
			ufcs_err("unknown sender status, status=%d\n", sender->status);
			return;
		}
	}

	if (!class->config.msg_resend)
		retry_max = 1;
	else
		retry_max = MSG_RETRY_COUNT_MAX;

	if (sender->msg_retry_count >= retry_max) {
		stop_ack_receive_timer(class);
		sender->status = MSG_RETRY_OVER;
		complete(&sender->ack);
		return;
	}

	if (ufcs_dump_msg_info(sender->msg, "send") < 0)
		return;
	rc = __ufcs_send_msg(class, sender->msg);
	if (rc < 0) {
		ufcs_err("send msg error\n");
		sender->status = MSG_SEND_FAIL;
		complete(&sender->ack);
		return;
	}
	if (!ack_nck_msg) {
		sender->status = MSG_WAIT_ACK;
		start_ack_receive_timer(class);
	} else {
		sender->status = MSG_SEND_OK;
		complete(&sender->ack);
	}
}

static void ufcs_msg_sender_init(struct ufcs_msg_sender *sender)
{
	init_completion(&sender->ack);
	mutex_init(&sender->lock);
	sender->status = MSG_SEND_START;
	sender->msg_number_counter = 0;
	sender->msg_retry_count = 0;
}

static void ufcs_msg_sender_reset(struct ufcs_msg_sender *sender)
{
	sender->status = MSG_SEND_START;
	sender->msg_number_counter = 0;
	sender->msg_retry_count = 0;
}

int ufcs_event_init(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}

	kthread_init_work(&class->event_work, ufcs_event_work);
	kthread_init_work(&class->msg_send_work, ufcs_msg_send_work);
	kthread_init_work(&class->recv_work, ufcs_recv_work);
	class->recv_msg = NULL;

	spin_lock_init(&class->event.event_list_lock);
	INIT_LIST_HEAD(&class->event.event_list);
	class->event.exit = false;
	init_completion(&class->event.ack);

	ufcs_msg_sender_init(&class->sender);

	return 0;
}

int ufcs_event_reset(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("ufcs class is NULL\n");
		return -EINVAL;
	}

	class->recv_msg = NULL;
	class->event.exit = false;

	ufcs_msg_sender_reset(&class->sender);

	return 0;
}

void ufcs_free_event(struct ufcs_class *class, struct ufcs_event **event)
{
	struct ufcs_event *tmp;

	if (class == NULL || event == NULL || *event == NULL) {
		ufcs_err("class or event is NULL");
		return;
	}
	tmp = *event;

	spin_lock(&class->event.event_list_lock);
	/* here to prevent the event has been released by ufcs_free_all_event */
	if (!list_empty(&class->event.event_list) && !list_empty(&tmp->list))
		list_del_init(&tmp->list);
	spin_unlock(&class->event.event_list_lock);
	if (tmp->msg)
		devm_kfree(&class->ufcs->dev, tmp->msg);
	devm_kfree(&class->ufcs->dev, tmp);
	*event = NULL;
}

void ufcs_free_all_event(struct ufcs_class *class)
{
	struct list_head tmp_list;
	struct ufcs_event *event, *tmp;

	if (class == NULL) {
		ufcs_err("class is NULL");
		return;
	}

	spin_lock(&class->event.event_list_lock);
	if (list_empty(&class->event.event_list)) {
		spin_unlock(&class->event.event_list_lock);
		return;
	}
	list_replace_init(&class->event.event_list, &tmp_list);
	spin_unlock(&class->event.event_list_lock);

	list_for_each_entry_safe(event, tmp, &tmp_list, list) {
		list_del_init(&event->list);
		if (event->msg)
			devm_kfree(&class->ufcs->dev, event->msg);
		devm_kfree(&class->ufcs->dev, event);
	}
}

struct ufcs_event *ufcs_get_next_event(struct ufcs_class *class)
{
	struct ufcs_event *event;

	if (class == NULL) {
		ufcs_err("class is NULL");
		return NULL;
	}

recheck:
	spin_lock(&class->event.event_list_lock);
	if (list_empty(&class->event.event_list)) {
		/*
		 * completion must be re-initialized before exiting the atomic
		 * range, so that events sent before waiting for completion can
		 * be processed normally
		 */
		reinit_completion(&class->event.ack);
		spin_unlock(&class->event.event_list_lock);
		wait_for_completion(&class->event.ack);
		if (class->event.exit)
			return NULL;
		goto recheck;
	}
	event = list_first_entry(&class->event.event_list, struct ufcs_event, list);
	spin_unlock(&class->event.event_list_lock);

	ufcs_debug("get %s event\n", ufcs_get_event_name(event));

	return event;
}
