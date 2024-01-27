/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __GF_SPI_H
#define __GF_SPI_H

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
#ifdef FP_NAVIGATION_ENABLE
#define GF_KEY_INPUT_CAMERA		KEY_PRINT
#else
#define GF_KEY_INPUT_CAMERA		KEY_CAMERA
#endif

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
#define GF_IOC_ENABLE_SPI_CLK   _IOW(GF_IOC_MAGIC, 5, uint32_t)
#define GF_IOC_DISABLE_SPI_CLK  _IO(GF_IOC_MAGIC, 6)
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
#define GF_IOC_AUTO_SEND_TOUCHDOWN        _IO(GF_IOC_MAGIC, 21 )
#define GF_IOC_AUTO_SEND_TOUCHUP        _IO(GF_IOC_MAGIC, 22 )
#define GF_IOC_STOP_WAIT_INTERRUPT_EVENT _IO(GF_IOC_MAGIC, 23)

#define GF_IOC_CS_SET_VALUE    (_IOW(GF_IOC_MAGIC, 30, uint8_t))
#define GF_IOC_CS_PULL_DOWN    (_IOW(GF_IOC_MAGIC, 31, uint8_t))
#define GF_IOC_RST_PULL_DOWN    (_IOW(GF_IOC_MAGIC, 32, uint8_t))

#if defined(SUPPORT_NAV_EVENT)
#define GF_IOC_NAV_EVENT	_IOW(GF_IOC_MAGIC, 14, gf_nav_event_t)
#define  GF_IOC_MAXNR    15  /* THIS MACRO IS NOT USED NOW... */
#else
#define  GF_IOC_MAXNR    14  /* THIS MACRO IS NOT USED NOW... */
#endif

//#define AP_CONTROL_CLK       1
//#define  USE_PLATFORM_BUS     1
#define  USE_SPI_BUS	1
//#define GF_FASYNC   1	/*If support fasync mechanism.*/
#define GF_NETLINK_ENABLE 1
#define GF_NET_EVENT_FB_BLACK 2
#define GF_NET_EVENT_FB_UNBLACK 3
#define NETLINK_TEST 25

#ifdef CONFIG_REGULATOR_OPLUS_WL2868C_FP_LDO
#define BSP_GF_FP_EXT_PMIC
#endif

enum NETLINK_CMD {
    GF_NET_EVENT_TEST = 0,
    GF_NET_EVENT_IRQ = 1,
    GF_NET_EVENT_SCR_OFF,
    GF_NET_EVENT_SCR_ON,
    GF_NET_EVENT_TP_TOUCHDOWN,
    GF_NET_EVENT_TP_TOUCHUP,
    GF_NET_EVENT_UI_READY,
    GF_NET_EVENT_MAX,
};

struct gf_dev {
	dev_t devt;
	struct list_head device_entry;
#if defined(USE_SPI_BUS)
	struct spi_device *spi;
#elif defined(USE_PLATFORM_BUS)
	struct platform_device *spi;
#endif
	struct clk *core_clk;
	struct clk *iface_clk;

	struct input_dev *input;
	/* buffer is NULL unless this device is open (users > 0) */
	unsigned users;
	signed irq_gpio;
	signed reset_gpio;
        signed cs_gpio;
#ifdef CONGIG_MTK_P90M
    signed pw_en_gpio;
#endif
#ifdef CONFIG_MT6771_17331
	signed ldo_gpio;
#endif
	signed pwr_gpio;
	int irq;
	int irq_enabled;
	int clk_enabled;
#ifdef GF_FASYNC
	struct fasync_struct *async;
#endif
	struct notifier_block notifier;
	char device_available;
	char fb_black;
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pstate_spi_6mA;
	struct pinctrl_state *pstate_default;
	struct pinctrl_state *pstate_cs_func;
	struct pinctrl_state *pstate_irq_no_pull;
        struct pinctrl_state *pstate_cs_pull_down;
        struct pinctrl_state *pstate_rst_pull_down;
	bool cs_gpio_set;
	unsigned power_num;
	fp_power_info_t pwr_list[FP_MAX_PWR_LIST_LEN];
	bool is_optical;
	int gf_ext_pmic_flag;
	unsigned int gf_ext_pmic_ldo_num;
	unsigned int gf_ext_pmic_ldo_mv_max;
	unsigned int gf_ext_pmic_ldo_mv_min;
};

int gf_parse_dts(struct gf_dev* gf_dev);
void gf_cleanup(struct gf_dev *gf_dev);

int gf_power_on(struct gf_dev *gf_dev);
int gf_power_off(struct gf_dev *gf_dev);

int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms);
int gf_irq_num(struct gf_dev *gf_dev);
int gf_power_reset(struct gf_dev *gf_dev);

int gf_cs_pull_down(struct gf_dev *gf_dev);
int gf_rst_pull_down(struct gf_dev *gf_dev);

void sendnlmsg(char *msg);
int netlink_init(void);
void netlink_exit(void);
#endif /*__GF_SPI_H*/
