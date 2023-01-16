/*
 * Copyright (C) 2016 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_sun8iw18_H
#define __MACH_SUNXI_CLK_sun8iw18_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#include "clk-factors.h"

/* CCMU Register List */
#define PLL_CPU             0x0000
#define PLL_DDR             0x0010
#define PLL_PERIPH0         0x0020
#define PLL_PERIPH1         0x0028
#define PLL_AUDIO           0x0078
#define PLL_32K             0x00d8
#define PLL_DDRPAT          0x0110
#define PLL_PERI0PAT0       0x0120
#define PLL_PERI0PAT1       0x0124
#define PLL_PERI1PAT0       0x0128
#define PLL_PERI1PAT1       0x012C
#define PLL_AUDIOPAT0       0x0178
#define PLL_AUDIOPAT1       0x017C
#define PLL_32kPAT0         0x01D8
#define PLL_32kPAT1         0x01DC
#define PLL_CPUX_BIAS       0X0300
#define PLL_DDR_BIAS        0X0310
#define PLL_PERI0_BIAS      0X0320
#define PLL_PERI1_BIAS      0X0328
#define PLL_AUDIO_BIAS      0X0378
#define PLL_32K_BIAS        0X03D8
#define PLL_CPUX_TUN        0X0400
#define CPU_CFG             0x0500
#define PSI_CFG             0x0510
#define AHB3_CFG            0x051C
#define APB1_CFG            0x0520
#define APB2_CFG            0x0524
#define MBUS_CFG            0x0540

/* Accelerator */
#define CE_CFG              0x0680
#define CE_GATE             0x068C

/* SYS Resource */
#define DMA_GATE            0x070C
#define HSTIMER_GATE        0x073C
#define AVS_CFG             0x0740
#define DBGSYS_GATE         0x078C
#define PSI_GATE            0x079C
#define PWM_GATE            0x07AC

/* Storage Medium */
#define DRAM_CFG            0x0800
#define MBUS_GATE           0x0804
#define DRAM_GATE           0x080C
#define NAND0_CFG           0x0810
#define NAND1_CFG           0x0814
#define NAND_GATE           0x082C
#define SMHC1_CFG           0x0834
#define SMHC_GATE           0x084C

/* Common Interface */
#define UART_GATE           0x090C
#define TWI_GATE            0x091C
#define SPI0_CFG            0x0940
#define SPI1_CFG            0x0944
#define SPI_GATE            0x096C
#define GPADC_GATE          0x09EC
#define THS_GATE            0x09FC
#define I2S0_CFG            0x0A10
#define I2S1_CFG            0x0A14
#define I2S2_CFG            0x0A18
#define I2S_GATE            0x0A1C
#define SPDIF_CFG           0x0A20
#define SPDIF_GATE          0x0A2C
#define DMIC_CFG            0x0A40
#define DMIC_GATE           0x0A4C
#define CODEC_1X_CFG        0x0A50
#define CODEC_4X_CFG        0x0A54
#define CODEC_GATE          0x0A5C
#define USB0_CFG            0x0A70
#define USB_GATE            0x0A8C
#define MAD_GATE            0x0ACC
#define LPSD_CFG            0x0AD0
#define LPSD_GATE           0x0ADC
#define LEDC_CFG            0x0BF0
#define LEDC_GATE           0x0BFC
#define CCMU_SEC_SWITCH     0x0F00
#define PLL_LOCK_DBG_CTRL   0x0F04
#define PLL_CPUX_HW_FM      0x0F20
#define MOD_SPE_CLK         0x0F30
#define HOSC_CFG            0x0F40
#define CCMU_VERSION        0x0FF0

/* range of the registers we want to dump via debugfs/ccudbg/command "dumpreg" */
#define SUNXI_CLK_MAX_REG   LEDC_GATE

/* RTC Register List */
#define LOSC_OUT_GATE       0x0060

#define F_N8X8_P16x2(nv, pv)      (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, pv, 16, 2, 0, 0, 0, 0, 0, 0))
#define F_N8X8_D1V1X1_D2V0X1(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 0, 1))
#define F_N8X8_D1V1X1(nv, d1v)             (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, 0, 0, 0))
#define F_N8X8_D1V4X2_D2V0X2(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 4, 2, d2v, 0, 2))
#define F_N8X8_P16X6_D1V1X1_D2V0X1(nv, pv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, pv, 16, 6, d1v, 1, 1, d2v, 0, 1))

#define PLLCPU(n, p, freq)        {F_N8X8_P16x2(n, p), freq}
#define PLLDDR(n, d1, d2, freq)     {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLPERIPH0(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLPERIPH1(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLAUDIO(n, p, d1, d2, freq) {F_N8X8_P16X6_D1V1X1_D2V0X1(n, p, d1, d2), freq}

#endif
