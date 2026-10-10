
#include <soc/cortina/ca_kernel_hook.h>

#include "classifier.h"
#include "ca_kernel_hook_util.h"

#define MAX_PORT_BINDING	16

#define TRSVC_WAN_CONN_TYPE_NONE        0
#define TRSVC_WAN_CONN_TYPE_BRIDGE      1
#define TRSVC_WAN_CONN_TYPE_ROUTE       2

DEFINE_SPINLOCK(ca_kh_port_bind_lock);

ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
ca_status_t ca_kh_port_binding_del(ca_device_id_t device_id, ca_ip_afi_t afi, ca_port_id_t src_port_id, ca_int16_t dst_ifindex, ca_uint16_t vid, ca_uint16_t connect_mode);

typedef struct ca_kh_port_binding_s {
	ca_uint8_t	valid;
	ca_ip_afi_t	afi;
	ca_port_id_t    src_port_id;
	ca_intf_id_t    dst_intf_id;
	ca_ip_address_t	dst_ip_addr;
	ca_mac_addr_t	mac_da;
	ca_uint16_t	vid;
	ca_uint32_t	class_index;
	ca_uint16_t	connect_mode;
} ca_kh_port_binding_t;

static ca_kh_port_binding_t wan_port_binding[MAX_PORT_BINDING];

typedef struct ca_l3_user_data {
	ca_boolean_t	found_mac;
	ca_intf_id_t    intf_id;
        ca_ip_address_t ip_addr;
        ca_mac_addr_t   mac_da;
} ca_l3_user_data_t;

static int ca_kh_l3_route_traverse_cb(ca_device_id_t device_id, ca_l3_route_t *l3_route, void *user_data)
{
        int ret;
        ca_l3_user_data_t *l3_user_data = (ca_l3_user_data_t *)user_data;
        ca_l3_nexthop_id_t l3_nexthop_id;
        ca_l3_nexthop_t l3_nexthop;
	ca_ip_address_t ip_addr;

        if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                if (l3_route->prefix.afi == CA_IPV4) {
                        printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4, l3_route->prefix.addr_len=%d\n",
                                __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv4_addr, l3_route->prefix.addr_len);
                }
                else {
                        printk("%s: l3_route->prefix.afi=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6, l3_route->prefix.addr_len=%d\n",
                                __func__, l3_route->prefix.afi, &l3_route->prefix.ip_addr.ipv6_addr, l3_route->prefix.addr_len);
                }
                printk("%s: l3_user_data->ip_addr.afi=%d, l3_user_data->intf_id=%d\n", __func__, l3_user_data->ip_addr.afi, l3_user_data->intf_id);
        }

	/* prefix of default route is 0.0.0.0/0 */
	memset(&ip_addr, 0, sizeof(ca_ip_address_t));
	ip_addr.afi = l3_user_data->ip_addr.afi;
        if (ca_kh_is_ip_equal(&(l3_route->prefix), &ip_addr)) {

                if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                        printk("%s: default route found ..\n", __func__);
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
                if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                        printk("%s: nexthop id = %d\n", __func__, l3_nexthop_id);
                }

                ret = ca_l3_nexthop_get(device_id, l3_nexthop_id, &l3_nexthop);
                if (ret != CA_E_OK) {
                        printk("%s: ca_l3_nexthop_get(l3_nexthop_id=%d), return failed!!, ret=%d\n", __func__, ret, l3_nexthop_id);
                        return CA_E_OK;
                }

                /* Modify the nexthop entry which is linked by static route to WAN port, like as 192.168.2.100/32 */
                if (l3_nexthop.intf_id == l3_user_data->intf_id) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                                printk("%s: The nexthop for default route to WAN port is found!!\n", __func__);
                        }
                	memcpy(&(l3_user_data->mac_da[0]), &(l3_nexthop.da_mac[0]), 6);
			l3_user_data->found_mac = 1;
                }
        }
        return CA_E_OK;
}

static ca_status_t ca_kh_multi_wan_class_rule_add(ca_kh_port_binding_t *port_binding)
{
	int ret;
	ca_classifier_key_t key;
        ca_classifier_key_mask_t  key_mask;
        ca_classifier_action_t    action;

	if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: afi=%d, src_port_id=0x%x, dst_intf_id=%d, mac_da=%pM, vid=%d, class_index=%d\n", __func__,
			port_binding->afi, port_binding->src_port_id, port_binding->dst_intf_id, &port_binding->mac_da,
			port_binding->vid, port_binding->class_index);
	}
	memset(&key, 0, sizeof(ca_classifier_key_t));
	memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
	memset(&action, 0, sizeof(ca_classifier_action_t));

	key_mask.src_intf = 1;
	key_mask.src_port = 1;
	key_mask.l2 = 1;
//	key_mask.l2_mask.vlan_tag = 1;
//	key_mask.l2_mask.vlan_tag_mask.vid = 1;
//	key_mask.l2_mask.vlan_tag_mask.dei = 0;
//	key_mask.l2_mask.vlan_tag_mask.pri = 0;
//	key.l2.vlan_tag.vid = port_binding->vid;

	key_mask.ip = 1;
	key_mask.ip_mask.ip_version = 1;
	key.ip.ip_version = port_binding->afi;
	key.src_port = port_binding->src_port_id;
	action.dest.intf = port_binding->dst_intf_id;
	memcpy(&(action.options.mac_da[0]), &(port_binding->mac_da[0]), 6);

	if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: key_mask.src_intf=%d, key_mask.src_port=%d, key_mask.ip=%d, key_mask.ip_mask.ip_version=%d\n", __func__,
			key_mask.src_intf, key_mask.src_port, key_mask.ip, key_mask.ip_mask.ip_version);
//		printk("%s: key_mask.l2=%d, key_mask.l2_mask.vlan_tag=%d, key_mask.l2_mask.vlan_tag_mask.vid=%d\n", __func__,
//			key_mask.l2, key_mask.l2_mask.vlan_tag, key_mask.l2_mask.vlan_tag_mask.vid);
//		printk("%s: key_mask.l2_mask.vlan_tag_mask.pri=%d, key.l2.vlan_tag.vid=%d\n", __func__,
//			key_mask.l2_mask.vlan_tag_mask.pri, key.l2.vlan_tag.vid);
		printk("%s: key.src_port=0x%x, action.dest.intf=%d, action.options.mac_da=%pM\n", __func__,
			key.src_port, action.dest.intf, &action.options.mac_da);
	}

	ret = ca_classifier_rule_add(0, 0, &key, &key_mask, &action, &port_binding->class_index);
	if (ret != CA_E_OK) {
		printk("%s: ca_classifier_rule_add() failed, ret=%d!!\n", __func__, ret);
		printk("%s: afi=%d, src_port_id=0x%x, dst_intf_id=%d, mac_da=%pM\n", __func__,
			port_binding->afi, port_binding->src_port_id, port_binding->dst_intf_id, &port_binding->mac_da);
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: ca_classifier_rule_add() OK, class_index=%d\n", __func__, port_binding->class_index);
	}
	return CA_E_OK;
}

ca_status_t ca_kh_multi_wan_pppoe_classifier_add(ca_ip_address_t *ip_addr, ca_uint8_t *mac_da, ca_int16_t ifindex)
{
        int i;
        int ret;
	ca_intf_id_t    intf_id;
	ca_ip_address_t tmp_ip_addr;

        if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                printk("%s: mac_da=%pM\n", __func__, mac_da);
                printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d\n", __func__, ip_addr->afi, ip_addr->addr_len);
                if (ip_addr->afi == CA_IPV4) {
                        printk("%s: ip_addr=%pI4\n", __func__, &(ip_addr->ip_addr.ipv4_addr));
                }
                else {
                        printk("%s: ip_addr=%pI6\n", __func__, &(ip_addr->ip_addr.ipv6_addr));
                }
                printk("%s: ifindex=%d\n", __func__, ifindex);
        }

	/* update the MAC address in default gateway data */
	ca_kh_ip_address_ntohl(&tmp_ip_addr, ip_addr);
	ret = ca_int_l3_gw_upd(ifindex, &tmp_ip_addr, mac_da);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                	printk("%s: ca_int_l3_gw_upd(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
		}
		// don't care the result of ca_int_l3_gw_upd()
		// return ret;
        }

	ret = ca_kh_l3_get_intf_id_by_ifindex(0, ip_addr->afi, ifindex, CA_L3_INTF_BCAST, &intf_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
                return ret;
        }

        spin_lock_bh(&ca_kh_port_bind_lock);

        for (i = 0; i < MAX_PORT_BINDING; i++) {

		/* only add once if already got class_index do not add again */
		if (wan_port_binding[i].valid == 0 || wan_port_binding[i].class_index != 0 || wan_port_binding[i].connect_mode != TRSVC_WAN_CONN_TYPE_ROUTE) {
			continue;
		}

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
			printk("%s: i=%d, wan_port_binding[i].afi=%d, ip_addr->afi=%d, intf_id=%d, wan_port_binding[i].dst_intf_id=%d\n", __func__,
				i, wan_port_binding[i].afi, ip_addr->afi, intf_id, wan_port_binding[i].dst_intf_id);
		}

		/* if intf_id is not the same, skip */
		if ((wan_port_binding[i].afi == ip_addr->afi) && (intf_id == wan_port_binding[i].dst_intf_id)) {

 			/* save the DA MAC address to port binding array */
			memcpy(&(wan_port_binding[i].mac_da[0]), mac_da, 6);

			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                		printk("%s: port binding is found at index=%d!!\n", __func__, i);
			}
			ca_kh_multi_wan_class_rule_add(&(wan_port_binding[i]));
		}
        }

        spin_unlock_bh(&ca_kh_port_bind_lock);
        return CA_E_OK;
}

ca_status_t ca_kh_multi_wan_classifier_add(ca_ip_address_t *ip_addr, ca_uint8_t *mac_da)
{
	int i;
	int ret;
	ca_l3_intf_t l3_intf;
	//ca_l3_user_data_t l3_user_data;
	ca_int16_t ifindex;
	ca_int_l3_gw_t l3_gw;

        if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                printk("%s: mac_da=%pM\n", __func__, mac_da);
                printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d\n", __func__, ip_addr->afi, ip_addr->addr_len);
                if (ip_addr->afi == CA_IPV4) {
                        printk("%s: ip_addr=%pI4\n", __func__, &(ip_addr->ip_addr.ipv4_addr));
                }
                else {
                        printk("%s: ip_addr=%pI6\n", __func__, &(ip_addr->ip_addr.ipv6_addr));
                }
        }

	spin_lock_bh(&ca_kh_port_bind_lock);

	for (i = 0; i < MAX_PORT_BINDING; i++) {

		/* only add once if already got class_index do not add again */
		if (wan_port_binding[i].valid == 0 || wan_port_binding[i].class_index != 0 || wan_port_binding[i].connect_mode != TRSVC_WAN_CONN_TYPE_ROUTE) {
			continue;
		}

		ret = ca_kh_l3_get_ifindex_by_intf_id(0, &wan_port_binding[i].afi, wan_port_binding[i].dst_intf_id, &ifindex);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: index=%d, ca_kh_l3_get_ifindex_by_intf_id(afi=%d, intf_id=%d), return failed!!, ret=%d\n",
					__func__, i, wan_port_binding[i].afi, wan_port_binding[i].dst_intf_id, ret);
			}
			continue;
		}
		ret = ca_int_l3_gw_get(wan_port_binding[i].afi, ifindex, &l3_gw);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: index=%d, ca_int_l3_gw_get(afi=%d, ifindex=%d), return failed!!, ret=%d\n",
					__func__, i,  wan_port_binding[i].afi, ifindex, ret);
			}
			continue;
		}
		l3_intf.ip_addr.afi = wan_port_binding[i].afi;
		l3_intf.intf_id = wan_port_binding[i].dst_intf_id;

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
			printk("%s: index=%d, wan_port_binding[i].afi=%d, wan_port_binding[i].dst_intf_id=%d\n", __func__,
				i, wan_port_binding[i].afi, wan_port_binding[i].dst_intf_id);
		}
		ret = ca_l3_intf_get(0, &l3_intf);
		if (ret != CA_E_OK) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
			}
			continue;
		}
		/* the MAC address of default route not update yet */
		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
			printk("%s: l3_gw.l3_nexthop.da_mac=%pM\n", __func__, &l3_gw.l3_nexthop.da_mac);
		}
		if (memcmp(&(l3_intf.mac_addr[0]), &(l3_gw.l3_nexthop.da_mac[0]), 6) == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: MAC of default route=%pM, input MAC=%pM\n", __func__,
						&l3_gw.l3_nexthop.da_mac, mac_da);
			}
			continue;
		}

		/* save the DA MAC address to port binding array */
		memcpy(&(wan_port_binding[i].mac_da[0]), &(l3_gw.l3_nexthop.da_mac[0]), 6);

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
			printk("%s: default gateway for port binding is found at index=%d!!\n", __func__, i);
		}

		ca_kh_multi_wan_class_rule_add(&(wan_port_binding[i]));

#if 0
		l3_user_data.found_mac = 0;
		l3_user_data.ip_addr.afi = wan_port_binding[i].afi;
		l3_user_data.intf_id = wan_port_binding[i].dst_intf_id;
		memcpy(&(l3_user_data.mac_da), mac_da, 6);

		ca_l3_route_traverse(0, ip_addr->afi, ca_kh_l3_route_traverse_cb, &l3_user_data);

		if (l3_user_data.found_mac) {

			l3_intf.ip_addr.afi = wan_port_binding[i].afi;
			l3_intf.intf_id = wan_port_binding[i].dst_intf_id;
			ret = ca_l3_intf_get(0, &l3_intf);
			if (ret != CA_E_OK) {
				printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
				spin_unlock_bh(&ca_kh_port_bind_lock);
				return ret;
			}
			/* the MAC address of default route not update yet */
			if (memcmp(&(l3_intf.mac_addr[0]), &(l3_user_data.mac_da[0]), 6) == 0) {
				if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
					printk("%s: MAC of default route=%pM, input MAC=%pM\n", __func__,
							&l3_user_data.mac_da, mac_da);
				}
				continue;
			}

			/* save the DA MAC address to port binding array */
			memcpy(&(wan_port_binding[i].mac_da[0]), &(l3_user_data.mac_da[0]), 6);

			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: default gateway for port binding is found at index=%d!!\n", __func__, i);
			}

			ca_kh_multi_wan_class_rule_add(&(wan_port_binding[i]));
		}
#endif
	}

	spin_unlock_bh(&ca_kh_port_bind_lock);
	return CA_E_OK;
}

ca_status_t ca_kh_multi_wan_classifier_del(ca_ip_address_t *ip_addr, ca_uint8_t *mac_da)
{
        int i;
        int ret = CA_E_OK;
        ca_classifier_key_t key;
        ca_classifier_key_mask_t  key_mask;
        ca_classifier_action_t    action;
        ca_uint32_t priority;

        if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: mac_da=%pM\n", __func__, mac_da);
                printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d\n", __func__, ip_addr->afi, ip_addr->addr_len);
                if (ip_addr->afi == CA_IPV4) {
                        printk("%s: ip_addr=%pI4\n", __func__, &(ip_addr->ip_addr.ipv4_addr));
                }
                else {
                        printk("%s: ip_addr=%pI6\n", __func__, &(ip_addr->ip_addr.ipv6_addr));
                }
        }

	spin_lock_bh(&ca_kh_port_bind_lock);

        for (i = 0; i < MAX_PORT_BINDING; i++) {

                /* only add once if already got class_index do not add again */
                if (wan_port_binding[i].valid == 0 || wan_port_binding[i].class_index == 0) {
                        continue;
                }
		/* if the input MAC address does not match with the MAC address stored in wan_port_binding[] then do nothing */
		if (memcmp(&(wan_port_binding[i].mac_da[0]), mac_da, 6) != 0) {
			continue;
		}

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                       	printk("%s: default gateway for port binding is found at index=%d!!\n", __func__, i);
			printk("%s: wan_port_binding[i].class_index=%d, \n", __func__, wan_port_binding[i].class_index);
		}

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                       	ret = ca_classifier_rule_get(0, wan_port_binding[i].class_index, &priority, &key, &key_mask, &action);
                       	if (ret != CA_E_OK) {
                               	printk("%s: ca_classifier_rule_get() failed at index=%d, ret=%d!!\n", __func__, i, ret);
				break;
                       	}
                       	printk("%s: key_mask.ip_mask.ip_version=%d, key.src_port=0x%x, action.dest.intf=%d, action.options.mac_da=%pM\n",
                               	__func__, key_mask.ip_mask.ip_version, key.src_port, action.dest.intf, action.options.mac_da);
		}

		ret = ca_classifier_rule_delete(0, wan_port_binding[i].class_index);
                if (ret != CA_E_OK) {
                	printk("%s: ca_classifier_rule_delete(class_idex=%d) failed!!\n", __func__, wan_port_binding[i].class_index);
			break;
                }
		wan_port_binding[i].class_index = 0;

		if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
			printk("%s: ca_classifier_rule_del() OK at index=%d\n", __func__, i);
		}
        }

	spin_unlock_bh(&ca_kh_port_bind_lock);
        return ret;
}

ca_status_t ca_kh_port_binding_add(ca_device_id_t device_id, ca_ip_afi_t afi, ca_port_id_t src_port_id, ca_int16_t dst_ifindex, ca_uint16_t vid, ca_uint16_t connect_mode)
{
	int i;
	int ret;
	ca_intf_id_t    intf_id;

	//if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: afi=%d, src_port_id=0x%x, dst_ifindex=%d, vid=0x%x, connect_mode=%d\n",
			__func__, afi, src_port_id, dst_ifindex, vid, connect_mode);
	//}

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, afi, dst_ifindex, CA_L3_INTF_BCAST, &intf_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(dst_ifindex=%d), return failed!!, ret=%d\n", __func__, dst_ifindex, ret);
                return ret;
        }

	/* check whether existed or not */
	spin_lock_bh(&ca_kh_port_bind_lock);
	for (i = 0; i < MAX_PORT_BINDING; i++) {
                if (wan_port_binding[i].valid == 0) {
                        continue;
                }
                if (wan_port_binding[i].afi == afi && wan_port_binding[i].src_port_id == src_port_id &&
                        wan_port_binding[i].dst_intf_id == intf_id && wan_port_binding[i].vid == vid &&
			wan_port_binding[i].dst_intf_id == connect_mode) {
			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s:  afi=%d, src_port_id=0x%x, dst_ifindex=%d, vid=0x%x already exists!!\n", __func__, afi, src_port_id, dst_ifindex, vid);
			}
			spin_unlock_bh(&ca_kh_port_bind_lock);
			return CA_E_EXISTS;
		}
	}

	/* find an empty one entry */
	for (i = 0; i < MAX_PORT_BINDING; i++) {
                if (wan_port_binding[i].valid == 0) {
			break;
		}
        }
	if (i == MAX_PORT_BINDING) {
		printk("%s: No more port binding entry is available!!\n", __func__);
		spin_unlock_bh(&ca_kh_port_bind_lock);
		return CA_E_ERROR;
	}

	printk("%s: free entry found at index=%d\n", __func__, i);
	wan_port_binding[i].valid = 1;
	wan_port_binding[i].afi = afi;
	wan_port_binding[i].src_port_id = src_port_id;
	wan_port_binding[i].dst_intf_id = intf_id;
	wan_port_binding[i].vid = vid;
	wan_port_binding[i].connect_mode = connect_mode;

	spin_unlock_bh(&ca_kh_port_bind_lock);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_port_binding_add);

ca_status_t ca_kh_port_binding_del(ca_device_id_t device_id, ca_ip_afi_t afi, ca_port_id_t src_port_id, ca_int16_t dst_ifindex, ca_uint16_t vid, ca_uint16_t connect_mode)
{
	int i;
	int ret;
	ca_intf_id_t    intf_id;

	if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
                printk("%s: afi=%d, src_port_id=0x%x, dst_ifindex=%d, vid=0x%x\n", __func__, afi, src_port_id, dst_ifindex, vid);
	}

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, afi, dst_ifindex, CA_L3_INTF_BCAST, &intf_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(dst_ifindex=%d), return failed!!, ret=%d\n", __func__, dst_ifindex, ret);
                return ret;
        }

	spin_lock_bh(&ca_kh_port_bind_lock);
        for (i = 0; i < MAX_PORT_BINDING; i++) {
                if (wan_port_binding[i].valid == 0) {
                        continue;
                }
		if (wan_port_binding[i].afi == afi && wan_port_binding[i].src_port_id == src_port_id &&
			wan_port_binding[i].dst_intf_id == intf_id && wan_port_binding[i].vid == vid &&
			wan_port_binding[i].connect_mode == connect_mode) {

			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk("%s: delete the port binding at index=%d\n", __func__, i);
			}

			if (wan_port_binding[i].class_index != 0) {
				ret = ca_classifier_rule_delete(device_id, wan_port_binding[i].class_index);
       				if (ret != CA_E_OK) {
                			printk("%s: ca_classifier_rule_delete(class_idex=%d) failed!!\n", __func__, wan_port_binding[i].class_index);
					spin_unlock_bh(&ca_kh_port_bind_lock);
                			return ret;
				}
        		}
			memset(&(wan_port_binding[i]), 0, sizeof(ca_kh_port_binding_t));
			spin_unlock_bh(&ca_kh_port_bind_lock);
			return CA_E_OK;
		}
        }
	spin_unlock_bh(&ca_kh_port_bind_lock);

        return CA_E_ERROR;

}
EXPORT_SYMBOL(ca_kh_port_binding_del);

ca_status_t ca_kh_port_binding_check(ca_device_id_t device_id, struct sk_buff *skb)
{
	int i;
	int ret = CA_E_OK; /* default: accept packet */
	ca_port_id_t src_port_id;
	char *device_name = skb->dev->name;

	// TODO
	// src_port_id = ca_eth_dev_to_portid(0, device_name);
	if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
		printk("%s: device_name=%s\n", __func__, device_name);
		printk("%s: src_port_id=0x%x\n", __func__, src_port_id);
	}

	/* check whether existed or not */
	spin_lock_bh(&ca_kh_port_bind_lock);
	for (i = 0; i < MAX_PORT_BINDING; i++) {
		if (wan_port_binding[i].valid == 0) {
			continue;
		}

		if (wan_port_binding[i].connect_mode != TRSVC_WAN_CONN_TYPE_NONE && wan_port_binding[i].src_port_id == src_port_id) {

			if (wan_port_binding[i].vid == 0 && skb->vid == 0xffff) {
				ret = CA_E_OK;
			} else if (wan_port_binding[i].vid != 0 && wan_port_binding[i].vid == skb->vid) {
				ret = CA_E_OK;
			} else {
				ret = CA_E_EXISTS;
			}

			if (ca_kh_debug & CA_KH_DEBUG_PORT_BINDING) {
				printk(KERN_INFO "%s: Binded. src_port_id=0x%x, binded VID %d, skb->vid=%d, ret=%d\n",
					__func__, src_port_id, wan_port_binding[i].vid, skb->vid, ret);
			}

                        spin_unlock_bh(&ca_kh_port_bind_lock);
                        return ret;
		}
	}
	spin_unlock_bh(&ca_kh_port_bind_lock);
	return ret;
}

ca_status_t ca_kh_port_binding_dump(void)
{
	int i;

	spin_lock_bh(&ca_kh_port_bind_lock);
	printk("%s: =================== Port Binding table ====================== \n", __func__);
	for (i = 0; i < MAX_PORT_BINDING; i++) {
		if (wan_port_binding[i].valid == 0) {
			continue;
		}
		printk("%s: <%d> afi=%d, src_port_id=0x%x, dst_intf_id=%d, vid=%d, connect_mode=%d\n",
			__func__, i, wan_port_binding[i].afi, wan_port_binding[i].src_port_id,
			wan_port_binding[i].dst_intf_id, wan_port_binding[i].vid, wan_port_binding[i].connect_mode);
	}
	printk("%s: =================== END of Port Binding table ====================== \n", __func__);
	spin_unlock_bh(&ca_kh_port_bind_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_port_binding_dump);

ca_status_t ca_kh_port_binding_init(void)
{
	memset(&(wan_port_binding[0]), 0, sizeof(wan_port_binding));
	return CA_E_OK;
}
