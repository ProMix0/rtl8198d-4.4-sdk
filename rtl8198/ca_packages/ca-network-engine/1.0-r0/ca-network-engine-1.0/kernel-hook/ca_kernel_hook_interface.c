
#include <linux/if_vlan.h>
#include <linux/netdevice.h>
#include <linux/in.h>

#include <soc/cortina/ca_kernel_hook.h>

#include "eth_port.h"

#include "ca_kernel_hook_util.h"

typedef struct ca_l3_user_data {
        ca_intf_id_t    intf_id;
	ca_ip_address_t ip_addr;
} ca_l3_user_data_t;

/* export function in NI driver */
ca_status_t ca_ni_intf_get_ports(struct net_device *dev, ca_uint8_t *num_ports, int *ports, ca_uint8_t *is_wan_intf);

ca_status_t ca_kh_l3_get_ifindex_by_intf_id(ca_device_id_t device_id, ca_ip_afi_t *afi, ca_intf_id_t intf_id, ca_int16_t *ifindex)
{
	/* interface id start from 0 */
	if (intf_id < 0 || intf_id > CA_L3_IP_MAX_INTF) {
		printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
		return CA_E_ERROR;
	}

	spin_lock_bh(&ca_kh_intf_lock);
	*ifindex = ca_kh_l3_ifindex_array[intf_id].ifindex;
	*afi = ca_kh_l3_ifindex_array[intf_id].afi;
	spin_unlock_bh(&ca_kh_intf_lock);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l3_get_ifindex_by_intf_id);

ca_status_t ca_kh_l3_tunnel_get_ifindex_by_intf_id(ca_device_id_t device_id, ca_ip_afi_t *afi, ca_intf_id_t intf_id, ca_int16_t *ifindex,
		ca_tunnel_type_t tunnel_type)
{
	if ((tunnel_type == CA_TUNNEL_DSLITE) || (tunnel_type == CA_TUNNEL_MAP_E)) {
		if (intf_id < CA_DSLITE_INTF_MIN || intf_id > CA_DSLITE_INTF_MAX) {
			printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
			return CA_E_ERROR;
		}
	}
	else if (tunnel_type == CA_TUNNEL_6RD) {
		if (intf_id < CA_6RD_INTF_MIN || intf_id > CA_6RD_INTF_MAX) {
			printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
			return CA_E_ERROR;
		}
	}
	else {
		printk("%s: tunnel_type=%d is not a valid type!!\n", __func__, tunnel_type);
	}

        spin_lock_bh(&ca_kh_intf_lock);
        *ifindex = ca_kh_l3_ifindex_array[intf_id].ifindex;
        *afi = ca_kh_l3_ifindex_array[intf_id].afi;
        spin_unlock_bh(&ca_kh_intf_lock);

        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l3_tunnel_get_ifindex_by_intf_id);

static ca_status_t get_intf_id_by_ifindex(ca_ip_afi_t afi, ca_int16_t ifindex, ca_l3_intf_type_t intf_type, ca_intf_id_t *intf_id,
	ca_uint32_t min_intf_id, ca_uint32_t max_intf_id)
{
	int i;
	int empty_index;
	ca_uint32_t tmp_ifindex;
	ca_ip_afi_t tmp_afi;
	ca_l3_intf_type_t tmp_intf_type;
	ca_tunnel_type_t tmp_tunnel_type;

	empty_index = -1;
	for (i = min_intf_id; i < max_intf_id; i++) {

		/* there is only one interface table for IPV4/IPV6 */
		spin_lock_bh(&ca_kh_intf_lock);
		tmp_ifindex = ca_kh_l3_ifindex_array[i].ifindex;
		tmp_afi = ca_kh_l3_ifindex_array[i].afi;
		tmp_intf_type = ca_kh_l3_ifindex_array[i].intf_type;
		tmp_tunnel_type = ca_kh_l3_ifindex_array[i].tunnel_type;
		spin_unlock_bh(&ca_kh_intf_lock);

		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
			printk("%s: i=%d, afi=%d, tmp_afi=%d, ifindex=%d, tmp_ifindex=%d, intf_type=%d, tmp_intf_type=%d, tmp_tunnel_type=%d\n",
				__func__, i, afi, tmp_afi, ifindex, tmp_ifindex, intf_type, tmp_intf_type, tmp_tunnel_type);
		}

		if ( (intf_type == CA_L3_INTF_TUNNEL) &&
		     ((tmp_tunnel_type == CA_TUNNEL_DSLITE) || (tmp_tunnel_type == CA_TUNNEL_6RD) || (tmp_tunnel_type == CA_TUNNEL_MAP_E)) &&
		     (ifindex == tmp_ifindex) &&
		     (intf_type == tmp_intf_type) ) {

			/* if ca_kh_l3_ifindex_array[] is DSLite/6RD, skip checking afi */
			/* TODO: DSLite/6RD multicast interface */
			*intf_id = i;
			if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
				printk("%s: Found *intf_id=%d\n", __func__, *intf_id);
			}
			return CA_E_OK;

		} else if ((afi == tmp_afi) && (ifindex == tmp_ifindex) && (intf_type == tmp_intf_type)) {

			*intf_id = i;
			if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
				printk("%s: Found *intf_id=%d\n", __func__, *intf_id);
			}
			return CA_E_OK;

		}

		/* find an emtry entry for new ifindex */
		if (tmp_ifindex == 0 && empty_index == -1)
			empty_index = i;
	}
	if (empty_index == -1) {
		return  CA_E_ERROR;
	} else {
		*intf_id = empty_index;
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
			printk("%s: Not found ifindex=%d, intf_id=%d\n", __func__, ifindex, *intf_id);
		}
		return CA_E_NOT_FOUND;
	}
}

ca_status_t ca_kh_l3_get_intf_id_by_ifindex(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int16_t ifindex, ca_l3_intf_type_t intf_type, ca_intf_id_t *intf_id)
{
	/* interface id start from 0 */
	return get_intf_id_by_ifindex(afi, ifindex, intf_type, intf_id, 0, CA_L3_IP_MAX_INTF);
}
EXPORT_SYMBOL(ca_kh_l3_get_intf_id_by_ifindex);

ca_status_t ca_kh_l3_tunnel_get_intf_id_by_ifindex(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int16_t ifindex, ca_intf_id_t *intf_id,
		ca_tunnel_type_t tunnel_type)
{
	if ((tunnel_type == CA_TUNNEL_DSLITE) || (tunnel_type == CA_TUNNEL_MAP_E)) {
		return get_intf_id_by_ifindex(afi, ifindex, CA_L3_INTF_TUNNEL, intf_id, CA_DSLITE_INTF_MIN, CA_DSLITE_INTF_MAX);
	}
	else if (tunnel_type == CA_TUNNEL_6RD) {
		return get_intf_id_by_ifindex(afi, ifindex, CA_L3_INTF_TUNNEL, intf_id, CA_6RD_INTF_MIN, CA_6RD_INTF_MAX);
	}
	else {
		return CA_E_ERROR;
	}
}
EXPORT_SYMBOL(ca_kh_l3_tunnel_get_intf_id_by_ifindex);

ca_status_t ca_kh_l3_set_intf_id(ca_device_id_t device_id, ca_intf_id_t intf_id, ca_kh_ifindex_entry_t *entry)
{
	if (entry == NULL) return CA_E_PARAM;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: intf_id=%d, afi=%d, ifindex=%d, intf_type=%d, tunnel_type=%d, is_mcast_intf=%d\n",
			__func__, intf_id, entry->afi, entry->ifindex, entry->intf_type, entry->tunnel_type, entry->is_mcast_intf);
	}

	/* interface id start from 0 */
	if (intf_id < 0 || intf_id > CA_L3_IP_MAX_INTF) {
		printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
		return CA_E_ERROR;
	}

	spin_lock_bh(&ca_kh_intf_lock);
	memcpy(&ca_kh_l3_ifindex_array[intf_id], entry, sizeof(ca_kh_ifindex_entry_t));
	spin_unlock_bh(&ca_kh_intf_lock);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l3_set_intf_id);

ca_status_t ca_kh_l3_tunnel_set_intf_id(ca_device_id_t device_id, ca_ip_afi_t afi, ca_int16_t ifindex, ca_intf_id_t intf_id,
		ca_tunnel_type_t tunnel_type)
{
	if ((tunnel_type == CA_TUNNEL_DSLITE) || (tunnel_type == CA_TUNNEL_MAP_E)) {
		if (intf_id < CA_DSLITE_INTF_MIN || intf_id > CA_DSLITE_INTF_MAX) {
			printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
			return CA_E_ERROR;
		}
	}
	else if (tunnel_type == CA_TUNNEL_6RD) {
		if (intf_id < CA_6RD_INTF_MIN || intf_id > CA_6RD_INTF_MAX) {
			printk("%s: intf_id=%d is not a valid id!!\n", __func__, intf_id);
			return CA_E_ERROR;
		}
	}
	else if ((tunnel_type <= CA_TUNNEL_MIN) || (tunnel_type >= CA_TUNNEL_MAX)) {
		printk("%s: tunnel_type=%d is not a valid type!!\n", __func__, tunnel_type);
		return CA_E_ERROR;
	}

        spin_lock_bh(&ca_kh_intf_lock);
        ca_kh_l3_ifindex_array[intf_id].ifindex = ifindex;
        ca_kh_l3_ifindex_array[intf_id].afi = afi;
	ca_kh_l3_ifindex_array[intf_id].intf_type = CA_L3_INTF_TUNNEL;
	ca_kh_l3_ifindex_array[intf_id].tunnel_type = tunnel_type;
	ca_kh_l3_ifindex_array[intf_id].is_mcast_intf = 0; // TODO
        spin_unlock_bh(&ca_kh_intf_lock);
        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l3_tunnel_set_intf_id);

static ca_status_t add_a_l3_intf(ca_ip_afi_t afi, int ifindex, ca_l3_intf_t *l3_intf)
{
	int ret;
#if 0
	ca_intf_id_t intf_id;
#endif
	ca_kh_ifindex_entry_t kh_ifindex_entry;

	ret = ca_l3_intf_add(0, l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_add(), return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: ca_l3_intf_add(intf_id=%d) OK\n", __func__, l3_intf->intf_id);
	}

	memset(&kh_ifindex_entry, 0, sizeof(kh_ifindex_entry));
	kh_ifindex_entry.afi = afi;
	kh_ifindex_entry.ifindex = ifindex;
	kh_ifindex_entry.intf_type = l3_intf->type;
	ca_kh_l3_set_intf_id(0, l3_intf->intf_id, &kh_ifindex_entry);

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: set ifindex=%d, intf_id=%d for CA_L3_INTF_BCAST\n", __func__, ifindex, l3_intf->intf_id);
	}
#if 0
	/* create a CA_L3_INTF_CPU type interface for receiving packet to CPU */
	l3_intf->type = CA_L3_INTF_CPU;
	ca_kh_l3_get_intf_id_by_ifindex(0, afi, ifindex, l3_intf->type, &intf_id);
	l3_intf->intf_id = intf_id;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: add a CA_L3_INTF_CPU type interface, intf_id=%d\n", __func__, intf_id);
	}

	if (afi == CA_IPV4) {
		l3_intf->ip_addr.addr_len = 32;
	}
	ret = ca_l3_intf_add(0, l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_add(), intferace type CA_L3_INTF_CPU, return failed!!, ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: ca_l3_intf_add(intf_id=%d) OK\n", __func__, l3_intf->intf_id);
	}

	kh_ifindex_entry.intf_type = l3_intf->type;
	ca_kh_l3_set_intf_id(0, l3_intf->intf_id, &kh_ifindex_entry);

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: set ifindex=%d, intf_id=%d for CA_L3_INTF_CPU\n", __func__, ifindex, l3_intf->intf_id);
	}
#endif
	return CA_E_OK;
}

ca_status_t ca_kh_l3_intf_add(ca_device_id_t device_id, struct net_device *dev, ca_ip_address_t *ip_addr, ca_port_id_t port_id, ca_tunnel_id_t tunnel_id)
{
	struct net_device *parent_dev1 = NULL, *parent_dev2 = NULL; /* only support double-tag vlan */
	struct net_device *real_dev = NULL;
	int ret;
	ca_l3_intf_t l3_intf;
	ca_l3_intf_t tmp_l3_intf;
	ca_intf_id_t intf_id;
	char dev_name[IFNAMSIZ];

	bool new_one;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: dev->name=%s, dev->ifindex=%d, dev->mtu=%d\n", __func__, dev->name, dev->ifindex, dev->mtu);

		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv4_addr=%pI4\n", __func__,
				ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv4_addr);
		}
		else {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv6_addr=%pI6\n", __func__,
                                ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv6_addr);
		}

		if (!(dev->flags & IFF_LOOPBACK)) {

			if (dev->priv_flags & IFF_802_1Q_VLAN)  {
				printk("%s: port_id=%d, dev->dev_addr=%pM, vid=%d, proto=0x%04x, tunnel_id=%d\n", __func__,
					port_id, dev->dev_addr, vlan_dev_vlan_id(dev), ntohs(vlan_dev_vlan_proto(dev)), tunnel_id);
			}
			else {
				printk("%s: port_id=%d, dev->dev_addr=%pM, tunnel_id=%d\n", __func__,
				port_id, dev->dev_addr, tunnel_id);
			}
		}
	}

	if (strcmp(dev->name, "br-mc") == 0) {
		printk("%s: device name is br-mc, do not need add to interface table!!\n", __func__);
		return CA_E_OK;
	}

	memset(&l3_intf, 0, sizeof(l3_intf));

	/* loopback device */
	if (dev->flags & IFF_LOOPBACK)
		l3_intf.type = CA_L3_INTF_CPU;
	else if (dev->flags & IFF_POINTOPOINT)
		l3_intf.type = CA_L3_INTF_TUNNEL;
	else
		l3_intf.type = CA_L3_INTF_BCAST;

	ret = ca_kh_l3_get_intf_id_by_ifindex(0, ip_addr->afi, dev->ifindex, l3_intf.type, &intf_id);
	if (ret == CA_E_ERROR) {
		printk("%s: interface table is full!!\n", __func__);
		return CA_E_FULL;
	}

	if (ret == CA_E_NOT_FOUND)
		new_one = true;
	else
		new_one = false;

	if (dev->flags & IFF_LOOPBACK) {
		return CA_E_NOT_SUPPORT;
		//l3_intf.port_id = ((ca_uint32_t)CA_PORT_TYPE_CPU << CA_PORT_TYPE_SHIFT) | 0;
		//l3_intf.mask.port_id = 1;
	}
	else if (dev->flags & IFF_POINTOPOINT) {
		/* tunnel type interface has parent interface which should have a port_id, but tunnel type interface
		   no need to have a port_id */
	}
	else {
		memcpy(dev_name, dev->name, 4);
		dev_name[4] = 0;

		l3_intf.mask.port_id = 1;
		if (dev->priv_flags & IFF_EBRIDGE) {
			/* In G3, for the bridge interface, it will always
			 * forward packets to L2FE for mac lookup
			 */
			l3_intf.port_id = ((ca_uint32_t)CA_PORT_TYPE_L2RP << CA_PORT_TYPE_SHIFT) | 0x19;
		}
		else {
			ca_uint8_t num_ports = CA_ETH_PORT_NUM;
			int ports[CA_ETH_PORT_NUM];
			ca_uint8_t is_wan_intf = 0;

			if (is_vlan_dev(dev)) {
				real_dev = vlan_dev_real_dev(dev);
				parent_dev1 = vlan_dev_priv(dev)->real_dev;

				if (is_vlan_dev(parent_dev1)) {
					parent_dev2 = vlan_dev_priv(parent_dev1)->real_dev;

					if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
						printk("%s: outer_vid=%u, outer_tpid=0x%04x\n", __func__, vlan_dev_vlan_id(parent_dev1), ntohs(vlan_dev_vlan_proto(parent_dev1)));
						printk("%s: inner_vid=%u, inner_tpid=0x%04x\n", __func__, vlan_dev_vlan_id(dev), ntohs(vlan_dev_vlan_proto(dev)));
					}

					if (parent_dev2 != real_dev)
						printk("%s: WARNING! Do not support three or more VLAN tags.\n", __func__);
				} else {

					if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
						printk("%s: outer_vid=%u, outer_tpid=0x%04x\n", __func__, vlan_dev_vlan_id(dev), ntohs(vlan_dev_vlan_proto(dev)));
					}

				}

			} else {
				real_dev = dev;
			}

			if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
				printk("%s: real_dev->name=%s, ifindex=%d\n", __func__, real_dev->name, real_dev->ifindex);
			}

			/* call NI driver to determine WAN interface or LAN interface */
			ret = ca_ni_intf_get_ports(real_dev, &num_ports, ports, &is_wan_intf);
			if (ret != CA_E_OK) {
				printk("%s: Fail to get port info of dev '%s'. (ret %d)\n", __func__, real_dev->name, ret);
				return CA_E_NOT_FOUND;
			}

			if (is_wan_intf) {
				l3_intf.port_id = ((ca_uint32_t)CA_PORT_TYPE_L3 << CA_PORT_TYPE_SHIFT) | 0x18;
			}
			else {
				l3_intf.port_id = ((ca_uint32_t)CA_PORT_TYPE_L3 << CA_PORT_TYPE_SHIFT) | 0x19;
			}
			//l3_intf.port_id = ca_eth_dev_to_portid(device_id, dev_name);
		}
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: l3_intf.port_id=0x%x\n", __func__, l3_intf.port_id);
        	}
	}

	if (dev->flags & IFF_LOOPBACK) {
		memset(&(l3_intf.mac_addr[0]), 0, 6);
	} else {
		memcpy(&(l3_intf.mac_addr[0]), dev->dev_addr, 6);
		l3_intf.mask.mac_addr = 1;
	}

	if (dev->flags & IFF_LOOPBACK) {
		l3_intf.outer_vid = CA_UINT16_INVALID;
		l3_intf.tunnel_id = 0;
	}
	else {
		/* if p2p interface do not change the vid */
		if (!(dev->flags & IFF_POINTOPOINT)) {
			if (is_vlan_dev(dev)) {
				l3_intf.outer_vid = vlan_dev_vlan_id(dev);
				l3_intf.outer_tpid = ntohs(vlan_dev_vlan_proto(dev));
				l3_intf.mask.outer_tpid = 1;
			} else {
				l3_intf.outer_vid = CA_UINT16_INVALID;
			}

			if ((parent_dev1 != NULL) && (is_vlan_dev(parent_dev1))) {
				/* its double-tag vlan, move l3_intf.outer_xxx to l3_intf.inner_xxx */
				l3_intf.inner_vid = l3_intf.outer_vid;
				l3_intf.mask.inner_vid = 1;
				l3_intf.inner_tpid = l3_intf.outer_tpid;
				l3_intf.mask.inner_tpid = 1;

				/* set correct l3_intf.outer_xxx from parent_dev1 */
				l3_intf.outer_vid = vlan_dev_vlan_id(parent_dev1);
				l3_intf.outer_tpid = ntohs(vlan_dev_vlan_proto(parent_dev1));
				l3_intf.mask.outer_tpid = 1;
			} else {
				l3_intf.inner_vid = CA_UINT16_INVALID;
			}

			l3_intf.tunnel_id = tunnel_id;
			if (l3_intf.type == CA_L3_INTF_TUNNEL)
				l3_intf.mask.tunnel_id = 1;
		}
		l3_intf.mask.outer_vid = 1; /* must be 1 no matter untag or tagged; otherwise it means a interface that "don't care vid" */
	}

	l3_intf.mtu = dev->mtu;
	l3_intf.mask.mtu = 1;
	if (l3_intf.type != CA_L3_INTF_BCAST)
		ca_kh_ip_address_ntohl(&l3_intf.ip_addr, ip_addr);
	l3_intf.ip_addr.afi = ip_addr->afi;
	l3_intf.mask.ip_addr = 1;

	if (new_one) {
		l3_intf.intf_id = intf_id;

		ca_kh_decide_nat_enable(&l3_intf);

		ret = add_a_l3_intf(ip_addr->afi, dev->ifindex, &l3_intf);
		if (ret != CA_E_OK) {
			return ret;
		}
	}
	else {
		/* remove the NAT entries of old ip address */
		//if (ip_addr->afi == CA_IPV4) {
		//tmp_l3_intf.ip_addr.afi = ip_addr->afi;
		tmp_l3_intf.intf_id = intf_id;

		ret = ca_l3_intf_get(0, &tmp_l3_intf);
		if (ret != CA_E_OK) {
			printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, intf_id, ret);
			return ret;
		}

		///TODO:
		//ca_kh_nat_entry_ifdown(device_id, &(tmp_l3_intf.ip_addr));

		ret = ca_kh_l3_intf_delete(0, ip_addr->afi, dev);
		if (ret != CA_E_OK) {
			if (ret != CA_E_NOT_SUPPORT)
				printk("%s: ca_kh_l3_intf_delete(dev->ifindex=%d), return failed!!, ret=%d\n", __func__, dev->ifindex, ret);
			return ret;
		}

		if (tmp_l3_intf.type == CA_L3_INTF_TUNNEL) {
			if (memcmp(l3_intf.mac_addr, "\0\0\0\0\0\0", 6) == 0) {
				memcpy(l3_intf.mac_addr, tmp_l3_intf.mac_addr, 6);
				l3_intf.mask.mac_addr = 1;
			}
			l3_intf.tunnel_id = tmp_l3_intf.tunnel_id;
			l3_intf.mask.tunnel_id = 1;
		}
		l3_intf.nat_enable = tmp_l3_intf.nat_enable;
		l3_intf.mask.nat_enable = tmp_l3_intf.mask.nat_enable;
		l3_intf.intf_id = intf_id;

		ret = add_a_l3_intf(ip_addr->afi, dev->ifindex, &l3_intf);
		if (ret != CA_E_OK) {
			return ret;
		}
	}

	/* because when dslite tunnel add the local address maybe not assigned yet */
	ca_kh_ip_address_ntohl(ip_addr, ip_addr);
	//ca_kh_tunnel_update_parent_intf_id(device_id, ip_addr, l3_intf.intf_id);

	/* support "ip link add link eth0 name eth0.100 type vlan id 100" */
	ca_kh_l2_vlan_add(device_id, dev, vlan_dev_vlan_id(dev));

	return ret;
}

ca_status_t ca_kh_l3_intf_update_ip_addr(ca_device_id_t device_id, int ifindex, ca_ip_address_t *ip_addr)
{
	return CA_E_OK;
}

ca_status_t ca_kh_l3_intf_update_mtu(ca_device_id_t device_id, int ifindex, int mtu)
{
        int ret;
        ca_l3_intf_t l3_intf;
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
        	printk("%s: ifindex=%d, mtu=%d\n", __func__, ifindex, mtu);
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get device from ifindex %d.\n", __func__, ifindex);
		return CA_E_ERROR;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	/* update IPV4 interface */
        l3_intf.ip_addr.afi = CA_IPV4;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
		/* if get intf_id failed means no ipv4 address assigned be quiet */
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		}
               // return ret;
        }
	else {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        	}

        	ret = ca_l3_intf_get(0, &l3_intf);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                	return ret;
        	}
        	l3_intf.mtu = mtu;

        	ret = ca_l3_intf_update(0, &l3_intf);

        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_update(), return failed!!, ret=%d\n", __func__, ret);
        		return ret;
        	}
	}

	/* update IPV6 interface */
	l3_intf.ip_addr.afi = CA_IPV6;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
		/* if get intf_id failed means no ipv4 address assigned be quiet */
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		}
                //return ret;
        }
	else {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        	}

        	ret = ca_l3_intf_get(0, &l3_intf);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                	return ret;
        	}
        	l3_intf.mtu = mtu;

        	ret = ca_l3_intf_update(0, &l3_intf);

        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_update(), return failed!!, ret=%d\n", __func__, ret);
			return ret;
        	}
	}
        return ret;
}

ca_status_t ca_kh_l3_intf_update_mac_addr(ca_device_id_t device_id, int ifindex, u8 *mac_addr, int addr_len)
{
        int ret;
        ca_l3_intf_t l3_intf;
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
        	printk("%s: ifindex=%d, mac_addr=%pM, addr_len=%d\n", __func__, ifindex, mac_addr, addr_len);
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get device from ifindex %d.\n", __func__, ifindex);
		return CA_E_ERROR;
	}
	dev_flags = dev->flags;
	dev_put(dev);

        /* update IPV4 interface */
        l3_intf.ip_addr.afi = CA_IPV4;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		}
                //return ret;
        }
	else {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        	}

        	ret = ca_l3_intf_get(0, &l3_intf);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                	return ret;
        	}
       		memcpy(&(l3_intf.mac_addr[0]), &(mac_addr[0]), addr_len);

        	ret = ca_l3_intf_update(0, &l3_intf);

        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_update(), return failed!!, ret=%d\n", __func__, ret);
        		return ret;
        	}
	}

        /* update IPV6 interface */
        l3_intf.ip_addr.afi = CA_IPV6;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
		}
                //return ret;
        }
	else {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                	printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        	}

        	ret = ca_l3_intf_get(0, &l3_intf);
        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                	return ret;
        	}
        	memcpy(&(l3_intf.mac_addr[0]), &(mac_addr[0]), addr_len);

        	ret = ca_l3_intf_update(0, &l3_intf);

        	if (ret != CA_E_OK) {
                	printk("%s: ca_l3_intf_update(), return failed!!, ret=%d\n", __func__, ret);
			return ret;
        	}
	}
        return ret;
}

ca_status_t ca_kh_l3_intf_update_nat_enable(ca_device_id_t device_id, int ifindex, int nat_enable)
{
        int ret;
        ca_l3_intf_t l3_intf;
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;

        if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                printk("%s: ifindex=%d,nat_enable =%d\n", __func__, ifindex, nat_enable);
        }

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get device from ifindex %d.\n", __func__, ifindex);
		return CA_E_ERROR;
	}
	dev_flags = dev->flags;
	dev_put(dev);

        /* update IPV4 interface */
        l3_intf.ip_addr.afi = CA_IPV4;

	if (dev_flags & IFF_BROADCAST)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_BCAST, &(l3_intf.intf_id));
	else if (dev_flags & IFF_POINTOPOINT)
		ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &(l3_intf.intf_id));
        if (ret != CA_E_OK) {
                printk("%s: ca_kh_l3_get_intf_id_by_ifindex(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }

        if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
                printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
        }

        ret = ca_l3_intf_get(0, &l3_intf);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_get(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }
        l3_intf.nat_enable = nat_enable;

        ret = ca_l3_intf_update(0, &l3_intf);

        if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_update(), return failed!!, ret=%d\n", __func__, ret);
                return ret;
        }

        return ret;
}

ca_status_t ca_kh_l3_intf_delete(ca_device_id_t device_id, ca_ip_afi_t afi, struct net_device *dev)
{
        int ret;
	int ifindex = dev->ifindex;
        ca_l3_intf_t l3_intf;
	ca_kh_ifindex_entry_t kh_ifindex_entry;

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
        	printk("%s: afi=%d, ifindex=%d\n", __func__, afi, ifindex);
	}

	/* Non-VLAN interface shouldn't be deleted such as eth0, eth1.
	 * P2P type interface is often tunnel interface, they have their own handling flow and functions.
	 */
	if ((is_vlan_dev(dev) == 0) || (dev->flags & IFF_POINTOPOINT)) {
		if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
			printk("%s: dev->name=%s is not allowed to be deleted here!!\n", __func__, dev->name);
		}
		return CA_E_NOT_SUPPORT;
	}

        memset(&l3_intf, 0, sizeof(ca_l3_intf_t));

	ret = ca_kh_l3_get_intf_id_by_ifindex(0, afi, ifindex, CA_L3_INTF_BCAST, &l3_intf.intf_id);
	if (ret == CA_E_ERROR) {
		printk("%s: No intf_id found by ifindex=%d\n", __func__, ifindex);
		return CA_E_ERROR;
	}

	l3_intf.ip_addr.afi = afi;
	ret = ca_l3_intf_delete(0, l3_intf.intf_id);
	if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_delete(l3_intf.intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
		return ret;
        }

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: ca_l3_intf_delete(l3_intf.intf_id=%d), return OK, ret=%d\n", __func__, l3_intf.intf_id, ret);
	}

	memset(&kh_ifindex_entry, 0, sizeof(kh_ifindex_entry));
	kh_ifindex_entry.afi = afi;
	kh_ifindex_entry.ifindex = 0;
	kh_ifindex_entry.intf_type = CA_L3_INTF_BCAST;
	ca_kh_l3_set_intf_id(0, l3_intf.intf_id, &kh_ifindex_entry);

	/* delete CA_L3_INTF_CPU type interface */
	/* find the same ifindex in array */
#if 0
        if (ca_kh_l3_get_intf_id_by_ifindex(0, afi, ifindex, CA_L3_INTF_CPU, &l3_intf.intf_id) == CA_E_ERROR) {
                printk("%s: No CA_L3_INTF_CPU type intf_id found by ifindex=%d\n", __func__, ifindex);
                return CA_E_ERROR;
        }

        l3_intf.ip_addr.afi = afi;
        ret = ca_l3_intf_delete(0, l3_intf.intf_id);
        if (ret != CA_E_OK) {
                printk("%s: ca_l3_intf_delete(l3_intf.intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
                return ret;
        }

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
        	printk("%s: ca_l3_intf_delete(l3_intf.intf_id=%d), return OK, ret=%d\n", __func__, l3_intf.intf_id, ret);
	}

	kh_ifindex_entry.intf_type = CA_L3_INTF_CPU;
	ca_kh_l3_set_intf_id(0, l3_intf.intf_id, &kh_ifindex_entry);
#endif
        /* support "ip link delete eth0.100" */
        ca_kh_l2_vlan_delete(device_id, dev);

        return ret;
}

/* TODO: This function can't work with G3 Cortina API because BCAST interface no longer has IP addr */
#if 0
ca_status_t ca_kh_l3_intf_get_intf_id_by_ip_addr(ca_device_id_t device_id, ca_l3_ip_addr_t *l3_ip_addr, ca_intf_id_t *got_intf_id)
{
	int ret;
	ca_uint32_t	intf_id;
        ca_l3_intf_t l3_intf;
	ca_ip_address_t ip_addr;

	*got_intf_id = 0;
	ip_addr.afi = CA_IPV6;
	ip_addr.addr_len = 128;
	ip_addr.ip_addr = *l3_ip_addr;

	ca_kh_ip_address_ntohl(&ip_addr, &ip_addr);

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
        	printk("%s: ip_addr.afi=%d, ip_addr.addr_len=%d, ip_addr.ip_addr.ipv6_addr=%pI6\n", __func__,
                	ip_addr.afi, ip_addr.addr_len, &ip_addr.ip_addr.ipv6_addr);
        }

	for (intf_id = 1; intf_id <= CA_L3_IP_MAX_INTF; intf_id++) {
       		l3_intf.ip_addr.afi = CA_IPV6;
       		l3_intf.intf_id = intf_id;
        	ret = ca_l3_intf_get(0, &l3_intf);
		if (ret == CA_E_OK) {
			l3_intf.ip_addr.addr_len = 128;
			if (ca_kh_is_ip_equal(&ip_addr, &l3_intf.ip_addr)) {
				*got_intf_id = l3_intf.intf_id;
				if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
					printk("%s: got interface id = %d\n", __func__, *got_intf_id);
				}
				return CA_E_OK;
			}
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: Can not got interface id!!\n", __func__);
        }
	return CA_E_ERROR;
}
#endif

ca_status_t ca_kh_l3_intf_set_mtu(ca_device_id_t device_id, ca_int16_t ifindex, unsigned int dev_flags, int mtu)
{
	ca_l3_intf_t l3_intf;
	ca_l3_intf_mask_t l3_intf_mask;
	ca_status_t ret;

	if (dev_flags & IFF_BROADCAST)
		l3_intf.type = CA_L3_INTF_BCAST;
	else if (dev_flags & IFF_POINTOPOINT)
		l3_intf.type = CA_L3_INTF_TUNNEL;
	else
		l3_intf.type = CA_L3_INTF_BCAST;

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, l3_intf.type, &(l3_intf.intf_id));
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_get_intf_id_by_ifindex(ifindex=%d), return failed!!, ret=%d\n", __func__, ifindex, ret);
		return ret;
	}
	ret = ca_l3_intf_get(0, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(intf_id=%d) failed, ret=%d\n", __func__, l3_intf.intf_id, ret);
		return ret;
	}
	memset(&l3_intf_mask, 0, sizeof(ca_l3_intf_mask_t));
	l3_intf_mask.mtu = 1;
	l3_intf.mtu = mtu;

	ret = ca_l3_intf_update(0, &l3_intf);

	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_update(intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_INTERFACE) {
		printk("%s: Change mtu to %d for intf_id %d.\n", __func__, l3_intf.intf_id, mtu);
	}
	return CA_E_OK;
}

