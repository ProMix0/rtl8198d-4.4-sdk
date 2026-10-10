lt_vp890_lt-y := voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_all_gr_909.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_cap.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_cap_ringers.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_cross_connect.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_dc_feed.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_dc_voltage.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_flt_dscrm.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_line_v.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_loopback.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_master_socket.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_on_off_hook_st.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_pre_linev.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_rd_bat_cond.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_rd_loop_cond.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_res_flt.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_ringers.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_ringing_st.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_roh.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890_vvp_calculate_results.o
lt_vp890_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/lt_api_vp890.o

OBJS_CFLAGS :=
#OBJS_CFLAGS += -Werror
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/
#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

OBJS_CFLAGS += -I$(DIR_VOIP)/include/

#include $(DIR_VOIP)/rules.mak

CFLAGS_lt_api_vp890_all_gr_909.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_cap.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_cap_ringers.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_cross_connect.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_dc_feed.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_dc_voltage.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_flt_dscrm.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_line_v.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_loopback.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_master_socket.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_on_off_hook_st.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_pre_linev.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_rd_bat_cond.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_rd_loop_cond.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_res_flt.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_ringers.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_ringing_st.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_roh.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890_vvp_calculate_results.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp890.o := $(OBJS_CFLAGS)
