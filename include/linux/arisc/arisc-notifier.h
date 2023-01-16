/*
 * linux/include/arisc-notifer.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * arisc notifier framework.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LINUX_ARISC_NOTIFIER_H__
#define __LINUX_ARISC_NOTIFIER_H__

#include <linux/notifier.h>

#define ARISC_INIT_READY  0x0001 /* arisc module init ready */

int  arisc_register_notifier(struct notifier_block *nb);
int  arisc_notify(unsigned long val, void *v);
void arisc_unregister_notifier(struct notifier_block *nb);

#endif /* __LINUX_ARISC_NOTIFIER_H__ */

