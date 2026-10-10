obj-y   :=      common/ifx_crc.o	\
		common/ifx_fifo.o	\
		common/ifxos_debug.o	\
		common/ifxos_sys_show.o	\
		common/ifxos_version.o	\
		linux/ifxos_linux_lock_drv.o	\
		linux/ifxos_linux_copy_user_space_drv.o	\
		linux/ifxos_linux_mutex_drv.o	\
		linux/ifxos_linux_socket_drv.o	\
		linux/ifxos_linux_memory_alloc_drv.o	\
		linux/ifxos_linux_event_drv.o	\
		linux/ifxos_linux_memory_map_drv.o	\
		linux/ifxos_linux_thread_drv.o	\
		linux/ifxos_linux_select_drv.o	\
		linux/ifxos_linux_time_drv.o

#EXTRA_CFLAGS	+= -I
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/include
EXTRA_CFLAGS += -I$(LINUXDIR)/include
EXTRA_CFLAGS += -D__KERNEL__ -D__LINUX__ -DLINUX -DRW_MULTI_THREAD -D_REENTRANT -D_THREAD_SAFE-D_POSIX_SOURCE -D_GNU_SOURCE

include $(DIR_VOIP)/rules.mak
