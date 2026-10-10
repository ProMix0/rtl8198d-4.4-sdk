vp886_api-y := voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_adaptive_ringing.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_calibration_common.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_common.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_control.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_events.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_init.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_query.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_seq.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_slac.o
vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/vp886_timers.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
OBJS_CFLAGS += -I$(DIR_VOIP)/include/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp886_adaptive_ringing.o := $(OBJS_CFLAGS)
CFLAGS_vp886_calibration_common.o := $(OBJS_CFLAGS)
CFLAGS_vp886_common.o := $(OBJS_CFLAGS)
CFLAGS_vp886_control.o := $(OBJS_CFLAGS)
CFLAGS_vp886_events.o := $(OBJS_CFLAGS)
CFLAGS_vp886_init.o := $(OBJS_CFLAGS)
CFLAGS_vp886_query.o := $(OBJS_CFLAGS)
CFLAGS_vp886_seq.o := $(OBJS_CFLAGS)
CFLAGS_vp886_slac.o := $(OBJS_CFLAGS)
CFLAGS_vp886_timers.o := $(OBJS_CFLAGS)
