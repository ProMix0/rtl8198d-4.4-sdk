#include <linux/netdevice.h>
#include <net/ipv6.h>
#include <net/ip6_tunnel.h>

#include "ca_kernel_hook_util.h"
#include <soc/cortina/ca_kernel_hook.h>

typedef struct ca_l3_user_data {
	ca_intf_id_t	intf_id;
	ca_ip_afi_t	afi;
} ca_l3_user_data_t;

ca_status_t ca_kh_tunnel_update_parent_intf_id(ca_device_id_t device_id, ca_ip_address_t *ip_addr, ca_uint32_t parent_l3_intf_id)
{
	int i, ret;
	ca_tunnel_cfg_t tunnel_cfg;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: parent_l3_intf_id=%d\n", __func__, parent_l3_intf_id);
		printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv6_addr=%pI6\n",
			__func__, ip_addr->afi, ip_addr->addr_len, ip_addr->ip_addr.ipv6_addr);
	}

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {

		ret = ca_tunnel_get(device_id, i, &tunnel_cfg);
		if (ret != CA_E_OK) continue;

		if (tunnel_cfg.type == CA_TUNNEL_PPPOE || tunnel_cfg.src_addr.afi == CA_IPV4) continue;

		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: type=%d, parent_l3_intf_id=%d\n", __func__, tunnel_cfg.type, tunnel_cfg.parent_l3_intf_id);
			printk("%s: src_addr.afi=%d, src_addr.addr_len=%d, src_addr.ip_addr.ipv6_addr=%pI6\n",
				__func__, tunnel_cfg.src_addr.afi, tunnel_cfg.src_addr.addr_len, &tunnel_cfg.src_addr.ip_addr.ipv6_addr);
		}

		/* no need to check address length */
		tunnel_cfg.src_addr.addr_len = 128;
		ip_addr->addr_len = 128;

		if (ca_kh_is_ip_equal(&tunnel_cfg.src_addr, ip_addr)) {
			if (ca_kh_debug & CA_KH_DEBUG_TUNNEL){
				printk("%s: found tunnel at index=%d\n", __func__, i);
			}
			tunnel_cfg.parent_l3_intf_id = parent_l3_intf_id;
			ret = ca_tunnel_update(device_id, &tunnel_cfg, i);
			if (ret != CA_E_OK) {
				printk("%s: ca_tunnel_update(), return failed! ret=%d\n", __func__, ret);
				return ret;
			}

			return CA_E_OK;
		}
	}
	return CA_E_ERROR;
}

ca_status_t ca_kh_dslite_add(ca_device_id_t device_id, int ifindex, struct __ip6_tnl_parm *p, ca_tunnel_id_t *ret_tunnel_id)
{
	int ret;
	ca_tunnel_id_t tunnel_id;
#if 0
	ca_l3_nexthop_t l3_nexthop;
	ca_l3_nexthop_id_t l3_nexthop_id;
	ca_kh_nexthop_entry_t kh_nexthop;
#endif
	ca_tunnel_cfg_t tunnel_cfg;
	ca_l3_intf_t l3_intf, parent_l3_intf;
	ca_kh_tunnel_entry_t kh_tunnel;
	bool is_mcast;
	struct net_device *dev = NULL, *parent_dev = NULL;
	ca_l3_intf_type_t parent_intf_type;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: name=%s, ifindex=%d, parent_ifindex=%d\n", __func__, p->name, ifindex, p->link);
		printk("%s: local ipv6_addr=%pI6\n", __func__, &p->laddr);
		printk("%s: remote ipv6_addr=%pI6\n", __func__, &p->raddr);
	}


	if (ipv6_addr_type(&p->laddr) & IPV6_ADDR_MULTICAST)
		is_mcast = true;
	else
		is_mcast = false;

	/* get parent intf_id and its intf */

	parent_dev = dev_get_by_index(&init_net, p->link);
	if (parent_dev == NULL) {
		printk("%s: Fail to get parent_dev by ifindex %d\n", __func__, p->link);
		return CA_E_PARAM;
	}
	dev_put(parent_dev);

	if (parent_dev->flags & IFF_POINTOPOINT)
		parent_intf_type = CA_L3_INTF_TUNNEL;
	else
		parent_intf_type = CA_L3_INTF_BCAST;

	parent_l3_intf.ip_addr.afi = CA_IPV6;
	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, parent_l3_intf.ip_addr.afi, p->link, parent_intf_type, &parent_l3_intf.intf_id);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: ca_kh_l3_intf_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		}
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: parent_l3_intf.intf_id=%d\n", __func__, parent_l3_intf.intf_id);
	}

	ret = ca_l3_intf_get(0, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(intf_id=%d), return failed! ret=%d\n", __func__, parent_l3_intf.intf_id, ret);
		return ret;
	}

	/* search an empty intf_id for tunnel interface */

	memset(&l3_intf, 0, sizeof(ca_l3_intf_t));

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_E_PARAM;
	}
	l3_intf.mtu = dev->mtu;
	l3_intf.mask.mtu = 1;
	dev_put(dev);

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, &l3_intf.intf_id, CA_TUNNEL_DSLITE);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: ca_kh_l3_tunnel_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		}

		if(ret != CA_E_NOT_FOUND)
			return ret;
	}
	tunnel_id = l3_intf.intf_id; /* workaround: pass wanted intf_id to AAL by tunnel_id */

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
	}

	/* add a tunnel for tunnel_id */

	memset(&tunnel_cfg, 0, sizeof(ca_tunnel_cfg_t));
	tunnel_cfg.type = CA_TUNNEL_DSLITE;
	tunnel_cfg.parent_l3_intf_id = parent_l3_intf.intf_id;

	tunnel_cfg.dest_addr.afi = CA_IPV6;
	memcpy(&(tunnel_cfg.dest_addr.ip_addr.ipv6_addr[0]), &p->raddr, sizeof(ca_l3_ip_addr_t));
	tunnel_cfg.dest_addr.addr_len = 128;
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	tunnel_cfg.src_addr.afi = CA_IPV6;
	memcpy(&(tunnel_cfg.src_addr.ip_addr.ipv6_addr[0]), &p->laddr, sizeof(ca_l3_ip_addr_t));
	tunnel_cfg.src_addr.addr_len = 128;
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	if (is_mcast) {
		tunnel_cfg.tunnel.dslite.mc_flag = 1;
		tunnel_cfg.tunnel.dslite.mc_drop = 0;
	} else {
		tunnel_cfg.tunnel.dslite.mc_flag = 0;
		tunnel_cfg.tunnel.dslite.mc_drop = 1;
	}
	tunnel_cfg.tunnel.dslite.rebuild_mc_mac = 1;
	tunnel_cfg.tunnel.dslite.mc_mac[0] = 0x01;
	tunnel_cfg.tunnel.dslite.mc_mac[1] = 0x00;
	tunnel_cfg.tunnel.dslite.mc_mac[2] = 0x5e;
	tunnel_cfg.tunnel.dslite.mc_mac[3] = 0x00;
	tunnel_cfg.tunnel.dslite.mc_mac[4] = 0x00;
	tunnel_cfg.tunnel.dslite.mc_mac[5] = 0x00;
	tunnel_cfg.tunnel.dslite.dsl_ipmc_illegal_check_en = 0;
	tunnel_cfg.tunnel.dslite.dsl_ipmc_to_l2fe = 0;
	tunnel_cfg.tunnel.dslite.dsl_ipmc_addr_prefix_check_en = 0;
	tunnel_cfg.tunnel.dslite.dsl_ipmc_addr_consistency_check_en = 0;

	tunnel_cfg.tunnel.dslite.dsl_reverse_path_check_en = 0;
	tunnel_cfg.tunnel.dslite.dsl_ttl_keep_outer = 0;
	tunnel_cfg.tunnel.dslite.dsl_dscp_keep_outer = (p->flags & IP6_TNL_F_RCV_DSCP_COPY) ? 1 : 0;
	tunnel_cfg.tunnel.dslite.dsl_ecn_keep_outer = 0;
	tunnel_cfg.tunnel.dslite.dsl_ecn_check_enable = 0;
	tunnel_cfg.tunnel.dslite.validation_check_failed_dest_port = 0x1f; /* or to CPU? */

	tunnel_cfg.tunnel.dslite.egress_tc_value = (p->flags & IP6_TNL_F_USE_ORIG_TCLASS) ? 0 /* ? */ : p->flowinfo & IPV6_TCLASS_MASK;
	tunnel_cfg.tunnel.dslite.egress_hoplimit_value = (p->hop_limit == 0) ? 64 : p->hop_limit;
	tunnel_cfg.tunnel.dslite.egress_flow_label_value = (p->flags & IP6_TNL_F_USE_ORIG_FLOWLABEL) ? 0 /* ? */ : p->flowinfo & IPV6_FLOWLABEL_MASK;

	ret = ca_tunnel_add(device_id, &tunnel_cfg, &tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_tunnel_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	/* set returned tunnel_id first */
	if (ret_tunnel_id != NULL)
		*ret_tunnel_id = tunnel_id;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: tunnel_id=%d\n", __func__, tunnel_id);
	}

	/* record the added tunnel info */

	kh_tunnel.type = CA_TUNNEL_DSLITE;
	kh_tunnel.dest_addr.afi = CA_IPV6;
	kh_tunnel.dest_addr.addr_len = 128;
	memcpy(&(kh_tunnel.dest_addr.ip_addr.ipv6_addr[0]), &p->raddr, sizeof(ca_l3_ip_addr_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	kh_tunnel.src_addr.afi = CA_IPV6;
	kh_tunnel.src_addr.addr_len = 128;
	memcpy(&(kh_tunnel.src_addr.ip_addr.ipv6_addr[0]), &p->laddr, sizeof(ca_l3_ip_addr_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	kh_tunnel.tunnel_id = tunnel_id;
	kh_tunnel.ifindex = ifindex;
	ret = ca_kh_l3_tunnel_add(device_id, &kh_tunnel);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnl_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	/* add tunnel interface */

	l3_intf.type = CA_L3_INTF_TUNNEL;
	l3_intf.port_id = parent_l3_intf.port_id;
	l3_intf.tunnel_id = tunnel_id;
	l3_intf.outer_vid = parent_l3_intf.outer_vid;
	l3_intf.ip_addr.afi = CA_IPV6;
	memcpy(&(l3_intf.ip_addr.ip_addr.ipv6_addr[0]), &p->laddr, sizeof(ca_l3_ip_addr_t));
	l3_intf.ip_addr.addr_len = 128;
	ca_kh_ip_address_ntohl(&l3_intf.ip_addr, &l3_intf.ip_addr);
	memcpy(&l3_intf.mac_addr, &parent_l3_intf.mac_addr, sizeof(ca_mac_addr_t));

	l3_intf.mask.port_id = 1;
	l3_intf.mask.tunnel_id = 1;
	l3_intf.mask.outer_vid = parent_l3_intf.mask.outer_vid;
	l3_intf.mask.ip_addr = 1;
	l3_intf.mask.mac_addr = 1;

	ca_kh_decide_nat_enable(&l3_intf);

	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: Add L3 interface intf.intf_id=%d\n", __func__, l3_intf.intf_id);
	}

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV6, ifindex, l3_intf.intf_id, CA_TUNNEL_DSLITE);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_set_intf_id(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

#if 0
	/* add static nexthop to tunnel */

	memset(&l3_nexthop, 0, sizeof(ca_l3_nexthop_t));
	l3_nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;
	l3_nexthop.intf_id = l3_intf.intf_id;
	// l3_nexthop.addr = ?
	// l3_nexthop.da_mac = ?
	ret = ca_l3_nexthop_add(device_id, &l3_nexthop, &l3_nexthop_id);
	if (ret != CA_E_OK) {
		if (ret != CA_E_EXISTS) {
			printk("%s: ca_l3_nexthop_add(), return failed! ret=%d\n", __func__, ret);
		}
		return ret;
	}

	kh_nexthop.ip_addr = l3_nexthop.addr;
	kh_nexthop.ifindex = ifindex;
	kh_nexthop.l3_nexthop_id = l3_nexthop_id;
	kh_nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;
	ret = ca_kh_l3_nexthop_add(device_id, &kh_nexthop);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_nexthop_add() return failed! ret=%d\n", __func__, ret);
		return ret;
	}
#endif
	return ret;
}

ca_status_t ca_kh_dslite_delete_intf_only(ca_device_id_t device_id, int ifindex)
{
	int ret;
	int tunnel_id;
	ca_l3_intf_t l3_intf;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: ifindex=%d\n", __func__, ifindex);
	}

	l3_intf.ip_addr.afi = CA_IPV6;

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, &(l3_intf.intf_id), CA_TUNNEL_DSLITE);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_delete(), return failed! ret=%d\n", __func__, ret);
	}

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV6, 0, l3_intf.intf_id, CA_TUNNEL_DSLITE);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_set_intf_id(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	return ret;
}

ca_status_t ca_kh_dslite_delete(ca_device_id_t device_id, int ifindex, ca_tunnel_id_t kept_tunnel_id)
{
	int ret;
	int tunnel_id;
	ca_l3_intf_t l3_intf;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: ifindex=%d\n", __func__, ifindex);
	}

	l3_intf.ip_addr.afi = CA_IPV6;

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV6, ifindex, &(l3_intf.intf_id), CA_TUNNEL_DSLITE);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		/* interface may be deleted already */

		ret = ca_tunnel_delete(device_id, kept_tunnel_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
			return ret;
		}

		ret = ca_kh_l3_tunnel_delete(device_id, kept_tunnel_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
			return ret;
		}
	} else {
		tunnel_id = l3_intf.tunnel_id;

		ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_l3_intf_delete(), return failed! ret=%d\n", __func__, ret);
		}

		ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV6, 0, l3_intf.intf_id, CA_TUNNEL_DSLITE);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_tunnel_set_intf_id(), return failed! ret=%d\n", __func__, ret);
		}

		ret = ca_tunnel_delete(device_id, tunnel_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
		}

		ret = ca_kh_l3_tunnel_delete(device_id, tunnel_id);
		if (ret != CA_E_OK) {
			printk("%s: ca_kh_l3_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
		}
	}

	return ret;
}

