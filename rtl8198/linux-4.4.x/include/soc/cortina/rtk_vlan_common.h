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

#ifndef __RTK_VLAN_COMMON_H__
#define __RTK_VLAN_COMMON_H__

#define	RTL_DEFAULT_VLANID		1

#define	RTL_WANVLANID			8
#define	RTL_LANVLANID			9

#define CONFIG_RTL_IVL_SUPPORT

#define RTL_LAN_FID				0
#if defined (CONFIG_RTL_IVL_SUPPORT)	
#define RTL_WAN_FID				1
#else
#define RTL_WAN_FID				0
#endif

#define	RTL_DEFAULT_FID			2

#define MAX_ETH_NUM				5

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef RTK_SKB_COPIED
#define RTK_SKB_COPIED 1
#endif

#define RTK_VLAN_MAX_DBG_MSG_LEN     512

typedef enum rtk_vlan_debug_level_e
{
	RTK_VLAN_DBG_LEVEL_DEBUG=0x1,
	RTK_VLAN_DBG_LEVEL_ERROR=0x2,
	RTK_VLAN_DBG_LEVEL_ALL=0xffffffff,
} rtk_vlan_debug_level_t;


/* debug function for vlan related */
#ifndef RTK_VLAN_DEBUG_PRK
#define RTK_VLAN_DEBUG_PRK( bitmask, string, color,bgcolor,comment ,arg...) \
do {\
	int show=bitmask; \
    if(show & rtk_vlan_debug)\
    {\
            int mt_trace_i;\
            snprintf(rtk_vlan_dbg_buff, sizeof(rtk_vlan_dbg_buff),comment,## arg);\
            for(mt_trace_i=1;mt_trace_i<1024;mt_trace_i++) \
            { \
                    if(rtk_vlan_dbg_buff[mt_trace_i]==0) \
                    { \
                            if(rtk_vlan_dbg_buff[mt_trace_i-1]=='\n') rtk_vlan_dbg_buff[mt_trace_i-1]=' '; \
                            else break; \
                    } \
            } \
            printk("\033[1;%d;%dm[%s] %s \033[1;30;40m@%s(%d)\033[0m\n",color,bgcolor,string,rtk_vlan_dbg_buff,__FUNCTION__,__LINE__); \
    } \
} while(0)
#endif

#define VLAN_TRACE(comment ,arg...) 	RTK_VLAN_DEBUG_PRK(RTK_VLAN_DBG_LEVEL_DEBUG,"TRACE",35,40,comment,##arg)
#define VLAN_ERROR( comment ,arg...) RTK_VLAN_DEBUG_PRK(RTK_VLAN_DBG_LEVEL_ERROR,"ERROR",34,40,comment,##arg)



extern unsigned int rtk_vlan_debug;
extern unsigned char rtk_vlan_dbg_buff[RTK_VLAN_MAX_DBG_MSG_LEN];

extern int rtk_insert_vlan_tag_and_priority(unsigned short vid, unsigned char priority, struct sk_buff **pskb);
extern int rtk_mod_vlan_tag_priority(unsigned char priority, struct sk_buff **pskb);
extern int remove_vlan_tag(struct sk_buff **pskb);
extern unsigned short rtk_get_skb_vid(struct sk_buff *skb);

#endif
