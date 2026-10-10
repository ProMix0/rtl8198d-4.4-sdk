lt_vp886_lt-y := voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_909_all.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_cap.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_dc_feed_st.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_dc_voltage.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_line_v.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_loopback.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_msocket.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_on_off_hook_st.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_pre_line_v.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_rd_bat_cond.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_rd_loop_cond.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_res_flt.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_ringers.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_ringing_st.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_roh.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886_xconnect.o
lt_vp886_lt-y += voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/lt_api_vp886.o

OBJS_CFLAGS :=
#OBJS_CFLAGS += -Werror
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI

#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/
#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

OBJS_CFLAGS += -I$(DIR_VOIP)/include/

#include $(DIR_VOIP)/rules.mak

CFLAGS_lt_api_vp886_909_all.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_cap.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_dc_feed_st.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_dc_voltage.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_line_v.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_loopback.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_msocket.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_on_off_hook_st.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_pre_line_v.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_rd_bat_cond.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_rd_loop_cond.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_res_flt.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_ringers.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_ringing_st.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_roh.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886_xconnect.o := $(OBJS_CFLAGS)
CFLAGS_lt_api_vp886.o := $(OBJS_CFLAGS)
