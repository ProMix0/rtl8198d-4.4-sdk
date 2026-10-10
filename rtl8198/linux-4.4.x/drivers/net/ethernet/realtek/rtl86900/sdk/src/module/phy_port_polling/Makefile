#########################################################
#	Build Realtek Packet Redirect modules									#
#########################################################

include $(APOLLODIR)/Compiler_Flag

MODULE_NAME=phy_port_polling
PWD = $(shell pwd)

SDKDIR=$(APOLLODIR)/sdk
SDK_INCLUDE   = $(SDKDIR)/include
SYS_INCLUDE   = $(SDKDIR)/system/include
DRV_INCLUDE   = $(SDKDIR)/system/linux
MOD_INCLUDE   = $(SDK_INCLUDE)/module/phy_port_polling

EXTRA_CFLAGS+=$(PON_CFLAGS)

FLAGS+=-I$(SDK_INCLUDE) -I$(SYS_INCLUDE) -I$(DRV_INCLUDE) -I$(NETDIR)

EXTRA_CFLAGS+=$(FLAGS)

ifeq ($(CONFIG_MODULE_BUILD_IN),y)
BUILD_MODE=y
else
BUILD_MODE=m
endif

obj-$(BUILD_MODE) := $(MODULE_NAME).o

$(MODULE_NAME)-objs:= phy_port_poll.o

all:
	@echo "   BUILD module $(MODULE_NAME), $(FLAGS)"
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=rlx CROSS_COMPILE=$(CROSS_COMPILE)
 

install:
	cp -af $(MODULE_NAME).ko $(ROMFSDIR)/lib/modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.ko 
