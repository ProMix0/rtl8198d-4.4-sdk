include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/common/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/module.mak

zarlink_lib-y := $(zarlink_lib_common-y) $(zarlink_lib_profiles-y) $(zarlink_lib_arch-y)

#EXTRA_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/module.mak
zarlink_lib-y += $(vp890_api-y)

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/common/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_api/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp890_lt/module.mak
zarlink_lib-y += $(lt_common-y)
zarlink_lib-y += $(lt_vp890_api-y)
zarlink_lib-y += $(lt_vp890_lt-y)
endif
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/module.mak
zarlink_lib-y += $(vp880_api-y)
endif

ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)" "y"
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/module.mak
zarlink_lib-y += $(vp886_api-y)

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/common/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/module.mak
include $(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/module.mak
zarlink_lib-y += $(lt_common-y)
zarlink_lib-y += $(lt_vp886_api-y)
zarlink_lib-y += $(lt_vp886_lt-y)
endif
endif

#include $(DIR_VOIP)/rules.mak
