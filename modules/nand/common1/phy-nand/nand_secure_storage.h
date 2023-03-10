/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NAND_SECURE_STORAGE_H__
#define __NAND_SECURE_STORAGE_H__

/*****************************************************************************/

#define MAX_SECURE_STORAGE_ITEM 32

#define MIN_SECURE_STORAGE_BLOCK_NUM 8
#define MAX_SECURE_STORAGE_BLOCK_NUM 50

extern int nand_secure_storage_block;
extern int nand_secure_storage_block_bak;

unsigned int NAND_Get_SecureBlock_Start(void);
int nand_is_support_secure_storage(void);
int nand_secure_storage_write_init(unsigned int block);
int nand_secure_storage_clean_data(unsigned int num);
int nand_secure_storage_clean(void);
int is_nand_secure_storage_build(void);
int is_nand_secure_storage_block(unsigned int block);
int nand_secure_storage_init(int flag);
int nand_secure_storage_first_build(unsigned int start_block);
int get_nand_secure_storage_max_item(void);
unsigned int nand_secure_check_sum(unsigned char *mbuf, unsigned int len);
int nand_secure_storage_read_one(unsigned int block, int item,
				 unsigned char *mbuf, unsigned int len);
int nand_secure_storage_read(int item, unsigned char *buf, unsigned int len);
int nand_secure_storage_write(int item, unsigned char *buf, unsigned int len);
int nand_secure_storage_check(void);
int nand_secure_storage_repair(int flag);
int nand_secure_storage_update(void);
int nand_secure_storage_test(unsigned int item);
int nand_secure_storage_test_read(unsigned int item, unsigned int block);
extern int nand_physic_lock(void);
extern int nand_physic_unlock(void);

int sunxi_secure_object_read(const char *item_name, char *buffer,
			     int buffer_len, int *data_len);

int sunxi_secure_object_write(const char *item_name, char *buffer,
		int length, int write_protect);
int sunxi_secure_storage_init(void);
int sunxi_secure_storage_exit(int mode);

#endif /*NAND_SECURE_STORAGE_H*/
