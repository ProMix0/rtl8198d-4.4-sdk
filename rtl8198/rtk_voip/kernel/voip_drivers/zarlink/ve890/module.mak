ve890-y	:= voip_drivers/zarlink/ve890/ve890_int.o voip_drivers/zarlink/ve890/ve890_api.o

ifneq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK_ON_NEW_ARCH)" "y"
ve890-y	+= voip_drivers/zarlink/ve890/ve890_int_ext.o
endif

OBJS_CFLAGS :=
#OBJS_CFLAGS += -Werror
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/
endif
endif

OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/spi/

#include $(DIR_VOIP)/rules.mak

CFLAGS_ve890_int.o := $(OBJS_CFLAGS)
CFLAGS_ve890_api.o := $(OBJS_CFLAGS)
CFLAGS_ve890_int_ext.o := $(OBJS_CFLAGS)
