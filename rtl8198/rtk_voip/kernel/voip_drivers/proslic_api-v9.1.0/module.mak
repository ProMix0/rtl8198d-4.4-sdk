include $(DIR_VOIP)/voip_drivers/proslic_api/custom/module.mak
include $(DIR_VOIP)/voip_drivers/proslic_api/src/module.mak
include $(DIR_VOIP)/voip_drivers/proslic_api/patch_files/module.mak

proslic_api-y := $(proslic_custom-y)
proslic_api-y += $(proslic_src-y)
proslic_api-y += $(proslic_patch-y)

include $(DIR_VOIP)/rules.mak
