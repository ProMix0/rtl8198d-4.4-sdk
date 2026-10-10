#
# Copyright (C) 2023 Shenzhen TP-LINK Technologies Co.Ltd.
#
# Author  : <zhuhaijun@tp-link.com.hk>
# Version : 0.1
# Date    : 16 May, 2023
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=ovpn-dco
PKG_RELEASE:=1.0
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

define KernelPackage/ovpn-dco
  SUBMENU:=Network Support
  TITLE:=OpenVPN data channel offload
  DEPENDS:=+kmod-crypto-aead +kmod-udptunnel4 +kmod-udptunnel6
  FILES:=$(PKG_BUILD_DIR)/drivers/net/ovpn-dco/ovpn-dco-v2.ko
  AUTOLOAD:=$(call AutoLoad,80,ovpn-dco-v2)
  KCONFIG:=
endef

define KernelPackage/ovpn-dco/description
  This module enhances the performance of the OpenVPN userspace software
  by offloading the data channel processing to kernelspace.
endef

define KernelPackage/ovpn-dco/config
  if PACKAGE_kmod-ovpn-dco

	config OVPN_DCO_DEBUG
		bool "Enable debugging"
		default n
		help
		  To be used for development and debugging, enabled additional, but
		  noisy, debugging messages.
  endif
endef

NOSTDINC_FLAGS += \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(PKG_BUILD_DIR)/drivers/net/ovpn-dco \
	-I$(PKG_BUILD_DIR)/include \
	-include $(PKG_BUILD_DIR)/linux-compat.h

EXTRA_KCONFIG:= \
	CONFIG_OVPN_DCO_V2=m
PKG_EXTRA_CFLAGS:= $(if $(CONFIG_OVPN_DCO_DEBUG),-DDEBUG=1,)

ifneq (,$(findstring ipq50xx,$(BOARD)))
	PKG_EXTRA_CFLAGS += -DQCA_IPQ50XX
endif

ifneq (,$(findstring mtk798x,$(BOARD)))
	PKG_EXTRA_CFLAGS += -DMTK_798X
	PKG_EXTRA_CFLAGS += -DOVPN_CRYPTO_USING_THREE_CORES
endif

ifneq (,$(findstring ipq95xx,$(BOARD)))
	PKG_EXTRA_CFLAGS += -DQCA_IPQ95XX
	PKG_EXTRA_CFLAGS += -DOVPN_CRYPTO_USING_THREE_CORES
endif

ifneq (,$(findstring rtl8198,$(BOARD)))
	PKG_EXTRA_CFLAGS += -DOVPN_NO_KERNEL_COMPAT
	PKG_EXTRA_CFLAGS += -DOVPN_MIPS_ARCH
	PKG_EXTRA_CFLAGS += -DOVPN_CHECKSUM_NONE
	PKG_EXTRA_CFLAGS += -DRTL_8198
	PKG_EXTRA_CFLAGS += -DOVPN_CRYPTO_USING_THREE_CORES
endif

PKG_EXTMOD_SUBDIRS = drivers/net/ovpn-dco

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(KERNEL_CROSS)" \
	M="$(PKG_BUILD_DIR)/drivers/net/ovpn-dco" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,ovpn-dco))