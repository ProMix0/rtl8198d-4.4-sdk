include $(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/module.mak
include $(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/src/module.mak
include $(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/src/module.mak

intel-y := $(lib_ifxos-y)
intel-y += $(drv_tapi-y)
intel-y += $(drv_dxs-y)

#include $(DIR_VOIP)/rules.mak
