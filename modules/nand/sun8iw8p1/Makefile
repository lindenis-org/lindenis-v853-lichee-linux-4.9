obj-$(CONFIG_SUNXI_NAND) += nand.o

libnand = libnand_sun8iw8p1.a

$(obj)/$(libnand) : FORCE
ifneq ($(wildcard $(obj)/lib),)
	$(MAKE) -C $(obj)/.. \
		LICHEE_KDIR="$(shell pwd)" \
		build
endif
	cp $(obj)/$(basename $(libnand)) $@

nand-objs += nand_interface.o \
	nfd/nand_ota_burn.o \
	nfd/boot_head/boot0_op.o \
	nfd/nand_base.o \
	nfd/nand_blk.o \
	nfd/nand_dev.o \
	nfd/nand_class.o \
	nfd/nand_osal_for_linux.o \
	spinand/spinand_ecc_op.o \
	spinand/spinand.o \
	spinand/spinand_ids.o \
	$(libnand)
