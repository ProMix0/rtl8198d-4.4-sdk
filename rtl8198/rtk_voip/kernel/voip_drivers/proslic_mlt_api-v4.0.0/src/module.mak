#Makefile       proslic_mlt_dcfeed.c     proslic_mlt_math.c     si3217x_mlt.c
#proslic_mlt.c  proslic_mlt_diag_madc.c  proslic_mlt_version.c  si3226x_mlt.c

proslic_mlt-y := voip_drivers/proslic_mlt_api-v4.0.0/src/proslic_mlt_dcfeed.o
proslic_mlt-y += voip_drivers/proslic_mlt_api-v4.0.0/src/proslic_mlt_math.o
proslic_mlt-y += voip_drivers/proslic_mlt_api-v4.0.0/src/proslic_mlt_diag_madc.o
proslic_mlt-y += voip_drivers/proslic_mlt_api-v4.0.0/src/proslic_mlt_version.o
#proslic_mlt-y += voip_drivers/proslic_mlt_api-v4.0.0/src/proslic_tstin.o

proslic_mlt-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) += voip_drivers/proslic_mlt_api-v4.0.0/src/si3226x_mlt.o

proslic_mlt-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/proslic_mlt_api-v4.0.0/src/si3217x_mlt.o

proslic_mlt-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += voip_drivers/proslic_mlt_api-v4.0.0/src/si3218x_mlt.o

proslic_mlt-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x) += voip_drivers/proslic_mlt_api-v4.0.0/src/si3219x_mlt.o

proslic_mlt-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x) += voip_drivers/proslic_mlt_api-v4.0.0/src/si3228x_mlt.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/inc
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/custom
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_mlt_api-v4.0.0/inc

OBJS_CFLAGS += -DREALTEK_PATCH_FOR_SILAB
OBJS_CFLAGS += -DPROSLIC_LINUX_KERNEL
OBJS_CFLAGS += -DENABLE_DEBUG

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x)" "y"
OBJS_CFLAGS += -DSI3217X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x)" "y"
OBJS_CFLAGS += -DSI3218X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x)" "y"
OBJS_CFLAGS += -DSI3226X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x)" "y"
OBJS_CFLAGS += -DSI3219X
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x)" "y"
OBJS_CFLAGS += -DSI3228X
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_proslic_mlt_dcfeed.o := $(OBJS_CFLAGS)
CFLAGS_proslic_mlt_math.o := $(OBJS_CFLAGS)
CFLAGS_proslic_mlt_diag_madc.o := $(OBJS_CFLAGS)
CFLAGS_proslic_mlt_version.o := $(OBJS_CFLAGS)
CFLAGS_proslic_tstin.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_mlt.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_mlt.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_mlt.o := $(OBJS_CFLAGS)
CFLAGS_si3219x_mlt.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_mlt.o := $(OBJS_CFLAGS)
