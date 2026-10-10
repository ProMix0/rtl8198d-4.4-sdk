include $(TOPDIR)/rules.mk

PKG_NAME:=librpc
PKG_VERSION:=2015-11-04
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://nbd.name/uclibc-rpc.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=a921e3ded051746f9f7cd5e5a312fb6771716aac
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
CMAKE_INSTALL:=1
PKG_USE_MIPS16:=0

PKG_LICENSE:=LGPL-2.1
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Felix Fietkau <nbd@openwrt.org>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

ifeq ($(CONFIG_TOOLCHAIN_GCC_VERSION),"10.3")
	TARGET_CFLAGS += -DCONFIG_GCC_VERSION_10_3=y
endif

define Package/librpc
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=uClibc RPC library
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/librpc/install
	$(INSTALL_DIR) $(1)/lib/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/librpc.so $(1)/lib/
endef

$(eval $(call BuildPackage,librpc))
