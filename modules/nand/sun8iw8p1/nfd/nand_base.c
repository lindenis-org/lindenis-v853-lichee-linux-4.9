/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
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
#include "nand_base.h"

struct device *ndfc_dev;
struct completion spinand_dma_done;
void *SPIC0_IO_BASE;

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_suspend(struct platform_device *plat_dev, pm_message_t state)
{
#if 0
	if (NORMAL_STANDBY == standby_type) {
		nand_dbg_err("[NAND] nand_suspend normal\n");

		NandHwNormalStandby();
	} else if (SUPER_STANDBY == standby_type) {
		nand_dbg_err("[NAND] nand_suspend super\n");
		NandHwSuperStandby();
	}

	nand_dbg_err("[NAND] nand_suspend ok \n");
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_resume(struct platform_device *plat_dev)
{
#if 0
	if (NORMAL_STANDBY == standby_type) {
		nand_dbg_err("[NAND] nand_resume normal\n");
		NandHwNormalResume();
	} else if (SUPER_STANDBY == standby_type) {
		nand_dbg_err("[NAND] nand_resume super\n");
		NandHwSuperResume();
	}

	nand_dbg_err("[NAND] nand_resume ok \n");
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_probe(struct platform_device *plat_dev)
{
	int ret = 0;
	int nand_cache_level = 0, nand_capacity_level = 0;
	nand_dbg_inf("nand probe\n");
	ndfc_dev = &plat_dev->dev;

	SPIC0_IO_BASE = (void *)of_iomap(ndfc_dev->of_node, 0);
	if (!SPIC0_IO_BASE) {
		nand_dbg_err("Failed to map SPIC0 IO space1\n");
		return -EAGAIN;
	}

	init_completion(&spinand_dma_done);
	p_nand_info = NandHwInit();
	if (p_nand_info == NULL) {
		return -EAGAIN;
	}

	/* nand0_cache_level */
	ret = of_property_read_u32(ndfc_dev->of_node, "nand0_cache_level",
				   &nand_cache_level);
	if (ret) {
		nand_dbg_err("Failed to get nand0_cache_level in dts\n");
		nand_cache_level = 0;
	} else if (nand_cache_level == 0x55aaaa55) {
		nand_dbg_inf("nand0_cache_level is no used\n");
		nand_cache_level = 0;
	}
	set_cache_level(p_nand_info, nand_cache_level);

	/* nand0_capacity_level */
	ret = of_property_read_u32(ndfc_dev->of_node, "nand0_capacity_level",
				   &nand_capacity_level);
	if (ret) {
		nand_dbg_err("Failed to get nand_capacity_level in dts\n");
		nand_capacity_level = 0;
	} else if (nand_capacity_level == 0x55aaaa55) {
		nand_dbg_inf("nand_capacity_level is no used\n");
		nand_capacity_level = 0;
	}
	set_capacity_level(p_nand_info, nand_capacity_level);

	ret = nand_info_init(p_nand_info, 0, 20, NULL);
	if (ret != 0) {
		nand_dbg_err("nand_info_init failed\n");
		return -EINVAL;
	}

	init_blklayer();

	kthread_run(nand_thread, &mytr, "%sd", "nand_rc");
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_remove(struct platform_device *plat_dev)
{
	nand_dbg_inf("nand_remove\n");
	return 0;
}

static void nand_release_dev(struct device *dev)
{
	nand_dbg_inf("nand_release dev\n");
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 shutdown_flush_write_cache(void)
{
	struct _nftl_blk *nftl_blk;
	struct nand_blk_ops *tr = &mytr;

	nftl_blk = tr->nftl_blk_head.nftl_blk_next;

	while (nftl_blk != NULL) {
		nand_dbg_err("shutdown_flush_write_cache\n");
		mutex_lock(nftl_blk->blk_lock);
		nftl_blk->flush_write_cache(nftl_blk, 0xffff);
		nftl_blk = nftl_blk->nftl_blk_next;
		/*mutex_unlock(nftl_blk->blk_lock);*/
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
void nand_shutdown(struct platform_device *plat_dev)
{
	/*    struct nand_blk_dev *dev;*/
	/*   struct nand_blk_ops *tr = &mytr;*/

	/*    nand_dbg_err("[NAND]shutdown first\n");*/
	/*    list_for_each_entry(dev, &tr->devs, list){*/
	/*        while(blk_fetch_request(dev->rq) != NULL){*/
	/*            nand_dbg_err("nand_shutdown wait dev %d\n",dev->devnum);*/
	/*            set_current_state(TASK_INTERRUPTIBLE);*/
	/*            schedule_timeout(HZ>>3);*/
	/*        }*/
	/*    }*/
	/**/
	/*    nand_dbg_err("[NAND]shutdown second\n");*/
	/*    list_for_each_entry(dev, &tr->devs, list){*/
	/*        while(blk_fetch_request(dev->rq) != NULL){*/
	/*            nand_dbg_err("nand_shutdown wait dev %d\n",dev->devnum);*/
	/*            set_current_state(TASK_INTERRUPTIBLE);*/
	/*            schedule_timeout(HZ>>3);*/
	/*        }*/
	/*    }*/

	shutdown_flush_write_cache();
	NandHwShutDown();
	nand_dbg_err("[NAND]shutdown end\n");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static const struct of_device_id of_nand_id_table[] = {
    {.compatible = "allwinner,sunxi-spinand"},
    {},
};

static struct platform_driver spinand_driver = {
    .probe = nand_probe,
    .remove = nand_remove,
    .shutdown = nand_shutdown,
    .suspend = nand_suspend,
    .resume = nand_resume,
    .driver = {
	.name = "sw_nand",
	.owner = THIS_MODULE,
	.of_match_table = of_nand_id_table,
    } };

static struct resource flash_resource = {
    .start = 0,
    .end = 1,
    .flags = 0x1,
};

struct platform_device nand_device = {
    .name = "sw_nand",
    .id = 33,
    .resource = &flash_resource,
    .num_resources = 1,
    .dev = {
	.release = nand_release_dev,
    } };

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int __init nand_init(void)
{
	return platform_driver_register(&spinand_driver);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void __exit nand_exit(void)
{
	platform_driver_unregister(&spinand_driver);
	exit_blklayer();
}

/*module_init(nand_init);*/
/*module_exit(nand_exit);*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("nand flash groups");
MODULE_DESCRIPTION("Generic NAND flash driver code");
