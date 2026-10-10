/*****************************************************************************
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @file    eth_port.c
 * @brief   Cortina Ethernet Port APIs implementation
 * @version 1.1
 * @date    2018-09-28
 *  history 2019-01-23: add EEE and DOS-Attack APIs
 *
 *****************************************************************************/
#include "ca_types.h"
#include "osal_common.h"
#include "ca-scfg/scfg.h"
#include "ca_event.h"
#include "aal_common.h"
#include "aal_ni_l2.h"
#include "aal_arb.h"
#include "aal_ni.h"
#include "aal_phy.h"
#include "aal_l3_te.h"
#if defined(CONFIG_NE_AAL_XFI)
#include "aal_xfi.h"
#endif
#include "api_common.h"
#include "port.h"
#include "eth_port.h"
#include "sys.h"

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
#include "aal_l2_te.h"
#endif

#ifdef RTK_MEM_CORRUPTTION_FIX
#include<linux/kernel.h>
#endif
/* UNI LED control: SoC support different speed LED control for each UNI port, 
 * once the Hardware use this feature (i.e. Hardware doesn't use PHY Chip to control LED)
 * the device-tree will include the LED config, and software will trigger the LED behavior
 */

#if defined(CONFIG_ARCH_RTL8198F) && defined(CONFIG_RTL_83XX_SUPPORT)
#include "../ni-drv/ca_ni.h"
#endif

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
#if !defined(CONFIG_CORTINA_BOARD_FPGA) && defined(CONFIG_LEDS_TRIGGER_CA_PHY)
#include <linux/ledtrig-ca-phy.h>

#define  LED_SPEED_SUPPORT 1
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
#define MAX_AAL_PPORT_VALUE  AAL_PPORT_NI4
#else
#define MAX_AAL_PPORT_VALUE  AAL_PPORT_NI3
#endif

#ifdef LED_SPEED_SUPPORT
#define __ETH_LED_ON(port,speed)   do { \
    if (port <= MAX_AAL_PPORT_VALUE) { \
        if ((speed) >= AAL_PHY_SPEED_1000) { \
            ledtrig_ca_phy_ctrl(port, PHY_HIGH_SPEED, 1); \
        } else { \
            ledtrig_ca_phy_ctrl(port, PHY_LOW_SPEED, 1); \
        } \
    }}while(0)
#else
#define __ETH_LED_ON(port,speed)   do { ledtrig_ca_phy_ctrl(port, PHY_NORMAL_SPEED, 1);}while(0)
#endif /* LED_SPEED_SUPPORT */

#define __ETH_LED_OFF(port)  do {\
            if (port <= MAX_AAL_PPORT_VALUE) {\
                ledtrig_ca_phy_ctrl(port, PHY_LOW_SPEED, 0);\
            }}while(0)

#else

#define __ETH_LED_ON(port,speed)
#define __ETH_LED_OFF(port)

#endif /* !defined(CONFIG_CORTINA_BOARD_FPGA) && defined(CONFIG_LEDS_TRIGGER_CA_PHY) */

#else //defined(ONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)

#if !defined(CONFIG_CORTINA_BOARD_FPGA) && defined(CONFIG_LEDS_TRIGGER_CA_PHY)
#include <linux/ledtrig-ca-phy.h>

#define  LED_SPEED_SUPPORT 1

#ifdef LED_SPEED_SUPPORT
#define __ETH_LED_ON(port,speed)   do { \
    if (port <= AAL_PPORT_NI3) { \
        if ((speed) >= AAL_PHY_SPEED_1000) { \
            ledtrig_ca_phy_ctrl(port, PHY_HIGH_SPEED, 1); \
        } else { \
            ledtrig_ca_phy_ctrl(port, PHY_LOW_SPEED, 1); \
        } \
    }}while(0)
#else
#define __ETH_LED_ON(port,speed)   do { ledtrig_ca_phy_ctrl(port, PHY_NORMAL_SPEED, 1);}while(0)
#endif /* LED_SPEED_SUPPORT */

#define __ETH_LED_OFF(port)  do {\
            if (port <= AAL_PPORT_NI3) {\
                ledtrig_ca_phy_ctrl(port, PHY_LOW_SPEED, 0);\
            }}while(0)

#else

#define __ETH_LED_ON(port,speed)
#define __ETH_LED_OFF(port)

#endif /* !defined(CONFIG_CORTINA_BOARD_FPGA) && defined(CONFIG_LEDS_TRIGGER_CA_PHY) */
#endif


/******************************************************************************/
/*                             Import Variables and Functions                 */
/******************************************************************************/
extern int ni_initialized; /* ni driver init done flag */

#if  defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
extern ca_uint32_t ca_plat_glb_reg_read(ca_device_id_t device_id, ca_uint32_t addr);
extern void ca_plat_glb_reg_write(ca_device_id_t device_id, ca_uint32_t addr, ca_uint32_t data);
#define __GLB_REG_READ(addr)        ca_plat_glb_reg_read(__ETH_DEV_LOCAL, addr)
#define __GLB_REG_WRITE(data,addr)  ca_plat_glb_reg_write(__ETH_DEV_LOCAL, addr, data)
#endif

/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/

/* Local MACROs */
#define __ETH_DEV_LOCAL                0 /* Local device_id */
#define __MAX_ETH_PORT_NUM             CA_ETH_PORT_NUM /* MAX Ethernet port number per Chip */
#define __MAX_ETH_PORT_ID              (__MAX_ETH_PORT_NUM-1) /* MAX Ethernet port index per Chip */

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
#define __ETH_PORT_LINK_SCAN_INTERVAL  1000 /* 1 seconds */
#else
#define __ETH_PORT_LINK_SCAN_INTERVAL  300 /* 0.3 seconds */
#endif
#define __ETH_PORT_LOCK(dev)        ca_spin_lock(g_eth_port_drv.eth_port_lock)
#define __ETH_PORT_UNLOCK(dev)      ca_spin_unlock(g_eth_port_drv.eth_port_lock)
#define __ETH_PHY_ADDR(aal_port)    (g_eth_port_drv.eth_mdio_addr[aal_port])
#define __ETH_EEE_IS_ENABLE(aal_port)  g_eth_port_drv.eee_enable[aal_port]
#define __ETH_DETECTED_PHY(aal_port)  (g_eth_port_drv.ext_phy_detected[aal_port])/* check PHY existance by AAL port */
#define __ETH_PORT_SCAN_IS_ENABLE(aal_port)  (0x1 & (g_eth_port_drv.eth_port_scan_enable >> (aal_port)))
#define __ETH_PORT_IS_VALID(aal_port)        ((0x1<<(aal_port)) & g_eth_port_drv.eth_port_bmp)

#define __FOR_ALL_ETH_PORTS_DO(port)   \
        for((port)=0; (port)<=__MAX_ETH_PORT_ID; ++(port)) \
            if ((1<<port) & g_eth_port_drv.eth_port_bmp)

#define __FOR_ALL_GE_PORTS_DO(port)   \
        for((port)=0; (port)<=AAL_NI_GE_PORT_END; ++(port)) \
            if ((1<<port) & g_eth_port_drv.ge_port_bmp)

#define __FOR_ALL_XE_PORTS_DO(port)   \
        for((port)=0; (port)<=__MAX_ETH_PORT_ID; ++(port)) \
            if ((1<<port) & g_eth_port_drv.xe_port_bmp)

#define __FOR_ALL_MDIO_DO(port)   \
        for((port)=1; (port)<=CA_MDIO_ADDR_MAX; ++(port))

/* Sanity check for ETH port parameter */
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define __ETH_PORT_CHK(port)      do {\
        if (!(CA_PORT_IS_ETHERNET(port) && ((1 << PORT_ID(port)) & g_eth_port_drv.eth_port_bmp))\
            && !((CA_PORT_IS_PON(port) && (PORT_ID(port) == AAL_LPORT_ETH_WAN)))){\
            __ETH_DBG("%s(): invalid port %#x\r\n", __func__, port);\
            return CA_E_PARAM;\
        }}while(0)

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __ETH_PORT_CHK(port)      do {\
    if (!(CA_PORT_IS_ETHERNET(port) && ((1 << PORT_ID(port)) & g_eth_port_drv.eth_port_bmp) && (PORT_ID(port) < AAL_NI_INTERNAL_XE))\
        && !(CA_PORT_IS_PON(port))){\
        __ETH_DBG("%s(): invalid port %#x\r\n", __func__, port);\
        return CA_E_PARAM;\
    }}while(0)

#else
#define __ETH_PORT_CHK(port)      do {\
        if (!CA_PORT_IS_ETHERNET(port)) {\
            __ETH_DBG("%s(): invalid ethernet port %#x\r\n", __func__, port);\
            return CA_E_PARAM;\
        }}while(0)
#endif

/* Check ETH driver initialization state */
#define __ETH_INIT_CHK(dev)  do {\
                                if (!g_eth_port_init_done) {\
                                    ca_printf("ETH Port API is NOT initialized ready\r\n"); \
                                    return CA_E_INIT; \
                                }\
                             }while(0)

/* ETH Debug MACROs */
#define __ETH_DBG(fmt,...)  do {\
                                if (ca_api_debug & API_DBG_ETH_PORT)\
                                    ca_printf("%s():"fmt, __func__,##__VA_ARGS__);\
                            }while(0)

#define __ETH_INIT_PRINT(fmt,...)    ca_printf("ca-ne: "fmt,##__VA_ARGS__)

#if defined(CONFIG_NE_AAL_XFI)

#if  defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __ETH_IS_XFI(aal_port)   ((aal_port) >= AAL_NI_PORT_XGE1 && (aal_port) <= AAL_NI_PORT_XGE2)
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define __ETH_IS_XFI(aal_port)   ((aal_port) >= AAL_NI_PORT_XGE0 && (aal_port) <= AAL_NI_PORT_XGE1)
#else
#define __ETH_IS_XFI(aal_port)   (FALSE)
#endif

#endif /* CONFIG_NE_AAL_XFI */

/* Ethernet interface types */
#define __ETH_INTF_IS_USXGMII(aal_port)  (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_USXGMII)
#define __ETH_INTF_IS_SGMII(aal_port)    (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_GE_SGMII)
#define __ETH_INTF_IS_HISGMII(aal_port)  (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_HISGMII)
#define __ETH_INTF_IS_XFI(aal_port)      (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_XFI)
#define __ETH_INTF_IS_RGMII(aal_port)    (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_GE_RGMII)

/* Internal Ethernet MAC is XGE MAC or GE MAC */
#define __ETH_IS_XGE(aal_port)  ((aal_port) >= AAL_NI_PORT_XGE0 && (aal_port) <= AAL_NI_PORT_XGE2)
#define __ETH_IS_GE(aal_port)   ((aal_port) >= AAL_NI_PORT_GE0 && (aal_port) <= AAL_NI_PORT_GE4)

/* Ethernet MAC port default speed for initialization */
#ifdef CONFIG_CORTINA_BOARD_FPGA
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
    AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
};
#elif defined(CONFIG_ARCH_CORTINA_G3LITE)
#if  defined(CONFIG_ARCH_RTL8198F)
#define __ETH_IS_CONNECT_SWITCH(aal_port)  ((aal_port) == HOST_GMII_PORT)
    #if defined(CONFIG_RTL_8367S_SUPPORT)
    ca_uint32_t __default_mac_speed[] = { 
    	AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
    	AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
    };
    #elif defined(CONFIG_RTL_8367R_SUPPORT)
    ca_uint32_t __default_mac_speed[] = { 
    	AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
    	AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
    };
    #else
    ca_uint32_t __default_mac_speed[] = {
        AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
        AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
	};
    #endif	
#else
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
};
#endif
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M,
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_1000M
};
#else
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M,
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_10G
};
#endif

/* !NOTE: the following is Hardware related MACROs, NOT for SoC chipset,
 *        if your hardware design use switch chip to the SoC chipset, please
 *        modify the SCFG related fields like BOARD_INFO, and then modify the
 *        following MACROs, it means the following MACROs are for customer edit
 */
#ifdef CONFIG_CORTINA_BOARD_FPGA
#define __ETH_IS_CONNECT_SWITCH(aal_port)  (FALSE)
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
#define __ETH_IS_CONNECT_SWITCH(aal_port)  (FALSE)
#elif defined(CONFIG_ARCH_CORTINA_G3LITE)
#define __ETH_IS_CONNECT_SWITCH(aal_port)  ((aal_port) == AAL_NI_PORT_GE4)
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __ETH_IS_CONNECT_SWITCH(aal_port)  ((((aal_port) == AAL_NI_PORT_GE4) && __board_connect_ext_switch()) || (((aal_port) == AAL_NI_PORT_XGE1) && board_use2port_connect_ext_switch()))
#else
#define __ETH_IS_CONNECT_SWITCH(aal_port)  (FALSE)
#endif

/* API speed convert to AAL PHY speed */
#define _SPEED2PHY(speed)     ((speed) <= CA_PORT_SPEED_AUTO  ? __api2phy_speed_map[speed] : AAL_PHY_SPEED_10)
#define _PHY2SPEED(phy_speed) ((phy_speed) <= AAL_PHY_SPEED_5000  ? __phy2api_speed_map[phy_speed] : CA_PORT_SPEED_AUTO)

/* API speed convert to AAL MAC speed */
#define _SPEED2MAC(speed)     ((speed) > CA_PORT_SPEED_INVALID  ? (speed) - 1: (speed))
#define _MAC2SPEED(mac_speed) ((mac_speed)+1)

/* AAL PHY speed convert to AAL MAC speed */
#define _PHY_SPEED2MAC_SPEED(phy_speed) _SPEED2MAC(_PHY2SPEED(phy_speed))
#define _MAC_SPEED2PHY_SPEED(mac_speed) _SPEED2PHY(_MAC2SPEED(mac_speed))

/******************************************************************************/
/*                             Local global Var                               */
/******************************************************************************/
typedef struct {
    ca_uint      eth_port_lock; /* driver spin lock */
    ca_boolean_t eth_xfi_autoreset; /* auto reset XFI when no link received */
    ca_boolean_t eth_port_scan_stop; /* timer or thread id */
    ca_uint8_t   eth_dynamic_deep_queue; /* enable dynamic deep Queue feature or NOT for etherent ports */
    ca_uint32_t  eth_port_scan_enable; /* port scan bitmap */
    ca_uint32_t  ge_port_bmp; /* GE port bitmap */
    ca_uint32_t  xe_port_bmp; /* XE port bitmap */
    ca_uint32_t  eth_port_bmp; /* port bitmap */
    ca_uint8_t   port_enable[__MAX_ETH_PORT_NUM]; /* port admin state */
    ca_uint8_t   eth_mdio_addr[__MAX_ETH_PORT_NUM]; /* port PHY MIDO addr, if it is 0xff, means valid */
    ca_uint8_t   ext_phy_detected[__MAX_ETH_PORT_NUM]; /* port connected to external PHY or NOT */
    ca_uint8_t   disable_phy2mac_sync[__MAX_ETH_PORT_NUM]; /* disable PHY to MAC auto-sync*/
    ca_uint8_t   eth_if_mode[__MAX_ETH_PORT_NUM]; /* port PMD interface */
    ca_uint8_t   autoneg[__MAX_ETH_PORT_NUM]; /* port autoneg configure state */
    ca_uint8_t   speed[__MAX_ETH_PORT_NUM]; /* port speed configure state */
    ca_uint8_t   eee_enable[__MAX_ETH_PORT_NUM];  /* port EEE enable state */
    ca_eth_port_ability_t  adv[__MAX_ETH_PORT_NUM]; /* port advertize configure state */
} __eth_port_drv_t;

ca_uint8_t g_eth_port_init_done = 0;/* ETH drv init successfully */
__eth_port_drv_t g_eth_port_drv;

static const char *speed_str[] = {
    "unk",
     "10M",
     "100M",
     "1G",
     "2.5G",
     "10G",
     "5G",
     "auto"
    };
static const char *duplex_str[] = {
    "half",
    "full",
    "auto"
    };
static const char *intf_str[] = {
    "GMII",
    "MII",
    "RGMII-GE",
    "RGMII-FE",
    "TBI",
    "RMII",
    "XFI",
    "SSSMII",
    "SXGMII",
    "SGMII",
    "HISGMII",
    "RXAUI",
    "USXGMII"
    };
static const char *stp_str[] = {
    "disable",
    "block",
    "learn",
    "forward"
    };

static ca_eth_port_speed_t __phy2api_speed_map[] = {
    CA_PORT_SPEED_10M, CA_PORT_SPEED_100M, CA_PORT_SPEED_1G, 
    CA_PORT_SPEED_10G, CA_PORT_SPEED_AUTO, CA_PORT_SPEED_2_5G, CA_PORT_SPEED_5G
};

static aal_phy_speed_mode_t __api2phy_speed_map[] = {
    AAL_PHY_SPEED_10, AAL_PHY_SPEED_10, AAL_PHY_SPEED_100, AAL_PHY_SPEED_1000,
    AAL_PHY_SPEED_2500, AAL_PHY_SPEED_10000, AAL_PHY_SPEED_5000, AAL_PHY_SPEED_10000 
};

/******************************************************************************/
/*                       Local Function Implementation                     */
/******************************************************************************/

/**__eth_if_str_get
 *@brief: Get interface naming string by interface mode
 * */
static const char * __eth_if_str_get(ca_uint8_t  if_mode)
{
    if (if_mode < sizeof(intf_str) / sizeof(intf_str[0]))
        return intf_str[if_mode];
    else
        return "Invalid";
}

/**__board_connect_ext_switch
 *@brief: identify SoC connect to external switch chipset, only will be used in SFU/HGU/G3
 *
 * !NOTE: if hardware design is changed, please change this function
 */
static ca_boolean_t __board_connect_ext_switch(void)
{
#ifdef CONFIG_ARCH_CORTINA_G3LITE
    return CA_E_ERROR;
#else
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  board_info = 0;
    ca_uint32_t  custer_bit = 0;
    ret = scfg_read( 0, CFG_ID_CUSTOMER_BIT, sizeof(ca_uint32), (ca_uint8_t *)&custer_bit); 
    ret |= scfg_read( 0, CFG_ID_BOARD_INFO_BIT, sizeof(ca_uint32), (ca_uint8_t *)&board_info);
    if ((CA_E_OK == ret) && (custer_bit&CUSTOMER_BIT_13) &&
        ((BOARD_INFO_BIT_1 ==  board_info)
        	|| (BOARD_INFO_BIT_3 == board_info)
        	|| (BOARD_INFO_BIT_4 == board_info))){
        /* TODO: once your board connected to ext switch, please add BOARD_INFO_BIT_XXX == board info here !*/
        return TRUE;
    }
    return FALSE;
#endif
}

/**board_use2port_connect_ext_switch
 *@brief: identify SoC use port 4, 6 to connect to external switch chipset
 *
 * !NOTE: if hardware design is changed, please change this function
 */
int  board_use2port_connect_ext_switch( void )
{
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  board_info = 0;
    ca_uint32_t  custer_bit = 0;     
    ret = scfg_read( 0, CFG_ID_CUSTOMER_BIT, sizeof(ca_uint32), (ca_uint8_t *)&custer_bit); 
    ret |= scfg_read( 0, CFG_ID_BOARD_INFO_BIT, sizeof(ca_uint32), (ca_uint8_t *)&board_info);
    if ((CA_E_OK == ret) && (custer_bit&CUSTOMER_BIT_13) &&
        ((BOARD_INFO_BIT_1 ==  board_info)
        || (BOARD_INFO_BIT_3 == board_info))) {        
        __ETH_DBG("port 4,6 external to switch\r\n");
        return TRUE;
    } else {
        return FALSE;
    }
}

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
/**__eth_port_gphy_speed_sync
 * @brief: SoC's port4 support RGMII/SGMII/HSGMII with the same SerDes, here sync the Fiber/Copper speed to PCS
 */
static void __eth_port_gphy_speed_sync(ca_device_id_t device_id)
{
    if (__ETH_INTF_IS_SGMII(AAL_NI_PORT_GE4) || 
        __ETH_INTF_IS_RGMII(AAL_NI_PORT_GE4) ||
        __ETH_INTF_IS_HISGMII(AAL_NI_PORT_GE4) ) { /* sync speed to PCS */

        GLOBAL_PHY_SGMII_PCS_CONTROL_t pcs_ctrl;
        GLOBAL_PHY_SGMII_PCS_INFO_t    pcs_info;

        pcs_info.wrd = __GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_INFO);
        pcs_ctrl.wrd = __GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);

        if (!pcs_info.bf.sgmii_sds_link_ok) {
            return;
        }
        __ETH_DBG("GLOBAL_PHY_SGMII_PCS_INFO = %#x\r\n", pcs_info.wrd);

       /*
       #//////////////////////////////////
       # SGMII 10Mbps
       # sgmii_cf_spd0x [24:22]=0x0
       # sgmii_cf_sds_mode0 [6:2]=0x2
       # sgmii_cf_sds0_ablty [21:9]=0x0
       #devmem 0xf4320034 w32 0x02000008
       #//////////////////////////////////
       # SGMII 100Mbps
       # sgmii_cf_spd0x [24:22]=0x1
       # sgmii_cf_sds_mode0 [6:2]=0x2
       # sgmii_cf_sds0_ablty [21:9]=0x1
       #devmem 0xf4320034 w32 0x02400208
       #//////////////////////////////////
       # SGMII 1Gbps
       # sgmii_cf_spd0x [24:22]=0x2
       # sgmii_cf_sds_mode0 [6:2]=0x2
       # sgmii_cf_sds0_ablty [21:9]=0x2
       devmem 0xf4320034 w32 0x02800408
       #//////////////////////////////////
       # HSGMII
       # sgmii_cf_spd0x [24:22]=0x5
       # sgmii_cf_sds_mode0 [6:2]=0x12
       # sgmii_cf_sds0_ablty [21:9]=0x1001
       #devmem 0xf4320034 w32 0x03600248
       #///////////////////////////////////
       */

        pcs_info.bf.sgmii_sds0_link_partner_ablty &= 0x1003;

        switch (pcs_info.bf.sgmii_sds0_link_partner_ablty) {
        case 0x0:
            pcs_ctrl.bf.sgmii_cf_spd0x = 0x0;
            pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
            pcs_ctrl.bf.sgmii_sds0_ablty = 0x0;
            break;
        case 0x1:
            pcs_ctrl.bf.sgmii_cf_spd0x = 0x1;
            pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
            pcs_ctrl.bf.sgmii_sds0_ablty = 0x1;
            break;
        case 0x2:
            pcs_ctrl.bf.sgmii_cf_spd0x = 0x2;
            pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
            pcs_ctrl.bf.sgmii_sds0_ablty = 0x2;
            break;
        case 0x1001:
            pcs_ctrl.bf.sgmii_cf_spd0x = 0x5;
            pcs_ctrl.bf.sgmii_sds_mode0 = 0x12;
            pcs_ctrl.bf.sgmii_sds0_ablty = 0x1001;
            break;
        default:
            break;
        }
        __GLB_REG_WRITE(pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
        __ETH_DBG(" changed GLOBAL_PHY_SGMII_PCS_CONTROL = %#x\r\n", __GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL));
    }
}

/**__eth_port_gphy_ctrl_init
 * @brief: SoC's port4 support RGMII/SGMII/HSGMII with the same SerDes, here initialize it according to SCFG
 */
static void __eth_port_gphy_ctrl_init(ca_device_id_t device_id)
{
/*
HSGMII Analog Parameter Modify
0xF433A404 0x0000E05C --> 0x0000E050 --> CDR
0xF433A418 0x00000011 --> 0x0000001F --> CMU
0xF433A428 0x0000C601 --> 0x0000F601 --> CMU
0xF433A480 0x000094AA --> 0x0000D466 --> TX  Driver
 
SGMII Analog Parameter Modify
0xF433A204 0x0000E05C --> 0x0000E070 --> CDR
0xF433A218 0x00000015 --> 0x0000001F --> CMU
0xF433A228 0x0000C600 --> 0x0000F600 --> CMU
0xF433A280 0x000094AA --> 0x0000D466 --> TX  Driver

# GLOBAL_PHY_SGMII_PCS_CONTROL, [25] sgmii_spdup_ini=1
#//////////////////////////////////
# SGMII 10Mbps
# sgmii_cf_spd0x [24:22]=0x0
# sgmii_cf_sds_mode0 [6:2]=0x2
# sgmii_cf_sds0_ablty [21:9]=0x0
#devmem 0xf4320034 w32 0x02000008
#//////////////////////////////////
# SGMII 100Mbps
# sgmii_cf_spd0x [24:22]=0x1
# sgmii_cf_sds_mode0 [6:2]=0x2
# sgmii_cf_sds0_ablty [21:9]=0x1
#devmem 0xf4320034 w32 0x02400208
#//////////////////////////////////
# SGMII 1Gbps
# sgmii_cf_spd0x [24:22]=0x2
# sgmii_cf_sds_mode0 [6:2]=0x2
# sgmii_cf_sds0_ablty [21:9]=0x2
devmem 0xf4320034 w32 0x02800408
#//////////////////////////////////
# HSGMII
# sgmii_cf_spd0x [24:22]=0x5
# sgmii_cf_sds_mode0 [6:2]=0x12
# sgmii_cf_sds0_ablty [21:9]=0x1001
#devmem 0xf4320034 w32 0x03600248
#///////////////////////////////////
*/
    GLOBAL_PHY_CONTROL_t           gphy_ctrl;
    GLOBAL_PHY_SGMII_PCS_CONTROL_t pcs_ctrl;

    __ETH_DBG("GLOBAL_DPHY_RESET = %#x\r\n", __GLB_REG_READ(GLOBAL_DPHY_RESET));
    gphy_ctrl.wrd = __GLB_REG_READ(GLOBAL_PHY_CONTROL);
    pcs_ctrl.wrd = __GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
    __ETH_DBG("GLOBAL_PHY_CONTROL = %#x\r\n", __func__, gphy_ctrl.wrd);
    __ETH_DBG("GLOBAL_PHY_SGMII_PCS_CONTROL = %#x\r\n", pcs_ctrl.wrd);

    if (!gphy_ctrl.bf.s0_rxaui_mode) {
        g_eth_port_drv.xe_port_bmp  &= ~(0x0001 << AAL_NI_XAUI);
        g_eth_port_drv.eth_port_bmp &= ~(0x0001 << AAL_NI_XAUI);
        __ETH_DBG("XAUI port is used for PCIE on the board, disable XAUI MAC\r\n");
    }

    if (g_eth_port_drv.eth_if_mode[AAL_NI_PORT_GE4] == AAL_NI_IF_MODE_HISGMII) {
        pcs_ctrl.bf.sgmii_sds_mode0  = 0x12; /* 0x02: SGMII mode 0x12: HISGMII mode*/
        pcs_ctrl.bf.sgmii_cf_spd0x   = 0x5; /* Speed configuration:000 : 10Mbps001 : 100Mbps010 : 1Gbps011 : 500Mbps101 : 2.5Gbps*/
        pcs_ctrl.bf.sgmii_sds0_ablty = 0x1001;
        CA_NE_REG_WRITE(0x0000E050, 0xF433A404);
        CA_NE_REG_WRITE(0x0000001F, 0xF433A418);
        CA_NE_REG_WRITE(0x0000F601, 0xF433A428);
        CA_NE_REG_WRITE(0x0000D466, 0xF433A480);
        
        g_eth_port_drv.speed[AAL_NI_PORT_GE4] = (ca_uint8_t)CA_PORT_SPEED_2_5G;
    } else if (g_eth_port_drv.eth_if_mode[4] == AAL_NI_IF_MODE_GE_RGMII || g_eth_port_drv.eth_if_mode[4] == AAL_NI_IF_MODE_GE_SGMII) {
        pcs_ctrl.bf.sgmii_sds_mode0  = 0x2; /* 0x02: SGMII mode 0x12: HISGMII mode*/
        pcs_ctrl.bf.sgmii_cf_spd0x   = 0x2; /* Speed configuration:000 : 10Mbps001 : 100Mbps010 : 1Gbps011 : 500Mbps101 : 2.5Gbps*/
        pcs_ctrl.bf.sgmii_sds0_ablty = 0x2;
        CA_NE_REG_WRITE(0x0000E070, 0xF433A404);
        CA_NE_REG_WRITE(0x0000001F, 0xF433A418);
        CA_NE_REG_WRITE(0x0000F600, 0xF433A428);
        CA_NE_REG_WRITE(0x0000D466, 0xF433A480);
        g_eth_port_drv.speed[AAL_NI_PORT_GE4] = (ca_uint8_t)CA_PORT_SPEED_1G;
    } else {
        return;
    }
    __GLB_REG_WRITE(pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);

}
#endif	


/**__xge_intf_link_status_get
 * @brief: get the XGE port's interface link status
 */
static ca_status_t __xge_intf_link_status_get(
    ca_device_id_t device_id,
    ca_uint32_t aal_port,
    aal_phy_link_status_t *status)
{
    ca_status_t  ret = CA_E_OK;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)

    if (aal_port == AAL_NI_PORT_XGE0) {
        aal_ni_xaui_xgxs_intr_status_t  xaui_status;

        memset(&xaui_status, 0, sizeof(xaui_status));

        ret = aal_ni_xaui_xgxs_intr_status_get(
            device_id, &xaui_status);
        if (CA_E_OK == ret) {
            status->link_up = xaui_status.Tx_xgxs_align_det & xaui_status.Tx_SyncDet0 & xaui_status.Tx_SyncDet1 & xaui_status.Tx_SyncDet2 & xaui_status.Tx_SyncDet3;
            status->duplex = TRUE;
            status->speed  = AAL_PHY_SPEED_10000;
        }
    } else {
        ca_boolean_t  linkup = 0;
		ca_uint32_t  speed  = AAL_PHY_SPEED_10000;

#if defined(CONFIG_NE_AAL_XFI)
		ret = aal_xfi_speed_get(
			device_id,
			aal_port - AAL_NI_PORT_XGE1,
			&speed);
		if(CA_E_OK == ret) {
			status->speed = (aal_phy_speed_mode_t)speed;
		}
#endif
		if(speed == AAL_PHY_SPEED_1000) {
#if defined(CONFIG_NE_AAL_XFI)
			ret = aal_xfi_link_status_get(
				device_id,
				aal_port - AAL_NI_PORT_XGE1,
				&linkup);
#endif
			status->link_up = linkup;
		} else if(speed == AAL_PHY_SPEED_10000) {
			ret = aal_ni_xge_sxgra_sxgsts_get(
				device_id,
				aal_port - AAL_NI_PORT_XGE1,
				&linkup);
		} else {
			ret == CA_E_PARAM;
			goto ERROR_XGE_INTF_LINK_STATUS_GET;
		}

        if (CA_E_OK == ret) {
            status->link_up = linkup;
            status->duplex = TRUE;
			status->speed  = speed;
		}
        }
#elif  defined( CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS )
	if(aal_port >= AAL_NI_PORT_XGE0 && aal_port <= AAL_NI_PORT_XGE1) {
		ca_boolean_t link_up = 0;
		ca_uint32_t  speed   = AAL_PHY_SPEED_10000;
#if defined(CONFIG_NE_AAL_XFI)
		ret = aal_xfi_link_status_get(
			device_id,
			aal_port - AAL_NI_PORT_XGE0,
			&link_up);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_xfi_speed_get(
			device_id,
			aal_port - AAL_NI_PORT_XGE0,
			&speed);
		CA_RET_VALUE_CHECK(ret);
#endif
		status->link_up = link_up;
            status->speed = (aal_phy_speed_mode_t)speed;
		status->duplex = TRUE;
	} else {
		__ETH_DBG("warning: port %d is NOT XGE\n",aal_port);
		return CA_E_PARAM;
    }
#elif  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (aal_port >= AAL_NI_PORT_XGE0 && aal_port <= AAL_NI_PORT_XGE1) {
        ca_boolean_t  link_up = 0;
        ca_uint32_t   speed   = AAL_PHY_SPEED_10000;

#if defined(CONFIG_NE_AAL_XFI)
        ret = aal_xfi_speed_get(device_id,
            aal_port - AAL_NI_PORT_XGE0, &speed);
        CA_RET_VALUE_CHECK(ret);
#endif

		if(speed == AAL_PHY_SPEED_1000) {
#if defined(CONFIG_NE_AAL_XFI)
			ret = aal_xfi_link_status_get(
				device_id,
				aal_port - AAL_NI_PORT_XGE0,
				&link_up);
#endif
		} else if(speed == AAL_PHY_SPEED_10000) {
			ret = aal_ni_xge_sxgra_sxgsts_get(
				device_id,
				aal_port - AAL_NI_PORT_XGE0,
				&link_up);
		} else {
			ret == CA_E_PARAM;
			goto ERROR_XGE_INTF_LINK_STATUS_GET;
		}

		if(ret == CA_E_OK) {
        status->link_up = link_up;
        status->speed = (aal_phy_speed_mode_t)speed;
        status->duplex = TRUE;
		}
    } else {
        __ETH_DBG("warning: port %d is NOT XGE\n", aal_port);
        return CA_E_PARAM;
    }
#endif

ERROR_XGE_INTF_LINK_STATUS_GET:
    return ret;

}

/**__ge_intf_link_status_get
 * @brief: get the GE port's interface link status
 */
static ca_status_t __ge_intf_link_status_get(
	ca_device_id_t device_id,
    ca_uint32_t aal_port,
    aal_phy_link_status_t *status)
{
	ca_status_t  ret = CA_E_OK;
    aal_eth_port_status_t  mac_status;

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
    if (aal_port == AAL_NI_PORT_GE4 &&
        (__ETH_INTF_IS_SGMII(AAL_NI_PORT_GE4) || 
        __ETH_INTF_IS_RGMII(AAL_NI_PORT_GE4) ||
        __ETH_INTF_IS_HISGMII(AAL_NI_PORT_GE4))) {

        GLOBAL_PHY_SGMII_PCS_INFO_t    pcs_info;

        pcs_info.wrd = __GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_INFO);

        status->link_up = pcs_info.bf.sgmii_sds_link_ok & ((pcs_info.bf.sgmii_sds0_link_partner_ablty >> 4) & 0x1); /*bit4 : link ok */
        if (status->link_up) {
            status->speed  = (((pcs_info.bf.sgmii_sds0_link_partner_ablty >> 12) & 0x1) << 2) | (pcs_info.bf.sgmii_sds0_link_partner_ablty  & 0x3);
            status->duplex = (pcs_info.bf.sgmii_sds0_link_partner_ablty >> 2) & 0x1;
        } else {
            status->speed = AAL_PHY_SPEED_10;
            status->duplex = 0;
        }

        return CA_E_OK;
    }
#endif

    if ( g_eth_port_drv.eth_if_mode[aal_port] != AAL_NI_IF_MODE_GE_RGMII &&
        g_eth_port_drv.eth_if_mode[aal_port] != AAL_NI_IF_MODE_FE_RGMII) {
        /* !NOTE: Curernt ASIC ONLY support get MAC status under RGMII mode */
        memset(status, 0, sizeof(aal_phy_link_status_t));
        return CA_E_OK;
    }

    /* Read the status synced from PHY under RGMII mode */
    memset(&mac_status, 0, sizeof(mac_status));
    ret = aal_ni_eth_port_status_get(device_id, aal_port, &mac_status);
    if (CA_E_OK == ret) {
        status->link_up = mac_status.link_status;
        status->duplex = mac_status.duplex;
        status->speed = _MAC_SPEED2PHY_SPEED(mac_status.speed);
    }

        return ret;
    }

/**__eth_link_status_get
 * @brief: get the etherent port's link status
 */
static ca_status_t __eth_link_status_get(
    ca_device_id_t         device_id,
    ca_uint32_t            aal_port,
    aal_phy_link_status_t *link_status)
{
#ifdef RTK_MEM_CORRUPTTION_FIX
    if ( aal_port < __MAX_ETH_PORT_NUM) {
#endif
        if (__ETH_DETECTED_PHY(aal_port)) { /* retrieve link status from external/intergrated PHY */
            return aal_eth_phy_link_status_get(device_id, __ETH_PHY_ADDR(aal_port), link_status);
        } else if (__ETH_IS_XGE(aal_port)) { /* retrieve link status from XGE interface like SerDes or PCS */
            return __xge_intf_link_status_get(device_id, aal_port, link_status);
        } else { /* retrieve link status from GE interface like SerDes or PCS */
            return __ge_intf_link_status_get(device_id, aal_port, link_status);
        }

            return CA_E_OK;
#ifdef RTK_MEM_CORRUPTTION_FIX
    } else {
	    ca_printf("aal_port > __MAX_ETH_PORT_NUM\n ");
	    return CA_E_ERROR;
    }
#endif

			}

#if defined(CONFIG_NE_AAL_XFI)
/**__xge_phy2mac_sync
 * @brief: sync the speed and duplex from external PHY to internal XFI, XE-MAC
 */
static ca_status_t __xge_phy2mac_sync(
    ca_device_id_t device_id,
    ca_port_id_t aal_port,
    aal_phy_link_status_t *link_status)
{

    ca_status_t     ret = CA_E_OK;
    ca_uint32_t     xfi_idx = 0;
    ca_boolean_t    xfi_link = 0;


    xfi_idx = aal_xfi_port2xfi(aal_port);
    if (__ETH_INTF_IS_USXGMII(aal_port)) {
        (void)aal_xfi_autoneg_set(device_id, xfi_idx, 1);
        ret = aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
        __ETH_DBG("USXGMII %d re-autoneg link %s!\n", xfi_idx, xfi_link ? "up" : "down");
    } else {
    (void)aal_xfi_speed_set(device_id, xfi_idx, link_status->speed);
        ca_mdelay(20);
		ret = aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
    if (ret == CA_E_OK && !xfi_link) {
            aal_eth_phy_reset(device_id, __ETH_PHY_ADDR(aal_port));
        ca_mdelay(10);
            aal_xfi_reset(device_id, xfi_idx);
        ca_mdelay(10);
        (void)aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
            __ETH_DBG("WARNING: XFI %d link %s with PHY interface!\n", xfi_idx, xfi_link ? "up" : "down");
        }
	}
	return ret;
}
#endif

/**__ge_phy2mac_sync
 * @brief: sync the speed and duplex from external PHY to internal GE/FE-MAC
 */
static ca_status_t __ge_phy2mac_sync(
	ca_device_id_t device_id,
    ca_port_id_t aal_port,
	aal_phy_link_status_t *link_status)
{
    ca_status_t  ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
	aal_phy_autoneg_adv_t lp_adv;
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
    aal_ni_eth_mac_config_t tmp_eth_mac_config;
    aal_l2_te_policer_cfg_msk_t l2_te_policer_cfg_msk;
    aal_l2_te_policer_cfg_t l2_te_policer_cfg;
#endif
        eth_mac_mask.u32      = 0;
        eth_mac_mask.s.speed  = 1;
        eth_mac_mask.s.duplex = 1;
        memset(&eth_mac_config, 0, sizeof(eth_mac_config));
	memset(&lp_adv,0x00,sizeof( aal_phy_autoneg_adv_t ));
	/*
	**	Get LP's adv_ability
	**	if LP.adv_pause is 1, enable tx_flow_control on mac. 
	**	if LP.adv_pause is 0, disable tx_flow_control on mac. 
	*/
	aal_eth_phy_partner_ability_adv_get(device_id,__ETH_PHY_ADDR(aal_port),&lp_adv);
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
    aal_ni_eth_port_mac_get(device_id, aal_port, &tmp_eth_mac_config);

    l2_te_policer_cfg_msk.u32 = 0;
    memset(&l2_te_policer_cfg,0,sizeof(aal_l2_te_policer_cfg_t));
    l2_te_policer_cfg_msk.s.bypass_red = 1;

    if ( lp_adv.adv_pause == 1 && tmp_eth_mac_config.tx_pause_en == 1)
    {
        l2_te_policer_cfg.bypass_red = 1;
    }
    else
    {
        l2_te_policer_cfg.bypass_red = 0;
    }
    aal_l2_te_policer_port_cfg_set(device_id,aal_port,l2_te_policer_cfg_msk,&l2_te_policer_cfg);
#else
	if ( lp_adv.adv_pause == 1 ) {
		eth_mac_mask.s.tx_flow_disable = 1;
		eth_mac_config.tx_flow_disable = 0;
	} else if ( lp_adv.adv_pause == 0 ) {
		eth_mac_mask.s.tx_flow_disable = 1;
		eth_mac_config.tx_flow_disable = 1;
	}
#endif
        /* sync MAC speed/duplex with PHY */
        eth_mac_config.speed = _PHY_SPEED2MAC_SPEED(link_status->speed);
        eth_mac_config.duplex = link_status->duplex;
    ret = aal_ni_eth_port_mac_set(device_id, aal_port, eth_mac_mask,
                    &eth_mac_config);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)

    if (aal_port == AAL_NI_PORT_GE4) {
        __eth_port_gphy_speed_sync(device_id);
    }

#endif

    return ret;

}

/**__port_phy2mac_sync
 * @brief: sync the speed and duplex from external PHY to Etherent MAC and SerDes
 */
static ca_status_t __port_phy2mac_sync(
    ca_device_id_t device_id,
    ca_port_id_t aal_port,
    aal_phy_link_status_t *link_status)
{
    ca_status_t  ret = CA_E_OK;

    if (!link_status || !link_status->link_up) {
        return CA_E_OK;
    }

    if (__ETH_IS_XGE(aal_port)) { /* XGE */
#if defined(CONFIG_NE_AAL_XFI)
        ret =__xge_phy2mac_sync(device_id, aal_port, link_status);
#endif
    } else { /* GE */
        ret = __ge_phy2mac_sync(device_id, aal_port, link_status);
    }
    __ETH_DBG("INFO: Sync Eth PHY speed %d(%sbps) duplex %d to MAC Port %u\n",
        link_status->speed, aal_eth_phy_speed_str_get(device_id, link_status->speed), link_status->duplex, aal_port);

    return ret;
}


/**__port_eee_trigger
 *@brief: EEE function handle after link change
 */
static ca_status_t __port_eee_trigger(
    ca_device_id_t device_id,
    ca_port_id_t   aal_port,
    ca_boolean_t   link_status)
{
/*
All EEE state-machines are implemented in EEE PHY. EEE configurations can be 
set/get via MDIO/MMD registers (HW advised not to make any changes)

G3 just control the MAC:
1.	EEE_enable/disable - configurable via API (need to enable EEE in PHY via MMD)
2.	LPI_tx_enable/disable sequence:
    a.	TX_LPI control behavior :  (PHY link change INT trigger or polling trigger ) 
    b.	[1] Default  : disable TXLPI , (cpu_lpi_deassert= 1) 
    c.	[2] When detect PHY = link down , disable TXLPI , (cpu_lpi_deassert= 1) 
    d.	[3] When detect PHY = link up , should wait more than 1 second then enable  TXLPI , (cpu_lpi_deassert= 0)  
3.	LPI_data_mode - use HW default - no need to change, in case we need to change this during debugging
4.	txfifo_empty_cnt - use HW default (NOTE: value should be longer than ts_timeout in PHY)
5.	lpi_min_assert_cnt - user HW default
6.	lpi_min_wakeup_cnt (NOE: value should be longer than tw_timeout in PHY)) , set to be 30us enough, 300
7.	lpi_mac_dis_wait_cnt - user HW default
8.	stop_txclk_en_cnt - user HW default

*/
    aal_ni_eee_mask_t     eee_msk;
    aal_ni_eee_cfg_t      eee_cfg;

    if (__ETH_IS_GE(aal_port)) {
        eee_msk.u32 = 0;
        memset(&eee_cfg, 0, sizeof(eee_cfg));

        eee_msk.s.cpu_lpi_deassert = 1;
        eee_cfg.cpu_lpi_deassert = !link_status;
        if (link_status) {
            eee_msk.s.lpi_min_wakeup_cnt = 1;
            eee_cfg.lpi_min_wakeup_cnt = 300;
        }
        return aal_ni_eee_cfg_set(device_id, aal_port, eee_msk, &eee_cfg);
    } else if (__ETH_IS_XGE(aal_port)) {
        aal_ni_xge_eee_mask_t  xge_eee_mask;
        aal_ni_xge_eee_cfg_t   xge_eee_cfg;

        xge_eee_mask.u32 = 0;
        xge_eee_mask.s.tx_tql_val = 1;
        xge_eee_cfg.tx_tql_val = 12500;
        return aal_ni_xge_eee_cfg_set(device_id, aal_port, xge_eee_mask, &xge_eee_cfg);

}

    return CA_E_OK;

}

/**__port_link_evt_send
 * @brief: send the etherent port link change event to iRos system
 * @param device_id : SoC device ID
 * @param port_id: CA port ID
 * @param link_status: link state: up/down
 */
static ca_status_t  __port_link_evt_send(ca_device_id_t device_id, ca_port_id_t port_id,  ca_boolean_t  link_status)
{
    ca_event_port_link_t link_event;
    ca_status_t          ret = CA_E_OK;
    
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if ((device_id == CA_DEV_LAN && PORT_ID(port_id) == AAL_NI_PORT_XGE2) 
        || (device_id == CA_DEV_WAN && PORT_ID(port_id) == AAL_NI_PORT_XGE1)) {
        /* Internal port, neednt send event */
        return CA_E_OK;
    }
#endif

    memset(&link_event, 0, sizeof(ca_event_port_link_t));
    link_event.port_id = port_id;
    link_event.status  = link_status ? CA_PORT_LINK_UP : CA_PORT_LINK_DOWN ;
    ret = (ca_status_t)ca_event_send(device_id, CA_EVENT_ETH_PORT_LINK, 
        (void *)&link_event, sizeof(ca_event_port_link_t));

    return ret;
}

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
/**__hgu_internal_link_scan
 * @brief: check internal 10G connection between SATURN and G3, to recover internal connection if link down
 */
static void __hgu_internal_link_scan(void)
{
    static ca_boolean_t  internal_link[2] = {0};
    static ca_boolean_t  tmp_link[2] = {0};
    aal_ni_xge_pcs_status_t  pcs_status[2];
    ca_status_t ret = CA_E_OK;

    ret = aal_xfi_link_status_get(CA_DEV_LAN, 1, &tmp_link[0]);
    ret |= aal_xfi_link_status_get(CA_DEV_WAN, 1, &tmp_link[1]);
    memset(&pcs_status, 0, sizeof(pcs_status));
    ret |= aal_ni_xge_pcs_status_get(CA_DEV_LAN, 1, &pcs_status[0]);
    ret |= aal_ni_xge_pcs_status_get(CA_DEV_WAN, 1, &pcs_status[1]);
    tmp_link[0] &= pcs_status[0].rxlinksts;
    tmp_link[1] &= pcs_status[1].rxlinksts;

    if ( unlikely( ret != CA_E_OK ))
    {
        __ETH_DBG("FAIL to get internal link status.\n");
        return;
    }

    if ( unlikely(!tmp_link[0] || !tmp_link[1] ))
    {
        if ( tmp_link[0] != internal_link[0] || tmp_link[1] != internal_link[1] )
        {
            ca_printf ( 
				"WARNING: HGU internal XFI link loss (SATURN link=%d, G3 link=%d)\n" , 
				tmp_link[0] , 
				tmp_link[1] );
        }
		//
		// Check link_status of saturn. If link_status is down
		// Reset XFI 1 of G3.
		//
        if (!tmp_link[0])
        {
            aal_xfi_reset(CA_DEV_WAN, 1);
        }
		//
		// Check link_status of G3. If link_status is down
		// Reset XFI 1 of saturn.
		// Note, Saturn & g3 share speed configuration.
		// e.g. g_xfi_drv.xfi_speed[1] 
		//
        if (!tmp_link[1])
        {
            aal_xfi_reset(CA_DEV_LAN, 1);
        }
    }
    else if ( tmp_link[0] != internal_link[0] ||
              tmp_link[1] != internal_link[1] )
    {
        ca_printf("INFO: HGU internal XFI link up\n");
    }

    internal_link[0] = tmp_link[0];
    internal_link[1] = tmp_link[1];

}
#endif

static inline void __eth_link_chg_print(
    ca_device_id_t         device_id, 
    ca_port_id_t           aal_port, 
    aal_phy_link_status_t *status)
{
    if (status->link_up) {
        ca_printf("INFO: Device %d Ethernet Port %u Link UP %sbps/%s\n",
            device_id, aal_port, 
            aal_eth_phy_speed_str_get(device_id, status->speed),
            status->duplex ? "FULL" : "HALF");
    }  else {
        ca_printf("INFO: Device %d Ethernet Port %u Link DOWN\n", device_id, aal_port);
    }
}

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
/**__sfpplus_eth_port_link_scan
 *@brief: software polling SFPPLUS UNI port 
 */
static void
__sfpplus_eth_port_link_scan (
        ca_device_id_t device_id)
{
    ca_port_id_t          ge = 0;
    aal_phy_link_status_t link_status;
        aal_xfi_status_t      xfi_sts;
    static ca_boolean_t port_link_up[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_speed[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_duplex[__MAX_ETH_PORT_NUM] = { 0 };

    memset(&link_status, 0, sizeof(aal_phy_link_status_t));

        for ( ge = AAL_NI_GE_PORT_START; ge <= AAL_NI_GE_PORT_END; ge ++ ) /* check GE port link state */
        {

                if ( !__ETH_PORT_SCAN_IS_ENABLE ( ge ) || !__ETH_PORT_IS_VALID ( ge ) )
                {
            continue;
        }

                if ( CA_E_OK != __eth_link_status_get ( device_id , ge , &link_status ) )
                {
            __ETH_DBG("__eth_link_status_get ge %d fail\r\n", ge);
            continue;
        }

        if ((link_status.link_up != port_link_up[ge]) ||
          (link_status.speed != port_link_speed[ge]) ||
                     (link_status.duplex != port_link_duplex[ge]) ) /* link state change */
                {

            __eth_link_chg_print(device_id, ge, &link_status);

                        if ( link_status.link_up ) /* sync speed & duplex from PHY to MAC sync*/
                        {
                __ge_phy2mac_sync(device_id, ge, &link_status);
                                __ETH_LED_ON ( ge , link_status.speed );
                        }
                        else
                        {
                                __ETH_LED_OFF ( ge );
            }

            /* Send port link change event */
            __port_link_evt_send(device_id, CA_PORT_ID(CA_PORT_TYPE_ETHERNET, ge), link_status.link_up);
        }

        port_link_up[ge]     = link_status.link_up;
        port_link_speed[ge]  = link_status.speed;
        port_link_duplex[ge] = link_status.duplex;
    }

        if ( __ETH_PORT_SCAN_IS_ENABLE ( AAL_NI_INTERNAL_XE ) )
        {
                if ( CA_E_OK != __xge_intf_link_status_get ( device_id , AAL_NI_INTERNAL_XE , &link_status ) )
                {
            return;
        }
                //
                // As per rxidle, check whether the module is plugged
                // rxidle 0: module is presented, 1: empty.
                //
                aal_xfi_status_get ( device_id ,  (AAL_NI_INTERNAL_XE - AAL_NI_PORT_XGE0) , &xfi_sts );

                if ( !link_status.link_up && !xfi_sts.rxidle )
                {
            aal_xfi_link_toggle_rx_en(device_id, 1);
        }

                if ( link_status.link_up != port_link_up[AAL_NI_INTERNAL_XE] )
                {
                        if ( link_status.link_up )
                        {
                port_link_speed[AAL_NI_INTERNAL_XE] = link_status.speed;
                port_link_duplex[AAL_NI_INTERNAL_XE] = link_status.duplex;
                ca_printf("INFO: Device %d Ethernet Port %u Link UP %sbps/%s\n", 
                    device_id, AAL_NI_INTERNAL_XE, aal_eth_phy_speed_str_get(device_id, link_status.speed),
                    link_status.duplex ? "FULL" : "HALF");
            
            }
                        else
                        {
                port_link_speed[AAL_NI_INTERNAL_XE]  = AAL_PHY_SPEED_10;
                port_link_duplex[AAL_NI_INTERNAL_XE] = FALSE;
                ca_printf("INFO: Device %d Ethernet Port %u Link DOWN\n", device_id, AAL_NI_INTERNAL_XE);
                aal_xfi_reset(device_id, 1); /* reset XFI if linkdown found, to support link-partner shut/no-shut case */
            }
            
            /* Send port link change event */
            __port_link_evt_send(device_id,
                CA_PORT_ID(CA_PORT_TYPE_ETHERNET, AAL_NI_INTERNAL_XE), link_status.link_up);
            /* Save the link state */
            port_link_up[AAL_NI_INTERNAL_XE] = link_status.link_up;
        }
    }
}


ca_boolean_t  __eth_port3_active_evb_check (void )
{
    ca_uint32_t     port_bitmap = 0;
    ca_boolean_t    flag = FALSE;
    ca_status_t     ret = 0;

    ret= aal_scfg_read(CFG_ID_GE_PORT_BIT_MAP,
                        sizeof(ca_uint32_t),
                        (ca_uint8_t *)&port_bitmap);
    if ((ret == CA_E_OK)
        && ( 0x8 == (port_bitmap & 0x8) )){
        flag = TRUE;
    }

    return flag;
        }


#endif


/**__eth_port_use_deep_queue
 *@brief: enable the deepQueue for ehternet port egress to prevent packet buffer congestion TailDrop
 *
 * SoC internal packet buffer SRAM may be NOT big enough for buffering egress packet, so the external
 * buffer (deepQueue) can be used to buffer more packets.
 */
ca_status_t __eth_port_use_deep_queue(
                    CA_IN ca_device_id_t device_id, 
                    CA_IN ca_port_id_t aal_port,
                    CA_IN ca_boolean_t ena)
{
    aal_arb_port_dbuf_config_mask_t     port_dbuf_mask;
    aal_arb_port_dbuf_config_t              port_dbuf;
    ca_status_t            ret = CA_E_OK;
    ca_uint8_t             flow = 0;    
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ca_uint8_t upstream_use_dq = 0;
    ret = scfg_read(device_id, CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE,
    sizeof(ca_uint8_t), (ca_uint8_t *)&upstream_use_dq);
    if((upstream_use_dq == 1)&&(ena == TRUE)){
        return CA_E_NOT_SUPPORT;
    }
#endif

    if(aal_port > __MAX_ETH_PORT_ID){
        return CA_E_PARAM;
    }
    
    for(flow = 0; flow < 8; flow ++){
        ret = aal_arb_port_dbuf_get(device_id,flow,&port_dbuf);
        if(CA_E_OK != ret){
            goto __end;
        }
        if((port_dbuf.dbuf_flg == 1)&&(port_dbuf.ldpid == aal_port) &&(port_dbuf.ldpid_vld == TRUE)){
            if (ena)
                return CA_E_OK;
            else
                break;
        }
        if((port_dbuf.dbuf_flg == 0) && ena){
            break;
        }
    }
    if(flow == 8){
        if(ena){
            ret = CA_E_RESOURCE;
        }else{
            ret = CA_E_NOT_FOUND;
        }
        goto __end;
    }
    
    port_dbuf_mask.wrd = 0;
    memset(&port_dbuf, 0, sizeof(aal_arb_port_dbuf_config_t));
    if(ena){
        port_dbuf.dbuf_flg              = 1;
        port_dbuf.ldpid_vld             = TRUE;
        port_dbuf.ldpid                 = aal_port;
    }
    port_dbuf_mask.bf.dbuf_flg  = 1;
    port_dbuf_mask.bf.ldpid     = 1;
    port_dbuf_mask.bf.ldpid_vld = 1;
    
    ret = aal_arb_port_dbuf_set(device_id,flow,port_dbuf_mask,&port_dbuf);
__end:
    return ret;
}

#if defined(CONFIG_NE_AAL_XFI)
static void __internal_xfi_update(ca_device_id_t device_id, ca_uint32_t aal_port)
{
    ca_boolean_t link_up = 0;
    static int  internal_link_fail_count = 0;

    if (__ETH_IS_XFI(aal_port)) {
        (void)aal_xfi_link_status_get(device_id, aal_xfi_port2xfi(aal_port), &link_up);
        if (!link_up && (15 == (internal_link_fail_count++ % 16))) {
            if (__ETH_INTF_IS_SGMII(aal_port))
                aal_xfi_speed_set(device_id, aal_xfi_port2xfi(aal_port), AAL_PHY_SPEED_1000);
            else if (__ETH_INTF_IS_HISGMII(aal_port))
                aal_xfi_speed_set(device_id, aal_xfi_port2xfi(aal_port), AAL_PHY_SPEED_2500);
            else
                aal_xfi_speed_set(device_id, aal_xfi_port2xfi(aal_port), AAL_PHY_SPEED_10000);
        }
    }
    return;
}
#endif

/**__eth_port_link_scan
 *@brief: software polling etherent port link changing
 *
 * Once the external interrupts from PHY cannot be used or dont want to use, the polling task
 * can help to detect port link/speed/duplex state change and then do MAC-PHY sync and port 
 * event report.
 */
static void 
__eth_port_link_scan(
	ca_device_id_t device_id)
{

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)

    __sfpplus_eth_port_link_scan(device_id);

#else /* !CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS */

    ca_uint32_t aal_port = 0;
    aal_phy_link_status_t link_status;
#if defined(CONFIG_NE_AAL_XFI)
	aal_xfi_status_t      xfi_sts;
#endif
    static ca_boolean_t port_link_up[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_speed[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_duplex[__MAX_ETH_PORT_NUM] = { 0 };

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(likely(__ETH_PORT_SCAN_IS_ENABLE(AAL_NI_INTERNAL_XE))) {
        __hgu_internal_link_scan();
    }
#endif

    __FOR_ALL_ETH_PORTS_DO(aal_port) {
        if (!__ETH_PORT_SCAN_IS_ENABLE(aal_port)) {
            continue;
        }

        if (__ETH_IS_CONNECT_SWITCH(aal_port)) {
#if defined(CONFIG_NE_AAL_XFI)
        if (__ETH_IS_XFI(aal_port)) {
            __internal_xfi_update(device_id, aal_port);
        }
#endif
            continue;
        }

        memset(&link_status, 0, sizeof(link_status));

        /* ONLY scan port when port enable, otherwise regard port as link down */
        if (g_eth_port_drv.port_enable[aal_port]) {
            if (CA_E_OK != __eth_link_status_get(device_id, aal_port, &link_status)) {
                __ETH_DBG("__eth_link_status_get fail!\n");
                continue; /* Link status get FAIL */
            }
        }

        if ((!link_status.link_up && !port_link_up[aal_port]) ||
            (link_status.link_up && port_link_up[aal_port]
            && (port_link_speed[aal_port] == link_status.speed 
            && port_link_duplex[aal_port] == link_status.duplex))) {

#if defined(CONFIG_NE_AAL_XFI)
            if (!port_link_up[aal_port]) {
				if(!__ETH_IS_XFI(aal_port) || __ETH_DETECTED_PHY(aal_port)) {
					continue;
				}
				aal_xfi_status_get(device_id,aal_xfi_port2xfi(aal_port),&xfi_sts);
				if(!xfi_sts.rxidle) {
                    aal_xfi_link_toggle_rx_en(device_id, aal_xfi_port2xfi(aal_port));
                }
			}
#endif
            continue; /* Link status and speed/duplex NOT change */
        }

        /*
         * Handle the port link change in the following
         */
		if(link_status.link_up && __ETH_DETECTED_PHY(aal_port)
		   && !g_eth_port_drv.disable_phy2mac_sync[aal_port]) {
			/* PHY status sync to MAC config */
			__port_phy2mac_sync(device_id,aal_port,&link_status);
        }
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
		if(link_status.link_up && (aal_port == AAL_NI_PORT_GE4) &&
		   !__ETH_DETECTED_PHY(aal_port)) {
			/* sync SFP fiber/copper link speed to GPHY PCS */
            __eth_port_gphy_speed_sync(device_id);
        }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#if !defined(CONFIG_LUNA_G3_SERIES) // no need to enable deep buffer, otherwise loopback mode data path would fail. DMA TX->L2FE->L2TM->L3QM->L2TM(redirect to port 6 without fake vlan)
        /* Enable deepQ for speed <=5G ports when link state change  */
        if (link_status.link_up && g_eth_port_drv.eth_dynamic_deep_queue) {
            if (link_status.speed == AAL_PHY_SPEED_10000)
                __eth_port_use_deep_queue(device_id, aal_port, FALSE);
            else
                __eth_port_use_deep_queue(device_id, aal_port, TRUE);
		}
#endif		
#endif
        /* EEE handle */
        if (__ETH_EEE_IS_ENABLE(aal_port) && __ETH_DETECTED_PHY(aal_port)) {
            (void)__port_eee_trigger(device_id, aal_port, link_status.link_up);
        }
        /* save Link status */
        port_link_up[aal_port] = link_status.link_up;

        if (link_status.link_up) {
#if !defined(CONFIG_RTL_MULTI_LAN_DEV)
            if (!__ETH_IS_XGE(aal_port))
                __ETH_LED_ON(aal_port, link_status.speed);
#endif
            port_link_speed[aal_port] = link_status.speed;
            port_link_duplex[aal_port] = link_status.duplex;
#if defined(CONFIG_NE_AAL_XFI)
			if(__ETH_IS_XFI(aal_port) && !__ETH_INTF_IS_USXGMII(aal_port)) {
				if(!g_eth_port_drv.disable_phy2mac_sync[aal_port])
					aal_xfi_reset(device_id,aal_xfi_port2xfi(aal_port));
			}
#endif
            ca_printf("INFO: Device %d Ethernet Port %u Link UP %sbps/%s\n", 
                device_id, aal_port, 
                aal_eth_phy_speed_str_get(device_id, link_status.speed), link_status.duplex ? "FULL" : "HALF");
        } else {
#if !defined(CONFIG_RTL_MULTI_LAN_DEV)
            if (!__ETH_IS_XGE(aal_port))
                __ETH_LED_OFF(aal_port);
#endif
            port_link_speed[aal_port]  = AAL_PHY_SPEED_10;
            port_link_duplex[aal_port] = FALSE;
            ca_printf("INFO: Device %d Ethernet Port %u Link DOWN\n", device_id, aal_port);
        }

        /* Send port link change event */
        __port_link_evt_send(device_id,
            CA_PORT_ID(CA_PORT_TYPE_ETHERNET, aal_port), link_status.link_up);
    }
#endif /* else of CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS */

}

/**__eth_port_link_scan_timer_func
 * @brief: timer handle for polling etherent port link status change,
 *         and sync PHY speed/duplex to MAC
 * @note: for pon related product, the port scan should add some delay to wait application bringup 
 */
static void __eth_port_link_scan_timer_func(void *cookie)
{
    __ETH_DBG("ca-ne: port scan thread create successfully.\n");

    while(!g_eth_port_drv.eth_port_scan_stop) {

        ca_sleep(__ETH_PORT_LINK_SCAN_INTERVAL);
        if (!ni_initialized) {
            continue;
        }
        __eth_port_link_scan(__ETH_DEV_LOCAL);
    }

    __ETH_DBG("ca-ne: port scan thread exit successfully.\n");

}

/**__eth_chk_port_bmp
 * @brief: check the ethernet port bitmap capability in current chipset and startup config,
 *         retrive the correct GE bitmap, XE bitmap
 */
static ca_status_t __eth_chk_port_bmp(ca_device_id_t device_id, ca_uint32_t *ge_bmp, ca_uint32_t *xe_bmp)
{
    ca_status_t              ret = CA_E_OK;
    aal_ni_eth_capability_t  cap;

    memset(&cap, 0, sizeof(cap));

    ret = aal_ni_eth_capability_get(device_id, &cap);
    CA_RET_VALUE_CHECK(ret);

    if ((*ge_bmp | cap.ge_bmp) != cap.ge_bmp) {
        ca_printf("SCFG GE bmp %#x is ilegal, Chip GE capability is %#x\n", *ge_bmp, cap.ge_bmp);
        *ge_bmp &= cap.ge_bmp;
    }

    if ((*xe_bmp | cap.xe_bmp) != cap.xe_bmp) {
        ca_printf("SCFG XE bmp %#x is ilegal, Chip XE capability is %#x\n", *xe_bmp, cap.xe_bmp);
        *xe_bmp &= cap.xe_bmp;
    }

    return CA_E_OK;
}

/**__eth_port_scfg_load
 * @brief: load ethernet port related startup configurations
 * 
 * The ethernet ports software needs the SCFG fields as below: 
 *        PHY MDIO address;
 *        port interface mode;
 *        valid eth-port bitmap on the specific Hardware board;
 * If load SCFG fail, software will use default configurations.
 */
static ca_status_t __eth_port_scfg_load(ca_device_id_t device_id)
{
    ca_uint8_t  scfg_array[CFG_ID_PHY_INTERFACE_LEN + CFG_ID_PHY_BASE_ADDR_LEN] = {0};
    ca_status_t ret = CA_E_OK;
    ca_uint32_t phy_id = 0;
    ca_uint32_t default_mdio[] = {/* FPGA board has 8 Eth Ports */
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
    8, 1, 2, 3, 4, 31, 31, 31/* Joey verify in G3Lite ASIC */
#else
    1, 2, 3, 4, 16, 17, 18, 19
#endif
    };

/**
    Port Interface Configuration
FOR G3:
    LAN Port 0~3 ASIC: support GMII_GE/MII mode1.
    In GMII_GE mode, support 1Gbps and full-duplex2.
    In MII mode, support 10/100Mbps and full/half-duplex
    LAN Port 0~3 FPGA: support RGMII_GE/RGMII_FE1.
    In RGMII_GE mode, support 1Gbps and full-duplex2.
    In RGMII_FE mode, support 10/100Mpbs and half/full-duplex
    LAN Port 4 ASIC: support GMII_GE/RGMII_GE/RGMII_FE mode
    1. In GMII_GE mode, support 1000/100/10Mbps;
    2. In GMII_GE mode, support full-duplex at 1000Mbps;
    3. In GMII_GE mode, support half/full-duplex at 10/100Mbps;
    4. In RGMII_GE mode, support full-duplex at 1000Mbps;
    5. In RGMII_FE mode, support 10/100Mbps and full/half-duplex
    LAN Port 4 FPGA: support RGMII_GE/RGMII_FE mode
    1. In RGMII_GE mode, support full-duplex at 1000Mbps;
    2. In RGMII_FE mode, support 10/100Mbps and full/half-duplex
    Note: when Port 4 connects to SGMII PCS, it should set to GMII_GE mode.
    In thismode,port 4 can run 1000Mbps/full-duplex, 100Mbps/10Mbps/full or half-duplex.
FOR Raptor:Port 0~3 can be configured as 10/100Mbps MII mode or 1000Mbps GMII-like SerDesmode
    set int_cfg == GMII_GE for GMII-like SerDes mode
    set int_cfg == MII for 10/100M MII mode
    Port 4 can be configured as 10/100Mbps MII mode or RGMII_GE/RGMII_FE mode
    set int_cfg == MII for 10/100M MII mode
    set int_cfg == RGMII_GE for 1000M RGMII_GE mode
    set int_cfg == RGMII_FE for 100/10M RGMII_FE mode
*/
#ifdef CONFIG_CORTINA_BOARD_FPGA
    ca_uint32_t default_if_mode[] = {
        AAL_NI_IF_MODE_FE_RGMII, /* p0 */
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII, /* p4 FE */
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI    /* p7 */
    };
#else  /* CONFIG_CORTINA_BOARD_FPGA */
    ca_uint32_t default_if_mode[] = {
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
        AAL_NI_IF_MODE_FE_MII, /* p0 */
        AAL_NI_IF_MODE_FE_MII,
        AAL_NI_IF_MODE_FE_MII,
        AAL_NI_IF_MODE_FE_MII,
        AAL_NI_IF_MODE_FE_MII, /* p4 RGMII */
#else
        AAL_NI_IF_MODE_GE_GMII, /* p0 */
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII, /* p4 GMII, check with Aaron/Lidong for G3/HGU Port 4? */       
#endif
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI    /* p7 */
    };
#endif /* CONFIG_CORTINA_BOARD_FPGA */

    /* NOTE: should Load ethernet port bitmap firstly */
#ifdef CONFIG_CORTINA_BOARD_FPGA
    g_eth_port_drv.ge_port_bmp = 0x3f;
    g_eth_port_drv.xe_port_bmp = 0x00;
#else
    g_eth_port_drv.ge_port_bmp = 0;
    g_eth_port_drv.xe_port_bmp = 0;
    ret = scfg_read(device_id, CFG_ID_GE_PORT_BIT_MAP, sizeof(ca_uint32_t), &g_eth_port_drv.ge_port_bmp);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("!Err: read CA_CFG_ID_GE_PORT_BIT_MAP fail %#x!.\n", ret);
        g_eth_port_drv.ge_port_bmp = 0x3C;
    }
    __ETH_INIT_PRINT("load CA_CFG_ID_GE_PORT_BIT_MAP: %#x\n", g_eth_port_drv.ge_port_bmp);
    

    ret = scfg_read(device_id, CFG_ID_XGE_PORT_BIT_MAP, sizeof(ca_uint32_t), &g_eth_port_drv.xe_port_bmp);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("!Err: read CA_CFG_ID_XGE_PORT_BIT_MAP fail %#x!\n", ret);
        g_eth_port_drv.xe_port_bmp = 0;
    }
    __ETH_INIT_PRINT("load CA_CFG_ID_XGE_PORT_BIT_MAP: %#x.\n", g_eth_port_drv.xe_port_bmp);
#endif

    (void)__eth_chk_port_bmp(device_id, &g_eth_port_drv.ge_port_bmp, &g_eth_port_drv.xe_port_bmp);

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_WAN_PORT_INDEX)
	if(CONFIG_RG_G3_WAN_PORT_INDEX!=AAL_PPORT_NI7){
		g_eth_port_drv.ge_port_bmp |= (1<<CONFIG_RG_G3_WAN_PORT_INDEX); /* hacking */
	}
#endif
	
    g_eth_port_drv.eth_port_bmp = g_eth_port_drv.ge_port_bmp | g_eth_port_drv.xe_port_bmp;
    if (0 == g_eth_port_drv.eth_port_bmp) {
        __ETH_INIT_PRINT("!!!critical error: No Ethernet port exist, please check your scfg.txt!");
        return CA_E_ERROR;
    }

    __ETH_INIT_PRINT("Valid Ethernet ports bitmap : %#x\r\n", g_eth_port_drv.eth_port_bmp);

#ifdef CONFIG_CORTINA_BOARD_FPGA

    __ETH_INIT_PRINT("FPGA Board: use default MDIO configurations\n");
    __FOR_ALL_ETH_PORTS_DO(phy_id) {
        g_eth_port_drv.eth_mdio_addr[phy_id] = default_mdio[phy_id];
    }

    __ETH_INIT_PRINT("FPGA Board: use default I/F mode configurations\n");
    __FOR_ALL_ETH_PORTS_DO(phy_id) {
        g_eth_port_drv.eth_if_mode[phy_id] = default_if_mode[phy_id];
    }

#else /* CONFIG_CORTINA_BOARD_FPGA */

    /* Read PHY MDIO address from STARTUP config */
    memset(&scfg_array[0], 0, sizeof(scfg_array));
    ret = scfg_read(device_id, CFG_ID_PHY_BASE_ADDR, CFG_ID_PHY_BASE_ADDR_LEN, &scfg_array[0]);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("!Err: read PHY_BASE_ADDR fail %#x!\n", ret);
        /* Use following default MDIO configuration for ETH ports */
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            g_eth_port_drv.eth_mdio_addr[phy_id] = default_mdio[phy_id];
        }
    } else {
        memcpy(g_eth_port_drv.eth_mdio_addr, scfg_array, CFG_ID_PHY_BASE_ADDR_LEN);
    }

    /* Read ETH Interface mode */
    memset(&scfg_array[0], 0, sizeof(scfg_array));
    ret = scfg_read(device_id, CFG_ID_PHY_INTERFACE, CFG_ID_PHY_INTERFACE_LEN, &scfg_array[0]);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("!Err: read CA_CFG_ID_PHY_INTERFACE fail %#x!\n",
              ret);
        /* Use following default I/F configuration for ETH ports */
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            g_eth_port_drv.eth_if_mode[phy_id] = default_if_mode[phy_id];
        }
    } else {
        memcpy(g_eth_port_drv.eth_if_mode, scfg_array, CFG_ID_PHY_INTERFACE_LEN);
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            __ETH_INIT_PRINT("ethernet port %d interface type:  %s\n", phy_id,
                __eth_if_str_get(g_eth_port_drv.eth_if_mode[phy_id]));
            if (g_eth_port_drv.eth_if_mode[phy_id] > AAL_NI_IF_MODE_MAX) {
                g_eth_port_drv.eth_port_bmp &= ~(1 << phy_id); /* invalid port */
            }
        }
    }
#endif /* else of CONFIG_CORTINA_BOARD_FPGA */


        g_eth_port_drv.eth_dynamic_deep_queue = 1;


    __ETH_INIT_PRINT("load Ethernet port SCFG OK\n");

    return CA_E_OK;

}

/**__eth_port_global_init
 * @brief: Initialize etherent port driver global resources: control block, spinlock.etc
 */
static ca_status_t __eth_port_global_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    memset(&g_eth_port_drv, 0, sizeof(g_eth_port_drv));
    memset(&g_eth_port_drv.eth_mdio_addr[0], 0xFF, sizeof(ca_uint8_t) * __MAX_ETH_PORT_NUM); /* init MDIO address as invalid */

    /* Initialize Ethernet port API lock */
    if (CA_E_OK != ca_spin_lock_init(&g_eth_port_drv.eth_port_lock, SPINLOCK_BH)) {
        return CA_E_RESOURCE;
    }

    /* Initialize MDIO global resource */
    ret = aal_mdio_global_init(device_id);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("MDIO global init fail, return %d\r\n", ret);
        return ret;
    }
    /* Load startup configuration into global var */
    ret = __eth_port_scfg_load(device_id);

    /* enable ethernet port scan for G3 ASIC */
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
    g_eth_port_drv.eth_port_scan_enable = g_eth_port_drv.eth_port_bmp;
#else
    g_eth_port_drv.eth_port_scan_enable = ~0;
#endif

    return ret;
}

/**__eth_port_scan_start
 * @brief: Start etherent port polling
 */
static ca_status_t __eth_port_scan_start(ca_device_id_t device_id)
{
    ca_uint  tid = 0;
    ca_int   ret = 0;

    /* Port scan thread */
    g_eth_port_drv.eth_port_scan_stop = 0;
    g_eth_port_drv.eth_xfi_autoreset = 1;

    ret = ca_thread_create(&tid, "port_scan", __eth_port_link_scan_timer_func, NULL, 10240, 15, 0);
    if (CA_E_OK != ret) {
        ca_printf("error: create port_scan thread fail %d.\n", ret);
        return ret;
    }

    return CA_E_OK;
}

/******************************************************************************/
/*                  MDIO(Management Data I/O) API Implementation              */
/******************************************************************************/
/******************************************************************************
API
    cap_mdio_speed_set
Description
    This API routines is used to set MDIO clock frequency..
Parameters
    [IN] dev_id: CA device number
    [IN] addr: Address of MDIO slave device {0x01 ~ 0x1f}
    [IN] clock: MDIO clock frequency in unit of KHZ {0x0001 ~ 0x4e20}
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t cap_mdio_speed_set(CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t addr,
    CA_IN ca_uint32_t clock)
{
    CA_ASSERT_RET(addr >= CA_MDIO_ADDR_MIN && addr <= CA_MDIO_ADDR_MAX, CA_E_PARAM);
    CA_ASSERT_RET(clock >= CA_MDIO_CLOCK_MIN && clock <= CA_MDIO_CLOCK_MAX, CA_E_PARAM);

    return aal_mdio_speed_set(device_id, addr, clock);

}

/******************************************************************************
API
    cap_mdio_speed_get
Description
    This API routines is used to get MDIO clock frequency..
Parameters
    [IN] dev_id: CA device number
    [IN] addr: Address of MDIO slave device {0x01 ~ 0x1f}
    [OUT] clock: MDIO clock frequency in unit of KHZ {0x0001 ~ 0x4e20}
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t cap_mdio_speed_get(CA_IN ca_device_id_t device_id,
    CA_IN  ca_uint32_t addr,
    CA_OUT ca_uint32_t * clock)
{
    CA_ASSERT_RET(addr >= CA_MDIO_ADDR_MIN && addr <= CA_MDIO_ADDR_MAX, CA_E_PARAM);
    CA_ASSERT_RET(clock != NULL, CA_E_PARAM);

    return aal_mdio_speed_get(device_id, addr, clock);


}

/******************************************************************************
API
    cap_mdio_write
Description
    These API routines are used write 16 bits MDIO data into
    MDIO Manageable Device. See IEEE 802.3 clause 22.2.4.5, 
    clause 45.3 for detail explanations of the parameters.
Parameters
    [IN] dev_id : CA device number
    [IN] st_code: Start frame code (1 for IEEE 802.3 clause 22.2.4.5.3, 0 for IEEE 802.3 clause 45.3.3)
    [IN] phy_port_addr: PHY/port address (see IEEE 802.3 clause 22.2.4.5.5, clause 45.3.5)
    [IN] reg_dev_addr: REG/device address (see IEEE 802.3 clause 22.2.4.5.6, clause 45.3.6)
    [IN] addr: Address (see IEEE 802.3 clause 45.3.8)
    [IN] data: Data (see IEEE 802.3 clause 22.2.4.5.8, clause 45.3.8)
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t cap_mdio_write(CA_IN ca_device_id_t device_id,
    CA_IN      ca_uint8_t             st_code, /* START of Frame: it is 0 for IEEE802.3 Clause 45 and 1 for IEEE802.3 Clause 22 */
    CA_IN      ca_uint8_t             phy_port_addr, /* PHY address */
    CA_IN      ca_uint8_t             reg_dev_addr, /* Device Type */
    CA_IN      ca_uint16_t            addr, /* register address */
    CA_IN      ca_uint16_t            data)
{
    CA_ASSERT_RET(phy_port_addr >= CA_MDIO_ADDR_MIN && phy_port_addr <= CA_MDIO_ADDR_MAX, CA_E_PARAM);

    return aal_mdio_write(device_id, st_code, phy_port_addr, reg_dev_addr, addr, data);

}

/******************************************************************************
API
    cap_mdio_read
Description
    These API routines are used read 16 bits MDIO data from
    MDIO Manageable Device. See IEEE 802.3 clause 22.2.4.5, 
    clause 45.3 for detail explanations of the parameters.
Parameters
    [IN] dev_id : CA device number
    [IN] st_code: Start frame code (1 for IEEE 802.3 clause 22.2.4.5.3, 0 for IEEE 802.3 clause 45.3.3)
    [IN] phy_port_addr: PHY/port address (see IEEE 802.3 clause 22.2.4.5.5, clause 45.3.5)
    [IN] reg_dev_addr: REG/device address (see IEEE 802.3 clause 22.2.4.5.6, clause 45.3.6)
    [IN] addr: Address (see IEEE 802.3 clause 45.3.8)
    [OUT] data: Data (see IEEE 802.3 clause 22.2.4.5.8, clause 45.3.8)
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t cap_mdio_read(CA_IN ca_device_id_t device_id,
    CA_IN    ca_uint8_t           st_code,
    CA_IN    ca_uint8_t           phy_port_addr,
    CA_IN    ca_uint8_t           reg_dev_addr,
    CA_IN    ca_uint16_t          addr,
    CA_OUT   ca_uint16_t         *data)
{
    CA_ASSERT_RET(phy_port_addr >= CA_MDIO_ADDR_MIN && phy_port_addr <= CA_MDIO_ADDR_MAX, CA_E_PARAM);
    CA_ASSERT_RET(data != NULL, CA_E_PARAM);

    return aal_mdio_read(device_id, st_code, phy_port_addr, reg_dev_addr, addr, data);
}


/******************************************************************************/
/*                       Ethernet Port API Implementation                     */
/******************************************************************************/

static ca_status_t __ge_pause_set(
        ca_device_id_t  device_id,
        ca_uint32_t  aal_port,
        ca_boolean_t pfc_enable,
        ca_boolean_t pause_rx,
        ca_boolean_t pause_tx)
{
    aal_ni_eth_mac_config_t      aal_config;
    aal_ni_eth_mac_config_mask_t aal_mask;
    aal_phy_autoneg_adv_t adv;

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    if (aal_port >= AAL_NI_PORT_NUM || aal_port < AAL_PORT_NUM_FIRST_ETH)
        return CA_E_PARAM;
#else
    if (aal_port >= AAL_NI_PORT_NUM)
        return CA_E_PARAM;
#endif
    /* Set PHY pause */
    aal_eth_phy_ability_adv_get(device_id,
                      __ETH_PHY_ADDR(aal_port), &adv);

    adv.adv_pause = (pause_rx && pause_tx);
    adv.adv_asym_pause = (pause_rx || pause_tx);

    aal_eth_phy_ability_adv_set(device_id,
                      __ETH_PHY_ADDR(aal_port), adv);

    memset(&aal_config, 0, sizeof(aal_ni_eth_mac_config_t));

    /* Set MAC pause */
    aal_config.tx_pause_en      = pause_tx;
    aal_config.rx_pause_en      = pause_rx;
    /* enable stop tx request to Tx MAC when recevie PAUSE */
    aal_config.rx_flow_to_tx_en = pause_rx;

    if (pfc_enable) {/* Enable PFC should disable FC according to ASIC */
        aal_config.rx_pfc_disable   = 0;//!pause_rx; enable signal to TE
        aal_config.tx_pfc_disable   = !pause_tx;
        aal_config.rx_flow_disable  = 1;
        aal_config.tx_flow_disable  = 1;
    } else {
        aal_config.rx_pfc_disable   = 1;
        aal_config.tx_pfc_disable   = 1;
        aal_config.rx_flow_disable  = !pause_rx;
        aal_config.tx_flow_disable  = !pause_tx;
    }

    aal_mask.u32 = 0;
    aal_mask.s.rx_flow_disable  = 1;
    aal_mask.s.tx_flow_disable  = 1;
    aal_mask.s.rx_pause_en      = 1;
    aal_mask.s.tx_pause_en      = 1;
    aal_mask.s.rx_flow_to_tx_en = 1;
    aal_mask.s.rx_pfc_disable   = 1;
    aal_mask.s.tx_pfc_disable   = 1;

    return aal_ni_eth_port_mac_set(device_id, aal_port,
                      aal_mask, &aal_config);
}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
static ca_status_t 
__xge_pause_set(
	ca_device_id_t device_id, 
	ca_uint32_t aal_port, 
	ca_boolean_t pfc_enable, 
	ca_boolean_t pause_rx, 
	ca_boolean_t pause_tx)
{
    ca_status_t ret = CA_E_OK;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ni_xaui_mac_tx_cfg_mask_t  xaui_mac_tx_mask;
    aal_ni_xaui_mac_tx_cfg_t       xaui_mac_tx;
    aal_ni_xaui_mac_rx_cfg_mask_t  xaui_mac_rx_mask;
    aal_ni_xaui_mac_rx_cfg_t       xaui_mac_rx;
    aal_ni_xaui_mac_rx_ctrl_mask_t xaui_mac_rx_ctrl_mask;
    aal_ni_xaui_mac_tx_ctrl_mask_t xaui_mac_tx_ctrl_mask;
    aal_ni_xaui_mac_rx_ctrl_t      xaui_mac_rx_ctrl;
    aal_ni_xaui_mac_tx_ctrl_t      xaui_mac_tx_ctrl;
#endif
    aal_ni_eth_mac_config_mask_t   ni_config_mask;
    aal_ni_eth_mac_config_t        ni_config;

    __ETH_DBG("device_id = %u, pfc_enable = %u, pause_rx = %u, pause_tx = %u.\n",
        device_id, pfc_enable, pause_rx, pause_tx);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    memset(&xaui_mac_tx, 0, sizeof(aal_ni_xaui_mac_tx_cfg_t));
    memset(&xaui_mac_rx, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));
    memset(&xaui_mac_rx_ctrl, 0, sizeof(aal_ni_xaui_mac_rx_ctrl_t));
    memset(&xaui_mac_tx_ctrl, 0, sizeof(aal_ni_xaui_mac_tx_ctrl_t));
#endif

    ni_config_mask.u32 = 0;
    memset(&ni_config, 0, sizeof(ni_config));
    ni_config_mask.s.tx_pause_en = 1;
    ni_config_mask.s.rx_pause_en = 1;

    ni_config.tx_pause_en = pause_tx;
    ni_config.rx_pause_en = pause_rx;

    ret = aal_ni_eth_port_mac_set(device_id, aal_port,
        ni_config_mask, &ni_config);
    CA_RET_VALUE_CHECK(ret);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_port == AAL_NI_PORT_XGE0) {
        xaui_mac_tx_mask.u32 = 0;
        xaui_mac_rx_mask.u32 = 0;
        xaui_mac_rx_ctrl_mask.u32 = 0;
        xaui_mac_tx_ctrl_mask.u32 = 0;
        xaui_mac_tx_mask.s.flow_en  = 1;
        xaui_mac_rx_mask.s.pause_en = 1;
        xaui_mac_rx_ctrl_mask.s.qbb_pause_det_en  = 1;
        xaui_mac_tx_ctrl_mask.s.qbb_flow_en = 1;
        xaui_mac_rx_ctrl_mask.s.frame_pause_drop_en  = 1;
        xaui_mac_rx_ctrl_mask.s.qbb_pause_drop_en = 1;
        xaui_mac_tx_ctrl_mask.s.qbb_qen = 1;

        if (pfc_enable) { /* PFC and FC should not enable at the same time according to ASIC */
            xaui_mac_tx_ctrl.qbb_qen = 0xFF;
            xaui_mac_tx.flow_en  = 0;
            xaui_mac_rx.pause_en = 0;
            xaui_mac_rx_ctrl.qbb_pause_det_en = 1; /* Enable PFC detect */
            xaui_mac_rx_ctrl.frame_pause_drop_en = 1;
            xaui_mac_rx_ctrl.qbb_pause_drop_en = !pause_rx;
            xaui_mac_tx_ctrl.qbb_flow_en = pause_tx;
        } else {
            xaui_mac_tx_ctrl.qbb_qen = 0;
            xaui_mac_tx.flow_en  = pause_tx;
            xaui_mac_rx.pause_en = pause_rx;
            xaui_mac_rx_ctrl.frame_pause_drop_en = !pause_rx;
            xaui_mac_rx_ctrl.qbb_pause_drop_en = 1;
            xaui_mac_rx_ctrl.qbb_pause_det_en = 0; /* Disable PFC detect */
            xaui_mac_tx_ctrl.qbb_flow_en = 0;
        }

        ret = aal_ni_xaui_mac_tx_ctrl_set(device_id, xaui_mac_tx_ctrl_mask, &xaui_mac_tx_ctrl);
        CA_RET_VALUE_CHECK(ret);


        ret = aal_ni_xaui_mac_rx_ctrl_set(device_id, xaui_mac_rx_ctrl_mask, &xaui_mac_rx_ctrl);
        CA_RET_VALUE_CHECK(ret);


        ret = aal_ni_xaui_mac_tx_config_set(device_id, xaui_mac_tx_mask, &xaui_mac_tx);
        CA_RET_VALUE_CHECK(ret);

        return aal_ni_xaui_mac_rx_config_set(device_id, xaui_mac_rx_mask, &xaui_mac_rx);
    }else
#endif
    if (aal_port >= AAL_NI_XE_START && aal_port <= AAL_NI_XE_END) {
        aal_ni_xge_flowctrl_mask_t      xge_fc_mask;
        aal_ni_xge_flowctrl_t           xge_fc_config;
        aal_ni_xge_cfg_tx_mask_t        xge_tx_mask;
        aal_ni_xge_cfg_tx_t             xge_tx_cfg;
        aal_ni_xge_cfg_rx_mask_t        xge_cfg_rx_mask;
        aal_ni_xge_cfg_rx_t             xge_cfg_rx;
        ca_uint32_t                     xe_start = AAL_NI_XE_START;                                      

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        xe_start = (CA_DEV_WAN == device_id) ? AAL_NI_PORT_XGE0 : AAL_NI_PORT_XGE1; 
#endif
        xge_tx_mask.u32 = 0;
        xge_tx_mask.s.pause_req_en = 1;
        xge_tx_mask.s.pause_req_auto_xon = 1;
        memset(&xge_tx_cfg, 0, sizeof(aal_ni_xge_cfg_tx_t));

        xge_fc_mask.u32 = 0;
        xge_fc_mask.s.pau_tx_en = 1;
        xge_fc_mask.s.pau_rx_en = 1;
        xge_fc_mask.s.qbb_tx_en = 1;
        xge_fc_mask.s.qbb_rx_en = 1;
        xge_fc_mask.s.qbb_qen   = 1;

        memset(&xge_fc_config, 0, sizeof(aal_ni_xge_flowctrl_t));
        if (pfc_enable) {
            xge_fc_config.qbb_qen   = 0xFF;
            xge_fc_config.pau_tx_en = 0;
            xge_fc_config.pau_rx_en = 0;
            xge_fc_config.qbb_tx_en = pause_tx;
            xge_fc_config.qbb_rx_en = pause_rx;
            xge_tx_cfg.pause_req_en = 0;
            xge_tx_cfg.pause_req_auto_xon = 0;
        } else {
            xge_fc_config.qbb_qen   = 0;
            xge_fc_config.pau_tx_en = pause_tx;
            xge_fc_config.pau_rx_en = pause_rx;
            xge_fc_config.qbb_tx_en = 0;
            xge_fc_config.qbb_rx_en = 0;
            xge_tx_cfg.pause_req_auto_xon = pause_tx;
        }
        ret = aal_ni_xge_flowctrl_set(device_id, aal_port - xe_start, xge_fc_mask, &xge_fc_config);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_ni_xge_tx_config_set(device_id, aal_port - xe_start, xge_tx_mask, &xge_tx_cfg);
        CA_RET_VALUE_CHECK(ret);

        xge_cfg_rx_mask.u32 = 0;
        memset(&xge_cfg_rx, 0, sizeof(xge_cfg_rx));
        xge_cfg_rx_mask.s.pause_extract = 1;
        xge_cfg_rx.pause_extract = pause_rx;
        ret = aal_ni_xge_rx_config_set(device_id,  aal_port - xe_start, xge_cfg_rx_mask, &xge_cfg_rx);

    }

    return ret;

}
#endif
/******************************************************************************
API
    ca_eth_port_pause_set
Description
    These API routines can be used to set PAUSE TX and RX independently, 
    which are also known as asymmetric flow control. If pause_rx is enabled, 
    the device will process PAUSE frame and decide to transmit data or not 
    according to time parameter in PAUSE frame. If pause_tx is enabled, the
    device will transmit PAUSE frame when the traffic load exceed the a 
    configured threshold value.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] pfc_enable: Enable/Disable priority based flow control (802.1Qbb)
    [IN] pause_rx: Reception control for PAUSE frame
    [IN] pause_tx: Transmission control for PAUSE frame
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_pause_set(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_IN ca_boolean_t pfc_enable,
                  CA_IN ca_boolean_t pause_rx,
                  CA_IN ca_boolean_t pause_tx)
{
    ca_status_t ret = CA_E_OK;
    aal_l3_te_flow_ctrl_mask_t l3_te_fc_mask;
    aal_l3_te_flow_ctrl_t       l3_te_fc;
    static ca_uint16_t t_port_flow_control= 0;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x pause_rx = %u pause_tx = %u.\n",
        device_id, port_id, pause_rx, pause_tx);

    __ETH_INIT_CHK(device_id);
    //__ETH_PORT_CHK(port_id);
    CA_BOOL_VAL_CHECK(pfc_enable);
    CA_BOOL_VAL_CHECK(pause_rx);
    CA_BOOL_VAL_CHECK(pause_tx);

    if (aal_port > __MAX_ETH_PORT_NUM) {
        ca_printf("%s(): invalid port_id %#x\r\n", __func__, port_id);
        return CA_E_PARAM;
    }

    l3_te_fc_mask.u32 = 0;
    memset(&l3_te_fc, 0, sizeof(aal_l3_te_flow_ctrl_t));  

    __ETH_PORT_LOCK(device_id);
        
    if (__ETH_IS_XGE(aal_port)) {/* XGE */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        ret = __xge_pause_set(device_id, aal_port,
            pfc_enable, pause_rx, pause_tx);
#endif
    } else { /* GE/FE */
        ret = __ge_pause_set(device_id, aal_port,
            pfc_enable, pause_rx, pause_tx);
    }

    if (CA_E_OK != ret){
        goto __api_end;
    }

    if (aal_port == AAL_LPORT_ETH_WAN) {
        l3_te_fc_mask.s.tx_wan_ena = 1;
        if (pause_tx){
            l3_te_fc.tx_wan_ena = 1;
        } else {
            l3_te_fc.tx_wan_ena = 0;
        }
    } else {
        if(pause_tx) {
            t_port_flow_control |= (1<<aal_port);
        } else {
            t_port_flow_control &= ~(1<<aal_port);
        }
        l3_te_fc_mask.s.tx_lan_ena = 1;
    }

    l3_te_fc.tx_lan_ena = (t_port_flow_control == 0) ? 0 : 1;

    ret = aal_l3_te_flow_control_set(device_id,l3_te_fc_mask,&l3_te_fc);
    __ETH_DBG("Set aal_port = %d, t_port_flow_control = 0x%x, tx_lan_ena= %d\n",
         aal_port, t_port_flow_control, l3_te_fc.tx_lan_ena);

__api_end:
    __ETH_PORT_UNLOCK(device_id);
    return ret;

}

/******************************************************************************
API
  ca_eth_port_pause_get
Description
  These API routines can be used to get PAUSE TX and RX independently, 
  which are also known as asymmetric flow control. If pause_rx is enabled, 
  the device will process PAUSE frame and decide to transmit data or not 
  according to time parameter in PAUSE frame. If pause_tx is enabled, the
  device will transmit PAUSE frame when the traffic load exceed the a 
  configured threshold value.
Parameters
  [IN] device_id: CA device number
  [IN] port_id: Ethernet port ID
  [OUT] pfc_enable: Enable/Disable priority based flow control (802.1Qbb)
  [OUT] pause_rx: Reception control for PAUSE frame
  [OUT] pause_tx: Transmission control for PAUSE frame
Returns
  CA_E_OK :Operation completed successfully
  CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_pause_get(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t   port_id,
                  CA_OUT ca_boolean_t *pfc_enable,
                  CA_OUT ca_boolean_t *pause_rx,
                  CA_OUT ca_boolean_t *pause_tx)
{
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;

    __ETH_DBG("device_id = %u port_id = %#x pfc_enable = %p pause_rx =%p pause_tx = %p.\n",
        device_id, port_id, pfc_enable, pause_rx, pause_tx);

    __ETH_INIT_CHK(device_id);
    //__ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(pfc_enable);
    CA_NULL_PTR_CHECK(pause_rx);
    CA_NULL_PTR_CHECK(pause_tx);

    if (aal_port > __MAX_ETH_PORT_NUM) {
        ca_printf("%s(): invalid port_id %#x\r\n", __func__, port_id);
        return CA_E_PARAM;
    }

    __ETH_PORT_LOCK(device_id);

    if (__ETH_IS_XGE(aal_port)) {/* XGE */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (aal_port == AAL_NI_PORT_XGE0 && ((1 << aal_port) & g_eth_port_drv.eth_port_bmp)) {
            aal_ni_xaui_mac_tx_cfg_t  xaui_mac_tx;
            aal_ni_xaui_mac_rx_cfg_t  xaui_mac_rx;
            aal_ni_xaui_mac_rx_ctrl_t xaui_mac_rx_ctrl;
            aal_ni_xaui_mac_tx_ctrl_t xaui_mac_tx_ctrl;

            memset(&xaui_mac_tx, 0, sizeof(aal_ni_xaui_mac_tx_cfg_t));
            memset(&xaui_mac_rx, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));
            memset(&xaui_mac_rx_ctrl, 0, sizeof(aal_ni_xaui_mac_rx_ctrl_t));
            memset(&xaui_mac_tx_ctrl, 0, sizeof(aal_ni_xaui_mac_tx_ctrl_t));

            ret = aal_ni_xaui_mac_tx_config_get(device_id, &xaui_mac_tx);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            ret = aal_ni_xaui_mac_rx_config_get(device_id, &xaui_mac_rx);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            ret = aal_ni_xaui_mac_tx_ctrl_get(device_id, &xaui_mac_tx_ctrl);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            ret = aal_ni_xaui_mac_rx_ctrl_get(device_id, &xaui_mac_rx_ctrl);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            if (xaui_mac_rx_ctrl.qbb_pause_det_en) {
                *pfc_enable = TRUE;
                *pause_tx = xaui_mac_tx_ctrl.qbb_flow_en;
                *pause_rx = !xaui_mac_rx_ctrl.qbb_pause_drop_en;
            } else {
                *pfc_enable = FALSE;
                *pause_tx = xaui_mac_tx.flow_en;
                *pause_rx = xaui_mac_rx.pause_en;
            }

        }
        else if (aal_port <= AAL_NI_PORT_XGE2){

            aal_ni_xge_flowctrl_t  xge_fc_config;

            memset(&xge_fc_config, 0, sizeof(aal_ni_xge_flowctrl_t));

            ret = aal_ni_xge_flowctrl_get(device_id, aal_port - AAL_NI_PORT_XGE1, &xge_fc_config);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            if (xge_fc_config.qbb_qen) {
                *pause_tx = xge_fc_config.qbb_tx_en;
                *pause_rx = xge_fc_config.qbb_rx_en;
                *pfc_enable = TRUE;
            } else {
                *pause_tx = xge_fc_config.pau_tx_en;
                *pause_rx = xge_fc_config.pau_rx_en;
                *pfc_enable = FALSE;
            }

        }
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (aal_port >= AAL_NI_PORT_XGE0 && aal_port <= AAL_NI_PORT_XGE1){

            aal_ni_xge_flowctrl_t  xge_fc_config;

            memset(&xge_fc_config, 0, sizeof(aal_ni_xge_flowctrl_t));

            ret = aal_ni_xge_flowctrl_get(device_id, aal_port - AAL_NI_PORT_XGE0, &xge_fc_config);
            if (CA_E_OK != ret) {
                goto __api_end;
            }

            if (xge_fc_config.qbb_qen) {
                *pause_tx = xge_fc_config.qbb_tx_en;
                *pause_rx = xge_fc_config.qbb_rx_en;
                *pfc_enable = TRUE;
            } else {
                *pause_tx = xge_fc_config.pau_tx_en;
                *pause_rx = xge_fc_config.pau_rx_en;
                *pfc_enable = FALSE;
            }
        }
#endif
    }
    else { /* GE/FE */
        aal_ni_eth_mac_config_t aal_config;

        ret = aal_ni_eth_port_mac_get(device_id,
                          aal_port, &aal_config);

        if (CA_E_OK != ret) {
            goto __api_end;
        }

        if (aal_config.rx_pfc_disable && aal_config.tx_pfc_disable) {
            *pfc_enable = FALSE;
            *pause_rx = !aal_config.rx_flow_disable;
            *pause_tx = !aal_config.tx_flow_disable;
        } else {
            *pfc_enable = TRUE;
            *pause_rx = !aal_config.rx_pfc_disable;
            *pause_tx = !aal_config.tx_pfc_disable;
        }

    }

__api_end:

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
    ca_eth_port_link_scan_set
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] enable: Link scan enable flag
Description
    These API routines can be used to set or get the link  scanning state for
    the specified port. If link scanning function is enabled, event 
    CA_EVENT_ETH_PORT_LINK will be posted the caller upon the change of link 
    status.
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_link_scan_set(CA_IN ca_device_id_t device_id,
                      CA_IN ca_port_id_t port_id,
                      CA_IN ca_boolean_t enable)
{
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x enable = %u.\n",
        device_id, port_id, enable);

    __ETH_INIT_CHK(device_id);
    CA_BOOL_VAL_CHECK(enable);

    __ETH_PORT_LOCK(device_id);

    if (enable) {
        /* enable the port-mapping bit */
        g_eth_port_drv.eth_port_scan_enable |= 0x1 << aal_port;
    } else {
        g_eth_port_drv.eth_port_scan_enable &= ~(0x1 << aal_port);
    }

    __ETH_PORT_UNLOCK(device_id);

    return CA_E_OK;

}

/******************************************************************************
API
  ca_eth_port_link_scan_get
Parameters
  [IN] device_id: CA device number
  [IN] port_id: Ethernet Port ID
  [OUT] enable: Link scan enable flag
Description
  These API routines can be used to get the link  scanning state for the 
  specified port. If link scanning function is enabled, event 
  CA_EVENT_ETH_PORT_LINK will be posted the caller upon the change of link 
  status.
Returns
  CA_E_OK: Operation completed successfully
  CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_link_scan_get(CA_IN ca_device_id_t device_id,
                      CA_IN ca_port_id_t port_id,
                      CA_OUT ca_boolean_t * enable)
{
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x enable = %p.\n",
        device_id, port_id, enable);

    __ETH_INIT_CHK(device_id);
    CA_NULL_PTR_CHECK(enable);

    __ETH_PORT_LOCK(device_id);

    *enable = (__ETH_PORT_SCAN_IS_ENABLE(aal_port)) ? TRUE : FALSE;

    __ETH_PORT_UNLOCK(device_id);

    return CA_E_OK;
}

/******************************************************************************
API
ca_eth_port_link_status_get
Description
    System provides CA_EVENT_ETH_PORT_LINK event to notify application 
    software of link status in real time. In addition, application 
    software can use this API to retrieve link status.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [OUT]status: Pointer to link status
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_link_status_get(CA_IN ca_device_id_t device_id,
            CA_IN ca_port_id_t port_id,
            CA_OUT ca_eth_port_link_status_t *status)
{
    ca_uint32_t           aal_port = PORT_ID(port_id);
    ca_status_t           ret = CA_E_OK;
    aal_phy_link_status_t phy_lnk;
#if 0
    __ETH_DBG("device_id = %u port_id = %#x status = %p.\n",
        device_id, port_id, status);
#endif

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(status);

    if (aal_port >= __MAX_ETH_PORT_NUM) {
        __ETH_DBG("the port %#x is invalid\n", port_id);
        return CA_E_PARAM;
    }

    memset(&phy_lnk, 0, sizeof(phy_lnk));

    __ETH_PORT_LOCK(device_id);

    ret = __eth_link_status_get(device_id, aal_port, &phy_lnk);

    __ETH_PORT_UNLOCK(device_id);

    if (CA_E_OK == ret) {
        *status = phy_lnk.link_up ? CA_PORT_LINK_UP : CA_PORT_LINK_DOWN;
    }

    return ret;

}

/**__eth_stat_get:
*@breif: Get GE/FE port statistics
*@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
*/
static ca_status_t
__ge_stat_get (
    ca_device_id_t device_id,
                 ca_uint32_t aal_port,
                 ca_boolean_t read_clear,
    ca_eth_port_stats_t* data)
{
    aal_ni_mib_stats_t ni_mib;
    ca_status_t ret = CA_E_OK;

    memset(&ni_mib, 0, sizeof(aal_ni_mib_stats_t));

    ret = aal_ni_eth_port_mib_get(device_id, aal_port, read_clear, &ni_mib);
    CA_RET_VALUE_CHECK(ret);

    data->rx_good_octets = ca_inter_to64((ca_uint32_t)ni_mib.rx_byte_cnt_high, (ca_uint32_t)ni_mib.rx_byte_cnt_low);
    data->rx_uc_frames = ni_mib.rx_uc_pkt_cnt;
    data->rx_mc_frames = ni_mib.rx_mc_frame_cnt;
    data->rx_bc_frames = ni_mib.rx_bc_frame_cnt;
    data->rx_error_frames = ni_mib.rx_crc_error_frame_cnt + ni_mib.rx_undersize_frame_cnt +
                            ni_mib.rx_runt_frame_cnt + ni_mib.rx_ovsize_frame_cnt;
    data->rx_fcs_error_frames = ni_mib.rx_crc_error_frame_cnt;
    data->rx_pause_frames = ni_mib.rx_pause_frame_cnt;
    data->rx_undersize_frames = ni_mib.rx_undersize_frame_cnt;
    data->rx_runt_frames = ni_mib.rx_runt_frame_cnt;
    data->rx_oversize_frames = ni_mib.rx_ovsize_frame_cnt;
    data->rx_64_frames = ni_mib.rx_frame_64_cnt;
    data->rx_65_127_frames = ni_mib.rx_frame_65to127_cnt;
    data->rx_128_255_frames = ni_mib.rx_frame_128to255_cnt;
    data->rx_256_511_frames = ni_mib.rx_frame_256to511_cnt;
    data->rx_512_1023_frames = ni_mib.rx_frame_512to1023_cnt;
    data->rx_1024_1518_frames = ni_mib.rx_frame_1024to1518_cnt;
    data->rx_1519_max_frames = ni_mib.rx_frame_1519to2100_cnt + ni_mib.rx_frame_2101to9200_cnt + 
                               ni_mib.rx_frame_9201tomax_cnt;
    data->tx_good_octets = ca_inter_to64((ca_uint32_t)ni_mib.tx_byte_cnt_high, (ca_uint32_t)ni_mib.tx_byte_cnt_low);
    data->tx_uc_frames = ni_mib.tx_uc_pkt_cnt;
    data->tx_mc_frames = ni_mib.tx_mc_frame_cnt;
    data->tx_bc_frames = ni_mib.tx_bc_frame_cnt;
    data->tx_error_frames = ni_mib.tx_crc_error_frame_cnt + ni_mib.tx_ovsize_frame_cnt +
            ni_mib.tx_single_col_frame_cnt + ni_mib.tx_multi_col_frame_cnt +
            ni_mib.tx_late_col_frame_cnt;
    data->tx_pause_frames = ni_mib.tx_pause_frame_cnt;
    data->tx_64_frames = ni_mib.tx_frame_64_cnt;
    data->tx_65_127_frames = ni_mib.tx_frame_65to127_cnt;
    data->tx_128_255_frames = ni_mib.tx_frame_128to255_cnt;
    data->tx_256_511_frames = ni_mib.tx_frame_256to511_cnt;
    data->tx_512_1023_frames = ni_mib.tx_frame_512to1023_cnt;
    data->tx_1024_1518_frames = ni_mib.tx_frame_1024to1518_cnt;
    data->tx_1519_max_frames = ni_mib.tx_frame_1519to2100_cnt + ni_mib.tx_frame_2101to9200_cnt + 
                               ni_mib.tx_frame_9201tomax_cnt;

    return ret;

}

 /**__xge_stat_get:
 *@breif: Get XGE port statistics
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
static ca_status_t 
__xge_stat_get (
    ca_device_id_t device_id,
                 ca_uint32_t aal_port,
                 ca_boolean_t read_clear,
    ca_eth_port_stats_t* data)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_mib_stats_t ni_mib;

    memset(&ni_mib, 0, sizeof(aal_ni_mib_stats_t));

#if defined(CONFIG_ARCH_CORTINA_G3)
    if ( AAL_NI_PORT_XGE0 == aal_port ) /* XGE */
    {
        aal_ni_xaui_mac_ctrl_tx_mib_t  xaui_tx_mib;
        aal_ni_xaui_mac_ctrl_rx_mib_t  xaui_rx_mib;

        memset(&xaui_tx_mib, 0, sizeof(aal_ni_xaui_mac_ctrl_tx_mib_t));
        memset(&xaui_tx_mib, 0, sizeof(aal_ni_xaui_mac_ctrl_rx_mib_t));

        ret = aal_ni_xaui_mac_ctrl_tx_mib_get(device_id, &xaui_tx_mib);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_ni_xaui_mac_ctrl_rx_mib_get(device_id, &xaui_rx_mib);
        CA_RET_VALUE_CHECK(ret);

        data->rx_good_octets      = xaui_rx_mib.byte_lo;
        data->rx_uc_frames        = xaui_rx_mib.uc_cnt;
        data->rx_mc_frames        = xaui_rx_mib.mc_cnt;
        data->rx_bc_frames        = xaui_rx_mib.bc_cnt;
        data->rx_error_frames     = xaui_rx_mib.crc_err_cnt + xaui_rx_mib.undersize_cnt +
                                    xaui_rx_mib.oversize_cnt;
        data->rx_fcs_error_frames = xaui_rx_mib.crc_err_cnt;
        data->rx_pause_frames     = xaui_rx_mib.pause_cnt;
        data->rx_undersize_frames = xaui_rx_mib.undersize_cnt;
        data->rx_oversize_frames  = xaui_rx_mib.oversize_cnt;
        data->rx_64_frames        = xaui_rx_mib.rmon64_cnt ;
        data->rx_65_127_frames    = xaui_rx_mib.rmon127_cnt;
        data->rx_128_255_frames   = xaui_rx_mib.rmon128_255_cnt ;
        data->rx_256_511_frames   = xaui_rx_mib.rmon256_511_cnt ;
        data->rx_512_1023_frames  = xaui_rx_mib.rmon512_1023_cnt ;
        data->rx_1024_1518_frames = xaui_rx_mib.rmon1024_1518_cnt;
        data->rx_1519_max_frames  = xaui_rx_mib.rmon1519_max_cnt;
        data->tx_good_octets      = xaui_tx_mib.byte_lo;
        data->tx_uc_frames        = xaui_tx_mib.uc_cnt;
        data->tx_mc_frames        = xaui_tx_mib.mc_cnt;
        data->tx_bc_frames        = xaui_tx_mib.bc_cnt;
        data->tx_error_frames     = 0;
        data->tx_pause_frames     = xaui_tx_mib.pause_cnt;
        data->tx_64_frames        = xaui_tx_mib.rmon64_cnt ;
        data->tx_65_127_frames    = xaui_tx_mib.rmon127_cnt;
        data->tx_128_255_frames   = xaui_tx_mib.rmon128_255_cnt ;
        data->tx_256_511_frames   = xaui_tx_mib.rmon256_511_cnt ;
        data->tx_512_1023_frames  = xaui_tx_mib.rmon512_1023_cnt ;
        data->tx_1024_1518_frames = xaui_tx_mib.rmon1024_1518_cnt;
        data->tx_1519_max_frames  = xaui_tx_mib.rmon1519_max_cnt;

        return ret;
    }
    else if ( aal_port >= AAL_NI_PORT_XGE1 )
    {
        aal_ni_xge_cmn_cfg_mask_t   cmn_msk;
        aal_ni_xge_cmn_cfg_t        cmn_cfg;

        cmn_msk.u32 = 0;
        memset(&cmn_cfg, 0, sizeof(aal_ni_xge_cmn_cfg_t));
        cmn_msk.s.clronread = 1;
        cmn_cfg.clronread = read_clear;
        ret = aal_ni_xge_cmn_config_set ( 
                    device_id , 
                    aal_port - AAL_NI_PORT_XGE1 , 
                    cmn_msk , 
                    &cmn_cfg );
        
        ret |= aal_ni_xge_mac_mib_get ( 
                    device_id ,
                    aal_port - AAL_NI_PORT_XGE1 , 
                    &ni_mib );
        CA_RET_VALUE_CHECK(ret);
    }
    else
    {
        return CA_E_PARAM;
    }

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    if ( aal_port >= AAL_NI_PORT_XGE1 )
    {
        aal_ni_xge_cmn_cfg_mask_t   cmn_msk;
        aal_ni_xge_cmn_cfg_t        cmn_cfg;

        cmn_msk.u32 = 0;
        memset(&cmn_cfg, 0, sizeof(aal_ni_xge_cmn_cfg_t));
        cmn_msk.s.clronread = 1;
        cmn_cfg.clronread = read_clear;
        ret = aal_ni_xge_cmn_config_set ( 
                    device_id , 
                    aal_port - AAL_NI_PORT_XGE1 , 
                    cmn_msk , 
                    &cmn_cfg );

        ret |= aal_ni_xge_mac_mib_get ( 
                    device_id ,
                    aal_port - AAL_NI_PORT_XGE1 , 
                    &ni_mib );
        CA_RET_VALUE_CHECK(ret);
    }
    else
    {
        return CA_E_PARAM;
    }
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if ( aal_port >= AAL_NI_PORT_XGE0 )
    {
        aal_ni_xge_cmn_cfg_mask_t   cmn_msk;
        aal_ni_xge_cmn_cfg_t        cmn_cfg;

        cmn_msk.u32 = 0;
        memset(&cmn_cfg, 0, sizeof(aal_ni_xge_cmn_cfg_t));
        cmn_msk.s.clronread = 1;
        cmn_cfg.clronread = read_clear;
        ret = aal_ni_xge_cmn_config_set(device_id, aal_port - AAL_NI_PORT_XGE0, cmn_msk, &cmn_cfg);

        ret |= aal_ni_xge_mac_mib_get(device_id,
                aal_port - AAL_NI_PORT_XGE0, &ni_mib);
        CA_RET_VALUE_CHECK(ret);

    }
    else
    {
        return CA_E_PARAM;
    }
#endif
    data->rx_good_octets = ((ca_uint64_t)ni_mib.rx_byte_cnt_high << 32) + 
                           (ca_uint64_t)ni_mib.rx_byte_cnt_low;
    data->rx_uc_frames = ni_mib.rx_uc_pkt_cnt;
    data->rx_mc_frames = ni_mib.rx_mc_frame_cnt;
    data->rx_bc_frames = ni_mib.rx_bc_frame_cnt;
    data->rx_error_frames = ni_mib.rx_crc_error_frame_cnt + ni_mib.rx_undersize_frame_cnt +
                            ni_mib.rx_runt_frame_cnt + ni_mib.rx_ovsize_frame_cnt;
    data->rx_fcs_error_frames = ni_mib.rx_crc_error_frame_cnt;
    data->rx_pause_frames = ni_mib.rx_pause_frame_cnt;
    data->rx_undersize_frames = ni_mib.rx_undersize_frame_cnt;
    data->rx_runt_frames = ni_mib.rx_runt_frame_cnt;
    data->rx_oversize_frames = ni_mib.rx_ovsize_frame_cnt;
    data->rx_64_frames = ni_mib.rx_frame_64_cnt;
    data->rx_65_127_frames = ni_mib.rx_frame_65to127_cnt;
    data->rx_128_255_frames = ni_mib.rx_frame_128to255_cnt;
    data->rx_256_511_frames = ni_mib.rx_frame_256to511_cnt;
    data->rx_512_1023_frames = ni_mib.rx_frame_512to1023_cnt;
    data->rx_1024_1518_frames = ni_mib.rx_frame_1024to1518_cnt;
    data->rx_1519_max_frames = ni_mib.rx_frame_1519to2100_cnt + ni_mib.rx_frame_2101to9200_cnt + 
                               ni_mib.rx_frame_9201tomax_cnt;
    data->tx_good_octets = ((ca_uint64_t)ni_mib.tx_byte_cnt_high << 32) + (ca_uint64_t)ni_mib.tx_byte_cnt_low;
    data->tx_uc_frames = ni_mib.tx_uc_pkt_cnt;
    data->tx_mc_frames = ni_mib.tx_mc_frame_cnt;
    data->tx_bc_frames = ni_mib.tx_bc_frame_cnt;
    data->tx_error_frames = ni_mib.tx_crc_error_frame_cnt + ni_mib.tx_ovsize_frame_cnt + 
            ni_mib.tx_single_col_frame_cnt + ni_mib.tx_multi_col_frame_cnt +
            ni_mib.tx_late_col_frame_cnt;
    data->tx_pause_frames = ni_mib.tx_pause_frame_cnt;
    data->tx_64_frames = ni_mib.tx_frame_64_cnt;
    data->tx_65_127_frames = ni_mib.tx_frame_65to127_cnt;
    data->tx_128_255_frames = ni_mib.tx_frame_128to255_cnt;
    data->tx_256_511_frames = ni_mib.tx_frame_256to511_cnt;
    data->tx_512_1023_frames = ni_mib.tx_frame_512to1023_cnt;
    data->tx_1024_1518_frames = ni_mib.tx_frame_1024to1518_cnt;
    data->tx_1519_max_frames = ni_mib.tx_frame_1519to2100_cnt + ni_mib.tx_frame_2101to9200_cnt + 
                               ni_mib.tx_frame_9201tomax_cnt;

    return ret;
}

/******************************************************************************
API
    ca_eth_port_eee_enable_set
Description
    These API routines can be used to set or get MAC/PHY EEE (IEEE 802.3az) 
    enable state.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] eee_enable: Enable/Disable EEE function
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_eee_enable_set (
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_port_id_t            port_id,
    CA_IN       ca_boolean_t            eee_enable)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eee_mask_t     eee_msk;
    aal_ni_eee_cfg_t      eee_cfg;
    aal_ni_xge_eee_mask_t xge_eee_msk;
    aal_ni_xge_eee_cfg_t  xge_eee_cfg;
    aal_phy_autoneg_adv_t adv;
    ca_uint32_t           aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x eee_enable = %u.\n",
        device_id, port_id, eee_enable);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_BOOL_VAL_CHECK(eee_enable);

    if(aal_port >= __MAX_ETH_PORT_ID) {
        __ETH_DBG("port_id is exceeded MAX ETH PORT NUM: %d\r\n", aal_port);
        return CA_E_PARAM;
    }

#ifdef RTK_MEM_CORRUPTTION_FIX
    CA_ASSERT_RET(aal_port <= __MAX_ETH_PORT_ID, CA_E_PARAM);
#endif
    __ETH_PORT_LOCK(device_id);

    if (__ETH_IS_GE(aal_port)) {
        eee_msk.u32 = 0;
        memset(&eee_cfg, 0, sizeof(eee_cfg));

        eee_msk.s.tx_eee_enable = 1;
        eee_msk.s.cpu_lpi_deassert = 1;
        eee_msk.s.lpi_min_wakeup_cnt = 1;
        eee_cfg.tx_eee_enable = eee_enable;
        eee_cfg.cpu_lpi_deassert = !eee_enable;
        eee_cfg.lpi_min_wakeup_cnt = eee_enable ? 300 : 10;
        ret = aal_ni_eee_cfg_set(device_id, aal_port, eee_msk, &eee_cfg);
    } else if (__ETH_IS_XGE(aal_port)) {
        xge_eee_msk.u32 = 0;
        memset(&xge_eee_cfg, 0, sizeof(xge_eee_cfg));

        xge_eee_msk.s.eee_enable = 1;
        xge_eee_cfg.eee_enable = eee_enable;
        ret = aal_ni_xge_eee_cfg_set(device_id, aal_port, xge_eee_msk, &xge_eee_cfg);
    }

    if (__ETH_DETECTED_PHY(aal_port)) {
        memset(&adv, 0, sizeof(adv));
        ret = aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(aal_port), &adv);
        if (CA_E_OK == ret) {
            adv.adv_10base_te = 1;
            adv.adv_100base_tx_eee = 1;
            adv.adv_1000base_t_eee = 1;
            adv.adv_10gbase_t_eee = __ETH_IS_XGE(aal_port) ? 1 : 0;
            ret = aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(aal_port), adv);
        }
    }

    if (CA_E_OK == ret) {
        g_eth_port_drv.eee_enable[aal_port] = eee_enable;
    }
    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
    ca_eth_port_eee_enable_get
Description
    These API routines can be used to get MAC/PHY EEE (IEEE 802.3az) 
    enable state.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [OUT] eee_enable: Enable/Disable EEE function
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_eee_enable_get (
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_port_id_t            port_id,
    CA_OUT      ca_boolean_t           *eee_enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x eee_enable = %p.\n",
        device_id, port_id, eee_enable);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(eee_enable);

   *eee_enable = g_eth_port_drv.eee_enable[aal_port];

    return ret;
}

#ifdef SUPPORT_DOS_ATTACK
#include "rate.h"
/******************************************************************************
API
    ca_eth_port_dos_attack_set
Description
    The API routine can be used to set the DoS attack detection 
    configuration for a port.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] dos_types: Types of DoS pattern to detect 
                   (see definitions CA_DOS_ATTACK_TYPE_XXX)
    [IN] trap_flags: 0=drop, 1=trap to CPU. 1 bit for each of the DoS type 
                     (see definitions CA_DOS_ATTACK_TYPE_XXX)
    [IN] policer_ids: Valid if trap flag = 1.Policer ID used for rate limiting
                      packets trapped to CPU
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_dos_attack_set (
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_port_id_t            port_id,
    CA_IN       ca_uint32_t             dos_types,
    CA_IN       ca_uint32_t             trap_flags,
    CA_IN       ca_policer_t           *policer_ids
)
{
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(policer_ids);

    // TODO: for Gateway SoC 
    
    return CA_E_OK;
}

/******************************************************************************
API
    ca_eth_port_dos_attack_get
Description
    The API routine can be used to get the DoS attack detection 
    configuration for a port.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] dos_types: Types of DoS pattern to detect
    [IN] trap_flags: 0=drop, 1=trap to CPU. 1 bit for each of the DoS type
    [OUT] policer_ids: Policer ID used for rate limiting packets trapped to CPU
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_dos_attack_get (
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_port_id_t            port_id,
    CA_OUT      ca_uint32_t             dos_types,
    CA_OUT      ca_uint32_t            *trap_flags,
    CA_OUT      ca_policer_t           *policer_ids
)
{
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(trap_flags);
    CA_NULL_PTR_CHECK(policer_ids);

    // TODO: for Gateway SoC 
    
    return CA_E_OK;
}
#endif

/******************************************************************************
 API
     ca_eth_port_stats_get
 Description
     These APIs can be used to get port Tx/Rx statistics.
 Parameters
     [IN] device_id: CA device number
     [IN] port_id: Ethernet Port ID
     [IN] read_clear: Counter read clear flag {FALSE = don't clear after read, TRUE = clear after read}
     [OUT] data: Statistics data
 Returns
     CA_E_OK: Operation completed successfully
     CA_E_XXX: Other errors
 ******************************************************************************/
ca_status_t 
ca_eth_port_stats_get(
    CA_IN ca_device_id_t device_id,
                 CA_IN ca_port_id_t port_id,
                 CA_IN ca_boolean_t read_clear,
    CA_OUT ca_eth_port_stats_t* data)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
#if 0
    __ETH_DBG("device_id = %u port_id = %#x read_clear = %u data = %p.\n",
        device_id, port_id, read_clear, data);
#endif
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(data);
    CA_BOOL_VAL_CHECK(read_clear);

    __ETH_PORT_LOCK(device_id);

    memset ( data, 0, sizeof( ca_eth_port_stats_t ) );

    if (__ETH_IS_XGE(aal_port)) 
    {
        /* XGE */
        ret = __xge_stat_get(device_id, aal_port, read_clear, data);
    } 
    else 
    {
        ret = __ge_stat_get(device_id, aal_port, read_clear, data);
    }

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
 API
 ca_eth_port_autoneg_set
 Description
    These APIs can be used to enable, disable or retrieve the auto-negotiation
    function of a port (MAC layer). If auto-negotiation function is enabled 
    before, the re-enable operation will cause auto-negotiation to be restarted.
 Parameters
     [IN] device_id: CA device number
     [IN] port_id: Ethernet Port ID
     [IN] enable: Enable or disable auto-negotiation function
 Returns
     CA_E_OK: Operation completed successfully
     CA_E_XXX: Other errors
 ******************************************************************************/
ca_status_t ca_eth_port_autoneg_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,
                    CA_IN ca_boolean_t enable)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  aal_port = PORT_ID(port_id);


    __ETH_DBG("device_id = %u port_id = %#x enable = %u.\n",
        device_id, port_id, enable);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_BOOL_VAL_CHECK(enable);

    __ETH_PORT_LOCK(device_id);
    if (__ETH_IS_GE(aal_port)) {
        ret = aal_ni_eth_port_config_set(device_id, aal_port, AAL_NI_MAC_SPEED_1000M);
    }
    if (CA_E_OK == ret) {
        g_eth_port_drv.autoneg[aal_port] = enable;
    } else {
        __ETH_DBG("aal_ni_eth_port_config_set fail, return %d.\r\n", ret);
    }
    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
ca_eth_port_autoneg_get
Description
   These APIs can be used to retrieve the auto-negotiation function of a 
   port (MAC layer). If auto-negotiation function is enabled  before,
   the re-enable operation will cause auto-negotiation to be restarted.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] enable: Enable or disable auto-negotiation function
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_autoneg_get(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,
                    CA_OUT ca_boolean_t * enable)
{
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x enable = %p.\n",
        device_id, port_id, enable);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(enable);

    *enable = g_eth_port_drv.autoneg[aal_port];

    return CA_E_OK;
}

/******************************************************************************
 API
 ca_eth_port_phy_autoneg_set
 Description
    These APIs can be used to enable, disable  the auto-negotiation function 
    of a port (PHY layer). If auto-negotiation function is enabled before,
    the re-enabling operation will cause auto-negotiation to be restarted.
    Note that prior to calling this API, the desired advertised mode must be
    configured.
    These APIs are supported only for PHYs integrated into Cortina-Access ASICs
 Parameters
     [IN] device_id: CA device number
     [IN] port_id: Ethernet Port ID
     [IN] enable: Enable or disable auto-negotiation function
 Returns
     CA_E_OK: Operation completed successfully
     CA_E_XXX: Other errors
 ******************************************************************************/
ca_status_t ca_eth_port_phy_autoneg_set (
        CA_IN ca_device_id_t  device_id,
        CA_IN ca_port_id_t    port_id,
        CA_IN ca_boolean_t    enable)
{
    ca_boolean_t prev_cfg = FALSE;
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  aal_port = PORT_ID(port_id);


    __ETH_DBG("device_id = %u port_id = %#x enable = %u.\n",
        device_id, port_id, enable);

#ifdef RTK_MEM_CORRUPTTION_FIX
    CA_ASSERT_RET(aal_port <= __MAX_ETH_PORT_ID, CA_E_PARAM);
#endif

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_BOOL_VAL_CHECK(enable);

#ifndef CONFIG_CORTINA_BOARD_FPGA

    if (__ETH_IS_XGE(aal_port) && !__ETH_DETECTED_PHY(aal_port)) {/* XGE */
#if defined(CONFIG_NE_AAL_XFI)
        if (__ETH_IS_XFI(aal_port)) {
            __ETH_PORT_LOCK(device_id);
            ret= aal_xfi_autoneg_set(device_id, aal_xfi_port2xfi(aal_port), enable);
            __ETH_PORT_UNLOCK(device_id);
            CA_RET_VALUE_CHECK(ret);
        }
#endif
        return ret;
    }
#endif /* CONFIG_CORTINA_BOARD_FPGA */

    if(aal_port >= __MAX_ETH_PORT_ID) {
        __ETH_DBG("port_id is exceeded MAX ETH PORT NUM: %d\r\n", aal_port);
        return CA_E_PARAM;
    }

    if (!__ETH_DETECTED_PHY(aal_port)) {
        __ETH_DBG("There is no Etherent PHY connected to port %d\r\n", aal_port);
        return CA_E_OK;
    }

    __ETH_PORT_LOCK(device_id);

    ret = aal_eth_phy_auto_neg_get(device_id,
                       __ETH_PHY_ADDR(aal_port), &prev_cfg);

    if (CA_E_OK == ret) {
        if (prev_cfg && enable) {/* re-enable autoneg */
            ret = aal_eth_phy_auto_neg_restart(device_id,
                               __ETH_PHY_ADDR(aal_port));
        } else {
            ret = aal_eth_phy_auto_neg_set(device_id,
                                __ETH_PHY_ADDR(aal_port), enable);
        }
    }

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
 API
 ca_eth_port_phy_autoneg_get
 Description
    These APIs can be used to retrieve the auto-negotiation function 
    of a port (PHY layer). If auto-negotiation function is enabled before,
    the re-enabling operation will cause auto-negotiation to be restarted.
    Note that prior to calling this API, the desired advertised mode must be
    configured.
    These APIs are supported only for PHYs integrated into Cortina-Access ASICs
 Parameters
     [IN] device_id: CA device number
     [IN] port_id: Ethernet Port ID
     [OUT] enable: Enable or disable auto-negotiation function
 Returns
     CA_E_OK: Operation completed successfully
     CA_E_XXX: Other errors
 ******************************************************************************/
ca_status_t ca_eth_port_phy_autoneg_get (
        CA_IN  ca_device_id_t  device_id,
        CA_IN  ca_port_id_t    port_id,
        CA_OUT ca_boolean_t   *enable)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x enable = %p.\n",
        device_id, port_id, enable);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(enable);

    if(aal_port >= __MAX_ETH_PORT_ID) {
        __ETH_DBG("port_id is exceeded MAX ETH PORT NUM: %d\r\n", aal_port);
        return CA_E_PARAM;
    }

#ifndef CONFIG_CORTINA_BOARD_FPGA
    if (__ETH_IS_XGE(aal_port) && !__ETH_DETECTED_PHY(aal_port)) {/* XGE */
#if defined(CONFIG_NE_AAL_XFI)
            if (__ETH_IS_XFI(aal_port)) {
                ret= aal_xfi_autoneg_get(device_id, aal_xfi_port2xfi(aal_port), enable);
                CA_RET_VALUE_CHECK(ret);
            }
#endif
        return ret;
    }
#endif /* CONFIG_CORTINA_BOARD_FPGA */
#ifdef RTK_MEM_CORRUPTTION_FIX
    if (aal_port >= __MAX_ETH_PORT_NUM) {
	printk("port_id >= __MAX_ETH_PORT_NUM(8)\n");
	return CA_E_ERROR;
    }	    
#endif
    if (!__ETH_DETECTED_PHY(aal_port)) {
        __ETH_DBG("There is no Etherent PHY connected to port %d\r\n", aal_port);
        return CA_E_NOT_FOUND;
    }

    __ETH_PORT_LOCK(device_id);

    ret = aal_eth_phy_auto_neg_get(device_id,
                       __ETH_PHY_ADDR(aal_port), enable);
    __ETH_PORT_UNLOCK(device_id);

    return ret;
}

/******************************************************************************
 API
     ca_eth_port_phy_advert_set
 Description
     These APIs can be used to set the advertisement settings for
     the specified port (PHY layer). If the port is operating with auto-negotiation
     disabled, these settings will only be configured into hardware but not take
     effect till auto-negotiation is enabled. If the port is in auto-negotiation
     enabled state when these settings are changed, auto-negotiation must be 
     restarted using ca_eth_port_phy_autoneg_set. Setting or getting current 
     ability can be done at any time, including in auto-negotiation disabled state.
     However the setting will only take effect when auto-negotiation is enabled or 
     restarted.
     NOTE: These APIs are supported only for PHYs integrated into Cortina-Access ASICs
 Parameters
     [IN] device_id: CA device number
     [IN] port_id: Ethernet Port ID
     [IN] ability: Mask for port advertisement ability
 Returns
     CA_E_OK: Operation completed successfully
     CA_E_XXX: Other errors
 ******************************************************************************/
ca_status_t ca_eth_port_phy_advert_set(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,
                   CA_IN ca_eth_port_ability_t ability)
{
    ca_status_t ret = CA_E_OK;
    aal_phy_autoneg_adv_t adv;
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x "\
        "ability.half_10 = %u "\
        "ability.full_10 = %u "\
        "ability.half_100 = %u "\
        "ability.full_100 = %u "\
        "ability.full_1g = %u "\
        "ability.full_10g = %u "\
        "ability.full_2d5g = %u "\
        "ability.full_5g = %u "\
        "ability.flow_ctrl = %u "\
        "ability.asym_flow_ctrl = %u .\n",
        device_id, port_id,
        ability.half_10,
        ability.full_10,
        ability.half_100,
        ability.full_100,
        ability.full_1g,
        ability.full_10g,
        ability.full_2d5g,
        ability.full_5g,
        ability.flow_ctrl,
        ability.asym_flow_ctrl);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    memset(&adv, 0, sizeof(adv));


    __ETH_PORT_LOCK(device_id);

    if (__ETH_DETECTED_PHY(aal_port)) {
        ret = aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(aal_port), &adv);

        adv.adv_10base_t_half   = ability.half_10;
        adv.adv_10base_t_full   = ability.full_10;
        adv.adv_100base_tx_half = ability.half_100;
        adv.adv_100base_tx_full = ability.full_100;
        adv.adv_1000base_t_full = ability.full_1g;
        adv.adv_10000base_t_full= ability.full_10g;
        adv.adv_2500base_t_full = ability.full_2d5g;
        adv.adv_5000base_t_full = ability.full_5g;
        adv.adv_pause           = ability.flow_ctrl;
        adv.adv_asym_pause      = ability.asym_flow_ctrl;
        adv.adv_100base_tx_eee  = ability.eee_100_capable;
        adv.adv_1000base_t_eee  = ability.eee_1g_capable;
        adv.adv_10gbase_t_eee   = ability.eee_10g_capable;
        adv.adv_2d5gbase_t_eee  = ability.eee_2d5g_capable;
        adv.adv_5gbase_t_eee    = ability.eee_5g_capable;

        ret = aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(aal_port), adv);
    }

    __ETH_PORT_UNLOCK(device_id);

    if (CA_E_OK == ret) {
        memcpy(&g_eth_port_drv.adv[aal_port], &ability, sizeof(ca_eth_port_ability_t));
    }

    return ret;
}

/******************************************************************************
API
    ca_eth_port_phy_advert_get
Description
    These APIs can be used to get the advertisement settings for
    the specified port (PHY layer). If the port is operating with auto-negotiation
    disabled, these settings will only be configured into hardware but not take
    effect till auto-negotiation is enabled. If the port is in auto-negotiation
    enabled state when these settings are changed, auto-negotiation must be 
    restarted using ca_eth_port_phy_autoneg_set. Setting or getting current 
    ability can be done at any time, including in auto-negotiation disabled state.
    However the setting will only take effect when auto-negotiation is enabled or 
    restarted.
    NOTE: These APIs are supported only for PHYs integrated into Cortina-Access ASICs
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [OUT] ability: Mask for port advertisement ability
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_phy_advert_get(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,
                   CA_OUT ca_eth_port_ability_t * ability)
{
    ca_status_t ret = CA_E_OK;
    aal_phy_autoneg_adv_t adv;
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x ability = %p.\n",
        device_id, port_id, ability);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(ability);

    memset(&adv, 0, sizeof(adv));

    __ETH_PORT_LOCK(device_id);

    if (__ETH_DETECTED_PHY(aal_port)) {
        ret = aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(aal_port), &adv);
    } else {
        memcpy(ability, &g_eth_port_drv.adv[aal_port], sizeof(ca_eth_port_ability_t));
    }

    __ETH_PORT_UNLOCK(device_id);

    ability->half_10        = adv.adv_10base_t_half;
    ability->full_10        = adv.adv_10base_t_full;
    ability->half_100       = adv.adv_100base_tx_half;
    ability->full_100       = adv.adv_100base_tx_full;
    ability->full_1g        = adv.adv_1000base_t_full;
    ability->full_10g       = adv.adv_10000base_t_full;
    ability->full_2d5g      = adv.adv_2500base_t_full;
    ability->full_5g        = adv.adv_5000base_t_full;
    ability->flow_ctrl      = adv.adv_pause;
    ability->asym_flow_ctrl = adv.adv_asym_pause;
    ability->eee_100_capable = adv.adv_100base_tx_eee;
    ability->eee_1g_capable  = adv.adv_1000base_t_eee;
    ability->eee_10g_capable = adv.adv_10gbase_t_eee;
    ability->eee_2d5g_capable= adv.adv_2d5gbase_t_eee;
    ability->eee_5g_capable  = adv.adv_5gbase_t_eee;

    return ret;

}

/******************************************************************************
API
   ca_eth_port_phy_advert_remote_get
Description
   The API can be used to get the advertisement for the link partner of 
   port (PHY layer), which is captured by local PHY during auto-negotiation.
   If auto-negotiation is in progress or disabled, the remote 
   advertisement is returned as 0. 
   NOTE: These APIs are supported only for PHYs integrated into Cortina-Access ASICs
Parameters
   [IN] device_id: CA device number
   [IN] port_id: Ethernet Port ID
   [OUT] ability: Mask for port advertisement ability
Returns
   CA_E_OK: Operation completed successfully
   CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_phy_advert_remote_get(CA_IN ca_device_id_t device_id,
                      CA_IN ca_port_id_t port_id,
                      CA_OUT ca_eth_port_ability_t *ability)
{
    ca_status_t ret = CA_E_OK;
    aal_phy_autoneg_adv_t adv;
    ca_uint32_t  aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x ability = %p.\n",
        device_id, port_id, ability);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(ability);

    if (!__ETH_DETECTED_PHY(aal_port)) {
        __ETH_DBG("There is no Etherent PHY connected to port %d\r\n", aal_port);
        return CA_E_NOT_FOUND;
    }

    memset(&adv, 0, sizeof(adv));

    __ETH_PORT_LOCK(device_id);
    ret = aal_eth_phy_partner_ability_adv_get(device_id,
                          __ETH_PHY_ADDR(aal_port),
                          &adv);
    __ETH_PORT_UNLOCK(device_id);

    ability->half_10        = adv.adv_10base_t_half;
    ability->full_10        = adv.adv_10base_t_full;
    ability->half_100       = adv.adv_100base_tx_half;
    ability->full_100       = adv.adv_100base_tx_full;
    ability->full_1g        = adv.adv_1000base_t_half;
    ability->full_10g       = adv.adv_10000base_t_full;
    ability->full_2d5g      = adv.adv_2500base_t_full;
    ability->full_5g        = adv.adv_5000base_t_full;
    ability->flow_ctrl      = adv.adv_pause;
    ability->asym_flow_ctrl = adv.adv_asym_pause;
    ability->eee_100_capable = adv.adv_100base_tx_eee;
    ability->eee_1g_capable  = adv.adv_1000base_t_eee;
    ability->eee_10g_capable = adv.adv_10gbase_t_eee;
    ability->eee_2d5g_capable= adv.adv_2d5gbase_t_eee;
    ability->eee_5g_capable  = adv.adv_5gbase_t_eee;

    return ret;

}

/******************************************************************************
API
   ca_eth_port_speed_set
Description
   These API routines can be used to set or get the link speed for a port (MAC layer).
   Setting operation will cause auto-negotiation to be disabled if it is enabled.
   When retrieving the speed for a port, current negotiated speed is returned if
   auto-negotiation is enabled. If auto-negotiation is in progress or link is not
   established properly, speed is returned as value 0. If auto-negotiation is disabled,
   the configured speed of the port is returned regardless of the link state.
Parameters
   [IN] device_id: CA device number
   [IN] port_id: Ethernet Port ID
   [IN] speed: Selector for operating speed
Returns
   CA_E_OK: Operation completed successfully
   CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_speed_set(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_IN ca_eth_port_speed_t speed)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x speed = %u.\n",
        device_id, port_id, speed);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_ASSERT_RET(speed > CA_PORT_SPEED_INVALID
              && speed <= CA_PORT_SPEED_AUTO, CA_E_PARAM);

    if(aal_port >= __MAX_ETH_PORT_ID) {
        __ETH_DBG("port_id is exceeded MAX ETH PORT NUM: %d\r\n", aal_port);
        return CA_E_PARAM;
    }

    __ETH_PORT_LOCK(device_id);

    if (__ETH_IS_XGE(aal_port)) {/* XGE */
        if (speed < CA_PORT_SPEED_100M) {
            __ETH_DBG("port %#x cannot support speed %d\n", port_id, speed);
            ret = CA_E_OK;
            goto __end;
        }

    } else {
        if (speed == CA_PORT_SPEED_AUTO) {
            speed = CA_PORT_SPEED_1G;
        }

        if (speed > CA_PORT_SPEED_1G) {
            __ETH_DBG("port %#x cannot support speed %d\n", port_id, speed);
            ret = CA_E_NOT_SUPPORT;
            goto __end;
        }
        eth_mac_mask.u32 = 0;
        eth_mac_mask.s.speed = 1;

        memset(&eth_mac_config, 0, sizeof(eth_mac_config));
        eth_mac_config.speed = _SPEED2MAC(speed);
        
        ret = aal_ni_eth_port_mac_set(device_id, aal_port,
                              eth_mac_mask, &eth_mac_config);
    }
#ifdef RTK_MEM_CORRUPTTION_FIX
    if (CA_E_OK == ret && aal_port >= 0 && aal_port < 8) {
#else
    if (CA_E_OK == ret) {
#endif
        g_eth_port_drv.speed[aal_port] = (ca_uint8_t)speed;
    }
__end:

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
 ca_eth_port_speed_get
Description
 These API routines can be used to get the link speed for a port (MAC layer).
 Setting operation will cause auto-negotiation to be disabled if it is enabled.
 When retrieving the speed for a port, current negotiated speed is returned if
 auto-negotiation is enabled. If auto-negotiation is in progress or link is not
 established properly, speed is returned as value 0. If auto-negotiation is disabled,
 the configured speed of the port is returned regardless of the link state.
Parameters
 [IN] device_id: CA device number
 [IN] port_id: Ethernet Port ID
 [OUT] speed: Selector for operating speed
Returns
 CA_E_OK: Operation completed successfully
 CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_speed_get(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_OUT ca_eth_port_speed_t * speed)
{
    ca_uint32_t aal_port = PORT_ID(port_id);


    __ETH_DBG("device_id = %u port_id = %#x speed = %p.\n",
      device_id, port_id, speed);

    CA_NULL_PTR_CHECK(speed);
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    *speed = g_eth_port_drv.speed[aal_port];

    return CA_E_OK;

}

/******************************************************************************
API
ca_eth_port_phy_speed_set
Description
    These API routines can be used to set the link speed for a port 
    (PHY layer). Setting operation will cause auto-negotiation to be disabled
    if it is enabled.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] speed: Selector for operating speed
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
NOTE
    These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_speed_set(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_IN ca_eth_port_speed_t speed)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
    aal_phy_speed_mode_t aal_speed = 0;
    aal_phy_speed_mode_t aal_config_speed = 0;
    char phy_name[32] = {0};

    __ETH_DBG("device_id = %u port_id = %#x speed = %u.\n",
        device_id, port_id, speed);
    
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    CA_ASSERT_RET(speed > CA_PORT_SPEED_INVALID
              && speed <= CA_PORT_SPEED_AUTO, CA_E_PARAM);

    if(aal_port >= __MAX_ETH_PORT_ID) {
        __ETH_DBG("port_id is exceeded MAX ETH PORT NUM: %d\r\n", aal_port);
        return CA_E_PARAM;
    }

#ifdef RTK_MEM_CORRUPTTION_FIX
    CA_ASSERT_RET(aal_port <= __MAX_ETH_PORT_ID, CA_E_PARAM);
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_port == AAL_NI_INTERNAL_XE) /* internal XE port */
        return CA_E_OK;
#endif

    if (speed == CA_PORT_SPEED_AUTO) {
        aal_config_speed = __ETH_IS_XGE(aal_port) ? AAL_PHY_SPEED_10000 : AAL_PHY_SPEED_1000;
    } else {
        aal_config_speed = _SPEED2PHY(speed);
    }

    if (__ETH_IS_GE(aal_port) && speed > CA_PORT_SPEED_1G &&  speed != CA_PORT_SPEED_AUTO) {
        __ETH_DBG("Not support the speed %d for port %#x\r\n", speed, port_id);
        return CA_E_PARAM;
    }

    __ETH_PORT_LOCK(device_id);

    if (__ETH_DETECTED_PHY(aal_port)) {
        if ((speed == CA_PORT_SPEED_100M || speed == CA_PORT_SPEED_10M) && __ETH_IS_XGE(aal_port)) {

            aal_eth_phy_name_get(device_id,  __ETH_PHY_ADDR(aal_port), phy_name);
            //ca_printf("PHY name %s\r\n", phy_name);
            if (0 == strncasecmp(phy_name, "AQR107", 5)) {/* work-around for AQR107 PHY */
                aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(aal_port), 1);
                //ca_printf("1enable AUTONEG for speed %d!\n", speed);
            } else {
                aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(aal_port), 0);
                //ca_printf("1disable AUTONEG for speed %d!\n", speed);
            }
        } else if (speed != CA_PORT_SPEED_AUTO && speed < CA_PORT_SPEED_1G) {/* disable autoneg */
            aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(aal_port), 0);
            //ca_printf("2disable AUTONEG speed %d!\n", speed);
        } else if (speed >= CA_PORT_SPEED_1G){ /* always enable autoneg for 1G,2.5G,5G,10G */
            aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(aal_port), 1);
            //ca_printf("3enable AUTONEG for speed %d!\n", speed);
        }
#ifdef ETH_TEST_ONLY
        if (speed != CA_PORT_SPEED_AUTO) { /* disable unused ability*/
            aal_phy_autoneg_adv_t adv, adv_old;

            memset(&adv_old, 0, sizeof(adv_old));
            ret = aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(aal_port), &adv_old);
            if (CA_E_OK != ret)
                goto __api_end;
            
            memset(&adv, 0, sizeof(adv));
            /* save the fields which is NOT speed */
            adv.adv_asym_pause     = adv_old.adv_asym_pause;
            adv.adv_pause          = adv_old.adv_pause;
            adv.adv_10base_te      = adv_old.adv_10base_te;
            adv.adv_10gbase_t_eee  = adv_old.adv_10gbase_t_eee;
            adv.adv_100base_tx_eee = adv_old.adv_100base_tx_eee;
            adv.adv_1000base_t_eee = adv_old.adv_1000base_t_eee;
            adv.adv_2d5gbase_t_eee = adv_old.adv_2d5gbase_t_eee;
            adv.adv_5gbase_t_eee = adv_old.adv_5gbase_t_eee;
            /* only open the speed which configured */
            switch(speed) {
                case CA_PORT_SPEED_10M:
                    adv.adv_10base_t_full = 1;
                    adv.adv_10base_t_half = 1;
                    break;
                    
                case CA_PORT_SPEED_100M :
                    adv.adv_100base_tx_full = 1;
                    adv.adv_100base_tx_half = 1;
                    break;
                    
                case CA_PORT_SPEED_1G :
                    adv.adv_1000base_t_full = 1;
                    adv.adv_1000base_t_half = 1;
                    break;

                case CA_PORT_SPEED_2_5G :
                    adv.adv_2500base_t_full = 1;
                    adv.adv_2500base_t_half = 0;
                    break;

                case CA_PORT_SPEED_5G :
                    adv.adv_5000base_t_full = 1;
                    adv.adv_5000base_t_half = 0;
                    break;
                case CA_PORT_SPEED_10G:
                    adv.adv_10000base_t_full = 1;
                    break;
                default:
                    break;
            }
            ret = aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(aal_port), adv);
        }
#endif /* ETH_TEST_ONLY */
        ret = aal_eth_phy_speed_get(device_id,
            __ETH_PHY_ADDR(aal_port), &aal_speed);
        if (CA_E_OK != ret ) {
            __ETH_DBG("aal_eth_phy_speed_get fail for port %#x\r\n", port_id);
            goto __api_end;
        }

        ret = aal_eth_phy_speed_set(device_id,
            __ETH_PHY_ADDR(aal_port), aal_config_speed);
        if (CA_E_OK != ret ) {
            __ETH_DBG("aal_eth_phy_speed_set speed %d fail for port %#x\r\n", aal_config_speed, port_id);
            goto __api_end;
        }

        if (aal_speed != aal_config_speed) {/* change config then power reset */
            ret |= aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(aal_port), 0);
            ret |= aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(aal_port), 1);
        }

    }

#ifndef CONFIG_CORTINA_BOARD_FPGA
    if (__ETH_IS_XGE(aal_port) && !g_eth_port_drv.disable_phy2mac_sync[port_id]) {/* XGE */
#if defined(CONFIG_NE_AAL_XFI)
        aal_phy_sxgmii_pcs_control_index_t pcs_index;
        aal_phy_speed_mode_t pcs_speed;
        pcs_speed = aal_config_speed;
        pcs_index = aal_xfi_port2xfi(aal_port);

        ret = aal_xfi_speed_set(device_id, pcs_index, pcs_speed);
#endif
    }
#endif /* CONFIG_CORTINA_BOARD_FPGA */

__api_end:

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
ca_eth_port_phy_speed_get
Description
  These API routines can be used to get the link speed for a port 
  (PHY layer). Setting operation will cause auto-negotiation to be disabled
  if it is enabled. When retrieving the speed for a port, current negotiated
  speed is returned if auto-negotiation is enabled. If auto-negotiation is in
  progress or link is not established properly, speed is returned as value 0.
  If auto-negotiation is disabled, the configured speed of the port is 
  returned regardless of the link state.
Parameters
  [IN] device_id: CA device number
  [IN] port_id: Ethernet Port ID
  [IN] speed: Selector for operating speed
Returns
  CA_E_OK: Operation completed successfully
  CA_E_XXX: Other errors
NOTE
  These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_speed_get(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_OUT ca_eth_port_speed_t * speed)
{
    ca_status_t ret = CA_E_OK;
    aal_phy_link_status_t phy_lnk;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x speed = %p.\n",
        device_id, port_id, speed);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(speed);

    memset(&phy_lnk, 0, sizeof(phy_lnk));

    __ETH_PORT_LOCK(device_id);
    ret = __eth_link_status_get(device_id, aal_port, &phy_lnk);
    __ETH_PORT_UNLOCK(device_id);

    if (CA_E_OK != ret) {
        *speed = CA_PORT_SPEED_10M;
    } else {/* link down then return 10M always */
        *speed = phy_lnk.link_up ? _PHY2SPEED(phy_lnk.speed) : CA_PORT_SPEED_10M;
    }

    return ret;

}

/******************************************************************************
API
ca_eth_port_duplex_set
Description
    These API routines can be used to set or get the link duplex mode for
    a port (MAC layer). Setting operation will cause auto-negotiation to
    be disabled if it is enabled. When retrieving the duplex mode for a
    port, current negotiated duplex status is returned if auto-negotiation
    is enabled. If auto-negotiation is in progress or link is not established
    properly, duplex is returned as value 0(half-duplex). If auto-negotiation
    is disabled, the configured duplex of the port is returned regardless of
    the link state.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] duplex: Duplex mode
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_duplex_set(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,
                   CA_IN ca_eth_port_duplex_t duplex)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x duplex = %u.\n",
        device_id, port_id, duplex);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_ASSERT_RET(duplex <= CA_PORT_DUPLEX_AUTO, CA_E_PARAM);

    if (__ETH_IS_XGE(aal_port)) {/* XGE */
        return CA_E_OK;
    }
    
    eth_mac_mask.u32 = 0;
    eth_mac_mask.s.duplex = 1;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    eth_mac_config.duplex = (duplex == CA_PORT_DUPLEX_AUTO) ? CA_PORT_DUPLEX_FULL : duplex;

    __ETH_PORT_LOCK(device_id);

    ret = aal_ni_eth_port_mac_set(device_id, aal_port,
                      eth_mac_mask, &eth_mac_config);

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
ca_eth_port_duplex_get
Description
    These API routines can be used to get the link duplex mode for
    a port (MAC layer). Setting operation will cause auto-negotiation to
    be disabled if it is enabled. When retrieving the duplex mode for a
    port, current negotiated duplex status is returned if auto-negotiation
    is enabled. If auto-negotiation is in progress or link is not established
    properly, duplex is returned as value 0(half-duplex). If auto-negotiation
    is disabled, the configured duplex of the port is returned regardless of
    the link state.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [OUT] duplex: Duplex mode
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
******************************************************************************/
ca_status_t ca_eth_port_duplex_get(CA_IN ca_device_id_t device_id,
                  CA_IN  ca_port_id_t port_id,
                  CA_OUT ca_eth_port_duplex_t *duplex)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_t eth_mac_config;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x duplex = %p.\n",
       device_id, port_id, duplex);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(duplex);

    if (__ETH_IS_XGE(aal_port)) {/* XGE */
        *duplex = CA_PORT_DUPLEX_FULL;
        return CA_E_OK;
    }

    /* GE */
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));
    __ETH_PORT_LOCK(device_id);
    ret = aal_ni_eth_port_mac_get(device_id, aal_port, &eth_mac_config);
    __ETH_PORT_UNLOCK(device_id);

    if (CA_E_OK != ret) {
        __ETH_DBG("port %#x aal_ni_eth_port_mac_get fail, return %d.\n", ret);
    } else {
        *duplex = eth_mac_config.duplex;
    }

    return ret;

}

/******************************************************************************
API
ca_eth_port_phy_duplex_set
Description
    These API routines can be used to set the link duplex mode for
    a port (PHY layer). Setting operation will cause auto-negotiation to
    be disabled if it is enabled.
Parameters
  [IN] device_id: CA device number
  [IN] port_id: Ethernet Port ID
  [IN] duplex: Duplex mode
Returns
  CA_E_OK: Operation completed successfully
  CA_E_XXX: Other errors
NOTE
  These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_duplex_set(CA_IN ca_device_id_t device_id,
                 CA_IN ca_port_id_t port_id,
                 CA_IN ca_eth_port_duplex_t duplex)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_boolean_t is_full = 0;
#ifdef ETH_TEST_ONLY
    aal_phy_autoneg_adv_t adv, adv_old;
    aal_phy_speed_mode_t  phy_speed = 0;
#endif
    __ETH_DBG("device_id = %u port_id = %#x duplex = %u.\n",
      device_id, port_id, duplex);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_ASSERT_RET(duplex <= CA_PORT_DUPLEX_AUTO, CA_E_PARAM);

    if (!__ETH_DETECTED_PHY(aal_port)) {/* NOT detect PHY (intergrated or external) */
        if (__ETH_IS_XGE(aal_port)) {
            if (duplex != CA_PORT_DUPLEX_FULL) /* For XE ports SerDes, only full duplex can support */
                return CA_E_NOT_SUPPORT;
            else
                return CA_E_OK;
        } else {
            __ETH_DBG("NOT found PHY connected to port %d\r\n", aal_port);
            return CA_E_NOT_FOUND;
        }
    }

    __ETH_PORT_LOCK(device_id);
#ifdef ETH_TEST_ONLY
    memset(&adv_old, 0, sizeof(adv_old));
    memset(&adv, 0, sizeof(adv));
    ret = aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(aal_port), &adv_old);
    if (CA_E_OK != ret)
        goto __api_end;
    memcpy(&adv, &adv_old, sizeof(adv));
    ret = aal_eth_phy_speed_get(device_id, __ETH_PHY_ADDR(aal_port),  &phy_speed);
    if (CA_E_OK != ret)
        goto __api_end;

    if (duplex == CA_PORT_DUPLEX_AUTO) { /* AUTO */
        switch (phy_speed) {
        case AAL_PHY_SPEED_10:
            adv.adv_10base_t_full = 1;
            adv.adv_10base_t_half = 1;
            break;
        case AAL_PHY_SPEED_100:
            adv.adv_100base_tx_full = 1;
            adv.adv_100base_tx_half = 1;
            break;
        case AAL_PHY_SPEED_1000:
            adv.adv_1000base_t_full = 1;
            adv.adv_1000base_t_half = 1;
            break;
        case AAL_PHY_SPEED_2500:
            adv.adv_2500base_t_full = 1;
            adv.adv_2500base_t_half = 1;
            break;
        case AAL_PHY_SPEED_5000:
            adv.adv_5000base_t_full = 1;
            adv.adv_5000base_t_half = 1;
            break;
        case AAL_PHY_SPEED_10000:
            adv.adv_10000base_t_full = 1;
            break;
        default:
            break;
        }
    } else if (duplex == CA_PORT_DUPLEX_HALF) {/* HALF: disable FULL ability*/
        switch (phy_speed) {
        case AAL_PHY_SPEED_10:
            adv.adv_10base_t_full = 0;
            adv.adv_10base_t_half = 1;
            break;
        case AAL_PHY_SPEED_100:
            adv.adv_100base_tx_full = 0;
            adv.adv_100base_tx_half = 1;
            break;
        case AAL_PHY_SPEED_1000:
            adv.adv_1000base_t_full = 0;
            adv.adv_1000base_t_half = 1;
            break;
        case AAL_PHY_SPEED_2500:
            adv.adv_2500base_t_full = 0;
            adv.adv_2500base_t_half = 1;
            break;
        case AAL_PHY_SPEED_5000:
            adv.adv_5000base_t_full = 0;
            adv.adv_5000base_t_half = 1;
            break;
        case AAL_PHY_SPEED_10000:
            adv.adv_10000base_t_full = 0;
            break;
            
        default:
            break;
        }
    } else { /* FULL disable HALF ability*/
        switch (phy_speed) {
        case AAL_PHY_SPEED_10:
            adv.adv_10base_t_full = 1;
            adv.adv_10base_t_half = 0;
            break;
        case AAL_PHY_SPEED_100:
            adv.adv_100base_tx_full = 1;
            adv.adv_100base_tx_half = 0;
            break;
        case AAL_PHY_SPEED_1000:
            adv.adv_1000base_t_full = 1;
            adv.adv_1000base_t_half = 0;
            break;
        case AAL_PHY_SPEED_2500:
            adv.adv_2500base_t_full = 1;
            adv.adv_2500base_t_half = 0;
            break;
        case AAL_PHY_SPEED_5000:
            adv.adv_5000base_t_full = 1;
            adv.adv_5000base_t_half = 0;
            break;
        case AAL_PHY_SPEED_10000:
            adv.adv_10000base_t_full = 1;
            break;
            
        default:
            break;
        }
    }         
    ret = aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(aal_port), adv);
#endif
    ret = aal_eth_phy_duplex_get(device_id, __ETH_PHY_ADDR(aal_port), &is_full);
    if (CA_E_OK != ret)
        goto __api_end;

    if (is_full != (ca_boolean_t)duplex) { /* change config */
        ret = aal_eth_phy_duplex_set(device_id,
                   __ETH_PHY_ADDR(aal_port), CA_PORT_DUPLEX_HALF != duplex);
    }

#if 0 /* the autoneg is decided by phy speed set, not duplex set */
    ret = aal_eth_phy_auto_neg_set(device_id,
           __ETH_PHY_ADDR(aal_port), duplex == CA_PORT_DUPLEX_AUTO);
#endif

__api_end:
    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
ca_eth_port_phy_duplex_set
Description
    These API routines can be used to  get the link duplex mode for
    a port (PHY layer). When retrieving the duplex mode for a port,
    current negotiated duplex status is returned if auto-negotiation is enabled.
    If auto-negotiation is in progress or link is not established properly,
    duplex is returned as value 0(half-duplex). If auto-negotiation is disabled,
    the configured duplex of the port is returned regardless of the link state.
Parameters
  [IN] device_id: CA device number
  [IN] port_id: Ethernet Port ID
  [OUT] duplex: Duplex mode
Returns
  CA_E_OK: Operation completed successfully
  CA_E_XXX: Other errors
NOTE
  These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_duplex_get(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,
                   CA_OUT ca_eth_port_duplex_t * duplex)
{
    ca_status_t ret = CA_E_OK;
    aal_phy_link_status_t phy_lnk;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x duplex = %p.\n",
        device_id, port_id, duplex);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(duplex);

    memset(&phy_lnk, 0, sizeof(phy_lnk));
    __ETH_PORT_LOCK(device_id);
    ret = __eth_link_status_get(device_id, aal_port, &phy_lnk);
    __ETH_PORT_UNLOCK(device_id);

    if (CA_E_OK != ret) {
        *duplex = CA_PORT_DUPLEX_HALF;
    } else {/* link down then return HALF always */
        *duplex = phy_lnk.link_up ? phy_lnk.duplex : CA_PORT_DUPLEX_HALF;
    }

    return ret;

}

/**__ge_pause_quanta_set
 *@brief: set PAUSE Quanta time for GE/FE ports
 */
static ca_status_t __ge_pause_quanta_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t       aal_port,
    CA_IN ca_uint16_t       pause_quanta)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  cos = 0;

    for (cos = 0; cos < 8; ++cos){
        ret = aal_ni_eth_pfc_quanta_set(device_id, aal_port, cos, pause_quanta);
        if (CA_E_OK != ret) {
            __ETH_DBG("aal_ni_eth_pfc_quanta_set port %d cos %d quanta %d fail\n",
                aal_port, cos, pause_quanta);
            return ret;
        }
    }

    return CA_E_OK;
}

/**_xge_pause_quanta_set
 *@brief: set PAUSE Quanta time for XE/XAUI ports
 */
static ca_status_t __xge_pause_quanta_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t       aal_port,
    CA_IN ca_uint16_t       pause_quanta)
{
    ca_status_t                          ret = CA_E_OK;


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (AAL_NI_XAUI == aal_port) {/* XAUI */
        aal_ni_xaui_mac_ctrl_flowctrl_mask_t xaui_mask;
        aal_ni_xaui_mac_ctrl_flowctrl_t      xaui_cfg;

        xaui_mask.u32 = 0;
        xaui_mask.s.qbb_pause_quanta0 = 1;
        xaui_mask.s.qbb_pause_quanta1 = 1;
        xaui_mask.s.qbb_pause_quanta2 = 1;
        xaui_mask.s.qbb_pause_quanta3 = 1;
        xaui_mask.s.qbb_pause_quanta4 = 1;
        xaui_mask.s.qbb_pause_quanta5 = 1;
        xaui_mask.s.qbb_pause_quanta6 = 1;
        xaui_mask.s.qbb_pause_quanta7 = 1;
        xaui_cfg.qbb_pause_quanta0 = pause_quanta;
        xaui_cfg.qbb_pause_quanta1 = pause_quanta;
        xaui_cfg.qbb_pause_quanta2 = pause_quanta;
        xaui_cfg.qbb_pause_quanta3 = pause_quanta;
        xaui_cfg.qbb_pause_quanta4 = pause_quanta;
        xaui_cfg.qbb_pause_quanta5 = pause_quanta;
        xaui_cfg.qbb_pause_quanta6 = pause_quanta;
        xaui_cfg.qbb_pause_quanta7 = pause_quanta;

        ret = aal_ni_xaui_mac_ctrl_flowctrl_set(device_id,
            xaui_mask, &xaui_cfg);
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_port == AAL_NI_INTERNAL_XE) {/* internal XE port */
        __ETH_DBG("Internal XE port is reserved, cannot be set by API.\n");
        return CA_E_OK;
    }
#endif

    if (aal_port >= AAL_NI_XE_START && aal_port <= AAL_NI_XE_END) { /* XGE ports */
        ret = aal_ni_xge_pause_quanta_set(device_id,
            aal_port - AAL_NI_XE_START, pause_quanta);
    }

    return ret;
}


/******************************************************************************
API
ca_eth_port_pause_quanta_set
Description
    These API routines can be used to set PAUSE quanta of a given port.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] pause_quanta: Pause quanta for the port (each quanta is 512bit times)
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_pause_quanta_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint16_t       pause_quanta
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x pause_quanta = %u.\n",
        device_id, port_id, pause_quanta);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    __ETH_PORT_LOCK(device_id);
    if (__ETH_IS_XGE(aal_port)) {/* XGE */
        __xge_pause_quanta_set(device_id, aal_port, pause_quanta);
    }
    else { /* GE/FE */
        __ge_pause_quanta_set(device_id, aal_port, pause_quanta);
    }

    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
ca_eth_port_pause_quanta_get
Description
    These API routines can be used to get PAUSE quanta of a given port.
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet port ID
    [IN] pause_quanta: Pause quanta for the port (each quanta is 512bit times)
Returns
    CA_E_OK :Operation completed successfully
    CA_E_XXX :Other errors
******************************************************************************/
ca_status_t ca_eth_port_pause_quanta_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t     port_id,
    CA_OUT ca_uint16_t     *pause_quanta
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x pause_quanta = %p.\n",
        device_id, port_id, pause_quanta);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(pause_quanta);

    __ETH_PORT_LOCK(device_id);
    if (__ETH_IS_XGE(aal_port)) {/* XGE */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (AAL_NI_XAUI == aal_port) { /* XAUI */
            aal_ni_xaui_mac_ctrl_flowctrl_t  xaui_cfg;
            memset(&xaui_cfg, 0, sizeof(xaui_cfg));
            ret = aal_ni_xaui_mac_ctrl_flowctrl_get(device_id, &xaui_cfg);
            if (CA_E_OK == ret)
                *pause_quanta = xaui_cfg.qbb_pause_quanta0;
        }
#endif
        if (aal_port >= AAL_NI_XE_START && aal_port <= AAL_NI_XE_END) { /* XE ports */
            ret = aal_ni_xge_pause_quanta_get(device_id,
                aal_port - AAL_NI_XE_START, pause_quanta);
        }
    }
    else { /* GE/FE, get COS 0 only */
        ret = aal_ni_eth_pfc_quanta_get(device_id, aal_port, 0, pause_quanta);
    }

    __ETH_PORT_UNLOCK(device_id);

    __ETH_DBG("RETURN device_id = %u port_id = %#x pause_quanta = %d.\n",
        device_id, port_id, *pause_quanta);

    return ret;

}

/******************************************************************************
API
    ca_eth_port_phy_mdix_set
Description
    These API routines can be used to set the link MDI/MDIX/MDIX_auto
    mode for a port (PHY layer). 
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] mode: MDI/MDIX mode
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
NOTE
    These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_mdix_set (
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_eth_port_mdix_t   mode)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
    aal_phy_mdi_mode_t  old_mode = AAL_PHY_MDI_MDI;

    __ETH_DBG("device_id = %u port_id = %#x mode = %u.\n",
        device_id, port_id, mode);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_ASSERT_RET(mode <= CA_PORT_MDIX_AUTO, CA_E_PARAM);

    if (!__ETH_DETECTED_PHY(aal_port)) {/* NOT detect PHY (intergrated or external) */
        if (__ETH_IS_XGE(aal_port)) {
            return CA_E_OK;
        } else {
            __ETH_DBG("NOT found PHY connected to port %d\r\n", aal_port);
            return CA_E_NOT_FOUND;
        }
    }

    __ETH_PORT_LOCK(device_id);
    ret = aal_eth_phy_mdix_get(device_id, __ETH_PHY_ADDR(aal_port), &old_mode);

    if (CA_E_OK == ret && old_mode != (aal_phy_mdi_mode_t)mode) {
        ret = aal_eth_phy_mdix_set(device_id, __ETH_PHY_ADDR(aal_port), (aal_phy_mdi_mode_t)mode);
    }
    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/******************************************************************************
API
    ca_eth_port_phy_mdix_get
Description
    These API routines can be used to get the link MDI/MDIX/MDIX_auto
    mode for a port (PHY layer). 
Parameters
    [IN] device_id: CA device number
    [IN] port_id: Ethernet Port ID
    [IN] mode: MDI/MDIX mode
Returns
    CA_E_OK: Operation completed successfully
    CA_E_XXX: Other errors
NOTE
    These APIs are supported only for PHYs integrated into Cortina-Access ASICs
******************************************************************************/
ca_status_t ca_eth_port_phy_mdix_get (
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT ca_eth_port_mdix_t   *mode)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_DBG("device_id = %u port_id = %#x mode = %p.\n",
        device_id, port_id, mode);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(mode);

    if (!__ETH_DETECTED_PHY(aal_port)) {/* NOT detect PHY (intergrated or external) */
        if (__ETH_IS_XGE(aal_port)) {
            *mode = CA_PORT_MDIX_AUTO;
            return CA_E_OK;
        } else {
            __ETH_DBG("NOT found PHY connected to port %d\r\n", aal_port);
            return CA_E_NOT_FOUND;
        }
    }

    __ETH_PORT_LOCK(device_id);
    ret = aal_eth_phy_mdix_get(device_id,
        __ETH_PHY_ADDR(aal_port), (aal_phy_mdi_mode_t *)mode);
    __ETH_PORT_UNLOCK(device_id);

    return ret;

}

/**ca_eth_ports_restore
 *@brief: restore Ethernet ports configurations(like admin, link scan) from STARTUP CFG
 */
ca_status_t ca_eth_ports_restore(CA_IN ca_device_id_t device_id)
{
    ca_port_id_t port_id   = 0;
    ca_boolean_t enable    = 0;
    ca_port_id_t xge_start = AAL_NI_XE_START;
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t  eth_mac_mask;
    aal_ni_eth_mac_config_t        eth_mac_config;
    aal_ni_xge_cmn_cfg_mask_t      xge_mask;
    aal_ni_xge_cmn_cfg_t           xge_config;
    ca_uint8_t uni_connect[CFG_ID_UNI_CONNECTION_LEN] = {0};

    __ETH_INIT_PRINT("start restore Ethernet ports configuration from SCFG\n");

    ret = scfg_read( device_id, CFG_ID_UNI_CONNECTION, CFG_ID_UNI_CONNECTION_LEN, uni_connect);
    if (CA_E_OK != ret) {
        ca_printf("ca-ne: Load %s fail, ret = %d, so set UNI default to be enable\n", CFG_ID_UNI_CONNECTION, ret);
        memset(uni_connect, 0x01, CFG_ID_UNI_CONNECTION_LEN);
    }

    xge_mask.u32 = 0;
    xge_mask.s.tx_rst = 1;
    xge_mask.s.rx_rst = 1;

#if  defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* Enable internal XGE ports */
    xge_config.tx_rst = 0;
    xge_config.rx_rst = 0;
    aal_ni_xge_cmn_config_set(CA_DEV_WAN, AAL_NI_PORT_XGE1, xge_mask, &xge_config);
    aal_ni_xge_cmn_config_set(CA_DEV_LAN, AAL_NI_PORT_XGE2, xge_mask, &xge_config);
#endif

    /* Only set rx, when tx is always enable to prevent buffer hang */
    eth_mac_mask.u32 = 0;
    eth_mac_mask.s.mac_rx_rst = 1;
    eth_mac_mask.s.mac_tx_rst = 1;
    eth_mac_mask.s.rx_en = 1;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    __FOR_ALL_ETH_PORTS_DO(port_id) {
        enable = uni_connect[port_id];

        if (__ETH_IS_XGE(port_id)) {/* XGE */

#if defined(CONFIG_NE_AAL_XFI)
            if (__ETH_IS_XFI(port_id)) {
                aal_xfi_cfg_mask_t   cfg_mask;
                aal_xfi_cfg_t        cfg;

                cfg_mask.data32 = 0;
                memset(&cfg, 0, sizeof(cfg));

                cfg_mask.s.pdown = 1;
                cfg_mask.s.rx_en = 1;
                cfg.pdown = !enable;
                cfg.rx_en = enable ? 1 : 0;
                ret = aal_xfi_cfg_set(device_id,
                    aal_xfi_port2xfi(port_id),
                    cfg_mask, &cfg);
            }
#endif

#if  defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (AAL_NI_XAUI == port_id && ((1 << port_id) & g_eth_port_drv.eth_port_bmp)) {

                aal_ni_xaui_mac_tx_cfg_mask_t  xaui_mac_tx_mask;
                aal_ni_xaui_mac_tx_cfg_t       xaui_mac_tx;
                aal_ni_xaui_mac_rx_cfg_mask_t  xaui_mac_rx_mask;
                aal_ni_xaui_mac_rx_cfg_t       xaui_mac_rx;
                memset(&xaui_mac_tx, 0, sizeof(xaui_mac_tx));
                memset(&xaui_mac_rx, 0, sizeof(xaui_mac_rx));
                xaui_mac_tx_mask.u32 = 0;
                xaui_mac_rx_mask.u32 = 0;
                xaui_mac_tx_mask.s.mac_en = 1;
                xaui_mac_rx_mask.s.mac_en = 1;
                xaui_mac_tx.mac_en = enable;
                xaui_mac_rx.mac_en = enable;

                ret = aal_ni_xaui_mac_tx_config_set(device_id, xaui_mac_tx_mask, &xaui_mac_tx);
                ret |= aal_ni_xaui_mac_rx_config_set(device_id, xaui_mac_rx_mask, &xaui_mac_rx);

            } else {
                xge_config.tx_rst  = !enable;
                xge_config.rx_rst  = !enable;
                ret = aal_ni_xge_cmn_config_set(device_id, port_id - xge_start, xge_mask, &xge_config);
            }
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
            xge_config.tx_rst  = !enable;
            xge_config.rx_rst  = !enable;
            ret = aal_ni_xge_cmn_config_set(device_id, port_id - xge_start, xge_mask, &xge_config);
#endif
        } else {
            /* Leave GPHY power on always */
            eth_mac_config.mac_rx_rst = !enable;
            eth_mac_config.mac_tx_rst = !enable;
            eth_mac_config.rx_en = enable;

            ret = aal_ni_eth_port_mac_set(device_id, port_id,
                               eth_mac_mask, &eth_mac_config);
        }

        if (ret != CA_E_OK) {
            __ETH_INIT_PRINT("%s: restore fail(retrun code = %d)\n", __func__, ret);
        } else {
            __ETH_INIT_PRINT("%s: %s port %d OK\n", __func__, "Enable", port_id );
        }

        g_eth_port_drv.port_enable[port_id] = enable;
    }

    (void)__eth_port_link_scan(device_id); /* sync the hardware link state to software in the initialization */
    (void)__eth_port_scan_start(device_id); /* start the port scan thread */

    return CA_E_OK;

}

/**__xge_port_enable
 *@brief: enable/disable XE port
 */
static ca_status_t __xge_port_enable(ca_device_id_t device_id, ca_port_id_t port_id, ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_xge_cmn_cfg_mask_t      xge_mask;
    aal_ni_xge_cmn_cfg_t           xge_config;

    __ETH_DBG("set XE port %d mac enable to %d \r\n", port_id, enable);

    xge_mask.u32 = 0;
    xge_mask.s.tx_rst = 1;
    xge_mask.s.rx_rst = 1;

    xge_config.tx_rst  = !enable;
    xge_config.rx_rst  = !enable;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ret = aal_ni_xge_cmn_config_set(device_id,
        port_id - AAL_NI_PORT_XGE0, xge_mask, &xge_config);
        
#else
    if (AAL_NI_XAUI == port_id  && ((1 << port_id) & g_eth_port_drv.eth_port_bmp)) {
        aal_ni_xaui_mac_tx_cfg_mask_t  xaui_mac_tx_mask;
        aal_ni_xaui_mac_tx_cfg_t       xaui_mac_tx;
        aal_ni_xaui_mac_rx_cfg_mask_t  xaui_mac_rx_mask;
        aal_ni_xaui_mac_rx_cfg_t       xaui_mac_rx;
        xaui_mac_tx_mask.u32 = 0;
        xaui_mac_rx_mask.u32 = 0;
        memset(&xaui_mac_tx, 0, sizeof(xaui_mac_tx));
        memset(&xaui_mac_rx, 0, sizeof(xaui_mac_rx));
        xaui_mac_tx_mask.s.mac_en  = 1;
        xaui_mac_rx_mask.s.mac_en  = 1;
        xaui_mac_tx.mac_en = enable;
        xaui_mac_rx.mac_en = enable;

        ret = aal_ni_xaui_mac_tx_config_set(device_id, xaui_mac_tx_mask, &xaui_mac_tx);
        ret |= aal_ni_xaui_mac_rx_config_set(device_id, xaui_mac_rx_mask, &xaui_mac_rx);

    } else {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (port_id == AAL_NI_PORT_XGE1) /* dont touch internal XGE for HGU */
#endif
        ret = aal_ni_xge_cmn_config_set(device_id, port_id - AAL_NI_PORT_XGE1, xge_mask, &xge_config);
    }
#endif

    return ret;
}

/**__ge_port_enable
 *@brief: enable/disable GE/FE port
 */
static ca_status_t __ge_port_enable(ca_device_id_t device_id, ca_port_id_t port_id, ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;

    __ETH_DBG("set port %d mac enable to %d \r\n", port_id, enable);

    /* Only set rx, when tx is always enable to prevent buffer overflow */
    eth_mac_mask.u32 = 0;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    eth_mac_mask.s.mac_rx_rst = 1;
    eth_mac_mask.s.rx_en      = 1;

    eth_mac_config.mac_rx_rst = !enable;
    eth_mac_config.rx_en      = enable;

    ret = aal_ni_eth_port_mac_set(device_id, port_id, eth_mac_mask, &eth_mac_config);

    return ret;
}

/**__eth_ports_enable
 *@brief: enable/disable ETHERNET ports
 */
static ca_status_t __eth_ports_enable(ca_device_id_t device_id, ca_boolean_t enable)
{
    ca_port_id_t port_id = 0;
    ca_status_t  ret = CA_E_OK;

    __FOR_ALL_ETH_PORTS_DO(port_id) {

        __ETH_DBG("port_id=%d enable = %d \n", port_id, enable);

        if (__ETH_IS_XGE(port_id)) {/* XGE */
            ret =__xge_port_enable(device_id, port_id, enable);
        } else {
            ret =__ge_port_enable(device_id, port_id, enable);
        }

        if (CA_E_OK != ret && CA_E_PARAM != ret) {
            __ETH_INIT_PRINT("%s: (return code=%d)\n", __func__, ret);
            return CA_E_ERROR;
        }
    }

    return CA_E_OK;

}

/**ca_eth_port_max_frame_size_get
 *@brief: get MRU of ETHERNET ports
 */
ca_status_t ca_eth_port_max_frame_size_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * size)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_pkt_len_config_t ge_cfg;
    aal_ni_xge_mac_frame_len_config_t xge_cfg;

    CA_DEV_ID_CHECK(device_id);
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(size);

    aal_port = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_cfg, 0, sizeof(aal_ni_pkt_len_config_t));
        ret = aal_ni_pkt_len_get(device_id, &ge_cfg);
        
        if(ret == CA_E_OK){
            *size = ge_cfg.max_pkt_size;
        }
        return ret;
        
    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;

        if(aal_port == AAL_NI_XAUI){    /* XAUI */
            memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));
            ret = aal_ni_xaui_mac_rx_config_get(device_id, &xaui_rx_cfg);
            if(ret == CA_E_OK){
                *size = xaui_rx_cfg.max_size;
            }
            return ret;
        }
#endif
        xge_idx = aal_port - AAL_NI_XE_START;
        memset(&xge_cfg, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));
        ret = aal_ni_xge_mac_frame_len_get(device_id, xge_idx, &xge_cfg);
        if(ret == CA_E_OK){
            *size = xge_cfg.max_len;
        }
        return ret;
    }

    return ret;
}

/**ca_eth_port_max_frame_size_set
 *@brief: set MRU of ETHERNET ports
 */
ca_status_t ca_eth_port_max_frame_size_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t size)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_pkt_len_config_mask_t ge_msk;
    aal_ni_pkt_len_config_t ge_cfg;
    aal_ni_xge_mac_frame_len_config_mask_t xge_msk;
    aal_ni_xge_mac_frame_len_config_t xge_cfg;

    CA_DEV_ID_CHECK(device_id);
    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    aal_port = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){
        memset(&ge_msk, 0, sizeof(aal_ni_pkt_len_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_pkt_len_config_t));

        ge_msk.s.max_pkt_size = 1;
        ge_cfg.max_pkt_size = size;
        //ca_printf("aal_ni_pkt_len_set size %d \r\n", size);
        ret = aal_ni_pkt_len_set(device_id, ge_msk, &ge_cfg);
        
    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */
    
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_ni_xaui_mac_rx_cfg_mask_t xaui_rx_msk;
        aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;

        if(aal_port == AAL_NI_XAUI){    /* XAUI */
            memset(&xaui_rx_msk, 0, sizeof(aal_ni_xaui_mac_rx_cfg_mask_t));
            memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));

            xaui_rx_msk.s.max_size = 1;
            xaui_rx_cfg.max_size = size;
            //ca_printf("aal_ni_xaui_mac_rx_config_set size %d \r\n", size);
            ret = aal_ni_xaui_mac_rx_config_set(device_id, xaui_rx_msk, &xaui_rx_cfg);
        }
#endif
        if (aal_port >= AAL_NI_XE_START) {
            xge_idx = aal_port - AAL_NI_XE_START;
            memset(&xge_msk, 0, sizeof(aal_ni_xge_mac_frame_len_config_mask_t));
            memset(&xge_cfg, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));

            xge_msk.s.max_len = 1;
            xge_cfg.max_len = size;
            //ca_printf("aal_ni_xge_mac_frame_len_set xge_idx %d size %d \r\n", xge_idx, size);
            ret = aal_ni_xge_mac_frame_len_set(device_id, xge_idx, xge_msk, &xge_cfg);
        }
    }

    __ETH_PORT_UNLOCK(device_id);

    return ret;
}


/**ca_eth_port_enable_get:
 *@breif: local API for get etherent port enable configuration
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t  ca_eth_port_ipg_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_OUT ca_uint32_t   *ipg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_mac_config_t ge_cfg;
    aal_ni_xge_cfg_tx_t xge_tx_cfg;

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(ipg);

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));
        ret = aal_ni_eth_port_mac_get(device_id, aal_port, &ge_cfg);
        if (ret == CA_E_OK) {
            *ipg = ge_cfg.tx_ipg_sel + 8;
        }
    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */
        xge_idx = aal_port - AAL_NI_XE_START;
        memset(&xge_tx_cfg, 0, sizeof(aal_ni_xge_cfg_tx_t));
        ret = aal_ni_xge_tx_config_get(device_id, xge_idx, &xge_tx_cfg);
        if (ret == CA_E_OK) {
            *ipg = xge_tx_cfg.ifg;
        }
    }

    return ret;
}

/**ca_eth_port_ipg_set:
 *@breif: local API for set etherent port enable configuration
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t  ca_eth_port_ipg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_OUT ca_uint32_t    ipg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_mac_config_mask_t ge_msk;
    aal_ni_eth_mac_config_t ge_cfg;
    aal_ni_xge_cfg_tx_mask_t xge_tx_msk;
    aal_ni_xge_cfg_tx_t xge_tx_cfg;

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);

    __ETH_PORT_LOCK(device_id);

    if(CA_PORT_IS_GE(port_id)){ /* GE */

        if (ipg < CA_PORT_IPG_GE_VAL_MIN || ipg > CA_PORT_IPG_GE_VAL_MAX) {
            __ETH_DBG("the IPG %u is invalid\n", ipg);
            return CA_E_PARAM;
        }
        memset(&ge_msk, 0, sizeof(aal_ni_eth_mac_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));

        ge_msk.s.tx_ipg_sel = 1;
        ge_cfg.tx_ipg_sel = ipg - 8;

        ret = aal_ni_eth_port_mac_set(device_id, aal_port, ge_msk, &ge_cfg);

    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */

        if (ipg < CA_PORT_IPG_XGE_VAL_MIN || ipg > CA_PORT_IPG_XGE_VAL_MAX) {
            __ETH_DBG("the IPG %u is invalid\n", ipg);
            return CA_E_PARAM;
        }

        xge_idx = aal_port - AAL_NI_XE_START;
        memset(&xge_tx_msk, 0, sizeof(aal_ni_xge_cfg_tx_mask_t));
        memset(&xge_tx_cfg, 0, sizeof(aal_ni_xge_cfg_tx_t));

        xge_tx_msk.s.ifg  = 1;
        xge_tx_cfg.ifg = ipg;
        ret = aal_ni_xge_tx_config_set(device_id, xge_idx, xge_tx_msk, &xge_tx_cfg);
    }
    __ETH_PORT_UNLOCK(device_id);

    return ret;
}


/**ca_eth_port_enable_get:
 *@breif: local API for get etherent port enable configuration
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t  ca_eth_port_enable_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_OUT ca_boolean_t * enable)

{
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_NULL_PTR_CHECK(enable);

    if (aal_port >= __MAX_ETH_PORT_NUM) {
        __ETH_DBG("the port %#x is invalid\n", port_id);
        return CA_E_PARAM;
    }

    __ETH_PORT_LOCK(device_id);
    *enable = g_eth_port_drv.port_enable[aal_port];
    __ETH_PORT_UNLOCK(device_id);

    return CA_E_OK;


}

/**ca_eth_port_enable_set:
 *@breif: local API for set etherent port enable configuration
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t  ca_eth_port_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_boolean_t    enable)
{
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;

    __ETH_INIT_CHK(device_id);
    __ETH_PORT_CHK(port_id);
    CA_BOOL_VAL_CHECK(enable);

    if (aal_port >= __MAX_ETH_PORT_NUM) {
        __ETH_DBG("the port %#x is invalid\n", port_id);
        return CA_E_PARAM;
    }

    __ETH_PORT_LOCK(device_id);

    if (__ETH_DETECTED_PHY(aal_port)) { /* external PHY */
        __ETH_DBG("set ext phy %d enable to %d \r\n", aal_port, enable);
        ret = aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(aal_port), enable);
#if defined(CONFIG_NE_AAL_XFI)
    } else if (__ETH_IS_XFI(aal_port)) { /* XFI serdes */
        aal_xfi_cfg_mask_t msk;
        aal_xfi_cfg_t cfg;
        ca_uint32_t xfi_idx = 0;

        xfi_idx = aal_xfi_port2xfi(aal_port);
        memset(&cfg, 0, sizeof(cfg));
        msk.data32 = 0;
        msk.s.pdown = 1;
        msk.s.rx_en = 1;
        cfg.pdown = !enable;
        cfg.rx_en = enable ? 1 : 0;
        __ETH_DBG("set xfi %d enable to %d \r\n", xfi_idx, enable);
        ret = aal_xfi_cfg_set(device_id, xfi_idx, msk, &cfg);
#endif
    } else { /* MAC */
        if (__ETH_IS_GE(aal_port)) {
            ret = __ge_port_enable(device_id, aal_port, enable);
        } else if (__ETH_IS_XGE(aal_port)) {
            ret = __xge_port_enable(device_id, aal_port, enable);
        }
    }

    if (CA_E_OK == ret) {
        g_eth_port_drv.port_enable[aal_port] = enable;
    }
    __ETH_PORT_UNLOCK(device_id);

    return ret;
}

/**__eth_phy_auto_detect:
 *@breif: poll all the MDIO address to detect PHY automaticlly
 */
static inline void __eth_phy_auto_detect(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t addr = 0, phy_count = 0, phy_id = 0;

    __ETH_INIT_PRINT("auto detect Ethernet PHY start ...\n");


    __FOR_ALL_MDIO_DO(addr) {
        ret = aal_eth_phy_init(device_id, addr);
        if (CA_E_OK == ret) {
            phy_count++;
            for (phy_id = 0; phy_id < __MAX_ETH_PORT_NUM; phy_id ++) {
                if (g_eth_port_drv.eth_mdio_addr[phy_id] == addr) {
                    g_eth_port_drv.ext_phy_detected[phy_id] = TRUE;
                    __ETH_INIT_PRINT("attch PHY addr %d to port %d driver\n", addr, phy_id);
                }
            }
        }
    }

    __ETH_INIT_PRINT("detect Ethernet PHY done, find %d PHYs\n", phy_count);

}

/**__eth_port_phy_init:
 *@breif: initialize PHY/SerDes to enable PHY, enable autoneg
 */
static ca_status_t __eth_port_phy_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port_id = 0;
    aal_phy_autoneg_adv_t  adv;
    ca_boolean_t    eth_port3_active_flag = TRUE;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
    ca_boolean_t   sfpplus_evb_flag = FALSE;

     sfpplus_evb_flag = __eth_port3_active_evb_check( );
     if (sfpplus_evb_flag == FALSE) {
        eth_port3_active_flag = FALSE;
     }
#endif

    if ( eth_port3_active_flag  == TRUE){
        ca_uint32_t  board_info = 0;
        ca_uint32_t  custer_bit = 0;
        ret = scfg_read(device_id, CFG_ID_CUSTOMER_BIT, sizeof(ca_uint32), (ca_uint8_t *)&custer_bit);
        ret |= scfg_read(device_id, CFG_ID_BOARD_INFO_BIT, sizeof(ca_uint32), (ca_uint8_t *)&board_info);
        if ((CA_E_OK == ret) && (custer_bit&CUSTOMER_BIT_13) &&
            (board_info == BOARD_INFO_BIT_3 || board_info == BOARD_INFO_BIT_1)  &&  
            __ETH_INTF_IS_XFI(AAL_NI_PORT_XGE0)) {
            g_eth_port_drv.disable_phy2mac_sync[AAL_NI_PORT_XGE0] = TRUE; /* for Ubiqous board with AQR107 PHY, neednt sync */
            ca_printf("ca-ne: detect 10G PHY interface with XFI rate matching mode\r\n");
        }

    AAL_FOR_ALL_GE_DO(port_id) {
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
        if (__ETH_IS_CONNECT_SWITCH(port_id)) { /* Connect to RTL switch chipset */
            continue;
        }
#endif
        aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(port_id), &adv);

            adv.adv_pause = 0;
            adv.adv_asym_pause = 0;
        adv.adv_100base_tx_half = 1;
        adv.adv_100base_tx_full = 1;
        adv.adv_10base_t_half   = 1;
        adv.adv_10base_t_full   = 1;

#ifdef CONFIG_CORTINA_BOARD_FPGA
        adv.adv_1000base_t_half = 0;
        adv.adv_1000base_t_full = 0;
#else
        adv.adv_1000base_t_half = 1;
        adv.adv_1000base_t_full = 1;
#endif
            //aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(port_id), adv);
        aal_eth_phy_duplex_set(device_id, __ETH_PHY_ADDR(port_id), TRUE); /* FULL */

#ifdef CONFIG_CORTINA_BOARD_FPGA
            aal_eth_phy_speed_set(device_id, __ETH_PHY_ADDR(port_id), AAL_PHY_SPEED_100);
        aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), FALSE);
            g_eth_port_drv.autoneg[port_id] = FALSE;
#else
            aal_eth_phy_speed_set(device_id, __ETH_PHY_ADDR(port_id), _MAC_SPEED2PHY_SPEED(__default_mac_speed[port_id]));
        aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), TRUE);
        g_eth_port_drv.autoneg[port_id] = TRUE;
#endif

            g_eth_port_drv.adv[port_id].half_10  = 1;
            g_eth_port_drv.adv[port_id].full_10  = 1;
            g_eth_port_drv.adv[port_id].half_100 = 1;
            g_eth_port_drv.adv[port_id].full_100 = 1;
            g_eth_port_drv.adv[port_id].full_1g  = 1;
            g_eth_port_drv.adv[port_id].full_10g = 0;
            g_eth_port_drv.adv[port_id].flow_ctrl = 0;
            g_eth_port_drv.adv[port_id].asym_flow_ctrl  = 0;
            g_eth_port_drv.adv[port_id].eee_100_capable = 0;
            g_eth_port_drv.adv[port_id].eee_1g_capable  = 0;
            g_eth_port_drv.adv[port_id].eee_10g_capable = 0;
            //ca_printf("%s(): init ge %d done\r\n", __func__, port_id);
    }
    
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
    AAL_FOR_ALL_XE_DO(port_id) {
            memset(&g_eth_port_drv.adv[port_id], 0, sizeof(ca_eth_port_ability_t));
            if (__ETH_DETECTED_PHY(port_id)) {
                aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(port_id), &adv);

                adv.adv_pause = 0;
                adv.adv_asym_pause = 0;
                adv.adv_100base_tx_half = 1;
                adv.adv_100base_tx_full = 1;
                adv.adv_10base_t_half   = 1;
                adv.adv_10base_t_full   = 1;
                adv.adv_1000base_t_half = 1;
                adv.adv_1000base_t_full = 1;
                adv.adv_2500base_t_full = 1;
                adv.adv_5000base_t_full = 1;
                adv.adv_10000base_t_full = 1;
                adv.adv_10gbase_t_eee   = 0;
                adv.adv_1000base_t_eee  = 0;
                adv.adv_100base_tx_eee  = 0;
                adv.adv_5gbase_t_eee    = 0;
                adv.adv_2d5gbase_t_eee  = 0;
                aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(port_id), adv);
            aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), TRUE);
			g_eth_port_drv.autoneg[port_id] = TRUE;

		} else {
            g_eth_port_drv.autoneg[port_id] = FALSE;
        }
            g_eth_port_drv.adv[port_id].full_10g  = 1;
            g_eth_port_drv.adv[port_id].full_5g   = 1;
            g_eth_port_drv.adv[port_id].full_2d5g = 1;
            g_eth_port_drv.adv[port_id].full_1g   = 1;
            g_eth_port_drv.adv[port_id].full_1g   = 1;
            g_eth_port_drv.adv[port_id].full_100  = 1;
            g_eth_port_drv.adv[port_id].half_100  = 1;
            g_eth_port_drv.adv[port_id].half_10  = 1;
            g_eth_port_drv.adv[port_id].full_10  = 1;
            g_eth_port_drv.adv[port_id].eee_100_capable = 0;
            g_eth_port_drv.adv[port_id].eee_1g_capable  = 0;
            g_eth_port_drv.adv[port_id].eee_10g_capable = 0;
    }
#endif

#ifndef CONFIG_CORTINA_BOARD_FPGA /* ASIC internal GPHY init */
    aal_internal_phy_init(device_id);

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_SWITCH_INIT_LINKDOWN)
    aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(port_id), 0);
#endif

#endif
   }

#if defined(CONFIG_NE_AAL_XFI)
    aal_xfi_init(device_id);
    AAL_FOR_ALL_XE_DO(port_id) {
        if (__ETH_DETECTED_PHY(port_id) && __ETH_IS_XFI(port_id)) { /* reset the XFI SERDES interfaces between PHY and MAC */
            aal_eth_phy_reset(device_id, __ETH_PHY_ADDR(port_id));
            aal_xfi_reset(device_id, aal_xfi_port2xfi(port_id));
        }
    }
#endif

    return ret;
}


/**__eth_port_mac_init:
 *@breif: initialize ethernet MAC
 */
static ca_status_t __eth_port_mac_init(ca_device_id_t device_id)
{
    aal_ni_xge_mac_frame_len_config_mask_t xe_len_mask;
    aal_ni_xge_mac_frame_len_config_t xe_len_config;
    aal_ni_pkt_len_config_mask_t  ge_len_mask;
    aal_ni_pkt_len_config_t       ge_len_config;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
    aal_ni_eth_if_config_mask_t eth_if_mask;
    aal_ni_eth_if_config_t eth_if_cfg;
    ca_uint32_t port_id = 0;
    ca_status_t ret = CA_E_OK;

    eth_mac_mask.u32 = 0;
    eth_mac_mask.s.duplex = 1;
    eth_mac_mask.s.speed = 1;
    eth_mac_mask.s.mac_crc_calc_en = 1;
    eth_mac_mask.s.tx_en = 1;
    eth_mac_mask.s.rx_pfc_disable = 1;

    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    eth_mac_config.duplex = AAL_NI_MAC_DUPLEX_FULL; /* FULL Duplex */
    eth_mac_config.mac_crc_calc_en = 1; /* enable CRC caculation */
    eth_mac_config.tx_en = 1;   /* Enable */
    eth_mac_config.rx_pfc_disable = 1;  /* Disable PFC function */

    eth_if_mask.u32 = 0;
    eth_if_mask.s.int_cfg = 1;
    eth_if_mask.s.phy_mode = 1;

    memset(&eth_if_cfg, 0, sizeof(eth_if_cfg));


    AAL_FOR_ALL_GE_DO(port_id) {
        eth_if_cfg.int_cfg = g_eth_port_drv.eth_if_mode[port_id];
        eth_if_cfg.phy_mode = AAL_NI_PHY_MODE_MAC;  /* connect to PHY */
        eth_mac_config.speed = __default_mac_speed[port_id];
        g_eth_port_drv.speed[port_id] = (ca_uint8_t)_MAC2SPEED(__default_mac_speed[port_id]);

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        if (__ETH_IS_CONNECT_SWITCH(port_id)) {
            eth_if_cfg.phy_mode = AAL_NI_PHY_MODE_PHY;  /* Ethernet interface connect to MAC */
            ca_printf ("ca-ne: device is connected to switch chip\r\n" );
        }
#endif

        ret = aal_ni_eth_if_set(device_id, port_id,
                    eth_if_mask, &eth_if_cfg);

        if (CA_E_OK != ret) {
            ca_printf("port %u interface init fail %#x.", port_id, ret);
            return ret;
        }

        ret = aal_ni_eth_port_mac_set(device_id, port_id,
                          eth_mac_mask, &eth_mac_config);

        if (CA_E_OK != ret) {
            ca_printf("port %u MAC init fail %#x.", port_id, ret);
            return ret;
        }
    }

    /* Init GE MAX frame size */
    memset(&ge_len_mask, 0, sizeof(aal_ni_pkt_len_config_mask_t));
    memset(&ge_len_config, 0, sizeof(aal_ni_pkt_len_config_t));
    
    ge_len_mask.s.max_pkt_size = 1;
    ge_len_mask.s.max_store_pkt_len = 1;
    ge_len_config.max_pkt_size = CA_ETH_MAX_FRAME_SIZE_DEFAULT;
    ge_len_config.max_store_pkt_len = CA_ETH_MAX_FRAME_SIZE_MAX;
    (void)aal_ni_pkt_len_set(device_id, ge_len_mask, &ge_len_config);

    /* Init XGE MAX frame size */
    AAL_FOR_ALL_XE_DO(port_id) {
        memset(&xe_len_mask, 0, sizeof(aal_ni_xge_mac_frame_len_config_mask_t));
        memset(&xe_len_config, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));
        
        xe_len_mask.s.max_len = 1;
        xe_len_config.max_len = CA_ETH_MAX_FRAME_SIZE_DEFAULT;
        (void)aal_ni_xge_mac_frame_len_set(device_id, port_id - AAL_NI_XE_START, xe_len_mask, &xe_len_config);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        (void)__xge_pause_set(device_id, port_id, 0, 0, 0);
#endif
    }

#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
    AAL_FOR_ALL_XE_DO(port_id) {
        g_eth_port_drv.speed[port_id] = (ca_uint8_t)CA_PORT_SPEED_10G;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
    __eth_port_gphy_ctrl_init(device_id);
#endif

    return ret;

}

/*
 * ONLY used for ni driver exit invoking
 */
void ca_eth_port_exit(CA_IN ca_device_id_t device_id)
{
    g_eth_port_drv.eth_port_scan_stop = 1;

    if (g_eth_port_drv.eth_port_lock) {
        ca_spin_lock_destroy(g_eth_port_drv.eth_port_lock);
        g_eth_port_drv.eth_port_lock = 0;
    }

    aal_mdio_global_exit(device_id);
    g_eth_port_init_done = 0;
}


/*
 * ONLY used for ca_init invoking
 */
ca_status_t ca_eth_port_init(CA_IN ca_device_id_t device_id,
                 CA_IN ca_init_config_t * p_cfg)
{
    ca_status_t ret = CA_E_OK;

    if (g_eth_port_init_done) {
        __ETH_INIT_PRINT("Ethernet port driver has already initialized.\r\n");
        return CA_E_OK;
    }

    /* Global data init */
    __eth_port_global_init(device_id);

    /* Disable all ports' Rx */
    ret = __eth_ports_enable(device_id, FALSE);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("__eth_ports_enable fail, return code %#x.\n", ret);
    } else {
        __ETH_INIT_PRINT("disalbe all ethernet ports OK\n");
    }

    if (g_eth_port_drv.eth_port_bmp) {
        /* ETH PHY init */
        __eth_phy_auto_detect(device_id);
    }

    ret =__eth_port_phy_init(device_id);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("__eth_port_phy_init fail, return code %#x.\n", ret);
    }

    /* ETH MAC init */
    ret = __eth_port_mac_init(device_id);
    if (CA_E_OK != ret) {
        __ETH_INIT_PRINT("__eth_port_mac_init fail, return code %#x.\n", ret);
    }
    /* disable unused ports */
    aal_ni_reset_intf(device_id, g_eth_port_drv.eth_port_bmp);

    if (CA_E_OK != ret) {
        ca_eth_port_exit(device_id);
        ca_printf("!!!error: eth_port_init fail, return %#x!!!\r\n", ret);
    } else {
        g_eth_port_init_done = 1;
    }

    return ret;
}

/*
ena/    speed/ link auto    STP                  lrn  inter   max  p
      port  link    duplex scan neg?   state   pause  discrd ops   face frame  k

*/
void eth_port_dump(void)
{
    ca_status_t    ret = CA_E_OK;
    ca_device_id_t dev  = 0;
    ca_uint32_t    pp   = 0, max_frame = 0, ipg = 0;
    ca_port_id_t   port = 0;
    char name_str[CA_AAL_PHY_NAME_LEN_MAX] = {0};
    ca_boolean_t   enable = FALSE, autoneg = FALSE, pfc = FALSE, pause_rx = FALSE, pause_tx = FALSE, link_scan = FALSE;
    ca_eth_port_link_status_t  link_status = 0;
    ca_eth_port_speed_t        speed = 0;
    ca_eth_port_duplex_t       duplex = 0;
    ca_port_stp_state_t        stp_state = 0;

    if (!g_eth_port_init_done) {
        ca_printf("Ethernet Port driver was not installed!\r\n");
        return;
    }

    ca_printf("\r\n");
    ca_printf("Port  Enable  Link  Speed  Duplex  Autoneg   Pause  PFC  Interface   STP    Maxframe  IPG  Link-scan  Eexternal-Connect\r\n");
    ca_printf("-----------------------------------------------------------------------------------------------------------------------\r\n");

    __FOR_ALL_ETH_PORTS_DO(pp) {
        port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, pp);
        enable = 0;
        ret = ca_port_enable_get(dev, port, &enable);
        if (ret) {
            ca_printf("ca_port_enable_get fail port %#x, return %d.\n", port, ret);
            continue;
        }
        if (__ETH_IS_CONNECT_SWITCH(pp)) {
            link_status = 1;
            speed       = CA_PORT_SPEED_1G;
            duplex      = CA_PORT_DUPLEX_FULL;
            autoneg     = 1;
        } else {
            link_status = 0;
            ret = ca_eth_port_link_status_get(dev, port,  &link_status);
            if (ret) {
                ca_printf("ca_eth_port_link_status_get fail port %#x, return %d.\n", port, ret);
                continue;
            }

            speed = 0;
            ret = ca_eth_port_phy_speed_get(dev, port,  &speed);
            if (ret) {
                ca_printf("ca_eth_port_speed_get fail port %#x, return %d.\n", port, ret);
                continue;
            }

            duplex = 0;
            ret = ca_eth_port_phy_duplex_get(dev, port,  &duplex);
            if (ret) {
                ca_printf("ca_eth_port_duplex_get fail port %#x, return %d.\n", port, ret);
                continue;
            }

            autoneg = 0;
            ret = ca_eth_port_autoneg_get(dev, port,  &autoneg);
            //ret = ca_eth_port_phy_autoneg_get(dev, port,  &autoneg);
            if (ret) {
                ca_printf("ca_eth_port_autoneg_get fail port %#x, return %d.\n", port, ret);
                continue;
            }
        }
        pfc = pause_rx = pause_tx = 0;
        ret = ca_eth_port_pause_get(dev, port, &pfc, &pause_rx, &pause_tx);
        if (ret) {
            ca_printf("ca_eth_port_pause_get fail port %#x, return %d.\n", port, ret);
            continue;
        }

        stp_state = 0;
        ret = ca_port_stp_state_get(dev, port, &stp_state);
        if (ret) {
            ca_printf("ca_port_stp_state_get fail port %#x, return %d.\n", port, ret);
            continue;
        }

        max_frame = 0;
        ret = ca_eth_port_max_frame_size_get(dev, port, &max_frame);
        if (ret) {
            ca_printf("ca_port_max_frame_size_get fail port %#x, return %d.\n", port, ret);
            continue;
        }

        ipg = 0;
        ret = ca_eth_port_ipg_get(dev, port, &ipg);
        if (ret) {
            ca_printf("ca_eth_port_ipg_get fail port %#x, return %d.\n", port, ret);
            continue;
        }

        ret = ca_eth_port_link_scan_get(dev, port, &link_scan);
        if (ret) {
            ca_printf("ca_eth_port_link_scan_get fail port %#x, return %d.\n", port, ret);
            continue;
        }

        if (__ETH_DETECTED_PHY(pp)) {
            ret = aal_eth_phy_name_get(dev, __ETH_PHY_ADDR(pp), name_str);
        }

        ca_printf("%-4u  %-6s  %-4s  %-5s  %-5s   %-7s   %-2s %-2s  %-3s  %-9s  %-7s  %-6u   %-3u  %-7s    %s %s\r\n",
            pp, enable ? "Yes" : "No", link_status == CA_PORT_LINK_UP ? "up" : "down", speed_str[speed], duplex_str[duplex],
            autoneg ? "enable" : "disable", pause_rx ? "Rx" : "No", pause_tx ? "Tx" : "No",
            pfc ? "Yes" : "No",  __eth_if_str_get(g_eth_port_drv.eth_if_mode[pp]), stp_str[stp_state], max_frame, ipg,
            link_scan ? "enable" : "disable",
            __ETH_DETECTED_PHY(pp) ? name_str : (__ETH_IS_CONNECT_SWITCH(pp) ? "RTL8364 Switch" : "None"),
            __ETH_DETECTED_PHY(pp) ? "PHY" : " ");
    }
    ca_printf("\n");
}

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_ARCH_RTL8198F)
/* for 98F NE QoS */
unsigned int l3felan_tx_en_bak = 0;
unsigned int l3fewan_tx_en_bak = 0;

int rtk_backup_l3fe_flow_control(void)
{
	ca_status_t ret = CA_E_OK;
	aal_l3_te_flow_ctrl_t cfg;
	memset(&cfg, 0, sizeof(aal_l3_te_flow_ctrl_t));
		
	if((ret = aal_l3_te_flow_control_get(0, &cfg)) != CA_E_OK){
		ca_printf("aal_l3_te_flow_control_get fail, return %d.\n", ret);
		return ret;
	}

	l3felan_tx_en_bak = cfg.tx_lan_ena;
	l3fewan_tx_en_bak = cfg.tx_wan_ena;
	
	return ret;
}
	
int rtk_restore_l3fe_flow_control(void)
{
	ca_status_t ret = CA_E_OK;
	aal_l3_te_flow_ctrl_t cfg;
	aal_l3_te_flow_ctrl_mask_t	mask;
	
	memset(&cfg, 0, sizeof(aal_l3_te_flow_ctrl_t));
	memset(&mask, 0, sizeof(aal_l3_te_flow_ctrl_mask_t));

	if(l3felan_tx_en_bak){
		cfg.tx_lan_ena = l3felan_tx_en_bak;
		mask.s.tx_lan_ena = 1;
	}

	if(l3fewan_tx_en_bak){
		cfg.tx_wan_ena = l3fewan_tx_en_bak;
		mask.s.tx_wan_ena = 1;
	}

	if((ret = aal_l3_te_flow_control_set(0, mask, &cfg)) != CA_E_OK){
		ca_printf("aal_l3_te_flow_control_set fail, return %d.\n", ret);
		return ret;
	}

	l3felan_tx_en_bak = 0;
	l3fewan_tx_en_bak = 0;
	
	return ret;
}
	
int rtk_set_l3fe_flow_control(unsigned int lan_en, unsigned int wan_en)
{
	ca_status_t ret = CA_E_OK;
	aal_l3_te_flow_ctrl_t cfg;
	aal_l3_te_flow_ctrl_mask_t	mask;
	
	memset(&cfg, 0, sizeof(aal_l3_te_flow_ctrl_t));
	memset(&mask, 0, sizeof(aal_l3_te_flow_ctrl_mask_t));
	
	if((ret = rtk_backup_l3fe_flow_control()) != CA_E_OK){
		ca_printf("rtk_backup_l3fe_flow_control fail, return %d.\n", ret);
		return ret;
	}

	cfg.tx_lan_ena = lan_en;
	mask.s.tx_lan_ena = 1;

	cfg.tx_wan_ena = wan_en;
	mask.s.tx_wan_ena = 1;

	if((ret = aal_l3_te_flow_control_set(0, mask, &cfg)) != CA_E_OK){
		ca_printf("aal_l3_te_flow_control_set fail, return %d.\n", ret);
		return ret;
	}
	
	return ret;
}
#endif


#ifdef __KERNEL__

EXPORT_SYMBOL(cap_mdio_write);
EXPORT_SYMBOL(cap_mdio_read);
EXPORT_SYMBOL(cap_mdio_speed_set);
EXPORT_SYMBOL(cap_mdio_speed_get);
EXPORT_SYMBOL(ca_eth_port_link_status_get);
EXPORT_SYMBOL(ca_eth_port_link_scan_get);
EXPORT_SYMBOL(ca_eth_port_link_scan_set);
EXPORT_SYMBOL(ca_eth_port_autoneg_set);
EXPORT_SYMBOL(ca_eth_port_autoneg_get);
EXPORT_SYMBOL(ca_eth_port_pause_set);
EXPORT_SYMBOL(ca_eth_port_pause_get);
EXPORT_SYMBOL(ca_eth_port_speed_set);
EXPORT_SYMBOL(ca_eth_port_speed_get);
EXPORT_SYMBOL(ca_eth_port_phy_advert_remote_get);
EXPORT_SYMBOL(ca_eth_port_phy_autoneg_set);
EXPORT_SYMBOL(ca_eth_port_phy_autoneg_get);
EXPORT_SYMBOL(ca_eth_port_phy_speed_set);
EXPORT_SYMBOL(ca_eth_port_phy_speed_get);
EXPORT_SYMBOL(ca_eth_port_phy_advert_get);
EXPORT_SYMBOL(ca_eth_port_phy_advert_set);
EXPORT_SYMBOL(ca_eth_port_phy_duplex_set);
EXPORT_SYMBOL(ca_eth_port_phy_duplex_get);
EXPORT_SYMBOL(ca_eth_port_phy_mdix_set);
EXPORT_SYMBOL(ca_eth_port_phy_mdix_get);
EXPORT_SYMBOL(ca_eth_port_stats_get);
EXPORT_SYMBOL(ca_eth_port_duplex_set);
EXPORT_SYMBOL(ca_eth_port_duplex_get);
EXPORT_SYMBOL(ca_eth_port_pause_quanta_set);
EXPORT_SYMBOL(ca_eth_port_pause_quanta_get);
EXPORT_SYMBOL(ca_eth_port_eee_enable_set);
EXPORT_SYMBOL(ca_eth_port_eee_enable_get);
EXPORT_SYMBOL(ca_eth_port_enable_set);
EXPORT_SYMBOL(ca_eth_port_enable_get);
EXPORT_SYMBOL(ca_eth_port_ipg_set);
EXPORT_SYMBOL(ca_eth_port_ipg_get);
EXPORT_SYMBOL(ca_eth_port_max_frame_size_set);
EXPORT_SYMBOL(ca_eth_port_max_frame_size_get);
EXPORT_SYMBOL(ca_eth_port_exit);
EXPORT_SYMBOL(eth_port_dump);
EXPORT_SYMBOL(board_use2port_connect_ext_switch);

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_ARCH_RTL8198F)
EXPORT_SYMBOL(l3felan_tx_en_bak);
EXPORT_SYMBOL(l3fewan_tx_en_bak);
EXPORT_SYMBOL(rtk_backup_l3fe_flow_control);
EXPORT_SYMBOL(rtk_restore_l3fe_flow_control);
EXPORT_SYMBOL(rtk_set_l3fe_flow_control);
#endif
#endif

