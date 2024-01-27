/* SPDX-License-Identifier: GPL-2.0-only  */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#ifndef __OPLUS_NU1619_H__
#define __OPLUS_NU1619_H__

#include <linux/mutex.h>
#include <linux/completion.h>
#include "../oplus_wireless.h"
#include "../oplus_chg_track.h"

/*Add for nu1619 update fw */
#define nu1619_DRIVER_NAME      "nu1619"
#define BOOT_AREA   0
#define RX_AREA     1
#define TX_AREA     2


#define GOOD                 0
#define STILL                1
#define LIMIT                2
#define EPP_MODE             1
#define BPP_MODE             0
#define MIN_VOUT            4000
#define MAX_VOUT            15000
#define AP_REV_DATA_OK       0xaa
#define AP_SENT_DATA_OK      0x55
#define PRIVATE_VBUCK_SET_CMD 0x80
#define PRIVATE_ID_CMD       0x86
#define PRIVATE_USB_TYPE_CMD 0x87
#define PRIVATE_FAST_CHG_CMD 0x88
#define PRIVATE_PRODUCT_TEST_CMD 0x8D
#define PRIVATE_TX_HW_ID_CMD 0x8b

/*0x000b[0:3]  0000:no charger, 0001:SDP, 0010:CDP, 0011:DCP, 0101:QC2-other,
* 0110:QC3-other, 0111:PD, 1000:fail charger, 1001:QC3-27W, 1010:PD-27W */
#define EPP_MODE_CURRENT 600000
#define DC_OTHER_CURRENT 800000
#define DC_LOW_CURRENT 200000
#define DC_SDP_CURRENT 800000
#define DC_DCP_CURRENT 800000
#define DC_CDP_CURRENT 800000
#define DC_QC2_CURRENT 1000000
#define DC_QC3_CURRENT 1000000
#define DC_QC3_20W_CURRENT 2000000
#define DC_PD_CURRENT      1000000
#define DC_PD_20W_CURRENT  2000000

/* used registers define */
#define REG_RX_SENT_CMD      0x0000
#define REG_RX_SENT_DATA1    0x0001
#define REG_RX_SENT_DATA2    0x0002
#define REG_RX_SENT_DATA3    0x0003
#define REG_RX_SENT_DATA4    0x0004
#define REG_RX_SENT_DATA5    0x0005
#define REG_RX_SENT_DATA6    0x0006
#define REG_RX_REV_CMD       0x0020
#define REG_RX_REV_DATA1     0x0021
#define REG_RX_REV_DATA2     0x0022
#define REG_RX_REV_DATA3     0x0023
#define REG_RX_REV_DATA4     0x0024

#define REG_CEP_VALUE        0x0008
#define REG_RX_VOUT          0x0009
#define REG_RX_VRECT         0x000a
#define REG_RX_IOUT          0x000b

#define REG_AP_RX_COMM       0x000C
/*******************************/


#define FASTCHG_CURR_MAX_UA         1500000
#define NU1619_FASTCHG_CURR_MAX	3600
#define NU1619_FASTCHG_CURR_MIDDLE	1800

#define P922X_REG_RTX_STATUS                                 0x78
#define P922X_RTX_READY                                     BIT(0)
#define P922X_RTX_DIGITALPING                               BIT(1)
#define P922X_RTX_ANALOGPING                                BIT(2)
#define P922X_RTX_TRANSFER                                  BIT(3)

#define P922X_REG_RTX_ERR_STATUS                            0x79
#define P922X_REG_RTX_ERR_TX_RXAC                           BIT(0)
#define P922X_REG_RTX_ERR_TX_OCP                            BIT(1)
#define P922X_REG_RTX_ERR_TX_OVP                            BIT(2)
#define P922X_REG_RTX_ERR_TX_LVP                            BIT(3)
#define P922X_REG_RTX_ERR_TX_FOD                            BIT(4)
#define P922X_REG_RTX_ERR_TX_OTP                            BIT(5)
#define P922X_REG_RTX_ERR_TX_CEPTIMEOUT                     BIT(6)
#define P922X_REG_RTX_ERR_TX_RXEPT                          BIT(7)

#define P922X_PRE2CC_CHG_THD_LO                             3400
#define P922X_PRE2CC_CHG_THD_HI                             3420

#define P922X_CC2CV_CHG_THD_LO                              4350
#define P922X_CC2CV_CHG_THD_HI                              4370

/*The message to WPC DOCK */
#define P9221_CMD_GET_RXTX_POWER							0x4A
#define P9221_CMD_INDENTIFY_ADAPTER							0xA1
#define P9221_CMD_INTO_FASTCHAGE							0xA2
#define P9221_CMD_GET_VENDOR_ID								0xA3
#define P9221_CMD_GET_EXTERN_CMD							0xA4
#define P9221_CMD_INTO_USB_CHARGE							0xA3
#define P9221_CMD_INTO_NORMAL_CHARGE						0xA4
#define P9221_CMD_GET_PRODUCT_ID							0xC4
#define P9221_CMD_SEND_BATT_TEMP_SOC						0xC5
#define P9221_CMD_SET_AES_DATA1								0xD1
#define P9221_CMD_SET_AES_DATA2								0xD2
#define P9221_CMD_SET_AES_DATA3								0xD3
#define P9221_CMD_SET_AES_DATA4								0xD4
#define P9221_CMD_SET_AES_DATA5								0xD5
#define P9221_CMD_SET_AES_DATA6								0xD6
#define P9221_CMD_GET_AES_DATA1								0xD7
#define P9221_CMD_GET_AES_DATA2								0xD8
#define P9221_CMD_GET_AES_DATA3								0xD9
#define P9221_CMD_GET_AES_DATA4								0xDA
#define P9221_CMD_GET_AES_DATA5								0xDB
#define P9221_CMD_GET_AES_DATA6								0xDC
#define P9237_RESPONE_VENDOR_ID								0xF3
#define P9237_RESPONE_EXTERN_CMD								0xF4
#define P9237_RESPONE_INTO_USB_CHARGE						0xF3
#define P9237_RESPONE_INTO_NORMAL_CHARGER					0xF4
#define P9221_CMD_RESPONE_SET_AES_DATA1						0x91
#define P9221_CMD_RESPONE_SET_AES_DATA2						0x92
#define P9221_CMD_RESPONE_SET_AES_DATA3						0x93
#define P9221_CMD_RESPONE_SET_AES_DATA4						0x94
#define P9221_CMD_RESPONE_SET_AES_DATA5						0x95
#define P9221_CMD_RESPONE_SET_AES_DATA6						0x96
#define P9221_CMD_RESPONE_GET_AES_DATA1						0x97
#define P9221_CMD_RESPONE_GET_AES_DATA2						0x98
#define P9221_CMD_RESPONE_GET_AES_DATA3						0x99
#define P9221_CMD_RESPONE_GET_AES_DATA4						0x9A
#define P9221_CMD_RESPONE_GET_AES_DATA5						0x9B
#define P9221_CMD_RESPONE_GET_AES_DATA6						0x9C
#define P9221_CMD_RESPONE_PRODUCT_ID						0x84
#define P9221_CMD_RESPONE_BATT_TEMP_SOC						0x85

#define P9221_CMD_SET_FAN_WORK								0xA5
#define P9221_CMD_SET_FAN_SILENT							0xA6
#define P9221_CMD_RESET_SYSTEM								0xA9
#define P9221_CMD_ENTER_TEST_MODE							0xAA
#define P9221_CMD_GET_FW_VERSION							0xAB
#define P9221_CMD_SET_LED_BRIGHTNESS						0xAC
#define P9221_CMD_SET_CEP_TIMEOUT							0xAD
#define P9221_CMD_SET_PWM_PULSE								0xAE
#define P9221_CMD_SEND_1ST_RANDOM_DATA						0xB1
#define P9221_CMD_SEND_2ND_RANDOM_DATA						0xB2
#define P9221_CMD_SEND_3RD_RANDOM_DATA						0xB3
#define P9221_CMD_GET_1ST_ENCODE_DATA						0xB4
#define P9221_CMD_GET_2ND_ENCODE_DATA						0xB5
#define P9221_CMD_GET_3RD_ENCODE_DATA						0xB6
#define P9221_CMD_NULL										0x00

/*The message from WPC DOCK */
#define P9237_RESPONE_RXTX_POWER							0x4A
#define P9237_RESPONE_ADAPTER_TYPE							0xF1
#define P9237_RESPONE_INTO_FASTCHAGE						0xF2
#define P9237_RESPONE_INTO_USB_CHARGE						0xF3
#define P9237_RESPONE_INTO_NORMAL_CHARGER					0xF4
#define P9237_RESPONE_FAN_WORK								0xF5
#define P9237_RESPONE_FAN_SILENT							0xF6
#define P9237_RESPONE_WORKING_IN_EPP						0xF8
#define P9237_RESPONE_RESET_SYSTEM							0xF9
#define P9237_RESPONE_ENTER_TEST_MODE						0xFA
#define P9237_RESPONE_FW_VERSION							0xFB
#define P9237_RESPONE_LED_BRIGHTNESS						0xFC
#define P9237_RESPONE_CEP_TIMEOUT							0xFD
#define P9237_RESPONE_PWM_PULSE								0xFE
#define P9237_RESPONE_RX_1ST_RANDOM_DATA					0xE1
#define P9237_RESPONE_RX_2ND_RANDOM_DATA					0xE2
#define P9237_RESPONE_RX_3RD_RANDOM_DATA					0xE3
#define P9237_RESPONE_SEND_1ST_DECODE_DATA					0xE4
#define P9237_RESPONE_SEND_2ND_DECODE_DATA					0xE5
#define P9237_RESPONE_SEND_3RD_DECODE_DATA					0xE6
#define P9237_RESPONE_NULL									0x00


#define P922X_TASK_INTERVAL							round_jiffies_relative(msecs_to_jiffies(500))
#define P922X_CEP_INTERVAL							round_jiffies_relative(msecs_to_jiffies(200))
#define P922X_UPDATE_INTERVAL							round_jiffies_relative(msecs_to_jiffies(3000))
#define P922X_UPDATE_RETRY_INTERVAL						round_jiffies_relative(msecs_to_jiffies(500))

#define NU1619_WPC_CHARGE_VOLTAGE_FASTCHG							15000
#define NU1619_WPC_CHARGE_VOLTAGE_FASTCHG_MIN						12000
#define NU1619_WPC_CHARGE_VOLTAGE_FASTCHG_MAX						15000

#define NU1619_VOOC_DEFAULT_MIN_TEMP						0
#define NU1619_VOOC_DEFAULT_MAX_TEMP						450
#define NU1619_SVOOC_DEFAULT_MIN_TEMP						-20
#define NU1619_SVOOC_DEFAULT_MAX_TEMP						530

#define CHG_CMD_DATA_LEN_V1		256
#define CHG_CMD_TIME_MS_V1		3000

struct oplus_chg_cmd_v1 {
	unsigned int cmd;
	unsigned int data_size;
	unsigned char data_buf[CHG_CMD_DATA_LEN_V1];
};

enum oplus_chg_cmd_type_v1{
	CMD_WLS_THIRD_PART_AUTH_V1,
	CMD_UPDATE_UI_SOH_V1,
	CMD_INIT_UI_SOH_V1,
};

enum oplus_chg_cmd_error_v1{
	CMD_ACK_OK_V1,
	CMD_ERROR_CHIP_NULL_V1,
	CMD_ERROR_DATA_NULL_V1,
	CMD_ERROR_DATA_INVALID_V1,
	CMD_ERROR_HIDL_NOT_READY_V1,
	CMD_ERROR_TIME_OUT_V1,
};

struct oplus_nu1619_ic{
	struct i2c_client *client;
	struct device *dev;

	struct notifier_block wls_aes_nb_v1;
	struct mutex update_data_ok;
	struct power_supply *wireless_psy;
	enum power_supply_type wireless_type;
	enum wireless_mode wireless_mode;
	bool disable_charge;
	int quiet_mode_need;
	int pre_quiet_mode_need;
	bool quiet_mode_ack;
	bool cep_timeout_ack;

	int idt_en_gpio;
	int idt_con_gpio;
	int idt_con_irq;
	int idt_int_gpio;
	int idt_int_irq;
	int vt_sleep_gpio;
	int vbat_en_gpio;
	int booster_en_gpio;
	int ext1_wired_otg_en_gpio;
	int ext2_wireless_otg_en_gpio;
	int ext3_wireless_otg_en_gpio;
	int cp_ldo_5v_gpio;
	int wrx_en_gpio;
	int wrx_otg_en_gpio;
	int wls_pg_gpio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *idt_con_active;
	struct pinctrl_state *idt_con_sleep;
	struct pinctrl_state *idt_con_default;
	struct pinctrl_state *idt_int_active;
	struct pinctrl_state *idt_int_sleep;
	struct pinctrl_state *idt_int_default;
	struct pinctrl_state *vt_sleep_active;
	struct pinctrl_state *vt_sleep_sleep;
	struct pinctrl_state *vt_sleep_default;
	struct pinctrl_state *vbat_en_active;
	struct pinctrl_state *vbat_en_sleep;
	struct pinctrl_state *vbat_en_default;
	struct pinctrl_state *booster_en_active;
	struct pinctrl_state *booster_en_sleep;
	struct pinctrl_state *booster_en_default;
	struct pinctrl_state *idt_con_out_active;
	struct pinctrl_state *idt_con_out_sleep;
	struct pinctrl_state *idt_con_out_default;

	struct pinctrl_state *ext1_wired_otg_en_active;
	struct pinctrl_state *ext1_wired_otg_en_sleep;
	struct pinctrl_state *ext1_wired_otg_en_default;
	struct pinctrl_state *ext2_wireless_otg_en_active;
	struct pinctrl_state *ext2_wireless_otg_en_sleep;
	struct pinctrl_state *ext2_wireless_otg_en_default;
	struct pinctrl_state *ext3_wireless_otg_en_active;
	struct pinctrl_state *ext3_wireless_otg_en_sleep;
	struct pinctrl_state *ext3_wireless_otg_en_default;
	struct pinctrl_state *cp_ldo_5v_active;
	struct pinctrl_state *cp_ldo_5v_sleep;
	struct pinctrl_state *cp_ldo_5v_default;
	struct pinctrl_state *wrx_en_active;
	struct pinctrl_state *wrx_en_sleep;
	struct pinctrl_state *wrx_en_default;
	struct pinctrl_state *wrx_otg_en_active;
	struct pinctrl_state *wrx_otg_en_sleep;
	struct pinctrl_state *wrx_otg_en_default;
	struct pinctrl_state *wls_pg_default;
	struct pinctrl_state *wls_pg_active;

	struct delayed_work idt_con_work;
	struct delayed_work nu1619_task_work;
	struct delayed_work nu1619_CEP_work;
	struct delayed_work nu1619_update_work;
	struct delayed_work nu1619_test_work;
	struct delayed_work idt_event_int_work;
	struct delayed_work idt_event_int_probe_work;
	struct delayed_work idt_connect_int_work;
	struct delayed_work idt_dischg_work;
	struct delayed_work nu1619_self_reset_work;
	struct delayed_work charger_suspend_work;
	struct delayed_work charger_disconnect_work;
	struct delayed_work charger_start_work;
	struct delayed_work wls_get_third_part_verity_data_work_v1;
	struct wpc_data nu1619_chg_status;
	oplus_chg_track_trigger trx_err_load_trigger;
	struct delayed_work trx_err_load_trigger_work;
	oplus_chg_track_trigger trx_info_load_trigger;
	struct delayed_work trx_info_load_trigger_work;
/*int         batt_volt_2cell_max;*/
/*int         batt_volt_2cell_min;*/
	atomic_t suspended;
	atomic_t volt_low_flag;
	const char *fw_boot_data;
	const char *fw_rx_data;
	const char *fw_tx_data;
	int fw_boot_lenth;
	int fw_rx_lenth;
	int fw_tx_lenth;
	u32 fw_boot_version;
	u32 fw_rx_version;
	u32 fw_tx_version;
	struct delayed_work status_keep_clean_work;
	struct wakeup_source *status_wake_lock;
	bool status_wake_lock_on;
	bool cmd_data_ok;
	bool hidl_handle_cmd_ready;
	struct mutex read_lock;
	struct mutex cmd_data_lock;
	struct mutex cmd_ack_lock;
	struct oplus_chg_cmd_v1 cmd;
	struct completion cmd_ack;
	wait_queue_head_t read_wq;
};

struct target_ichg_table {
	int vbat;			/*mv*/
	int target_ichg;	/*ma*/
	int input_current;	/*ma*/
};

bool nu1619_wpc_get_fast_charging(void);
bool nu1619_wpc_get_ffc_charging(void);
bool nu1619_wpc_get_normal_charging(void);
bool nu1619_wpc_get_otg_charging(void);
void nu1619_set_rtx_function_prepare(void);
void nu1619_set_rtx_function(bool is_on);
void nu1619_wpc_print_log(void);
void nu1619_set_vt_sleep_val(int value);
int nu1619_get_vt_sleep_val(void);
void nu1619_set_vbat_en_val(int value);
int nu1619_get_vbat_en_val(void);
void nu1619_set_booster_en_val(int value);
int nu1619_get_booster_en_val(void);
bool nu1619_wireless_charge_start(void);
void nu1619_set_wireless_charge_stop(void);
bool nu1619_firmware_is_updating(void);
void nu1619_set_ext1_wired_otg_en_val(int value);
int nu1619_get_ext1_wired_otg_en_val(void);
void nu1619_set_ext2_wireless_otg_en_val(int value);
void nu1619_set_ext3_wireless_otg_en_val(int value);
int nu1619_get_ext2_wireless_otg_en_val(void);
void nu1619_set_cp_ldo_5v_val(int value);
int nu1619_get_cp_ldo_5v_val(void);
int nu1619_wpc_get_adapter_type(void);
bool nu1619_check_chip_is_null(void);
int nu1619_set_tx_cep_timeout_1500ms(void);
int nu1619_get_CEP_flag(struct oplus_nu1619_ic * chip);
/*int nu1619_set_dock_fan_pwm_pulse(int pwm_pulse);*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
int nu1619_driver_init(void);
void nu1619_driver_exit(void);
#endif
ssize_t oplus_chg_send_mutual_cmd(char *buf);
ssize_t oplus_chg_response_mutual_cmd(const char *buf, size_t count);
#endif /*__OPLUS_NU1619_H__*/

