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

#ifndef __RTK_LAN_VLAN_H__
#define __RTK_LAN_VLAN_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/if.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/mm_types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <asm/uaccess.h>


#ifdef __ECOS
#define RTK_USE_MEMPOOL_FOR_VLAN_INFO
#endif

#ifdef __ECOS
#define SEQ_PRINT(format, args...) panic_printk(format, ##args)
#else
#define SEQ_PRINT(format, args...) seq_printf(s, format, ##args)
#endif

#ifdef RTK_USE_MEMPOOL_FOR_VLAN_INFO
#define LAN_VLAN_POOL_SIZE 4
#endif

#define RTK_PARAMETER_SIZE	12



#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT)
#define RTK_LAN_ETH_NUM 4

extern int rtk_vlan_mode;
extern int rtk_lan_vlan_enable;
extern int rtk_lan_vlan_dev_mode;
#endif

enum rtk_vlan_mode {
	RTK_WAN_VLAN_MODE,
	RTK_SOURCE_VLAN_MODE
};

enum rtk_vlan_dev_mode {
  	RTK_NORMAL_VLAN_DEV_MODE,
  	RTK_NO_VLAN_DEV_MODE,
  	RTK_ALL_VLAN_DEV_MODE
};

enum rtk_lan_vlan_info_find_mask {
  	RTK_LAN_VLAN_INFO_VID = (1<<0),
	RTK_LAN_VLAN_INFO_PORTMASK = (1<<1),
	RTK_LAN_VLAN_INFO_TAGMASK = (1<<2),
	RTK_LAN_VLAN_INFO_BR_WAN = (1<<3),
	RTK_LAN_VLAN_INFO_HW_NAT = (1<<4)
};

struct rtk_lan_vlan_info {
	/* lanside vlan id, unique, duplicate check key */
	int lan_vid;
	/* binding wan’s vlan id */
	int wan_vid;
	/* virtual wan name such as nas0_0 etc. */
	unsigned char wan_dev_name[IFNAMSIZ];
	/* the portmask of lan_vid */
	uint64_t portmask;
	/* the tagmask of lan_vid */
	uint64_t tagmask;
	/* vlan binding to bridge wan or nat wan ? 
	 * 1:bridge wan, 0 nat wan, if binding to bridge wan,
	 * and lanside vid not equal to wanside vid, set this value to 0.
	 */
	int br_wan;
	int hw_nat;
	#ifndef RTK_USE_MEMPOOL_FOR_VLAN_INFO
	struct list_head list;
	#else
	int valid;
	#endif
};

struct rtk_lan_vlan_info_find{
	int vid;
	uint64_t portmask;
	uint64_t tagmask;
	int br_wan;
	int hw_nat;
	unsigned int find_mask;
	unsigned int inverse_mask;
};

#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT)
extern int rtk_find_lan_vlan_info(struct rtk_lan_vlan_info *info, struct rtk_lan_vlan_info_find *info_find);
extern int rtk_lan_vlan_process_rx(struct sk_buff **pskb);
extern int rtk_remove_lan_vlan_info(int vid, uint64_t portmask, uint64_t tagmask);
extern int rtk_add_lan_vlan_info(struct rtk_lan_vlan_info lan_info);
#endif
#endif
