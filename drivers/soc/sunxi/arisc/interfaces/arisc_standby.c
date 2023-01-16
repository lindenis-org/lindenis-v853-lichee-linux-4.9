/*
 *  drivers/arisc/interfaces/arisc_standby.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../arisc_i.h"
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>

/* record super-standby wakeup event */
static unsigned long dram_crc_error;
static unsigned long dram_crc_total_count;
static unsigned long dram_crc_error_count;

/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_wakeup_source(u32 *event)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_QUERY_WAKEUP_SRC_REQ,
			(page_to_phys(virt_to_page(event)) + offset_in_page(event)), 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_query_wakeup_source);

/**
 * set wakeup source.
 * @para:  wakeup source irq.
 *
 * return: result, 0 - set successed,
 *                !0 - set failed;
 */
int arisc_set_wakeup_source(u32 wakeup_irq)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_WAKEUP_SRC_REQ,
				wakeup_irq, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_set_wakeup_source);

/**
 * clear wakeup source.
 * @para:  wakeup source irq.
 *
 * return: result, 0 - set successed,
 *                !0 - set failed;
 */
int arisc_clear_wakeup_source(u32 wakeup_irq)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_CLEAR_WAKEUP_SRC_REQ,
				wakeup_irq, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_clear_wakeup_source);

/*
 * query super-standby dram crc result.
 * @para:  point of buffer to store dram crc result informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_dram_crc_result(unsigned long *perror, unsigned long *ptotal_count,
	unsigned long *perror_count)
{
	*perror = dram_crc_error;
	*ptotal_count = dram_crc_total_count;
	*perror_count = dram_crc_error_count;

	return 0;
}
EXPORT_SYMBOL(arisc_query_dram_crc_result);

int arisc_set_dram_crc_result(unsigned long error, unsigned long total_count,
	unsigned long error_count)
{
	dram_crc_error = error;
	dram_crc_total_count = total_count;
	dram_crc_error_count = error_count;

	return 0;
}
EXPORT_SYMBOL(arisc_set_dram_crc_result);
