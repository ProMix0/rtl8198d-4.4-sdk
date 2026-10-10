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

#ifndef __RTK_COMMON_UTILITY_H__
#define __RTK_COMMON_UTILITY_H__

#define	RTK_PS_HOOKS_RETURN		0		/*	should immediately return from the caller function	*/
#define	RTK_PS_HOOKS_DROP		1
#define	RTK_PS_HOOKS_CONTINUE	2		/*	keep the process flow	*/

extern struct proc_dir_entry *rtk_proc_dir;
extern int rtk_op_mode;

/* should keep pace with user layer */
typedef enum rtk_op_mode_enum_e
{
	GATEWAY_MODE =0,
	BRIDGE_MODE,
	WISP_MODE,
	REPEATER_MODE
	
}rtk_op_mode_enum_t;


#if defined(CONFIG_RTL_8226_SUPPORT)
typedef unsigned int (*rtl_get_8226_lan_dev_name_hook_t)(char *dev_name);
extern rtl_get_8226_lan_dev_name_hook_t rtl_get_8226_lan_dev_name_hook;
extern int rtk_get_8226_lan_dev_name(void);
#endif

typedef int (*aal_mdio_read_indirect_hook_t)(unsigned short device_id, unsigned char phy_port_addr, unsigned char dev_type, unsigned short addr, unsigned short *data);
typedef int (*aal_mdio_write_indirect_hook_t)(unsigned short device_id, unsigned char phy_port_addr, unsigned char dev_type, unsigned short addr, unsigned short data);
extern aal_mdio_read_indirect_hook_t aal_mdio_read_indirect_hook;
extern aal_mdio_write_indirect_hook_t aal_mdio_write_indirect_hook;

typedef int (*rtl83xx_op_mode_set_hook_t)(int);
extern rtl83xx_op_mode_set_hook_t rtl83xx_op_mode_set_hook;

int rtk_get_op_mode(void);

int rtk_netif_receive_skb_hooks(struct sk_buff **pskb);
int rtk_netif_xmit_skb_hooks(struct sk_buff **pskb);

#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)
typedef unsigned int (*rtl_get_8226_lan_dev_portnum_hook_t)(void);
extern rtl_get_8226_lan_dev_portnum_hook_t rtl_get_8226_lan_dev_portnum_hook;

typedef unsigned int (*rtl_get_portnum_in_8367_hook_t)(unsigned int portnum);
extern rtl_get_portnum_in_8367_hook_t rtl_get_portnum_in_8367_hook;
#endif


#if defined(CONFIG_RTL_BRIDGE_VLAN_SPECIAL_SUPPORT)
int rtk_insert_vlan_tag_and_priority(unsigned short vid, unsigned char priority, struct sk_buff **pskb);
#endif

#endif
