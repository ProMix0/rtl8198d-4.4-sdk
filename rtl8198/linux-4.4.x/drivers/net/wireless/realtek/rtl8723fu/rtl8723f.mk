EXTRA_CFLAGS += -DCONFIG_RTL8723F

#ifeq ($(CONFIG_MP_INCLUDED), y)
### 8723F Default Enable VHT MP HW TX MODE ###
#EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
#CONFIG_MP_VHT_HW_TX_MODE = y
#endif

_HAL_INTFS_FILES +=	hal/rtl8723f/rtl8723f_halinit.o \
			hal/rtl8723f/rtl8723f_mac.o \
			hal/rtl8723f/rtl8723f_cmd.o \
			hal/rtl8723f/rtl8723f_phy.o \
			hal/rtl8723f/rtl8723f_ops.o \
			hal/rtl8723f/hal8723f_fw.o

ifeq ($(CONFIG_USB_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8723f/$(HCI_NAME)/rtl8723fu_halinit.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_halmac.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_io.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_xmit.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_recv.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_led.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fu_ops.o

_HAL_INTFS_FILES +=	hal/efuse/rtl8723f/HalEfuseMask8723F_USB.o
endif
ifeq ($(CONFIG_SDIO_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8723f/$(HCI_NAME)/rtl8723fs_halinit.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_halmac.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_io.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_xmit.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_recv.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_led.o \
			hal/rtl8723f/$(HCI_NAME)/rtl8723fs_ops.o

_HAL_INTFS_FILES +=	hal/efuse/rtl8723f/HalEfuseMask8723F_SDIO.o

_HAL_INTFS_FILES +=	hal/hal_hci/hal_sdio_coex.o
endif

include $(src)/halmac.mk

_BTC_FILES +=		hal/btc/halbtc8723fwifionly.o
ifeq ($(CONFIG_BT_COEXIST), y)
_BTC_FILES +=		hal/btc/halbtccommon.o \
			hal/btc/halbtc8723f.o
endif
