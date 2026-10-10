fs_debug-y := voip_fs/debug/voip_time_measure.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
OBJS_CFLAGS += -I$(DIR_VOIP)/include

ifeq "$(CONFIG_VOIP_COP3_PROFILE)" "y"
OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_voip_time_measure.o := $(OBJS_CFLAGS)
