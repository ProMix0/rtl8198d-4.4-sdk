#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <net/if_inet6.h>
#include <net/addrconf.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <soc/cortina/ca_kernel_hook.h>

//#include "sdl_eth.h"
#include "ca_kernel_hook_tunnel.h"
#include "ca_kernel_hook_util.h"

/*
 * NOTE: PPPoE client must run in kernel mode. Otherwise, kernel hook for PPPoE doesn't work.
 */

typedef struct ca_l3_user_data {
	ca_intf_id_t	intf_id;
	ca_ip_afi_t	afi;
} ca_l3_user_data_t;

#define MAX_PPPOE_DATA        16

typedef struct ca_kh_pppoe_data_s {
	ca_uint8_t  valid;
	ca_int16_t ifindex;
	ca_uint8_t da_mac[6];
	ca_ip_address_t ip_addr;
} ca_kh_pppoe_data_t;

static spinlock_t pppoe_data_lock;

static ca_kh_pppoe_data_t pppoe_data[MAX_PPPOE_DATA];
ca_status_t ca_kh_pppoe_data_traverse(ca_device_id_t device_id);
ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
// TODO: ca_kh_multi_wan_pppoe_classifier_add is for port binding.
// ca_status_t ca_kh_multi_wan_pppoe_classifier_add(ca_ip_address_t *ip_addr, ca_uint8_t *mac_da, ca_int16_t ifindex);

ca_status_t ca_kh_pppoe_data_add(ca_device_id_t device_id, ca_int16_t ifindex, ca_uint8_t *da_mac, ca_ip_address_t *ip_addr)
{
	int i;
	ca_status_t ret = CA_E_OK;

	printk(KERN_INFO "%s: enter\n", __func__);

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: ifindex=%d, da_mac=%pM\n", __func__, ifindex, da_mac);
		if (ip_addr->afi == CA_IPV4) {
			printk(KERN_INFO "%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
		}
		else {
			printk(KERN_INFO "%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
		}
	}

	spin_lock_bh(&pppoe_data_lock);

	/* check whether existed or not */
	for (i = 0; i < MAX_PPPOE_DATA; i++) {
		if (pppoe_data[i].valid == 0) {
			continue;
		}

		if (   !memcmp(pppoe_data[i].da_mac, da_mac, 6)
		    && (pppoe_data[i].ifindex == ifindex)
		    && (ca_kh_is_ip_equal(&(pppoe_data[i].ip_addr), ip_addr)) ) {
			if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
				printk(KERN_INFO "%s: entry already existed!!, ifindex=%d, da_mac=%pM\n", __func__, ifindex, da_mac);
				if (ip_addr->afi == CA_IPV4) {
					printk(KERN_INFO "%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
				}
				else {
					printk(KERN_INFO "%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__, ip_addr->afi, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
				}
			}
			ret = CA_E_EXISTS;
			goto _add_end;
		}
	}

	for (i = 0; i < MAX_PPPOE_DATA; i++) {
		if (pppoe_data[i].valid == 0)
			break;
	}

	if (i == MAX_PPPOE_DATA) {
		printk(KERN_ERR "%s: No more multicast vlan entry is available!!\n", __func__);
		ret = CA_E_ERROR;
		goto _add_end;
	}

	pppoe_data[i].valid = 1;
	pppoe_data[i].ifindex = ifindex;
	memcpy(pppoe_data[i].da_mac, da_mac, 6);
	pppoe_data[i].ip_addr = *ip_addr;

_add_end:
	spin_unlock_bh(&pppoe_data_lock);
	return ret;
}
EXPORT_SYMBOL(ca_kh_pppoe_data_add);

ca_status_t ca_kh_pppoe_data_delete(ca_device_id_t device_id, ca_int16_t ifindex)
{
	int i;

	printk(KERN_INFO "%s: enter\n", __func__);

	spin_lock_bh(&pppoe_data_lock);

	/* search if exists */
	for (i = 0; i < MAX_PPPOE_DATA; i++) {
		if (pppoe_data[i].valid == 0) {
			continue;
		}
		if (pppoe_data[i].ifindex == ifindex) {
			memset(&pppoe_data[i], 0, sizeof(pppoe_data[i]));
			break;
		}
	}

	spin_unlock_bh(&pppoe_data_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_pppoe_data_delete);

ca_status_t ca_kh_pppoe_data_traverse(ca_device_id_t device_id)
{
        int i;

	printk(KERN_INFO "%s: enter\n", __func__);

	spin_lock_bh(&pppoe_data_lock);

        for (i = 0; i < MAX_PPPOE_DATA; i++) {
                if (pppoe_data[i].valid == 0) {
                        continue;
                }

		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_INFO "%s: index=%d, ifindex=%d, da_mac=%pM\n", __func__, i, pppoe_data[i].ifindex, pppoe_data[i].da_mac);
			if (pppoe_data[i].ip_addr.afi == CA_IPV4) {
				printk(KERN_INFO "%s: afi=%d, ip_addr=%pI4, addr_len=%d\n", __func__,
					pppoe_data[i].ip_addr.afi, &pppoe_data[i].ip_addr.ip_addr.ipv4_addr, pppoe_data[i].ip_addr.addr_len);
			}
			else {
				printk(KERN_INFO "%s: afi=%d, ip_addr=%pI6, addr_len=%d\n", __func__,
					pppoe_data[i].ip_addr.afi, &pppoe_data[i].ip_addr.ip_addr.ipv6_addr, pppoe_data[i].ip_addr.addr_len);
			}
		}
		//ca_kh_neigh_update(device_id, pppoe_data[i].ifindex, pppoe_data[i].da_mac, &(pppoe_data[i].ip_addr));
		// TODO: ca_kh_multi_wan_pppoe_classifier_add is for port binding.
		// ca_kh_multi_wan_pppoe_classifier_add(&(pppoe_data[i].ip_addr), pppoe_data[i].da_mac, pppoe_data[i].ifindex);
		//memset(&(pppoe_data[i]), 0, sizeof(ca_kh_pppoe_data_t));
	}

	spin_unlock_bh(&pppoe_data_lock);
        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_pppoe_data_traverse);

void ca_kh_pppoe_data_dump(void)
{
	int i;

	printk(KERN_INFO "idx valid ifindex       da_mac            ip_addr\n");
	printk(KERN_INFO "-------------------------------------------------\n");

	for (i = 0; i < MAX_PPPOE_DATA; i++) {
		printk(KERN_INFO "%2d    %d      %2d     %pM  ", i, pppoe_data[i].valid, pppoe_data[i].ifindex, pppoe_data[i].da_mac);
		if (pppoe_data[i].ip_addr.afi == CA_IPV4) {
			printk("%pI4\n", &pppoe_data[i].ip_addr.ip_addr);
		} else if (pppoe_data[i].ip_addr.afi == CA_IPV6) {
			printk("%pI6\n", &pppoe_data[i].ip_addr.ip_addr);
		} else {
			printk("INVALID_ADDR\n");
		}
	}
}
EXPORT_SYMBOL(ca_kh_pppoe_data_dump);

ca_status_t ca_kh_pppoe_add(ca_device_id_t device_id, struct net_device *ppp_dev, struct net_device *eth_dev, struct pppox_sock *po)
{
	int ret;
	ca_tunnel_id_t tunnel_id;
	ca_tunnel_cfg_t tunnel_cfg;
	ca_l3_intf_t l3_intf;
	ca_l3_intf_t parent_l3_intf;
	ca_kh_tunnel_entry_t kh_tunnel;
	ca_kh_ifindex_entry_t kh_ifindex_entry;

	ca_ip_address_t local_addr, remote_addr; /* store tunnel IP addr, network order */

	struct in_device *ppp_in_dev;
	struct in_ifaddr *ppp_ifa;

	struct in_device *eth_in_dev;
	struct in_ifaddr *eth_ifa;

	struct inet6_dev *ppp_in6_dev;
	struct inet6_ifaddr *ppp_ifa6;

	struct inet6_dev *eth_in6_dev;
	struct inet6_ifaddr *eth_ifa6;

	printk(KERN_INFO "%s: enter\n", __func__);

	if (po == NULL) {
		printk("%s: po is NULL!!!\n", __func__);
		return CA_E_OK;
	}

	if (ppp_dev == NULL) {
		printk("%s: ppp_dev is NULL!!!\n", __func__);
		return CA_E_OK;
	}

	/* check ifindex if not WAN interface then do nothing */
	if (eth_dev == NULL) {
		printk("%s: eth_dev is NULL!!!\n", __func__);
		return CA_E_OK;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: ppp_dev name=%s, ifindex=%d; eth_dev name=%s, ifindex=%d\n", __func__, __LINE__, ppp_dev->name, ppp_dev->ifindex, eth_dev->name, eth_dev->ifindex);
	}

	/* search IPv4 address from ppp_dev */
	ppp_in_dev = __in_dev_get_rtnl(ppp_dev);
	ppp_ifa = ppp_in_dev->ifa_list;
	for (; ppp_ifa != NULL; ppp_ifa = ppp_ifa->ifa_next) {
		if (strcmp(ppp_dev->name, ppp_ifa->ifa_label) == 0) {
			/* found correct ppp_ifa */
			break;
		}
	}

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.afi = CA_IPV4;
	local_addr.addr_len = 32;

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.afi = CA_IPV4;
	remote_addr.addr_len = 32;

	if (ppp_ifa != NULL) {
		local_addr.ip_addr.ipv4_addr = ppp_ifa->ifa_local;
		remote_addr.ip_addr.ipv4_addr = ppp_ifa->ifa_address;
	}

	/* search IPv4 address from eth_dev */
	eth_in_dev = __in_dev_get_rtnl(eth_dev);
	eth_ifa = eth_in_dev->ifa_list;
	for (; eth_ifa != NULL; eth_ifa = eth_ifa->ifa_next) {
		if (strcmp(eth_dev->name, eth_ifa->ifa_label) == 0) {
			/* found correct eth_ifa */
			break;
		}
	}

	/*** Update PPPoE session ID to parent IPv4 interface ***/

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, eth_dev->ifindex, CA_L3_INTF_BCAST, &parent_l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_add_ipv6;
	}

	ret = ca_l3_intf_get(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_add_ipv6;
	}
#if 0
	/* this doesn't make sense; this design can't support multiple PPPoE over single interface */
	parent_l3_intf.pppoe_session_id = ntohs(po->pppoe_pa.sid);
	ret = ca_l3_intf_update(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_update() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: set PPPoE SID 0x%04x to IPv4 parent_l3_intf %d\n", __func__, __LINE__, parent_l3_intf.pppoe_session_id, parent_l3_intf.intf_id);
	}
#endif
	/*** Create tunnel for PPP ***/

	memset(&tunnel_cfg, 0, sizeof(tunnel_cfg));
	tunnel_cfg.type = CA_TUNNEL_PPPOE;

	tunnel_cfg.tunnel.pppoe.pppoe_session_id = ntohs(po->pppoe_pa.sid);
	memcpy(&tunnel_cfg.tunnel.pppoe.mac_da, &po->pppoe_pa.remote, ETH_ALEN);
	tunnel_cfg.parent_l3_intf_id = parent_l3_intf.intf_id;
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &local_addr);
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &remote_addr);

	ret = ca_tunnel_add(device_id, &tunnel_cfg, &tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_tunnel_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_add_ipv6;
	}

	kh_tunnel.type = CA_TUNNEL_PPPOE;
	memcpy(&kh_tunnel.src_addr, &local_addr, sizeof(ca_ip_address_t));
	memcpy(&kh_tunnel.dest_addr, &remote_addr, sizeof(ca_ip_address_t));
	kh_tunnel.tunnel_id = tunnel_id;
	ret = ca_kh_l3_tunnel_add(device_id, &kh_tunnel);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_tunnl_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_add_ipv6;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: tunnel_id=%d\n", __func__, __LINE__, tunnel_id);
	}

	/*** Create IPv4 PPPoE interface for PPP tunnel ***/

	memset(&l3_intf, 0, sizeof(l3_intf));
	l3_intf.port_id = parent_l3_intf.port_id;
	l3_intf.type = CA_L3_INTF_TUNNEL;
	l3_intf.tunnel_id = tunnel_id;
	l3_intf.mtu = ppp_dev->mtu;
	l3_intf.ip_addr.afi = CA_IPV4;
	l3_intf.ip_addr.addr_len = 32;
	memcpy(&l3_intf.mac_addr, eth_dev->dev_addr, 6);
	ca_kh_ip_address_ntohl(&l3_intf.ip_addr, &local_addr);

	l3_intf.mask.port_id = 1;
	l3_intf.mask.tunnel_id = 1;
	l3_intf.mask.mtu = 1;
	l3_intf.mask.ip_addr = 1;
	l3_intf.mask.mac_addr = 1;

	ca_kh_decide_nat_enable(&l3_intf);

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ppp_dev->ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret == CA_E_ERROR) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (is_vlan_dev(eth_dev)) {
		l3_intf.outer_vid = vlan_dev_vlan_id(eth_dev);
	} else {
		l3_intf.outer_vid = CA_UINT16_INVALID;
	}
	l3_intf.mask.outer_vid = 1;

	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: add an IPv4 interface for %s, l3_intf.intf_id=%d\n", __func__, __LINE__, ppp_dev->name, l3_intf.intf_id);
	}

	/* Add the new l3_intf into table */
	memset(&kh_ifindex_entry, 0, sizeof(kh_ifindex_entry));
	kh_ifindex_entry.afi = l3_intf.ip_addr.afi;
	kh_ifindex_entry.ifindex = ppp_dev->ifindex;
	kh_ifindex_entry.intf_type = CA_L3_INTF_TUNNEL;
	kh_ifindex_entry.tunnel_type = CA_TUNNEL_PPPOE;
	ca_kh_l3_set_intf_id(device_id, l3_intf.intf_id, &kh_ifindex_entry);

	/******************************************************************/

_pppoe_add_ipv6:
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.afi = CA_IPV6;
	local_addr.addr_len = 128;

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.afi = CA_IPV6;
	remote_addr.addr_len = 128;

	/* search IPv6 address from ppp_dev */
	ppp_in6_dev = __in6_dev_get(ppp_dev);
	if (ppp_in6_dev == NULL) {
		printk(KERN_ERR "%s: %d: Fail to get IPv6 dev from %s\n", __func__, __LINE__, ppp_dev->name);
		return CA_E_ERROR;
	}

	list_for_each_entry(ppp_ifa6, &ppp_in6_dev->addr_list, if_list) {
		memcpy(&local_addr.ip_addr.ipv6_addr, &ppp_ifa6->addr, sizeof(struct in6_addr));
		local_addr.addr_len = ppp_ifa6->prefix_len;
		break;
	}

	/* search IPv6 address from eth_dev */
	eth_in6_dev = __in6_dev_get(eth_dev);
	if (eth_in6_dev == NULL) {
		printk(KERN_ERR "%s: %d: Fail to get IPv6 dev from %s\n", __func__, __LINE__, eth_dev->name);
		return CA_E_ERROR;
	}

	list_for_each_entry(eth_ifa6, &eth_in6_dev->addr_list, if_list) {
		/* IPv6 addr of eth_dev is in eth_ifa6->addr */
	}

	/*** Update PPPoE session ID to parent IPv6 interface ***/

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, eth_dev->ifindex, CA_L3_INTF_BCAST, &parent_l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = ca_l3_intf_get(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}
#if 0
	/* this doesn't make sense; this design can't support multiple PPPoE over single interface */
	parent_l3_intf.pppoe_session_id = ntohs(po->pppoe_pa.sid);
	ret = ca_l3_intf_update(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_update() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: set PPPoE SID 0x%04x to IPv6 parent_l3_intf %d\n", __func__, __LINE__, parent_l3_intf.pppoe_session_id, parent_l3_intf.intf_id);
	}
#endif

	/*** Create IPv6 PPPoE interface for PPP tunnel ***/

	memset(&l3_intf, 0, sizeof(l3_intf));
	l3_intf.port_id = parent_l3_intf.port_id; // TODO
	l3_intf.type = CA_L3_INTF_TUNNEL;
	l3_intf.tunnel_id = tunnel_id;
	l3_intf.mtu = ppp_dev->mtu;
	l3_intf.ip_addr.afi = CA_IPV6;
	l3_intf.ip_addr.addr_len = 128;
	memcpy(&l3_intf.mac_addr, eth_dev->dev_addr, 6);
	ca_kh_ip_address_ntohl(&l3_intf.ip_addr, &local_addr);

	l3_intf.mask.port_id = 1;
	l3_intf.mask.tunnel_id = 1;
	l3_intf.mask.mtu = 1;
	l3_intf.mask.ip_addr = 1;
	l3_intf.mask.mac_addr = 1;

	ca_kh_decide_nat_enable(&l3_intf);

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ppp_dev->ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret == CA_E_ERROR) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (is_vlan_dev(eth_dev)) {
		l3_intf.outer_vid = vlan_dev_vlan_id(eth_dev);
	} else {
		l3_intf.outer_vid = CA_UINT16_INVALID;
	}
	l3_intf.mask.outer_vid = 1;

	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: add an IPv6 interface for %s, l3_intf.intf_id=%d\n", __func__, __LINE__, ppp_dev->name, l3_intf.intf_id);
	}

	/* Add the new l3_intf into table */
	kh_ifindex_entry.afi = l3_intf.ip_addr.afi;
	ca_kh_l3_set_intf_id(device_id, l3_intf.intf_id, &kh_ifindex_entry);

	return ret;
}

ca_status_t ca_kh_pppoe_update(ca_device_id_t device_id, struct net_device *ppp_dev)
{
	int ret;
	ca_l3_intf_t l3_intf, parent_l3_intf;
	ca_tunnel_cfg_t tunnel_cfg;

	ca_ip_address_t local_addr, remote_addr; /* store tunnel IP addr, network order */

	struct in_device *ppp_in_dev;
	struct in_ifaddr *ppp_ifa;

#if 0
	struct inet6_dev *ppp_in6_dev;
	struct inet6_ifaddr *ppp_ifa6;
#endif

	printk(KERN_INFO "%s: enter\n", __func__);

	if (ppp_dev == NULL) {
		printk(KERN_INFO "%s: ppp_dev is NULL!\n", __func__);
		return CA_E_OK;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: name=%s, ifindex=%d\n", __func__, ppp_dev->name, ppp_dev->ifindex);
	}

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.afi = CA_IPV4;
	local_addr.addr_len = 32;

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.afi = CA_IPV4;
	remote_addr.addr_len = 32;

	/* search IPv4 address from ppp_dev */
	ppp_in_dev = __in_dev_get_rtnl(ppp_dev);
	ppp_ifa = ppp_in_dev->ifa_list;
	for (; ppp_ifa != NULL; ppp_ifa = ppp_ifa->ifa_next) {
		if (strcmp(ppp_dev->name, ppp_ifa->ifa_label) == 0) {
			/* found correct ppp_ifa */
			break;
		}
	}

	if (ppp_ifa == NULL) {
		printk(KERN_ERR "%s: Fail to get IP entry from %s.\n", __func__, ppp_dev->name);
		return CA_E_ERROR;
	} else {
		local_addr.ip_addr.ipv4_addr = ppp_ifa->ifa_local;
		remote_addr.ip_addr.ipv4_addr = ppp_ifa->ifa_address;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: %s, ifa_local=%pI4, ifa_address=%pI4\n",
			__func__, __LINE__, ppp_dev->name, &ppp_ifa->ifa_local, &ppp_ifa->ifa_address);
	}

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ppp_dev->ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: get intf_id=%d from %s ifindex=%d\n", __func__, __LINE__, l3_intf.intf_id, ppp_dev->name, ppp_dev->ifindex);
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/*** Update remote IPv4 of PPP tunnel ***/

	ret = ca_tunnel_get(device_id, l3_intf.tunnel_id, &tunnel_cfg);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_tunnel_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &local_addr);
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &remote_addr);

	ret = ca_tunnel_update(device_id, &tunnel_cfg, l3_intf.tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_tunnel_update() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/*** Get parent Ethernet interface ***/

	parent_l3_intf.intf_id = tunnel_cfg.parent_l3_intf_id;
	ret = ca_l3_intf_get(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/*** Update address, VID, and MAC of IPv4 PPPoE interface ***/

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: Update addr, VID, MAC, MTU of IPv4 PPPoE interface in intf table.\n", __func__, __LINE__);
		printk(KERN_INFO "%s:%d: intf_id=%u, VID=%u, MAC=%pM, MTU=%u\n", __func__, __LINE__, l3_intf.intf_id, parent_l3_intf.outer_vid, parent_l3_intf.mac_addr, ppp_dev->mtu);
	}

	l3_intf.mtu = ppp_dev->mtu;
	l3_intf.outer_vid = parent_l3_intf.outer_vid;
	memcpy(&l3_intf.mac_addr, &parent_l3_intf.mac_addr, sizeof(ca_mac_addr_t));
	ca_kh_ip_address_ntohl(&l3_intf.ip_addr, &local_addr);

	l3_intf.mask.mtu = 1;
	l3_intf.mask.outer_vid = 1;

	/* delete according to l3_intf.intf_id */
	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}
	/* add the updated l3_intf */
	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/*** Update nexthop MAC of PPPoE peer ***/

	/* NOTE:
	 * System appends a route for PPPoE connection after this hook function,
	 * therefore this function can't update nexthop MAC of PPPoE peer.
	 */

	/******************************************************************/
	/* NOTE:
	 * PPPoE IPv6 won't call ioctl SIOCSIFDSTADDR to set remote addr, so ignore IPv6.
	 *
	 * Linux treats PPPoE IPv6 as a normal IPv6 routing.
	 * Global IPv6 address is not assigned by pppd, system should acquire one through DHCPv6.
	 */

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ppp_dev->ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_update_end;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: get intf_id=%d from %s ifindex=%d\n", __func__, __LINE__, l3_intf.intf_id, ppp_dev->name, ppp_dev->ifindex);
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_update_end;
	}

	/*** Update MAC and VID of IPv6 PPPoE interface ***/

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s:%d: Update addr, VID, MAC, MTU of IPv6 PPPoE interface in intf table.\n", __func__, __LINE__);
		printk(KERN_INFO "%s:%d: intf_id=%u, VID=%u, MAC=%pM, MTU=%u\n", __func__, __LINE__, l3_intf.intf_id, parent_l3_intf.outer_vid, parent_l3_intf.mac_addr, ppp_dev->mtu);
	}

	l3_intf.mtu = ppp_dev->mtu;
	l3_intf.outer_vid = parent_l3_intf.outer_vid;
	memcpy(&l3_intf.mac_addr, &parent_l3_intf.mac_addr, sizeof(ca_mac_addr_t));

	l3_intf.mask.mtu = 1;
	l3_intf.mask.outer_vid = 1;

	/* delete according to l3_intf.intf_id */
	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_update_end;
	}
	/* add the updated l3_intf */
	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_add() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_update_end;
	}

	/******************************************************************/
_pppoe_update_end:
	printk(KERN_INFO "%s: Add a PPPoE data, IP=%pI4, mac_da=%pM\n", __func__, &remote_addr.ip_addr.ipv4_addr, tunnel_cfg.tunnel.pppoe.mac_da);
	ca_kh_pppoe_data_add(device_id, ppp_dev->ifindex, tunnel_cfg.tunnel.pppoe.mac_da, &remote_addr);

	return ret;
}

ca_status_t ca_kh_pppoe_delete(ca_device_id_t device_id, int ifindex)
{
	int ret;
	int tunnel_id;
	ca_l3_intf_t l3_intf;
	ca_kh_ifindex_entry_t kh_ifindex_entry;

	printk(KERN_INFO "%s: enter\n", __func__);

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: ifindex=%d\n", __func__, ifindex);
	}

	/* get IPv4 intf_id */

	l3_intf.ip_addr.afi = CA_IPV4;
	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/* delete PPPoE tunnel */

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Delete PPPoE IPv4 tunnel. tunnel_id=%u\n", __func__, l3_intf.tunnel_id);
	}

	tunnel_id = l3_intf.tunnel_id;
	ret = ca_tunnel_delete(device_id, tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_tunnel_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = ca_kh_l3_tunnel_delete(device_id, tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_tunnel_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/* delete IPv4 interface */

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Delete PPPoE IPv4 interface. intf_id=%u\n", __func__, l3_intf.intf_id);
	}

	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_delete_end;
	}

	memset(&kh_ifindex_entry, 0, sizeof(kh_ifindex_entry));
	kh_ifindex_entry.afi = CA_IPV4;
	kh_ifindex_entry.ifindex = 0;
	kh_ifindex_entry.intf_type = CA_L3_INTF_TUNNEL;
	kh_ifindex_entry.tunnel_type = CA_TUNNEL_PPPOE;
	ca_kh_l3_set_intf_id(device_id, l3_intf.intf_id, &kh_ifindex_entry);

	/******************************************************************/

	/* get IPv6 intf_id */

	l3_intf.ip_addr.afi = CA_IPV6;
	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, CA_L3_INTF_TUNNEL, &l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_kh_l3_get_intf_id_by_ifindex() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_delete_end;
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_get() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_delete_end;
	}

	/* delete IPv6 interface */

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Delete PPPoE IPv6 interface. intf_id=%u\n", __func__, l3_intf.intf_id);
	}

	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s:%d: ca_l3_intf_delete() returns failed!! (%d)\n", __func__, __LINE__, ret);
		goto _pppoe_delete_end;
	}

	kh_ifindex_entry.afi = CA_IPV6;
	kh_ifindex_entry.ifindex = 0;
	ca_kh_l3_set_intf_id(device_id, l3_intf.intf_id, &kh_ifindex_entry);

	/******************************************************************/
_pppoe_delete_end:
	ca_kh_pppoe_data_delete(device_id, ifindex);

	return ret;
}

ca_status_t ca_kh_pppoe_init(void)
{
	spin_lock_init(&pppoe_data_lock);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_pppoe_init);

