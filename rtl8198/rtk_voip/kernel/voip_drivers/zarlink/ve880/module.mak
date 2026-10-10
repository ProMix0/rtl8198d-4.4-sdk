ve880-y	:= voip_drivers/zarlink/ve880/ve880_int.o voip_drivers/zarlink/ve880/ve880_api.o

ifneq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK_ON_NEW_ARCH)" "y"
ve880-y	+= voip_drivers/zarlink/ve880/ve880_int_ext.o
endif

OBJS_CFLAGS :=
#OBJS_CFLAGS += -Werror

OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
endif
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
endif

OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/spi/

#include $(DIR_VOIP)/rules.mak

CFLAGS_ve880_int.o := $(OBJS_CFLAGS)
CFLAGS_ve880_api.o := $(OBJS_CFLAGS)
CFLAGS_ve880_int_ext.o := $(OBJS_CFLAGS)
