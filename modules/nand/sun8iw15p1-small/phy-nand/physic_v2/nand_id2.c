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

#include "nand_physic_inc.h"

struct _optional_phy_op_par physic_arch_para[] = {
	{
		/* 0 */
		.multi_plane_read_cmd = {0x00, 0x30},
		/*
		 * the first byte is the end command for first plane
		 * the second byte is the first command for next plane
		 * here means: (80h, 11h) & (81h, 10h)
		 */
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0xf1,
		.inter_bnk1_status_cmd = 0xf2,
		.bad_block_flag_position = 0x01,
		.multi_plane_block_offset = 1,
	},
	{
		/* 1 */
		.multi_plane_read_cmd = {0x60, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x60, 0x60, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0xf1,
		.inter_bnk1_status_cmd = 0xf2,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 1,
	},
	{
		/* 2 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0xf1,
		.inter_bnk1_status_cmd = 0xf2,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
	{
		/* 3 */
		.multi_plane_read_cmd = {0x60, 0x60},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x60, 0x60, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0xf1,
		.inter_bnk1_status_cmd = 0xf2,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
	{
		/* 4 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x80},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x78,
		.inter_bnk1_status_cmd = 0x78,
		.bad_block_flag_position = 0x01,
		.multi_plane_block_offset = 1,
	},
	{
		/* 5 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x30},
		.multi_plane_copy_write_cmd = {0x8c, 0x11, 0x8c},
		.multi_plane_status_cmd = 0x71,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 0,
	},
	{
		/* 6 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x30},
		.multi_plane_copy_write_cmd = {0x8c, 0x11, 0x8c},
		.multi_plane_status_cmd = 0x71,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 1024,
	},
	{
		/* 7 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x30},
		.multi_plane_copy_write_cmd = {0x8c, 0x11, 0x8c},
		.multi_plane_status_cmd = 0x71,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 2048,
	},
	{
		/* 8 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x30},
		.multi_plane_copy_write_cmd = {0x8c, 0x11, 0x8c},
		.multi_plane_status_cmd = 0x71,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
	{
		/* 9 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x30},
		.multi_plane_copy_write_cmd = {0x8c, 0x11, 0x8c},
		.multi_plane_status_cmd = 0x71,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
	{
		/* 10 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x10, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x80},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
	{
		/* 11 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x10, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x80},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x01,
		.multi_plane_block_offset = 1,
	},
	{
		/* 12 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0xf1,
		.inter_bnk1_status_cmd = 0xf2,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 1,
	},
	{
		/* 13 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x80},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x78,
		.inter_bnk1_status_cmd = 0x78,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 1,
	},
	{
		/* 14 */
		.multi_plane_read_cmd = {0x60, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x60, 0x60, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x78,
		.inter_bnk0_status_cmd = 0x78,
		.inter_bnk1_status_cmd = 0x78,
		.bad_block_flag_position = 0x00,
		.multi_plane_block_offset = 1024,
	},
	{
		/* 15 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x81},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x81},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x70,
		.inter_bnk1_status_cmd = 0x70,
		.bad_block_flag_position = 0x01,
		.multi_plane_block_offset = 1,
	},
	{
		/* 16 */
		.multi_plane_read_cmd = {0x00, 0x30},
		.multi_plane_write_cmd = {0x11, 0x80},
		.multi_plane_copy_read_cmd = {0x00, 0x00, 0x35},
		.multi_plane_copy_write_cmd = {0x85, 0x11, 0x85},
		.multi_plane_status_cmd = 0x70,
		.inter_bnk0_status_cmd = 0x78,
		.inter_bnk1_status_cmd = 0x78,
		.bad_block_flag_position = 0x02,
		.multi_plane_block_offset = 1,
	},
};



struct _nfc_init_ddr_info def_ddr_info[] = {
	{
		.en_dqs_c = 0,
		.en_re_c = 0,
		.odt = 0,
		.en_ext_verf = 0,
		.dout_re_warmup_cycle = 0,
		.din_dqs_warmup_cycle = 0,
		.output_driver_strength = 0,
		.rb_pull_down_strength = 0,
	},
	{
		.en_dqs_c = 0,
		.en_re_c = 0,
		.odt = 0,
		.en_ext_verf = 0,
		.dout_re_warmup_cycle = 0,
		.din_dqs_warmup_cycle = 0,
		.output_driver_strength = 2,
		.rb_pull_down_strength = 2,
	},
};


/*
   LSB Page Type: OpOpt[19:16]  (.OperationOpt)
0x0 : hynix 26nm, 20nm; micron 256 page;
0x1 : samsung & toshiba 128 page;
0x2 : micron 20nm, 256 page; (20nm)
0x3 : hynix 16nm, 128 page per block
0x4 : hynix 16nm, 256 page per block
0x5 : micrion l85a, 512 page per block(20nm)
0x6 : micrion l95b, 512 page per block(16nm)
0x6~0xF : Reserved
*/

struct _nand_phy_info_par nand_tbl[] = {
	//SAMSUNG NAND FLASH
	//                NAND_CHIP_ID                     DieCnt SecCnt  PagCnt   BlkCnt      OpOpt    DatBlk  Freq   EccMode ReadRetry  DDRType  OperationPar   DDRInfo   random_cmd2_send_flag   random_addr_num (0:2cycle,3:5cycle)   nand_real_page_size
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//{ {0xec, 0xf1, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,     0,       0,        0,         0,       0,  0x000000, 20000,    0,    15},   // K9F1G08
	//{ {0xec, 0xf1, 0x00, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,     0,       0,        0,         0,       0,  0x000001, 20000,    0,    15},   // K9F1G08
	{ {0xec, 0xda, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     1024,   0x00000080,   896,    15,     0,       0,        0,         0,       0,  0x000002, 20000,    0,    15   },   // K9K2G08
	{ {0xec, 0xda, 0x10, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000008c,   896,    30,     0,       0,        0,         0,       0,  0x000003, 60000,    0,    30   },   // K9F2G08
	{ {0xec, 0xdc, 0xc1, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     2048,   0x00000080,   896,    15,     0,       0,        0,         0,       0,  0x000004, 20000,    0,    15   },   // K9K4G08
	{ {0xec, 0xdc, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000088,   896,    15,     0,       0,        0,         0,       0,  0x000005, 20000,    0,    15   },   // K9F4G08
	{ {0xec, 0xd3, 0x51, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000088,   896,    30,     0,       0,        0,         0,       0,  0x000006, 20000,    0,    30   },   // K9K8G08
	{ {0xec, 0xd3, 0x50, 0xa6, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000098,   896,    30,     0,       0,        0,         1,       0,  0x000007, 10000,    0,    30   },   // K9F8G08
	{ {0xec, 0xd5, 0x51, 0xa6, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x000000B8,   896,    30,     0,       0,        0,         1,       0,  0x000008, 10000,    0,    30   },   // K9KAG08
	{ {0xec, 0xdc, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000088,   896,    20,     0,       0,        0,         2,       0,  0x000009, 10000,    0,    20   },   // K9G4G08
	{ {0xec, 0xdc, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000088,   896,    30,     0,       0,        0,         2,       0,  0x00000a, 10000,    0,    30   },   // K9G4G08
	{ {0xec, 0xd3, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000088,   896,    20,     0,       0,        0,         2,       0,  0x00000b, 10000,    0,    20   },   // K9L8G08
	{ {0xec, 0xd3, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000088,   896,    30,     0,       0,        0,         2,       0,  0x00000c, 10000,    0,    30   },   // K9L8G08
	{ {0xec, 0xd3, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    20,     0,       0,        0,         2,       0,  0x00000d, 10000,    0,    20   },   // K9G8G08
	{ {0xec, 0xd3, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    30,     0,       0,        0,         2,       0,  0x00000e, 10000,    0,    30   },   // K9G8G08
	{ {0xec, 0xd5, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x000000A8,   896,    30,     0,       0,        0,         2,       0,  0x00000f, 10000,    0,    30   },   // K9LAG08
	{ {0xec, 0xd5, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x000000A8,   896,    30,     0,       0,        0,         2,       0,  0x000010, 10000,    0,    30   },   // K9LAG08
	{ {0xec, 0xd5, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     0,       0,        0,         3,       0,  0x000011, 10000,    0,    30   },   // K9GAG08
	{ {0xec, 0xd7, 0x55, 0xb6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x000000A8,   896,    30,     0,       0,        0,         3,       0,  0x000012, 10000,    0,    30   },   // K9LBG08
	{ {0xec, 0xd7, 0xd5, 0x29, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x000000A8,   896,    30,     0,       0,        0,         3,       0,  0x000013, 10000,    0,    30   },   // K9LBG08
	{ {0xec, 0xd7, 0x94, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    30,     2,       0,        0,         3,       0,  0x000014, 5000,    0,     30   },   // K9GBG08
	{ {0xec, 0xd5, 0x98, 0x71, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00000088,   896,    30,     3,       0,        0,         3,       0,  0x000015, 4000,    0,     30   },   // K9AAG08
	{ {0xec, 0xd5, 0x94, 0x29, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     0,       0,        0,         3,       0,  0x000016, 10000,    0,    30   },   // K9GAG08U0D
	{ {0xec, 0xd5, 0x84, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00000080,   896,    24,     2,       0,        0,         3,       0,  0x000017, 5000,    0,     24   },   // K9GAG08U0E
	{ {0xec, 0xd5, 0x94, 0x76, 0x54, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00000488,   896,    30,     2,       0,        0,         3,       0,  0x000018, 5000,    0,     30   },   // K9GAG08U0E
	{ {0xec, 0xd3, 0x84, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     1024,   0x00000080,   896,    24,     2,       0,        0,         3,       0,  0x000019, 5000,    0,     24   },   // K9G8G08U0C
	{ {0xec, 0xd7, 0x94, 0x76, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    30,     3,       0,        0,         3,       0,  0x00001a, 4000,    0,     30, 0, 0, 8 * 1024 + 640},   // K9GBG08U0A
	{ {0xec, 0xde, 0xd5, 0x7A, 0x58, 0xff, 0xff, 0xff }, 2,    16,     128,     4096,   0x00000888,   896,    30,     3,       0,        0,         3,       0,  0x00001c, 4000,    0,     30, 0, 0, 8 * 1024 + 640},   // K9LCG08U0A
	{ {0xec, 0xd7, 0x94, 0x7A, 0x54, 0xc3, 0xff, 0xff }, 1,    16,     128,     4096,   0x20000088,   896,    60,     1,       0,        3,         3,       0,  0x00001d, 4000,    0,     60   },   // toogle nand 1.0
	{ {0xec, 0xd7, 0x94, 0x7A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    30,     3,       0,        0,         3,       0,  0x00001b, 8000,    0,     30   },   // K9GBG08U0A
	{ {0xec, 0xde, 0xa4, 0x7a, 0x68, 0xc4, 0xff, 0xff }, 1,    16,     128,     8192,   0x20020588,   896,    60,     4,   0x200e04,     3,         3,       0,  0x00001e, 3500,    5,     60   },   // toogle nand 2.0 K9GCGD8U0A
	{ {0xec, 0xd7, 0x94, 0x7E, 0x64, 0xc4, 0xff, 0xff }, 1,    16,     128,     4096,   0x20020588,   896,    60,     4,   0x200e04,     3,         3,       0,  0x00001f, 3500,    5,     60, 0, 0, (8*1024 + 1024)},   // toogle nand 2.0 K9GBGD8U0B
	{ {0xec, 0xd7, 0x94, 0x7e, 0x64, 0x44, 0xff, 0xff }, 1,    16,     128,     4096,   0x00020188,   896,    40,     4,   0x200e04,     0,         3,       0,  0x000020, 3500,    5,     40, 0, 0, (8*1024 + 1024)},   // 21nm sdr K9GBG08U0B
	{ {0xec, 0xde, 0xd5, 0x7e, 0x68, 0x44, 0xff, 0xff }, 2,    16,     128,     4096,   0x00020188,   896,    30,     4,   0x200e04,     0,         3,       0,  0x000021, 3500,    5,     30   },   // 21nm sdr K9LCG08U0B
	{ {0xec, 0xd7, 0x14, 0x76, 0x54, 0xc2, 0xff, 0xff }, 1,    16,     128,     4096,   0x20000088,   896,    30,     3,       0,        3,         3,       0,  0x000022, 4000,    0,     30   },   // K9GBGD8U0M
	{ {0xec, 0xde, 0xd5, 0x7A, 0x58, 0x43, 0xff, 0xff }, 1,    16,     128,     4196,   0x00000888,   896,    30,     3,       0,        0,         3,       0,  0x000023, 0,       0,     30   },   // K9HDG08U1A
	{ {0xec, 0xda, 0x10, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000088,   896,    25,     0,       0,        0,        10,       0,  0x000024, 20000,   0,     25   },   // K9F2G08U0C
	{ {0xec, 0xf1, 0x00, 0x95, 0x41, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    30,     0,       0,        0,        11,       0,  0x000025, 20000,   0,     30   },   // K9F1G08U0E
	{ {0xec, 0xf1, 0x00, 0x95, 0x40, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    30,     0,       0,        0,        11,       0,  0x000026, 20000,   0,     30   },   // K9F1G08U0F
	{ {0xec, 0xdc, 0x10, 0x95, 0x56, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000088,   896,    30,     0,       0,        0,         0,       0,  0x000027, 20000,   0,     30   },   // K9F4G08U0F //FS33ND04GS108TFI0
	{
		/* K9F2G08U0D */
		.nand_id = {0xec, 0xda, 0x10, 0x95, 0x46, 0xff, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 2048,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 20,
		.ecc_mode = 0,
		.read_retry_type = 0,
		.ddr_type = 0,
		.option_phyisc_op_no = 15,
		.ddr_info_no = 1,
		.id_number = 0x000028,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 20
	},

	/* FORESEE NAND FLASH */
	{
		/* FS33ND02GS2 */
		/* The ID on spec marks: (hex) ec, dc, 10, 95, 56.
		 * Unfortunately, the id is the same as K9F4G08U0F and
		 * FS33ND04GS108TFI0. They are different nand since different
		 * size. So, I tried to get more id number from nand and got
		 * that: (hex) ec, dc, 10, 95, 56, c6, ec, dc. To be more
		 * compatible, the last two bytes are set to 0xFF.
		 */
		.nand_id = {0xec, 0xdc, 0x10, 0x95, 0x56, 0xc6, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 2048,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 40,
		.ecc_mode = 0,
		.read_retry_type = 0,
		.ddr_type = 0,
		.option_phyisc_op_no = 15,
		.ddr_info_no = 0,
		.id_number = 0x000000,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 40
	},

	//HYNIX NAND FLASH
	//                NAND_CHIP_ID                     DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar  DDRInfo
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0xad, 0xf1, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    15,     0,      0,         0,         0,       0,  0x010000, 20000,    0,    15   },   // HY27UF081G2M
	{ {0xad, 0xf1, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010001, 20000,    0,    20   },   // HY27UF081G2A
	{ {0xad, 0xf1, 0x00, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010002, 20000,    0,    20   },   // H27U1G8F2B
	{ {0xad, 0xda, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    15,     0,      0,         0,         0,       0,  0x010003, 20000,    0,    15   },   // HY27UF082G2M
	{ {0xad, 0xda, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010004, 20000,    0,    20   },   // HY27UF082G2A
	{ {0xad, 0xda, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010005, 20000,    0,    20   },   // HY27UF082G2B
	{ {0xad, 0xdc, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 4,     4,      64,     1024,   0x00000080,   896,    15,     0,      0,         0,         0,       0,  0x010006, 20000,    0,    15   },   // HY27UH084G2M
	{ {0xad, 0xdc, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010007, 20000,    0,    20   },   // HY27UF084G2M, HY27UG088G5M
	{ {0xad, 0xdc, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000088,   896,    20,     0,      0,         0,         0,       0,  0x010008, 20000,    0,    20   },   // HY27UF084G2B, HY27UG088G5B
	{ {0xad, 0xd3, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 4,     4,      64,     2048,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x010009, 20000,    0,    20   },   // HY27UG084G2M, HY27H088G2M
	{ {0xad, 0xd3, 0xc1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000080,   896,    20,     0,      0,         0,         0,       0,  0x01000a, 20000,    0,    20   },   // HY27UG088G2M, HY27UH08AG5M
	{ {0xad, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000080,   896,    12,     0,      0,         0,         2,       0,  0x01000b, 10000,    0,    12   },   // HY27UT084G2M, HY27UU088G5M
	{ {0xad, 0xdc, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000088,   896,    15,     0,      0,         0,         2,       0,  0x01000c, 10000,    0,    15   },   // HY27U4G8T2BTR
	{ {0xad, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000080,   896,    10,     0,      0,         0,         2,       0,  0x01000d, 10000,    0,    10   },   // HY27UV08AG5M, HY27UW08BGFM
	{ {0xad, 0xd3, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    12,     0,      0,         0,         2,       0,  0x01000e, 10000,    0,    12   },   // HY27UT088G2M, HY27UU08AG5M
	{ {0xad, 0xd3, 0x14, 0x2d, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    25,     0,      0,         0,         2,       0,  0x01000f, 10000,    0,    25   },   // HY27UT088G2M, HY27UU08AG5M
	{ {0xad, 0xd3, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    15,     0,      0,         0,         2,       0,  0x010010, 10000,    0,    15   },   // HY27UT088G2M, HY27UU08AG5M
	{ {0xad, 0xd5, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000088,   896,    15,     0,      0,         0,         2,       0,  0x010011, 10000,    0,    15   },   // HY27UV08BG5M, HY27UW08CGFM
	{ {0xad, 0xd5, 0x55, 0x2d, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000088,   896,    25,     0,      0,         0,         2,       0,  0x010012, 10000,    0,    25   },   // HY27UV08BG5M, HY27UW08CGFM
	{ {0xad, 0xd5, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     8192,   0x00000088,   896,    30,     0,      0,         0,         2,       0,  0x010013, 10000,    0,    30   },   // HY27UV08BG5M, HY27UW08CGFM
	{ {0xad, 0xd3, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     2048,   0x00000088,   896,    30,     0,      0,         0,         3,       0,  0x010014, 8000,    0,    30    },   // H27U8G8T2B
	{ {0xad, 0xd5, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     0,      0,         0,         3,       0,  0x010015, 8000,    0,    30    },   // H27UAG8T2M, H27UBG8U5M
	{ {0xad, 0xd7, 0x55, 0xb6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000088,   896,    30,     0,      0,         0,         3,       0,  0x010016, 8000,    0,    30    },   // H27UCG8V5M
	{ {0xad, 0xd5, 0x94, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x010017, 5000,    0,    30    },   // H27UBG8U5A
	{ {0xad, 0xd7, 0x95, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x010018, 5000,    0,    30    },   // H27UCG8V5A
	{ {0xad, 0xd5, 0x95, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x010019, 5000,    0,    30    },   // H27UCG8VFA
	{ {0xad, 0xd5, 0x94, 0x9A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     1024,   0x00000080,   896,    30,     2,      0,         0,         3,       0,  0x01001a, 5000,    0,    30    },   // H27UAG8T2B
	{ {0xad, 0xd7, 0x94, 0x9A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x01001b, 5000,    0,    30    },   // H27UBG8T2A H27UCG8U5(D)A H27UDG8VF(D)A
	{ {0xad, 0xde, 0xd5, 0x9A, 0xff, 0xff, 0xff, 0xff }, 2,    16,     256,     2048,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x01001c, 5000,    0,    30    },   // H27UDG8V5A
	{ {0xad, 0xd7, 0x94, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000088,   896,    30,     2,      0,         0,         3,       0,  0x01001d, 5000,    0,    30    },   // H27UBG8T2M
	{ {0xad, 0xde, 0x94, 0xd2, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00001188,   896,    30,     2,  0x000604,      0,         3,       0,  0x01001e, 5000,    3,    30    },   // H27UCG8T2M
	{ {0xad, 0xd7, 0x94, 0xda, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x00001188,   896,    30,     3,  0x010604,      0,         3,       0,  0x01001f, 4000,    3,    30,  0, 0, 8192+832 },   // H27UBG8T2B
	{ {0xad, 0xde, 0x94, 0xda, 0x74, 0xff, 0xff, 0xff }, 1,    16,     256,     4120,   0x00001188,   896,    40,     4,  0x020708,      0,         3,       0,  0x010020, 2000,    2,    40    },   // H27UCG8T2A
	{ {0xad, 0xd7, 0x94, 0x91, 0x60, 0xff, 0xff, 0xff }, 1,    16,     256,     2092,   0x00001188,   896,    40,     4,  0x030708,      0,         3,       0,  0x010021, 2000,    2,    40    },   // H27UBG8T2C
	{ {0xad, 0xde, 0x94, 0xeb, 0x74, 0xff, 0xff, 0xff }, 1,    32,     256,     2132,   0x00001188,   896,    40,     4,  0x030708,      0,         3,       0,  0x010022, 2000,    2,    40    },   // H27UCG8T2B 20nm 8G
	{ {0xad, 0xde, 0x14, 0xA7, 0x42, 0x4a, 0xff, 0xff }, 1,    32,     256,     2120,   0x00002188,   896,    40,     6,  0x040704,      0,         3,       0,  0x010023, 2000,    1,    40,     0, 0, 16384+1664},   // H27UCG8T2ETR 16nm 8G
	{ {0xad, 0xd7, 0x14, 0x9e, 0x34, 0x4a, 0xff, 0xff }, 1,    16,     256,     2112,   0x00002188,   896,    40,     6,  0x040704,      0,         3,       0,  0x010024, 2000,    8,    40,     0, 0, 8192+832 },   // H27UBG8T2DTR 16nm 4G
	{ {0xad, 0xde, 0x94, 0xA7, 0x42, 0x48, 0xff, 0xff }, 1,    32,     256,     2120,   0x00002188,   896,    30,     6,  0x040704,      0,         3,       0,  0x010025, 2000,    1,    30    },   // H27UCG8T2ETR 16nm 8G customer's nand
	{ {0xad, 0xde, 0x14, 0xab, 0x42, 0x4a, 0xff, 0xff }, 1,    32,     256,     2112,   0x00002188,   896,    40,     8,  0x000604,      0,        14,       0,  0x010026, 3000,    1,    40,     0, 0, 16384+1664},   // I3T-8GQ8T2H5TARC(H27UCG8T2FTR-BC) 16nm 8G
	{ {0xad, 0xda, 0x90, 0x95, 0x46, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000088,   896,    30,     3,      0,         0,        15,       0,  0x010027, 50000,   0,    30    },   // FS33ND02GH208TFCX 2Gbit Foresee

	//TOSHIBA NAND FLASH
	//                    NAND_CHIP_ID                 DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x98, 0xf1, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    20,     0,     0,          0,         5,       0,  0x020000, 20000,    0,    20   },   // TC58NVG0S3B
	{ {0x98, 0xda, 0xff, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    20,     0,     0,          0,         5,       0,  0x020001, 20000,    0,    20   },   // TC58NVG1S3B
	{ {0x98, 0xdc, 0x81, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000080,   896,    20,     0,     0,          0,         5,       0,  0x020002, 20000,    0,    20   },   // TC58NVG2S3B
	{ {0x98, 0xd1, 0x90, 0x15, 0x76, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    25,     0,     0,          0,         9,       0,  0x020003, 20000,    0,    25   },   // TC58NVG0S3E
	{ {0x98, 0xda, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     1024,   0x00000080,   896,    20,     0,     0,          0,         6,       0,  0x020004, 10000,    0,    20   },   // TC58NVG1D4B
	{ {0x98, 0xdc, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000088,   896,    20,     0,     0,          0,         6,       0,  0x020005, 10000,    0,    20   },   // TC58NVG2D4B
	{ {0x98, 0xd3, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    20,     0,     0,          0,         7,       0,  0x020006, 10000,    0,    20   },   // TC58NVG3D4C
	{ {0x98, 0xd5, 0x85, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000088,   896,    20,     0,     0,          0,         7,       0,  0x020007, 10000,    0,    20   },   // TC58NVG4D4C, TC58NVG5D4C
	{ {0x98, 0xd3, 0x94, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     2048,   0x00000088,   896,    20,     0,     0,          0,         6,       0,  0x020008, 10000,    0,    20   },   // TC58NVG3D1DTG00
	{ {0x98, 0xd7, 0x95, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000088,   896,    30,     2,     0,          0,         7,       0,  0x020009, 5000,    0,    30    },   // TC58NVG6D1DTG20
	{ {0x98, 0xd5, 0x94, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     2,     0,          0,         7,       0,  0x02000a, 5000,    0,    30    },    // TH58NVG5D1DTG20
	{ {0x98, 0xd5, 0x94, 0x32, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00000088,   896,    25,     1,     0,          0,         8,       0,  0x02000b, 8000,    0,    25    },    // TH58NVG4D2ETA20 TH58NVG4D2FTA20 TH58NVG5D2ETA00
	{ {0x98, 0xd7, 0x94, 0x32, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    25,     2,     0,          0,         8,       0,  0x02000c, 5000,    0,    25    },    // TH58NVG5D2FTA00 TH58NVG6D2FTA20
	{ {0x98, 0xd7, 0x95, 0x32, 0xff, 0xff, 0xff, 0xff }, 2,    16,     128,     4096,   0x00000088,   896,    25,     1,     0,          0,         8,       0,  0x02000d, 8000,    0,    25    },    // TH58NVG6D2ETA20
	{ {0x98, 0xde, 0x94, 0x82, 0x76, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00010588,   896,    40,     4, 0x100604,       0,         9,       0,  0x02000e, 4000,    6,    40    },    // TH58NVG6D2ETA20
	{ {0x98, 0xd7, 0x94, 0x32, 0x76, 0x56, 0xff, 0xff }, 1,    16,     128,     4096,   0x00010588,   896,    40,     4, 0x100604,       0,         9,       0,  0x02000f, 4000,    6,    40    },    // TH58NVG5D2HTA20
	{ {0x98, 0xd5, 0x84, 0x32, 0x72, 0x56, 0xff, 0xff }, 1,    16,     128,     2048,   0x00010580,   896,    40,     4, 0x100604,       0,         9,       0,  0x020010, 4000,    6,    40    },    // TH58NVG4D2HTA20
	{ {0x98, 0xde, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,    32,     256,     2092,   0x00010580,   864,    40,     4, 0x100604,       0,         9,       0,  0x020011, 4000,    6,    40    },    // TC58NVG6DCJTA00
	{ {0x98, 0xd7, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,    32,     256,     1060,   0x00010580,   864,    40,     4, 0x100604,       0,         9,       0,  0x020012, 4000,    6,    40    },    // TC58TEG5DCJTA00
	{ {0x98, 0xde, 0x94, 0x93, 0x76, 0x57, 0xff, 0xff }, 1,    32,     256,     2116,   0x00010580,   864,    40,     4, 0x100604,       0,         9,       0,  0x020013, 4000,    6,    40    },    // TC58TEG6DDJTA00
	{ {0x98, 0xde, 0x84, 0x93, 0x72, 0xd7, 0xff, 0xff }, 1,    32,     256,     2092,   0x20010580,   864,    40,     4, 0x100604,       3,         9,       1,  0x020014, 4000,    6,    40    },    // TC58TEG6DCJTA00
	{ {0x98, 0xd7, 0xd4, 0x32, 0x76, 0x55, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    30,     2,     0,          0,         8,       0,  0x020015, 4000,    0,    30    },    // TH58NVG6E2FTA00
	{ {0x98, 0xde, 0x94, 0x93, 0x76, 0x50, 0xff, 0xff }, 1,    32,     256,     2132,   0x00010588,   840,    40,     4, 0x110705,       0,         9,       0,  0x020016, 4000,    6,    40    },    // TH58TEG7DDKTA20 or TC58TEG6DDKTA00(A19nm)

	{ {0x98, 0xde, 0xa4, 0x82, 0x76, 0xd7, 0xff, 0xff }, 1,    16,     256,     4180,   0x20010588,   864,    30,     4, 0x100604,       3,         9,       1,  0x020017, 3000,    6,    30    },    //
	{ {0x98, 0xd7, 0x84, 0x93, 0x72, 0x50, 0xff, 0xff }, 1,    32,     256,     1060,   0x00010580,   840,    40,     4, 0x110705,       0,         9,       0,  0x020018, 3000,    6,    40    },    // TC58TEG5DCKTA00(A19nm)
	{ {0x98, 0xde, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,    32,     256,     2148,   0x00010588,   840,    40,     4, 0x120a05,       0,         9,       0,  0x020019, 3000,    6,    40    },    // TC58TEG6DDLTA00(15nm)
	{ {0x98, 0x3a, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,    32,     256,     4212,   0x00010588,   840,    40,     4, 0x120a05,       0,         9,       0,  0x02001a, 3000,    6,    40    },    // TC58TFG7DDLTA0D/TH58TFG8DDLTA2D(15nm)
	{ {0x98, 0xda, 0x90, 0x15, 0x76, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,    0x00000088,   896,    25,     3, 	    0,       0,         9,       0,  0x02001b, 10000,   0,    25     },    // TC58NVG1S3HTA00
	{ {0x98, 0xd3, 0x91, 0x26, 0x76, 0xff, 0xff, 0xff }, 1,     8,      64,     4096,    0x00000088,   896,    30,     3, 	    0,       0,         9,       0,  0x02001c, 10000,   0,    30     },    // TC58NVG3S0HTAI0
	{ {0x98, 0xdc, 0x90, 0x26, 0x76, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,    0x00000088,   896,    30,     3, 	    0,       0,         9,       0,  0x02001d, 10000,   0,    30     },    // TC58NVG2S0HTA00
	{ {0x98, 0xdc, 0x90, 0x26, 0xf6, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,    0x00000088,   896,    30,     0,       0,       0,         9,       0,  0x02001e, 30000,   0,    30     },    // TC58BVG2S0HTAI0, need to test
	{ {0x98, 0xf1, 0x80, 0x15, 0x72, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,    0x00000080,   896,    30,     3,       0,       0,         10,      0,  0x02001f, 30000,   0,    30     },   // XT27G01ATSIGA, need to test
	{
		/* TC58BVG1S3HTAI0 */
		.nand_id = {0x98, 0xda, 0x90, 0x15, 0xf6, 0xff, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 2048,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 35,
		.ecc_mode = 0,
		.read_retry_type = 0,
		.ddr_type = 0,
		.option_phyisc_op_no = 15,
		.ddr_info_no = 0,
		.id_number = 0x000000,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 35
	},

	//MICON NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x2c, 0xda, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    25,     0,     0,          0,         4,       0,  0x030000, 20000,    0,    25   },   // MT29F2G08AAC, JS29F02G08AAN
	{ {0x2c, 0xdc, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     2048,   0x00000080,   896,    25,     0,     0,          0,         4,       0,  0x030001, 20000,    0,    25   },   // MT29F4G08BAB, MT29F8G08FAB, JS29F04G08BAN, JS29F08G08FAN
	{ {0x2c, 0xdc, 0x90, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000088,   896,    25,     0,     0,          0,         4,       0,  0x030002, 20000,    0,    25   },   // MT29F4G08AAA, MT29F8G08DAA, JS29F04G08AAN
	{ {0x2c, 0xd3, 0xd1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000088,   896,    25,     0,     0,          0,         4,       0,  0x030003, 20000,    0,    25   },   // MT29F8G08BAB, MT29F16G08FAB, JS29F08G08BAN, JS29F16G08FAN
	{ {0x2c, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000080,   896,    20,     0,     0,          0,         4,       0,  0x030004, 10000,    0,    20   },   // MT29F4G08MAA, MT29F8G08QAA
	{ {0x2c, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000080,   896,    20,     0,     0,          0,         4,       0,  0x030005, 10000,    0,    20   },   // MT29F16GTAA
	{ {0x2c, 0xd3, 0x94, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    30,     0,     0,          0,         4,       0,  0x030006, 10000,    0,    30   },   // MT29F8G08MAA, MT29F16G08QAA, JS29F08G08AAM, JS29F16G08CAM
	{ {0x2c, 0xd5, 0x95, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000088,   896,    20,     0,     0,          0,         4,       0,  0x030007, 10000,    0,    20   },   // MT29F32G08TAA, JS29F32G08FAM
	{ {0x2c, 0xd5, 0xd5, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x000000A8,   896,    20,     0,     0,          0,         4,       0,  0x030008, 10000,    0,    20   },   // MT29F32G08TAA, JS29F32G08FAM
	{ {0x2c, 0xd5, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   896,    30,     0,     0,          0,         4,       0,  0x030009, 10000,    0,    30   },   // MT29F16G08MAA, MT29F32G08QAA, JS29F32G08AAM, JS29F32G08CAM
	{ {0x2c, 0xd5, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000088,   896,    30,     0,     0,          0,         4,       0,  0x03000a, 10000,    0,    30   },   // MT29F64G08TAA, JS29F64G08FAM
	{ {0x2c, 0xd7, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000288,   896,    30,     2,     0,          0,         4,       0,  0x03000b, 5000,    0,    30    },   // MT29F32G08CBAAA,MT29F64G08CFAAA
	{ {0x2c, 0xd7, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000088,   896,    30,     2,     0,          0,         4,       0,  0x03000c, 5000,    0,    30    },   // MT29F64G08CTAA
	{ {0x2c, 0xd9, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     8192,   0x00000088,   896,    30,     2,     0,          0,         4,       0,  0x03000d, 5000,    0,    30    },   // MT29F128G08,
	{ {0x2c, 0x68, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00000288,   896,    30,     2,     0,          0,         4,       0,  0x03000e, 5000,    0,    30    },   // MT29F32G08CBABA
	{ {0x2c, 0x88, 0x05, 0xC6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     256,     4096,   0x00000288,   896,    30,     2,     0,          0,         4,       0,  0x03000f, 5000,    0,    30    },   // MT29F128G08CJABA
	{ {0x2c, 0x88, 0x04, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00000288,   896,    40,     2,     0,          0,         4,       0,  0x030010, 5000,    0,    40    },   // MT29F64G08CBAAA
	{ {0x2c, 0x68, 0x04, 0x4A, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00000288,   896,    40,     2,     0,          0,         4,       0,  0x030011, 5000,    0,    40    },   // MT29F32G08CBACA
	{ {0x2c, 0x48, 0x04, 0x4A, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00000288,   896,    40,     2,     0,          0,         4,       0,  0x030012, 5000,    0,    40    },   // MT29F16G08CBACA
	{ {0x2c, 0x48, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00000288,   896,    30,     2,     0,          0,         4,       0,  0x030013, 5000,    0,    30    },   // MT29F16G08CBABA
	{ {0x2c, 0x64, 0x44, 0x4B, 0xA9, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x1F041788,   896,    30,     5,   0x400a01,     0,       4,       0,  0x030014,  4000,    10,   30},   // MT29F64G08CBABA  MT29F128G08CFABA
	{ {0x2c, 0x44, 0x44, 0x4B, 0xA9, 0xff, 0xff, 0xff }, 1,    16,     256,     2128,   0x1f041788,   896,    30,     5,   0x400a01,     0,      4,       1,  0x030015, 4000,    10,    30, 0, 0, 8192 + 744},   // MT29F32G08CBADA
	{ {0x2c, 0x88, 0x24, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00000788,   870,    40,     2,   0x400a01,     0,      4,       0,  0x030016, 5000,    0,    40       },   // MT29F64G08CBAAA(special for customer's nand)
	{ {0x2c, 0x84, 0x64, 0x3c, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     2048,   0x1f041788,   896,    40,     4,   0x400a01,     0,      4,       1,  0x030017, 4000,    10,    40, 0, 0, 16384 + 1216},   // MT29F128G08CBCAB(20nm, L85A)
	{ {0x2c, 0x84, 0x64, 0x54, 0xa9, 0xff, 0xff, 0xff }, 1,    32,     512,     2096,   0x1f042788,   896,    40,     8,   0x410c01,     0,      4,       1,  0x030018, 3500,    11,    40, 0, 0, 16384 + 1872},   // MT29F128G08CBECB(16nm, L95B)
	{ {0x2c, 0x84, 0xc5, 0x4b, 0xa9, 0xff, 0xff, 0xff }, 2,    16,     256,     4096,   0x00041788,   896,    40,     5,   0x400a01,     0,      4,       0,  0x030019, 3500,    10,    40, 0, 0, 8192 + 744},   // MT29F256G08CJABA
	{ {0x2c, 0x64, 0x64, 0x3c, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     1048,   0x1f041788,   896,    40,     4,   0x410c01,     0,      4,       0,  0x03001a, 3500,    10,    40      },   // MT29F64G08CBCEB   MT29F64G08CBEEB(20nm, L84D,customer's nand)
	{ {0x2c, 0xf1, 0x80, 0x95, 0x04, 0xff, 0xff, 0xff }, 1,     4,       64,     1024,   0x4000008c,   896,   30,     0,       0,        0,     10,       0,  0x03001b, 60000,    0,    30   },   // MT29F1G08ABAEA
	{ {0x2c, 0xda, 0x90, 0x95, 0x04, 0xff, 0xff, 0xff }, 1,     4,       64,     2048,   0x0000008c,   896,   30,     7,       0,        0,      4,       0,  0x03001c, 60000,    0,    30   },   // MT29F2G08ABAFA
	{ {0x2c, 0xda, 0x90, 0x95, 0x06, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000028c,   896,    40,     0,       0,        0,      4,       0,  0x03001d, 60000,    0,    40},   // MT29F2G08ABAEA
	{ {0x2c, 0x84, 0x64, 0x3c, 0xa9, 0x04, 0xff, 0xff }, 1,    32,     512,     2096,   0x1f041388,   896,    75,     4,   0x410c01,    0x2,    13,       1,  0x03001e,  1000,   10,    75}, //MT29F128G08CBEBB (L85C)
	{ {0x2c, 0x64, 0x44, 0x32, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     1088,   0x9f000788,   896,    75,     8,   0x410c01,    0x2,     4,       1,  0x03001f,  1000,   12,    75}, // MT29F64G08CBCGB   (L04a 3D nand)
	{ {0x2c, 0x84, 0x44, 0x32, 0xaa, 0x04, 0x00, 0x00 }, 1,    32,     512,     2192,   0x9f000780,   896,    75,     13,  0x410c01,    0x2,     4,       1,  0x030020,  1000,   12,    75}, // FNNL05B128G1KDBABJ4   (L05B 3D nand)
	{
		/* FBNL06B256G1KDBABJ4(L06B) */
		.nand_id = {0x2c, 0xa4, 0x64, 0x32, 0xaa, 0x04, 0xff, 0xff },
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 32,
		.page_cnt_per_blk = 1024,
		.blk_cnt_per_die = 2192,
		.operation_opt = NAND_RANDOM | NAND_READ_RETRY | NAND_READ_UNIQUE_ID | NAND_PAGE_ADR_NO_SKIP | NAND_VCCQ_1p8V |
			NAND_TIMING_MODE | NAND_DDR2_CFG | NAND_IO_DRIVER_STRENGTH | NAND_VENDOR_SPECIFIC_CFG |
			NAND_ONFI_SYNC_RESET_OP,
		.valid_blk_ratio = 896,
		.access_freq = 75,
		.ecc_mode = BCH_76,
		.read_retry_type = 0x421201,
		.ddr_type = ONFI_DDR,
		.option_phyisc_op_no = 13,
		.ddr_info_no = 1,
		.id_number = 0x030021,
		.max_blk_erase_times = 1000,
		.driver_no = 12,
		.access_high_freq = 75,
		.nand_real_page_size = 18592
	},
	//INTEL NAND FLASH
	//                 NAND_CHIP_ID                    DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x89, 0xd3, 0x94, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000088,   896,    30,     0,     0,          0,         4,        0,  0x040000, 20000,   0,    30   },   // 29F08G08AAMB2, 29F16G08CAMB2
	{ {0x89, 0xd5, 0xd5, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x000000A8,   896,    20,     0,     0,          0,         4,        0,  0x040001, 20000,   0,    20   },   // 29F32G08FAMB2
	{ {0x89, 0xd7, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000088,   864,    30,     2,     0,          0,         4,        0,  0x040002, 5000,   0,    30    },   // MLC32GW8IMA,MLC64GW8IMA, 29F32G08AAMD2, 29F64G08CAMD2
	{ {0x89, 0xd5, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000088,   864,    30,     2,     0,          0,         4,        0,  0x040003, 5000,   0,    30    },   // 29F32G08CAMC1
	{ {0x89, 0xd7, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000088,   864,    30,     2,     0,          0,         4,        0,  0x040004, 5000,   0,    30    },   // 29F64G08FAMC1
	{ {0x89, 0x68, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00000288,   864,    30,     2,     0,          0,         4,        0,  0x040005, 5000,   0,    30    },   // 29F32G08AAMDB
	{ {0x89, 0x88, 0x24, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x1f000288,   864,    40,     2,     0,          0,         4,        0,  0x040006, 5000,   0,    40    },   // 29F64G08CBAAA 29F64G083AME1 29F64G08ACME3
	{ {0x89, 0xA8, 0x25, 0xCB, 0xff, 0xff, 0xff, 0xff }, 2,    16,     256,     4096,   0x00000288,   864,    30,     2,     0,          0,         4,        0,  0x040007, 5000,   0,    30    },   // ?
	{ {0x89, 0x88, 0x24, 0x4B, 0xa9, 0x84, 0xff, 0xff }, 1,    16,     256,     4096,   0x1f050788,   864,    30,     4,  0x500701,      0,         4,        1,  0x040008, 3000,   4,    30,  0,  0,  8192 + 744},   // JS29F64G08ACMF3 JS29F16B08CCMF3 JS29F32B08JCMF3
	{ {0x89, 0x88, 0x24, 0x4B, 0xa9, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00000288,   864,    40,     2,     0,          0,         4,        0,  0x040009, 0,   0,    40},
	{ {0x89, 0x84, 0x64, 0x3c, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     2048,   0x1f000788,   896,    40,     4,  0x500701,      0,         4,        0,  0x04000a, 3000,  12,    40},   // PF2916B08LCMFs/PF2916B08LCMF2/EM29DG08MBVTA

	{ {0x89, 0x88, 0x24, 0x4B, 0xa9, 0x00, 0x00, 0x00 }, 1,    16,     256,     4096,   0x00000288,   864,    40,     2,     0,          0,         4,        0,  0x040009, 0,   0,    40,     0,  0,  8192 + 448},

	//SANDDISK NAND
	//                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar  DDRInfo
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x45, 0xde, 0x94, 0x93, 0xff, 0xff, 0xff, 0xff }, 1,     32,     256,    2116,   0x00030188,   896,    40,     4,      0x301409,  0,         0,        0,  0x080000, 4000,   7,    40, 0, 0, 16384 + 1280},   // SDTNQGAMA-008G
	{ {0x45, 0xd7, 0x84, 0x93, 0xff, 0xff, 0xff, 0xff }, 1,     32,     256,    1060,   0x00030180,   896,    40,     4,      0x301409,  0,         0,        0,  0x080001, 4000,   7,    40    },   // SDTNQFAMA-004G
	{ {0x45, 0xde, 0xa4, 0x82, 0x76, 0xd7, 0xff, 0xff }, 1,     16,     256,    4096,   0x20030188,   896,    40,     4,      0x301409,  3,         0,        0,  0x080002, 4000,   7,    40    },   // 008G
	{ {0x45, 0xde, 0x94, 0x93, 0x76, 0xd7, 0xff, 0xff }, 1,     32,     256,    2116,   0x20030188,   896,    40,     4,      0x301409,  3,         0,        1,  0x080003, 4000,   7,    40    },   // SDTNQGAMA-008GP
	{ {0x45, 0xde, 0xa4, 0x82, 0x76, 0x56, 0xff, 0xff }, 1,     16,     256,    4096,   0x00030188,   944,    40,     4,      0x310909,  0,         0,        0,  0x080004, 4000,   7,    40    },   // SDTNPMAHEM-008G
	{ {0x45, 0xde, 0x94, 0x93, 0x76, 0x50, 0xff, 0xff }, 1,     32,     256,    2132,   0x00030188,   944,    40,     4,      0x321f04,  0,         0,        0,  0x080005, 4000,   9,    40, 0, 0, 16384 + 1280},   // SDTNRGAMA-008G
	{ {0x45, 0xd7, 0x84, 0x93, 0x72, 0x50, 0xff, 0xff }, 1, 	32,	    256, 	  1060,	0x00030180,   944,    40,	  4,	  0x331f04,  0,	        0,        0,  0x080006, 3500,   9,       40, 0, 0, 16384 + 1280    },   // SDTNRFAMA-004Gk
	{ {0x45, 0xde, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,     32,     256,    2048,   0x00030180,   864,    30,     4,      0x301409,  0,         0,        0,  0x080007, 4000,   7,   40, 0, 0, 16384 + 1280},   //19nm
	{ {0x45, 0x3a, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,     32,     256,    4212,   0x08030188,   896,    75,     4, 	  0x342009,  3,     	0,  	  1,  0x080008, 3000,   9,    40},   // SDTNSGAMA-016G(M)(1Znm)
	{ {0x45, 0xde, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,     32,     256,    2148,   0x08030188,   896,    15,     4, 	  0x351f09,  3,     	0,  	  1,  0x080009, 3000,   9,    15},   // SDTNSGAMA-008GM(1Znm)



	//ST NAND FLASH
	//              NAND_CHIP_ID                       DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar  DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x20, 0xf1, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050000, 20000,   0,    15   },  // NAND01GW3B
	{ {0x20, 0xf1, 0x00, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050001, 20000,   0,    15   },  // NAND01G001
	{ {0x20, 0xda, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050002, 20000,   0,    15   },  // NAND02GW3B
	{ {0x20, 0xda, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050003, 20000,   0,    15   },  // NAND02GW3B2DN6
	{ {0x20, 0xdc, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050004, 20000,   0,    15   },  // NAND04GW3B
	{ {0x20, 0xd3, 0xc1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000080,   896,    15,    0,       0,         0,         0,        0,  0x050005, 20000,   0,    15   },  // NAND08GW3B
	{ {0x20, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000080,   896,    15,    0,       0,         0,         2,        0,  0x050006, 10000,   0,    15   },  // NAND04GW3C
	{ {0x20, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000080,   896,    15,    0,       0,         0,         2,        0,  0x050007, 10000,   0,    15   },  // NAND08GW3C
	{ {0x20, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 4,     4,     128,     2048,   0x00000080,   896,    15,    0,       0,         0,         2,        0,  0x050008, 10000,   0,    15   },  // NAND16GW3C

	//SPANSION NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x01, 0xaa, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000080,   896,    30,     0,      0,         0,         0,        0,  0x060000, 20000,   0,    30   },   // S39MS02G
	{ {0x01, 0xa1, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    30,     0,      0,         0,         0,        0,  0x060001, 20000,   0,    30   },   // S39MS01G
	{ {0x01, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    30,     0,      0,         0,         0,        0,  0x060002, 20000,   0,    30   },   // DFT01GR08P1PM0
	{ {0x01, 0xda, 0x90, 0x95, 0x46, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000088,   896,    25,     3,      0,         0,         0,        0,  0x060003, 20000,   0,    25   },	//S34ML02G2
	{ {0x01, 0xdc, 0x90, 0x95, 0x56, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000088,   896,    30,     3,      0,         0,         0,        0,  0x060004, 60000,   0,    30   },	//S34ML04G2
	{ {0x01, 0xda, 0x90, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000088,   896,    30,     0,      0,         0,         0,        0,  0x060005, 60000,   0,    30},	  //S34ML02G100TFV00
	{ {0x01, 0xf1, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000080,   896,    30,     0,      0,         0,         11,       0,  0x060006, 60000,   0,    30  },  //S34ML01G200, need to test
	//POWER NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0x92, 0xf1, 0x80, 0x95, 0x40, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    30,     0,      0,         0,        10,        0,  0x070000, 60000,   0,    30   },   // PSU1GA30HT

	//MXIC NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0xc2, 0xda, 0x90, 0x95, 0x07, 0xff, 0xff, 0xff}, 1,     4,      64,     2048,   0x0000028c,   944,    30,     2,           0,    0,           4,        0,  0x090000, 40000,   0,    30   }, //MX30LF2G28AB
	{ {0xc2, 0xda, 0x90, 0x95, 0x06, 0xff, 0xff, 0xff}, 1,     4,      64,     2048,   0x0000028c,   944,    30,     0,           0,    0,           4,        0,  0x090001, 40000,   0,    30   }, //MX30LF2G18AC
	{ {0xc2, 0xf1, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,          10,        0,  0x090002, 40000,   0,    25   }, //MX30LF1G08AA
	{ {0xc2, 0xdc, 0x90, 0x95, 0x57, 0xff, 0xff, 0xff}, 1,     4,      64,     4096,   0x0000028c,   944,    30,     2,           0,    0,           4,        0,  0x090003, 40000,   0,    30   }, //MX30LF4G28AB
	{ {0xc2, 0xf1, 0x80, 0x95, 0x02, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,          10,        0,  0x090004, 40000,   0,    30   }, //MX30LF1G18AC
	{ {0xc2, 0xdc, 0x90, 0x95, 0x56, 0xff, 0xff, 0xff}, 1,     4,      64,     4096,   0x0000008c,   944,    30,     0,           0,    0,           4,        0,  0x090005, 40000,   0,    30   }, //MX30LF4G18AC
	{ {0xc2, 0xdc, 0x90, 0xa2, 0x57, 0x03, 0xff, 0xff }, 1,     8,      64,     2048,   0x0000028c,   944,    40,     2,           0,    0,           4,        0,  0x090003, 40000,   0,    40   }, //MX30LF4G28AD

	//WINBOND NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0xef, 0xf1, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,          10,        0,  0x0a0000, 40000,   0,    25   }, //W29N01GV

	//==============================================================================
	//============================ FIDELIX NAND FLASH ==============================
	//==============================================================================
	//FIDELIX NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0xf8, 0xf1, 0x80, 0x91, 0xff, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,          10,        0,  0x0b0000, 40000,   0,    25, 0, 0, 2048 + 64}, //FMND1G08U3B
	{ {0xf8, 0xda, 0x90, 0x91, 0x46, 0xff, 0xff, 0xff}, 1,     4,      64,     2048,   0x0000008c, 944,    25,     0,           0,    0,           4,        0,  0x0b0001, 40000,   0,    25, 0, 0, 2048 + 64}, //FMND2G08U3B

	//ESMT NAND FLASH
	//                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{ {0xc8, 0xd1, 0x80, 0x95, 0x40, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,          10,        0,   0x0c0000, 60000,   0,    30, 0, 0, 2048 + 64}, //F59L1G81MA
	{ {0xc8, 0xda, 0x90, 0x95, 0x44, 0xff, 0xff, 0xff}, 1,     4,      64,     2048,   0x0000008c, 944,    30,     0,           0,    0,           2,        0,   0x0c0001, 60000,   0,    30, 0, 0, 2048 + 64}, //F59L2G81A
	{ {0xc8, 0xdc, 0x90, 0x95, 0x54, 0xff, 0xff, 0xff}, 1,     4,      64,     4096,   0x0000008c, 944,    30,     0,           0,    0,           2,        0,   0x0c0002, 60000,   0,    30, 0, 0, 2048 + 64}, //F59L4G81A
	{ {0xc8, 0xd1, 0x80, 0x95, 0x42, 0xff, 0xff, 0xff}, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,          10,        0,   0x0c0003, 60000,   0,    30  }, //PSU1GA30BT

	/* GD NAND FLASH */
	{
		/* GD9FU1G8F2A */
		.nand_id = {0xc8, 0xf1, 0x80, 0x1d, 0x42, 0xff, 0xff, 0xff },
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 1024,
		.operation_opt = 0x4000008c,
		.valid_blk_ratio = 944,
		.access_freq = 40,
		.ecc_mode = 0,
		.read_retry_type = 0,
		.ddr_type = 0,
		.option_phyisc_op_no = 10,
		.ddr_info_no = 0,
		.id_number = 0x0c0004,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 40
	},
#if 0
	{
		/* GD9FU2G8F2A */
		.nand_id = {0xc8, 0xf2, 0x90, 0x1d, 0x46, 0xff, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 2048,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 40,
		.ecc_mode = 0x3,
		.read_retry_type = 0,
		.ddr_type = 0,
		.option_phyisc_op_no = 16,
		.ddr_info_no = 0,
		.id_number = 0x0c0005,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 40
	},
#endif
	{
		/* GD9FU2G8F2A */
		.nand_id = {0xc8, 0xda, 0x90, 0x95, 0x46, 0xff, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 2048,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 40,
		.ecc_mode = BCH_32,
		.read_retry_type = 0,
		.ddr_type = SDR,
		.option_phyisc_op_no = 16,
		.ddr_info_no = 0,
		.id_number = 0x0c0005,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 40
	},
	{
		/* GD9FU4G8E2A */
		.nand_id = {0xc8, 0xdc, 0x91, 0x95, 0x46, 0xff, 0xff, 0xff},
		.die_cnt_per_chip = 1,
		.sect_cnt_per_page = 4,
		.page_cnt_per_blk = 64,
		.blk_cnt_per_die = 4096,
		.operation_opt = NAND_MULTI_PROGRAM | NAND_RANDOM,
		.valid_blk_ratio = 896,
		.access_freq = 40,
		.ecc_mode = BCH_32,
		.read_retry_type = 0,
		.ddr_type = SDR,
		.option_phyisc_op_no = 16,
		.ddr_info_no = 0,
		.id_number = 0x0c0006,
		.max_blk_erase_times = 60000,
		.driver_no = 0,
		.access_high_freq = 40
	},

	// DEFAULT NAND FLASH
	//                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar  DDRInfo
	//-----------------------------------------------------------------------------------------------------------------------
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 0,     0,       0,        0,   0x00000000,     0,     0,     0,           0,    0,         11,       0,  0xffffff, 0,   0,     0}, //defaul

};
