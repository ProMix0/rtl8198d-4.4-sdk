#ifndef CA_KERNEL_HOOK_API_H
#define CA_KERNEL_HOOK_API_H 1

/***************************************************************************
 * ca_kernel_hook_api.h
 *
 * This exports hook functions and definition for Linux.
 *
 ***************************************************************************/

#include <linux/netdevice.h>
#include <linux/if_pppox.h>
#include <net/ip_tunnels.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <net/ip6_tunnel.h>
#include <net/netfilter/nf_conntrack_tuple.h>

/***************************************************************************
 * proc 'hook'
 ***************************************************************************/

#define CA_KH_HOOK_IPV4_FORWARD			0x00000001
#define CA_KH_HOOK_IPV6_FORWARD			0x00000002
#define CA_KH_HOOK_IPV4_MULTICAST		0x00000004
#define CA_KH_HOOK_IPV6_MULTICAST		0x00000008
#define CA_KH_HOOK_NF_DROP			0x00000010
#define CA_KH_HOOK_PPPOE_KERNEL			0x00000020
#define CA_KH_HOOK_IPSEC			0x00000040
#define CA_KH_HOOK_L2TP				0x00000080
#define CA_KH_HOOK_DSLITE			0x00000100
#define CA_KH_HOOK_6RD				0x00000200
#define CA_KH_HOOK_WFO				0x00000400
#define CA_KH_HOOK_IPV4_IPSEC_PASSTHROUGH	0x00010000
#define CA_KH_HOOK_IPV6_IPSEC_PASSTHROUGH	0x00020000
#define CA_KH_HOOK_GRE_PASSTHROUGH		0x00040000
#define CA_KH_HOOK_IPV4_IN_IPV4_PASSTHROUGH	0x00100000
#define CA_KH_HOOK_IPV6_IN_IPV4_PASSTHROUGH	0x00200000 /* 6RD */
#define CA_KH_HOOK_IPV4_IN_IPV6_PASSTHROUGH	0x00400000 /* DS-Lite, MAP-E */
#define CA_KH_HOOK_IPV6_IN_IPV6_PASSTHROUGH	0x00800000

extern uint32_t ca_kh_hook;

/***************************************************************************
 * proc 'debug'
 ***************************************************************************/

#define CA_KH_DEBUG_INTERFACE			0x00000001
#define CA_KH_DEBUG_ARP				0x00000002
#define CA_KH_DEBUG_ROUTE			0x00000004
#define CA_KH_DEBUG_NAT				0x00000008
#define CA_KH_DEBUG_TUNNEL			0x00000010
#define CA_KH_DEBUG_MCAST			0x00000020
#define CA_KH_DEBUG_VLAN			0x00000040
#define CA_KH_DEBUG_BYPASS_PORTLIST		0x00000080
#define CA_KH_DEBUG_PORT_BINDING		0x00000100

extern uint32_t ca_kh_debug;

/***************************************************************************
 * proc 'udp_offload_occasion'
 ***************************************************************************/

enum {
	CA_KH_UDP_OFFLOAD_WHEN_ASSURED = 0,
	CA_KH_UDP_OFFLOAD_WHEN_CONFIRMED = 1,
};

extern uint32_t ca_kh_udp_offload_occasion;

/***************************************************************************
 * proc 'bypass portlist'
 ***************************************************************************/

enum {
	CA_BYPASS_CNT_INVALID,
	CA_BYPASS_CNT_VALID,
};

/***************************************************************************
 * common
 ***************************************************************************/

typedef unsigned int            __ca_uint32_t;
typedef int                     __ca_int32_t;
typedef unsigned short          __ca_uint16_t;
typedef short                   __ca_int16_t;
typedef unsigned char           __ca_uint8_t;
typedef signed char             __ca_int8_t;

typedef unsigned int 	 	__ca_status_t;
typedef __ca_uint16_t           __ca_dev_id_t;
typedef __ca_uint32_t           __ca_port_id_t;
typedef __ca_uint32_t           __ca_tunnel_id_t;
typedef __ca_uint32_t           __ca_l3_nexthop_id_t;

#define __CA_ETH_ADDR_LEN         6
typedef __ca_uint8_t __ca_mac_addr_t[__CA_ETH_ADDR_LEN];

#define CA_MCAST_MAX_ADDRESS          8

typedef enum {
	__CA_IPV4 = 0,
	__CA_IPV6 = 1,
} __ca_ip_afi_t; /* (to be defined newly if doesn't exist) */

typedef enum {
    __CA_MCAST_EXCLUDE = 0,
    __CA_MCAST_INCLUDE = 1
} __ca_mcast_filtermode_t;


typedef union {
	__ca_uint32_t		addr[4];
	__ca_uint32_t		ipv4_addr;
	__ca_uint32_t		ipv6_addr[4];
} __ca_l3_ip_addr_t;

typedef struct __ca_ip_address {
	__ca_ip_afi_t		afi;		/* address family identifier */
	__ca_l3_ip_addr_t	ip_addr;	/* IP address */
	__ca_uint8_t		addr_len;	/* length in bits */
} __ca_ip_address_t;

typedef struct __ca_l2_mcast_entry_s {
    __ca_uint16_t                 mcast_vlan;
    __ca_mac_addr_t               grp_mac_addr;
    __ca_mcast_filtermode_t       filter_mode;
    __ca_uint16_t                 src_num;                                    /* number of SA in list */
    __ca_ip_address_t             src_ip_address_list[CA_MCAST_MAX_ADDRESS];  /* incl/excl list */
} __ca_l2_mcast_entry_t;

typedef struct __net_bridge_port_s {
        struct net_device               *dev;
        struct list_head                list;
} __net_bridge_port_t;

struct kernel_hook_ops {
	__ca_status_t (*kho_l3_intf_add)(__ca_dev_id_t device_id, struct net_device *dev, __ca_ip_address_t *ip_addr, __ca_port_id_t port_id, __ca_tunnel_id_t tunnel_id);
	__ca_status_t (*kho_l3_intf_update_ip_addr)(__ca_dev_id_t device_id, int ifindex, __ca_ip_address_t *ip_addr);
	__ca_status_t (*kho_l3_intf_update_mtu)(__ca_dev_id_t device_id, int ifindex, int mtu);
	__ca_status_t (*kho_l3_intf_update_mac_addr)(__ca_dev_id_t device_id, int ifindex, u8 *mac_addr, int addr_len);
	__ca_status_t (*kho_l3_intf_delete)(__ca_dev_id_t device_id, __ca_ip_afi_t afi, struct net_device *dev);

	__ca_status_t (*kho_l3_route_add_ipv4_static)(__ca_dev_id_t device_id, int ifindex, struct fib_config *cfg);
	__ca_status_t (*kho_l3_route_add_ipv4_dynamic)(__ca_dev_id_t device_id, int ifindex, __ca_uint32_t daddr);
	__ca_status_t (*kho_l3_route_del_ipv4_ifdown)(__ca_dev_id_t device_id, struct net_device *dev);
	__ca_status_t (*kho_l3_route_del_ipv4_static)(__ca_dev_id_t device_id, int ifindex, struct fib_config *cfg);
	__ca_status_t (*kho_l3_route_add_ipv6_static)(__ca_dev_id_t device_id, int ifindex, struct fib6_config *cfg);
	__ca_status_t (*kho_l3_route_del_ipv6_static)(__ca_dev_id_t device_id, struct rt6_info *rt);
	__ca_status_t (*kho_l3_route_del_ipv6_ifdown)(__ca_dev_id_t device_id, struct net_device *dev);
	__ca_status_t (*kho_l3_nexthop_aging_timer_set)(__ca_dev_id_t device_id, __ca_uint32_t time);

	__ca_status_t (*kho_neigh_add)(__ca_dev_id_t device_id, int ifindex, __ca_uint8_t *da_mac, __ca_ip_address_t *ip_addr, int child_ifindex, bool is_static);
	__ca_status_t (*kho_neigh_delete)(__ca_dev_id_t device_id, int ifindex, __ca_ip_address_t *ip_addr, bool is_static);
	__ca_status_t (*kho_neigh_update)(__ca_dev_id_t device_id, int ifindex, __ca_uint8_t *da_mac, __ca_ip_address_t *ip_addr, int child_ifindex, bool is_static);
	__ca_status_t (*kho_neigh_update_used)(__ca_dev_id_t device_id, __ca_int32_t ifindex, __ca_ip_address_t *ip_addr, __ca_uint32_t *used_jiffies);

	__ca_status_t (*kho_nat_entry_add)(__ca_dev_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, __ca_uint32_t old_src_ip, __ca_uint32_t new_src_ip,
                                __ca_uint32_t old_dst_ip, __ca_uint32_t new_dst_ip, __ca_uint16_t old_src_port, __ca_uint16_t new_src_port,
                                __ca_uint16_t old_dst_port, __ca_uint16_t new_dst_port, __ca_int32_t ifindex);
	__ca_status_t (*kho_nat_entry_delete)(__ca_dev_id_t device_id, __ca_uint32_t ip_addr);
	__ca_status_t (*kho_nat_entry_session_delete)(__ca_dev_id_t device_id, uint8_t ip_proto, __ca_uint32_t old_src_ip, __ca_uint32_t old_dst_ip,
                                           __ca_uint16_t old_src_port, __ca_uint16_t old_dst_port);
	__ca_status_t (*kho_nat_entry_session_delete_force)(__ca_dev_id_t device_id, struct nf_conntrack_tuple *tuple);
	__ca_status_t (*kho_nat_entry_delete_by_port)(__ca_dev_id_t device_id, __ca_uint16_t port);
	__ca_status_t (*kho_nat_entry_timer_refresh)(__ca_dev_id_t device_id, struct nf_conntrack_tuple *tuple1, struct nf_conntrack_tuple *tuple2, __ca_uint32_t *aging_time);

	__ca_status_t (*kho_nat6_entry_add)(__ca_dev_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, __ca_uint32_t *old_src_ip, __ca_uint32_t *new_src_ip,
                                __ca_uint32_t *old_dst_ip, __ca_uint32_t *new_dst_ip, __ca_uint16_t old_src_port, __ca_uint16_t new_src_port,
                                __ca_uint16_t old_dst_port, __ca_uint16_t new_dst_port, __ca_int32_t ifindex);
	__ca_status_t (*kho_nat6_entry_delete)(__ca_dev_id_t device_id, __ca_uint32_t *ip_addr);

	__ca_status_t (*kho_dslite_add)(__ca_dev_id_t device_id, int ifindex, struct __ip6_tnl_parm *p, __ca_tunnel_id_t *ret_tunnel_id);
	__ca_status_t (*kho_dslite_delete)(__ca_dev_id_t device_id, int ifindex, __ca_tunnel_id_t kept_tunnel_id);
	__ca_status_t (*kho_dslite_delete_intf_only)(__ca_dev_id_t device_id, int ifindex);

	__ca_status_t (*kho_6rd_add)(__ca_dev_id_t device_id, int ifindex, struct ip_tunnel *t);
	__ca_status_t (*kho_6rd_delete)(__ca_dev_id_t device_id, int ifindex);
	__ca_status_t (*kho_6rd_update)(__ca_dev_id_t device_id, int ifindex, struct ip_tunnel *t);

	__ca_status_t (*kho_map_e_add)(__ca_dev_id_t device_id, int ifindex, struct __ip6_tnl_parm *p);
	__ca_status_t (*kho_map_e_delete)(__ca_dev_id_t device_id, int ifindex);

	__ca_status_t (*kho_pppoe_add)(__ca_dev_id_t device_id, struct net_device *ppp_dev, struct net_device *eth_dev, struct pppox_sock *po);
	__ca_status_t (*kho_pppoe_delete)(__ca_dev_id_t device_id, int ifindex);
	__ca_status_t (*kho_pppoe_update)(__ca_dev_id_t device_id, struct net_device *ppp_dev);

	__ca_status_t (*kho_ipsec_add)(__ca_dev_id_t device_id, struct xfrm_state *x_in, struct xfrm_policy *xp_in, struct xfrm_state *x_out, struct xfrm_policy *xp_out);
	__ca_status_t (*kho_ipsec_delete)(__ca_dev_id_t device_id, struct xfrm_policy *x_in, struct xfrm_policy *x_out);
	__ca_status_t (*kho_ipsec_redirect_to_pe)(struct sk_buff *skb, struct xfrm_state *x, __ca_ip_afi_t afi, unsigned char dir);

	__ca_status_t (*kho_l2_mcast_member_add)(__ca_dev_id_t device_id, __ca_uint8_t *device_name, __ca_ip_address_t *group_addr, __ca_uint16_t vid);
	__ca_status_t (*kho_l2_mcast_member_delete)(__ca_dev_id_t device_id, __ca_uint8_t *device_name, __ca_ip_address_t *group_addr, __ca_uint16_t vid);
	__ca_status_t (*kho_mcast_set_max_grp_number)(__ca_dev_id_t device_id, __ca_port_id_t port_id, __ca_uint32_t max_grp_number);
	__ca_status_t (*kho_mcast_get_max_grp_number)(__ca_dev_id_t device_id, __ca_port_id_t port_id, __ca_uint32_t *max_grp_number, __ca_uint8_t *cur_mcast_grp_number);
	__ca_status_t (*kho_mcast_set_multicast_snooping)(__ca_dev_id_t device_id, __ca_uint8_t mcast_snooping);
	__ca_status_t (*kho_mcast_get_multicast_snooping)(__ca_dev_id_t device_id, __ca_uint8_t *mcast_snooping);
	__ca_status_t (*kho_mcast_set_multicast_fastleave)(__ca_dev_id_t device_id, __ca_uint8_t mcast_fastleave);
	__ca_status_t (*kho_mcast_get_multicast_fastleave)(__ca_dev_id_t device_id, __ca_uint8_t *mcast_fastleave);

	__ca_status_t (*kho_bypass_tcp_portlist_add)(__ca_dev_id_t dev_id, __ca_uint16_t port, __ca_uint8_t flag);
	__ca_status_t (*kho_bypass_tcp_portlist_delete)(__ca_dev_id_t dev_id, __ca_uint16_t port, __ca_uint8_t flag);
	void (*kho_bypass_tcp_portlist_dump)(void);
	int (*kho_bypass_tcp_portlist_exist)(__ca_uint16_t port);

	__ca_status_t (*kho_bypass_udp_portlist_add)(__ca_dev_id_t dev_id, __ca_uint16_t port, __ca_uint8_t flag);
	__ca_status_t (*kho_bypass_udp_portlist_delete)(__ca_dev_id_t dev_id, __ca_uint16_t port, __ca_uint8_t flag);
	void (*kho_bypass_udp_portlist_dump)(void);
	int (*kho_bypass_udp_portlist_exist)(__ca_uint16_t port);

	__ca_status_t (*kho_8021q_pcp_handler)(struct sk_buff *skb);

	__ca_status_t (*kho_port_binding_check)(__ca_dev_id_t device_id, struct sk_buff *skb);

	__ca_status_t (*kho_l2_vlan_add)(__ca_dev_id_t device_id, struct net_device *parent_dev, int vid);
	__ca_status_t (*kho_l2_vlan_delete)(__ca_dev_id_t device_id, struct net_device *dev);
	__ca_status_t (*kho_l2_br_add_if)(__ca_dev_id_t device_id, __net_bridge_port_t *br_port);
};

extern struct kernel_hook_ops ca_kernel_hook_ops;

#endif /* CA_KERNEL_HOOK_API_H */

