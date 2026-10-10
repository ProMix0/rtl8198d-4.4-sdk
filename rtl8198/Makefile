#
# Lets work out what the user wants, and if they have configured us yet
#

ifeq (.config,$(wildcard .config))
include .config
else
all: config_error
endif

ifeq ($(CONFIG_PRODUCT), luna_ap_mips)
all:  show_toolchain linux backports subdirs romfs rcX modules modules_install
else
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
all:  show_toolchain bootloader_gpt_image packages_ca linux packages_user subdirs romfs rcX modules modules_install image vmimg 
else
all:  show_toolchain linux backports subdirs romfs rcX modules modules_install image vmimg
LINUXTARGET = uImage
endif
endif
ifeq ($(CONFIG_USER_XMLCONFIG), y)
PROVINCE_NAME=province
else
PROVINCE_NAME=province_boa_xml
endif
#############################################
# Do not include linux/.config 
# Linux config has the same option name 
# with u-boot
#############################################
LINUXDIR = $(CONFIG_LINUXDIR)
CA_KMOD = ca_packages
CA_SCFG_CFG = $(CA_KMOD)/ca-scfg/config
3RD_PACKAGES = 3rd_packages
CA_USER = ca_user
LIBNL_3_2_25_DIR = $(CA_USER)/libnl/libnl-3.2.25
CA_SYSROOT = sysroot-destdir
LIBUSB_1_0_21_DIR = lib/libusb-1.0.21/libusb-1.0.21
JSON_C_DIR = user/json-c
EUDEV_3_2_DIR = lib/eudev-3.2/eudev-3.2
###################################
HOST_CPU_NUM = $(shell cat /proc/cpuinfo | grep -c processor)
ROOTDIR  = $(shell pwd)
PATH     := $(PATH):$(ROOTDIR)/tools:$(ROOTDIR)/vendors/Realtek/luna_ap_arm
HOSTCC   = unset GCC_EXEC_PREFIX; cc
IMAGEDIR = $(ROOTDIR)/images
ROMFSDIR = $(ROOTDIR)/romfs
TMPFSDIR = $(ROOTDIR)/tmpfs
PKG_CONFIG_PATH = $(TMPFSDIR)/lib/pkgconfig
ROMFSINST= romfs-inst.sh
KERNEL_SRC = $(ROOTDIR)/$(LINUXDIR)
KERNEL_DIR = $(ROOTDIR)/$(LINUXDIR)
STAGING_KERNEL_BUILDDIR = $(ROOTDIR)/$(LINUXDIR)
LINUX_CONFIG  = $(ROOTDIR)/$(LINUXDIR)/.config
LINUX_DTS_DIR = $(ROOTDIR)/$(LINUXDIR)/arch/arm64/boot/dts/cortina
CONFIG_CONFIG = $(ROOTDIR)/config/.config
MODULES_CONFIG = $(ROOTDIR)/modules/.config
DEFAULT_UBINIZE_CFG_MIPS=$(ROOTDIR)/tools/ubinize-luna-eng.cfg
DEFAULT_UBINIZE_CFG_ARM=$(ROOTDIR)/tools/ubinize-g3-eng_ubi.cfg
BACKPORT_DIR = backports-5.2.8-1
BACKPORT_LOCAL_SYM = local-symbols
BACKPORT_LOCAL_SYM_OLD = local-symbols.old
BACKPORT_LOCAL_SYM_UPDATED = .local-sym-updated
BACKPORT_CONFIG = $(ROOTDIR)/$(BACKPORT_DIR)/.config
BACKPORT_AUTOCONF_H = $(BACKPORT_DIR)/backport-include/backport/autoconf.h
#### DTS PATH ###
DTS_PATH = $(LINUXDIR)/arch/arm64/boot/dts/realtek/

# Follow kernel tree
ifeq ($(CONFIG_LINUXDIR),linux-4.4.x)
KERNEL_VERSION = 4.4.140
CONF =  conf44
else
KERNEL_VERSION = 3.18.20
CONF = conf318
endif
export KERNEL_VERSION

#### BACKPORT MODULE ####
BACKPORT_KERNEL_MODULE_DIR := 
BACKPORT_SUPPORT := $(shell if grep -q 'CONFIG_RTL8852AE_BACKPORTS=y' $(LINUX_CONFIG) ; \
				then echo 1; else echo 0; fi)
WFO_VIRT_SUPPORT := $(shell if grep -q 'CPTCFG_WFO_VIRT=m' $(BACKPORT_CONFIG) ; \
				then echo 1; else echo 0; fi)
THER_CTRL_SUPPORT := $(shell if grep -q 'CPTCFG_THER_CTRL=m' $(BACKPORT_CONFIG) ; \
				then echo 1; else echo 0; fi)

ifeq ($(BACKPORT_SUPPORT), 1)
BACKPORT_KERNEL_MODULE_DIR += backports-5.2.8-1/
BACKPORT_KERNEL_MODULE_DIR += linux-4.4.x/drivers/net/wireless/realtek/rtl8821cu/
BACKPORT_KERNEL_MODULE_DIR += linux-4.4.x/drivers/net/wireless/realtek/rtl8723fu/
endif
ifeq ($(THER_CTRL_SUPPORT), 1)
BACKPORT_KERNEL_MODULE_DIR += linux-4.4.x/drivers/net/wireless/realtek/ther_ctrl/
endif
ifeq ($(WFO_VIRT_SUPPORT), 1)
BACKPORT_KERNEL_MODULE_DIR += linux-4.4.x/drivers/net/wireless/realtek/wfo_virt/
endif
#######################################################################################
ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
	ifneq ($(wildcard ./vendors/Realtek/luna/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna
#	else ifneq ($(wildcard ./vendors/Realtek/luna_mips/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
#		CONFIG_PRODUCT=luna_mips
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna_ap_mips
	else
		$(error configuration found)
	endif 
endif
ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
	ifneq ($(wildcard ./vendors/Realtek/luna/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_ap_mips
	else
		$(error configuration found)
	endif 
endif

ifeq ($(MAKECMDGOALS:preconfig_%=preconfig),preconfig)
	ifneq ($(wildcard ./vendors/Realtek/luna/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),)
		CONFIG_PRODUCT=luna
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
#	else ifneq ($(wildcard ./vendors/Realtek/luna_mips/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
#		CONFIG_PRODUCT=luna_mips
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
		CONFIG_PRODUCT=luna_ap_mips
	else
		$(error configuration found)
	endif 
endif

ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/conf44/$(MAKECMDGOALS:preconfig44_%=preconfig44)))
else ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
ifeq ($(CONFIG_PRODUCT), luna)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/conf318/$(MAKECMDGOALS:preconfig318_%=preconfig318)))
else ifeq ($(CONFIG_PRODUCT), luna_xdsl)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna_xdsl/conf318/$(MAKECMDGOALS:preconfig318_%=preconfig318)))
endif
else ifeq ($(MAKECMDGOALS:preconfig_%=preconfig),preconfig)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/$(CONF)/$(MAKECMDGOALS:preconfig_%=preconfig)))
endif

ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f , g3lite
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
CA_TARGET = g3lite
else
CA_TARGET = g3
endif
else ##pon , g3
CA_TARGET = g3
endif
export CA_TARGET

ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
LINUX_DTS_DIR = $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/dts/realtek
endif
endif

$(info CONFIG_PRODUCT=$(CONFIG_PRODUCT))

CONFIG_PRODUCT ?= luna
CONFIG_VENDOR ?= Realtek
VENDDIR = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.
DIRS    = lib user2 user 
BUSYBOXDIR_DEST = user/busybox-1.22.1
INSTALL_MOD_PATH = $(ROMFSDIR)
INSTALL_PATH = $(IMAGEDIR)
BOOTLOADER_PATH= $(ROOTDIR)/bootloader

MULTI_LANG_DL ?= 0
export MULTI_LANG_DL

ifeq (config.arch,$(wildcard config.arch))
include config.arch
ARCH_CONFIG = $(ROOTDIR)/config.arch
export ARCH_CONFIG LINUX_DTS_DIR
endif

.EXPORT_ALL_VARIABLES:

############################################################################
#
# Software Version
#
ifdef CONFIG_YUEME
VERSIONPKG = V4.0.0-$(shell date +%y%m%d%H%M)
else
ifdef CONFIG_CU_BASEON_YUEME
VERSIONPKG = V01.01.0
else
VERSIONPKG = V4.0.0-$(shell date +%y%m%d)
endif
endif
VERSIONSTR = $(VERSIONPKG)

ifdef OMCI_VERSION
VERSIONPKG := $(OMCI_VERSION)
else
ifdef CONFIG_JIO
icver:=$(shell cat .config | grep CONFIG_PRD_SIGNATURE | cut -d"=" -f2)
CHIPVENDOR:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep CHIPVENDOR | cut -d"=" -f2)
OT:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep OT | cut -d"=" -f2)
DEVICEMODE:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep DEVICEMODE | cut -d"=" -f2)
RELEASEVERION:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep $(icver) | cut -d"=" -f3)
MODELNAME:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep $(icver) | cut -d"=" -f2)
RELEASETYPE:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep RELEASETYPE | cut -d"=" -f2)
VERSIONPKG := $(CHIPVENDOR)$(OT)$(DEVICEMODE)$(MODELNAME)$(R)$(RELEASETYPE)$(RELEASEVERION)
else
endif # CONFIG_JIO
endif # OMCI_VERSION

############################################################################
#
# Get the ccache
#
ifeq ($(USE_CCACHE),1)
$(info ###### use ccache #####)
CCACHE=ccache
else
$(info ##### NOT use ccache #####)
CCACHE=
endif

USE_PMAKE=1
ifeq ($(USE_PMAKE),1)
PARAL_MAKE=-j $(HOST_CPU_NUM) 
else
PARAL_MAKE=
endif

############################################################################
#
# Get the core stuff worked out
#

MAKEARCH = $(MAKE) ARCH=$(ARCH) CROSS_COMPILE="$(CCACHE) $(CROSS_COMPILE)"
MAKEARCH_KERNEL = $(MAKEARCH)
PATH := $(RSDK)/bin:$(PATH)
SCRIPTSDIR = $(ROOTDIR)/config/scripts
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)
CROSS_HOST ?=mips-linux
CROSS_TARGET ?=mips-linux
export CROSS_HOST CROSS_TARGET  HOST_CPU_NUM RSDK
export MAKEARCH MAKEARCH_KERNEL KERNEL_SRC KERNEL_DIR ROOTDIR STAGING_KERNEL_BUILDDIR LINUXDIR IMAGEDIR
export CROSS_COMPILE SCRIPTSDIR CONFIG_SHELL ROMFSDIR TMPFSDIR VERSIONPKG VERSIONSTR INSTALL_MOD_PATH PKG_CONFIG_PATH
#### Use CONFIG_PRODUCT to judge g3 or g3-lite
export CONFIG_PRODUCT

KDIR=$(ROOTDIR)/$(LINUXDIR)
NETDIR=$(KDIR)/drivers/net/ethernet/realtek

APOLLODIR=$(NETDIR)/rtl86900
SDKDIR=$(APOLLODIR)/sdk
export KDIR APOLLODIR SDKDIR NETDIR

KBUILD_EXTRA_SYMBOLS=$(APOLLODIR)/ca-ne/Module.symvers

ifdef CONFIG_USE_RSDK_WRAPPER
SSTRIP = $(CROSS_COMPILE)sstrip
LSTRIP = $(CROSS_COMPILE)lstrip
LSTRIPPED = $(ROMFSDIR)/.lstripped
endif #CONFIG_USE_RSDK_WRAPPER


UBOOTENVF:=$(shell echo $(CONFIG_UBOOT_ENVFILE) | cut -d "." -f1 )

##### preconfig ########################
ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
		PRECFGDIR  := $(ROOTDIR)/vendors/Realtek/$(CONFIG_PRODUCT)/conf44/$(MAKECMDGOALS:preconfig44_%=%)
endif

ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
		PRECFGDIR  := $(ROOTDIR)/vendors/Realtek/$(CONFIG_PRODUCT)/conf318/$(MAKECMDGOALS:preconfig318_%=%)
endif
export PRECFGDIR

tools_prepare:
	@echo "Host Tools Prepare......"
	@cd tools; sh ./build_tools.sh
	@echo "Host Tools Prepare......Done!"

preconfig_clean_cfg:
	rm -f .UBIFS_ARGSCFG
	rm -f $(ROOTDIR)/user2/proc_var/board_init.sh
preconfig_%: preconfig_clean_cfg
	@if [ -f $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98*-eng.txt ] ; then \
		rm $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98*-eng.txt; \
	fi
	@if [ -d $(VENDDIR)/$(CONF)/$* ] ; then \
		if egrep "^CONFIG_CMCC=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			if [ -f $(VENDDIR)/$(CONF)/$*/backgroup_style.css ]; then \
				cp $(VENDDIR)/$(CONF)/$*/*.css user/boa_e8/src/web_cmcc/style/;\
			fi;\
			touch $(LIBUSB_1_0_21_DIR)/aclocal.m4 $(LIBUSB_1_0_21_DIR)/configure $(LIBUSB_1_0_21_DIR)/Makefile.in;\
			touch $(JSON_C_DIR)/aclocal.m4 $(JSON_C_DIR)/configure;\
			touch $(EUDEV_3_2_DIR)/aclocal.m4 $(EUDEV_3_2_DIR)/configure $(EUDEV_3_2_DIR)/Makefile.in;\
		fi; \
    	if egrep "^CONFIG_CU_BASEON_CMCC=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			if [ -f $(VENDDIR)/$(CONF)/$*/backgroup_style.css ]; then \
				cp $(VENDDIR)/$(CONF)/$*/*.css user/boa_e8/src/web_cu/style/;\
			fi;\
			touch $(LIBUSB_1_0_21_DIR)/aclocal.m4 $(LIBUSB_1_0_21_DIR)/configure $(LIBUSB_1_0_21_DIR)/Makefile.in;\
			touch $(JSON_C_DIR)/aclocal.m4 $(JSON_C_DIR)/configure;\
			touch $(EUDEV_3_2_DIR)/aclocal.m4 $(EUDEV_3_2_DIR)/configure $(EUDEV_3_2_DIR)/Makefile.in;\
		fi; \
		cp $(VENDDIR)/$(CONF)/$*/config .config && \
		cp $(VENDDIR)/$(CONF)/$*/config_kernel $(LINUXDIR)/.config && \
		cp $(VENDDIR)/$(CONF)/$*/config_user $(CONFIG_CONFIG) ; \
		cp $(VENDDIR)/$(CONF)/$*/config_backports-5.2.8-1 backports-5.2.8-1/.config 2>/dev/null || :; \
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_mips" ];then  \
			rm -f config.arch;\
			ln -s $(VENDDIR)/config.arch config.arch;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_arm" ];then  \
			if egrep "^CONFIG_RSDK_DIR=*-a17-EL-*" $(ROOTDIR)/.config > /dev/null; then \
				echo "This is 98F." ; \
				rm -f config.arch;\
				ln -s $(VENDDIR)/config.arch config.arch;\
			else  \
				echo "This is 98XA." ; \
				rm -f config.arch;\
				ln -s $(VENDDIR)/config.arch.arm64 config.arch;\
			fi;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_mips" ];then  \
			FLASH_TYPE=$$(cat .config | grep UBOOT_FLASH_TYPE) ;\
			echo $$FLASH_TYPE;\
			cd $(BOOTLOADER_PATH);\
			rm -f FLASH_TYPE* ;\
			if [ "$$FLASH_TYPE" = "UBOOT_FLASH_TYPE=SPI_NAND" ];then\
				touch FLASH_TYPE_SPI_NAND ;\
			fi;\
			if [ "$$FLASH_TYPE" = "UBOOT_FLASH_TYPE=NOR" ];then\
                                touch FLASH_TYPE_NOR ;\
                        fi;\
			cd - ;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna" ];then  \
			if [ -f  $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi  ] ;then cp $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
			else cp $(LINUX_DTS_DIR)/default/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
			fi ;\
			find $(VENDDIR)/$(CONF)/$*/ -name 'uboot-env-*.txt' -exec cp {} $(BOOTLOADER_PATH)/u-boot-env/  \;; \
			if [ -f $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg ] ; then  \
				ln -sf $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg $(ROOTDIR)/tools/device_ubinize.cfg;  \
			else  \
				if egrep "^CONFIG_MIPS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
					ln -sf $(DEFAULT_UBINIZE_CFG_MIPS) $(ROOTDIR)/tools/device_ubinize.cfg; \
				else \
					ln -sf $(DEFAULT_UBINIZE_CFG_ARM) $(ROOTDIR)/tools/device_ubinize.cfg; \
				fi;\
			fi ; \
		fi ;\
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_arm" ];then  \
			if egrep "^CONFIG_RSDK_DIR=*-a17-EL-*" $(ROOTDIR)/.config > /dev/null; then \
				echo "This is 98F." ; \
			else  \
				echo "This is 98XA." ; \
				if [ -f  $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi  ] ;then cp $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
				else cp $(LINUX_DTS_DIR)/default/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
				fi ;\
				find $(VENDDIR)/$(CONF)/$*/ -name 'uboot-env-*.txt' -exec cp {} $(BOOTLOADER_PATH)/u-boot-env/  \;; \
				if [ -f $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg ] ; then  \
					ln -sf $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg $(ROOTDIR)/tools/device_ubinize.cfg;  \
				else  \
					if egrep "^CONFIG_MIPS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
						ln -sf $(DEFAULT_UBINIZE_CFG_MIPS) $(ROOTDIR)/tools/device_ubinize.cfg; \
					else \
						ln -sf $(DEFAULT_UBINIZE_CFG_ARM) $(ROOTDIR)/tools/device_ubinize.cfg; \
					fi;\
				fi ; \
			fi ;\
		fi ;\
		if [ "$(CONFIG_PRODUCT)" = "luna_xdsl" ];then  \
			if [ -f  $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi  ] ;then cp $(VENDDIR)/$(CONF)/$*/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
			else cp $(LINUX_DTS_DIR)/default/ca77xx-nand.dtsi $(LINUX_DTS_DIR)/ca77xx-nand.dtsi ; \
			fi ;\
			find $(VENDDIR)/$(CONF)/$*/ -name 'uboot-env-*.txt' -exec cp {} $(BOOTLOADER_PATH)/u-boot-env/  \;; \
			if [ -f $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg ] ; then  \
				ln -sf $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg $(ROOTDIR)/tools/device_ubinize.cfg;  \
			else  \
				if egrep "^CONFIG_MIPS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
					ln -sf $(DEFAULT_UBINIZE_CFG_MIPS) $(ROOTDIR)/tools/device_ubinize.cfg; \
				else \
					ln -sf $(DEFAULT_UBINIZE_CFG_ARM) $(ROOTDIR)/tools/device_ubinize.cfg; \
				fi;\
			fi ; \
		fi ;\
		[ -f $(VENDDIR)/$(CONF)/$*/run.sh ] &&  $(VENDDIR)/$(CONF)/$*/run.sh $(VENDDIR)/$(CONF)/$* ; \
		[ -f $(ROOTDIR)/ca_packages/scfg_cfg_cp.sh ] &&  $(ROOTDIR)/ca_packages/scfg_cfg_cp.sh  $(VENDDIR)/$(CONF)/$* ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/rtl8198f-engboard.dts  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/rtl8198f-engboard.dts $(LINUX_DTS_DIR)/rtl8198f-engboard.dts ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/uboot-env-98f-eng.txt  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/uboot-env-98f-eng.txt  $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98f-eng.txt ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/uboot-env-98d-eng.txt  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/uboot-env-98d-eng.txt  $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98d-eng.txt ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/config_busybox_1221 ] && cp $(VENDDIR)/$(CONF)/$*/config_busybox_1221 $(BUSYBOXDIR_DEST)/.config ; \
		if [ -f $(VENDDIR)/$(CONF)/$*/soc.h ]; then \
			cp $(VENDDIR)/$(CONF)/$*/soc.h $(LINUXDIR)/include/soc/soc.h ; \
		fi; \
		if [ -f $(VENDDIR)/$(CONF)/$*/luna_cfg.h ]; then \
			cp $(VENDDIR)/$(CONF)/$*/luna_cfg.h $(LINUXDIR)/include/soc/luna_cfg.h ; \
		else \
			echo '/* dummy file */' > $(LINUXDIR)/include/soc/luna_cfg.h ; \
		fi; \
		[ -f config/config_default.xml ] && mv config/config_default.xml config/config_default.xml.backup; \
		[ -f config/config_default_hs.xml ] && mv config/config_default_hs.xml config/config_default_hs.xml.backup; \
		cp $(VENDDIR)/$(CONF)/$*/config_default.xml config/config_default.xml ; \
		cp $(VENDDIR)/$(CONF)/$*/config_default_hs.xml config/config_default_hs.xml ; \
		if egrep "^CONFIG_E8B=y" $(LINUXDIR)/.config > /dev/null; then \
                [ -d config/province.bak ] && rm -rf config/province.bak; \
                [ -d config/province ] && mv config/province config/province.bak; \
				if egrep "^CONFIG_CMCC=y" $(LINUXDIR)/.config > /dev/null; then \
					if egrep "^CONFIG_USER_CONF_ON_XMLFILE=y" $(ROOTDIR)/config/.config > /dev/null; then \
						if [ -d $(VENDDIR)/$(CONF)/province_cmcc_xml ]; then \
							mkdir -p config/province ;\
							cp -raf $(VENDDIR)/$(CONF)/province_cmcc_xml/* config/province/. ;\
						fi;\
					else \
						if [ -d $(VENDDIR)/$(CONF)/province_cmcc ]; then \
							mkdir -p config/province ;\
							cp -raf $(VENDDIR)/$(CONF)/province_cmcc/* config/province/. ;\
						fi;\
					fi;\
				else \
					if egrep "^CONFIG_USER_CONF_ON_XMLFILE=y" $(ROOTDIR)/config/.config > /dev/null; then \
						if egrep "^CONFIG_CU=y" $(LINUXDIR)/.config > /dev/null; then \
							if [ -d $(VENDDIR)/$(CONF)/province_cu_xml ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/$(CONF)/province_cu_xml/* config/province/. ;\
							fi;\
						else \
							if [ -d $(VENDDIR)/$(CONF)/$(PROVINCE_NAME) ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/$(CONF)/$(PROVINCE_NAME)/* config/province/. ;\
							fi;\
						fi;\
					else \
        					if egrep "^CONFIG_CU=y" $(LINUXDIR)/.config > /dev/null; then \
							if [ -d $(VENDDIR)/$(CONF)/province_cu ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/$(CONF)/province_cu/* config/province/. ;\
							fi;\
						else \
							if [ -d $(VENDDIR)/$(CONF)/$(PROVINCE_NAME) ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/$(CONF)/$(PROVINCE_NAME)/* config/province/. ;\
							fi;\
						fi;\
        				fi;\
                		fi;\
		fi; \
		[ -f $(BOOTLOADER_PATH)/Flash_and_DDR_Support.txt ] && rm  $(BOOTLOADER_PATH)/Flash_and_DDR_Support.txt;  \
		[ -f $(VENDDIR)/$(CONF)/$*/Flash_and_DDR_Support.txt ] && cp  $(VENDDIR)/$(CONF)/$*/Flash_and_DDR_Support.txt  $(BOOTLOADER_PATH)/ ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/config_ca_ne  ]  && cp $(VENDDIR)/$(CONF)/$*/config_ca_ne ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/configs/g3lite_defconfig ; \
		if [ "$(LINUXDIR)" = "linux-4.4.x" ];then  \
			echo "Configuration44 $* loaded." ; \
			touch preconfig44_$*_loaded ; \
		else \
			echo "Configuration318 $* loaded." ; \
			touch preconfig318_$*_loaded ; \
		fi;\
	else \
		echo "No $(CONFIG_PRODUCT) configuration found" ; \
	fi
	@# FIXME Workaround for those who can not access VOIP
	@if [ ! -d $(ROOTDIR)/rtk_voip/kernel ]; then \
		mkdir -p $(ROOTDIR)/rtk_voip/kernel;	\
		touch $(ROOTDIR)/rtk_voip/kernel/Kconfig; \
		mkdir -p $(ROOTDIR)/rtk_voip/aipc_char;	\
		touch $(ROOTDIR)/rtk_voip/aipc_char/Kconfig; \
	fi
	@# Checkout src for 98D
###	@$(MAKE) -f Makefile.98D preconfig44_$*
	@if egrep "^CONFIG_PRODUCT=luna_ap*" $(ROOTDIR)/.config > /dev/null; then \
		if [ -d $(KDIR)/drivers/net/wireless/realtek/rtl8192cd_3.4 ]; then \
			mv $(KDIR)/drivers/net/wireless/realtek/rtl8192cd $(KDIR)/drivers/net/wireless/realtek/rtl8192cd-`date +%Y%m%d_%H%M%S`; \
			ln -sf $(KDIR)/drivers/net/wireless/realtek/rtl8192cd_3.4 $(KDIR)/drivers/net/wireless/realtek/rtl8192cd; \
		fi; \
		if [ ! -d $(KDIR)/drivers/net/wireless/rtl8192cd ]; then \
			mkdir -p $(KDIR)/drivers/net/wireless/rtl8192cd; \
			ln -sf $(KDIR)/drivers/net/wireless/realtek/rtl8192cd/Kconfig-usb $(KDIR)/drivers/net/wireless/rtl8192cd/Kconfig-usb; \
		fi; \
	fi;


preconfig44_%: 
	echo "CONFIG_LINUXDIR=linux-4.4.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	make preconfig_$*;

preconfig318_%: 
	echo "CONFIG_LINUXDIR=linux-3.18.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	make preconfig_$*;


### Build Package Directiory #####################
###### Linux #######
linux_dtbs:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) dtbs || exit 1
	$(MAKE) -f Makefile.image dtbimg

backports_autoconf:
	@. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		if [ -d $(BACKPORT_DIR) ]; then \
			unset CC && \
			if [ ! -f $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_UPDATED) ]; then \
				echo -n "Update backports local-symbols..."; \
				cp $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM) $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_OLD); \
				echo "done."; \
			fi && \
			[ -d $(ROOTDIR)/include/rtk ] || mkdir -p $(ROOTDIR)/include/rtk && \
			[ -d $(ROOTDIR)/include/backports ] || mkdir -p $(ROOTDIR)/include/backports && \
			ln -fs $(ROOTDIR)/$(BACKPORT_DIR)/backport-include/backport/autoconf.h include/rtk/backport_autoconf.h && \
			ln -fs $(ROOTDIR)/$(BACKPORT_DIR)/backport-include/backport/autoconf.h include/backports/autoconf.h && \
			make backport-include/backport/autoconf.h -C $(BACKPORT_DIR) -j $(HOST_CPU_NUM) || exit $$? && \
			[ -f $(FW_IMG) ] && \
			cp $(FW_IMG) user/hostapd-2.9/hostapd/conf/rtl8852ae/ && cp $(FW_IMG) user/hostapd/conf/rtl8852ae/ || : && \
			find $(BACKPORT_KERNEL_MODULE_DIR) -name "*.ko" -exec cp {} $(ROOTDIR)/user/hostapd-2.9/hostapd/conf \; && \
			find $(BACKPORT_KERNEL_MODULE_DIR) -name "*.ko" -exec cp {} $(ROOTDIR)/user/hostapd/conf \;;\
		fi \
	else \
		echo "CONFIG_RTL8852AE_BACKPORTS not set in linux.config, skip backports" ;\
	fi

backports:
	@. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		if [ -d $(BACKPORT_DIR) ]; then \
			unset CC && \
			if [ ! -f $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_UPDATED) ]; then \
				echo -n "Update backports local-symbols..."; \
				cp $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM) $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_OLD); \
				echo "done."; \
			fi && \
			[ -d $(ROOTDIR)/include/rtk ] || mkdir -p $(ROOTDIR)/include/rtk && \
			[ -d $(ROOTDIR)/include/backports ] || mkdir -p $(ROOTDIR)/include/backports && \
			ln -fs $(ROOTDIR)/$(BACKPORT_DIR)/backport-include/backport/autoconf.h include/rtk/backport_autoconf.h && \
			ln -fs $(ROOTDIR)/$(BACKPORT_DIR)/backport-include/backport/autoconf.h include/backports/autoconf.h && \
			make -C $(BACKPORT_DIR) -j $(HOST_CPU_NUM) || exit $$? && \
			[ -f $(FW_IMG) ] && \
			cp $(FW_IMG) user/hostapd-2.9/hostapd/conf/rtl8852ae/ && cp $(FW_IMG) user/hostapd/conf/rtl8852ae/ || : && \
			find $(BACKPORT_KERNEL_MODULE_DIR) -name "*.ko" -exec cp {} $(ROOTDIR)/user/hostapd-2.9/hostapd/conf \; && \
			find $(BACKPORT_KERNEL_MODULE_DIR) -name "*.ko" -exec cp {} $(ROOTDIR)/user/hostapd/conf \;;\
		fi \
	else \
		echo "CONFIG_RTL8852AE_BACKPORTS not set in linux.config, skip backports" ;\
	fi

.PHONY: linux
linux linux_only: backports_autoconf
	[ -d $(ROMFSDIR) ] || mkdir -p $(ROMFSDIR)
	[ -d $(TMPFSDIR) ] || mkdir -p $(TMPFSDIR)
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) all $(LINUXTARGET) || exit 1
	[ -d $(ROOTDIR)/include ] || mkdir -p $(ROOTDIR)/include
	[ -d $(ROOTDIR)/include/linux ] || mkdir -p $(ROOTDIR)/include/linux
	ln -fs $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h include/linux/autoconf.h
	ln -fs $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h include/linux/config.h
	 [ -d $(ROOTDIR)/$(LINUXDIR)/user_headers ] || $(MAKEARCH_KERNEL) -C $(LINUXDIR) INSTALL_HDR_PATH=$(ROOTDIR)/$(LINUXDIR)/user_headers headers_install

.PHONY: voip_prepare
voip_prepare:
	@if [ -d $(ROOTDIR)/rtk_voip ] ; then \
		cd $(LINUXDIR)/drivers/char ; ln -fsn ../../../rtk_voip/aipc_char  aipc     ; cd - ; \
		cd $(LINUXDIR)              ; ln -fsn ../rtk_voip/kernel           rtk_voip ; cd - ; \
		cd $(ROOTDIR)/user          ; ln -fsn ../rtk_voip/aipc_util        aipc_util; cd - ; \
	else \
		cd $(LINUXDIR)/drivers/char ; ln -fsn ../../dummy aipc ; \
		cd $(LINUXDIR)              ; ln -fsn dummy rtk_voip   ; \
	fi


linux_clean:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) clean

linux_distclean:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) distclean

.PHONY: linux_install

	
linux_menuconfig: voip_prepare
	$(MAKEARCH_KERNEL) -C $(LINUXDIR) menuconfig
	$(MAKEARCH_KERNEL) -C $(LINUXDIR) modules_prepare
###### CA_PACKAGES #######
packages_ca:
	$(MAKEARCH) -C $(CA_KMOD) all

%_ca:
		if [ -d "$(@:_ca=)" ]; then \
			if [ -n "$(findstring ca_packages/,$@)" ]; then \
				$(MAKEARCH) -C ca_packages $(@:ca_packages/%=%); \
			else \
				$(MAKEARCH) -C $(@:_ca=); \
				fi; \
		fi	
	
	
packages_ca_clean:
	$(MAKEARCH) -C $(CA_KMOD) clean

packages_ca_install:
	$(MAKEARCH) -C $(CA_KMOD) modules_install

packages_user: tools_prepare
	touch $(LIBNL_3_2_25_DIR)/configure.ac $(LIBNL_3_2_25_DIR)/aclocal.m4 $(LIBNL_3_2_25_DIR)/configure $(LIBNL_3_2_25_DIR)/Makefile.am $(LIBNL_3_2_25_DIR)/Makefile.in
	$(MAKEARCH) -C $(CA_USER) all

packages_user_clean:
	$(MAKEARCH) -C $(CA_USER) clean

packages_user_kmod_install:
	$(MAKEARCH) -C $(CA_USER) modules_install

##sysroot-destdir: CA's libs, headers , binaries.....
sysroot_install:
	rm -rf ./sysroot-destdir/home/* 
	[ ! -d ./sysroot-destdir ]		|| cp -arf ./sysroot-destdir/*   $(ROMFSDIR)
	[ ! -d $(ROMFSDIR)/usr/lib ]		|| chmod -R 755  $(ROMFSDIR)/usr/lib
	[ ! -d $(ROMFSDIR)/usr/lib ]		|| rm -rf  $(ROMFSDIR)/usr/lib/*.a
	[ ! -d $(ROMFSDIR)/usr/include ]	|| rm -rf $(ROMFSDIR)/usr/include/*
	[ ! -d $(ROMFSDIR)/usr/share/doc ]	|| rm -rf $(ROMFSDIR)/usr/share/doc
	[ ! -d $(ROMFSDIR)/usr/share/man ]	|| rm -rf $(ROMFSDIR)/usr/share/man/*
	[ ! -d $(ROMFSDIR)/usr/share/terminfo ]	|| rm -rf $(ROMFSDIR)/usr/share/terminfo
	cd $(ROMFSDIR) && rm -rf config/ && ln -fsn ./var/config/ config; 

################## Menuconfig	  #######
############################################################################

#
# Config stuff,  we recall ourselves to load the new config.arch before
# running the kernel and other config scripts
#

.PHONY: config.tk config.in

config.in:
	config/mkconfig > config.in

config.tk: config.in
	$(MAKE) -C $(SCRIPTSDIR) tkparse
	ARCH=dummy $(SCRIPTSDIR)/tkparse < config.in > config.tmp
	@if [ -f /usr/local/bin/wish ];	then \
		echo '#!'"/usr/local/bin/wish -f" > config.tk; \
	else \
		echo '#!'"/usr/bin/wish -f" > config.tk; \
	fi
	cat $(SCRIPTSDIR)/header.tk >> ./config.tk
	cat config.tmp >> config.tk
	rm -f config.tmp
	echo "set defaults \"/dev/null\"" >> config.tk
	echo "set help_file \"config/Configure.help\"" >> config.tk
	cat $(SCRIPTSDIR)/tail.tk >> config.tk
	chmod 755 config.tk

.PHONY: xconfig
xconfig: config.tk
	@wish -f config.tk
	@if [ ! -f .config ]; then \
		echo; \
		echo "You have not saved your config, please re-run make config"; \
		echo; \
		exit 1; \
	 fi
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_xconfig; \
	 fi
	@config/setconfig final

.PHONY: config
config: config.in
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Configure config.in
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_config; \
	 fi
	@config/setconfig final



##### menuconfig
config_bootloader:
	@config/setBootloaderConf NewConf

config_menuconfig:
#	$(MAKE) -C config menuconfig	
	$(MAKE) -f Makefile.sub_menuconf menuconfig_vendor

.PHONY: menuconfig
menuconfig: 
	$(MAKE) menuconfig_phase1
	$(MAKE) menuconfig_phase2
	$(MAKE) menuconfig_phase3
	$(MAKE) menuconfig_phase4
	@config/setconfig final

.PHONY: menuconfig_phase1
menuconfig_phase1: config.in voip_prepare
	$(MAKE) -C $(SCRIPTSDIR)/lxdialog all
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Menuconfig config.in
	@if [ ! -f .config ]; then \
		echo; \
		echo "You have not saved your config, please re-run make config"; \
		echo; \
		exit 1; \
	 fi
	if egrep "CONFIG_RTL8852AE_BACKPORTS=y" $(shell pwd)/linux-4.4.x/.config > /dev/null; then \
		sed -i "s/.*source.*rtl8192cd\/Kconfig\"/\#source \"drivers\/net\/wireless\/realtek\/rtl8192cd\/Kconfig\"/g" $(shell pwd)/linux-4.4.x/drivers/net/wireless/Kconfig; \
		else \
		sed -i "s/.*source.*rtl8192cd\/Kconfig\"/source \"drivers\/net\/wireless\/realtek\/rtl8192cd\/Kconfig\"/g" $(shell pwd)/linux-4.4.x/drivers/net/wireless/Kconfig; \
	fi
	@config/setconfig defaults
ifeq ($(CONFIG_PRODUCT), luna)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	@config/setBootloaderConf NewConf
	@config/setBootloaderConf UBIFSConf
endif
endif
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	@config/setBootloaderConf NewConf
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do menuconfig_phase1 - 98F.";
	@config/setBootloaderConf NewConf
else
	echo "do menuconfig_phase1 - 98XA.";
	@config/setBootloaderConf NewConf
	@config/setBootloaderConf UBIFSConf
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_mips)  ##98D
	@config/setBootloaderConf NewConf
endif

.PHONY: menuconfig_phase2
menuconfig_phase2: 
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_menuconfig; \
	fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_menuconfig; \
	fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) -f Makefile.sub_menuconf menuconfig_vendor; \
	fi	

	
.PHONY: menuconfig_phase3

menuconfig_phase3:
	@if egrep "^CONFIG_MCONF_BZBOX=y" .config > /dev/null; then \
		$(MAKE) -f Makefile.sub_menuconf menuconfig_busybox; \
	fi
	@if egrep "^CONFIG_USE_PRELOADER_PARAMETERS=y" .config > /dev/null; then \
		$(MAKE) soc_h_prepare; \
	fi

.PHONY: menuconfig_phase4

menuconfig_phase4:
	@. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		if [ -d $(BACKPORT_DIR) ]; then \
			unset CC \
		fi \
	fi

bootloader_menuconfig:
	$(MAKE) -C $(BOOTLOADER_PATH) menuconfig

bootloader_clean:
	$(MAKE) -C $(BOOTLOADER_PATH) clean

###########################################################################	
### Build

bootloader_gpt_image bootloader_image:
	$(MAKE) -C $(BOOTLOADER_PATH) all

subdirs:
	for dir in $(DIRS) ; do $(MAKEARCH) $(PARAL_MAKE) -C $$dir || exit 1 ; done

.PHONY: modules
modules:
	. $(LINUX_CONFIG); if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(LINUXDIR)/modules ] || mkdir $(LINUXDIR)/modules; \
		$(MAKEARCH_KERNEL) -C $(LINUXDIR) modules; \
	fi

.PHONY: modules_install
modules_install:
	. $(LINUX_CONFIG); if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(ROMFSDIR)/lib/modules ] || mkdir -p $(ROMFSDIR)/lib/modules; \
		$(MAKEARCH_KERNEL) -C $(LINUXDIR) INSTALL_MOD_PATH=$(ROMFSDIR) modules_install INSTALL_MOD_STRIP=1; \
		rm -f $(ROMFSDIR)/lib/modules/*/build; \
	fi
#ifneq ($(CONFIG_PRODUCT), luna_ap_mips)
	.  $(LINUX_CONFIG); \
	$(MAKEARCH_KERNEL)  packages_ca_install; \
	$(MAKEARCH_KERNEL)  packages_user_kmod_install; \
	if [[ "$$CONFIG_CMCC" = "y" || "$$CONFIG_CU_BASEON_CMCC" = "y" ]]; then \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/mac_filter modules_install; \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/flow_counter modules_install; \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/traffic_monitor modules_install; \
	fi
#endif

.PHONY: rcX
rcX: rcX_clean
	for dir in $(VENDDIR) user user2 ; do \
		if [ -d $$dir ]; then \
		if [ "`grep "^rcX:" $$dir/Makefile`" != "" ]; then \
			$(MAKEARCH) -C $$dir rcX || exit 1 ; \
		fi \
		fi \
	done		
	for rcfile in $(ROMFSDIR)/etc/init.d/rc[0-9]*; do \
		if [ -f $$rcfile ]; then \
			chmod -f 755 $$rcfile; \
		fi \
	done

.PHONY: rcX_clean
rcX_clean:
	rm -rf $(ROMFSDIR)/etc/init.d/rc[0-9]*


############################################################################
#
# normal make targets
#

.PHONY: romfs
romfs:
	$(MAKEARCH) -C $(VENDDIR) mkromfsdir
	for dir in $(DIRS) ; do $(MAKEARCH) -C $$dir romfs STRIP=echo || exit 1 ; done
	-find $(ROMFSDIR)/. -name CVS | xargs -r rm -rf
	-find $(ROMFSDIR)/. -name .svn -type d | xargs -r rm -rf
ifdef CONFIG_USE_RSDK_WRAPPER
	cp -R $(RSDK)/lib/*.so $(ROMFSDIR)/lib
	cp -R $(RSDK)/lib/*.so.* $(ROMFSDIR)/lib
endif
	if [ "$(shell ls -A $(ROOTDIR)/lib/librtk/*.so)" ]; then \
		cp -R $(ROOTDIR)/lib/librtk/*.so $(ROMFSDIR)/lib; \
	fi
	if [ "$(shell ls -A $(ROOTDIR)/lib/librtk/*.so.*)" ]; then \
		cp -R $(ROOTDIR)/lib/librtk/*.so.* $(ROMFSDIR)/lib; \
	fi
	chmod 755 $(ROMFSDIR)/lib/*.so
	rm -f $(LSTRIPPED)

	if [ -d luna_patch ]; then \
		cp -a luna_patch $(ROMFSDIR) || exit 1 ; \
	fi	
	make sysroot_install
	
	# release_date                                                                                                                                                                                                                                                        
	date +"%Y-%m-%d %H:%M:%S" > release_date
	cp release_date $(ROMFSDIR)/etc

	# GTB ARK
	cp -r gtb $(ROMFSDIR)/etc/
	chmod +x $(ROMFSDIR)/etc/gtb/*.sh

	# clean used files
	rm -rf $(ROMFSDIR)/sbin

#8192FE  hostadp
	if [ -d ./vendors/Realtek/luna_ap_mips/conf44/V074G_98D_8832A_8192FE_8211FS ]; then \
		cp ./vendors/Realtek/luna_ap_mips/conf44/V074G_98D_8832A_8192FE_8211FS/92fehapdconf	$(ROMFSDIR)/etc/conf; \
		cp ./vendors/Realtek/luna_ap_mips/conf44/V074G_98D_8832A_8192FE_8211FS/92fe_start_hapd.sh $(ROMFSDIR)/etc/conf; \
	fi
#### luna image ########################################################	
progimages_fullcombined:
ifeq ($(CONFIG_PRODUCT), luna)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	make -f Makefile.image progimages_fullcombined
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do progimages_fullcombined - 98F.";
else
	echo "do progimages_fullcombined - 98XA.";
	make -f Makefile.image progimages_fullcombined
endif
endif

progimages_loader:
ifeq ($(CONFIG_PRODUCT), luna)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	make -f Makefile.image progimages_loader
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do progimages_loader - 98F.";
else
	echo "do progimages_loader - 98XA.";
	make -f Makefile.image progimages_loader
endif
endif

image:
ifeq ($(CONFIG_PRODUCT), luna)
	make -f Makefile.image image
else
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
	make -f Makefile.image xdsl_image
else
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do image - 98F.";
	make -f Makefile.image ap_image
else
	echo "do image - 98XA.";
	make -f Makefile.image image
endif
else
	make -f Makefile.image ap_image
endif
endif
endif


vmimg:
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
	make -f Makefile.image xdsl_vmimg
else
ifeq ($(CONFIG_PRODUCT), luna)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	make bimage;
endif
	make -f Makefile.image vmimg
else
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do vmimg - 98F.";
	make -f Makefile.image ap_vmimg
	make show_toolchain
else
	echo "do vmimg - 98XA.";
	make bimage;
	make -f Makefile.image vmimg
endif
else
	$(MAKEARCH) -j $(HOST_CPU_NUM) -f Makefile.image ap_vmimg
	make show_toolchain
endif
endif
endif

bimage:
	make -f Makefile.image bimage

### $(PRODUCT98F) ###########################################################
## Change Makefile.image to do detail process
98f_image:
	make -f Makefile.image 98f_image

98f_vmimg:
	make -f Makefile.image 98f_vmimg

###################################################################	
%_only:
	if [ -d "$(@:_only=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        if [ ! -f $(@:_only=)/Makefile ]; then \
                                cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
                        fi; \
                        $(MAKEARCH) -C $(@:_only=); \
                fi; \
        fi


%_romfs:
	if [ -d "$(@:_romfs=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        $(MAKEARCH) -C $(@:_romfs=) romfs; \
                fi; \
        fi

%_clean:
	if [ -d "$(@:_clean=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        $(MAKEARCH) -C $(@:_clean=) clean; \
                fi; \
        fi
############################################
## clean section

kernel_aipc_clean:
	@[ ! -d $(ROOTDIR)/rtk_voip/aipc_char ] || find $(ROOTDIR)/rtk_voip/aipc_char \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name 'Module.markers' -o -name '.tmp_*.o.*' \) \
		-exec rm -f {} \;

kernel_voip_clean:
	@[ ! -d $(ROOTDIR)/rtk_voip/kernel ] || find $(ROOTDIR)/rtk_voip/kernel \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name 'Module.markers' -o -name '.tmp_*.o.*' \) \
		-exec rm -f {} \;
	
modules_clean:
	[ ! -d modules ] || $(MAKEARCH) -C modules clean
	
clean: modules_clean
	for dir in $(DIRS); do $(MAKEARCH) -C $$dir clean ; done
	rm -rf $(ROMFSDIR)/*
	rm -rf $(IMAGEDIR)/*
	rm -rf $(LINUXDIR)/net/ipsec/alg/libaes $(LINUXDIR)/net/ipsec/alg/perlasm
	rm -rf $(ROOTDIR)/include
	make -C config/scripts/lxdialog/ clean
	$(MAKEARCH) -C $(CA_KMOD) clean
	$(MAKEARCH) -C $(CA_USER) clean
	make -C $(LINUXDIR) clean
	make kernel_aipc_clean kernel_voip_clean
	rm -rf $(CA_SYSROOT)
	$(MAKE) -C $(BOOTLOADER_PATH) clean
	if [ -f $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_UPDATED) ]; then \
		cd $(BACKPORT_DIR) ; mv $(BACKPORT_LOCAL_SYM_OLD) $(BACKPORT_LOCAL_SYM); rm $(BACKPORT_LOCAL_SYM_UPDATED) -f; \
	fi
	if [ -f $(BACKPORT_AUTOCONF_H) ];then \
		rm -f $(BACKPORT_AUTOCONF_H); \
	fi
	if [ -d $(TMPFSDIR) ]; then \
		rm -rf $(TMPFSDIR); \
	fi

configure_user2:
	make -C user2 configure

sdk.shell_tree:
	$(MAKE) -C user/diagshell shell_tree

user_prepare:
	$(MAKE) -C user prepare

user_voip_prepare:
	$(MAKE) -C user voip_prepare

show_toolchain:
	@echo "Toolchain: $(shell readlink -f $(CONFIG_RSDK_DIR))"

