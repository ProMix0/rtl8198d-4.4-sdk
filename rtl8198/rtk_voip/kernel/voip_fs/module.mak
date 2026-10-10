include $(DIR_VOIP)/voip_fs/proc/module.mak
include $(DIR_VOIP)/voip_fs/dev/module.mak
include $(DIR_VOIP)/voip_fs/debug/module.mak

voip_fs-y := $(fs_proc-y) $(fs_dev-y) $(fs_debug-y)

#include $(DIR_VOIP)/rules.mak
