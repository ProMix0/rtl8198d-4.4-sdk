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
 * ca_ni_tx.c: Cortina NI Tx handler
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_arp.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <asm/uaccess.h>
#include <linux/if_tunnel.h>
#include <linux/if_pppox.h>
#include <linux/of_irq.h>
#include <net/ip6_checksum.h>
#include <soc/cortina/cortina-soc.h>

#include "scfg.h"
#include "aal_common.h"
#include "aal_port.h"
#include "ca_ni.h"
#include "ca_aal.h"
#include "aal_ni.h"
#include "aal_l3qm.h"
#include "aal_fbm.h"

#include "rx.h"
#include "ca_mut.h"
#include "port.h"

#include "ca_ni.h"
#include "ca_ni_tx.h"
#include "ca_virt_ni.h"
#include "ca_ni_proc.h"
#include "ca_ni_dump.h"
#ifdef CONFIG_NE_L2FP
#include "ca_l2fp.h"
#endif

#include "ca-ne-rtk/ca_ext.h"

#include "ptp.h"

#define HDRA_CPU_PKT	0xc30001

/* spin lock flag for ca_ni_send_packet() */
spinlock_t	ca_ni_send_packet_lock;

u32 ca_qos_preference;

#include "aal_pon.h"

extern ca_uint32_t ca_pon_debug;

extern void  ca_buffer_dump( char *tp, ca_uint8_t *dp, ca_uint16_t len);
static ca_uint8_t puc_voq_mode = 0;

extern char ca_ni_tx_timer_enable;

static ca_uint8_t dma_lso_ace_test = 0;
static ca_uint8_t ca_tx_by_start_xmit = 0;

/* there are 12 VPs in DMA LSO */
static ca_uint8_t ca_ni_dma_lso_cpu_vp_num = CA_NI_DMA_LSO_CPU_VP_NUM;

ca_boolean ca_ni_rx_check_ptp(uint8_t *data, uint8_t *data_end, ca_boolean dump);

/* for DMA LSO VP 10 irq number */
int dma_lso_vp10_irq, dma_lso_vp11_irq, dma_lso_irq;

static ca_status_t ca_ni_tx_find_dma_aft_l2fib_index(
	ca_classifier_vlan_action_t top_vlan_action,
	ca_classifier_vlan_action_t inner_vlan_action,
	ca_pppoe_hdr_action_t       pppoe_hdr_action,
	int     *index);

#define CA_NI_CHIP_REVISION_C	1
#define CA_NI_CHIP_REVISION_D   4
static ca_uint32_t ca_ni_chip_revision = CA_NI_CHIP_REVISION_C;
static ca_uint32_t ca_ni_get_chip_revision(void)
{
	struct ca_soc_data	data;

	ca_soc_data_get(&data);
	printk("%s: data.chip_revision=%d\n", __func__, data.chip_revision);
	return data.chip_revision;
}

static void ca_ni_dma_lso_init_startup_config(void)
{
	ca_ni_chip_revision = ca_ni_get_chip_revision();
	printk("%s: ca_ni_chip_revision=%d\n", __func__, ca_ni_chip_revision);

	aal_scfg_read(CFG_ID_DMA_LSO_ACE_TEST, 1, &dma_lso_ace_test);
	printk("%s: aal_scfg_read(CFG_ID_DMA_LSO_ACE_TEST), dma_lso_ace_test=%d\n", __func__, dma_lso_ace_test);

	aal_scfg_read(CFG_ID_NE_CA_TX_BY_START_XMIT, 1, &ca_tx_by_start_xmit);
	printk("%s: aal_scfg_read(CFG_ID_NE_CA_TX_BY_START_XMIT), ca_tx_by_start_xmit=%d\n", __func__, ca_tx_by_start_xmit);

        aal_scfg_read(CFG_ID_PON_VOQ_MODE, 1, &puc_voq_mode);
        printk("%s: aal_scfg_read(CFG_ID_PON_VOQ_MODE), puc_voq_mode=%d\n", __func__, puc_voq_mode);

}

void cancel_tx_completion_timer(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	del_timer_sync(&cep->tx_completion_timer);
}

static inline void update_tx_completion_timer(int tx_qid, struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	set_bit(tx_qid, &cep->tx_completion_queues);
	mod_timer(&cep->tx_completion_timer, jiffies + msecs_to_jiffies(10));
}

/* in iperf TCP test if waste too much time to free buffers will cause performance drop */
#define TXQDMA_CLEAN_NUM        1
/* there is share resource operation in this function should call spin_lock_bh() before call this function */
static inline bool check_to_perform_tx_complete(int txq_index, struct net_device *dev)
{
	int ret;
	int port_id;
	ni_info_t *ni = &ni_info_data;
	dma_swtxq_t *swtxq;
	dma_rptr_t rptr_reg;
	ca_eth_private_t *cep = netdev_priv(dev);

	port_id = cep->egress_cpu_port_id;

	swtxq = &ni->swtxq[port_id][txq_index];

	spin_lock_bh(&swtxq->lock);

	rptr_reg.wrd = aal_ni_get_tx_read_ptr(swtxq->rptr_reg);

	if (((rptr_reg.bf.rptr + swtxq->total_desc_num - swtxq->finished_idx) & (swtxq->total_desc_num - 1)) >= TXQDMA_CLEAN_NUM)
		ret = true;
	else
		ret = false;

	spin_unlock_bh(&swtxq->lock);
	return ret;
}

/* there is share resource operation in this function should call spin_lock_bh() before call this function */
static inline int ca_dma_tx_complete_easy(int txq_index, struct net_device *dev)
{
	int port_id;
	ni_info_t *ni = &ni_info_data;
	dma_swtxq_t *swtxq;
	dma_rptr_t rptr_reg;
	unsigned int free_count = 0;
	ca_eth_private_t *cep = netdev_priv(dev);
	unsigned int curr_idx;

	/* get tx H/W completed descriptor virtual address */
	/* check tx status and accumulate tx statistics */

	/* get the DMA LSO VP */
	port_id = cep->egress_cpu_port_id;

	swtxq = &ni->swtxq[port_id][txq_index];

	spin_lock_bh(&swtxq->lock);
	swtxq->intr_cnt++;

	rptr_reg.wrd = aal_ni_get_tx_read_ptr(swtxq->rptr_reg);

	while (rptr_reg.bf.rptr != swtxq->finished_idx) {
		curr_idx = swtxq->finished_idx;
		if (swtxq->xmit_done_cb_fn[curr_idx]) {
			free_count++;

			swtxq->xmit_done_cb_fn[curr_idx](swtxq->xmit_done_cookie[curr_idx],
				swtxq->tx_skb[curr_idx]);
			if (swtxq->tx_skb[curr_idx]) {
				ca_skb_out(swtxq->tx_skb[curr_idx]);
				swtxq->tx_skb[curr_idx] = NULL;
			}
			swtxq->xmit_done_cb_fn[curr_idx] = NULL;
			swtxq->xmit_done_cookie[curr_idx] = NULL;
			swtxq->total_finished++;
		} else if (swtxq->tx_skb[curr_idx]) {
			free_count++;
			ca_dev_kfree_skb_any(swtxq->tx_skb[curr_idx]);

			swtxq->tx_skb[curr_idx] = NULL;
			swtxq->total_finished++;
		}

		swtxq->finished_idx = CA_NI_RWPTR_ADVANCE_ONE(swtxq->finished_idx, swtxq->total_desc_num);
	}

	spin_unlock_bh(&swtxq->lock);

	return free_count;
}

/* there is share resource operation in this function should call spin_lock_bh() before call this function */
static inline void ca_dma_tx_complete(int tx_qid, struct net_device *dev)
{
	int free_count = ca_dma_tx_complete_easy(tx_qid, dev);

	if (free_count > 0) {
#ifdef CONFIG_NET_SCH_MULTIQ
		if (__netif_subqueue_stopped(dev, tx_qid))
			netif_wake_subqueue(dev, tx_qid);
#else
		if (netif_queue_stopped(dev)) {
			printk(KERN_INFO "%s: Device queue is stopped!!\n", dev->name);
			netif_wake_queue(dev);
		}
#endif
	}
	return;
}

void ca_ni_tx_completion_timer_cb(unsigned long data)
{
	int tx_qid;
	int port_id;
	struct net_device *dev = (struct net_device *)data;
	ca_eth_private_t *cep = netdev_priv(dev);
        dma_swtxq_t *swtxq;
	ni_info_t *ni = &ni_info_data;

	for (tx_qid = 0; tx_qid < CA_NI_DMA_LSO_TXQ_NUM; ++tx_qid) {
		if (test_and_clear_bit(tx_qid, &cep->tx_completion_queues)) {
			port_id = cep->egress_cpu_port_id;
			swtxq = &ni->swtxq[port_id][tx_qid];
			ca_dma_tx_complete(tx_qid, dev);
		}
	}
}

static u32 ca_ni_get_lso_para0_ipv4(struct sk_buff *skb, u16 seg_size, dma_lsopara_0_t *lso_para0)
{
	struct iphdr *iph = ip_hdr(skb);
	struct udphdr *udph;

	if (iph == NULL)
		return 0;

	/* when iph is not NULL means this is a IP packet */
	/* enable IPv4 fragmentation and checksum calculation */
	lso_para0->bf.ipv4_en = 1;
	if (iph->protocol == IPPROTO_UDP) {

		/* check IP header more fragments bit, if it is set, do not set udp_en */
		/* should also check frag_off for fragmented packets (2nd and so on packets) */
		if ((ntohs(iph->frag_off) & 0x2000) || (ntohs(iph->frag_off) & 0x1fff)) {
			lso_para0->bf.segment_en = 0;
			if (ca_ni_debug & NI_DBG_LSO) {
				printk("%s: Got a UDP fragment packet, lso_para0->wrd=0x%x, ntohs(iph->frag_off)=0x%x!!\n",
					__func__, lso_para0->wrd, ntohs(iph->frag_off));
			}
			return 0;
		}

		/* if UDP checksum is 0, do not enable UDP for DMA LSO */
		udph = udp_hdr(skb);
		if (ca_ni_debug & NI_DBG_LSO) {
			printk("%s: skb->data=0x%p, udph=0x%p, UDP checksum=0x%x\n", __func__, skb->data, udph, udph->check);
			printk("%s: skb->head=0x%p, skb->transport_header=%d\n", __func__, skb->head, skb->transport_header);
		}
		if (udph->check != 0)
			lso_para0->bf.udp_en = 1;

	} else if (iph->protocol == IPPROTO_TCP) {
		lso_para0->bf.tcp_en = 1;
	}
	return lso_para0->wrd;
}

static u32 ca_ni_get_lso_para0_ipv6(struct sk_buff *skb, struct net_device *dev, int total_len, dma_lsopara_0_t *lso_para0)
{
	uint8_t nexthdr;
	uint8_t *payload;
	struct ipv6hdr *ipv6h = ipv6_hdr(skb);
	struct udphdr *udph;
	bool is_frag;

	/* if IPv6 packet contains trailer as padding, DMA LSO should not change payload_len field of IPv6 header */
	if (skb->len > (ETH_HLEN + sizeof(struct ipv6hdr) + ntohs(ipv6h->payload_len))) {
		if (ca_ni_debug & NI_DBG_LSO) {
			printk("%s: IPv6 packet contains trailer\n", __func__);
		}

		lso_para0->bf.lenfix_en = 1;
	}

	/* use ipv6 parse to get the last nexthdr */
	nexthdr = ca_ni_parse_ipv6((ca_uint8_t *)ipv6h, &payload, (ca_ni_debug & NI_DBG_LSO) ? true : false, &is_frag);

	if (ca_ni_debug & NI_DBG_LSO) {
		printk("%s: nexthdr=%d\n", __func__, nexthdr);
	}

	if (!is_frag) {
		if (nexthdr == NEXTHDR_TCP) {
			lso_para0->bf.ipv6_en = 1;
			lso_para0->bf.tcp_en = 1;
		} else if (nexthdr == NEXTHDR_UDP) {
			lso_para0->bf.ipv6_en = 1;

			udph = (struct udphdr *)payload;

			/* if UDP checksum is 0 do not enable UDP for DMA LSO */
			if (ca_ni_debug & NI_DBG_LSO) {
				printk("%s: UDP checksum=0x%x\n", __func__, udph->check);
			}
			if (udph->check != 0)
				lso_para0->bf.udp_en = 1;
		} else {
			if (ipv6h->nexthdr != NEXTHDR_ROUTING && ipv6h->nexthdr != NEXTHDR_FRAGMENT) {
				lso_para0->bf.ipv6_en = 1;
			}
		}
	}

	if (ca_ni_debug & NI_DBG_LSO) {
		printk("%s: ipv6_en=%d, tcp_en=%d, udp_en=%d\n", __func__,
				lso_para0->bf.ipv6_en, lso_para0->bf.tcp_en, lso_para0->bf.udp_en);
	}

	return lso_para0->wrd;
}

static u32 ca_ni_get_lso_para0(struct sk_buff *skb, struct net_device *dev, int total_len, dma_lsopara_0_t *lso_para0)
{
	ca_uint_t network_loc, mac_loc;
	u16 seg_size;
	struct iphdr *iph = ip_hdr(skb);
	struct tcphdr *th = tcp_hdr(skb);
	u32 mtu;
	uint16_t ethertype;
	unsigned char *cp = skb->data;

	/* only IPv4 and IPv6 should be segemented */
	ethertype = ntohs(ca_ni_parse_l2(cp, NULL, NULL, NULL, &cp, false));
	if (ca_ni_debug & NI_DBG_LSO) {
		printk("%s: ethertype=0x%x, (cp-skb->data)=%d\n", __func__, ethertype, (int)(cp-skb->data));
	}

	skb_reset_mac_header(skb);

	/* if there is vlan tag correct the network header poisiton */
	if ((cp - skb->data) > 14) {
		skb_set_network_header(skb, cp - skb->data);
		iph = ip_hdr(skb);
		th = tcp_hdr(skb);
	}

	if (ca_ni_debug & NI_DBG_LSO) {
		printk("%s: ethertype=0x%x\n", __func__, ethertype);
	}
	/* if PPPoE packets should also do hardware checksum */
	if (ethertype == ETH_P_PPP_SES) {
		skb_set_network_header(skb, (cp - skb->data) + 8);
		iph = ip_hdr(skb);
		th = tcp_hdr(skb);
		if (ca_ni_debug & NI_DBG_LSO) {
			printk("%s: Got PPP packet..., iph->version=%d\n", __func__, iph->version);
			printk("%s: iph->protocol=0x%x, IPPROTO_UDP=0x%x\n", __func__, iph->protocol, IPPROTO_UDP);
		}
		if (iph->version != 4 && iph->version != 6) {
			return 0;
		}
	}
	else {
		if (ethertype != ETH_P_IP && ethertype != ETH_P_IPV6) {
			return 0;
		}
	}

	network_loc = (ca_uint_t)skb_network_header(skb);
	mac_loc = (ca_uint_t)skb_mac_header(skb);

	/* Only deal with TCP mss now. Let TSO support PMTU. */
	if (iph && iph->protocol == IPPROTO_TCP && skb_is_gso(skb) && th &&
			skb_shinfo(skb)->gso_size + ((iph->ihl + th->doff) << 2) < dev->mtu)
		mtu = skb_shinfo(skb)->gso_size + ((iph->ihl + th->doff) << 2);
	else
		mtu = dev->mtu;

	if ((network_loc != 0) && ((network_loc - mac_loc) > 14))
		seg_size = mtu + network_loc - mac_loc;
	else
		seg_size = mtu + 14;

	if (ca_ni_debug & NI_DBG_LSO) {
		printk("%s: mtu=%d, seg_size=%d, network_loc=%llx, mac_loc=%llx\n", __func__,
			mtu, seg_size, (ca_uint64_t)network_loc, (ca_uint64_t)mac_loc);
	}

	lso_para0->bf.segment_size = seg_size;
	lso_para0->bf.segment_en = 1;

	if (iph == NULL)
		return lso_para0->wrd;

	if (iph->version == 4)  /* IPv4 */
		ca_ni_get_lso_para0_ipv4(skb, seg_size, lso_para0);
	else if (iph->version == 6) /* IPv6 */
		ca_ni_get_lso_para0_ipv6(skb, dev, total_len, lso_para0);
	else {
		/* bypass_en should always 0 */
                lso_para0->bf.bypass_en = 0;
        }

	if (lso_para0->bf.ipv6_en) {
		/* use 1510 instead of 1500 as segment size */
		if (lso_para0->bf.segment_size == (dev->mtu + 14)) {
			lso_para0->bf.segment_size -= 4;
		}
	}

	/* if total_len < 64, lenfix_en=1: IP total length will not be changed by Hardware */
	if (total_len < 64)
		lso_para0->bf.lenfix_en = 1;

	return lso_para0->wrd;
}

static struct net_device *ca_ni_get_cpu_port_dev(ca_uint16_t lspid)
{
	struct net_device *dev = NULL;

	//do not enable CPU256
	if ((lspid >= AAL_LPORT_CPU_0) || (lspid <= AAL_LPORT_CPU_7))
		dev = ni_info_data.dev[lspid - AAL_LPORT_CPU_0];

	/* if the netdevice for lspid has not been created yet, use netdevice 0 */
	if (dev == NULL) {
		dev = ni_info_data.dev[0];
	}
	return dev;
}

static struct net_device *ca_ni_get_cpu_port_by_lspid(ca_uint16_t lspid)
{
	ca_uint8_t cpu_port;

	//do not enable CPU256
	if (lspid >= 0x20 && lspid <= 0x3f)
		cpu_port = (lspid % 6) + 2; //use the resources the same as port 0x12~0x17
	else
		cpu_port = lspid - AAL_LPORT_CPU_0;

	return cpu_port;
}

void ca_ni_dma_lso_dump(void) {
	dma_txdesc_t *desc_base, *curr_desc, *phy_curr_desc;
	dma_swtxq_t *swtxq;
	ni_info_t *ni = &ni_info_data;
	int port_id;
	int txq_index;
	char *pkt_datap;
	u32 dma_lso_reg;
	u32 data;
	u32 rptr, wptr;

	for (port_id = 0; port_id < ca_ni_dma_lso_cpu_vp_num; port_id ++)
		for (txq_index = 0; txq_index < CA_NI_DMA_LSO_TXQ_NUM; txq_index++) {
				swtxq = &ni->swtxq[port_id][txq_index];
				dma_lso_reg = DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + port_id * 0xa0;
				aal_ni_set_tx_write_ptr(dma_lso_reg, 0x80000000 + txq_index);

				phy_curr_desc = (dma_txdesc_t *)(ca_uint_t)aal_ni_get_tx_read_ptr(dma_lso_reg + 8);
				data = (u32) ((ca_uint_t)phy_curr_desc & 0xF);
				phy_curr_desc = (dma_txdesc_t *) ((ca_uint_t)phy_curr_desc & 0xfffffff0);

				printk("== port_id %d, txq_index %d desc_base 0x%p rptr[%x] %d wptr[%x] %d total_desc_num %d wptr %d finished_idx %d \n",
					port_id, txq_index, phy_curr_desc, swtxq->rptr_reg,
					aal_ni_get_tx_read_ptr(swtxq->rptr_reg), swtxq->wptr_reg,
					aal_ni_get_tx_read_ptr(swtxq->wptr_reg), swtxq->total_desc_num,
					swtxq->wptr,
					swtxq->finished_idx);
				if (aal_ni_get_tx_read_ptr(swtxq->rptr_reg) !=
					aal_ni_get_tx_read_ptr(swtxq->wptr_reg)) {
					curr_desc = swtxq->desc_base + aal_ni_get_tx_read_ptr(swtxq->rptr_reg);
					printk("curr_desc 0x%p \n", (void *)virt_to_phys(curr_desc));
					printk("curr_desc word0 0x%08x word1 0x%08x \n", curr_desc->word0.wrd, curr_desc->word1.wrd);
					printk("buffer content len %d \n", curr_desc->word1.bf.buf_len);
					if (curr_desc->word1.bf.buf_len != 0) {
						pkt_datap = phys_to_virt(curr_desc->word0.wrd);
						print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1,
							pkt_datap, curr_desc->word1.bf.buf_len, true);
					}

					if (aal_ni_get_tx_read_ptr(swtxq->rptr_reg) != 0)
						curr_desc = swtxq->desc_base + aal_ni_get_tx_read_ptr(swtxq->rptr_reg) -1;
					else
						curr_desc = swtxq->desc_base + swtxq->total_desc_num - 1;

					printk("pre_desc 0x%p \n", (void *)virt_to_phys(curr_desc));
					printk("pre_desc word0 0x%08x word1 0x%08x \n", curr_desc->word0.wrd, curr_desc->word1.wrd);
					printk("pre buffer content len %d \n", curr_desc->word1.bf.buf_len);
					if (curr_desc->word1.bf.buf_len != 0) {
						pkt_datap = phys_to_virt(curr_desc->word0.wrd);
						print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1,
							pkt_datap, curr_desc->word1.bf.buf_len, true);
					}
				}
		}

	for (port_id = ca_ni_dma_lso_cpu_vp_num; port_id < CA_NI_DMA_LSO_CPU_VP_NUM; port_id ++){
		printk("== %s port_id %d DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS %08x\n",
			__func__, port_id, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS);
		dma_lso_reg = DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + port_id * 0xa0;
		printk(" dma_lso_reg %08x\n", dma_lso_reg);

		for (txq_index = 0; txq_index < CA_NI_DMA_LSO_TXQ_NUM; txq_index++) {
			aal_ni_set_tx_write_ptr(dma_lso_reg, 0x80000000 + txq_index);
			phy_curr_desc = (dma_txdesc_t *)(ca_uint_t)aal_ni_get_tx_read_ptr(dma_lso_reg + 8);
			data = (u32)((ca_uint_t)phy_curr_desc & 0xF);
			phy_curr_desc = (dma_txdesc_t *) ((ca_uint_t)phy_curr_desc & 0xfffffff0);
			desc_base = phys_to_virt((ca_uint_t)phy_curr_desc);
			if (desc_base != 0) {
				data = 1 << data;
				rptr = aal_ni_get_tx_read_ptr(dma_lso_reg + 0x18 + txq_index * 8);
				wptr = aal_ni_get_tx_read_ptr(dma_lso_reg + 0x14 + txq_index * 8);
				printk("== port_id %d, txq_index %d desc_base 0x%p rptr[%x] %d wptr[%x] %d total_desc_num %d\n",
					port_id, txq_index, phy_curr_desc, dma_lso_reg + 0x18 + txq_index * 8,
					rptr,
					dma_lso_reg + 0x14 + txq_index * 8,
					wptr,
					data);
				if (rptr != wptr) {
					curr_desc = desc_base + rptr;
					printk("curr_desc 0x%p \n", (void *)virt_to_phys(curr_desc));
					printk("curr_desc word0 0x%08x word1 0x%08x \n", curr_desc->word0.wrd, curr_desc->word1.wrd);
					printk("buffer content len %d \n", curr_desc->word1.bf.buf_len);
					if (curr_desc->word1.bf.buf_len != 0) {
						pkt_datap = phys_to_virt(curr_desc->word0.wrd);
						print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1,
							pkt_datap, curr_desc->word1.bf.buf_len, true);
					}

					if (rptr != 0)
						curr_desc = desc_base + rptr - 1;
					else
						curr_desc = desc_base + data - 1;

					printk("pre_desc 0x%p \n", (void *)virt_to_phys(curr_desc));
					printk("pre_desc word0 0x%08x word1 0x%08x \n", curr_desc->word0.wrd, curr_desc->word1.wrd);
					printk("pre buffer content len %d \n", curr_desc->word1.bf.buf_len);
					if (curr_desc->word1.bf.buf_len != 0) {
						pkt_datap = phys_to_virt(curr_desc->word0.wrd);
						print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1,
							pkt_datap, curr_desc->word1.bf.buf_len, true);
					}
				}
			}
		}
	}

	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_FDES_ADDR1));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_CDES_ADDR1));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_TX_DES_WORD1));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_LSO_PARA_WORD1));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_DEBUG0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_DEBUG0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_DEBUG0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_DEBUG1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_DEBUG1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_DEBUG1));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_DEBUG2 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_DEBUG2, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_DEBUG2));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_SPARE0 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_SPARE0, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_SPARE0));
	printk("(%08x)DMA_SEC_DMA_GLB_DMA_LSO_SPARE1 \t %08x \n", DMA_SEC_DMA_GLB_DMA_LSO_SPARE1, aal_ni_get_tx_read_ptr(DMA_SEC_DMA_GLB_DMA_LSO_SPARE1));
}

/*
 * Do a one's complement checksum.
 * The data being checksum'd is in network byte order.
 * The returned checksum is in network byte order.
 */
static unsigned short ca_ni_calc_chksum(unsigned short *w, int len, int init_sum)
{
    int sum = init_sum;

    union {
        volatile unsigned char c[2];
        volatile unsigned short s;
    } su;

    union {
        volatile unsigned short s[2];
        volatile int i;
    } iu;

    while ((len -= 2) >= 0)
        sum += *w++;

    if (len == -1) {
        su.c[0] = *(char *)w;
        su.c[1] = 0;
        sum += su.s;
    }

    iu.i = sum;
    sum = iu.s[0] + iu.s[1];
    if (sum > 65535)
        sum -= 65535;

    su.s = ~sum;

    return (su.c[0] << 8) | su.c[1];
}

static void ca_ni_calc_udp_frags_chksum(struct sk_buff *skb)
{
	ca_uint8_t *cp;
	int len;
	__wsum csum = 0;
	ca_uint16_t ethertype;
	struct udphdr *udph;

	/* sanity check for IPv6 UDP packets */
	cp = skb->data;
	ethertype = ca_ni_parse_l2(cp, NULL, NULL, NULL, &cp, false);
	if (ntohs(ethertype) == ETH_P_IPV6) {
		ca_uint8_t ipv6_nexthdr;
		struct in6_addr *saddr, *daddr;
		struct ipv6hdr *iph6;

		ipv6_nexthdr = ca_ni_parse_ipv6(cp, &cp, false, NULL);
		if (ipv6_nexthdr != IPPROTO_UDP) {
			return;
		}

		iph6 = ipv6_hdr(skb);
		saddr = &(iph6->saddr);
		daddr = &(iph6->daddr);

		udph = udp_hdr(skb);
		len = ntohs(udph->len);

		udph->check = 0;
		csum = skb_checksum(skb, skb_headlen(skb) - sizeof(struct udphdr), len, 0);
		udph->check = udp_v6_check(len, saddr, daddr, csum);
	}
	else if (ntohs(ethertype) == ETH_P_IP) {
		int offset = skb_transport_offset(skb);
		int hlen = skb->len - offset;
		struct sk_buff *frags;
		struct iphdr *iph;
		__be32	saddr;
		__be32	daddr;

		iph = ip_hdr(skb);

		if (iph->protocol != IPPROTO_UDP) {
			return;
		}

		saddr = iph->saddr;
		daddr = iph->daddr;

		udph = udp_hdr(skb);
		len = ntohs(udph->len);
		
		skb_walk_frags(skb, frags) {
			csum = csum_add(csum, frags->csum);
			hlen -= frags->len;
		}

		udph->check = 0;
		csum = skb_checksum(skb, offset, hlen, 0);
		udph->check = udp_v4_check(len, saddr, daddr, csum);
	}
}


static void ca_ni_fix_gre_checksum(struct sk_buff *skb)
{
	const struct iphdr *iph = ip_hdr(skb);
	u8 *xprth = skb_network_header(skb) + iph->ihl * 4;
	__be16 *greflags;
	__be32 *gre_hdr;
	unsigned short chksum;
	unsigned short *gre_start;

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: iph->protocol=%d, IPPROTO_GRE=%d, ip_is_fragment(iph)=%d\n", __func__,
				iph->protocol, IPPROTO_GRE, ip_is_fragment(iph));
		printk("%s: skb_network_header(skb)=%p, iph->ihl=%d\n", __func__, skb_network_header(skb), iph->ihl);
	}

        if (iph->protocol != IPPROTO_GRE || ip_is_fragment(iph))
                return;

        if (xprth + 12 < skb->data || pskb_may_pull(skb, xprth + 12 - skb->data)) {

                xprth = skb_network_header(skb) + iph->ihl * 4;
                greflags = (__be16 *)xprth;
                gre_hdr = (__be32 *)xprth;

		if (greflags[0] & GRE_CSUM) {

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s: GRE checksum is enabled...\n", __func__);
			}

			gre_start = greflags;
			greflags++;  /* skip flags field */
			greflags++;  /* skip IP protocol field */
			*greflags = 0;  /* clear check sum field */
			chksum = ca_ni_calc_chksum(gre_start, ntohs(iph->tot_len) - sizeof(struct iphdr), 0);
			*greflags = htons(chksum);

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s: iph->tot_len=%d, sizeof(struct iphdr)=%ld\n", __func__, ntohs(iph->tot_len), sizeof(struct iphdr));
				printk("%s: chksum=0x%x\n", __func__, chksum);
			}
		}
	}
}

#if 0
static void ca_ni_tx_remove_ptp_cmd(char *data, HEADER_A_T *hdr_a, int total_len)
{
        ca_ptp_cmd_t *ptp_cmd;

        ptp_cmd = (ca_ptp_cmd_t *)(data + total_len);

        if (ca_ni_debug & NI_DBG_DUMP_TX) {
                printk("%s: ptp_tx_type=%d, udp_checksum_offset=%d, modified_field_offset=%d, tx_cmd=%d, correction_select=%d\n",
                                __func__, ptp_cmd->ptp_tx_type, ptp_cmd->udp_checksum_offset, ptp_cmd->modified_field_offset,
                                ptp_cmd->tx_cmd, ptp_cmd->correction_select);
        }
        hdr_a->bits.bits_32_63.ptp_cmd.ptp_type = ptp_cmd->ptp_tx_type;
        hdr_a->bits.bits_32_63.ptp_cmd.udp_cksum_offset = ptp_cmd->udp_checksum_offset;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_offset = ptp_cmd->modified_field_offset;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode = ptp_cmd->tx_cmd;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_op = ptp_cmd->correction_select;
        hdr_a->bits.hdr_type = HEADER_TYPE_PTP;
}
#endif

static void ca_ni_tx_set_hdr_a_ptp_cmd(HEADER_A_T *hdr_a, ca_ptp_cmd_t *ptp_cmd)
{
        if (ca_ni_debug & NI_DBG_DUMP_TX) {
                printk("%s: ptp_tx_type=%d, udp_checksum_offset=%d, modified_field_offset=%d, tx_cmd=%d, correction_select=%d\n",
                                __func__, ptp_cmd->ptp_tx_type, ptp_cmd->udp_checksum_offset, ptp_cmd->modified_field_offset,
                                ptp_cmd->tx_cmd, ptp_cmd->correction_select);
        }
        hdr_a->bits.bits_32_63.ptp_cmd.ptp_type = ptp_cmd->ptp_tx_type;
        hdr_a->bits.bits_32_63.ptp_cmd.udp_cksum_offset = ptp_cmd->udp_checksum_offset;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_offset = ptp_cmd->modified_field_offset;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode = ptp_cmd->tx_cmd;
        hdr_a->bits.bits_32_63.ptp_cmd.cmd_op = ptp_cmd->correction_select;

	/* the cpu flag should set to 1 to avoid data error */
        hdr_a->bits.bits_32_63.ptp_cmd.cpu_flg = 1;

        hdr_a->bits.hdr_type = HEADER_TYPE_PTP;
}

netdev_tx_t __ca_ni_start_xmit(struct sk_buff *skb, struct net_device *dev,
	ca_ni_tx_config_t *tx_config)
{
	char *pkt_datap;
	int cpu_id;
	int port_id;
	int snd_pages;
	int frag_id, free_desc, total_len, len;
	int go_9th_queue = tx_config->go_9th_queue;
	ca_eth_private_t *cep;
	struct net_device_stats *stats;
	ni_info_t *ni = &ni_info_data;
	dma_swtxq_t *swtxq;
	dma_txdesc_t *curr_desc;
	dma_txbuf_t	*tx_buf;
	dma_txdesc_0_t	tx_desc0;
	dma_txdesc_1_t	tx_desc1;
	dma_lsopara_0_t lso_para0;
	dma_lsopara_1_t lso_para1;
	HEADER_A_T	ni_header_a;
	struct platform_device *pdev;
	int txq_index = tx_config->core_config.bf.txq_index;
	int ldpid = tx_config->core_config.bf.ldpid;
	int lspid = tx_config->core_config.bf.lspid;
	int is_from_ca_tx = tx_config->core_config.bf.is_from_ca_tx;
	ca_uint8_t bypass_fwd_engine = tx_config->core_config.bf.bypass_fwd_engine;
	ca_uint8_t hdr_cpu = 0;
	ca_uint8_t bypass_lso = tx_config->bypass_lso;
	ca_uint8_t is_ptp = 0;
	ca_ptp_cmd_t ptp_tx_cmd = tx_config->ptp_tx_cmd;
	ca_pkt_type_t pkt_type = tx_config->pkt_type;
	ca_uint8_t dma_aft_l2fib_enable = tx_config->dma_aft_l2fib_enable;
	ca_uint8_t dma_aft_l2fib_index = tx_config->dma_aft_l2fib_index;

	/* default has no header CPU included but if sw_id is not 0 then header CPU should be included */
	if (tx_config->core_config.bf.sw_id != 0)
		hdr_cpu = 1;

	if (dev == NULL) {
		dev = ca_ni_get_cpu_port_dev(tx_config->core_config.bf.lspid);
		if (dev == NULL)
			return NETDEV_TX_BUSY;
	}
	cep = netdev_priv(dev);
	stats = &cep->dev->stats;
	pdev = cep->pdev;
	/* if device not up and runing do not transmit out */
	if (!(dev->flags & IFF_UP)) {
		pr_err("device %s is not up!!\n", dev->name);
		return NETDEV_TX_BUSY;
	}

	if (skb == NULL) {
		pr_err("%s: Why skb is NULL?\n", dev->name);
		return NETDEV_TX_OK;
	}

	snd_pages = skb_shinfo(skb)->nr_frags + 1;

	/* if cos=8 comes from ca_tx() we need to adjust it to txq_index=7 to avoid get wrong txq */
	if (txq_index == 8) {
		txq_index = 7;
		if (is_from_ca_tx) {
			go_9th_queue = 1;
		}
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		ca_ni_dump_tx_pkts(skb);
		printk("%s: txq_index=%d, ldpid=%d, is_from_ca_tx=%d, dma_lso_ace_test=%d\n",
			__func__, txq_index, ldpid, is_from_ca_tx, dma_lso_ace_test);
	}

	/* use XPS method to decide which CPU take care which TX queue */
	cpu_id = smp_processor_id();

	/* if called from ca_tx() get port_id from lspid */
	if (is_from_ca_tx) {
		port_id = ca_ni_get_cpu_port_by_lspid(lspid);
	}
	else {
		port_id = cep->egress_cpu_port_id;
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: port_id=%d\n", __func__, port_id);
	}

	/* use cpu port to determined which DMA LSO we should use */
	/* because DMA LSO id will be used as lspid for L2FE */
	swtxq = &ni->swtxq[port_id][txq_index];

	/* use CFG_ID_NE_TX_TIMER_ENABLE flag in scfg.txt to decide whether to have a timer */
	if (ca_ni_tx_timer_enable) {
		cancel_tx_completion_timer(cep->dev);
		if (check_to_perform_tx_complete(txq_index, cep->dev))
			ca_dma_tx_complete(txq_index, cep->dev);
		else
			update_tx_completion_timer(txq_index, cep->dev);
	}
	else
		ca_dma_tx_complete(txq_index, cep->dev);

	/* use spin lock bottom half here to avoid software irq intercept here */
	spin_lock_bh(&swtxq->lock);

	/// check if TX buffer/queue is full, return NETDEV_TX_BUSY
	if (swtxq->wptr >= swtxq->finished_idx)
		free_desc = swtxq->total_desc_num - swtxq->wptr - 1 + swtxq->finished_idx;
	else
		free_desc = swtxq->finished_idx - swtxq->wptr - 1;

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: free_desc=%d, snd_pages=%d\n", __func__, free_desc, snd_pages);
	}

	/* try to reserve 1 descriptor in case skb is extended in xmit function */
	if (free_desc <= snd_pages) {
		spin_unlock_bh(&swtxq->lock);
		return NETDEV_TX_BUSY;
	}

	total_len = skb->len;

	/* In order to support packet size=30 PPP packets even packet size is smaller than CA_NI_MIN_DMA_SIZE=34 */
	/* we need to do software padded here */
	if (total_len < CA_NI_MIN_DMA_SIZE) {
		total_len = CA_NI_MIN_DMA_SIZE;
	}

	/* while loop to send out all of the pages */
	frag_id = 0;

	/* DMA LSO does not support GRE checksum, calculate by sw here */
        ca_ni_fix_gre_checksum(skb);

	if (is_from_ca_tx) {
		/* check pkt_type CA_PKT_TYPE_L2_PTP and CA_PKT_TYPE_L4_PTP */
		if (pkt_type == CA_PKT_TYPE_L2_PTP || pkt_type == CA_PKT_TYPE_L4_PTP) {
			is_ptp = 1;
		}
	}
	else {
		/* get the ca_ptp_cmd_t from end of packet, for non-fragment packets */
		if (ca_ni_rx_check_ptp(skb->data, skb->data + ni_header_a.bits.pkt_size, (ca_ni_debug & NI_DBG_DUMP_TX) ? 1:0)) {
			is_ptp = 1;
			total_len -= sizeof(ca_ptp_cmd_t);
		}
	}

	/* do software IPV6 UDP checksum if packet exceed MTU */
	if (snd_pages > 1) {
		ca_ni_calc_udp_frags_chksum(skb);
	}

	while (snd_pages != 0) {

		tx_desc0.wrd = tx_desc1.wrd = lso_para0.wrd = lso_para1.wrd = 0;
		ni_header_a.bits64 = 0;

		curr_desc = swtxq->desc_base + swtxq->wptr;
		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("%s: frag_id=%d\n", __func__, frag_id);
			printk("%s:   curr_desc=%p, swtxq->desc_base=%p, swtxq->wptr=%d\n", __func__, curr_desc, swtxq->desc_base, swtxq->wptr);
			printk("%s:   physical curr_desc=%p, swtxq->desc_base=%p\n", __func__,
				(void *)virt_to_phys(curr_desc), (void *)virt_to_phys(swtxq->desc_base));
		}
		/* the first descriptor should not include packet data */
		/* it includes only LSO Para[63:0] and Header A */
		if (frag_id == 0) {
			len = sizeof(dma_txbuf_t);   				/* there is no data in the first descriptor */

			tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_SOF;

			tx_desc1.bf.hp1 = 1;    /* HP[1:0]=10-header A and header CPU both exist */
			tx_desc1.bf.hp0 = 0;

			/* if PTP packet always setup header CPU */
			if (is_ptp)
				hdr_cpu = 1;

			if (hdr_cpu == 0) {
				tx_desc1.bf.hp1 = 0;    /* HP[1:0]=01-header A exists and header CPU does not exist  */
				tx_desc1.bf.hp0 = 1;
				len -= sizeof(HEADER_CPU_T);
			}

			pkt_datap = (char *)(swtxq->buf_base + swtxq->wptr);

			/* assign dma address to TX desc */
			tx_desc0.wrd = swtxq->buf_dma_addr + (swtxq->wptr * sizeof(dma_txbuf_t));
			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:   frag_id==0, tx_desc1.wrd=0x%x\n", __func__, tx_desc1.wrd);
			}

			if (!bypass_lso) {
				if (cep->use_default_lso_para0 == 1)
					lso_para0.wrd = cep->default_lso_para0;
				else
					ca_ni_get_lso_para0(skb, dev, total_len, &lso_para0);
			}
			else {
				/* just set LSO para0 to 0 if TX configure "bypass_lso" is set */
				lso_para0.wrd = 0;
			}

		} else {
			/* the first descriptor does not carry data buffer */
			//skb_frag_t *frag = &skb_shinfo(skb)->frags[frag_id - 2];

			if (dma_lso_ace_test) {
				tx_desc1.bf.buf_addr_39_32 = 2;
				if (ca_ni_debug & NI_DBG_DUMP_TX) {
					printk("%s: tx_desc1.wrd=0x%x\n", __func__, tx_desc1.wrd);
				}
			}

			/* the 2nd descriptor point to the begin of packet data */
			if (frag_id == 1) {

				/* no need to do virtual to phys convert because will get the physical addres by dma_map_single() */
				pkt_datap = (char *)(uintptr_t) skb->data;
				len = total_len - skb->data_len;

				if (ca_ni_debug & NI_DBG_DUMP_TX) {
					printk("%s:   total_len=%d, skb->data_len=%d\n", __func__, total_len, skb->data_len);
				}
				if (dma_lso_ace_test)
					tx_desc0.wrd = virt_to_phys(pkt_datap);
				else
					tx_desc0.wrd = dma_map_single(&(pdev->dev), pkt_datap, len, DMA_TO_DEVICE);
			} else {
				skb_frag_t *frag = &skb_shinfo(skb)->frags[frag_id - 2];
				pkt_datap = (char *)(page_address(frag->page.p) + frag->page_offset);
				len = frag->size;

				if (dma_lso_ace_test)
					tx_desc0.wrd = virt_to_phys(pkt_datap);
				else
					tx_desc0.wrd = dma_map_single(&(pdev->dev), pkt_datap, len, DMA_TO_DEVICE);

				if (ca_ni_debug & NI_DBG_DUMP_TX) {
					printk("%s:   frag_id=%d, len=%d, pkt_datap=0x%p\n", __func__, frag_id, len, pkt_datap);
					printk("%s: skb=0x%p, frag=0x%p\n", __func__, skb, frag);
				}
			}
			if (len > total_len) {
				/* TODO: when this happened should we jump out of while loop and return??? */
				printk(KERN_INFO "%s: Fatal Error! Send Frag size %d > Total Size %d!\n",
					__func__, len, total_len);
				printk(KERN_INFO "\t cep->dma_l3fe %d cep %p dev %s\n" , cep->dma_l3fe, cep, dev->name);
			}

			/* if buffer is allocated from FBM, then tell DMA LSO to recycle this buffer to FBM */
		} /* if (frag_id == 0) */

		ni_header_a.bits.bits_32_63.pkt_info.deep_q = cep->deep_q;

		/* test DMA_AFT, drop_code[2:0], bit 2=1 to enable AFT */
		if (dma_aft_l2fib_enable) {
			ni_header_a.bits.bits_32_63.pkt_info.drop_code =
				(dma_aft_l2fib_enable << 2) | ((dma_aft_l2fib_index & 0x8) >> 3);
			ni_header_a.bits.bits_32_63.pkt_info.pol_grp_id = dma_aft_l2fib_index & 0x7;
		}

		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("%s:   pkt_datap=%p, len=%d dma_l3fe=%d cep %p\n", __func__, pkt_datap, len, cep->dma_l3fe, cep);
		}
		tx_desc1.bf.buf_len = len;  /* transfer buffer size locates at bit [23:8] */

		skb->ip_summed = CHECKSUM_UNNECESSARY;

		/* snd_pages==1 means no fragment data */
		if ((snd_pages == 1) && (frag_id != 0)) {
			tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_EOF; /* EOF */
			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:  xmit_done_cb_fn=%p, cb_cookie=%p\n", __func__,
					tx_config->xmit_done_cb_fn, tx_config->cb_cookie);
			}

			swtxq->tx_skb[swtxq->wptr] = skb;
			if (tx_config->core_config.bf.cb_fn_mask) {
				swtxq->xmit_done_cb_fn[swtxq->wptr] = tx_config->xmit_done_cb_fn;
				swtxq->xmit_done_cookie[swtxq->wptr] = tx_config->cb_cookie;
			}
		} else {
			swtxq->tx_skb[swtxq->wptr] = NULL;
		}

		/* should move out of the while loop */
		if (frag_id != 0) {  /* exclude frag_id = 0, which is sizeof(dma_txbuf_t) */
			stats->tx_bytes += len;
		}

		/* if the first descriptor, fill the LSO Para[63:0] and HEADER_A */
		if (frag_id == 0) {
			tx_buf = (dma_txbuf_t *)(swtxq->buf_base + swtxq->wptr);

			tx_buf->lso_word0.wrd = lso_para0.wrd;

			lso_para1.bf.packet_size = total_len;
			tx_buf->lso_word1.wrd = lso_para1.wrd;

			/* fill structure HEADER_A */
			ni_header_a.bits.ldpid = ldpid;

			/* if called from ca_tx() the lspid comes from ca_pkt_t */
			if (is_from_ca_tx)
				ni_header_a.bits.lspid = lspid;
			else {
				/* packet comes from CPU256 lo/hi cpu_port is 10 and 11 */
				if (cep->egress_cpu_port_id >= 10) {
					ni_header_a.bits.lspid = 0x20 + (cep->egress_cpu_port_id - 10) * 0x10;
				}
				else {
					ni_header_a.bits.lspid = 0x10 + cep->egress_cpu_port_id;  /* use the original cpu_port */
				}
			}

			ni_header_a.bits.pkt_size = lso_para1.bf.packet_size;

			if (tx_config->core_config.bf.flow_id_set) {
				ni_header_a.bits.bits_32_63.pkt_info.pol_id = tx_config->flow_id;

				// when wifi shared port do hw lookup, the pol_en must be 0 to prevent L2TE policing.
				// ni_header_a.bits.bits_32_63.pkt_info.pol_en = 1;

			}

			/* disabled bypass for testing */
			if (port_id == 0) {
				ni_header_a.bits.fe_bypass = 1;	     /* WAN port bypass L2FE */
				if (is_from_ca_tx) {
					/* if ldpid > 0x20 it means gem_index or llid */
					if (ni_header_a.bits.ldpid < 0x20) {
						if (ni_header_a.bits.ldpid != wan_port_id)
							ni_header_a.bits.fe_bypass = 0;	     /* go through L2FE let cos take effect  */
					}
				}

				/* called from ca_ni_send_packet() and cos=8 go through highest queue id */
				if (is_from_ca_tx && go_9th_queue) {
					ni_header_a.bits.ldpid += 8;  	/* 9th queue of NI port */

					/* ASIC team think it may set it to guarantee critical protocol packet */
					ni_header_a.bits.bits_32_63.pkt_info.no_drop = 1;

					/* Modify CA_PKT_TYPE_OAM pol_id so to mapping to highest priority queue */
					/* Note: below num 8 and 7 are hardcoded to indicate 8 voqs/LLID in PON PUC, to be modified according to SCFG */
					if ((0xff == skb->data[12])&&(0xf1 == skb->data[13])){
						ni_header_a.bits.bits_32_63.pkt_info.pol_id = (skb->data[5] & 0x3f) * 8 + 7;
					}
				}
			}
			else {
				if (ldpid > 0x17)
					ni_header_a.bits.fe_bypass = 0;
				else
					ni_header_a.bits.fe_bypass = 1;
			}

			/* set header A "cos" field */
			ni_header_a.bits.cos = txq_index;
			if (is_from_ca_tx) {
				/* if called from ca_tx() then use CA_TX_BYPASS_FWD_ENGINE to decide the bypass bit */
				ni_header_a.bits.fe_bypass = bypass_fwd_engine;
			}

			/* the headerA in DMA LSO should be [headerA 0:31][headerA 32:63] */
			/* [headerA 0:31] high word, [headerA 32:63] low word */
			/* Only Header CPU exists need to set cpu_flag to be 1 */
			if (hdr_cpu)
				ni_header_a.bits.bits_32_63.pkt_info.cpu_flg = 1;

			if (is_ptp) {
				if (is_from_ca_tx) {
					/* set ptp_cmd of Header A from user inputs */
					ca_ni_tx_set_hdr_a_ptp_cmd(&ni_header_a, &ptp_tx_cmd);
				}
			}

			tx_buf->header_a.bits64 = ni_header_a.bits64;

			/* [header CPU 0:31] high word, [header CPU 32:63] low word */
			tx_buf->header_cpu.bits64 = tx_config->core_config.bf.sw_id;

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:   sw_id=0x%llx\n", __func__, tx_config->core_config.bf.sw_id);
				printk("%s:   mdata_h=0x%x, mdata_l=0x%x\n", __func__,
					      tx_buf->header_cpu.mdata_raw.mdata_h, tx_buf->header_cpu.mdata_raw.mdata_l);
				printk("%s:   lso_para0.wrd=0x%x, lso_para1.wrd=0x%x\n", __func__, lso_para0.wrd, lso_para1.wrd);
				printk("%s:   tx_buf->header_a.bits64=0x%llx\n", __func__, tx_buf->header_a.bits64);
				printk("%s:   tx_buf->header_cpu.bits64=0x%llx\n", __func__, tx_buf->header_cpu.bits64);
				printk("%s:   tx_desc0.wrd=0x%x, tx_desc1.wrd=0x%x\n", __func__, tx_desc0.wrd, tx_desc1.wrd);
				printk("%s:   swtxq->total_desc_num=%d\n", __func__, swtxq->total_desc_num);
				ca_ni_dump_hdr_a(&ni_header_a, false);
			}
		} /* if (frag_id == 0) */
		else {
			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:   tx_desc0.wrd=0x%x, tx_desc1.wrd=0x%x\n", __func__, tx_desc0.wrd, tx_desc1.wrd);
				printk("%s:   swtxq->total_desc_num=%d\n", __func__, swtxq->total_desc_num);
			}
		}

		curr_desc->word0.wrd = tx_desc0.wrd;
		curr_desc->word1.wrd = tx_desc1.wrd;

		swtxq->wptr = CA_NI_RWPTR_ADVANCE_ONE(swtxq->wptr, swtxq->total_desc_num);
		free_desc--;

		/* the first descriptor is for LSO Para[63:0] and HEADER_A, donot descrement snd_pages */
		if (frag_id > 0) {
			snd_pages--;
		}
		frag_id++;
	} /* while (snd_pages != 0) */

	/* only count as one packet */
	if (frag_id > 0)
		stats->tx_packets++;

	smp_wmb();

        if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: swtxq->wptr_reg=0x%x, swtxq->wptr=%d\n", __func__, swtxq->wptr_reg, swtxq->wptr);
	}

	/* the wptr_reg has been assigned according to cpu_id already */
	aal_ni_set_tx_write_ptr(swtxq->wptr_reg, swtxq->wptr);

	spin_unlock_bh(&swtxq->lock);

	return NETDEV_TX_OK;
}

/*
 * buf0: need to have at least 24 bytes headroom for adding dma_lso, hdr_a, hdr_cpu
 *         (8277C new feature: hp0/hp1=11 not need headroom anymore)
 * skb: could be NULL and should not be monitored by ca_mut module
 */
static inline netdev_tx_t ca_ni_start_xmit_buf(void * buf0, int len0, void * buf1, int len1,
	struct sk_buff *skb, ca_ni_tx_config_t *tx_config)
{
	struct net_device *dev;
	char *pkt_datap;
	int cpu_id;
	int port_id;
	int free_desc, len;
	int go_9th_queue = 0;
	ca_eth_private_t *cep;
	struct net_device_stats *stats;
	ni_info_t *ni = &ni_info_data;
	dma_swtxq_t *swtxq;
	dma_txdesc_t *curr_desc;
	dma_txbuf_t	*tx_buf;
	dma_txdesc_0_t	tx_desc0;
	dma_txdesc_1_t	tx_desc1;
	dma_lsopara_0_t lso_para0;
	dma_lsopara_1_t lso_para1;
	HEADER_A_T	ni_header_a;
	struct platform_device *pdev;
	int txq_index = tx_config->core_config.bf.txq_index;
	int ldpid = tx_config->core_config.bf.ldpid;
	int lspid = tx_config->core_config.bf.lspid;
	int is_from_ca_tx = tx_config->core_config.bf.is_from_ca_tx;
	ca_uint8_t bypass_fwd_engine = tx_config->core_config.bf.bypass_fwd_engine;
	ca_uint8_t hdr_cpu;

	ca_uint8_t bypass_lso = tx_config->bypass_lso;
	ca_uint8_t dma_aft_l2fib_enable = tx_config->dma_aft_l2fib_enable;
	ca_uint8_t dma_aft_l2fib_index = tx_config->dma_aft_l2fib_index;
	ca_uint8_t dma_lso_mode = RTK_TX_MODE_DEFAULT;


	dev = ca_ni_get_cpu_port_dev(tx_config->core_config.bf.lspid);
	if (dev == NULL) {
		return NETDEV_TX_BUSY;
	}

	cep = netdev_priv(dev);
	stats = &cep->dev->stats;
	pdev = cep->pdev;
	/* if device not up and runing do not transmit out */
	if (!(dev->flags & IFF_UP)) {
		pr_err("device %s is not up!!\n", dev->name);
		return NETDEV_TX_BUSY;
	}

	/* if cos=8 comes from ca_tx() we need to adjust it to txq_index=7 to avoid get wrong txq */
	if (txq_index == 8) {
		txq_index = 7;
		if (is_from_ca_tx) {
			go_9th_queue = 1;
		}
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		ca_ni_dump_tx_pkts(skb);
		printk("%s: txq_index=%d, ldpid=%d, lspid=%d, is_from_ca_tx=%d, bypass_fwd_engine=%d, dma_lso_ace_test=%d\n",
			__func__, txq_index, ldpid, lspid, is_from_ca_tx, bypass_fwd_engine, dma_lso_ace_test);
	}

	/* use XPS method to decide which CPU take care which TX queue */
	cpu_id = smp_processor_id();

	/* if called from ca_tx() get port_id from lspid */
	if (is_from_ca_tx) {
		if (lspid >= 0x20)
			port_id = (lspid - 0x20) + 10;   /* use DMA LSO VP 10 (for CPU256 LO) and 11 (for CPU256 HI) */
		else
			port_id = lspid - AAL_LPORT_CPU_0;
	}
	else {
		port_id = cep->egress_cpu_port_id;
	}

	/* use cpu port to determined which DMA LSO we should use */
	/* because DMA LSO id will be used as lspid for L2FE */
	swtxq = &ni->swtxq[port_id][txq_index];

        /* use CFG_ID_NE_TX_TIMER_ENABLE flag in scfg.txt to decide whether to have a timer */
        if (ca_ni_tx_timer_enable) {
                cancel_tx_completion_timer(cep->dev);
                if (check_to_perform_tx_complete(txq_index, cep->dev))
                        ca_dma_tx_complete(txq_index, cep->dev);
                else
                        update_tx_completion_timer(txq_index, cep->dev);
        }
        else
                ca_dma_tx_complete(txq_index, cep->dev);

        /* use spin lock bottom half here to avoid software irq intercept here */
        spin_lock_bh(&swtxq->lock);

	/// check if TX buffer/queue is full, return NETDEV_TX_BUSY
	if (swtxq->wptr >= swtxq->finished_idx)
		free_desc = swtxq->total_desc_num - swtxq->wptr - 1 + swtxq->finished_idx;
	else
		free_desc = swtxq->finished_idx - swtxq->wptr - 1;

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: free_desc=%d\n", __func__, free_desc);
	}

	/* try to reserve 1 descriptor in case skb is extended in xmit function */
	if (free_desc <= 2) {
		spin_unlock_bh(&swtxq->lock);
		return NETDEV_TX_BUSY;
	}

	/* In order to support packet size=30 PPP packets even packet size is smaller than CA_NI_MIN_DMA_SIZE=34 */
	/* we need to do software padded here */
	if ((len0 + len1) < CA_NI_MIN_DMA_SIZE) {
		if (len1 == 0)
			len0 = CA_NI_MIN_DMA_SIZE;
		else
			len1 = CA_NI_MIN_DMA_SIZE - len0;
	}

	tx_desc0.wrd = tx_desc1.wrd = lso_para0.wrd = lso_para1.wrd = 0;
	ni_header_a.bits64 = 0;
	curr_desc = swtxq->desc_base + swtxq->wptr;
	len = sizeof(dma_txbuf_t);					/* there is no data in the first descriptor */

	tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_SOF;
	if (len1 == 0)
		tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_EOF;

	/*
	 * hdr_a.pkt_size should be (8 bytes cpu_hdr) + payload + 4 crc
	 * But this value will be update by dma_lso
	 */
	ni_header_a.bits.pkt_size = len0 + len1;

	hdr_cpu = 0;
	if (tx_config->core_config.bf.sw_id != 0)
		hdr_cpu = 1;

	if (hdr_cpu == 0) {
		dma_lso_mode = rtk_dma_lso_mode_check(skb, dev, tx_config);
		if(dma_lso_mode && (go_9th_queue == 0) && (tx_desc1.bf.sop_eop & CA_NI_DMA_LSO_SOF) && (tx_desc1.bf.sop_eop & CA_NI_DMA_LSO_EOF)) {
			len = 0;
			
			pkt_datap = (char *)(buf0);

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s: TX without any header!dma lso mode = %d, pkt len = %d\n", __func__, dma_lso_mode, len0);
			}

			goto TX_WITHOUT_HDR;	/* HP[1:0]=11 - lso_parameters, header_a and header_cpu not exist */
		}else {
			dma_lso_mode = RTK_TX_MODE_DEFAULT;
			tx_desc1.bf.hp1 = 0;	/* HP[1:0]=01-header A exists and header CPU does not exist */
			tx_desc1.bf.hp0 = 1;
			len -= sizeof(HEADER_CPU_T);
		}
	} else {
		tx_desc1.bf.hp1 = 1;	/* HP[1:0]=10-header A exists and header CPU exists */
		tx_desc1.bf.hp0 = 0;

		/* Header CPU always there so the cpu_flg should be 1 */
		ni_header_a.bits.bits_32_63.pkt_info.cpu_flg = 1;
		/*
		 * hdr_a.pkt_size should be 8 bytes cpu_hdr + payload + 4 crc
		 * But this value will be update by dma_lso
		 */
		ni_header_a.bits.pkt_size += sizeof(HEADER_CPU_T);
	}

	pkt_datap = (char *)(buf0 - len);

	/* fill structure dma_lso_para#0/1 */
	if (unlikely(hdr_cpu))
		lso_para0.wrd = 0;
	else {
#if defined(CONFIG_HWNAT_FLEETCONNTRACK)
		if(bypass_lso || lspid >= AAL_LPORT_CPU_2)
			lso_para0.wrd = 0;	// wifi hardware lookup
		else
			ca_ni_get_lso_para0(skb, dev, (len0 + len1), &lso_para0);
#else	
		// rome driver
		lso_para0.wrd = tx_config->lso_para0.wrd & 0xbfffffff;	// ca_ni_get_lso_para0(skb, dev, skb->len, &lso_para0);
#endif
	}
	

#ifdef CONFIG_PROC_FS
	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: lso_para0.ipv4_en=%d, lso_para0.ipv6_en=%d\n", __func__, lso_para0.bf.ipv4_en, lso_para0.bf.ipv6_en);
		printk("%s: lso_para0.tcp_en=%d, lso_para0.udp_en=%d\n", __func__, lso_para0.bf.tcp_en, lso_para0.bf.udp_en);
		printk("%s: lso_para0.segment_en=%d, lso_para0.segment_size=%d\n", __func__, lso_para0.bf.segment_en, lso_para0.bf.segment_size);
		printk("%s: lso_para0.bypass_en=%d, lso_para0.lenfix_en=%d, lso_para0.unknown_tag_en=%d\n", __func__, lso_para0.bf.bypass_en, lso_para0.bf.lenfix_en, lso_para0.bf.unknown_tag_en);
	}
#endif	

	/*dma_lso.packet_size must be the payload size (no crc)*/
	lso_para1.bf.packet_size = len0 + len1;

	tx_buf = (dma_txbuf_t *)(pkt_datap);
	tx_buf->lso_word0.wrd = lso_para0.wrd;
	tx_buf->lso_word1.wrd = lso_para1.wrd;

	/* fill structure HEADER_A */
	ni_header_a.bits.ldpid = ldpid;
	ni_header_a.bits.bits_32_63.pkt_info.deep_q = cep->deep_q;

	/* if called from ca_tx() the lspid comes from ca_pkt_t */
	if (is_from_ca_tx) {
		ni_header_a.bits.cos = txq_index;
		/* if called from ca_tx() then use CA_TX_BYPASS_FWD_ENGINE to decide the bypass bit */
		ni_header_a.bits.fe_bypass = bypass_fwd_engine;
		ni_header_a.bits.lspid = lspid;
		if (go_9th_queue) {
			ni_header_a.bits.ldpid += 8;  	/* 9th queue of NI port */
			/* ASIC team think it may set it to guarantee critical protocol packet */
			ni_header_a.bits.bits_32_63.pkt_info.no_drop = 1;
		}
	} else {
		/* packet comes from CPU256 lo/hi cpu_port is 10 and 11 */
		if (cep->egress_cpu_port_id >= 10) {
			ni_header_a.bits.lspid = 0x20 + (cep->egress_cpu_port_id - 10) * 0x10;
		}
		else {
			ni_header_a.bits.lspid = 0x10 + cep->egress_cpu_port_id;  /* use the original cpu_port */
		}

		if (ldpid > 0x17)
			ni_header_a.bits.fe_bypass = 0;
		else
			ni_header_a.bits.fe_bypass = 1;
	}
	if (tx_config->core_config.bf.flow_id_set) {
		ni_header_a.bits.bits_32_63.pkt_info.pol_id = tx_config->flow_id;

		// when wifi shared port do hw lookup, the pol_en must be 0 to prevent L2TE policing.
		// ni_header_a.bits.bits_32_63.pkt_info.pol_en = 1;

	}

	/* test DMA_AFT, drop_code[2:0], bit 2=1 to enable AFT */
	if (dma_aft_l2fib_enable) {
		ni_header_a.bits.bits_32_63.pkt_info.drop_code =
			(dma_aft_l2fib_enable << 2) | ((dma_aft_l2fib_index & 0x8) >> 3);
		ni_header_a.bits.bits_32_63.pkt_info.pol_grp_id = dma_aft_l2fib_index & 0x7;
	}

	/* the headerA in DMA LSO should be [headerA 0:31][headerA 32:63] */
	/* [headerA 0:31] high word, [headerA 32:63] low word */
	tx_buf->header_a.bits64 = ni_header_a.bits64;

	/* [header CPU 0:31] high word, [header CPU 32:63] low word */
	if (hdr_cpu) {
		tx_buf->header_cpu.bits64 = tx_config->core_config.bf.sw_id;
	}
	

	if (dma_lso_ace_test) {
		tx_desc1.bf.buf_addr_39_32 = 2;
		tx_desc0.wrd = virt_to_phys(pkt_datap);
	} else
		tx_desc0.wrd = dma_map_single(&(pdev->dev), pkt_datap, len + len0, DMA_TO_DEVICE);

	tx_desc1.bf.buf_len = len + len0;

TX_WITHOUT_HDR:
	if(dma_lso_mode){
		tx_desc0.wrd = tx_desc1.wrd = 0;

		if (dma_lso_ace_test)
			tx_desc0.wrd = virt_to_phys(pkt_datap);
		else
			tx_desc0.wrd = dma_map_single(&(pdev->dev), pkt_datap, len + len0, DMA_TO_DEVICE);

		if(dma_lso_mode == RTK_TX_MODE_DIRECT_TX_WAN){	//direct tx to wan
			tx_desc1.bf_direct_tx_wan.stream_id = tx_config->core_config.bf.flow_id_set?tx_config->flow_id:0;
			tx_desc1.bf_direct_tx_wan.buf_len = len + len0;
			tx_desc1.bf_direct_tx_wan.chk_sel = 1;
			tx_desc1.bf_direct_tx_wan.mode = 1;	//direct tx
			tx_desc1.bf_direct_tx_wan.direct = 1;	//to wan
			tx_desc1.bf_direct_tx_wan.aft_fib = dma_aft_l2fib_enable?dma_aft_l2fib_index:0;
			tx_desc1.bf_direct_tx_wan.hp0 = tx_desc1.bf_direct_tx_wan.hp1 = 1;
			if (ca_ni_debug & NI_DBG_DUMP_TX)
				printk("\t [DIRECT_TX_WAN] gem_id=%d chk_sel=%d aft_fib=%d\n", tx_desc1.bf_direct_tx_wan.stream_id, tx_desc1.bf_direct_tx_wan.chk_sel, tx_desc1.bf_direct_tx_wan.aft_fib);
		}else if(dma_lso_mode == RTK_TX_MODE_DIRECT_TX_LAN){	//direct tx to lan
			tx_desc1.bf_direct_tx_lan.dest = tx_config->core_config.bf.ldpid;
			tx_desc1.bf_direct_tx_lan.cos = tx_config->core_config.bf.txq_index;
			tx_desc1.bf_direct_tx_lan.buf_len = len + len0;
			tx_desc1.bf_direct_tx_lan.chk_sel = 1;
			tx_desc1.bf_direct_tx_lan.mode = 1;	//direct tx
			tx_desc1.bf_direct_tx_lan.direct = 0;	//to lan
			tx_desc1.bf_direct_tx_lan.aft_fib = dma_aft_l2fib_enable?dma_aft_l2fib_index:0;
			tx_desc1.bf_direct_tx_lan.hp0 = tx_desc1.bf_direct_tx_lan.hp0 = 1;
			if (ca_ni_debug & NI_DBG_DUMP_TX)
				printk("\t [DIRECT_TX_LAN] ldpid=%d cos=%d chk_sel=%d aft_fib=%d\n", tx_desc1.bf_direct_tx_lan.dest, tx_desc1.bf_direct_tx_lan.cos, tx_desc1.bf_direct_tx_lan.chk_sel, tx_desc1.bf_direct_tx_lan.aft_fib);
		}else if(dma_lso_mode == RTK_TX_MODE_WIFI_HW_LOOKUP){	//wifi hwlookup
			tx_desc1.bf_wifi_hwlookup.fe_bypass = tx_config->core_config.bf.bypass_fwd_engine;
			tx_desc1.bf_wifi_hwlookup.dev_id = tx_config->core_config.bf.flow_id_set?tx_config->flow_id:0;
			tx_desc1.bf_wifi_hwlookup.buf_len = len + len0;
			tx_desc1.bf_wifi_hwlookup.mode = 0;	//hwlookup
			tx_desc1.bf_wifi_hwlookup.dest_sel = (tx_config->core_config.bf.ldpid == AAL_LPORT_L3_LAN)?0:1;
			tx_desc1.bf_wifi_hwlookup.source = tx_config->core_config.bf.lspid;
			tx_desc1.bf_wifi_hwlookup.hp0 = tx_desc1.bf_wifi_hwlookup.hp1 = 1;
			if (ca_ni_debug & NI_DBG_DUMP_TX)
				printk("\t [WIFI_HWLOOKUP] dev_id=%d lspid=%d dest_sel=%d fe_bypass=%d\n", tx_desc1.bf_wifi_hwlookup.dev_id, tx_desc1.bf_wifi_hwlookup.source, tx_desc1.bf_wifi_hwlookup.dest_sel, tx_desc1.bf_wifi_hwlookup.fe_bypass);
		}
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s:   buf0=%p, len0=%d buf1=%p, len1=%d\n", __func__, buf0, len0, buf1, len1);
		printk("\t port_id=%d \n", port_id);
		if(dma_lso_mode) {
			printk("\t dma_lso_mode=%d hdr_len=%d cpu_hdr(0/1) %d dma_l3fe=%d cep %p\n", dma_lso_mode, len, hdr_cpu, cep->dma_l3fe, cep);
		}
		else if(!bypass_lso) {
			printk("\t hdr_len=%d cpu_hdr(0/1) %d dma_l3fe=%d cep %p\n", len, hdr_cpu, cep->dma_l3fe, cep);
			printk("\t lso_para0.wrd=0x%x, lso_para1.wrd=0x%x (%d)\n", lso_para0.wrd, lso_para1.wrd, lso_para1.bf.packet_size);
			printk("\t tx_buf->header_a.bits64=0x%llx\n", tx_buf->header_a.bits64);
			ca_ni_dump_hdr_a(&ni_header_a, false);
			printk("\t tx_buf->header_cpu.bits64=0x%llx\n", tx_buf->header_cpu.bits64);
		}
		printk("\t tx_desc0.wrd=0x%x, tx_desc1.wrd=0x%x\n", tx_desc0.wrd, tx_desc1.wrd);
		printk("\t\t tx_desc1.buf_len=%d\n", tx_desc1.bf.buf_len);
		printk("\t swtxq->total_desc_num=%d\n", swtxq->total_desc_num);
		printk("\t xmit_done_cb_fn=%p, cb_cookie=%p\n",
				tx_config->xmit_done_cb_fn, tx_config->cb_cookie);
	}


	curr_desc->word0.wrd = tx_desc0.wrd;
	curr_desc->word1.wrd = tx_desc1.wrd;

	if (len1) {
		swtxq->wptr = CA_NI_RWPTR_ADVANCE_ONE(swtxq->wptr, swtxq->total_desc_num);
		tx_desc0.wrd = tx_desc1.wrd = 0;

		curr_desc = swtxq->desc_base + swtxq->wptr;
		if (dma_lso_ace_test) {
			tx_desc1.bf.buf_addr_39_32 = 2;
			tx_desc0.wrd = virt_to_phys(buf1);
		} else
			tx_desc0.wrd = dma_map_single(&(pdev->dev), buf1, len1, DMA_TO_DEVICE);

		tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_EOF; /* EOF */
		tx_desc1.bf.buf_len = len1;
		curr_desc->word0.wrd = tx_desc0.wrd;
		curr_desc->word1.wrd = tx_desc1.wrd;
	}

	swtxq->tx_skb[swtxq->wptr] = skb;
	if (skb)
		ca_skb_in(skb, true);

	if (tx_config->core_config.bf.cb_fn_mask) {
		swtxq->xmit_done_cb_fn[swtxq->wptr] = tx_config->xmit_done_cb_fn;
		swtxq->xmit_done_cookie[swtxq->wptr] = tx_config->cb_cookie;
	}

	swtxq->wptr = CA_NI_RWPTR_ADVANCE_ONE(swtxq->wptr, swtxq->total_desc_num);

	stats->tx_bytes += len0 + len1;
	stats->tx_packets++;
	

	smp_wmb();

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: swtxq->wptr_reg=0x%x, swtxq->wptr=%d\n", __func__, swtxq->wptr_reg, swtxq->wptr);
	}

	/* the wptr_reg has been assigned according to cpu_id already */
	aal_ni_set_tx_write_ptr(swtxq->wptr_reg, swtxq->wptr);

	spin_unlock_bh(&swtxq->lock);

	return NETDEV_TX_OK;
}

netdev_tx_t __ca_ni_start_xmit_buf(struct sk_buff *skb, struct net_device *dev,
       ca_ni_tx_config_t *tx_config)
{
       return ca_ni_start_xmit_buf(skb->data, skb->len, NULL, 0, skb, tx_config);
}

#if defined(CONFIG_LUNA_G3_SERIES)
/*
 * buf0: need to have at least 24 bytes headroom for adding dma_lso, hdr_a, hdr_cpu
 * skb: could be NULL and should not be monitored by ca_mut module
 */
netdev_tx_t ca_ni_debugStart_xmit_buf(
    void * buf0, int len0, dma_txbuf_t *tx_headers, dma_txbuf_t *header_mask)
{
       /* TODO: add NIC API for 77C */
       return NETDEV_TX_OK;
}
#endif

/*
 * Handles DMA LSO interrupts.
 */
irqreturn_t ca_ni_tx_interrupt(int irq, void *dev_instance)
{
	aal_ni_get_dma_glb_lso_interrupt_status();
        return IRQ_HANDLED;
}

static void ca_ni_init_tx_dma_lso_interrupts(struct platform_device *pdev)
{
	int retval;
	struct device_node *parent_np = pdev->dev.of_node;
	struct device_node *np;

	/* got VP 10 interrupt */
	np = of_get_child_by_name(parent_np, "dma_lso_vp10_int");
	dma_lso_vp10_irq = irq_of_parse_and_map(np, 0);
	printk("%s: irq=%d\n", __func__, dma_lso_vp10_irq);

	/* got VP 11 interrupt */
	np = of_get_child_by_name(parent_np, "dma_lso_vp11_int");
	dma_lso_vp11_irq = irq_of_parse_and_map(np, 0);
	printk("%s: irq=%d\n", __func__, dma_lso_vp11_irq);

	/* got DMA LSO interrupt */
	np = of_get_child_by_name(parent_np, "dma_lso_int");
	dma_lso_irq = irq_of_parse_and_map(np, 0);
	printk("%s: irq=%d\n", __func__, dma_lso_irq);

	/* request irq for DMA LSO */
	retval = request_irq(dma_lso_irq, ca_ni_tx_interrupt, 0, "dma lso", &(pdev->dev));
	if (retval != 0 && retval != -EBUSY) {
		printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, dma_lso_irq, retval);
		return;
	}
}

void ca_ni_init_tx_dma_lso(struct platform_device *pdev)
{
	int i, j;
	ni_info_t *ni =	&ni_info_data;
	u32 rptr_addr, wptr_addr;
	u32 dma_tx_base_addr;
	dma_addr_t tx_desc_paddr[CA_NI_DMA_LSO_TXQ_NUM];
	dma_txdesc_t *tx_desc[CA_NI_DMA_LSO_TXQ_NUM];
	dma_addr_t tx_buf_paddr[CA_NI_DMA_LSO_TXQ_NUM];
	dma_txbuf_t *tx_buf[CA_NI_DMA_LSO_TXQ_NUM];

	/* because dma_alloc_coherent() need a non-NULL device */
	struct device *dev = &(pdev->dev);

	spin_lock_init(&ca_ni_send_packet_lock);

	ca_ni_dma_lso_init_startup_config();

	/* There are 12 virtual ports for DMA LSO, 4 for CPUs linux kernel, 4 for PEs */
	/* each virtual port has NI_DMA_LSO_TXQ_NUM=8 queues */
	for (j = 0; j < ca_ni_dma_lso_cpu_vp_num; j++){

		dma_tx_base_addr = DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + (j * DMA_SEC_DMA_LSO_VP_STRIDE);
		rptr_addr = DMA_SEC_DMA_LSO_VP_TXQ0_RPTR + (j * DMA_SEC_DMA_LSO_VP_STRIDE);
		wptr_addr = DMA_SEC_DMA_LSO_VP_TXQ0_WPTR + (j * DMA_SEC_DMA_LSO_VP_STRIDE);

		for (i = 0; i < CA_NI_DMA_LSO_TXQ_NUM; i++) {

			tx_buf[i] = dma_alloc_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), &tx_buf_paddr[i], GFP_KERNEL);

			if (!tx_buf[i]) {
				printk("%s: TX %d dma_alloc_coherent() for tx_buf fail !!!\n", __func__, i);
				dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), tx_buf, tx_buf_paddr[i]);
				return;
			}

			ni->swtxq[j][i].buf_base = tx_buf[i];
			ni->swtxq[j][i].buf_dma_addr = tx_buf_paddr[i];

			tx_desc[i] = dma_alloc_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), &tx_desc_paddr[i], GFP_KERNEL);

			ni->swtxq[j][i].rptr_reg = rptr_addr + (8 * i);      /* the stride to next queue is 8 */
			ni->swtxq[j][i].wptr_reg = wptr_addr + (8 * i);	     /* the stride to next queue is 8 */
			ni->swtxq[j][i].wptr = 0;
			ni->swtxq[j][i].total_desc_num = CA_NI_DMA_LSO_TXDESC_NUM;

			spin_lock_init(&ni->swtxq[j][i].lock);

			if (!tx_desc[i]) {
				printk("%s: TX %d dma_alloc_coherent() for tx_desc fail !!!\n", __func__, i);
				dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), tx_desc, tx_desc_paddr[i]);
				return;
			}

			/* the buffer address points to LSO para+Header A */
			tx_desc[i]->word0.wrd = tx_buf_paddr[i];
			ni->swtxq[j][i].desc_base = tx_desc[i];
			ni->swtxq[j][i].desc_dma_addr = tx_desc_paddr[i];

			aal_ni_set_dma_lso_base_depth_addr(j, i, tx_desc_paddr[i]);

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s: ni->swtxq[%d][%d].buf_base=%p, ni->swtxq[%d][%d].buf_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].buf_base, j, i, &ni->swtxq[j][i].buf_dma_addr);
				printk("%s: ni->swtxq[%d][%d].desc_base=%p, ni->swtxq[%d][%d].desc_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].desc_base, j, i, &ni->swtxq[j][i].desc_dma_addr);
			}
		}
	}

	ca_ni_init_tx_dma_lso_interrupts(pdev);
}

void ca_ni_cleanup_tx_dma_lso(struct platform_device *pdev)
{
	int i, j;
	ni_info_t *ni = &ni_info_data ;

	/* because dma_alloc_coherent() need a non-NULL device */
	struct device *dev = &(ni_info_data.dev[0]->dev);

	/* There are 12 virtual ports for DMA LSO, 4 for CPUs linux kernel, 4 for PEs */
	/* each virtual port has NI_DMA_LSO_TXQ_NUM=6 queues */
	for (j = 0; j < ca_ni_dma_lso_cpu_vp_num; j++){
		for (i = 0; i < CA_NI_DMA_LSO_TXQ_NUM; i++) {
			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s: ni->swtxq[%d][%d].buf_base=%p, ni->swtxq[%d][%d].buf_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].buf_base, j, i, (void *)ni->swtxq[j][i].buf_dma_addr);
				printk("%s: ni->swtxq[%d][%d].desc_base=%p, ni->swtxq[%d][%d].desc_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].desc_base, j, i, (void *)ni->swtxq[j][i].desc_dma_addr);
			}
			dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), ni->swtxq[j][i].buf_base, ni->swtxq[j][i].buf_dma_addr);
			dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), ni->swtxq[j][i].desc_base, ni->swtxq[j][i].desc_dma_addr);
		}
	}
}

/*
 * Reset all TX rings.
 */
void ca_ni_reset_tx_ring(void)
{
	int i, new_idx, cpu_port;
	ni_info_t *ni = &ni_info_data;
	dma_swtxq_t *swtxq;
	dma_rptr_t rptr_reg;
	dma_txdesc_0_t tx_desc0;
	dma_txdesc_t *curr_desc;

	for (cpu_port = 0; cpu_port < ca_ni_dma_lso_cpu_vp_num; cpu_port++) {
		for (i = 0; i < CA_NI_DMA_LSO_TXQ_NUM; i++) {
			swtxq = &ni->swtxq[cpu_port][i];
			swtxq->intr_cnt++;
			rptr_reg.wrd = aal_ni_get_tx_read_ptr(swtxq->rptr_reg);

			while (swtxq->wptr != swtxq->finished_idx) {
				curr_desc = swtxq->desc_base + swtxq->finished_idx;
				tx_desc0.wrd = curr_desc->word0.wrd;
				new_idx = swtxq->finished_idx;

				if (swtxq->tx_skb[new_idx]) {
					ca_dev_kfree_skb_any(swtxq->tx_skb[new_idx]);
					swtxq->tx_skb[new_idx] = NULL;
					swtxq->total_finished++; // or not update?
					swtxq->finished_idx = CA_NI_RWPTR_ADVANCE_ONE(new_idx, swtxq->total_desc_num);
				}
			}

			spin_lock_init(&swtxq->lock);
			swtxq->finished_idx = rptr_reg.wrd;
			swtxq->wptr = rptr_reg.bf.rptr;
			aal_ni_set_tx_write_ptr(swtxq->wptr_reg, rptr_reg.wrd);
		}
	}
}

/* for management mode transmit packet */
static unsigned char   pkt_buf[2048];
static u32 calc_crc(u32 crc, u8 const *p, u32 len)
{
	int i;

	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++) {
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
		}
	}
	return crc;
}

int ca_ni_mgmt_start_xmit(struct sk_buff *skb, struct net_device *dev, int txq_index, int dvoq)
{
	ca_ni_mgmt_send_packet(skb->data, skb->len);
	ca_dev_kfree_skb_any(skb);
	return 0;
}

int ca_ni_mgmt_send_packet(u8 *pkt_data, int length)
{
	u32     hw_tx_rd_ptr;
	u32     sw_tx_wr_ptr;
	unsigned int    new_pkt_len;
	unsigned char   valid_bytes=0;
	u32     *tx_xram_ptr;
	u16  next_link = 0;
	unsigned char   *pkt_buf_ptr;
	unsigned int    loop;
	u32     crc32;
	NI_HEADER_X_T   hdr_xt;
	int             pad=0;
	u32     *data_ptr;
	u32     *save_xram_ptr;
	NI_HV_XRAM_CPUXRAM_CPU_STAT_TX_0_t      cpuxram_cpu_stat_tx;
	NI_HV_XRAM_CPUXRAM_CPU_CFG_TX_0_t       cpuxram_cpu_cfg_tx;
	struct iphdr *iph;
	struct net_device *dev = ni_info_data.dev[0];
	ca_eth_private_t *cep = netdev_priv(dev);
        struct ca_ni_priv *priv = (struct ca_ni_priv *) &(cep->mgmt_priv);
	struct net_device_stats *stats = &cep->dev->stats;

	if (length > 2032) {
		printk("%s: packet size (%d) is too large!!!\n", __func__, length);
		return -1;
	}

	/* Get the hardware read pointer */
	cpuxram_cpu_stat_tx.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CPU_STAT_TX_0);
	hw_tx_rd_ptr = cpuxram_cpu_stat_tx.bf.pkt_rd_ptr;

	/* Get the software write pointer */
	cpuxram_cpu_cfg_tx.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CPU_CFG_TX_0);
	sw_tx_wr_ptr = cpuxram_cpu_cfg_tx.bf.pkt_wr_ptr;

	/* when packte is UDP then zero out the checksum word */
	iph = (struct iphdr *)(pkt_data + 14);
	if (iph->protocol == 0x11) {
		pkt_data[0x28] = 0;
		pkt_data[0x29] = 0;
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, pkt_data, length, true);
		printk("%s : hw_tx_rd_ptr = %x\n", __func__, hw_tx_rd_ptr);
		printk("%s : sw_tx_wr_ptr = %x\n", __func__, sw_tx_wr_ptr);
	}

	if (hw_tx_rd_ptr != sw_tx_wr_ptr) {
		printk("%s: Tx FIFO is not available!\n", __func__);
		return 1;
	}

	/* only copy the real length */
	memset(pkt_buf, 0x00, sizeof(pkt_buf));

	/* add 8-byte header_A at the beginning of packet */
	memcpy(&(pkt_buf[sizeof(HEADER_A_T)]), (const void *)pkt_data, length);

	/* a workaround on 2015/10/01 the packet size+CRC should be 8-byte alignment */
	if (((length+4) % 8) != 0)
		length += (8 - ((length+4) % 8));

	pad = 64 - (length + 4);        /* if packet length < 60 */
	pad = (pad < 0) ? 0 : pad;

	new_pkt_len = length + pad ;    /* new packet length */

	//pkt_buf_ptr = (unsigned char *)pkt_data;
	pkt_buf_ptr = (unsigned char *)pkt_buf;

	/* Calculate the CRC32 */
	/* skip 8-byte header_A */
	crc32 = ~(calc_crc(~0, (u8 *)(pkt_buf_ptr + sizeof(HEADER_A_T)), new_pkt_len));


	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: crc32 is 0x%x \n", __func__, crc32);
		printk("%s: ~crc32 is 0x%x \n", __func__, ~crc32);
		printk("%s: pkt len %d \n", __func__, new_pkt_len);
	}

	/* should add 8-byte headerA */
	memcpy((pkt_buf_ptr+new_pkt_len+sizeof(HEADER_A_T)), (u8 *)(&crc32), sizeof(crc32));
	new_pkt_len = new_pkt_len + 4;  /* add CRC */

	valid_bytes = new_pkt_len % 8;
	valid_bytes = valid_bytes ? valid_bytes : 0;

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: valid_bytes %d\n", __func__, valid_bytes);
	}

	next_link = sw_tx_wr_ptr + (new_pkt_len+7+sizeof(HEADER_A_T)) / 8 ; /* for headr XT */
	next_link = next_link + 1; /* add header */
	/* Wrap around if required */
	if(next_link > priv->tx_xram_end) {

		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("Wrap around next_link = %d \n", next_link);
		}
		next_link = priv->tx_xram_start + (next_link - (priv->tx_xram_end+1));
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: TX next_link %d\n", __func__, next_link);
	}

	hdr_xt.wrd = 0;
	hdr_xt.bf.ownership = 1;
	hdr_xt.bf.bytes_valid = valid_bytes;
	hdr_xt.bf.next_link = next_link;

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: sw_tx_wr_ptr 0x%x\n", __func__, sw_tx_wr_ptr);
	}

	tx_xram_ptr = (u32 *)((ca_uint_t)AAL_XRAM_REG_ADDR(sw_tx_wr_ptr * 8));


	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: tx_xram_ptr %p\n", __func__, tx_xram_ptr);
	}

	/* Wrap around if required */
	if (tx_xram_ptr >= (u32 *)(ca_uint_t)(priv->tx_xram_end_addr)) {
		tx_xram_ptr = (u32 *)(ca_uint_t)(priv->tx_xram_base_addr);
	}

	/* skip first 4 bytes before copying the headerXT */
	tx_xram_ptr++;

	/* Wrap around if required */
	if (tx_xram_ptr >= (u32 *)(ca_uint_t)(priv->tx_xram_end_addr)) {
		tx_xram_ptr = (u32 *)(ca_uint_t)(priv->tx_xram_base_addr);
	}


	*tx_xram_ptr = hdr_xt.wrd;
	tx_xram_ptr++;

	/* Wrap around if required */
	if (tx_xram_ptr >= (u32 *)(ca_uint_t)(priv->tx_xram_end_addr)) {
		tx_xram_ptr = (u32 *)(ca_uint_t)(priv->tx_xram_base_addr);
	}

	/* Now to copy the data . The first byte on the line goes first */
	data_ptr = (u32 *)pkt_buf_ptr;


	for (loop = 0; loop <= (new_pkt_len+sizeof(HEADER_A_T)) / 4; loop++) {
		save_xram_ptr = tx_xram_ptr;
		*tx_xram_ptr++ = *data_ptr++;

		/* Wrap around if required */
		if (tx_xram_ptr >= (u32 *)(ca_uint_t)(priv->tx_xram_end_addr)) {
			tx_xram_ptr = (u32 *)(ca_uint_t)(priv->tx_xram_base_addr);
		}
	}

	smp_wmb();

	/* Publish the write pointer */
	cpuxram_cpu_cfg_tx.bf.pkt_wr_ptr = next_link;
	CA_NI_REG_WRITE(cpuxram_cpu_cfg_tx.wrd, NI_HV_XRAM_CPUXRAM_CPU_CFG_TX_0);

	stats->tx_bytes += new_pkt_len;
	stats->tx_packets++;

	return 0;
}

#include "pkt_tx.h"

/* DA MAC for OAM packets */
static ca_mac_addr_t puc_da = {0x00, 0x13, 0x25, 0x00, 0x00, 0x00};

/* SA/DA MAC for OMCI packets */
static ca_mac_addr_t sg_da = {0x00, 0x13, 0x25, 0x00, 0x00, 0x00};
static ca_mac_addr_t sg_sa = {0x00, 0x13, 0x25, 0x00, 0x00, 0x01};

static void ca_ni_tx_encap_pon_control_packet(ca_pkt_t *pkt, ca_uint8_t *data)
{
	/* take care OAM/MPCP packets */
	if (pkt->pkt_type == CA_PKT_TYPE_OAM || pkt->pkt_type == CA_PKT_TYPE_MPCP) {
		memcpy(&(data[CA_ETH_ADDR_LEN]), sg_sa, CA_ETH_ADDR_LEN);
		puc_da[5] = (puc_da[5] & 0xc0) | PORT_ID(pkt->dst_sub_port_id);  /* add llid info to puc_da[5:0] */
		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("%s: pkt->dst_sub_port_id=0x%x, puc_da[5]=0x%x\n", __func__, pkt->dst_sub_port_id, puc_da[5]);
		}
		memcpy(data, &(puc_da[0]), sizeof(ca_mac_addr_t));
		data[12] = 0xff;
		if (pkt->pkt_type == CA_PKT_TYPE_OAM)
			data[13] = 0xf1;
		else
			data[13] = 0xf0;
		data[14] = data[15] = 0;
	}

	/* take care OMCI/PLOAM packets */
	if (pkt->pkt_type == CA_PKT_TYPE_OMCI || pkt->pkt_type == CA_PKT_TYPE_PLOAM) {
		/* get source mac address from source interface */
		/* the SA/DA MAC address of OMCI are fixed */
		memcpy(&(data[CA_ETH_ADDR_LEN]), sg_sa, CA_ETH_ADDR_LEN);
		memcpy(data, sg_da, CA_ETH_ADDR_LEN);
		data[12] = 0xff;
		if (pkt->pkt_type == CA_PKT_TYPE_OMCI)
			data[13] = 0xf1;
		else
			data[13] = 0xf0;
		data[14] = data[15] = 0;

                do {
                        ca_uint8_t pon_mode = 0;

                        scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
                        if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
                                if ((PON_DBG_GPON_OMCI_TX & ca_pon_debug) ) {
                                        ca_buffer_dump("Tx OMCI =>", data, pkt->pkt_len+16);
                                }
                        } else if ((PON_DBG_EPON_OAM_TX & ca_pon_debug)) {
                                ca_buffer_dump("Tx OAM =>", data, pkt->pkt_len+16);
                        }

                } while(0);
	}
}

static void ca_ni_tx_get_pon_paras(ca_pkt_t *pkt, ca_uint8_t *go_9th_queue, ca_uint8_t *flow_id_set, ca_uint32_t *flow_id, int *txq_index, ca_uint16_t *ldpid)
{
	if (puc_voq_mode == 2) {
		if (PORT_TYPE(pkt->dst_port_id) == CA_PORT_TYPE_GPON) {
			*flow_id_set = 1;
			if (pkt->pkt_type == CA_PKT_TYPE_OMCI) {
				*go_9th_queue = 1;
				if (pkt->cos == 7) {
					*flow_id = 7;
				}
				else {
					*flow_id = 6;
				}
			}
			else {
				*flow_id = PORT_ID(pkt->dst_sub_port_id) & 0x7f;
			}
		}
		if (PORT_TYPE(pkt->dst_port_id) == CA_PORT_TYPE_EPON) {
			*flow_id_set = 1;
			if (pkt->pkt_type == CA_PKT_TYPE_OAM) {
				*go_9th_queue = 1;
				*flow_id = PORT_ID(pkt->dst_sub_port_id) * 8 + 7;
				*txq_index = 7;
			}
			else {
				*flow_id = PORT_ID(pkt->dst_sub_port_id) * 8 + pkt->cos;
			}
		}
	}
	if (puc_voq_mode == 0) {
		if (PORT_TYPE(pkt->dst_port_id) == CA_PORT_TYPE_GPON) {
			*flow_id_set = 1;
			if (pkt->pkt_type == CA_PKT_TYPE_OMCI) {
				*go_9th_queue = 1;
				if (pkt->cos == 7) {
					*flow_id = 7;
					*txq_index = 7;
				}
				else {
					*flow_id = 6;
					*txq_index = 6;
				}
			}
			else {
				*ldpid = 0x20 + (PORT_ID(pkt->dst_sub_port_id) >> 11);
				*flow_id = PORT_ID(pkt->dst_sub_port_id) & 0x7f;
				*txq_index = (PORT_ID(pkt->dst_sub_port_id) >> 8) & 0x7;
			}
		}
		if (PORT_TYPE(pkt->dst_port_id) == CA_PORT_TYPE_EPON) {
			if (pkt->pkt_type == CA_PKT_TYPE_OAM) {
				*go_9th_queue = 1;
				*txq_index = 7;
			}
			else {
				*ldpid = 0x20 + PORT_ID(pkt->dst_sub_port_id);
			}
		}
	}
	if (ca_ni_debug & NI_DBG_DUMP_TX) {

		printk("%s: port type=%d, puc_voq_mode=%d, pkt_type=%d\n", __func__, PORT_TYPE(pkt->dst_port_id), puc_voq_mode, pkt->pkt_type);
		printk("%s: flow_id=%d, txq_index=%d, ldpid=%d, go_9th_queue=%d\n", __func__, *flow_id, *txq_index, *ldpid, *go_9th_queue);
	}
}


ca_status_t __ca_ni_send_single_pkt(ca_pkt_t *pkt, int go_9th_queue, ca_uint32_t pol_id, ca_ni_tx_config_t *tx_config);
int ca_ni_send_packet(ca_pkt_t *pkt, ca_uint8_t bypass_fwd_engine, int kernel_mode)
{
	int ret;
	struct sk_buff *skb;
	struct net_device *dev;
	int encap_length;
	ca_ni_tx_config_t tx_config;
	u8 *pkt_data;
	int length, txq_index, total_len;
	ca_uint16_t ldpid, lspid;
	ca_uint64_t *p_sw_id;
	ca_uint8_t go_9th_queue = 0;
	ca_uint8_t flow_id_set = 0;
	ca_uint32_t flow_id = CA_UINT32_INVALID;
	ca_uint8_t *data, *save_ptr;
	ni_info_t *ni = &ni_info_data;
	int cpu_port;
	dma_swtxq_t *swtxq;
	ca_pkt_block_t *pkt_blk = pkt->pkt_data;
	int index;
	uint16_t ethertype = 0;
	ca_uint8_t *cp;

	/* add spin lock bottom half here to avoid software irq intercept here */
	spin_lock_bh(&ca_ni_send_packet_lock);

	pkt_data = pkt_blk->data;
	length   = pkt_blk->len;
	ldpid    = PORT_ID(pkt->dst_port_id);
	lspid    = PORT_ID(pkt->src_port_id);
	txq_index = pkt->cos;
	p_sw_id  = (ca_uint64_t *)&(pkt->sw_id[0]);

	dev =  ca_ni_get_cpu_port_dev(lspid);
	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: dev->name=%s, length=%d, ldpid=%d, txq_index=%d, bypass_fwd_engine=%d\n", __func__, dev->name, length, ldpid, txq_index, bypass_fwd_engine);
		printk("%s: sw_id[0-3]=0x%x-0x%x-0x%x-0x%x\n", __func__, pkt->sw_id[0], pkt->sw_id[1], pkt->sw_id[2], pkt->sw_id[3]);
		printk("%s: sw_id=0x%llx\n", __func__, *p_sw_id);
	}
	encap_length = 0;
	/* if packet type is OAM/OMCI/PLOAM/MPCP, should be encap a 16-byte header before send out */
	if (pkt->pkt_type == CA_PKT_TYPE_OAM || pkt->pkt_type == CA_PKT_TYPE_OMCI || pkt->pkt_type == CA_PKT_TYPE_PLOAM || pkt->pkt_type == CA_PKT_TYPE_MPCP) {
		encap_length = CA_ETH_ADDR_LEN + CA_ETH_ADDR_LEN + 4;
	}

	/* sent by start xmit or sent by __ca_ni_send_single_pkt() */
	if (ca_tx_by_start_xmit) {
		skb = ca_netdev_alloc_skb(dev, length + encap_length, true);
		if (!skb) {
			spin_unlock_bh(&ca_ni_send_packet_lock);
			printk(KERN_ERR "%s: Not enough memory to allocate skb of size %d.\n", dev->name, length);
			return -ENOMEM;
		}

		if (!kernel_mode) {
			copy_from_user(skb->data + encap_length, pkt_data, length);
		}
		else {
			memcpy(skb->data + encap_length, pkt_data, length);
		}

		skb->len = length + encap_length;
		data = skb->data;

		/* set network header */
		skb_set_network_header(skb, ETH_HLEN);

		/* set transport header */
		cp = skb->data;
		ethertype = ca_ni_parse_l2(cp, NULL, NULL, NULL, &cp, false);

		if (ntohs(ethertype) == ETH_P_IP) {
			skb_set_transport_header(skb, ETH_HLEN + sizeof(struct iphdr));
		}
		else if (ntohs(ethertype) == ETH_P_IP) {
			skb_set_transport_header(skb, ETH_HLEN + sizeof(struct ipv6hdr));
		}

	}
	else {
		total_len = length + encap_length;
		data = kmalloc(total_len, GFP_KERNEL);
		if (data == NULL) {
			spin_unlock_bh(&ca_ni_send_packet_lock);
			printk(KERN_ERR "%s: Not enough memory to allocate skb of size %d.\n", dev->name, total_len);
			return -ENOMEM;
		}

		if (!kernel_mode) {
			copy_from_user(data + encap_length, pkt_data, length);
		}
		else {
			memcpy(data + encap_length, pkt_data, length);
		}

		/* keep pkt_blk->data unchanged, caller may want to keep it unchanged */
		save_ptr = pkt_blk->data;

		pkt_blk->data = data;
		pkt_blk->len = total_len;
		pkt->pkt_len = total_len;
	}

	/* take care OAM/MPCP and OMCI/PLOAM packets */
	ca_ni_tx_encap_pon_control_packet(pkt, data);

	/* get flow_id, ldpid and txq_index from GPON/EPON settings */
	ca_ni_tx_get_pon_paras(pkt, &go_9th_queue, &flow_id_set, &flow_id, &txq_index, &ldpid);

	memset(&tx_config, 0, sizeof(ca_ni_tx_config_t));
	tx_config.core_config.bf.txq_index = txq_index;
	tx_config.core_config.bf.ldpid = ldpid;
	tx_config.core_config.bf.lspid = lspid;
	tx_config.core_config.bf.sw_id = *p_sw_id;
	tx_config.core_config.bf.is_from_ca_tx = 1;
	tx_config.core_config.bf.bypass_fwd_engine = bypass_fwd_engine;
	tx_config.core_config.bf.flow_id_set = flow_id_set;
	tx_config.go_9th_queue = go_9th_queue;
	tx_config.flow_id = flow_id;
	tx_config.ptp_tx_cmd = pkt->ptp_tx_cmd;
	tx_config.pkt_type= pkt->pkt_type;

	/* find the entry of DMA AFT L2FIB */
	if (pkt->outer_vlan_act != 0 || pkt->inner_vlan_act != 0 || pkt->pppoe_hdr_act != 0) {
		ret = ca_ni_tx_find_dma_aft_l2fib_index(pkt->outer_vlan_act, pkt->inner_vlan_act, pkt->pppoe_hdr_act, &index);
		if (ret == CA_E_OK) {
			tx_config.dma_aft_l2fib_enable = 1;
			tx_config.dma_aft_l2fib_index = index;
		}
	}

	if (ca_tx_by_start_xmit) {
		ret = __ca_ni_start_xmit(skb, dev, &tx_config);
		if (ret != NETDEV_TX_OK)
			ca_dev_kfree_skb_any(skb);
	}
	else {
#if 0
		pkt->src_port_id = lspid;
		pkt->dst_port_id = ldpid;
		pkt->cos = txq_index;

		cpu_port = ca_ni_get_cpu_port_by_lspid(lspid);
		swtxq = &ni->swtxq[cpu_port][txq_index];
		spin_lock_bh(&swtxq->lock);
#endif
		__ca_ni_send_single_pkt(pkt, go_9th_queue, flow_id, &tx_config);

//		spin_unlock_bh(&swtxq->lock);

		kfree(data);

		/* keep pkt_blk->data unchanged, caller may want to keep it unchanged */
		pkt_blk->data = save_ptr;
	}

	spin_unlock_bh(&ca_ni_send_packet_lock);

	return ret;
}

int ca_ni_send_dying_gasp_packet(ca_pkt_t *pkt)
{
	int length, encap_length, total_len, txq_index, cpu_port;
	ca_uint16_t ldpid, lspid;
	ca_uint8_t go_9th_queue = 0;
	ca_uint8_t flow_id_set = 0;
	ca_uint32_t flow_id = CA_UINT32_INVALID;
	ca_uint8_t *data, *save_ptr;
	ca_pkt_block_t *pkt_blk = pkt->pkt_data;
	ca_ni_tx_config_t tx_config;

	length = pkt_blk->len;
	ldpid    = PORT_ID(pkt->dst_port_id);
	lspid    = PORT_ID(pkt->src_port_id);
	txq_index = pkt->cos;

	encap_length = 0;
	/* if packet type is OAM/OMCI/PLOAM/MPCP, should be encap a 16-byte header before send out */
	if (pkt->pkt_type == CA_PKT_TYPE_OAM || pkt->pkt_type == CA_PKT_TYPE_OMCI || pkt->pkt_type == CA_PKT_TYPE_PLOAM || pkt->pkt_type == CA_PKT_TYPE_MPCP) {
		encap_length = CA_ETH_ADDR_LEN + CA_ETH_ADDR_LEN + 4;
	}

	total_len = length + encap_length;
	data = kmalloc(total_len, GFP_KERNEL);
	if (data == NULL) {
		printk(KERN_ERR "%s: Not enough memory to allocate skb of size %d.\n", __func__, total_len);
		return -ENOMEM;
	}

	memcpy(data + encap_length, pkt_blk->data, length);

	/* do not change pkt_blk->data, caller may want to keep it unchanged */
	save_ptr = pkt_blk->data;

	pkt_blk->data = data;
	pkt_blk->len = total_len;
	pkt->pkt_len = total_len;

        /* take care OAM/MPCP and OMCI/PLOAM packets */
        ca_ni_tx_encap_pon_control_packet(pkt, data);

        /* get flow_id, ldpid and txq_index from GPON/EPON settings */
        ca_ni_tx_get_pon_paras(pkt, &go_9th_queue, &flow_id_set, &flow_id, &txq_index, &ldpid);

	pkt->src_port_id = lspid;
	pkt->dst_port_id = ldpid;
	pkt->cos = txq_index;

#if 0
	cpu_port = lspid - AAL_LPORT_CPU_0;
	if (cpu_port < 0) {
		printk("%s: invalid lspid value %d!!\n", __func__, lspid);
		kfree(data);
		return CA_E_ERROR;
	}
#endif

	memset(&tx_config, 0, sizeof(ca_ni_tx_config_t));
	tx_config.core_config.bf.txq_index = txq_index;
	tx_config.core_config.bf.ldpid = ldpid;
	tx_config.core_config.bf.lspid = lspid;
	tx_config.core_config.bf.bypass_fwd_engine = 1;
	tx_config.go_9th_queue = 1;

	__ca_ni_send_single_pkt(pkt, go_9th_queue, flow_id, &tx_config);

	kfree(data);

	/* do not change pkt_blk->data, caller may want to keep it unchanged */
	pkt_blk->data = save_ptr;

	return CA_E_OK;
}

/* this routine does not has spin lock protect, caller should spin lock before call it */
ca_status_t __ca_ni_send_single_pkt(ca_pkt_t *pkt, int go_9th_queue, ca_uint32_t pol_id, ca_ni_tx_config_t *tx_config)
{
	char *pkt_datap;
	int cpu_port;
	int snd_pages;
	int frag_id, free_desc, total_len, len;
	dma_swtxq_t *swtxq;
	dma_txdesc_t *curr_desc;
	dma_txbuf_t	*tx_buf;
	dma_txdesc_0_t	tx_desc0;
	dma_txdesc_1_t	tx_desc1;
	dma_lsopara_0_t lso_para0;
	dma_lsopara_1_t lso_para1;
	HEADER_A_T	ni_header_a;
	struct net_device_stats *stats;
	int txq_index = tx_config->core_config.bf.txq_index;
	int ldpid = tx_config->core_config.bf.ldpid;
	int lspid = tx_config->core_config.bf.lspid;
	ni_info_t *ni = &ni_info_data;
	struct platform_device *pdev;
	struct net_device *dev;
	ca_eth_private_t *cep;
	ca_pkt_block_t *pkt_blk = pkt->pkt_data;
	ca_uint64_t sw_id = tx_config->core_config.bf.sw_id;
	ca_pkt_type_t pkt_type = tx_config->pkt_type;
	ca_uint8_t bypass_fwd_engine = tx_config->core_config.bf.bypass_fwd_engine;
	ca_uint8_t dma_aft_l2fib_enable = tx_config->dma_aft_l2fib_enable;
	ca_uint8_t dma_aft_l2fib_index = tx_config->dma_aft_l2fib_index;
	ca_uint8_t hdr_cpu = 0;
	int is_ptp = 0;

	/* default has no header CPU included but if sw_id is not 0 then header CPU should be included */
	if (sw_id != 0)
		hdr_cpu = 1;

	dev = ca_ni_get_cpu_port_dev(lspid);
	if (dev == NULL) {
		printk("%s: lspid=%d, can not get net_device!!\n", __func__, lspid);
		return CA_E_ERROR;
	}

	if (!(dev->flags & IFF_UP)) {
		pr_err("device %s is not up!!\n", dev->name);
		return CA_E_ERROR;
	}

	cep = netdev_priv(dev);
	stats = &cep->dev->stats;
        pdev = cep->pdev;

	snd_pages = 1;

	cpu_port = ca_ni_get_cpu_port_by_lspid(lspid);

	/* use cpu port to determined which DMA LSO we should use */
	/* because DMA LSO id will be used as lspid for L2FE */
	if (txq_index == 8) {
		txq_index = 7;
		go_9th_queue = 1;
	}
	if (txq_index > 7) {
		printk("%s: invalid cos value %d!!\n", __func__, txq_index);
		return CA_E_ERROR;
	}
	swtxq = &ni->swtxq[cpu_port][txq_index];

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: cpu_port=%d, txq_index=%d, dma_lso_ace_test=%d\n", __func__, cpu_port, txq_index, dma_lso_ace_test);
	}

	spin_lock_bh(&swtxq->lock);

	/// check if TX buffer/queue is full, return CA_E_ERROR
	if (swtxq->wptr >= swtxq->finished_idx)
		free_desc = swtxq->total_desc_num - swtxq->wptr - 1 + swtxq->finished_idx;
	else
		free_desc = swtxq->finished_idx - swtxq->wptr - 1;

	/* try to reserve 1 descriptor in case skb is extended in xmit function */
	if (free_desc <= snd_pages) {
		spin_unlock_bh(&swtxq->lock);
		return CA_E_ERROR;
	}

	total_len = pkt_blk->len;

	/* In order to support packet size=30 PPP packets even packet size is smaller than CA_NI_MIN_DMA_SIZE=34 */
	/* we need to do software padded here */
	if (total_len < CA_NI_MIN_DMA_SIZE) {
		total_len = CA_NI_MIN_DMA_SIZE;
	}

	/* check pkt_type CA_PKT_TYPE_L2_PTP and CA_PKT_TYPE_L4_PTP */
	if (pkt_type == CA_PKT_TYPE_L2_PTP || pkt_type == CA_PKT_TYPE_L4_PTP) {
		is_ptp = 1;
	}

	/* while loop to send out all of the pages */
	frag_id = 0;

	while (snd_pages != 0) {

		tx_desc0.wrd = tx_desc1.wrd = lso_para0.wrd = lso_para1.wrd = 0;
		ni_header_a.bits64 = 0;

		curr_desc = swtxq->desc_base + swtxq->wptr;

		/* the first descriptor should not include packet data */
		/* it includes only LSO Para[63:0] and Header A */
		if (frag_id == 0) {
			len = sizeof(dma_txbuf_t);   				/* there is no data in the first descriptor */

			tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_SOF;

			tx_desc1.bf.hp1 = 1;    /* HP[1:0]=10-header A exists and header CPU does not exist */
			tx_desc1.bf.hp0 = 0;

			/* if PTP packet always setup header CPU */
			if (is_ptp)
				hdr_cpu = 1;

			if (hdr_cpu == 0) {
				tx_desc1.bf.hp1 = 0;    /* HP[1:0]=01-header A exists and header CPU does not exist  */
				tx_desc1.bf.hp0 = 1;
				len -= sizeof(HEADER_CPU_T);
			}

			pkt_datap = (char *)(swtxq->buf_base + swtxq->wptr);

			/* assign dma address to TX desc */
			tx_desc0.wrd = swtxq->buf_dma_addr + (swtxq->wptr * sizeof(dma_txbuf_t));
		} else {
			if (dma_lso_ace_test) {
				tx_desc1.bf.buf_addr_39_32 = 2;
			}

			/* the 2nd descriptor point to the begin of packet data */
			if (frag_id == 1) {

				len = total_len;

				/* no need to do virtual to phys convert because will get the physical addres by dma_map_single() */
				pkt_datap = (char *)(uintptr_t) pkt_blk->data;

				if (dma_lso_ace_test)
					tx_desc0.wrd = virt_to_phys(pkt_datap);
				else
					tx_desc0.wrd = dma_map_single(&(pdev->dev), pkt_datap, total_len, DMA_TO_DEVICE);
			}
		} /* if (frag_id == 0) */

		tx_desc1.bf.buf_len = len;  /* transfer buffer size locates at bit [23:8] */

		/* snd_pages==1 means no fragment data */
		if ((snd_pages == 1) && (frag_id != 0)) {
			tx_desc1.bf.sop_eop |= CA_NI_DMA_LSO_EOF; /* EOF */
		}

		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("%s:   frag_id=%d, tx_desc1.wrd=0x%x, tx_desc0.wrd=0x%x\n", __func__, frag_id, tx_desc1.wrd, tx_desc0.wrd);
		}

		/* should move out of the while loop */
		if (frag_id != 0) {  /* exclude frag_id = 0, which is sizeof(dma_txbuf_t) */
			stats->tx_bytes += len;
		}

		/* if the first descriptor, fill the LSO Para[63:0] and HEADER_A */
		if (frag_id == 0) {
			tx_buf = (dma_txbuf_t *)(swtxq->buf_base + swtxq->wptr);

			tx_buf->lso_word0.wrd = lso_para0.wrd;

			lso_para1.bf.packet_size = total_len;
			tx_buf->lso_word1.wrd = lso_para1.wrd;

			/* fill structure HEADER_A */
			ni_header_a.bits.ldpid = ldpid;  /* go through 9th queue */
			if (go_9th_queue) {
				ni_header_a.bits.ldpid += 8;  /* go through 9th queue */
				ni_header_a.bits.bits_32_63.pkt_info.no_drop = 1;
			}
			if (pol_id != CA_UINT32_INVALID) {
				ni_header_a.bits.bits_32_63.pkt_info.pol_id = pol_id;
				ni_header_a.bits.bits_32_63.pkt_info.pol_en = 1;
			}

			ni_header_a.bits.lspid = lspid;
			ni_header_a.bits.pkt_size = total_len;

			ni_header_a.bits.fe_bypass = bypass_fwd_engine;
			ni_header_a.bits.cos = txq_index;

			/* [header CPU 0:31] high word, [header CPU 32:63] low word */
			tx_buf->header_cpu.bits64 = sw_id;

			/* Modify CA_PKT_TYPE_OAM pol_id so to mapping to highest priority queue */
			/* Note: below num 8 and 7 are hardcoded to indicate 8 voqs/LLID in PON PUC, to be modified according to SCFG */
			if ((0xff == pkt_blk->data[12])&&(0xf1 == pkt_blk->data[13])){
				ni_header_a.bits.bits_32_63.pkt_info.pol_id = (pkt_blk->data[5] & 0x3f) * 8 + 7;
			}

			/* the headerA in DMA LSO should be [headerA 0:31][headerA 32:63] */
			/* [headerA 0:31] high word, [headerA 32:63] low word */
			/* Only Header CPU exists need to set cpu_flag to be 1 */
			if (hdr_cpu)
				ni_header_a.bits.bits_32_63.pkt_info.cpu_flg = 1;

			if (is_ptp) {
				/* set ptp_cmd of Header A from user inputs */
				ca_ni_tx_set_hdr_a_ptp_cmd(&ni_header_a, &(pkt->ptp_tx_cmd));
			}

			/* test DMA_AFT, drop_code[2:0], bit 2=1 to enable AFT */
			if (dma_aft_l2fib_enable) {
				ni_header_a.bits.bits_32_63.pkt_info.drop_code =
					(dma_aft_l2fib_enable << 2) | ((dma_aft_l2fib_index & 0x8) >> 3);
				ni_header_a.bits.bits_32_63.pkt_info.pol_grp_id = dma_aft_l2fib_index & 0x7;
			}

			tx_buf->header_a.bits64 = ni_header_a.bits64;

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:   frag_id=%d\n", __func__, frag_id);
				printk("%s:   lso_para0.wrd=0x%x, lso_para1.wrd=0x%x\n", __func__, lso_para0.wrd, lso_para1.wrd);
				printk("%s:   tx_buf->header_a.bits64=0x%llx\n", __func__, tx_buf->header_a.bits64);
				printk("%s:   tx_buf->header_cpu.bits64=0x%llx\n", __func__, tx_buf->header_cpu.bits64);
				printk("%s:   tx_desc0.wrd=0x%x, tx_desc1.wrd=0x%x\n", __func__, tx_desc0.wrd, tx_desc1.wrd);
				printk("%s:   swtxq->total_desc_num=%d\n", __func__, swtxq->total_desc_num);
				ca_ni_dump_hdr_a(&ni_header_a, false);
			}
		} /* if (frag_id == 0) */
		else {
			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				printk("%s:   frag_id=%d\n", __func__, frag_id);
				printk("%s:   tx_desc0.wrd=0x%x, tx_desc1.wrd=0x%x\n", __func__, tx_desc0.wrd, tx_desc1.wrd);
				printk("====== TX data@%p, len %d ======\n", pkt_datap, total_len);
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, pkt_datap, total_len, true);
			}
		}

		curr_desc->word0.wrd = tx_desc0.wrd;
		curr_desc->word1.wrd = tx_desc1.wrd;

		swtxq->wptr = CA_NI_RWPTR_ADVANCE_ONE(swtxq->wptr, swtxq->total_desc_num);
		free_desc--;

		/* the first descriptor is for LSO Para[63:0] and HEADER_A, donot descrement snd_pages */
		if (frag_id > 0) {
			snd_pages--;
		}
		frag_id++;
	} /* while (snd_pages != 0) */

	smp_wmb();

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		printk("%s: swtxq->wptr_reg=0x%x, swtxq->wptr=%d\n", __func__, swtxq->wptr_reg, swtxq->wptr);
	}

	aal_ni_clear_dma_lso_misc_interrupts(cpu_port, txq_index);

	/* the wptr_reg has been assigned according to cpu_id already */
	aal_ni_set_tx_write_ptr(swtxq->wptr_reg, swtxq->wptr);

	/* wait for tansmit done */
	{
		int i;
		ca_uint32_t rd_ptr, wr_ptr;

		wr_ptr = aal_ni_get_tx_read_ptr(swtxq->wptr_reg);
		for (i = 0; i < 200; i++) {
			rd_ptr = aal_ni_get_tx_read_ptr(swtxq->rptr_reg);
			if (rd_ptr == wr_ptr)
				break;
		}
		if (i == 200) {
			printk("%s: DMA LSO timeout send failed!!\n", __func__);
			printk("%s: read pointer=%d, write pointer=%d\n", __func__, rd_ptr, wr_ptr);
		}
	}


	/* increment finished_idx twice for 2 descriptors used*/
	swtxq->finished_idx = CA_NI_RWPTR_ADVANCE_ONE(swtxq->finished_idx, swtxq->total_desc_num);
	swtxq->finished_idx = CA_NI_RWPTR_ADVANCE_ONE(swtxq->finished_idx, swtxq->total_desc_num);

	stats->tx_packets++;

	spin_unlock_bh(&swtxq->lock);

	return CA_E_OK;
}

ca_status_t ca_ni_tx_set_ldpid(u16 cpu_port, u16 tx_ldpid)
{
	struct net_device *dev;
	ca_eth_private_t *cep;

	if (cpu_port >= CA_NI_TOTAL_CPU_PORT) {
		printk("%s: cpu_port should smaller than %d\n", __func__, CA_NI_TOTAL_CPU_PORT);
		return CA_E_ERROR;
	}
	if (tx_ldpid >= CA_PORT_MAX_NUM) {
		printk("%s: cpu_port should smaller than %d\n", __func__, CA_PORT_MAX_NUM);
		return CA_E_ERROR;
	}
	dev = ni_info_data.dev[cpu_port];
	cep = netdev_priv(dev);
	cep->port_cfg.tx_ldpid = tx_ldpid;
	return CA_E_OK;
}

ca_status_t aal_ni_dma_lso_get_aft_l2fib(
        int index,
        DMA_SEC_DMA_AFT_L2FIB_DATA0_t *data0,
        DMA_SEC_DMA_AFT_L2FIB_DATA1_t *data1,
        DMA_SEC_DMA_AFT_L2FIB_DATA2_t *data2);

static ca_status_t ca_ni_tx_find_dma_aft_l2fib_index(
	ca_classifier_vlan_action_t top_vlan_action,
	ca_classifier_vlan_action_t inner_vlan_action,
	ca_pppoe_hdr_action_t	    pppoe_hdr_action,
	int	*index)
{
	int i;
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t data2;

	printk("%s: top_vlan_action=%d, inner_vlan_action=%d, pppoe_hdr_action=%d\n", __func__,
		top_vlan_action, inner_vlan_action, pppoe_hdr_action);

	for (i = 0; i < 16; i++) {
		if (aal_ni_dma_lso_get_aft_l2fib(i, &data0, &data1, &data2) != CA_E_OK) {
			continue;
		}
		if (top_vlan_action == data2.bf.vlan_cnt && inner_vlan_action == data2.bf.inner_vlan_cmd
			&& pppoe_hdr_action == data0.bf.pppoe_cmd)
		{
			/* find the L2FIB table entry matches with our input */
			*index = i;

			printk("%s: index=%d\n", __func__, *index);
			return CA_E_OK;
		}
        }

	printk("%s: not found!!!\n", __func__);
	return CA_E_ERROR;
}
