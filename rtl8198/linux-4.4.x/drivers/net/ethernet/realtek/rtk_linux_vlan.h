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
#ifndef __RTK_LINUX_VLAN_H__
#define __RTK_LINUX_VLAN_H__

#define RTK_WLAN_NUM_MAX 18
#define RTK_LAN_NUM_MAX 6
#define RTK_VLAN_TYPE_NAT	0
#define RTK_VLAN_TYPE_BRIDGE	1

#define RTK_LANPORT_MASK        0xDF
#define RTK_PHY_PORT_NUM  		6
#define RTK_LINUX_LAN_VLAN_ID	9
#define RTK_CPU_PORT_BIT		7	
#define RTK_LAN_MAX_ETH_NUM		4

enum rtk_mac_port_idx
 {
#if defined(CONFIG_FC_RTL9607C_SERIES)
	 RTK_MAC_PORT0=0,
	 RTK_MAC_PORT1=1,	 
	 RTK_MAC_PORT2=2, 
	 RTK_MAC_PORT3=3,
	 RTK_MAC_PORT4=4,
	 RTK_MAC_PORT_PON=5, 
	 RTK_MAC_PORT_iNIC=6, 
	 RTK_MAC_PORT_SLAVECPU=7, 
	 RTK_MAC_PORT_RGMII=8, 
	 RTK_MAC_PORT_MASTERCPU_CORE0=9, 
	 RTK_MAC_PORT_MASTERCPU_CORE1=10,
	 RTK_MAC_PORT_MAX,
#elif defined(CONFIG_FC_CAG3_SERIES)
	 RTK_MAC_PORT0=0,
	 RTK_MAC_PORT1=1,	 
	 RTK_MAC_PORT2=2, 
	 RTK_MAC_PORT3=3,
	 RTK_MAC_PORT4=4,	 // not support
	 RTK_MAC_PORT5=5,		 // not support
	 RTK_MAC_PORT6=6, 
#if defined(CONFIG_RG_G3_WAN_PORT_INDEX)
	 RTK_MAC_PORT_PON=CONFIG_RG_G3_WAN_PORT_INDEX,
#else
	 RTK_MAC_PORT_PON=7, 
#endif
	 RTK_MAC_PORT_CPU=0x10,
	 RTK_MAC_PORT_WLAN_CPU0=0x12,
	 RTK_MAC_PORT_WLAN_CPU1=0x13,
	 RTK_MAC_PORT_WLAN_CPU2=0x14,
	 RTK_MAC_PORT_WLAN_CPU3=0x15,
	 RTK_MAC_PORT_WLAN_CPU4=0x16,
	 RTK_MAC_PORT_WLAN_CPU5=0x17,
	 RTK_MAC_PORT_MAX,
#elif defined(CONFIG_FC_RTL8198F_SERIES)
	 RTK_MAC_PORT0=0,
	 RTK_MAC_PORT1=1,
	 RTK_MAC_PORT2=2,
	 RTK_MAC_PORT3=3,
	 RTK_MAC_PORT_PON=4,
	 RTK_MAC_PORT4=5,
	 RTK_MAC_PORT5=6,
	 RTK_MAC_PORT6=7,
	 RTK_MAC_PORT_CPU=0x10,
	 RTK_MAC_PORT_WLAN_CPU0=0x12,
	 RTK_MAC_PORT_WLAN_CPU1=0x13,
	 RTK_MAC_PORT_WLAN_CPU2=0x14,
	 RTK_MAC_PORT_WLAN_CPU3=0x15,
	 RTK_MAC_PORT_WLAN_CPU4=0x16,
	 RTK_MAC_PORT_WLAN_CPU5=0x17,
	 RTK_MAC_PORT_MAX,
#endif
 };

extern int rtk_linux_vlan_rx(struct sk_buff **pskb, unsigned int portmask);
extern int rtk_linux_vlan_tx(struct sk_buff **pskb, unsigned int portmask);
 
#endif


