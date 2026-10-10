/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#ifndef _HALMAC_8723F_CFG_H_
#define _HALMAC_8723F_CFG_H_

#include "../../halmac_hw_cfg.h"
#include "../halmac_87xx_cfg.h"

#if HALMAC_8723F_SUPPORT

#define TX_FIFO_SIZE_8723F	32768
#define RX_FIFO_SIZE_8723F	16384
#define TRX_SHARE_SIZE0_8723F	8192
#define TRX_SHARE_SIZE1_8723F	8192
#define TRX_SHARE_SIZE2_8723F	8192

#define TX_FIFO_SIZE_LA_8723F	(TX_FIFO_SIZE_8723F >>  1)
#define TX_FIFO_SIZE_RX_EXPAND_1BLK_8723F	\
	(TX_FIFO_SIZE_8723F - TRX_SHARE_SIZE0_8723F)
#define RX_FIFO_SIZE_RX_EXPAND_1BLK_8723F	\
	(RX_FIFO_SIZE_8723F + TRX_SHARE_SIZE0_8723F)
#define TX_FIFO_SIZE_RX_EXPAND_2BLK_8723F	\
	(TX_FIFO_SIZE_8723F - TRX_SHARE_SIZE2_8723F)
#define RX_FIFO_SIZE_RX_EXPAND_2BLK_8723F	\
	(RX_FIFO_SIZE_8723F + TRX_SHARE_SIZE2_8723F)
#define TX_FIFO_SIZE_RX_EXPAND_3BLK_8723F	\
	(TX_FIFO_SIZE_8723F - TRX_SHARE_SIZE2_8723F - TRX_SHARE_SIZE0_8723F)
#define RX_FIFO_SIZE_RX_EXPAND_3BLK_8723F	\
	(RX_FIFO_SIZE_8723F + TRX_SHARE_SIZE2_8723F + TRX_SHARE_SIZE0_8723F)
#define TX_FIFO_SIZE_RX_EXPAND_4BLK_8723F	\
	(TX_FIFO_SIZE_8723F - (2 * TRX_SHARE_SIZE2_8723F))
#define RX_FIFO_SIZE_RX_EXPAND_4BLK_8723F	\
	(RX_FIFO_SIZE_8723F + (2 * TRX_SHARE_SIZE2_8723F))

#define EFUSE_SIZE_8723F	512
#define EEPROM_SIZE_8723F	768
#define BT_EFUSE_SIZE_8723F	128
#define PRTCT_EFUSE_SIZE_8723F	124

#define SEC_CAM_NUM_8723F	32

#define OQT_ENTRY_AC_8723F	32
#define OQT_ENTRY_NOAC_8723F	32
#define MACID_MAX_8723F		128

#define WLAN_FW_IRAM_MAX_SIZE_8723F	65536
#define WLAN_FW_DRAM_MAX_SIZE_8723F	65536
#define WLAN_FW_ERAM_MAX_SIZE_8723F	131072
#define WLAN_FW_MAX_SIZE_8723F		(WLAN_FW_IRAM_MAX_SIZE_8723F + \
	WLAN_FW_DRAM_MAX_SIZE_8723F + WLAN_FW_ERAM_MAX_SIZE_8723F)

#endif /* HALMAC_8723F_SUPPORT*/

#endif