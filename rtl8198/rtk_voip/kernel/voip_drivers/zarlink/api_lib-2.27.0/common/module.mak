zarlink_lib_common-y := voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_config.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_common.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_control.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_cslac_seq.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_init.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_query.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_api_test.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_dtmf_detect.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_pulse_decode.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_timer_queue.o
zarlink_lib_common-y += voip_drivers/zarlink/$(ZARLINK_LIB)/common/vp_debug.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

ifeq ($(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
endif
endif

ifeq ($(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
endif

ifeq ($(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
endif
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_vp_api_config.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_common.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_control.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_cslac_seq.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_init.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_query.o := $(OBJS_CFLAGS)
CFLAGS_vp_api_test.o := $(OBJS_CFLAGS)
CFLAGS_vp_dtmf_detect.o := $(OBJS_CFLAGS)
CFLAGS_vp_pulse_decode.o := $(OBJS_CFLAGS)
CFLAGS_vp_timer_queue.o := $(OBJS_CFLAGS)
CFLAGS_vp_debug.o := $(OBJS_CFLAGS)
