 /* Kernel module for matching the range of MAC address
 *
 * chloe_wang <chloe_wang@realsil.com.cn>
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
#include <linux/netfilter/xt_macrange.h>
#include <linux/netfilter/x_tables.h>
#include <linux/ip.h>
#include <net/dst.h>
#include <net/route.h>
#include <net/arp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chloe <chloe_wang@realsil.com.cn>");
MODULE_DESCRIPTION("Xtables: MAC address range match");
MODULE_ALIAS("ipt_macrange");
MODULE_ALIAS("ip6t_macrange");

// return 1:	pass dest mac ranges check
static int compare_header_cache_dest_mac(struct sk_buff *skb, unsigned char *lower_mac, unsigned char *upper_mac)
{
	int ret = 0;
	
	if(!skb || !lower_mac || !upper_mac) {
		return 0;
	}

	if((memcmp(eth_hdr(skb)->h_dest, lower_mac, ETH_ALEN) >= 0) && (memcmp(eth_hdr(skb)->h_dest, upper_mac, ETH_ALEN) <= 0)) {
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
				if((memcmp(neigh->ha, lower_mac, ETH_ALEN) >= 0) && (memcmp(neigh->ha, upper_mac, ETH_ALEN) <= 0)) {
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
						if((memcmp(neigh->ha, lower_mac, ETH_ALEN) >= 0) && (memcmp(neigh->ha, upper_mac, ETH_ALEN) <= 0)) {
				      		ret = 1;
						}
					} while (read_seqretry(&neigh->ha_lock, seqnum));
				}
			}
		}
	}
	
	return ret;
}

static bool mac_range_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_mac_range_info *info = par->matchinfo;   

	if (info->flags & MAC_SRC_RANGE) {
	     /* Is mac pointer valid? */
	    if (skb_mac_header(skb) < skb->head ||
			(skb_mac_header(skb) + ETH_HLEN) > skb->data ||
			((memcmp(eth_hdr(skb)->h_source, info->srcaddr.macaddr_start, ETH_ALEN) < 0) || 
			 (memcmp(eth_hdr(skb)->h_source, info->srcaddr.macaddr_end, ETH_ALEN) > 0)) ^ !!(info->flags & MAC_SRC_RANGE_INV)
		) {
			return 0;
		}
	}

	if (info->flags & MAC_DST_RANGE) {
		if (skb_mac_header(skb) < skb->head ||
			// !eth && !pppoe
			(((skb_mac_header(skb) + ETH_HLEN) > skb->data)&& !((skb_mac_header(skb) == skb->data) && (skb_mac_header(skb) > skb->head+ETH_HLEN))) ||
			!compare_header_cache_dest_mac(skb, info->dstaddr.macaddr_start, info->dstaddr.macaddr_end) ^ !!(info->flags & MAC_DST_RANGE_INV)
		) {
			return 0;
		}
	}
	
	return 1;
}

static struct xt_match mac_range_mt_reg __read_mostly = {
	.name      = "macrange",
	.revision  = 0,
	.family    = NFPROTO_UNSPEC,
	.match     = mac_range_mt,
	.matchsize = sizeof(struct xt_mac_range_info),
	.hooks     = (1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_LOCAL_IN) |
	             (1 << NF_INET_FORWARD),   
	.me        = THIS_MODULE,
};

static int __init mac_range_mt_init(void)
{
	return xt_register_match(&mac_range_mt_reg);
}

static void __exit mac_range_mt_exit(void)
{
	xt_unregister_match(&mac_range_mt_reg);
}

module_init(mac_range_mt_init);
module_exit(mac_range_mt_exit);
