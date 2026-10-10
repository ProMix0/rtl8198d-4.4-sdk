#include <linux/netdevice.h>
#include <net/neighbour.h>
#include <soc/cortina/ca_kernel_hook.h>

#include "ca_kernel_hook_util.h"

ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
ca_uint8 ca_kh_is_wan_intf(int ifindex);
//ca_status_t ca_kh_multi_wan_classifier_add(ca_ip_address_t *ip_addr, ca_uint8_t *da_mac);
//ca_status_t ca_kh_multi_wan_classifier_del(ca_ip_address_t *ip_addr, ca_uint8_t *da_mac);
//extern ca_status_t ca_kh_pppoe_data_traverse(ca_device_id_t device_id);

typedef struct ca_l3_user_data {
	ca_intf_id_t    intf_id;
	ca_ip_address_t	ip_addr;
	ca_mac_addr_t	da_mac;
} ca_l3_user_data_t;

ca_status_t mask_ip_by_len(ca_ip_address_t *ip, ca_uint8_t addr_len);
ca_status_t go_through_lpm_nexthop(ca_tunnel_cfg_t *tcfg, ca_tunnel_id_t tunnel_id);
ca_status_t update_tunnel_mac(void);

///TODO:
ca_uint8 ca_kh_is_wan_intf(int ifindex)
{
	return 0;
}

static ca_intf_id_t ca_kh_l3_nexthop_get_real_intf_id(ca_device_id_t device_id, ca_ip_afi_t afi, int ifindex, int child_ifindex, ca_intf_id_t intf_id)
{
	int ret;
	ca_intf_id_t child_intf_id;
	ca_l3_intf_t l3_intf;

	/* get the real intf_id for the member of a bridge, like as: eth1.10 */
	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: afi=%d, ifindex=%d, child_ifindex=%d, intf_id=%d\n", __func__, afi, ifindex, child_ifindex, intf_id);
	}

	if (child_ifindex == 0) /* 0 is interface 'lo' usually */
		return intf_id;

	/* Get child intf_id by child_ifindex. Ex: intf_id of member interface eth1.10 of bridge interface.
	 * child_intf_id may not exist while parent interface is eth0 or else.
	 */

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, afi, child_ifindex, CA_L3_INTF_BCAST, &child_intf_id);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: ca_kh_l3_get_intf_id_by_ifindex(child_ifindex=%d), return failed!!, ret=%d\n", __func__, child_ifindex, ret);
		}
		return intf_id;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: child_intf_id=%d\n", __func__, child_intf_id);
	}

	/* Get intf_id by ifindex. Ex: intf_id of bridge interface */

	ret =  ca_kh_l3_get_intf_id_by_ifindex(device_id, afi, ifindex, CA_L3_INTF_BCAST, &l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
		return intf_id;
	}

	/* get intf by intf_id, for determining the returned intf_id */

	l3_intf.ip_addr.afi = afi;
	ret = ca_l3_intf_get(0, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, intf_id, ret);
		return intf_id;
	}
	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: l3_intf.port_id=0x%x\n", __func__, l3_intf.port_id);
	}
	if ((l3_intf.port_id >> CA_PORT_TYPE_SHIFT) == CA_PORT_TYPE_L2RP) {
		intf_id = child_intf_id;
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: Bridge member use child intf_id=%d\n", __func__, intf_id);
		}
	}
	else {
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: Not bridge member keep old intf_id=%d\n", __func__, intf_id);
		}
	}
	return intf_id;
}

static int ca_kh_l3_nexthop_traverse_cb(ca_device_id_t dev, ca_l3_nexthop_t *l3_nexthop, void *user_data)
{
        int ret;
        ca_l3_user_data_t l3_user_data = *((ca_l3_user_data_t *)user_data);
        ca_l3_nexthop_id_t l3_nexthop_id;
	ca_uint8_t save_addr_len;

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: nexthop_id=%d\n", __func__, l3_nexthop->nexthop_id);
		printk("%s: afi=%d, addr_len=%d\n", __func__, l3_user_data.ip_addr.afi, l3_user_data.ip_addr.addr_len);
		if (l3_user_data.ip_addr.afi == CA_IPV4) {
			printk("%s: ip_addr=%pI4\n", __func__, &(l3_user_data.ip_addr.ip_addr.ipv4_addr));
		}
		else {
			printk("%s: ip_addr=%pI6\n", __func__, &(l3_user_data.ip_addr.ip_addr.ipv6_addr));
		}
		printk("%s: da_mac=%pM\n", __func__, &(l3_user_data.da_mac));
        }

	l3_nexthop_id = l3_nexthop->nexthop_id;
	if (l3_nexthop->attr_flags == CA_L3_NHOP_ATTR_ROUTER) {

		save_addr_len = l3_nexthop->addr.addr_len;
		if (l3_nexthop->addr.afi == CA_IPV4) {
			l3_nexthop->addr.addr_len = 32;
			l3_user_data.ip_addr.addr_len = 32;
		}
		else {
			l3_nexthop->addr.addr_len = 128;
			l3_user_data.ip_addr.addr_len = 128;
		}
		if (ca_kh_is_ip_equal(&(l3_nexthop->addr), &(l3_user_data.ip_addr))) {

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk("%s: Found the nexthop at nexthop_id=%d\n", __func__, l3_nexthop->nexthop_id);
			}

			/* if the nexthop is linked by route, we should update it's MAC */
                	/* Modify the nexthop entry which is linked by static route to WAN port, like as 192.168.2.100/32 */
			if (l3_nexthop->intf_id == l3_user_data.intf_id) {
				l3_nexthop->addr.addr_len = save_addr_len;
				memcpy(&(l3_nexthop->da_mac[0]), &(l3_user_data.da_mac[0]), 6);

				if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
					printk("%s: Hit l3_nexthop->intf_id=%d, l3_user_data.intf_id=%d\n", __func__, l3_nexthop->intf_id, l3_user_data.intf_id);
				}

				ret = ca_l3_nexthop_update(0, l3_nexthop, l3_nexthop_id);
				if (ret != CA_E_OK) {
					printk("%s: ca_l3_nexthop_update(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
					return CA_E_OK;
				}
			}
		}
        }
        return CA_E_OK;
}

static ca_status_t ca_l3_nexthop_traverse(ca_device_id_t device_id, ca_ip_afi_t afi, void *user_data)
{
	ca_iterator_t	iterator;
	ca_l3_nexthop_t	l3_nexthop;
	ca_uint32_t	count;
	ca_status_t	ret;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &l3_nexthop;
	iterator.user_buffer_size = sizeof(ca_l3_nexthop_t);

	ret = ca_l3_nexthop_iterate(device_id, afi, &iterator);
	while (ret == CA_E_OK) {
		if (iterator.entry_count == 0)
			break;
		ca_kh_l3_nexthop_traverse_cb(device_id, &l3_nexthop, user_data);
		ret = ca_l3_nexthop_iterate(device_id, afi, &iterator);
	}

	return ret;
}

#if 0
static int ca_kh_l3_route_traverse_cb(ca_device_id_t device_id, ca_l3_route_t *l3_route, void *user_data)
{
        int ret;
        ca_l3_user_data_t l3_user_data = *((ca_l3_user_data_t *)user_data);
        ca_l3_nexthop_id_t l3_nexthop_id;
	ca_l3_nexthop_t l3_nexthop;

        if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                if (l3_route->prefix.afi == CA_IPV4) {
                        printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
                                __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
                }
                else {
                        printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6, l3_route->prefix.addr_len=%d\n",
                                __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv6_addr, l3_route->prefix.addr_len);
                }
		printk("%s: l3_user_data.intf_id=%d\n", __func__, l3_user_data.intf_id);
		if (l3_user_data.ip_addr.afi == CA_IPV4) {
                        printk("%s: l3_user_data.ip_addr.afi=%d, l3_user_data.ip_addr.ip_addr.ipv4_addr=%pI4, l3_user_data.ip_addr.addr_len=%d\n",
                                __func__, l3_user_data.ip_addr.afi, &l3_user_data.ip_addr.ip_addr.ipv4_addr, l3_user_data.ip_addr.addr_len);
                }
                else {
                        printk("%s: l3_user_data.ip_addr.afi=%d, l3_user_data.ip_addr.ip_addr.ipv6_addr=%pI6, l3_user_data.ip_addr.addr_len=%d\n",
                                __func__, l3_user_data.ip_addr.afi, &l3_user_data.ip_addr.ip_addr.ipv6_addr, l3_user_data.ip_addr.addr_len);
                }
		printk("%s: l3_user_data.da_mac=%pM\n", __func__, &l3_user_data.da_mac);
        }

        if (ca_kh_is_ip_equal(&(l3_route->prefix), &(l3_user_data.ip_addr))) {

                if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                        printk("%s: static route found ..\n", __func__);
                        if (l3_route->prefix.afi == CA_IPV4) {
                                printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
                                         __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
                        }
                        else {
                                printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6, l3_route->prefix.addr_len=%d\n",
                                        __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv6_addr, l3_route->prefix.addr_len);
                        }
                }
		l3_nexthop_id = l3_route->nexthop_id;
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: nexthop id = %d\n", __func__, l3_nexthop_id);
		}

		ret = ca_l3_nexthop_get(device_id, l3_nexthop_id, &l3_nexthop);
		if (ret != CA_E_OK) {
			printk("%s: ca_l3_nexthop_get(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
			return CA_E_OK;
		}

		/* Modify the nexthop entry which is linked by static route to WAN port, like as 192.168.2.100/32 */
		if (l3_nexthop.intf_id == l3_user_data.intf_id) {
			if (ca_kh_debug & CA_KH_DEBUG_ARP) {
				printk("%s: The nexthop for static route to WAN port is found!!\n", __func__);
			}
			l3_nexthop.addr = l3_user_data.ip_addr;
			memcpy(&(l3_nexthop.da_mac[0]), &(l3_user_data.da_mac[0]), 6);

			ret = ca_l3_nexthop_update(0, &l3_nexthop, l3_nexthop_id);
			if (ret != CA_E_OK) {
				printk("%s: ca_l3_nexthop_update(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
				return CA_E_OK;
			}
		}
	}
	return CA_E_OK;
}
#endif

ca_status_t ca_kh_neigh_add(ca_device_id_t device_id, int ifindex, ca_uint8_t *da_mac, ca_ip_address_t *ip_addr, int child_ifindex, bool is_static)
{
	int ret;
	ca_l3_nexthop_t l3_nexthop;
	ca_intf_id_t     intf_id;
	ca_kh_nexthop_entry_t kh_nexthop;
	ca_l3_nexthop_id_t l3_nexthop_id;
	ca_l3_user_data_t l3_user_data;

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: ifindex=%d\n", __func__, ifindex);
		printk("%s: da_mac[]=%pM\n", __func__, da_mac);
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
		}
		else {
			printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
		}
		printk("%s: child_ifindex=%d\n", __func__, child_ifindex);
	}
#if 0
	/* do not add a link local address */
	if (ip_addr->afi == CA_IPV6) {
		if (ipv6_addr_type((const struct in6_addr *)&(ip_addr->ip_addr.ipv6_addr[0])) & IPV6_ADDR_LINKLOCAL)
			return CA_E_OK;
	}
#endif
	memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));

	/* Cortina API accepts host order IP address */
	ca_kh_ip_address_ntohl(&l3_nexthop.addr, ip_addr);

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, ip_addr->afi, ifindex, CA_L3_INTF_BCAST, &intf_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: intf_id=%d\n", __func__, intf_id);
	}

	/* get the real intf_id for the member of a bridge, like as: eth1.10 */
	l3_nexthop.intf_id = ca_kh_l3_nexthop_get_real_intf_id(device_id, ip_addr->afi, ifindex, child_ifindex, intf_id);
	l3_nexthop.attr_flags = (is_static) ? CA_L3_NHOP_ATTR_ROUTER : CA_L3_NHOP_ATTR_NEGH;

	memcpy(&(l3_nexthop.da_mac[0]), &(da_mac[0]), 6);
	ret = ca_l3_nexthop_add(0, &l3_nexthop, &l3_nexthop_id);
	if (ret != CA_E_OK) {
		if (ret != CA_E_EXISTS) {
			printk("%s: ca_l3_nexthop_add(), return failed!!, ret=%d\n", __func__, ret);
			return ret;
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: l3_nexthop_id=%d\n", __func__, l3_nexthop_id);
	}

	/* We still store the network order IP address for kernel hook nexthop table */
	kh_nexthop.ip_addr = *ip_addr;
	kh_nexthop.ifindex = ifindex;
	kh_nexthop.l3_nexthop_id = l3_nexthop_id;
	kh_nexthop.attr_flags = l3_nexthop.attr_flags;
	ret = ca_kh_l3_nexthop_add(device_id, &kh_nexthop);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_nexthop_add() return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	/* update the nexthop entry which is linked by static route to WAN port */
	if (ca_kh_is_wan_intf(ifindex)) {
#if 1
		/* update MAC address of static route */
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, ip_addr->afi, ifindex, CA_L3_INTF_BCAST, &l3_user_data.intf_id);
		if (ret == CA_E_OK) {
			l3_user_data.ip_addr = l3_nexthop.addr;
			memcpy(&(l3_user_data.da_mac[0]), &(l3_nexthop.da_mac[0]), 6);
			ca_l3_nexthop_traverse(device_id, ip_addr->afi, &l3_user_data);
		}
#endif
		/* search the database of gateway in stead of nexthop */
		///TODO:
#if 0
		ca_kh_ip_address_ntohl(ip_addr, ip_addr);
		ret = ca_int_l3_gw_upd(ifindex, ip_addr, &(l3_nexthop.da_mac[0]));
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_ARP) {
				printk("%s: ca_int_l3_gw_upd() return failed!!, ret=%d\n", __func__, ret);
			}
			// don't care the result of ca_int_l3_gw_upd()
			// return ret;
		}
#endif
	}

	/* if ip_addr and da_mac pair already exists no need to add again */
	if (ca_kh_is_wan_intf(ifindex)) {
		//ca_kh_multi_wan_classifier_add(ip_addr, da_mac);
	}

	update_tunnel_mac();

	return ret;
}

ca_status_t ca_kh_neigh_delete(ca_device_id_t device_id, int ifindex, ca_ip_address_t *ip_addr, bool is_static)
{
        int ret;
        ca_l3_nexthop_t l3_nexthop;
	ca_kh_nexthop_entry_t kh_nexthop;

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
        	printk("%s: ifindex=%d\n", __func__, ifindex);
                if (ip_addr->afi == CA_IPV4) {
                        printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
                }
                else {
                        printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
                }
	}

#if 0
	/* do not delete a link local address */
	if (ip_addr->afi == CA_IPV6) {
		if (ipv6_addr_type((const struct in6_addr *)&(ip_addr->ip_addr.ipv6_addr[0])) & IPV6_ADDR_LINKLOCAL)
			return CA_E_OK;
	}
#endif

	memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));
	kh_nexthop.ifindex = ifindex;
	kh_nexthop.ip_addr = *ip_addr;
	kh_nexthop.attr_flags = (is_static) ? CA_L3_NHOP_ATTR_ROUTER : CA_L3_NHOP_ATTR_NEGH;
	ret = ca_kh_l3_nexthop_get(device_id, &kh_nexthop);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_nexthop_get(), return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: nexthop id = %d\n", __func__, kh_nexthop.l3_nexthop_id);
	}

	ret = ca_l3_nexthop_get(device_id, kh_nexthop.l3_nexthop_id, &l3_nexthop);
	if (ret != CA_E_OK) {
                printk("%s: ca_l3_nexthop_get(), return failed!!, ret=%d\n", __func__, ret);
	}
#if 0
	if (l3_nexthop.attr_flags & CA_L3_NHOP_ATTR_ROUTER) {
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: l3_nexthop.attr_flags is CA_L3_NHOP_ATTR_ROUTER, do not delete!!\n", __func__);
		}
		return CA_E_OK;
	}
#endif

#if 0
        ret = ca_l3_nexthop_delete(device_id, kh_nexthop.l3_nexthop_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, kh_nexthop.l3_nexthop_id, ret);
        }

	ret = ca_kh_l3_nexthop_delete(device_id, ip_addr->afi, kh_nexthop.l3_nexthop_id);
	if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, kh_nexthop.l3_nexthop_id, ret);
        }
#else
	/* Set da_mac to zero; this disable hash entries by setting age to 0.
	 * Should NOT delete nexthop because route entry may exist still in Linux.
	 */
	memset(&(l3_nexthop.da_mac[0]), 0, 6);

	ret = ca_l3_nexthop_update(0, &l3_nexthop, kh_nexthop.l3_nexthop_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_nexthop_update(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, kh_nexthop.l3_nexthop_id, ret);
		return CA_E_OK;
	}
#endif
	if (ca_kh_is_wan_intf(ifindex))	{
		//ca_kh_multi_wan_classifier_del(ip_addr, &(l3_nexthop.da_mac[0]));
	}

	update_tunnel_mac();

        return ret;
}

ca_status_t ca_kh_neigh_update(ca_device_id_t device_id, int ifindex, ca_uint8_t *da_mac, ca_ip_address_t *ip_addr, int child_ifindex, bool is_static)
{
	int ret;
        ca_l3_nexthop_t l3_nexthop;
	ca_kh_nexthop_entry_t kh_nexthop;
	ca_l3_nexthop_id_t  l3_nexthop_id;
	ca_intf_id_t intf_id;
	ca_ip_address_t tmp_ip_addr; /* stored in host order */
	bool nh_exist = false;

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: ifindex=%d\n", __func__, ifindex);
		printk("%s: da_mac[]=%pM\n", __func__, da_mac);
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
		}
		else {
			printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
		}
	}
#if 0
	/* do not update a link local address */
        if (ip_addr->afi == CA_IPV6) {
                if (ipv6_addr_type((const struct in6_addr *)&(ip_addr->ip_addr.ipv6_addr[0])) & IPV6_ADDR_LINKLOCAL)
                        return CA_E_OK;
        }
#endif

	memset(&tmp_ip_addr, 0, sizeof(tmp_ip_addr));
	ca_kh_ip_address_ntohl(&tmp_ip_addr, ip_addr);

	/* memory set to 0 to avoid memcmp failed in ca_l3_nexthop_update() */
	memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));

	ret = ca_l3_nexthop_get_by_ip(0, tmp_ip_addr, &l3_nexthop);
	if (ret == CA_E_OK) {
		nh_exist = true;
		l3_nexthop_id = l3_nexthop.nexthop_id;
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: ca_kh_l3_nexthop_get_by_ip() found, nexthop_id=%u.\n", __func__, l3_nexthop.nexthop_id);
		}

		/* For updating existing nexthop in kh, always delete it first */
		ret = ca_kh_l3_nexthop_delete(device_id, ip_addr->afi, l3_nexthop.nexthop_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_nexthop_delete(), return failed!!, ret=%d\n", __func__, ret);
		}
	} else if (ret == CA_E_NOT_FOUND) {
		nh_exist = false;
		/* could be a new one, should add it directly */
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: ca_kh_l3_nexthop_get_by_ip() not found.\n", __func__);
		}

		/* should reset to all 0s here */
		memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));
		/* should get the intf_id for l3_nexthop */
		ret =  ca_kh_l3_get_intf_id_by_ifindex(device_id, ip_addr->afi, ifindex, CA_L3_INTF_BCAST, &l3_nexthop.intf_id);
                if (ret != CA_E_OK) {
                        printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
                        return ret;
                }
	} else {

		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: ca_kh_l3_nexthop_get_by_ip() return error, ret=%d.\n", __func__, ret);
		}
		return ret;
	}

	intf_id = l3_nexthop.intf_id;

	/* get the real intf_id for the member of a bridge, like as: eth1.10 */
	l3_nexthop.intf_id = ca_kh_l3_nexthop_get_real_intf_id(device_id, ip_addr->afi, ifindex, child_ifindex, intf_id);
	l3_nexthop.attr_flags = (is_static) ? CA_L3_NHOP_ATTR_ROUTER : CA_L3_NHOP_ATTR_NEGH;

	/* Cortina API accepts host order IP address */
        ca_kh_ip_address_ntohl(&l3_nexthop.addr, ip_addr);
        memcpy(&(l3_nexthop.da_mac[0]), &(da_mac[0]), 6);

	if (nh_exist)
		ret = ca_l3_nexthop_update(device_id, &l3_nexthop, l3_nexthop_id);
	else
		ret = ca_l3_nexthop_add(device_id, &l3_nexthop, &l3_nexthop_id);

	if (ret != CA_E_OK) {
		if (ret != CA_E_EXISTS) {
			printk("%s: l3_nexthop.intf_id=%d, l3_nexthop.attr_flags=%d\n", __func__, l3_nexthop.intf_id, l3_nexthop.attr_flags);
			if (l3_nexthop.addr.afi == CA_IPV4) {
				printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, l3_nexthop.addr.afi, &l3_nexthop.addr.ip_addr.ipv4_addr, l3_nexthop.addr.addr_len);
			}
			else {
				printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, l3_nexthop.addr.afi, &l3_nexthop.addr.ip_addr.ipv6_addr, l3_nexthop.addr.addr_len);
			}
			if (nh_exist)
				printk("%s: ca_l3_nexthop_update(), return failed!!, ret=%d\n", __func__, ret);
			else
				printk("%s: ca_l3_nexthop_add(), return failed!!, ret=%d\n", __func__, ret);
		}
		if (ca_kh_debug & CA_KH_DEBUG_ARP)
			printk("%s ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));
	kh_nexthop.ip_addr = *ip_addr;
	kh_nexthop.ifindex = ifindex;
	kh_nexthop.l3_nexthop_id = l3_nexthop_id;
	kh_nexthop.attr_flags = l3_nexthop.attr_flags;
	ret = ca_kh_l3_nexthop_add(device_id, &kh_nexthop);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_nexthop_add() return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	update_tunnel_mac();

	return ret;
}

ca_status_t ca_kh_neigh_update_used(ca_device_id_t device_id, ca_int16_t ifindex, ca_ip_address_t *ip_addr, ca_uint32_t *used_jiffies)
{
	int ret;
        ca_l3_nexthop_t l3_nexthop;
        ca_kh_nexthop_entry_t kh_nexthop;
	ca_uint32_t current_aging_time = 0;
	ca_uint32_t hw_last_used;

	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                printk("%s: ifindex=%d\n", __func__, ifindex);
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
                }
                else {
			printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
                }
		printk("%s: used_jiffies=0x%x\n", __func__, *used_jiffies);
        }

#if 0
	/* do not update a link local address */
	if (ip_addr->afi == CA_IPV6) {
		if (ipv6_addr_type((const struct in6_addr *)&(ip_addr->ip_addr.ipv6_addr[0])) & IPV6_ADDR_LINKLOCAL)
			return CA_E_OK;
	}
#endif

	/* update the pppoe data to route table and classifier */
	//ca_kh_pppoe_data_traverse(device_id);

	memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));
        kh_nexthop.ip_addr = *ip_addr;
        kh_nexthop.ifindex = ifindex;
	kh_nexthop.attr_flags = 0;
        ret = ca_kh_l3_nexthop_get(device_id, &kh_nexthop);
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                	printk("%s: ca_kh_l3_nexthop_get(), return failed!!, ret=%d\n", __func__, ret);
		}
                return ret;
        }

        if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                printk("%s: kh_nexthop.l3_nexthop_id=%d\n", __func__, kh_nexthop.l3_nexthop_id);
        }

        ret = ca_l3_nexthop_get(0, kh_nexthop.l3_nexthop_id, &l3_nexthop);
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                	printk("%s: ca_l3_nexthop_get(), kh_nexthop.l3_nexthop_id=%d, return failed!!, ret=%d\n", __func__, kh_nexthop.l3_nexthop_id, ret);
		}
                return ret;
        }

        if (ca_kh_debug & CA_KH_DEBUG_ARP) {
                printk("%s: after ca_l3_nexthop_get(), l3_nexhop becomes..\n", __func__);
		printk("%s: aging_timer=0x%x\n", __func__, l3_nexthop.aging_timer);
		printk("%s: da_mac[]=%pM\n", __func__, l3_nexthop.da_mac);
                if (ip_addr->afi == CA_IPV4) {
                        printk("%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, l3_nexthop.addr.afi, &l3_nexthop.addr.ip_addr.ipv4_addr, l3_nexthop.addr.addr_len);
                }
                else {
                        printk("%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, l3_nexthop.addr.afi, &l3_nexthop.addr.ip_addr.ipv6_addr, l3_nexthop.addr.addr_len);
                }
        }

	//ca_l3_nexthop_aging_timer_get(device_id, &current_aging_time);
	//hw_last_used = jiffies - msecs_to_jiffies((current_aging_time - l3_nexthop.aging_timer) * 1000);
	hw_last_used = jiffies - msecs_to_jiffies(l3_nexthop.aging_timer * 1000); /* aging_timer record the time expired after hit */
	if (ca_kh_debug & CA_KH_DEBUG_ARP) {
		printk("%s: current_aging_time=%d, l3_nexthop.aging_timer=%d, hw_last_used=0x%x\n", __func__, current_aging_time, l3_nexthop.aging_timer, hw_last_used);
		printk("%s: jiffies=0x%lx, *used_jiffies = 0x%x\n", __func__, jiffies, *used_jiffies);
	}
	if (time_before((unsigned long) *used_jiffies, (unsigned long) hw_last_used)) {
                *used_jiffies = hw_last_used;
		if (ca_kh_debug & CA_KH_DEBUG_ARP) {
			printk("%s: time before!!\n", __func__);
		}
	}

	return CA_E_OK;
}
