# Realtek Semiconductor Corp.
#
# bsp/Makefile
#     bsp Makefile
#
# Copyright (C) 2006-2015 Tony Wu (tonywu@realtek.com)
#

core-y   += arch/mips/rtl8198d/
cflags-y += -I$(srctree)/arch/mips/rtl8198d/
load-y   += 0x80010000

obj-y := bspgpio.o setup.o irq.o prom.o timer.o promlib.o dying_gasp.o
obj-y += init_l2cache.o
obj-$(CONFIG_RTL9603CVD_SERIES) += 9603cvd_scache.o 9603cvd_scache_init.o 9603cvd_usb.o 9603cvd_pci.o
obj-$(CONFIG_OF)  += of.o
#obj-$(CONFIG_PCI) += pci.o pci-fixup.o pci-ops.o
obj-$(CONFIG_PCI) += pci.o
obj-$(CONFIG_USB) += usb.o
obj-$(CONFIG_SMP) += smp.o
obj-$(CONFIG_EARLY_PRINTK) += early-printk.o
obj-$(CONFIG_MTD_SPI)	   += flash.o
obj-$(CONFIG_CPU_HAS_WB) += wb_flush.o
obj-$(CONFIG_SOC_TIMER) += cevt-ext.o
obj-$(CONFIG_LUNA_CMU) += rtk-cmu.o
CFLAGS_usb.o += -I$(APOLLODIR)/sdk/include/ -I$(APOLLODIR)/sdk/system/include
CFLAGS_pci.o += -I$(APOLLODIR)/sdk/include/ -I$(APOLLODIR)/sdk/system/include

#obj-$(CONFIG_LUNA_WDT) += luna_wdt.o
obj-$(CONFIG_LUNA_WDT_KTHREAD) += luna_watchdog.o

##
## uncomment following lines for customized linker script
##
#CPPFLAGS_vmlinux.lds := -DTEXT_OFFSET=$(TEXT_OFFSET)
#export KBUILD_LDS := arch/arm64/bsp/vmlinux.lds
#extra-y += vmlinux.lds
