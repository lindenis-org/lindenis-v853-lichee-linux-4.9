/*
 * drivers/input/keyboard/sunxi-gpio-ir-tx.h
 *
 * Copyright (c) 2013-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SUNXI_GPIO_IR_RX_H
#define _SUNXI_GPIO_IR_RX_H

/*
 * Defines
 */
#define DRV_VERSION	 "1.0"

#define GPIO_IR_L1_MIN        8000   /* 8ms, Lead1(9ms) > IR_L1_MIN */
#define GPIO_IR_L0_MIN        2000   /* 2ms, Lead0(4.5ms) Lead0R(2.25ms) */

#define GPIO_IR_PMAX          2000   /*2000us, Pluse < IR_PMAX */
#define GPIO_IR_PMIN          400    /*400us, Pluse > GPIO_IR_PMIN */
#define GPIO_IR_DMID          1400   /*1400us, D1 > IR_DMID, D0 =< IR_DMID */


#define GPIO_IR_ERROR_CODE    (0xffffffff)
#define GPIO_IR_REPEAT_CODE   (0x00000000)


struct gpio_ir_rx_info {
	struct input_dev *ir_dev;
	unsigned int ir_rx_gpio;
	unsigned int ir_rx_gpio_irq;
	struct tasklet_struct tsklet;
	struct fasync_struct *ir_fasync;
	//struct mutex lock;
	struct timer_list report_timer;
	struct timer_list sample_timer;
	ktime_t ktime;
	unsigned int timer_used;
	unsigned int key_count;
	unsigned int ir_code;
	unsigned int report_enable;
	unsigned int sampler_enable;
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

#endif
