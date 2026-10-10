proslic_custom-y :=

#### Si3226 ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/custom/si3226_QCUK_constants.o

#### Si3226x ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_FB) += voip_drivers/proslic_api/custom/si3226x_FLBK_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_QCUK) += voip_drivers/proslic_api/custom/si3226x_CUK_constants.o
#obj-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_PBB) += voip_drivers/proslic_api/custom/si3226x_PBB_VDC_4P5_16P0_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_PBB) += voip_drivers/proslic_api/custom/si3226x_BB_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCQC) += voip_drivers/proslic_api/custom/si3226x_LCQC6W_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCUB) += voip_drivers/proslic_api/custom/si3226x_LCUB_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCFB) += voip_drivers/proslic_api/custom/si3226x_LCFB_constants.o

#### Si3228x ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_CB) += voip_drivers/proslic_api/custom/si3228x_LCCB_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_CB_SR) += voip_drivers/proslic_api/custom/si3228x_LCCB_SR_constants.o

#### Si3217x ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_BB) += voip_drivers/proslic_api/custom/si3217x_BKBT_constants.o
#proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/custom/si3217x_FLBK_GDRV_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/custom/si3217x_FLBK_NO_GDRV_constants.o
#proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/custom/si3217x_PBB_VDC_3P0_6P0_GDRV_constants.o
#proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/custom/si3217x_PBB_VDC_9P0_24P0_GDRV_constants.o
#proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/custom/si3217x_PBB_VDC_3P0_6P0_NO_GDRV_constants.o
#proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/custom/si3217x_PBB_VDC_9P0_24P0_NO_GDRV_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCQC) += voip_drivers/proslic_api/custom/si3217x_LCQC6W_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCUB) += voip_drivers/proslic_api/custom/si3217x_LCUB_constants.o
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCFB) += voip_drivers/proslic_api/custom/si3217x_LCFB_constants.o

### Si3218x ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x_CB) += voip_drivers/proslic_api/custom/si3218x_LCCB_constants.o

### Si3219x ####
proslic_custom-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x_CB) += voip_drivers/proslic_api/custom/si3219x_LCCB_constants.o

#### Si3050 ####
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_FXO),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SI3050),y)
proslic_custom-y += voip_drivers/proslic_api/custom/vdaa_constants.o
endif
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/inc
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/custom

OBJS_CFLAGS += -DREALTEK_PATCH_FOR_SILAB
OBJS_CFLAGS += -DPROSLIC_LINUX_KERNEL
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

CFLAGS_si3226_QCUK_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_FLBK_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_CUK_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_PBB_VDC_4P5_16P0_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_BB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_LCQC6W_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_LCUB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_LCFB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_LCCB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_LCCB_SR_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_BKBT_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_FLBK_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_FLBK_NO_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_PBB_VDC_3P0_6P0_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_PBB_VDC_9P0_24P0_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_PBB_VDC_3P0_6P0_NO_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_PBB_VDC_9P0_24P0_NO_GDRV_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_LCQC6W_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_LCUB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_LCFB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_LCCB_constants.o := $(OBJS_CFLAGS)
CFLAGS_si3219x_LCCB_constants.o := $(OBJS_CFLAGS)
CFLAGS_vdaa_constants.o := $(OBJS_CFLAGS)
