#ifndef CA_KERNEL_HOOK_H
#define CA_KERNEL_HOOK_H 1

/***************************************************************************
 * ca_kernel_hook.h
 *
 * This is used internally in Kernel Hook module.
 *
 * IMPORTANT: DO NOT INCLUDE THIS IN LINUX TREE.
 *
 ***************************************************************************/

#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/if_pppox.h>
#include <net/ip_tunnels.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <net/ip6_tunnel.h>
#include <net/netfilter/nf_conntrack_tuple.h>

#include "nat.h"
#include "nexthop.h"
#include "route.h"
#include "interface.h"
#include "tunnel.h"
#include "mcast.h"

#include <soc/cortina/ca_kernel_hook_api.h>

/* NAT Translation Flags */
#define CA_NAT_TRANSLATE_SRC_IP			(1 << 0)	/* Enable change SIP */
#define CA_NAT_TRANSLATE_DST_IP			(1 << 1)	/* Enable change DIP */
#define CA_NAT_TRANSLATE_L4_PORT		(1 << 2)	/* Enable change port ID */
#define CA_NAT_TRANSLATE_STATIC			(1 << 3)	/* static flag */

/***************************************************************************
 * proc 'tcp_bypass_port'
 ***************************************************************************/


/***************************************************************************
 * proc 'udp_bypass_port'
 ***************************************************************************/


/***************************************************************************
 * proc 'expected_master_bypass_port'
 ***************************************************************************/


/***************************************************************************
 * proc 'hook'
 ***************************************************************************/

extern uint32_t ca_kh_hook;

/***************************************************************************
 * proc 'dump'
 ***************************************************************************/

#define CA_KH_DUMP_CLASSIFIER_TABLE		1
#define CA_KH_DUMP_INTF_TABLE			2
#define CA_KH_DUMP_NAT_TABLE			3
#define CA_KH_DUMP_NEXTHOP_TABLE		4
#define CA_KH_DUMP_ROUTE_TABLE			5
#define CA_KH_DUMP_TUNNEL_TABLE			6
#define CA_KH_DUMP_KH_NEXTHOP_TABLE		7
#define CA_KH_DUMP_IFINDEX			8
#define CA_KH_DUMP_L2_MULTICAST_TABLE		9
#define CA_KH_DUMP_L3_MULTICAST_TABLE		10
#define CA_KH_DUMP_L2_VLAN_TABLE		11
#define CA_KH_DUMP_LIMIT_TABLE			12
#define CA_KH_DUMP_LAN_DEV_TABLE		13
#define CA_KH_DUMP_PPPOE_DATA			14
#define CA_KH_DUMP_PORT_BINDING			15

#define CA_KH_CLEAR_MULTICAST_MAX_GRP		0xE0000000
#define CA_KH_DEL_NAT_ENTRY			0xF0000000

/***************************************************************************
 * proc 'debug'
 ***************************************************************************/

extern ca_uint32_t ca_kh_debug;

/***************************************************************************
 * proc 'udp_offload_occasion'
 ***************************************************************************/

extern ca_uint32_t ca_kh_udp_offload_occasion;

/***************************************************************************
 * others
 ***************************************************************************/

/* this array use intf_id as array index */
typedef struct {
	ca_ip_afi_t		afi;
	ca_int32_t		ifindex;
	ca_l3_intf_type_t	intf_type;
	ca_tunnel_type_t	tunnel_type;
	ca_boolean_t		is_mcast_intf; /* for DSLite/6RD */
} ca_kh_ifindex_entry_t;

/* there is only one interface table for IPV4/IPV6 */
//extern ca_uint32_t ca_kh_l3_ifindex_ipv4_array[];
//extern ca_uint32_t ca_kh_l3_ifindex_ipv6_array[];
extern ca_kh_ifindex_entry_t ca_kh_l3_ifindex_array[];

typedef struct {
	ca_uint16_t		is_used;
	ca_l3_nexthop_attr_t	attr_flags; /* Nexthop attribute flags */
	ca_ip_address_t		ip_addr;
	ca_int32_t		ifindex;
	ca_l3_nexthop_id_t	l3_nexthop_id;
} ca_kh_nexthop_entry_t;


extern ca_kh_nexthop_entry_t ca_kh_ipv4_nexthop_table[];
extern ca_kh_nexthop_entry_t ca_kh_ipv6_nexthop_table[];

typedef struct {
	ca_uint16_t		is_used;
	ca_tunnel_type_t	type;
	ca_ip_address_t		dest_addr;
	ca_ip_address_t		src_addr;
	ca_tunnel_id_t		tunnel_id;
	int			ifindex;	/* ifindex of tunnel interface */
} ca_kh_tunnel_entry_t;

extern ca_kh_tunnel_entry_t ca_kh_tunnel_table[];

/* there is only one interface table for IPV4/IPV6 */
//extern spinlock_t ca_kh_ipv4_intf_lock;
//extern spinlock_t ca_kh_ipv6_intf_lock;
extern spinlock_t ca_kh_intf_lock;
extern spinlock_t ca_kh_vlan_lock;

extern spinlock_t ca_kh_ipv4_nh_lock;
extern spinlock_t ca_kh_ipv6_nh_lock;
extern spinlock_t ca_kh_tnl_lock;
extern spinlock_t ca_kh_wan_map_lock;
extern spinlock_t ca_kh_gw_lock;

ca_status_t ca_kh_l3_get_ifindex_by_intf_id(ca_device_id_t device_id, ca_ip_afi_t *afi, ca_intf_id_t intf_id, ca_int32_t *ifindex);
ca_status_t ca_kh_l3_get_intf_id_by_dev(ca_device_id_t device_id, ca_ip_afi_t afi, struct net_device *dev, ca_l3_intf_type_t intf_type, bool allocate, ca_intf_id_t *intf_id);
ca_status_t ca_kh_l3_get_intf_id_by_ifindex(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int32_t ifindex, ca_l3_intf_type_t intf_type, bool allocate, ca_intf_id_t *intf_id);
ca_status_t ca_kh_l3_set_intf_id(ca_device_id_t device_id, ca_intf_id_t intf_id, ca_kh_ifindex_entry_t *entry);
ca_status_t ca_kh_l3_intf_get_intf_id_by_ip_addr(ca_device_id_t device_id, ca_l3_ip_addr_t *ip_addr, ca_intf_id_t *got_intf_id);
ca_status_t ca_kh_l3_tunnel_get_intf_id_by_ifindex(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int32_t ifindex, ca_tunnel_type_t tunnel_type, bool allocate, ca_intf_id_t *intf_id);
ca_status_t ca_kh_l3_tunnel_set_intf_id(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int32_t ifindex, ca_intf_id_t intf_id,
		ca_tunnel_type_t tunnel_type);

ca_status_t ca_kh_l3_intf_add(ca_device_id_t device_id, struct net_device *dev, ca_ip_address_t *ip_addr, ca_port_id_t port_id, ca_tunnel_id_t tunnel_id);
ca_status_t ca_kh_l3_intf_update_ip_addr(ca_device_id_t device_id, int ifindex, ca_ip_address_t *ip_addr);
ca_status_t ca_kh_l3_intf_update_mtu(ca_device_id_t device_id, int ifindex, int mtu);
ca_status_t ca_kh_l3_intf_update_mac_addr(ca_device_id_t device_id, int ifindex, u8 *mac_addr, int addr_len);
ca_status_t ca_kh_l3_intf_update_nat_enable(ca_device_id_t device_id, int ifindex, int nat_enable);
ca_status_t ca_kh_l3_intf_delete(ca_device_id_t device_id, ca_ip_afi_t afi, struct net_device *dev);

ca_status_t ca_kh_l3_nexthop_add(ca_device_id_t device_id, ca_kh_nexthop_entry_t *kh_nexthop);
ca_status_t ca_kh_l3_nexthop_get(ca_device_id_t device_id, ca_kh_nexthop_entry_t *kh_nexthop);
ca_status_t ca_kh_l3_nexthop_delete(ca_device_id_t device_id, ca_ip_afi_t afi, ca_l3_nexthop_id_t l3_nexthop_id);

ca_status_t ca_kh_l3_route_add(ca_device_id_t device_id, ca_l3_nexthop_t *l3_nexthop, ca_l3_route_t *l3_route, ca_int32_t ifindex);
ca_status_t ca_kh_l3_route_add_ipv4_static(ca_device_id_t device_id, int ifindex, struct fib_config *cfg);
ca_status_t ca_kh_l3_route_add_ipv4_dynamic(ca_device_id_t device_id, int ifindex, ca_uint32_t daddr);
ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
ca_status_t ca_kh_l3_route_delete(ca_device_id_t device_id, ca_l3_route_t *l3_route, ca_int32_t ifindex);
ca_status_t ca_kh_l3_route_del_ipv4_ifdown(ca_device_id_t device_id, struct net_device *dev);
ca_status_t ca_kh_l3_route_del_ipv4_static(ca_device_id_t device_id, int ifindex, struct fib_config *cfg);
ca_status_t ca_kh_l3_route_add_ipv6_static(ca_device_id_t device_id, int ifindex, struct fib6_config *cfg);
ca_status_t ca_kh_l3_route_del_ipv6_static(ca_device_id_t device_id, struct rt6_info *rt);
ca_status_t ca_kh_l3_route_del_ipv6_ifdown(ca_device_id_t device_id, struct net_device *dev);
ca_status_t ca_kh_l3_nexthop_aging_timer_set(ca_device_id_t device_id, ca_uint32_t time);

ca_status_t ca_kh_neigh_add(ca_device_id_t device_id, int ifindex, ca_uint8_t *da_mac, ca_ip_address_t *ip_addr, int child_ifindex, bool is_static);
ca_status_t ca_kh_neigh_delete(ca_device_id_t device_id, int ifindex, ca_ip_address_t *ip_addr, bool is_static);
ca_status_t ca_kh_neigh_update(ca_device_id_t device_id, int ifindex, ca_uint8_t *da_mac, ca_ip_address_t *ip_addr, int child_ifindex, bool is_static);
ca_status_t ca_kh_neigh_update_used(ca_device_id_t device_id, ca_int32_t ifindex, ca_ip_address_t *ip_addr, ca_uint32_t *used_jiffies);

ca_status_t ca_kh_nat_entry_add(ca_device_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, ca_uint32_t old_src_ip, ca_uint32_t new_src_ip,
		ca_uint32_t old_dst_ip, ca_uint32_t new_dst_ip, ca_uint16_t old_src_port, ca_uint16_t new_src_port,
		ca_uint16_t old_dst_port, ca_uint16_t new_dst_port, ca_int32_t ifindex);
ca_status_t ca_kh_nat_entry_delete(ca_device_id_t device_id, ca_uint32_t ip_addr);
ca_status_t ca_kh_nat_entry_session_delete(ca_device_id_t device_id, uint8_t ip_proto, ca_uint32_t old_src_ip, ca_uint32_t old_dst_ip,
		ca_uint16_t old_src_port, ca_uint16_t old_dst_port);
ca_status_t ca_kh_nat_entry_session_delete_force(ca_device_id_t device_id, struct nf_conntrack_tuple *tuple);
ca_status_t ca_kh_nat_entry_delete_by_port(ca_device_id_t device_id, ca_uint16_t port);
ca_status_t ca_kh_nat_entry_timer_refresh(ca_device_id_t device_id, struct nf_conntrack_tuple *tuple1, struct nf_conntrack_tuple *tuple2, ca_uint32_t *aging_time);

ca_status_t ca_kh_nat6_entry_add(ca_device_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, ca_uint32_t *old_src_ip, ca_uint32_t *new_src_ip,
		ca_uint32_t *old_dst_ip, ca_uint32_t *new_dst_ip, ca_uint16_t old_src_port, ca_uint16_t new_src_port,
		ca_uint16_t old_dst_port, ca_uint16_t new_dst_port, ca_int32_t ifindex);
ca_status_t ca_kh_nat6_entry_delete(ca_device_id_t device_id, ca_uint32_t *ip_addr);

ca_status_t ca_kh_l3_tunnel_add(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel);
ca_status_t ca_kh_l3_tunnel_get(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel);
ca_status_t ca_kh_l3_tunnel_delete(ca_device_id_t device_id, ca_tunnel_id_t tunnel_id);

ca_status_t ca_kh_dslite_add(ca_device_id_t device_id, int ifindex, struct __ip6_tnl_parm *p, ca_tunnel_id_t *ret_tunnel_id);
ca_status_t ca_kh_dslite_delete(ca_device_id_t device_id, int ifindex, ca_tunnel_id_t kept_tunnel_id);
ca_status_t ca_kh_dslite_delete_intf_only(ca_device_id_t device_id, int ifindex);

ca_status_t ca_kh_6rd_add(ca_device_id_t device_id, int ifindex, struct ip_tunnel *t);
ca_status_t ca_kh_6rd_delete(ca_device_id_t device_id, int ifindex);
ca_status_t ca_kh_6rd_update(ca_device_id_t device_id, int ifindex, struct ip_tunnel *t);

ca_status_t ca_kh_map_e_add(ca_device_id_t device_id, int ifindex, struct __ip6_tnl_parm *p);
ca_status_t ca_kh_map_e_delete(ca_device_id_t device_id, int ifindex);

ca_status_t ca_kh_pppoe_add(ca_device_id_t device_id, struct net_device *ppp_dev, struct net_device *eth_dev, struct pppox_sock *po);
ca_status_t ca_kh_pppoe_delete(ca_device_id_t device_id, int ifindex);
ca_status_t ca_kh_pppoe_update(ca_device_id_t device_id, struct net_device *ppp_dev);

ca_status_t ca_kh_ipsec_add(ca_device_id_t device_id, struct xfrm_state *x_in, struct xfrm_policy *xp_in, struct xfrm_state *x_out, struct xfrm_policy *xp_out);
ca_status_t ca_kh_ipsec_delete(ca_device_id_t device_id, struct xfrm_policy *xp_in, struct xfrm_policy *xp_out);
ca_status_t ca_kh_ipsec_redirect_to_pe(struct sk_buff *skb, struct xfrm_state *x, __ca_ip_afi_t afi, unsigned char dir);

ca_status_t ca_kh_l2_mcast_member_add(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr, ca_uint16_t vid);
ca_status_t ca_kh_l2_mcast_member_delete(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr, ca_uint16_t vid);
ca_status_t ca_kh_l2_mcast_member_add_allport(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr);
ca_status_t ca_kh_l2_mcast_member_delete_allport(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr);
ca_status_t ca_kh_mcast_set_max_group_number(ca_device_id_t device_id, ca_port_id_t port_id, ca_uint32_t max_grp_number);
ca_status_t ca_kh_mcast_get_max_group_number(ca_device_id_t device_id, ca_port_id_t port_id, ca_uint32_t *max_grp_number, ca_uint8_t *cur_mcast_grp_number);
ca_status_t ca_kh_mcast_set_multicast_snooping(ca_device_id_t device_id, ca_uint8_t mcast_snooping);
ca_status_t ca_kh_mcast_get_multicast_snooping(ca_device_id_t device_id, ca_uint8_t *mcast_snooping);
ca_status_t ca_kh_mcast_set_multicast_fastleave(ca_device_id_t device_id, ca_uint8_t mcast_fastleave);
ca_status_t ca_kh_mcast_get_multicast_fastleave(ca_device_id_t device_id, ca_uint8_t *mcast_fastleave);

#if 0
ca_status_t ca_kh_portal_init(void);
ca_status_t ca_kh_portal_fini(void);
ca_status_t ca_kh_portal_list_del_dev(ca_ip_afi_t afi, ca_l3_ip_addr_t *ip, ca_mac_addr_t mac, ca_kh_dev_type_t type);
ca_status_t ca_kh_portal_list_dump(void);

ca_status_t ca_kh_limit_init(void);
ca_status_t ca_kh_limit_fini(void);
ca_status_t ca_kh_limit_list_del_dev(ca_ip_afi_t afi, ca_l3_ip_addr_t *ip, ca_mac_addr_t mac, ca_kh_dev_type_t type);
ca_status_t ca_kh_limit_list_dump(void);

ca_status_t ca_kh_lan_dev_init(void);
ca_status_t ca_kh_lan_dev_fini(void);
ca_status_t ca_kh_lan_dev_get_type(ca_ip_afi_t afi, ca_l3_ip_addr_t *ip, ca_mac_addr_t mac, ca_kh_dev_type_t *type);
ca_status_t ca_kh_lan_dev_dump(void);
#endif

ca_uint8 ca_kh_is_wan_intf(ca_int32_t ifindex);

ca_status_t ca_kh_mcast_init(void);

ca_status_t ca_kh_pppoe_init(void);

ca_status_t ca_kh_bypass_tcp_portlist_add(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag);
ca_status_t ca_kh_bypass_tcp_portlist_delete(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag);
void ca_kh_bypass_tcp_portlist_dump(void);
int ca_kh_bypass_tcp_portlist_exist(ca_uint16_t port);
ca_status_t ca_kh_bypass_tcp_portlist_init(void);
void ca_kh_mcast_flush_max_group_number(void);
ca_status_t ca_kh_mcast_member_delete_all(ca_device_id_t device_id);

ca_status_t ca_kh_bypass_udp_portlist_add(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag);
ca_status_t ca_kh_bypass_udp_portlist_delete(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag);
void ca_kh_bypass_udp_portlist_dump(void);
int ca_kh_bypass_udp_portlist_exist(ca_uint16_t port);
ca_status_t ca_kh_bypass_udp_portlist_init(void);

ca_status_t ca_kh_tr069_intf_get(ca_int32_t *ifindex, ca_uint8_t *cos);
ca_status_t ca_kh_tr069_intf_clean(void);

ca_status_t ca_kh_8021q_pcp_handler(struct sk_buff *skb);

/* for multiple route */
/* bitwise flags of WAN type */
typedef enum {
	CA_WAN_TYPE_NONE	= 0,
	CA_WAN_TYPE_TR069	= 0x01,
	CA_WAN_TYPE_INTERNET	= 0x02,
	CA_WAN_TYPE_VOIP	= 0x04,
	CA_WAN_TYPE_OTHER	= 0x08,
	CA_WAN_TYPE_MAX		= 0x0F
} ca_wan_type_t;

typedef struct ca_int_wan_map_s {
	ca_boolean_t	valid;
	ca_boolean_t	routable;
	ca_uint8_t	wan_type;	/* bitwise flag defined in ca_wan_type_t */
	ca_uint8_t	if_name[IFNAMSIZ];
	ca_int32_t	ifindex;
} ca_int_wan_map_t;

#define CA_MAX_WAN_NUM		8

typedef struct ca_int_l3_gw_s {
	ca_device_id_t	device_id;
	ca_uint8_t	wan_type;	/* bitwise flag defined in ca_wan_type_t */
	ca_uint32_t	ifindex;
	ca_l3_nexthop_t	l3_nexthop;
	ca_l3_route_t	l3_route;
	struct ca_int_l3_gw_s *next, *prev;
} ca_int_l3_gw_t;

ca_status_t ca_int_wan_map_set(ca_uint32_t wan_id, ca_int_wan_map_t *wan_map);
ca_status_t ca_int_wan_map_get(ca_uint32_t wan_id, ca_int_wan_map_t *wan_map);
ca_status_t ca_int_wan_map_find(ca_uint8_t *if_name, ca_uint32_t *wan_id, ca_int_wan_map_t *wan_map);
ca_status_t ca_int_l3_gw_add(ca_ip_afi_t afi, ca_int_l3_gw_t *gw);
ca_status_t ca_int_l3_gw_del(ca_ip_afi_t afi, ca_uint32_t ifindex);
ca_status_t ca_int_l3_gw_upd(ca_uint32_t ifindex, ca_ip_address_t *ip, ca_uint8_t *mac);
ca_status_t ca_int_l3_gw_get(ca_ip_afi_t afi, ca_uint32_t ifindex, ca_int_l3_gw_t *gw);

ca_status_t ca_kh_port_binding_check(ca_device_id_t device_id, struct sk_buff *skb);

ca_status_t ca_kh_l2_vlan_add(ca_device_id_t device_id, struct net_device *parent_dev, int vid);
ca_status_t ca_kh_l2_vlan_delete(ca_device_id_t device_id, struct net_device *dev);
ca_status_t ca_kh_l2_br_add_if(ca_device_id_t device_id, __net_bridge_port_t *br_port);

#endif /* CA_KERNEL_HOOK_H */

