lib_ifxos-y := voip_drivers/intel/lib_ifxos-1.6.9/src/common/ifx_crc.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/common/ifx_fifo.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/common/ifxos_debug.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/common/ifxos_sys_show.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/common/ifxos_version.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_lock_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_copy_user_space_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_mutex_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_socket_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_memory_alloc_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_event_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_memory_map_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_thread_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_select_drv.o
lib_ifxos-y += voip_drivers/intel/lib_ifxos-1.6.9/src/linux/ifxos_linux_time_drv.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/include
OBJS_CFLAGS += -I$(LINUXDIR)/include
OBJS_CFLAGS += -D__KERNEL__ -D__LINUX__ -DLINUX -DRW_MULTI_THREAD -D_REENTRANT -D_THREAD_SAFE-D_POSIX_SOURCE -D_GNU_SOURCE

#include $(DIR_VOIP)/rules.mak

CFLAGS_ifx_crc.o := $(OBJS_CFLAGS)
CFLAGS_ifx_fifo.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_debug.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_sys_show.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_version.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_lock_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_copy_user_space_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_mutex_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_socket_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_memory_alloc_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_event_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_memory_map_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_thread_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_select_drv.o := $(OBJS_CFLAGS)
CFLAGS_ifxos_linux_time_drv.o := $(OBJS_CFLAGS)
