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
#ifndef __CA_NI_H__
#define __CA_NI_H__

#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>

#include "aal_ni.h"

#include "ca_ni_common.h"
#include "ca_ethtool.h"
#include "ca_ni_tx.h"
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#include "sys.h"
#endif

#define DRV_NAME	"Cortina Network Interface Driver"
#define DRV_VERSION	"0.0.1"

#ifdef CONFIG_NET_SCH_MULTIQ
#define QUEUE_PER_INSTANCE  8
#endif

#define CA_ENABLE			1
#define CA_DISABLE			0

/* max number of interfaces supportted is 8 the real number of interfaces should be specified in startup config */
#define CA_NI_MAX_INTF			8

/* the min DMA size of G3 DMA LSO is 34 */
#define CA_NI_MIN_DMA_SIZE		34

/* min packet size */
#define CA_NI_MIN_PKT_SIZE		64

#define CA_NI_TX_TIMEOUT		(6 * HZ)
#define CA_NI_XRAM_RX_INSTANCE		8

#define CA_NI_NUM_OF_CPU		8

#define REG32(addr)			(*(volatile u32*)(addr))

#define CA_NI_XRAM_BASE                                    0xF4500000

/* index of interrupts in .dts file */
#define CA_NI_IRQ_QM_CPU_FIFO0          0
#define CA_NI_IRQ_QM_CPU_FIFO1          1
#define CA_NI_IRQ_QM_CPU_FIFO2          2
#define CA_NI_IRQ_QM_CPU_FIFO3          3
#define CA_NI_IRQ_QM_CPU_FIFO4          4
#define CA_NI_IRQ_QM_CPU_FIFO5          5
#define CA_NI_IRQ_QM_CPU_FIFO6          6
#define CA_NI_IRQ_QM_CPU_FIFO7          7
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_8   8
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_9   9
#define CA_NI_IRQ_QM_CPU256_FIFO_REQ_HI 10
#define CA_NI_IRQ_QM_CPU256_FIFO_REQ_LO 11
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_4   12
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_5   13
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_6   14
#define CA_NI_IRQ_QM_CPU_REFILL_REQ_7   15


/* defined in "ca_ni_tx.h" */
struct ca_ni_priv;
struct dma_swtxq_t;

/* the port configure should be handle in Cortina port APIs directly */
typedef struct ca_port_cfg_info {
	u8 link;
	u8 autoneg;
	u16 speed;
	u8 duplex;
	u8 flowctrl;
	u8 num_phy_port;
	u8 port_id[CA_NI_NUM_OF_PORT];
	u8 rx_ldpid;			/* used to tell where the incoming packet come from, 0x10=from LAN port, 0x11=from WAN port */
	u8 tx_ldpid;			/* used to tell where the ougoing packet will go to, 0x18=go to LAN port, 0x19=go to WAN port */
	int irq[CA_NI_NUM_OF_CPU];      /* to support multiple cpu port in one interface */
	u8 *mac_addr;
	u8 rmii_clk_src;
} ca_port_cfg_info_t;

#if defined(CONFIG_RTK_NIC_RX_HOOK)
//necessary parameters for rx hook func.
typedef struct nic_hook_private
{
	ca_uint8_t *save_skb_data;
	ca_mac_addr_t sa_mac;
	ca_mac_addr_t da_mac;
	HEADER_A_T *hdr_a;
	HEADER_CPU_T *hdr_cpu;
	HEADER_PTP_T *hdr_ptp;

}nic_hook_private_t ;

typedef int (*p2rfunc_t)(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv);//, struct rx_info *pRxInfo);
typedef int (*p2NicRxHook_t)(int portmask,int priority,p2rfunc_t rx) ;
typedef int (*p2NicRxUnHook_t)(int portmask,int priority,p2rfunc_t rx) ;
int nic_register_rxfunc_by_port(unsigned int port, p2rfunc_t pfunc);
int nic_register_rxfunc_all_port(p2rfunc_t pfunc);
void nic_reset_rxfunc_to_default(void);

typedef int (*p2tfunc_t)(struct sk_buff *skb, struct net_device *dev);

#endif /*CONFIG_RTK_NIC_RX_HOOK*/


typedef struct ca_eth_private {
	struct net_device *dev;
	struct platform_device *pdev;
	struct mii_bus *mdio_bus;
	struct phy_device *phydev;
	struct napi_struct napi[CA_NI_NUM_OF_CPU];

	void __iomem *base;		/* Ethernet controller base address */

	u8 status;			/* enable/disable ??? */
	int ni_driver_state;		/* 1: ready, 0: initializing */
	u32 mdio_inited;		/* 1: mdio is inited, 0: mdio is not inited */

	/*
	 * Device status & features
	 */
	ca_port_cfg_info_t port_cfg;

	/* to support multiple cpu ports in one interface */
	int num_cpu_port;
	u8 cpu_port[CA_NI_NUM_OF_CPU];          /* indicate which cpu port attached to this interface */
	u8 active_cpu_port;
	u8 egress_cpu_port_id;

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
	unsigned long tx_completion_queues;
	struct timer_list tx_completion_timer;
	struct ca_ni_priv mgmt_priv;		/* private data for management mode */

	/*
	 * Hardware-specific parameters
	 */
	bool  use_default_lso_para0;
	u32 default_lso_para0;
	u32 deep_q:1;
	u32 dma_l3fe:1;

	/*
	 * lock for access to global data
	 */
	///      these locks are used for device based not NI driver based
	spinlock_t stats_lock;
	spinlock_t lock;

#ifdef CONFIG_CA_NE_VIRTUAL_NI
	/*
	 * virtual NI
	 */
	u16 vni_vlan_proto;		/// ca_virt_ni_t.vlan_proto
	u16 vni_vlan_tci;		/// ca_virt_ni_t.vlan_tci
#endif

#ifdef CONFIG_RTK_NIC_RX_HOOK
	spinlock_t		tx_lock;
	spinlock_t		rx_lock;
	int (*port2rxfunc[CA_NI_MAX_INTF])(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv);//, struct rx_info *pRxInfo);
#endif
#ifdef CONFIG_RTK_NIC_TX_HOOK
	int (*txfunc)(struct sk_buff *skb, struct net_device *dev);
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	struct net_device* port2dev[CA_MAX_ETHERNET_PORT_NUMBER];
#endif

} ca_eth_private_t;

typedef struct ni_private {
	struct net_device *dev[CA_NI_MAX_INTF];
	dma_swtxq_t swtxq[CA_NI_DMA_LSO_CPU_VP_NUM][CA_NI_DMA_LSO_TXQ_NUM];

	/* default values for NI registers */
	u32 intr_cpuxram_rxpkt_mask;
	u32 intr_cpuxram_txpkt_mask;
	u32 intr_sch_mask;

        /*
         * L3QM buffer management
         */
        u8 *l3qm_eb_base_addr;          		/* virtual address of EB pool for NI ports */
        dma_addr_t l3qm_eb_base_dma_addr;          	/* dma address of EB pool for NI ports */
        u32 *l3qm_cpu_epp_base_addr[CA_NI_TOTAL_CPU_PORT * CA_NI_NUM_VOQ_PER_PORT];  		/* virtual address of CPU EPP for transmiting */
        dma_addr_t l3qm_cpu_epp_base_dma_addr[CA_NI_TOTAL_CPU_PORT * CA_NI_NUM_VOQ_PER_PORT];  /* dma address of CPU EPP for transmiting */

	/*
	 * task for reset TX queues
	 */
	struct work_struct reset_task;

	/* refill threshold irq number for CPU64 EQ 8/9 */
	u16 	refill_req_8;
	u16 	refill_req_9;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	/* cpu256 HI/LO FIFO irq number */
	u16	cpu256_fifo_req_hi;
	u16	cpu256_fifo_req_lo;
	struct net_device *cpu256_dev[2];

	/* refill threshold irq number for CPU256 EQ 4/5/6/7 */
	u16     cpu256_refill_req_4;
	u16     cpu256_refill_req_5;
	u16     cpu256_refill_req_6;
	u16     cpu256_refill_req_7;
#endif

	/* used for dma_alloc_coherent() */
	struct platform_device *pdev;
} ni_info_t;

extern ca_port_cfg_info_t port_cfg_infos[];
extern ni_info_t ni_info_data;
extern int ni_use_fbm;
extern int ca_ni_mgmt_mode;
extern u8 ca_ni_eth_mac[CA_NI_MAX_INTF][6];
extern int ca_ni_num_intf;
extern u32 ca_ni_debug;
extern ca_uint8_t ca_ni_use_qm_eq_refill_int;
extern ca_uint8_t ca_ni_use_qm_eq_refill_tasklet;
extern ca_uint8_t ca_ni_use_build_skb;

u32 ca_ni_fill_empty_buffer_by_rule(struct net_device *dev, u8 eqid, int rule);
int ca_ni_fill_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 inactive_bid_cntr, int rule);


int ca_ni_refill_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt);
ca_status_t ca_ni_intf_get_ports(struct net_device *dev, ca_uint8_t *num_ports, int *ports, ca_uint8_t *is_wan_intf);
struct net_device * ca_ni_virtual_instance_alloc(int cpu_idx, char *name);
ca_status_t ca_ni_virtual_instance_free(int cpu_idx);
ca_status_t ca_ni_virtual_instance_open(int cpu_idx);
ca_status_t ca_ni_virtual_instance_close(int cpu_idx);
ca_status_t ca_ni_virtual_instance_xmit(int cpu_idx, struct sk_buff *skb, ca_ni_tx_config_t *tx_config);

/* spin lock for global resources */
extern spinlock_t  ca_ni_rx_refill_lock;

#ifdef CONFIG_RTK_NIC_RX_HOOK
ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data);
void ca_ni_l2_addr_add(ca_uint16_t vid, ca_mac_addr_t *sa_mac, int port_id, ca_uint8_t lrn_info);
void ca_ni_l2_flooding(ca_uint16_t vid, ca_mac_addr_t da_mac, struct sk_buff *skb, struct napi_struct *napi, HEADER_A_T *hdr_a);
#endif

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef CONFIG_ARCH_RTL8198F
#if defined(CONFIG_RTK_WANPORT_AT_PORT0)
#define LAN_PORT1				1
#define LAN_PORT2				2
#define LAN_PORT3				3
#define LAN_PORT4				4
#define LAN_PORT5				5
#define WAN_PORT1				0
#elif defined(CONFIG_RTK_WANPORT_AT_PORT1)
#define LAN_PORT1                               0
#define LAN_PORT2                               2
#define LAN_PORT3                               3
#define LAN_PORT4                               4
#define LAN_PORT5                               5
#define WAN_PORT1                               1
#else
#define LAN_PORT1				0
#define LAN_PORT2				1
#define LAN_PORT3				2
#define LAN_PORT4				3
#define LAN_PORT5				5
#define WAN_PORT1				4
#endif
#if defined(CONFIG_RTK_PE_FC_SUPPORT)
#define PE_PORT					22
#endif
#else
#define LAN_PORT1				0
#define LAN_PORT2				1
#define LAN_PORT3				2
#define LAN_PORT4				3
#define LAN_PORT5				6
#if defined(CONFIG_RG_G3_WAN_PORT_INDEX)
#define WAN_PORT1				CONFIG_RG_G3_WAN_PORT_INDEX
#else
#define WAN_PORT1				7
#endif
#endif
#if defined(CONFIG_RG_G3_SERIES_DEVELOPMENT)
#define MAX_CA_NI_MULTI_LAN_DEV_NUM	18
#else
#if defined(CONFIG_RTK_PE_FC_SUPPORT)
#define MAX_CA_NI_MULTI_LAN_DEV_NUM	7
#else
#define MAX_CA_NI_MULTI_LAN_DEV_NUM	6
#endif
#endif
struct ca_ni_multi_lan_dev_table_entry {
	unsigned char 			ifname[IFNAMSIZ];
	unsigned char			ifflag; //0:ELAN, 1:WAN, 2:WLAN
	unsigned short			vid;
	unsigned int			phyPort;
	unsigned char 			isPanelPort;
	struct net_device 		*dev_instant;
};
#endif


#endif /* __CA_NI_H__ */

