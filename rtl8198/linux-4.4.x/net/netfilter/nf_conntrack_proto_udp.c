/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2006-2012 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/udp.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <net/ip6_checksum.h>
#include <net/checksum.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/netfilter/nf_log.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/netfilter/ipv6/nf_conntrack_ipv6.h>

#ifdef CONFIG_CORTINA_KERNEL_HOOK
#include <linux/ip.h>
#include <soc/cortina/ca_kernel_hook_api.h>
#endif

/*static*/ unsigned int udp_timeouts[UDP_CT_MAX] = {
	[UDP_CT_UNREPLIED]	= 30*HZ,
	[UDP_CT_REPLIED]	= 180*HZ,
};

static inline struct nf_udp_net *udp_pernet(struct net *net)
{
	return &net->ct.nf_ct_proto.udp;
}

static bool udp_pkt_to_tuple(const struct sk_buff *skb,
			     unsigned int dataoff,
			     struct net *net,
			     struct nf_conntrack_tuple *tuple)
{
	const struct udphdr *hp;
	struct udphdr _hdr;

	/* Actually only need first 8 bytes. */
	hp = skb_header_pointer(skb, dataoff, sizeof(_hdr), &_hdr);
	if (hp == NULL)
		return false;

	tuple->src.u.udp.port = hp->source;
	tuple->dst.u.udp.port = hp->dest;

	return true;
}

static bool udp_invert_tuple(struct nf_conntrack_tuple *tuple,
			     const struct nf_conntrack_tuple *orig)
{
	tuple->src.u.udp.port = orig->dst.u.udp.port;
	tuple->dst.u.udp.port = orig->src.u.udp.port;
	return true;
}

/* Print out the per-protocol part of the tuple. */
static void udp_print_tuple(struct seq_file *s,
			    const struct nf_conntrack_tuple *tuple)
{
	seq_printf(s, "sport=%hu dport=%hu ",
		   ntohs(tuple->src.u.udp.port),
		   ntohs(tuple->dst.u.udp.port));
}

static unsigned int *udp_get_timeouts(struct net *net)
{
	return udp_pernet(net)->timeouts;
}

#ifdef CONFIG_CORTINA_KERNEL_HOOK
static void ca_udp_nat_entry_add(struct nf_conn *ct,
				const struct sk_buff *skb,
				unsigned int dataoff,
				enum ip_conntrack_info ctinfo,
				u_int8_t pf,
				unsigned int hooknum,
				unsigned int *timeouts)
{
	struct nf_conntrack_tuple *orig_tuple;
	struct nf_conntrack_tuple *reply_tuple;
	unsigned int udplen = skb->len - dataoff;
	const struct udphdr *hdr;
	struct udphdr _hdr;

	/* Extract UDP Header */
	hdr = skb_header_pointer(skb, dataoff, sizeof(_hdr), &_hdr);

	if (unlikely(ca_kernel_hook_ops.kho_nat_entry_add == NULL))
		return;

	if ((skb->dev == NULL) || (strcmp(skb->dev->name, "lo") == 0))
		return;

	orig_tuple = &(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	reply_tuple = &(ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist != NULL) {

		if (hdr != NULL && ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(hdr->source))) {
			return;
		}

		if (hdr != NULL && ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(hdr->dest))) {
			return;
		}

		if (ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(orig_tuple->src.u.udp.port))) {
			return;
		}

		if(ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(orig_tuple->dst.u.udp.port))) {
			return;
		}

		if(ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(reply_tuple->src.u.udp.port))) {
			return;
		}

		if(ca_kernel_hook_ops.kho_bypass_udp_portlist_exist(ntohs(reply_tuple->dst.u.udp.port))) {
			return;
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_NAT)
		printk("%s: ct@%p status=0x%x, dev->name=%s, pf=%u, ctinfo=%d\n", __func__, ct, ct->status, skb->dev->name, pf, ctinfo);

	if (ca_kh_udp_offload_occasion == 0) {

		/* Assume the flows/packets of a NAT session are ORIG and REPLY.
		 * Case 1: ORIG * n, REPLY * 1.
		 *         Create ORIG hash when 2nd ORIG packet is received. (ctinfo is IP_CT_NEW)
		 *         Create REPLY hash when 1st REPLY packet is received. (ctinfo is IP_CT_IS_REPLY)
		 * Case 2: ORIG * 1, REPLY * 1, ORIG * 1.
		 *         Create hash of REPLY when 1st REPLY packet is received. (ctinfo is IP_CT_IS_REPLY)
		 *         Create hash of ORIG when 2nd ORIG packet is received. (ctinfo is IP_CT_ESTABLISHED)
		 *
		 * Conclusion:
		 *         ORIG hash is created when 2nd ORIG packet is received. (ctinfo may be IP_CT_NEW or IP_CT_ESTABLISHED)
		 *         REPLY hash is created when 1st REPLY packet is received. (ctinfo must be IP_CT_IS_REPLY)
		 */

		if (((ctinfo == IP_CT_NEW) || (ctinfo == IP_CT_ESTABLISHED)) && test_bit(IPS_CONFIRMED_BIT, &ct->status) && (pf == NFPROTO_IPV4) && (ca_kh_hook & CA_KH_HOOK_IPV4_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* new flow */
			if ((orig_tuple->dst.u3.ip == reply_tuple->src.u3.ip) && (orig_tuple->src.u3.ip != reply_tuple->dst.u3.ip)) {
				/* SNAT/MASQUERADE, orig_tuple change src_ip */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((orig_tuple->src.u3.ip == reply_tuple->dst.u3.ip) && (orig_tuple->dst.u3.ip != reply_tuple->src.u3.ip)) {
				/* DNAT, orig_tuple change dst_ip */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 2,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			}
		} else if ((ctinfo >= IP_CT_IS_REPLY) && (pf == NFPROTO_IPV4) && (ca_kh_hook & CA_KH_HOOK_IPV4_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* replied flow */
			if ((orig_tuple->dst.u3.ip == reply_tuple->src.u3.ip) && (orig_tuple->src.u3.ip != reply_tuple->dst.u3.ip)) {
				/* SNAT/MASQUERADE, reply_tuple change dst_ip */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((orig_tuple->src.u3.ip == reply_tuple->dst.u3.ip) && (orig_tuple->dst.u3.ip != reply_tuple->src.u3.ip)) {
				/* DNAT, reply_tuple change src_ip */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 2,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			}

			/* Must set SEEN_REPLY status, then it's able to set timeout as REPLIED. Next set ASSURED status. */
			set_bit(IPS_SEEN_REPLY_BIT, &ct->status);
			nf_ct_refresh_acct(ct, ctinfo, skb, timeouts[UDP_CT_REPLIED]);
			if (!test_and_set_bit(IPS_ASSURED_BIT, &ct->status))
				nf_conntrack_event_cache(IPCT_ASSURED, ct);
		}

		if (((ctinfo == IP_CT_NEW) || (ctinfo == IP_CT_ESTABLISHED)) && test_bit(IPS_CONFIRMED_BIT, &ct->status) && (pf == NFPROTO_IPV6) && (ca_kh_hook & CA_KH_HOOK_IPV6_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* new flow */
			if (   (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* SNAT/MASQUERADE, reply_tuple change dst_ip */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* DNAT, reply_tuple change src_ip */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here; API doesn't support translating IPv6 SA/DA together */
				/* DNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				/* SNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
			}
		} else if ((ctinfo >= IP_CT_IS_REPLY) && (pf == NFPROTO_IPV6) && (ca_kh_hook & CA_KH_HOOK_IPV6_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* replied flow */
			if (   (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* SNAT/MASQUERADE, reply_tuple change dst_ip */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* DNAT, reply_tuple change src_ip */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here; API doesn't support translating IPv6 SA/DA together */
				/* DNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);

				/* SNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			}

			/* Must set SEEN_REPLY status, then it's able to set timeout as REPLIED. Next set ASSURED status. */
			set_bit(IPS_SEEN_REPLY_BIT, &ct->status);
			nf_ct_refresh_acct(ct, ctinfo, skb, timeouts[UDP_CT_REPLIED]);
			if (!test_and_set_bit(IPS_ASSURED_BIT, &ct->status))
				nf_conntrack_event_cache(IPCT_ASSURED, ct);
		}

	} else if (ca_kh_udp_offload_occasion == 1) {

		/* The ct->status is not current status, it is the status of previous packet received.
		 *
		 * Assume the flows/packets of a NAT session are ORIG and REPLY.
		 * The only case: ORIG * n, REPLY * 1.
		 * Then, no matter next packet is ORIG or REPLY, the status IPS_SEEN_REPLY_BIT is already set when next packet is received.
		 * Meanwhile, ASSURED status bit will be set.
		 * Hash entries of bi-directional flow are created at this moment.
		 */

		if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status) && (pf == NFPROTO_IPV4) && (ca_kh_hook & CA_KH_HOOK_IPV4_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=%pI4:%u, dst=%pI4:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* if pure NAT setup DNAT and SNAT separately */
			if ((orig_tuple->dst.u3.ip == reply_tuple->src.u3.ip) && (orig_tuple->src.u3.ip != reply_tuple->dst.u3.ip)) {
				/* MASQUERADE rule runs here */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((orig_tuple->src.u3.ip == reply_tuple->dst.u3.ip) && (orig_tuple->dst.u3.ip != reply_tuple->src.u3.ip)) {
				/* DNAT rule runs here */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here */
				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 2,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat_entry_add(0, IPPROTO_UDP, 2,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			}

		} else if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status) && (pf == NFPROTO_IPV6) && (ca_kh_hook & CA_KH_HOOK_IPV6_FORWARD)) {
			if (ca_kh_debug & CA_KH_DEBUG_NAT) {
				printk("%s:%d: orig_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&orig_tuple->src.u3.ip, ntohs(orig_tuple->src.u.all),
					&orig_tuple->dst.u3.ip, ntohs(orig_tuple->dst.u.all));
				printk("%s:%d: reply_tuple, src=[%pI6c]:%u, dst=[%pI6c]:%u\n", __func__, __LINE__,
					&reply_tuple->src.u3.ip, ntohs(reply_tuple->src.u.all),
					&reply_tuple->dst.u3.ip, ntohs(reply_tuple->dst.u.all));
			}

			/* if pure NAT setup DNAT and SNAT separately */
			if (   (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* MASQUERADE rule runs here */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else if ((memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) == 0)
			    && (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) != 0) ) {
				/* DNAT rule runs here */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			} else {
				/* Pure routing & NAT Hairpin rule runs here; API doesn't support translating IPv6 SA/DA together */
				/* DNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);

				/* SNAT */
				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

				ca_kernel_hook_ops.kho_nat6_entry_add(0, IPPROTO_UDP, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
			}
		} /* end of if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status) */
	} /* end of if (ca_kh_udp_offload_occasion == 1) */
}
#endif

/* Returns verdict for packet, and may modify conntracktype */
static int udp_packet(struct nf_conn *ct,
		      const struct sk_buff *skb,
		      unsigned int dataoff,
		      enum ip_conntrack_info ctinfo,
		      u_int8_t pf,
		      unsigned int hooknum,
		      unsigned int *timeouts)
{
#ifdef CONFIG_CORTINA_KERNEL_HOOK
	ca_udp_nat_entry_add(ct, skb, dataoff, ctinfo, pf, hooknum, timeouts);
#endif

	/* If we've seen traffic both ways, this is some kind of UDP
	   stream.  Extend timeout. */
	if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
		nf_ct_refresh_acct(ct, ctinfo, skb,
				   timeouts[UDP_CT_REPLIED]);
		/* Also, more likely to be important, and not a probe */
		if (!test_and_set_bit(IPS_ASSURED_BIT, &ct->status))
			nf_conntrack_event_cache(IPCT_ASSURED, ct);
	} else {
		nf_ct_refresh_acct(ct, ctinfo, skb,
				   timeouts[UDP_CT_UNREPLIED]);
	}
	return NF_ACCEPT;
}

/* Called when a new connection for this protocol found. */
static bool udp_new(struct nf_conn *ct, const struct sk_buff *skb,
		    unsigned int dataoff, unsigned int *timeouts)
{
	return true;
}

static int udp_error(struct net *net, struct nf_conn *tmpl, struct sk_buff *skb,
		     unsigned int dataoff, enum ip_conntrack_info *ctinfo,
		     u_int8_t pf,
		     unsigned int hooknum)
{
	unsigned int udplen = skb->len - dataoff;
	const struct udphdr *hdr;
	struct udphdr _hdr;

	/* Header is too small? */
	hdr = skb_header_pointer(skb, dataoff, sizeof(_hdr), &_hdr);
	if (hdr == NULL) {
		if (LOG_INVALID(net, IPPROTO_UDP))
			nf_log_packet(net, pf, 0, skb, NULL, NULL, NULL,
				      "nf_ct_udp: short packet ");
		return -NF_ACCEPT;
	}

	/* Truncated/malformed packets */
	if (ntohs(hdr->len) > udplen || ntohs(hdr->len) < sizeof(*hdr)) {
		if (LOG_INVALID(net, IPPROTO_UDP))
			nf_log_packet(net, pf, 0, skb, NULL, NULL, NULL,
				"nf_ct_udp: truncated/malformed packet ");
		return -NF_ACCEPT;
	}

	/* Packet with no checksum */
	if (!hdr->check)
		return NF_ACCEPT;

	/* Checksum invalid? Ignore.
	 * We skip checking packets on the outgoing path
	 * because the checksum is assumed to be correct.
	 * FIXME: Source route IP option packets --RR */
	if (net->ct.sysctl_checksum && hooknum == NF_INET_PRE_ROUTING &&
	    nf_checksum(skb, hooknum, dataoff, IPPROTO_UDP, pf)) {
		if (LOG_INVALID(net, IPPROTO_UDP))
			nf_log_packet(net, pf, 0, skb, NULL, NULL, NULL,
				"nf_ct_udp: bad UDP checksum ");
		return -NF_ACCEPT;
	}

	return NF_ACCEPT;
}

#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_cttimeout.h>

static int udp_timeout_nlattr_to_obj(struct nlattr *tb[],
				     struct net *net, void *data)
{
	unsigned int *timeouts = data;
	struct nf_udp_net *un = udp_pernet(net);

	/* set default timeouts for UDP. */
	timeouts[UDP_CT_UNREPLIED] = un->timeouts[UDP_CT_UNREPLIED];
	timeouts[UDP_CT_REPLIED] = un->timeouts[UDP_CT_REPLIED];

	if (tb[CTA_TIMEOUT_UDP_UNREPLIED]) {
		timeouts[UDP_CT_UNREPLIED] =
			ntohl(nla_get_be32(tb[CTA_TIMEOUT_UDP_UNREPLIED])) * HZ;
	}
	if (tb[CTA_TIMEOUT_UDP_REPLIED]) {
		timeouts[UDP_CT_REPLIED] =
			ntohl(nla_get_be32(tb[CTA_TIMEOUT_UDP_REPLIED])) * HZ;
	}
	return 0;
}

static int
udp_timeout_obj_to_nlattr(struct sk_buff *skb, const void *data)
{
	const unsigned int *timeouts = data;

	if (nla_put_be32(skb, CTA_TIMEOUT_UDP_UNREPLIED,
			 htonl(timeouts[UDP_CT_UNREPLIED] / HZ)) ||
	    nla_put_be32(skb, CTA_TIMEOUT_UDP_REPLIED,
			 htonl(timeouts[UDP_CT_REPLIED] / HZ)))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -ENOSPC;
}

static const struct nla_policy
udp_timeout_nla_policy[CTA_TIMEOUT_UDP_MAX+1] = {
       [CTA_TIMEOUT_UDP_UNREPLIED]	= { .type = NLA_U32 },
       [CTA_TIMEOUT_UDP_REPLIED]	= { .type = NLA_U32 },
};
#endif /* CONFIG_NF_CT_NETLINK_TIMEOUT */

#ifdef CONFIG_SYSCTL
static struct ctl_table udp_sysctl_table[] = {
	{
		.procname	= "nf_conntrack_udp_timeout",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_udp_timeout_stream",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
static struct ctl_table udp_compat_sysctl_table[] = {
	{
		.procname	= "ip_conntrack_udp_timeout",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "ip_conntrack_udp_timeout_stream",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#endif /* CONFIG_NF_CONNTRACK_PROC_COMPAT */
#endif /* CONFIG_SYSCTL */

static int udp_kmemdup_sysctl_table(struct nf_proto_net *pn,
				    struct nf_udp_net *un)
{
#ifdef CONFIG_SYSCTL
	if (pn->ctl_table)
		return 0;
	pn->ctl_table = kmemdup(udp_sysctl_table,
				sizeof(udp_sysctl_table),
				GFP_KERNEL);
	if (!pn->ctl_table)
		return -ENOMEM;
	pn->ctl_table[0].data = &un->timeouts[UDP_CT_UNREPLIED];
	pn->ctl_table[1].data = &un->timeouts[UDP_CT_REPLIED];
#endif
	return 0;
}

static int udp_kmemdup_compat_sysctl_table(struct nf_proto_net *pn,
					   struct nf_udp_net *un)
{
#ifdef CONFIG_SYSCTL
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
	pn->ctl_compat_table = kmemdup(udp_compat_sysctl_table,
				       sizeof(udp_compat_sysctl_table),
				       GFP_KERNEL);
	if (!pn->ctl_compat_table)
		return -ENOMEM;

	pn->ctl_compat_table[0].data = &un->timeouts[UDP_CT_UNREPLIED];
	pn->ctl_compat_table[1].data = &un->timeouts[UDP_CT_REPLIED];
#endif
#endif
	return 0;
}

static int udp_init_net(struct net *net, u_int16_t proto)
{
	int ret;
	struct nf_udp_net *un = udp_pernet(net);
	struct nf_proto_net *pn = &un->pn;

	if (!pn->users) {
		int i;

		for (i = 0; i < UDP_CT_MAX; i++)
			un->timeouts[i] = udp_timeouts[i];
	}

	if (proto == AF_INET) {
		ret = udp_kmemdup_compat_sysctl_table(pn, un);
		if (ret < 0)
			return ret;

		ret = udp_kmemdup_sysctl_table(pn, un);
		if (ret < 0)
			nf_ct_kfree_compat_sysctl_table(pn);
	} else
		ret = udp_kmemdup_sysctl_table(pn, un);

	return ret;
}

static struct nf_proto_net *udp_get_net_proto(struct net *net)
{
	return &net->ct.nf_ct_proto.udp.pn;
}

struct nf_conntrack_l4proto nf_conntrack_l4proto_udp4 __read_mostly =
{
	.l3proto		= PF_INET,
	.l4proto		= IPPROTO_UDP,
	.name			= "udp",
	.pkt_to_tuple		= udp_pkt_to_tuple,
	.invert_tuple		= udp_invert_tuple,
	.print_tuple		= udp_print_tuple,
	.packet			= udp_packet,
	.get_timeouts		= udp_get_timeouts,
	.new			= udp_new,
	.error			= udp_error,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
	.tuple_to_nlattr	= nf_ct_port_tuple_to_nlattr,
	.nlattr_to_tuple	= nf_ct_port_nlattr_to_tuple,
	.nlattr_tuple_size	= nf_ct_port_nlattr_tuple_size,
	.nla_policy		= nf_ct_port_nla_policy,
#endif
#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)
	.ctnl_timeout		= {
		.nlattr_to_obj	= udp_timeout_nlattr_to_obj,
		.obj_to_nlattr	= udp_timeout_obj_to_nlattr,
		.nlattr_max	= CTA_TIMEOUT_UDP_MAX,
		.obj_size	= sizeof(unsigned int) * CTA_TIMEOUT_UDP_MAX,
		.nla_policy	= udp_timeout_nla_policy,
	},
#endif /* CONFIG_NF_CT_NETLINK_TIMEOUT */
	.init_net		= udp_init_net,
	.get_net_proto		= udp_get_net_proto,
};
EXPORT_SYMBOL_GPL(nf_conntrack_l4proto_udp4);

struct nf_conntrack_l4proto nf_conntrack_l4proto_udp6 __read_mostly =
{
	.l3proto		= PF_INET6,
	.l4proto		= IPPROTO_UDP,
	.name			= "udp",
	.pkt_to_tuple		= udp_pkt_to_tuple,
	.invert_tuple		= udp_invert_tuple,
	.print_tuple		= udp_print_tuple,
	.packet			= udp_packet,
	.get_timeouts		= udp_get_timeouts,
	.new			= udp_new,
	.error			= udp_error,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
	.tuple_to_nlattr	= nf_ct_port_tuple_to_nlattr,
	.nlattr_to_tuple	= nf_ct_port_nlattr_to_tuple,
	.nlattr_tuple_size	= nf_ct_port_nlattr_tuple_size,
	.nla_policy		= nf_ct_port_nla_policy,
#endif
#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)
	.ctnl_timeout		= {
		.nlattr_to_obj	= udp_timeout_nlattr_to_obj,
		.obj_to_nlattr	= udp_timeout_obj_to_nlattr,
		.nlattr_max	= CTA_TIMEOUT_UDP_MAX,
		.obj_size	= sizeof(unsigned int) * CTA_TIMEOUT_UDP_MAX,
		.nla_policy	= udp_timeout_nla_policy,
	},
#endif /* CONFIG_NF_CT_NETLINK_TIMEOUT */
	.init_net		= udp_init_net,
	.get_net_proto		= udp_get_net_proto,
};
EXPORT_SYMBOL_GPL(nf_conntrack_l4proto_udp6);
