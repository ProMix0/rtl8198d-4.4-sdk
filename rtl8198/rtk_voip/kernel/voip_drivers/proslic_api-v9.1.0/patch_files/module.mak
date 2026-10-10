proslic_patch-y :=

#### Si3226 ####
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_C_FLBK_2010AUG10.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_D_FLBK_2010AUG10.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_C_QCUK_2010AUG10.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_D_QCUK_2010AUG10.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_E_FLBK_2010AUG10.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226) += voip_drivers/proslic_api/patch_files/si3226_patch_E_FLBK_2010JAN06.o

#### Si3226x ####
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_FB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_FB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2013AUG09.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_FB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2012DEC06.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_TSS_ISO) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_TSS_ISO_2014JUN18.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_TSS_ISO) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_TSS_ISO_2013AUG09.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_TSS_ISO) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_TSS_ISO_2012DEC06.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_QCUK) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_QCUK) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2012DEC06.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_PBB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_PBB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2013AUG09.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_PBB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2012DEC06.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCQC) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCFB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x_LCUB) += voip_drivers/proslic_api/patch_files/si3226x_patch_C_FB_2017MAY26.o

#### Si3217x ####
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_BB) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_BB_2012DEC10.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_BB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_BB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2014JUN18.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_BB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2013FEB04.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2014JUN18.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_FB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2013FEB04.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2014JUN18.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_PBB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2013FEB04.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCQC) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCQC) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCQC) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2014JUN18.o
#proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCQC) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2013FEB04.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCUB) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCUB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCFB) += voip_drivers/proslic_api/patch_files/si3217x_patch_B_FB_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x_LCFB) += voip_drivers/proslic_api/patch_files/si3217x_patch_C_FB_2017MAY25.o

#### Si3218x ####
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x_CB) += voip_drivers/proslic_api/patch_files/si3218x_patch_A_2017MAY25.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x_LCQC) += voip_drivers/proslic_api/patch_files/si3218x_patch_A_2017MAY25.o

#### Si3219x ####
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x_CB) += voip_drivers/proslic_api/patch_files/si3219x_patch_A_2017MAY25.o

#### Si3228x ####
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_CB) += voip_drivers/proslic_api/patch_files/si3228x_patch_A_2017MAY26.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_CB_SR) += voip_drivers/proslic_api/patch_files/si3228x_patch_A_2017MAY26.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_LCQC) += voip_drivers/proslic_api/patch_files/si3228x_patch_A_2017MAY26.o
proslic_patch-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x_MULTI_BOM) += voip_drivers/proslic_api/patch_files/si3228x_patch_A_2017MAY26.o

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

CFLAGS_si3226_patch_C_FLBK_2010AUG10.o := $(OBJS_CFLAGS)
CFLAGS_si3226_patch_D_FLBK_2010AUG10.o := $(OBJS_CFLAGS)
CFLAGS_si3226_patch_C_QCUK_2010AUG10.o := $(OBJS_CFLAGS)
CFLAGS_si3226_patch_D_QCUK_2010AUG10.o := $(OBJS_CFLAGS)
CFLAGS_si3226_patch_E_FLBK_2010AUG10.o := $(OBJS_CFLAGS)
CFLAGS_si3226_patch_E_FLBK_2010JAN06.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2013AUG09.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2012DEC06.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_TSS_ISO_2014JUN18.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_TSS_ISO_2013AUG09.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_TSS_ISO_2012DEC06.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2012DEC06.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2013AUG09.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2012DEC06.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3226x_patch_C_FB_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_BB_2012DEC10.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2014JUN18.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2013FEB04.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2014JUN18.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2013FEB04.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2014JUN18.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2013FEB04.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2014JUN18.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2013FEB04.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_B_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3217x_patch_C_FB_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_patch_A_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3218x_patch_A_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3219x_patch_A_2017MAY25.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_patch_A_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_patch_A_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_patch_A_2017MAY26.o := $(OBJS_CFLAGS)
CFLAGS_si3228x_patch_A_2017MAY26.o := $(OBJS_CFLAGS)
