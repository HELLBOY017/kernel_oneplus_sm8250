/*
 * LEDs driver for GPIOs
 *
 * Copyright (C) 2007 8D Technologies inc.
 * Raphael Assenat <raph@8d.com>
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>

#include <linux/string.h>
#include "oplus_optiga.h"
#include "Platform/board.h"
#include "ECC/Optiga_Ecc.h"
#include "SWI/Optiga_Nvm.h"
#include "../../oplus_gauge.h"
#include "../../oplus_charger.h"
#include <linux/gpio/consumer.h>

struct oplus_optiga_chip *g_oplus_optiga_chip;
#define DEFAULT_ADDR       0x00
#define TEST_COUNT         10
#define MAX_DEV			   8
#define MAX_NR_GPIO 300

#ifndef CONFIG_OPLUS_CHARGER_MTK
#if 0
/*add for Qcom project*/
#include "../../../../../drivers/gpio/gpiolib.h"
#include <soc/qcom/scm.h>
#include "../../../../../drivers/pinctrl/core.h"
#include "../../../../../drivers/pinctrl/pinconf.h"
#include "../../../../../drivers/pinctrl/qcom/pinctrl-msm.h"
#include "../../../../../drivers/pinctrl/pinctrl-utils.h"
#endif /* 0 */
#endif /*CONFIG_OPLUS_CHARGER_MTK*/
#ifdef CONFIG_OPLUS_OPTIGA_LOW_DELAY
/*add for Qcom project*/
#include "gpiolib.h"
#include <soc/qcom/scm.h>
#include "core.h"
#include "pinconf.h"
#include "pinctrl-msm.h"
#include "pinctrl-utils.h"


#define gpio_reg_set(reg, value) ({__iowmb(); writel_relaxed_no_log(value,reg);})

struct msm_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctrl;
	struct gpio_chip chip;
	struct notifier_block restart_nb;
	int irq;

	raw_spinlock_t lock;

	DECLARE_BITMAP(dual_edge_irqs, MAX_NR_GPIO);
	DECLARE_BITMAP(enabled_irqs, MAX_NR_GPIO);

	const struct msm_pinctrl_soc_data *soc;
	void __iomem *regs;
	void __iomem *pdc_regs;
#ifdef CONFIG_FRAGMENTED_GPIO_ADDRESS_SPACE
	/* For holding per tile virtual address */
	void __iomem *per_tile_regs[4];
#endif
	phys_addr_t spi_cfg_regs;
	phys_addr_t spi_cfg_end;
#ifdef CONFIG_HIBERNATION
	struct msm_gpio_regs *gpio_regs;
	struct msm_tile *msm_tile_regs;
	unsigned int *spi_cfg_regs_val;
#endif
};

void oem_gpio_dir_control(unsigned gpio, int dir)
{
	const struct msm_pingroup *g;
	struct msm_pinctrl *pctrl;
	struct gpio_desc *gpio_desc;
	u32 val;

	gpio_desc = gpio_to_desc(gpio);
	pctrl = container_of(gpio_desc->gdev->chip, struct msm_pinctrl, chip);
	g = &pctrl->soc->groups[gpio];

/*configure gpio to output, can ignor if gpio is already configured to output*/
	if (dir) {
		val = readl(pctrl->regs + g->ctl_reg);
		val |= BIT(g->oe_bit);
		writel(val, pctrl->regs + g->ctl_reg);
	} else {
		val = readl(pctrl->regs + g->ctl_reg);
		val &= ~BIT(g->oe_bit);
		writel(val, pctrl->regs + g->ctl_reg);
	}
}
EXPORT_SYMBOL_GPL(oem_gpio_dir_control);

void oem_gpio_control(unsigned gpio, int level)
{
	const struct msm_pingroup *g;
	struct msm_pinctrl *pctrl;
	struct gpio_desc *gpio_desc;
	void __iomem *io_regs;
	u32 val;

	gpio_desc = gpio_to_desc(gpio);
	pctrl = container_of(gpio_desc->gdev->chip, struct msm_pinctrl, chip);
	g = &pctrl->soc->groups[gpio];
	io_regs = pctrl->regs + g->io_reg;


	val = readl(pctrl->regs + g->ctl_reg);
	val |= BIT(g->oe_bit);
	writel(val, pctrl->regs + g->ctl_reg);

	if (level) {
		gpio_reg_set(io_regs, 3); // output high
	} else {
		gpio_reg_set(io_regs, 0); //output low
	}
}
EXPORT_SYMBOL_GPL(oem_gpio_control);
#endif /* CONFIG_OPLUS_OPTIGA_LOW_DELAY */


static try_count = 0;
void set_optiga_pin(uint8_t level){
	int en = level;


#ifdef CONFIG_OPLUS_OPTIGA_LOW_DELAY
		if (en) {
			oem_gpio_control(g_oplus_optiga_chip->data_gpio, 1);
		} else {
			oem_gpio_control(g_oplus_optiga_chip->data_gpio, 0);
		}
#else
		if (en) {
			gpio_set_value(g_oplus_optiga_chip->data_gpio, 1);
		} else {
			gpio_set_value(g_oplus_optiga_chip->data_gpio, 0);
		}
#endif /* CONFIG_OPLUS_OPTIGA_LOW_DELAY */
}

int get_optiga_pin(void) {
	int ret = 0;

		ret = gpio_get_value(g_oplus_optiga_chip->data_gpio);
		return (uint8_t)ret;
}

void set_optiga_pin_dir(uint8_t dir) {
	int en = dir;

#ifdef CONFIG_OPLUS_OPTIGA_LOW_DELAY
	if (en){
		oem_gpio_control(g_oplus_optiga_chip->data_gpio, 1);
		return;
	} else {
		oem_gpio_dir_control(g_oplus_optiga_chip->data_gpio,0);
	}
#else
	if (en){
		gpio_direction_output(g_oplus_optiga_chip->data_gpio, 1);
		return;
	} else {
		gpio_direction_input(g_oplus_optiga_chip->data_gpio);
	}
#endif /* CONFIG_OPLUS_OPTIGA_LOW_DELAY */
}

int oplus_optiga_parse_dt(struct oplus_optiga_chip *chip)
{
	int rc;
	struct device_node *node = chip->dev->of_node;
	chip->pinctrl = devm_pinctrl_get(chip->dev);
	chip->optiga_active = pinctrl_lookup_state(chip->pinctrl, "optiga_active");
	if (IS_ERR_OR_NULL(chip->optiga_active)) {
		chg_err(": %d Failed to get the optiga_active pinctrl handle\n", __LINE__);
		return -1;
	}else{
		pinctrl_select_state(chip->pinctrl, chip->optiga_active);
		chg_err(": %d set optiga_active pinctrl handle\n", __LINE__);
	}

	chip->data_gpio = of_get_named_gpio(node, "data-gpio", 0);
	if (chip->data_gpio < 0) {
		chg_err("optiga data_gpio not specified\n");
		return -1;
	} else {
		if (gpio_is_valid(chip->data_gpio)) {
			rc = gpio_request(chip->data_gpio, "optiga-data-gpio");
			if (rc) {
				chg_err("unable to request gpio [%d]\n", chip->data_gpio);
				return -1;
			}
		} else {
			chg_err("optiga data_gpio invalid\n");
			return -1;
		}
	}
	rc = of_property_read_u32(node, "cpu-id", &chip->cpu_id);
	if (rc) {
		chip->cpu_id = 7;
	}
	chg_err("cpu-id:%d\n", chip->cpu_id);
	rc = of_property_read_u32(node, "key-id", &chip->key_id);
	if (rc) {
		chip->key_id = 0;
	}
	chg_err("key-id:%d\n", chip->key_id);

	chip->support_optiga_in_lk = of_property_read_bool(node, "support_optiga_in_lk");
	chg_err("support_optiga_in_lk: %d\n", chip->support_optiga_in_lk);
	return 0;
}

int optiga_authenticate(void){
	int ret = false;
	int devloop = 0;
	/*int i = 0;*/
	static S_OPTIGA_PUID stDetectedPuids[MAX_DEV];
	static int first_read_uid = 0;
	unsigned long flags;

	chg_err("optiga_authenticate devloop:%d\n",devloop);
	timing_init();
	/*set_pin_dir(1);
	for(i=0;i<50;i++){
		set_pin(1);
		ic_udelay(10);
		set_pin(0);
		ic_udelay(10);
	}
	for(i=0;i<50;i++){
		set_pin(1);
		ic_udelay(30);
		set_pin(0);
		ic_udelay(30);
	}
	for(i=0;i<50;i++){
		set_pin(1);
		ic_udelay(50);
		set_pin(0);
		ic_udelay(50);
	}*/
	Swi_PowerDown();
	Swi_PowerUp();
	//Swi_Reset();
	spin_lock_irqsave(&g_oplus_optiga_chip->slock, flags);
	Swi_SelectByAddress(DEFAULT_ADDR);

	if (first_read_uid == 0) {
#ifdef BURST_READ
		if(Swi_ReadUID_burst((uint8_t *) &stDetectedPuids[0], HIGH_BYTE_FIRST, BURST_BYTES_READ) != INF_SWI_SUCCESS)
#else
		if (Swi_ReadUID((uint8_t *) &stDetectedPuids[0]) == FALSE) 
#endif
		{
			chg_err("Select By Address Failed\n");
			Swi_PowerDownCmd();
			spin_unlock_irqrestore(&g_oplus_optiga_chip->slock, flags);
			return ret;
		}
	}

	//Swi_SetAddress(1);
	spin_unlock_irqrestore(&g_oplus_optiga_chip->slock, flags);

#ifdef BURST_READ_INTERVAL
	usleep_range(5000, 5000);
#endif

	if (Ecc_DoAuthenticationEnhanced((uint8_t*) &stDetectedPuids[devloop]) == FALSE) {
		chg_err("ECCE Failed\n");
	} else {
		chg_err("ECCE Passed\n");
		first_read_uid = 1;
		ret = true;
	}
	spin_lock_irqsave(&g_oplus_optiga_chip->slock, flags);
	Swi_PowerDownCmd();
	spin_unlock_irqrestore(&g_oplus_optiga_chip->slock, flags);
	return ret;
}

int oplus_optiga_get_test_result(int *count_total, int *count_now, int *fail_count){
	if (!g_oplus_optiga_chip) {
		return -1;
	}
	*count_total = g_oplus_optiga_chip->test_result.test_count_total;
	*count_now = g_oplus_optiga_chip->test_result.test_count_now;
	*fail_count = g_oplus_optiga_chip->test_result.test_fail_count;
	chg_err("count_total:%d,count_now:%d,fail_count:%d,real_count:%d,real_fail:%d\n",
			*count_total, *count_now, *fail_count,
			g_oplus_optiga_chip->test_result.real_test_count_now,
			g_oplus_optiga_chip->test_result.real_test_fail_count);
	return 0;
}

int oplus_optiga_get_hmac_status(int *status, int *fail_count, int *total_count,
	int *real_fail_count, int *real_total_count) {
	if (!g_oplus_optiga_chip) {
		return -1;
	}
	*status = g_oplus_optiga_chip->hmac_status.authenticate_result;
	*fail_count = g_oplus_optiga_chip->hmac_status.fail_count;
	*total_count = g_oplus_optiga_chip->hmac_status.total_count;
	*real_fail_count = g_oplus_optiga_chip->hmac_status.real_fail_count;
	*real_total_count = g_oplus_optiga_chip->hmac_status.real_total_count;
	chg_err("status:%d,fail_count:%d,total_count:%d,real_fail_count:%d,real_total_count:%d\n",
			*status, *fail_count, *total_count, *real_fail_count, *real_total_count);
	return 0;
}

int oplus_optiga_start_test(int count){
	if (!g_oplus_optiga_chip) {
		return -1;
	}
	cancel_delayed_work_sync(&g_oplus_optiga_chip->test_work);
	g_oplus_optiga_chip->test_result.test_count_now = 0;
	g_oplus_optiga_chip->test_result.test_count_total = count;
	g_oplus_optiga_chip->test_result.test_fail_count = 0;
	g_oplus_optiga_chip->test_result.real_test_count_now = 0;
	g_oplus_optiga_chip->test_result.real_test_fail_count = 0;
	schedule_delayed_work_on(7,&g_oplus_optiga_chip->test_work, 0);
	return 0;
}

static void oplus_optiga_auth_work(struct work_struct *work)
{
	int ret = false;
	if (!g_oplus_optiga_chip) {
		return;
	}
	try_count++;
	g_oplus_optiga_chip->test_result.real_test_count_now++;
	g_oplus_optiga_chip->hmac_status.real_total_count++;
	ret = optiga_authenticate();
	if (ret == false) {
		g_oplus_optiga_chip->test_result.real_test_fail_count++;
		g_oplus_optiga_chip->hmac_status.real_fail_count++;
		if (try_count < g_oplus_optiga_chip->try_count) {
			schedule_delayed_work_on(7, &g_oplus_optiga_chip->auth_work, 0);
			return;
		} else {
			complete(&g_oplus_optiga_chip->is_complete);
		}
		try_count = 0;
	} else {
		try_count = 0;
		g_oplus_optiga_chip->hmac_status.authenticate_result = true;
		complete(&g_oplus_optiga_chip->is_complete);
	}
}

int oplus_optiga_auth(void)
{
	if (!g_oplus_optiga_chip) {
		return 0;
	}
	reinit_completion(&g_oplus_optiga_chip->is_complete);
	schedule_delayed_work_on(7, &g_oplus_optiga_chip->auth_work, 0);
	if (!wait_for_completion_timeout(&g_oplus_optiga_chip->is_complete,
			msecs_to_jiffies(5000 * g_oplus_optiga_chip->try_count))) {
		chg_err("time out!\n");
	}
	return g_oplus_optiga_chip->hmac_status.authenticate_result;
}

static void oplus_optiga_test_func(struct work_struct *work)
{
	int ret;
	if (!g_oplus_optiga_chip) {
		return;
	}

	while (g_oplus_optiga_chip->test_result.test_count_now < g_oplus_optiga_chip->test_result.test_count_total) {
		g_oplus_optiga_chip->test_result.test_count_now++;
		g_oplus_optiga_chip->try_count = TEST_COUNT;
		ret = oplus_optiga_auth();
		if (ret == false){
			g_oplus_optiga_chip->test_result.test_fail_count++;
		}
	}
}

#define OPLUS_OPTIGA_RETRY_COUNT    (5)

int oplus_optiga_get_external_auth_hmac(void) {
	int ret;
	if (!g_oplus_optiga_chip) {
		return false;
	}
	if(g_oplus_optiga_chip->hmac_status.authenticate_result == false) {
		g_oplus_optiga_chip->hmac_status.total_count++;
		g_oplus_optiga_chip->try_count = 1;
		ret = oplus_optiga_auth();
		if (ret == false){
			g_oplus_optiga_chip->hmac_status.fail_count++;
		}
	}
	return g_oplus_optiga_chip->hmac_status.authenticate_result;
}

static const struct of_device_id of_oplus_optiga_match[] = {
	{ .compatible = "oplus-optiga", },
	{},
};
MODULE_DEVICE_TABLE(of, of_oplus_optiga_match);

struct oplus_optiga_chip * oplus_get_optiga_info (void)
{
	return g_oplus_optiga_chip;
}


#ifdef CONFIG_OPLUS_CHARGER_MTK
#define AUTH_MESSAGE_LEN	   20
#define OPLUS_OPTIGA_AUTH_TAG      "optiga_auth="
#define OPLUS_OPTIGA_AUTH_SUCCESS  "optiga_auth=TRUE"
#define OPLUS_OPTIGA_AUTH_FAILED   "optiga_auth=FALSE"

#ifdef MODULE
#include <asm/setup.h>

static char __oplus_chg_cmdline[COMMAND_LINE_SIZE];
static char *oplus_chg_cmdline = __oplus_chg_cmdline;

static const char *oplus_optiga_get_cmdline(void)
{
	struct device_node * of_chosen = NULL;
	char *optiga_auth = NULL;

	if (__oplus_chg_cmdline[0] != 0)
		return oplus_chg_cmdline;

	of_chosen = of_find_node_by_path("/chosen");
	if (of_chosen) {
		optiga_auth = (char *)of_get_property(
					of_chosen, "optiga_auth", NULL);
		if (!optiga_auth)
			pr_err("%s: failed to get optiga_auth\n", __func__);
		else {
			strcpy(__oplus_chg_cmdline, optiga_auth);
			pr_err("%s: optiga_auth: %s\n", __func__, optiga_auth);
		}
	} else {
		pr_err("%s: failed to get /chosen \n", __func__);
	}

	return oplus_chg_cmdline;
}
#else
static const char *oplus_optiga_get_cmdline(void)
{
	return NULL;
}
#endif
#endif

static bool oplus_optia_check_auth_msg(void) {
	bool ret = false;
#ifdef CONFIG_OPLUS_CHARGER_MTK
	char *str = NULL;

	if (NULL == oplus_optiga_get_cmdline()) {
		pr_err("oplus_chg_check_auth_msg: cmdline is NULL!!!\n");
		return false;
	}

	str = strstr(oplus_optiga_get_cmdline(), OPLUS_OPTIGA_AUTH_TAG);
	if (str == NULL) {
		pr_err("oplus_chg_check_auth_msg: Asynchronous authentication is not supported!!!\n");
		return false;
	}

	pr_info("oplus_chg_check_auth_msg: %s\n", str);
	if (0 == memcmp(str, OPLUS_OPTIGA_AUTH_SUCCESS, sizeof(OPLUS_OPTIGA_AUTH_SUCCESS))) {
		ret = true;
		pr_info("oplus_chg_check_auth_msg: %s\n", OPLUS_OPTIGA_AUTH_SUCCESS);
	} else {
		ret = false;
		pr_info("oplus_chg_check_auth_msg: %s\n", OPLUS_OPTIGA_AUTH_FAILED);
	}
#else
	/* QCom not realize now. */
#endif
	return ret;
}
static int oplus_optiga_probe(struct platform_device *pdev)
{
	int ret = 0;
	int retry = 0;
	struct oplus_external_auth_chip	*external_auth_chip = NULL;
	g_oplus_optiga_chip = devm_kzalloc(&pdev->dev, sizeof(*g_oplus_optiga_chip), GFP_KERNEL);
	chg_err("oplus_optiga_probe enter\n");

	if (!g_oplus_optiga_chip) {
		chg_err("kzalloc() failed.\n");
		ret = -ENOMEM;
		goto error;
	}

	g_oplus_optiga_chip->hmac_status.authenticate_result = false;
	g_oplus_optiga_chip->test_result.test_count_total = 0;
	g_oplus_optiga_chip->test_result.test_fail_count = 0;
	g_oplus_optiga_chip->test_result.test_count_now = 0;
	g_oplus_optiga_chip->test_result.real_test_count_now = 0;
	g_oplus_optiga_chip->test_result.real_test_fail_count = 0;
	g_oplus_optiga_chip->try_count = TEST_COUNT;
	g_oplus_optiga_chip->dev = &pdev->dev;
	ret = oplus_optiga_parse_dt(g_oplus_optiga_chip);
	if(ret) {
		chg_err("oplus_optiga_parse_dt fail\n");
		return 0;
	}

	Ecc_SwitchKey(g_oplus_optiga_chip->key_id);
	Nvm_SwitchUID(g_oplus_optiga_chip->key_id);

	spin_lock_init(&g_oplus_optiga_chip->slock);
/*	ret = optiga_authenticate();
	if (ret == 0){
		g_oplus_optiga_chip->hmac_status.authenticate_result = true;
	}
	ret = optiga_authenticate();
	ret = optiga_authenticate();*/
	init_completion(&g_oplus_optiga_chip->is_complete);
	INIT_DELAYED_WORK(&g_oplus_optiga_chip->auth_work, oplus_optiga_auth_work);
	INIT_DELAYED_WORK(&g_oplus_optiga_chip->test_work, oplus_optiga_test_func);

	if (g_oplus_optiga_chip->support_optiga_in_lk && oplus_optia_check_auth_msg()) {
		chg_err("get lk auth success.\n");
		ret = true;

		/*LK already auth success, Kernel not auth again.*/
		g_oplus_optiga_chip->hmac_status.authenticate_result = true;
	} else {
		chg_err("get lk auth failed or not support auth in LK.\n");
		gpio_direction_output(g_oplus_optiga_chip->data_gpio, 0);
		chg_err("g_oplus_optiga_chip->data_gpio: [%d]\n", g_oplus_optiga_chip->data_gpio);
		while (retry < OPLUS_OPTIGA_RETRY_COUNT) {
			ret = oplus_optiga_get_external_auth_hmac();
			if (ret) {
				chg_err("get_external_auth_hmac success after retry %d.\n", retry);
				break;
			}
			retry++;
		}
	}

	if (!ret) {
		chg_err("get_external_auth_hmac failed.\n");
	} else {
		chg_err("get_external_auth_hmac success.\n");
	}

	external_auth_chip = devm_kzalloc(&pdev->dev, sizeof(struct oplus_external_auth_chip), GFP_KERNEL);
	if (!external_auth_chip) {
		ret = -ENOMEM;
		goto error;
	}
	external_auth_chip->get_external_auth_hmac = oplus_optiga_get_external_auth_hmac;
	external_auth_chip->start_test_external_hmac = oplus_optiga_start_test;
	external_auth_chip->get_hmac_test_result = oplus_optiga_get_test_result;
	external_auth_chip->get_hmac_status = oplus_optiga_get_hmac_status;
	oplus_external_auth_init(external_auth_chip);
	chg_err("oplus_optiga_probe sucess\n");
	return 0;

error:
	if (g_oplus_optiga_chip) {
		kfree(g_oplus_optiga_chip);
	}
	/*
	if (external_auth_chip) {
		kfree(external_auth_chip);
	}
	*/
	chg_err("oplus_optiga_probe fail :%d\n",ret);
	return ret;
}

static void oplus_optiga_shutdown(struct platform_device *pdev)
{
	return;
}

static struct platform_driver oplus_optiga_driver = {
	.probe		= oplus_optiga_probe,
	.shutdown	= oplus_optiga_shutdown,
	.driver		= {
		.name	= "oplus-optiga",
		.of_match_table = of_oplus_optiga_match,
	},
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
module_platform_driver(oplus_optiga_driver);
#else
int oplus_optiga_driver_init(void)
{
	int ret;
	pr_err("%s: start\n", __func__);
	ret = platform_driver_register(&oplus_optiga_driver);
	return ret;
}

void oplus_optiga_driver_exit(void)
{
	platform_driver_unregister(&oplus_optiga_driver);
}
#endif

MODULE_AUTHOR("zhouhaikang");
MODULE_DESCRIPTION("oplus optiga driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:oplus-optiga");
