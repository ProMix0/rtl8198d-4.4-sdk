/* Kernel module to match MAC address parameters. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>

#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter/xt_mac.h>
#include <linux/netfilter/x_tables.h>

#if defined(CONFIG_RTK_DEV_AP)
#include <linux/ip.h>
#include <net/dst.h>
#include <net/route.h>
#include <net/arp.h>
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netfilter Core Team <coreteam@netfilter.org>");
MODULE_DESCRIPTION("Xtables: MAC address match");
MODULE_ALIAS("ipt_mac");
MODULE_ALIAS("ip6t_mac");

#if defined(CONFIG_RTK_DEV_AP)
static int equal_header_cache_dest_mac(struct sk_buff *skb, unsigned char *macaddr)
{
	int ret = 0;

	if(!skb || !macaddr) {
		return 0;
	}
	
	if(ether_addr_equal(eth_hdr(skb)->h_dest, macaddr)) {
		return 1;
	}
	else {
		struct dst_entry *dst = skb_dst(skb);
		struct rtable *rt = (struct rtable *)dst;
		struct neighbour *neigh = NULL;
		struct hh_cache *hh = NULL;
		unsigned int nexthop;
    	int seqnum = 0;
	
	#if defined(CONFIG_IPV6)
		if(skb->protocol == htons(ETH_P_IPV6)) {
			if(rt == NULL || ipv6_hdr(skb)==NULL) {
				return 0;
			}
			
			if((neigh = neigh_lookup(&nd_tbl, &(ipv6_hdr(skb)->daddr), dst->dev))) {
				read_lock_bh(&neigh->lock);
			
				if (memcmp(neigh->ha, macaddr, ETH_ALEN) == 0) {
					ret = 1;
				}
				read_unlock_bh(&neigh->lock);
				neigh_release(neigh);
			}
		}
		else
	#endif
		{
			if(ip_hdr(skb) == NULL) {
				return 0;
			}
		
			if(rt == NULL){
				if (ip_route_input(skb, ip_hdr(skb)->daddr,ip_hdr(skb)->saddr, ip_hdr(skb)->tos, skb->dev) != 0){
					printk("\n %s %d\n",__FUNCTION__,__LINE__);
				}
				dst = skb_dst(skb);
				rt = (struct rtable *)dst;

				if(rt == NULL) {
					return 0;
				}
			}

			nexthop = (__force u32) rt_nexthop(rt, ip_hdr(skb)->daddr);
			neigh = __ipv4_neigh_lookup_noref(dst->dev, nexthop);

			if (neigh != NULL) {
				hh = &(neigh->hh);

				if (hh && (neigh->nud_state & NUD_VALID))
				{
					do {
						seqnum = read_seqbegin(&neigh->ha_lock);
						if (memcmp(neigh->ha, macaddr, ETH_ALEN) == 0) {
				      		ret = 1;
						}
					} while (read_seqretry(&neigh->ha_lock, seqnum));
				}
			}
		}
	}
	
	return ret;
}
#endif

static bool mac_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_mac_info *info = par->matchinfo;
	//bool ret;
	int i, invert;
    unsigned char info_addr[ETH_ALEN];
    unsigned char pkt_addr[ETH_ALEN];
#if 1
	if (info->flags & MAC_SRC) {
		invert = !!(info->flags & MAC_SRC_INV);
		if (info->flags & SRC_MASK) {
			for (i=0; i<ETH_ALEN; i++) {
				info_addr[i] = (info->srcaddr[i] & info->srcmask[i]);
				pkt_addr[i] = (eth_hdr(skb)->h_source[i] & info->srcmask[i]);
			}
			if (skb_mac_header(skb) < skb->head
				|| (skb_mac_header(skb) + ETH_HLEN) > skb->data
				|| ((!ether_addr_equal(pkt_addr, info_addr))
				 ^ invert))
				return 0;
		}
		else {
			if (skb_mac_header(skb) < skb->head
				|| (skb_mac_header(skb) + ETH_HLEN) > skb->data
				|| ((!ether_addr_equal(eth_hdr(skb)->h_source, info->srcaddr))
				 ^ invert))
				return 0;
		}
	}
	if (info->flags & MAC_DST) {
		invert = !!(info->flags & MAC_DST_INV);
		if (info->flags & DST_MASK) {
			for (i=0; i<ETH_ALEN; i++) {
				info_addr[i] = (info->dstaddr[i] & info->dstmask[i]);
				pkt_addr[i] = (eth_hdr(skb)->h_dest[i] & info->dstmask[i]);
			}
			if (skb_mac_header(skb) < skb->head
				|| (skb_mac_header(skb) + ETH_HLEN) > skb->data
				|| ((!ether_addr_equal(pkt_addr, info_addr))
				 ^ invert))
				return 0;
		}
		else {
			if (skb_mac_header(skb) < skb->head
				// !eth && !pppoe: 
				|| (((skb_mac_header(skb) + ETH_HLEN) > skb->data) && !((skb_mac_header(skb) == skb->data) && (skb_mac_header(skb) > skb->head+ETH_HLEN)))
			#if defined(CONFIG_RTK_DEV_AP)
				|| ((!equal_header_cache_dest_mac(skb, info->dstaddr)) 
			#else
				|| ((!ether_addr_equal(eth_hdr(skb)->h_dest, info->dstaddr))
			#endif
				 ^ invert))
				return 0;
		}
	}

	return 1;
#else
	if (skb->dev == NULL || skb->dev->type != ARPHRD_ETHER)
		return false;
	if (skb_mac_header(skb) < skb->head)
		return false;
	if (skb_mac_header(skb) + ETH_HLEN > skb->data)
		return false;
	ret  = ether_addr_equal(eth_hdr(skb)->h_source, info->srcaddr);
	ret ^= info->invert;
	return ret;
#endif		
}

static struct xt_match mac_mt_reg __read_mostly = {
	.name      = "mac",
	.revision  = 0,
	.family    = NFPROTO_UNSPEC,
	.match     = mac_mt,
	.matchsize = sizeof(struct xt_mac_info),
	.hooks     = (1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_LOCAL_IN) |
	             (1 << NF_INET_FORWARD),
	.me        = THIS_MODULE,
};

static int __init mac_mt_init(void)
{
	return xt_register_match(&mac_mt_reg);
}

static void __exit mac_mt_exit(void)
{
	xt_unregister_match(&mac_mt_reg);
}

module_init(mac_mt_init);
module_exit(mac_mt_exit);
