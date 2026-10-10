vp890_api-y := voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_apiCal.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_common.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_config.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_control.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_fxo_control.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_fxs_control.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_lp_control.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_seq.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_init.o
vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/vp890_query.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
OBJS_CFLAGS += -I$(DIR_VOIP)/include/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp890_apiCal.o := $(OBJS_CFLAGS)
CFLAGS_vp890_common.o := $(OBJS_CFLAGS)
CFLAGS_vp890_config.o := $(OBJS_CFLAGS)
CFLAGS_vp890_control.o := $(OBJS_CFLAGS)
CFLAGS_vp890_fxo_control.o := $(OBJS_CFLAGS)
CFLAGS_vp890_fxs_control.o := $(OBJS_CFLAGS)
CFLAGS_vp890_lp_control.o := $(OBJS_CFLAGS)
CFLAGS_vp890_seq.o := $(OBJS_CFLAGS)
CFLAGS_vp890_init.o := $(OBJS_CFLAGS)
CFLAGS_vp890_query.o := $(OBJS_CFLAGS)
