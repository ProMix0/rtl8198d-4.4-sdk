#include "ca_types.h"
#include <soc/cortina/ca_kernel_hook.h>

static spinlock_t mcast_lock;

#define MAX_MCAST_VLAN	16
#define MC_MEMBER_MAX	16
#define UNI_PORT_NUMBER	2
#define CA_MAX_MCAST_GROUP_NUMBER   16
#define CA_PORT_UNI0 0
#define CA_PORT_UNI1 1

typedef struct ca_kh_mcast_vlan_s {
	ca_uint8_t  valid;
	ca_uint8_t lan_name[IFNAMSIZ+1];
	ca_uint16_t wan_vid;
	ca_uint16_t mcast_vid;
} ca_kh_mcast_vlan_t;

int uni0_max_mcast_grp_number = 8, uni1_max_mcast_grp_number = 8;
int l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number;

static ca_kh_mcast_vlan_t mcast_vlan[MAX_MCAST_VLAN];
ca_status_t ca_kh_mcast_vlan_get(ca_device_id_t device_id, ca_uint8_t lan_name[], ca_uint16_t *wan_vid, ca_uint16_t *mcast_vid);

ca_status_t ca_kh_l2_mcast_member_add(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr, ca_uint16_t vid)
{
	int ret = 0;
#if 0
	int i_port_id;
	ca_port_id_t port_id;
	ca_uint16_t wan_vid, mcast_vid;
#endif
	ca_l2_mcast_entry_t ca_entry;

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: device_name %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, vid %d\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), vid);
	}

	//spin_lock(&mcast_lock);
	memset(&ca_entry, 0, sizeof(ca_entry));

	///TODO:
	//port_id = ca_eth_dev_to_portid(0, device_name);

#if 0
	ret = ca_kh_mcast_vlan_get(device_id, device_name, &wan_vid, &mcast_vid);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s: ca_kh_mcast_vlan_get(device_name=%s) failed, ret=%d\n", __func__, device_name, ret);
		}
		return CA_E_ERROR;
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: device_name=%s, wan_vid=%d, mcast_vid=%d\n", __func__, device_name, wan_vid, mcast_vid);
	}
	ca_entry.mcast_vlan = mcast_vid;
	ca_entry.mcast_vlan = vid;
	ca_entry.filter_mode = CA_MCAST_EXCLUDE;
	if (group_addr->afi == CA_IPV4) {
		// IPv4
		ca_entry.grp_mac_addr[0] = 0x01;
		ca_entry.grp_mac_addr[1] = 0x00;
		ca_entry.grp_mac_addr[2] = 0x5e;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv4_addr >> 16)& 0x7f;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv4_addr >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv4_addr)& 0xff;
	} else {
		// IPv6
		ca_entry.grp_mac_addr[0] = 0x33;
		ca_entry.grp_mac_addr[1] = 0x33;
		ca_entry.grp_mac_addr[2] = (group_addr->ip_addr.ipv6_addr[3] >> 24) & 0xff;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv6_addr[3] >> 16) & 0xff;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv6_addr[3] >> 8) & 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv6_addr[3]) & 0xff;
	}
	ca_entry.src_num = 0;

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: port_id 0x%x, grp_mac_addr %pM\n",
			__func__, __LINE__, port_id, &(ca_entry.grp_mac_addr));
	}

	if(!strncmp(device_name, "eth1", 4))
		i_port_id = CA_PORT_UNI0;

	if(!strncmp(device_name, "eth2", 4))
		i_port_id = CA_PORT_UNI1;

	if ((i_port_id == CA_PORT_UNI0) &&
		(l2_cur_uni0_mcast_grp_number >= uni0_max_mcast_grp_number)) {
		printk("%s:%d:: UNI0-%s > UNI0 maximum number %d\n",
			__func__, __LINE__, device_name, uni0_max_mcast_grp_number);
		return ret;
	} else if ((i_port_id == CA_PORT_UNI1) &&
		(l2_cur_uni1_mcast_grp_number >= uni1_max_mcast_grp_number)) {
		printk("%s:%d:: UNI1-%s > UNI1 maximum number %d\n",
			__func__, __LINE__,  device_name, uni1_max_mcast_grp_number);
		return ret;
	}
	if ((l2_cur_uni0_mcast_grp_number + l2_cur_uni1_mcast_grp_number) >  MC_MEMBER_MAX) {
		printk("%s:%d:: Current is UNI%d,[UNI0(Max=%d) + UNI1(Max=%d)] > maximum group number=%d\n",
			__func__, __LINE__, i_port_id, uni0_max_mcast_grp_number,
			uni1_max_mcast_grp_number, MC_MEMBER_MAX);
		return ret;
	}

	ret = ca_l2_mcast_member_add(0, port_id, &ca_entry);

	if (ret == CA_E_OK) { /* Hard code for LYNXY turnkey. UNI-0: eth1, UNI-1: eth2 */
		if (i_port_id == CA_PORT_UNI0)
			l2_cur_uni0_mcast_grp_number++;
		if (i_port_id == CA_PORT_UNI1)
			l2_cur_uni1_mcast_grp_number++;
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s::%d: port_id=%d, i_port_id=%d, %s \n",
				__func__, __LINE__, port_id, i_port_id, device_name);
			printk("%s::%d l2_cur_uni0_mcast_grp_number=%d,l2_cur_uni1_mcast_grp_number=%d\n",
			__func__, __LINE__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number);
		}
	} else if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d::DUPLICATE! %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, ret = %d\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), ret);
	}
	//spin_unlock(&mcast_lock);
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s::%d, l2_cur_uni0_mcast_grp_number=%d, l2_cur_uni1_mcast_grp_number=%d, uni0_max_mcast_grp_number=%d, "
		"uni1_max_mcast_grp_number = %d\n", __func__, __LINE__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number,
		uni0_max_mcast_grp_number, uni1_max_mcast_grp_number);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(ca_kh_l2_mcast_member_add);

ca_status_t ca_kh_l2_mcast_member_delete(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr, ca_uint16_t vid)
{
	int ret;
#if 0
	int	i_port_id;
	ca_port_id_t port_id;
	ca_uint16_t wan_vid, mcast_vid;
#endif
	ca_l2_mcast_entry_t ca_entry;

	ret = CA_E_OK;
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: device_name %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, vid %d\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), vid);
	}

	//spin_lock(&mcast_lock);
	memset(&ca_entry, 0, sizeof(ca_entry));

	///TODO
	//port_id = ca_eth_dev_to_portid(0, device_name);
#if 0
	ret = ca_kh_mcast_vlan_get(device_id, device_name, &wan_vid, &mcast_vid);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s: ca_kh_mcast_vlan_get(device_name=%s) failed, ret=%d\n", __func__, device_name, ret);
		}
		return CA_E_ERROR;
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: wan_vid=%d, mcast_vid=%d\n", __func__, wan_vid, mcast_vid);
	}

	ca_entry.mcast_vlan = mcast_vid;
	ca_entry.mcast_vlan = vid;
	ca_entry.filter_mode = CA_MCAST_EXCLUDE;
	if (group_addr->afi == CA_IPV4) {
		// IPv4
		ca_entry.grp_mac_addr[0] = 0x01;
		ca_entry.grp_mac_addr[1] = 0x00;
		ca_entry.grp_mac_addr[2] = 0x5e;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv4_addr >> 16)& 0x7f;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv4_addr >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv4_addr)& 0xff;
	} else {
		// IPv6
		ca_entry.grp_mac_addr[0] = 0x33;
		ca_entry.grp_mac_addr[1] = 0x33;
		ca_entry.grp_mac_addr[2] = (group_addr->ip_addr.ipv6_addr[3] >> 24)& 0xff;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv6_addr[3] >> 16)& 0xff;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv6_addr[3] >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv6_addr[3])& 0xff;
	}
	ca_entry.src_num = 0;
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: port_id 0x%x, grp_mac_addr %pM\n",
			__func__, __LINE__, port_id, &(ca_entry.grp_mac_addr));
	}

	ret = ca_l2_mcast_member_delete(0, port_id, &ca_entry);

	if (ret == CA_E_OK) {
		if(!strncmp(device_name, "eth1", 4))
			i_port_id = CA_PORT_UNI0;
		if(!strncmp(device_name, "eth2", 4))
			i_port_id = CA_PORT_UNI1;
		if (i_port_id == CA_PORT_UNI0 && l2_cur_uni0_mcast_grp_number == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s::%d: l2_cur_uni0_mcast_grp_number=%d, i_port_id=%d, %s\n",
				__func__, __LINE__, l2_cur_uni0_mcast_grp_number, i_port_id, device_name);
			}
			return CA_E_ERROR;
		} else if (i_port_id == CA_PORT_UNI1 && l2_cur_uni1_mcast_grp_number == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s::%d: l2_cur_uni1_mcast_grp_number=%d, i_port_id=%d, %s\n",
				__func__, __LINE__, l2_cur_uni1_mcast_grp_number, i_port_id, device_name);
			}
			return CA_E_ERROR;
		}

		if (i_port_id == CA_PORT_UNI0)
			l2_cur_uni0_mcast_grp_number--;
		if (i_port_id == CA_PORT_UNI1)
			l2_cur_uni1_mcast_grp_number--;
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s::%d: port_id=%d, i_port_id=%d, %s \n",
				__func__, __LINE__, port_id, i_port_id, device_name);
			printk("%s:: l2_cur_uni0_mcast_grp_number=%d,l2_cur_uni1_mcast_grp_number=%d\n",
				__func__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number);
		}
	} else if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d::Not EXISTED! %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, ret = %d\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), ret);
	}
	//spin_unlock(&mcast_lock);
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s::%d, l2_cur_uni0_mcast_grp_number=%d, l2_cur_uni1_mcast_grp_number=%d, uni0_max_mcast_grp_number=%d, "
		"uni1_max_mcast_grp_number = %d\n", __func__, __LINE__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number,
		uni0_max_mcast_grp_number, uni1_max_mcast_grp_number);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(ca_kh_l2_mcast_member_delete);

ca_status_t ca_kh_l2_mcast_member_add_allport(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr)
{
	int ret;
#if 0
	int i_port_id;
	ca_port_id_t port_id;
	ca_uint16_t wan_vid, mcast_vid;
#endif
	ca_l2_mcast_entry_t ca_entry;

	ret = CA_E_OK;
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: device_name %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr));
	}

	memset(&ca_entry, 0, sizeof(ca_entry));
#if 0
	ret = ca_kh_mcast_vlan_get(device_id, device_name, &wan_vid, &mcast_vid);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s: ca_kh_mcast_vlan_get(device_name=%s) failed, ret=%d\n", __func__, device_name, ret);
		}
		return CA_E_ERROR;
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: wan_vid=%d, mcast_vid=%d\n", __func__, wan_vid, mcast_vid);
	}
	ca_entry.mcast_vlan = mcast_vid;
	ca_entry.filter_mode = CA_MCAST_EXCLUDE;
	if (group_addr->afi == CA_IPV4) {
		// IPv4
		ca_entry.grp_mac_addr[0] = 0x01;
		ca_entry.grp_mac_addr[1] = 0x00;
		ca_entry.grp_mac_addr[2] = 0x5e;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv4_addr >> 16)& 0x7f;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv4_addr >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv4_addr)& 0xff;
	} else {
		// IPv6
		ca_entry.grp_mac_addr[0] = 0x33;
		ca_entry.grp_mac_addr[1] = 0x33;
		ca_entry.grp_mac_addr[2] = (group_addr->ip_addr.ipv6_addr[3] >> 24) & 0xff;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv6_addr[3] >> 16) & 0xff;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv6_addr[3] >> 8) & 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv6_addr[3]) & 0xff;
	}
	ca_entry.src_num = 0;

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: grp_mac_addr %pM\n",
			__func__, __LINE__, &(ca_entry.grp_mac_addr));
	}

	/* workaround:
	 * If device_name contains "eth1", its port_id is 0x300.
	 * If device_name contains "eth2", its port_id is 0x301.
	 * No other choice.
	 */
	if (strncmp(device_name, "eth1", 4) == 0) {
		port_id = (CA_PORT_TYPE_ETHERNET << 8) + 0;
		i_port_id = CA_PORT_UNI0;
	} else if (strncmp(device_name, "eth2", 4) == 0) {
		port_id = (CA_PORT_TYPE_ETHERNET << 8) + 1;
		i_port_id = CA_PORT_UNI1;
	} else {
		return CA_E_ERROR;
	}

	if ((i_port_id == CA_PORT_UNI0) &&
		(l2_cur_uni0_mcast_grp_number >= uni0_max_mcast_grp_number)) {
		printk("%s:%d:: UNI0-%s > UNI0 maximum number %d\n",
		__func__, __LINE__, device_name, uni0_max_mcast_grp_number);
		return CA_E_FULL;
	} else if ((i_port_id == CA_PORT_UNI1) &&
		(l2_cur_uni1_mcast_grp_number >= uni1_max_mcast_grp_number)) {
		printk("%s:%d:: UNI1-%s > UNI1 maximum number %d\n",
		__func__, __LINE__,  device_name, uni1_max_mcast_grp_number);
		return CA_E_FULL;
	}
	if ((l2_cur_uni0_mcast_grp_number + l2_cur_uni1_mcast_grp_number) >  MC_MEMBER_MAX) {
		printk("%s:%d:: Current is UNI%d,[UNI0(Max=%d) + UNI1(Max=%d)] > maximum group number=%d\n",
		__func__, __LINE__, i_port_id, uni0_max_mcast_grp_number,
		uni1_max_mcast_grp_number, MC_MEMBER_MAX);
		return CA_E_FULL;
	}

	ret = ca_l2_mcast_member_add(0, port_id, &ca_entry);
	if (ret == CA_E_OK) {
		if (i_port_id == CA_PORT_UNI0)
			l2_cur_uni0_mcast_grp_number++;
		if (i_port_id == CA_PORT_UNI1)
			l2_cur_uni1_mcast_grp_number++;
	} else {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s:%d::DUPLICATE! %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, ret = %d\n",
				__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
				&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), ret);
		}
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s::%d, l2_cur_uni0_mcast_grp_number=%d, l2_cur_uni1_mcast_grp_number=%d, uni0_max_mcast_grp_number=%d, "
		"uni1_max_mcast_grp_number = %d\n", __func__, __LINE__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number,
		uni0_max_mcast_grp_number, uni1_max_mcast_grp_number);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(ca_kh_l2_mcast_member_add_allport);

ca_status_t ca_kh_l2_mcast_member_delete_allport(ca_device_id_t device_id, ca_uint8_t *device_name, ca_ip_address_t *group_addr)
{
	int ret;
#if 0
	int  i_port_id;
	ca_port_id_t port_id;
	ca_uint16_t wan_vid, mcast_vid;
#endif
	ca_l2_mcast_entry_t ca_entry;

	ret = CA_E_OK;
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr));
	}

	/* FIXME!!BUG#46255, Do we need to lock at kernel? */
	memset(&ca_entry, 0, sizeof(ca_entry));
#if 0
	ret = ca_kh_mcast_vlan_get(device_id, device_name, &wan_vid, &mcast_vid);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s: ca_kh_mcast_vlan_get(device_name=%s) failed, ret=%d\n", __func__, device_name, ret);
		}
		return CA_E_ERROR;
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: wan_vid=%d, mcast_vid=%d\n", __func__, wan_vid, mcast_vid);
	}

	ca_entry.mcast_vlan = mcast_vid;
	ca_entry.filter_mode = CA_MCAST_EXCLUDE;
	if (group_addr->afi == CA_IPV4) {
		// IPv4
		ca_entry.grp_mac_addr[0] = 0x01;
		ca_entry.grp_mac_addr[1] = 0x00;
		ca_entry.grp_mac_addr[2] = 0x5e;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv4_addr >> 16)& 0x7f;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv4_addr >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv4_addr)& 0xff;
	} else {
		// IPv6
		ca_entry.grp_mac_addr[0] = 0x33;
		ca_entry.grp_mac_addr[1] = 0x33;
		ca_entry.grp_mac_addr[2] = (group_addr->ip_addr.ipv6_addr[3] >> 24)& 0xff;
		ca_entry.grp_mac_addr[3] = (group_addr->ip_addr.ipv6_addr[3] >> 16)& 0xff;
		ca_entry.grp_mac_addr[4] = (group_addr->ip_addr.ipv6_addr[3] >> 8)& 0xff;
		ca_entry.grp_mac_addr[5] = (group_addr->ip_addr.ipv6_addr[3])& 0xff;
	}
	ca_entry.src_num = 0;

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s:%d:: grp_mac_addr %pM\n",
			__func__, __LINE__, &(ca_entry.grp_mac_addr));
	}

	/* workaround:
	 * If device_name contains "eth1", its port_id is 0x300.
	 * If device_name contains "eth2", its port_id is 0x301.
	 * No other choice.
	 */
	if (strncmp(device_name, "eth1", 4) == 0) {
		port_id = (CA_PORT_TYPE_ETHERNET << 8) + 0;
		i_port_id = CA_PORT_UNI0;
	} else if (strncmp(device_name, "eth2", 4) == 0) {
		port_id = (CA_PORT_TYPE_ETHERNET << 8) + 1;
		i_port_id = CA_PORT_UNI1;
	} else {
		return CA_E_ERROR;
	}

	ret = ca_l2_mcast_member_delete(0, port_id, &ca_entry);
	if (ret == CA_E_OK) {
		if (i_port_id == CA_PORT_UNI0 && l2_cur_uni0_mcast_grp_number == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s::%d: l2_cur_uni0_mcast_grp_number=%d, i_port_id=%d, %s\n",
				__func__, __LINE__, l2_cur_uni0_mcast_grp_number, i_port_id, device_name);
			}
			return CA_E_ERROR;
		} else if (i_port_id == CA_PORT_UNI1 && l2_cur_uni1_mcast_grp_number == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s::%d: l2_cur_uni1_mcast_grp_number=%d, i_port_id=%d, %s\n",
				__func__, __LINE__, l2_cur_uni1_mcast_grp_number, i_port_id, device_name);
			}
			return CA_E_ERROR;
		}
		if (i_port_id == CA_PORT_UNI0) {
			l2_cur_uni0_mcast_grp_number--;
		} else if (i_port_id == CA_PORT_UNI1) {
			l2_cur_uni1_mcast_grp_number--;
		}

		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s::%d: port_id=%d, i_port_id=%d, %s \n",
				__func__, __LINE__, port_id, i_port_id, device_name);
			printk("%s:: l2_cur_uni0_mcast_grp_number=%d,l2_cur_uni1_mcast_grp_number=%d\n",
				__func__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number);
		}

	} else {
		if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
			printk("%s:%d::Not EXISTED! %s, afi %d, group.addr_len %d, ipv4 %pI4, ipv6 %pI6, ret = %d\n",
			__func__, __LINE__, device_name, group_addr->afi, group_addr->addr_len,
			&(group_addr->ip_addr.ipv4_addr), &(group_addr->ip_addr.ipv6_addr), ret);
		}
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s::%d, l2_cur_uni0_mcast_grp_number=%d, l2_cur_uni1_mcast_grp_number=%d, uni0_max_mcast_grp_number=%d, "
		"uni1_max_mcast_grp_number = %d\n",__func__, __LINE__, l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number,
		uni0_max_mcast_grp_number, uni1_max_mcast_grp_number);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(ca_kh_l2_mcast_member_delete_allport);

ca_status_t ca_kh_mcast_set_max_group_number(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_port_id_t port_id,
	CA_IN ca_uint32_t max_grp_number)
{
	int i_port_id;

	if (port_id == AAL_PORT_UNI0) { /* AAL_PORT_UNI0 = 0x0*/
		i_port_id = CA_PORT_UNI0;
		uni0_max_mcast_grp_number = max_grp_number;
	}
	if (port_id == AAL_PORT_UNI1) { /* AAL_PORT_UNI1 = 0x5*/
		i_port_id = CA_PORT_UNI1;
		uni1_max_mcast_grp_number = max_grp_number;
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_set_max_group_number);

ca_status_t ca_kh_mcast_get_max_group_number(
	CA_IN  ca_device_id_t  device_id,
	CA_IN  ca_port_id_t port_id,
	CA_OUT ca_uint32_t *max_grp_number,
	CA_OUT ca_uint8_t *cur_mcast_grp_number)
{
	if (port_id == CA_PORT_UNI0) {
		*max_grp_number = uni0_max_mcast_grp_number;
		*cur_mcast_grp_number = l2_cur_uni0_mcast_grp_number;
	}
	if (port_id == CA_PORT_UNI1) {
		*max_grp_number = uni1_max_mcast_grp_number;
		*cur_mcast_grp_number = l2_cur_uni1_mcast_grp_number;
	}
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s::%d: port_id=%d, *max_grp_number=%d, *cur_mcast_grp_number=%d\n",
			__func__, __LINE__, port_id, *max_grp_number, *cur_mcast_grp_number);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_get_max_group_number);

void ca_kh_mcast_flush_max_group_number(void)
{
	l2_cur_uni0_mcast_grp_number = 0;
	l2_cur_uni1_mcast_grp_number = 0;
	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("UNI0-l2_cur_uni0_mcast_grp_number = %d, UNI1-l2_cur_uni0_mcast_grp_number = %d,"
			"CLEAR multicast maximum group number and Clear UNI port's L2 HW multicast entries.\n",
			l2_cur_uni0_mcast_grp_number, l2_cur_uni1_mcast_grp_number);
	}
}
EXPORT_SYMBOL(ca_kh_mcast_flush_max_group_number);

ca_uint8_t multicast_snooping = 1;
ca_status_t ca_kh_mcast_set_multicast_snooping(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t mcast_snooping)
{
	//spin_lock(&mcast_lock);
	multicast_snooping= mcast_snooping;
	//spin_unlock(&mcast_lock);

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk(KERN_INFO "%s: multicast_snooping is set to %u\n", __func__, multicast_snooping);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_set_multicast_snooping);

ca_status_t ca_kh_mcast_get_multicast_snooping(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint8_t *mcast_snooping)
{
	//spin_lock(&mcast_lock);
	*mcast_snooping = multicast_snooping;
	//spin_unlock(&mcast_lock);

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk(KERN_INFO "%s: multicast_snooping=%u\n", __func__, multicast_snooping);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_get_multicast_snooping);

ca_uint8_t multicast_fastleave = FALSE;
ca_status_t ca_kh_mcast_set_multicast_fastleave(
	CA_IN ca_device_id_t device_id,
        CA_IN ca_uint8_t mcast_fastleave)
{
        //spin_lock(&mcast_lock);
        multicast_fastleave= mcast_fastleave;
        //spin_unlock(&mcast_lock);

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk(KERN_INFO "%s: mcast_fastleave is set to %u\n", __func__, multicast_fastleave);
	}

        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_set_multicast_fastleave);

ca_status_t ca_kh_mcast_get_multicast_fastleave(
	CA_IN ca_device_id_t device_id,
        CA_OUT ca_uint8_t *mcast_fastleave)
{
        //spin_lock(&mcast_lock);
        *mcast_fastleave = multicast_fastleave;
        //spin_unlock(&mcast_lock);

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk(KERN_INFO "%s: mcast_fastleave=%u\n", __func__, (unsigned int) multicast_fastleave);
	}

        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_get_multicast_fastleave);

ca_status_t ca_kh_mcast_vlan_add(ca_device_id_t device_id, ca_uint8_t lan_name[], ca_uint16_t wan_vid, ca_uint16_t mcast_vid)
{
	int i;

	if (lan_name == NULL || (strlen(lan_name) > IFNAMSIZ)) {
		printk("%s: Incorrect parameter lan_name!!\n", __func__);
		return CA_E_ERROR;
	}

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: lan_name=%s, wan_vid=%d, mcast_vid=%d\n", __func__, lan_name, wan_vid, mcast_vid);
	}

	/* check whether existed or not */
	for (i = 0; i < MAX_MCAST_VLAN; i++) {
		if (mcast_vlan[i].valid == 0) {
			break;
		}
		if (!strcmp(mcast_vlan[i].lan_name, lan_name) && (mcast_vlan[i].wan_vid == wan_vid) && (mcast_vlan[i].mcast_vid == mcast_vid)) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s:  lan_name=%s, wan_vid=%d, mcast_vid=%d already exists!!\n", __func__, lan_name, wan_vid, mcast_vid);
			}
			return CA_E_EXISTS;
		}
	}

	if (i == MAX_MCAST_VLAN) {
		printk("%s: No more multicast vlan entry is available!!\n", __func__);
		return CA_E_ERROR;
	}

	mcast_vlan[i].valid = 1;
	strcpy(mcast_vlan[i].lan_name, lan_name);
	mcast_vlan[i].wan_vid = wan_vid;
	mcast_vlan[i].mcast_vid = mcast_vid;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_vlan_add);

ca_status_t ca_kh_mcast_vlan_del(ca_device_id_t device_id, ca_uint8_t lan_name[], ca_uint16_t wan_vid, ca_uint16_t mcast_vid)
{
	int i;

	if (lan_name == NULL || (strlen(lan_name) > IFNAMSIZ)) {
		printk("%s: Incorrect parameter lan_name!!\n", __func__);
                return CA_E_ERROR;
	}

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: lan_name=%s, wan_vid=%d, mcast_vid=%d\n", __func__, lan_name, wan_vid, mcast_vid);
	}

	for (i = 0; i < MAX_MCAST_VLAN; i++) {
		if (mcast_vlan[i].valid == 0) {
			continue;
		}

		if (!strcmp(mcast_vlan[i].lan_name, lan_name) && (mcast_vlan[i].wan_vid == wan_vid) && (mcast_vlan[i].mcast_vid == mcast_vid)) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s:  lan_name=%s, wan_vid=%d, mcast_vid=%d found!!\n", __func__, lan_name, wan_vid, mcast_vid);
			}
			memset(&(mcast_vlan[i]), 0, sizeof(ca_kh_mcast_vlan_t));
			return CA_E_OK;
		}
	}
	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_kh_mcast_vlan_del);

ca_status_t ca_kh_mcast_member_delete_all(ca_device_id_t device_id)
{
	int ret;

	spin_lock(&mcast_lock);

	ret = ca_l2_mcast_group_delete_all(0);
	ret |= ca_l3_mcast_group_delete_all(0);

	//if (ret == CA_E_OK) {
		l2_cur_uni0_mcast_grp_number = 0;
		l2_cur_uni1_mcast_grp_number = 0;
	//}
	spin_unlock(&mcast_lock);

	return ret;
}
EXPORT_SYMBOL(ca_kh_mcast_member_delete_all);

ca_status_t ca_kh_mcast_vlan_get(ca_device_id_t device_id, ca_uint8_t lan_name[], ca_uint16_t *wan_vid, ca_uint16_t *mcast_vid)
{
	int i;

	if (lan_name == NULL || (strlen(lan_name) > IFNAMSIZ)) {
		printk("%s: Incorrect parameter lan_name!!\n", __func__);
		return CA_E_ERROR;
	}

	if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
		printk("%s: lan_name=%s\n", __func__, lan_name);
	}

	for (i = 0; i < MAX_MCAST_VLAN; i++) {
		if (mcast_vlan[i].valid == 0) {
			continue;
		}
		if (!strcmp(mcast_vlan[i].lan_name, lan_name)) {
			if (ca_kh_debug & CA_KH_DEBUG_MCAST) {
				printk("%s:  lan_name=%s, wan_vid=%d, mcast_vid=%d found!!\n", __func__, lan_name, mcast_vlan[i].wan_vid, mcast_vlan[i].mcast_vid);
			}
			*wan_vid = mcast_vlan[i].wan_vid;
			*mcast_vid = mcast_vlan[i].mcast_vid;
			return CA_E_OK;
		}
	}
	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_kh_mcast_vlan_get);

ca_status_t ca_kh_mcast_init(void)
{
	spin_lock_init(&mcast_lock);

	/* init max group number for each port */
	uni0_max_mcast_grp_number = CA_MAX_MCAST_GROUP_NUMBER;
	uni1_max_mcast_grp_number = CA_MAX_MCAST_GROUP_NUMBER;
	memset(&(mcast_vlan[0]), 0, sizeof(ca_kh_mcast_vlan_t) * MAX_MCAST_VLAN);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_mcast_init);
