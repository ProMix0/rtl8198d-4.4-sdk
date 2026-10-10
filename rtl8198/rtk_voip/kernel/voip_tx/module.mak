voip_tx-y := voip_tx/direct_send_mgr.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
ifeq ($(CONFIG_RTK_VOIP_PACKAGE_865X),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8651)$(CONFIG_RTK_VOIP_DRIVERS_PCM865xC),y)
OBJS_CFLAGS += -Idrivers/net/re865x/rtl865x
endif
endif

ifeq ($(CONFIG_RTK_VOIP_SRTP),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
#OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/crypto/include
endif

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

ifeq "$(CONFIG_AUDIOCODES_VOIP)" "y"
OBJS_CFLAGS += -DAUDIOCODES_VOIP
OBJS_CFLAGS += -DACMW_LX
OBJS_CFLAGS += -DACL_BYPASS
OBJS_CFLAGS += -DACL_RELEASE
OBJS_CFLAGS += -DAUDIOCODES_VOIP_SIP
endif

# rock: add module name for PRINT
#OBJS_CFLAGS += -DMODULE_NAME=\"TX\"

#include $(DIR_VOIP)/rules.mak

CFLAGS_direct_send_mgr.o := $(OBJS_CFLAGS)
