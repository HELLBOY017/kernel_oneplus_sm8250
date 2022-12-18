/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __GF_SPI_H
#define __GF_SPI_H

#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include "../include/oplus_fp_common.h"

/**********************************************************/
enum FP_MODE{
	GF_IMAGE_MODE = 0,
	GF_KEY_MODE,
	GF_SLEEP_MODE,
	GF_FF_MODE,
	GF_DEBUG_MODE = 0x56
};

#define SUPPORT_NAV_EVENT

#if defined(SUPPORT_NAV_EVENT)
#define GF_NAV_INPUT_UP			KEY_UP
#define GF_NAV_INPUT_DOWN		KEY_DOWN
#define GF_NAV_INPUT_LEFT		KEY_LEFT
#define GF_NAV_INPUT_RIGHT		KEY_RIGHT
#define GF_NAV_INPUT_CLICK		KEY_VOLUMEDOWN
#define GF_NAV_INPUT_DOUBLE_CLICK	KEY_VOLUMEUP
#define GF_NAV_INPUT_LONG_PRESS		KEY_SEARCH
#define GF_NAV_INPUT_HEAVY		KEY_CHAT
#endif

#define GF_KEY_INPUT_HOME		KEY_HOME
#define GF_KEY_INPUT_MENU		KEY_MENU
#define GF_KEY_INPUT_BACK		KEY_BACK
#define GF_KEY_INPUT_POWER		KEY_POWER
#define GF_KEY_INPUT_CAMERA		KEY_CAMERA

#if defined(SUPPORT_NAV_EVENT)
typedef enum gf_nav_event {
	GF_NAV_NONE = 0,
	GF_NAV_FINGER_UP,
	GF_NAV_FINGER_DOWN,
	GF_NAV_UP,
	GF_NAV_DOWN,
	GF_NAV_LEFT,
	GF_NAV_RIGHT,
	GF_NAV_CLICK,
	GF_NAV_HEAVY,
	GF_NAV_LONG_PRESS,
	GF_NAV_DOUBLE_CLICK,
} gf_nav_event_t;
#endif

typedef enum gf_key_event {
	GF_KEY_NONE = 0,
	GF_KEY_HOME,
	GF_KEY_POWER,
	GF_KEY_MENU,
	GF_KEY_BACK,
	GF_KEY_CAMERA,
} gf_key_event_t;

struct gf_key {
	enum gf_key_event key;
	uint32_t value;   /* key down = 1, key up = 0 */
};

struct gf_key_map {
	unsigned int type;
	unsigned int code;
};

struct gf_ioc_chip_info {
	unsigned char vendor_id;
	unsigned char mode;
	unsigned char operation;
	unsigned char reserved[5];
};

#define GF_IOC_MAGIC    'g'     //define magic number
#define GF_IOC_INIT             _IOR(GF_IOC_MAGIC, 0, uint8_t)
#define GF_IOC_EXIT             _IO(GF_IOC_MAGIC, 1)
#define GF_IOC_RESET            _IO(GF_IOC_MAGIC, 2)
#define GF_IOC_ENABLE_IRQ       _IO(GF_IOC_MAGIC, 3)
#define GF_IOC_DISABLE_IRQ      _IO(GF_IOC_MAGIC, 4)
#define GF_IOC_ENABLE_POWER     _IO(GF_IOC_MAGIC, 7)
#define GF_IOC_DISABLE_POWER    _IO(GF_IOC_MAGIC, 8)
#define GF_IOC_INPUT_KEY_EVENT  _IOW(GF_IOC_MAGIC, 9, struct gf_key)
#define GF_IOC_ENTER_SLEEP_MODE _IO(GF_IOC_MAGIC, 10)
#define GF_IOC_GET_FW_INFO      _IOR(GF_IOC_MAGIC, 11, uint8_t)
#define GF_IOC_REMOVE           _IO(GF_IOC_MAGIC, 12)
#define GF_IOC_CHIP_INFO        _IOW(GF_IOC_MAGIC, 13, struct gf_ioc_chip_info)
#define GF_IOC_POWER_RESET      _IO(GF_IOC_MAGIC, 17)
#define GF_IOC_WAKELOCK_TIMEOUT_ENABLE        _IO(GF_IOC_MAGIC, 18 )
#define GF_IOC_WAKELOCK_TIMEOUT_DISABLE        _IO(GF_IOC_MAGIC, 19 )
#define GF_IOC_CLEAN_TOUCH_FLAG        _IO(GF_IOC_MAGIC, 20 )
#define GF_IOC_AUTO_SEND_TOUCHDOWN        _IO(GF_IOC_MAGIC, 21)
#define GF_IOC_AUTO_SEND_TOUCHUP        _IO(GF_IOC_MAGIC, 22)
#define GF_IOC_STOP_WAIT_INTERRUPT_EVENT _IO(GF_IOC_MAGIC, 23)

#if defined(SUPPORT_NAV_EVENT)
#define GF_IOC_NAV_EVENT	_IOW(GF_IOC_MAGIC, 14, gf_nav_event_t)
#define  GF_IOC_MAXNR    15  /* THIS MACRO IS NOT USED NOW... */
#else
#define  GF_IOC_MAXNR    14  /* THIS MACRO IS NOT USED NOW... */
#endif

#define  USE_PLATFORM_BUS     1
#define GF_NETLINK_ENABLE 1
#define GF_NET_EVENT_FB_BLACK 2
#define GF_NET_EVENT_FB_UNBLACK 3
#define NETLINK_TEST 25
#define MAX_MSGSIZE 32

enum NETLINK_CMD {
    GF_NET_EVENT_TEST = 0,
    GF_NET_EVENT_IRQ = 1,
    GF_NET_EVENT_SCR_OFF,
    GF_NET_EVENT_SCR_ON,
    GF_NET_EVENT_TP_TOUCHDOWN,
    GF_NET_EVENT_TP_TOUCHUP,
    GF_NET_EVENT_UI_READY,
    GF_NET_EVENT_UI_DISAPPEAR,
    GF_NET_EVENT_MAX,
};


struct gf_dev {
	dev_t devt;
	struct list_head device_entry;
	struct platform_device *spi;
	struct clk *core_clk;
	struct clk *iface_clk;

	struct input_dev *input;
	/* buffer is NULL unless this device is open (users > 0) */
	unsigned users;
	signed irq_gpio;
	signed reset_gpio;
	signed pwr_gpio;
	int irq;
	int irq_enabled;
	int clk_enabled;
	struct notifier_block notifier;
	char device_available;
	char fb_black;

    /* jinrong add for power */
    unsigned power_num;
    fp_power_info_t pwr_list[FP_MAX_PWR_LIST_LEN];
    uint32_t notify_tpinfo_flag;
    uint32_t ftm_poweroff_flag;
};

static inline int vreg_setup(struct gf_dev *goodix_fp, fp_power_info_t *pwr_info,
        bool enable)
{
    int rc;
    struct regulator *vreg;
    struct device *dev = &goodix_fp->spi->dev;
    const char *name = NULL;

    if (NULL == pwr_info || NULL == pwr_info->vreg_config.name) {
        return -EINVAL;
    }
    vreg = pwr_info->vreg;
    name = pwr_info->vreg_config.name;
    if (enable) {
        if (!vreg) {
            vreg = regulator_get(dev, name);
            if (IS_ERR(vreg)) {
                return PTR_ERR(vreg);
            }
        }
        if (regulator_count_voltages(vreg) > 0) {
            rc = regulator_set_voltage(vreg, pwr_info->vreg_config.vmin,
                    pwr_info->vreg_config.vmax);
            if (rc) {
            }
        }
        rc = regulator_set_load(vreg, pwr_info->vreg_config.ua_load);
        if (rc < 0) {
        }
        rc = regulator_enable(vreg);
        if (rc) {
            regulator_put(vreg);
            vreg = NULL;
        }
        pwr_info->vreg = vreg;
    } else {
        if (vreg) {
            if (regulator_is_enabled(vreg)) {
                regulator_disable(vreg);
            }
            regulator_put(vreg);
            pwr_info->vreg = NULL;
        } else {
        }
        rc = 0;
    }
    return rc;
}

static inline void gf_cleanup_ftm_poweroff_flag(struct gf_dev* gf_dev) {
    gf_dev->ftm_poweroff_flag = 0;
    pr_err("%s cleanup", __func__);
}


static inline void gf_cleanup_pwr_list(struct gf_dev* gf_dev) {
    unsigned index = 0;
    for (index = 0; index < gf_dev->power_num; index++) {
        if (gf_dev->pwr_list[index].pwr_type == FP_POWER_MODE_GPIO) {
            gpio_free(gf_dev->pwr_list[index].pwr_gpio);
        }
        memset(&(gf_dev->pwr_list[index]), 0, sizeof(fp_power_info_t));
    }
}

static inline int gf_parse_dts(struct gf_dev* gf_dev);
static inline void gf_cleanup(struct gf_dev *gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }

    gf_cleanup_pwr_list(gf_dev);
}

static inline int gf_power_on(struct gf_dev* gf_dev)
{
    int rc = 0;
    unsigned index = 0;

    for (index = 0; index < gf_dev->power_num; index++) {
        switch (gf_dev->pwr_list[index].pwr_type) {
        case FP_POWER_MODE_LDO:
            rc = vreg_setup(gf_dev, &(gf_dev->pwr_list[index]), true);
            break;
        case FP_POWER_MODE_GPIO:
            gpio_set_value(gf_dev->pwr_list[index].pwr_gpio, gf_dev->pwr_list[index].poweron_level);
            break;
        case FP_POWER_MODE_AUTO:
            break;
        case FP_POWER_MODE_NOT_SET:
        default:
            rc = -1;
            break;
        }
        if (rc) {
            break;
        } else {
        }
        msleep(gf_dev->pwr_list[index].delay);
    }

    msleep(30);
    return rc;
}

static inline int gf_power_off(struct gf_dev* gf_dev)
{
    int rc = 0;
    unsigned index = 0;

    for (index = 0; index < gf_dev->power_num; index++) {
        switch (gf_dev->pwr_list[index].pwr_type) {
        case FP_POWER_MODE_LDO:
            rc = vreg_setup(gf_dev, &(gf_dev->pwr_list[index]), false);
            break;
        case FP_POWER_MODE_GPIO:
            gpio_set_value(gf_dev->pwr_list[index].pwr_gpio, (gf_dev->pwr_list[index].poweron_level == 0 ? 1 : 0));
            break;
        case FP_POWER_MODE_AUTO:
            break;
        case FP_POWER_MODE_NOT_SET:
        default:
            rc = -1;
            break;
        }
        if (rc) {
            break;
        }
    }

    msleep(30);
    return rc;
}

static inline int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms);
static inline int gf_power_reset(struct gf_dev *gf_dev);

static inline void sendnlmsg(char *msg);
static inline int netlink_init(void);
static inline void netlink_exit(void);

static inline int gf_parse_pwr_list(struct gf_dev* gf_dev);
static inline int gf_parse_ftm_poweroff_flag(struct gf_dev* gf_dev) {
    int ret = 0;
    struct device *dev = &gf_dev->spi->dev;
    struct device_node *np = dev->of_node;
    uint32_t ftm_poweroff_flag = 0;

    gf_cleanup_ftm_poweroff_flag(gf_dev);

    ret = of_property_read_u32(np, FP_FTM_POWEROFF_FLAG, &ftm_poweroff_flag);
    if (ret) {
        pr_err("failed to request %s, ret = %d\n", FP_FTM_POWEROFF_FLAG, ret);
        goto exit;
    }
    gf_dev->ftm_poweroff_flag = ftm_poweroff_flag;
    pr_err("gf_dev->ftm_poweroff_flag = %d\n", gf_dev->ftm_poweroff_flag);

exit:
    if (ret) {
        gf_cleanup_ftm_poweroff_flag(gf_dev);
    }
    return ret;
}

#endif /*__GF_SPI_H*/
