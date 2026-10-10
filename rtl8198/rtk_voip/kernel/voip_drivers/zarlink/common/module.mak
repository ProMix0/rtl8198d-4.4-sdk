zarlink_common-y := voip_drivers/zarlink/common/zarlinkCommonInit.o
zarlink_common-y += voip_drivers/zarlink/common/zarlinkCommonSlic.o
zarlink_common-y += voip_drivers/zarlink/common/zarlinkCommonDaa.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve880/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)" "y"
ifeq "$(SLIC_V886_FXS_LOW_POWER_MODE)" "y"
OBJS_CFLAGS += -DSLIC_V886_FXS_LOW_POWER_MODE
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve886/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
endif
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
ifeq "$(SLIC_V890_FXS_LOW_POWER_MODE)" "y"
OBJS_CFLAGS += -DSLIC_V890_FXS_LOW_POWER_MODE
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve890/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/
endif
endif

OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/
OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_zarlinkCommonInit.o := $(OBJS_CFLAGS)
CFLAGS_zarlinkCommonSlic.o := $(OBJS_CFLAGS)
CFLAGS_zarlinkCommonDaa.o := $(OBJS_CFLAGS)
