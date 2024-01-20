#ifndef __OPLUS_CHG_COMM_H__
#define __OPLUS_CHG_COMM_H__

#include <oplus_mms.h>

#define FFC_CHG_STEP_MAX			4
#define AGAIN_FFC_CYCLY_THR_COUNT		2
#define BATT_TEMP_HYST				20
#define BATT_VOL_HYST				50
#define OPCHG_PWROFF_FORCE_UPDATE_BATT_TEMP	500
#define OPCHG_PWROFF_HIGH_BATT_TEMP		770
#define OPCHG_PWROFF_EMERGENCY_BATT_TEMP	850
#define FACTORY_PWROFF_HIGH_BATT_TEMP		650
#define FACTORY_PWROFF_EMERGENCY_BATT_TEMP	700
#define FFC_VOLT_COUNTS				4
#define FFC_CURRENT_COUNTS			2
#define SOFT_REST_VOL_THRESHOLD			4300
#define SOFT_REST_SOC_THRESHOLD			95
#define SOFT_REST_CHECK_DISCHG_MAX_CUR		200
#define SOFT_REST_RETRY_MAX_CNT			2
#define OPLUS_BMS_HEAT_THRE			200

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
	COMM_ITEM_CHG_SUB_BATT_FULL,
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
	COMM_ITEM_FFC_STEP,
	COMM_ITEM_SMOOTH_SOC,
	COMM_ITEM_CHG_CYCLE_STATUS,
	COMM_ITEM_SLOW_CHG,
	COMM_ITEM_SHUTDOWN_SOC,
};

/*
 * +--------+--------+--------+
 * | 31  24 | 23   8 | 7    0 |
 * +--------+--------+--------+
 *     |         |       |
 *     |         |       +------------> enable
 *     |         +--------------------> watt
 *     +------------------------------> pct
 */
#define SLOW_CHG_TO_PARAM(pct, watt, en) ((((pct) & 0xff) << 24) | (((watt) & 0xffff) << 8) | ((en) & 0xff))
#define SLOW_CHG_TO_PCT(param) (((param) >> 24) & 0xff)
#define SLOW_CHG_TO_WATT(param) (((param) >> 8) & 0xffff)
#define SLOW_CHG_TO_ENABLE(param) ((param) & 0xff)

enum oplus_chg_ffc_status {
	FFC_DEFAULT = 0,
	FFC_WAIT,
	FFC_FAST,
	FFC_IDLE,
};

enum aging_ffc_version {
	AGING_FFC_NOT_SUPPORT,
	AGING_FFC_V1,
	AGING_FFC_VERSION_MAX
};

typedef enum {
	CHG_CYCLE_VOTER__NONE		= 0,
	CHG_CYCLE_VOTER__ENGINEER	= (1 << 0),
	CHG_CYCLE_VOTER__USER		= (1 << 1),
}OPLUS_CHG_CYCLE_VOTER;

int oplus_comm_get_vbatt_over_threshold(struct oplus_mms *topic);
int oplus_comm_switch_ffc(struct oplus_mms *topic);
const char *oplus_comm_get_temp_region_str(enum oplus_temp_region temp_region);
int read_signed_data_from_node(struct device_node *node,
			       const char *prop_str,
			       s32 *addr, int len_max);
int read_unsigned_data_from_node(struct device_node *node,
				 const char *prop_str, u32 *addr,
				 int len_max);

int oplus_comm_get_wired_aging_ffc_offset(struct oplus_mms *topic, int step);
int oplus_comm_get_current_wired_ffc_cutoff_fv(struct oplus_mms *topic, int step);
int oplus_comm_get_wired_ffc_cutoff_fv(struct oplus_mms *topic, int step,
	enum oplus_ffc_temp_region temp_region);
int oplus_comm_get_wired_ffc_step_max(struct oplus_mms *topic);
int oplus_comm_get_wired_aging_ffc_version(struct oplus_mms *topic);
bool oplus_comm_get_boot_completed(void);
int oplus_comm_get_bms_heat_temp_compensation(struct oplus_mms *topic);
void oplus_comm_set_bms_heat_temp_compensation(struct oplus_mms *topic, int bms_heat_temp_compensation);
void oplus_comm_set_slow_chg(struct oplus_mms *topic, int pct, int watt, bool en);
#endif /* __OPLUS_CHG_COMM_H__ */
