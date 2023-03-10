/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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
#ifndef __RAWNAND_DRV_CFG_H
#define __RAWNAND_DRV_CFG_H


//#define  NAND_VERSION                 0x0223
//#define  NAND_DRV_DATE                  0x20150312


#define   SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ       (1)

#define   SUPPORT_CHANGE_ONFI_TIMING_MODE           (1)

#define   SUPPORT_SCAN_EDO_FOR_SDR_NAND             (0)
#define   GOOD_DDR_EDO_DELAY_CHAIN_TH               (7)

#define   MAX_SECTORS_PER_PAGE_FOR_TWO_PLANE        (32)



#if 1
#define	   PHY_DBG(...)		PRINT_DBG(__VA_ARGS__)
#else
#define    PHY_DBG(...)
#endif

#if 1
#define     PHY_ERR(...)	PRINT(__VA_ARGS__)
#else
#define     PHY_ERR(...)
#endif



#endif //ifndef __NAND_DRV_CFG_H

