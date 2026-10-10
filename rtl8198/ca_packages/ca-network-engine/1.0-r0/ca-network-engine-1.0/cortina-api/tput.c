/*
 * ca_tput.c
 *
 * $Id$
 *
 * This file contains the implementation for CA TPUT Offload Kernel Module.
 */
#include <generated/ca_ne_autoconf.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/ip6_route.h>
#include <net/esp.h>
#include <net/ah.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define USE_DDR_PKT_BUF
#define SFU_PON_MODE
#define GET_INTF_FROM_L2
#define USE_CLS_FOR_RX
#endif

#ifndef USE_CLS_FOR_RX
#include "aal_hash.h"
#endif

#ifndef GET_INTF_FROM_L2
#include "route.h"
#endif

#ifdef SFU_PON_MODE
#include "ca-scfg/scfg.h"
#include "aal_pon.h"
#include "epon.h"
#include "pkt_tx.h"
#endif

#include "aal_port.h"
#include "tput.h"
#include "l2.h"
#include "classifier.h"
#include "aal_fdb.h"

#define IPERF_TPUT


/***************************************************************************
 * TPUT and Common Declaration
 ***************************************************************************/

#define CA_MAX_TPUT_FLOW_COUNT	2

static spinlock_t ca_tput_lock;
ca_uint8_t tput_init_done = 0;

ca_uint8_t upstream_active = 0;
ca_uint8_t downstream_active = 0;
ca_uint_t tx_jiffies_timeout;

ca_tput_upstream_flow_spec_t priv_upstream_flow[CA_MAX_TPUT_FLOW_COUNT];
ca_tput_downstream_flow_spec_t priv_downstream_flow[CA_MAX_TPUT_FLOW_COUNT];
ca_uint16_t downstream_flow_id[CA_MAX_TPUT_FLOW_COUNT];

ca_uint8_t downstream_rest_active_cnt = 0;

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define SHARE_MEM_PHY_BASE 0x40000000
#else
#define SHARE_MEM_PHY_BASE 0xC0001000
#endif

#define SHARE_MEM_PHY_SIZE (0x4E800 + 0x2030)

void __iomem *ca_tput_sram_base;

#ifdef USE_DDR_PKT_BUF
void __iomem *ca_tput_ddr_base;
#define PKT_BUFF_PHY_BASE 0x02900000
#define PKT_BUFF_PHY_SIZE 0x4E800
#endif

#define TPUT_PKT_HEADROOM 0x40

#define SHARE_MEM_BASE	ca_tput_sram_base

#define PE1_MIB_RX_PKT_CNT_ADDR  (SHARE_MEM_BASE + 0x28)
#define PE1_MIB_RX_BYTE_CNT_ADDR (SHARE_MEM_BASE + 0x30)
#define PE1_MIB_TX_PKT_CNT_ADDR  (SHARE_MEM_BASE + 0x38)
#define PE1_MIB_TX_BYTE_CNT_ADDR (SHARE_MEM_BASE + 0x40)

#define PE1_CTRL_BASED	(SHARE_MEM_BASE + 0x2000)

#define PE0_MIB_RX_PKT_CNT_ADDR  (SHARE_MEM_BASE + 0x88)
#define PE0_MIB_RX_BYTE_CNT_ADDR (SHARE_MEM_BASE + 0x90)
#define PE0_MIB_TX_PKT_CNT_ADDR  (SHARE_MEM_BASE + 0x98)
#define PE0_MIB_TX_BYTE_CNT_ADDR (SHARE_MEM_BASE + 0xA0)

#define TPUT_OFFSET_PKT_SIZE	0
#define TPUT_OFFSET_INTERVAL	4
#define TPUT_OFFSET_PKT_NUM	8
#define TPUT_OFFSET_PHY_PORT	12
#define TPUT_OFFSET_TX_ENABLE	16
#define TPUT_OFFSET_RX_ENABLE	20
#define TPUT_OFFSET_RX1_ENABLE		24

#ifdef SFU_PON_MODE
#define TPUT_OFFSET_PON_ID		28
#endif

#ifndef USE_DDR_PKT_BUF
#define TPUT_OFFSET_PKT_CTX		0x100
#endif

#define TPUT_BPS		(1000000/8)
#define TPUT_FCS		4
#define TPUT_PREAMBLE_IPG	20
#define TPUT_TAROKO_CPU_CLOCK	800

#define TPUT_DBG(x)

extern ca_boolean_t ca_api_is_ip_equal (ca_ip_address_t *ip_addr1,
					ca_ip_address_t *ip_addr2);

#define FPS_REF_COUNT		14
#define UNSPEC_U16			0xffff

typedef struct interval_to_fps_s {
	ca_uint32_t fps;
	ca_uint16_t interval;
} interval_to_fps_t;

/***************************************************************************
 * TPUT and Common Function
 ***************************************************************************/

interval_to_fps_t int_to_fps[FPS_REF_COUNT] = {
	/* rate: UDP rate 1514 pkt size(include CRC) */
	{1169925, 5},
	{1092903, 10},
	{1005033, 15},
	{973300, 17},
	{812886, 18},		/* 10G: 9621 Mbps 814786 fps */
	{728723, 38},		/* 9G : 8659 Mbps 733376 fps */
	{647058, 49},		/* 8G : 7697 Mbps 651846 fps */
	{487808, 80},		/* 6G : 5773 Mbps 488905 fps */
	{407000, 105},		/* 5G : 4810 Mbps 407350 fps */
	{325240, 143},		/* 4G : 3848 Mbps 325880 fps */
	{244095, 206},		/* 3G : 2886 Mbps 244410 fps */
	{162700, 332},		/* 2G : 1924 Mbps 162940 fps */
	{81493, 709},		/* 1G :  962 Mbps  81470 fps */
	{40747, 1464},		/* 500M: 481 Mbps  40735 fps */
};

static ca_status_t __convert_mbps_to_delay_counter (
	ca_uint32_t udp_mbps, ca_uint32_t total_len,
	ca_uint32_t ip_payload_len,
	ca_uint32_t * p_delay_i,
	ca_uint32_t total_pkt)
{
	ca_uint32_t fps;
	ca_uint32_t t = 0;
	ca_uint32_t i;

	fps = udp_mbps * TPUT_BPS / ip_payload_len;

	if ((fps > int_to_fps[0].fps) || (fps < int_to_fps[FPS_REF_COUNT-1].fps)) {
		printk("%s udp_mbps %d udp length = %d fps = %d out of support range %d ~ %d\n",
			__func__, udp_mbps, ip_payload_len, fps, int_to_fps[0].fps,
			int_to_fps[FPS_REF_COUNT-1].fps);
		return CA_E_PARAM;
	}

	for (i = 0; i < (FPS_REF_COUNT - 1); i++) {
		if ((fps <= int_to_fps[i].fps) && (fps >= int_to_fps[i+1].fps)) {
			t = int_to_fps[i].interval + ((int_to_fps[i+1].interval - int_to_fps[i].interval) *
				(int_to_fps[i].fps - fps) / (int_to_fps[i].fps - int_to_fps[i+1].fps));
			/* for roundup() */
			if (fps != int_to_fps[i+1].fps)
				t++;
			break;
		}
	}

	i = fps * 8 / 10;
	i = ((total_pkt / i) + 1) * 1000;
	tx_jiffies_timeout = jiffies + msecs_to_jiffies(i);
	printk("%s udp_mbps = %d udp length = %d fps = %d t=%d time_out = %d ms\n",
		__func__, udp_mbps, ip_payload_len, fps, t, i);
	*p_delay_i = (ca_uint32_t) t;

	return CA_E_OK;
}

ca_status_t ca_tput_init(CA_IN ca_device_id_t device_id)
{
	if (tput_init_done == 0) {
		memset(priv_upstream_flow, 0,
		       sizeof(ca_tput_upstream_flow_spec_t) *
		       CA_MAX_TPUT_FLOW_COUNT);
		memset(priv_downstream_flow, 0,
		       sizeof(ca_tput_downstream_flow_spec_t) *
		       CA_MAX_TPUT_FLOW_COUNT);
		spin_lock_init(&ca_tput_lock);
		tput_init_done = 1;
		ca_tput_sram_base =
		    ioremap_nocache(SHARE_MEM_PHY_BASE, SHARE_MEM_PHY_SIZE);
#ifdef USE_DDR_PKT_BUF
		ca_tput_ddr_base =
		    ioremap_nocache(PKT_BUFF_PHY_BASE, PKT_BUFF_PHY_SIZE);
#endif
		TPUT_DBG(printk
			 ("%s ca_tput_sram_base %p\n", __func__,
			  ca_tput_sram_base));
	}
	return CA_E_OK;
}

EXPORT_SYMBOL(ca_tput_init);

#if 0
/* set tcp checksum: given IP header and tcp segment */
static void
__compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload)
{
	register unsigned long sum = 0;
	unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl << 2);
	struct tcphdr *tcphdrp = (struct tcphdr *)(ipPayload);

	/* add the pseudo header */
	/* the source ip */
	sum += (pIph->saddr >> 16) & 0xFFFF;
	sum += (pIph->saddr) & 0xFFFF;
	/* the dest ip */
	sum += (pIph->daddr >> 16) & 0xFFFF;
	sum += (pIph->daddr) & 0xFFFF;
	/* protocol and reserved: 6 */
	sum += htons(IPPROTO_TCP);
	/* the length */
	sum += htons(tcpLen);

	/* add the IP payload */
	/* initialize checksum to 0 */
	tcphdrp->check = 0;
	while (tcpLen > 1) {
		sum += *ipPayload++;
		tcpLen -= 2;
	}

	/* if any bytes left, pad the bytes and add */
	if (tcpLen > 0) {
		sum += ((*ipPayload) & htons(0xFF00));
	}
	/* Fold 32-bit sum to 16 bits: add carrier to result */
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	sum = ~sum;
	/* set computation result */
	tcphdrp->check = (unsigned short)sum;
}
#endif

/* set tcp checksum: given IP header and UDP datagram */

static void
__compute_udp_checksum(struct iphdr *pIph, unsigned short *ipPayload)
{
	register unsigned long sum = 0;
	struct udphdr *udphdrp = (struct udphdr *)(ipPayload);
	unsigned short udpLen = htons(udphdrp->len);
	unsigned short counter = 0;
	TPUT_DBG(printk("%s %d\n", __func__, __LINE__));
	/* add the pseudo header */
	/* the source ip */
	sum += (pIph->saddr >> 16) & 0xFFFF;
	TPUT_DBG(printk
		 ("%s %d (pIph->saddr>>16)&0xFFFF=0x%x\n", __func__, __LINE__,
		  (pIph->saddr >> 16) & 0xFFFF));
	sum += (pIph->saddr) & 0xFFFF;
	TPUT_DBG(printk
		 ("%s %d (pIph->saddr)&0xFFFF=0x%x\n", __func__, __LINE__,
		  (pIph->saddr) & 0xFFFF));
	/* the dest ip */
	sum += (pIph->daddr >> 16) & 0xFFFF;
	TPUT_DBG(printk
		 ("%s %d (pIph->daddr>>16)&0xFFFF=0x%x\n", __func__, __LINE__,
		  (pIph->daddr >> 16) & 0xFFFF));
	sum += (pIph->daddr) & 0xFFFF;
	TPUT_DBG(printk
		 ("%s %d (pIph->daddr)&0xFFFF=0x%x\n", __func__, __LINE__,
		  (pIph->daddr) & 0xFFFF));
	/* protocol and reserved: 17 */
	sum += htons(IPPROTO_UDP);
	/* the length */
	TPUT_DBG(printk
		 ("%s %d htons(IPPROTO_UDP)=0x%x\n", __func__, __LINE__,
		  htons(IPPROTO_UDP)));
	sum += udphdrp->len;
	TPUT_DBG(printk
		 ("%s %d udphdrp->len=0x%x\n", __func__, __LINE__,
		  udphdrp->len));

	/* add the IP payload */
	/* initialize checksum to 0 */
	udphdrp->check = 0;
	while (udpLen > 1) {
		sum += *ipPayload++;
		counter++;
		udpLen -= 2;
	}
	/* if any bytes left, pad the bytes and add */
	if (udpLen > 0) {
		sum += ((0) & htons(0xFF00));
		TPUT_DBG(printk("%s %d pad the bytes\n", __func__, __LINE__));
	}

	/* Fold sum to 16 bits: add carrier to result */
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	sum = ~sum;
	/* set computation result */
	udphdrp->check =
	    ((unsigned short)sum == 0x0000) ? 0xFFFF : (unsigned short)sum;
	TPUT_DBG(printk
		 ("%s %d udp checksum 0x%x\n", __func__, __LINE__,
		  udphdrp->check));
}

static ca_status_t
__ca_fill_ip_protocol(struct iphdr *iph, void *l4hdr,
		      ca_tput_upstream_flow_spec_t * flows)
{
	struct udphdr udp_hdr, *p_udp_hdr = &udp_hdr;
#ifdef CONFIG_IPV6
	struct ipv6hdr *ip6h = (struct ipv6hdr *) iph;
#endif
	ca_uint16_t protonum;
	memset(p_udp_hdr, 0, sizeof(struct udphdr));

	if (iph->version == 4)
		protonum = iph->protocol;
#ifdef CONFIG_IPV6
	else if (iph->version == 6)
		protonum = ip6h->nexthdr;
#endif
	if (protonum == IPPROTO_UDP) {	/* UDP */
		p_udp_hdr->source = htons(flows->l4_src_port);
		p_udp_hdr->dest = htons(flows->l4_dst_port);
		if (iph->version == 4)
			p_udp_hdr->len = htons(ntohs(iph->tot_len) - 20);
#ifdef CONFIG_IPV6
		if (iph->version == 6)
			p_udp_hdr->len = ip6h->payload_len;
#endif
		/* Count udp checksum by hardware*/
		/*__compute_udp_checksum(iph, (unsigned short *) p_udp_hdr);*/
		memcpy_toio(l4hdr, p_udp_hdr, sizeof(struct udphdr));
		return CA_E_OK;
	}
	return CA_E_PARAM;
}

#ifdef SFU_PON_MODE
static ca_status_t
__read_scfg(	CA_IN ca_uint32_t type, CA_IN ca_uint32_t len,
		CA_OUT ca_device_config_tlv_t *scfg_tlv)
{
	if (!scfg_tlv)
		return CA_E_PARAM;

	memset(scfg_tlv, 0, sizeof(*scfg_tlv));
	scfg_tlv->type = type;
	scfg_tlv->len  = len;
	return ca_device_config_tlv_get(0, scfg_tlv);
}

/* identify pon_mac_mode(active_ethernet/epon/gpon) by scfg */
static ca_status_t
__pon_mac_mode_get(ca_uint8_t * ptr_pon_mode)
{
	ca_status_t ret = CA_E_OK;
	ca_device_config_tlv_t scfg_tlv;

	if (!ptr_pon_mode) {
		printk(KERN_ERR"%s:%d: null parameter error!\n",
			__func__, __LINE__);
		return CA_E_PARAM;
	}

	ret = __read_scfg((ca_uint32_t)CA_CFG_ID_PON_MAC_MODE,
				sizeof(ca_uint8_t), &scfg_tlv);

	if (ret != CA_E_OK)
		printk(KERN_WARNING
			"%s:%d: fail to get CA_CFG_ID_PON_MAC_MODE.\n",
			__func__, __LINE__);
	else
		*ptr_pon_mode = scfg_tlv.value[0];

	return ret;
}
#endif

static ca_status_t
__set_dst_entry(	CA_IN struct dst_entry *dst,
			CA_OUT struct sk_buff **skb)
{
	if (!skb || !dst)
		return CA_E_PARAM;

	*skb = alloc_skb(sizeof(struct vlan_ethhdr) + 40 + 8 + 128, GFP_ATOMIC);
	if (*skb == NULL)
		return CA_E_MEMORY;

	skb_reserve(*skb, sizeof(struct vlan_ethhdr));
	skb_reset_mac_header(*skb);

	skb_dst_set(*skb, dst_clone(dst));
	(*skb)->dev = skb_dst(*skb)->dev;
	return CA_E_OK;
}

static ca_status_t
__get_eth_by_neighbour(	CA_IN struct neighbour *neigh,
			CA_IN ca_uint16_t eth_proto,
			CA_OUT struct sk_buff **skb)
{
	if (!skb || !(*skb))
		return CA_E_PARAM;

	if (!neigh) {
		printk(KERN_WARNING "destination neighbour is NULL.dst %s \n",
			(*skb)->dev->name);
		return CA_E_NOT_FOUND;
	} else if (	neigh->nud_state != NUD_NONE &&
			neigh->nud_state != NUD_INCOMPLETE){
		ether_addr_copy(eth_hdr(*skb)->h_source, neigh->dev->dev_addr);
		ether_addr_copy(eth_hdr(*skb)->h_dest, neigh->ha);

		if (is_vlan_dev(neigh->dev)) {
			vlan_eth_hdr(*skb)->h_vlan_proto =
				vlan_dev_vlan_proto(neigh->dev);
			vlan_eth_hdr(*skb)->h_vlan_TCI =
				htons(vlan_dev_vlan_id(neigh->dev));
			vlan_eth_hdr(*skb)->h_vlan_encapsulated_proto =
				htons(eth_proto);
		} else {
			eth_hdr(*skb)->h_proto = htons(eth_proto);
		}

		return CA_E_OK;

	} else {
		printk(KERN_WARNING "can't find destination neighbour.\n");
		return CA_E_NOT_FOUND;
	}
}

static ca_status_t
__get_dst_entry_by_key(	CA_IN ca_ip_afi_t afi,
			CA_IN struct flowi key,
			CA_OUT struct dst_entry **dst)
{
	extern struct net init_net;
	struct rtable *rt = NULL;

	if (!dst || *dst)
		return CA_E_PARAM;

	if (afi == CA_IPV4) {
		rt = ip_route_output_key(&init_net, (struct flowi4 *)&key);
		if (!rt || IS_ERR(rt)) {
			printk(KERN_ERR "%s %d: No route for ipv4\n",
			       __func__, __LINE__);
			return CA_E_PARAM;
		}
		*dst = &rt->dst;
	} else {
		*dst = ip6_route_lookup(&init_net, (struct flowi6 *)&key,
					RT6_LOOKUP_F_IFACE);
		if (!(*dst) || IS_ERR(*dst) || (*dst)->error) {
			printk(KERN_ERR "%s %d: No route for ipv6\n",
				__func__, __LINE__);
			return CA_E_PARAM;
		}
	}

	return CA_E_OK;
}

static struct flowi
__get_flowi_key(CA_IN ca_ip_address_t src_ip,
		CA_IN ca_ip_address_t dst_ip)
{
	struct flowi key;
	ca_uint8_t i;
	ca_ip_address_t src_ip_net, dst_ip_net;

	memset(&key, 0, sizeof(key));

	for (i = 0; i < 4; i++) {
		src_ip_net.ip_addr.addr[i] = htonl(src_ip.ip_addr.addr[i]);
		dst_ip_net.ip_addr.addr[i] = htonl(dst_ip.ip_addr.addr[i]);
	}

	if (src_ip.afi == CA_IPV4) {
		((struct flowi4 *)&key)->flowi4_oif = 0;
		((struct flowi4 *)&key)->flowi4_proto = IPPROTO_UDP;
		((struct flowi4 *)&key)->flowi4_flags = FLOWI_FLAG_ANYSRC;
		((struct flowi4 *)&key)->flowi4_tos = (RT_TOS(0));
		((struct flowi4 *)&key)->saddr = src_ip_net.ip_addr.ipv4_addr;
		((struct flowi4 *)&key)->daddr = dst_ip_net.ip_addr.ipv4_addr;
	} else {
		((struct flowi6 *)&key)->flowi6_proto = IPPROTO_UDP;
		((struct flowi6 *)&key)->flowi6_flags = FLOWI_FLAG_ANYSRC;
		((struct flowi6 *)&key)->saddr = *(struct in6_addr*)
						&src_ip_net.ip_addr.addr;
		((struct flowi6 *)&key)->daddr = *(struct in6_addr*)
						&dst_ip_net.ip_addr.addr;
	}

	return key;
}

static ca_status_t
__get_eth_by_ip(CA_IN ca_ip_address_t src_ip,
		CA_IN ca_ip_address_t dst_ip,
		CA_OUT struct sk_buff **skb)
{
	struct neighbour *neigh = NULL;
	struct dst_entry *dst = NULL;
	struct flowi key;
	ca_status_t ret = CA_E_OK;

	if (!skb || *skb)
		return CA_E_PARAM;

	key = __get_flowi_key(src_ip, dst_ip);

	ret = __get_dst_entry_by_key(src_ip.afi, key, &dst);
	if (ret != CA_E_OK)
		return ret;

	ret = __set_dst_entry(dst, skb);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s-%s():set_entry failed:0x%x\n",
			__FILE__, __FUNCTION__, ret);
		return ret;
	}

	if (src_ip.afi == CA_IPV4) {
		neigh = dst_neigh_lookup(dst,
					&(((struct flowi4 *)&key)->daddr));
		ret = __get_eth_by_neighbour(neigh, ETH_P_IP, skb);
	} else {
		neigh = dst_neigh_lookup(dst,
					&(((struct flowi6 *)&key)->daddr));
		ret = __get_eth_by_neighbour(neigh, ETH_P_IPV6, skb);
	}
	neigh_release(neigh);

	return ret;
}

ca_status_t
__hton_ipaddr(CA_IN ca_l3_ip_addr_t *hostip,
	      CA_OUT ca_l3_ip_addr_t *netip)
{
	ca_uint8_t i;

	if (!hostip || !netip)
		return CA_E_PARAM;

	for (i = 0; i < 4; i++)
		netip->addr[i] = htonl(hostip->addr[i]);

	return CA_E_OK;
}

#ifndef GET_INTF_FROM_L2
static ca_status_t
__get_intf_info_by_dst_mac(CA_IN ca_mac_addr_t dst_mac,
			   CA_IN ca_ip_afi_t   afi,
			   CA_OUT ca_port_id_t *port_id,
			   CA_OUT ca_uint16_t  *vid)
{
	ca_iterator_t return_entry;
	ca_l3_nexthop_t l3_entry = {0};
	ca_l3_intf_t l3_intf = {0};
	ca_status_t ret;

	if (!dst_mac || !port_id || !vid)
		return CA_E_PARAM;

	memset(&return_entry, 0, sizeof(ca_iterator_t));
	memset(&l3_entry, 0, sizeof(l3_entry));
	return_entry.entry_count = 1;
	return_entry.user_buffer_size =  sizeof(l3_entry);
	return_entry.entry_data = &l3_entry;

	while (1) {
		memset(&l3_entry, 0, sizeof(l3_entry));
		ret = ca_l3_nexthop_iterate(0, afi, &return_entry);
		if (ret != CA_E_OK){
			printk(KERN_ERR "Iterate RET %d\n", ret);
			return ret;
		}
		TPUT_DBG(printk(KERN_DEBUG "ret cnt %d, prevhndl %p\n",
				return_entry.entry_count,
				return_entry.prev_handle));
		TPUT_DBG(printk(KERN_DEBUG "fdb mac %pM\n",
				l3_entry.da_mac));
		if (memcmp(dst_mac, l3_entry.da_mac, 6) == 0) {
			l3_intf.intf_id = l3_entry.intf_id;
			ret = ca_l3_intf_get(0, &l3_intf);
			if (ret != CA_E_OK) {
				printk(KERN_ERR "Interface_Get RET %d\n", ret);
				return ret;
			}
			if ((*vid == UNSPEC_U16 && l3_intf.outer_vid > 0xfff) ||
				*vid == l3_intf.outer_vid) {
				TPUT_DBG(printk(KERN_DEBUG
					"GOT mac %pM, vid %d\n",
					l3_entry.da_mac,
					l3_intf.outer_vid
				));
				break;
			}
		}
	}

	*port_id = l3_intf.port_id;
	if (l3_intf.outer_vid <= 0xfff)
		*vid = l3_intf.outer_vid;
	else
		*vid = UNSPEC_U16;

	return CA_E_OK;
}
#endif

static ca_status_t
__fill_info_by_ip(	CA_IN ca_ip_address_t src_ip,
			CA_IN ca_ip_address_t dst_ip,
			CA_OUT struct sk_buff **skb,
			CA_OUT ca_uint16_t *eth_proto,
			CA_OUT ca_uint16_t *vid,
			CA_OUT ca_port_id_t *port_id)
{
	ca_status_t ret = CA_E_OK;
	aal_fdb_entry_cfg_t fdb_entry;

	memset(&fdb_entry, 0, sizeof(fdb_entry));
	ret = __get_eth_by_ip(src_ip, dst_ip, skb);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s %d get mac error:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}
	if (src_ip.afi == CA_IPV4)
		*eth_proto = htons(ETH_P_IP);
	else
		*eth_proto = htons(ETH_P_IPV6);

	/* Get ldpid by MAC DA */
	if (eth_hdr(*skb)->h_proto != *eth_proto)
		*vid = ntohs(vlan_eth_hdr(*skb)->h_vlan_TCI) & VLAN_VID_MASK;
	else
		*vid = UNSPEC_U16;

#ifdef GET_INTF_FROM_L2
	ret = aal_fdb_entry_get(0, eth_hdr(*skb)->h_dest,
				(*vid) & VLAN_VID_MASK, &fdb_entry);
	if (ret == CA_E_OK) {
		*port_id = PORT_ID(fdb_entry.port_id);
	} else {
		/* TODO: there's no established fdb table for multiple llid */
		printk(KERN_WARNING "%s %d get fdb entry fail:%d\n",
			__func__, __LINE__, ret);
		ret = CA_E_OK;
	}
#else
	ret = __get_intf_info_by_dst_mac(eth_hdr(*skb)->h_dest, src_ip.afi,
						port_id, vid);
#endif

	if (ret != CA_E_OK)
		printk(KERN_ERR "%s %d get port error:%d\n",
			__func__, __LINE__, ret);

	return ret;
}

#ifdef SFU_PON_MODE
static ca_uint8_t
__scfg_wan_id(void)
{
	ca_device_config_tlv_t scfg_tlv;

	if (__read_scfg((ca_uint32_t)CA_CFG_ID_WAN_PORT_ID,
			sizeof(ca_uint8_t), &scfg_tlv)
		!= CA_E_OK) {
		printk(KERN_WARNING
			"%s:%d: fail to get %s, use 0x7 instead.\n",
			__func__, __LINE__, "CFG_ID_WAN_PORT_ID");
		return 0x7;
	} else {
		return scfg_tlv.value[0];
	}
}

static ca_uint8_t
__check_is_wan_port(CA_IN ca_port_id_t port_id)
{
	if (	(port_id == __scfg_wan_id()) ||
		(port_id == AAL_LPORT_L3_WAN) ||
		((port_id >= 0x20) &&
		 (port_id <= 0x20 + CA_MAX_EPON_PORT_LLID_NUMBER)))
		return 1;
	else
		return 0;
}

static ca_status_t
__select_port_id_by_pon_mode(	CA_IN ca_port_id_t *port_id,
				CA_IN ca_mac_addr_t dst_mac)
{
	ca_uint8_t pon_mode;
	ca_status_t ret = CA_E_OK;
	ca_uint16_t usr_pon_idx = UNSPEC_U16;
	ca_uint32_t llid_index = UNSPEC_U16;

	if (!port_id)
		return CA_E_PARAM;

	if (__check_is_wan_port(*port_id) == 0 && *port_id != UNSPEC_U16)
		return CA_E_OK;

	ret = __pon_mac_mode_get(&pon_mode);
	if (ret != CA_E_OK)
		return ret;

	usr_pon_idx = (ca_uint16_t) readl(PE1_CTRL_BASED + TPUT_OFFSET_PON_ID);

	switch (pon_mode) {
		case ONU_PON_MAC_EPON_1G:
		case ONU_PON_MAC_EPON_D10G:
		case ONU_PON_MAC_EPON_BI10G:
		case ONU_PON_MAC_TURBO_EPON:
			/* EPON mode */
			if (usr_pon_idx != UNSPEC_U16) {
				*port_id = usr_pon_idx + 0x20;
			} else if (ca_tx_get_llid(0, dst_mac, &llid_index) ==
					CA_E_OK) {
				*port_id = llid_index + 0x20;
			} else if (*port_id != UNSPEC_U16) {
				printk(KERN_WARNING "%s %d:get llid fail.\n",
					__func__, __LINE__);
			} else {
				printk(KERN_WARNING
					"%s %d:get llid fail, fdb fail.\n",
					__func__, __LINE__);
				*port_id = 0x20;
			}
			break;
		case ONU_PON_MAC_GPON:
		case ONU_PON_MAC_XGPON1:
		case ONU_PON_MAC_XGSPON:
		case ONU_PON_MAC_NGPON2:
			/* TODO:GPON mode */
			if (usr_pon_idx != UNSPEC_U16) {
				*port_id = usr_pon_idx + 0x20;
			} else if (*port_id != UNSPEC_U16) {
				printk(KERN_WARNING "%s %d:get gem_id fail.\n",
					__func__, __LINE__);
		} else {
				printk(KERN_WARNING
					"%s %d:get gem_id fail, fdb fail.\n",
					__func__, __LINE__);
				*port_id = 0x20;
		}
			break;
		case ONU_PON_MAC_ETHERNET:
			break;
		default:
			ret = CA_E_NOT_FOUND;
	}

	if (*port_id == UNSPEC_U16)
		ret = CA_E_NOT_FOUND;

	return ret;
}
#endif

ca_status_t
ca_tput_upstream_test_start(CA_IN ca_device_id_t device_id,
			    CA_IN ca_uint8_t flow_count,
			    CA_IN ca_tput_upstream_flow_spec_t * flows)
{
	struct ethhdr *eth = NULL;
	struct vlan_ethhdr *vlan_eth = NULL;
	struct iphdr ip_header, *iph = &ip_header;
	struct iphdr *iph_pkt_ctx;
	void *l4hdr;
	ca_uint16_t vid = UNSPEC_U16;
	ca_port_id_t port_id = UNSPEC_U16;
	ca_uint8_t cur, ip_hdr_len, l2_hdr_len;
	ca_uint32_t total_pkt_len;
	ca_uint64_t pe_ctrl_base;
	ca_port_id_t phy_port;
#ifdef IPERF_TPUT
	ca_uint32_t j;
	unsigned char *combined_ptr=NULL;
#endif
#ifdef CONFIG_IPV6
	struct ipv6hdr ip6_header, *ip6h = &ip6_header;
#endif
	ca_uint16_t mtu = 0;
	ca_uint16_t eth_proto = 0;
	struct sk_buff *skb = NULL;
	ca_status_t ret = CA_E_OK;

	spin_lock(&ca_tput_lock);
	if ((flow_count == 0) || (flow_count > CA_MAX_TPUT_FLOW_COUNT)) {
		spin_unlock(&ca_tput_lock);
		printk("%s %d flow count=%d err=CA_E_PARAM", __func__, __LINE__,
		       flow_count);
		return CA_E_PARAM;
	}

	if (upstream_active != 0) {
		spin_unlock(&ca_tput_lock);
		printk("%s %d upstream_active != 0 err=CA_E_BUSY", __func__,
		       __LINE__);
		return CA_E_BUSY;
	}

	memset(iph, 0, sizeof(struct iphdr));

	/*clear counters */
	writel(0, PE1_MIB_TX_PKT_CNT_ADDR);
	writel(0, PE1_MIB_TX_BYTE_CNT_ADDR);

	/*fill packet content */
	cur = 0;
	while (cur < flow_count) {
		priv_upstream_flow[cur].tx_count = 0;
		memcpy(&priv_upstream_flow[cur], flows,
		       sizeof(ca_tput_upstream_flow_spec_t));
		if (flows->src_ip_addr.afi != flows->dst_ip_addr.afi) {
			spin_unlock(&ca_tput_lock);
			printk("%s %d afi is not equal err=CA_E_PARAM",
			       __func__, __LINE__);
			return CA_E_PARAM;
		}
#ifndef CONFIG_IPV6
		if (flows->src_ip_addr.afi == CA_IPV6) {
			spin_unlock(&ca_tput_lock);
			printk("%s %d CA_IPV6 err=CA_E_PARAM", __func__,
			       __LINE__);
			return CA_E_PARAM;
		}
#endif
		pe_ctrl_base = (ca_uint64_t) PE1_CTRL_BASED;

#ifdef USE_DDR_PKT_BUF
		eth = (struct ethhdr *)
			((ca_uint64_t) (ca_tput_ddr_base + TPUT_PKT_HEADROOM));
#else
		eth = (struct ethhdr *)
			((ca_uint64_t) (pe_ctrl_base + TPUT_OFFSET_PKT_CTX +
					TPUT_PKT_HEADROOM));
#endif

		/* DST IP -> get MAC -> lookup FDB to get port */
		ret = __fill_info_by_ip(flows->src_ip_addr,
					flows->dst_ip_addr,
					&skb, &eth_proto, &vid, &port_id);
		if (ret != CA_E_OK) {
			kfree_skb(skb);
			spin_unlock(&ca_tput_lock);
			printk(KERN_ERR "%s %d fill_info_by_ip error:%d\n",
				__func__, __LINE__, ret);
			return ret;
		}

#ifdef SFU_PON_MODE
		ret = __select_port_id_by_pon_mode(
				&port_id, eth_hdr(skb)->h_dest);
		if (ret != CA_E_OK) {
			spin_unlock(&ca_tput_lock);
			printk(KERN_ERR
				"%s:%d: Fail to select port_id code:%d.\n",
				__func__, __LINE__, ret);
			return ret;
		}
#endif

		mtu = (ca_uint16_t)skb->dev->mtu;
		TPUT_DBG(printk(KERN_DEBUG "MTU=%d Total_len=%d\n",
				mtu, flows->tot_len));
		if (flows->tot_len > mtu) {
			kfree_skb(skb);
			spin_unlock(&ca_tput_lock);
			printk(KERN_ERR
				"Total length(%d) larger than MTU size(%d)!\n",
				flows->tot_len, mtu);
			return CA_E_PARAM;
		}

		l2_hdr_len = 14 + ((vid != UNSPEC_U16) ? 4 : 0);
		ip_hdr_len = (flows->src_ip_addr.afi == CA_IPV4) ? 20 : 40;
		total_pkt_len = flows->tot_len + l2_hdr_len;
		memset_io(eth, 0, total_pkt_len);

		memcpy_toio(eth, eth_hdr(skb), sizeof(struct ethhdr));
		TPUT_DBG(printk("DA %pM SA %pM\n",
				eth->h_dest, eth->h_source));
		kfree_skb(skb);

		if (vid == UNSPEC_U16) {
			eth->h_proto = eth_proto;
			iph_pkt_ctx =
			    (struct iphdr *)((ca_uint64_t) eth + l2_hdr_len);
		} else {
			vlan_eth = (struct vlan_ethhdr *)eth;
			vlan_eth->h_vlan_proto = htons(ETH_P_8021Q);
			vlan_eth->h_vlan_TCI = htons(vid);
			vlan_eth->h_vlan_encapsulated_proto =
			    eth_proto;
			iph_pkt_ctx =
			    (struct iphdr *)((ca_uint64_t) eth + l2_hdr_len);
		}
		TPUT_DBG(printk("port_id:0x%x vid:0x%x eth_proto:0x%x\n",
				port_id, vid, eth_proto));

		iph->version = (flows->src_ip_addr.afi == CA_IPV4) ? 4 : 6;
		if (flows->src_ip_addr.afi == CA_IPV4) {
			iph->ihl = 5;
			iph->saddr =
			    ntohl(flows->src_ip_addr.ip_addr.ipv4_addr);
			iph->daddr =
			    ntohl(flows->dst_ip_addr.ip_addr.ipv4_addr);
			iph->protocol = flows->ip_proto;
			iph->tos = flows->tos;
			iph->id = htons(flows->id);
			iph->frag_off = flows->frag_off;
			iph->ttl = flows->ttl;
			iph->tot_len = htons(flows->tot_len);
			iph->check =
			    ip_fast_csum((unsigned char *)iph, iph->ihl);
		}
#ifdef CONFIG_IPV6
		ip6h->version = (flows->src_ip_addr.afi == CA_IPV4) ? 4 : 6;
		if (flows->src_ip_addr.afi == CA_IPV6) {
			iph = (struct iphdr *)ip6h;
			ip6h->flow_lbl[0] = 0x1; /* assign a non-zero value TODO */
			ip6h->payload_len = htons(flows->tot_len - 40);
			ip6h->nexthdr = flows->ip_proto; /* udp */
			ip6h->hop_limit = 0x40; /* follow iperf3 usage TODO */
			__hton_ipaddr(&flows->src_ip_addr.ip_addr,
					(ca_l3_ip_addr_t *)&ip6h->saddr);
			__hton_ipaddr(&flows->dst_ip_addr.ip_addr,
					(ca_l3_ip_addr_t *)&ip6h->daddr);
		}
#endif
		l4hdr = ((void *)iph_pkt_ctx + ip_hdr_len);
		if (__ca_fill_ip_protocol(iph, l4hdr, flows) != CA_E_OK) {
			spin_unlock(&ca_tput_lock);
			printk("%s %d __ca_fill_ip_protocol err=CA_E_PARAM\n",
			       __func__, __LINE__);
			return CA_E_PARAM;
		}
#ifndef IPERF_TPUT
		memcpy_toio(iph_pkt_ctx, iph, ip_hdr_len);
		ret =
		    __convert_mbps_to_delay_counter(flows->tx_rate,
						    total_pkt_len,
						    (flows->tot_len -
						     ip_hdr_len), &i,
						    flows->tx_count);

		if (ret != CA_E_OK) {
			spin_unlock(&ca_tput_lock);
			return ret;
		}

		writel(i, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_INTERVAL));
#else
		for (j=0;j<16;j++) {
			combined_ptr=(unsigned char *)iph_pkt_ctx+(0x680*j);
			memcpy_toio(combined_ptr,iph, ip_hdr_len);
			TPUT_DBG(printk("IPERF: iph_pkt_ctx:%p, iph:%p length:%d\n",
				combined_ptr, iph, ip_hdr_len));
		}

		writel(flows->tx_rate, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_INTERVAL));
#endif
		writel(flows->tx_count, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_PKT_NUM));
		writel(total_pkt_len, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_PKT_SIZE));

		phy_port = PORT_ID(port_id);
		if (phy_port == AAL_LPORT_L3_WAN)
			phy_port = wan_port_id;

#ifdef IPERF_TPUT
		writel(phy_port, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_PHY_PORT));
#else
		if (phy_port >= AAL_LPORT_ETH_NI0
		    && phy_port <= AAL_LPORT_ETH_NI7) {
			writel(phy_port, (volatile void *)(pe_ctrl_base + TPUT_OFFSET_PHY_PORT));
		} else {
			spin_unlock(&ca_tput_lock);
			printk
			    ("%s %d Outgoing port_id=0x%x phy_port %d err=CA_E_PARAM\n",
			     __func__, __LINE__, port_id, phy_port);
			return CA_E_PARAM;
		}
#endif

		flows++;
		cur++;
	}
#ifndef IPERF_TPUT
	/*start the test */
	writel(1, (volatile void *)(PE1_CTRL_BASED + TPUT_OFFSET_TX_ENABLE));
#endif

	upstream_active = flow_count;
	spin_unlock(&ca_tput_lock);
	return CA_E_OK;
}

EXPORT_SYMBOL(ca_tput_upstream_test_start);

ca_status_t
ca_tput_upstream_test_stats_get(CA_IN ca_device_id_t device_id,
				CA_OUT ca_uint32_t * p_packet_sent)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t i, total_waiting_pkt = 0;

	spin_lock(&ca_tput_lock);
	if (upstream_active == 0) {
		spin_unlock(&ca_tput_lock);
		return CA_E_INACTIVE;
	}

	*p_packet_sent = readl(PE1_MIB_TX_PKT_CNT_ADDR);

	for (i = 0; i < CA_MAX_TPUT_FLOW_COUNT; i++) {
		total_waiting_pkt += priv_upstream_flow[i].tx_count;
	}

	if (*p_packet_sent < (total_waiting_pkt)) {
		if (time_is_before_jiffies(tx_jiffies_timeout) | 1) {
			TPUT_DBG(printk
			    ("TIME_OUT: packet_sent=0x%x wait flow %d %ld curr time %ld \n",
			     *p_packet_sent, total_waiting_pkt,
			     tx_jiffies_timeout, jiffies));
			upstream_active = 0;
		}
		ret = CA_E_BUSY;
	} else {
		upstream_active = 0;
		ret = CA_E_OK;
	}

	spin_unlock(&ca_tput_lock);
	return ret;
}

EXPORT_SYMBOL(ca_tput_upstream_test_stats_get);

#ifdef SFU_PON_MODE
static ca_status_t
__set_pon_cls_key(	CA_IN ca_mac_addr_t dst_mac,
			CA_IN ca_uint32_t default_pon_idx,
			CA_OUT ca_classifier_key_t *key)
{
	ca_uint8_t pon_mode;
	ca_uint32_t usr_pon_idx = UNSPEC_U16;
	ca_uint32_t llid_index = UNSPEC_U16;
	ca_status_t stats;

	if (!key) {
		printk(KERN_ERR "%s %d null input key.\n", __func__, __LINE__);
		return CA_E_PARAM;
	}

	stats = __pon_mac_mode_get(&pon_mode);
	if (stats != CA_E_OK)
		return stats;

	usr_pon_idx = (ca_uint32_t) readl(PE1_CTRL_BASED + TPUT_OFFSET_PON_ID);

	/* set cls key.handle_type based on pon_mac_mode */
	switch (pon_mode) {
		case ONU_PON_MAC_EPON_1G:
		case ONU_PON_MAC_EPON_D10G:
		case ONU_PON_MAC_EPON_BI10G:
		case ONU_PON_MAC_TURBO_EPON:
			/* EPON mode */
			key->handle_type = CA_KEY_HANDLE_TYPE_LLID_COS;
			if (usr_pon_idx != UNSPEC_U16) {
				key->key_handle.llid_cos_index =
					usr_pon_idx << 11;
			} else if (ca_tx_get_llid(0, dst_mac, &llid_index) ==
					CA_E_OK) {
				key->key_handle.llid_cos_index =
					llid_index << 11;
			} else if (default_pon_idx != UNSPEC_U16) {
				printk(KERN_WARNING "%s %d not found llid\n",
					__func__, __LINE__);
				key->key_handle.llid_cos_index =
					default_pon_idx << 11;
			} else {
				printk(KERN_WARNING "%s %d:%s\n", __func__,
					__LINE__, "not found llid, fdb fail");
				key->key_handle.llid_cos_index = 0x20 << 11;
			}
			break;
		case ONU_PON_MAC_GPON:
		case ONU_PON_MAC_XGPON1:
		case ONU_PON_MAC_XGSPON:
		case ONU_PON_MAC_NGPON2:
			/* TODO:GPON mode */
			key->handle_type = CA_KEY_HANDLE_TYPE_GEM_INDEX;
			if (usr_pon_idx != UNSPEC_U16) {
				key->key_handle.gem_index = usr_pon_idx;
			} else if (default_pon_idx != UNSPEC_U16) {
				printk(KERN_WARNING "%s %d not found gem_id\n",
					__func__, __LINE__);
				key->key_handle.gem_index = default_pon_idx;
			} else {
				printk(KERN_WARNING "%s %d:%s\n",
					__func__, __LINE__,
					"not found gem_id, fdb fail");
				key->key_handle.gem_index = 0x20;
			}
			break;
		case ONU_PON_MAC_ETHERNET:
			if (default_pon_idx == UNSPEC_U16)
				return CA_E_NOT_FOUND;
			key->handle_type = CA_KEY_HANDLE_TYPE_FLOW_ID;
			key->key_handle.flow_id = 0xFF;
			break;
		default:
			/* TODO: need to design this case */
			printk(KERN_ERR "%s:%d: wrong PON_MAC_MODE(%u).\n",
				__func__, __LINE__, pon_mode);
			return CA_E_NOT_FOUND;
	}
	return CA_E_OK;
}
#endif

static ca_status_t
__bound_src_port(CA_OUT ca_port_id_t *port_id)
{
	if (!port_id)
		return CA_E_PARAM;

	if (CA_PORT_ID_NI7 != CA_PORT_INDEX_INVALID) {
		if (*port_id > CA_PORT_ID_NI7) {
			printk(KERN_WARNING "%s %d: %s:0x%x, %s\n",
				__func__, __LINE__, "invalid port_id",
				*port_id, "use 7 instead.");
			*port_id = CA_PORT_ID_NI7;
		}
	} else if (*port_id > CA_PORT_ID_NI4) {
	/* G3Lite case only have CA_PORT_ID_NI4 for maximum */
		printk(KERN_WARNING "%s %d: %s:0x%x, %s\n",
			__func__, __LINE__, "invalid port_id",
			*port_id, "use 4 instead.");
		*port_id = CA_PORT_ID_NI4;
	}

	return CA_E_OK;
}

#ifdef USE_CLS_FOR_RX
static ca_status_t
__redirect_pkt_rule_add(CA_IN ca_tput_downstream_flow_spec_t * flows,
			CA_IN ca_mac_addr_t dst_mac,
			CA_IN ca_uint16_t vid,
			CA_IN ca_port_id_t port_id,
			CA_IN ca_uint8_t cur)
{
	ca_classifier_key_t key;
	ca_classifier_key_mask_t key_mask;
	ca_classifier_action_t	action;
	ca_uint32_t rule_index;
	ca_status_t ret = CA_E_OK;

	memset(&key, 0, sizeof(ca_classifier_key_t));
	memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
	memset(&action, 0, sizeof(ca_classifier_action_t));

	key.key_handle.flow_id = 0xFF;

#ifdef SFU_PON_MODE
	/* EPON multiple llid isn't established in fdb table */
	if (__check_is_wan_port(port_id) || port_id == UNSPEC_U16) {
		ret = __set_pon_cls_key(dst_mac, (ca_uint32_t)port_id, &key);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s %d set pon_mode classifer fail\n",
				__func__, __LINE__);
			return ret;
		}
		port_id = __scfg_wan_id();
	}
#endif

	__bound_src_port(&port_id);

	/* setup cls key, address */
	if (port_id == CA_PORT_ID_NI7)
		key.src_port = CA_PORT_ID(CA_PORT_TYPE_EPON, port_id);
	else
		key.src_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port_id);
	memcpy(&key.ip.ip_sa, &flows->src_ip_addr, sizeof(key.ip.ip_sa));
	key.l4.src_port.min = key.l4.src_port.max = flows->l4_src_port;
	key.l4.dst_port.min = key.l4.dst_port.max = flows->l4_dst_port;
	if (vid != UNSPEC_U16) {
		key.l2.vlan_otag.vlan_min.vid = vid;
		key.l2.vlan_otag.vlan_max.vid = vid;
	}

	/* setup cls key mask */
	key_mask.src_port		= 1;
	key_mask.key_handle		= 1;
	key_mask.ip			= 1;
	key_mask.ip_mask.ip_sa		= 1;
	key_mask.l4			= 1;
	key_mask.l4_mask.dst_port	= 1;
	key_mask.l4_mask.src_port	= 1;
	if (vid != UNSPEC_U16) {
		key_mask.l2				= 1;
		key_mask.l2_mask.vlan_otag		= 1;
		key_mask.l2_mask.vlan_otag_mask.vid	= 1;
	}

	/* setup cls action */
	action.forward		= CA_CLASSIFIER_FORWARD_PORT;
	action.dest.port	= CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU7);
	ret = ca_classifier_rule_add(0, 7, &key, &key_mask, &action, &rule_index);

	if (ret != CA_E_OK) {
		printk("%s: classifier_rule_add() failed!!, ret=%d\n", __func__, ret);
	}

	//printk("%s: rule_index=%d\n", __func__, rule_index);
	downstream_flow_id[cur] = rule_index;

	return ret;
}
#else
static ca_status_t
__redirect_pkt_rule_add(CA_IN ca_tput_downstream_flow_spec_t * flows,
			CA_IN ca_mac_addr_t dst_mac,
			CA_IN ca_uint16_t vid,
			CA_IN ca_port_id_t port_id,
			CA_IN ca_uint8_t cur)
{
	aal_hash_key_t key = { 0 }
	, *hash_key = &key;
	aal_hash_action_t action = { 0 }
	, *hash_action = &action;
	ca_uint32_t h_idx, *hash_idx = &h_idx;
	ca_uint8_t profile_id;
	ca_status_t rc = CA_E_OK;

	if (port_id == CA_PORT_ID_L3_LAN)
		profile_id = l3fe_main_hash_profile_lan;
	else
		profile_id = l3fe_main_hash_profile_wan;

		/*create hash */
		hash_key->ip_vld = 1;
		if (flows->dst_ip_addr.afi == CA_IPV4) {
			hash_key->ip_ver = 0;
			hash_key->ip_da_0 =
			    flows->dst_ip_addr.ip_addr.ipv4_addr;
			hash_key->ip_da_1 = 0;
			hash_key->ip_da_2 = 0;
			hash_key->ip_da_3 = 0;
			hash_key->ip_sa_0 =
			    flows->src_ip_addr.ip_addr.ipv4_addr;
			hash_key->ip_sa_1 = 0;
			hash_key->ip_sa_2 = 0;
			hash_key->ip_sa_3 = 0;
		} else {
			hash_key->ip_ver = 1;
			hash_key->ip_da_0 = flows->dst_ip_addr.ip_addr.addr[3];
			hash_key->ip_da_1 = flows->dst_ip_addr.ip_addr.addr[2];
			hash_key->ip_da_2 = flows->dst_ip_addr.ip_addr.addr[1];
			hash_key->ip_da_3 = flows->dst_ip_addr.ip_addr.addr[0];
			hash_key->ip_sa_0 = flows->src_ip_addr.ip_addr.addr[3];
			hash_key->ip_sa_1 = flows->src_ip_addr.ip_addr.addr[2];
			hash_key->ip_sa_2 = flows->src_ip_addr.ip_addr.addr[1];
			hash_key->ip_sa_3 = flows->src_ip_addr.ip_addr.addr[0];
		}

		hash_key->ip_protocol = flows->ip_proto;
		hash_key->l4_sp_exact_range = flows->l4_src_port;
		hash_key->l4_dp_exact_range = flows->l4_dst_port;
	hash_key->ctrl_set_id = profile_id;

		hash_action->dpid_vld = 1;
		hash_action->dpid_pri = 1;
		hash_action->permit = 1;
		hash_action->mc = 0;
#ifdef IPERF_TPUT
	hash_action->mcgid = 0x16;	// PE0
#else
		hash_action->mcgid = 0x17;	// PE1
#endif

		hash_action->t5_ctrl_vld = 1;
		hash_action->t5_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;

	rc = aal_hash_add(0, hash_key, hash_action, TUPLE_TYPE_FWD,
				  HASH_AGING_STATIC, hash_mask_nat_idx,
			profile_id, hash_idx);
	if (rc != CA_E_OK)
			return rc;
		downstream_flow_id[cur] = h_idx;
	return CA_E_OK;
}
#endif

ca_status_t
ca_tput_downstream_test_start(CA_IN ca_device_id_t device_id,
			      CA_IN ca_uint8_t flow_count,
			      CA_OUT ca_tput_downstream_flow_spec_t * flows)
{
	ca_uint8_t cur = 0;
	struct sk_buff *skb = NULL;
	ca_uint16_t eth_proto;
	ca_uint16_t vid = UNSPEC_U16;
	ca_port_id_t port_id = UNSPEC_U16;
	ca_status_t rc = CA_E_OK;

	if ((flow_count == 0) || (flow_count > CA_MAX_TPUT_FLOW_COUNT)) {
		printk("%s %d flow count=%d err=CA_E_PARAM", __func__, __LINE__,
		       flow_count);
		return CA_E_PARAM;
	}

	spin_lock(&ca_tput_lock);

#ifndef IPERF_TPUT
	if (downstream_active != 0) {
		spin_unlock(&ca_tput_lock);
		return CA_E_BUSY;
	}
	/*clear counters */
	writel(0, (volatile void *) PE1_MIB_RX_PKT_CNT_ADDR);
	writel(0, (volatile void *) PE1_MIB_RX_BYTE_CNT_ADDR);
	writel(1, (volatile void *) (PE1_CTRL_BASED + TPUT_OFFSET_RX_ENABLE));
#else
	switch ((flows->l4_dst_port) & 1) {
		case 1:
			writel(0, (volatile void *) PE1_MIB_RX_PKT_CNT_ADDR);
			writel(0, (volatile void *) PE1_MIB_RX_BYTE_CNT_ADDR);
			writel(1, (volatile void *) (PE1_CTRL_BASED + TPUT_OFFSET_RX_ENABLE));
			break;
		default:
			// PE0 if port is not 5201 for example (or is 0,2 technically)
			writel(0, (volatile void *) PE0_MIB_RX_PKT_CNT_ADDR);
			writel(0, (volatile void *) PE0_MIB_RX_BYTE_CNT_ADDR);
			writel(1, (volatile void *) (PE1_CTRL_BASED + TPUT_OFFSET_RX1_ENABLE));
			break;
	}
#endif
	rc = __fill_info_by_ip(flows->dst_ip_addr,
				flows->src_ip_addr,
				&skb, &eth_proto, &vid, &port_id);
	if (rc != CA_E_OK) {
		kfree_skb(skb);
		spin_unlock(&ca_tput_lock);
		printk(KERN_ERR "%s %d fill_info_by_ip error:%d\n",
			__func__, __LINE__, rc);
		return rc;
	}
	port_id = PORT_ID(port_id);
	if (port_id == AAL_LPORT_L3_WAN)
		port_id = wan_port_id;

	while (cur < flow_count) {
		priv_downstream_flow[cur+downstream_active].rx_count = 0;
		memcpy(&priv_downstream_flow[cur+downstream_active], flows,
		       sizeof(ca_tput_downstream_flow_spec_t));

		rc = __redirect_pkt_rule_add(flows, eth_hdr(skb)->h_dest,
					vid, port_id, cur+downstream_active);
		if (rc != CA_E_OK) {
			spin_unlock(&ca_tput_lock);
			return rc;
		}

		flows++;
		cur++;
	}

	kfree_skb(skb);
	downstream_active += flow_count;
	downstream_rest_active_cnt++;
	spin_unlock(&ca_tput_lock);
	return CA_E_OK;
}

EXPORT_SYMBOL(ca_tput_downstream_test_start);

ca_status_t
ca_tput_downstream_test_stats_get(CA_IN ca_device_id_t device_id,
				  CA_OUT ca_uint32_t * p_packet_received)
{
	ca_uint32_t i, total_waiting_pkt = 0;
	ca_status_t ret = CA_E_OK;

	spin_lock(&ca_tput_lock);
	if (downstream_active == 0) {
		spin_unlock(&ca_tput_lock);
		return CA_E_INACTIVE;
	}

	*p_packet_received = readl(PE1_MIB_RX_PKT_CNT_ADDR);

	for (i = 0; i < CA_MAX_TPUT_FLOW_COUNT; i++) {
		total_waiting_pkt += priv_downstream_flow[i].rx_count;
	}

	if (*p_packet_received != (total_waiting_pkt))
		ret = CA_E_BUSY;
	spin_unlock(&ca_tput_lock);
	return ret;
}

EXPORT_SYMBOL(ca_tput_downstream_test_stats_get);

ca_status_t ca_tput_downstream_test_stop(CA_IN ca_device_id_t device_id)
{
	ca_uint8_t i = 0;
	spin_lock(&ca_tput_lock);
	if (downstream_active == 0) {
		spin_unlock(&ca_tput_lock);
		return CA_E_INACTIVE;
	}

	downstream_rest_active_cnt--;
	if (downstream_rest_active_cnt > 0) {
		spin_unlock(&ca_tput_lock);
		return CA_E_OK;
	}
#ifndef USE_CLS_FOR_RX
	while (i < downstream_active) {
		/* delete hash */
		aal_hash_delete(0, downstream_flow_id[i]);
		i++;
	}
#else
	while (i < downstream_active) {
		/* delete hash */
		ca_classifier_rule_delete(0, downstream_flow_id[i]);
		i++;
	}
#endif

	downstream_active = 0;
	spin_unlock(&ca_tput_lock);
	return CA_E_OK;
}

EXPORT_SYMBOL(ca_tput_downstream_test_stop);
