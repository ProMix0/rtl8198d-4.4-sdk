#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/netpoll.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/list.h>
#include <linux/netfilter_bridge.h>

#include <asm/uaccess.h>
#include "br_private.h"
#include "rtk_br_igmpv3_hook.h"


void rtk_br_igmpv3_hook_get_fwdPortMask(struct net_bridge *br,struct sk_buff *skb,unsigned int *fwdPortMask)
{
	*fwdPortMask = 0;
	unsigned int sip[4] = {0},gip[4] = {0};
	
	switch (skb->protocol) {
	case htons(ETH_P_IP):
		sip[0] = ntohl(ip_hdr(skb)->saddr);
		gip[0] = ntohl(ip_hdr(skb)->daddr);
		break;
#if IS_ENABLED(CONFIG_IPV6)
	case htons(ETH_P_IPV6):
		sip[0] = ntohl(ipv6_hdr(skb)->saddr.s6_addr32[0]);
		sip[1] = ntohl(ipv6_hdr(skb)->saddr.s6_addr32[1]);
		sip[2] = ntohl(ipv6_hdr(skb)->saddr.s6_addr32[2]);
		sip[3] = ntohl(ipv6_hdr(skb)->saddr.s6_addr32[3]);

		gip[0] = ntohl(ipv6_hdr(skb)->daddr.s6_addr32[0]);
		gip[1] = ntohl(ipv6_hdr(skb)->daddr.s6_addr32[1]);
		gip[2] = ntohl(ipv6_hdr(skb)->daddr.s6_addr32[2]);
		gip[3] = ntohl(ipv6_hdr(skb)->daddr.s6_addr32[3]);
		break;
#endif
	default:
		return;
	}
	
	if(rtk_igmpv3_database_hook_get_fwdPortMask(br,sip,gip,skb->protocol,fwdPortMask) != RTK_IGMPV3_SUCCESS){
		//printk(KERN_ERR "%s:get_fwdPortMask fail,sip=%pI4,gip=%pI4\n",__func__,sip,gip);
		return;
	}

	if(*fwdPortMask == 0){
		//printk(KERN_ERR "%s:fwdPortMask is zero,sip=%pI4,gip=%pI4\n",__func__,sip,gip);
		return;
	}

	return;
}

EXPORT_SYMBOL(rtk_br_igmpv3_hook_get_fwdPortMask);

