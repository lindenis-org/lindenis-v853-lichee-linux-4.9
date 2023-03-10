// SPDX-License-Identifier:	GPL-2.0+
/*****************************************************************************/
#define _BUILD_PHY_PARTITION_C_
/*****************************************************************************/

//#include <type.h>
//#include <nandsim.h>
#include "phy.h"
#include <linux/slab.h>
/*****************************************************************************/

extern int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum,
		unsigned int nPage, uint64 SectBitmap,
		void *pBuf, void *pSpare);
extern int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum,
		unsigned int nPage, uint64 SectBitmap,
		void *pBuf, void *pSpare);
extern int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);
extern int write_new_block_table(struct _nand_info *nand_info);
extern int nand_wait_all_rb_ready(void);

void print_phy_partition(struct _nand_phy_partition *phy_partition);
void print_all_bad_block(struct _nand_phy_partition *phy_partition);
int BlockCheck(unsigned short nDieNum, unsigned short nBlkNum);
struct _nand_phy_partition *
get_next_phy_partition(struct _nand_phy_partition *phy_partition);
struct _nand_disk *
get_disk_from_phy_partition(struct _nand_phy_partition *phy_partition);
uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
uint32 get_phy_partition_num(struct _nand_info *nand_info);
void WaitAllRb(void);
extern int write_new_block_table_v2(struct _nand_info *nand_info);

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
uint64 bitmap_change(unsigned short SectBitmap)
{
	uint64 bitmap;
	//    int i;
	//
	//    for(i=0,bitmap=0;i<SectBitmap;i++)
	//    {
	//        bitmap <<= 1;
	//        bitmap |= 0x01;
	//    }

	bitmap = SectBitmap & 0xff;
	return bitmap;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void WaitAllRb(void)
{
	NAND_PhysicLock();
	nand_wait_all_rb_ready();
	NAND_PhysicUnLock();
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int PageRead(unsigned short nDieNum, unsigned short nBlkNum,
		unsigned short nPage, unsigned short SectBitmap, void *pBuf,
		void *pSpare)
{
	uint64 bitmap;
	int ret;
	NAND_PhysicLock();
	bitmap = bitmap_change(SectBitmap);
	ret =
		PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int PageWrite(unsigned short nDieNum, unsigned short nBlkNum,
		unsigned short nPage, unsigned short SectBitmap, void *pBuf,
		void *pSpare)
{
	uint64 bitmap;
	int ret;
	NAND_PhysicLock();
	bitmap = bitmap_change(SectBitmap);
	ret =
		PHY_VirtualPageWrite(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int BlockErase(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret;
	NAND_PhysicLock();
	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int BlockCheck(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret = 0;
	/*unsigned char spare[BYTES_OF_USER_PER_PAGE];*/

	//	spare[0] = 0xff;
	//    PHY_VirtualPageRead(nDieNum,nBlkNum,0, 0,NULL,spare);
	//    if(spare[0] != 0xff)
	//    {
	//		NFTL_ERR("[NE]Nand BlockCheck fail!\n");
	//        ret = 1;
	//    }
	NAND_PhysicLock();
	ret = PHY_VirtualBadBlockCheck(nDieNum, nBlkNum);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int is_factory_bad_block(struct _nand_info *nand_info, unsigned short nDieNum,
		unsigned short nBlkNum)
{
	uint32 num, i;

	num = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->factory_bad_block[i].Chip_NO == nDieNum) &&
				(nand_info->factory_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->factory_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int is_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum,
		unsigned short nBlkNum)
{
	uint32 num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->new_bad_block[i].Chip_NO == nDieNum) &&
				(nand_info->new_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int add_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum,
		unsigned short nBlkNum)
{
	uint32 num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	if (i < num) {
		nand_info->new_bad_block[i].Chip_NO = nDieNum;
		nand_info->new_bad_block[i].Block_NO = nBlkNum;

	} else {
		PRINT("[NE]too much new bad block\n");
	}
	NAND_PhysicLock();
	write_new_block_table_v2(nand_info);
	NAND_PhysicUnLock();
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int put_factory_bad_block(struct _nand_info *nand_info,
		struct _nand_phy_partition *phy_partition)
{
	int count, i, j;

	count = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);

	for (i = 0; i < count; i++) {
		if (nand_info->factory_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}

	if (i == count) {
		PRINT("[NE]too much bad block\n");
		return NFTL_FAILURE;
	}

	for (j = 0; i < count; i++, j++) {
		if (phy_partition->factory_bad_block[j].Chip_NO != 0xffff) {
			nand_info->factory_bad_block[i].Chip_NO =
				phy_partition->factory_bad_block[j].Chip_NO;
			nand_info->factory_bad_block[i].Block_NO =
				phy_partition->factory_bad_block[j].Block_NO;
		} else {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :logic_size:sectors
 *Return       :
 *Note         :
 *****************************************************************************/
struct _nand_phy_partition *build_phy_partition(struct _nand_info *nand_info,
		uint32 logic_size, uint16 flag)
{
	struct _nand_phy_partition *phy_partition;
	struct _nand_phy_partition *p;
	uint32 total_blocks, temp1, temp2, factory_bad_count, new_bad_count;
	unsigned short nDieNum, nBlkNum, PartitionNO, last_phy_partition;

	phy_partition = MALLOC(sizeof(struct _nand_phy_partition));

	phy_partition->next_phy_partition = NULL;
	if (nand_info->phy_partition_head == NULL) {
		nand_info->phy_partition_head = phy_partition;
		phy_partition->PartitionNO = 0;
		phy_partition->StartBlock.Chip_NO =
			nand_info->no_used_block_addr.Chip_NO;
		phy_partition->StartBlock.Block_NO =
			nand_info->no_used_block_addr.Block_NO;
	} else {
		PartitionNO = 1;
		p = nand_info->phy_partition_head;
		while (p->next_phy_partition != NULL) {
			p = p->next_phy_partition;
			PartitionNO++;
		}
		p->next_phy_partition = phy_partition;
		phy_partition->PartitionNO = PartitionNO;

		phy_partition->StartBlock.Chip_NO = p->EndBlock.Chip_NO;
		phy_partition->StartBlock.Block_NO = p->EndBlock.Block_NO + 1;

		while (phy_partition->StartBlock.Block_NO >=
				nand_info->BlkPerChip) {
			phy_partition->StartBlock.Chip_NO++;
			phy_partition->StartBlock.Block_NO -=
				nand_info->BlkPerChip;
			if (phy_partition->StartBlock.Chip_NO >=
					nand_info->ChipNum) {
				PRINT("[NE]no block\n");
				return NULL;
			}
		}
	}

	phy_partition->CrossTalk = flag;
	phy_partition->page_read = PageRead;
	phy_partition->page_write = PageWrite;
	phy_partition->block_erase = BlockErase;

	phy_partition->nand_info = nand_info;
	phy_partition->SectorNumsPerPage = nand_info->SectorNumsPerPage;
	phy_partition->BytesUserData = nand_info->BytesUserData;
	phy_partition->PageNumsPerBlk = nand_info->PageNumsPerBlk;
	phy_partition->FullBitmapPerPage = phy_partition->SectorNumsPerPage;

	phy_partition->EndBlock.Chip_NO = phy_partition->StartBlock.Chip_NO;
	phy_partition->EndBlock.Block_NO = phy_partition->StartBlock.Block_NO;

	/////////////////////////////////////////////////////////////////
	if (logic_size != 0xffffffff) {
		phy_partition->TotalSectors = logic_size;
		temp1 = logic_size / phy_partition->SectorNumsPerPage;
		temp1 /= phy_partition->PageNumsPerBlk;
		total_blocks =
			temp1 +
			temp1 / phy_partition->PageNumsPerBlk; // logic block num

		temp2 = temp1 / SYS_ZONE_RESERVED_BLOCK_RATIO;
		if (temp2 < MIN_FREE_BLOCK_NUM) {
			temp2 = MIN_FREE_BLOCK_NUM;
		}

		total_blocks += temp2; // add nftl free block

		phy_partition->FreeBlock =
			total_blocks / PHY_RESERVED_BLOCK_RATIO;
		if (phy_partition->FreeBlock < MIN_PHY_RESERVED_BLOCK) {
			phy_partition->FreeBlock = MIN_PHY_RESERVED_BLOCK;
		}
		if (phy_partition->FreeBlock > MAX_PHY_RESERVED_BLOCK) {
			phy_partition->FreeBlock = MAX_PHY_RESERVED_BLOCK;
		}
		total_blocks += phy_partition->FreeBlock; // add phy free block
		last_phy_partition = 0;
	} else {
		total_blocks = 0xffffffff;
		last_phy_partition = 1;
	}

	/////////////////////////////////////////////////////////////////

	phy_partition->TotalBlkNum = 0;
	phy_partition->factory_bad_block = MALLOC(FACTORY_BAD_BLOCK_SIZE);
	MEMSET(phy_partition->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);

	phy_partition->new_bad_block = MALLOC(PHY_PARTITION_BAD_BLOCK_SIZE);
	MEMSET(phy_partition->new_bad_block, 0xff,
			PHY_PARTITION_BAD_BLOCK_SIZE);

	factory_bad_count = 0;
	new_bad_count = 0;
	nDieNum = phy_partition->StartBlock.Chip_NO;
	nBlkNum = phy_partition->StartBlock.Block_NO;
	while (total_blocks != 0) {
		if (nand_info->FirstBuild == 1) {
			if (BlockCheck(nDieNum, nBlkNum) == 0) {
				total_blocks--;
			} else {
				phy_partition
					->factory_bad_block[factory_bad_count]
					.Chip_NO = nDieNum;
				phy_partition
					->factory_bad_block[factory_bad_count]
					.Block_NO = nBlkNum;
				factory_bad_count++;
				if (factory_bad_count ==
						FACTORY_BAD_BLOCK_SIZE /
						sizeof(struct _nand_super_block)) {
					PRINT("[NE]too much bad block %d\n",
							factory_bad_count);
					kfree(phy_partition);
					return NULL;
				}
			}
		} else {
			if (is_factory_bad_block(nand_info, nDieNum, nBlkNum) ==
					1) {
				PRINT_DBG("[ND]factory bad block:%d,%d "
						"PartitionNO:%d\n",
						nDieNum, nBlkNum,
						phy_partition->PartitionNO);
				phy_partition
					->factory_bad_block[factory_bad_count]
					.Chip_NO = nDieNum;
				phy_partition
					->factory_bad_block[factory_bad_count]
					.Block_NO = nBlkNum;
				factory_bad_count++;
				if (factory_bad_count ==
						FACTORY_BAD_BLOCK_SIZE /
						sizeof(struct _nand_super_block)) {
					PRINT("[NE]too much bad block %d\n",
							factory_bad_count);
					kfree(phy_partition);
					return NULL;
				}
			} else if (is_new_bad_block(nand_info, nDieNum,
						nBlkNum) == 1) {
				PRINT_DBG(
						"[ND]new bad block:%d,%d PartitionNO:%d\n",
						nDieNum, nBlkNum,
						phy_partition->PartitionNO);
				phy_partition->new_bad_block[new_bad_count]
					.Chip_NO = nDieNum;
				phy_partition->new_bad_block[new_bad_count]
					.Block_NO = nBlkNum;
				new_bad_count++;
				if (new_bad_count ==
						PHY_PARTITION_BAD_BLOCK_SIZE /
						sizeof(struct _nand_super_block)) {
					PRINT("[NE]too much bad block %d\n",
							factory_bad_count);
					kfree(phy_partition);
					return NULL;
				}
			} else {
				total_blocks--;
			}
		}
		phy_partition->EndBlock.Chip_NO = nDieNum;
		phy_partition->EndBlock.Block_NO = nBlkNum;
		phy_partition->TotalBlkNum++;
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			nBlkNum = 0;
			nDieNum++;
			if (nDieNum == nand_info->ChipNum) {
				break;
			}
		}
	}

	if (last_phy_partition == 1) {
		total_blocks = phy_partition->TotalBlkNum - factory_bad_count;

		phy_partition->FreeBlock =
			total_blocks / PHY_RESERVED_BLOCK_RATIO;
		if (phy_partition->FreeBlock < MIN_PHY_RESERVED_BLOCK) {
			phy_partition->FreeBlock = MIN_PHY_RESERVED_BLOCK;
		}
		if (phy_partition->FreeBlock > MAX_PHY_RESERVED_BLOCK) {
			phy_partition->FreeBlock = MAX_PHY_RESERVED_BLOCK;
		}
		total_blocks -= phy_partition->FreeBlock;
		temp2 = total_blocks / ZONE_RESERVED_BLOCK_RATIO;

		if (temp2 < MIN_FREE_BLOCK_NUM) {
			temp2 = MIN_FREE_BLOCK_NUM;
		}
		total_blocks -= temp2;

		temp1 = total_blocks * phy_partition->PageNumsPerBlk;
		temp1 *= phy_partition->SectorNumsPerPage;
		phy_partition->TotalSectors = temp1;
	}

	if (nand_info->FirstBuild == 1) {
		put_factory_bad_block(nand_info, phy_partition);
	}

	phy_partition->disk =
		nand_info->partition[phy_partition->PartitionNO].nand_disk;

	print_phy_partition(phy_partition);
	print_all_bad_block(phy_partition);

	return phy_partition;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int free_phy_partition(struct _nand_phy_partition *phy_partition)
{
	kfree(phy_partition->factory_bad_block);
	kfree(phy_partition->new_bad_block);
	kfree(phy_partition);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void print_phy_partition(struct _nand_phy_partition *phy_partition)
{
	PRINT_DBG("[ND]phy_partition->PartitionNO :%d\n",
			phy_partition->PartitionNO);
	PRINT_DBG("[ND]phy_partition->SectorNumsPerPage :%d\n",
			phy_partition->SectorNumsPerPage);
	PRINT_DBG("[ND]phy_partition->PageNumsPerBlk :%d\n",
			phy_partition->PageNumsPerBlk);
	PRINT_DBG("[ND]phy_partition->TotalBlkNum :%d\n",
			phy_partition->TotalBlkNum);

	PRINT_DBG("[ND]phy_partition->FullBitmapPerPage :%d\n",
			phy_partition->FullBitmapPerPage);
	PRINT_DBG("[ND]phy_partition->FreeBlock :%d\n",
			phy_partition->FreeBlock);
	PRINT_DBG("[ND]phy_partition->TotalSectors :%d\n",
			phy_partition->TotalSectors);
	PRINT_DBG("[ND]phy_partition->StartBlock.Chip_NO :%d\n",
			phy_partition->StartBlock.Chip_NO);
	PRINT_DBG("[ND]phy_partition->StartBlock.Block_NO :%d\n",
			phy_partition->StartBlock.Block_NO);
	PRINT_DBG("[ND]phy_partition->EndBlock.Chip_NO :%d\n",
			phy_partition->EndBlock.Chip_NO);
	PRINT_DBG("[ND]phy_partition->EndBlock.Block_NO :%d\n",
			phy_partition->EndBlock.Block_NO);
	PRINT_DBG("[ND]phy_partition->next_phy_partition :%d\n",
			phy_partition->next_phy_partition);
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void print_all_bad_block(struct _nand_phy_partition *phy_partition)
{
	int i, num;
	num = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	PRINT_DBG("[ND]phy_partition->PartitionNO %d  FACTORY BAD BLOCK:\n",
			phy_partition->PartitionNO);
	for (i = 0; i < num; i++) {
		if (phy_partition->factory_bad_block[i].Chip_NO != 0xffff) {
			PRINT_DBG("[ND]BAD Chip:%d;Block:%d.\n",
					phy_partition->factory_bad_block[i].Chip_NO,
					phy_partition->factory_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	PRINT_DBG("[ND]phy_partition->PartitionNO %d  NEW BAD BLOCK:\n",
			phy_partition->PartitionNO);
	for (i = 0; i < num; i++) {
		if (phy_partition->new_bad_block[i].Chip_NO != 0xffff) {
			PRINT_DBG("[ND]BAD Chip:%d;Block:%d.\n",
					phy_partition->new_bad_block[i].Chip_NO,
					phy_partition->new_bad_block[i].Block_NO);
		} else {
			break;
		}
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
	struct _nand_phy_partition *
get_next_phy_partition(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->next_phy_partition;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
	struct _nand_disk *
get_disk_from_phy_partition(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->disk;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
uint16 get_partitionNO(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->PartitionNO;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
uint32 get_phy_partition_num(struct _nand_info *nand_info)
{
	uint32 num = 0;

	struct _nand_phy_partition *p;

	p = nand_info->phy_partition_head;
	while (p != NULL) {
		num++;
		p = p->next_phy_partition;
	}
	return num;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
	struct _nand_phy_partition *
get_phy_partition_from_num(struct _nand_info *nand_info, uint32 num)
{
	uint32 i = 0;
	struct _nand_phy_partition *p;

	p = nand_info->phy_partition_head;
	for (i = 0; i < num; i++) {
		p = p->next_phy_partition;
	}
	return p;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int erase_phy_partition(struct _nand_info *nand_info,
		struct _nand_phy_partition *phy_partition)
{
	unsigned short nDieNum, nBlkNum;
	int i, ret;
	uint32 total_blocks = phy_partition->TotalBlkNum;

	nDieNum = phy_partition->StartBlock.Chip_NO;
	nBlkNum = phy_partition->StartBlock.Block_NO;

	PRINT("[NE]erase_whole_phy_partition start! %d %d %d\n", total_blocks,
			nDieNum, nBlkNum);

	for (i = 0; i < total_blocks; i++) {
		if (BlockCheck(nDieNum, nBlkNum) == 0) {
			ret = phy_partition->block_erase(nDieNum, nBlkNum);
			if (ret != 0) {
				PRINT("[NE]erase_whole_phy_partition error %d "
						"%d\n",
						nDieNum, nBlkNum);
			}
		}

		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			nBlkNum = 0;
			nDieNum++;
			if (nDieNum == nand_info->ChipNum) {
				break;
			}
		}
	}
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int erase_last_phy_partition(struct _nand_info *nand_info)
{
	int partition_num, ret;
	struct _nand_phy_partition *phy_partition;

	partition_num = get_phy_partition_num(nand_info);
	if ((partition_num < 1) || (partition_num > 5)) {
		PRINT("[NE]erase_last_phy_partition error 1\n");
		return 1;
	}

	phy_partition =
		get_phy_partition_from_num(nand_info, partition_num - 1);
	if (phy_partition == NULL) {
		PRINT("[NE]erase_last_phy_partition error 2\n");
		return 1;
	}

	ret = erase_phy_partition(nand_info, phy_partition);
	return ret;
}
