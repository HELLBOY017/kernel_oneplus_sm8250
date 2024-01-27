/*
 * TI TPS65132 Driver
 *
 * Copyright 2018 Texas Instruments
 *
 * Author: LiPing-m
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pwm.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <soc/oplus/device_info.h>
#include <soc/oplus/oplus_project.h>
#include <soc/oplus/boot_mode.h>
#include <linux/regmap.h>


/* Registers */
#define TPS65132_REG_OUTPUT_CFG       0x10

#define TPS65132_REG_BRT_CFG      0x16

#define TPS65132_REG_BOOST_CTL    0x1A
#define TPS65132_REG_BOOST        0x06

#define TPS65132_REG_PWM_CFG      0x1C

#define TPS65132_REG_IMAX_A       0x17
#define TPS65132_REG_IMAX_B       0x18

#define TPS65132_REG_BRT_A_LSB        0x00
#define TPS65132_REG_BRT_A_MSB        0x01
#define TPS65132_REG_BRT_B_LSB        0x22
#define TPS65132_REG_BRT_B_MSB        0x23
#define TPS65132_BRT_LSB_MASK     (BIT(0) | BIT(1) | BIT(2))
#define TPS65132_BRT_MSB_SHIFT        3

#define TPS65132_REG_ENABLE       0x24

/* Other definitions */
#define TPS65132_PWM_ID           1
#define TPS65132_MAX_REGISTERS    0xB4
#define TPS65132_MAX_STRINGS      3
#define TPS65132_MAX_BRIGHTNESS       2047
#define TPS65132_IMAX_OFFSET      6

#define TPS65132_EXPONENTIAL      1
#define TPS65132_LINEAR           0
#define TPS65132_LEVEL_2048_SUPPROT    1

#define REG_PWM     0x1C
#define TPS65132_MIN_BRIGHTNESS 6
#define FILTER_STR 0x50
#define RUN_RAMP 0x08
#define REG_REVISION 0x1F


struct tps65132_pw_platform_data {
    const char *name;
    unsigned long bl_string;
    unsigned int init_brightness;

    /* Only valid in case of PWM mode */
    unsigned int pwm_period;
};

/*
 * struct TPS65132_platform_data
 * @enn_gpio: GPIO for HWEN pin
 * @bl_pdata: Backlight platform data
 * @num_backlights: Number of backlight outputs
 */
struct tps65132_platform_data {
    int enn_gpio;
    int enp_gpio;
    struct tps65132_pw_platform_data *bl_pdata;
    int num_backlights;
};
enum TPS65132_bl_ctrl_mode {
    BL_REGISTER_BASED,
    BL_PWM_BASED,
};

/*
 * struct TPS65132_pw_chip
 * @dev: Parent device structure
 * @regmap: Used for I2C register access
 * @pdata: TPS65132 platform data
 */
struct TPS65132_pw_chip {
    struct device *dev;
    struct tps65132_platform_data *pdata;
    struct regmap *regmap;
};

/*
 * struct TPS65132_pw
 * @bank_id: Control bank ID. BANK A or BANK A and B
 * @bl_dev: Backlight device structure
 * @chip: TPS65132 backlight chip structure for low level access
 * @bl_pdata: TPS65132 backlight platform data
 * @mode: Backlight control mode
 * @pwm: PWM device structure. Only valid in PWM control mode
 * @pwm_name: PWM device name
 */
struct TPS65132_pw {
    int bank_id;
    struct backlight_device *bl_dev;
    struct TPS65132_pw_chip *chip;
    struct tps65132_pw_platform_data *bl_pdata;
    enum TPS65132_bl_ctrl_mode mode;
    struct pwm_device *pwm;
    char pwm_name[20];
};

static struct TPS65132_pw_chip *TPS65132_pchip;

static int TPS65132_dt(struct device *dev, struct tps65132_platform_data *pdata)
{
    struct device_node *np = dev->of_node;

	pdata->enn_gpio = of_get_named_gpio(np, "ti,enable_neg-gpio", 0);
	pr_err("%s enn_gpio =%d\n", __func__, pdata->enn_gpio);

    if (!gpio_is_valid(pdata->enn_gpio))
            pr_err("%s:%d, tps65132_enn gpio not specified\n", __func__, __LINE__);

	pdata->enp_gpio = of_get_named_gpio(np, "ti,enable_pos-gpio", 0);
	pr_err("%s enp_gpio =%d\n", __func__, pdata->enp_gpio);

    if (!gpio_is_valid(pdata->enp_gpio))
            pr_err("%s:%d, tps65132_enp gpio not specified\n", __func__, __LINE__);

    return 0;
}

int TPS65132_reg_init(void)
{
    struct TPS65132_pw_chip *pchip = TPS65132_pchip;
    int ret =0;

    if(!pchip || !TPS65132_pchip){
        pr_err("TPS65132_reg_init pdata is null\n");
        return 0;
    }

    if (!pchip->regmap || !TPS65132_pchip->regmap || !TPS65132_pchip->pdata) {
        pr_err("%s  pchip->regmap is NULL.\n", __func__);
        return 0;
    }

    pr_err("%s: init TPS65132 reg\n", __func__);

    ret = regmap_write(pchip->regmap, 0x00, 0x0F);  //5V
        if (ret < 0) {
            pr_err("%s: regmap_write 0x00 failed, ret = %d", __func__, ret);
            goto out;
        }
    ret = regmap_write(pchip->regmap, 0x01, 0x0F);  //-5V
        if (ret < 0) {
            pr_err("%s: regmap_write 0x01 failed, ret = %d", __func__, ret);
            goto out;
        }
    ret = regmap_write(pchip->regmap, 0x03, 0x43);
        if (ret < 0){
            pr_err("%s: regmap_write 0x03 failed, ret = %d", __func__, ret);
            goto out;
        }

    return ret;
out:
    pr_err("i2c failed to access register\n");
    return ret;
}
EXPORT_SYMBOL(TPS65132_reg_init);

void TPS65132_pw_enable(int enable)
{
    struct TPS65132_pw_chip *pchip = TPS65132_pchip;

    if(!pchip || !TPS65132_pchip){
        pr_err("TPS65132_pw_enable pdata is null\n");
        return;
    }
    if (!pchip->regmap || !TPS65132_pchip->regmap || !TPS65132_pchip->pdata) {
        pr_err("%s  pchip->regmap is NULL.\n", __func__);
        return;
    }

    pr_err("mdss %s  = %d\n", __func__, enable);

    if(enable){
        if (gpio_is_valid(pchip->pdata->enn_gpio)){
            gpio_set_value((pchip->pdata->enn_gpio), 1);
        }else{
            pr_err("%s: enable enn_gpio failed", __func__);
        }
        if (gpio_is_valid(pchip->pdata->enp_gpio)){
            gpio_set_value((pchip->pdata->enp_gpio), 1);
        }else{
            pr_err("%s: enable enp_gpio failed", __func__);
        }
    }else{
        if (gpio_is_valid(pchip->pdata->enn_gpio)){
            gpio_set_value((pchip->pdata->enn_gpio), 0);
        }else{
            pr_err("%s: disable enn_gpio failed", __func__);
        }
        if (gpio_is_valid(pchip->pdata->enp_gpio)){
            gpio_set_value((pchip->pdata->enp_gpio), 0);
        }else{
            pr_err("%s: disable enp_gpio failed", __func__);
        }
    }
}
EXPORT_SYMBOL(TPS65132_pw_enable);

static int TPS65132_chip_init(struct TPS65132_pw_chip *pchip){
    int ret = 0;

    ret = regmap_write(pchip->regmap, 0x00, 0x0F);  //5V
        if (ret < 0)
         goto out;
    ret = regmap_write(pchip->regmap, 0x01, 0x0F);  //-5V
        if (ret < 0)
         goto out;
    ret = regmap_write(pchip->regmap, 0x03, 0x03);
        if (ret < 0)
         goto out;

    return ret;

out:
    pr_err("TPS65132_chip_init i2c failed to access register\n");
    return ret;
}

static struct regmap_config TPS65132_regmap = {
    .reg_bits = 8,
    .val_bits = 8,
};

static int TPS65132_bl_probe(struct i2c_client *client,
               const struct i2c_device_id *id)
{
    struct tps65132_platform_data *pdata = client->dev.platform_data;
    struct TPS65132_pw_chip *pchip;
    int ret = 0;

    pr_err("%s Enter\n", __func__);

    if (client->dev.of_node) {
        pdata = devm_kzalloc(&client->dev,
            sizeof(struct tps65132_platform_data), GFP_KERNEL);
        if (!pdata) {
            pr_err("Failed to allocate memory\n");
            return -ENOMEM;
        }

        ret = TPS65132_dt(&client->dev, pdata);
        if (ret)
            return ret;
    } else
        pdata = client->dev.platform_data;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_err("fail : i2c functionality check...\n");
        return -EOPNOTSUPP;
    }

    if (pdata == NULL) {
        pr_err("fail : no platform data.\n");
        return -ENODATA;
    }

    pchip = devm_kzalloc(&client->dev, sizeof(struct TPS65132_pw_chip),
                 GFP_KERNEL);
    if (!pchip) {
		pr_err("no memory to alloc struct TPS65132_pw_chip\n");
        return -ENOMEM;
	}
    TPS65132_pchip = pchip;


    pchip->pdata = pdata;
    pchip->dev = &client->dev;

    ret = gpio_request(pdata->enn_gpio, "tps65132-neg");
    if (ret) {
        pr_err("request tps65132_enn_enable gpio failed, ret=%d\n", ret);
    }
    ret = gpio_request(pdata->enp_gpio, "tps65132-pos");
    if (ret) {
        pr_err("request tps65132_enp_enable gpio failed, ret=%d\n", ret);
    }

    if (gpio_is_valid(pdata->enn_gpio)){
        gpio_set_value((pdata->enn_gpio), 1);
        gpio_direction_output((pdata->enn_gpio), 1);
        pr_err("%s tps65132_enn_enable =%d\n", __func__, pdata->enn_gpio);
    }

    if (gpio_is_valid(pdata->enp_gpio)){
        gpio_set_value((pdata->enp_gpio), 1);
        gpio_direction_output((pdata->enp_gpio), 1);
        pr_err("%s tps65132_enp_enable =%d\n", __func__, pdata->enp_gpio);
    }

    msleep(5);
    pchip->regmap = devm_regmap_init_i2c(client, &TPS65132_regmap);
    if (IS_ERR(pchip->regmap)) {
        ret = PTR_ERR(pchip->regmap);
        pr_err("fail : allocate register map: %d\n",
            ret);
        return ret;
    }

    msleep(5);
    i2c_set_clientdata(client, pchip);

    /* chip initialize */
    msleep(5);
    ret = TPS65132_chip_init(pchip);
    if (ret < 0) {
        pr_err("fail : init chip\n");
        goto error_enable;
    }

    pr_info("%s : probe done\n", __func__);

    return 0;

error_enable:

    devm_kfree(&client->dev, pchip->pdata);
    devm_kfree(&client->dev, pchip);
    pr_err("%s : probe failed\n", __func__);
    return ret;
}

static int TPS65132_bl_remove(struct i2c_client *client){
    struct TPS65132_pw_chip *pchip = i2c_get_clientdata(client);
    int ret = 0;

	if(!pchip || !pchip->regmap || !pchip->pdata) {
		pr_err("%s, have null pointer!\n", __func__);
		return -1;
	}

    dev_info(pchip->dev, "%s :  failed\n", __func__);

    ret = regmap_write(pchip->regmap, TPS65132_REG_BRT_A_LSB, 0);
    if (ret < 0)
        pr_err("i2c failed to access register\n");

    ret = regmap_write(pchip->regmap, TPS65132_REG_BRT_A_MSB, 0);
    if (ret < 0)
        pr_err("i2c failed to access register\n");

    if (gpio_is_valid(pchip->pdata->enn_gpio)){
        gpio_set_value(pchip->pdata->enn_gpio, 0);
        gpio_free(pchip->pdata->enn_gpio);
    }
    if (gpio_is_valid(pchip->pdata->enp_gpio)){
        gpio_set_value(pchip->pdata->enp_gpio, 0);
        gpio_free(pchip->pdata->enp_gpio);
    }

    return 0;
}

static const struct i2c_device_id TPS65132_bl_ids[] = {
    { "ti,tps65132", 0 },
    { }
};

static const struct of_device_id tps65132_of_match[] = {
	{ .compatible = "ti,tps65132",},
	{},
};

static struct i2c_driver TPS65132_i2c_driver = {
    .probe = TPS65132_bl_probe,
    .remove = TPS65132_bl_remove,
    .driver = {
        .name = "tps65132",
        .of_match_table = tps65132_of_match,
        .owner = THIS_MODULE,
    },
    .id_table = TPS65132_bl_ids,
};

module_i2c_driver(TPS65132_i2c_driver);

MODULE_DESCRIPTION("TPS65132 Power Driver");
MODULE_AUTHOR("LiPing-M");
MODULE_LICENSE("GPL");
