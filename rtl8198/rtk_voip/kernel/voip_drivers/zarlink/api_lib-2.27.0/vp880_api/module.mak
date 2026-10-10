vp880_api-y := voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_abs_calibration.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_calibration_common.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_control_common.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_fxo_control.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_fxs_control.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_init.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_linestate_control.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_lp_control.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_query.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_seq.o
vp880_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/vp880_tracker_calibration.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp880_abs_calibration.o := $(OBJS_CFLAGS)
CFLAGS_vp880_calibration_common.o := $(OBJS_CFLAGS)
CFLAGS_vp880_api/vp880_control_common.o := $(OBJS_CFLAGS)
CFLAGS_vp880_fxo_control.o := $(OBJS_CFLAGS)
CFLAGS_vp880_fxs_control.o := $(OBJS_CFLAGS)
CFLAGS_vp880_init.o := $(OBJS_CFLAGS)
CFLAGS_vp880_linestate_control.o := $(OBJS_CFLAGS)
CFLAGS_vp880_lp_control.o := $(OBJS_CFLAGS)
CFLAGS_vp880_query.o := $(OBJS_CFLAGS)
CFLAGS_vp880_seq.o := $(OBJS_CFLAGS)
CFLAGS_vp880_tracker_calibration.o := $(OBJS_CFLAGS)
