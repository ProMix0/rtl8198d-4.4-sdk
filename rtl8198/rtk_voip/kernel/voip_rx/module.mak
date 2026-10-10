voip_rx-y := voip_rx/rtk_trap.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_tx

ifeq ($(CONFIG_RTK_VOIP_SRTP),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/crypto/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
#OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
endif

ifeq ($(CONFIG_RTK_VOIP_T38),y)
ifneq ($(CONFIG_VOIP_SDK),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/t38/rly_src
endif
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
#OBJS_CFLAGS += -DMODULE_NAME=\"RX\"

#include $(DIR_VOIP)/rules.mak

CFLAGS_rtk_trap.o := $(OBJS_CFLAGS)
