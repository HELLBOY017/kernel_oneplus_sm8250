#ifndef __OPLUS_SMART_CHG_H__
#define __OPLUS_SMART_CHG_H__

typedef enum {
	DOUBLE_SERIES_WOUND_CELLS = 0,
	SINGLE_CELL,
	DOUBLE_PARALLEL_WOUND_CELLS,
} SCC_CELL_TYPE;

typedef enum {
	TI_GAUGE = 0,
	SW_GAUGE,
	UNKNOWN_GAUGE_TYPE,
} SCC_GAUGE_TYPE;

#define DEVICE_BQ27541 0
#define DEVICE_BQ27411 1
#define DEVICE_BQ28Z610 2
#define DEVICE_ZY0602 3
#define DEVICE_ZY0603 4

#define BCC_TYPE_IS_SVOOC 1
#define BCC_TYPE_IS_VOOC 0

#define BCC_PARMS_COUNT 19
#define BCC_PARMS_COUNT_LEN   (BCC_PARMS_COUNT * sizeof(int))

int oplus_smart_chg_set_normal_current(int curr);
int oplus_smart_chg_set_normal_cool_down(int cool_down);
int oplus_smart_chg_get_normal_cool_down(void);
long oplus_smart_chg_get_quick_mode_time_gain(void);
int oplus_smart_chg_get_quick_mode_percent_gain(void);
int oplus_smart_chg_get_battery_bcc_parameters(char *buf);
int oplus_smart_chg_get_fastchg_battery_bcc_parameters(char *buf);
int oplus_smart_chg_get_prev_battery_bcc_parameters(char *buf);
int oplus_smart_chg_set_bcc_debug_parameters(const char *buf);
int oplus_smart_chg_get_soh_support(void);
#endif /* __OPLUS_SMART_CHG_H__ */
