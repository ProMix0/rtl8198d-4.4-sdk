
-include $(KERNEL_SRC)/include/config/auto.conf

ifeq ($(CONFIG_ARCH_CORTINA_G3), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-gen1
NI_DRV_DIR = ni-drv-gen1
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-gen1
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
else ifeq ($(CONFIG_ARCH_CORTINA_G3HGU), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-gen1
NI_DRV_DIR = ni-drv-gen1
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-gen1
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
else ifeq ($(CONFIG_ARCH_CORTINA_G3LITE), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-gen1
NI_DRV_DIR = ni-drv-gen1
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-gen1
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
else ifeq ($(CONFIG_ARCH_CORTINA_SATURN), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-gen1
NI_DRV_DIR = ni-drv-gen1
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-gen1
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
else ifeq ($(CONFIG_ARCH_REALTEK_TAURUS), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-77c
NI_DRV_DIR = ni-drv-77c
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-77c
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
else ifeq ($(CONFIG_ARCH_CORTINA_VENUS), y)
COMMON_DIR = common
PON_DRV_DIR = pon-drv-gen2
NI_DRV_DIR = ni-drv-gen2
API_DIR = cortina-api
PLOAM_DIR = ploam
AAL_DIR = aal-gen2
ADAPT_DIR = adapt
KHOOK_DIR = kernel-hook
endif

ifeq ($(CONFIG_LUNA_G3_SERIES), y)
### RTK FPGA TEST ###
RTK_FPGA_DIR = rtk_venus_fpga
endif

