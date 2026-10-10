/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/udp.h>

#include <linux/netfilter.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l3proto.h>
#include <net/netfilter/nf_nat_l4proto.h>

#ifdef CONFIG_CORTINA_KERNEL_HOOK
#include <linux/ip.h>
#include <soc/cortina/ca_kernel_hook_api.h>
#endif

static u16 udp_port_rover;

static void
udp_unique_tuple(const struct nf_nat_l3proto *l3proto,
		 struct nf_conntrack_tuple *tuple,
		 const struct nf_nat_range *range,
		 enum nf_nat_manip_type maniptype,
		 const struct nf_conn *ct)
{
	nf_nat_l4proto_unique_tuple(l3proto, tuple, range, maniptype, ct,
				    &udp_port_rover);
}

static bool
udp_manip_pkt(struct sk_buff *skb,
	      const struct nf_nat_l3proto *l3proto,
	      unsigned int iphdroff, unsigned int hdroff,
	      const struct nf_conntrack_tuple *tuple,
	      enum nf_nat_manip_type maniptype)
{
	struct udphdr *hdr;
	__be16 *portptr, newport;

#if 0
#ifdef CONFIG_CORTINA_KERNEL_HOOK
        const struct iphdr *iph = (struct iphdr *)(skb->data + iphdroff);
#endif
#endif

	if (!skb_make_writable(skb, hdroff + sizeof(*hdr)))
		return false;
	hdr = (struct udphdr *)(skb->data + hdroff);

	if (maniptype == NF_NAT_MANIP_SRC) {
		/* Get rid of src port */
		newport = tuple->src.u.udp.port;
		portptr = &hdr->source;
	} else {
		/* Get rid of dst port */
		newport = tuple->dst.u.udp.port;
		portptr = &hdr->dest;
	}
	if (hdr->check || skb->ip_summed == CHECKSUM_PARTIAL) {

	/* move to udp_packet() for both DNAT and SNAT */
#if 0
#ifdef CONFIG_CORTINA_KERNEL_HOOK
        {
                int bypass_alg = 0;
		struct nf_conn *ct;
		enum ip_conntrack_info ctinfo;

                ct = nf_ct_get(skb, &ctinfo);
#if 0
                if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist != NULL) {
                        if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(hdr->source)))
                                bypass_alg = 1;
                        if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(tuple->src.u.udp.port)))
                                bypass_alg = 1;
                        if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(hdr->dest)))
                                bypass_alg = 1;
                        if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(tuple->dst.u.udp.port)))
                                bypass_alg = 1;
                }
#endif
		if (ct != NULL) {
			if (ct->status & IPS_ASSURED_BIT) {
				printk(KERN_INFO "%s: UDP status is ASSURED!!\n", __func__);
				if ((bypass_alg == 0) && (skb->dev->name != NULL)) {
					if ((ca_kernel_hook_ops.kho_nat_entry_add != NULL) && (ca_kh_hook & CA_KH_HOOK_IPV4_FORWARD)) {
						ca_kernel_hook_ops.kho_nat_entry_add(0, 0, (maniptype == NF_NAT_MANIP_SRC) ? 1 : 0,  (__ca_uint32_t)(iph->saddr), tuple->src.u3.ip,
								(__ca_uint32_t)(iph->daddr), tuple->dst.u3.ip, hdr->source, tuple->src.u.udp.port, hdr->dest, tuple->dst.u.udp.port);
					}
				}
			}
		}
	}
#endif
#endif

		l3proto->csum_update(skb, iphdroff, &hdr->check,
				     tuple, maniptype);
		inet_proto_csum_replace2(&hdr->check, skb, *portptr, newport,
					 false);
		if (!hdr->check)
			hdr->check = CSUM_MANGLED_0;
	}
	*portptr = newport;
	return true;
}

const struct nf_nat_l4proto nf_nat_l4proto_udp = {
	.l4proto		= IPPROTO_UDP,
	.manip_pkt		= udp_manip_pkt,
	.in_range		= nf_nat_l4proto_in_range,
	.in_portset		= nf_nat_l4proto_in_portset,
	.unique_tuple		= udp_unique_tuple,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
	.nlattr_to_range	= nf_nat_l4proto_nlattr_to_range,
#endif
};
