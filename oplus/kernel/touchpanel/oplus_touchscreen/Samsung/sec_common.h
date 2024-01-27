/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef SEC_H
#define SEC_H

/*********PART1:Head files**********************/
#include <linux/firmware.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/time.h>

#include "../touchpanel_common.h"
#include "../touchpanel_healthinfo.h"

#define Limit_ItemMagic     0x4F50504F
#define Limit_ItemMagic_V2  0x4F504C53

#define I2C_BURSTMAX                        (256)
#define GRIP_PARAMETER_LEN                  (128)
/*********PART2:Define Area**********************/
struct sec_testdata{
    int TX_NUM;
    int RX_NUM;
    int fd;
    int irq_gpio;
    uint64_t  TP_FW;
    const struct firmware *fw;
    uint64_t test_item;
};

struct sec_test_header {
    uint32_t magic1;
    uint32_t magic2;
    uint64_t test_item;
};

enum {
    LIMIT_TYPE_NO_DATA          = 0x00,     //means no limit data
    LIMIT_TYPE_CERTAIN_DATA     = 0x01,     //means all nodes limit data is a certain data
    LIMIT_TYPE_EACH_NODE_DATA   = 0x02,     //means all nodes have it's own limit
    LIMIT_TYPE_INVALID_DATA     = 0xFF,     //means wrong limit data type
};

//test item
enum {
    TYPE_ERROR                              = 0x00,
    TYPE_MUTUAL_RAW_OFFSET_DATA_SDC         = 0x01,
    TYPE_MUTUAL_RAW_DATA                    = 0x02,
    TYPE_SELF_RAW_OFFSET_DATA_SDC           = 0x03,
    TYPE_MUTU_RAW_NOI_P2P                   = 0x04,
    TYPE_MAX                                = 0xFF,
};

struct sec_test_item_header {
    uint32_t    item_magic;
    uint32_t    item_size;
    uint16_t    item_bit;
    uint16_t    item_limit_type;
    uint32_t    top_limit_offset;
    uint32_t    floor_limit_offset;
    uint32_t    para_num;
};

/*********PART3:Struct Area**********************/
struct sec_proc_operations {
    void    (*auto_test)    (struct seq_file *s, void *chip_data, struct sec_testdata *sec_testdata);
    void    (*verify_calibration)    (struct seq_file *s, void *chip_data);
    void    (*set_curved_rejsize)  (void *chip_data, uint8_t range_size);
    uint8_t (*get_curved_rejsize)  (void *chip_data);
    void    (*set_grip_handle)  (void *chip_data, int para_num, char *buf);
    void    (*set_kernel_grip_para)  (int para);
};

/*********PART4:function declare*****************/
int sec_create_proc(struct touchpanel_data *ts, struct sec_proc_operations *sec_ops);
void sec_flash_proc_init(struct touchpanel_data *ts, const char *name);
void sec_limit_read(struct seq_file *s, struct touchpanel_data *ts);
void sec_raw_device_init(struct touchpanel_data *ts);

#endif
