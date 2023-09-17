// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __OP_CHARGEPUMP_H__
#define __OP_CHARGEPUMP_H__

#define CP_STATUS_ADDR 0x04
#define CP_NOT_REEADY  0
#define CP_REEADY      BIT(0)
#define CP_DWP         BIT(1)
#define CP_OTP         BIT(2)
#define CP_SWITCH_OCP  BIT(3)
#define CP_CRP         BIT(4)
#define CP_VOUT_OVP    BIT(5)
#define CP_CLP         BIT(6)
#define CP_VBUS_OVP    BIT(7)

struct chip_chargepump {
	struct i2c_client *client;
	struct device *dev;
	struct pinctrl *pinctrl;
	int chargepump_en_gpio;
	struct pinctrl_state *chargepump_en_active;
	struct pinctrl_state *chargepump_en_sleep;
	struct pinctrl_state *chargepump_en_default;
	atomic_t chargepump_suspended;
	struct delayed_work watch_dog_work;
};

extern int chargepump_hw_init(void);
extern int chargepump_enable(void);
extern int chargepump_set_for_otg(char enable);
extern int chargepump_set_for_LDO(void);
extern int chargepump_disable(void);
void __chargepump_show_registers(void);
int chargepump_enable_dwp(struct chip_chargepump *chip, bool enable);
int chargepump_disable_dwp(void);
int chargepump_status_check(u8 *status);
int chargepump_i2c_test(void);
int chargepump_check_config(void);

#endif
