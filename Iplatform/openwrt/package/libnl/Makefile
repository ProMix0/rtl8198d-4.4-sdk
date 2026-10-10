#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnl
PKG_RELEASE:=1

ifeq ($(CONFIG_PACKAGE_LIBNL_VERSION), "3.5.0")
PKG_VERSION:=3.5.0
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/thom311/libnl/releases/download/libnl$(subst .,_,$(PKG_VERSION))
PKG_HASH:=9fe43ccbeeea72c653bdcf8c93332583135cda46a79507bfd0a483bb57f65939

PKG_LICENSE:=LGPL-2.1
PKG_LICENSE_FILES:=COPYING
PKG_CPE_ID:=cpe:/a:libnl_project:libnl

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf

PKG_BUILD_FLAGS:=gc-sections
PATCH_DIR:=

include $(INCLUDE_DIR)/package.mk

define Package/libnl/default
  SECTION:=libs
  CATEGORY:=Libraries
  URL:=http://www.infradead.org/~tgr/libnl/
  ABI_VERSION:=200
endef

define Package/libnl-core
$(call Package/libnl/default)
  TITLE:=Core Netlink Library
  DEPENDS:=+libpthread
endef

define Package/libnl-genl
$(call Package/libnl/default)
  TITLE:=Generic Netlink Library
  DEPENDS:=+libnl-core
endef

define Package/libnl-route
$(call Package/libnl/default)
  TITLE:=Routing Netlink Library
  DEPENDS:=+libnl-core
endef

define Package/libnl-nf
$(call Package/libnl/default)
  TITLE:=Netfilter Netlink Library
  DEPENDS:=+libnl-route
endef

define Package/libnl
$(call Package/libnl/default)
  TITLE:=Full Netlink Library
  DEPENDS:=+libnl-genl +libnl-route +libnl-nf
endef

define Package/libnl-core/description
 Common code for all netlink libraries
endef

define Package/libnl-genl/description
 Generic Netlink Library Functions
endef

define Package/libnl-route/description
 Routing Netlink Library Functions
endef

define Package/libnl-nf/description
 Netfilter Netlink Library Functions
endef

define Package/libnl/description
 Socket handling, connection management, sending and receiving of data,
 message construction and parsing, object caching system, etc.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--disable-debug

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libnl3 $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libnl3/* $(1)/usr/include/libnl3/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/*.pc $(1)/usr/lib/pkgconfig

	# Copy symlinks
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-3.so $(1)/usr/lib/libnl.so
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-genl-3.so $(1)/usr/lib/libnl-genl.so
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-nf-3.so $(1)/usr/lib/libnl-nf.so
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-route-3.so $(1)/usr/lib/libnl-route.so
endef

define Package/libnl-core/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-3.so.* $(1)/usr/lib/
endef

define Package/libnl-genl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-genl-3.so.* $(1)/usr/lib/
endef

define Package/libnl-route/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-route-3.so.* $(1)/usr/lib/
endef

define Package/libnl-nf/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-nf-3.so.* $(1)/usr/lib/
endef

define Package/libnl/install
	:
endef

$(eval $(call BuildPackage,libnl-core))
$(eval $(call BuildPackage,libnl-genl))
$(eval $(call BuildPackage,libnl-route))
$(eval $(call BuildPackage,libnl-nf))
$(eval $(call BuildPackage,libnl))
else
PKG_VERSION:=2.0
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.infradead.org/~tgr/libnl/files/
PKG_MD5SUM:=6aaf1e9802a17a7d702bb0638044ffa7

include $(INCLUDE_DIR)/package.mk

define Package/libnl
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=netlink socket library
  URL:=http://people.suug.ch/~tgr/libnl/
endef

define Package/libnl/description
 This package contains a library for applications dealing with netlink sockets
endef

TARGET_CFLAGS += -ffunction-sections $(FPIC)

define Build/Compile
	$(call Build/Compile/Default)
	make -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)
	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
	mkdir -p $(1)/usr/include/libnl
	$(CP) $(PKG_BUILD_DIR)/include/linux $(1)/usr/include/libnl/
endef

define Package/libnl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-route.so.* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-genl.so.* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-nf.so.* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-route.so.* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnl))
endif
