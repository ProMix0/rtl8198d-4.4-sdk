fs_dev-y := voip_fs/dev/voip_dev_core.o
fs_dev-y += voip_fs/dev/data_dump_dev.o
fs_dev-y += voip_fs/dev/voip_dev_proc.o
fs_dev-$(CONFIG_RTK_VOIP_CIRCULAR_LOG_DEV) += voip_fs/dev/circular_log.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_manager
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_ipc
OBJS_CFLAGS += -I$(DIR_VOIP)/include

#include $(DIR_VOIP)/rules.mak

CFLAGS_voip_dev_core.o := $(OBJS_CFLAGS)
CFLAGS_data_dump_dev.o := $(OBJS_CFLAGS)
CFLAGS_voip_dev_proc.o := $(OBJS_CFLAGS)
CFLAGS_circular_log.o := $(OBJS_CFLAGS)
