/*
 * drivers/power/axp/axp2101/axp2101-charger.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * charger driver of axp2101
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "axp2101-charger.h"

static int axp2101_get_usb_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp2101_get_usb_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp2101_set_usb_vhold(struct axp_charger_dev *cdev, int vol)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol) {
		axp_regmap_update(map, axp2101_VINDPM_CFG, 0x06, 0x1F);
		if (vol >= 4000 && vol <= 4700) {
			tmp = (vol - 3880)/80;
			axp_regmap_update(map, axp2101_VINDPM_CFG,
				tmp, 0x1F);
		} else {
			pr_err("set usb limit voltage error, %d mV\n",
				axp2101_config.pmu_usbpc_vol);
		}
	} else {
		axp_regmap_update(map, axp2101_VINDPM_CFG, 0x06, 0x1F);
	}

	return 0;

}

static int axp2101_get_usb_vhold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, axp2101_VINDPM_CFG, &tmp);
	tmp = tmp & 0x1F;

	return 3880 + tmp * 80;
}

static int axp2101_set_usb_ihold(struct axp_charger_dev *cdev, int cur)
{
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur) {
		if (cur <= 100)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x00, 0x07);
		else if (cur > 100 && cur <= 500)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x01, 0x07);
		else if (cur > 100 && cur <= 900)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x02, 0x07);
		else if (cur > 900 && cur <= 1000)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x03, 0x07);
		else if (cur > 1000 && cur <= 1500)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x04, 0x07);
		else if (cur > 1500 && cur <= 2000)
			axp_regmap_update(map, axp2101_IIN_LIM, 0x05, 0x07);
		else
			axp_regmap_update(map, axp2101_IIN_LIM, 0x05, 0x07);

	} else {
		axp_regmap_update(map, axp2101_IIN_LIM, 0x05, 0x07);
	}

	return 0;
}

static int axp2101_get_usb_ihold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, axp2101_IIN_LIM, &tmp);
	tmp = tmp & 0x3;
	if (tmp == 0x0)
		return 100;
	else if (tmp == 0x1)
		return 500;
	else if (tmp == 0x2)
		return 900;
	else if (tmp == 0x3)
		return 1000;
	else if (tmp == 0x4)
		return 1500;
	else
		return 2500;
}

static struct axp_ac_info axp2101_ac_info = {
	.det_bit = 5,
	.det_offset = axp2101_COMM_STAT0,
	.valid_bit = 5,
	.valid_offset = axp2101_COMM_STAT0,
};

static struct axp_usb_info axp2101_usb_info = {
	.det_unused      = 1,
	.det_bit         = 5,
	.det_offset      = axp2101_COMM_STAT0,
	.valid_bit       = 5,
	.valid_offset    = axp2101_COMM_STAT0,
	.get_usb_voltage = axp2101_get_usb_voltage,
	.get_usb_current = axp2101_get_usb_current,
	.set_usb_vhold   = axp2101_set_usb_vhold,
	.get_usb_vhold   = axp2101_get_usb_vhold,
	.set_usb_ihold   = axp2101_set_usb_ihold,
	.get_usb_ihold   = axp2101_get_usb_ihold,
};

static int axp2101_get_rest_cap(struct axp_charger_dev *cdev)
{
	u8 val;
	int rest_vol = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, axp2101_SOC, &val);
	rest_vol = (int)(val);
	return rest_vol;
}

static inline int axp2101_vts_to_mv(u32 reg)
{
	return ((int)(((reg >> 8) & 0x003F) | (reg & 0x000F))) * 800 / 1000;
}

/* static int axp2101_get_bat_temp(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 vts_res;
	struct axp_regmap *map = cdev->chip->regmap;
	int bat_temp_mv;

	axp_regmap_reads(map, axp2101_TS_H, 2, tmp);
	vts_res = (tmp[1] << 8) | tmp[0];
	bat_temp_mv = axp2101_vts_to_mv(vts_res);

	return axp_charger_vts_to_temp(bat_temp_mv, &axp2101_config);
} */

static int axp2101_get_bat_health(struct axp_charger_dev *cdev)
{
	u8 val;
	/* int bat_temp; */
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, axp2101_COMM_STAT0, &val);
	if (val & AXP2101_FAULT_LOG_BATINACT)
		return POWER_SUPPLY_HEALTH_DEAD;
	/* bat_temp = axp2101_get_bat_temp(cdev);
	if (val & AXP2101_FAULT_LOG_OVER_TEMP){
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	}
	else if (val & AXP2101_FAULT_LOG_COLD){
		return POWER_SUPPLY_HEALTH_COLD;
	}		 */
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int axp2101_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) & 0x003F) | (reg & 0x000F))) * 1100 / 1000;
}

static int axp2101_get_vbat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, axp2101_VBAT_H, 2, tmp);
	res = ((tmp[1] << 8) | tmp[0]);

	return axp2101_vbat_to_mV(res);
}


static int axp2101_set_chg_cur(struct axp_charger_dev *cdev, int cur)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur == 0)
		axp_regmap_clr_bits(map, axp2101_MODULE_EN, 0x02);
	else
		axp_regmap_set_bits(map, axp2101_MODULE_EN, 0x02);

	if (cur > 200 && cur <= 2000) {
		tmp = (cur - 200) / 100+8;
		axp_regmap_update(map, axp2101_CHG_CFG, tmp, 0x1F);
	} else if (cur < 200) {
		tmp = cur/25;
		axp_regmap_update(map, axp2101_CHG_CFG, tmp, 0x1F);
	} else {
		axp_regmap_update(map, axp2101_CHG_CFG, 0x10, 0x1F);
	}

	return 0;
}

static int axp2101_set_chg_vol(struct axp_charger_dev *cdev, int vol)
{
	uint8_t tmp = 0x0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol <= 3900) {
		tmp = 0x00;
	} else if (vol <= 4000) {
		tmp = 0x01;
	} else if (vol <= 4100) {
		tmp = 0x02;
	} else if (vol <= 4200) {
		tmp = 0x03;
	} else if (vol <= 4350) {
		tmp = 0x04;
	} else if (vol <= 4400) {
		tmp = 0x05;
	} else {
		tmp = 0x03;
	}

	axp_regmap_update(map, axp2101_CHG_V_CFG, tmp, 0x7);

	return 0;
}

static struct axp_battery_info axp2101_batt_info = {
	.det_unused           = 0,
	.chgstat_bit          = 1,
	.chgstat_offset       = axp2101_COMM_STAT1,
	.det_bit              = 3,
	.det_valid_bit        = 0,
	.det_valid            = 0,
	.det_offset           = axp2101_COMM_STAT0,
	.cur_direction_bit    = 5,
	.cur_direction_offset = axp2101_COMM_STAT1,
	.get_rest_cap         = axp2101_get_rest_cap,
	.get_bat_health       = axp2101_get_bat_health,
	.get_vbat             = axp2101_get_vbat,
	/* .get_bat_temp      = axp2101_get_bat_temp, */
	.set_chg_cur          = axp2101_set_chg_cur,
	.set_chg_vol          = axp2101_set_chg_vol,
};

static struct power_supply_info battery_data = {
	.name               = "PTI PL336078",
	.technology         = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
	.use_for_apm        = 1,
};

static struct axp_supply_info axp2101_spy_info = {
	.ac   = &axp2101_ac_info,
	.usb  = &axp2101_usb_info,
	.batt = &axp2101_batt_info,
};

static int axp2101_charger_init(struct axp_dev *axp_dev)
{
	u8 val;
	int tmp;
	struct axp_regmap *map = axp_dev->regmap;
	/* int update_min_times[8] = {30, 60, 120, 164, 0, 5, 10, 20}; */
	tmp = axp2101_config.pmu_init_chgend_rate;

	if (tmp) {
		tmp = tmp / 25;
		axp_regmap_update(map, axp2101_CHG_V_CFG, tmp, 0x0F);
	} else {
		axp_regmap_update(map, axp2101_CHG_V_CFG, tmp, 0x0F);
	}

	if (axp2101_config.pmu_init_chg_pretime < 40)
		axp2101_config.pmu_init_chg_pretime = 40;
	val = (axp2101_config.pmu_init_chg_pretime-40)/10;
	axp_regmap_update(map, axp2101_CHG_TMR_CFG, val, 0x03);
	if (axp2101_config.pmu_init_chg_csttime < 360)
		axp2101_config.pmu_init_chg_csttime = 360;
	val = (axp2101_config.pmu_init_chg_csttime - 360) / 120;
	axp_regmap_update(map, axp2101_CHG_TMR_CFG, val, 0x30);

	/* adc set */
	val = AXP2101_ADC_BATVOL_ENABLE;
	if (0 != axp2101_config.pmu_bat_temp_enable)
		val = val | AXP2101_ADC_TSVOL_ENABLE;
	axp_regmap_update(map, axp2101_ADC_CH_EN0, val,
						AXP2101_ADC_BATVOL_ENABLE
						| AXP2101_ADC_TSVOL_ENABLE);

	/* bat para */
	axp_regmap_write(map, axp2101_GAUGE_THLD,
		((axp2101_config.pmu_battery_warning_level1 - 5) << 4)
		+ axp2101_config.pmu_battery_warning_level2);

	/* Init CHGLED function */
	if (axp2101_config.pmu_chgled_func)
		axp_regmap_set_bits(map, axp2101_CHGLED_CFG, 0x01); /* ENABLE CHGLED */
	else
		axp_regmap_clr_bits(map, axp2101_CHGLED_CFG, 0x01); /* DISABLE CHGLED*/

	/* set CHGLED Indication Type */
	if (axp2101_config.pmu_chgled_type)
		axp_regmap_update(map, axp2101_CHGLED_CFG, 0x01, 0x30); /* Type B */
	else
		axp_regmap_update(map, axp2101_CHGLED_CFG, 0x00, 0x30); /* Type A */



	if (axp2101_config.pmu_bat_unused == 1)
		axp2101_spy_info.batt->det_unused = 1;
	else
		axp2101_spy_info.batt->det_unused = 0;

	return 0;
}

static struct axp_interrupts axp_charger_irq[] = {
	{"usb in",               axp_usb_in_isr},
	{"usb out",              axp_usb_out_isr},
	{"bat in",               axp_capchange_isr},
	{"bat out",              axp_capchange_isr},
	{"bat untemp work",      axp_change_isr},
	{"bat ovtemp work",      axp_change_isr},
	{"bat untemp chg",       axp_change_isr},
	{"bat ovtemp chg",       axp_change_isr},
	{"charging",             axp_change_isr},
	{"charge over",          axp_change_isr},
	{"low warning1",         axp_low_warning1_isr},
	{"low warning2",         axp_low_warning2_isr},
};

static void axp2101_private_debug(struct axp_charger_dev *cdev)
{

}

static int axp2101_charger_probe(struct platform_device *pdev)
{
	int ret, i, irq;
	struct axp_charger_dev *chg_dev;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		ret = axp_charger_dt_parse(pdev->dev.of_node, &axp2101_config);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp2101 charger device tree err!\n");
		return -EBUSY;
	}

	axp2101_usb_info.usb_pc_vol = axp2101_config.pmu_usbpc_vol;
	axp2101_usb_info.usb_pc_cur = axp2101_config.pmu_usbpc_cur;
	axp2101_usb_info.usb_ad_vol = axp2101_config.pmu_ac_vol;
	axp2101_usb_info.usb_ad_cur = axp2101_config.pmu_ac_cur;
	axp2101_batt_info.runtime_chgcur = axp2101_config.pmu_runtime_chgcur;
	axp2101_batt_info.suspend_chgcur = axp2101_config.pmu_suspend_chgcur;
	axp2101_batt_info.shutdown_chgcur = axp2101_config.pmu_shutdown_chgcur;
	battery_data.voltage_max_design = axp2101_config.pmu_init_chgvol
								* 1000;
	battery_data.voltage_min_design = axp2101_config.pmu_pwroff_vol
								* 1000;
	axp2101_charger_init(axp_dev);

	chg_dev = axp_power_supply_register(&pdev->dev, axp_dev,
					&battery_data, &axp2101_spy_info);
	if (IS_ERR_OR_NULL(chg_dev))
		goto fail;
	chg_dev->private_debug = axp2101_private_debug;

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;

		ret = axp_request_irq(axp_dev, irq,
				axp_charger_irq[i].isr, chg_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
					axp_charger_irq[i].name, irq, ret);
			goto out_irq;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_charger_irq[i].name, irq, ret);
	}

	platform_set_drvdata(pdev, chg_dev);

	return 0;

out_irq:
	for (i = i - 1; i >= 0; i--) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}
fail:
	return -1;
}

static int axp2101_charger_remove(struct platform_device *pdev)
{
	int i, irq;
	struct axp_charger_dev *chg_dev = platform_get_drvdata(pdev);
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}

	axp_power_supply_unregister(chg_dev);

	return 0;
}

static int axp2101_charger_suspend(struct platform_device *dev,
				pm_message_t state)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);

	axp_suspend_flag = AXP_WAS_SUSPEND;
	axp_charger_suspend(chg_dev);

	return 0;
}

static int axp2101_charger_resume(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	struct axp_regmap *map = chg_dev->chip->regmap;
	int pre_rest_vol;

	if (axp_suspend_flag == AXP_SUSPEND_WITH_IRQ) {
		axp_suspend_flag = AXP_NOT_SUSPEND;
#ifdef CONFIG_AXP_NMI_USED
		enable_nmi();
#endif
	} else {
		axp_suspend_flag = AXP_NOT_SUSPEND;
	}

	pre_rest_vol = chg_dev->rest_vol;
	axp_charger_resume(chg_dev);

	if (chg_dev->rest_vol - pre_rest_vol) {
		pr_info("battery vol change: %d->%d\n",
				pre_rest_vol, chg_dev->rest_vol);
		axp_regmap_write(map, axp2101_BUFFERC, chg_dev->rest_vol | 0x80);
	}

	return 0;
}

static void axp2101_charger_shutdown(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	axp_charger_shutdown(chg_dev);
}

static const struct of_device_id axp2101_charger_dt_ids[] = {
	{ .compatible = "axp2101-charger", },
	{},
};
MODULE_DEVICE_TABLE(of, axp2101_charger_dt_ids);

static struct platform_driver axp2101_charger_driver = {
	.driver     = {
		.name   = "axp2101-charger",
		.of_match_table = axp2101_charger_dt_ids,
	},
	.probe    = axp2101_charger_probe,
	.remove   = axp2101_charger_remove,
	.suspend  = axp2101_charger_suspend,
	.resume   = axp2101_charger_resume,
	.shutdown = axp2101_charger_shutdown,
};

static int __init axp2101_charger_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp2101_charger_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall_sync(axp2101_charger_initcall);

MODULE_DESCRIPTION("Charger Driver of AXP2101");
MODULE_AUTHOR("pannan");
MODULE_LICENSE("GPL");
