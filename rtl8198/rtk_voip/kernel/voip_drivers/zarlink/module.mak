zarlink-y := voip_drivers/zarlink/zarlink_api.o

include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/module.mak
zarlink-y += $(zarlink_lib-y)

include $(DIR_VOIP)/voip_drivers/zarlink/common/module.mak
zarlink-y += $(zarlink_common-y)

ifneq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK_ON_NEW_ARCH)" "y"
zarlink-y += voip_drivers/zarlink/zarlink_int.o
endif

OBJS_CFLAGS :=
#OBJS_CFLAGS += -Werror
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"

include $(DIR_VOIP)/voip_drivers/zarlink/ve880/module.mak
zarlink-y += $(ve880-y)

OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve880/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)" "y"

include $(DIR_VOIP)/voip_drivers/zarlink/ve886/module.mak
zarlink-y += $(ve886-y)

export SLIC_V886_FXS_LOW_POWER_MODE=y
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve886/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
endif
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"

include $(DIR_VOIP)/voip_drivers/zarlink/ve890/module.mak
zarlink-y += $(ve890-y)

export SLIC_V890_FXS_LOW_POWER_MODE=y
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve890/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
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

CFLAGS_zarlink_api.o := $(OBJS_CFLAGS)
