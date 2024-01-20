#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/power_supply.h>
#include <linux/iio/consumer.h>
#include <linux/rtc.h>
#include <linux/pm_wakeup.h>

#include "../oplus_charger.h"
#include "../oplus_chg_track.h"
#include "../oplus_gauge.h"
#include "../oplus_vooc.h"
#include "../oplus_short.h"
#include "../oplus_wireless.h"
#include "../oplus_adapter.h"
#ifdef CONFIG_OPLUS_CHARGER_MTK
#include <mt-plat/mtk_boot_common.h>
#include <tcpm.h>
#include <tcpci.h>
#endif
#include "../charger_ic/oplus_short_ic.h"
#include "../gauge_ic/oplus_bq27541.h"
#include <../../system/include/boot_mode.h>
#include "../oplus_chg_ops_manager.h"
#include "../voocphy/oplus_voocphy.h"

#include "oplus_sy6974b.h"

struct chip_sy6974b {
	struct device		*dev;
	struct i2c_client	*client;
	int			sy6974b_irq_gpio;
	enum power_supply_type	oplus_charger_type;
	atomic_t		driver_suspended;
	atomic_t		charger_suspended;
	struct mutex		i2c_lock;
	struct mutex		dpdm_lock;
	bool charge_enabled;
	const char *chg_dev_name;
	struct charger_device *chg_dev;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct tcpc_device *tcpc;
	struct charger_consumer *chg_consumer;
	struct notifier_block pd_nb;
	struct regulator	*dpdm_reg;
	bool			dpdm_enabled;
	bool			power_good;
	bool			is_sy6974b;
	bool			is_bq25601d;
	bool			bc12_done;
	char			bc12_delay_cnt;
	char			bc12_retried;
	int			hw_aicl_point;
	int			sw_aicl_point;
	int			reg_access;
	int			before_suspend_icl;
	int			before_unsuspend_icl;
	int			normal_init_delay_ms;
	int			other_init_delay_ms;

	struct wakeup_source *suspend_ws;
	/*fix chgtype identify error*/
	struct wakeup_source *keep_resume_ws;
	wait_queue_head_t wait;

	struct delayed_work init_work;

	int typec_port;
	bool otg_online;
	bool otg_present;
	bool real_suspend_6974b;
	bool batfet_reset_disable;
	struct delayed_work	bc12_retry_work;
};

enum {
	CHARGER_NORMAL_CHG_CURVE,
	CHARGER_FASTCHG_VOOC_AND_QCPD_CURVE,
	CHARGER_FASTCHG_SVOOC_CURVE,
};

static struct chip_sy6974b *charger_ic = NULL;
static int aicl_result = 500;

#define OPLUS_BC12_RETRY_CNT 1
#define OPLUS_BC12_DELAY_CNT 18
#define INIT_WORK_NORMAL_DELAY 1500
#define INIT_WORK_OTHER_DELAY 1000

static bool dumpreg_by_irq = 0;
static int sy6974b_debug = 0;
#define ENABLE_DUMP_LOG BIT(0)
module_param(sy6974b_debug, int, 0644);
MODULE_PARM_DESC(sy6974b_debug, "debug sy6974b");

static int sy6974b_request_dpdm(struct chip_sy6974b *chip, bool enable);
static void sy6974b_get_bc12(struct chip_sy6974b *chip);
void sy6974b_really_suspend_charger(bool en);
static void oplus_chg_wakelock(struct chip_sy6974b *chip, bool awake);
#ifdef CONFIG_OPLUS_CHARGER_MTK
static int sy6974b_inform_charger_type(struct chip_sy6974b *chip);
static const struct charger_properties sy6974b_chg_props = {
	.alias_name = "sy6974b",
};
void oplus_notify_device_mode(bool enable)
{
}
#endif
#define I2C_RETRY_DELAY_US	5000
#define I2C_RETRY_WRITE_MAX_COUNT	3
#define I2C_RETRY_READ_MAX_COUNT	20
#ifdef CONFIG_OPLUS_CHARGER_MTK
#define CDP_TIMEOUT	30
static bool hw_bc12_init(void)
{
	int timeout = CDP_TIMEOUT;
	static bool first_connect = true;
	if (first_connect == true) {
		/* add make sure USB Ready */
		if (is_usb_rdy() == false) {
			pr_err("CDP, block\n");
			while (is_usb_rdy() == false && timeout > 0) {
				msleep(10);
				timeout--;
			}
			if (timeout == 0) {
				pr_err("CDP, timeout\n");
				return false;
			} else {
				pr_err("CDP, free, timeout:%d\n", timeout);
			}
		} else {
			pr_err("CDP, PASS\n");
		}
		first_connect = false;
	}
	return true;
}
#endif
static int __sy6974b_read_reg(struct chip_sy6974b *chip, int reg, int *data)
{
	s32 ret = 0;
	int retry = I2C_RETRY_READ_MAX_COUNT;

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_read_byte_data(chip->client, reg);
	mutex_unlock(&chip->i2c_lock);

	if (ret < 0) {
		while(retry > 0 && atomic_read(&chip->driver_suspended) == 0) {
			usleep_range(I2C_RETRY_DELAY_US, I2C_RETRY_DELAY_US);
			mutex_lock(&chip->i2c_lock);
			ret = i2c_smbus_read_byte_data(chip->client, reg);
			mutex_unlock(&chip->i2c_lock);
			if (ret < 0) {
				retry--;
			} else {
				break;
			}
		}
	}

	if (ret < 0) {
		chg_err("i2c read fail: can't read from %02x: %d\n", reg, ret);
		return ret;
	} else {
		*data = ret;
	}

	return 0;
}

static int __sy6974b_write_reg(struct chip_sy6974b *chip, int reg, int val)
{
	s32 ret = 0;
	int retry = I2C_RETRY_WRITE_MAX_COUNT;

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_write_byte_data(chip->client, reg, val);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0) {
		while(retry > 0) {
			usleep_range(I2C_RETRY_DELAY_US, I2C_RETRY_DELAY_US);
			mutex_lock(&chip->i2c_lock);
			ret = i2c_smbus_write_byte_data(chip->client, reg, val);
			mutex_unlock(&chip->i2c_lock);
			if (ret < 0) {
				retry--;
			} else {
				break;
			}
		}
	}

	if (ret < 0) {
		chg_err("i2c write fail: can't write %02x to %02x: %d\n", val, reg, ret);
		return ret;
	}

	return 0;
}

#define I2C_RETRY_READ_BLK_MAX_COUNT        3
static int __sy6974b_read_block(struct chip_sy6974b *chip, u8 reg, u8 length, u8 *data)
{
	s32 ret = 0;
	int retry = I2C_RETRY_READ_BLK_MAX_COUNT;

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(chip->client, reg, length, data);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0 ) {
		while(retry > 0) {
			usleep_range(I2C_RETRY_DELAY_US, I2C_RETRY_DELAY_US);
			mutex_lock(&chip->i2c_lock);
			ret = i2c_smbus_read_i2c_block_data(chip->client, reg, length, data);
			mutex_unlock(&chip->i2c_lock);
			if (ret < 0 ) {
				retry--;
			} else {
				break;
			}
		}
	}

	if (ret < 0 ) {
		chg_err("i2c read block fail: can't read from %02x len=%d ret=%d\n", reg, length, ret);
		return ret;
	}

	return 0;
}


static int sy6974b_read_reg(struct chip_sy6974b *chip, int reg, int *data)
{
	int ret;

	ret = __sy6974b_read_reg(chip, reg, data);

	return ret;
}

static __maybe_unused int sy6974b_write_reg(struct chip_sy6974b *chip, int reg, int data)
{
	int ret;

	ret = __sy6974b_write_reg(chip, reg, data);

	return ret;
}

static __maybe_unused int sy6974b_read_block(struct chip_sy6974b *chip, u8 reg, u8 length, u8 *data)
{
	int ret;

	ret = __sy6974b_read_block(chip, reg, length, data);

	return ret;
}

static __maybe_unused int sy6974b_config_interface(struct chip_sy6974b *chip, int reg, int data, int mask)
{
	int ret;
	int tmp;

	ret = __sy6974b_read_reg(chip, reg, &tmp);
	if (ret) {
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sy6974b_write_reg(chip, reg, tmp);
	if (ret)
		chg_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	return ret;
}

int sy6974b_set_vindpm_vol(int vol)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;
	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	if (vol < REG06_SY6974B_VINDPM_OFFSET)
		vol = REG06_SY6974B_VINDPM_OFFSET;

	tmp = (vol - REG06_SY6974B_VINDPM_OFFSET) / REG06_SY6974B_VINDPM_STEP_MV;
	rc = sy6974b_config_interface(chip, REG06_SY6974B_ADDRESS,
					tmp << REG06_SY6974B_VINDPM_SHIFT,
					REG06_SY6974B_VINDPM_MASK);

	return rc;
}

int oplus_sy6974b_enter_shipmode(bool en)
{
	struct chip_sy6974b *chip = charger_ic;
	u8 val = 0;
	int rc = 0;

	if(!chip) {
		return 0;
	}

	chg_err("enter ship_mode:en:%d\n", en);

	if(en) {
		val = SY6974_BATFET_OFF << REG07_SY6974B_BATFET_DIS_SHIFT;
	} else {
		val = SY6974_BATFET_ON << REG07_SY6974B_BATFET_DIS_SHIFT;
	}
	rc = sy6974b_config_interface(chip, REG07_SY6974B_ADDRESS, val, REG07_SY6974B_BATFET_DIS_MASK);

	chg_err("enter ship_mode:done\n");

	return rc;
}


int sy6974b_usb_icl[] = {
	300, 500, 900, 1200, 1350, 1500, 1750, 2000, 3000,
};

static int sy6974b_get_usb_icl(void)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG00_SY6974B_ADDRESS, &tmp);
	if (rc) {
		chg_err("Couldn't read REG00_SY6974B_ADDRESS rc = %d\n", rc);
		return 0;
	}
	tmp = (tmp & REG00_SY6974B_INPUT_CURRENT_LIMIT_MASK) >> REG00_SY6974B_INPUT_CURRENT_LIMIT_SHIFT;
	return (tmp * REG00_SY6974B_INPUT_CURRENT_LIMIT_STEP + REG00_SY6974B_INPUT_CURRENT_LIMIT_OFFSET);
}

int sy6974b_input_current_limit_without_aicl(int current_ma)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		chg_err("in suspend\n");
		return 0;
	}

	chg_err("tmp current [%d]ma\n", current_ma);

	if (current_ma > REG00_SY6974B_INPUT_CURRENT_LIMIT_MAX)
		current_ma = REG00_SY6974B_INPUT_CURRENT_LIMIT_MAX;

	if (current_ma < REG00_SY6974B_INPUT_CURRENT_LIMIT_OFFSET)
		current_ma = REG00_SY6974B_INPUT_CURRENT_LIMIT_OFFSET;

	tmp = (current_ma - REG00_SY6974B_INPUT_CURRENT_LIMIT_OFFSET) / REG00_SY6974B_INPUT_CURRENT_LIMIT_STEP;

	rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
					tmp << REG00_SY6974B_INPUT_CURRENT_LIMIT_SHIFT,
					REG00_SY6974B_INPUT_CURRENT_LIMIT_MASK);

	if (rc < 0) {
		chg_err("Couldn't set aicl rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_chg_get_dyna_aicl_result(void)
{
	return aicl_result;
}

#define AICL_POINT_VOL_5V_PHASE1 4140
#define AICL_POINT_VOL_5V_PHASE2 4000
#define HW_AICL_POINT_VOL_5V_PHASE1 4440
#define HW_AICL_POINT_VOL_5V_PHASE2 4520
#define SW_AICL_POINT_VOL_5V_PHASE1 4500
#define SW_AICL_POINT_VOL_5V_PHASE2 4535
void sy6974b_set_aicl_point(int vbatt)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return;

	if (chip->hw_aicl_point == HW_AICL_POINT_VOL_5V_PHASE1 &&
	    vbatt > AICL_POINT_VOL_5V_PHASE1) {
		chip->hw_aicl_point = HW_AICL_POINT_VOL_5V_PHASE2;
		chip->sw_aicl_point = SW_AICL_POINT_VOL_5V_PHASE2;
		sy6974b_set_vindpm_vol(chip->hw_aicl_point);
	} else if(chip->hw_aicl_point == HW_AICL_POINT_VOL_5V_PHASE2 &&
	          vbatt < AICL_POINT_VOL_5V_PHASE2) {
		chip->hw_aicl_point = HW_AICL_POINT_VOL_5V_PHASE1;
		chip->sw_aicl_point = SW_AICL_POINT_VOL_5V_PHASE1;
		sy6974b_set_vindpm_vol(chip->hw_aicl_point);
	}
}

int sy6974b_get_charger_vol(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	int vol_vaule = 0;
	get_vbus_voltage(&vol_vaule);
	chg_debug("sy6974b_get_charger_vol: %d\n", vol_vaule);
	return vol_vaule;
	#else
	return qpnp_get_prop_charger_voltage_now();
	#endif
}

int sy6974b_get_vbus_voltage(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	int vol_vaule = 0;
	get_vbus_voltage(&vol_vaule);
	chg_err("sy6974b_get_charger_vol: %d\n", vol_vaule);
	return vol_vaule;
	#else
	return qpnp_get_prop_charger_voltage_now();
	#endif
}

int sy6974b_get_ibus_current(void)
{
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	return 0;
	#else
	return qpnp_get_prop_ibus_now();
	#endif
}

#define AICL_DOWN_DELAY_MS	50
#define AICL_DELAY_MIN_US	90000
#define AICL_DELAY_MAX_US	91000
#define SUSPEND_IBUS_MA		100
#define DEFAULT_IBUS_MA		500
int sy6974b_input_current_limit_write(int current_ma)
{
	int i = 0, rc = 0;
	int chg_vol = 0;
	int sw_aicl_point = 0;
	struct chip_sy6974b *chip = charger_ic;
	int pre_icl_index = 0, pre_icl = 0;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	if (atomic_read(&chip->charger_suspended) == 1) {
		chg_err("suspend,ignore set current=%dmA\n", current_ma);
		return 0;
	}

	/*first: icl down to 500mA, step from pre icl*/
	pre_icl = sy6974b_get_usb_icl();
	for (pre_icl_index = ARRAY_SIZE(sy6974b_usb_icl) - 1; pre_icl_index >= 0; pre_icl_index--) {
		if (sy6974b_usb_icl[pre_icl_index] < pre_icl) {
			break;
		}
	}
	chg_err("icl_set: %d, pre_icl: %d, pre_icl_index: %d\n", current_ma, pre_icl, pre_icl_index);
	for (i = pre_icl_index; i > 1; i--) {
		rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
		if (rc) {
			chg_err("icl_down: set icl to %d mA fail, rc=%d\n", sy6974b_usb_icl[i], rc);
		} else {
			chg_err("icl_down: set icl to %d mA\n", sy6974b_usb_icl[i]);
		}
		msleep(AICL_DOWN_DELAY_MS);
	}

	/*second: aicl process, step from 500ma*/
	if (current_ma < 500) {
		i = 0;
		goto aicl_end;
	}

	sw_aicl_point = chip->sw_aicl_point;

	i = 1; /* 500 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		chg_debug( "use 500 here\n");
		goto aicl_end;
	} else if (current_ma < 900)
		goto aicl_end;

	i = 2; /* 900 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 1;
		goto aicl_pre_step;
	} else if (current_ma < 1200)
		goto aicl_end;

	i = 3; /* 1200 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 1;
		goto aicl_pre_step;
	}

	i = 4; /* 1350 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 2; //We DO NOT use 1.2A here
		goto aicl_pre_step;
	} else if (current_ma < 1350) {
		i = i - 1; //We use 1.2A here
		goto aicl_end;
	}

	i = 5; /* 1500 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 3; //We DO NOT use 1.2A here
		goto aicl_pre_step;
	} else if (current_ma < 1500) {
		i = i - 2; //We use 1.2A here
		goto aicl_end;
	} else if (current_ma < 2000) {
		goto aicl_end;
	}

	i = 6; /* 1750 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 3; //1.2
		goto aicl_pre_step;
	}

	i = 7; /* 2000 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i - 2; //1.5
		goto aicl_pre_step;
	} else if (current_ma < 3000) {
		goto aicl_end;
	}

	i = 8; /* 3000 */
	rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	usleep_range(AICL_DELAY_MIN_US, AICL_DELAY_MAX_US);
	chg_vol = sy6974b_get_charger_vol();
	if (chg_vol < sw_aicl_point) {
		i = i -1;
		goto aicl_pre_step;
	} else if (current_ma >= 3000) {
		goto aicl_end;
	}

aicl_pre_step:
	chg_debug( "usb input max current limit aicl chg_vol=%d j[%d]=%d sw_aicl_point:%d aicl_pre_step\n", chg_vol, i, sy6974b_usb_icl[i], sw_aicl_point);
	goto aicl_rerun;
aicl_end:
	chg_debug( "usb input max current limit aicl chg_vol=%d j[%d]=%d sw_aicl_point:%d aicl_end\n", chg_vol, i, sy6974b_usb_icl[i], sw_aicl_point);
	goto aicl_rerun;
aicl_rerun:
	aicl_result = sy6974b_usb_icl[i];
	if (atomic_read(&chip->charger_suspended) == 1) {
		chip->before_suspend_icl = sy6974b_usb_icl[i];
		chg_err("during aicl, force input current to 100mA,before=%dmA\n", chip->before_suspend_icl);
		rc = sy6974b_input_current_limit_without_aicl(SUSPEND_IBUS_MA);
	} else {
		rc = sy6974b_input_current_limit_without_aicl(sy6974b_usb_icl[i]);
	}
	rc = sy6974b_set_vindpm_vol(chip->hw_aicl_point);
	return rc;
}

#define VOOC_AICL_STEP_MA	500
#define VOOC_AICL_DELAY_MS	35
int sy6974b_input_current_limit_ctrl_by_vooc_write(int current_ma)
{
	int rc = 0;
	int cur_usb_icl  = 0;
	int temp_curr = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	if (atomic_read(&chip->charger_suspended) == 1) {
		chg_err("suspend,ignore set current=%dmA\n", current_ma);
		return 0;
	}

	cur_usb_icl = sy6974b_get_usb_icl();
	chg_err(" get cur_usb_icl = %d\n", cur_usb_icl);

	if (current_ma > cur_usb_icl) {
		for (temp_curr = cur_usb_icl; temp_curr < current_ma; temp_curr += VOOC_AICL_STEP_MA) {
			msleep(VOOC_AICL_DELAY_MS);
			rc = sy6974b_input_current_limit_without_aicl(temp_curr);
			chg_err("[up] set input_current = %d\n", temp_curr);
		}
	} else {
		for (temp_curr = cur_usb_icl; temp_curr > current_ma; temp_curr -= VOOC_AICL_STEP_MA) {
			msleep(VOOC_AICL_DELAY_MS);
			rc = sy6974b_input_current_limit_without_aicl(temp_curr);
			chg_err("[down] set input_current = %d\n", temp_curr);
		}
	}

	rc = sy6974b_input_current_limit_without_aicl(current_ma);
	return rc;
}

static ssize_t sy6974b_access_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct chip_sy6974b *chip = dev_get_drvdata(dev);
	if (!chip) {
		chg_err("chip is null\n");
		return 0;
	}
	return sprintf(buf, "0x%02x\n", chip->reg_access);
}

static ssize_t sy6974b_access_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct chip_sy6974b *chip = dev_get_drvdata(dev);
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;

	if (!chip) {
		chg_err("chip is null\n");
		return 0;
	}

	if (buf != NULL && size != 0) {
		pr_info("[%s] buf is %s and size is %zu\n", __func__, buf, size);

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&chip->reg_access);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&chip->reg_access);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err("[%s] write sy6974b reg 0x%02x with value 0x%02x !\n",
				__func__, (unsigned int) chip->reg_access, reg_value);
			ret = sy6974b_write_reg(chip, chip->reg_access, reg_value);
		} else {
			ret = sy6974b_read_reg(chip, chip->reg_access, &reg_value);
			pr_err("[%s] read sy6974b reg 0x%02x with value 0x%02x !\n",
				__func__, (unsigned int) chip->reg_access, reg_value);
		}
	}
	return size;
}

static DEVICE_ATTR(sy6974b_access, 0660, sy6974b_access_show, sy6974b_access_store);

void sy6974b_dump_registers(void)
{
	int ret = 0;
	int addr = 0;
	u8 val_buf[SY6974B_REG_NUMBER] = {0x0};
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return;
	}

	ret = sy6974b_read_block(chip, SY6974B_FIRST_REG, SY6974B_REG_NUMBER, val_buf);
	if (ret) {
		chg_err("Couldn't read 0x%02x length=%d ret = %d\n", addr, SY6974B_REG_NUMBER, ret);
	}

	chg_err("[0x%02x, 0x%02x, 0x%02x, 0x%02x], [0x%02x, 0x%02x, 0x%02x, 0x%02x], "
		"[0x%02x, 0x%02x, 0x%02x, 0x%02x]\n",
		val_buf[0], val_buf[1], val_buf[2], val_buf[3],
		val_buf[4], val_buf[5], val_buf[6], val_buf[7],
		val_buf[8], val_buf[9], val_buf[10], val_buf[11]);
}

#define DUMP_REG_LOG_CNT_30S 6
void sy6974b_dump_registers_debug(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();
	static int dump_count = 0;

	if (!g_oplus_chip)
		return;

	if (g_oplus_chip->charger_exist
			|| g_oplus_chip->ac_online
			|| (sy6974b_debug & ENABLE_DUMP_LOG))
		dump_count = DUMP_REG_LOG_CNT_30S;

	if(dump_count >= DUMP_REG_LOG_CNT_30S) {
		dump_count = 0;
		sy6974b_dump_registers();
	} else {
		dump_count++;
	}
}

int sy6974b_kick_wdt(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_WDT_TIMER_RESET,
					REG01_SY6974B_WDT_TIMER_RESET_MASK);
	if (rc) {
		chg_err("Couldn't sy6974b kick wdt rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_set_wdt_timer(int reg)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	sy6974b_kick_wdt();

	rc = sy6974b_config_interface(chip, REG05_SY6974B_ADDRESS,
					reg,
					REG05_SY6974B_WATCHDOG_TIMER_MASK);
	if (rc) {
		chg_err("Couldn't set recharging threshold rc = %d\n", rc);
	}

	return 0;
}

static void sy6974b_wdt_enable(bool wdt_enable)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return;
	}

	if (atomic_read(&chip->driver_suspended) == 1)
		return;

	if (wdt_enable)
		sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_40S);
	else
		sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);

	chg_err("sy6974b_wdt_enable[%d]\n", wdt_enable);
}

int sy6974b_set_stat_dis(bool enable)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
					enable ? REG00_SY6974B_STAT_DIS_ENABLE : REG00_SY6974B_STAT_DIS_DISABLE,
					REG00_SY6974B_STAT_DIS_MASK);
	if (rc) {
		chg_err("Couldn't sy6974b set_stat_dis rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_set_int_mask(int val)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG0A_SY6974B_ADDRESS,
					val,
					REG0A_SY6974B_VINDPM_INT_MASK | REG0A_SY6974B_IINDPM_INT_MASK);
	if (rc) {
		chg_err("Couldn't sy6974b set_int_mask rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_set_chg_timer(bool enable)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG05_SY6974B_ADDRESS,
					enable ? REG05_SY6974B_CHG_SAFETY_TIMER_ENABLE : REG05_SY6974B_CHG_SAFETY_TIMER_DISABLE,
					REG05_SY6974B_CHG_SAFETY_TIMER_MASK);
	if (rc) {
		chg_err("Couldn't sy6974b set_chg_timer rc = %d\n", rc);
	}

	return rc;
}

bool sy6974b_get_bus_gd(void)
{
	int rc = 0;
	int reg_val = 0;
	bool bus_gd = false;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG0A_SY6974B_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't oplus_chg_is_usb_present rc = %d\n", rc);
		return false;
	}

	bus_gd = ((reg_val & REG0A_SY6974B_BUS_GD_MASK) == REG0A_SY6974B_BUS_GD_YES) ? 1 : 0;
	return bus_gd;
}

bool sy6974b_get_power_gd(void)
{
	int rc = 0;
	int reg_val = 0;
	bool power_gd = false;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG08_SY6974B_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't get_power_gd rc = %d\n", rc);
		return false;
	}

	power_gd = ((reg_val & REG08_SY6974B_POWER_GOOD_STAT_MASK) == REG08_SY6974B_POWER_GOOD_STAT_GOOD) ? 1 : 0;
	return power_gd;
}

bool oplus_chg_is_usb_present(void)
{
	static bool pre_vbus_status = false;
	struct chip_sy6974b *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (oplus_get_otg_online_status()) {
		chg_err("otg,return false");
		pre_vbus_status = false;
		return pre_vbus_status;
	}
#else
	if (oplus_get_otg_online_status_default()) {
		chg_err("otg,return false");
		pre_vbus_status = false;
		return pre_vbus_status;
	}
#endif
	if (oplus_voocphy_get_fastchg_commu_ing()) {
		/*chg_err("fastchg_commu_ing,return true");*/
		pre_vbus_status = true;
		return pre_vbus_status;
	}

	if (chip && atomic_read(&chip->driver_suspended) == 1
	    && g_oplus_chip && g_oplus_chip->unwakelock_chg == 1
	    && g_oplus_chip->charger_type != POWER_SUPPLY_TYPE_UNKNOWN) {
		chg_err("unwakelock_chg=1, use pre status=%d\n", pre_vbus_status);
		return pre_vbus_status;
	}

	pre_vbus_status = sy6974b_get_bus_gd();
	return pre_vbus_status;
}

int sy6974b_charging_current_write_fast(int chg_cur)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	chg_err("set charge current = %d\n", chg_cur);

	if (chg_cur > REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_MAX)
		chg_cur = REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_MAX;

	if (chg_cur < REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_OFFSET)
		chg_cur = REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_OFFSET;

	tmp = chg_cur - REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_STEP;

	rc = sy6974b_config_interface(chip, REG02_SY6974B_ADDRESS,
					tmp << REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_SHIFT,
					REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_MASK);

	return rc;
}

int sy6974b_float_voltage_write(int vfloat_mv)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	chg_err("vfloat_mv = %d\n", vfloat_mv);

	if (chip->is_bq25601d) {
		if (vfloat_mv > REG04_BQ25601D_CHG_VOL_LIMIT_MAX)
			vfloat_mv = REG04_BQ25601D_CHG_VOL_LIMIT_MAX;

		if (vfloat_mv < REG04_BQ25601D_CHG_VOL_LIMIT_OFFSET)
			vfloat_mv = REG04_BQ25601D_CHG_VOL_LIMIT_OFFSET;

		tmp = vfloat_mv - REG04_BQ25601D_CHG_VOL_LIMIT_OFFSET;
	} else {
		if (vfloat_mv > REG04_SY6974B_CHG_VOL_LIMIT_MAX)
			vfloat_mv = REG04_SY6974B_CHG_VOL_LIMIT_MAX;

		if (vfloat_mv < REG04_SY6974B_CHG_VOL_LIMIT_OFFSET)
			vfloat_mv = REG04_SY6974B_CHG_VOL_LIMIT_OFFSET;

		tmp = vfloat_mv - REG04_SY6974B_CHG_VOL_LIMIT_OFFSET;
	}
	tmp = tmp / REG04_SY6974B_CHG_VOL_LIMIT_STEP;

	rc = sy6974b_config_interface(chip, REG04_SY6974B_ADDRESS,
					tmp << REG04_SY6974B_CHG_VOL_LIMIT_SHIFT,
					REG04_SY6974B_CHG_VOL_LIMIT_MASK);

	return rc;
}

int sy6974b_set_termchg_current(int term_curr)
{
	int rc = 0;
	int tmp = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	chg_err("term_current = %d\n", term_curr);
	tmp = term_curr - REG03_SY6974B_TERM_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG03_SY6974B_TERM_CHG_CURRENT_LIMIT_STEP;

	rc = sy6974b_config_interface(chip, REG03_SY6974B_ADDRESS,
					tmp << REG03_SY6974B_TERM_CHG_CURRENT_LIMIT_SHIFT,
					REG03_SY6974B_TERM_CHG_CURRENT_LIMIT_MASK);
	return 0;
}

int sy6974b_otg_ilim_set(int ilim)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG02_SY6974B_ADDRESS,
					ilim,
					REG02_SY6974B_OTG_CURRENT_LIMIT_MASK);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_otg_ilim_set  rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_otg_enable(void)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	sy6974b_really_suspend_charger(false);

	sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);

	rc = sy6974b_otg_ilim_set(REG02_SY6974B_OTG_CURRENT_LIMIT_1200MA);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_otg_ilim_set rc = %d\n", rc);
	}

	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_OTG_ENABLE,
					REG01_SY6974B_OTG_MASK);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_otg_enable  rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_otg_disable(void)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_OTG_DISABLE,
					REG01_SY6974B_OTG_MASK);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_otg_disable rc = %d\n", rc);
	}

	/*sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_40S);*/

	return rc;
}


int oplus_sy6974b_otg_enable(void)
{
	int rc = 0;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();
	rc = sy6974b_otg_enable();
	if (g_oplus_chip && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);
	return rc;
}

int oplus_sy6974b_otg_disable(void)
{
	int rc = 0;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();
	rc = sy6974b_otg_disable();
	if (g_oplus_chip && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);
	return rc;
}

int sy6974b_enable_charging(void)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	if (!g_oplus_chip || !g_oplus_chip->otg_online)
		sy6974b_otg_disable();
	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_CHARGING_ENABLE,
					REG01_SY6974B_CHARGING_MASK);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_enable_charging rc = %d\n", rc);
	}

	chg_err("sy6974b_enable_charging \n");
	return rc;
}

int sy6974b_disable_charging(void)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	if (!g_oplus_chip || !g_oplus_chip->otg_online)
		sy6974b_otg_disable();
	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_CHARGING_DISABLE,
					REG01_SY6974B_CHARGING_MASK);
	if (rc < 0) {
		chg_err("Couldn't sy6974b_disable_charging rc = %d\n", rc);
	}
	chg_err("sy6974b_disable_charging \n");
	return rc;
}

int sy6974b_check_charging_enable(void)
{
	int rc = 0;
	int reg_val = 0;
	struct chip_sy6974b *chip = charger_ic;
	bool charging_enable = false;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG01_SY6974B_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read REG01_SY6974B_ADDRESS rc = %d\n", rc);
		return 0;
	}

	charging_enable = ((reg_val & REG01_SY6974B_CHARGING_MASK) == REG01_SY6974B_CHARGING_ENABLE) ? 1 : 0;

	return charging_enable;
}

int sy6974b_suspend_charger(void)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

#if 0
	if (chip->real_suspend_6974b) {
		rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
				REG00_SY6974B_SUSPEND_MODE_ENABLE,
				REG00_SY6974B_SUSPEND_MODE_MASK);
		if (rc < 0) {
			chg_err("REG00_SY6974B_SUSPEND_MODE_ENABLE fail rc = %d\n", rc);
			return rc;
		}

		return rc;
	} else {
#else
	atomic_set(&chip->charger_suspended, 1);
	chip->before_suspend_icl = sy6974b_get_usb_icl();
	sy6974b_input_current_limit_without_aicl(SUSPEND_IBUS_MA);
	if (oplus_vooc_get_fastchg_to_normal() == false
		&& oplus_vooc_get_fastchg_to_warm() == false) {
		sy6974b_disable_charging();
	}

	return 0;
#endif
}

int sy6974b_unsuspend_charger(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;
    struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}
#if 0
	if (chip->real_suspend_6974b) {
		rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
				REG00_SY6974B_SUSPEND_MODE_DISABLE,
				REG00_SY6974B_SUSPEND_MODE_MASK);
		if (rc < 0) {
			chg_err("Couldn't read REG00_SY6974B_SUSPEND_MODE_DISABLE fail rc = %d\n", rc);
			return rc;
		}

		return rc;
	} else {
#else
		atomic_set(&chip->charger_suspended, 0);
		chip->before_unsuspend_icl = sy6974b_get_usb_icl();
		if ((chip->before_unsuspend_icl == 0)
				|| (chip->before_suspend_icl == 0)
				|| (chip->before_unsuspend_icl != SUSPEND_IBUS_MA)
				|| (chip->before_unsuspend_icl == chip->before_suspend_icl)) {
			chg_err("ignore set icl [%d %d]\n", chip->before_suspend_icl, chip->before_unsuspend_icl);
		} else {
			sy6974b_input_current_limit_without_aicl(chip->before_suspend_icl);
		}

		rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
				REG00_SY6974B_SUSPEND_MODE_DISABLE,
				REG00_SY6974B_SUSPEND_MODE_MASK);
		if (rc < 0) {
			chg_err("REG00_SY6974B_SUSPEND_MODE_DISABLE fail rc = %d\n", rc);
		}

		if (g_oplus_chip) {
				if (oplus_vooc_get_fastchg_to_normal() == false
						&& oplus_vooc_get_fastchg_to_warm() == false) {
					if (g_oplus_chip->authenticate
							&& g_oplus_chip->mmi_chg
							&& !g_oplus_chip->balancing_bat_stop_chg
							&& (g_oplus_chip->charging_state != CHARGING_STATUS_FAIL)
							&& oplus_vooc_get_allow_reading()
							&& !oplus_is_rf_ftm_mode()) {
						sy6974b_enable_charging();
					}
				}
			} else {
				sy6974b_enable_charging();
			}
	return rc;
#endif
}

bool sy6974b_check_suspend_charger(void)
{
#if 0
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;
	int data = 0;

	if (!chip) {
		return 0;
	}

	if (chip->real_suspend_6974b) {
		if (atomic_read(&chip->driver_suspended) == 1) {
			return 0;
		}

		rc = sy6974b_read_reg(chip, REG00_SY6974B_ADDRESS, &data);
		if (rc < 0) {
			chg_err("REG00_SY6974B_ADDRESS fail rc = %d\n", rc);
			return 0;
		}

		if ((data & REG00_SY6974B_SUSPEND_MODE_MASK) == REG00_SY6974B_SUSPEND_MODE_ENABLE) {
			return true;
		}

		return false;
	} else {
#else
	    struct chip_sy6974b *chip = charger_ic;
		return atomic_read(&chip->charger_suspended);
#endif
}

void sy6974b_check_ic_suspend(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!g_oplus_chip || (g_oplus_chip->mmi_chg && g_oplus_chip->stop_chg))
		return;

	if (sy6974b_check_suspend_charger()) {
		if (sy6974b_check_charging_enable())
			sy6974b_disable_charging();
		if (sy6974b_get_usb_icl() != SUSPEND_IBUS_MA)
			sy6974b_input_current_limit_without_aicl(SUSPEND_IBUS_MA);
	}
}

void sy6974b_really_suspend_charger(bool en)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return;
	}

	rc = sy6974b_config_interface(chip, REG00_SY6974B_ADDRESS,
					en ? REG00_SY6974B_SUSPEND_MODE_ENABLE : REG00_SY6974B_SUSPEND_MODE_DISABLE,
					REG00_SY6974B_SUSPEND_MODE_MASK);
	if (rc < 0) {
		chg_err("fail en=%d rc = %d\n", en, rc);
	}
}

int sy6974b_set_rechg_voltage(int recharge_mv)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG04_SY6974B_ADDRESS,
					recharge_mv,
					REG04_SY6974B_RECHG_THRESHOLD_VOL_MASK);

	if (rc) {
		chg_err("Couldn't set recharging threshold rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_reset_charger(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG0B_SY6974B_ADDRESS,
					REG0B_SY6974B_REG_RST_RESET,
					REG0B_SY6974B_REG_RST_MASK);

	if (rc) {
		chg_err("Couldn't sy6974b_reset_charger rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_registers_read_full(void)
{
	int rc = 0;
	int reg_full = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG08_SY6974B_ADDRESS, &reg_full);
	if (rc) {
		chg_err("Couldn't read REG08_SY6974B_ADDRESS rc = %d\n", rc);
		return 0;
	}

	reg_full = ((reg_full & REG08_SY6974B_CHG_STAT_MASK) == REG08_SY6974B_CHG_STAT_CHG_TERMINATION) ? 1 : 0;
	if (reg_full) {
		chg_err("the sy6974b is full");
		sy6974b_dump_registers();
	}

	return rc;
}

int sy6974b_set_chging_term_disable(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG05_SY6974B_ADDRESS,
					REG05_SY6974B_TERMINATION_DISABLE,
					REG05_SY6974B_TERMINATION_MASK);
	if (rc) {
		chg_err("Couldn't set chging term disable rc = %d\n", rc);
	}

	return rc;
}

bool sy6974b_check_charger_resume(void)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return false;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return false;
	}

	return true;
}

bool sy6974b_need_to_check_ibatt(void)
{
	return false;
}

int sy6974b_get_chg_current_step(void)
{
	return REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_STEP;
}

int sy6974b_set_prechg_voltage_threshold(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1)
		return 0;

	rc = sy6974b_config_interface(chip, REG01_SY6974B_ADDRESS,
					REG01_SY6974B_SYS_VOL_LIMIT_3400MV,
					REG01_SY6974B_SYS_VOL_LIMIT_MASK);

	return rc;
}

int sy6974b_set_prechg_current( int ipre_mA)
{
	int tmp = 0;
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1)
		return 0;

	tmp = ipre_mA - REG03_SY6974B_PRE_CHG_CURRENT_LIMIT_OFFSET;
	tmp = tmp / REG03_SY6974B_PRE_CHG_CURRENT_LIMIT_STEP;
	rc = sy6974b_config_interface(chip, REG03_SY6974B_ADDRESS,
					(tmp + 1) << REG03_SY6974B_PRE_CHG_CURRENT_LIMIT_SHIFT,
					REG03_SY6974B_PRE_CHG_CURRENT_LIMIT_MASK);

	return 0;
}

int sy6974b_set_otg_voltage(void)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG06_SY6974B_ADDRESS,
					REG06_SY6974B_OTG_VLIM_5150MV,
					REG06_SY6974B_OTG_VLIM_MASK);

	return rc;
}

int sy6974b_set_ovp(int val)
{
	int rc = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_config_interface(chip, REG06_SY6974B_ADDRESS,
					val,
					REG06_SY6974B_OVP_MASK);

	return rc;
}

int sy6974b_get_vbus_stat(void)
{
	int rc = 0;
	int vbus_stat = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG08_SY6974B_ADDRESS, &vbus_stat);
	if (rc) {
		chg_err("Couldn't read REG08_SY6974B_ADDRESS rc = %d\n", rc);
		return 0;
	}

	vbus_stat = vbus_stat & REG08_SY6974B_VBUS_STAT_MASK;

	return vbus_stat;

}

int sy6974b_set_iindet(void)
{
	int rc;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (hw_bc12_init() == false) {
		return -1;
	}
	msleep(10);
#endif
	rc = sy6974b_config_interface(chip, REG07_SY6974B_ADDRESS,
					REG07_SY6974B_IINDET_EN_MASK,
					REG07_SY6974B_IINDET_EN_FORCE_DET);
	if (rc < 0) {
		chg_err("Couldn't set REG07_SY6974B_IINDET_EN_MASK rc = %d\n", rc);
	}

	return rc;
}

int sy6974b_get_iindet(void)
{
	int rc = 0;
	int reg_val = 0;
	bool is_complete = false;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return 0;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG07_SY6974B_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read REG07_SY6974B_ADDRESS rc = %d\n", rc);
		return false;
	}

	is_complete = ((reg_val & REG07_SY6974B_IINDET_EN_MASK) == REG07_SY6974B_IINDET_EN_DET_COMPLETE) ? 1 : 0;
	return is_complete;
}

#ifdef CONFIG_OPLUS_RTC_DET_SUPPORT
static int rtc_reset_check(void)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc = 0;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return 0;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Error reading rtc device (%s) : %d\n",
			CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n",
			CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	if ((tm.tm_year == 110) && (tm.tm_mon == 0) && (tm.tm_mday <= 1)) {
		chg_debug(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  @@@ wday: %d, yday: %d, isdst: %d\n",
			tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
			tm.tm_wday, tm.tm_yday, tm.tm_isdst);
		rtc_class_close(rtc);
		return 1;
	}

	chg_debug(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  ###  wday: %d, yday: %d, isdst: %d\n",
		tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
		tm.tm_wday, tm.tm_yday, tm.tm_isdst);

close_time:
	rtc_class_close(rtc);
	return 0;
}
#endif /* CONFIG_OPLUS_RTC_DET_SUPPORT */

void sy6974b_vooc_timeout_callback(bool vbus_rising)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return;
	}

	chip->power_good = vbus_rising;
	if (!vbus_rising) {
		sy6974b_unsuspend_charger();
		sy6974b_request_dpdm(chip, false);
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		#ifdef CONFIG_OPLUS_CHARGER_MTK
		sy6974b_inform_charger_type(chip);
		#else
		oplus_set_usb_props_type(chip->oplus_charger_type);
		#endif
		oplus_chg_wakelock(chip, false);
		sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);
	}
	sy6974b_dump_registers();
}

void sy6974b_force_pd_to_dcp(void)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return;
	}

	chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	sy6974b_inform_charger_type(chip);
	#else
	oplus_set_usb_props_type(chip->oplus_charger_type);
	#endif
}

bool sy6974b_get_otg_enable(void)
{
	int rc;
	int reg_val = 0;
	bool otg_enabled = false;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return false;

	if (atomic_read(&chip->driver_suspended) == 1) {
		return false;
	}

	rc = sy6974b_read_reg(chip, REG01_SY6974B_ADDRESS, &reg_val);
	if (rc) {
		chg_err("Couldn't read REG01_SY6974B_ADDRESS rc = %d\n", rc);
		return false;
	}

	otg_enabled = ((reg_val & REG01_SY6974B_OTG_MASK) == REG01_SY6974B_OTG_ENABLE);
	return otg_enabled;

}

static int sy6974b_batfet_reset_disable(struct chip_sy6974b *chip, bool enable)
{
	int rc = 0;
	int val = 0;

	if(enable) {
		val = SY6974_BATFET_RST_DISABLE << REG07_SY6974B_BATFET_RST_EN_SHIFT;
	} else {
		val = SY6974_BATFET_RST_ENABLE << REG07_SY6974B_BATFET_RST_EN_SHIFT;
	}

	rc = sy6974b_config_interface(chip, REG07_SY6974B_ADDRESS, val, REG07_SY6974B_BATFET_RST_EN_MASK);

	return rc;
}

__attribute__((unused)) static int sy6974b_check_charge_done(bool *done)
{
	int rc = 0;
	int reg_full = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip) {
		return 0;
	}

	if (atomic_read(&chip->driver_suspended) == 1) {
		return 0;
	}

	rc = sy6974b_read_reg(chip, REG08_SY6974B_ADDRESS, &reg_full);
	if (rc) {
		chg_err("Couldn't read REG08_SY6974B_ADDRESS rc = %d\n", rc);
		return 0;
	}

	reg_full = ((reg_full & REG08_SY6974B_CHG_STAT_MASK) == REG08_SY6974B_CHG_STAT_CHG_TERMINATION) ? 1 : 0;
	if (reg_full) {
		chg_err("the sy6974b is full");
		sy6974b_dump_registers();
	}
	*done = reg_full;
	return reg_full;
}

__attribute__((unused)) static int sy6974b_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct chip_sy6974b *chip = charger_ic;
	int reg_val;
	int ichg;
	int ret;

	ret = sy6974b_read_reg(chip, REG02_SY6974B_ADDRESS, &reg_val);
	if (!ret) {
		ichg = (reg_val & REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_MASK) >> REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_SHIFT;
		ichg = ichg * REG02_SY6974B_FAST_CHG_CURRENT_LIMIT_STEP;
		*curr = ichg * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sy6974b_set_chargevolt( u32 volt)
{
	struct chip_sy6974b *chip = charger_ic;
	u8 val;

	if(!chip)
	return -1;

	if (volt < REG04_SY6974B_CHG_VOL_LIMIT_OFFSET)
		volt = REG04_SY6974B_CHG_VOL_LIMIT_OFFSET;

	val = (volt - REG04_SY6974B_CHG_VOL_LIMIT_OFFSET)/REG04_SY6974B_CHG_VOL_LIMIT_STEP;
	return sy6974b_config_interface(chip, REG04_SY6974B_ADDRESS,
				val << REG04_SY6974B_CHG_VOL_LIMIT_SHIFT,
				REG04_SY6974B_CHG_VOL_LIMIT_MASK);
}

__attribute__((unused)) static int sy6974b_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct chip_sy6974b *chip = charger_ic;
	int reg_val;
	int vchg;
	int ret;

	ret = sy6974b_read_reg(chip, REG04_SY6974B_ADDRESS, &reg_val);
	if (!ret) {
		vchg = (reg_val & REG04_SY6974B_CHG_VOL_LIMIT_MASK) >> REG04_SY6974B_CHG_VOL_LIMIT_SHIFT;
		vchg = vchg * REG04_SY6974B_CHG_VOL_LIMIT_STEP + REG04_SY6974B_CHG_VOL_LIMIT_OFFSET;
		*volt = vchg * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sy6974b_set_ivl(struct charger_device *chg_dev, u32 volt)
{
	return sy6974b_set_vindpm_vol( volt/1000);
}

__attribute__((unused)) static int sy6974b_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct chip_sy6974b *chip = charger_ic;
	int reg_val;
	int icl;
	int ret;

	ret = sy6974b_read_reg(chip, REG00_SY6974B_ADDRESS, &reg_val);
	if (!ret) {
		icl = (reg_val & REG00_SY6974B_INPUT_CURRENT_LIMIT_MASK) >> REG00_SY6974B_INPUT_CURRENT_LIMIT_SHIFT;
		icl = icl * REG00_SY6974B_INPUT_CURRENT_LIMIT_STEP + REG00_SY6974B_INPUT_CURRENT_LIMIT_OFFSET;
		*curr = icl * 1000;
	}

	return ret;
}

__attribute__((unused)) static int sy6974b_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	struct chip_sy6974b *chip = charger_ic;
	int ret;
	int reg_val;

	ret = sy6974b_read_reg(chip, REG05_SY6974B_ADDRESS, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG05_SY6974B_CHG_SAFETY_TIMER_MASK);

	return ret;
}

__attribute__((unused)) static int sy6974b_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	int ret;

	if (en)
		ret = sy6974b_set_chg_timer(true);
	else
		ret = sy6974b_set_chg_timer(false);

	return ret;
}

int sy6974b_hardware_init(void)
{
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return false;

	chg_err("init sy6974b hardware! \n");

	//must be before set_vindpm_vol and set_input_current
	chip->hw_aicl_point = HW_AICL_POINT_VOL_5V_PHASE1;
	chip->sw_aicl_point = SW_AICL_POINT_VOL_5V_PHASE1;

	//sy6974b_reset_charger();
	sy6974b_set_stat_dis(false);
	sy6974b_set_int_mask(REG0A_SY6974B_VINDPM_INT_NOT_ALLOW | REG0A_SY6974B_IINDPM_INT_NOT_ALLOW);

	sy6974b_set_chg_timer(false);

	sy6974b_disable_charging();

	sy6974b_set_ovp(REG06_SY6974B_OVP_14P0V);

	sy6974b_set_chging_term_disable();

	sy6974b_float_voltage_write(WPC_TERMINATION_VOLTAGE);

	sy6974b_otg_ilim_set(REG02_SY6974B_OTG_CURRENT_LIMIT_1200MA);

	sy6974b_set_prechg_voltage_threshold();

	sy6974b_set_prechg_current(WPC_PRECHARGE_CURRENT);

	sy6974b_charging_current_write_fast(WPC_CHARGE_CURRENT_DEFAULT);

	sy6974b_set_termchg_current(WPC_TERMINATION_CURRENT);

	sy6974b_set_rechg_voltage(WPC_RECHARGE_VOLTAGE_OFFSET);

	sy6974b_set_vindpm_vol(chip->hw_aicl_point);

	sy6974b_set_otg_voltage();

	sy6974b_batfet_reset_disable(chip, chip->batfet_reset_disable);

	if (oplus_is_rf_ftm_mode()) {
		sy6974b_disable_charging();
		sy6974b_suspend_charger();
	} else {
		sy6974b_unsuspend_charger();
		sy6974b_enable_charging();
	}

	/*sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_40S);*/

	if (atomic_read(&chip->charger_suspended) == 1) {
		chg_err("suspend,ignore set current=500mA\n");
		return 0;
	} else {
		sy6974b_input_current_limit_without_aicl(DEFAULT_IBUS_MA);
	}

	return true;
}
#ifndef CONFIG_OPLUS_CHARGER_MTK
struct oplus_chg_operations  sy6974b_chg_ops = {
	.dump_registers = sy6974b_dump_registers_debug,
	.kick_wdt = sy6974b_kick_wdt,
	.hardware_init = sy6974b_hardware_init,
	.charging_current_write_fast = sy6974b_charging_current_write_fast,
	.set_aicl_point = sy6974b_set_aicl_point,
	.input_current_write = sy6974b_input_current_limit_write,
	.input_current_ctrl_by_vooc_write = sy6974b_input_current_limit_ctrl_by_vooc_write,
	.float_voltage_write = sy6974b_float_voltage_write,
	.term_current_set = sy6974b_set_termchg_current,
	.charging_enable = sy6974b_enable_charging,
	.charging_disable = sy6974b_disable_charging,
	.get_charging_enable = sy6974b_check_charging_enable,
	.charger_suspend = sy6974b_suspend_charger,
	.charger_unsuspend = sy6974b_unsuspend_charger,
	.charger_suspend_check = sy6974b_check_suspend_charger,
	.set_rechg_vol = sy6974b_set_rechg_voltage,
	.reset_charger = sy6974b_reset_charger,
	.read_full = sy6974b_registers_read_full,
	.otg_enable = sy6974b_otg_enable,
	.otg_disable = sy6974b_otg_disable,
	.set_charging_term_disable = sy6974b_set_chging_term_disable,
	.check_charger_resume = sy6974b_check_charger_resume,
	.get_chargerid_volt = smbchg_get_chargerid_volt,
	.set_chargerid_switch_val = smbchg_set_chargerid_switch_val,
	.get_chargerid_switch_val = smbchg_get_chargerid_switch_val,
	.need_to_check_ibatt = sy6974b_need_to_check_ibatt,
	.get_chg_current_step = sy6974b_get_chg_current_step,
	.get_charger_type = opchg_get_charger_type,
	.get_real_charger_type = opchg_get_real_charger_type,
	.get_charger_volt = sy6974b_get_vbus_voltage,
	.get_charger_current = sy6974b_get_ibus_current,
	.check_chrdet_status = oplus_chg_is_usb_present,
	.get_instant_vbatt = qpnp_get_battery_voltage,
	.get_boot_mode = get_boot_mode,
	.get_boot_reason = smbchg_get_boot_reason,
	.get_rtc_soc = oplus_chg_get_shutdown_soc,
	.set_rtc_soc = oplus_chg_backup_soc,
#ifdef CONFIG_OPLUS_SHORT_C_BATT_CHECK
	.get_dyna_aicl_result = sy6974b_chg_get_dyna_aicl_result,
#endif
#ifdef CONFIG_OPLUS_RTC_DET_SUPPORT
	.check_rtc_reset = rtc_reset_check,
#endif
	.get_charger_subtype = oplus_chg_get_charger_subtype,
	.oplus_chg_pd_setup = oplus_chg_set_pd_config,
	.set_qc_config = oplus_chg_set_qc_config,
	.oplus_chg_get_pd_type = oplus_sm8150_get_pd_type,
	.enable_qc_detect = oplus_chg_enable_qc_detect,
	.input_current_write_without_aicl = sy6974b_input_current_limit_without_aicl,
	.oplus_chg_wdt_enable = sy6974b_wdt_enable,
	.get_usbtemp_volt = oplus_get_usbtemp_volt,
	.set_typec_sinkonly = oplus_set_typec_sinkonly,
	.set_typec_cc_open = oplus_set_typec_cc_open,
	.really_suspend_charger = sy6974b_really_suspend_charger,
	.oplus_usbtemp_monitor_condition = oplus_usbtemp_condition,
	.vooc_timeout_callback = sy6974b_vooc_timeout_callback,
	.force_pd_to_dcp = sy6974b_force_pd_to_dcp,
	.get_otg_enable = sy6974b_get_otg_enable,
	.get_subboard_temp = oplus_get_subboard_temp,
};

static int sy6974b_parse_dt(struct chip_sy6974b *chip)
{
	int ret = 0;

	chip->real_suspend_6974b = of_property_read_bool(chip->client->dev.of_node, "qcom,use_real_suspend_6974b");

	chip->sy6974b_irq_gpio = of_get_named_gpio(chip->client->dev.of_node, "sy6974b-irq-gpio", 0);
	if (!gpio_is_valid(chip->sy6974b_irq_gpio)) {
		chg_err("gpio_is_valid fail sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request(chip->dev, chip->sy6974b_irq_gpio, "sy6974b-irq-gpio");
	if (ret) {
		chg_err("unable to request sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);
		return -EINVAL;
	}

	chg_err("sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);

	chip->batfet_reset_disable = of_property_read_bool(chip->client->dev.of_node, "qcom,batfet_reset_disable");

	if (of_property_read_u32(chip->client->dev.of_node, "normal-init-work-delay-ms", &chip->normal_init_delay_ms))
		chip->normal_init_delay_ms = INIT_WORK_NORMAL_DELAY;

	if (of_property_read_u32(chip->client->dev.of_node, "other-init-work-delay-ms", &chip->other_init_delay_ms))
		chip->other_init_delay_ms = INIT_WORK_OTHER_DELAY;

	chg_err("init work delay [%d %d]\n", chip->normal_init_delay_ms, chip->other_init_delay_ms);

	return ret;
}
#else

static int sy6974b_inform_charger_type(struct chip_sy6974b *chip)
{
	int ret = 0;
	union power_supply_propval propval;

	if (!chip->psy) {
		chip->psy = power_supply_get_by_name("mtk-master-charger");
		if (IS_ERR_OR_NULL(chip->psy)) {
			chg_err("Couldn't get chip->psy");
			return -EINVAL;
		}
	}

	if (chip->power_good)
		propval.intval = 1;
	else
		propval.intval = 0;

	chg_debug("inform power supply online status:%d", propval.intval);
	ret = power_supply_set_property(chip->psy, POWER_SUPPLY_PROP_ONLINE, &propval);
	if (ret < 0) {
		chg_err("inform power supply online failed:%d", ret);
	}

	power_supply_changed(chip->psy);
	return ret;
}

static int oplus_sy6974b_hardware_init(void)
{
	int ret = 0;
	struct chip_sy6974b *chip = charger_ic;

	if (!chip)
		return false;

	chg_err("oplus_sy6974b_hardware_init \n");
	sy6974b_set_vindpm_vol(chip->hw_aicl_point);
	/* Enable charging */
	if (strcmp(chip->chg_dev_name, "primary_chg") == 0) {
		ret = sy6974b_enable_charging();
		if (ret < 0) {
			chg_err("oplus_sy6974b_hardware_init enable charging failed \n");
		} else {
			sy6974b_input_current_limit_without_aicl(DEFAULT_IBUS_MA);
		}
	}
	return ret;
}

static int sy6974b_dump_register(struct charger_device *chg_dev)
{
	sy6974b_dump_registers();
	return 0;
}
int oplus_sy6974b_kick_wdt(struct charger_device *chg_dev)
{
	return sy6974b_kick_wdt();
}

static int sy6974b_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	int rc;
	int val;
	struct chip_sy6974b *chip = charger_ic;

	rc = sy6974b_read_reg(chip, REG01_SY6974B_ADDRESS, &val);
	sy6974b_dump_registers();
	if (!rc)
		chip->charge_enabled = !!(val & REG01_SY6974B_CHARGING_MASK);

	*en = chip->charge_enabled;

	return 0;
}

static int sy6974b_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	int rc = 0;
	rc = sy6974b_check_charge_done(done);
	return rc;
}

static int oplus_sy6974b_is_charging_done(void)
{
	bool done;
	sy6974b_check_charge_done(&done);
	return done;
}
static int  sy6974b_get_chargerid_volt(void)
{
	return 0;
}
static void sy6974b_set_chargerid_switch_val(int value)
{
	mt_set_chargerid_switch_val(value);
}

static int sy6974b_get_chargerid_switch_val(void)
{
	return mt_get_chargerid_switch_val();
}
static int oplus_get_boot_mode(void)
{
	return (int)get_boot_mode();
}

static int oplus_get_boot_reason(void)
{
	return 0;
}

static int sy6974b_get_charger_subtype(void)
{
	return CHARGER_SUBTYPE_DEFAULT;

}
#ifdef CONFIG_OPLUS_SHORT_HW_CHECK
/* Add for HW shortc */
static bool oplus_shortc_check_is_gpio(struct oplus_chg_chip *chip)
{
	if (!chip) {
		printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip not ready!\n", __func__);
		return false;
	}

	if (gpio_is_valid(chip->normalchg_gpio.shortc_gpio)) {
		return true;
	}

	return false;
}

static bool sy6974b_get_shortc_hw_gpio_status(void)
{
	bool shortc_hw_status = true;
	struct oplus_chg_chip *chip = g_oplus_chip;
	if (!chip) {
		printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip not ready!\n", __func__);
		return shortc_hw_status;
	}

	if (oplus_shortc_check_is_gpio(chip) == true) {
		shortc_hw_status = !!(gpio_get_value(chip->normalchg_gpio.shortc_gpio));
	}
	return shortc_hw_status;
}
#else /* CONFIG_OPLUS_SHORT_HW_CHECK */
static bool sy6974b_get_shortc_hw_gpio_status(void)
{
	bool shortc_hw_status = true;
	return shortc_hw_status;
}
#endif /* CONFIG_OPLUS_SHORT_HW_CHECK */


static int sy6974b_charging(struct charger_device *chg_dev, bool enable)
{
	int ret = 0;

	if (enable)
		ret = sy6974b_enable_charging();
	else
		ret = sy6974b_disable_charging();

	chg_debug(" %s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}

static int sy6974b_plug_in(struct charger_device *chg_dev)
{
	int ret;

	ret = sy6974b_charging(chg_dev, true);

	if (ret) {
		chg_err("Failed to enable charging:%d", ret);
	}
	return ret;
}

static int sy6974b_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = sy6974b_charging(chg_dev, false);

	if (ret) {
		chg_err("Failed to disable charging:%d", ret);
	}
	return ret;
}

static int sy6974b_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	chg_debug("charge curr = %d", curr);
	return sy6974b_charging_current_write_fast(curr / 1000);
}

static int sy6974b_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	*curr = 60 * 1000;
	return 0;
}

static int sy6974b_set_vchg(struct charger_device *chg_dev, u32 volt)
{
	return sy6974b_set_chargevolt(volt/1000);
}

static int sy6974b_set_icl(struct charger_device *chg_dev, u32 curr)
{
	return sy6974b_input_current_limit_without_aicl(curr/1000);
}

static int sy6974b_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;

	if (en)
		ret = sy6974b_otg_enable();
	else
		ret = sy6974b_otg_disable();

	power_supply_changed(g_oplus_chip->usb_psy);
	return ret;
}

static int sy6974b_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	int ret = 0;
	u8 val = REG02_SY6974B_OTG_CURRENT_LIMIT_500MA;

	if ((curr/1000) >= REG02_SY6974B_BOOSTI_1200)
		val = REG02_SY6974B_OTG_CURRENT_LIMIT_1200MA;

	ret = sy6974b_otg_ilim_set(val);

	return ret;
}

static int sy6974b_chgdet_en(bool en)
{
	struct oplus_chg_chip *chip = g_oplus_chip;

	if(g_oplus_chip == NULL || g_oplus_chip->chg_ops == NULL
		|| g_oplus_chip->chg_ops->get_charging_enable == NULL){
		return -1;
	}
	if (en) {
		if (oplus_vooc_get_fastchg_to_normal() == false
			&& oplus_vooc_get_fastchg_to_warm() == false) {
			if (chip->authenticate
				&& chip->mmi_chg
				&& !chip->balancing_bat_stop_chg
				&& (chip->charging_state != CHARGING_STATUS_FAIL)
				&& oplus_vooc_get_allow_reading()
				&& !oplus_is_rf_ftm_mode()) {
				sy6974b_enable_charging();
			} else {
				chg_err("should not turn on charging here! \n");
			}
		}
	} else {
		sy6974b_disable_charging();
		Charger_Detect_Release();
		charger_ic->bc12_done = false;
		charger_ic->bc12_retried = 0;
		charger_ic->bc12_delay_cnt = 0;
		charger_ic->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sy6974b_inform_charger_type(charger_ic);
	}
	return 0;
}
static int sy6974b_enable_chgdet(struct charger_device *chg_dev, bool en)
{
    int ret;

    pr_notice("sy6974b_enable_chgdet:%d\n",en);
    ret = sy6974b_chgdet_en(en);

    return ret;
}
static int sy6974b_get_pd_type(void)
{
	return 0;
}

#ifdef CONFIG_OPLUS_CHARGER_MTK
static void oplus_mt_power_off(void)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!g_oplus_chip) {
		printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip not ready!\n", __func__);
		return;
	}

	if (g_oplus_chip->ac_online != true) {
		if(!oplus_chg_is_usb_present())
			kernel_power_off();
	} else {
		printk(KERN_ERR "[OPLUS_CHG][%s]: ac_online is true, return!\n", __func__);
	}
}

static void oplus_sy6974b_choose_gauge_curve(int index_curve)
{
	static last_curve_index = -1;
	int target_index_curve = -1;

	if (index_curve == CHARGER_SUBTYPE_QC || index_curve == CHARGER_SUBTYPE_PD
	|| index_curve == CHARGER_SUBTYPE_FASTCHG_VOOC)
		target_index_curve = CHARGER_FASTCHG_VOOC_AND_QCPD_CURVE;
	else if (index_curve == 0)
		target_index_curve = CHARGER_NORMAL_CHG_CURVE;
	else
		target_index_curve = CHARGER_FASTCHG_SVOOC_CURVE;

	chg_err(" index_curve =%d  target_index_curve =%d last_curve_index =%d\n",
	index_curve, target_index_curve, last_curve_index);

	if (target_index_curve != last_curve_index) {
		oplus_gauge_set_power_sel(target_index_curve);
		last_curve_index = target_index_curve;
	}
	return;
}
#endif

static struct charger_ops sy6974b_chg_ops = {
	/* Normal charging */
	.plug_in = sy6974b_plug_in,
	.plug_out = sy6974b_plug_out,
	.dump_registers = sy6974b_dump_register,
	.enable = sy6974b_charging,
	.is_enabled = sy6974b_is_charging_enable,
	.get_charging_current = sy6974b_get_ichg,
	.set_charging_current = sy6974b_set_ichg,
	.get_input_current = sy6974b_get_icl,
	.set_input_current = sy6974b_set_icl,
	.get_constant_voltage = sy6974b_get_vchg,
	.set_constant_voltage = sy6974b_set_vchg,
	.kick_wdt = oplus_sy6974b_kick_wdt,
	.set_mivr = sy6974b_set_ivl,
	.is_charging_done = sy6974b_is_charging_done,
	.get_min_charging_current = sy6974b_get_min_ichg,

	/* Safety timer */
	.enable_safety_timer = sy6974b_set_safety_timer,
	.is_safety_timer_enabled = sy6974b_is_safety_timer_enabled,

	/* Power path */
	.enable_powerpath = NULL,
	.is_powerpath_enabled = NULL,

	.enable_chg_type_det = sy6974b_enable_chgdet,
	/* OTG */
	.enable_otg = sy6974b_set_otg,
	.set_boost_current_limit = sy6974b_set_boost_ilmt,
	.enable_discharge = NULL,

	/* ADC */
	.get_tchg_adc = NULL,
};

struct oplus_chg_operations  oplus_chg_sy6974b_ops = {
	.dump_registers = sy6974b_dump_registers_debug,
	.kick_wdt = sy6974b_kick_wdt,
	.hardware_init = oplus_sy6974b_hardware_init,
	.charging_current_write_fast = sy6974b_charging_current_write_fast,
	.set_aicl_point = sy6974b_set_aicl_point,
	.input_current_write = sy6974b_input_current_limit_write,
	.float_voltage_write = sy6974b_float_voltage_write,
	.term_current_set = sy6974b_set_termchg_current,
	.charging_enable = sy6974b_enable_charging,
	.charging_disable = sy6974b_disable_charging,
	.get_charging_enable = sy6974b_check_charging_enable,
	.charger_suspend = sy6974b_suspend_charger,
	.charger_unsuspend = sy6974b_unsuspend_charger,
	.read_full = oplus_sy6974b_is_charging_done,
	.otg_enable = oplus_sy6974b_otg_enable,
	.otg_disable = oplus_sy6974b_otg_disable,
	.set_charging_term_disable = sy6974b_set_chging_term_disable,
	.check_charger_resume = sy6974b_check_charger_resume,
	.get_chargerid_volt = sy6974b_get_chargerid_volt,
	.set_chargerid_switch_val = sy6974b_set_chargerid_switch_val,
	.get_chargerid_switch_val = sy6974b_get_chargerid_switch_val,
	.need_to_check_ibatt = sy6974b_need_to_check_ibatt,
	.get_chg_current_step = sy6974b_get_chg_current_step,
	.get_charger_type = opchg_get_charger_type,
	.get_charger_volt = sy6974b_get_charger_vol,
#ifdef CONFIG_OPLUS_CHARGER_MTK
	.set_power_off = oplus_mt_power_off,
#endif
	.check_chrdet_status = oplus_chg_is_usb_present,
	.get_instant_vbatt = oplus_battery_meter_get_battery_voltage,
	.get_boot_mode = oplus_get_boot_mode,
	.get_boot_reason = oplus_get_boot_reason,
	.get_rtc_soc = get_rtc_spare_oplus_fg_value,
	.set_rtc_soc = set_rtc_spare_oplus_fg_value,
	.usb_connect = oplus_mt_usb_connect,
	.usb_disconnect = oplus_mt_usb_disconnect,
	.get_dyna_aicl_result = sy6974b_chg_get_dyna_aicl_result,
	.get_charger_subtype = sy6974b_get_charger_subtype,
	.enable_shipmode = oplus_sy6974b_enter_shipmode,
	.input_current_write_without_aicl = sy6974b_input_current_limit_without_aicl,
	.oplus_chg_wdt_enable = sy6974b_wdt_enable,
	.get_usbtemp_volt = oplus_get_usbtemp_volt,
	.set_typec_cc_open = oplus_mt6789_usbtemp_set_cc_open,
	.set_typec_sinkonly = oplus_mt6789_usbtemp_set_typec_sinkonly,
	.get_shortc_hw_gpio_status = sy6974b_get_shortc_hw_gpio_status,
	.oplus_chg_get_pd_type = sy6974b_get_pd_type,
	.really_suspend_charger = sy6974b_really_suspend_charger,
	.oplus_usbtemp_monitor_condition = oplus_usbtemp_condition,
	.vooc_timeout_callback = sy6974b_vooc_timeout_callback,
	.get_subboard_temp = oplus_force_get_subboard_temp,
#ifdef CONFIG_OPLUS_CHARGER_MTK
	.get_platform_gauge_curve = oplus_sy6974b_choose_gauge_curve,
#endif
};

static int sy6974b_parse_dt(struct device_node *np, struct chip_sy6974b *chip)
{
	int ret = 0;
	if (of_property_read_string(np, "charger_name", &chip->chg_dev_name) < 0) {
		chip->chg_dev_name = "primary_chg";
		chg_err("no charger name\n");
	}
	chip->real_suspend_6974b = of_property_read_bool(chip->client->dev.of_node, "qcom,use_real_suspend_6974b");

	chip->sy6974b_irq_gpio = of_get_named_gpio(chip->client->dev.of_node, "sy6974b-irq-gpio", 0);
	if (!gpio_is_valid(chip->sy6974b_irq_gpio)) {
		chg_err("gpio_is_valid fail sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request(chip->dev, chip->sy6974b_irq_gpio, "sy6974b-irq-gpio");
	if (ret) {
		chg_err("unable to request sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);
		return -EINVAL;
	}

	chg_err("sy6974b-irq-gpio[%d]\n", chip->sy6974b_irq_gpio);

	chip->batfet_reset_disable = of_property_read_bool(chip->client->dev.of_node, "qcom,batfet_reset_disable");

	if (of_property_read_u32(chip->client->dev.of_node, "normal-init-work-delay-ms", &chip->normal_init_delay_ms))
		chip->normal_init_delay_ms = INIT_WORK_NORMAL_DELAY;

	if (of_property_read_u32(chip->client->dev.of_node, "other-init-work-delay-ms", &chip->other_init_delay_ms))
		chip->other_init_delay_ms = INIT_WORK_OTHER_DELAY;

	chg_err("init work delay [%d %d]\n", chip->normal_init_delay_ms, chip->other_init_delay_ms);

	return ret;
}
#endif
int opchg_get_charger_type(void)
{
	struct chip_sy6974b *chip = charger_ic;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip || !g_oplus_chip)
		return POWER_SUPPLY_TYPE_UNKNOWN;

	if (chip->oplus_charger_type != g_oplus_chip->charger_type && g_oplus_chip->usb_psy)
		power_supply_changed(g_oplus_chip->usb_psy);
	return chip->oplus_charger_type;
}

static int sy6974b_request_dpdm(struct chip_sy6974b *chip, bool enable)
{
	int ret = 0;
#ifndef CONFIG_OPLUS_CHARGER_MTK
	if (!chip)
		return 0;
	/* fetch the DPDM regulator */
	if (!chip->dpdm_reg && of_get_property(chip->dev->of_node,
				"dpdm-supply", NULL)) {
		chip->dpdm_reg = devm_regulator_get(chip->dev, "dpdm");
		if (IS_ERR(chip->dpdm_reg)) {
			ret = PTR_ERR(chip->dpdm_reg);
			chg_err("Couldn't get dpdm regulator ret=%d\n", ret);
			chip->dpdm_reg = NULL;
			return ret;
		}
	}

	mutex_lock(&chip->dpdm_lock);
	if (enable) {
		if (chip->dpdm_reg && !chip->dpdm_enabled) {
			chg_err("enabling DPDM regulator\n");
			ret = regulator_enable(chip->dpdm_reg);
			if (ret < 0)
				chg_err("Couldn't enable dpdm regulator ret=%d\n", ret);
			else
				chip->dpdm_enabled = true;
		}
	} else {
		if (chip->dpdm_reg && chip->dpdm_enabled) {
			chg_err("disabling DPDM regulator\n");
			ret = regulator_disable(chip->dpdm_reg);
			if (ret < 0)
				chg_err("Couldn't disable dpdm regulator ret=%d\n", ret);
			else
				chip->dpdm_enabled = false;
		}
	}
	mutex_unlock(&chip->dpdm_lock);
#endif
	return ret;
}

static void sy6974b_bc12_retry_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct chip_sy6974b *chip = container_of(dwork, struct chip_sy6974b, bc12_retry_work);

	if (chip->is_sy6974b) {

		do {
			if (!oplus_chg_is_usb_present()) {
				chg_err("plugout during BC1.2,delay_cnt=%d,return\n", chip->bc12_delay_cnt);
				chip->bc12_delay_cnt = 0;
				return;
			}

			if (chip->bc12_delay_cnt >= OPLUS_BC12_DELAY_CNT) {
				chg_err("BC1.2 not complete delay_cnt to max\n");
				return;
			}

			chip->bc12_delay_cnt++;

			chg_err("BC1.2 not complete delay 50ms,delay_cnt=%d\n", chip->bc12_delay_cnt);
			msleep(50);
		} while (!sy6974b_get_iindet());

		chg_err("BC1.2 complete,delay_cnt=%d\n", chip->bc12_delay_cnt);
		sy6974b_get_bc12(chip);
	}
}

static void sy6974b_start_bc12_retry(struct chip_sy6974b *chip) {
	if (!chip)
		return;

	sy6974b_set_iindet();
	if (chip->is_sy6974b) {
		schedule_delayed_work(&chip->bc12_retry_work, msecs_to_jiffies(100));
	}
}

static void sy6974b_get_bc12(struct chip_sy6974b *chip)
{
	int vbus_stat = 0;

	if (!chip)
		return;

	if (!chip->bc12_done) {
		vbus_stat = sy6974b_get_vbus_stat();
		switch (vbus_stat) {
		case REG08_SY6974B_VBUS_STAT_SDP:
			if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
				chip->bc12_retried++;
				chg_err("bc1.2 sdp retry cnt=%d\n", chip->bc12_retried);
				sy6974b_start_bc12_retry(chip);
				break;
			} else {
				oplus_notify_device_mode(true);
			}
			chip->bc12_done = true;
			sy6974b_check_ic_suspend();
			if (oplus_pd_without_usb()) {
				chg_err("pd without usb_comm,force sdp to dcp\n");
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
			} else {
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB;
			}
			#ifdef CONFIG_OPLUS_CHARGER_MTK
			Charger_Detect_Release();
			sy6974b_inform_charger_type(chip);
			#else
			oplus_set_usb_props_type(chip->oplus_charger_type);
			#endif
			oplus_chg_wake_update_work();
			break;
		case REG08_SY6974B_VBUS_STAT_CDP:
			if (chip->bc12_retried < OPLUS_BC12_RETRY_CNT) {
				chip->bc12_retried++;
				chg_err("bc1.2 cdp retry cnt=%d\n", chip->bc12_retried);
				sy6974b_start_bc12_retry(chip);
				break;
			}

			chip->bc12_done = true;
			sy6974b_check_ic_suspend();
			if (oplus_pd_without_usb()) {
				chg_err("pd without usb_comm,force cdp to dcp\n");
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
			} else {
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_CDP;
			}
			#ifdef CONFIG_OPLUS_CHARGER_MTK
			Charger_Detect_Release();
			sy6974b_inform_charger_type(chip);
			#else
			oplus_set_usb_props_type(chip->oplus_charger_type);
			#endif
			oplus_notify_device_mode(true);
			oplus_chg_wake_update_work();
			break;
		case REG08_SY6974B_VBUS_STAT_DCP:
		case REG08_SY6974B_VBUS_STAT_OCP:
		case REG08_SY6974B_VBUS_STAT_FLOAT:
			chip->bc12_done = true;
			sy6974b_check_ic_suspend();
			#ifdef CONFIG_OPLUS_CHARGER_MTK
			chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
			chg_err("bc1.2 dcp\n");
			Charger_Detect_Release();
			sy6974b_inform_charger_type(chip);
			#else
			if (oplus_pd_connected() && oplus_sm8150_get_pd_type() == PD_INACTIVE) {
				chg_err("pd adapter not ready sleep 300ms \n");
				msleep(300);
				if (!oplus_chg_is_usb_present()) {
					sy6974b_request_dpdm(chip, false);
					chip->bc12_done = false;
					chip->bc12_retried = 0;
					chip->bc12_delay_cnt = 0;
					chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
					oplus_set_usb_props_type(chip->oplus_charger_type);
					chg_err("vbus not good,break\n");
					break;
				}
			}
			chip->oplus_charger_type = POWER_SUPPLY_TYPE_USB_DCP;
			oplus_set_usb_props_type(chip->oplus_charger_type);
			#endif
			oplus_chg_wake_update_work();
			break;
		case REG08_SY6974B_VBUS_STAT_OTG_MODE:
		case REG08_SY6974B_VBUS_STAT_UNKNOWN:
		default:
			break;
		}
	}
}

static void oplus_chg_awake_init(struct chip_sy6974b *chip)
{
	chip->suspend_ws = NULL;
	if (!chip) {
		pr_err("[%s]chip is null\n", __func__);
		return;
	}
	chip->suspend_ws = wakeup_source_register(NULL, "split chg wakelock");
	return;
}

static void oplus_chg_wakelock(struct chip_sy6974b *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->suspend_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->suspend_ws);
		pr_err("[%s] true\n", __func__);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->suspend_ws);
		pm_flag = false;
		pr_err("[%s] false\n", __func__);
	}
	return;
}

static void oplus_keep_resume_awake_init(struct chip_sy6974b *chip)
{
	chip->keep_resume_ws = NULL;
	if (!chip) {
		chg_err("[%s]chip is null\n", __func__);
		return;
	}
	chip->keep_resume_ws = wakeup_source_register(NULL, "split_chg_keep_resume");
	return;
}

static void oplus_keep_resume_wakelock(struct chip_sy6974b *chip, bool awake)
{
	static bool pm_flag = false;

	if (!chip || !chip->keep_resume_ws)
		return;

	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->keep_resume_ws);
		chg_err("[%s] true\n", __func__);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->keep_resume_ws);
		pm_flag = false;
		chg_err("[%s] false\n", __func__);
	}
	return;
}

#define OPLUS_WAIT_RESUME_TIME	200
static irqreturn_t sy6974b_irq_handler(int irq, void *data)
{
	struct chip_sy6974b *chip = (struct chip_sy6974b *)data;
	bool prev_pg = false, curr_pg = false, bus_gd = false;
	int reg_val = 0;
	int ret = 0;
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	if (!chip) {
		oplus_keep_resume_wakelock(chip, false);
		return IRQ_HANDLED;
	}
	#ifdef CONFIG_OPLUS_CHARGER_MTK
	if (oplus_get_otg_online_status()) {
		chg_err("otg,ignore\n");
		oplus_keep_resume_wakelock(chip, false);
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		sy6974b_inform_charger_type(chip);
		return IRQ_HANDLED;
	}
	#else
	if (oplus_get_otg_online_status_default()) {
		chg_err("otg,ignore\n");
		oplus_keep_resume_wakelock(chip, false);
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		oplus_set_usb_props_type(chip->oplus_charger_type);
		return IRQ_HANDLED;
	}
	#endif
	chg_err(" sy6974b_irq_handler:enter improve irq time\n");
	oplus_keep_resume_wakelock(chip, true);

	/*for check bus i2c/spi is ready or not*/
	if (atomic_read(&chip->driver_suspended) == 1) {
		chg_err(" sy6974b_irq_handler:suspended and wait_event_interruptible %d\n", OPLUS_WAIT_RESUME_TIME);
		wait_event_interruptible_timeout(chip->wait, atomic_read(&chip->driver_suspended) == 0, msecs_to_jiffies(OPLUS_WAIT_RESUME_TIME));
	}

	prev_pg = chip->power_good;
	ret = sy6974b_read_reg(chip, REG0A_SY6974B_ADDRESS, &reg_val);
	if (ret) {
		chg_err("[%s] SY6974B_REG_0B read failed ret[%d]\n", __func__, ret);
		oplus_keep_resume_wakelock(chip, false);
		return IRQ_HANDLED;
	}
	curr_pg = bus_gd = sy6974b_get_bus_gd();

	if(curr_pg) {
		oplus_chg_wakelock(chip, true);
	}

	sy6974b_dump_registers();
	oplus_chg_check_break(bus_gd);
	oplus_chg_track_check_wired_charging_break(curr_pg);
	if (oplus_vooc_get_fastchg_started() == true
			&& oplus_vooc_get_adapter_update_status() != 1) {
		chg_err("oplus_vooc_get_fastchg_started = true!(%d %d)\n", prev_pg, curr_pg);
		chip->power_good = curr_pg;
		goto POWER_CHANGE;
	} else {
		chip->power_good = curr_pg;
	}
	chg_err("(%d,%d, %d, %d)\n", prev_pg, chip->power_good, curr_pg, bus_gd);

	if (!prev_pg && chip->power_good) {
		oplus_chg_track_check_wired_charging_break(1);
		oplus_chg_wakelock(chip, true);
		sy6974b_request_dpdm(chip, true);
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		oplus_voocphy_set_adc_enable(true);
		sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_40S);
		oplus_wake_up_usbtemp_thread();
#ifdef CONFIG_OPLUS_CHARGER_MTK
		if (get_boot_mode() != META_BOOT) {
			Charger_Detect_Init();
		}
#endif
		if (chip->oplus_charger_type == POWER_SUPPLY_TYPE_UNKNOWN) {
			sy6974b_get_bc12(chip);
		}
		if (g_oplus_chip) {
			if (oplus_vooc_get_fastchg_to_normal() == false
					&& oplus_vooc_get_fastchg_to_warm() == false) {
				if (g_oplus_chip->authenticate
						&& g_oplus_chip->mmi_chg
						&& !g_oplus_chip->balancing_bat_stop_chg
						&& oplus_vooc_get_allow_reading()
						&& !oplus_is_rf_ftm_mode()) {
					sy6974b_enable_charging();
				}
			}
		}
		goto POWER_CHANGE;
	} else if (prev_pg && !chip->power_good) {
#ifdef CONFIG_OPLUS_CHARGER_MTK
		Charger_Detect_Release();
#endif
		sy6974b_request_dpdm(chip, false);
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
		oplus_chg_track_check_wired_charging_break(0);
		#ifdef CONFIG_OPLUS_CHARGER_MTK
		sy6974b_inform_charger_type(chip);
		#else
		oplus_set_usb_props_type(chip->oplus_charger_type);
		#endif
		sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);
		oplus_vooc_reset_fastchg_after_usbout();
		if (oplus_vooc_get_fastchg_started() == false) {
			oplus_chg_set_chargerid_switch_val(0);
			oplus_chg_clear_chargerid_info();
		}
		oplus_chg_set_charger_type_unknown();
		oplus_chg_wake_update_work();
		oplus_wake_up_usbtemp_thread();
		oplus_notify_device_mode(false);
		oplus_voocphy_set_adc_enable(false);
		oplus_chg_wakelock(chip, false);
		goto POWER_CHANGE;
	} else if (!prev_pg && !chip->power_good) {
		chg_err("prev_pg & now_pg is false\n");
#ifdef CONFIG_OPLUS_CHARGER_MTK
		Charger_Detect_Release();
#endif
		chip->bc12_done = false;
		chip->bc12_retried = 0;
		chip->bc12_delay_cnt = 0;
		goto POWER_CHANGE;
	}

	sy6974b_get_bc12(chip);
POWER_CHANGE:
	if(dumpreg_by_irq)
		sy6974b_dump_registers();
	oplus_keep_resume_wakelock(chip, false);
	return IRQ_HANDLED;
}

static int sy6974b_irq_register(struct chip_sy6974b *chip)
{
	int ret = 0;

	ret = devm_request_threaded_irq(chip->dev, gpio_to_irq(chip->sy6974b_irq_gpio), NULL,
					sy6974b_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"sy6974b-eint",
					chip);
	if (ret < 0) {
		chg_err("sy6974b request_irq fail!");
		return -EFAULT;
	}

	ret = enable_irq_wake(gpio_to_irq(chip->sy6974b_irq_gpio));
	if (ret != 0) {
		chg_err("enable_irq_wake: sy6974b_irq_gpio failed %d\n", ret);
	}

	return ret;
}

static void bq25601_hw_component_detect(struct chip_sy6974b *chip)
{
	int rc = 0;
	int tmp = 0;

	rc = sy6974b_read_reg(chip, REG0B_SY6974B_ADDRESS, &tmp);
	if (rc) {
		chg_err("Couldn't read REG0B_SY6974B_ADDRESS rc = %d\n", rc);
		return;
	}

	switch (tmp & REG0B_SY6974B_PN_MASK) {
	case REG0B_SY6974B_PN:
		chip->is_sy6974b = true;
		chg_err("is sy6974b\n");
		break;
	case REG0B_BQ25601D_PN:
		chip->is_bq25601d = true;
		chg_err("is bq25601d\n");
		break;
	default:
		chg_err("not support REG0B:[0x%02x]\n", tmp);
		break;
	}
}


static void sy6974b_init_work_handler(struct work_struct *work)
{
	struct chip_sy6974b *chip= NULL;

	if (charger_ic) {
		chip = charger_ic;

		sy6974b_irq_handler(0, chip);
#ifndef CONFIG_OPLUS_CHARGER_MTK
		if (oplus_chg_is_usb_present())
			sy6974b_irq_handler(0, chip);
#endif
	}

	return;
}

static void register_charger_devinfo(struct chip_sy6974b *chip)
{
#ifndef CONFIG_DISABLE_OPLUS_FUNCTION
	int ret = 0;
	char *version;
	char *manufacture;

	if (!chip) {
		chg_err("chip is null\n");
		return;
	}
	if (chip->is_sy6974b) {
		version = "sy6974b";
		manufacture = "Silergy Corp.";
	} else if (chip->is_bq25601d) {
		version = "bq25601d";
		manufacture = "Texas Instruments";
	} else {
		version = "unknown";
		manufacture = "UNKNOWN";
	}
	ret = register_device_proc("charger", version, manufacture);
	if (ret) {
		pr_err("register_charger_devinfo fail\n");
	}
#endif
}
#ifdef CONFIG_OPLUS_CHARGER_MTK
static enum power_supply_usb_type sy6974b_charger_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_DRP,
	POWER_SUPPLY_USB_TYPE_APPLE_BRICK_ID
};

static enum power_supply_property sy6974b_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
};

static int sy6974b_charger_get_online(struct chip_sy6974b *chip, bool *val)
{
	bool pwr_rdy = false;
	int ret = 0;
	int reg_val;
	ret = sy6974b_read_reg(chip, REG08_SY6974B_ADDRESS, &reg_val);
	if (0 == ret) {
		pwr_rdy = !!(reg_val & REG08_SY6974B_POWER_GOOD_STAT_MASK);
	}

	if ((oplus_vooc_get_fastchg_started() == TRUE) && (pwr_rdy == 0))
		pwr_rdy = 1;

	pr_info("reg_val:%x, online = %d\n", reg_val, pwr_rdy);
	*val = pwr_rdy;
	return 0;
}
static int sy6974b_charger_get_property(struct power_supply *psy,
						   enum power_supply_property psp,
						   union power_supply_propval *val)
{
	struct chip_sy6974b *chip = power_supply_get_drvdata(psy);
	bool pwr_rdy = false;
	int ret = 0;
	int boot_mode = get_boot_mode();

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = sy6974b_charger_get_online(chip, &pwr_rdy);
		val->intval = pwr_rdy;
		break;
	case POWER_SUPPLY_PROP_TYPE:
	case POWER_SUPPLY_PROP_USB_TYPE:
		if (boot_mode == META_BOOT) {
			val->intval = POWER_SUPPLY_TYPE_USB;
		} else {
			val->intval = charger_ic->oplus_charger_type;
		}
		pr_info("sy6974b get power_supply_type = %d\n", val->intval);
		break;
	default:
		ret = -ENODATA;
	}
	return ret;
}

static char *sy6974b_charger_supplied_to[] = {
	"battery",
	"mtk-master-charger"
};

static const struct power_supply_desc sy6974b_charger_desc = {
	.type			= POWER_SUPPLY_TYPE_USB,
	.usb_types      = sy6974b_charger_usb_types,
	.num_usb_types  = ARRAY_SIZE(sy6974b_charger_usb_types),
	.properties 	= sy6974b_charger_properties,
	.num_properties 	= ARRAY_SIZE(sy6974b_charger_properties),
	.get_property		= sy6974b_charger_get_property,
};
static int sy6974b_chg_init_psy(struct chip_sy6974b *chip)
{
	struct power_supply_config cfg = {
		.drv_data = chip,
		.of_node = chip->dev->of_node,
		.supplied_to = sy6974b_charger_supplied_to,
		.num_supplicants = ARRAY_SIZE(sy6974b_charger_supplied_to),
	};

	pr_err("%s\n", __func__);
	memcpy(&chip->psy_desc, &sy6974b_charger_desc, sizeof(chip->psy_desc));
	chip->psy_desc.name = "sy6974b";
	chip->psy = devm_power_supply_register(chip->dev, &chip->psy_desc,
						&cfg);
	return IS_ERR(chip->psy) ? PTR_ERR(chip->psy) : 0;
}
static int pd_tcp_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	struct chip_sy6974b * chip =
		(struct chip_sy6974b *)container_of(nb,
		struct chip_sy6974b, pd_nb);

	switch (event) {
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.old_state == TYPEC_UNATTACHED &&
		    noti->typec_state.new_state == TYPEC_ATTACHED_SNK) {
			chg_debug("USB Plug in\n");
			power_supply_changed( chip->psy);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SNK
			&& noti->typec_state.new_state == TYPEC_UNATTACHED) {
			sy6974b_unsuspend_charger();
			chg_debug("USB Plug out\n");
			if (oplus_vooc_get_fast_chg_type() == CHARGER_SUBTYPE_FASTCHG_VOOC && oplus_chg_get_wait_for_ffc_flag() != true) {
				chip->power_good = 0;
				chip->bc12_done = false;
				chip->bc12_retried = 0;
				chip->bc12_delay_cnt = 0;
				chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
				sy6974b_disable_charging();
				oplus_chg_set_charger_type_unknown();
				oplus_vooc_set_fastchg_type_unknow();
				sy6974b_inform_charger_type(chip);
				oplus_vooc_reset_fastchg_after_usbout();
				oplus_chg_clear_chargerid_info();
				Charger_Detect_Release();
				oplus_chg_wake_update_work();
				oplus_chg_wakelock(chip, false);
				chg_debug("usb real remove vooc fastchg clear flag!\n");
			}
		}
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static int sy6974b_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct chip_sy6974b *chip = NULL;
	int ret = 0;

	chg_err("sy6974b probe enter\n");
	chip = devm_kzalloc(&client->dev, sizeof(struct chip_sy6974b), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	charger_ic = chip;
	chip->dev = &client->dev;
	chip->client = client;

	i2c_set_clientdata(client, chip);
	mutex_init(&chip->i2c_lock);
	mutex_init(&chip->dpdm_lock);
	atomic_set(&chip->driver_suspended, 0);
	atomic_set(&chip->charger_suspended, 0);
	oplus_chg_awake_init(chip);
	init_waitqueue_head(&chip->wait);
	oplus_keep_resume_awake_init(chip);
	Charger_Detect_Init();
	chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->power_good = false;
	chip->before_suspend_icl = 0;
	chip->before_unsuspend_icl = 0;
	chip->is_sy6974b = false;
	chip->is_bq25601d = false;
	chip->bc12_done = false;
	chip->bc12_retried = 0;
	chip->bc12_delay_cnt = 0;
	chip->chg_consumer =
		charger_manager_get_by_name(&client->dev, "sy6974b");


	ret = sy6974b_parse_dt(chip->dev->of_node,chip);
	if (ret) {
		chg_err("Couldn't parse device tree ret=%d\n", ret);
		goto err_parse_dt;
	}

	bq25601_hw_component_detect(chip);
	if (!chip->is_sy6974b && !chip->is_bq25601d) {
		chg_err("not support\n");
		ret = -ENOTSUPP;
		goto err_parse_dt;
	}
	register_charger_devinfo(chip);
	sy6974b_dump_registers();
	sy6974b_reset_charger();
	sy6974b_hardware_init();
	sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);

	ret = sy6974b_chg_init_psy(chip);
	if (ret < 0) {
		pr_err("failed to init power supply\n");
		goto err_register_psy;
	}

	chip->chg_dev = charger_device_register(chip->chg_dev_name,
					      &client->dev, chip,
					      &sy6974b_chg_ops,
					      &sy6974b_chg_props);
	if (IS_ERR_OR_NULL(chip->chg_dev)) {
		ret = PTR_ERR(chip->chg_dev);
		chg_err("sy6974b charger_device_register fail\n");
		goto err_device_register;
	}

	ret = sy6974b_irq_register(chip);
	if (ret) {
		chg_err("Failed to register irq ret=%d\n", ret);
		goto err_irq;
	}

	ret = device_create_file(chip->dev, &dev_attr_sy6974b_access);
	if (ret) {
		chg_err("create sy6974b_access file fail ret=%d\n", ret);
		goto err_create_file;
	}
	INIT_DELAYED_WORK(&chip->bc12_retry_work, sy6974b_bc12_retry_work);
	INIT_DELAYED_WORK(&chip->init_work, sy6974b_init_work_handler);

	if (strcmp(chip->chg_dev_name, "primary_chg") == 0) {
		chip->tcpc = tcpc_dev_get_by_name("type_c_port0");
		if (!chip->tcpc) {
			chr_err("%s get tcpc device type_c_port0 fail\n", __func__);
		}
	}

	chip->pd_nb.notifier_call = pd_tcp_notifier_call;
	ret = register_tcp_dev_notifier(chip->tcpc, &chip->pd_nb, TCP_NOTIFY_TYPE_ALL);
	if (ret < 0) {
		pr_notice("register tcpc notifer fail\n");
		ret = -EINVAL;
		goto err_register_tcp_notifier;
	}

	if (NORMAL_BOOT == get_boot_mode())
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(chip->normal_init_delay_ms));
	else
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(chip->other_init_delay_ms));

	if (oplus_daily_build()
			|| get_eng_version() == HIGH_TEMP_AGING
			|| get_eng_version() == AGING)
		sy6974b_debug |= ENABLE_DUMP_LOG;
	set_charger_ic(BQ2560X);
	chr_err("%s successful\n", __func__);
	return 0;

err_register_tcp_notifier:
err_create_file:
	charger_device_unregister(chip->chg_dev);
err_irq:
	device_remove_file(chip->dev, &dev_attr_sy6974b_access);
err_device_register:
err_register_psy:
err_parse_dt:
	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&chip->i2c_lock);
	charger_ic = NULL;
	devm_kfree(chip->dev, chip);
	return ret;
}
#else
static int sy6974b_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct chip_sy6974b *chip = NULL;
	int ret = 0;

	chg_err("sy6974b probe enter\n");
	chip = devm_kzalloc(&client->dev, sizeof(struct chip_sy6974b), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	charger_ic = chip;
	chip->dev = &client->dev;
	chip->client = client;

	i2c_set_clientdata(client, chip);
	mutex_init(&chip->i2c_lock);
	mutex_init(&chip->dpdm_lock);
	INIT_DELAYED_WORK(&chip->bc12_retry_work, sy6974b_bc12_retry_work);
	atomic_set(&chip->driver_suspended, 0);
	atomic_set(&chip->charger_suspended, 0);
	oplus_chg_awake_init(chip);
	init_waitqueue_head(&chip->wait);
	oplus_keep_resume_awake_init(chip);
	chip->oplus_charger_type = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->power_good = false;
	chip->before_suspend_icl = 0;
	chip->before_unsuspend_icl = 0;
	chip->is_sy6974b = false;
	chip->is_bq25601d = false;
	chip->bc12_done = false;
	chip->bc12_retried = 0;
	chip->bc12_delay_cnt = 0;

	if (!chip->dpdm_reg && of_get_property(chip->dev->of_node,
				"dpdm-supply", NULL)) {
		chip->dpdm_reg = devm_regulator_get(chip->dev, "dpdm");
		if (IS_ERR(chip->dpdm_reg)) {
			ret = PTR_ERR(chip->dpdm_reg);
			chg_err("Couldn't get dpdm regulator ret=%d\n", ret);
			chip->dpdm_reg = NULL;
			goto err_parse_dt;
		}
	}

	ret = sy6974b_parse_dt(chip);
	if (ret) {
		chg_err("Couldn't parse device tree ret=%d\n", ret);
		goto err_parse_dt;
	}

	bq25601_hw_component_detect(chip);
	if (!chip->is_sy6974b && !chip->is_bq25601d) {
		chg_err("not support\n");
		ret = -ENOTSUPP;
		goto err_parse_dt;
	}
	register_charger_devinfo(chip);
	oplus_chg_ops_register("ext-sy6974b", &sy6974b_chg_ops);
	sy6974b_dump_registers();
	sy6974b_reset_charger();
	sy6974b_hardware_init();
	sy6974b_set_wdt_timer(REG05_SY6974B_WATCHDOG_TIMER_DISABLE);
	ret = device_create_file(chip->dev, &dev_attr_sy6974b_access);
	if (ret) {
		chg_err("create sy6974b_access file fail ret=%d\n", ret);
		goto err_create_file;
	}

	ret = sy6974b_irq_register(chip);
	if (ret) {
		chg_err("Failed to register irq ret=%d\n", ret);
		goto err_irq;
	}

	INIT_DELAYED_WORK(&chip->init_work, sy6974b_init_work_handler);

	if (MSM_BOOT_MODE__NORMAL == get_boot_mode())
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(chip->normal_init_delay_ms));
	else
		schedule_delayed_work(&chip->init_work, msecs_to_jiffies(chip->other_init_delay_ms));

	if (oplus_daily_build()
			|| get_eng_version() == HIGH_TEMP_AGING
			|| get_eng_version() == AGING)
		sy6974b_debug |= ENABLE_DUMP_LOG;
	sy6974b_irq_handler(0,chip);
	return 0;

err_irq:
err_create_file:
	device_remove_file(chip->dev, &dev_attr_sy6974b_access);
err_parse_dt:
	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&chip->i2c_lock);
	charger_ic = NULL;
	return ret;
}
#endif
static int sy6974b_charger_remove(struct i2c_client *client)
{
	struct chip_sy6974b *chip = i2c_get_clientdata(client);

	mutex_destroy(&chip->dpdm_lock);
	mutex_destroy(&chip->i2c_lock);

	return 0;
}

static unsigned long suspend_tm_sec = 0;
static int get_rtc_time(unsigned long *rtc_time)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("Failed to open rtc device (%s)\n",
				CONFIG_RTC_HCTOSYS_DEVICE);
		return -EINVAL;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Failed to read rtc time (%s) : %d\n",
				CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n",
				CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}
	rtc_tm_to_time(&tm, rtc_time);

close_time:
	rtc_class_close(rtc);
	return rc;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static int sy6974b_pm_resume(struct device *dev)
{
	unsigned long resume_tm_sec = 0;
	unsigned long sleep_time = 0;
	int rc = 0;
	struct chip_sy6974b *chip = NULL;
	struct i2c_client *client = to_i2c_client(dev);

	pr_err("+++ complete %s: enter +++\n", __func__);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			atomic_set(&chip->driver_suspended, 0);
			wake_up_interruptible(&charger_ic->wait);
			rc = get_rtc_time(&resume_tm_sec);
			if (rc || suspend_tm_sec == -1) {
				chg_err("RTC read failed\n");
				sleep_time = 0;
			} else {
				sleep_time = resume_tm_sec - suspend_tm_sec;
			}
			if ((resume_tm_sec > suspend_tm_sec) && (sleep_time > 60)) {
				oplus_chg_soc_update_when_resume(sleep_time);
			}
		}
	}

	pr_err("+++ complete %s: exit +++\n", __func__);
	return 0;
}

static int sy6974b_pm_suspend(struct device *dev)
{
	struct chip_sy6974b *chip = NULL;
	struct i2c_client *client = to_i2c_client(dev);

	pr_err("+++ prepare %s: enter +++\n", __func__);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			atomic_set(&chip->driver_suspended, 1);
			if (get_rtc_time(&suspend_tm_sec)) {
				chg_err("RTC read failed\n");
				suspend_tm_sec = -1;
			}
		}
	}

	pr_err("+++ prepare %s: exit +++\n", __func__);
	return 0;
}

static const struct dev_pm_ops sy6974b_pm_ops = {
	.resume			= sy6974b_pm_resume,
	.suspend		= sy6974b_pm_suspend,
};
#else
static int sy6974b_resume(struct i2c_client *client)
{
	unsigned long resume_tm_sec = 0;
	unsigned long sleep_time = 0;
	int rc = 0;
	struct chip_sy6974b *chip = i2c_get_clientdata(client);

	if(!chip) {
		return 0;
	}

	atomic_set(&chip->driver_suspended, 0);
	rc = get_rtc_time(&resume_tm_sec);
	if (rc || suspend_tm_sec == -1) {
		chg_err("RTC read failed\n");
		sleep_time = 0;
	} else {
		sleep_time = resume_tm_sec - suspend_tm_sec;
	}
	if ((resume_tm_sec > suspend_tm_sec) && (sleep_time > 60)) {
		oplus_chg_soc_update_when_resume(sleep_time);
	}

	return 0;
}

static int sy6974b_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct chip_sy6974b *chip = i2c_get_clientdata(client);

	if(!chip) {
		return 0;
	}

	atomic_set(&chip->driver_suspended, 1);
	if (get_rtc_time(&suspend_tm_sec)) {
		chg_err("RTC read failed\n");
		suspend_tm_sec = -1;
	}

	return 0;
}
#endif

static void sy6974b_charger_shutdown(struct i2c_client *client)
{
	struct oplus_chg_chip *g_oplus_chip = oplus_chg_get_chg_struct();

	sy6974b_otg_disable();
	sy6974b_unsuspend_charger();
	if (g_oplus_chip && g_oplus_chip->chg_ops && g_oplus_chip->chg_ops->set_typec_sinkonly) {
		g_oplus_chip->chg_ops->set_typec_sinkonly();
		chg_err("sy697b_charger_shutdown disable adc and otg sinkonly\n!");
	}
}

static struct of_device_id sy6974b_charger_match_table[] = {
	{.compatible = "oplus,sy6974b",},
	{},
};

MODULE_DEVICE_TABLE(of, sy6974b_charger_match_table);

static const struct i2c_device_id sy6974b_i2c_device_id[] = {
	{ "sy6974b", 0x6b },
	{ },
};

MODULE_DEVICE_TABLE(i2c, sy6974b_i2c_device_id);

static struct i2c_driver sy6974b_charger_driver = {
	.driver = {
		.name = "sy6974b-charger",
		.owner = THIS_MODULE,
		.of_match_table = sy6974b_charger_match_table,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
		.pm 	= &sy6974b_pm_ops,
#endif
	},

	.probe = sy6974b_charger_probe,
	.remove = sy6974b_charger_remove,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	.resume		= sy6974b_resume,
	.suspend	= sy6974b_suspend,
#endif
	.shutdown = sy6974b_charger_shutdown,
	.id_table = sy6974b_i2c_device_id,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
module_i2c_driver(sy6974b_charger_driver);
#else
void sy6974b_charger_exit(void)
{
	i2c_del_driver(&sy6974b_charger_driver);
}

int sy6974b_charger_init(void)
{
	int ret = 0;
	chg_err(" init start\n");

	if (i2c_add_driver(&sy6974b_charger_driver) != 0) {
		chg_err(" failed to register sy6974b i2c driver.\n");
	} else {
		chg_debug(" Success to register sy6974b i2c driver.\n");
	}
	return ret;
}
#endif

MODULE_DESCRIPTION("SY6974B Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SY");
