#include <linux/netdevice.h>
#include <net/ip_tunnels.h>

#include "ca_kernel_hook_util.h"
#include <soc/cortina/ca_kernel_hook.h>

typedef struct ca_l3_user_data {
	ca_intf_id_t	intf_id;
	ca_ip_afi_t	afi;
} ca_l3_user_data_t;

ca_status_t ca_kh_6rd_add(ca_device_id_t device_id, int ifindex, struct ip_tunnel *t)
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
	struct net_device *dev = NULL, *parent_dev = NULL;
	ca_l3_intf_type_t parent_intf_type;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: ifindex=%d, parent_ifindex=%d\n", __func__, ifindex, t->parms.link);
		printk("%s: local ipv4_addr=%pI4\n", __func__, &t->parms.iph.saddr);
		printk("%s: remote ipv4_addr=%pI4\n", __func__, &t->parms.iph.daddr);
		printk("%s: ttl=%u, id=0x%04x, tos=%u, frag_off=0x%04x\n",
			__func__, t->parms.iph.ttl, t->parms.iph.id, t->parms.iph.tos, t->parms.iph.frag_off);
	}

	/* get parent intf_id and its intf */

	parent_dev = dev_get_by_index(&init_net, t->parms.link);
	if (parent_dev == NULL) {
		printk("%s: Fail to get parent_dev by ifindex %d\n", __func__, t->parms.link);
		return CA_E_PARAM;
	}
	dev_put(parent_dev);

	if (parent_dev->flags & IFF_POINTOPOINT)
		parent_intf_type = CA_L3_INTF_TUNNEL;
	else
		parent_intf_type = CA_L3_INTF_BCAST;

	parent_l3_intf.ip_addr.afi = CA_IPV4;
	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, parent_l3_intf.ip_addr.afi, t->parms.link, parent_intf_type, &parent_l3_intf.intf_id);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: ca_kh_l3_intf_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		}
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: parent_l3_intf.intf_id=%d\n", __func__, parent_l3_intf.intf_id);
	}

	ret = ca_l3_intf_get(device_id, &parent_l3_intf);
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

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, &l3_intf.intf_id, CA_TUNNEL_6RD);
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
	tunnel_cfg.type = CA_TUNNEL_6RD;
	tunnel_cfg.parent_l3_intf_id = parent_l3_intf.intf_id;

	tunnel_cfg.dest_addr.afi = CA_IPV4;
	memcpy(&(tunnel_cfg.dest_addr.ip_addr.ipv4_addr), &t->parms.iph.daddr, sizeof(ca_uint32_t));
	tunnel_cfg.dest_addr.addr_len = 32;
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	tunnel_cfg.src_addr.afi = CA_IPV4;
	memcpy(&(tunnel_cfg.src_addr.ip_addr.ipv4_addr), &t->parms.iph.saddr, sizeof(ca_uint32_t));
	tunnel_cfg.src_addr.addr_len = 32;
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	tunnel_cfg.tunnel.six_rd.mc_drop = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ipmc_illegal_check_en = 0;
	tunnel_cfg.tunnel.six_rd.rebuild_mc_mac = 1;
	tunnel_cfg.tunnel.six_rd.mc_mac[0] = 0x33;
	tunnel_cfg.tunnel.six_rd.mc_mac[1] = 0x33;
	tunnel_cfg.tunnel.six_rd.mc_mac[2] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[3] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[4] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[5] = 0x00;

	memcpy(&tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[0], &t->ip6rd.prefix, sizeof(ca_l3_ip_addr_t));
	tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.afi = CA_IPV6;
	tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.addr_len = t->ip6rd.prefixlen;

	tunnel_cfg.tunnel.six_rd.six_rd_v4_mask_len = 8; // TBD
	tunnel_cfg.tunnel.six_rd.six_rd_v6_prefix_len = t->ip6rd.prefixlen;
	tunnel_cfg.tunnel.six_rd.six_rd_ipda_from_v6 = (t->parms.iph.daddr == 0) ? 1 : 0;

	tunnel_cfg.tunnel.six_rd.six_rd_ingress_check_en = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ipsa_match = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ecn_check_enable = 0;
	tunnel_cfg.tunnel.six_rd.ce_ce_allowed = 0;
	tunnel_cfg.tunnel.six_rd.validation_check_failed_dest_port = 0x1f;

	tunnel_cfg.tunnel.six_rd.six_rd_ttl_keep_outer = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_dscp_keep_outer = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ecn_keep_outer = 0;

	tunnel_cfg.tunnel.six_rd.egress_tos_value = t->parms.iph.tos;
	tunnel_cfg.tunnel.six_rd.egress_ttl_value = (t->parms.iph.ttl != 0) ? t->parms.iph.ttl : 64;
	tunnel_cfg.tunnel.six_rd.egress_identification_start = 0;
	tunnel_cfg.tunnel.six_rd.egress_identification_end = 65535;

	ret = ca_tunnel_add(device_id, &tunnel_cfg, &tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_tunnel_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: tunnel_id=%d\n", __func__, tunnel_id);
	}

	/* record the added tunnel info */

	kh_tunnel.type = CA_TUNNEL_6RD;
	kh_tunnel.dest_addr.afi = CA_IPV4;
	kh_tunnel.dest_addr.addr_len = 32;
	memcpy(&(kh_tunnel.dest_addr.ip_addr.ipv4_addr), &t->parms.iph.daddr, sizeof(ca_uint32_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	kh_tunnel.src_addr.afi = CA_IPV4;
	kh_tunnel.src_addr.addr_len = 32;
	memcpy(&(kh_tunnel.src_addr.ip_addr.ipv4_addr), &t->parms.iph.saddr, sizeof(ca_uint32_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	kh_tunnel.tunnel_id = tunnel_id;
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
	l3_intf.ip_addr.afi = CA_IPV4;
	memcpy(&(l3_intf.ip_addr.ip_addr.ipv6_addr[0]), &t->parms.iph.saddr, sizeof(ca_l3_ip_addr_t));
	l3_intf.ip_addr.addr_len = 32;
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

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV4, ifindex, l3_intf.intf_id, CA_TUNNEL_6RD);
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

/* only update tunnel self */
ca_status_t ca_kh_6rd_update(ca_device_id_t device_id, int ifindex, struct ip_tunnel *t)
{
	int ret;
#if 0
	ca_l3_nexthop_t l3_nexthop;
	ca_l3_nexthop_id_t l3_nexthop_id;
	ca_kh_nexthop_entry_t kh_nexthop;
#endif
	ca_tunnel_cfg_t tunnel_cfg;
	ca_l3_intf_t l3_intf, parent_l3_intf;
	ca_kh_tunnel_entry_t kh_tunnel;
	struct net_device *dev = NULL, *parent_dev = NULL;
	ca_l3_intf_type_t parent_intf_type;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: ifindex=%d, parent_ifindex=%d\n", __func__, ifindex, t->parms.link);
		printk("%s: local ipv4_addr=%pI4\n", __func__, &t->parms.iph.saddr);
		printk("%s: remote ipv4_addr=%pI4\n", __func__, &t->parms.iph.daddr);
		printk("%s: ttl=%u, id=0x%04x, tos=%u, frag_off=0x%04x\n",
			__func__, t->parms.iph.ttl, t->parms.iph.id, t->parms.iph.tos, t->parms.iph.frag_off);
	}

	/* get parent intf_id and its intf */

	parent_dev = dev_get_by_index(&init_net, t->parms.link);
	if (parent_dev == NULL) {
		printk("%s: Fail to get parent_dev by ifindex %d\n", __func__, t->parms.link);
		return CA_E_PARAM;
	}
	dev_put(parent_dev);

	if (parent_dev->flags & IFF_POINTOPOINT)
		parent_intf_type = CA_L3_INTF_TUNNEL;
	else
		parent_intf_type = CA_L3_INTF_BCAST;

	parent_l3_intf.ip_addr.afi = CA_IPV4;
	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, parent_l3_intf.ip_addr.afi, t->parms.link, parent_intf_type, &parent_l3_intf.intf_id);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: ca_kh_l3_intf_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		}
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: parent_l3_intf.intf_id=%d\n", __func__, parent_l3_intf.intf_id);
	}

	ret = ca_l3_intf_get(device_id, &parent_l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(intf_id=%d), return failed! ret=%d\n", __func__, parent_l3_intf.intf_id, ret);
		return ret;
	}

	/* search intf_id for tunnel interface */

	memset(&l3_intf, 0, sizeof(ca_l3_intf_t));

	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_E_PARAM;
	}
	l3_intf.mtu = dev->mtu;
	dev_put(dev);

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, &l3_intf.intf_id, CA_TUNNEL_6RD);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: ca_kh_l3_tunnel_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		}

		if(ret != CA_E_NOT_FOUND)
			return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: l3_intf.intf_id=%d\n", __func__, l3_intf.intf_id);
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(intf_id=%d), return failed! ret=%d\n", __func__, parent_l3_intf.intf_id, ret);
		return ret;
	}

	if (l3_intf.type != CA_L3_INTF_TUNNEL) {
		printk("%s: ca_l3_intf_get() doesn't return a tunnel interface.\n", __func__);
		return CA_E_ERROR;
	} else {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk("%s: Found intf. intf_id=%d, tunnel_id=%d.\n", __func__, l3_intf.intf_id, l3_intf.tunnel_id);
		}
	}

	ret = ca_tunnel_get(device_id, l3_intf.tunnel_id, &tunnel_cfg);
	if (ret != CA_E_OK) {
		printk("%s: ca_tunnel_get(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	if (tunnel_cfg.type != CA_TUNNEL_6RD) {
		printk("%s: ca_tunnel_get() doesn't return 6RD tunnel. tunnel_id=%d\n", __func__, l3_intf.tunnel_id);
		return CA_E_ERROR;
	}

	/* update tunnel_cfg */

	tunnel_cfg.type = CA_TUNNEL_6RD;
	tunnel_cfg.parent_l3_intf_id = parent_l3_intf.intf_id;

	tunnel_cfg.dest_addr.afi = CA_IPV4;
	memcpy(&(tunnel_cfg.dest_addr.ip_addr.ipv4_addr), &t->parms.iph.daddr, sizeof(ca_uint32_t));
	tunnel_cfg.dest_addr.addr_len = 32;
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	tunnel_cfg.src_addr.afi = CA_IPV4;
	memcpy(&(tunnel_cfg.src_addr.ip_addr.ipv4_addr), &t->parms.iph.saddr, sizeof(ca_uint32_t));
	tunnel_cfg.src_addr.addr_len = 32;
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	tunnel_cfg.tunnel.six_rd.mc_drop = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ipmc_illegal_check_en = 0;
	tunnel_cfg.tunnel.six_rd.rebuild_mc_mac = 1;
	tunnel_cfg.tunnel.six_rd.mc_mac[0] = 0x33;
	tunnel_cfg.tunnel.six_rd.mc_mac[1] = 0x33;
	tunnel_cfg.tunnel.six_rd.mc_mac[2] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[3] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[4] = 0x00;
	tunnel_cfg.tunnel.six_rd.mc_mac[5] = 0x00;

	memcpy(&tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[0], &t->ip6rd.prefix, sizeof(ca_l3_ip_addr_t));
	tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.afi = CA_IPV6;
	tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix.addr_len = t->ip6rd.prefixlen;
	ca_kh_ip_address_ntohl(&tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix, &tunnel_cfg.tunnel.six_rd.ipv6_6rd_prefix);

	tunnel_cfg.tunnel.six_rd.six_rd_v4_mask_len = 8; // TBD
	tunnel_cfg.tunnel.six_rd.six_rd_v6_prefix_len = t->ip6rd.prefixlen;
	tunnel_cfg.tunnel.six_rd.six_rd_ipda_from_v6 = (t->parms.iph.daddr == 0) ? 1 : 0;

	tunnel_cfg.tunnel.six_rd.six_rd_ingress_check_en = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ipsa_match = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ecn_check_enable = 0;
	tunnel_cfg.tunnel.six_rd.ce_ce_allowed = 0;
	tunnel_cfg.tunnel.six_rd.validation_check_failed_dest_port = 0x1f;

	tunnel_cfg.tunnel.six_rd.six_rd_ttl_keep_outer = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_dscp_keep_outer = 0;
	tunnel_cfg.tunnel.six_rd.six_rd_ecn_keep_outer = 0;

	tunnel_cfg.tunnel.six_rd.egress_tos_value = t->parms.iph.tos;
	tunnel_cfg.tunnel.six_rd.egress_ttl_value = (t->parms.iph.ttl != 0) ? t->parms.iph.ttl : 64;
	tunnel_cfg.tunnel.six_rd.egress_identification_start = 0;
	tunnel_cfg.tunnel.six_rd.egress_identification_end = 65535;

	ret = ca_tunnel_update(device_id, &tunnel_cfg, l3_intf.tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_tunnel_update(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: tunnel_id=%d\n", __func__, l3_intf.tunnel_id);
	}

	/* delete old tunnel info, add new tunnel info */

	ret = ca_kh_l3_tunnel_delete(device_id, l3_intf.tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnl_delete(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	kh_tunnel.type = CA_TUNNEL_6RD;
	kh_tunnel.dest_addr.afi = CA_IPV4;
	kh_tunnel.dest_addr.addr_len = 32;
	memcpy(&(kh_tunnel.dest_addr.ip_addr.ipv4_addr), &t->parms.iph.daddr, sizeof(ca_uint32_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.dest_addr, &tunnel_cfg.dest_addr);

	kh_tunnel.src_addr.afi = CA_IPV4;
	kh_tunnel.src_addr.addr_len = 32;
	memcpy(&(kh_tunnel.src_addr.ip_addr.ipv4_addr), &t->parms.iph.saddr, sizeof(ca_uint32_t));
	ca_kh_ip_address_ntohl(&tunnel_cfg.src_addr, &tunnel_cfg.src_addr);

	kh_tunnel.tunnel_id = l3_intf.tunnel_id;
	ret = ca_kh_l3_tunnel_add(device_id, &kh_tunnel);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnl_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

#if 0
	/* add tunnel interface */

	l3_intf.type = CA_L3_INTF_TUNNEL;
	l3_intf.port_id = parent_l3_intf.port_id;
	l3_intf.tunnel_id = tunnel_id;
	l3_intf.outer_vid = parent_l3_intf.outer_vid;
	l3_intf.ip_addr.afi = CA_IPV4;
	memcpy(&(l3_intf.ip_addr.ip_addr.ipv6_addr[0]), &t->parms.iph.saddr, sizeof(ca_l3_ip_addr_t));
	l3_intf.ip_addr.addr_len = 32;
	ca_kh_ip_address_ntohl(&l3_intf.ip_addr, &l3_intf.ip_addr);
	memcpy(&l3_intf.mac_addr, &parent_l3_intf.mac_addr, sizeof(ca_mac_addr_t));

	l3_intf.mask.outer_vid = 1;

	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_add(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: Add L3 interface intf.intf_id=%d\n", __func__, l3_intf.intf_id);
	}

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV4, ifindex, l3_intf.intf_id, CA_TUNNEL_6RD);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_set_intf_id(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}
#endif

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

ca_status_t ca_kh_6rd_delete(ca_device_id_t device_id, int ifindex)
{
	int ret;
	int tunnel_id;
	ca_l3_intf_t l3_intf;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: ifindex=%d\n", __func__, ifindex);
	}

	l3_intf.ip_addr.afi = CA_IPV4;

	ret = ca_kh_l3_tunnel_get_intf_id_by_ifindex(device_id, CA_IPV4, ifindex, &(l3_intf.intf_id), CA_TUNNEL_6RD);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_get_intf_id_by_ifindex(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_get(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	tunnel_id = l3_intf.tunnel_id;
	ret = ca_tunnel_delete(device_id, tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_kh_l3_tunnel_delete(device_id, tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_delete(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
	if (ret != CA_E_OK) {
		printk("%s: ca_l3_intf_delete(), return failed! ret=%d\n", __func__, ret);
	}

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, CA_IPV4, 0, l3_intf.intf_id, CA_TUNNEL_6RD);
	if (ret != CA_E_OK) {
		printk("%s: ca_kh_l3_tunnel_set_intf_id(), return failed! ret=%d\n", __func__, ret);
		return ret;
	}

	return ret;
}

