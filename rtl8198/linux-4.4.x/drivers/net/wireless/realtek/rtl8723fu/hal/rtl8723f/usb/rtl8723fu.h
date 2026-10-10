/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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
 *****************************************************************************/
#ifndef _RTL8723FU_H_
#define _RTL8723FU_H_

#include <drv_types.h>		/* PADAPTER, basic_types.h, rtw_xmit.h and etc. */

#define USB_AGG_EN_8723F			BIT(7)

#ifdef CONFIG_LPS_LCLK
/* for CONFIG_LPS_LCLK setting in rtl8723fu_ops.c */
#define REG_USB_HRPWM_8723F		0xFE58
#define REG_USB_HCPWM_8723F		0xFE57
#endif /* CONFIG_LPS_LCLK */

/* rtl8723fu_halinit.c */
u32 rtl8723fu_init(PADAPTER);
u32 rtl8723fu_deinit(PADAPTER);
u32 rtl8723fu_inirp_init(PADAPTER);
u32 rtl8723fu_inirp_deinit(PADAPTER);
void rtl8723fu_interface_configure(PADAPTER);

/* rtl8723fu_halmac.c */
int rtl8723fu_halmac_init_adapter(PADAPTER);

/* rtl8723fu_io.c */
#ifdef CONFIG_RTW_SW_LED
/* rtl8723fu_led.c */
void rtl8723fu_initswleds(PADAPTER);
void rtl8723fu_deinitswleds(PADAPTER);
#endif
/* rtl8723fu_xmit.c */
#define OFFSET_SZ 0
#define MAX_TX_AGG_PACKET_NUMBER_8723F 0xff

s32 rtl8723fu_init_xmit_priv(PADAPTER);
void rtl8723fu_free_xmit_priv(PADAPTER);
s32 rtl8723fu_mgnt_xmit(PADAPTER, struct xmit_frame *);
s32 rtl8723fu_hal_xmit(PADAPTER, struct xmit_frame *);
#ifdef CONFIG_RTW_MGMT_QUEUE
s32 rtl8723fu_hal_mgmt_xmitframe_enqueue(PADAPTER, struct xmit_frame *);
#endif
s32 rtl8723fu_hal_xmitframe_enqueue(PADAPTER, struct xmit_frame *);
s32 rtl8723fu_hostap_mgnt_xmit_entry(PADAPTER, _pkt *);
#ifdef CONFIG_XMIT_THREAD_MODE
s32 rtl8723fu_xmit_buf_handler(PADAPTER);
#endif /* CONFIG_XMIT_THREAD_MODE */

/* rtl8723fu_recv.c */
int rtl8723fu_init_recv_priv(PADAPTER);
void rtl8723fu_free_recv_priv(PADAPTER);

#endif /* _RTL8723FU_H_ */
