/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack_l4proto.h>

#ifdef CONFIG_CORTINA_KERNEL_HOOK
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <soc/cortina/ca_kernel_hook_api.h>
#endif

static unsigned int nf_ct_generic_timeout __read_mostly = 600*HZ;

static bool nf_generic_should_process(u8 proto)
{
	switch (proto) {
#ifdef CONFIG_NF_CT_PROTO_SCTP_MODULE
	case IPPROTO_SCTP:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_DCCP_MODULE
	case IPPROTO_DCCP:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_GRE_MODULE
	case IPPROTO_GRE:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_UDPLITE_MODULE
	case IPPROTO_UDPLITE:
		return false;
#endif
	default:
		return true;
	}
}

static inline struct nf_generic_net *generic_pernet(struct net *net)
{
	return &net->ct.nf_ct_proto.generic;
}

static bool generic_pkt_to_tuple(const struct sk_buff *skb,
				 unsigned int dataoff,
				 struct net *net, struct nf_conntrack_tuple *tuple)
{
	tuple->src.u.all = 0;
	tuple->dst.u.all = 0;

	return true;
}

static bool generic_invert_tuple(struct nf_conntrack_tuple *tuple,
				 const struct nf_conntrack_tuple *orig)
{
	tuple->src.u.all = 0;
	tuple->dst.u.all = 0;

	return true;
}

/* Print out the per-protocol part of the tuple. */
static void generic_print_tuple(struct seq_file *s,
				const struct nf_conntrack_tuple *tuple)
{
}

static unsigned int *generic_get_timeouts(struct net *net)
{
	return &(generic_pernet(net)->timeout);
}

/* Returns verdict for packet, or -1 for invalid. */
static int generic_packet(struct nf_conn *ct,
			  const struct sk_buff *skb,
			  unsigned int dataoff,
			  enum ip_conntrack_info ctinfo,
			  u_int8_t pf,
			  unsigned int hooknum,
			  unsigned int *timeout)
{
#ifdef CONFIG_CORTINA_KERNEL_HOOK
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_tuple *orig_tuple;
	struct nf_conntrack_tuple *reply_tuple;
	unsigned int outer_dataoff = 0; /* offset of L4 payload; if packet is IP-in-IP format, it is offset of inner L3. */
	unsigned int inner_dataoff = 0; /* data offset of L4 payload in inner L3 packet */
	u_int8_t outer_protonum = 0; /* proto number in outer L3 header */
	u_int8_t inner_protonum = 0; /* proto number in inner L3 packet */
	int ret;

	struct tcphdr _tcph, *tcph = NULL;
	struct udphdr _udph, *udph = NULL;

	orig_tuple = &(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	reply_tuple = &(ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	/* outer L3; need outer_protonum */
	l3proto = __nf_ct_l3proto_find(pf);
	ret = l3proto->get_l4proto(skb, skb_network_offset(skb), &outer_dataoff, &outer_protonum);
	if (ret != NF_ACCEPT) {
		/* What's wrong? */
		printk(KERN_ERR "%s: Fail to get L4 proto. (ret %d)\n", __func__, ret);
		goto leave_kernel_hook;
	}

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk(KERN_INFO "%s: ct@%p status=0x%x, dev->name=%s, pf=%u, outer_protonum=%u, ctinfo=%d, dataoff=%u, outer_dataoff=%u\n",
			__func__, ct, ct->status, skb->dev->name, pf, outer_protonum, ctinfo, dataoff, outer_dataoff);
	}

	/* decide to offload or not */
	switch (outer_protonum) {
	case IPPROTO_IPIP:
		if ((pf == NFPROTO_IPV4) && ((ca_kh_hook & CA_KH_HOOK_IPV4_IN_IPV4_PASSTHROUGH) == 0))
			goto leave_kernel_hook;
		if ((pf == NFPROTO_IPV6) && ((ca_kh_hook & CA_KH_HOOK_IPV4_IN_IPV6_PASSTHROUGH) == 0)) /* DS-Lite, MAP-E */
			goto leave_kernel_hook;
		break;
	case IPPROTO_IPV6:
		if ((pf == NFPROTO_IPV4) && ((ca_kh_hook & CA_KH_HOOK_IPV6_IN_IPV4_PASSTHROUGH) == 0)) /* 6RD */
			goto leave_kernel_hook;
		if ((pf == NFPROTO_IPV6) && ((ca_kh_hook & CA_KH_HOOK_IPV6_IN_IPV6_PASSTHROUGH) == 0))
			goto leave_kernel_hook;
		break;
	case IPPROTO_GRE:
		if ((ca_kh_hook & CA_KH_HOOK_GRE_PASSTHROUGH) == 0)
			goto leave_kernel_hook;
		break;
	case IPPROTO_ESP:
		if ((pf == NFPROTO_IPV4) && ((ca_kh_hook & CA_KH_HOOK_IPV4_IPSEC_PASSTHROUGH) == 0))
			goto leave_kernel_hook;
		if ((pf == NFPROTO_IPV6) && ((ca_kh_hook & CA_KH_HOOK_IPV6_IPSEC_PASSTHROUGH) == 0))
			goto leave_kernel_hook;
		break;

	default:
		goto leave_kernel_hook;
	}

	if (((ctinfo == IP_CT_NEW) || (ctinfo == IP_CT_ESTABLISHED)) && test_bit(IPS_CONFIRMED_BIT, &ct->status) && (pf == NFPROTO_IPV4)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk(KERN_INFO "%s: orig_tuple, src=%pI4, dst=%pI4, outer_protonum=%u\n", __func__, &orig_tuple->src.u3.ip, &orig_tuple->dst.u3.ip, outer_protonum);
			printk(KERN_INFO "%s: reply_tuple, src=%pI4, dst=%pI4\n", __func__, &reply_tuple->src.u3.ip, &reply_tuple->dst.u3.ip);
		}

		/* new flow */
		if ((orig_tuple->dst.u3.ip == reply_tuple->src.u3.ip) && (orig_tuple->src.u3.ip != reply_tuple->dst.u3.ip)) {
			/* SNAT/MASQUERADE, orig_tuple change src_ip */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 1,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		} else if ((orig_tuple->src.u3.ip == reply_tuple->dst.u3.ip) && (orig_tuple->dst.u3.ip != reply_tuple->src.u3.ip)) {
			/* DNAT, orig_tuple change dst_ip */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 0,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		} else {
			/* Pure routing & NAT Hairpin rule runs here */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 2,
					orig_tuple->src.u3.ip, reply_tuple->dst.u3.ip,
					orig_tuple->dst.u3.ip, reply_tuple->src.u3.ip,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		}
	} else if ((ctinfo >= IP_CT_IS_REPLY) && (pf == NFPROTO_IPV4)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk(KERN_INFO "%s: orig_tuple, src=%pI4, dst=%pI4, outer_protonum=%u\n", __func__, &orig_tuple->src.u3.ip, &orig_tuple->dst.u3.ip, outer_protonum);
			printk(KERN_INFO "%s: reply_tuple, src=%pI4, dst=%pI4\n", __func__, &reply_tuple->src.u3.ip, &reply_tuple->dst.u3.ip);
		}

		/* replied flow */
		if ((orig_tuple->dst.u3.ip == reply_tuple->src.u3.ip) && (orig_tuple->src.u3.ip != reply_tuple->dst.u3.ip)) {
			/* SNAT/MASQUERADE, reply_tuple change dst_ip */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 0,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		} else if ((orig_tuple->src.u3.ip == reply_tuple->dst.u3.ip) && (orig_tuple->dst.u3.ip != reply_tuple->src.u3.ip)) {
			/* DNAT, reply_tuple change src_ip */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 1,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		} else {
			/* Pure routing & NAT Hairpin rule runs here */
			ca_kernel_hook_ops.kho_nat_entry_add(0, outer_protonum, 2,
					reply_tuple->src.u3.ip, orig_tuple->dst.u3.ip,
					reply_tuple->dst.u3.ip, orig_tuple->src.u3.ip,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		}
	}

	if (((ctinfo == IP_CT_NEW) || (ctinfo == IP_CT_ESTABLISHED)) && test_bit(IPS_CONFIRMED_BIT, &ct->status) && (pf == NFPROTO_IPV6)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk(KERN_INFO "%s: orig_tuple, src=%pI6c, dst=%pI6c, outer_protonum=%u\n", __func__, &orig_tuple->src.u3.ip, &orig_tuple->dst.u3.ip, outer_protonum);
			printk(KERN_INFO "%s: reply_tuple, src=%pI6c, dst=%pI6c\n", __func__, &reply_tuple->src.u3.ip, &reply_tuple->dst.u3.ip);
		}

		/* new flow */
		if (   (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) == 0)
				&& (memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) != 0) ) {
			/* SNAT/MASQUERADE, reply_tuple change dst_ip */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		} else if ((memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) == 0)
				&& (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) != 0) ) {
			/* DNAT, reply_tuple change src_ip */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		} else {
			/* Pure routing & NAT Hairpin rule runs here; API doesn't support translating IPv6 SA/DA together */
			/* DNAT */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 1,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);

			/* SNAT */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 0,
					orig_tuple->src.u3.ip6, reply_tuple->dst.u3.ip6,
					orig_tuple->dst.u3.ip6, reply_tuple->src.u3.ip6,
					orig_tuple->src.u.udp.port, reply_tuple->dst.u.udp.port,
					orig_tuple->dst.u.udp.port, reply_tuple->src.u.udp.port, skb->dev->ifindex);
		}
	} else if ((ctinfo >= IP_CT_IS_REPLY) && (pf == NFPROTO_IPV6)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk(KERN_INFO "%s: orig_tuple, src=%pI6c, dst=%pI6c, outer_protonum=%u\n", __func__, &orig_tuple->src.u3.ip, &orig_tuple->dst.u3.ip, outer_protonum);
			printk(KERN_INFO "%s: reply_tuple, src=%pI6c, dst=%pI6c\n", __func__, &reply_tuple->src.u3.ip, &reply_tuple->dst.u3.ip);
		}

		/* replied flow */
		if (   (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) == 0)
				&& (memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) != 0) ) {
			/* SNAT/MASQUERADE, reply_tuple change dst_ip */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		} else if ((memcmp(&orig_tuple->src.u3.in6, &reply_tuple->dst.u3.in6, sizeof(struct in6_addr)) == 0)
				&& (memcmp(&orig_tuple->dst.u3.in6, &reply_tuple->src.u3.in6, sizeof(struct in6_addr)) != 0) ) {
			/* DNAT, reply_tuple change src_ip */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		} else {
			/* Pure routing & NAT Hairpin rule runs here; API doesn't support translating IPv6 SA/DA together */
			/* DNAT */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 0,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);

			/* SNAT */
			ca_kernel_hook_ops.kho_nat6_entry_add(0, outer_protonum, 1,
					reply_tuple->src.u3.ip6, orig_tuple->dst.u3.ip6,
					reply_tuple->dst.u3.ip6, orig_tuple->src.u3.ip6,
					reply_tuple->src.u.udp.port, orig_tuple->dst.u.udp.port,
					reply_tuple->dst.u.udp.port, orig_tuple->src.u.udp.port, skb->dev->ifindex);
		}
	}
leave_kernel_hook:
#endif /* CONFIG_CORTINA_KERNEL_HOOK */

	nf_ct_refresh_acct(ct, ctinfo, skb, *timeout);
	return NF_ACCEPT;
}

/* Called when a new connection for this protocol found. */
static bool generic_new(struct nf_conn *ct, const struct sk_buff *skb,
			unsigned int dataoff, unsigned int *timeouts)
{
	bool ret;

	ret = nf_generic_should_process(nf_ct_protonum(ct));
	if (!ret)
		pr_warn_once("conntrack: generic helper won't handle protocol %d. Please consider loading the specific helper module.\n",
			     nf_ct_protonum(ct));
	return ret;
}

#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_cttimeout.h>

static int generic_timeout_nlattr_to_obj(struct nlattr *tb[],
					 struct net *net, void *data)
{
	unsigned int *timeout = data;
	struct nf_generic_net *gn = generic_pernet(net);

	if (tb[CTA_TIMEOUT_GENERIC_TIMEOUT])
		*timeout =
		    ntohl(nla_get_be32(tb[CTA_TIMEOUT_GENERIC_TIMEOUT])) * HZ;
	else {
		/* Set default generic timeout. */
		*timeout = gn->timeout;
	}

	return 0;
}

static int
generic_timeout_obj_to_nlattr(struct sk_buff *skb, const void *data)
{
	const unsigned int *timeout = data;

	if (nla_put_be32(skb, CTA_TIMEOUT_GENERIC_TIMEOUT, htonl(*timeout / HZ)))
		goto nla_put_failure;

	return 0;

nla_put_failure:
        return -ENOSPC;
}

static const struct nla_policy
generic_timeout_nla_policy[CTA_TIMEOUT_GENERIC_MAX+1] = {
	[CTA_TIMEOUT_GENERIC_TIMEOUT]	= { .type = NLA_U32 },
};
#endif /* CONFIG_NF_CT_NETLINK_TIMEOUT */

#ifdef CONFIG_SYSCTL
static struct ctl_table generic_sysctl_table[] = {
	{
		.procname	= "nf_conntrack_generic_timeout",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
static struct ctl_table generic_compat_sysctl_table[] = {
	{
		.procname	= "ip_conntrack_generic_timeout",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#endif /* CONFIG_NF_CONNTRACK_PROC_COMPAT */
#endif /* CONFIG_SYSCTL */

static int generic_kmemdup_sysctl_table(struct nf_proto_net *pn,
					struct nf_generic_net *gn)
{
#ifdef CONFIG_SYSCTL
	pn->ctl_table = kmemdup(generic_sysctl_table,
				sizeof(generic_sysctl_table),
				GFP_KERNEL);
	if (!pn->ctl_table)
		return -ENOMEM;

	pn->ctl_table[0].data = &gn->timeout;
#endif
	return 0;
}

static int generic_kmemdup_compat_sysctl_table(struct nf_proto_net *pn,
					       struct nf_generic_net *gn)
{
#ifdef CONFIG_SYSCTL
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
	pn->ctl_compat_table = kmemdup(generic_compat_sysctl_table,
				       sizeof(generic_compat_sysctl_table),
				       GFP_KERNEL);
	if (!pn->ctl_compat_table)
		return -ENOMEM;

	pn->ctl_compat_table[0].data = &gn->timeout;
#endif
#endif
	return 0;
}

static int generic_init_net(struct net *net, u_int16_t proto)
{
	int ret;
	struct nf_generic_net *gn = generic_pernet(net);
	struct nf_proto_net *pn = &gn->pn;

	gn->timeout = nf_ct_generic_timeout;

	ret = generic_kmemdup_compat_sysctl_table(pn, gn);
	if (ret < 0)
		return ret;

	ret = generic_kmemdup_sysctl_table(pn, gn);
	if (ret < 0)
		nf_ct_kfree_compat_sysctl_table(pn);

	return ret;
}

static struct nf_proto_net *generic_get_net_proto(struct net *net)
{
	return &net->ct.nf_ct_proto.generic.pn;
}

struct nf_conntrack_l4proto nf_conntrack_l4proto_generic __read_mostly =
{
	.l3proto		= PF_UNSPEC,
	.l4proto		= 255,
	.name			= "unknown",
	.pkt_to_tuple		= generic_pkt_to_tuple,
	.invert_tuple		= generic_invert_tuple,
	.print_tuple		= generic_print_tuple,
	.packet			= generic_packet,
	.get_timeouts		= generic_get_timeouts,
	.new			= generic_new,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)
	.ctnl_timeout		= {
		.nlattr_to_obj	= generic_timeout_nlattr_to_obj,
		.obj_to_nlattr	= generic_timeout_obj_to_nlattr,
		.nlattr_max	= CTA_TIMEOUT_GENERIC_MAX,
		.obj_size	= sizeof(unsigned int),
		.nla_policy	= generic_timeout_nla_policy,
	},
#endif /* CONFIG_NF_CT_NETLINK_TIMEOUT */
	.init_net		= generic_init_net,
	.get_net_proto		= generic_get_net_proto,
};
