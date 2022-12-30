#ifndef __OPLUS_CHG_COMM_H__
#define __OPLUS_CHG_COMM_H__

#include <oplus_mms.h>
#ifdef CONFIG_OPLUS_CHG_DYNAMIC_CONFIG
#include <oplus_chg_cfg.h>
#endif

#define FFC_CHG_STEP_MAX			4
#define BATT_TEMP_HYST				20
#define BATT_VOL_HYST				50
#define OPCHG_PWROFF_FORCE_UPDATE_BATT_TEMP	500
#define OPCHG_PWROFF_HIGH_BATT_TEMP		770
#define OPCHG_PWROFF_EMERGENCY_BATT_TEMP	850
#define FFC_VOLT_COUNTS				4
#define FFC_CURRENT_COUNTS			2
#define SOFT_REST_VOL_THRESHOLD			4300
#define SOFT_REST_SOC_THRESHOLD			95
#define SOFT_REST_CHECK_DISCHG_MAX_CUR		200
#define SOFT_REST_RETRY_MAX_CNT			2

enum oplus_temp_region {
	TEMP_REGION_COLD = 0,
	TEMP_REGION_LITTLE_COLD,
	TEMP_REGION_COOL,
	TEMP_REGION_LITTLE_COOL,
	TEMP_REGION_PRE_NORMAL,
	TEMP_REGION_NORMAL,
	TEMP_REGION_WARM,
	TEMP_REGION_HOT,
	TEMP_REGION_MAX,
};

enum oplus_ffc_temp_region {
	FFC_TEMP_REGION_COOL,
	FFC_TEMP_REGION_PRE_NORMAL,
	FFC_TEMP_REGION_NORMAL,
	FFC_TEMP_REGION_WARM,
	FFC_TEMP_REGION_MAX,
};

enum oplus_fcc_gear {
	FCC_GEAR_LOW,
	FCC_GEAR_HIGH,
};

enum oplus_chg_full_type {
	CHG_FULL_SW,
	CHG_FULL_HW_BY_SW,
};

enum comm_topic_item {
	COMM_ITEM_TEMP_REGION,
	COMM_ITEM_FCC_GEAR,
	COMM_ITEM_CHG_FULL,
	COMM_ITEM_FFC_STATUS,
	COMM_ITEM_CHARGING_DISABLE,
	COMM_ITEM_CHARGE_SUSPEND,
	COMM_ITEM_COOL_DOWN,
	COMM_ITEM_BATT_STATUS,
	COMM_ITEM_BATT_HEALTH,
	COMM_ITEM_BATT_CHG_TYPE,
	COMM_ITEM_UI_SOC,
	COMM_ITEM_NOTIFY_CODE,
	COMM_ITEM_NOTIFY_FLAG,
	COMM_ITEM_SHELL_TEMP,
	COMM_ITEM_FACTORY_TEST,
	COMM_ITEM_LED_ON,
	COMM_ITEM_UNWAKELOCK,
	COMM_ITEM_POWER_SAVE,
	COMM_ITEM_RECHGING,
};

enum oplus_chg_ffc_status {
	FFC_DEFAULT = 0,
	FFC_WAIT,
	FFC_FAST,
	FFC_IDLE,
};

int oplus_comm_switch_ffc(struct oplus_mms *topic);
const char *oplus_comm_get_temp_region_str(enum oplus_temp_region temp_region);
int read_signed_data_from_node(struct device_node *node,
			       const char *prop_str,
			       s32 *addr, int len_max);
int read_unsigned_data_from_node(struct device_node *node,
				 const char *prop_str, u32 *addr,
				 int len_max);
#ifdef CONFIG_OPLUS_CHG_DYNAMIC_CONFIG
int oplus_comm_set_config(struct oplus_mms *topic, struct oplus_chg_param_head *param_head);
#endif

#endif /* __OPLUS_CHG_COMM_H__ */
