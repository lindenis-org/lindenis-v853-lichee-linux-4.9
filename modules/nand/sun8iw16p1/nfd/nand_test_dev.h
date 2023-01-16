/*
 * nand_blk.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __BSP_NAND_TEST_H__
#define __BSP_NAND_TEST_H__

extern int nand_driver_test_init(void);
extern int nand_driver_test_exit(void);
extern unsigned int get_nftl_num(void);
extern unsigned int get_nftl_cap(void);
extern unsigned int get_first_nftl_cap(void);
extern unsigned int nftl_test_read(unsigned int start_sector, unsigned int len,
				   unsigned char *buf);
extern unsigned int nftl_test_write(unsigned int start_sector, unsigned int len,
				    unsigned char *buf);
extern unsigned int nftl_test_flush_write_cache(void);

extern struct _nand_info *p_nand_info;
extern struct kobj_type ktype;
extern struct _nftl_blk nftl_blk_head;

extern struct _nand_partition *
build_nand_partition(struct _nand_phy_partition *phy_partition);
extern void add_nftl_blk_list(struct _nftl_blk *head,
struct  _nftl_blk *nftl_blk);
extern uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
extern struct _nand_phy_partition *
get_head_phy_partition_from_nand_info(struct _nand_info *nand_info);
extern struct _nand_phy_partition *
get_next_phy_partition(struct _nand_phy_partition *phy_partition);

extern uint32 nand_wait_rb_mode(void);
extern uint32 nand_wait_dma_mode(void);
extern void do_nand_interrupt(unsigned int no);

extern int nand_ftl_exit(void);
extern unsigned int nftl_read(unsigned int start_sector, unsigned int len,
							unsigned char *buf);
extern unsigned int nftl_write(unsigned int start_sector, unsigned int len,
							 unsigned char *buf);
extern unsigned int nftl_flush_write_cache(void);
extern int nand_ftl_exit(void);

extern uint32 gc_all(struct _nftl_zone *zone);
extern uint32 gc_one(struct _nftl_zone *zone);
extern void print_nftl_zone(struct _nftl_zone *zone);
extern void print_free_list(struct _nftl_zone *zone);
extern void print_block_invalid_list(struct _nftl_zone *zone);
extern uint32 nftl_set_zone_test(void *_zone, uint32 num);


static ssize_t nand_test_store(struct kobject *kobject,
		struct attribute *attr, const char *buf, size_t count);
static ssize_t nand_test_show(struct kobject *kobject,
		struct attribute *attr, char *buf);
void obj_test_release(struct kobject *kobject);

int add_nand_for_test(struct _nand_phy_partition *phy_partition);
int build_all_nftl(struct _nand_info *nand_info);


#endif
