/*
 * Copyright (C) 2019 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : PHY 8226 Driver
 *
 * Feature : PHY 8226 Driver
 *
 */
#ifndef __NIC_RTL8226_TYPEDEF_H__
#define __NIC_RTL8226_TYPEDEF_H__

/* from typedef.h and rtl8156_mmd.h */

#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000

#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif

#ifndef uint64
#define uint64 unsigned long long
#endif
#ifndef int64
#define int64 signed long long
#endif
#ifndef uint32
#define uint32 unsigned int
#endif
#ifndef int32
#define int32 signed int
#endif
#ifndef uint16
#define uint16 unsigned short
#endif
#ifndef int16
#define int16 signed short
#endif
#ifndef uint8
#define uint8 unsigned char
#endif
#ifndef int8
#define int8 signed char
#endif

#define SUCCESS     TRUE
#define FAILURE     FALSE



typedef struct {
    uint32      unit;
    uint32      port;
    uint32      gpio;
} HANDLE;


#ifndef bool
  #define bool          int
#endif

#define BOOLEAN         bool
#define BOOL            uint32
#define UINT32          uint32
#define UINT16          uint16
#define UINT8           uint8
#define Sleep(_t)       osal_time_udelay(_t*1000)
#define IN
#define OUT


#define MMD_PMAPMD     1
#define MMD_PCS        3
#define MMD_AN         7
#define MMD_VEND1      30   /* Vendor specific 2 */
#define MMD_VEND2      31   /* Vendor specific 2 */


typedef struct
{
    UINT16 dev;
    UINT16 addr;
    UINT16 value;
} MMD_REG;


BOOLEAN
MmdPhyRead(
    IN  HANDLE hDevice,
    IN  UINT16 dev,
    IN  UINT16 addr,
    OUT UINT16 *data);

BOOLEAN
MmdPhyWrite(
    IN HANDLE hDevice,
    IN UINT16 dev,
    IN UINT16 addr,
    IN UINT16 data);



#define RTL8226B_MSG(fmt,args...)	printk(KERN_INFO "rtl8226b: "fmt"\n", ##args)
#define RTL8226B_INFO(fmt,args...)	printk(KERN_INFO "\033[1;33;46m""rtl8226b: "fmt"\033[m""\n", ##args)
#define RTL8226B_ERROR(fmt,args...)	printk(KERN_INFO "\033[1;33;41m""rtl8226b: "fmt"\033[m""\n", ##args)
#define phy_osal_printf			RTL8226B_INFO
//#define RTL8226B_PHY_DEBUG
#ifdef RTL8226B_PHY_DEBUG
#define RTL8226B_DEBUG			RTL8226B_INFO
#else
#define RTL8226B_DEBUG(fmt,args...)
#endif
//BOOLEAN init_mdio_lock(void);

#endif /* __NIC_RTL8226_TYPEDEF_H__ */


