#
# Makefile for nvmem drivers.
#

obj-$(CONFIG_NVMEM)		+= nvmem_core.o
nvmem_core-y			:= core.o

# Devices
obj-$(CONFIG_CORTINA_EFUSE)	+= nvmem_cortina_efuse.o
nvmem_cortina_efuse-y		:= cortina-efuse.o
obj-$(CONFIG_CORTINA_OTP)	+= nvmem_cortina_otp.o
nvmem_cortina_otp-y		:= cortina-otp.o
obj-$(CONFIG_NVMEM_IMX_OCOTP)	+= nvmem-imx-ocotp.o
nvmem-imx-ocotp-y		:= imx-ocotp.o
obj-$(CONFIG_NVMEM_MXS_OCOTP)	+= nvmem-mxs-ocotp.o
nvmem-mxs-ocotp-y		:= mxs-ocotp.o
obj-$(CONFIG_QCOM_QFPROM)	+= nvmem_qfprom.o
nvmem_qfprom-y			:= qfprom.o
obj-$(CONFIG_ROCKCHIP_EFUSE)	+= nvmem_rockchip_efuse.o
nvmem_rockchip_efuse-y		:= rockchip-efuse.o
obj-$(CONFIG_NVMEM_SUNXI_SID)	+= nvmem_sunxi_sid.o
nvmem_sunxi_sid-y		:= sunxi_sid.o
obj-$(CONFIG_NVMEM_VF610_OCOTP)	+= nvmem-vf610-ocotp.o
nvmem-vf610-ocotp-y		:= vf610-ocotp.o
