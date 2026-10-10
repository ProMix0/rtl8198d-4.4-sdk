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
#ifndef __CA_NI_PROC_H__
#define __CA_NI_PROC_H__

#include <linux/proc_fs.h>

#if defined(CONFIG_RTL_FPGA_PHY_TEST)

#include "ca_ni.h"	//for ca_ni_rx_tx_test_t


typedef struct {
	ca_uint8_t 			test_mode;	//0: disable, 1: direct tx, 2: hwlookup
	ca_uint32_t 		remarking_dip;
	ca_ni_tx_config_t 	ni_tx_config;
	dma_lsopara_0_t		ni_tx_lsopara0;
	ca_uint32_t			hash_index;
	ca_uint8_t			hash_key_ip_protocol;
	ca_uint32_t			hash_key_ip_sa;
	ca_uint32_t			hash_key_ip_da;
	ca_uint16_t			hash_key_l4_sport;
	ca_uint16_t 		hash_key_l4_dport;
	ca_uint32_t 		hash_key_o_lspid;
	ca_uint32_t			hash_act_ldpid_msk	:1;
	ca_uint32_t			hash_act_ldpid;
	ca_uint32_t			hash_act_dscp_msk	:1;
	ca_uint8_t			hash_act_dscp;
	ca_uint32_t			hash_act_sw_id_msk	:1;
	ca_uint32_t			hash_act_sw_id[4];
	ca_uint32_t 		verify_ldpid;
} ca_ni_rx_tx_test_t;
#endif

enum CA_NI_DEBUG {
	NI_DBG_DIASBLE		= 0,
	NI_DBG_NI		= 0x00000001,
	NI_DBG_IRQ		= 0x00000002,
	NI_DBG_LSO		= 0x00000004,
	NI_DBG_DUMP_RX		= 0x00000008,
	NI_DBG_DUMP_TX		= 0x00000010,
	NI_DBG_IOCTL		= 0x00000020,
	NI_DBG_VIR_NI		= 0x00000040,
	NI_DBG_ETHTOOL		= 0x00000080,
	NI_DBG_PHY		= 0x00000100,
	NI_DBG_NETLINK		= 0x00000200,

	NI_DBG_MAX		= NI_DBG_NETLINK,
	NI_DBG_NETLINK_DUMP		= 0x80000000,
	NI_DBG_DMA_LSO_DUMP		= 0x40000000,
	NI_DBG_NETLINK_RX_QUEUE_DUMP    = 0x20000000,
};

int ca_ni_proc_init(void);
void ca_ni_proc_exit(void);


#endif /* __CA_NI_PROC_H__ */

