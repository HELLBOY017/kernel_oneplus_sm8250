// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>

#ifdef CONFIG_OPLUS_CHARGER_MTK


#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include <linux/module.h>
//#include <upmu_common.h>
//nclude <mt-plat/mtk_gpio.h>
//#include <mtk_boot_common.h>
#include <mt-plat/mtk_rtc.h>
//#include <mt-plat/charging.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
#include <mt-plat/charger_type.h>
#else
#include <mt-plat/v1/charger_type.h>
#endif
#include <soc/oplus/device_info.h>

extern void mt_power_off(void);
#else

#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/rtc.h>
#include <soc/oplus/device_info.h>


#endif

#include "../oplus_vooc.h"
#include "../oplus_gauge.h"
#include <oplus_max77932b.h>
#include <linux/proc_fs.h>

static struct chip_max77932b *the_chip = NULL;
static DEFINE_MUTEX(MAX77932B_i2c_access);

static int __max77932b_read_reg(int reg, int *returnData)
{
    int ret = 0;
    int retry = 3;
    struct chip_max77932b *chip = the_chip;

    ret = i2c_smbus_read_byte_data(chip->client, reg);
    if (ret < 0) {
        while(retry > 0) {
            msleep(10);
            ret = i2c_smbus_read_byte_data(chip->client, reg);
            if (ret < 0) {
                retry--;
            } else {
                *returnData = ret;
                return 0;
            }
        }
        chg_err("i2c read fail: can't read from %02x: %d\n", reg, ret);
        return ret;
    } else {
        *returnData = ret;
    }

    return 0;
}

static int max77932b_read_reg(int reg, int *returnData)
{
    int ret = 0;

    mutex_lock(&MAX77932B_i2c_access);
    ret = __max77932b_read_reg(reg, returnData);
    mutex_unlock(&MAX77932B_i2c_access);
    return ret;
}

static int __max77932b_write_reg(int reg, int val)
{
    int ret = 0;
    struct chip_max77932b *chip = the_chip;
	int retry = 3;

    ret = i2c_smbus_write_byte_data(chip->client, reg, val);

	if (ret < 0) {
		while(retry > 0) {
			usleep_range(5000, 5000);
			ret = i2c_smbus_write_byte_data(chip->client, reg, val);
			if (ret < 0) {
				retry--;
			} else {
				break;
			}
		}
	}

    if (ret < 0) {
        chg_err("i2c write fail: can't write %02x to %02x: %d\n",
        val, reg, ret);
        return ret;
    }

    return 0;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
/*
static int max77932b_read_interface (int RegNum, int *val, int MASK, int SHIFT)
{
    int max77932b_reg = 0;
    int ret = 0;

   //chg_err("--------------------------------------------------\n");
	
    ret = MAX77932B_read_reg(RegNum, &max77932b_reg);
	
   //chg_err(" Reg[%x]=0x%x\n", RegNum, max77932b_reg);
	
    max77932b_reg &= (MASK << SHIFT);
    *val = (max77932b_reg >> SHIFT);
	
   //chg_err(" val=0x%x\n", *val);
	
    return ret;
}
*/

static int max77932b_config_interface (int RegNum, int val, int MASK)
{
    int max77932b_reg = 0;
    int ret = 0;

    mutex_lock(&MAX77932B_i2c_access);
    ret = __max77932b_read_reg(RegNum, &max77932b_reg);

    if (ret >= 0) {
        max77932b_reg &= ~MASK;
        max77932b_reg |= val;
#ifndef CONFIG_OPLUS_MAX77932B	//lkl modify need to check
        if (RegNum == MAX77932B_SCC_CFG1) {
            if ((max77932b_reg & 0x01) == 0) {
                chg_err("[MAX77932B_SCC_CFG1] can't write 0 to bit0, max77932b_reg[0x%x] bug here\n", max77932b_reg);
                dump_stack();
                max77932b_reg |= 0x01;
            }
        }
#endif
        ret = __max77932b_write_reg(RegNum, max77932b_reg);
    }
    __max77932b_read_reg(RegNum, &max77932b_reg);

    mutex_unlock(&MAX77932B_i2c_access);

    return ret;
}

void max77932b_dump_registers(void)
{
#ifdef CONFIG_OPLUS_MAX77932B
    int rc;
    int addr;

    unsigned int val_buf[MAX77932B_REG_NUMBER] = {0x0};

    for (addr = MAX77932B_FIRST_REG; addr <= MAX77932B_LAST_REG; addr++) {
        rc = max77932b_read_reg(addr, &val_buf[addr]);
        if (rc) {
            chg_err("Couldn't read 0x%02x rc = %d\n", addr, rc);
        } else {
            pr_err("max77932b_reg[%d] = 0x%x\n", addr, val_buf[addr]);
        }
    }
#else
    int rc;
    int addr;

    unsigned int val_buf[max77932b_reg2_NUMBER] = {0x0};

    for (addr = MAX77932B_FIRST_REG; addr <= MAX77932B_LAST_REG; addr++) {
        rc = max77932b_read_reg(addr, &val_buf[addr]);
        if (rc) {
            chg_err("Couldn't read 0x%02x rc = %d\n", addr, rc);
        } else {
            pr_err("%s success addr = %d, value = 0x%x\n", 
            __func__, addr, val_buf[addr]);
        }
    }

    for (addr = MAX77932B_FIRST2_REG; addr <= MAX77932B_LAST2_REG; addr++) {
        rc = max77932b_read_reg(addr, &val_buf[addr]);
        if (rc) {
            chg_err("Couldn't read 0x%02x rc = %d\n", addr, rc);
        }
    }
#endif	
}
/*
static int max77932b_work_mode_set(int work_mode)
{
    int rc = 0;
    struct chip_max77932b *divider_ic = the_chip;

    if(divider_ic == NULL) {
        chg_err("%s: MAX77932B driver is not ready\n", __func__);
        return rc;
    }
    if (atomic_read(&divider_ic->suspended) == 1) {
        return 0;
    }
    if(divider_ic->fixed_mode_set_by_dev_file == true 
       && work_mode == MAX77932B_WORK_MODE_AUTO) {
        chg_err("%s: maybe in high GSM work fixed mode ,return here\n", __func__);
        return rc;
    }

    if (oplus_vooc_get_allow_reading() == false) {
        return -1;
    }
    chg_err("%s: work_mode [%d]\n", __func__, work_mode);
    if(work_mode != 0) {
        rc = max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_AUTO, MAX77932B_FIX_FREQ_MASK);
    } else {
        rc = max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_FIX, MAX77932B_FIX_FREQ_MASK);
    }
    return rc;
}

int oplus_set_divider_work_mode(int work_mode)
{
    return max77932b_work_mode_set(work_mode);
}
EXPORT_SYMBOL(oplus_set_divider_work_mode);
*/
int max77932b_hardware_init(void)
{
    int rc = 0;
    struct chip_max77932b *divider_ic = the_chip;

    if(divider_ic == NULL) {
        chg_err("%s: MAX77932B driver is not ready\n", __func__);
        return 0;
    }
    if (atomic_read(&divider_ic->suspended) == 1) {
        return 0;
    }

    rc = max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_AUTO, MAX77932B_FIX_FREQ_MASK);
    return rc;
}
static ssize_t proc_work_mode_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    uint8_t ret = 0;
    char page[10];
    int work_mode = 0;
    struct chip_max77932b *divider_ic = the_chip;

    if(divider_ic == NULL) {
        chg_err("%s: MAX77932B driver is not ready\n", __func__);
        return 0;
    }
    if (atomic_read(&divider_ic->suspended) == 1) {
        return 0;
    }

    if (oplus_vooc_get_allow_reading() == false) {
        return 0;
    }

    ret = max77932b_read_reg(MAX77932B_SCC_CFG1, &work_mode);
    work_mode = ((work_mode & 0x02)? 1:0);

    chg_err("%s: work_mode = %d.\n", __func__, work_mode);
    sprintf(page, "%d", work_mode);
    ret = simple_read_from_buffer(buf, count, ppos, page, strlen(page));

    return ret;
}

static ssize_t proc_work_mode_write(struct file *file, const char __user *buf, size_t count, loff_t *lo)
{
   char buffer[2] = {0};
    int work_mode = 0;
    struct chip_max77932b *divider_ic = the_chip;

    chg_err("%s: start.count[%ld]\n", __func__, count);
    if(divider_ic == NULL) {
        chg_err("%s: MAX77932B driver is not ready\n", __func__);
        return 0;
    }

    if (atomic_read(&divider_ic->suspended) == 1) {
        return 0;
    }

    if (copy_from_user(buffer, buf, 2)) {
        chg_err("%s: read proc input error.\n", __func__);
        return count;
    }

    if (1 == sscanf(buffer, "%d", &work_mode)) {
        chg_err("%s:new work_mode = %d.\n", __func__, work_mode);
    } else {
        chg_err("invalid content: '%s', length = %zd\n", buf, count);
    }

    if (work_mode != 0) {
        divider_ic->fixed_mode_set_by_dev_file = false;
    } else {
        divider_ic->fixed_mode_set_by_dev_file = true;
    }

    if (oplus_vooc_get_allow_reading() == false) {
        return 0;
    }

    if (work_mode != 0) {
        max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_AUTO, MAX77932B_FIX_FREQ_MASK);
    } else {
        max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_FIX, MAX77932B_FIX_FREQ_MASK);
    }

    return count;
}

static const struct file_operations proc_work_mode_ops =
{
    .read = proc_work_mode_read,
    .write  = proc_work_mode_write,
    .open  = simple_open,
    .owner = THIS_MODULE,
};

static int init_max77932b_proc(struct chip_max77932b *da)
{
    int ret = 0;
    struct proc_dir_entry *prEntry_da = NULL;
    struct proc_dir_entry *prEntry_tmp = NULL;

    prEntry_da = proc_mkdir("max77932b", NULL);
    if (prEntry_da == NULL) {
        ret = -ENOMEM;
        chg_debug("%s: Couldn't create MAX77932B proc entry\n", __func__);
    }

    prEntry_tmp = proc_create_data("work_mode", 0644, prEntry_da, &proc_work_mode_ops, da);
    if (prEntry_tmp == NULL) {
        ret = -ENOMEM;
        chg_debug("%s: Couldn't create proc entry, %d\n", __func__, __LINE__);
    }
    return 0;
}

static int max77932b_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    struct chip_max77932b *divider_ic;

    divider_ic = devm_kzalloc(&client->dev, sizeof(struct chip_max77932b), GFP_KERNEL);
    if (!divider_ic) {
        dev_err(&client->dev, "failed to allocate divider_ic\n");
        return -ENOMEM;
    }

    chg_debug( " call \n");
    divider_ic->client = client;
    divider_ic->dev = &client->dev;
    the_chip = divider_ic;
    divider_ic->fixed_mode_set_by_dev_file = false;

    max77932b_dump_registers();

    max77932b_hardware_init();
    init_max77932b_proc(divider_ic);

    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static int max77932b_pm_resume(struct device *dev)
{
        if (!the_chip) {
                return 0;
        }
        atomic_set(&the_chip->suspended, 0);
        return 0;
}

static int max77932b_pm_suspend(struct device *dev)
{
        if (!the_chip) {
                return 0;
        }
        atomic_set(&the_chip->suspended, 1);
        return 0;
}

static const struct dev_pm_ops max77932b_pm_ops = {
        .resume                = max77932b_pm_resume,
        .suspend                = max77932b_pm_suspend,
};
#else /*(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))*/
static int max77932b_resume(struct i2c_client *client)
{
        if (!the_chip) {
            return 0;
        }
        atomic_set(&the_chip->suspended, 0);
        return 0;
}

static int max77932b_suspend(struct i2c_client *client, pm_message_t mesg)
{
        if (!the_chip) {
            return 0;
        }
        atomic_set(&the_chip->suspended, 1);
        return 0;
}
#endif /*(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))*/

static struct i2c_driver max77932b_i2c_driver;

static int max77932b_driver_remove(struct i2c_client *client)
{
    int ret=0;

    chg_debug( "  ret = %d\n", ret);
    return 0;
}

static void max77932b_shutdown(struct i2c_client *client)
{
    int rc = 0;
    struct chip_max77932b *divider_ic = the_chip;

    if(divider_ic == NULL) {
        chg_err("%s: MAX77932B driver is not ready\n", __func__);
        return ;
    }
    if (atomic_read(&divider_ic->suspended) == 1) {
        return ;
    }

    rc = max77932b_config_interface(MAX77932B_SCC_CFG1, MAX77932B_FIX_FREQ_AUTO, MAX77932B_FIX_FREQ_MASK);

    return ;
}


/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/

static const struct of_device_id max77932b_match[] = {
    { .compatible = "oplus,MAX77932B-divider"},
    { },
};

static const struct i2c_device_id max77932b_id[] = {
    {"MAX77932B-divider", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, max77932b_id);


static struct i2c_driver max77932b_i2c_driver = {
    .driver		= {
        .name = "MAX77932B-divider",
        .owner	= THIS_MODULE,
        .of_match_table = max77932b_match,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
        .pm = &max77932b_pm_ops,
#endif
    },
    .probe		= max77932b_driver_probe,
    .remove		= max77932b_driver_remove,
    .id_table	= max77932b_id,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
    .resume         = max77932b_resume,
    .suspend        = max77932b_suspend,
#endif
	.shutdown	= max77932b_shutdown,
};


module_i2c_driver(max77932b_i2c_driver);
MODULE_DESCRIPTION("Driver for MAX77932B divider chip");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("i2c:MAX77932B-divider");
