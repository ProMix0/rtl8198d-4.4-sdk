
//#include <linux/printk.h>
#include <linux/netdevice.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>

#include <soc/cortina/ca_kernel_hook.h>
#include "ca_kernel_hook_util.h"

typedef struct ca_l3_user_data {
	ca_intf_id_t	intf_id;
	ca_ip_afi_t	afi;
} ca_l3_user_data_t;

//ca_status_t ca_kh_nat_entry_ifdown(ca_device_id_t device_id, ca_ip_address_t *ip_addr);

ca_status_t mask_ip_by_len(ca_ip_address_t *ip, ca_uint8_t addr_len)
{
	if (ip == NULL) return CA_E_PARAM;

	if (ip->afi == CA_IPV4) {
		if (addr_len > 32) return CA_E_PARAM;

		ip->ip_addr.ipv4_addr &= 0xffffffffULL << (32 - addr_len);
	} else {
		if (addr_len > 128) return CA_E_PARAM;

		if (addr_len < 32) {
			ip->ip_addr.ipv6_addr[0] &= 0xffffffffULL << (32 - addr_len); // MSB
			ip->ip_addr.ipv6_addr[1] = 0;
			ip->ip_addr.ipv6_addr[2] = 0;
			ip->ip_addr.ipv6_addr[3] = 0;
		} else if ((addr_len >= 32) && (addr_len < 64)) {
			ip->ip_addr.ipv6_addr[1] &= 0xffffffffULL << (64 - addr_len);
			ip->ip_addr.ipv6_addr[2] = 0;
			ip->ip_addr.ipv6_addr[3] = 0;
		} else if ((addr_len >= 64) && (addr_len < 96)) {
			ip->ip_addr.ipv6_addr[2] &= 0xffffffffULL << (96 - addr_len);
			ip->ip_addr.ipv6_addr[3] = 0;
		} else {
			ip->ip_addr.ipv6_addr[3] &= 0xffffffffULL << (128 - addr_len);
		}
	}

	return CA_E_OK;
}

ca_status_t go_through_lpm_nexthop(ca_tunnel_cfg_t *tcfg, ca_tunnel_id_t tunnel_id)
{
	ca_status_t ret;
	ca_iterator_t iter;
	ca_l3_route_t route, lpm_route;
	ca_ip_address_t tmp_ip;
	ca_l3_nexthop_t nexthop, iter_nexthop;
	ca_l3_intf_t tmp_intf;

	lpm_route.prefix.addr_len = 255; // invalid length, invalid route

	/* search all routes' prefix that match tcfg->dest_addr */

	iter.entry_data = &route;
	iter.user_buffer_size = sizeof(route);
	iter.entry_count = 1;
	iter.prev_handle = NULL;
	while (1) {
		ret = ca_l3_route_iterate(0, tcfg->dest_addr.afi, &iter);
		if (ret != CA_E_OK) {
			if (ret == CA_E_UNAVAIL) {
				break;
			} else {
				printk(KERN_ERR "ERROR! Fail to iterate route. (ret %d)\n", ret);
				return ret;
			}
		}

		tmp_ip = tcfg->dest_addr;
		mask_ip_by_len(&tmp_ip, route.prefix.addr_len);

#if 0
		printk("%s: route prefix afi=%d, ip=%08x-%08x-%08x-%08x / %u\n", __func__, route.prefix.afi,
				route.prefix.ip_addr.ipv6_addr[0], route.prefix.ip_addr.ipv6_addr[1],
				route.prefix.ip_addr.ipv6_addr[2], route.prefix.ip_addr.ipv6_addr[3], route.prefix.addr_len);
		printk("%s: masked tunnel dest_addr afi=%d, ip=%08x-%08x-%08x-%08x\n", __func__, tmp_ip.afi,
				tmp_ip.ip_addr.ipv6_addr[0], tmp_ip.ip_addr.ipv6_addr[1],
				tmp_ip.ip_addr.ipv6_addr[2], tmp_ip.ip_addr.ipv6_addr[3]);
#endif

		if ((tmp_ip.ip_addr.ipv6_addr[0] == route.prefix.ip_addr.ipv6_addr[0]) &&
		    (tmp_ip.ip_addr.ipv6_addr[1] == route.prefix.ip_addr.ipv6_addr[1]) &&
		    (tmp_ip.ip_addr.ipv6_addr[2] == route.prefix.ip_addr.ipv6_addr[2]) &&
		    (tmp_ip.ip_addr.ipv6_addr[3] == route.prefix.ip_addr.ipv6_addr[3])) {

			if ((route.prefix.addr_len >= lpm_route.prefix.addr_len) || (lpm_route.prefix.addr_len == 255)) {
				/* save the longest-prefix-matched route */
				if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
					printk(KERN_INFO "%s: Found a route matches tunnel's dest_addr. (prefix=%08x-%08x-%08x-%08x / %u)\n", __func__,
							route.prefix.ip_addr.ipv6_addr[0], route.prefix.ip_addr.ipv6_addr[1],
							route.prefix.ip_addr.ipv6_addr[2], route.prefix.ip_addr.ipv6_addr[3],
							route.prefix.addr_len);
				}

				lpm_route = route;
			}

		}
	}

	if (lpm_route.prefix.addr_len == 255) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk(KERN_INFO "%s: Cannot find any route that matches tunnel's dest_addr.\n", __func__);
		}
		return CA_E_NOT_FOUND;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk(KERN_INFO "%s: The longest matched prefix=%08x-%08x-%08x-%08x / %u, nexthop_id=%u.\n", __func__,
				lpm_route.prefix.ip_addr.ipv6_addr[0], lpm_route.prefix.ip_addr.ipv6_addr[1],
				lpm_route.prefix.ip_addr.ipv6_addr[2], lpm_route.prefix.ip_addr.ipv6_addr[3],
				lpm_route.prefix.addr_len, lpm_route.nexthop_id);
	}

	/* get the nexthop which tcfg->dest_addr is routed to */
	if (lpm_route.nexthop_id >= MIN_BM_SWONLY_NHID) {
		/* SW-only NextHop; it is routed to neighbor host. */
		ret = ca_l3_nexthop_get_by_ip(0, tcfg->dest_addr, &nexthop);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk(KERN_ERR "ERROR! Fail to get nexthop by tunnel dest_addr, maybe not in NextHop table yet. (ret %d)\n", ret);
			}
			return ret;
		}
	} else {
		/* Route to assigned NextHop; NextHop indeed contains a real destination. */
		ret = ca_l3_nexthop_get(0, lpm_route.nexthop_id, &nexthop);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk(KERN_ERR "ERROR! Fail to get nexthop id=%u. (ret %d)\n", lpm_route.nexthop_id, ret);
			}
			return ret;
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk(KERN_INFO "%s: The matched nexthop of id %u: da_mac=%02x-%02x-%02x-%02x-%02x-%02x\n", __func__,
				nexthop.nexthop_id, nexthop.da_mac[0], nexthop.da_mac[1], nexthop.da_mac[2],
				nexthop.da_mac[3], nexthop.da_mac[4], nexthop.da_mac[5]);
		printk(KERN_INFO "%s: Next, find any nexthop's intf that uses tunnel_id %u, then change nexthop's da_mac.\n", __func__, tunnel_id);
	}

	/* search for the IPv4 nexthop that uses tunnel_id, then update the nexthop's da_mac */

	iter.entry_data = &iter_nexthop;
	iter.user_buffer_size = sizeof(iter_nexthop);
	iter.entry_count = 1;
	iter.prev_handle = NULL;
	while (1) {
		ret = ca_l3_nexthop_iterate(0, CA_IPV4, &iter);
		if (ret != CA_E_OK) {
			if (ret == CA_E_UNAVAIL) {
				break;
			} else {
				printk(KERN_ERR "ERROR! Fail to iterate route. (ret %d)\n", ret);
				return ret;
			}
		}

#if 0
		printk(KERN_INFO "%s: iterate a nexthop, id=%u, intf_id=%u\n", __func__, iter_nexthop.nexthop_id, iter_nexthop.intf_id);
#endif

		tmp_intf.intf_id = iter_nexthop.intf_id;
		ret = ca_l3_intf_get(0, &tmp_intf);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to get interface of id %u. (ret %d)\n", __func__, iter_nexthop.intf_id, ret);
			return ret;
		}

		if ((tmp_intf.mask.tunnel_id != 0) && (tmp_intf.tunnel_id == tunnel_id)) {
			/* got the nexthop that uses tunnel_id, then update the MAC */

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk(KERN_INFO "%s: Found a tunnel's IPv4 nexthop to update MAC. (nexthop_id=%u, intf_id=%u)\n",
						__func__, iter_nexthop.nexthop_id, iter_nexthop.intf_id);
			}

			memcpy(&iter_nexthop.da_mac, &nexthop.da_mac, 6);

			ret = ca_l3_nexthop_update(0, &iter_nexthop, iter_nexthop.nexthop_id);
			if (ret != CA_E_OK) {
				printk(KERN_ERR "%s: ERROR! Fail to update nexthop entry of id %u. (ret %d)\n", __func__, iter_nexthop.nexthop_id, ret);
			}
		}
	}

	/* search for the IPv6 nexthop that uses tunnel_id, then update the nexthop's da_mac */

	iter.entry_data = &iter_nexthop;
	iter.user_buffer_size = sizeof(iter_nexthop);
	iter.entry_count = 1;
	iter.prev_handle = NULL;
	while (1) {
		ret = ca_l3_nexthop_iterate(0, CA_IPV6, &iter);
		if (ret != CA_E_OK) {
			if (ret == CA_E_UNAVAIL) {
				break;
			} else {
				printk(KERN_ERR "ERROR! Fail to iterate route. (ret %d)\n", ret);
				return ret;
			}
		}

#if 0
		printk(KERN_INFO "%s: iterate a nexthop, id=%u, intf_id=%u\n", __func__, iter_nexthop.nexthop_id, iter_nexthop.intf_id);
#endif

		tmp_intf.intf_id = iter_nexthop.intf_id;
		ret = ca_l3_intf_get(0, &tmp_intf);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to get interface of id %u. (ret %d)\n", __func__, iter_nexthop.intf_id, ret);
			return ret;
		}

		if ((tmp_intf.mask.tunnel_id != 0) && (tmp_intf.tunnel_id == tunnel_id)) {
			/* got the nexthop that uses tunnel_id, then update the MAC */

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk(KERN_INFO "%s: Found a tunnel's IPv6 nexthop to update MAC. (nexthop_id=%u, intf_id=%u)\n",
						__func__, iter_nexthop.nexthop_id, iter_nexthop.intf_id);
			}

			memcpy(&iter_nexthop.da_mac, &nexthop.da_mac, 6);

			ret = ca_l3_nexthop_update(0, &iter_nexthop, iter_nexthop.nexthop_id);
			if (ret != CA_E_OK) {
				printk(KERN_ERR "%s: ERROR! Fail to update nexthop entry of id %u. (ret %d)\n", __func__, iter_nexthop.nexthop_id, ret);
			}
		}
	}

	return CA_E_OK;
}

ca_status_t update_tunnel_mac(void)
{
	ca_status_t ret;
	ca_iterator_t iter;
	ca_tunnel_iterate_entry_t t_iter;

	iter.entry_data = &t_iter;
	iter.user_buffer_size = sizeof(t_iter);
	iter.entry_count = 1;
	iter.prev_handle = NULL;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk(KERN_INFO "%s: Start to update tunnel's nexthop MAC. Iterating all tunnels...\n", __func__);
	}

	while (1) {
		ret = ca_tunnel_iterate(0, &iter);
		if (ret != CA_E_OK) {
			if (ret == CA_E_UNAVAIL) {
				return ret;
			} else {
				printk(KERN_ERR "ERROR! Fail to iterate tunnel. (ret %d)\n", ret);
				return ret;
			}
		}

		if (iter.entry_count == 0) {
			printk(KERN_ERR "ERROR! Returned data count is 0.\n");
			return CA_E_OK;
		}

		if ((t_iter.tunnel.type == CA_TUNNEL_PPPOE) || (t_iter.tunnel.type == CA_TUNNEL_L2TP)) {
			/* these tunnel config contains MAC already */
			continue;
		}

		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk(KERN_INFO "%s: Found a tunnel. tunnel_id=%u, type=%d, dest_addr=%08x-%08x-%08x-%08x\n",
				__func__, t_iter.tunnel_id, t_iter.tunnel.type,
				t_iter.tunnel.src_addr.ip_addr.ipv6_addr[0], t_iter.tunnel.src_addr.ip_addr.ipv6_addr[1],
				t_iter.tunnel.src_addr.ip_addr.ipv6_addr[2], t_iter.tunnel.src_addr.ip_addr.ipv6_addr[3]);
		}

		go_through_lpm_nexthop(&t_iter.tunnel, t_iter.tunnel_id);
	}

	return CA_E_OK;
}

static ca_status_t ca_kh_nexthop_entry_ifdown(ca_device_id_t device_id, ca_ip_address_t *ip_addr)
{
	int i;
	int ret;
	ca_kh_nexthop_entry_t *tmp_kh_nexthop;
	ca_ip_address_t ip_addr1, ip_addr2;

	if (ip_addr->addr_len == 0)
		return CA_E_OK;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->afi=%d, ip_addr->ip_addr.ipv4_addr=%pI4, ip_addr->addr_len=%d\n",
				__func__, ip_addr->afi, &(ip_addr->ip_addr.ipv4_addr), ip_addr->addr_len);
                }
                else {
			printk("%s: ip_addr->afi=%d, ip_addr->ip_addr.ipv6_addr=%pI6, ip_addr->addr_len=%d\n",
                                __func__, ip_addr->afi, &(ip_addr->ip_addr.ipv6_addr), ip_addr->addr_len);
                }
	}

	if (ip_addr->afi == CA_IPV4) {

                spin_lock_bh(&ca_kh_ipv4_nh_lock);
                for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
                        if (ca_kh_ipv4_nexthop_table[i].is_used == 0)
                                continue;

                        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {

                                tmp_kh_nexthop = &(ca_kh_ipv4_nexthop_table[i]);
                                printk("%s: i=%d, tmp_kh_nexthop->ifindex=%d\n", __func__, i, tmp_kh_nexthop->ifindex);

                                printk("%s: tmp_kh_nexthop->ip_addr.afi=%d, tmp_kh_nexthop->ip_addr.ip_addr.ipv4_addr=%pI4, tmp_kh_nexthop->ip_addr.addr_len=%d\n",
                                        __func__, tmp_kh_nexthop->ip_addr.afi, &tmp_kh_nexthop->ip_addr.ip_addr.ipv4_addr, tmp_kh_nexthop->ip_addr.addr_len);
                        }

			if (ca_kh_ipv4_nexthop_table[i].attr_flags == 0) {

				ip_addr1 = ca_kh_ipv4_nexthop_table[i].ip_addr;
				ip_addr2 = *ip_addr;
				ip_addr1.addr_len = ip_addr2.addr_len;

				ca_kh_ip_address_ntohl(&ip_addr1, &ip_addr1);

				if (ca_kh_is_ip_equal(&ip_addr1, &ip_addr2)) {

					ret = ca_l3_nexthop_delete(device_id, ca_kh_ipv4_nexthop_table[i].l3_nexthop_id);
					if (ret != CA_E_OK) {
						if (ret != CA_E_EXISTS)
							printk("%s: ca_l3_nexthop_delete(nexthop_id=%d), return failed!!, ret=%d\n",
									__func__, ca_kh_ipv4_nexthop_table[i].l3_nexthop_id, ret);
						spin_unlock_bh(&ca_kh_ipv4_nh_lock);
						return ret;
					}

					ca_kh_ipv4_nexthop_table[i].is_used = 0;

					if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
						printk("%s: delete the nexthop at nexthop id = %d\n", __func__, ca_kh_ipv4_nexthop_table[i].l3_nexthop_id);
					}
				}
			}
		}
		spin_unlock_bh(&ca_kh_ipv4_nh_lock);
	}
	else {
		spin_lock_bh(&ca_kh_ipv6_nh_lock);
		for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i ++) {
			if (ca_kh_ipv6_nexthop_table[i].is_used == 0)
				continue;

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				tmp_kh_nexthop = &(ca_kh_ipv6_nexthop_table[i]);
				printk("%s: i=%d, tmp_kh_nexthop->ifindex=%d\n", __func__, i, tmp_kh_nexthop->ifindex);
				printk("%s: tmp_kh_nexthop->ip_addr.afi=%d, tmp_kh_nexthop->ip_addr.ip_addr.ipv6_addr=%pI6, tmp_kh_nexthop->ip_addr.addr_len=%d\n",
					__func__, tmp_kh_nexthop->ip_addr.afi, &tmp_kh_nexthop->ip_addr.ip_addr.ipv6_addr, tmp_kh_nexthop->ip_addr.addr_len);
			}

			if (ca_kh_ipv6_nexthop_table[i].attr_flags == 0) {
				ip_addr1 = ca_kh_ipv6_nexthop_table[i].ip_addr;
				ip_addr2 = *ip_addr;
				ip_addr1.addr_len = ip_addr2.addr_len;

				if (ca_kh_is_ip_equal(&ip_addr1, &ip_addr2)) {

					ret = ca_l3_nexthop_delete(device_id, ca_kh_ipv6_nexthop_table[i].l3_nexthop_id);
					if (ret != CA_E_OK) {
						if (ret != CA_E_EXISTS)
							printk("%s: ca_l3_nexthop_delete(nexthop_id=%d), return failed!!, ret=%d\n",
								__func__, ca_kh_ipv6_nexthop_table[i].l3_nexthop_id, ret);
						spin_unlock_bh(&ca_kh_ipv6_nh_lock);
						return ret;
					}

					ca_kh_ipv6_nexthop_table[i].is_used = 0;

					if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
						printk("%s: delete the nexthop at nexthop id = %d\n", __func__, ca_kh_ipv6_nexthop_table[i].l3_nexthop_id);
					}
				}
			}
		}
		spin_unlock_bh(&ca_kh_ipv6_nh_lock);
	}
	return CA_E_OK;
}

ca_status_t ca_kh_l3_nexthop_add(ca_device_id_t device_id, ca_kh_nexthop_entry_t *kh_nexthop)
{
	int i;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		if (kh_nexthop->ip_addr.afi == CA_IPV4) {
			printk("%s: kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv4_addr=%pI4, kh_nexthop->ip_addr.addr_len=%d\n",
				__func__, kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv4_addr, kh_nexthop->ip_addr.addr_len);
		}
		else {
			printk("%s: kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv6_addr=%pI6, kh_nexthop->ip_addr.addr_len=%d\n",
                        	__func__, kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv6_addr, kh_nexthop->ip_addr.addr_len);
		}
		printk("%s: kh_nexthop->ifindex=%d, kh_nexthop->l3_nexthop_id=%d\n", __func__, kh_nexthop->ifindex, kh_nexthop->l3_nexthop_id);
	}

	if (kh_nexthop->ip_addr.afi == CA_IPV4) {

		spin_lock_bh(&ca_kh_ipv4_nh_lock);
		for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
			if (ca_kh_ipv4_nexthop_table[i].is_used == 0)
				break;
 		}
		spin_unlock_bh(&ca_kh_ipv4_nh_lock);

		if (i == CA_L3_MAX_NEXTHOP_NUM) {
			printk("%s: No more IPV4 ca_kh_nexthop_entry_t is available!!\n", __func__);
			return CA_E_ERROR;
		}

		spin_lock_bh(&ca_kh_ipv4_nh_lock);

		ca_kh_ipv4_nexthop_table[i].is_used = 1;
        	ca_kh_ipv4_nexthop_table[i].attr_flags = kh_nexthop->attr_flags;
        	ca_kh_ipv4_nexthop_table[i].ip_addr = kh_nexthop->ip_addr;
        	ca_kh_ipv4_nexthop_table[i].ifindex = kh_nexthop->ifindex;
        	ca_kh_ipv4_nexthop_table[i].l3_nexthop_id = kh_nexthop->l3_nexthop_id;
		spin_unlock_bh(&ca_kh_ipv4_nh_lock);
	}
	else
	{
		spin_lock_bh(&ca_kh_ipv6_nh_lock);
		for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
                        if (ca_kh_ipv6_nexthop_table[i].is_used == 0)
                                break;
                }
		spin_unlock_bh(&ca_kh_ipv6_nh_lock);

                if (i == CA_L3_MAX_NEXTHOP_NUM) {
                        printk("%s: No more IPV6 ca_kh_nexthop_entry_t is available!!\n", __func__);
                        return CA_E_ERROR;
                }

		spin_lock_bh(&ca_kh_ipv6_nh_lock);
		ca_kh_ipv6_nexthop_table[i].is_used = 1;
        	ca_kh_ipv6_nexthop_table[i].attr_flags = kh_nexthop->attr_flags;
        	ca_kh_ipv6_nexthop_table[i].ip_addr = kh_nexthop->ip_addr;
        	ca_kh_ipv6_nexthop_table[i].ifindex = kh_nexthop->ifindex;
        	ca_kh_ipv6_nexthop_table[i].l3_nexthop_id = kh_nexthop->l3_nexthop_id;
		spin_unlock_bh(&ca_kh_ipv6_nh_lock);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l3_nexthop_add);

ca_status_t ca_kh_l3_nexthop_get(ca_device_id_t device_id, ca_kh_nexthop_entry_t *kh_nexthop)
{
	int i;
	ca_kh_nexthop_entry_t *tmp_kh_nexthop;

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                if (kh_nexthop->ip_addr.afi == CA_IPV4) {
                        printk("%s: kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv4_addr=%pI4, kh_nexthop->ip_addr.addr_len=%d\n",
                                __func__, kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv4_addr, kh_nexthop->ip_addr.addr_len);
                }
                else {
                        printk("%s: kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv6_addr=%pI6, kh_nexthop->ip_addr.addr_len=%d\n",
                                __func__, kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv6_addr, kh_nexthop->ip_addr.addr_len);
                }
                printk("%s: kh_nexthop->ifindex=%d, kh_nexthop->l3_nexthop_id=%d\n", __func__, kh_nexthop->ifindex, kh_nexthop->l3_nexthop_id);
        }

        if (kh_nexthop->ip_addr.afi == CA_IPV4) {

		spin_lock_bh(&ca_kh_ipv4_nh_lock);
                for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
                        if (ca_kh_ipv4_nexthop_table[i].is_used == 0)
                                continue;

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {

				tmp_kh_nexthop = &(ca_kh_ipv4_nexthop_table[i]);
				printk("%s: i=%d, tmp_kh_nexthop->ifindex=%d\n", __func__, i, tmp_kh_nexthop->ifindex);

				printk("%s: tmp_kh_nexthop->ip_addr.afi=%d, tmp_kh_nexthop->ip_addr.ip_addr.ipv4_addr=%pI4, tmp_kh_nexthop->ip_addr.addr_len=%d\n",
					__func__, tmp_kh_nexthop->ip_addr.afi, &tmp_kh_nexthop->ip_addr.ip_addr.ipv4_addr, tmp_kh_nexthop->ip_addr.addr_len);
			}

                	if ((ca_kh_ipv4_nexthop_table[i].ifindex == kh_nexthop->ifindex) &&
				(ca_kh_ipv4_nexthop_table[i].attr_flags == kh_nexthop->attr_flags) &&
				(ca_kh_ipv4_nexthop_table[i].ip_addr.afi == kh_nexthop->ip_addr.afi) &&
				(ca_kh_ipv4_nexthop_table[i].ip_addr.ip_addr.ipv4_addr == kh_nexthop->ip_addr.ip_addr.ipv4_addr) &&
				(ca_kh_ipv4_nexthop_table[i].ip_addr.addr_len == kh_nexthop->ip_addr.addr_len)) {

                		kh_nexthop->l3_nexthop_id = ca_kh_ipv4_nexthop_table[i].l3_nexthop_id;

				if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
					printk("%s: get the nexthop at nexthop id = %d\n", __func__, kh_nexthop->l3_nexthop_id);
				}
				break;
			}
                }
		spin_unlock_bh(&ca_kh_ipv4_nh_lock);

                if (i == CA_L3_MAX_NEXTHOP_NUM) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        	printk("%s: No matched IPV4 ca_kh_nexthop_entry_t is found!!\n", __func__);
			}
                        return CA_E_ERROR;
                }
        }
        else
        {
		spin_lock_bh(&ca_kh_ipv6_nh_lock);
		for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i ++) {
                        if (ca_kh_ipv6_nexthop_table[i].is_used == 0)
                                continue;

			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                                tmp_kh_nexthop = &(ca_kh_ipv6_nexthop_table[i]);
                                printk("%s: i=%d, tmp_kh_nexthop->ifindex=%d\n", __func__, i, tmp_kh_nexthop->ifindex);
                                printk("%s: tmp_kh_nexthop->ip_addr.afi=%d, tmp_kh_nexthop->ip_addr.ip_addr.ipv6_addr=%pI6, tmp_kh_nexthop->ip_addr.addr_len=%d\n",
                                        __func__, tmp_kh_nexthop->ip_addr.afi, &tmp_kh_nexthop->ip_addr.ip_addr.ipv6_addr, tmp_kh_nexthop->ip_addr.addr_len);
                        }

                        if ((ca_kh_ipv6_nexthop_table[i].ifindex == kh_nexthop->ifindex) &&
				(ca_kh_ipv6_nexthop_table[i].attr_flags == kh_nexthop->attr_flags) &&
				(ca_kh_ipv6_nexthop_table[i].ip_addr.afi == kh_nexthop->ip_addr.afi) &&
                                (!memcmp(&(ca_kh_ipv6_nexthop_table[i].ip_addr.ip_addr.ipv6_addr[0]), &(kh_nexthop->ip_addr.ip_addr.ipv6_addr[0]), sizeof(ca_l3_ip_addr_t))) &&
                                (ca_kh_ipv6_nexthop_table[i].ip_addr.addr_len == kh_nexthop->ip_addr.addr_len)) {

                                kh_nexthop->l3_nexthop_id = ca_kh_ipv6_nexthop_table[i].l3_nexthop_id;

                                if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                                        printk("%s: get the nexthop at nexthop id = %d\n", __func__, kh_nexthop->l3_nexthop_id);
                                }
                                break;
                        }
                }
		spin_unlock_bh(&ca_kh_ipv6_nh_lock);

                if (i == CA_L3_MAX_NEXTHOP_NUM) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        	printk("%s: No matched IPV6 ca_kh_nexthop_entry_t is found!!\n", __func__);
			}
                        return CA_E_ERROR;
                }
        }
        return CA_E_OK;

}
EXPORT_SYMBOL(ca_kh_l3_nexthop_get);

ca_status_t ca_kh_l3_nexthop_delete(ca_device_id_t device_id, ca_ip_afi_t afi, ca_l3_nexthop_id_t l3_nexthop_id)
{
        int i;

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: l3_nexthop_id=%d\n", __func__, l3_nexthop_id);
        }

        if (afi == CA_IPV4) {

                spin_lock_bh(&ca_kh_ipv4_nh_lock);
                for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
                        if ((ca_kh_ipv4_nexthop_table[i].is_used == 1) && (ca_kh_ipv4_nexthop_table[i].l3_nexthop_id == l3_nexthop_id))
                                break;
                }
                spin_unlock_bh(&ca_kh_ipv4_nh_lock);

                if (i == CA_L3_MAX_NEXTHOP_NUM) {
                        printk("%s: IPV4 nexthop id is not found!!\n", __func__);
                        return CA_E_ERROR;
                }

                spin_lock_bh(&ca_kh_ipv4_nh_lock);
                ca_kh_ipv4_nexthop_table[i].is_used = 0;
                spin_unlock_bh(&ca_kh_ipv4_nh_lock);
        }
        else
        {
		spin_lock_bh(&ca_kh_ipv6_nh_lock);
                for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
                        if ((ca_kh_ipv6_nexthop_table[i].is_used == 1) && (ca_kh_ipv6_nexthop_table[i].l3_nexthop_id == l3_nexthop_id))
                                break;
                }
                spin_unlock_bh(&ca_kh_ipv6_nh_lock);

                if (i == CA_L3_MAX_NEXTHOP_NUM) {
                        printk("%s: IPV6 nexthop id is not found!!\n", __func__);
                        return CA_E_ERROR;
                }

                spin_lock_bh(&ca_kh_ipv6_nh_lock);
                ca_kh_ipv6_nexthop_table[i].is_used = 0;
                spin_unlock_bh(&ca_kh_ipv6_nh_lock);
	}
	return CA_E_OK;
}

ca_status_t ca_kh_l3_route_add(ca_device_id_t device_id, ca_l3_nexthop_t *l3_nexthop, ca_l3_route_t *l3_route, ca_int16_t ifindex)
{
	int ret, nexthop_ret, route_ret;
        ca_l3_nexthop_id_t  l3_nexthop_id;
        ca_kh_nexthop_entry_t kh_nexthop;

        ret = ca_l3_nexthop_add(device_id, l3_nexthop, &l3_nexthop_id);
	nexthop_ret = ret;
        if (ret != CA_E_OK) {
                if (ret != CA_E_EXISTS) {
                        printk("%s: ca_l3_nexthop_add() return failed!!, ret=%d\n", __func__, ret);
                	return ret;
		} else {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE)
				printk("%s: Nexthop entry of ID %d exists already.\n", __func__, l3_nexthop_id);
                }
        }
	/* add kernel hook nexthop entry only when ca_l3_nexthop_add() return CA_E_OK */
	if (ret == CA_E_OK) {
        	memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));

        	/* We store the network order IP address */
       		ca_kh_ip_address_htonl(&kh_nexthop.ip_addr, &l3_nexthop->addr);

		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                	printk("%s: network order kh_nexthop.ip_addr.ip_addr.ipv4_addr=%pI4\n",
				__func__, &kh_nexthop.ip_addr.ip_addr.ipv4_addr);
        	}

        	kh_nexthop.ifindex = ifindex;
        	kh_nexthop.l3_nexthop_id = l3_nexthop_id;
        	kh_nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;
        	ret = ca_kh_l3_nexthop_add(device_id, &kh_nexthop);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_kh_l3_nexthop_add(l3_nexthop_id=%d) return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
                	return ret;
        	}
	}

        l3_route->nexthop_id = l3_nexthop_id;
        ret = ca_l3_route_add(0, l3_route);

        if (ret != CA_E_OK) {
		route_ret = ret;
		if (route_ret != CA_E_EXISTS) {
			printk("%s: ca_l3_route_add() return failed!!, route_ret=%d\n", __func__, route_ret);
			printk("%s:   IPV4 route, l3_route->prefix.addr_len=%d\n", __func__, l3_route->prefix.addr_len);
			printk("%s:   host order l3_route->prefix.ip_addr.ipv4_addr=%pI4\n", __func__, &l3_route->prefix.ip_addr.ipv4_addr);
		}
		if ((route_ret == CA_E_EXISTS) && (nexthop_ret != CA_E_EXISTS) && (nexthop_ret != CA_E_OK)) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk(KERN_INFO "%s: Route entry exists but nexthop entry may be not matched, delete the nexthop entry of id %d.\n", __func__, l3_nexthop_id);
			}
			ret = ca_l3_nexthop_delete(device_id, l3_nexthop_id);
			if (ret != CA_E_OK) {
				if (ret != CA_E_EXISTS)
					printk("%s: ca_l3_nexthop_delete(nexthop_id=%d), return failed!!, ret=%d\n",
						__func__, l3_nexthop_id, ret);
			}
			else {
				/* only delete kernel hook nexthop entry when ca_l3_nexthop_delete() return CA_E_OK */
				ret = ca_kh_l3_nexthop_delete(device_id, l3_route->prefix.afi, l3_nexthop_id);
				if (ret != CA_E_OK) {
					printk("%s: ca_kh_l3_nexthop_delete(l3_nexthop_id), return failed!!, l3_nexthop_id=%d, ret=%d\n",
						__func__, l3_nexthop_id, ret);
					return ret;
				}
			}
		}
        }

        return ret;
}

static ca_status_t ca_kh_l3_defroute_add(ca_device_id_t device_id, ca_l3_nexthop_t *l3_nexthop, ca_l3_route_t *l3_route, ca_ip_afi_t afi, ca_int16_t ifindex, struct net *net)
{
	int ret;
	ca_uint32_t wan_id;
	ca_int_wan_map_t wan_map;
	ca_int_l3_gw_t l3_gw;
	struct net_device *dev = NULL;

#if 0
	ret = ca_int_l3_gw_del_head(afi);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                	printk("%s: ca_int_l3_gw_del_head(afi=%d) failed!!, ret=%d\n", __func__, afi, ret);
		}
        }
#endif

	memset(&l3_gw, 0, sizeof(ca_int_l3_gw_t));
	dev = dev_get_by_index(net, ifindex);
	if (dev == NULL) {
		printk("%s: cannot get net_device from dev_get_by_index(ifindex=%d)\n", __func__, ifindex);
		return CA_E_PARAM;
	}
	else {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: dev->name=%s\n", __func__, dev->name);
	}
	}
	ret = ca_int_wan_map_find(dev->name, &wan_id, &wan_map);
	dev_put(dev);
	if (ret != CA_E_OK) {
		//printk("%s: ca_int_wan_map_find() failed!!, ret=%d\n", __func__, ret);
		//return ret;

		///TODO: because there is no wan map set we give it a default value
		wan_map.valid = CA_TRUE;
		wan_map.routable = CA_TRUE;
		wan_map.wan_type = CA_WAN_TYPE_INTERNET;       /* bitwise flag defined in ca_wan_type_t */
		strcpy(&(wan_map.if_name[0]), dev->name);
		wan_id = 0;
	}
	wan_map.ifindex = ifindex;
	ret = ca_int_wan_map_set(wan_id, &wan_map);
	if (ret != CA_E_OK) {
		printk("%s: ca_int_wan_map_set() failed!!, ret=%d\n", __func__, ret);
		return ret;
	}
	l3_gw.wan_type = wan_map.wan_type;
	l3_gw.ifindex  = ifindex;
	l3_gw.l3_nexthop = *l3_nexthop;
	l3_gw.l3_route = *l3_route;
	ret = ca_int_l3_gw_add(afi, &l3_gw);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_int_l3_gw_add() return failed!!, ret=%d\n", __func__, ret);
		}
	}
	return ret;
}

static ca_status_t ca_kh_l3_defroute_delete(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int16_t ifindex)
{
        int ret;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: afi=%d, ifindex=%d\n", __func__, afi, ifindex);
	}

	ret = ca_int_l3_gw_del(afi, ifindex);
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_int_l3_gw_del() failed!!, ret=%d\n", __func__, ret);
		}
                return ret;
        }
        return ret;
}

ca_status_t ca_kh_l3_route_add_ipv4_static(ca_device_id_t device_id, int ifindex, struct fib_config *cfg)
{
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;
	int ret;
	ca_l3_nexthop_t l3_nexthop;
	ca_l3_route_t   l3_route;
	ca_l3_intf_t l3_intf;
	ca_kh_nexthop_entry_t kh_nexthop;
	ca_l3_nexthop_t tmp_l3_nexthop;
	ca_tunnel_cfg_t tcfg;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: ifindex=%d, cfg->fc_dst_len=%d, cfg->fc_dst=%pI4, cfg->fc_gw=%pI4\n",
			__func__, ifindex, cfg->fc_dst_len, &cfg->fc_dst, &cfg->fc_gw);
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

        l3_intf.ip_addr.afi = CA_IPV4;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
	if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		l3_intf.intf_id = 0;
		memset(&l3_intf, 0, sizeof(ca_l3_intf_t));
	}
	else {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
		}

        	ret = ca_l3_intf_get(0, &l3_intf);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                	return ret;
        	}
	}

	memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));
	l3_nexthop.addr.ip_addr.ipv4_addr = cfg->fc_gw;

	if (cfg->fc_gw != 0)
		l3_nexthop.addr.addr_len = 32; // l3_intf.ip_addr.addr_len;

	l3_nexthop.addr.afi = CA_IPV4;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
        	printk("%s: network order IP address, l3_nexthop.addr=%pI4\n", __func__, &l3_nexthop.addr.ip_addr.ipv4_addr);
        }

	/* Cortina API accepts host order IP address */
       	ca_kh_ip_address_ntohl(&l3_nexthop.addr, &l3_nexthop.addr);

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: host order IP address, l3_nexthop.addr=%pI4\n", __func__, &l3_nexthop.addr.ip_addr.ipv4_addr);
        }

	l3_nexthop.intf_id = l3_intf.intf_id;
	l3_nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;

	if (l3_intf.type == CA_L3_INTF_TUNNEL) {
		ret = ca_tunnel_get(0, l3_intf.tunnel_id, &tcfg);
		if (ret != CA_E_OK) {
			printk("%s: Fail to get tunnel info. (tunnel_id=%u, ret=%d)\n", __func__, l3_intf.tunnel_id, ret);
			return ret;
		}

		if (tcfg.type == CA_TUNNEL_PPPOE) {
			memcpy(l3_nexthop.da_mac, tcfg.tunnel.pppoe.mac_da, 6);
		}

		if (tcfg.type == CA_TUNNEL_L2TP) {
			memcpy(l3_nexthop.da_mac, tcfg.tunnel.l2tp.peer_l2tp_src_mac, 6);
		}
	}

	memset(&l3_route, 0, sizeof(ca_l3_route_t));
        l3_route.prefix.afi = CA_IPV4;
        l3_route.prefix.ip_addr.ipv4_addr = cfg->fc_dst;
        l3_route.prefix.addr_len = cfg->fc_dst_len;

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: network order l3_route.prefix.ip_addr.ipv4_addr=%pI4\n", __func__, &l3_route.prefix.ip_addr.ipv4_addr);
        }

        /* Cortina API accepts host order IP address */
        ca_kh_ip_address_ntohl(&l3_route.prefix, &l3_route.prefix);

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: host order l3_route.prefix.ip_addr.ipv4_addr=%pI4\n", __func__, &l3_route.prefix.ip_addr.ipv4_addr);
        }

	/* check whether this route is a default route */
	if (cfg->fc_dst == 0) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: The route is a default route!!\n", __func__);
		}
		l3_nexthop.addr.addr_len = 32;
		ret = ca_kh_l3_defroute_add(device_id, &l3_nexthop, &l3_route, CA_IPV4, ifindex, cfg->fc_nlinfo.nl_net);
		if (ret != CA_E_OK) {
                        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                                printk("%s: ca_kh_l3_defroute_add() return failed!!, ret=%d\n", __func__, ret);
                        }
                }
	}
	else {
		/* for static route */
		if (cfg->fc_gw != 0) {
			memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));
			kh_nexthop.ifindex = ifindex;
			kh_nexthop.ip_addr.afi = CA_IPV4;
			kh_nexthop.ip_addr.ip_addr.ipv4_addr = cfg->fc_gw;
			kh_nexthop.ip_addr.addr_len = 32;
			kh_nexthop.attr_flags = 0;
			ret = ca_kh_l3_nexthop_get(device_id, &kh_nexthop);
			if (ret != CA_E_OK) {
				printk("%s: ca_kh_l3_nexthop_get(), return failed!!, ret=%d\n", __func__, ret);
				//return ret;
			}
			else {
				ret = ca_l3_nexthop_get(device_id, kh_nexthop.l3_nexthop_id, &tmp_l3_nexthop);
					if (ret != CA_E_OK) {
						printk("%s: ca_l3_nexthop_get(kh_nexthop.l3_nexthop_id=%d), return failed!!, ret=%d\n",
							__func__, kh_nexthop.l3_nexthop_id, ret);
					return CA_E_OK;
				}
				memcpy(&(l3_nexthop.da_mac[0]), &(tmp_l3_nexthop.da_mac[0]), 6);
				if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
					printk("%s: l3_nexthop.da_mac=%pM\n", __func__, &l3_nexthop.da_mac);
				}
			}
		}
		ret = ca_kh_l3_route_add(device_id, &l3_nexthop, &l3_route, ifindex);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk("%s: ca_kh_l3_route_add() return failed!!, ret=%d\n", __func__, ret);
			}
		}
	}

	update_tunnel_mac();

	return ret;
}

ca_status_t ca_kh_l3_route_add_ipv4_dynamic(ca_device_id_t device_id, int ifindex, ca_uint32_t daddr)
{
	return CA_E_OK;
}

static int ca_kh_l3_route_traverse_cb(ca_device_id_t device_id, ca_l3_route_t *l3_route, void *user_data)
{
	int ret;
	ca_l3_user_data_t l3_user_data = *((ca_l3_user_data_t *)user_data);
	ca_l3_nexthop_id_t l3_nexthop_id;
	ca_l3_intf_t l3_intf;
	ca_l3_nexthop_t l3_nexthop;
	ca_ip_address_t prefix;
	ca_tunnel_cfg_t tunnel_cfg;

        ca_iterator_t   iterator;
        ca_l3_route_t   tmp_l3_route;
        bool nexthop_in_use = false;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
        	if (l3_route->prefix.afi == CA_IPV4) {
			printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
				__func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
		}
		else {
			printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6, l3_route->prefix.addr_len=%d\n",
				__func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv6_addr, l3_route->prefix.addr_len);
		}
        }

	l3_intf.ip_addr.afi = l3_user_data.afi;
	l3_intf.intf_id = l3_user_data.intf_id;

        ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: l3_intf.ip_addr.afi=%d, l3_intf.intf_id=%d, l3_intf.ip_addr.addr_len=%d, l3_intf.type=%d\n", __func__,
			l3_intf.ip_addr.afi, l3_intf.intf_id, l3_intf.ip_addr.addr_len, l3_intf.type);
		if (l3_intf.ip_addr.afi == CA_IPV4) {
			printk("%s: l3_intf.ip_addr.ipv4_addr=%pI4\n", __func__, &l3_intf.ip_addr.ip_addr.ipv4_addr);
		}
		else {
			printk("%s: l3_intf.ip_addr.ipv6_addr=%pI6\n", __func__, &l3_intf.ip_addr.ip_addr.ipv6_addr);
		}
	}

	if (l3_intf.type == CA_L3_INTF_BCAST) {
		prefix = l3_intf.ip_addr;
	} else if (l3_intf.type == CA_L3_INTF_TUNNEL) {
		ret = ca_tunnel_get(device_id, l3_intf.tunnel_id, &tunnel_cfg);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s:%d: ca_tunnel_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
			return ret;
		}

		prefix = tunnel_cfg.dest_addr;
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE)
			printk("%s:%d: tunnel_cfg.type=%d\n", __func__, __LINE__, tunnel_cfg.type);
		/* dslite case */
		if ((tunnel_cfg.type == CA_TUNNEL_DSLITE) || (tunnel_cfg.type == CA_TUNNEL_MAP_E)) {
			prefix = l3_intf.ip_addr;
		}
	} else {
		prefix = l3_intf.ip_addr;
	}

	ret = ca_l3_nexthop_get(device_id, l3_route->nexthop_id, &l3_nexthop);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_nexthop_get(l3_route->nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_route->nexthop_id, ret);
		return CA_E_OK;
	}

	/* check whether the intf_id of nexthop is the same as the input for gateway */
	if (!ca_kh_is_ip_equal(&(l3_route->prefix), &prefix)) {
		/* check static route entry */
		l3_nexthop.addr.addr_len = prefix.addr_len;
		if (!ca_kh_is_ip_equal(&(l3_nexthop.addr), &prefix)) {
			return CA_E_OK;
		}

		/* skip default route */
		if (l3_route->prefix.afi == CA_IPV4) {
			if (l3_route->prefix.ip_addr.ipv4_addr == 0)
				return CA_E_OK;
		}
		else {
			if (l3_route->prefix.ip_addr.ipv6_addr[0] == 0 && l3_route->prefix.ip_addr.ipv6_addr[1] == 0 &&
				l3_route->prefix.ip_addr.ipv6_addr[2] == 0 && l3_route->prefix.ip_addr.ipv6_addr[3] == 0)
				return CA_E_OK;
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: l3_route->nexthop_id=%d\n", __func__, l3_route->nexthop_id);
		printk("%s: l3_nexthop.intf_id=%d, l3_intf.intf_id=%d\n", __func__, l3_nexthop.intf_id, l3_intf.intf_id);
	}
        if (l3_nexthop.intf_id != l3_intf.intf_id) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: intf_id not the same skip, l3_nexthop.intf_id=%d, l3_intf.intf_id=%d\n", __func__, l3_nexthop.intf_id, l3_intf.intf_id);
		}
		return CA_E_OK;
	}

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: Deleting route..\n", __func__);
		if (l3_route->prefix.afi == CA_IPV4) {
			printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
				 __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
		}
		else {
			printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6, l3_route->prefix.addr_len=%d\n",
                               	__func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv6_addr, l3_route->prefix.addr_len);
		}
	}

	/* delete the route entry and nexthop entry */
	ret = ca_l3_route_delete(device_id, l3_route);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_route_delete(), return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	/* Check if the nexthop entry is required by other route entry */
	iterator.prev_handle = NULL;
	iterator.entry_count = 1;
	iterator.entry_data = &tmp_l3_route;
	iterator.user_buffer_size = sizeof(ca_l3_route_t);
	while ((ret = ca_l3_route_iterate(device_id, l3_route->prefix.afi, &iterator)) == CA_E_OK) {
		if (iterator.entry_count == 0)
			break;

		if (tmp_l3_route.nexthop_id == l3_route->nexthop_id) {
			nexthop_in_use = true;
			break;
		}
	}

	if (nexthop_in_use == false) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: Deleting nexthop...\n", __func__);
			printk("%s: l3_route->nexthop_id=%d\n", __func__, l3_route->nexthop_id);
		}

		l3_nexthop_id = l3_route->nexthop_id;
		ret = ca_l3_nexthop_delete(device_id, l3_nexthop_id);
		if (ret != CA_E_OK) {
			if (ret != CA_E_EXISTS)
				printk("%s: ca_l3_nexthop_delete(), return failed!!, ret=%d\n", __func__, ret);
			if (ret == CA_E_EXISTS)
				return CA_E_OK;
			return ret;
		}
		ret = ca_kh_l3_nexthop_delete(device_id, l3_route->prefix.afi, l3_nexthop_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
			return ret;
		}
	} else {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: Nexthop ID %d is in use, skip deleting nexthop.\n", __func__, l3_route->nexthop_id);
		}
	}

	return CA_E_OK;
}

static ca_status_t ca_l3_route_traverse(ca_device_id_t device_id, ca_ip_afi_t afi, void *user_data)
{
        ca_iterator_t   iterator;
        ca_l3_route_t l3_route;
        ca_uint32_t     count;
        ca_status_t     ret;

        /* get first nexthop entry */
        iterator.prev_handle = NULL;
        count = 1;
        iterator.entry_count = count;
        iterator.entry_data = &l3_route;
        iterator.user_buffer_size = sizeof(ca_l3_route_t);

        ret = ca_l3_route_iterate(device_id, afi, &iterator);
        while (ret == CA_E_OK) {
		if (iterator.entry_count == 0)
			break;
                ca_kh_l3_route_traverse_cb(device_id, &l3_route, user_data);
                ret = ca_l3_route_iterate(device_id, afi, &iterator);
        }

	return ret;
}

#if 0
static int ca_kh_l3_nexthop_traverse_cb(ca_device_id_t dev, ca_l3_nexthop_t *l3_nexthop, ca_l3_nexthop_id_t *nexthop_id, void *user_data)
{
        int ret;
        ca_l3_user_data_t l3_user_data = *((ca_l3_user_data_t *)user_data);

        ca_l3_nexthop_id_t l3_nexthop_id;
        ca_l3_intf_t l3_intf;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: nexthop_id=%d\n", __func__, *nexthop_id);
	}

        l3_intf.ip_addr.afi = l3_user_data.afi;
        l3_intf.intf_id = l3_user_data.intf_id;

        ret = ca_l3_intf_get(0, &l3_intf);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_get(), return failed!!, intf_id=%d, ret=%d\n", __func__, l3_intf.intf_id, ret);
                return ret;
        }
	/* if nexthops are created independly then there are no way to store nexthop_ids */
        if (l3_nexthop->intf_id == l3_intf.intf_id) {
                l3_nexthop_id = *nexthop_id;
                ret = ca_l3_nexthop_delete(0, l3_nexthop_id);
                if (ret != CA_E_OK) {
			if (ret != CA_E_EXISTS)
                        	printk("%s: ca_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n",
					__func__, l3_nexthop_id, ret);
                        return ret;
                }
		else {
			ret = ca_kh_l3_nexthop_delete(0, l3_intf.ip_addr.afi, l3_nexthop_id);
			if (ret != CA_E_OK) {
                		printk("%s: ca_kh_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n",
					__func__, l3_nexthop_id, ret);
				return ret;
        		}
		}
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: nexthop_id=%d is deleted\n", __func__, *nexthop_id);
		}
	}
        return CA_E_OK;
}
#endif

ca_status_t ca_kh_l3_route_del_ipv4_ifdown(ca_device_id_t device_id, struct net_device *dev)
{
	int ret;
	ca_l3_user_data_t l3_user_data;
	ca_l3_intf_t l3_intf;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: name=%s, ifindex=%d\n", __func__, dev->name, dev->ifindex);
	}

	l3_user_data.afi = CA_IPV4;

	if (dev->flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, dev->ifindex, CA_L3_INTF_BCAST, &(l3_user_data.intf_id));
	else if (dev->flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, dev->ifindex, CA_L3_INTF_TUNNEL, &(l3_user_data.intf_id));
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, dev->ifindex, ret);
		}
                return ret;
        }

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: l3_user_data.intf_id=%d\n", __func__, l3_user_data.intf_id);
        }

	ret = ca_l3_route_traverse(0, CA_IPV4, &l3_user_data);
	if ((ret != CA_E_OK) && (ret != CA_E_UNAVAIL)) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_l3_route_traverse(), return failed!!, ret=%d\n", __func__, ret);
		}
	}

	l3_intf.ip_addr.afi = l3_user_data.afi;
	l3_intf.intf_id = l3_user_data.intf_id;
	ret = ca_l3_intf_get(0, &l3_intf);
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_l3_intf_get(), return failed!!, intf_id=%d, ret=%d\n", __func__, l3_intf.intf_id, ret);
		}
                return ret;
        }

	///TODO
#if 0
	ret = ca_kh_nat_entry_ifdown(device_id, &l3_intf.ip_addr);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_nat_entry_ifdown(), return failed!!, ret=%d\n", __func__, ret);
	}
#endif

	ca_kh_nexthop_entry_ifdown(device_id, &l3_intf.ip_addr);

	ca_kh_l3_defroute_delete(device_id, CA_IPV4, dev->ifindex);

	update_tunnel_mac();

	return CA_E_OK;

}

ca_status_t ca_kh_l3_route_delete(ca_device_id_t device_id, ca_l3_route_t *l3_route, ca_int16_t ifindex)
{
	struct net_device *dev = NULL;
	unsigned int dev_flags;
	ca_status_t ret;
	ca_l3_nexthop_id_t  l3_nexthop_id;
        ca_l3_nexthop_t l3_nexthop;
        ca_intf_id_t    intf_id;

	ca_iterator_t   iterator;
	ca_l3_route_t	tmp_l3_route;
	bool nexthop_in_use = false;

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	ret = ca_l3_route_get(0, l3_route);
        if (ret != CA_E_OK) {
		/* Maybe there is not any route created for an interface
		 * while this function is called from ca_kh_l3_route_del_ipv4_static or ca_kh_l3_route_del_ipv6_static.
		 */
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s:%d l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
					__func__, __LINE__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
			printk("%s:%d ca_l3_route_get(), return failed!!, ret=%d\n", __func__, __LINE__, ret);
		}
                return ret;
        }

        ret = ca_l3_route_delete(0, l3_route);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_route_delete(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }

        /* check the interface id match with nexthop or not */
	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, l3_route->prefix.afi, ifindex, CA_L3_INTF_BCAST, &intf_id);
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, l3_route->prefix.afi, ifindex, CA_L3_INTF_TUNNEL, &intf_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
                return ret;
        }
        /* check whether the interface table id is the same */
        ret = ca_l3_nexthop_get(device_id, l3_route->nexthop_id, &l3_nexthop);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_nexthop_get(l3_route->nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_route->nexthop_id, ret);
                return CA_E_OK;
        }
	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: l3_route->nexthop_id=%d\n", __func__, l3_route->nexthop_id);
		printk("%s: l3_nexthop.intf_id=%d, l3_intf.intf_id=%d\n", __func__, l3_nexthop.intf_id, intf_id);
	}
        if (l3_nexthop.intf_id != intf_id) {
                //if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        printk("%s: intf_id not the same skip, l3_nexthop.intf_id=%d, intf_id=%d\n", __func__, l3_nexthop.intf_id, intf_id);
                //}
                return CA_E_OK;
        }

	/* Check if the nexthop entry is required by other route entry */
	iterator.prev_handle = NULL;
	iterator.entry_count = 1;
	iterator.entry_data = &tmp_l3_route;
	iterator.user_buffer_size = sizeof(ca_l3_route_t);
	while ((ret = ca_l3_route_iterate(device_id, l3_route->prefix.afi, &iterator)) == CA_E_OK) {
		if (iterator.entry_count == 0)
			break;

		if (tmp_l3_route.nexthop_id == l3_route->nexthop_id) {
			nexthop_in_use = true;
			break;
		}
	}

	if (nexthop_in_use == false) {
		l3_nexthop_id = l3_route->nexthop_id;
		ret = ca_l3_nexthop_delete(0, l3_nexthop_id);
		if (ret != CA_E_OK) {
			if (ret != CA_E_EXISTS)
				printk("%s: ca_l3_nexthop_delete(), return failed!!, ret=%d\n", __func__, ret);
			if (ret == CA_E_EXISTS)
				return CA_E_OK;
			return ret;
		}
		ret = ca_kh_l3_nexthop_delete(device_id, l3_route->prefix.afi, l3_nexthop_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_nexthop_delete(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, l3_nexthop_id, ret);
			return ret;
		}
	}

        return CA_E_OK;
}

ca_status_t ca_kh_l3_route_del_ipv4_static(ca_device_id_t device_id, int ifindex, struct fib_config *cfg)
{
	struct net_device *dev = NULL;
	unsigned int dev_flags;
        int ret;
        ca_l3_route_t l3_route;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: ifindex=%d, cfg->fc_dst_len=%d, cfg->fc_dst=%pI4, cfg->fc_gw=%pI4\n",
			__func__, ifindex, cfg->fc_dst_len, &cfg->fc_dst, &cfg->fc_gw);
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	memset(&l3_route, 0, sizeof(ca_l3_route_t));
	l3_route.prefix.afi = CA_IPV4;
	l3_route.prefix.ip_addr.ipv4_addr = cfg->fc_dst;
	l3_route.prefix.addr_len = cfg->fc_dst_len;

	/* Cortina API accepts host order IP address */
        ca_kh_ip_address_ntohl(&l3_route.prefix, &l3_route.prefix);

	if (cfg->fc_dst == 0) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: This route is a default route!!\n", __func__);
		}
		ret = ca_kh_l3_defroute_delete(device_id, CA_IPV4, ifindex);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_defroute_delete() failed!!, ret=%d\n", __func__, ret);
			return ret;
		}
	} else {
		ret = ca_kh_l3_route_delete(device_id, &l3_route, ifindex);
		if (ret != CA_E_OK) {
			/* Maybe there is not any route created for an interface */
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk("%s:%d l3_route.prefix.afi=%d, l3_route.prefix.ip_addr.ipv4_addr=%pI4, l3_route.prefix.addr_len=%d\n",
						__func__, __LINE__, l3_route.prefix.afi, &l3_route.prefix.ip_addr.ipv4_addr, l3_route.prefix.addr_len);
				printk("%s:%d ca_kh_l3_route_del(), return failed!!, ret=%d\n", __func__, __LINE__, ret);
			}
                	return ret;
        	}
	}

	update_tunnel_mac();

	return ret;
}

ca_status_t ca_kh_l3_route_add_ipv6_static(ca_device_id_t device_id, int ifindex, struct fib6_config *cfg)
{
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;
        int ret;
        ca_l3_nexthop_t l3_nexthop;
        ca_l3_route_t   l3_route;
        ca_l3_intf_t l3_intf;
	ca_kh_nexthop_entry_t kh_nexthop;
	ca_l3_nexthop_t tmp_l3_nexthop;
	ca_tunnel_cfg_t tcfg;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: ifindex=%d, cfg->fc_table=%u, cfg->fc_flags=0x%08x, cfg->fc_type=0x%x, cfg->fc_protocol=%d\n",
			__func__, ifindex, cfg->fc_table, cfg->fc_flags, cfg->fc_type, cfg->fc_protocol);
		printk("%s: cfg->fc_dst=%pI6 / %d, cfg->fc_gateway=%pI6\n",
			__func__, &cfg->fc_dst, cfg->fc_dst_len, &cfg->fc_gateway);
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	if (ifindex <= 0) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE)
			printk("%s: Error! ifindex must be positive. (ifindex=%d)\n", __func__, ifindex);
		return CA_E_PARAM;
	}

        l3_intf.ip_addr.afi = CA_IPV6;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		l3_intf.intf_id = 0;
		memset(&l3_intf, 0, sizeof(ca_l3_intf_t));
        } else {
        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        }

        ret = ca_l3_intf_get(0, &l3_intf);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }
	}

        memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));

        memcpy(&(l3_nexthop.addr.ip_addr.ipv6_addr[0]), &(cfg->fc_gateway.s6_addr32[0]), sizeof(ca_l3_ip_addr_t));

	if (cfg->fc_gateway.s6_addr32[0] != 0 || cfg->fc_gateway.s6_addr32[1] != 0 || cfg->fc_gateway.s6_addr32[2] != 0 || cfg->fc_gateway.s6_addr32[3] != 0)
		l3_nexthop.addr.addr_len = 128; // l3_intf.ip_addr.addr_len;

       	l3_nexthop.addr.afi = CA_IPV6;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
               	printk("%s: network order IP address, l3_nexthop.addr=%pI6\n", __func__, &l3_nexthop.addr.ip_addr.ipv6_addr);
       	}

       	/* Cortina API accepts host order IP address */
       	ca_kh_ip_address_ntohl(&l3_nexthop.addr, &l3_nexthop.addr);

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: host order IP address, l3_nexthop.addr=%pI6\n", __func__, &l3_nexthop.addr.ip_addr.ipv6_addr);
        }

        l3_nexthop.intf_id = l3_intf.intf_id;
	l3_nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;

	if (l3_intf.type == CA_L3_INTF_TUNNEL) {
		ret = ca_tunnel_get(0, l3_intf.tunnel_id, &tcfg);
		if (ret != CA_E_OK) {
			printk("%s: Fail to get tunnel info. (tunnel_id=%u, ret=%d)\n", __func__, l3_intf.tunnel_id, ret);
			return ret;
		}

		if (tcfg.type == CA_TUNNEL_PPPOE) {
			memcpy(l3_nexthop.da_mac, tcfg.tunnel.pppoe.mac_da, 6);
		}

		if (tcfg.type == CA_TUNNEL_L2TP) {
			memcpy(l3_nexthop.da_mac, tcfg.tunnel.l2tp.peer_l2tp_src_mac, 6);
		}
	}

	memset(&l3_route, 0, sizeof(ca_l3_route_t));
        l3_route.prefix.afi = CA_IPV6;
        memcpy(&(l3_route.prefix.ip_addr.ipv6_addr), &(cfg->fc_dst.s6_addr32[0]), sizeof(ca_l3_ip_addr_t));
        l3_route.prefix.addr_len = cfg->fc_dst_len;

        /* Cortina API accepts host order IP address */
        ca_kh_ip_address_ntohl(&l3_route.prefix, &l3_route.prefix);

        /* check whether this route is a default route */
	if (cfg->fc_dst.s6_addr32[0] == 0 && cfg->fc_dst.s6_addr32[1] == 0 && cfg->fc_dst.s6_addr32[2] == 0 && cfg->fc_dst.s6_addr32[3] == 0) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: The route is a default route!!\n", __func__);
		}
		l3_nexthop.addr.addr_len = 128;
                ret = ca_kh_l3_defroute_add(device_id, &l3_nexthop, &l3_route, CA_IPV6, ifindex, cfg->fc_nlinfo.nl_net);
                if (ret != CA_E_OK) {
                        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                                printk("%s: ca_kh_l3_defroute_add() return failed!!, ret=%d\n", __func__, ret);
                        }
                }
        }
	else {
		/* for static route */
		if (cfg->fc_gateway.s6_addr32[0] != 0 || cfg->fc_gateway.s6_addr32[1] != 0 || cfg->fc_gateway.s6_addr32[2] != 0 || cfg->fc_gateway.s6_addr32[3] != 0) {
			memset(&kh_nexthop, 0, sizeof(ca_kh_nexthop_entry_t));
			kh_nexthop.ifindex = ifindex;
			kh_nexthop.ip_addr.afi = CA_IPV6;
			memcpy(&(kh_nexthop.ip_addr.ip_addr.ipv6_addr), &(cfg->fc_gateway.s6_addr32[0]), sizeof(ca_l3_ip_addr_t));
			kh_nexthop.ip_addr.addr_len = 128;
			kh_nexthop.attr_flags = 0;
			ret = ca_kh_l3_nexthop_get(device_id, &kh_nexthop);
			if (ret != CA_E_OK) {
				printk("%s: ca_kh_l3_nexthop_get(), return failed!!, ret=%d\n", __func__, ret);
				//return ret;
			}
			else {
				ret = ca_l3_nexthop_get(device_id, kh_nexthop.l3_nexthop_id, &tmp_l3_nexthop);
				if (ret != CA_E_OK) {
					printk("%s: ca_l3_nexthop_get(kh_nexthop.l3_nexthop_id=%d), return failed!!, ret=%d\n",
							__func__, kh_nexthop.l3_nexthop_id, ret);
					return CA_E_OK;
				}
				memcpy(&(l3_nexthop.da_mac[0]), &(tmp_l3_nexthop.da_mac[0]), 6);
				if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
					printk("%s: l3_nexthop.da_mac=%pM\n", __func__, &l3_nexthop.da_mac);
				}
			}
		}
		ret = ca_kh_l3_route_add(device_id, &l3_nexthop, &l3_route, ifindex);
        	if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        	printk("%s: ca_kh_l3_route_add() return failed!!, ret=%d\n", __func__, ret);
                	}
        	}
	}

	update_tunnel_mac();

        return ret;
}

ca_status_t ca_kh_l3_route_del_ipv6_static(ca_device_id_t device_id, struct rt6_info *rt)
{
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;
	int ret;
	ca_l3_route_t l3_route;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: ifindex=%d, rt6i_dst=%pI6/%d, rt6i_gateway=%pI6\n",
			__func__, rt->dst.dev->ifindex, &rt->rt6i_dst.addr, rt->rt6i_dst.plen, &rt->rt6i_gateway);
	}

	dev = dev_get_by_index(&init_net, rt->dst.dev->ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, rt->dst.dev->ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	memset(&l3_route, 0, sizeof(ca_l3_route_t));
	l3_route.prefix.afi = CA_IPV6;
	memcpy(&(l3_route.prefix.ip_addr.ipv6_addr), &(rt->rt6i_dst.addr.s6_addr32[0]), sizeof(ca_l3_ip_addr_t));
	l3_route.prefix.addr_len = rt->rt6i_dst.plen;

	/* Cortina API accepts host order IP address */
	ca_kh_ip_address_ntohl(&l3_route.prefix, &l3_route.prefix);

	if (rt->rt6i_dst.addr.s6_addr32[0] == 0 && rt->rt6i_dst.addr.s6_addr32[1] == 0 && rt->rt6i_dst.addr.s6_addr32[2] == 0 && rt->rt6i_dst.addr.s6_addr32[3] == 0) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: This route is a default route!!\n", __func__);
		}
		ret = ca_kh_l3_defroute_delete(device_id, CA_IPV6, rt->dst.dev->ifindex);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_defroute_delete() failed!!, ret=%d\n", __func__, ret);
			return ret;
		}
	} else {
	ret = ca_kh_l3_route_delete(device_id, &l3_route, rt->dst.dev->ifindex);
	if (ret != CA_E_OK) {
			/* Maybe there is not any route created for an interface */
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				printk("%s:%d l3_route.prefix.afi=%d, l3_route.prefix.ip_addr.ipv6_addr=%pI6, l3_route.prefix.addr_len=%d\n",
						__func__, __LINE__, l3_route.prefix.afi, &l3_route.prefix.ip_addr.ipv6_addr, l3_route.prefix.addr_len);
				printk("%s:%d ca_kh_l3_route_del(), return failed!!, ret=%d\n", __func__, __LINE__, ret);
			}
		return ret;
	}
	}

	update_tunnel_mac();

	return ret;
}

ca_status_t ca_kh_l3_route_del_ipv6_ifdown(ca_device_id_t device_id, struct net_device *dev)
{
        int ret;
        ca_l3_user_data_t l3_user_data;
	ca_l3_intf_t l3_intf;

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: name=%s, ifindex=%d\n", __func__, dev->name, dev->ifindex);
        }

        l3_user_data.afi = CA_IPV6;

	if (dev->flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, dev->ifindex, CA_L3_INTF_BCAST, &(l3_user_data.intf_id));
	else if (dev->flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, dev->ifindex, CA_L3_INTF_TUNNEL, &(l3_user_data.intf_id));
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }

        if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                printk("%s: l3_user_data.intf_id=%d\n", __func__, l3_user_data.intf_id);
        }

        ret = ca_l3_route_traverse(0, CA_IPV6, &l3_user_data);
	if ((ret != CA_E_OK) && (ret != CA_E_UNAVAIL)) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_l3_route_traverse(), return failed!!, ret=%d\n", __func__, ret);
		}
	}

	l3_intf.ip_addr.afi = l3_user_data.afi;
	l3_intf.intf_id = l3_user_data.intf_id;
	ret = ca_l3_intf_get(0, &l3_intf);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
			printk("%s: ca_l3_intf_get(), return failed!!, intf_id=%d, ret=%d\n", __func__, l3_intf.intf_id, ret);
		}
		return ret;
	}
	ca_kh_nexthop_entry_ifdown(device_id, &l3_intf.ip_addr);

	ca_kh_l3_defroute_delete(device_id, CA_IPV6, dev->ifindex);

	update_tunnel_mac();

        return CA_E_OK;
}

ca_status_t ca_kh_l3_nexthop_aging_timer_set(ca_device_id_t device_id, ca_uint32_t time)
{
	int ret;
	ca_uint32_t old_time;

	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: time=%d\n", __func__, time);
	}
	ret = ca_l3_nexthop_aging_timer_get(device_id, &old_time);
	if (ret != CA_E_OK){
		printk("%s: ca_l3_nexthop_aging_timer_get(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		printk("%s: old_time=%d\n", __func__, old_time);
	}
	ret = ca_l3_nexthop_aging_timer_set(device_id, time);
	if (ret != CA_E_OK){
                printk("%s: ca_l3_nexthop_aging_timer_set(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }
	ca_l3_nexthop_aging_timer_get(device_id, &old_time);
	if (ret != CA_E_OK){
                printk("%s: ca_l3_nexthop_aging_timer_get() after ca_l3_nexthop_aging_timer_set(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }
	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
       		printk("%s: new_time=%d\n", __func__, old_time);
	}
	return CA_E_OK;
}
