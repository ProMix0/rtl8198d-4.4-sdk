proslic_src-y := voip_drivers/proslic_api/src/proslic.o
proslic_src-y += voip_drivers/proslic_api/src/si_voice_version.o
proslic_src-y += voip_drivers/proslic_api/src/si_voice.o

#proslic_src-y += voip_drivers/proslic_api/src/proslic_tstin.o
proslic_src-y += voip_drivers/proslic_api/src/rtk_proslic_wrapper/proslic_ctrl_spi.o
proslic_src-y += voip_drivers/proslic_api/src/rtk_proslic_wrapper/proslic_timer_linux.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/src/si3226_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3226_userdef.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) += voip_drivers/proslic_api/src/si3226x_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3226x_userdef.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x) += voip_drivers/proslic_api/src/si3228x_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3228x_userdef.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/proslic_api/src/si3217x_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/proslic_api/src/si3217x_revb_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/proslic_api/src/si3217x_revc_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3217x_userdef.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += voip_drivers/proslic_api/src/si3218x_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3218x_userdef.o

proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x) += voip_drivers/proslic_api/src/si3219x_intf.o
proslic_src-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += voip_drivers/proslic_api/src/rtk_proslic_userdef/si3219x_userdef.o

ifeq ($(CONFIG_RTK_VOIP_DRIVERS_FXO),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SI3050),y)
proslic_src-y += voip_drivers/proslic_api/src/vdaa.o
endif
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/inc
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/custom

OBJS_CFLAGS += -DREALTEK_PATCH_FOR_SILAB
OBJS_CFLAGS += -DPROSLIC_LINUX_KERNEL
OBJS_CFLAGS += -DO0
OBJS_CFLAGS += -DPROSLICVERSION=8
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x)" "y"
OBJS_CFLAGS += -DSI3217X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x)" "y"
OBJS_CFLAGS += -DSI3218X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x)" "y"
OBJS_CFLAGS += -DSI3219X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226)" "y"
OBJS_CFLAGS += -DSI322X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x)" "y"
OBJS_CFLAGS += -DSI3226X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x)" "y"
OBJS_CFLAGS += -DSI3228X
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_proslic.o := $(OBJS_CFLAGS)
CFLAGS_si_voice_version.o := $(OBJS_CFLAGS)
CFLAGS_si_voice.o := $(OBJS_CFLAGS)
CFLAGS_proslic_tstin.o := $(OBJS_CFLAGS)
CFLAGS_proslic_ctrl_spi.o := $(OBJS_CFLAGS)
CFLAGS_proslic_timer_linux.o := $(OBJS_CFLAGS)
CFLAGS_si3226_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3226_userdef.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_userdef.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_userdef.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_revb_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_revc_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_userdef.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_userdef.o := $(OBJS_CFLAGS)
CFLAGS_si3219x_intf.o := $(OBJS_CFLAGS)
CFLAGS_si3219x_userdef.o := $(OBJS_CFLAGS)
CFLAGS_vdaa.o := $(OBJS_CFLAGS)
