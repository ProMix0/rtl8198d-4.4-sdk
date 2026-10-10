SRC_DIR:=`pwd`

-include $(KERNEL_SRC)/include/config/auto.conf
-include $(PWD)/.config
-include $(PWD)/dir.mk
.DEFAULT_GOAL := all
all modules: include/config/auto.conf
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC_DIR) modules

modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC_DIR) $@

distclean: clean
	$(RM) -r include/config include/generated .config .config.old

ifeq ($(CONFIG_NE_HAVE_DOT_CONFIG),)
.config: menuconfig
endif

-include include/config/auto.conf.cmd

menuconfig: $(KERNEL_SRC)/scripts/kconfig/mconf
	$< Kconfig

%defconfig:$(KERNEL_SRC)/scripts/kconfig/conf
	$< --defconfig=configs/$@ Kconfig

aal ni-drv  pon-drv:
	ln -s $($(shell echo $@ | tr a-z- A-Z_)_DIR) $@

include/config/auto.conf:.config $(KERNEL_SRC)/scripts/kconfig/conf | aal ni-drv  pon-drv
	-mkdir -p include/config include/generated
	KCONFIG_AUTOHEADER=include/generated/ca_ne_autoconf.h $(KERNEL_SRC)/scripts/kconfig/conf --silentoldconfig Kconfig

$(KERNEL_SRC)/scripts/kconfig/conf:$(KERNEL_SRC)/scripts/basic/fixdep
$(KERNEL_SRC)/scripts/kconfig/mconf:$(KERNEL_SRC)/scripts/basic/fixdep

$(KERNEL_SRC)/scripts/kconfig/conf \
$(KERNEL_SRC)/scripts/kconfig/mconf \
$(KERNEL_SRC)/scripts/basic/fixdep:
	$(MAKE) -C $(KERNEL_SRC) -f scripts/Makefile.build \
		obj=scripts/kconfig srctree=. CC=gcc HOSTCC=gcc \
		$(subst $(KERNEL_SRC)/,,$@)

PKG_DIR=$(shell pwd)

${DEST} ${DEST}/include:
	mkdir -p $@

include api.mk
include cli.mk
include iros.mk

.PHONY: all modules_install clean
