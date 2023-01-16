/*
 * drivers/power/supply/axp/axp20x/axp20x.h
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef AXP20X_H_
#define AXP20X_H_

#include "../axp-core.h"
#define AXP20X_RSB_RTSADDR         (0x2d)

#define AXP20X_STATUS              (0x00)
#define AXP20X_MODE_CHGSTATUS      (0x01)
#define AXP20X_OTG_STATUS	   (0x01)
#define AXP20X_IC_TYPE             (0x03)
#define AXP20X_BUFFER1             (0x04)
#define AXP20X_BUFFER2             (0x05)
#define AXP20X_BUFFER3             (0x06)
#define AXP20X_BUFFER4             (0x07)
#define AXP20X_BUFFER5             (0x08)
#define AXP20X_BUFFER6             (0x09)
#define AXP20X_BUFFER7             (0x0A)
#define AXP20X_BUFFER8             (0x0B)
#define AXP20X_BUFFER9             (0x0C)
#define AXP20X_BUFFERA             (0x0D)
#define AXP20X_BUFFERB             (0x0E)
#define AXP20X_BUFFERC             (0x0F)

#define AXP20X_LDO234_DC23_CTL     (0X12)
#define AXP20X_DC2OUT_VOL          (0x23)
#define AXP20X_LDO3_DC2_DVM        (0x25)
#define AXP20X_DC3OUT_VOL          (0x27)
#define AXP20X_LDO24OUT_VOL        (0x28)
#define AXP20X_LDO3OUT_VOL         (0x29)

#define AXP20X_IPS_SET             (0x30)
#define AXP20X_VOFF_SET            (0x31)
#define AXP20X_OFF_CTL             (0x32)
#define AXP20X_CHARGE1             (0x33)
#define AXP20X_CHARGE2             (0x34)
#define AXP20X_BACKUP_CHG          (0x35)
#define AXP20X_POK_SET             (0x36)
#define AXP20X_DCDC_FREQSET	   (0x37)
#define AXP20X_VLTF_CHGSET	   (0x38)
#define AXP20X_VHTF_CHGSET	   (0x39)
#define AXP20X_APS_WARNING1	   (0x3A)
#define AXP20X_APS_WARNING2	   (0x3B)
#define AXP20X_TLTF_DISCHGSET	   (0x3C)
#define AXP20X_THTF_DISCHGSET	   (0x3D)

#define AXP20X_DCDC_MODESET	   (0x80)
#define AXP20X_OP_MONITOR	   (0x81)
#define AXP20X_ADC_EN1		   (0x82)
#define AXP20X_ADC_EN2		   (0x83)
#define AXP20X_ADC_ADC_SPEED	   (0x84)
#define AXP20X_ADC_INPUTRANGE	   (0x85)
#define AXP20X_ADC_IRQ_RETFSET	   (0x86)
#define AXP20X_ADC_IRQ_FETFSET	   (0x87)
#define AXP20X_TIMER_CTL	   (0x8A)
#define AXP20X_VBUS_DET_SRP	   (0x8B)
#define AXP20X_HOTOVER_CTL	   (0x8F)
#define AXP20X_GPIO0_CTL	   (0x90)
#define AXP20X_GPIO0_VOL	   (0x91)
#define AXP20X_GPIO1_CTL	   (0x92)
#define AXP20X_GPIO2_CTL	   (0x93)
#define AXP20X_GPIO012_SIGNAL	   (0x94)
#define AXP20X_GPIO3_CTL	   (0x95)
#define AXP20X_GPIO_PULL	   (0x97)

#define AXP20X_INTEN1              (0x40)
#define AXP20X_INTEN2              (0x41)
#define AXP20X_INTEN3              (0x42)
#define AXP20X_INTEN4              (0x43)
#define AXP20X_INTEN5              (0x44)
#define AXP20X_INTSTS1             (0x48)
#define AXP20X_INTSTS2             (0x49)
#define AXP20X_INTSTS3             (0x4A)
#define AXP20X_INTSTS4             (0x4B)
#define AXP20X_INTSTS5             (0x4C)

#define AXP20X_BAT_CHGCOULOMB3     (0xB0)
#define AXP20X_BAT_CHGCOULOMB2     (0xB1)
#define AXP20X_BAT_CHGCOULOMB1     (0xB2)
#define AXP20X_BAT_CHGCOULOMB0     (0xB3)
#define AXP20X_BAT_DISCHGCOULOMB3  (0xB4)
#define AXP20X_BAT_DISCHGCOULOMB2  (0xB5)
#define AXP20X_BAT_DISCHGCOULOMB1  (0xB6)
#define AXP20X_BAT_DISCHGCOULOMB0  (0xB7)
#define AXP20X_COULOMB_CTL         (0xB8)

/* bit definitions for AXP events ,irq event */
#define AXP20X_IRQ_USBLO           (1)
#define AXP20X_IRQ_USBRE           (2)
#define AXP20X_IRQ_USBIN           (3)
#define AXP20X_IRQ_USBOV           (4)
#define AXP20X_IRQ_ACRE            (5)
#define AXP20X_IRQ_ACIN            (6)
#define AXP20X_IRQ_ACOV            (7)

#define AXP20X_IRQ_TEMLO           (8)
#define AXP20X_IRQ_TEMOV           (9)
#define AXP20X_IRQ_CHAOV           (10)
#define AXP20X_IRQ_CHAST           (11)
#define AXP20X_IRQ_BATATOU         (12)
#define AXP20X_IRQ_BATATIN         (13)
#define AXP20X_IRQ_BATRE           (14)
#define AXP20X_IRQ_BATIN           (15)

#define AXP20X_IRQ_POKLO           (16)
#define AXP20X_IRQ_POKSH           (17)
#define AXP20_IRQ_LDO3LO	   (18)
#define AXP20_IRQ_DCDC3LO	   (19)
#define AXP20_IRQ_DCDC2LO	   (20)
#define AXP20_IRQ_CHACURLO	   (22)
#define AXP20_IRQ_ICTEMOV	   (23)

#define AXP20X_IRQ_LOWN2           (24)
#define AXP20X_IRQ_LOWN1           (25)
#define AXP20_IRQ_USBSESUN	   (26)
#define AXP20_IRQ_USBSESVA	   (27)
#define AXP20_IRQ_USBUN		   (28)
#define AXP20_IRQ_USBVA		   (29)
#define AXP20_IRQ_NOECLO	   (30)
#define AXP20_IRQ_NOEOPE	   (31)

#define AXP20X_IRQ_GPIO0           (32)
#define AXP20X_IRQ_GPIO1           (33)
#define AXP20X_IRQ_GPIO2           (34)
#define AXP20X_IRQ_GPIO3           (35)
#define AXP20X_IRQ_PEKFE           (37)
#define AXP20X_IRQ_PEKRE           (38)
#define AXP20X_IRQ_TIMER           (39)

/* Status Query Interface */
#define AXP20X_STATUS_SOURCE       (1 <<  0)
#define AXP20X_STATUS_ACUSBSH      (1 <<  1)
#define AXP20X_STATUS_BATCURDIR    (1 <<  2)
#define AXP20X_STATUS_USBLAVHO     (1 <<  3)
#define AXP20X_STATUS_USBVA        (1 <<  4)
#define AXP20X_STATUS_USBEN        (1 <<  5)
#define AXP20X_STATUS_ACVA         (1 <<  6)
#define AXP20X_STATUS_ACEN         (1 <<  7)

#define AXP20_STATUS_CHACURLOEXP   (1 << 10)
#define AXP20X_STATUS_BATINACT     (1 << 11)

#define AXP20X_STATUS_BATEN        (1 << 13)
#define AXP20X_STATUS_INCHAR       (1 << 14)
#define AXP20X_STATUS_ICTEMOV      (1 << 15)

extern struct axp_config_info axp20x_config;

#endif /* AXP20X_H_ */
