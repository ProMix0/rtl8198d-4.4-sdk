#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libusb-1.0

ifeq ($(CONFIG_PACKAGE_LIBUSB_VERSION),"1.0.24")
PKG_VERSION:=1.0.24
PKG_RELEASE:=$(AUTORELEASE)
else
PKG_VERSION:=1.0.9
PKG_RELEASE:=1
endif

PKG_SOURCE:=libusb-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@SF/libusb

ifeq ($(CONFIG_PACKAGE_LIBUSB_VERSION),"1.0.24")
PKG_MD5SUM:=5BC27DF16155302F308D409E73589872
else
PKG_MD5SUM:=7f5a02375ad960d4e33a6dae7d63cfcb
endif


PKG_BUILD_DIR:=$(BUILD_DIR)/libusb-$(PKG_VERSION)
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libusb-1.0
  SECTION:=TP-LINK
  CATEGORY:=TP-LINK iplatform apps
  TITLE:=A library for accessing Linux USB devices (non-legacy)
  DEPENDS:=+libpthread +librt
  URL:=http://libusb.wiki.sourceforge.net/
endef

define Package/libusb-1.0/description
  libusb is a C library that gives applications easy access to USB devices on
  many different operating systems.
endef

TARGET_CFLAGS += $(FPIC)

ifeq ($(CONFIG_PACKAGE_LIBUSB_VERSION),"1.0.24")
PATCH_DIR:=./patches-1.0.24
CONFIGURE_ARGS += \
	--disable-udev \
	--disable-log

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./patches-1.0.24 $(PKG_BUILD_DIR)
	bzcat $(TOPDIR)/dl/$(PKG_SOURCE) | /bin/tar -C $(BUILD_DIR)/libusb-1.0.24/.. -xf -
	$(call PatchDir,$(PKG_BUILD_DIR),$(PATCH_DIR),)
	echo -------$(CONFIG_PACKAGE_LIBUSB_VERSION)--------

endef
endif

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libusb-1.0
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libusb-1.0/libusb.h \
		$(1)/usr/include/libusb-1.0/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusb*.{a,la,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libusb*.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libusb-1.0/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusb*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libusb-1.0))
