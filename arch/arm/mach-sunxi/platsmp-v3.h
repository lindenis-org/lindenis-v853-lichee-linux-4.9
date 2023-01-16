/*
 * arch/arm/mach-sunxi/platsmp-v3.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: zhuxianbin <zhuxianbin@allwinnertech.com>
 *
 * sunxi smp ops header file for platform v1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATSMP_V3_H__
#define __PLATSMP_V3_H__


#define SUNXI_R_PRCM_PBASE        (0x01f01400)
#define SUNXI_R_CPUCFG_PBASE      (0x01f01c00)
#define SUNXI_R_CPUCFG_VBASE      IO_ADDRESS(SUNXI_R_CPUCFG_PBASE)
#define SUNXI_R_PRCM_VBASE        IO_ADDRESS(SUNXI_R_PRCM_PBASE)

static inline int sunxi_is_wfi_mode(int cpu)
{
	u32 r_cpucfg_vbase = (u32)SUNXI_R_CPUCFG_VBASE;

	return (readl((void *)(r_cpucfg_vbase +
					CPUCFG_CPU_STATUS_REG(cpu))) & (1<<2));
}

static inline void sunxi_enable_cpu(int cpu)
{
	u32 pwr_reg;
	u32 r_prcm_v;
	u32 r_cpucfg_v;

	r_prcm_v = (u32)SUNXI_R_PRCM_VBASE;
	r_cpucfg_v = (u32)SUNXI_R_CPUCFG_VBASE;
	/* step1: Assert nCOREPORESET LOW and hold L1RSTDISABLE LOW.
	 * Ensure DBGPWRDUP is held LOW to prevent any external
	 * debug access to the processor.
	 */
	/* assert cpu core reset */
	writel(0, (void *)(r_cpucfg_v + CPUCFG_CPU_RST_CTRL_REG(cpu)));
	/* L1RSTDISABLE hold low */
	pwr_reg = readl((void *)(r_cpucfg_v + CPUCFG_GEN_CTRL_REG));
	pwr_reg &= ~(1<<cpu);
	writel(pwr_reg, (void *)(r_cpucfg_v + CPUCFG_GEN_CTRL_REG));
	udelay(10);

	/* step2: release power clamp */
	writel(0xFE, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(20);
	writel(0xF8, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(10);
	writel(0xE0, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(10);
	writel(0x80, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(10);
	writel(0x00, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(20);

	while (readl((void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu))))
		;


	/* step3: clear power-off gating */
	pwr_reg = readl((void *)(r_prcm_v + PRCM_CPU_PWROFF_REG));
	pwr_reg &= ~(0x00000001<<cpu);
	writel(pwr_reg, (void *)(r_prcm_v + PRCM_CPU_PWROFF_REG));
	udelay(20);

	/* step4: de-assert core reset */
	writel(3, (void *)(r_cpucfg_v + CPUCFG_CPU_RST_CTRL_REG(cpu)));
}

static inline void sunxi_disable_cpu(int cpu)
{
	u32 pwr_reg;
	u32 r_prcm_v = (u32)SUNXI_R_PRCM_VBASE;

	/* step1: set up power-off signal */
	pwr_reg = readl((void *)(r_prcm_v + PRCM_CPU_PWROFF_REG));
	pwr_reg |= (1<<cpu);
	writel(pwr_reg, (void *)(r_prcm_v + PRCM_CPU_PWROFF_REG));
	udelay(20);

	/* step2: active the power output clamp */
	writel(0xff, (void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu)));
	udelay(30);
	while (readl((void *)(r_prcm_v + PRCM_CPU_PWR_CLAMP_REG(cpu))) != 0xff)
		;

}

#endif /*__PLATSMP_V3_H__*/

