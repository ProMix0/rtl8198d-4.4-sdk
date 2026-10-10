# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=stubby
PKG_VERSION:=0.4.0
PKG_RELEASE:=1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/getdnsapi/$(PKG_NAME)
PKG_SOURCE_VERSION:=v$(PKG_VERSION)
PKG_MIRROR_HASH:=bc5f604da1b70287a6c3d89eac2e13ce8bca52840e7b72ab098a3deeb9935082

PKG_MAINTAINER:=Jonathan Underwood <jonathan.underwood@gmail.com>
PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=COPYING

CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/stubby/Default
  TITLE:=stubby
  URL:=https://dnsprivacy.org/wiki/display/DP/DNS+Privacy+Daemon+-+Stubby
endef

define Package/stubby
  $(call Package/stubby/Default)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=IP Addresses and Names
  TITLE+= - (daemon that uses getdns)
  USERID:=stubby=410:stubby=410
  DEPENDS:= +libyaml +getdns
endef

define Package/stubby/description
  This package contains the Stubby daemon (which utilizes the getdns library).
  See https://github.com/openwrt/packages/blob/master/net/stubby/files/README.md for more details.
endef

define Package/stubby/conffiles
/etc/stubby/stubby.yml
endef

define Package/stubby/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/stubby $(1)/usr/sbin/stubby
	$(INSTALL_BIN) ./filesystem/usr/sbin/detect_dot_server $(1)/usr/sbin/detect_dot_server
	$(INSTALL_BIN) ./filesystem/usr/sbin/dot_server_check $(1)/usr/sbin/dot_server_check
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN)  ./filesystem/etc/init.d/stubby $(1)/etc/init.d/stubby
	$(INSTALL_DIR) $(1)/etc/stubby
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/stubby/stubby.yml $(1)/etc/stubby/stubby.yml.default
	$(INSTALL_DATA) ./filesystem/etc/stubby/stubby.yml $(1)/etc/stubby/stubby.yml
	$(INSTALL_DATA) ./filesystem/etc/stubby/custom.yml $(1)/etc/stubby/custom.yml
	$(INSTALL_DIR) $(1)/etc/config
endef

$(eval $(call BuildPackage,stubby))