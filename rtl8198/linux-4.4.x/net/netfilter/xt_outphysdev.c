/* Kernel module to match Realsil extension functions. */

/* (C) 2009-2017 Chloe <chloe_wang@realsil.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/skbuff.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_outphysdev.h>

#include <net/netfilter/nf_conntrack.h>
#include <linux/ip.h>
#include <linux/inetdevice.h>
#include <net/arp.h>
#include "../bridge/br_private.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chloe <chloe_wang@realsil.com.cn>");
MODULE_DESCRIPTION("iptables realsil extensions matching module");
MODULE_ALIAS("xt_outphysdev");

int arp_req_get_ha(__be32 queryIP, struct net_device *dev, unsigned char *res_hw_addr)
{
	__be32 ip = queryIP;
	struct neighbour *neigh;
	int err = -ENXIO;

	if(!dev || !res_hw_addr)
		return err;
		
	neigh = neigh_lookup(&arp_tbl, &ip, dev);
	if (neigh) {
		read_lock_bh(&neigh->lock);
		memcpy(res_hw_addr, neigh->ha, dev->addr_len);
		read_unlock_bh(&neigh->lock);
		neigh_release(neigh);
		err = 0;
	}

	return err;
}

#ifdef CONFIG_IPV6
int arp6_req_get_ha(struct in6_addr *queryIP, struct net_device *dev, unsigned char *res_hw_addr)
{
	struct neighbour *neigh;
	int err = -ENXIO;

	if(!queryIP || !dev || !res_hw_addr)
		return err;

	neigh = neigh_lookup(&nd_tbl, queryIP, dev);
	if (neigh) {
		read_lock_bh(&neigh->lock);
		memcpy(res_hw_addr, neigh->ha, dev->addr_len);
		read_unlock_bh(&neigh->lock);
		neigh_release(neigh);
		err = 0;
	}

	return err;
}
#endif

unsigned int is_skb_downsteam(struct sk_buff *skb)
{
	struct net_device *br_dev = NULL;
	char *br_dev_name = "br0";
	struct iphdr *iph;
	struct ipv6hdr *ip6h = NULL;
	struct in_device *in_dev;
	struct in_ifaddr *ifap = NULL;
	unsigned int br_dev_ip = 0, br_dev_mask = 0;

	if(skb == NULL) {
		return 0;
	}
	
	if((br_dev = __dev_get_by_name(&init_net, br_dev_name)) == NULL) {
		return 0;
	}

#if defined(CONFIG_IPV6)
	if(skb->protocol == htons(ETH_P_IPV6)) {
		unsigned char mac[6] = {0};
		
		if((ip6h = ipv6_hdr(skb)) == NULL) {
			return 0;
		}
			
		if(arp6_req_get_ha(&ip6h->daddr, br_dev, mac)){
			return 0;
		}
		else{
			return 1;
		}
	}
	else 
#endif
	{
		if((iph = ip_hdr(skb)) == NULL) {
			return 0;
		}
		
		if ((in_dev = (struct in_device *)(br_dev->ip_ptr)) == NULL) {
			return 0;
		}

		if((ifap = in_dev->ifa_list) == NULL) {
			return 0;
		}

		br_dev_ip = ifap->ifa_address;
		br_dev_mask = ifap->ifa_mask;

		if(!br_dev_ip || !br_dev_mask || (((iph->daddr) & br_dev_mask) != (br_dev_ip & br_dev_mask))) {
			return 0;
		}
	}

	return 1;
}

unsigned int find_out_physdev(struct sk_buff *skb)
{
	struct iphdr *iph;
	struct nf_conn* ct;
	char *br_dev_name = "br0";
	struct net_device *br_dev = NULL;
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	unsigned char mac[6] = {0};
	unsigned short vid = 0;
	struct ipv6hdr *ip6h = NULL;

	if(skb == NULL) {
		return 0;
	}
	
	if((ct = (struct nf_conn *)skb->nfct) == NULL) {
		return 0;
	}
	
	if((br_dev = __dev_get_by_name(&init_net, br_dev_name)) == NULL) {
		return 0;
	}


	if ((br = netdev_priv(br_dev)) == NULL) {
		return 0;
	}

#if defined(CONFIG_IPV6)
	if(skb->protocol == htons(ETH_P_IPV6)) {
		if((ip6h = ipv6_hdr(skb)) == NULL) {
			return 0;
		}
		
		if(arp6_req_get_ha(&ip6h->daddr, br_dev, mac)) {
			return 0;
		}
	}
	else
#endif
	{
		if((iph = ip_hdr(skb)) == NULL) {
			return 0;
		}
		
		if(arp_req_get_ha(iph->daddr, br_dev, mac)) {
			return 0;
		}
	}
	
	if((dst = __br_fdb_get(br, mac, vid)) && dst->dst && dst->dst->dev) {
		spin_lock_bh(&ct->lock);
										
		snprintf(ct->out_physdev_name, IFNAMSIZ, "%s", dst->dst->dev->name);
		spin_unlock_bh(&ct->lock);
			
		//printk("[%s:%d] ct->out_physdev_name: %s\n", __FUNCTION__, __LINE__, ct->out_physdev_name);
		return 1;
	}

	return 0;
}

unsigned int out_physdev_match(const struct sk_buff *skb, const struct xt_outphysdev_info *info)
{
	char *br_dev_name = "br0";
	struct net_device *br_dev = NULL;
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	
	if(!skb || !info) {
		return 0;
	}

	if(!eth_hdr(skb) || !(br_dev = __dev_get_by_name(&init_net, br_dev_name))) {
		return 0;
	}
	
	if(!(br = netdev_priv(br_dev))) {
		return 0;
	}

	if((dst = __br_fdb_get(br, eth_hdr(skb)->h_dest, 0)) && dst->dst && dst->dst->dev) {
		if(!ifname_compare_aligned(dst->dst->dev->name, info->out_physdev,info->mask)){
			return 1;
		}
	}
	
	return 0;
}

static bool
match(const struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_outphysdev_info *info = par->matchinfo;
	struct nf_conn* ct = (struct nf_conn *)skb->nfct;

	if(out_physdev_match(skb, info) == 1) {
		return 1;
	}

	if(!is_skb_downsteam(skb))
		return 0;

	if((ct != NULL) && (strlen(ct->out_physdev_name) || find_out_physdev(skb))){
		if(!ifname_compare_aligned(ct->out_physdev_name, info->out_physdev,info->mask)){
			return 1;
		}
	}

	return 0;
}

static bool
checkentry(const struct xt_mtchk_param *par)
{
	return 0;
}

static struct xt_match xt_outphysdev_match[] = {
	{
		.name		= "outphysdev",
		.family		= AF_UNSPEC,
		.checkentry	= checkentry,
		.match		= match,
		.matchsize	= sizeof(struct xt_outphysdev_info),
		.me		= THIS_MODULE,
	},
};

static int __init xt_outphysdev_init(void)
{
	return xt_register_matches(xt_outphysdev_match, ARRAY_SIZE(xt_outphysdev_match));
}

static void __exit xt_outphysdev_fini(void)
{
	xt_unregister_matches(xt_outphysdev_match, ARRAY_SIZE(xt_outphysdev_match));
}

module_init(xt_outphysdev_init);
module_exit(xt_outphysdev_fini);

