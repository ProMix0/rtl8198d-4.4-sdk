#include <linux/netdevice.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>

#include <soc/cortina/ca_kernel_hook.h>

#include "ca_kernel_hook_util.h"

#define DBG(x)	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) (x)

static ca_int_wan_map_t wan_map_list[CA_MAX_WAN_NUM];
static ca_int_l3_gw_t *ca_gw_v4_list = NULL;
static ca_int_l3_gw_t *ca_gw_v6_list = NULL;

/*****************************************************************************/
/* USAGE:
 * 1. UI calls ca_int_wan_map_set() when a new WAN interface is created with specific if_name and wan_id.
 * 2. Routing kernel hook function calls ca_int_wan_map_find() to get wan_id and wan map by if_name.
 * 3. Routing kernel hook function calls ca_int_wan_map_set() to update ifindex.
 * 4. UI calls ca_int_wan_map_set() when a WAN interface is deleted.
 */

// Internal API for UI and routing kernel hook function
ca_status_t ca_int_wan_map_set(ca_uint32_t wan_id, ca_int_wan_map_t *wan_map) {
	/* store wan_map in index win_id */
	/* if wan_id >= 8, return CA_E_PARAM */
	DBG(printk("%s:%d: wan_id %d, &wan_map 0x%p\n",
		__func__, __LINE__,
		wan_id, wan_map));

	if (wan_id >= CA_MAX_WAN_NUM || wan_map == NULL)
		return CA_E_PARAM;

	DBG(printk("%s:%d: wan_map valid %d, wan_type 0x%x, if_name [%s], ifindex %d\n",
		__func__, __LINE__,
		wan_map->valid, wan_map->wan_type, wan_map->if_name, wan_map->ifindex));

	spin_lock_bh(&ca_kh_wan_map_lock);
	memcpy(&wan_map_list[wan_id], wan_map, sizeof(ca_int_wan_map_t));
	spin_unlock_bh(&ca_kh_wan_map_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_int_wan_map_set);

// Internal API for UI
ca_status_t ca_int_wan_map_get(ca_uint32_t wan_id, ca_int_wan_map_t *wan_map) {
	/* get wan_map from index win_id */
	/* if wan_id >= 8, return CA_E_PARAM */
	DBG(printk("%s:%d: wan_id %d, &wan_map 0x%p\n",
		__func__, __LINE__,
		wan_id, wan_map));

	if (wan_id >= CA_MAX_WAN_NUM || wan_map == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&ca_kh_wan_map_lock);
	memcpy(wan_map, &wan_map_list[wan_id], sizeof(ca_int_wan_map_t));
	spin_unlock_bh(&ca_kh_wan_map_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_int_wan_map_get);

// Internal API for routing kernel hook function
ca_status_t ca_int_wan_map_find(ca_uint8_t *if_name, ca_uint32_t *wan_id,
						ca_int_wan_map_t *wan_map) {
	/* find the wan_id and wan_map by if_name */
	int i;

	DBG(printk("%s:%d: &if_name 0x%p, &wan_id 0x%p, &wan_map 0x%p\n",
		__func__, __LINE__,
		if_name, wan_id, wan_map));

	if (if_name == NULL || wan_id == NULL || wan_map == NULL)
		return CA_E_PARAM;

	DBG(printk("%s:%d: if_name [%s]\n",
		__func__, __LINE__,
		if_name));

	spin_lock_bh(&ca_kh_wan_map_lock);
	for (i = 0; i < CA_MAX_WAN_NUM; i++) {
		if ((wan_map_list[i].valid == TRUE) &&
			(strcmp(if_name, wan_map_list[i].if_name) == 0)) {
			*wan_id = i;
			memcpy(wan_map, &wan_map_list[i],
					sizeof(ca_int_wan_map_t));
			spin_unlock_bh(&ca_kh_wan_map_lock);
			return CA_E_OK;

		}
	}
	spin_unlock_bh(&ca_kh_wan_map_lock);
	return CA_E_NOT_FOUND;
}

/*****************************************************************************/
/* USAGE:
 * For ca_kh_l3_route_add_ipv4_static()
 * 1. call ca_int_wan_map_get() and ca_int_wan_map_set() to check if it is INTERNET conneciton.
 * 2. call ca_int_l3_gw_add() to add the new default gateway
 * For ca_kh_l3_route_del_ipv4_static()
 * 1. call ca_int_wan_map_get() to check if it is INTERNET conneciton.
 * 2. call ca_int_l3_gw_del() to delete the default gateway
 * 3. if the default gateway in fast path is deleted, then add the latest default gateway
 * For ca_kh_l3_route_del_ipv4_ifdown()
 * 1. call ca_int_l3_gw_get() to get its gw
 * 2. call ca_int_l3_gw_del() to delete gw and get is_head
 * 3. if the default gateway in fast path is deleted, then add the latest default gateway
 */

// Internal API for ca_int_l3_gw_add()
static ca_status_t ca_int_l3_gw_del_head(ca_ip_afi_t afi) {
	/* delete a default gateway from the head of internal list */
	/* use l3_route and l3_nexthop_id to delete route and nexthop entry */
	/* the gw entry is still kept in the internal list */
	/*
	 * 1. get the head of internal list
	 * 2. call ca_kh_l3_route_delete() to remove the default gateway
	 */
	int ret;
	ca_int_l3_gw_t *gw_node;

	DBG(printk("%s:%d: afi %d\n",
		__func__, __LINE__,
		afi));

	if (afi == CA_IPV4) {
		gw_node = ca_gw_v4_list;
	} else {
		gw_node = ca_gw_v6_list;
	}

	if (gw_node != NULL) {
		/* delete the old default route */
		ret = ca_kh_l3_route_delete(gw_node->device_id,
				&gw_node->l3_route, gw_node->ifindex);
		DBG(printk("%s:%d: ca_kh_l3_route_delete() ret = %d\n",
			__func__, __LINE__, ret));
		return ret;
	}
	return CA_E_OK;
}


// Internal API for routing kernel hook function
ca_status_t ca_int_l3_gw_add(ca_ip_afi_t afi, ca_int_l3_gw_t *gw) {
	/* add a default gateway to a internal list */
	/* store enough information to call ca_l3_nexthop_add() and ca_l3_route_add() */
	/*
	 * 1. call ca_int_l3_gw_del_head() to delete the current default gateway
	 * 1. call ca_l3_nexthop_add() to add the nexthop of the new default gateway
	 * 2. call ca_l3_route_add() to add the new default gateway
	 * 3. allocate memory for gw and store gw in a internal list
	 */
	int ret;
	ca_int_l3_gw_t *gw_node, *head;
	ca_boolean_t hit = FALSE;
	int i;

	DBG(printk("%s:%d: afi %d, &gw 0x%p\n",
		__func__, __LINE__,
		afi, gw));

	if (gw == NULL)
		return CA_E_PARAM;

	DBG(printk("%s:%d: gw device_id %d, wan_type %d, ifindex %d\n",
		__func__, __LINE__,
		gw->device_id, gw->wan_type, gw->ifindex));

	if ((gw->wan_type & CA_WAN_TYPE_INTERNET) == 0)
		return CA_E_ERROR;

	spin_lock_bh(&ca_kh_gw_lock);
	/* force to delete old default gateway if possible */
	ca_int_l3_gw_del_head(afi);

	/* add the new default route */
	ret = ca_kh_l3_route_add(gw->device_id, &gw->l3_nexthop,
					&gw->l3_route, gw->ifindex);
	DBG(printk("%s:%d: ca_kh_l3_route_add() ret = %d\n",
		__func__, __LINE__, ret));
	if (ret != CA_E_OK) {
		spin_unlock_bh(&ca_kh_gw_lock);
		return ret;
	}

	/* check for duplicate entry */
	if (afi == CA_IPV4) {
		gw_node = ca_gw_v4_list;
		head = ca_gw_v4_list;
	} else {
		gw_node = ca_gw_v6_list;
		head = ca_gw_v6_list;
	}

	DBG(printk("%s:%d: afi %d, list head 0x%p\n",
		__func__, __LINE__,
		afi, gw_node));

	while (gw_node != NULL) {
		if (gw_node->ifindex == gw->ifindex) {
			/* hit */
			hit = TRUE;
			/* The entry exists, and we should overwrite it */
			/* we remove it from the list and then add it to the head */
			if (gw_node == head) {
				/* head of list */
				if (afi == CA_IPV4)
					ca_gw_v4_list = gw_node->next;
				else
					ca_gw_v6_list = gw_node->next;

				DBG(printk("%s:%d: remove node from head, new head = 0x%p\n",
					__func__, __LINE__, gw_node->next));
			}
			if (gw_node->next)
				gw_node->next->prev = gw_node->prev;

			if (gw_node->prev)
				gw_node->prev->next = gw_node->next;

			break;
		}
		gw_node = gw_node->next;
	}

	if (hit == FALSE) {
		gw_node = kmalloc(sizeof(ca_int_l3_gw_t), GFP_ATOMIC);
		if (gw_node == NULL) {
			spin_unlock_bh(&ca_kh_gw_lock);
			return CA_E_MEMORY;
		}
	}

	memcpy(gw_node, gw, sizeof(ca_int_l3_gw_t));

	if (afi == CA_IPV4) {
		gw_node->next = ca_gw_v4_list;
		ca_gw_v4_list = gw_node;
	} else {
		gw_node->next = ca_gw_v6_list;
		ca_gw_v6_list = gw_node;
	}

	if (gw_node->next)
		gw_node->next->prev = gw_node;

	gw_node->prev = NULL;

	DBG(printk("%s:%d: afi %d, gw_node 0x%p, next 0x%p, prev 0x%p\n",
		__func__, __LINE__,
		afi, gw_node, gw_node->next, gw_node->prev));

	i = 0;
	if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
		if (afi == CA_IPV4) {
			gw_node = ca_gw_v4_list;
			printk("%s:%d: head ca_gw_v4_list\n", __func__, __LINE__);
			while (gw_node) {
				printk("\t--> 0x%p (prev 0x%p) (ifindex %d) (ip %pI4)\n",
					gw_node, gw_node->prev, gw_node->ifindex,
					&gw_node->l3_nexthop.addr.ip_addr.ipv4_addr);
				gw_node = gw_node->next;
				i++;
			}
			printk("\t [cnt %d]\n", i);
		} else {
			gw_node = ca_gw_v6_list;
			printk("%s:%d: head ca_gw_v6_list\n", __func__, __LINE__);
			while (gw_node) {
				printk("\t--> 0x%p (prev 0x%p) (ifindex %d) (ip %pI6)\n",
					gw_node, gw_node->prev, gw_node->ifindex,
					gw_node->l3_nexthop.addr.ip_addr.ipv6_addr);
				gw_node = gw_node->next;
				i++;
			}
			printk("\t [cnt %d]\n", i);
		}
	}

	spin_unlock_bh(&ca_kh_gw_lock);
	return CA_E_OK;
}

// Internal API for routing kernel hook function
ca_status_t ca_int_l3_gw_del(ca_ip_afi_t afi, ca_uint32_t ifindex) {
	/* delete a default gateway to a internal list */
	/* use l3_route and l3_nexthop_id to delete route and nexthop entry */
	/*
	 * 1. find the gw by ifindex and remove it from the internal list
	 * 2. if it is not the head of the list, then go to step 8
	 * 3. call ca_kh_l3_route_delete() to remove the old default gateway
	 * 4. call ca_kh_l3_route_add() to add the new default gateway
	 * 5. free gw
	 */
	int ret;
	ca_int_l3_gw_t *gw_node, *head, *gw_2nd = NULL;
	int i;

	DBG(printk("%s:%d: afi %d, ifindex %d\n",
		__func__, __LINE__,
		afi, ifindex));

	spin_lock_bh(&ca_kh_gw_lock);
	if (afi == CA_IPV4) {
		gw_node = ca_gw_v4_list;
		head = ca_gw_v4_list;
	} else {
		gw_node = ca_gw_v6_list;
		head = ca_gw_v6_list;
	}

	DBG(printk("%s:%d: afi %d, list head 0x%p\n",
		__func__, __LINE__,
		afi, gw_node));

	while (gw_node != NULL) {
		if (gw_node->ifindex == ifindex) {
			/* hit */
			if (gw_node == head) {
				/* head of list */
				/* delete the old default route */
				ret = ca_kh_l3_route_delete(gw_node->device_id,
						&gw_node->l3_route, ifindex);
				DBG(printk("%s:%d: ca_kh_l3_route_delete() ret = %d\n",
					__func__, __LINE__, ret));

				if (ret != CA_E_OK) {
					spin_unlock_bh(&ca_kh_gw_lock);
					return ret;
				}

				gw_2nd = gw_node->next;
				if (afi == CA_IPV4)
					ca_gw_v4_list = gw_2nd;
				else
					ca_gw_v6_list = gw_2nd;

				/* add the latest default gateway in the list */
				if (gw_2nd) {
					ret = ca_kh_l3_route_add(gw_2nd->device_id,
							&gw_2nd->l3_nexthop,
							&gw_2nd->l3_route,
							gw_2nd->ifindex);
					DBG(printk("%s:%d: ca_kh_l3_route_add() ret = %d\n",
						__func__, __LINE__, ret));
				}
			}
			if (gw_node->next)
				gw_node->next->prev = gw_node->prev;

			if (gw_node->prev)
				gw_node->prev->next = gw_node->next;

			DBG(printk("%s:%d: afi %d, gw_node 0x%p, next 0x%p, prev 0x%p\n",
				__func__, __LINE__,
				afi, gw_node, gw_node->next, gw_node->prev));

			i = 0;
			if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
				if (afi == CA_IPV4) {
					gw_node = ca_gw_v4_list;
					printk("%s:%d: head ca_gw_v4_list\n", __func__, __LINE__);
					while (gw_node) {
						printk("\t--> 0x%p (prev 0x%p) (ifindex %d) (ip %pI4)\n",
							gw_node, gw_node->prev, gw_node->ifindex,
							&gw_node->l3_nexthop.addr.ip_addr.ipv4_addr);
						gw_node = gw_node->next;
						i++;
					}
					printk("\t [cnt %d]\n", i);
				} else {
					gw_node = ca_gw_v6_list;
					printk("%s:%d: head ca_gw_v6_list\n", __func__, __LINE__);
					while (gw_node) {
						printk("\t--> 0x%p (prev 0x%p) (ifindex %d) (ip %pI6)\n",
							gw_node, gw_node->prev, gw_node->ifindex,
							gw_node->l3_nexthop.addr.ip_addr.ipv6_addr);
						gw_node = gw_node->next;
						i++;
					}
					printk("\t [cnt %d]\n", i);
				}
			}

			spin_unlock_bh(&ca_kh_gw_lock);
			kfree(gw_node);
			return CA_E_OK;
		}
		gw_node = gw_node->next;
	}
	spin_unlock_bh(&ca_kh_gw_lock);
	return CA_E_NOT_FOUND;
}

// Internal API for ARP kernel hook function
ca_status_t ca_int_l3_gw_upd(ca_uint32_t ifindex, ca_ip_address_t *ip, ca_uint8_t *mac) {
	/* use ip->afi, ip->ip_addr and ifindex to find gw */
	/* update mac address of gw */
	ca_int_l3_gw_t *gw_node, *head;
	int ret;

	DBG(printk("%s:%d: ifindex %d, &ip 0x%p, &mac 0x%p\n",
		__func__, __LINE__,
		ifindex, ip, mac));

	if (ip == NULL || mac == NULL)
		return CA_E_PARAM;

	if (ip->afi == CA_IPV4) {
		DBG(printk("%s:%d: ip->afi %d, ip->ip_addr %pI4, mac %pM\n",
			__func__, __LINE__,
			ip->afi, ip->ip_addr.addr, mac));
	} else {
		DBG(printk("%s:%d: ip->afi %d, ip->ip_addr %pI6, mac %pM\n",
			__func__, __LINE__,
			ip->afi, ip->ip_addr.addr, mac));
	}

	spin_lock_bh(&ca_kh_gw_lock);
	if (ip->afi == CA_IPV4) {
		gw_node = ca_gw_v4_list;
		head = ca_gw_v4_list;
	} else {
		gw_node = ca_gw_v6_list;
		head = ca_gw_v6_list;
	}

	while (gw_node != NULL) {
		if (gw_node->ifindex == ifindex &&
			ca_kh_is_ip_equal(&gw_node->l3_nexthop.addr, ip) == TRUE) {
			/* hit */
			/* check if MAC is different */
			if (memcmp(gw_node->l3_nexthop.da_mac, mac, CA_ETH_ADDR_LEN) == 0) {
				spin_unlock_bh(&ca_kh_gw_lock);
				return CA_E_OK;
			}

			/* check if it is head */
			if (gw_node == head) {
				/* try to delete the old default route */
				ret = ca_kh_l3_route_delete(gw_node->device_id,
						&gw_node->l3_route, gw_node->ifindex);
				DBG(printk("%s:%d: ca_kh_l3_route_delete() ret = %d\n",
					__func__, __LINE__, ret));

				/* update MAC */
				memcpy(gw_node->l3_nexthop.da_mac, mac, CA_ETH_ADDR_LEN);

				/* add the new default route */
				ret = ca_kh_l3_route_add(gw_node->device_id, &gw_node->l3_nexthop,
								&gw_node->l3_route, gw_node->ifindex);
				DBG(printk("%s:%d: ca_kh_l3_route_add() ret = %d\n",
					__func__, __LINE__, ret));
				if (ret != CA_E_OK) {
					spin_unlock_bh(&ca_kh_gw_lock);
					return ret;
				}
			} else {
				/* update MAC */
				memcpy(gw_node->l3_nexthop.da_mac, mac, CA_ETH_ADDR_LEN);
			}

			spin_unlock_bh(&ca_kh_gw_lock);
			return CA_E_OK;
		}
		gw_node = gw_node->next;
	}
	spin_unlock_bh(&ca_kh_gw_lock);
	return CA_E_NOT_FOUND;
}

// Internal API for classifier
ca_status_t ca_int_l3_gw_get(ca_ip_afi_t afi, ca_uint32_t ifindex, ca_int_l3_gw_t *gw) {
	/* use ip->afi and ifindex to find gw */
	ca_int_l3_gw_t *gw_node;

	DBG(printk("%s:%d: afi %d, ifindex %d, &gw 0x%p\n",
		__func__, __LINE__,
		afi, ifindex, gw));

	if (gw == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&ca_kh_gw_lock);
	if (afi == CA_IPV4) {
		gw_node = ca_gw_v4_list;
	} else {
		gw_node = ca_gw_v6_list;
	}

	while (gw_node != NULL) {
		if (gw_node->ifindex == ifindex) {
			/* hit */
			memcpy(gw, gw_node, sizeof(ca_int_l3_gw_t));
			spin_unlock_bh(&ca_kh_gw_lock);
			return CA_E_OK;
		}
		gw_node = gw_node->next;
	}
	spin_unlock_bh(&ca_kh_gw_lock);
	return CA_E_NOT_FOUND;
}

