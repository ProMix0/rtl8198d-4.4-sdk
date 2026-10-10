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
#ifndef __CA_NI_TITAN_H__
#define __CA_NI_TITAN_H__

#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include "ca_ethtool.h"

#define DRV_NAME	"Cortina Network Interface Driver for Titan FPGA"
#define DRV_VERSION	"0.0.1"

#define CA_ENABLE			1
#define CA_DISABLE			0

/* max number of interfaces supportted is 8 the real number of interfaces should be specified in startup config */
#define CA_NI_MAX_INTF			2

#define CA_NI_TX_TIMEOUT		(6 * HZ)

#define AAL_IDP_PKT_SIZE		2048

/* defined in "ca_ni_tx.h" */
//struct ca_ni_priv;

typedef union {
	uint32_t	wrd;

	/* FPGA_IDP_DOWN_INT, FPGA_IDP_DOWN_INTE */
	struct {
		uint32_t	rx_pkt_rdy	: 1;
		uint32_t	rx_pkt_err	: 1;
		uint32_t	rx_oversize	: 1;
		uint32_t	rx_pkt_drop	: 1;
		uint32_t	useless		: 28;
	} wan;

	/* FPGA_IDP_UP_INT, FPGA_IDP_UP_INTE */
	struct {
		uint32_t	useless_0	: 8;
		uint32_t	rx_pkt_rdy	: 1;
		uint32_t	rx_pkt_err	: 1;
		uint32_t	rx_oversize	: 1;
		uint32_t	rx_pkt_drop	: 1;
		uint32_t	useless_1	: 20;
	} lan;
} ca_reg_interrupt_t;

/* the port configure should be handle in Cortina port APIs directly */
typedef struct ca_port_cfg_info {
#if 0
	u8 link;
	u8 autoneg;
	u16 speed;
	u8 duplex;
	u8 flowctrl;
	//u8 phy_mode;
	u8 num_phy_port;
	u8 port_id[CA_NI_NUM_OF_PORT];
	u8 rx_ldpid;			/* used to tell where the incoming packet come from, 0x10=from LAN port, 0x11=from WAN port */
	u8 tx_ldpid;			/* used to tell where the ougoing packet will go to, 0x18=go to LAN port, 0x19=go to WAN port */
	//u8 phy_addr;
	int irq;
	u8 *mac_addr;
	u8 rmii_clk_src;
#endif
} ca_port_cfg_info_t;

typedef struct ca_eth_private {
	struct net_device *dev;
	struct platform_device *pdev;
	struct mii_bus *mdio_bus;
	struct phy_device *phydev;
	struct napi_struct napi;

	int index;			/* device index; 0 is WAN (PON), 1 is LAN */
	ca_port_id_t port_id;

	/* register physical address */
	void __iomem *reg_rx_ctrl;	/* read done control */
	void __iomem *reg_rx_data;	/* 2K-byte FIFO */
	void __iomem *reg_rx_pkt_size;
	void __iomem *reg_rx_int;	/* interrupt */

	void __iomem *reg_tx_ctrl;	/* write done control */
	void __iomem *reg_tx_data;	/* 2K-byte FIFO */
	void __iomem *reg_tx_pkt_size;

	u8 status;			/* enable/disable ??? */
	int ni_driver_state;		/* 1: ready, 0: initializing */
	u32 mdio_inited;		/* 1: mdio is inited, 0: mdio is not inited */

	/*
	 * Device status & features
	 */
	// ca_port_cfg_info_t port_cfg;

	u8 cpu_port;			/* indicate which cpu port attached to this interface */

	u32 an_bng_mac_idx;
	u32 an_bng_mac_idx_pe0;
	u32 an_bng_mac_idx_pe1;
	u32 msg_enable;

	/*
	 * Statistics
	 */
	struct ca_ethtool_stats ni_stats;
	struct net_device_stats ifStatics;
	u8 rx_checksum;

	/*
	 * Rx-only resource
	 */


	/*
	 * Tx-only resource
	 */
	struct timer_list rx_poll_timer;

	/*
	 * Hardware-specific parameters
	 */

	/*
	 * lock for access to global data
	 */
	spinlock_t lock;

#ifdef CONFIG_CA_NE_VIRTUAL_NI
	/*
	 * virtual NI
	 */
	u16 vni_vlan_proto;		/// ca_virt_ni_t.vlan_proto
	u16 vni_vlan_tci;		/// ca_virt_ni_t.vlan_tci
#endif

	/*
	 * pure software Rx flow control
	 */
	struct timer_list flow_ctrl_timer;
	uint32_t fps; /* frames per second */
	atomic_t flow_cnt;
	atomic_t in_irq_handler;

} ca_eth_private_t;

typedef struct ni_private {
	struct net_device *dev[CA_NI_MAX_INTF];
} ni_info_t;

extern ni_info_t ni_info_data;
extern int ni_use_fbm;
extern int ca_ni_mgmt_mode;
extern u8 ca_ni_eth_mac[CA_NI_MAX_INTF][6];
extern int ca_ni_num_intf;

void ca_ni_refill_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt);

ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data);
ca_uint16_t ca_ni_rx_get_protocol(ca_uint8_t *data, ca_uint8_t **p_protocol);

/************************************************************************
**                  HEADER A
*************************************************************************/
/* from G3 Header_v32.xlsm */
typedef union {
	uint64_t bits64;
	struct header_a {
		uint32_t cos			: 3;	/* COS */
		uint32_t ldpid			: 6;	/* Logical Destination Port ID, Packet from MC Engine, this field carries MC_INDEX */
		uint32_t lspid			: 6;	/* Original Logical Source Port ID */
		uint32_t pkt_size		: 14;	/* Packet size */
		uint32_t fe_bypass		: 1;	/* Bypass L2FE and L3FE processing */
		uint32_t hdr_type		: 2;	/* 00: Generic* 01: MC HDR
							 * 01: MC HDR
							 * 10: CPU bound pkt
							 * 11: PTP ingress or egress pkt
							 */

		union {

			struct {
				uint32_t mcgid			: 8;	/* When packet type is 2'b10(CPU), redefined as L2 learn info */
				uint32_t drop_code		: 3;	/* Drop event code, used by TE */
				uint32_t rx_pkt_type		: 2;	/* 00:UC, 01:BC, 10:MC, 11:Unknown-UC
									   for l3fe->l2fe traffic, use uc(00) to disable l2 learning
									   uuc(11) to enable l2 learning */
				uint32_t no_drop		: 1;	/* No-Drop flag for high priority packet */
				uint32_t mirror			: 1;	/* Packet to be mirrored */
				uint32_t mark			: 1;	/* Conjestion experienced in QM */
				uint32_t pol_en			: 2;	/* 2'b00: Policer disable
									   2'b01: L2 Policer enable
									   2'b10: L3 Policer enable
									   2'b11: both L2,L3 Policer enable
									 */

				uint32_t pol_id			: 9;	/* Used for Policing in TE/QTM */
				uint32_t pol_grp_id		: 3;	/* Policer Group ID, group 0 is NULL */
				uint32_t deep_q			: 1;	/* Packet target to DeepQ in QM */
				uint32_t cpu_flg		: 1;	/* CPU header is included */
			} pkt_info;

			struct {
				uint32_t ptp_info		: 31;	/* Ingress PTP traffic, FE copies TS to PTP_INFO.
									   Egress PTP traffic carries PTP_CMD in PTP_INFO field.
									   This field is overloaded with {POL_EN,POL_GRP_ID,POL_ID,MIRROR,
									   DEEP_Q,MARK,CPU_FLAG,NO_DROP, RESERVED,RX_PKT_TYPE,MCGID}fields,
									   valid when HDR_TYPE is 11
									 */
				uint32_t cpu_flg		: 1;	/* CPU header is included */
			} ptp_info;

			struct {
				uint32_t cmd_asel		: 2;	/* select one of 4 asymmetry value programmed in CSRs */
				uint32_t cmd_op			: 2;	/* 00: NO_OP, take timestamp to global CSR (2 pass mode), interrupt CPU
									   01: TS_CORRECT, use timestamp without asymmetry offset
									   10: TS_P_OFFSET, use timestamp and (+) offset
									   11: TS_N_OFFSET, use timestamp and (-) offset
									 */
				uint32_t reserved_0		: 1;
				uint32_t cmd_mode		: 1;	/* 1: timestamp mode, if cmd_op != NO_OP, write timestamp to
									      location defined by cmd_offset
									   0: correction field mode, if cmd_op != NO_OP, take location
									      defined by cmd_offset, and timestamp, write it back to same
									      location (correction field)
									 */
				uint32_t ptp_type		: 2;	/* PTP packet type
									   00: PTP over Ethernet
									   01: CFM
									   10: PTP over UDP over IPv4; need to update UDP cksum if it is non-zero
									   11: PTP over UDP over IPv6; need to update UDP cksum
									 */
				uint32_t cmd_offset		: 7;	/* Start of Timestamp or correction field. This is the byte number
									   from the SOP of the packet
									 */
				uint32_t reserved_1		: 1;
				uint32_t udp_cksum_offset	: 7;	/* offset for UDP checksum field if ptp_type is PTP over UDP */
				uint32_t reserved_2		: 8;
				uint32_t cpu_flg		: 1;	/* CPU header is included */
			} ptp_cmd;
		} bits_32_63;
	} bits;
} HEADER_A_T;

#define MAX_FRAME_SIZE		(AAL_IDP_PKT_SIZE - sizeof(HEADER_A_T) - ETH_FCS_LEN)

#endif /* __CA_NI_TITAN_H__ */

