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

#ifndef __RTK_VLAN_PASSTHROUGH_H__
#define __RTK_VLAN_PASSTHROUGH_H__

#define RTK_SVLAN_TAGIF  (1<<0)
#define RTK_CVLAN_TAGIF  (1<<1)

#if 0
#ifndef SVLAN_TAGIF
#define SVLAN_TAGIF  (1<<0)
#endif

#ifndef CVLAN_TAGIF
#define CVLAN_TAGIF  (1<<1)
#endif
#endif

extern struct proc_dir_entry proc_root;

typedef int (*rtl83xx_vlan_passthrough_set_hook_t)(int);
extern rtl83xx_vlan_passthrough_set_hook_t rtl83xx_vlan_passthrough_set_hook;

int rtk_get_vlan_passthrough_enable(void);
int rtk_vlan_passthrough_rx(struct sk_buff **pskb, int flag);
int rtk_vlan_passthrough_tx(struct sk_buff **pskb);
int rtk_vlan_passthrough_read(struct seq_file *s, void *v);
int rtk_vlan_passthrough_write( struct file *filp, const char *buff,unsigned long len, void *data );


#endif
