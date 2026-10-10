lt_vp890_api-y := voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_3ele_cap.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_3ele_res_hg.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_3ele_res_lg.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_ac_rloop.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_calibrate.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_conclude.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_cross_connect.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_dc_rloop.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_flt_dscrm.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_hybrid_loss.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_loop_conditions.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_loopback.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_met_ramp.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_met_ramp_int.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_msocket.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_open_vxc.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_test_timer.o
lt_vp890_api-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/vp890_testline.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp890_calibrate.o := $(OBJS_CFLAGS)
CFLAGS_vp890_conclude.o := $(OBJS_CFLAGS)
CFLAGS_vp890_cross_connect.o := $(OBJS_CFLAGS)
CFLAGS_vp890_api/vp890_dc_rloop.o := $(OBJS_CFLAGS)
CFLAGS_vp890_flt_dscrm.o := $(OBJS_CFLAGS)
CFLAGS_vp890_hybrid_loss.o := $(OBJS_CFLAGS)
CFLAGS_vp890_loop_conditions.o := $(OBJS_CFLAGS)
CFLAGS_vp890_loopback.o := $(OBJS_CFLAGS)
CFLAGS_vp890_met_ramp.o := $(OBJS_CFLAGS)
CFLAGS_vp890_met_ramp_int.o := $(OBJS_CFLAGS)
CFLAGS_vp890_msocket.o := $(OBJS_CFLAGS)
CFLAGS_vp890_open_vxc.o := $(OBJS_CFLAGS)
CFLAGS_vp890_test_timer.o := $(OBJS_CFLAGS)
CFLAGS_vp890_testline.o := $(OBJS_CFLAGS)
