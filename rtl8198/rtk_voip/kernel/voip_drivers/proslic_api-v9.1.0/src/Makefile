obj-y := proslic.o si_voice_version.o si_voice.o
#obj-y += proslic_tstin.o
obj-y += rtk_proslic_wrapper/proslic_ctrl_spi.o rtk_proslic_wrapper/proslic_timer_linux.o 

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += si3226_intf.o rtk_proslic_userdef/si3226_userdef.o

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) += si3226x_intf.o rtk_proslic_userdef/si3226x_userdef.o

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x) += si3228x_intf.o rtk_proslic_userdef/si3228x_userdef.o

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += si3217x_intf.o si3217x_revb_intf.o si3217x_revc_intf.o rtk_proslic_userdef/si3217x_userdef.o

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += si3218x_intf.o rtk_proslic_userdef/si3218x_userdef.o

obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x) += si3219x_intf.o rtk_proslic_userdef/si3219x_userdef.o
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_FXO),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SI3050),y)
obj-y += vdaa.o
endif
endif

EXTRA_CFLAGS += -I$(DIR_VOIP)/include
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/inc
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/custom

EXTRA_CFLAGS += -DREALTEK_PATCH_FOR_SILAB
EXTRA_CFLAGS += -DPROSLIC_LINUX_KERNEL
EXTRA_CFLAGS += -DO0
EXTRA_CFLAGS += -DPROSLICVERSION=8
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x)" "y"
EXTRA_CFLAGS += -DSI3217X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x)" "y"
EXTRA_CFLAGS += -DSI3218X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x)" "y"
EXTRA_CFLAGS += -DSI3219X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226)" "y"
EXTRA_CFLAGS += -DSI322X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x)" "y"
EXTRA_CFLAGS += -DSI3226X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x)" "y"
EXTRA_CFLAGS += -DSI3228X
endif

include $(DIR_VOIP)/rules.mak
