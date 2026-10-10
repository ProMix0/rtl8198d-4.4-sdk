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
#ifndef __CA_NI_CPU256_H__
#define __CA_NI_CPU256_H__

#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>

#include "aal_ni.h"

#include "ca_ni_common.h"

irqreturn_t ca_ni_qm_cpu256_refill_interrupt(int irq, void *dev_instance);
int ca_ni_init_cpu256_intfs(struct platform_device *pdev);
void ca_ni_fbm_fill_cpu256_hi_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count);
void ca_ni_fbm_fill_cpu256_lo_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count);
void ca_ni_fbm_fill_cpu256_hi_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count);
void ca_ni_refill_cpu256_lo_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt);
void ca_ni_fill_cpu256_eq_buf_pool(struct net_device *dev);
void ca_ni_refill_cpu256_hi_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt);
void ca_ni_refill_cpu256_lo_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt);
int ca_ni_register_cpu256_netdev(void);
void ca_load_cpu256_init(void);
void ca_ni_init_cpu256_module_probe(struct platform_device *pdev);

#endif
