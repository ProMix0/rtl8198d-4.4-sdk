#ifndef _NET_IP6_TUNNEL_H
#define _NET_IP6_TUNNEL_H

#include <linux/ipv6.h>
#include <linux/netdevice.h>
#include <linux/if_tunnel.h>
#include <linux/ip6_tunnel.h>
#include <net/ip_tunnels.h>
#include <net/dst_cache.h>

#define IP6TUNNEL_ERR_TIMEO (30*HZ)

/* capable of sending packets */
#define IP6_TNL_F_CAP_XMIT 0x10000
/* capable of receiving packets */
#define IP6_TNL_F_CAP_RCV 0x20000
/* determine capability on a per-packet basis */
#define IP6_TNL_F_CAP_PER_PACKET 0x40000

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
struct mape_psid_info{
	struct in6_addr laddr;
	struct in6_addr raddr;

	__u8 psid_offset;
	__u8 psid_len;
	__u16 psid;
};

struct __ip6_tnl_psid{
	__u8 set;
	struct mape_psid_info psid_info;
	rwlock_t	portset_id_lock;
	__u16 kernel_ipid;
	__u16 fc_ipid;
};
#endif

/* IPv6 tunnel FMR */
struct ip6_tnl_fmr_rule{
	struct in6_addr ip6_prefix;
	struct in_addr ip4_prefix;

	__u8 ip6_prefix_len;
	__u8 ip4_prefix_len;
	__u8 ea_len;
	__u8 offset;
};

struct ipv6_tnl_fmr{
	struct in6_addr laddr;
	struct in6_addr raddr;
	struct ip6_tnl_fmr_rule rule;
};

struct __ip6_tnl_fmr {
	struct __ip6_tnl_fmr *next; /* next fmr in list */
	struct ip6_tnl_fmr_rule rule;
};

struct __ip6_tnl_parm {
	char name[IFNAMSIZ];	/* name of tunnel device */
	int link;		/* ifindex of underlying L2 interface */
	__u8 proto;		/* tunnel protocol */
	__u8 encap_limit;	/* encapsulation limit for tunnel */
	__u8 hop_limit;		/* hop limit for tunnel */
	__be32 flowinfo;	/* traffic class and flowlabel for tunnel */
	__u32 flags;		/* tunnel flags */
	struct in6_addr laddr;	/* local tunnel end-point address */
	struct in6_addr raddr;	/* remote tunnel end-point address */
	struct __ip6_tnl_fmr *fmrs;	/* FMRs */
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	struct __ip6_tnl_psid mape_psid; /* psid */
#endif
	__be16			i_flags;
	__be16			o_flags;
	__be32			i_key;
	__be32			o_key;
};

/* IPv6 tunnel */
struct ip6_tnl {
	struct ip6_tnl __rcu *next;	/* next tunnel in list */
	struct net_device *dev;	/* virtual device associated with tunnel */
	struct net *net;	/* netns for packet i/o */
	struct __ip6_tnl_parm parms;	/* tunnel configuration parameters */
	struct flowi fl;	/* flowi template for xmit */
	struct dst_cache dst_cache;	/* cached dst */

	int err_count;
	unsigned long err_time;

	/* These fields used only by GRE */
	__u32 i_seqno;	/* The last seen seqno	*/
	__u32 o_seqno;	/* The last output seqno */
	int hlen;       /* Precalculated GRE header length */
	int mlink;
#ifdef CONFIG_CORTINA_KERNEL_HOOK
	uint32_t tunnel_id;
#endif
};

/* Tunnel encapsulation limit destination sub-option */

struct ipv6_tlv_tnl_enc_lim {
	__u8 type;		/* type-code for option         */
	__u8 length;		/* option length                */
	__u8 encap_limit;	/* tunnel encapsulation limit   */
} __packed;

int ip6_tnl_rcv_ctl(struct ip6_tnl *t, const struct in6_addr *laddr,
		const struct in6_addr *raddr);
int ip6_tnl_xmit_ctl(struct ip6_tnl *t, const struct in6_addr *laddr,
		     const struct in6_addr *raddr);
__u16 ip6_tnl_parse_tlv_enc_lim(struct sk_buff *skb, __u8 *raw);
__u32 ip6_tnl_get_cap(struct ip6_tnl *t, const struct in6_addr *laddr,
			     const struct in6_addr *raddr);
struct net *ip6_tnl_get_link_net(const struct net_device *dev);
int ip6_tnl_get_iflink(const struct net_device *dev);

static inline void ip6tunnel_xmit(struct sock *sk, struct sk_buff *skb,
				  struct net_device *dev)
{
	struct net_device_stats *stats = &dev->stats;
	int pkt_len, err;

	memset(skb->cb, 0, sizeof(struct inet6_skb_parm));
	pkt_len = skb->len - skb_inner_network_offset(skb);
	err = ip6_local_out(dev_net(skb_dst(skb)->dev), sk, skb);

	if (net_xmit_eval(err) == 0) {
		struct pcpu_sw_netstats *tstats = get_cpu_ptr(dev->tstats);
		u64_stats_update_begin(&tstats->syncp);
		tstats->tx_bytes += pkt_len;
		tstats->tx_packets++;
		u64_stats_update_end(&tstats->syncp);
		put_cpu_ptr(tstats);
	} else {
		stats->tx_errors++;
		stats->tx_aborted_errors++;
	}
}

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
bool is_mape_session(void *nf_ct);/*Return Value: false-no, true-yes*/
bool mape_is_port_in_portset_range(char *tun_name, __u16 port);
bool ftp_alg_port_is_in_mape_portset(void *nf_ct, __u16 port);/*Return Value: false-no, true-yes*/
__u16 ftp_alg_get_mape_available_port(void *nf_ct, __u16 port);/*Return Value: 0-error, >0-available port*/

#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
__u16 kernel_get_mape_ipid(char *tun_name);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
__u16 fc_get_mape_ipid(char *tun_name);	
#endif
#endif //CONFIG_IPV6_MAPE_IPID_ADJUST

#endif // CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK

#endif
