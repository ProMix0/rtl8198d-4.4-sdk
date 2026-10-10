fs_proc-y := voip_fs/proc/voip_info_proc.o
fs_proc-y += voip_fs/proc/ethernet_dsp_proc.o
fs_proc-y += voip_fs/proc/cpu_info_proc.o
fs_proc-y += voip_fs/proc/voip_counter_proc.o
fs_proc-y += voip_fs/proc/generic.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_manager
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_ipc
OBJS_CFLAGS += -I$(DIR_VOIP)/include

#include $(DIR_VOIP)/rules.mak

CFLAGS_voip_info_proc.o := $(OBJS_CFLAGS) -Wno-error=date-time
CFLAGS_ethernet_dsp_proc.o := $(OBJS_CFLAGS)
CFLAGS_cpu_info_proc.o := $(OBJS_CFLAGS)
CFLAGS_voip_counter_proc.o := $(OBJS_CFLAGS)
CFLAGS_generic.o := $(OBJS_CFLAGS)

#include $(DIR_VOIP)/rules.mak
