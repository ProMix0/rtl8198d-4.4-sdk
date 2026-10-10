/* FTP extension for TCP NAT alteration. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <linux/netfilter_ipv4.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <linux/netfilter/nf_conntrack_ftp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rusty Russell <rusty@rustcorp.com.au>");
MODULE_DESCRIPTION("ftp NAT helper");
MODULE_ALIAS("ip_nat_ftp");

/* FIXME: Time out? --RR */

static int nf_nat_ftp_fmt_cmd(struct nf_conn *ct, enum nf_ct_ftp_type type,
			      char *buffer, size_t buflen,
			      union nf_inet_addr *addr, u16 port)
{
	switch (type) {
	case NF_CT_FTP_PORT:
	case NF_CT_FTP_PASV:
		return snprintf(buffer, buflen, "%u,%u,%u,%u,%u,%u",
				((unsigned char *)&addr->ip)[0],
				((unsigned char *)&addr->ip)[1],
				((unsigned char *)&addr->ip)[2],
				((unsigned char *)&addr->ip)[3],
				port >> 8,
				port & 0xFF);
	case NF_CT_FTP_EPRT:
		if (nf_ct_l3num(ct) == NFPROTO_IPV4)
			return snprintf(buffer, buflen, "|1|%pI4|%u|",
					&addr->ip, port);
		else
			return snprintf(buffer, buflen, "|2|%pI6|%u|",
					&addr->ip6, port);
	case NF_CT_FTP_EPSV:
		return snprintf(buffer, buflen, "|||%u|", port);
	}

	return 0;
}

/* So, this packet has hit the connection tracking matching code.
   Mangle it, and change the expectation to match the new version. */
static unsigned int nf_nat_ftp(struct sk_buff *skb,
			       enum ip_conntrack_info ctinfo,
			       enum nf_ct_ftp_type type,
			       unsigned int protoff,
			       unsigned int matchoff,
			       unsigned int matchlen,
			       struct nf_conntrack_expect *exp)
{
	union nf_inet_addr newaddr;
	u_int16_t port;
	int dir = CTINFO2DIR(ctinfo);
	struct nf_conn *ct = exp->master;
	char buffer[sizeof("|1||65535|") + INET6_ADDRSTRLEN];
	unsigned int buflen;
	#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	u16 ori_port = 0;
	u16 delta_port = 0;
	u16 delta = 0;
	struct nf_conntrack_tuple data_tuple;
	extern bool is_mape_session(void *nf_ct);
	extern __u16 ftp_alg_get_mape_available_port(void *nf_ct, __u16 port);
	extern __u16 ftp_alg_get_mape_delta_port(void *nf_ct, __u16 port, __u16 delta);
	#endif

	pr_debug("FTP_NAT: type %i, off %u len %u\n", type, matchoff, matchlen);

	/* Connection will come from wherever this packet goes, hence !dir */
	newaddr = ct->tuplehash[!dir].tuple.dst.u3;
	exp->saved_proto.tcp.port = exp->tuple.dst.u.tcp.port;
	exp->dir = !dir;

	/* When you see the packet, we need to NAT it the same as the
	 * this one. */
	exp->expectfn = nf_nat_follow_master;

	#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	if(is_mape_session(ct))
	{
		ori_port = ntohs(ct->tuplehash[!dir].tuple.dst.u.tcp.port);
		delta = ntohs(exp->saved_proto.tcp.port) - ntohs(ct->tuplehash[dir].tuple.src.u.tcp.port);
		
		if(delta_port = ftp_alg_get_mape_delta_port(ct, ori_port, delta))
		{
			
			for (port = delta_port; port != ori_port; port++) {
				int ret;
				u16 get_port = 0;

				if(get_port = ftp_alg_get_mape_available_port(ct, port))
				{
					pr_debug("[%s:%d] internal port = %d, ext port = %d, before port = %d, after port = %d, delta port = %d, delta = %d\n",
						__FUNCTION__,__LINE__,ntohs(ct->tuplehash[dir].tuple.src.u.tcp.port),ori_port, port, get_port,delta_port,delta);
					port = get_port;
				}

				data_tuple.src.u3 = ct->tuplehash[!dir].tuple.dst.u3;
				data_tuple.src.u.tcp.port = htons(port);
				data_tuple.src.l3num = nf_ct_l3num(ct);
	
				data_tuple.dst.u3 = ct->tuplehash[!dir].tuple.src.u3;
				data_tuple.dst.u.tcp.port = htons(FTP_DATA_PORT);
				data_tuple.dst.protonum = IPPROTO_TCP;
					
				if(nf_nat_used_tuple(&data_tuple, ct))
				{
					pr_debug("[%s:%d] port:%d tuple used!\n",__FUNCTION__,__LINE__,port);
					continue;
				}

				exp->tuple.dst.u.tcp.port = htons(port);
				ret = nf_ct_expect_related(exp);
				if (ret == 0)
					break;
				else if (ret != -EBUSY) {
					port = 0;
					break;
				}
			}
		}
		else
			pr_debug("[%s:%d] nat src port not in mape portset!\n",__FUNCTION__,__LINE__);
	}
	else{
	#endif
	/* Try to get same port: if not, try to change it. */
	for (port = ntohs(exp->saved_proto.tcp.port); port != 0; port++) {
		int ret;

		exp->tuple.dst.u.tcp.port = htons(port);
		ret = nf_ct_expect_related(exp);
		if (ret == 0)
			break;
		else if (ret != -EBUSY) {
			port = 0;
			break;
		}
	}
	#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	}
	#endif
	
	if (port == 0) {
		nf_ct_helper_log(skb, ct, "all ports in use");
		return NF_DROP;
	}

	buflen = nf_nat_ftp_fmt_cmd(ct, type, buffer, sizeof(buffer),
				    &newaddr, port);
	if (!buflen)
		goto out;

	pr_debug("calling nf_nat_mangle_tcp_packet\n");

	if (!nf_nat_mangle_tcp_packet(skb, ct, ctinfo, protoff, matchoff,
				      matchlen, buffer, buflen))
		goto out;

	return NF_ACCEPT;

out:
	nf_ct_helper_log(skb, ct, "cannot mangle packet");
	nf_ct_unexpect_related(exp);
	return NF_DROP;
}

static void __exit nf_nat_ftp_fini(void)
{
	RCU_INIT_POINTER(nf_nat_ftp_hook, NULL);
	synchronize_rcu();
}

static int __init nf_nat_ftp_init(void)
{
	BUG_ON(nf_nat_ftp_hook != NULL);
	RCU_INIT_POINTER(nf_nat_ftp_hook, nf_nat_ftp);
	return 0;
}

/* Prior to 2.6.11, we had a ports param.  No longer, but don't break users. */
static int warn_set(const char *val, struct kernel_param *kp)
{
	printk(KERN_INFO KBUILD_MODNAME
	       ": kernel >= 2.6.10 only uses 'ports' for conntrack modules\n");
	return 0;
}
module_param_call(ports, warn_set, NULL, NULL, 0);

module_init(nf_nat_ftp_init);
module_exit(nf_nat_ftp_fini);
