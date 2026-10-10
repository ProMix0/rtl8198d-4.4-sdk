include $(TOPDIR)/rules.mk

PKG_NAME:=https-dns-proxy
PKG_RELEASE:=1
PKG_VERSION:=1.0
PKG_BUILD_DIR:=$(BUILD_DIR)/https-dns-proxy-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/https-dns-proxy
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libcurl +libev +libc-ares
	TITLE:=https-dns-proxy
endef

define Package/https-dns-proxy/description
	https-dns-proxy is a light-weight DNS<-->HTTPS, non-caching translation proxy for the RFC 8484 DoH standard.
	It receives regular (UDP) DNS requests and issues them via DoH. 
endef

TARGET_CFLAGS += -std=gnu99

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/https-dns-proxy/conffiles
/etc/config/https-dns-proxy
endef

define Package/https-dns-proxy/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/https-dns-proxy $(1)/usr/sbin/
	$(INSTALL_BIN) ./filesystem/usr/sbin/detect_doh_server $(1)/usr/sbin/
	$(INSTALL_BIN) ./filesystem/usr/sbin/dohdot_reload $(1)/usr/sbin/
	$(INSTALL_BIN) ./filesystem/usr/sbin/doh_server_check $(1)/usr/sbin/
	$(INSTALL_BIN) ./filesystem/etc/init.d/https-dns-proxy $(1)/etc/init.d/https-dns-proxy
	$(INSTALL_CONF) ./filesystem/etc/config/dns_privacy $(1)/etc/config/dns_privacy
endef

$(eval $(call BuildPackage,https-dns-proxy))