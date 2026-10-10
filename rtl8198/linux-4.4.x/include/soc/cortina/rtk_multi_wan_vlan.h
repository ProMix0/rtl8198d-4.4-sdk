/*
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
 */

#ifndef __RTK_MULTI_WAN_VLAN_H__
#define __RTK_MULTI_WAN_VLAN_H__

#define CA_MAX_ETHERNET_PORT_NUMBER		8

#define RTK_MAX_DBG_MSG_LEN     512


/* may exchange portmask for wan port 
 * in web ui lan1/lan2/lan3/lan4 means device port mask
 * lan1-bit0, lan2-bit1, lan3-bit2, lan4-bit3
 * when set portmapping in web ui, 
 * it should change device port mask to real physical portmask in function rtl_update_multi_wan_port_mapping
* currently only consider ethernet lans
*/
#define RTL_DEV_NAME_NUM(name,num)	name#num

#define RTL_DRV_LAN_NAME_PREFIX "eth0."
#define RTL_DRV_LAN1_NAME RTL_DEV_NAME_NUM(RTL_DRV_LAN_NAME_PREFIX, 2)
#define RTL_DRV_LAN2_NAME RTL_DEV_NAME_NUM(RTL_DRV_LAN_NAME_PREFIX, 3)
#define RTL_DRV_LAN3_NAME RTL_DEV_NAME_NUM(RTL_DRV_LAN_NAME_PREFIX, 4)
#define RTL_DRV_LAN4_NAME RTL_DEV_NAME_NUM(RTL_DRV_LAN_NAME_PREFIX, 5)
#define RTL_DRV_LAN5_NAME RTL_DEV_NAME_NUM(RTL_DRV_LAN_NAME_PREFIX, 6)

#define RTL_ETH_LAN1_BIND_MASK		(1<<0)
#define RTL_ETH_LAN2_BIND_MASK		(1<<1)
#define RTL_ETH_LAN3_BIND_MASK		(1<<2)
#define RTL_ETH_LAN4_BIND_MASK		(1<<3)
#define RTL_ETH_LAN5_BIND_MASK		(1<<4)

#define RTL_DRV_WLAN0_NAME			"wlan0"
#define RTL_DRV_WLAN0_VAP0_NAME		"wlan0-vap0"
#define RTL_DRV_WLAN0_VAP1_NAME		"wlan0-vap1"
#define RTL_DRV_WLAN0_VAP2_NAME		"wlan0-vap2"
#define RTL_DRV_WLAN0_VAP3_NAME		"wlan0-vap3"
#define RTL_DRV_WLAN1_NAME			"wlan1"
#define RTL_DRV_WLAN1_VAP0_NAME		"wlan1-vap0"
#define RTL_DRV_WLAN1_VAP1_NAME		"wlan1-vap1"
#define RTL_DRV_WLAN1_VAP2_NAME		"wlan1-vap2"
#define RTL_DRV_WLAN1_VAP3_NAME		"wlan1-vap3"

#ifdef CONFIG_RTL_VAP_SUPPORT
#define RTL_WLAN0_VAP_NUM				(4)
#else
#define RTL_WLAN0_VAP_NUM				(0)
#endif

#define RTL_WLAN0_BIND_MASK_BIT		(4)
#define RTL_WLAN0_BIND_MASK			(1<<RTL_WLAN0_BIND_MASK_BIT)
#ifdef CONFIG_RTL_VAP_SUPPORT
#define RTL_WLAN0_VAP0_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+1))
#define RTL_WLAN0_VAP1_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+2))	
#define RTL_WLAN0_VAP2_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+3))
#define RTL_WLAN0_VAP3_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+4))	
#endif
#ifdef CONFIG_USE_PCIE_SLOT_0
#define RTL_WLAN1_BIND_MASK			(1<<(RTL_WLAN0_BIND_MASK_BIT+RTL_WLAN0_VAP_NUM+1))
#ifdef CONFIG_RTL_VAP_SUPPORT
#define RTL_WLAN1_VAP0_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+RTL_WLAN0_VAP_NUM+2))
#define RTL_WLAN1_VAP1_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+RTL_WLAN0_VAP_NUM+3))	
#define RTL_WLAN1_VAP2_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+RTL_WLAN0_VAP_NUM+4))
#define RTL_WLAN1_VAP3_BIND_MASK	(1<<(RTL_WLAN0_BIND_MASK_BIT+RTL_WLAN0_VAP_NUM+5))	
#endif
#endif

struct lan_dev_bind_mask_drv 
{
	char 	ifname[IFNAMSIZ];
	unsigned int bind_mask;
};

typedef enum rtk_multi_wan_debug_level_e
{
	RTK_MULTI_WAN_DBG_LEVEL_DEBUG=0x1,
	RTK_MULTI_WAN_DBG_LEVEL_ERROR=0x2,
	RTK_MULTI_WAN_DBG_LEVEL_ALL=0xffffffff,
} rtk_multi_wan_debug_level_t;

#ifdef CONFIG_RTL_MULTI_ETH_WAN
#ifndef RTK_MULTI_WAN_DEBUG_PRK
#define RTK_MULTI_WAN_DEBUG_PRK( bitmask, string, color,bgcolor,comment ,arg...) \
do {\
	int show=bitmask; \
    if(show & rtk_multi_wan_debug)\
    {\
            int mt_trace_i;\
            snprintf(rtk_multi_wan_dbg_buff, sizeof(rtk_multi_wan_dbg_buff),comment,## arg);\
            for(mt_trace_i=1;mt_trace_i<1024;mt_trace_i++) \
            { \
                    if(rtk_multi_wan_dbg_buff[mt_trace_i]==0) \
                    { \
                            if(rtk_multi_wan_dbg_buff[mt_trace_i-1]=='\n') rtk_multi_wan_dbg_buff[mt_trace_i-1]=' '; \
                            else break; \
                    } \
            } \
            printk("\033[1;%d;%dm[%s] %s \033[1;30;40m@%s(%d)\033[0m\n",color,bgcolor,string,rtk_multi_wan_dbg_buff,__FUNCTION__,__LINE__); \
    } \
} while(0)
#endif

#define TRACE(comment ,arg...) 	RTK_MULTI_WAN_DEBUG_PRK(RTK_MULTI_WAN_DBG_LEVEL_DEBUG,"TRACE",35,40,comment,##arg)
#define ERROR( comment ,arg...) RTK_MULTI_WAN_DEBUG_PRK(RTK_MULTI_WAN_DBG_LEVEL_ERROR,"ERROR",34,40,comment,##arg)
#else
#define TRACE(comment ,arg...)
#define ERROR( comment ,arg...)
#endif

extern unsigned int rtk_multi_wan_debug;
extern unsigned char rtk_multi_wan_dbg_buff[RTK_MAX_DBG_MSG_LEN];


typedef int (*rtl_register_multi_wan_hook_t)(const char *, const char *, int ,int , unsigned int, int , int );
typedef int (*rtl_unregister_multi_wan_hook_t)(const char*, int);
extern rtl_register_multi_wan_hook_t rtl_register_multi_wan_hook;
extern rtl_unregister_multi_wan_hook_t rtl_unregister_multi_wan_hook;

typedef int (*rtl_update_multi_wan_port_mapping_hook_t)(char *, int, unsigned int, char, unsigned int);
extern rtl_update_multi_wan_port_mapping_hook_t rtl_update_multi_wan_port_mapping_hook;

typedef int (*rtl83xx_vlan_set_hook_t)(unsigned int, unsigned int, unsigned int, unsigned short, unsigned short);
extern rtl83xx_vlan_set_hook_t rtl83xx_vlan_set_hook;
typedef int (*rtl83xx_vlan_get_hook_t)(unsigned int, unsigned int *, unsigned int *, unsigned short *,unsigned short *);
extern rtl83xx_vlan_get_hook_t rtl83xx_vlan_get_hook;
typedef int (*rtl83xx_pvid_set_hook_t)(unsigned int, unsigned int, unsigned int);
extern rtl83xx_pvid_set_hook_t rtl83xx_pvid_set_hook;
typedef int (*rtl83xx_pvid_get_hook_t)(unsigned int , unsigned int *, unsigned int *);
extern rtl83xx_pvid_get_hook_t rtl83xx_pvid_get_hook;
typedef int (*rtl83xx_get_cpu_portmask_hook_t)(void);
extern rtl83xx_get_cpu_portmask_hook_t rtl83xx_get_cpu_portmask_hook;

typedef unsigned int (*ca_ni_get_wan_port_id_hook_t)(void);
extern ca_ni_get_wan_port_id_hook_t ca_ni_get_wan_port_id_hook;
typedef unsigned int (*rtl_get_board_type_hook_t)(void);
extern rtl_get_board_type_hook_t rtl_get_board_type_hook;

typedef unsigned int (*rtl_get_phy_portmask_by_dev_name_hook_t)(char *);
typedef unsigned int (*rtl_get_cpu_portmask_by_dev_name_hook_t)(char *);
extern rtl_get_phy_portmask_by_dev_name_hook_t rtl_get_phy_portmask_by_dev_name_hook; 

extern int rtl_register_multi_wan_dev(const char *rifname, const char *nifname, int smux_proto,
										int vid, unsigned int priority, int napt, int brpppoe);
extern int rtl_unregister_multi_wan_dev(const char *nifname, int vid);

extern int  rtl_multi_wan_init(void);

extern void  rtl_multi_wan_exit(void);

extern int rtl_multi_wan_rx_process(struct sk_buff *skb, unsigned int phy_src_port);

extern unsigned int rtl_get_board_type(void);

#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT) && defined(CONFIG_RTK_LINUX_VLAN_SUPPORT)
extern int rtl_change_dev_mask_to_port_mask(unsigned int *port_mask, unsigned int *tag_mask);
#endif

#endif
