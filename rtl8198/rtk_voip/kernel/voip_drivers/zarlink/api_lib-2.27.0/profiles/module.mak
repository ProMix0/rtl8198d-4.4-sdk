zarlink_lib_profiles-y := voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Ve_profile.o

# for Narrow band High voltage 88221/241/266/286
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le880NB_HV.o
endif

# for Narrow band High voltage 89136/89336
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
#zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le890NB_HV.o
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le890NB_LVHV.o
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES_WIDEBAND)" "y"
zarlink_lib_profiles-$(CONFIG_BOARD_LE89156) += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le89156_100V_IB_Profile.o
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le890WB_HV.o
endif
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9642.o
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/ZLR966121L.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9641.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9643.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9651.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9653.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601)" "y"
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Realtek_88601_zsi.o
else
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Realtek_88601.o
endif
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9632.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9622.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652)" "y"
zarlink_lib_profiles-y += voip_drivers/zarlink/$(ZARLINK_LIB)/profiles/Le9652.o
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/

#include $(DIR_VOIP)/rules.mak

CFLAGS_Ve_profile.o := $(OBJS_CFLAGS)
CFLAGS_Le880NB_HV.o := $(OBJS_CFLAGS)
CFLAGS_Le890NB_HV.o := $(OBJS_CFLAGS)
CFLAGS_Le890NB_LVHV.o := $(OBJS_CFLAGS)
CFLAGS_Le89156_100V_IB_Profile.o := $(OBJS_CFLAGS)
CFLAGS_Le890WB_HV.o := $(OBJS_CFLAGS)
CFLAGS_Le9642.o := $(OBJS_CFLAGS)
CFLAGS_ZLR966121L.o := $(OBJS_CFLAGS)
CFLAGS_Le9641.o := $(OBJS_CFLAGS)
CFLAGS_Le9643.o := $(OBJS_CFLAGS)
CFLAGS_Le9651.o := $(OBJS_CFLAGS)
CFLAGS_Le9653.o := $(OBJS_CFLAGS)
CFLAGS_Realtek_88601_zsi.o := $(OBJS_CFLAGS)
CFLAGS_Realtek_88601.o := $(OBJS_CFLAGS)
CFLAGS_Le9632.o := $(OBJS_CFLAGS)
CFLAGS_Le9622.o := $(OBJS_CFLAGS)
CFLAGS_Le9652.o := $(OBJS_CFLAGS)
