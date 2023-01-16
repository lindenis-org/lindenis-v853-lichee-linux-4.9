/*
 * Based on m25p80.c, by Mike Lavender (mike@steroidmicros.com), with
 * influence from lart.c (Abraham Van Der Merwe) and mtd_dataflash.c
 *
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/sizes.h>

#include <linux/mtd/mtd.h>
#include <linux/of_platform.h>
#include <linux/spi/flash.h>
#include <linux/mtd/spi-nor.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

/* Define max times to check status register before we give up. */

/*
 * For everything but full-chip erase; probably could be much smaller, but kept
 * around for safety for now
 */
#define DEFAULT_READY_WAIT_JIFFIES		(40UL * HZ)

/*
 * For full-chip erase, calibrated to a 2MB flash (M25P16); should be scaled up
 * for larger flash
 */
#define CHIP_ERASE_2MB_READY_WAIT_JIFFIES	(40UL * HZ)

#define SPI_NOR_MAX_ID_LEN	6
#define SPI_NOR_MAX_ADDR_WIDTH	4

static struct spinordbg_data spinordbg_priv;
static struct spi_nor *spinordbg;

struct flash_info {
	char		*name;

	/*
	 * This array stores the ID bytes.
	 * The first three bytes are the JEDIC ID.
	 * JEDEC ID zero means "no ID" (mostly older chips).
	 */
	u8		id[SPI_NOR_MAX_ID_LEN];
	u8		id_len;

	/* The size listed here is what works with SPINOR_OP_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned	sector_size;
	u16		n_sectors;
	u16		n_banks;

	u16		page_size;
	u16		addr_width;

	u32		flags;
#define SECT_4K			BIT(0)	/* SPINOR_OP_BE_4K works uniformly */
#define SPI_NOR_NO_ERASE	BIT(1)	/* No erase command needed */
#define SST_WRITE		BIT(2)	/* use SST byte programming */
#define SPI_NOR_NO_FR		BIT(3)	/* Can't do fastread */
#define SECT_4K_PMC		BIT(4)	/* SPINOR_OP_BE_4K_PMC works uniformly */
#define SPI_NOR_DUAL_READ	BIT(5)	/* Flash supports Dual Read */
#define SPI_NOR_QUAD_READ	BIT(6)	/* Flash supports Quad Read */
#define USE_FSR			BIT(7)	/* use flag status register */
#define SPI_NOR_HAS_LOCK	BIT(8)	/* Flash supports lock/unlock via SR */
#define SPI_NOR_HAS_TB		BIT(9)	/*
					 * Flash SR has Top/Bottom (TB) protect
					 * bit. Must be used with
					 * SPI_NOR_HAS_LOCK.
					 */
#define SPI_NOR_INDIVIDUAL_LOCK BIT(16) /* individual block/sector lock mode */
#define SPI_NOR_HAS_LOCK_HANDLE BIT(17) /* OP/ERASE for lock operation */
};

#define JEDEC_MFR(info)	((info)->id[0])
#define JEDEC_ID(info)		(((info)->id[1]) << 8 | ((info)->id[2]))

#define BP_SZ_4K       (4 * 1024)
#define BP_SZ_32K      (32 * 1024)
#define BP_SZ_64K      (64 * 1024)
#define BP_SZ_128K     (128 * 1024)
#define BP_SZ_256K     (256 * 1024)
#define BP_SZ_512K     (512 * 1024)
#define BP_SZ_1M       (1 * 1024 * 1024)
#define BP_SZ_2M       (2 * 1024 * 1024)
#define BP_SZ_4M       (4 * 1024 * 1024)
#define BP_SZ_6M       (6 * 1024 * 1024)
#define BP_SZ_7M       (7 * 1024 * 1024)
#define BP_SZ_7680K	(7680 * 1024)
#define BP_SZ_7936K	(7936 * 1024)
#define BP_SZ_8064K	(8064 * 1024)
#define BP_SZ_8128K	(8128 * 1024)
#define BP_SZ_8M       (8 * 1024 * 1024)
#define BP_SZ_12M      (12 * 1024 * 1024)
#define BP_SZ_14M      (14 * 1024 * 1024)
#define BP_SZ_15M      (15 * 1024 * 1024)
#define BP_SZ_15872K   (15872 * 1024)
#define BP_SZ_16128K   (16128 * 1024)
#define BP_SZ_16M      (16 * 1024 * 1024)
#define BP_SZ_32M      (32 * 1024 * 1024)
#define BP_SZ_64M      (64 * 1024 * 1024)

#define NOR_LOCK_BLOCK_SIZE  (64 * 1024)
#define NOR_LOCK_SECTOR_SIZE (4 * 1024)

/* TB bit is OTP (one times program) */
struct nor_protection mxic_protection[] = {
    { .boundary = 0, .bp = 0, .flag = 0 },
    { .boundary = BP_SZ_64K, .bp = BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_128K, .bp = BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_256K, .bp = BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_512K, .bp = BIT(2), .flag = SET_TB },
    { .boundary = BP_SZ_1M, .bp = BIT(2) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_2M, .bp = BIT(2) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_4M, .bp = BIT(2) | BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_8M, .bp = BIT(3), .flag = SET_TB },
    { .boundary = BP_SZ_16M, .bp = BIT(3) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_32M, .bp = BIT(3) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_64M, .bp = BIT(3) | BIT(1) | BIT(0), .flag = SET_TB },
};

struct nor_protection gd_protection[] = {
    { .boundary = 0, .bp = 0, .flag = 0 },
    { .boundary = BP_SZ_256K, .bp = BIT(3) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_512K, .bp = BIT(3) | BIT(1), .flag = 0 },
    { .boundary = BP_SZ_1M, .bp = BIT(3) | BIT(1) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_2M, .bp = BIT(3) | BIT(2), .flag = 0 },
    { .boundary = BP_SZ_4M, .bp = BIT(3) | BIT(2) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_8M, .bp = BIT(3) | BIT(2) | BIT(1), .flag = 0 },
    { .boundary = BP_SZ_12M, .bp = BIT(2) | BIT(0), .flag = SET_CMP },
    { .boundary = BP_SZ_14M, .bp = BIT(2), .flag = SET_CMP },
    { .boundary = BP_SZ_15M, .bp = BIT(1) | BIT(0), .flag = SET_CMP },
    { .boundary = BP_SZ_15872K, .bp = BIT(1), .flag = SET_CMP },
    { .boundary = BP_SZ_16128K, .bp = BIT(0), .flag = SET_CMP },
    { .boundary = BP_SZ_16M, .bp = BIT(2) | BIT(1) | BIT(0), .flag = 0},
};

struct nor_protection esmt_protection[] = {
    { .boundary = 0, .bp = 0, .flag = 0 },
    { .boundary = BP_SZ_256K, .bp = BIT(3) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_512K, .bp = BIT(3) | BIT(1), .flag = 0 },
    { .boundary = BP_SZ_1M, .bp = BIT(3) | BIT(1) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_2M, .bp = BIT(3) | BIT(2), .flag = 0 },
    { .boundary = BP_SZ_4M, .bp = BIT(3) | BIT(2) | BIT(0), .flag = 0 },
    { .boundary = BP_SZ_8M, .bp = BIT(3) | BIT(2) | BIT(1), .flag = 0 },
#if 0 /* TB in otp zone */
    { .boundary = BP_SZ_12M, .bp = BIT(2) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_14M, .bp = BIT(2), .flag = SET_TB },
    { .boundary = BP_SZ_15M, .bp = BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_15872K, .bp = BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_16128K, .bp = BIT(0), .flag = SET_TB },
#endif
    { .boundary = BP_SZ_16M, .bp = BIT(3) | BIT(2) | BIT(1) | BIT(0), .flag = 0 },
};

struct nor_protection esmt_protection_8M[] = {
    { .boundary = 0, .bp = 0, .flag = 0 },
    { .boundary = BP_SZ_64K, .bp = BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_128K, .bp = BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_256K, .bp = BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_512K, .bp = BIT(2), .flag = SET_TB },
    { .boundary = BP_SZ_1M, .bp = BIT(2) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_2M, .bp = BIT(2) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_4M, .bp = BIT(2) | BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_6M, .bp = BIT(3) | BIT(2) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_7M, .bp = BIT(3) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_7680K, .bp = BIT(3) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_7936K, .bp = BIT(3) | BIT(1) | BIT(0), .flag = SET_TB },
    { .boundary = BP_SZ_8064K, .bp = BIT(3) | BIT(2), .flag = SET_TB },
    { .boundary = BP_SZ_8128K, .bp = BIT(3) | BIT(2) | BIT(1), .flag = SET_TB },
    { .boundary = BP_SZ_8M, .bp = BIT(3) | BIT(2) | BIT(1) | BIT(0), .flag = SET_TB },
};

static const struct flash_info *spi_nor_match_id(const char *name);

struct spi_nor *get_spinor(void)
{
	return spinordbg;
}

/*
 * Read the status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
static int read_sr(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDSR, &val, 1);
	if (ret < 0) {
		pr_err("error %d reading SR\n", (int) ret);
		return ret;
	}

	return val;
}

/*
 * Read the status register2, returning its value in the
 * location. Return the configuration register value.
 * Returns negative if error occurred.
 */
static int read_sr2(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDSR2, &val, 1);
	if (ret < 0) {
		dev_err(nor->dev, "error %d reading CR\n", ret);
		return ret;
	}

	return val;
}

/*
 * Read the status register3, returning its value in the
 * location. Return the configuration register value.
 * Returns negative if error occurred.
 */
static int read_sr3(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDSR3, &val, 1);
	if (ret < 0) {
		dev_err(nor->dev, "error %d reading CR\n", ret);
		return ret;
	}

	return val;
}

/*
 * Read the flag status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
static int read_fsr(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDFSR, &val, 1);
	if (ret < 0) {
		pr_err("error %d reading FSR\n", ret);
		return ret;
	}

	return val;
}

/*
 * Read configuration register, returning its value in the
 * location. Return the configuration register value.
 * Returns negative if error occured.
 */
static int read_cr(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDCR, &val, 1);
	if (ret < 0) {
		dev_err(nor->dev, "error %d reading CR\n", ret);
		return ret;
	}

	return val;
}

/*
 * Read MXIC security register, returning its value in
 * the location. Return the configuration register value.
 * Returns negative if error occurred.
 */
static inline int read_mxic_sr(struct spi_nor *nor)
{
	int ret = 0;
	u8 val = 0;

	ret = nor->read_reg(nor, SPINOR_OP_RDSCUR, &val, 1);
	if (ret < 0) {
		pr_debug("error %d reading MXIC scur\n", (int)ret);
		return ret;
	}

	return val;
}

/*
 * Dummy Cycle calculation for different type of read.
 * It can be used to support more commands with
 * different dummy cycle requirements.
 */
static inline int spi_nor_read_dummy_cycles(struct spi_nor *nor)
{
	switch (nor->flash_read) {
	case SPI_NOR_FAST:
	case SPI_NOR_DUAL:
	case SPI_NOR_QUAD:
		return 8;
	case SPI_NOR_NORMAL:
		return 0;
	}
	return 0;
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct spi_nor *nor)
{
	return nor->write_reg(nor, SPINOR_OP_WREN, NULL, 0);
}

/*
 * Send write disble instruction to the chip.
 */
static inline int write_disable(struct spi_nor *nor)
{
	return nor->write_reg(nor, SPINOR_OP_WRDI, NULL, 0);
}

static inline struct spi_nor *mtd_to_spi_nor(struct mtd_info *mtd)
{
	return mtd->priv;
}

/* Enable/disable 4-byte addressing mode. */
static inline int set_4byte(struct spi_nor *nor, const struct flash_info *info,
			    int enable)
{
	int status;
	bool need_wren = false;
	int ids = 0;
	u8 cmd;

	switch (JEDEC_MFR(info)) {
	case SNOR_MFR_MICRON:
		/* Some Micron need WREN command; all will accept it */
		need_wren = true;
	case SNOR_MFR_MACRONIX:
	case SNOR_MFR_WINBOND:
		ids = nor->n_banks;
		do {
			if (nor->flash_select_bank)
				nor->flash_select_bank(nor, 0, ids);

			if (need_wren)
				write_enable(nor);

			cmd = enable ? SPINOR_OP_EN4B : SPINOR_OP_EX4B;
			status = nor->write_reg(nor, cmd, NULL, 0);
			if (need_wren)
				write_disable(nor);
		} while (ids--);

		return status;
	default:
		/* Spansion style */
		nor->cmd_buf[0] = enable << 7;
		return nor->write_reg(nor, SPINOR_OP_BRWR, nor->cmd_buf, 1);
	}
}
static inline int spi_nor_sr_ready(struct spi_nor *nor)
{
	int sr = read_sr(nor);
	if (sr < 0)
		return sr;
	else
		return !(sr & SR_WIP);
}

static inline int spi_nor_fsr_ready(struct spi_nor *nor)
{
	int fsr = read_fsr(nor);
	if (fsr < 0)
		return fsr;
	else
		return fsr & FSR_READY;
}

static int spi_nor_ready(struct spi_nor *nor)
{
	int sr, fsr;
	sr = spi_nor_sr_ready(nor);
	if (sr < 0)
		return sr;
	fsr = nor->flags & SNOR_F_USE_FSR ? spi_nor_fsr_ready(nor) : 1;
	if (fsr < 0)
		return fsr;
	return sr && fsr;
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int spi_nor_wait_till_ready_with_timeout(struct spi_nor *nor,
						unsigned long timeout_jiffies)
{
	unsigned long deadline;
	int timeout = 0, ret;

	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		ret = spi_nor_ready(nor);
		if (ret < 0)
			return ret;
		if (ret)
			return 0;

		cond_resched();
	}

	dev_err(nor->dev, "flash operation timed out\n");

	return -ETIMEDOUT;
}

static int spi_nor_wait_till_ready(struct spi_nor *nor)
{
	return spi_nor_wait_till_ready_with_timeout(nor,
						    DEFAULT_READY_WAIT_JIFFIES);
}

/*
 * Write status register 1 byte
 * Returns negative if error occurred.
 */
static inline int write_sr(struct spi_nor *nor, u8 val)
{
	nor->cmd_buf[0] = val;
	return nor->write_reg(nor, SPINOR_OP_WRSR, nor->cmd_buf, 1);
}

/*
 * Write status register2 1 byte
 * Returns negative if error occurred.
 */
static int write_sr2(struct spi_nor *nor, u8 val)
{
	int ret;

	nor->cmd_buf[0] = val;
	write_enable(nor);
	ret = nor->write_reg(nor, SPINOR_OP_WRSR2, nor->cmd_buf, 1);
	if (ret < 0) {
		dev_dbg(nor->dev,
			"error while writing configuration register\n");
		return -EINVAL;
	}

	ret = spi_nor_wait_till_ready(nor);
	if (ret) {
		dev_dbg(nor->dev,
			"timeout while writing configuration register\n");
		return ret;
	}

	return 0;
}

/*
 * Write status register3 1 byte
 * Returns negative if error occurred.
 */
static int write_sr3(struct spi_nor *nor, u8 val)
{
	int ret;

	nor->cmd_buf[0] = val;
	write_enable(nor);
	ret = nor->write_reg(nor, SPINOR_OP_WRSR3, nor->cmd_buf, 1);
	if (ret < 0) {
		dev_dbg(nor->dev,
			"error while writing configuration register\n");
		return -EINVAL;
	}

	ret = spi_nor_wait_till_ready(nor);
	if (ret) {
		dev_dbg(nor->dev,
			"timeout while writing configuration register\n");
		return ret;
	}

	return 0;
}

/*
 * Write control register 1 byte
 * Returns negative if error occurred.
 */
static int write_cr(struct spi_nor *nor, u8 val)
{
	nor->cmd_buf[0] = val;
	return nor->write_reg(nor, SPINOR_OP_WRSR2, nor->cmd_buf, 1);
}

/*
 * Write status Register and configuration register with 2 bytes
 * The first byte will be written to the status register, while the
 * second byte will be written to the configuration register.
 * Return negative if error occured.
 */
static int write_sr_cr(struct spi_nor *nor, u8 *sr_cr)
{
	int ret;

	write_enable(nor);

	ret = nor->write_reg(nor, SPINOR_OP_WRSR, sr_cr, 2);
	if (ret < 0) {
		dev_dbg(nor->dev,
			"error while writing configuration register\n");
		return -EINVAL;
	}

	ret = spi_nor_wait_till_ready(nor);
	if (ret) {
		dev_dbg(nor->dev,
			"timeout while writing configuration register\n");
		return ret;
	}

	return 0;
}

static int enter_otp(struct spi_nor *nor)
{
	return nor->write_reg(nor, SPINOR_OP_ENTER_OTP, NULL, 0);
}

static int exit_otp(struct spi_nor *nor)
{
	return nor->write_reg(nor, SPINOR_OP_EXIT_OTP, NULL, 0);
}

static int write_vsr_enable(struct spi_nor *nor)
{
	return nor->write_reg(nor, SPINOR_OP_WREN_VSR, NULL, 0);
}

static int write_otp_sr(struct spi_nor *nor, u8 val)
{
	int ret;
	ret = enter_otp(nor);
	if (ret)
		return ret;

	ret = write_vsr_enable(nor);
	if (ret)
		goto exit_otp;

	ret = write_sr(nor, val);
	if (ret)
		goto exit_otp;

	ret = spi_nor_wait_till_ready(nor);
	if (ret)
		goto exit_otp;
exit_otp:
	exit_otp(nor);
	return ret;
}

static int read_otp_sr(struct spi_nor *nor)
{
	int ret;
	int val;

	ret = enter_otp(nor);
	if (ret)
		return ret;

	ret = write_vsr_enable(nor);
	if (ret)
		return ret;

	val = read_sr(nor);
	if (val < 0) {
		exit_otp(nor);
		return val;
	}

	ret = exit_otp(nor);
	if (ret)
		return ret;

	return val | 0xff;
}

static int gigadevice_config_cmp(struct spi_nor *nor, int cmp)
{
	int ret = 0, sr2_old, sr2_new;
	sr2_old = read_sr2(nor);
	if (sr2_old < 0)
		return sr2_old;

	if (cmp)
		sr2_new = sr2_old | SR2_CMP_GD;
	else
		sr2_new = sr2_old & ~SR2_CMP_GD;

	if (sr2_old != sr2_new) {
		write_enable(nor);
		ret = write_sr2(nor, sr2_new & 0xff);
		spi_nor_wait_till_ready(nor);
	}
	dev_dbg(nor->dev, "sr2:0x%x --> 0x%x\n", sr2_old, sr2_new);

	return ret;
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct spi_nor *nor)
{
	dev_dbg(nor->dev, " %lldKiB\n", (long long)(nor->mtd.size >> 10));

	return nor->write_reg(nor, SPINOR_OP_CHIP_ERASE, NULL, 0);
}

static int spi_nor_lock_and_prep(struct spi_nor *nor, enum spi_nor_ops ops)
{
	int ret = 0;

	mutex_lock(&nor->lock);

	if (nor->prepare) {
		ret = nor->prepare(nor, ops);
		if (ret) {
			dev_err(nor->dev, "failed in the preparation.\n");
			mutex_unlock(&nor->lock);
			return ret;
		}
	}
	return ret;
}

static void spi_nor_unlock_and_unprep(struct spi_nor *nor, enum spi_nor_ops ops)
{
	if (nor->unprepare)
		nor->unprepare(nor, ops);
	mutex_unlock(&nor->lock);
}

/*
 * Initiate the erasure of a single sector
 */
static int spi_nor_erase_sector(struct spi_nor *nor, u32 addr)
{
	u8 buf[SPI_NOR_MAX_ADDR_WIDTH];
	int i;

	if (nor->erase)
		return nor->erase(nor, addr);

	/*
	 * Default implementation, if driver doesn't have a specialized HW
	 * control
	 */
	for (i = nor->addr_width - 1; i >= 0; i--) {
		buf[i] = addr & 0xff;
		addr >>= 8;
	}

	return nor->write_reg(nor, nor->erase_opcode, buf, nor->addr_width);
}

/*
 * Erase an address range on the nor chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int spi_nor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	u32 addr, len;
	uint32_t rem;
	int ids = 0;
	int ret;

	dev_dbg(nor->dev, "at 0x%llx, len %lld\n", (long long)instr->addr,
			(long long)instr->len);

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	addr = instr->addr;
	len = instr->len;

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_ERASE);
	if (ret)
		return ret;

	/* whole-chip erase? */
	if (len == mtd->size) {
		unsigned long timeout;
		if (nor->n_banks && nor->flash_select_bank)
			ids = nor->n_banks;
		else
			ids = 0;

		do {
			if (nor->flash_select_bank)
				nor->flash_select_bank(nor, 0, ids);

			write_enable(nor);

			if (erase_chip(nor)) {
				ret = -EIO;
				goto erase_err;
			}

			/*
			 * Scale the timeout linearly with the size of the flash, with
			 * a minimum calibrated to an old 2MB flash. We could try to
			 * pull these from CFI/SFDP, but these values should be good
			 * enough for now.
			 */
			timeout = max(CHIP_ERASE_2MB_READY_WAIT_JIFFIES,
					CHIP_ERASE_2MB_READY_WAIT_JIFFIES *
					(unsigned long)(mtd->size / SZ_2M));
			ret = spi_nor_wait_till_ready_with_timeout(nor, timeout);
			if (ret)
				goto erase_err;
		} while (ids--);

	/* REVISIT in some cases we could speed up erasing large regions
	 * by using SPINOR_OP_SE instead of SPINOR_OP_BE_4K.  We may have set up
	 * to use "small sector erase", but that's not always optimal.
	 */

	/* "sector"-at-a-time erase */
	} else {
		u32 bank_size = 0;

		if (nor->n_banks && nor->flash_select_bank)
			bank_size = (u32)div_u64(mtd->size, nor->n_banks);

		while (len) {

			if (nor->n_banks && nor->flash_select_bank)
				nor->flash_select_bank(nor, addr, addr / bank_size);

			write_enable(nor);

			ret = spi_nor_erase_sector(nor, addr);
			if (ret)
				goto erase_err;

			addr += mtd->erasesize;
			len -= mtd->erasesize;

			ret = spi_nor_wait_till_ready(nor);
			if (ret)
				goto erase_err;
		}
	}

	write_disable(nor);

erase_err:
	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_ERASE);

	instr->state = ret ? MTD_ERASE_FAILED : MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return ret;
}

static int spi_nor_has_lock_erase(struct mtd_info *mtd,
		struct erase_info *instr)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	int ret;

	if (mtd->_unlock(mtd, instr->addr, instr->len))
		dev_err(nor->dev, "erase unlock error\n");

	ret = spi_nor_erase(mtd, instr);

	mtd->_lock(mtd, 0, mtd->size);

	return ret;
}

/* Write status register and ensure bits in mask match written values */
static int write_sr_and_check(struct spi_nor *nor, u8 status_new, u8 mask)
{
	int ret;

	write_enable(nor);
	ret = write_sr(nor, status_new);
	if (ret)
		return ret;

	ret = spi_nor_wait_till_ready(nor);
	if (ret)
		return ret;

	ret = read_sr(nor);
	if (ret < 0)
		return ret;

	return ((ret & mask) != (status_new & mask)) ? -EIO : 0;
}

static void stm_get_locked_range(struct spi_nor *nor, u8 sr, loff_t *ofs,
				 uint64_t *len)
{
	struct mtd_info *mtd = &nor->mtd;
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	int shift = ffs(mask) - 1;
	int pow;

	if (!(sr & mask)) {
		/* No protection */
		*ofs = 0;
		*len = 0;
	} else {
		pow = ((sr & mask) ^ mask) >> shift;
		*len = mtd->size >> pow;
		if (nor->flags & SNOR_F_HAS_SR_TB && sr & SR_TB)
			*ofs = 0;
		else
			*ofs = mtd->size - *len;
	}
}

/*
 * Return 1 if the entire region is locked (if @locked is true) or unlocked (if
 * @locked is false); 0 otherwise
 */
static int stm_check_lock_status_sr(struct spi_nor *nor, loff_t ofs, uint64_t len,
				    u8 sr, bool locked)
{
	loff_t lock_offs;
	uint64_t lock_len;

	if (!len)
		return 1;

	stm_get_locked_range(nor, sr, &lock_offs, &lock_len);

	if (locked)
		/* Requested range is a sub-range of locked range */
		return (ofs + len <= lock_offs + lock_len) && (ofs >= lock_offs);
	else
		/* Requested range does not overlap with locked range */
		return (ofs >= lock_offs + lock_len) || (ofs + len <= lock_offs);
}

static int stm_is_locked_sr(struct spi_nor *nor, loff_t ofs, uint64_t len,
			    u8 sr)
{
	return stm_check_lock_status_sr(nor, ofs, len, sr, true);
}

static int stm_is_unlocked_sr(struct spi_nor *nor, loff_t ofs, uint64_t len,
			      u8 sr)
{
	return stm_check_lock_status_sr(nor, ofs, len, sr, false);
}

/*
 * Lock a region of the flash. Compatible with ST Micro and similar flash.
 * Supports the block protection bits BP{0,1,2} in the status register
 * (SR). Does not support these features found in newer SR bitfields:
 *   - SEC: sector/block protect - only handle SEC=0 (block protect)
 *   - CMP: complement protect - only support CMP=0 (range is not complemented)
 *
 * Support for the following is provided conditionally for some flash:
 *   - TB: top/bottom protect
 *
 * Sample table portion for 8MB flash (Winbond w25q64fw):
 *
 *   SEC  |  TB   |  BP2  |  BP1  |  BP0  |  Prot Length  | Protected Portion
 *  --------------------------------------------------------------------------
 *    X   |   X   |   0   |   0   |   0   |  NONE         | NONE
 *    0   |   0   |   0   |   0   |   1   |  128 KB       | Upper 1/64
 *    0   |   0   |   0   |   1   |   0   |  256 KB       | Upper 1/32
 *    0   |   0   |   0   |   1   |   1   |  512 KB       | Upper 1/16
 *    0   |   0   |   1   |   0   |   0   |  1 MB         | Upper 1/8
 *    0   |   0   |   1   |   0   |   1   |  2 MB         | Upper 1/4
 *    0   |   0   |   1   |   1   |   0   |  4 MB         | Upper 1/2
 *    X   |   X   |   1   |   1   |   1   |  8 MB         | ALL
 *  ------|-------|-------|-------|-------|---------------|-------------------
 *    0   |   1   |   0   |   0   |   1   |  128 KB       | Lower 1/64
 *    0   |   1   |   0   |   1   |   0   |  256 KB       | Lower 1/32
 *    0   |   1   |   0   |   1   |   1   |  512 KB       | Lower 1/16
 *    0   |   1   |   1   |   0   |   0   |  1 MB         | Lower 1/8
 *    0   |   1   |   1   |   0   |   1   |  2 MB         | Lower 1/4
 *    0   |   1   |   1   |   1   |   0   |  4 MB         | Lower 1/2
 *
 * Returns negative on errors, 0 on success.
 */
static int stm_lock(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	struct mtd_info *mtd = &nor->mtd;
	int status_old, status_new;
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	u8 shift = ffs(mask) - 1, pow, val;
	loff_t lock_len;
	bool can_be_top = true, can_be_bottom = nor->flags & SNOR_F_HAS_SR_TB;
	bool use_top;
	int ret;

	status_old = read_sr(nor);
	if (status_old < 0)
		return status_old;

	/* If nothing in our range is unlocked, we don't need to do anything */
	if (stm_is_locked_sr(nor, ofs, len, status_old))
		return 0;

	/* If anything below us is unlocked, we can't use 'bottom' protection */
	if (!stm_is_locked_sr(nor, 0, ofs, status_old))
		can_be_bottom = false;

	/* If anything above us is unlocked, we can't use 'top' protection */
	if (!stm_is_locked_sr(nor, ofs + len, mtd->size - (ofs + len),
				status_old))
		can_be_top = false;

	if (!can_be_bottom && !can_be_top)
		return -EINVAL;

	/* Prefer top, if both are valid */
	use_top = can_be_top;

	/* lock_len: length of region that should end up locked */
	if (use_top)
		lock_len = mtd->size - ofs;
	else
		lock_len = ofs + len;

	/*
	 * Need smallest pow such that:
	 *
	 *   1 / (2^pow) <= (len / size)
	 *
	 * so (assuming power-of-2 size) we do:
	 *
	 *   pow = ceil(log2(size / len)) = log2(size) - floor(log2(len))
	 */
	pow = ilog2(mtd->size) - ilog2(lock_len);
	val = mask - (pow << shift);
	if (val & ~mask)
		return -EINVAL;
	/* Don't "lock" with no region! */
	if (!(val & mask))
		return -EINVAL;

	status_new = (status_old & ~mask & ~SR_TB) | val;

	/* Disallow further writes if WP pin is asserted */
	status_new |= SR_SRWD;

	if (!use_top)
		status_new |= SR_TB;

	/* Don't bother if they're the same */
	if (status_new == status_old)
		return 0;

	/* Only modify protection if it will not unlock other areas */
	if ((status_new & mask) < (status_old & mask))
		return -EINVAL;

	write_enable(nor);
	ret = write_sr(nor, status_new);
	if (ret)
		return ret;
	return spi_nor_wait_till_ready(nor);
}

/*
 * Unlock a region of the flash. See stm_lock() for more info
 *
 * Returns negative on errors, 0 on success.
 */
static int stm_unlock(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	struct mtd_info *mtd = &nor->mtd;
	int status_old, status_new;
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	u8 shift = ffs(mask) - 1, pow, val;
	loff_t lock_len;
	bool can_be_top = true, can_be_bottom = nor->flags & SNOR_F_HAS_SR_TB;
	bool use_top;
	int ret;

	status_old = read_sr(nor);
	if (status_old < 0)
		return status_old;

	/* If nothing in our range is locked, we don't need to do anything */
	if (stm_is_unlocked_sr(nor, ofs, len, status_old))
		return 0;

	/* If anything below us is locked, we can't use 'top' protection */
	if (!stm_is_unlocked_sr(nor, 0, ofs, status_old))
		can_be_top = false;

	/* If anything above us is locked, we can't use 'bottom' protection */
	if (!stm_is_unlocked_sr(nor, ofs + len, mtd->size - (ofs + len),
				status_old))
		can_be_bottom = false;

	if (!can_be_bottom && !can_be_top)
		return -EINVAL;

	/* Prefer top, if both are valid */
	use_top = can_be_top;

	/* lock_len: length of region that should remain locked */
	if (use_top)
		lock_len = mtd->size - (ofs + len);
	else
		lock_len = ofs;

	/*
	 * Need largest pow such that:
	 *
	 *   1 / (2^pow) >= (len / size)
	 *
	 * so (assuming power-of-2 size) we do:
	 *
	 *   pow = floor(log2(size / len)) = log2(size) - ceil(log2(len))
	 */
	pow = ilog2(mtd->size) - order_base_2(lock_len);
	if (lock_len == 0) {
		val = 0; /* fully unlocked */
	} else {
		val = mask - (pow << shift);
		/* Some power-of-two sizes are not supported */
		if (val & ~mask)
			return -EINVAL;
	}

	status_new = (status_old & ~mask & ~SR_TB) | val;

	/* Don't protect status register if we're fully unlocked */
	if (lock_len == 0)
		status_new &= ~SR_SRWD;

	if (!use_top)
		status_new |= SR_TB;

	/* Don't bother if they're the same */
	if (status_new == status_old)
		return 0;

	/* Only modify protection if it will not lock other areas */
	if ((status_new & mask) > (status_old & mask))
		return -EINVAL;

	write_enable(nor);
	ret = write_sr(nor, status_new);
	if (ret)
		return ret;
	return spi_nor_wait_till_ready(nor);
}

/*
 * Check if a region of the flash is (completely) locked. See stm_lock() for
 * more info.
 *
 * Returns 1 if entire region is locked, 0 if any portion is unlocked, and
 * negative on errors.
 */
static int stm_is_locked(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	int status;

	status = read_sr(nor);
	if (status < 0)
		return status;

	return stm_is_locked_sr(nor, ofs, len, status);
}

static void sunxi_get_locked_range(struct spi_nor *nor, u8 sr, u8 cr,
		loff_t *ofs, uint64_t *len)
{
	u8 mask, shift;
	int bp_check, i, flash_protection_size = 0;
	struct nor_protection *flash_protection = NULL;

	if (JEDEC_MFR(nor->info) == SNOR_MFR_MACRONIX) {
		mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
		flash_protection = mxic_protection;
		flash_protection_size = ARRAY_SIZE(mxic_protection);
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_EON) { /* esmt */
		mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
		if (nor->mtd.size == BP_SZ_16M) {
			flash_protection = esmt_protection;
			flash_protection_size = ARRAY_SIZE(esmt_protection);
		} else {
			flash_protection = esmt_protection_8M;
			flash_protection_size = ARRAY_SIZE(esmt_protection_8M);
		}
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_GIGADEVICE) {
		mask = SR_BP4 | SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
		flash_protection = gd_protection;
		flash_protection_size = ARRAY_SIZE(gd_protection);
	} else {
		dev_err(nor->dev, "not support lock 0x%x\n",
				JEDEC_MFR(nor->info));
		*ofs = 0;
		*len = 0;
		return;
	}
	shift = ffs(mask) - 1;

	bp_check = (sr & mask) >> shift;

	if (!(sr & mask)) {
		/* No protection */
		*ofs = 0;
		*len = 0;
	} else {
		for (i = 0; i < flash_protection_size; i++) {
			if (bp_check == flash_protection[i].bp) {
				*len = flash_protection[i].boundary;
				break;
			}
		}

		if (JEDEC_MFR(nor->info) == SNOR_MFR_MACRONIX) {
			/* if (nor->flags & SNOR_F_HAS_CR_TB && cr & CR_TB_MX) */
				/* *ofs = 0; */
			/* else */
				/* *ofs = mtd->size - *len; */
			*ofs = 0;	/* now only support bottom in mxic_protection */
		} else if (JEDEC_MFR(nor->info) == SNOR_MFR_EON) {
			/* TODO: read TB */
			*ofs = 0;	/* now only support bottom in esmt_protection */
		} else if (JEDEC_MFR(nor->info) == SNOR_MFR_GIGADEVICE) {
			/* TODO: read TB */
			*ofs = 0;	/* now only support bottom in gd_protection */
		}
	}
	dev_dbg(nor->dev, "ofs:0x%llx,len=0x%llx flag:0x%x sr:0x%x cr:0x%x\n",
			*ofs, *len, nor->flags, sr, cr);
}


/*
 * Return 1 if the entire region is locked (if @locked is true) or unlocked (if
 * @locked is false); 0 otherwise
 */
static int sunxi_check_lock_status_sr_cr(struct spi_nor *nor, loff_t ofs,
		u64 len, u8 sr, u8 cr, bool locked)
{
	loff_t lock_offs;
	uint64_t lock_len;

	if (!len)
		return 1;

	sunxi_get_locked_range(nor, sr, cr, &lock_offs, &lock_len);

	if (locked)
		/* Requested range is a sub-range of locked range */
		return (ofs + len <= lock_offs + lock_len) && (ofs >= lock_offs);
	else
		/* Requested range does not overlap with locked range */
		return (ofs >= lock_offs + lock_len) || (ofs + len <= lock_offs);
}

static int sunxi_is_locked_sr_cr(struct spi_nor *nor, loff_t ofs,
		uint64_t len, u8 sr, u8 cr)
{
	return sunxi_check_lock_status_sr_cr(nor, ofs, len, sr, cr, true);
}

static int sunxi_is_unlocked_sr_cr(struct spi_nor *nor, loff_t ofs,
		uint64_t len, u8 sr, u8 cr)
{
	return sunxi_check_lock_status_sr_cr(nor, ofs, len, sr, cr, false);
}

static int sunxi_handle_lock(struct spi_nor *nor, loff_t ofs,
		uint64_t len, int lock)
{
	struct mtd_info *mtd = &nor->mtd;
	int status_old = 0, status_new = 0;
	int config_old = 0, config_new = 0;
	int otp_status_old = 0, otp_status_new = 0;
	u8 mask, shift, val = 0;
	loff_t lock_len;
	/* hardcode now, sunxi only use bottom */
	bool can_be_top = false, can_be_bottom = true;
	bool use_bottom;
	int i, ret, protect_flag = 0, flash_protection_size = 0;
	struct nor_protection *flash_protection = NULL;

	if (JEDEC_MFR(nor->info) == SNOR_MFR_MACRONIX) {
		flash_protection = mxic_protection;
		flash_protection_size = ARRAY_SIZE(mxic_protection);
		mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
		config_old = read_cr(nor);
		if (config_old < 0)
			return config_old;
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_EON) {
		if (JEDEC_ID(nor->info) == GM_64A_ID) {
			dev_dbg(nor->dev, "not support lock 0x%x\n",
				JEDEC_ID(nor->info));
			return 0;
		}
		if (mtd->size == BP_SZ_16M) {
			flash_protection = esmt_protection;
			flash_protection_size = ARRAY_SIZE(esmt_protection);
		} else {
			flash_protection = esmt_protection_8M;
			flash_protection_size = ARRAY_SIZE(esmt_protection_8M);
		}
		mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
		otp_status_old = read_otp_sr(nor);
		if (otp_status_old < 0)
			return otp_status_old;
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_GIGADEVICE) {
		flash_protection = gd_protection;
		flash_protection_size = ARRAY_SIZE(gd_protection);
		mask = SR_BP4 | SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
	} else {
		dev_dbg(nor->dev, "not support lock 0x%x\n",
				JEDEC_MFR(nor->info));
		return 0;
	}
	shift = ffs(mask) - 1;

	status_old = read_sr(nor);
	if (status_old < 0)
		return status_old;

	if (lock) {
		/* If nothing in our range is unlocked, we don't need to do anything */
		if (sunxi_is_locked_sr_cr(nor, ofs, len,
					status_old, config_old))
			return 0;
	} else {
		/* If nothing in our range is locked, we don't need to do anything */
		if (sunxi_is_unlocked_sr_cr(nor, ofs, len,
					status_old, config_old))
			return 0;
	}

	if (!can_be_bottom && !can_be_top)
		return -EINVAL;

	/* for sunxi, prefer bottom */
	use_bottom = can_be_bottom;

	/* lock_len: length of region that should end up locked */
	if (use_bottom)
		lock_len = lock ? (ofs + len) : ofs;
	else
		lock_len = lock ? (mtd->size - ofs) : (mtd->size - (ofs + len));

	/* in case over the max, we set the max default */
	val = flash_protection[flash_protection_size - 1].bp << shift;
	protect_flag = flash_protection[flash_protection_size - 1].flag;
	for (i = 1; i < flash_protection_size; i++) {
		if (lock_len < flash_protection[i].boundary) {
			val = flash_protection[i - 1].bp << shift;
			protect_flag = flash_protection[i - 1].flag;
			break;
		}
	}

	dev_dbg(nor->dev, "sunxi_lock:%d val:0x%x, flag:0x%x\n",
			lock, val, protect_flag);
	status_new = (status_old & ~mask) | (val & mask);

#if 0 /* not suitable for gd */
	if (lock) {
		/* Only modify protection if it will not unlock other areas */
		if ((status_new & mask) < (status_old & mask))
			return -EINVAL;
	} else {
		/* Only modify protection if it will not lock other areas */
		if ((status_new & mask) > (status_old & mask))
			return -EINVAL;
	}
#endif

	if (JEDEC_MFR(nor->info) == SNOR_MFR_MACRONIX) {
		if (protect_flag & SET_TB)
			config_new = config_old | CR_TB_MX;
		else
			config_new = config_old & ~CR_TB_MX;

		if ((status_old != status_new) || (config_old != config_new)) {
			u8 sr_cr[2] = {status_new, config_new};
			ret = write_sr_cr(nor, sr_cr);
			if (ret)
				return ret;
		}
		dev_dbg(nor->dev, "sr:0x%x --> 0x%x cr:0x%x --> 0x%x\n",
				status_old, status_new, config_old, config_new);
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_EON) {
		if (protect_flag & SET_TB) {
			otp_status_new = otp_status_old | OTP_SR_TB_EON;
		} else {
			otp_status_new = otp_status_old & ~OTP_SR_TB_EON;
		}

		if (otp_status_old != otp_status_new) {
			ret = write_otp_sr(nor, otp_status_new);
			if (ret)
				return ret;
		}

		if (status_old != status_new) {
			ret = write_sr_and_check(nor, status_new, mask);
			if (ret)
				return ret;
		}
		dev_dbg(nor->dev, "sr:0x%x --> 0x%x otp_sr:0x%x --> 0x%x\n",
				status_old, status_new, otp_status_old,
				otp_status_new);
	} else if (JEDEC_MFR(nor->info) == SNOR_MFR_GIGADEVICE) {
		if (protect_flag & SET_CMP)
			ret = gigadevice_config_cmp(nor, 1);
		else
			ret = gigadevice_config_cmp(nor, 0);
		if (ret)
			return ret;

		if (status_old != status_new) {
			ret = write_sr_and_check(nor, status_new, mask);
			if (ret)
				return ret;
		}
		dev_dbg(nor->dev, "sr:0x%x --> 0x%x\n", status_old, status_new);
	}

	return 0;
}

static int sunxi_lock(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	return sunxi_handle_lock(nor, ofs, len, true);

}
static int sunxi_unlock(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	return sunxi_handle_lock(nor, ofs, len, false);
}

/*
 * Check if a region of the flash is (completely) locked. See sunxi_lock() for
 * more info.
 *
 * Returns 1 if entire region is locked, 0 if any portion is unlocked, and
 * negative on errors.
 */
static int sunxi_is_locked(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	int status = 0;
	int config = 0;

	status = read_sr(nor);
	if (status < 0)
		return status;

	if (JEDEC_MFR(nor->info) == SNOR_MFR_MACRONIX) {
		config = read_cr(nor);
		if (config < 0)
			return config;
	}

	return sunxi_is_locked_sr_cr(nor, ofs, len, status, config);
}

static int sunxi_individual_lock_status(struct spi_nor *nor,
					loff_t ofs, uint64_t len, bool locked)
{
	u8 read_opcode, read_dummy;
	loff_t ofs_lock = ofs;
	loff_t ofs_lock_end = ofs + len;
	u8 lock_status;
	u8 addr_width;
	u8 lock_mask;

	if (ofs > nor->mtd.size) {
		dev_err(nor->dev, "The lock address exceeds the flash size\n");
		return -1;
	}
	if (ofs_lock_end > nor->mtd.size)
		ofs_lock_end = nor->mtd.size - ofs_lock;
	if (locked)
		lock_mask = 0xff;
	else
		lock_mask = 0;

	/* align down */
	if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
			(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
		ofs_lock &= ~(NOR_LOCK_SECTOR_SIZE - 1);
	else
		ofs_lock &= ~(NOR_LOCK_BLOCK_SIZE - 1);

	read_opcode = nor->read_opcode;
	read_dummy = nor->read_dummy;
	addr_width = nor->addr_width;

	nor->read_opcode = SPINOR_OP_RDBLK;
	nor->read_dummy = 0;

	write_enable(nor);
	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_MXIC:
		nor->read_opcode = SPINOR_OP_MXICRBLK;
		nor->addr_width = 4;

		while (ofs_lock < ofs_lock_end) {
			nor->read(nor, ofs_lock, 1, &lock_status);
			dev_dbg(nor->dev, "check %s to:%lld lock_status:%d\n",
					locked ? "lock" : "unlock",
					ofs_lock, lock_status);
			/* 0xff is locked sign */
			if (lock_status == lock_mask) {
				if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
					(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
					/* sector for first and last block */
					ofs_lock += NOR_LOCK_SECTOR_SIZE;
				else
					/* block for middle blocks */
					ofs_lock += NOR_LOCK_BLOCK_SIZE;
				continue;
			} else {
				nor->read_opcode = read_opcode;
				nor->read_dummy = read_dummy;
				nor->addr_width = addr_width;
				return -1;
			}
		}
		break;
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
	case SNOR_MFR_FM:
	case SNOR_MFR_XTX:
		while (ofs_lock < ofs_lock_end) {
			nor->read(nor, ofs_lock, 1, &lock_status);
			dev_dbg(nor->dev, "check %s to:%lld lock_status:%d\n",
					locked == 1 ? "lock" : "unlock",
					ofs_lock, lock_status);

			if (lock_status != locked) {
				nor->read_opcode = read_opcode;
				nor->read_dummy = read_dummy;
				return -1;
			}

			if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
				(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
				/* sector for first and last block */
				ofs_lock += NOR_LOCK_SECTOR_SIZE;
			else
				/* block for middle blocks */
				ofs_lock += NOR_LOCK_BLOCK_SIZE;
		}
		break;
	default:
		dev_err(nor->dev, "not support individual is lock nor 0x%x\n",
				JEDEC_MFR(nor->info));
	}

	nor->read_opcode = read_opcode;
	nor->read_dummy = read_dummy;
	nor->addr_width = addr_width;
	return 1;
}

static int sunxi_individual_is_lock(struct spi_nor *nor,
					loff_t ofs, uint64_t len)
{
	return sunxi_individual_lock_status(nor, ofs, len, true);
}

#ifdef WLOCK_CHECK
static int sunxi_individual_is_unlock(struct spi_nor *nor,
					loff_t ofs, uint64_t len)
{
	return sunxi_individual_lock_status(nor, ofs, len, false);
}
#endif
/* Write status register and ensure bits in mask match written values */
static int write_lock_and_check(struct spi_nor *nor, loff_t to, bool locked)
{
	int ret;
#ifdef WLOCK_CHECK
	int check_len;

	if ((to < NOR_LOCK_BLOCK_SIZE) ||
			(to >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
		check_len = NOR_LOCK_SECTOR_SIZE;
	else
		check_len = NOR_LOCK_BLOCK_SIZE;
#endif

	write_enable(nor);
	ret = nor->write(nor, to, 0, NULL);

	ret = spi_nor_wait_till_ready(nor);
	if (ret)
		return -1;
#ifdef WLOCK_CHECK
	if (locked)
		ret = sunxi_individual_is_lock(nor, to, check_len);
	else
		ret = sunxi_individual_is_unlock(nor, to, check_len);
#endif
	return ret < 0 ? ret : 0;
}

/* Write DPB register lock block/sector */
static int write_mxic_lock(struct spi_nor *nor, loff_t ofs, uint64_t len, bool locked)
{
	u32 ret;
	u_char lock_com;
	loff_t ofs_lock = ofs;
	loff_t ofs_lock_end = ofs + len;

	if (ofs_lock_end > nor->mtd.size)
		ofs_lock_end = nor->mtd.size - ofs_lock;

	if (locked) {
		lock_com = 0xff;

		/* area is locked, no need to do antrhing */
		if (sunxi_individual_is_lock(nor, ofs_lock, len) > 0)
			return 0;
	} else {
		lock_com = 0;

		#ifdef WLOCK_CHECK
		/* area is locked, no need to do antrhing */
		if (sunxi_individual_is_unlock(nor, ofs_lock, len) > 0)
			return 0;
		#endif
	}

	while (ofs_lock < ofs_lock_end) {
		write_enable(nor);
		ret = nor->write(nor, ofs_lock, 1, &lock_com);

		if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
			(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
		/* sector for first and last block */
			ofs_lock += NOR_LOCK_SECTOR_SIZE;
		else
			/* block for middle blocks */
			ofs_lock += NOR_LOCK_BLOCK_SIZE;
	}

	return ret < 0 ? ret : 0;
}

/*
 * Support Individual Locks functions
 * the device will utilize the Individual Block Locks to protect any
 * individual sector or block.
 */
static int sunxi_individual_lock_global(struct spi_nor *nor)
{
	write_enable(nor);

	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
	case SNOR_MFR_FM:
	case SNOR_MFR_XTX:
	case SNOR_MFR_MXIC:
		return nor->write_reg(nor, SPINOR_OP_GBLK, NULL, 0);
	default:
		dev_err(nor->dev, "not support individual global lock nor 0x%x\n",
				JEDEC_MFR(nor->info));
	}

	return 0;
}

static int sunxi_individual_unlock_global(struct spi_nor *nor)
{
	write_enable(nor);

	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
	case SNOR_MFR_FM:
	case SNOR_MFR_XTX:
		return nor->write_reg(nor, SPINOR_OP_UGBLK, NULL, 0);
	case SNOR_MFR_MXIC:
		nor->write_reg(nor, SPINOR_OP_USPB, NULL, 0);
		spi_nor_wait_till_ready(nor);
		write_enable(nor);
		return nor->write_reg(nor, SPINOR_OP_UGBLK, NULL, 0);
	default:
		dev_err(nor->dev,
			"not support individual global unlock nor 0x%x\n",
			JEDEC_MFR(nor->info));
	}
	return 0;
}
static int sunxi_individual_handle_lock(struct spi_nor *nor,
					loff_t ofs, uint64_t len, bool locked)
{
	u8 program_opcode;
	u32 ret;
	u8 read_opcode;
	u8 read_dummy;
	u8 addr_width;
	loff_t ofs_lock = ofs;
	loff_t ofs_lock_end = ofs + len;

	if (ofs > nor->mtd.size) {
		dev_err(nor->dev, "The lock address exceeds the flash size\n");
		return -1;
	}
	if (ofs_lock_end > nor->mtd.size)
		ofs_lock_end = nor->mtd.size - ofs_lock;

	/* align down */
	if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
			(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
		ofs_lock &= ~(NOR_LOCK_SECTOR_SIZE - 1);
	else
		ofs_lock &= ~(NOR_LOCK_BLOCK_SIZE - 1);

	read_opcode = nor->read_opcode;
	read_dummy = nor->read_dummy;
	program_opcode = nor->program_opcode;
	addr_width = nor->addr_width;

	write_enable(nor);
	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_MXIC:
		nor->program_opcode = SPINOR_OP_MXICWBLK;
		nor->read_opcode = SPINOR_OP_MXICRBLK;
		nor->read_dummy = 0;
		nor->addr_width = 4;

		write_mxic_lock(nor, ofs_lock, len, locked);
		break;
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
	case SNOR_MFR_FM:
	case SNOR_MFR_XTX:
		if (locked)
			nor->program_opcode = SPINOR_OP_IBLK;
		else
			nor->program_opcode = SPINOR_OP_UIBLK;

		while (ofs_lock < ofs_lock_end) {
			dev_dbg(nor->dev, "%s handle to:%lld\n",
					locked ?
					"lock" : "unlock", ofs_lock);
			ret = write_lock_and_check(nor, ofs_lock, locked);
			if (ret)
				dev_err(nor->dev, "%s to:%lld err\n",
						locked ?
						"lock" : "unlock", ofs_lock);

			if ((ofs_lock < NOR_LOCK_BLOCK_SIZE) ||
				(ofs_lock >= nor->mtd.size - NOR_LOCK_BLOCK_SIZE))
				/* sector for first and last block */
				ofs_lock += NOR_LOCK_SECTOR_SIZE;
			else
				/* block for middle blocks */
				ofs_lock += NOR_LOCK_BLOCK_SIZE;
		}
		break;
	default:
		dev_err(nor->dev, "not support individual lock nor 0x%x...\n",
				JEDEC_MFR(nor->info));
	}

	nor->read_opcode = read_opcode;
	nor->read_dummy = read_dummy;
	nor->program_opcode = program_opcode;
	nor->addr_width = addr_width;
	return 0;
}

static int sunxi_individual_lock(struct spi_nor *nor, loff_t ofs, uint64_t len)
{
	if (ofs == 0 && len == nor->mtd.size)
		return sunxi_individual_lock_global(nor);

	return sunxi_individual_handle_lock(nor, ofs, len, true);
}

static int sunxi_individual_unlock(struct spi_nor *nor,
					loff_t ofs, uint64_t len)
{
	if (ofs == 0 && len == nor->mtd.size)
		return sunxi_individual_unlock_global(nor);

	return sunxi_individual_handle_lock(nor, ofs, len, false);
}

static int sunxi_individual_lock_enable(struct spi_nor *nor)
{
	u8 status;

	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
		status = read_sr3(nor);
		write_sr3(nor, status | SR_WPS_EN_WINBOND);
		if (read_sr3(nor) | SR_WPS_EN_WINBOND)
			return 0;
		else
			return -1;
	case SNOR_MFR_FM:
		status = read_sr2(nor);
		write_sr2(nor, status | SR_WPS_EN_FM);
		if (read_sr2(nor) | SR_WPS_EN_FM)
			return 0;
		else
			return -1;
	case SNOR_MFR_XTX:
		status = read_sr2(nor);
		write_sr2(nor, status | SR_WPS_EN_XTX);
		if (read_sr2(nor) | SR_WPS_EN_XTX)
			return 0;
		else
			return -1;
	default:
		dev_err(nor->dev, "not support individual lock nor 0x%x\n",
				JEDEC_MFR(nor->info));
	}

	return -1;
}

static int sunxi_individual_lock_is_enable(struct spi_nor *nor)
{
	switch (JEDEC_MFR(nor->info)) {
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_PUYA:
		if (read_sr3(nor) & SR_WPS_EN_WINBOND)
			return 1;
		else
			return 0;
	case SNOR_MFR_FM:
		if (read_sr2(nor) & SR_WPS_EN_FM)
			return 1;
		else
			return 0;
	case SNOR_MFR_XTX:
		if (read_sr2(nor) & SR_WPS_EN_XTX)
			return 1;
		else
			return 0;
	case SNOR_MFR_MXIC:
		if (read_mxic_sr(nor) & SR_WPSEL)
			return 1;
		else
			return 0;
	default:
		return 0;
	}
}

static void sunxi_individual_lock_init(struct spi_nor *nor)
{
	unsigned int rval = 0;

	if (nor->info->flags & SPI_NOR_INDIVIDUAL_LOCK) {
		if (!of_property_read_u32(nor->dev->of_node,
				"individual_lock", &rval)) {
			if (rval) {
				nor->flags |= SNOR_F_INDIVIDUAL_LOCK;
				sunxi_individual_lock_enable(nor);
			}
		} else
			dev_err(nor->dev, "get individual_lock fail\n");
	}

	/*
	 * If the Individual lock is not used and the Flash is locked,
	 * it will be unlocked globally
	 */
	if (!(nor->flags & SNOR_F_INDIVIDUAL_LOCK))
		if (sunxi_individual_lock_is_enable(nor))
			sunxi_individual_unlock_global(nor);

	return;
}

static int spi_nor_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	int ret;

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_LOCK);
	if (ret)
		return ret;

	ret = nor->flash_lock(nor, ofs, len);

	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_UNLOCK);
	return ret;
}

static int spi_nor_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	int ret;

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_UNLOCK);
	if (ret)
		return ret;

	ret = nor->flash_unlock(nor, ofs, len);

	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_LOCK);
	return ret;
}

static int spi_nor_is_locked(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	int ret;

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_UNLOCK);
	if (ret)
		return ret;

	ret = nor->flash_is_locked(nor, ofs, len);

	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_LOCK);
	return ret;
}



/* Used when the "_ext_id" is two bytes at most */
#define _INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _n_banks, _flags)	\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = (!(_jedec_id) ? 0 : (3 + ((_ext_id) ? 2 : 0))),	\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.n_banks = _n_banks,					\
		.flags = (_flags),

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
	_INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, 0, _flags)


#define INFO6(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 16) & 0xff,			\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = 6,						\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.n_banks = 0,						\
		.page_size = 256,					\
		.flags = (_flags),

#define CAT25_INFO(_sector_size, _n_sectors, _page_size, _addr_width, _flags)	\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = (_page_size),				\
		.addr_width = (_addr_width),				\
		.flags = (_flags),

/* NOTE: double check command sets and memory organization when you add
 * more nor chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 *
 * All newly added entries should describe *hardware* and should use SECT_4K
 * (or SECT_4K_PMC) if hardware supports erasing 4 KiB sectors. For usage
 * scenarios excluding small sectors there is config option that can be
 * disabled: CONFIG_MTD_SPI_NOR_USE_4K_SECTORS.
 * For historical (and compatibility) reasons (before we got above config) some
 * old entries may be missing 4K flag.
 */
static const struct flash_info spi_nor_ids[] = {
	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ "at25fs010",  INFO(0x1f6601, 0, 32 * 1024,   4, SECT_4K) },
	{ "at25fs040",  INFO(0x1f6604, 0, 64 * 1024,   8, SECT_4K) },

	{ "at25df041a", INFO(0x1f4401, 0, 64 * 1024,   8, SECT_4K) },
	{ "at25df321a", INFO(0x1f4701, 0, 64 * 1024,  64, SECT_4K) },
	{ "at25df641",  INFO(0x1f4800, 0, 64 * 1024, 128, SECT_4K) },

	{ "at26f004",   INFO(0x1f0400, 0, 64 * 1024,  8, SECT_4K) },
	{ "at26df081a", INFO(0x1f4501, 0, 64 * 1024, 16, SECT_4K) },
	{ "at26df161a", INFO(0x1f4601, 0, 64 * 1024, 32, SECT_4K) },
	{ "at26df321",  INFO(0x1f4700, 0, 64 * 1024, 64, SECT_4K) },

	{ "at45db081d", INFO(0x1f2500, 0, 64 * 1024, 16, SECT_4K) },

	/* EON -- en25xxx */
	{ "en25f32",    INFO(0x1c3116, 0, 64 * 1024,   64, SECT_4K) },
	{ "en25p32",    INFO(0x1c2016, 0, 64 * 1024,   64, 0) },
	{ "en25q32b",   INFO(0x1c3016, 0, 64 * 1024,   64, 0) },
	{ "en25p64",    INFO(0x1c2017, 0, 64 * 1024,  128, 0) },
	{ "en25q64",    INFO(0x1c3017, 0, 64 * 1024,  128, SECT_4K) },
	{ "en25qh64a",  INFO(0x1c7017, 0, 64 * 1024,  128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "en25qh128",  INFO(0x1c7018, 0, 64 * 1024,  256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "en25qh256",  INFO(0x1c7019, 0, 64 * 1024,  512, 0) },
	{ "en25s64",	INFO(0x1c3817, 0, 64 * 1024,  128, SECT_4K) },
	{ "en25qx128",  INFO(0x1c7118, 0, 64 * 1024,  256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* ESMT */
	{ "f25l32pa", INFO(0x8c2016, 0, 64 * 1024, 64, SECT_4K) },

	/* Everspin */
	{ "mr25h256", CAT25_INFO( 32 * 1024, 1, 256, 2, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },
	{ "mr25h10",  CAT25_INFO(128 * 1024, 1, 256, 3, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },

	/* Fujitsu */
	{ "mb85rs1mt", INFO(0x047f27, 0, 128 * 1024, 1, SPI_NOR_NO_ERASE) },

	/* GigaDevice */
	{
		"gd25q32", INFO(0xc84016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"gd25q64", INFO(0xc84017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"gd25lq64c", INFO(0xc86017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"gd25q128", INFO(0xc84018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"gd25q256d", INFO(0xc84019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"gd25f256f", INFO(0xc84319, 0x0, 64 * 1024, 512,
			SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ)
	},

	/* Intel/Numonyx -- xxxs33b */
	{ "160s33b",  INFO(0x898911, 0, 64 * 1024,  32, 0) },
	{ "320s33b",  INFO(0x898912, 0, 64 * 1024,  64, 0) },
	{ "640s33b",  INFO(0x898913, 0, 64 * 1024, 128, 0) },

	/* ISSI */
	{ "is25cd512", INFO(0x7f9d20, 0, 32 * 1024,   2, SECT_4K) },
	{ "is25wp032", INFO(0x9d7016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "is25wp064", INFO(0x9d7017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "is25wp128", INFO(0x9d7018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* Macronix */
	{ "mx25l512e",   INFO(0xc22010, 0, 64 * 1024,   1, SECT_4K) },
	{ "mx25l2005a",  INFO(0xc22012, 0, 64 * 1024,   4, SECT_4K) },
	{ "mx25l4005a",  INFO(0xc22013, 0, 64 * 1024,   8, SECT_4K) },
	{ "mx25l8005",   INFO(0xc22014, 0, 64 * 1024,  16, 0) },
	{ "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K) },
	{ "mx25l3205d",  INFO(0xc22016, 0, 64 * 1024,  64, SECT_4K) },
	{ "mx25l3255e",  INFO(0xc29e16, 0, 64 * 1024,  64, SECT_4K) },
	{ "mx25l6405d",  INFO(0xc22017, 0, 64 * 1024, 128, SECT_4K) },
	{ "mx25u6435f",  INFO(0xc22537, 0, 64 * 1024, 128, SECT_4K) },
	{ "mx25l12835f", INFO(0xc22018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "mx25l12805d", INFO(0xc22018, 0, 64 * 1024, 256, 0) },
	{ "mx25l12855e", INFO(0xc22618, 0, 64 * 1024, 256, 0) },
	{ "mx25l25635e", INFO(0xc22019, 0, 64 * 1024, 512, 0) },
	{ "mx25l25655e", INFO(0xc22619, 0, 64 * 1024, 512, 0) },
	{ "mx66l51235l", INFO(0xc2201a, 0, 64 * 1024, 1024, SPI_NOR_QUAD_READ) },
	{ "mx66l1g55g",  INFO(0xc2261b, 0, 64 * 1024, 2048, SPI_NOR_QUAD_READ) },

	/* Micron */
	{ "n25q032",	 INFO(0x20ba16, 0, 64 * 1024,   64, SPI_NOR_QUAD_READ) },
	{ "n25q032a",	 INFO(0x20bb16, 0, 64 * 1024,   64, SPI_NOR_QUAD_READ) },
	{ "n25q064",     INFO(0x20ba17, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "n25q064a",    INFO(0x20bb17, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "n25q128a11",  INFO(0x20bb18, 0, 64 * 1024,  256, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "n25q128a13",  INFO(0x20ba18, 0, 64 * 1024,  256, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "n25q256a",    INFO(0x20ba19, 0, 64 * 1024,  512, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "n25q512a",    INFO(0x20bb20, 0, 64 * 1024, 1024, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },
	{ "n25q512ax3",  INFO(0x20ba20, 0, 64 * 1024, 1024, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },
	{ "n25q00",      INFO(0x20ba21, 0, 64 * 1024, 2048, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },
	{ "n25q00a",     INFO(0x20bb21, 0, 64 * 1024, 2048, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },

	/* PMC */
	{ "pm25lv512",   INFO(0,        0, 32 * 1024,    2, SECT_4K_PMC) },
	{ "pm25lv010",   INFO(0,        0, 32 * 1024,    4, SECT_4K_PMC) },
	{ "pm25lq032",   INFO(0x7f9d46, 0, 64 * 1024,   64, SECT_4K) },

	/* Spansion -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ "s25sl032p",  INFO(0x010215, 0x4d00,  64 * 1024,  64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25sl064p",  INFO(0x010216, 0x4d00,  64 * 1024, 128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl256s0", INFO(0x010219, 0x4d00, 256 * 1024, 128, 0) },
	{ "s25fl256s1", INFO(0x010219, 0x4d01,  64 * 1024, 512, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl512s",  INFO(0x010220, 0x4d00, 256 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s70fl01gs",  INFO(0x010221, 0x4d00, 256 * 1024, 256, 0) },
	{ "s25sl12800", INFO(0x012018, 0x0300, 256 * 1024,  64, 0) },
	{ "s25sl12801", INFO(0x012018, 0x0301,  64 * 1024, 256, 0) },
	{ "s25fl128s",	INFO6(0x012018, 0x4d0180, 64 * 1024, 256, SECT_4K | SPI_NOR_QUAD_READ) },
	{ "s25fl129p0", INFO(0x012018, 0x4d00, 256 * 1024,  64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl129p1", INFO(0x012018, 0x4d01,  64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25sl004a",  INFO(0x010212,      0,  64 * 1024,   8, 0) },
	{ "s25sl008a",  INFO(0x010213,      0,  64 * 1024,  16, 0) },
	{ "s25sl016a",  INFO(0x010214,      0,  64 * 1024,  32, 0) },
	{ "s25sl032a",  INFO(0x010215,      0,  64 * 1024,  64, 0) },
	{ "s25sl064a",  INFO(0x010216,      0,  64 * 1024, 128, 0) },
	{ "s25fl004k",  INFO(0xef4013,      0,  64 * 1024,   8, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl008k",  INFO(0xef4014,      0,  64 * 1024,  16, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl016k",  INFO(0xef4015,      0,  64 * 1024,  32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl064k",  INFO(0xef4017,      0,  64 * 1024, 128, SECT_4K) },
	{ "s25fl116k",  INFO(0x014015,      0,  64 * 1024,  32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "s25fl132k",  INFO(0x014016,      0,  64 * 1024,  64, SECT_4K) },
	{ "s25fl164k",  INFO(0x014017,      0,  64 * 1024, 128, SECT_4K) },
	{ "s25fl204k",  INFO(0x014013,      0,  64 * 1024,   8, SECT_4K | SPI_NOR_DUAL_READ) },

	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ "sst25vf040b", INFO(0xbf258d, 0, 64 * 1024,  8, SECT_4K | SST_WRITE) },
	{ "sst25vf080b", INFO(0xbf258e, 0, 64 * 1024, 16, SECT_4K | SST_WRITE) },
	{ "sst25vf016b", INFO(0xbf2541, 0, 64 * 1024, 32, SECT_4K | SST_WRITE) },
	{ "sst25vf032b", INFO(0xbf254a, 0, 64 * 1024, 64, SECT_4K | SST_WRITE) },
	{ "sst25vf064c", INFO(0xbf254b, 0, 64 * 1024, 128, SECT_4K) },
	{ "sst25wf512",  INFO(0xbf2501, 0, 64 * 1024,  1, SECT_4K | SST_WRITE) },
	{ "sst25wf010",  INFO(0xbf2502, 0, 64 * 1024,  2, SECT_4K | SST_WRITE) },
	{ "sst25wf020",  INFO(0xbf2503, 0, 64 * 1024,  4, SECT_4K | SST_WRITE) },
	{ "sst25wf020a", INFO(0x621612, 0, 64 * 1024,  4, SECT_4K) },
	{ "sst25wf040b", INFO(0x621613, 0, 64 * 1024,  8, SECT_4K) },
	{ "sst25wf040",  INFO(0xbf2504, 0, 64 * 1024,  8, SECT_4K | SST_WRITE) },
	{ "sst25wf080",  INFO(0xbf2505, 0, 64 * 1024, 16, SECT_4K | SST_WRITE) },

	/* ST Microelectronics -- newer production may have feature updates */
	{ "m25p05",  INFO(0x202010,  0,  32 * 1024,   2, 0) },
	{ "m25p10",  INFO(0x202011,  0,  32 * 1024,   4, 0) },
	{ "m25p20",  INFO(0x202012,  0,  64 * 1024,   4, 0) },
	{ "m25p40",  INFO(0x202013,  0,  64 * 1024,   8, 0) },
	{ "m25p80",  INFO(0x202014,  0,  64 * 1024,  16, 0) },
	{ "m25p16",  INFO(0x202015,  0,  64 * 1024,  32, 0) },
	{ "m25p32",  INFO(0x202016,  0,  64 * 1024,  64, 0) },
	{ "m25p64",  INFO(0x202017,  0,  64 * 1024, 128, 0) },
	{ "m25p128", INFO(0x202018,  0, 256 * 1024,  64, 0) },

	{ "m25p05-nonjedec",  INFO(0, 0,  32 * 1024,   2, 0) },
	{ "m25p10-nonjedec",  INFO(0, 0,  32 * 1024,   4, 0) },
	{ "m25p20-nonjedec",  INFO(0, 0,  64 * 1024,   4, 0) },
	{ "m25p40-nonjedec",  INFO(0, 0,  64 * 1024,   8, 0) },
	{ "m25p80-nonjedec",  INFO(0, 0,  64 * 1024,  16, 0) },
	{ "m25p16-nonjedec",  INFO(0, 0,  64 * 1024,  32, 0) },
	{ "m25p32-nonjedec",  INFO(0, 0,  64 * 1024,  64, 0) },
	{ "m25p64-nonjedec",  INFO(0, 0,  64 * 1024, 128, 0) },
	{ "m25p128-nonjedec", INFO(0, 0, 256 * 1024,  64, 0) },

	{ "m45pe10", INFO(0x204011,  0, 64 * 1024,    2, 0) },
	{ "m45pe80", INFO(0x204014,  0, 64 * 1024,   16, 0) },
	{ "m45pe16", INFO(0x204015,  0, 64 * 1024,   32, 0) },

	{ "m25pe20", INFO(0x208012,  0, 64 * 1024,  4,       0) },
	{ "m25pe80", INFO(0x208014,  0, 64 * 1024, 16,       0) },
	{ "m25pe16", INFO(0x208015,  0, 64 * 1024, 32, SECT_4K) },

	{ "m25px16",    INFO(0x207115,  0, 64 * 1024, 32, SECT_4K) },
	{ "m25px32",    INFO(0x207116,  0, 64 * 1024, 64, SECT_4K) },
	{ "m25px32-s0", INFO(0x207316,  0, 64 * 1024, 64, SECT_4K) },
	{ "m25px32-s1", INFO(0x206316,  0, 64 * 1024, 64, SECT_4K) },
	{ "m25px64",    INFO(0x207117,  0, 64 * 1024, 128, 0) },
	{ "m25px80",    INFO(0x207114,  0, 64 * 1024, 16, 0) },

	/* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	{ "w25x05", INFO(0xef3010, 0, 64 * 1024,  1,  SECT_4K) },
	{ "w25x10", INFO(0xef3011, 0, 64 * 1024,  2,  SECT_4K) },
	{ "w25x20", INFO(0xef3012, 0, 64 * 1024,  4,  SECT_4K) },
	{ "w25x40", INFO(0xef3013, 0, 64 * 1024,  8,  SECT_4K) },
	{ "w25x80", INFO(0xef3014, 0, 64 * 1024,  16, SECT_4K) },
	{ "w25x16", INFO(0xef3015, 0, 64 * 1024,  32, SECT_4K) },
	{ "w25x32", INFO(0xef3016, 0, 64 * 1024,  64, SECT_4K) },
	{ "w25q32", INFO(0xef4016, 0, 64 * 1024,  64, SECT_4K) },
	{
		"w25q32dw", INFO(0xef6016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{ "w25x64", INFO(0xef3017, 0, 64 * 1024, 128, SECT_4K) },
	{ "w25q64", INFO(0xef4017, 0, 64 * 1024, 128, SECT_4K |SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{
		"w25q64dw", INFO(0xef6017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		"w25q128fw", INFO(0xef6018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{ "w25q80", INFO(0xef5014, 0, 64 * 1024,  16, SECT_4K) },
	{ "w25q80bl", INFO(0xef4014, 0, 64 * 1024,  16, SECT_4K) },
	{ "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "w25q256", INFO(0xef4019, 0, 64 * 1024, 512, SECT_4K) },
	{ "w25m512jv", _INFO(0xef7119, 0, 64 * 1024, 1024, 2,
			SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_DUAL_READ) },
	{"w25q128jv", INFO(0xef7018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
								SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)},

	/* puya */
	{ "p25q128h", INFO(0x852018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "p25q128", INFO(0x856018, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "p25q64", INFO(0x856017, 0, 64 * 1024, 128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "p25q32", INFO(0x856016, 0, 64 * 1024, 64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* ZB */
	{ "z25vq64as", INFO(0x5e4017, 0, 64 * 1024, 128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "z25vq128as", INFO(0x5e4018, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* FM */
	{ "fm25q128a", INFO(0xa14018, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "fm25q64",   INFO(0xa14017, 0, 64 * 1024, 128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* Catalyst / On Semiconductor -- non-JEDEC */
	{ "cat25c11", CAT25_INFO(  16, 8, 16, 1, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },
	{ "cat25c03", CAT25_INFO(  32, 8, 16, 2, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },
	{ "cat25c09", CAT25_INFO( 128, 8, 32, 2, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },
	{ "cat25c17", CAT25_INFO( 256, 8, 32, 2, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },
	{ "cat25128", CAT25_INFO(2048, 8, 64, 2, SPI_NOR_NO_ERASE | SPI_NOR_NO_FR) },

	/* BOYA*/
	{ "25q128",   INFO(0x684018, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ) },

	/* XT */
	{ "xt25p1288",  INFO(0x0b4018, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* Adesto */
	{ "at25sf128a",  INFO(0x1f8901, 0, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/* XMC (Wuhan Xinxin Semiconductor Manufacturing Corp.) */
	{ "XM25QH64A",  INFO(0x207017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH128A", INFO(0x207018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH256B", INFO(0x206019, 0, 64 * 1024, 512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	/*GFX*/
	{ "GM25Q64A", INFO(0x1c4017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "GM25Q128A", INFO(0x1c4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	{ },
};

static const struct flash_info *spi_nor_read_id(struct spi_nor *nor)
{
	int			tmp;
	u8			id[SPI_NOR_MAX_ID_LEN];
	const struct flash_info	*info;

	tmp = nor->read_reg(nor, SPINOR_OP_RDID, id, SPI_NOR_MAX_ID_LEN);
	if (tmp < 0) {
		dev_dbg(nor->dev, "error %d reading JEDEC ID\n", tmp);
		return ERR_PTR(tmp);
	}

	for (tmp = 0; tmp < ARRAY_SIZE(spi_nor_ids) - 1; tmp++) {
		info = &spi_nor_ids[tmp];
		if (info->id_len) {
			if (!memcmp(info->id, id, info->id_len))
				return &spi_nor_ids[tmp];
		}
	}
	dev_err(nor->dev, "unrecognized JEDEC id bytes: %02x, %02x, %02x\n",
		id[0], id[1], id[2]);
	return ERR_PTR(-ENODEV);
}

static int spi_nor_read(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	u32 bank_size = 0, bank_offset;
	u32 bid = 0;
	int ret;

	dev_dbg(nor->dev, "from 0x%08x, len %zd\n", (u32)from, len);

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_READ);
	if (ret)
		return ret;

	if (nor->n_banks && nor->flash_select_bank)
		bank_size = (u32)div_u64(mtd->size, nor->n_banks);
	else
		bank_size = mtd->size;

	while (len) {
		size_t rlen;

		bank_offset = from & (bank_size - 1);
		bid = (u32)div_u64(from, bank_size);

		if (nor->flash_select_bank)
			nor->flash_select_bank(nor, from, bid);

		if ((len + bank_offset) > bank_size)
			rlen =  bank_size - bank_offset;
		else
			rlen = len;

		ret = nor->read(nor, bank_offset, rlen, buf);
		if (ret == 0) {
			/* We shouldn't see 0-length reads */
			ret = -EIO;
			goto read_err;
		}
		if (ret < 0)
			goto read_err;

		WARN_ON(ret > len);

		*retlen += ret;
		from += ret;
		buf += ret;
		len -= ret;

	}
	ret = 0;

read_err:
	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_READ);
	return ret;
}

static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	size_t actual;
	int ret;

	dev_dbg(nor->dev, "to 0x%08x, len %zd\n", (u32)to, len);

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_WRITE);
	if (ret)
		return ret;

	write_enable(nor);

	nor->sst_write_second = false;

	actual = to % 2;
	/* Start write from odd address. */
	if (actual) {
		nor->program_opcode = SPINOR_OP_BP;

		/* write one byte. */
		ret = nor->write(nor, to, 1, buf);
		if (ret < 0)
			goto sst_write_err;
		WARN(ret != 1, "While writing 1 byte written %i bytes\n",
		     (int)ret);
		ret = spi_nor_wait_till_ready(nor);
		if (ret)
			goto sst_write_err;
	}
	to += actual;

	/* Write out most of the data here. */
	for (; actual < len - 1; actual += 2) {
		nor->program_opcode = SPINOR_OP_AAI_WP;

		/* write two bytes. */
		ret = nor->write(nor, to, 2, buf + actual);
		if (ret < 0)
			goto sst_write_err;
		WARN(ret != 2, "While writing 2 bytes written %i bytes\n",
		     (int)ret);
		ret = spi_nor_wait_till_ready(nor);
		if (ret)
			goto sst_write_err;
		to += 2;
		nor->sst_write_second = true;
	}
	nor->sst_write_second = false;

	write_disable(nor);
	ret = spi_nor_wait_till_ready(nor);
	if (ret)
		goto sst_write_err;

	/* Write out trailing byte if it exists. */
	if (actual != len) {
		write_enable(nor);

		nor->program_opcode = SPINOR_OP_BP;
		ret = nor->write(nor, to, 1, buf + actual);
		if (ret < 0)
			goto sst_write_err;
		WARN(ret != 1, "While writing 1 byte written %i bytes\n",
		     (int)ret);
		ret = spi_nor_wait_till_ready(nor);
		if (ret)
			goto sst_write_err;
		write_disable(nor);
		actual += 1;
	}
sst_write_err:
	*retlen += actual;
	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_WRITE);
	return ret;
}

/*
 * Write an address range to the nor chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int spi_nor_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	size_t page_offset, page_remain;
	int ret;
	u32 bid;
	u32 bank_size, bank_offset;

	dev_dbg(nor->dev, "to 0x%08x, len %zd\n", (u32)to, len);

	ret = spi_nor_lock_and_prep(nor, SPI_NOR_OPS_WRITE);
	if (ret)
		return ret;

	if (nor->n_banks && nor->flash_select_bank)
		bank_size = (u32)div_u64(mtd->size, nor->n_banks);
	else
		bank_size = mtd->size;

	while (len) {
		ssize_t written;
		size_t wlen = 0;

		bank_offset = to & (bank_size - 1);
		bid = (u32)div_u64(to, bank_size);

		if (nor->flash_select_bank)
			nor->flash_select_bank(nor, to, bid);

		if ((len + bank_offset) > bank_size)
			wlen = bank_size - bank_offset;
		else
			wlen = len;

		page_offset = to & (nor->page_size - 1);
		/* the size of data remaining on the first page */
		page_remain = min_t(size_t,
				    nor->page_size - page_offset, wlen);

		write_enable(nor);
		written = nor->write(nor, to, page_remain, buf);
		if (written < 0)
			goto write_err;

		ret = spi_nor_wait_till_ready(nor);
		if (ret)
			goto write_err;

		*retlen += written;
		to += written;
		buf += written;
		len -= written;

		if (written != page_remain) {
			dev_err(nor->dev,
				"While writing %zu bytes written %zd bytes\n",
				page_remain, written);
			ret = -EIO;
			goto write_err;
		}
	}

write_err:
	spi_nor_unlock_and_unprep(nor, SPI_NOR_OPS_WRITE);
	return ret;
}

static int spi_nor_has_lock_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	ssize_t ret;

	if (mtd->_unlock(mtd, to, len))
		dev_err(nor->dev, "write unlock err\n");

	/* sst nor chips use AAI word program */
	if (nor->info->flags & SST_WRITE)
		ret = sst_write(mtd, to, len, retlen, buf);
	else
		ret = spi_nor_write(mtd, to, len, retlen, buf);

	mtd->_lock(mtd, 0, mtd->size);
	return ret;
}

static int macronix_quad_enable(struct spi_nor *nor)
{
	int ret, val;

	val = read_sr(nor);
	if (val < 0)
		return val;
	write_enable(nor);

	write_sr(nor, val | SR_QUAD_EN_MX);

	if (spi_nor_wait_till_ready(nor))
		return 1;

	ret = read_sr(nor);
	if (!(ret > 0 && (ret & SR_QUAD_EN_MX))) {
		dev_err(nor->dev, "Macronix Quad bit not set\n");
		return -EINVAL;
	}

	return 0;
}

static int spansion_quad_enable(struct spi_nor *nor)
{
	u8 sr_cr[2];
	int sr, cr;

	/* Keep the current value of the Status Register. */
	sr = read_sr(nor);
	if (sr < 0) {
		dev_dbg(nor->dev, "error while reading status register\n");
		return -EINVAL;
	}
	cr = read_cr(nor);
	if (cr < 0) {
		dev_dbg(nor->dev, "error while reading config register\n");
		return -EINVAL;
	}
	sr_cr[0] = sr;
	sr_cr[1] = cr | CR_QUAD_EN_SPAN;

	return write_sr_cr(nor, sr_cr);
}

static int write_sr_gd(struct spi_nor *nor, u8 val)
{
	nor->cmd_buf[0] = val;
	return nor->write_reg(nor, SPINOR_OP_WRSR2, nor->cmd_buf, 1);
}

static int gd_read_cr_quad_enable(struct spi_nor *nor)
{
	int ret, val;

	val = read_cr(nor);
	if (val < 0)
		return val;
	if (val & CR_QUAD_EN_GD)
		return 0;

	write_enable(nor);
	write_sr_gd(nor, val | CR_QUAD_EN_GD);

	ret = spi_nor_wait_till_ready(nor);
	if (ret)
		return ret;

	ret = read_cr(nor);
	if (!(ret > 0 && (ret & CR_QUAD_EN_GD))) {
		dev_err(nor->dev, "Gd Quad bit not set\n");
		return -EINVAL;
	}

	return 0;
}

static int read_cr_EON(struct spi_nor *nor)
{
	int ret;
	u8 val;

	ret = nor->read_reg(nor, SPINOR_OP_RDCR_EON, &val, 1);
	if (ret < 0) {
		dev_dbg(nor->dev, "error %d reading CR\n", ret);
		return ret;
	}

	return val;
}

static int Eon_quad_enable(struct spi_nor *nor)
{
	int ret, val;
	if ((JEDEC_ID(nor->info) == 0x7018) || (JEDEC_ID(nor->info) == 0x7017)) {
		/*
		 * EN25QH128A no QE bit,you should enter otp mode,
		 * than you can see WXDIS bit on status register.
		 * Set it and enable quad mode.
		 */
		enter_otp(nor);
		val = read_sr(nor);
		if (val < 0) {
			exit_otp(nor);
			return val;
		}
		if (val & SR_OTP_WXDIS_EN_EON) {
			exit_otp(nor);
			return 0;
		}
		write_enable(nor);
		write_sr(nor, val | SR_OTP_WXDIS_EN_EON);

		ret = spi_nor_wait_till_ready(nor);
		if (ret) {
			exit_otp(nor);
			return ret;
		}

		ret = read_sr(nor);
		if (!(ret > 0 && (ret & SR_OTP_WXDIS_EN_EON))) {
			exit_otp(nor);
			dev_err(nor->dev, "EON WXDIS bit not set\n");
			return -EINVAL;
		}

		exit_otp(nor);

	} else {
		/*
		 * other ESMT nor still enable quad mode by setting
		 * QE bit,use 31h to write control register.
		 *
		 */
		val = read_cr_EON(nor);
		if (val < 0)
			return val;
		if (val & CR_QUAD_EN_EON)
			return 0;

		write_enable(nor);
		write_cr(nor, val | CR_QUAD_EN_EON);

		ret = spi_nor_wait_till_ready(nor);
		if (ret)
			return ret;

		ret = read_cr_EON(nor);
		if (!(ret > 0 && (ret & CR_QUAD_EN_EON))) {
			dev_err(nor->dev, "EON Quad bit not set\n");
			return -EINVAL;
		}
	}
	return 0;

}

static int set_quad_mode(struct spi_nor *nor, const struct flash_info *info)
{
	int status;

	switch (JEDEC_MFR(info)) {
	case SNOR_MFR_MACRONIX:
	case SNOR_MFR_XMC:
		//SNOR_MFR_MICRON
		if (info->id[1] >> 4 == 'b') {////because XMC and MICRON id[0] equal
			return 0;
		}
		status = macronix_quad_enable(nor);
		if (status) {
			dev_err(nor->dev, "Macronix quad-read not enabled\n");
			return -EINVAL;
		}
		return status;

	case SNOR_MFR_GIGADEVICE:
	case SNOR_MFR_ADESTO:
		status = gd_read_cr_quad_enable(nor);
		if (status) {
			dev_err(nor->dev, "GIGADEVICE quad-read not enabled\n");
			return -EINVAL;
		}
		return status;

	case SNOR_MFR_SPANSION:
	case SNOR_MFR_WINBOND:
	case SNOR_MFR_XTX:
		status = spansion_quad_enable(nor);
		if (status) {
			dev_err(nor->dev, "Spansion quad-read not enabled\n");
			return -EINVAL;
		}
		return status;

	case SNOR_MFR_EON:
		switch (JEDEC_ID(info)) {
		case GM_64A_ID:
		case GM_128A_ID:
			status = gd_read_cr_quad_enable(nor);
			if (status) {
				dev_err(nor->dev, "gfx quad-read not enabled\n");
				return -EINVAL;
			}
			return status;

		default:
			status = Eon_quad_enable(nor);
			if (status) {
				dev_err(nor->dev, "eon quad-read not enabled\n");
				return -EINVAL;
			}
			return status;
		}
	default:
		dev_err(nor->dev, "SF: Need set QEB func for %02x flash\n", JEDEC_MFR(info));
		return 0;
	}
}

static int winb_select_bank(struct spi_nor *nor, loff_t ofs, u32 id)
{
	int ret;

	nor->cmd_buf[0] = id;
	ret = nor->write_reg(nor, SPINOR_OP_DIESEL, nor->cmd_buf, 1);

	return ret;
}

static int spi_nor_check(struct spi_nor *nor)
{
	if (!nor->dev || !nor->read || !nor->write ||
		!nor->read_reg || !nor->write_reg) {
		pr_err("spi-nor: please fill all the necessary fields!\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_lock_init(struct spi_nor *nor)
{
	struct mtd_info *mtd = &nor->mtd;
	const struct  flash_info *info = nor->info;

	/* NOR protection support for STmicro/Micron chips and similar */
	if (JEDEC_MFR(info) == SNOR_MFR_MICRON ||
			info->flags & SPI_NOR_HAS_LOCK) {
		sunxi_individual_lock_init(nor);

		if (info->flags & SPI_NOR_HAS_LOCK_HANDLE)
			nor->flags |= SNOR_F_HAS_LOCK_HANDLE;

		if (nor->flags & SNOR_F_INDIVIDUAL_LOCK) {
			nor->flash_lock = sunxi_individual_lock;
			nor->flash_unlock = sunxi_individual_unlock;
			nor->flash_is_locked = sunxi_individual_is_lock;
		} else {
			if (JEDEC_MFR(info) == SNOR_MFR_MICRON ||
			    JEDEC_MFR(info) == SNOR_MFR_SST) {
				nor->flash_lock = stm_lock;
				nor->flash_unlock = stm_unlock;
				nor->flash_is_locked = stm_is_locked;
			} else {
				nor->flash_lock = sunxi_lock;
				nor->flash_unlock = sunxi_unlock;
				nor->flash_is_locked = sunxi_is_locked;
			}
		}
	} else {
		/*
		 * If the lock is not used and the Flash is locked,
		 * it will be unlocked globally
		 */
		if (sunxi_individual_lock_is_enable(nor))
			sunxi_individual_unlock_global(nor);

		if (JEDEC_MFR(info) == SNOR_MFR_MICRON ||
		    JEDEC_MFR(info) == SNOR_MFR_SST)
			stm_unlock(nor, 0, mtd->size);
		else
			sunxi_unlock(nor, 0, mtd->size);
	}

	if (nor->flash_lock && nor->flash_unlock && nor->flash_is_locked) {
		mtd->_lock = spi_nor_lock;
		mtd->_unlock = spi_nor_unlock;
		mtd->_is_locked = spi_nor_is_locked;

		/* If the lock handle function is enabled, lock the flash */
		if (nor->flags & SNOR_F_HAS_LOCK_HANDLE) {
			pr_warn("enable lock handle function\n");
			mtd->_erase = spi_nor_has_lock_erase;
			mtd->_write = spi_nor_has_lock_write;
			mtd->_lock(mtd, 0, mtd->size);
		}
	}

	return 0;
}

static int spi_nor_init(struct spi_nor *nor)
{
	const struct  flash_info *info = nor->info;
	int err;

	if (nor->flash_read == SPI_NOR_QUAD && info->flags & SPI_NOR_QUAD_READ) {
		err = set_quad_mode(nor, info);
		if (err) {
			dev_err(nor->dev, "quad mode not supported\n");
			return err;
		}
	}

	if ((nor->addr_width == 4) &&
	    (JEDEC_MFR(nor->info) != SNOR_MFR_SPANSION)) {
		/*
		 * If the RESET# pin isn't hooked up properly, or the system
		 * otherwise doesn't perform a reset command in the boot
		 * sequence, it's impossible to 100% protect against unexpected
		 * reboots (e.g., crashes). Warn the user (or hopefully, system
		 * designer) that this is bad.
		 */
		set_4byte(nor, nor->info, 1);
	}

	return 0;
}

/* mtd resume handler */
static void spi_nor_resume(struct mtd_info *mtd)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);
	int ret;

	/* re-initialize the nor chip */
	ret = spi_nor_init(nor);
	if (ret)
		dev_err(nor->dev, "resume() failed\n");
}

void spi_nor_restore(struct spi_nor *nor)
{
	/* restore the addressing mode */
	if ((nor->addr_width == 4) &&
	    (JEDEC_MFR(nor->info) != SNOR_MFR_SPANSION))
		set_4byte(nor, nor->info, 0);

	/*
	 * Send Reset Enable (66h) and Reset Device (99h)
	 */
	nor->write_reg(nor, SPINOR_OP_RESTEN, NULL, 0);
	nor->write_reg(nor, SPINOR_OP_RESET, NULL, 0);

}
EXPORT_SYMBOL_GPL(spi_nor_restore);

static int spi_nor_suspend(struct mtd_info *mtd)
{
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	/* re-initialize the nor chip */
	spi_nor_restore(nor);

	return 0;
}
static ssize_t spinordbg_param_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	int len = strlen(spinordbg_priv.param);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user
		    ((void __user *)buf, (const void *)spinordbg_priv.param,
		     (unsigned long)len)) {
			pr_warn("copy_to_user fail\n");
			return 0;
		}
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t spinordbg_param_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	if (copy_from_user(spinordbg_priv.param, buf, count)) {
		pr_warn("copy_from_user fail\n");
		return 0;
	}
	return count;
}

static ssize_t spinordbg_status_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct spi_nor *nor = get_spinor();
	char tmpstatus[80];
	u32 value;
	int len = 0;
	if (!strncmp(spinordbg_priv.param, "freq", 4)) {
		of_property_read_u32(nor->mtd.dev.of_node, "spi-max-frequency", &value);
		sprintf(tmpstatus, "%u", value);
		strcpy(spinordbg_priv.status, tmpstatus);

	} else if (!strncmp(spinordbg_priv.param, "readmode", 8)) {
		switch (nor->read_opcode)	{
		case SPINOR_OP_READ_1_1_4:
			strcpy(spinordbg_priv.status, "QUAD");
			break;
		case SPINOR_OP_READ_1_1_2:
			strcpy(spinordbg_priv.status, "DUAL");
			break;
		case SPINOR_OP_READ_FAST:
			strcpy(spinordbg_priv.status, "FAST");
			break;
		case SPINOR_OP_READ:
			strcpy(spinordbg_priv.status, "NORMAL");
			break;
		default:
			strcpy(spinordbg_priv.status, "NONE");
		}

	} else if (!strncmp(spinordbg_priv.param, "size", 4)) {
		sprintf(tmpstatus, "%llu", nor->mtd.size);
		strcpy(spinordbg_priv.status, tmpstatus);


	} else if (!strncmp(spinordbg_priv.param, "name", 4)) {
		strcpy(spinordbg_priv.status, nor->info->name);


	} else if (!strncmp(spinordbg_priv.param, "erasesize", 9)) {
		sprintf(tmpstatus, "%d", nor->mtd.erasesize);
		strcpy(spinordbg_priv.status, tmpstatus);


	} else {
		strcpy(spinordbg_priv.status, "please set param before cat status\n \
			freq    ----return spi frequency\n \
			readmode    ----return QUAD/DUAL/FAST/NORMAL\n \
			size    ----return chip size\n \
			name    ----return the matched flash ID\n \
			erasesize    ----return erase size\n");
	}

	len = strlen(spinordbg_priv.status);
	spinordbg_priv.status[len] = 0x0A;
	spinordbg_priv.status[len + 1] = 0x0;
	len = strlen(spinordbg_priv.status);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user
		   ((void __user *)buf, (const void *)spinordbg_priv.status,
		  (unsigned long)len)) {
			pr_warn("copy_to_user fail\n");
			return 0;
		}
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t spinordbg_status_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	return count;
}
static const struct file_operations param_ops = {
	.write = spinordbg_param_write,
	.read = spinordbg_param_read,
};

static const struct file_operations status_ops = {
	.write = spinordbg_status_write,
	.read = spinordbg_status_read,
};

int spinor_debug_init(void)
{
	struct dentry *my_spinordbg_root;

	my_spinordbg_root = debugfs_create_dir("spinordbg", NULL);
	if (!debugfs_create_file
	    ("status", 0644, my_spinordbg_root, NULL, &status_ops))
		goto Fail;
	if (!debugfs_create_file
	    ("param", 0644, my_spinordbg_root, NULL, &param_ops))
		goto Fail;

	return 0;

Fail:
	debugfs_remove_recursive(my_spinordbg_root);
	my_spinordbg_root = NULL;
	return -ENOENT;
}

#ifdef CONFIG_SPI_FLASH_SR
int security_regiser_is_locked(struct mtd_info *mtd, u8 sr_num)
{
	u8 val, mask;
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	if (sr_num <= 0 || sr_num >= 4) {
		dev_dbg(nor->dev, "%s, error security register num %d\n", __func__, sr_num);
		return -1;
	}

	val = read_sr2(nor);
	mask = 1 << (LB_OFS + sr_num);

	return val & mask;
}

int security_register_lock(struct mtd_info *mtd, u8 sr_num)
{
	u8 val, mask;
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	if (sr_num <= 0 || sr_num >= 4) {
		dev_dbg(nor->dev, "%s, error security register num %d\n", __func__, sr_num);
		return -1;
	}

	val = read_sr2(nor);
	mask = 1 << (LB_OFS + sr_num);
	val |= mask;
	write_sr2(nor, val);

	return 0;
}

static int security_regiser_erase(struct mtd_info *mtd, loff_t addr)
{
	u8 program_opcode;
	u8 flash_read;
	int ret;
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	program_opcode = nor->program_opcode;
	flash_read = nor->flash_read;

	nor->program_opcode = SR_OP_ERASE;
	nor->flash_read = 0;

	write_enable(nor);
	ret = nor->write(nor, addr, 0, NULL);
	ret = spi_nor_wait_till_ready(nor);

	nor->program_opcode = program_opcode;
	nor->flash_read = flash_read;

	return ret;
}

int security_regiser_read_data(struct mtd_info *mtd,
					loff_t addr, loff_t len, u_char *buf)
{
	u8 read_opcode;
	u8 read_dummy;
	u8 addr_width;
	u8 flash_read;
	int ret;
	int num;
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	read_opcode = nor->read_opcode;
	read_dummy = nor->read_dummy;
	addr_width = nor->addr_width;
	flash_read = nor->flash_read;

	nor->read_opcode = SR_OP_READ;
	nor->flash_read = 0;
	nor->read_dummy = 8;
	nor->addr_width = 3;

	ret = spi_nor_read(mtd, addr, len, &num, buf);

	nor->read_opcode = read_opcode;
	nor->flash_read = flash_read;
	nor->read_dummy = read_dummy;
	nor->addr_width = addr_width;

	return ret;

}

int security_regiser_write_data(struct mtd_info *mtd,
					loff_t addr, loff_t len, u_char *buf)
{
	u8 program_opcode;
	u8 addr_width;
	u8 flash_read;
	int ret;
	int num;
	struct spi_nor *nor = mtd_to_spi_nor(mtd);

	ret = security_regiser_erase(mtd, addr);
	if (ret)
		return ret;

	program_opcode = nor->program_opcode;
	addr_width = nor->addr_width;
	flash_read = nor->flash_read;

	nor->program_opcode = SR_OP_PROGRAM;
	nor->addr_width = 3;
	nor->flash_read = 0;

	ret = spi_nor_write(mtd, addr, len, &num, buf);

	nor->program_opcode = program_opcode;
	nor->addr_width = addr_width;
	nor->flash_read = flash_read;

	return ret;
}
#endif

int spi_nor_scan(struct spi_nor *nor, const char *name, enum read_mode mode)
{
	const struct flash_info *info = NULL;
	struct device *dev = nor->dev;
	struct mtd_info *mtd = &nor->mtd;
	struct device_node *np = spi_nor_get_flash_node(nor);
	int ret;
	int i;
	spinordbg = nor;

	ret = spi_nor_check(nor);
	if (ret)
		return ret;

	if (name)
		info = spi_nor_match_id(name);
	/* Try to auto-detect if chip name wasn't specified or not found */
	if (!info)
		info = spi_nor_read_id(nor);
	if (IS_ERR_OR_NULL(info))
		return -ENOENT;

	/*
	 * If caller has specified name of flash model that can normally be
	 * detected using JEDEC, let's verify it.
	 */
	if (name && info->id_len) {
		const struct flash_info *jinfo;

		jinfo = spi_nor_read_id(nor);
		if (IS_ERR(jinfo)) {
			return PTR_ERR(jinfo);
		} else if (jinfo != info) {
			/*
			 * JEDEC knows better, so overwrite platform ID. We
			 * can't trust partitions any longer, but we'll let
			 * mtd apply them anyway, since some partitions may be
			 * marked read-only, and we don't want to lose that
			 * information, even if it's not 100% accurate.
			 */
			dev_warn(dev, "found %s, expected %s\n",
				 jinfo->name, info->name);
			info = jinfo;
		}
	}

	mutex_init(&nor->lock);

	/*
	 * We need to check the device if have multi-chip-die capabilities.
	 * There is some devices can switch the chip die by command,
	 * such as WINBOND's devices.
	 * But, other devices also can switch the bank by gpio, such as Micross's
	 * MYXN25Q512A, it can control the S# pin to select die.
	 *
	 * TODO: implement the gpio contorl and die mapping table.
	 */
	if (info->n_banks) {
		nor->n_banks = info->n_banks;
		if (JEDEC_MFR(info) == SNOR_MFR_WINBOND) {
			dev_dbg(dev, "manufacturer is winbond\n");
			nor->flash_select_bank = winb_select_bank;
		}
	} else
		nor->n_banks = 0;

	if (!mtd->name)
		mtd->name = dev_name(dev);
	mtd->priv = nor;
	mtd->type = MTD_NORFLASH;
	mtd->writesize = 1;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->size = info->sector_size * info->n_sectors;
	mtd->_erase = spi_nor_erase;
	mtd->_read = spi_nor_read;
	mtd->_resume = spi_nor_resume;
	mtd->_suspend = spi_nor_suspend;
	/* sst nor chips use AAI word program */
	if (info->flags & SST_WRITE)
		mtd->_write = sst_write;
	else
		mtd->_write = spi_nor_write;

	if (info->flags & USE_FSR)
		nor->flags |= SNOR_F_USE_FSR;
	if (info->flags & SPI_NOR_HAS_TB)
		nor->flags |= SNOR_F_HAS_SR_TB;

#ifdef CONFIG_MTD_SPI_NOR_USE_4K_SECTORS
	/* prefer "small sector" erase if possible */
	if (info->flags & SECT_4K) {
		nor->erase_opcode = SPINOR_OP_BE_4K;
		mtd->erasesize = 4096;
	} else if (info->flags & SECT_4K_PMC) {
		nor->erase_opcode = SPINOR_OP_BE_4K_PMC;
		mtd->erasesize = 4096;
	} else
#endif
	{
		nor->erase_opcode = SPINOR_OP_SE;
		mtd->erasesize = info->sector_size;
	}

	if (info->flags & SPI_NOR_NO_ERASE)
		mtd->flags |= MTD_NO_ERASE;

	mtd->dev.parent = dev;
	nor->page_size = info->page_size;
	mtd->writebufsize = nor->page_size;

	if (np) {
		/* If we were instantiated by DT, use it */
		if (of_property_read_bool(np, "m25p,fast-read"))
			nor->flash_read = SPI_NOR_FAST;
		else
			nor->flash_read = SPI_NOR_NORMAL;
	} else {
		/* If we weren't instantiated by DT, default to fast-read */
		nor->flash_read = SPI_NOR_FAST;
	}

	/* Some devices cannot do fast-read, no matter what DT tells us */
	if (info->flags & SPI_NOR_NO_FR)
		nor->flash_read = SPI_NOR_NORMAL;

	/* Quad/Dual-read mode takes precedence over fast/normal */
	if (mode == SPI_NOR_QUAD && info->flags & SPI_NOR_QUAD_READ) {
		nor->flash_read = SPI_NOR_QUAD;
	} else if (mode == SPI_NOR_DUAL && info->flags & SPI_NOR_DUAL_READ) {
		nor->flash_read = SPI_NOR_DUAL;
	}

	/* Default commands */
	switch (nor->flash_read) {
	case SPI_NOR_QUAD:
		nor->read_opcode = SPINOR_OP_READ_1_1_4;
		break;
	case SPI_NOR_DUAL:
		nor->read_opcode = SPINOR_OP_READ_1_1_2;
		break;
	case SPI_NOR_FAST:
		nor->read_opcode = SPINOR_OP_READ_FAST;
		break;
	case SPI_NOR_NORMAL:
		nor->read_opcode = SPINOR_OP_READ;
		break;
	default:
		dev_err(dev, "No Read opcode defined\n");
		return -EINVAL;
	}

	nor->program_opcode = SPINOR_OP_PP;

	if (info->addr_width)
		nor->addr_width = info->addr_width;
	else if (mtd->size > 0x1000000) {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		nor->addr_width = 4;
		if ((JEDEC_MFR(info) == SNOR_MFR_SPANSION) ||
			(JEDEC_MFR(info) == SNOR_MFR_GIGADEVICE)) {
			/* Dedicated 4-byte command set */
			switch (nor->flash_read) {
			case SPI_NOR_QUAD:
				nor->read_opcode = SPINOR_OP_READ4_1_1_4;
				break;
			case SPI_NOR_DUAL:
				nor->read_opcode = SPINOR_OP_READ4_1_1_2;
				break;
			case SPI_NOR_FAST:
				nor->read_opcode = SPINOR_OP_READ4_FAST;
				break;
			case SPI_NOR_NORMAL:
				nor->read_opcode = SPINOR_OP_READ4;
				break;
			}
			nor->program_opcode = SPINOR_OP_PP_4B;
			/* No small sector erase for 4-byte command set */
			nor->erase_opcode = SPINOR_OP_SE_4B;
			mtd->erasesize = info->sector_size;
		}
	} else {
		nor->addr_width = 3;
	}

	if (nor->addr_width > SPI_NOR_MAX_ADDR_WIDTH) {
		dev_err(dev, "address width is too large: %u\n",
			nor->addr_width);
		return -EINVAL;
	}

	nor->read_dummy = spi_nor_read_dummy_cycles(nor);

	/* Send all the required SPI flash commands to initialize device */
	nor->info = info;
	ret = spi_nor_init(nor);
	if (ret)
		return ret;

	sunxi_lock_init(nor);

	dev_info(dev, "%s (%lld Kbytes)\n", info->name,
			(long long)mtd->size >> 10);

	dev_dbg(dev,
		"mtd .name = %s, .size = 0x%llx (%lldMiB), "
		".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
		mtd->name, (long long)mtd->size, (long long)(mtd->size >> 20),
		mtd->erasesize, mtd->erasesize / 1024, mtd->numeraseregions);

	if (mtd->numeraseregions)
		for (i = 0; i < mtd->numeraseregions; i++)
			dev_dbg(dev,
				"mtd.eraseregions[%d] = { .offset = 0x%llx, "
				".erasesize = 0x%.8x (%uKiB), "
				".numblocks = %d }\n",
				i, (long long)mtd->eraseregions[i].offset,
				mtd->eraseregions[i].erasesize,
				mtd->eraseregions[i].erasesize / 1024,
				mtd->eraseregions[i].numblocks);
	return 0;
}
EXPORT_SYMBOL_GPL(spi_nor_scan);

static const struct flash_info *spi_nor_match_id(const char *name)
{
	const struct flash_info *id = spi_nor_ids;

	while (id->name) {
		if (!strcmp(name, id->name))
			return id;
		id++;
	}
	return NULL;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huang Shijie <shijie8@gmail.com>");
MODULE_AUTHOR("Mike Lavender");
MODULE_DESCRIPTION("framework for SPI NOR");
