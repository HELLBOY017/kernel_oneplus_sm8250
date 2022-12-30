/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _SYNAPTICS_REDREMOTE_H_
#define _SYNAPTICS_REDREMOTE_H_
struct remotepanel_data{
    struct i2c_client *client;
    struct input_dev *input_dev;
    //    struct input_dev *kpd;
    struct mutex *pmutex;
    int irq_gpio;
    unsigned int irq;
    int *enable_remote;
};
struct remotepanel_data *remote_alloc_panel_data(void);
int register_remote_device(struct remotepanel_data *pdata);
void unregister_remote_device(void);
#endif
