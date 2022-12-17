/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __BQ2591X_HEADER__
#define __BQ2591X_HEADER__

/* Register 00h */
#define BQ2591X_REG_00      		0x00
#define	BQ2591X_VREG_MASK		0xFF
#define	BQ2591X_VREG_SHIFT		0
#define	BQ2591X_VREG_BASE		3500
#define	BQ2591X_VREG_LSB		5

/* Register 0x01*/
#define BQ2591X_REG_01             	0x01
#define BQ2591X_ICHG_MASK          	0x7F
#define BQ2591X_ICHG_SHIFT         	0
#define BQ2591X_ICHG_BASE          	0
#define BQ2591X_ICHG_LSB           	50

/* Register 02h */
#define BQ2591X_REG_02		    	0x02
#define BQ2591X_VINDPM_MASK		0x7F
#define BQ2591X_VINDPM_SHIFT		0
#define	BQ2591X_VINDPM_BASE		3900
#define	BQ2591X_VINDPM_LSB		100


/* Register 03h */
#define BQ2591X_REG_03		    	0x03
#define BQ2591X_IINLIM_MASK		0x3F
#define BQ2591X_IINLIM_SHIFT		0
#define BQ2591X_IINLIM_BASE		500
#define BQ2591X_IINLIM_LSB		100


/* Register 04h */
#define BQ2591X_REG_04		    	0x04
#define	BQ2591X_ITERM_MASK		0x03
#define	BQ2591X_ITERM_SHIFT		0
#define	BQ2591X_ITERM_500MA		0
#define	BQ2591X_ITERM_650MA		1
#define	BQ2591X_ITERM_800MA		2
#define	BQ2591X_ITERM_1000MA		3

/* Register 05h */
#define BQ2591X_REG_05		    	0x05
#define BQ2591X_EN_TERM_MASK		0x80
#define BQ2591X_EN_TERM_SHIFT		7
#define	BQ2591X_TERM_ENABLE		1
#define	BQ2591X_TERM_DISABLE		0

#define BQ2591X_WDT_RESET_MASK		0x40
#define BQ2591X_WDT_RESET_SHIFT    	6
#define BQ2591X_WDT_RESET          	1
#define BQ2591X_WDT_MASK		0x30
#define BQ2591X_WDT_SHIFT		4
#define BQ2591X_WDT_DISABLE		0
#define BQ2591X_WDT_40S			1
#define BQ2591X_WDT_80S			2
#define BQ2591X_WDT_160S		3
#define BQ2591X_WDT_BASE		0
#define BQ2591X_WDT_LSB			40
#define BQ2591X_EN_TIMER_MASK		0x08
#define BQ2591X_EN_TIMER_SHIFT		3
#define BQ2591X_CHG_TIMER_ENABLE	1
#define BQ2591X_CHG_TIMER_DISABLE	0

#define BQ2591X_CHG_TIMER_MASK		0x06
#define BQ2591X_CHG_TIMER_SHIFT		1
#define BQ2591X_CHG_TIMER_5HOURS	0
#define BQ2591X_CHG_TIMER_8HOURS	1
#define BQ2591X_CHG_TIMER_12HOURS	2
#define BQ2591X_CHG_TIMER_20HOURS	3
#define BQ2591X_TMR2X_EN_MASK		0x01
#define BQ2591X_TMR2X_EN_SHIFT		0
#define BQ2591X_TMR2X_ENABLE		1
#define BQ2591X_TMR2X_DISABLE		0

/* Register 06h */
#define BQ2591X_REG_06		    	0x06
#define BQ2591X_EN_CHG_MASK		0x08
#define	BQ2591X_EN_CHG_SHIFT		0x03
#define	BQ2591X_CHG_ENABLE		1
#define	BQ2591X_CHG_DISABLE		0
#define BQ2591X_VBATLOWV_MASK		0x03
#define	BQ2591X_VBATLOWV_SHIFT		0
#define	BQ2591X_VBATLOWV_2600MV		0
#define	BQ2591X_VBATLOWV_2900MV		1
#define	BQ2591X_VBATLOWV_3200MV		2
#define	BQ2591X_VBATLOWV_3500MV		3


#define	BQ2591X_REG_07			0x07
#define	BQ2591X_PG_STAT_MASK		0x80
#define	BQ2591X_IINDPM_STAT_MASK	0x40
#define	BQ2591X_VINDPM_STAT_MASK	0x20
#define	BQ2591X_TREG_STAT_MASK		0x10
#define	BQ2591X_WD_STAT_MASK		0x08
#define	BQ2591X_CHRG_STAT_MASK		0x07
#define	BQ2591X_CHRG_STAT_SHIFT		0
#define	BQ2591X_CHRG_STAT_NCHG		0
#define	BQ2591X_CHRG_STAT_FCHG		3
#define	BQ2591X_CHRG_STAT_TCHG		4


#define	BQ2591X_REG_08			0x08
#define	BQ2591X_VBUS_OVP_STAT_MASK	0x80
#define	BQ2591X_TSHUT_STAT_MASK		0x40
#define	BQ2591X_BATOVP_STAT_MASK	0x20
#define	BQ2591X_CFLY_STAT_MASK		0x10
#define	BQ2591X_TMR_STAT_MASK		0x08
#define	BQ2591X_CAP_COND_STAT_MASK	0x04
#define BQ2591X_POORSRC_STAT_MASK	0x02

#define	BQ2591X_REG_09			0x09
#define	BQ2591X_PG_FLAG_MASK		0x80
#define	BQ2591X_IINDPM_FLAG_MASK	0x40
#define	BQ2591X_VINDPM_FLAG_MASK	0x20
#define	BQ2591X_TREG_FLAG_MASK		0x10
#define	BQ2591X_WD_FLAG_MASK		0x08
#define	BQ2591X_CHRG_TERM_FLAG_MASK	0x04
#define	BQ2591X_CHRG_FLAG_MASK		0x01


#define	BQ2591X_REG_0A			0x0A
#define	BQ2591X_VBUS_OVP_FLAG_MASK	0x80
#define	BQ2591X_TSHUT_FLAG_MASK		0x40
#define	BQ2591X_BATOVP_FLAG_MASK	0x20
#define	BQ2591X_CFLY_FLAG_MASK		0x10
#define	BQ2591X_TMR_FLAG_MASK		0x08
#define	BQ2591X_CAP_COND_FLAG_MASK	0x04
#define BQ2591X_POORSRC_FLAG_MASK	0x02


#define	BQ2591X_REG_0B			0x0B
#define	BQ2591X_PG_MASK_MASK		0x80
#define	BQ2591X_IINDPM_MASK_MASK	0x40
#define	BQ2591X_VINDPM_MASK_MASK	0x20
#define	BQ2591X_TREG_MASK_MASK		0x10
#define	BQ2591X_WD_MASK_MASK		0x08
#define	BQ2591X_CHRG_TERM_MASK_MASK	0x04
#define	BQ2591X_CHRG_MASK_MASK		0x01

#define	BQ2591X_REG_0C			0x0C
#define	BQ2591X_VBUS_OVP_MASK_MASK	0x80
#define	BQ2591X_TSHUT_MASK_MASK		0x40
#define	BQ2591X_BATOVP_MASK_MASK	0x20
#define	BQ2591X_CFLY_MASK_MASK		0x10
#define	BQ2591X_TMR_MASK_MASK		0x08
#define	BQ2591X_CAP_COND_MASK_MASK	0x04
#define BQ2591X_POORSRC_MASK_MASK	0x02

/* Register 0D*/
#define BQ2591X_REG_0D			0x0D
#define BQ2591X_RESET_MASK		0x80             
#define BQ2591X_RESET_SHIFT		7
#define BQ2591X_RESET			1
#define BQ2591X_PN_MASK			0x78
#define BQ2591X_PN_SHIFT		3
#define BQ2591X_DEV_REV_MASK		0x03
#define BQ2591X_DEV_REV_SHIFT		0

extern int oplus_battery_meter_get_battery_voltage(void);
extern int oplus_get_rtc_ui_soc(void);
extern int oplus_set_rtc_ui_soc(int value);
extern int set_rtc_spare_fg_value(int val);
extern void mt_power_off(void);
extern bool pmic_chrdet_status(void);
extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
bool bq2591x_is_detected(void);
extern void oplus_bq2591x_dump_registers(void);
extern int oplus_bq2591x_hardware_init(void);
extern int oplus_bq2591x_charging_enable(void);
extern int oplus_bq2591x_set_ichg(int cur);
extern int oplus_bq2591x_charging_disable(void);
extern int oplus_bq2591x_charger_suspend(void);
extern int oplus_bq2591x_charger_unsuspend(void);
extern int oplus_bq2591x_set_cv(int mv);
#endif
