#include <linux/module.h>
#include <linux/types.h>

#include "ca_types.h"
#include "ca-scfg/scfg.h"

#include <soc/cortina/ca_kernel_hook.h>

ca_kh_nexthop_entry_t ca_kh_ipv4_nexthop_table[CA_L3_MAX_NEXTHOP_NUM];
EXPORT_SYMBOL(ca_kh_ipv4_nexthop_table);

ca_kh_nexthop_entry_t ca_kh_ipv6_nexthop_table[CA_L3_MAX_NEXTHOP_NUM];
EXPORT_SYMBOL(ca_kh_ipv6_nexthop_table);

ca_kh_tunnel_entry_t ca_kh_tunnel_table[CA_TUNNEL_MAX_LIMIT];

/* there is only one interface table for IPV4/IPV6 */
//ca_uint32_t ca_kh_l3_ifindex_ipv4_array[CA_L3_IP_MAX_INTF];
//ca_uint32_t ca_kh_l3_ifindex_ipv6_array[CA_L3_IP_MAX_INTF];
//DEFINE_SPINLOCK(ca_kh_ipv4_intf_lock);
//DEFINE_SPINLOCK(ca_kh_ipv6_intf_lock);
ca_kh_ifindex_entry_t ca_kh_l3_ifindex_array[CA_L3_IP_MAX_INTF];

DEFINE_SPINLOCK(ca_kh_intf_lock);
DEFINE_SPINLOCK(ca_kh_vlan_lock);

DEFINE_SPINLOCK(ca_kh_ipv4_nh_lock);
DEFINE_SPINLOCK(ca_kh_ipv6_nh_lock);
DEFINE_SPINLOCK(ca_kh_tnl_lock);
DEFINE_SPINLOCK(ca_kh_wan_map_lock);
DEFINE_SPINLOCK(ca_kh_gw_lock);

#ifndef CONFIG_CORTINA_KERNEL_HOOK
struct kernel_hook_ops ca_kernel_hook_ops;
#endif

ca_status_t ca_kh_tr069_intf_clean(void);
ca_status_t ca_kh_tr069_intf_set(ca_ip_afi_t afi, ca_l3_ip_addr_t *ip, ca_uint8_t cos);
ca_status_t ca_kh_port_binding_init(void);
int ca_kernel_hook_proc_init_module(void);
void ca_kernel_hook_proc_cleanup_module(void);

struct in6_addr ca_kh_in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

ca_status_t ca_kh_l3_intf_set_mtu(ca_device_id_t device_id, ca_int16_t ifindex, unsigned int dev_flags, int mtu);

/* Net event notifiier */
static int kernel_hook_event_handle(struct notifier_block *unused, unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);

	switch (event) {
		case NETDEV_CHANGEMTU:
			printk("%s: event NETDEV_CHANGEMTU...\n", __func__);
			ca_kh_l3_intf_set_mtu(0, dev->ifindex, dev->flags, dev->mtu);
			break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block kernel_hook_notifier = {
	.notifier_call = kernel_hook_event_handle
};

int ca_kernel_hook_set_event_notifier(void)
{
	int ret;

	ret = register_netdevice_notifier(&kernel_hook_notifier);
	if (ret) {
		printk("%s: register_netdevice_notifier() failed, ret=%d\n", __func__, ret);
	}
	return ret;
}

//static int __init ca_kernel_hook_init(void)
int ca_kernel_hook_init(void)
{
	int ret;
	unsigned int khook_enable;

	ret = scfg_read(0, CFG_ID_KERNEL_HOOK_ENABLE, sizeof(khook_enable), (ca_uint8_t *) &khook_enable);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_KERNEL_HOOK_ENABLE. (ret=%d)\n", __func__, ret);
		khook_enable = 0;
	}

	if (khook_enable == 0) {
		printk(KERN_INFO "%s: Kernel Hook is diabled in scfg.\n", __func__);
		/*** DO NOT RETURN ERROR! Some user-space apps use Kernel Hook functions thru IROS. ***/
	} else {
#ifdef CONFIG_CORTINA_KERNEL_HOOK
		printk(KERN_INFO "%s: Enable the Kernel Hook functions.\n", __func__);
		memset(&ca_kernel_hook_ops, 0, sizeof(struct kernel_hook_ops));

		ca_kernel_hook_ops.kho_l3_intf_add 		= (void *)ca_kh_l3_intf_add;
		ca_kernel_hook_ops.kho_l3_intf_update_ip_addr 	= (void *)ca_kh_l3_intf_update_ip_addr;
		ca_kernel_hook_ops.kho_l3_intf_update_mtu	= (void *)ca_kh_l3_intf_update_mtu;
		ca_kernel_hook_ops.kho_l3_intf_update_mac_addr	= (void *)ca_kh_l3_intf_update_mac_addr;
		ca_kernel_hook_ops.kho_l3_intf_delete		= (void *)ca_kh_l3_intf_delete;

		ca_kernel_hook_ops.kho_l3_route_add_ipv4_static	= (void *)ca_kh_l3_route_add_ipv4_static;
		ca_kernel_hook_ops.kho_l3_route_add_ipv4_dynamic= (void *)ca_kh_l3_route_add_ipv4_dynamic;
		ca_kernel_hook_ops.kho_l3_route_del_ipv4_ifdown	= (void *)ca_kh_l3_route_del_ipv4_ifdown;
		ca_kernel_hook_ops.kho_l3_route_del_ipv4_static	= (void *)ca_kh_l3_route_del_ipv4_static;
		ca_kernel_hook_ops.kho_l3_route_add_ipv6_static	= (void *)ca_kh_l3_route_add_ipv6_static;
		ca_kernel_hook_ops.kho_l3_route_del_ipv6_static	= (void *)ca_kh_l3_route_del_ipv6_static;
		ca_kernel_hook_ops.kho_l3_route_del_ipv6_ifdown	= (void *)ca_kh_l3_route_del_ipv6_ifdown;

		ca_kernel_hook_ops.kho_l3_nexthop_aging_timer_set = (void *)ca_kh_l3_nexthop_aging_timer_set;

		ca_kernel_hook_ops.kho_neigh_add		= (void *)ca_kh_neigh_add;
		ca_kernel_hook_ops.kho_neigh_delete		= (void *)ca_kh_neigh_delete;
		ca_kernel_hook_ops.kho_neigh_update		= (void *)ca_kh_neigh_update;
		ca_kernel_hook_ops.kho_neigh_update_used	= (void *)ca_kh_neigh_update_used;

		ca_kernel_hook_ops.kho_nat_entry_add		= (void *)ca_kh_nat_entry_add;
		ca_kernel_hook_ops.kho_nat_entry_delete		= (void *)ca_kh_nat_entry_delete;
		ca_kernel_hook_ops.kho_nat_entry_session_delete	= (void *)ca_kh_nat_entry_session_delete;
		ca_kernel_hook_ops.kho_nat_entry_session_delete_force	= (void *)ca_kh_nat_entry_session_delete_force;
		ca_kernel_hook_ops.kho_nat_entry_delete_by_port = (void *)ca_kh_nat_entry_delete_by_port;
		ca_kernel_hook_ops.kho_nat_entry_timer_refresh	= (void *)ca_kh_nat_entry_timer_refresh;

		ca_kernel_hook_ops.kho_nat6_entry_add		= (void *)ca_kh_nat6_entry_add;
		ca_kernel_hook_ops.kho_nat6_entry_delete	= (void *)ca_kh_nat6_entry_delete;

		ca_kernel_hook_ops.kho_dslite_add		= (void *)ca_kh_dslite_add;
		ca_kernel_hook_ops.kho_dslite_delete		= (void *)ca_kh_dslite_delete;
		ca_kernel_hook_ops.kho_dslite_delete_intf_only	= (void *)ca_kh_dslite_delete_intf_only;

#ifdef CONFIG_IPV6_SIT_6RD
		ca_kernel_hook_ops.kho_6rd_add			= (void *)ca_kh_6rd_add;
		ca_kernel_hook_ops.kho_6rd_delete		= (void *)ca_kh_6rd_delete;
		ca_kernel_hook_ops.kho_6rd_update		= (void *)ca_kh_6rd_update;
#endif

		ca_kernel_hook_ops.kho_map_e_add		= (void *)ca_kh_map_e_add;
		ca_kernel_hook_ops.kho_map_e_delete		= (void *)ca_kh_map_e_delete;

		ca_kernel_hook_ops.kho_l2_mcast_member_add	= (void *)ca_kh_l2_mcast_member_add;
		ca_kernel_hook_ops.kho_l2_mcast_member_delete	= (void *)ca_kh_l2_mcast_member_delete;
		ca_kernel_hook_ops.kho_mcast_set_max_grp_number = (void *)ca_kh_mcast_set_max_group_number;
		ca_kernel_hook_ops.kho_mcast_get_max_grp_number = (void *)ca_kh_mcast_get_max_group_number;

		ca_kernel_hook_ops.kho_mcast_set_multicast_snooping = (void *)ca_kh_mcast_set_multicast_snooping;
		ca_kernel_hook_ops.kho_mcast_get_multicast_snooping = (void *)ca_kh_mcast_get_multicast_snooping;
		ca_kernel_hook_ops.kho_mcast_set_multicast_fastleave = (void *)ca_kh_mcast_set_multicast_fastleave;
		ca_kernel_hook_ops.kho_mcast_get_multicast_fastleave = (void *)ca_kh_mcast_get_multicast_fastleave;

		ca_kernel_hook_ops.kho_pppoe_add		= (void *)ca_kh_pppoe_add;
		ca_kernel_hook_ops.kho_pppoe_delete		= (void *)ca_kh_pppoe_delete;
		ca_kernel_hook_ops.kho_pppoe_update		= (void *)ca_kh_pppoe_update;

		ca_kernel_hook_ops.kho_ipsec_add		= (void *)ca_kh_ipsec_add;
		ca_kernel_hook_ops.kho_ipsec_delete		= (void *)ca_kh_ipsec_delete;
		ca_kernel_hook_ops.kho_ipsec_redirect_to_pe	= (void *)ca_kh_ipsec_redirect_to_pe;

		ca_kernel_hook_ops.kho_l2_vlan_add		= (void *)ca_kh_l2_vlan_add;
		ca_kernel_hook_ops.kho_l2_vlan_delete		= (void *)ca_kh_l2_vlan_delete;

		ca_kernel_hook_ops.kho_l2_br_add_if		= (void *)ca_kh_l2_br_add_if;

		ca_kernel_hook_ops.kho_bypass_tcp_portlist_add = (void *)ca_kh_bypass_tcp_portlist_add;
		ca_kernel_hook_ops.kho_bypass_tcp_portlist_delete = (void *)ca_kh_bypass_tcp_portlist_delete;
		ca_kernel_hook_ops.kho_bypass_tcp_portlist_dump = (void *)ca_kh_bypass_tcp_portlist_dump;
		ca_kernel_hook_ops.kho_bypass_tcp_portlist_exist = (void *)ca_kh_bypass_tcp_portlist_exist;
		ca_kernel_hook_ops.kho_bypass_udp_portlist_add = (void *)ca_kh_bypass_udp_portlist_add;
		ca_kernel_hook_ops.kho_bypass_udp_portlist_delete = (void *)ca_kh_bypass_udp_portlist_delete;
		ca_kernel_hook_ops.kho_bypass_udp_portlist_dump = (void *)ca_kh_bypass_udp_portlist_dump;
		ca_kernel_hook_ops.kho_bypass_udp_portlist_exist = (void *)ca_kh_bypass_udp_portlist_exist;
#if 0
		ca_kernel_hook_ops.kho_8021q_pcp_handler = ca_kh_8021q_pcp_handler;

		ca_kernel_hook_ops.kho_port_binding_check = ca_kh_port_binding_check;
#endif

#endif /* CONFIG_CORTINA_KERNEL_HOOK */
	}

	/* PPPoE init */
	ca_kh_pppoe_init();


	/* port list init */
	ca_kh_bypass_tcp_portlist_init();
	ca_kh_bypass_udp_portlist_init();


	memset(&(ca_kh_ipv4_nexthop_table[0]), 0, sizeof(ca_uint32_t) * CA_L3_IP_MAX_INTF);
	memset(&(ca_kh_ipv6_nexthop_table[0]), 0, sizeof(ca_uint32_t) * CA_L3_IP_MAX_INTF);
	memset(&(ca_kh_ipv4_nexthop_table[0]), 0, sizeof(ca_kh_nexthop_entry_t) * CA_L3_MAX_NEXTHOP_NUM);
	memset(&(ca_kh_ipv6_nexthop_table[0]), 0, sizeof(ca_kh_nexthop_entry_t) * CA_L3_MAX_NEXTHOP_NUM);

#if 0
	/* init portal management */
	ca_kh_portal_init();
	/* init limit terminal */
	ca_kh_limit_init();
	/* init lan dev table */
	ca_kh_lan_dev_init();

	/* init multicast */
	ca_kh_mcast_init();

	/* init tr069 interface */
	ca_kh_tr069_intf_clean();

	/* initialize port binding */
	ca_kh_port_binding_init();
#endif

	ca_kernel_hook_proc_init_module();

	/* setup event notifier for Net events */
	ca_kernel_hook_set_event_notifier();

	return 0;
}

//static void __exit ca_kernel_hook_exit(void)
void ca_kernel_hook_exit(void)
{
#ifdef CONFIG_CORTINA_KERNEL_HOOK
	printk(KERN_INFO "%s: Removing Kernel Hook functions.\n", __func__);

	memset(&ca_kernel_hook_ops, 0, sizeof(struct kernel_hook_ops));

#if 0
	/* cleanup portal management */
	ca_kh_portal_fini();
	/* cleanup limit terminal */
	ca_kh_limit_fini();
	/* cleanup lan dev table */
	ca_kh_lan_dev_fini();
#endif
	ca_kernel_hook_proc_cleanup_module();

#endif
}

