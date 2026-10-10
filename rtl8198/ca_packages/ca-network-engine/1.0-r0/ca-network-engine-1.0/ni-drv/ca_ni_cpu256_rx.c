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
/*
 * ca_ni_rx_cpu256.c: Cortina NI Rx handler for QM CPU256
 *
 */

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_arp.h>
#include <linux/igmp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/addrconf.h>

#include "osal_cmn.h"

#include "aal_common.h"
#include "aal_port.h"
#include "aal_ni.h"
#include "aal_l3qm.h"
#include "aal_l3qm_cpu256.h"
#include "aal_fbm.h"
#include "aal_fdb.h"
#include "aal_l3_pe.h"

#include "ca_ni_rx.h"
#include "ca_virt_ni.h"
#include "ca_ni.h"
#include "ca_ni_cpu256.h"
#include "ca_ni_proc.h"
#include "ca_ni_dump.h"

#include "l2.h"
#include "rx.h"
#include "vlan.h"
#include "port.h"

#include "ca-scfg/scfg.h"

#include "ca_mut.h"

void ca_ni_netlink_igmpsnooping_reply(int pid, int seq, ca_uint8_t *skb_data, ca_uint32_t data_len);
ca_status_t ca_ni_netlink_send_packet(int cpu_port, HEADER_A_T *hdr_a, ca_uint8_t *data, ca_uint32_t pkt_size, HEADER_CPU_T *hdr_cpu);

#define CA_NI_RX_MIN_REFILL_COUNT 8
#define CA_NI_RX_MAX_REFILL_COUNT 16
static int ca_ni_rx_get_bad_packet=0;

/* there is no head room in CPU256 profile LO/HI, always use build skb */
char ca_ni_cpu256_use_build_skb = 1;

static ca_uint8_t l3qm_cpu256_lo_pool0_eq_id = 6;
static ca_uint8_t l3qm_cpu256_lo_pool1_eq_id = 7;
static ca_uint8_t l3qm_cpu256_hi_pool0_eq_id = 4;
static ca_uint8_t l3qm_cpu256_hi_pool1_eq_id = 5;
static ca_uint32_t l3qm_cpu256_hi_pool0_buf_sz = 2048;

/* there is no head room in CPU256 profile LO/HI */
static int l3qm_cpu256_head_room_first = 0;

static int l3qm_cpu256_buf_size = 2048;
static ca_uint8_t l3qm_cpu256_desc_size = 4;
static ca_uint8_t l3qm_cpu256_desc_per_epp = 1;
static ca_uint8_t l3pm_cpu256_lo_use_fbm = 1;
static ca_uint8_t l3pm_cpu256_hi_use_fbm = 1;

static void ca_ni_rx_get_startup_config(void)
{
	static int init_done = 0;

	if (init_done == 0) {
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_SZ, 4, &l3qm_cpu256_hi_pool0_buf_sz);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID, 4, &l3qm_cpu256_lo_pool0_eq_id);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID, 4, &l3qm_cpu256_lo_pool1_eq_id);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID, 4, &l3qm_cpu256_hi_pool0_eq_id);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID, 4, &l3qm_cpu256_hi_pool1_eq_id);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_SZ, 4, &l3qm_cpu256_buf_size);
		//aal_scfg_read(CFG_ID_L3QM_CPU_PORT_HEAD_ROOM_FIRST, 4, &l3qm_cpu256_head_room_first);
		aal_scfg_read(CFG_ID_L3QM_DESC_SIZE, 1, &l3qm_cpu256_desc_size);
		aal_scfg_read(CFG_ID_L3QM_DESC_PER_EPP, 1, &l3qm_cpu256_desc_per_epp);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_USE_FBM, 1, &l3pm_cpu256_lo_use_fbm);
		aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_USE_FBM, 1, &l3pm_cpu256_hi_use_fbm);

		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_SZ=%d\n", __func__, l3qm_cpu256_hi_pool0_buf_sz);
		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID=%d\n", __func__, l3qm_cpu256_lo_pool0_eq_id);
		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID=%d\n", __func__, l3qm_cpu256_lo_pool1_eq_id);
		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID=%d\n", __func__, l3qm_cpu256_hi_pool0_eq_id);
		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID=%d\n", __func__, l3qm_cpu256_hi_pool1_eq_id);
		printk("%s: CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_SZ=%d\n", __func__, l3qm_cpu256_buf_size);
		//printk("%s: CFG_ID_L3QM_CPU_PORT_HEAD_ROOM_FIRST=%d\n", __func__, l3qm_cpu256_head_room_first);
		printk("%s: CFG_ID_L3QM_DESC_SIZE=%d\n", __func__, l3qm_cpu256_desc_size);
		printk("%s: CFG_ID_L3QM_DESC_PER_EPP=%d\n", __func__, l3qm_cpu256_desc_per_epp);
		init_done = 1;
	}
}

static inline struct net_device *get_dev_by_cpu_hdr(HEADER_A_T *hdr_a, struct net_device *orig_dev)
{
	int i;
	struct net_device *tmp_dev;
	ca_eth_private_t *cep;

	/* use ldpid to return the related net_device */
	for (i = 0; i < ca_ni_num_intf; i++) {
		tmp_dev = ni_info_data.dev[i];
		cep = netdev_priv(tmp_dev);

		/* at receive side check the logical destination port id */
		if (hdr_a->bits.ldpid == cep->port_cfg.rx_ldpid)
			return tmp_dev;
	}
	printk("%s: HEADER_A lspid=%d is not valid!!!\n", __func__, hdr_a->bits.lspid);
	return ni_info_data.dev[0];
}

static int ca_ni_rx_napi_get_first_segment_length(int cpu_port, HEADER_A_T *hdr_a)
{
	int seg_len;
	int head_room = l3qm_cpu256_head_room_first;

	seg_len = aal_l3qm_eq_get_cpu_pool0_buf_size() - head_room - sizeof(HEADER_A_T);
	if (hdr_a->bits.bits_32_63.pkt_info.cpu_flg != 0)
		seg_len -= sizeof(HEADER_CPU_T);
	if (hdr_a->bits.hdr_type == HEADER_TYPE_PTP)
		seg_len -= sizeof(HEADER_PTP_T);

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: head_room=%d, seg_len=%d\n", __func__, head_room, seg_len);
	}

	return seg_len;
}

static void ca_ni_cpu256_rx_napi_read_epp(int cpu_port, u8 voq, struct napi_struct *napi, u8 **ret_rx_virt_addr, CPU_EPP_FIFO_CMD_T *ret_epp_fifo_cmd,
		HEADER_A_T *ret_hdr_a, HEADER_CPU_T *ret_hdr_cpu, HEADER_PTP_T *ret_hdr_ptp, u32 *hw_rd_ptr)
{
	ca_eth_private_t *cep = netdev_priv(napi->dev);
	struct platform_device *pdev = cep->pdev;
	int pkt_len, seg_len;
	//u32 hw_wr_ptr, hw_rd_ptr;
	//u32 *rx_start_addr;
	u8 *rx_virt_addr;
	u16 epp_desc_size;
	int head_room = l3qm_cpu256_head_room_first;
	//u8 eqid;

	dma_addr_t rx_dma_addr;
	//struct net_device *dev = napi->dev;

	CPU_EPP_FIFO_CMD_T epp_fifo_cmd;
	HEADER_A_T hdr_a;
	HEADER_CPU_T hdr_cpu;
	HEADER_PTP_T hdr_ptp;

	if (napi != NULL) {
		cep = netdev_priv(napi->dev);
	}
	else {
		cep = netdev_priv(ni_info_data.cpu256_dev[0]);
	}
	pdev = cep->pdev;

	epp_desc_size = l3qm_cpu256_desc_size * l3qm_cpu256_desc_per_epp;

	/* construct start pointer of CPU EPP FIFO command block */
	/* bit[31:7] - physical address */
	/* bit[6] - EOP */
	/* bit[5] - Layer 4 checksum error */
	/* bit[4] - SOP */
	/* bit[3-0] - EQID */

	rx_virt_addr = phys_to_virt(aal_l3qm_cpu256_get_rx_start_addr(voq));

	/* should do cache invalidate if EPP not locates at non-cache */
	if (aal_l3qm_get_ace_test() == 0) {
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: call dma_sync_single_for_cpu()  for CPU EPP FIFO!!!\n", __func__);
		}
		dma_sync_single_for_cpu(&(pdev->dev), virt_to_phys(rx_virt_addr + *hw_rd_ptr), sizeof(CPU_EPP_FIFO_CMD_T), DMA_FROM_DEVICE);
	}

	epp_fifo_cmd = *(CPU_EPP_FIFO_CMD_T *)(rx_virt_addr + *hw_rd_ptr);

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: phy_addr=0x%x, eop=%d, l4_csum_err=%d, sop=%d, eqid=%d\n", __func__,
				epp_fifo_cmd.bits.phy_addr << CA_L3QM_PHY_ADDR_SHIFT, epp_fifo_cmd.bits.eop,
				epp_fifo_cmd.bits.l4_csum_err, epp_fifo_cmd.bits.sop, epp_fifo_cmd.bits.eqid);
		printk("%s: *hw_rd_ptr=%d\n", __func__, *hw_rd_ptr);
	}

	rx_dma_addr = epp_fifo_cmd.bits.phy_addr << CA_L3QM_PHY_ADDR_SHIFT;
	rx_virt_addr = phys_to_virt(rx_dma_addr);

	if (epp_fifo_cmd.bits.sop == 1) {

		if (aal_l3qm_get_ace_test() == 0) {
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: call dma_sync_single_for_cpu() for HEADERs!!!\n", __func__);
			}
			dma_sync_single_for_cpu(&(pdev->dev), rx_dma_addr, head_room + sizeof(HEADER_A_T) + sizeof(HEADER_CPU_T) + sizeof(HEADER_PTP_T), DMA_FROM_DEVICE);
		}

		rx_virt_addr += head_room; /* skip the head room which is preallocated by software */
		hdr_a = (*(HEADER_A_T *)rx_virt_addr);
		hdr_a.bits32.bits32_h = be32_to_cpu(hdr_a.bits32.bits32_h);
		hdr_a.bits32.bits32_l = be32_to_cpu(hdr_a.bits32.bits32_l);

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: hdr_a.bits64=0x%llx\n", __func__, hdr_a.bits64);
			printk("%s: HEADER_A bits: ldpid=%d, deep_q=%d, lspid=%d, cpu_flg=%d, pkt_size=%d, rx_pkt_type=%d\n", __func__,
					hdr_a.bits.ldpid, hdr_a.bits.bits_32_63.pkt_info.deep_q, hdr_a.bits.lspid, hdr_a.bits.bits_32_63.pkt_info.cpu_flg,
					hdr_a.bits.pkt_size, hdr_a.bits.bits_32_63.pkt_info.rx_pkt_type);
		}

		pkt_len = hdr_a.bits.pkt_size;     /* get the packet size from HEADER_A, 14 bits=16K */

		seg_len = min_t(u16, CA_NI_SKB_PKT_LEN, pkt_len);

		if (aal_l3qm_get_ace_test() == 0) {
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: call dma_sync_single_for_cpu() for packet data!!!, rx_dma_addr=%p, seg_len=%d\n", __func__, (void *)rx_dma_addr, seg_len);
			}
			dma_sync_single_for_cpu(&(pdev->dev), rx_dma_addr, seg_len + head_room + sizeof(HEADER_A_T) + sizeof(HEADER_CPU_T) + sizeof(HEADER_PTP_T), DMA_FROM_DEVICE);
		}

		rx_virt_addr += sizeof(HEADER_A_T);

		/* the following if statement might not be satisfied in a rare
		 * case that when RE sends a packet marked with FE_BYPASS, CPU
		 * headers will not be inserted. */
		if (likely(hdr_a.bits.bits_32_63.pkt_info.cpu_flg != 0)) {
			hdr_cpu = (*(HEADER_CPU_T *)rx_virt_addr);
			hdr_cpu.mdata_raw.mdata_h = be32_to_cpu(hdr_cpu.mdata_raw.mdata_h);
			hdr_cpu.mdata_raw.mdata_l = be32_to_cpu(hdr_cpu.mdata_raw.mdata_l);
			rx_virt_addr += sizeof(HEADER_CPU_T);
		}

		if (hdr_a.bits.hdr_type == HEADER_TYPE_PTP) {
			hdr_ptp = (*(HEADER_PTP_T *)rx_virt_addr);
			rx_virt_addr += sizeof(HEADER_PTP_T);
		}

		*ret_hdr_a = hdr_a;
		*ret_hdr_cpu = hdr_cpu;
		*ret_hdr_ptp = hdr_ptp;
	}
	else {
		seg_len = ca_ni_rx_napi_get_first_segment_length(cpu_port, ret_hdr_a);
		pkt_len = ret_hdr_a->bits.pkt_size - 4 - seg_len;

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: hdr_a.bits.pkt_size=%d, rx_dma_addr=0x%llx, pkt_len=%d, seg_len=%d\n",
				__func__, ret_hdr_a->bits.pkt_size, (ca_uint64_t)rx_dma_addr, pkt_len, seg_len);
		}

		if (aal_l3qm_get_ace_test() == 0) {
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: call dma_sync_single_for_cpu() for packet data!!!\n", __func__);
			}
			dma_sync_single_for_cpu(&(pdev->dev), rx_dma_addr, pkt_len + head_room, DMA_FROM_DEVICE);
		}
	}

	*ret_epp_fifo_cmd = epp_fifo_cmd;
	memcpy(ret_epp_fifo_cmd, &epp_fifo_cmd, sizeof(CPU_EPP_FIFO_CMD_T));

	*ret_rx_virt_addr = (u8 *)rx_virt_addr;

	/* advance to next CPU EPP FIFO */
	*hw_rd_ptr += epp_desc_size;

	/* wrap around here */
	if (*hw_rd_ptr >= aal_l3qm_eq_get_cpu256_max_write_ptr()) {
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: read pointer wrap around at %d\n", __func__, *hw_rd_ptr);
		}

		*hw_rd_ptr = 0;
	}

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: epp_desc_size=%d, hw_rd_ptr=%d\n", __func__, epp_desc_size, *hw_rd_ptr);
	}

	aal_l3qm_cpu256_set_rx_read_ptr(voq, *hw_rd_ptr);
}

extern ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data);

static int ca_ni_cpu256_rx_napi(int cpu_port, u8 voq, struct napi_struct *napi, int budget)
{
	ca_eth_private_t *cep;
	struct platform_device *pdev;
	struct net_device_stats *stats;
	int received_pkts = 0;
	int jumbo_pkt_index = 0;
	int pkt_len, seg_len;
	u32 refill_cnt = 0;
        u32 refill_cnt_pool0 = 0;
        u32 refill_cnt_pool1 = 0;
	u16 count;
	u32 hw_wr_ptr, hw_rd_ptr;
	//u32 *rx_start_addr;
	u8 *rx_virt_addr;
	u16 epp_desc_size;
	u8 eqid;
	int header_size;
	int head_room;
	int buf_size;
	ca_mac_addr_t sa_mac;
	ca_mac_addr_t da_mac;
#if 0
	u16 vid;
#endif
	int ret;
	ca_uint16_t ethertype;
	ca_status_t rx_callback_ret = CA_E_ERROR;

	dma_addr_t rx_dma_addr;
	struct net_device *dev;
	struct sk_buff *skb, *tmp_skb = NULL, *tail_skb = NULL;
	ca_uint8_t *save_skb_data;
	ca_uint8_t *tmp_ptr;

	CPU_EPP_FIFO_CMD_T epp_fifo_cmd;
	HEADER_A_T hdr_a;
	HEADER_CPU_T hdr_cpu;
	HEADER_PTP_T hdr_ptp;
	u32 payload_len;

	int vlan_offset;

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: cpu_port=%d, voq=%d, budget=%d\n", __func__, cpu_port, voq, budget);
	}

#ifdef CONFIG_RTK_PERF_EVENTS
	start_perf_counter(0x1); // save to idx 0
#endif

	cep = netdev_priv(napi->dev);
	pdev = cep->pdev;
	stats = &cep->dev->stats;

	/* net_device will be assign later */
	dev = napi->dev;

	head_room = l3qm_cpu256_head_room_first;
	buf_size = l3qm_cpu256_buf_size;

	epp_desc_size = l3qm_cpu256_desc_size * l3qm_cpu256_desc_per_epp;

	/* get hardware write pointer */
	hw_wr_ptr = aal_l3qm_cpu256_get_rx_write_ptr(voq, &count);
	hw_rd_ptr = aal_l3qm_cpu256_get_rx_read_ptr(voq);

#ifdef CONFIG_RTK_PERF_EVENTS
	stop_perf_counter(0, 0x1);
#endif

	/* loop until all of packets available are recived or budget is reached */
	/* the unit of refill_cnt is number of CPU EPP command */
	/* the unit of budget is number of packet */
	while ((refill_cnt < count) && (received_pkts < budget)) {

#ifdef CONFIG_RTK_PERF_EVENTS
		start_perf_counter(0x1); // save to idx 2
#endif

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: hw_wr_ptr=%d, hw_rd_ptr=%d, refill_cnt=%d, count=%d, received_pkts=%d, budget=%d\n", __func__,
				hw_wr_ptr, hw_rd_ptr, refill_cnt, count, received_pkts, budget);
		}

		header_size = 0;

		ca_ni_cpu256_rx_napi_read_epp(cpu_port, voq, napi, &rx_virt_addr, &epp_fifo_cmd, &hdr_a, &hdr_cpu, &hdr_ptp, &hw_rd_ptr);

		/* get net_device pointer according to header A for WAN port bridge mode */
		//dev = ca_ni_rx_get_dev_by_hdr_a(&hdr_a, napi);

		/* the rx_virt_addr here has been skip head_room and headers */
		rx_dma_addr = epp_fifo_cmd.bits.phy_addr << CA_L3QM_PHY_ADDR_SHIFT;
		rx_virt_addr = phys_to_virt(rx_dma_addr);

		eqid = epp_fifo_cmd.bits.eqid;

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: rx_virt_addr=%p, sop=%d, eop=%d, pkt_len=%d\n", __func__, rx_virt_addr, epp_fifo_cmd.bits.sop, epp_fifo_cmd.bits.eop, hdr_a.bits.pkt_size);
		}

		if (epp_fifo_cmd.bits.sop != 1) {
			printk("%s: no SOP is given in packet!!\n", __func__);
			ca_ni_rx_get_bad_packet = 1;
#ifdef CONFIG_RTK_PERF_EVENTS
			stop_perf_counter(2, 0x1);
#endif
			return received_pkts;
		}

		/* use build_skb() here when there is no sk_buff stored in QM pool */
		if (ca_ni_cpu256_use_build_skb) {
			int tmp_len = CA_NI_SKB_PKT_LEN + head_room + 16;

			/* check whether this is a packet for this cpu port */
			//if (hdr_a.bits.pkt_size == 0 || (hdr_a.bits.ldpid != (AAL_LPORT_CPU_0 + cpu_port))) {
			if (hdr_a.bits.pkt_size == 0) {
				skb_free_frag(rx_virt_addr);
				//printk("%s: hdr_a.bits.pkt_size=%d OR hdr_a.bits.ldpid=%d not correct!!\n", __func__, hdr_a.bits.pkt_size, hdr_a.bits.ldpid);
				printk("%s: hdr_a.bits.pkt_size=%d is not correct!!\n", __func__, hdr_a.bits.pkt_size);
#ifdef CONFIG_RTK_PERF_EVENTS
				stop_perf_counter(2, 0x1);
#endif
				return 0;
			}
			tmp_len += NET_SKB_PAD + SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
			tmp_len = SKB_DATA_ALIGN(tmp_len);
			skb = build_skb(rx_virt_addr, tmp_len);
			if (skb == NULL) {
				skb_free_frag(rx_virt_addr);
				printk("%s: not able to allocate skb here!!\n", __func__);
#ifdef CONFIG_RTK_PERF_EVENTS
				stop_perf_counter(2, 0x1);
#endif
				return 0;
			}
			skb->dev = dev;

			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: build_skb skb=0x%p, rx_virt_addr=0x%p, skb->data=0x%p\n", __func__, skb, rx_virt_addr, skb->data);
			}
		}
		else {
			/* the sk_buff address stored at first 8 byte of packet buffer */
			skb = (struct sk_buff *)(*((ca_uint_t *)(rx_virt_addr)));
		}

		skb->data += head_room; /* skip head_room */
		header_size += head_room;

		pkt_len = hdr_a.bits.pkt_size;     /* get the packet size from HEADER_A, 14 bits=16K */

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: pkt_len=%d\n", __func__, pkt_len);
		}

		skb->data += sizeof(HEADER_A_T);
		header_size += sizeof(HEADER_A_T);

		/* the following if statement might not be satisfied in a rare
		 * case that when RE sends a packet marked with FE_BYPASS, CPU
		 * headers will not be inserted. */
		if (likely(hdr_a.bits.bits_32_63.pkt_info.cpu_flg != 0)) {
			skb->data += sizeof(HEADER_CPU_T);
			header_size += sizeof(HEADER_CPU_T);
			/* pkt_len does not include header size */
			pkt_len -= sizeof(HEADER_CPU_T);

			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: Header CPU exists, pkt_len=%d\n", __func__, pkt_len);
			}
		}

		if (hdr_a.bits.hdr_type == HEADER_TYPE_PTP) {
			skb->data += sizeof(HEADER_PTP_T);
			header_size += sizeof(HEADER_PTP_T);
		}

		/* keep the skb->data which point to the beginning of packet data */
		save_skb_data = skb->data;

		/* store sa_mac and da_mac for later use */
		memcpy(&da_mac, (skb->data), sizeof(ca_mac_addr_t));
		memcpy(&sa_mac, (skb->data+6), sizeof(ca_mac_addr_t));

		/* decrease pkt len by CRC length */
		//pkt_len -= 4;		/* the packet length of header A does not include CRC length */

		skb_reset_tail_pointer(skb);

		//skb->tail = (ca_uint_t)(skb->data);
		if (pkt_len > (buf_size - header_size)) {
			seg_len = buf_size - header_size;
		}
		else {
			seg_len = pkt_len;
		}

		skb_put(skb, seg_len);   /* skb->tail += seg_len, skb->len += seg_len */
		payload_len = pkt_len;
		skb->len = pkt_len;

		/* if not a jumbo packet, the pkt_len will become to 0 */
		pkt_len -= seg_len;

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: seg_len=%d, pkt_len=%d, buf_size=%d payload_len=%d\n",
				__func__, seg_len, pkt_len, buf_size, payload_len);
		}

		skb->data_len = pkt_len;   /* skb->data_len will be 0 or greater than 0 (jumbo packet) */
		jumbo_pkt_index = 0;

		refill_cnt++;

		/* record refill_cnt for EQ pool0/1 */
		if (cpu_port == 0) {
			if (eqid == l3qm_cpu256_lo_pool0_eq_id)
				refill_cnt_pool0++;

			if (eqid == l3qm_cpu256_lo_pool1_eq_id)
				refill_cnt_pool1++;
		}
		else {
			if (eqid == l3qm_cpu256_hi_pool0_eq_id)
				refill_cnt_pool0++;

			if (eqid == l3qm_cpu256_hi_pool1_eq_id)
				refill_cnt_pool1++;
		}

		/* consider jumbo frame */
		while (pkt_len > 0  && !epp_fifo_cmd.bits.eop) {

			ca_ni_cpu256_rx_napi_read_epp(cpu_port, voq, napi, &rx_virt_addr, &epp_fifo_cmd, &hdr_a, &hdr_cpu, &hdr_ptp, &hw_rd_ptr);

			rx_dma_addr = virt_to_phys(rx_virt_addr);

			/* use build_skb() here when there is no sk_buff stored in QM pool */
			if (ca_ni_cpu256_use_build_skb) {
				int tmp_len = CA_NI_SKB_PKT_LEN + head_room + 16;

				tmp_len += NET_SKB_PAD + SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
				tmp_len = SKB_DATA_ALIGN(tmp_len);
				tmp_skb = build_skb(rx_virt_addr, tmp_len);
				if (tmp_skb == NULL) {
					skb_free_frag(rx_virt_addr);
					printk("%s: not able to allocate skb here!!\n", __func__);
#ifdef CONFIG_RTK_PERF_EVENTS
					stop_perf_counter(2, 0x1);
#endif
					return 0;
				}
				tmp_skb->dev = dev;

				if (ca_ni_debug & NI_DBG_DUMP_RX) {
					printk("%s: build_skb skb=0x%p, rx_virt_addr=0x%p, tmp_skb->data=0x%p\n", __func__, skb, rx_virt_addr, tmp_skb->data);
				}
			}
			else {
				/* the sk_buff address stored at first 8 byte of packet buffer */
				tmp_skb = (struct sk_buff *)(*((ca_uint_t *)(rx_virt_addr)));
			}

			tmp_skb->data += head_room; 	/* skip head room rest same size as head room first */

			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: jumbo_pkt_index=%d, tmp_skb=%p, tmp_skb->data=%p, phy_data=0x%llx, tmp_skb->head=%p\n",
						__func__, jumbo_pkt_index, tmp_skb, tmp_skb->data, (ca_uint64_t)virt_to_phys(tmp_skb->data), tmp_skb->head);
			}

			/* the buffer size should minus head romm reset size */
			seg_len = min_t(u16, buf_size - head_room, pkt_len);

			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: seg_len=%d, buf_size=%d, pkt_len=%d\n", __func__, seg_len, buf_size, pkt_len);
			}

			/* 3.4.11 pass NULL to force to do arm dma map if QM ACP disabled */
			if (aal_l3qm_get_ace_test() == 0) {
				if (ca_ni_debug & NI_DBG_DUMP_RX) {
					printk("%s: call dma_sync_single_for_cpu() for packet fragment data!!!\n", __func__);
				}
				dma_sync_single_for_cpu(&(pdev->dev), rx_dma_addr, seg_len + head_room, DMA_FROM_DEVICE);
			}

			tmp_skb->len = 0;
			skb_put(tmp_skb, seg_len);   /* tmp_skb->len += seg_len, tmp_skb->tail += seg_len */
			tmp_skb->next = NULL;
			tmp_skb->prev = NULL;
			if (jumbo_pkt_index == 0) {
				skb_shinfo(skb)->frag_list = tmp_skb;
				tail_skb = tmp_skb;
			} else {
				tail_skb->next = tmp_skb;
				tail_skb = tmp_skb;
			}
			pkt_len -= seg_len;

			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s: after fragment pkt_len=%d\n", __func__, pkt_len);
			}

			jumbo_pkt_index++;
			refill_cnt++;

			/* record refill_cnt for EQ pool0/1 */
			if (cpu_port == 0) {
				if (eqid == l3qm_cpu256_lo_pool0_eq_id)
					refill_cnt_pool0++;

				if (eqid == l3qm_cpu256_lo_pool1_eq_id)
					refill_cnt_pool1++;
			}
			else {
				if (eqid == l3qm_cpu256_hi_pool0_eq_id)
					refill_cnt_pool0++;

				if (eqid == l3qm_cpu256_hi_pool1_eq_id)
					refill_cnt_pool1++;
			}

			/* end of packet */
			if (epp_fifo_cmd.bits.eop) {
				if (ca_ni_debug & NI_DBG_DUMP_RX) {
					printk("%s: end of fragment packet!!!\n", __func__);
				}

				break;
			}
		}

		/* end of skb list */
		skb->next = NULL;
		skb->prev = NULL;

		/* fill the structure of skb */
		if (dev == NULL) {
			if (hdr_a.bits.bits_32_63.pkt_info.cpu_flg != 0)
				skb->dev = get_dev_by_cpu_hdr(&hdr_a, ni_info_data.dev[0]);
			else
				skb->dev = ni_info_data.dev[0];
			dev = skb->dev;
		} else {
			skb->dev = dev;
		}

		skb->ip_summed = CHECKSUM_NONE;

		/* Default RX HW checksum enable */
		if (cep->rx_checksum == CA_ENABLE) {
			if (epp_fifo_cmd.bits.l4_csum_err == 0) {
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			} else {
				stats->rx_errors++;
			}
		}

#if 0
#ifdef CONFIG_NE_L2FP
		if (ca_l2fp_receive_pkt(cpu_port, voq, skb) == CA_E_OK) {
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				printk("%s cpu_port 0x%x, voq %d, skb %p to l2fp module\n", __func__, cpu_port, voq, skb);
			}
			ca_skb_out(skb);
#ifdef CONFIG_RTK_PERF_EVENTS
			stop_perf_counter(2, 0x1);
#endif
			goto skb_handled;
		}
#endif
#endif

		skb->protocol = eth_type_trans(skb, skb->dev);
		skb->pkt_type = PACKET_HOST;
		/* end of fill the sturcture of skb */

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			ca_ni_dump_rx_pkts(skb, &hdr_a, &hdr_cpu, &hdr_ptp);  /* for segmented packets */
		}
#if 0
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (*(skb->data - 2) == 0xff && *(skb->data - 1) == 0xf1) {
            ca_uint8_t pon_mode = 0;
            scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
            if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
                if ((PON_DBG_GPON_OMCI_RX & ca_pon_debug)) {
                    ca_buffer_dump("Rx OMCI <=", skb->data - ETH_HLEN, skb->len + ETH_HLEN);
                }
            } else if ((PON_DBG_EPON_OAM_RX & ca_pon_debug) ) {
                ca_buffer_dump("Rx OAM <=", skb->data - ETH_HLEN, skb->len + ETH_HLEN);
            }

        }while(0);
#endif

#endif

	/* G3Lite has no need to do the following codes */
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
#if 0
		/* call ca_l2_addr_add() to add a FDB entry */
		/* if hdr type = 2 (CPU bound for l2 learning then call l2 addr add */
		if (hdr_a.bits.hdr_type == 0x2) {

			vid = ca_ni_rx_get_vid(save_skb_data);
			ca_ni_l2_addr_add(vid, &sa_mac, hdr_a.bits.lspid, hdr_a.bits.bits_32_63.pkt_info.mcgid & 0x7);

			/* should check if UUC packet flooding to other ports in the same flooding domain */
			ca_ni_l2_flooding(vid, da_mac, skb, napi, &hdr_a);
		}
#endif

		/* call netlink to send packet to user space packet size should decrement by 4 (CRC) */
		rx_callback_ret = ca_ni_netlink_send_packet(cpu_port, &hdr_a, save_skb_data, payload_len, &hdr_cpu);

#if 0
		/* call back to the routines registered by kernel mode __ca_rx_register() */
		/* if user space has register this packet type use the return value from ca_ni_netlink_send_packet() */
		if (ca_ni_rx_register_callback(cpu_port, &hdr_a, save_skb_data, payload_len, &hdr_cpu) == CA_E_OK) {
			rx_callback_ret = CA_E_OK;
		}
#endif

		/* call netlink igmpsnooping reply if we got a IGMP or MLD control packet */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
		ethertype = ca_ni_parse_l2(save_skb_data, NULL, NULL, &tmp_ptr, NULL, false);
		vlan_offset = tmp_ptr - (save_skb_data + 12); /* offset from MAC_DA to EtherType within packet */

		if (ethertype == ntohs(ETH_P_IP)) {
			skb_set_network_header(skb, vlan_offset);
			skb_set_transport_header(skb, vlan_offset);
			ret = ip_mc_check_igmp(skb, NULL);
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				if (!ret) {
					printk("%s: It's a IPv4 IGMP packet\n", __func__);
				}
			}
		}
		else if (ethertype == ntohs(ETH_P_IPV6)) {
			skb_set_network_header(skb, vlan_offset);
			skb_set_transport_header(skb, vlan_offset);
			ret = ipv6_mc_check_mld(skb, NULL);
			if (ca_ni_debug & NI_DBG_DUMP_RX) {
				if (!ret) {
					printk("%s: It's a IPv6 IGMP packet\n", __func__);
				}
			}
		}
		if (!ret)
#endif
			ca_ni_netlink_igmpsnooping_reply(0, 1, save_skb_data, payload_len);
#endif

		/* modify the statistics count of stats */
		if (cep != NULL) {
			int extra_padding = 0;

			if (payload_len > (dev->mtu + ETH_HLEN + extra_padding)) {
				stats->rx_errors++;
				stats->rx_bytes += payload_len;

				if (ca_ni_debug & NI_DBG_DUMP_RX) {
					pr_err("%s:: RX [ldpid %d] pkt_len %d > %d\n", __func__, cep->port_cfg.rx_ldpid, payload_len, dev->mtu + ETH_HLEN + extra_padding);
				}
			} else {
				stats->rx_packets++;
				stats->rx_bytes += payload_len;
			}
		}

		ca_skb_out(skb);

#ifdef CONFIG_RTK_PERF_EVENTS
		stop_perf_counter(2, 0x1);
#endif

		/* if the incoming packet has been registered, then drop the packet */
		if (rx_callback_ret == CA_E_OK) {
			ca_dev_kfree_skb(skb);

			if (ca_ni_debug & NI_DBG_DUMP_RX) {

				printk("%s: packet is dropped!!\n", __func__);
			}
		}
		else {
			if ((dev->features) & NETIF_F_GRO)
				napi_gro_receive(&(cep->napi), skb);
			else
				netif_receive_skb(skb);
		}

#if 0
skb_handled:
#endif
		received_pkts++;
	}

#ifdef CONFIG_RTK_PERF_EVENTS
	start_perf_counter(0x1); // save to idx 5
#endif

	/* if CFG_ID_NE_USE_QM_EQ_REFILL_INT=1, refill will be done in interrupt service routine */
	if (ca_ni_use_qm_eq_refill_int == 0)  {

		if (refill_cnt_pool0) {
			if (cpu_port == 0)
				ca_ni_refill_cpu256_lo_eq_buf_pool(dev, cpu_port, l3qm_cpu256_lo_pool0_eq_id, refill_cnt_pool0);
			else
				ca_ni_refill_cpu256_hi_eq_buf_pool(dev, cpu_port, l3qm_cpu256_hi_pool0_eq_id, refill_cnt_pool0);
		}
		if (refill_cnt_pool1) {
			if (cpu_port == 0) 
				ca_ni_refill_cpu256_lo_eq_buf_pool(dev, cpu_port, l3qm_cpu256_lo_pool1_eq_id, refill_cnt_pool1);
			else
				ca_ni_refill_cpu256_hi_eq_buf_pool(dev, cpu_port, l3qm_cpu256_hi_pool1_eq_id, refill_cnt_pool1);
		}
	}

#ifdef CONFIG_RTK_PERF_EVENTS
	stop_perf_counter(5, 0x1);
#endif

	return received_pkts;
}

static int ca_ni_cpu256_rx_get_voq(u8 voq)
{
	u32 rx_status;

	rx_status = aal_l3qm_cpu256_get_rx_status(voq/32);

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: voq=%d, rx_status=0x%x\n", __func__, voq, rx_status);
	}

	if (rx_status & (1 << (voq % 32))) {
        if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: voq=%d, packet received\n", __func__, voq);
	}
		return 1;
	}

	return 0;
}

int ca_ni_cpu256_rx_poll(int cpu_port, struct napi_struct *napi, int budget)
{
	int received_pkts = 0;
	int total_received_pkts = 0;
	int voq;
	int rest_budget = budget;

	/* get starup config constants */
	ca_ni_rx_get_startup_config();

        if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: cpu_port=%d, budget=%d\n", __func__, cpu_port, budget);
	}

	if (cpu_port == 0) {
		for (voq = 127; voq >= 0; voq--) {
			if (ca_ni_cpu256_rx_get_voq((u8)voq)) {
				received_pkts = ca_ni_cpu256_rx_napi(cpu_port, voq, napi, rest_budget);
				rest_budget -= received_pkts;
				total_received_pkts += received_pkts;
			}
		}
	}
	else {
		for (voq = 255; voq >= 128; voq--) {
			if (ca_ni_cpu256_rx_get_voq((u8)voq)) {
				received_pkts = ca_ni_cpu256_rx_napi(cpu_port, voq, napi, rest_budget);
				rest_budget -= received_pkts;
				total_received_pkts += received_pkts;
			}
		}
	}

	if (total_received_pkts < budget) {

		napi_complete(napi);

		aal_l3qm_enable_cpu256_epp_interrupt(cpu_port, 1);

		return 0;
	}

	return total_received_pkts;
}

#endif

