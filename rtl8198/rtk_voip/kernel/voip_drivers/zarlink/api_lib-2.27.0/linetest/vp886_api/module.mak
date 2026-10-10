lt_vp886_api-y := voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_3ele_cap.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_ac_rloop.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_conclude.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_dc_rloop.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_gain_toolbox.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_loop_conditions.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_loopback.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_met_ramp.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_met_ramp_init.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_mpicmd.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_msocket.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_open_vxc.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_prepare.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_res_flt.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_testline.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_timer.o
lt_vp886_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/vp886_xconnect.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp886_3ele_cap.o := $(OBJS_CFLAGS)
CFLAGS_vp886_ac_rloop.o := $(OBJS_CFLAGS)
CFLAGS_vp886_conclude.o := $(OBJS_CFLAGS)
CFLAGS_vp886_dc_rloop.o := $(OBJS_CFLAGS)
CFLAGS_vp886_gain_toolbox.o := $(OBJS_CFLAGS)
CFLAGS_vp886_loop_conditions.o := $(OBJS_CFLAGS)
CFLAGS_vp886_loopback.o := $(OBJS_CFLAGS)
CFLAGS_vp886_met_ramp.o := $(OBJS_CFLAGS)
CFLAGS_vp886_met_ramp_init.o := $(OBJS_CFLAGS)
CFLAGS_vp886_mpicmd.o := $(OBJS_CFLAGS)
CFLAGS_vp886_msocket.o := $(OBJS_CFLAGS)
CFLAGS_vp886_open_vxc.o := $(OBJS_CFLAGS)
CFLAGS_vp886_prepare.o := $(OBJS_CFLAGS)
CFLAGS_vp886_res_flt.o := $(OBJS_CFLAGS)
CFLAGS_vp886_testline.o := $(OBJS_CFLAGS)
CFLAGS_vp886_timer.o := $(OBJS_CFLAGS)
CFLAGS_vp886_xconnect.o := $(OBJS_CFLAGS)
