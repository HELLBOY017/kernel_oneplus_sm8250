// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "[CORE]([%s][%d]): " fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/version.h>

#include "ufcs_core.h"
#include "ufcs_timer.h"
#include "ufcs_event.h"
#include "ufcs_msg.h"
#include "ufcs_policy_engine.h"

int ufcs_log_level = LOG_LEVEL_INFO;
module_param(ufcs_log_level, int, 0644);
MODULE_PARM_DESC(ufcs_log_level, "ufcs log level");

static struct ufcs_dev *g_ufcs_dev;

static ssize_t pdo_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct ufcs_dev *ufcs = dev_get_drvdata(dev);
	struct ufcs_class *class;
	struct ufcs_pdo_info *pdo;
	int index = 0;
	int i;

	if (ufcs == NULL)
		return -ENODEV;
	class = ufcs->class;
	if (class == NULL)
		return -ENODEV;

	pdo = &class->pdo;
	if (pdo->num == 0)
		return 0;

	for (i = 0; i < pdo->num; i++) {
		if (index >= PAGE_SIZE)
			return index;
		index += snprintf(buf, PAGE_SIZE - index,
				  "pdo[%d]: voltage[%llu-%llumV, step:%llumV], current[%llu-%llumA, step:%llumA]", i,
				  UFCS_OUTPUT_MODE_VOL_MIN(pdo->data[i]), UFCS_OUTPUT_MODE_VOL_MAX(pdo->data[i]),
				  UFCS_OUTPUT_MODE_VOL_STEP(pdo->data[i]), UFCS_OUTPUT_MODE_CURR_MIN(pdo->data[i]),
				  UFCS_OUTPUT_MODE_CURR_MAX(pdo->data[i]), UFCS_OUTPUT_MODE_CURR_STEP(pdo->data[i]));
	}

	return index;
}
static DEVICE_ATTR_RO(pdo);

static ssize_t err_flag_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct ufcs_dev *ufcs = dev_get_drvdata(dev);

	if (ufcs == NULL)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "0x%x\n", ufcs->dev_err_flag);
}
static DEVICE_ATTR_RO(err_flag);

static struct device_attribute *ufcs_ic_attributes[] = {
	&dev_attr_pdo,
	&dev_attr_err_flag,
	NULL
};

static int ufcs_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

static struct attribute *ufcs_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(ufcs);

static struct class g_ufcs_class = {
	.name = "ufcs",
	.owner = THIS_MODULE,
	.dev_uevent = ufcs_uevent,
	.dev_groups = ufcs_groups,
};

static void ufcs_release(struct device *dev)
{
	struct ufcs_dev *ufcs = container_of(dev, struct ufcs_dev, dev);
	struct ufcs_class *class = ufcs->class;

	kfree(ufcs);
	kfree(class);
}

void ufcs_clean_process_info(struct ufcs_class *class)
{
	if (class == NULL) {
		ufcs_err("class is NULL\n");
		return;
	}

	memset(class->pwr_change_info, 0, sizeof(class->pwr_change_info));
	class->power_changed = false;

	class->src_info = 0;
	class->cable_info = 0;
	class->dev_info = 0;
	class->err_info = 0;
	class->handshake_success = false;
	class->pdo.num = 0;
	class->emark_info = 0;
	class->pie.num = 0;
	memset(class->pdo.data, 0, sizeof(class->pdo.data));
	memset(class->verify_info.random_data, 0, sizeof(class->verify_info.random_data));
	memset(class->verify_info.auth_data, 0, sizeof(class->verify_info.auth_data));
}

struct ufcs_dev *__must_check
ufcs_device_register(struct device *parent, struct ufcs_dev_ops *ops,
	void *drv_data, struct ufcs_config *config)
{
	struct ufcs_class *class;
	struct ufcs_dev *ufcs;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int rc;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct sched_param sp = {.sched_priority = MAX_RT_PRIO / 2 };
#endif

	if (parent == NULL) {
		ufcs_err("parent is NULL\n");
		return ERR_PTR(-ENODEV);
	}
	if (ops == NULL) {
		ufcs_err("ops is NULL\n");
		return ERR_PTR(-EINVAL);
	}
	if (drv_data == NULL) {
		ufcs_err("drv_data is NULL\n");
		return ERR_PTR(-EINVAL);
	}
	if (config == NULL) {
		ufcs_err("config is NULL\n");
		return ERR_PTR(-EINVAL);
	}

	class = kzalloc(sizeof(struct ufcs_class), GFP_KERNEL);
	if (class == NULL) {
		ufcs_err("alloc ufcs class memory error\n");
		return ERR_PTR(-ENOMEM);
	}
	ufcs = kzalloc(sizeof(struct ufcs_dev), GFP_KERNEL);
	if (ufcs == NULL) {
		ufcs_err("alloc ufcs device memory error\n");
		kfree(class);
		return ERR_PTR(-ENOMEM);
	}

	ufcs->ops = ops;
	ufcs->drv_data = drv_data;
	ufcs->class = class;
	class->ufcs = ufcs;
	memcpy(&class->config, config, sizeof(struct ufcs_config));

	device_initialize(&ufcs->dev);
	ufcs->dev.class = &g_ufcs_class;
	ufcs->dev.parent = parent;
	ufcs->dev.release = ufcs_release;
	dev_set_drvdata(&ufcs->dev, ufcs);

	rc = dev_set_name(&ufcs->dev, "ufcs");
	if (rc)
		goto free_ufcs;

	rc = device_init_wakeup(&ufcs->dev, true);
	if (rc)
		goto free_ufcs;

	rc = device_add(&ufcs->dev);
	if (rc)
		goto free_ufcs;

	attrs = ufcs_ic_attributes;
	while ((attr = *attrs++)) {
		rc = device_create_file(&ufcs->dev, attr);
		if (rc) {
			ufcs_err("device_create_file fail!\n");
			goto device_create_file_err;
		}
	}

	class->worker = kthread_create_worker(0, "ufcs");
	if (IS_ERR(class->worker)) {
		rc = -ENOMEM;
		goto del_device;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	sched_setscheduler(class->worker->task, SCHED_FIFO, &sp);
#else
	sched_set_fifo(class->worker->task);
#endif
	init_completion(&class->request_ack);
	mutex_init(&class->pe_lock);
	mutex_init(&class->handshake_lock);
	mutex_init(&class->ext_req_lock);

	rc = ufcs_timer_init(class);
	if (rc)
		goto timer_init_err;
	rc = ufcs_policy_engine_init(class);
	if (rc)
		goto policy_engine_init_err;
	rc = ufcs_event_init(class);
	if (rc)
		goto event_init_err;

	rc = ops->init(ufcs);
	if (rc < 0) {
		ufcs_err("ufcs device init error, rc=%d\n", rc);
		goto dev_init_err;
	}

	ufcs_info("ufcs device register success\n");
	g_ufcs_dev = ufcs;
	return ufcs;

dev_init_err:
event_init_err:
	kthread_stop(class->sm_task);
policy_engine_init_err:
timer_init_err:
	kthread_destroy_worker(class->worker);
del_device:
device_create_file_err:
	device_del(&ufcs->dev);
free_ufcs:
	put_device(&ufcs->dev);
	kfree(ufcs);
	kfree(class);
	return ERR_PTR(rc);
}
EXPORT_SYMBOL(ufcs_device_register);

void ufcs_device_unregister(struct ufcs_dev *ufcs)
{
	struct ufcs_class *class;

	if (ufcs == NULL)
		return;

	g_ufcs_dev = NULL;
	class = ufcs->class;
	complete_all(&class->event.ack);
	kthread_stop(class->sm_task);
	kthread_destroy_worker(class->worker);
	device_del(&ufcs->dev);
	put_device(&ufcs->dev);
	kfree(ufcs);
	kfree(class);
}
EXPORT_SYMBOL(ufcs_device_unregister);

struct ufcs_dev *ufcs_get_ufcs_device(void)
{
	return g_ufcs_dev;
}
EXPORT_SYMBOL(ufcs_get_ufcs_device);

static int __init ufcs_init(void)
{
	return class_register(&g_ufcs_class);
}
module_init(ufcs_init);

static void __exit ufcs_exit(void)
{
	class_unregister(&g_ufcs_class);
}
module_exit(ufcs_exit);

MODULE_DESCRIPTION("oplus UFCS class");
MODULE_LICENSE("GPL v2");
