#include <net/addrconf.h>
#include <linux/inetdevice.h>

#include "ca_kernel_hook_util.h"
#include <soc/cortina/ca_kernel_hook.h>
#include <net/netfilter/nf_conntrack_tuple.h>

typedef struct ca_l3_user_data {
	ca_uint8_t          ip_proto;           /* IP protocol, e.g. TCP or UDP */
	ca_ip_address_t     src_ip_addr;        /* Source IP address */
	ca_ip_address_t     dst_ip_addr;        /* Destination IP address */
	ca_uint16_t         l4_src_port;        /* Source layer 4 port */
	ca_uint16_t         l4_dst_port;        /* Destination Layer 4 port */
} ca_l3_user_data_t;

//ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);

typedef int (*ca_nat_entry_traverse_cb) (ca_device_id_t device_id, ca_nat_entry_t *session, void *user_data);
static ca_status_t ca_nat_entry_traverse(ca_device_id_t device_id, ca_nat_entry_traverse_cb fn, void *user_data);
ca_boolean_t ca_l4_port_bypass_is_needed(ca_uint8_t ip_proto, u16 src_port ,u16 dst_port);

#if 0
static ca_boolean_t is_ip_broadcast_or_host(ca_int16_t ifindex, ca_uint32_t old_dst_ip)
{
	int i, ret;
	ca_intf_id_t    intf_id;
	ca_l3_intf_t    l3_intf;
	ca_uint32_t     net_mask;
	struct net_device *dev = NULL;
	unsigned int dev_flags = 0;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: ifindex=%d, old_dst_ip=0x%x\n", __func__, ifindex, old_dst_ip);
	}

        /* filter out broadcast or to host packets */
	dev = dev_get_by_index(&init_net, ifindex);
	if (dev == NULL) {
		printk("%s: Fail to get dev by ifindex %d\n", __func__, ifindex);
		return CA_FALSE;
	}
	dev_flags = dev->flags;
	dev_put(dev);

	if (dev_flags & IFF_BROADCAST)
		ca_kh_l3_get_intf_id_by_ifindex(0, CA_IPV4, ifindex, CA_L3_INTF_BCAST, &intf_id);
	else if (dev_flags & IFF_POINTOPOINT)
		ca_kh_l3_get_intf_id_by_ifindex(0, CA_IPV4, ifindex, CA_L3_INTF_TUNNEL, &intf_id);
        l3_intf.ip_addr.afi = CA_IPV4;
        l3_intf.intf_id = intf_id;
        ret = ca_l3_intf_get(0, &l3_intf);
        if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
		}
                return CA_FALSE;
        }

	/* old_dst_ip is network order */
	if (l3_intf.ip_addr.ip_addr.ipv4_addr == ntohl(old_dst_ip)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: old_dst_ip=0x%x is a host ip address!!\n", __func__, old_dst_ip);
		}
		return CA_TRUE;
	}

        net_mask = 0;
        for (i = 0; i < l3_intf.ip_addr.addr_len; i++) {
               net_mask |= (1 << i);
        }
	net_mask = ~net_mask;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: old_dst_ip=0x%x, net_mask=0x%x\n", __func__, old_dst_ip, net_mask);
	}

	if ((old_dst_ip & net_mask) == net_mask) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: old_dst_ip=0x%x is a boardcast ip address!!\n", __func__, old_dst_ip);
		}
		return CA_TRUE;
	}
	return CA_FALSE;
}
#endif

/* Check if ipv4_addr is one of interface IP address.
 * Parameter:
 *   ipv4_addr: IPv4 address, network order.
 */
static ca_boolean_t is_myip_v4(ca_uint32_t ipv4_addr)
{
	struct net *net;
	struct net_device *dev;
	struct in_device *in_dev;
	struct in_ifaddr *ifa;

	for_each_net(net) {
		for_each_netdev(net, dev) {
			in_dev = __in_dev_get_rtnl(dev);
			if (in_dev == NULL)
				continue;

			ifa = in_dev->ifa_list;
			for (; ifa != NULL; ifa = ifa->ifa_next) {
				if (ifa->ifa_local == ipv4_addr)
					return CA_TRUE;
			}
		}
	}

	return CA_FALSE;
}

/* Check if ipv6_addr is one of interface IP address.
 * Parameter:
 *   ipv6_addr: IPv6 address, network order.
 */
static ca_boolean_t is_myip_v6(struct in6_addr *ipv6_addr)
{
	struct net *net;
	struct net_device *dev;
	struct inet6_dev *in6_dev;
	struct inet6_ifaddr *ifa6;

	if (ipv6_addr == NULL)
		return CA_TRUE;

	for_each_net(net) {
		for_each_netdev(net, dev) {
			in6_dev = __in6_dev_get(dev);
			if (in6_dev == NULL)
				continue;

			list_for_each_entry(ifa6, &in6_dev->addr_list, if_list) {
				if (memcmp(&ifa6->addr, ipv6_addr, sizeof(struct in6_addr)) == 0)
					return CA_TRUE;
			}
		}
	}

	return CA_FALSE;
}

ca_status_t ca_kh_nat_entry_add(ca_device_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, ca_uint32_t old_src_ip, ca_uint32_t new_src_ip,
				ca_uint32_t old_dst_ip, ca_uint32_t new_dst_ip, ca_uint16_t old_src_port, ca_uint16_t new_src_port,
				ca_uint16_t old_dst_port, ca_uint16_t new_dst_port, ca_int16_t ifindex)
{
	int ret;
	ca_boolean_t is_l4_port_bypass;
	ca_nat_entry_t	nat_entry;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: ip_proto=%u, is_trans_src_ip=%d, old_src_ip=%pI4, new_src_ip=%pI4, old_dst_ip=%pI4, new_dst_ip=%pI4\n",
			__func__, ip_proto, is_trans_src_ip, &old_src_ip, &new_src_ip, &old_dst_ip, &new_dst_ip);
		printk("%s: old_src_port=%d, new_src_port=%d, old_dst_port=%d, new_dst_port=%d\n",
			__func__, ntohs(old_src_port), ntohs(new_src_port), ntohs(old_dst_port), ntohs(new_dst_port));
	}

	if ((is_myip_v4(old_src_ip) == CA_TRUE) || (is_myip_v4(new_dst_ip) == CA_TRUE)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: Found MyIP, ignore this entry. (old_src_ip=%pI4, new_dst_ip=%pI4)\n", __func__, &old_src_ip, &new_dst_ip);
		}
		return CA_E_NOT_SUPPORT;
	}

#if 0
	/* check whether the src/dst ip is broadcast or host */
	if (is_ip_broadcast_or_host(ifindex, old_src_ip) == CA_TRUE || is_ip_broadcast_or_host(ifindex, old_dst_ip) == CA_TRUE) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: old_dst_ip=0x%x or old_src_ip=0x%x is a broadcast ip address!!\n", __func__, old_dst_ip, old_src_ip);
		}
		return CA_E_ERROR;
	}
#endif

	memset(&nat_entry, 0, sizeof(ca_nat_entry_t));
	nat_entry.aging_timer = 300; /* set any non-zero for aging */
	nat_entry.src_ip_addr.afi = CA_IPV4;
	nat_entry.src_ip_addr.addr_len = 32;
	nat_entry.dst_ip_addr.afi = CA_IPV4;
	nat_entry.dst_ip_addr.addr_len = 32;
	nat_entry.new_src_ip_addr.afi = CA_IPV4;
	nat_entry.new_src_ip_addr.addr_len = 32;
	nat_entry.new_dst_ip_addr.afi = CA_IPV4;
	nat_entry.new_dst_ip_addr.addr_len = 32;
	nat_entry.ip_proto = ip_proto;

	is_l4_port_bypass = ca_l4_port_bypass_is_needed(nat_entry.ip_proto, ntohs(old_src_port), ntohs(old_dst_port));

	if(is_l4_port_bypass) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: bypass adding hw nat entry\n", __func__);
		}
		return CA_E_ERROR;
	}

	/* the L4 source/destination port should be convert to host order also */
	/* only need to change source ip/port when SNAT, and dest ip/port when DNAT */
	if (is_trans_src_ip == 0) {
		nat_entry.xlate_flags = CA_NAT_TRANSLATE_DST_IP;
	}
	else if (is_trans_src_ip == 1) {
		nat_entry.xlate_flags = CA_NAT_TRANSLATE_SRC_IP;
	}
        else if (is_trans_src_ip == 2) {
                nat_entry.xlate_flags = CA_NAT_TRANSLATE_SRC_IP | CA_NAT_TRANSLATE_DST_IP;
        }
        else {
                printk("%s: Not support mode %d!!!\n", __func__, is_trans_src_ip);
                return CA_E_OK;
        }
	nat_entry.src_ip_addr.ip_addr.ipv4_addr = old_src_ip;
	nat_entry.src_l4_port = ntohs(old_src_port);
	nat_entry.dst_ip_addr.ip_addr.ipv4_addr = old_dst_ip;
	nat_entry.dst_l4_port = ntohs(old_dst_port);
	nat_entry.new_src_ip_addr.ip_addr.ipv4_addr = new_src_ip;
	nat_entry.new_src_l4_port = ntohs(new_src_port);
	nat_entry.new_dst_ip_addr.ip_addr.ipv4_addr = new_dst_ip;
	nat_entry.new_dst_l4_port = ntohs(new_dst_port);

	if ((ip_proto == IPPROTO_TCP) || (ip_proto == IPPROTO_UDP))
		nat_entry.xlate_flags |= CA_NAT_TRANSLATE_L4_PORT;


	/* Cortina API accepts host order IP address */
        ca_kh_ip_address_ntohl(&nat_entry.src_ip_addr, &nat_entry.src_ip_addr);
        ca_kh_ip_address_ntohl(&nat_entry.dst_ip_addr, &nat_entry.dst_ip_addr);
        ca_kh_ip_address_ntohl(&nat_entry.new_src_ip_addr, &nat_entry.new_src_ip_addr);
        ca_kh_ip_address_ntohl(&nat_entry.new_dst_ip_addr, &nat_entry.new_dst_ip_addr);

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: host order old src IP address=%pI4\n", __func__, &nat_entry.src_ip_addr.ip_addr.ipv4_addr);
		printk("%s: host order old dst IP address=%pI4\n", __func__, &nat_entry.dst_ip_addr.ip_addr.ipv4_addr);
		printk("%s: host order new src IP address=%pI4\n", __func__, &nat_entry.new_src_ip_addr.ip_addr.ipv4_addr);
		printk("%s: host order new dst IP address=%pI4\n", __func__, &nat_entry.new_dst_ip_addr.ip_addr.ipv4_addr);
	}

	ret = ca_nat_entry_add(0, &nat_entry);

	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: ca_nat_entry_add(), return failed!!, ret=%d\n", __func__, ret);
		}
	}
	return ret;
}

ca_status_t ca_kh_nat6_entry_add(ca_device_id_t device_id, uint8_t ip_proto, int is_trans_src_ip, ca_uint32_t *old_src_ip, ca_uint32_t *new_src_ip,
				ca_uint32_t *old_dst_ip, ca_uint32_t *new_dst_ip, ca_uint16_t old_src_port, ca_uint16_t new_src_port,
				ca_uint16_t old_dst_port, ca_uint16_t new_dst_port, ca_int16_t ifindex)
{
	int ret;
	ca_nat_entry_t	nat_entry;
	ca_boolean_t is_l4_port_bypass;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: ip_proto=%u, is_trans_src_ip=%d, old_src_ip=%pI6c, new_src_ip=%pI6c, old_dst_ip=%pI6c, new_dst_ip=%pI6c\n",
			__func__, ip_proto, is_trans_src_ip, old_src_ip, new_src_ip, old_dst_ip, new_dst_ip);
		printk("%s: old_src_port=%d, new_src_port=%d, old_dst_port=%d, new_dst_port=%d\n",
			__func__, ntohs(old_src_port), ntohs(new_src_port), ntohs(old_dst_port), ntohs(new_dst_port));
	}

	if ((is_myip_v6((struct in6_addr *) old_src_ip) == CA_TRUE) || (is_myip_v6((struct in6_addr *) new_dst_ip) == CA_TRUE)) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: Found MyIP, ignore this entry. (old_src_ip=%pI6c, new_dst_ip=%pI6c)\n", __func__, old_src_ip, new_dst_ip);
		}
		return CA_E_NOT_SUPPORT;
	}

#if 0
	/* check whether the src/dst ip is broadcast or host */
	if (is_ip_broadcast_or_host(ifindex, old_src_ip) == CA_TRUE || is_ip_broadcast_or_host(ifindex, old_dst_ip) == CA_TRUE) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: old_dst_ip=0x%x or old_src_ip=0x%x is a broadcast ip address!!\n", __func__, old_dst_ip, old_src_ip);
		}
		return CA_E_ERROR;
	}
#endif

	memset(&nat_entry, 0, sizeof(ca_nat_entry_t));
	nat_entry.aging_timer = 300; /* set any non-zero for aging */
	nat_entry.src_ip_addr.afi = CA_IPV6;
	nat_entry.src_ip_addr.addr_len = 128;
	nat_entry.dst_ip_addr.afi = CA_IPV6;
	nat_entry.dst_ip_addr.addr_len = 128;
	nat_entry.new_src_ip_addr.afi = CA_IPV6;
	nat_entry.new_src_ip_addr.addr_len = 128;
	nat_entry.new_dst_ip_addr.afi = CA_IPV6;
	nat_entry.new_dst_ip_addr.addr_len = 128;
	nat_entry.ip_proto = ip_proto;

	is_l4_port_bypass = ca_l4_port_bypass_is_needed(nat_entry.ip_proto, ntohs(old_src_port), ntohs(old_dst_port));

	if(is_l4_port_bypass) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: bypass adding hw nat entry\n", __func__);
		}
		return CA_E_ERROR;
	}

	/* the L4 source/destination port should be convert to host order also */
	/* only need to change source ip/port when SNAT, and dest ip/port when DNAT */
	if (is_trans_src_ip == 0) {
		nat_entry.xlate_flags = CA_NAT_TRANSLATE_DST_IP;
	}
	else if (is_trans_src_ip == 1) {
		nat_entry.xlate_flags = CA_NAT_TRANSLATE_SRC_IP;
	}
	else if (is_trans_src_ip == 2) {
		nat_entry.xlate_flags = CA_NAT_TRANSLATE_SRC_IP | CA_NAT_TRANSLATE_DST_IP;
	}
	else {
		printk("%s: Not support mode %d!!!\n", __func__, is_trans_src_ip);
		return CA_E_OK;
	}
	nat_entry.src_ip_addr.ip_addr.ipv6_addr[0] = old_src_ip[0];
	nat_entry.src_ip_addr.ip_addr.ipv6_addr[1] = old_src_ip[1];
	nat_entry.src_ip_addr.ip_addr.ipv6_addr[2] = old_src_ip[2];
	nat_entry.src_ip_addr.ip_addr.ipv6_addr[3] = old_src_ip[3];
	nat_entry.src_l4_port = ntohs(old_src_port);
	nat_entry.dst_ip_addr.ip_addr.ipv6_addr[0] = old_dst_ip[0];
	nat_entry.dst_ip_addr.ip_addr.ipv6_addr[1] = old_dst_ip[1];
	nat_entry.dst_ip_addr.ip_addr.ipv6_addr[2] = old_dst_ip[2];
	nat_entry.dst_ip_addr.ip_addr.ipv6_addr[3] = old_dst_ip[3];
	nat_entry.dst_l4_port = ntohs(old_dst_port);
	nat_entry.new_src_ip_addr.ip_addr.ipv6_addr[0] = new_src_ip[0];
	nat_entry.new_src_ip_addr.ip_addr.ipv6_addr[1] = new_src_ip[1];
	nat_entry.new_src_ip_addr.ip_addr.ipv6_addr[2] = new_src_ip[2];
	nat_entry.new_src_ip_addr.ip_addr.ipv6_addr[3] = new_src_ip[3];
	nat_entry.new_src_l4_port = ntohs(new_src_port);
	nat_entry.new_dst_ip_addr.ip_addr.ipv6_addr[0] = new_dst_ip[0];
	nat_entry.new_dst_ip_addr.ip_addr.ipv6_addr[1] = new_dst_ip[1];
	nat_entry.new_dst_ip_addr.ip_addr.ipv6_addr[2] = new_dst_ip[2];
	nat_entry.new_dst_ip_addr.ip_addr.ipv6_addr[3] = new_dst_ip[3];
	nat_entry.new_dst_l4_port = ntohs(new_dst_port);

	if ((ip_proto == IPPROTO_TCP) || (ip_proto == IPPROTO_UDP))
		nat_entry.xlate_flags |= CA_NAT_TRANSLATE_L4_PORT;

	/* Cortina API accepts host order IP address */
	ca_kh_ip_address_ntohl(&nat_entry.src_ip_addr, &nat_entry.src_ip_addr);
	ca_kh_ip_address_ntohl(&nat_entry.dst_ip_addr, &nat_entry.dst_ip_addr);
	ca_kh_ip_address_ntohl(&nat_entry.new_src_ip_addr, &nat_entry.new_src_ip_addr);
	ca_kh_ip_address_ntohl(&nat_entry.new_dst_ip_addr, &nat_entry.new_dst_ip_addr);

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: host order old src IP address=%pI6\n", __func__, &nat_entry.src_ip_addr.ip_addr.ipv6_addr);
		printk("%s: host order old dst IP address=%pI6\n", __func__, &nat_entry.dst_ip_addr.ip_addr.ipv6_addr);
		printk("%s: host order new src IP address=%pI6\n", __func__, &nat_entry.new_src_ip_addr.ip_addr.ipv6_addr);
		printk("%s: host order new dst IP address=%pI6\n", __func__, &nat_entry.new_dst_ip_addr.ip_addr.ipv6_addr);
	}

	ret = ca_nat_entry_add(0, &nat_entry);

	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: ca_nat_entry_add(), return failed!!, ret=%d\n", __func__, ret);
		}
	}
	return ret;
}

static int ca_kh_nat_entry_traverse_cb(ca_device_id_t device_id, ca_nat_entry_t *session, void *user_data)
{
	int ret;
	ca_ip_address_t ip_addr;

	memcpy(&ip_addr, (char *)user_data, sizeof(ca_ip_address_t));

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		if (session->src_ip_addr.afi == CA_IPV4) {
			printk("%s: session->src_ip_addr.ip_addr.ipv4_add=%pI4 / %d\n",
				__func__, &session->src_ip_addr.ip_addr.ipv4_addr, session->src_ip_addr.addr_len);
		} else {
			printk("%s: session->src_ip_addr.ip_addr.ipv4_add=%pI6 / %d\n",
				__func__, &session->src_ip_addr.ip_addr.ipv6_addr, session->src_ip_addr.addr_len);
		}
		if (session->dst_ip_addr.afi == CA_IPV4) {
			printk("%s: session->dst_ip_addr.ip_addr.ipv4_add=%pI4 / %d\n",
				__func__, &session->dst_ip_addr.ip_addr.ipv4_addr, session->dst_ip_addr.addr_len);
		} else {
			printk("%s: session->dst_ip_addr.ip_addr.ipv4_add=%pI6 / %d\n",
				__func__, &session->dst_ip_addr.ip_addr.ipv6_addr, session->dst_ip_addr.addr_len);
		}
	}

	if (ca_kh_is_ip_equal(&(session->src_ip_addr), &ip_addr) || ca_kh_is_ip_equal(&(session->dst_ip_addr), &ip_addr)) {

		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: This session should be deleted!!\n", __func__);
		}
		ret = ca_nat_entry_delete(device_id, session);

		if (ret != CA_E_OK) {
			printk("%s: ca_nat_entry_delete(), return failed!!, ret=%d\n", __func__, ret);
		}
		return ret;
	}
	return CA_E_OK;
}

ca_status_t ca_kh_nat_entry_delete(ca_device_id_t device_id, ca_uint32_t ipv4_addr)
{
	ca_uint32_t ipv4_addr_host = ntohl(ipv4_addr);
	ca_ip_address_t ip_addr;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: ipv4_addr=%pI4\n", __func__, &ipv4_addr);
		printk("%s: ipv4_addr_host=%pI4\n", __func__, &ipv4_addr_host);
	}

	ip_addr.afi = CA_IPV4;
	ip_addr.ip_addr.ipv4_addr = ipv4_addr_host;
	ip_addr.addr_len = 32;
	ca_nat_entry_traverse(device_id, ca_kh_nat_entry_traverse_cb, &ip_addr);
	return CA_E_OK;
}

ca_status_t ca_kh_nat6_entry_delete(ca_device_id_t device_id, ca_uint32_t *ipv6_addr)
{
	ca_ip_address_t ip_addr;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: ipv6_addr=%pI6\n", __func__, ipv6_addr);
	}

	ip_addr.afi = CA_IPV6;
	ip_addr.ip_addr.ipv6_addr[0] = ntohl(ipv6_addr[0]);
	ip_addr.ip_addr.ipv6_addr[1] = ntohl(ipv6_addr[1]);
	ip_addr.ip_addr.ipv6_addr[2] = ntohl(ipv6_addr[2]);
	ip_addr.ip_addr.ipv6_addr[3] = ntohl(ipv6_addr[3]);
	ip_addr.addr_len = 128;
	ca_nat_entry_traverse(device_id, ca_kh_nat_entry_traverse_cb, &ip_addr);
	return CA_E_OK;
}

ca_status_t ca_kh_nat_entry_ifdown(ca_device_id_t device_id, ca_ip_address_t *ip_addr)
{
	/* The input ip address already in host order */
	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->ip_addr.ipv4_addr=%pI4 / %u\n", __func__, &ip_addr->ip_addr.ipv4_addr, ip_addr->addr_len);
		} else {
			printk("%s: ip_addr->ip_addr.ipv4_addr=%pI6 / %u\n", __func__, &ip_addr->ip_addr.ipv6_addr, ip_addr->addr_len);
		}
	}

        ca_nat_entry_traverse(device_id, ca_kh_nat_entry_traverse_cb, ip_addr);
        return CA_E_OK;
}

#if 0
static int ca_kh_nat_entry_traverse_cb1(ca_device_id_t device_id, ca_nat_entry_t *session, void *user_data)
{
        int ret;
	ca_l3_user_data_t *l3_user_data = (ca_l3_user_data_t *)user_data;

        if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: l3_user_data->ip_proto=%u\n", __func__, l3_user_data->ip_proto);
		printk("%s: l3_user_data->src_ip_addr.ip_addr.ipv4_addr=%pI4\n", __func__, &(l3_user_data->src_ip_addr.ip_addr.ipv4_addr));
		printk("%s: l3_user_data->dst_ip_addr.ip_addr.ipv4_addr=%pI4\n", __func__, &(l3_user_data->dst_ip_addr.ip_addr.ipv4_addr));
		printk("%s: l3_user_data->l4_src_port=%d\n", __func__, l3_user_data->l4_src_port);
		printk("%s: l3_user_data->l4_dst_port=%d\n", __func__, l3_user_data->l4_dst_port);

                printk("%s: session->ip_proto=%u\n", __func__, session->ip_proto);
                printk("%s: session->src_ip_addr.afi=%d\n", __func__, session->src_ip_addr.afi);
                printk("%s: session->src_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session->src_ip_addr.ip_addr.ipv4_addr);
                printk("%s: session->src_ip_addr.addr_len=%d\n", __func__, session->src_ip_addr.addr_len);
                printk("%s: session->src_l4_port=%d\n", __func__, session->src_l4_port);
                printk("%s: session->dst_ip_addr.afi=%d\n", __func__, session->dst_ip_addr.afi);
                printk("%s: session->dst_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session->dst_ip_addr.ip_addr.ipv4_addr);
                printk("%s: session->dst_ip_addr.addr_len=%d\n", __func__, session->dst_ip_addr.addr_len);
                printk("%s: session->dst_l4_port=%d\n", __func__, session->dst_l4_port);
        }

        if ((l3_user_data->ip_proto == session->ip_proto) &&
		ca_kh_is_ip_equal(&(session->src_ip_addr), &(l3_user_data->src_ip_addr))&&  ca_kh_is_ip_equal(&(session->dst_ip_addr), &(l3_user_data->dst_ip_addr))
		&& (l3_user_data->l4_src_port == session->src_l4_port) && (l3_user_data->l4_dst_port == session->dst_l4_port)) {

                if (ca_kh_debug & CA_KH_DEBUG_NAT) {
                        printk("%s: This session should be deleted!!\n", __func__);
                }
                ret = ca_nat_entry_delete(device_id, session);

                if (ret != CA_E_OK) {
                        printk("%s: ca_nat_entry_delete(), return failed!!, ret=%d\n", __func__, ret);
                }
                return ret;
        }
        return CA_E_OK;
}
#endif

ca_status_t ca_kh_nat_entry_session_delete(ca_device_id_t device_id, uint8_t ip_proto, ca_uint32_t old_src_ip, ca_uint32_t old_dst_ip,
                               		   ca_uint16_t old_src_port, ca_uint16_t old_dst_port)
{
	int ret;
	ca_nat_entry_t session;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
                printk("%s: ip_proto=%u, old_src_ip=%pI4, old_dst_ip=%pI4\n", __func__, ip_proto, &old_src_ip, &old_dst_ip);
                printk("%s: old_src_port=%d, old_dst_port=%d\n", __func__, ntohs(old_src_port), ntohs(old_dst_port));
	}

	memset(&session, 0, sizeof(session));
	session.ip_proto = ip_proto;
	session.src_ip_addr.afi = CA_IPV4;
	session.src_ip_addr.ip_addr.ipv4_addr = ntohl(old_src_ip);
	session.src_ip_addr.addr_len = 32;
	session.dst_ip_addr.afi = CA_IPV4;
	session.dst_ip_addr.ip_addr.ipv4_addr = ntohl(old_dst_ip);
	session.dst_ip_addr.addr_len = 32;
	session.src_l4_port = ntohs(old_src_port);
	session.dst_l4_port = ntohs(old_dst_port);
	ret = ca_nat_entry_get(device_id, &session);
	if (ret == CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: session.ip_proto=%u\n", __func__, session.ip_proto);
			printk("%s: session.src_ip_addr.afi=%d\n", __func__, session.src_ip_addr.afi);
			printk("%s: session.src_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session.src_ip_addr.ip_addr.ipv4_addr);
			printk("%s: session.src_ip_addr.addr_len=%d\n", __func__, session.src_ip_addr.addr_len);
			printk("%s: session.src_l4_port=%d\n", __func__, session.src_l4_port);
			printk("%s: session.dst_ip_addr.afi=%d\n", __func__, session.dst_ip_addr.afi);
			printk("%s: session.dst_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session.dst_ip_addr.ip_addr.ipv4_addr);
			printk("%s: session.dst_ip_addr.addr_len=%d\n", __func__, session.dst_ip_addr.addr_len);
			printk("%s: session.dst_l4_port=%d\n", __func__, session.dst_l4_port);
			printk("%s: session.aging_timer=%d\n", __func__, session.aging_timer);
		}

		/* if the NAT entry still exists means it has been hit */
		if ((session.xlate_flags & CA_NAT_TRANSLATE_STATIC) || (session.aging_timer != 0)) {
			return CA_E_BUSY;
		} else {
			/* aging_timer must be 0, and NAT entry is dynamic */
			ret = ca_nat_entry_delete(device_id, &session);
			if (ret != CA_E_OK) {
				printk("%s: ca_nat_entry_delete(), return failed!!, ret=%d\n", __func__, ret);
			}
		}
	}

        return CA_E_OK;
}

ca_status_t ca_kh_nat_entry_session_delete_force(ca_device_id_t device_id, struct nf_conntrack_tuple *tuple)
{
	ca_status_t ret;
	ca_nat_entry_t session;

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		if (tuple->src.l3num == AF_INET) {
			printk("%s: ip_proto=%u, old_src_ip=%pI4, old_dst_ip=%pI4, old_sport=%d, old_dport=%d\n",
				__func__, tuple->dst.protonum, &tuple->src.u3.ip, &tuple->dst.u3.ip, ntohs(tuple->src.u.all), ntohs(tuple->dst.u.all));
		} else if (tuple->src.l3num == AF_INET6) {
			printk("%s: ip_proto=%u, old_src_ip=%pI6c, old_dst_ip=%pI6c, old_sport=%d, old_dport=%d\n",
				__func__, tuple->dst.protonum, &tuple->src.u3.ip6, &tuple->dst.u3.ip6, ntohs(tuple->src.u.all), ntohs(tuple->dst.u.all));

		} else {
			printk("%s: unknown l3num %u\n", __func__, tuple->src.l3num);
			return CA_E_NOT_SUPPORT;
		}
	}

	memset(&session, 0, sizeof(session));
	session.ip_proto = tuple->dst.protonum;
	if (tuple->src.l3num == AF_INET) {
		session.src_ip_addr.afi = CA_IPV4;
		session.src_ip_addr.ip_addr.ipv4_addr = ntohl(tuple->src.u3.ip);
		session.src_ip_addr.addr_len = 32;
		session.dst_ip_addr.afi = CA_IPV4;
		session.dst_ip_addr.ip_addr.ipv4_addr = ntohl(tuple->dst.u3.ip);
		session.dst_ip_addr.addr_len = 32;
	} else {
		session.src_ip_addr.afi = CA_IPV6;
		session.src_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple->src.u3.ip6[0]);
		session.src_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple->src.u3.ip6[1]);
		session.src_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple->src.u3.ip6[2]);
		session.src_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple->src.u3.ip6[3]);
		session.src_ip_addr.addr_len = 128;
		session.dst_ip_addr.afi = CA_IPV6;
		session.dst_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple->dst.u3.ip6[0]);
		session.dst_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple->dst.u3.ip6[1]);
		session.dst_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple->dst.u3.ip6[2]);
		session.dst_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple->dst.u3.ip6[3]);
		session.dst_ip_addr.addr_len = 128;
	}
	session.src_l4_port = ntohs(tuple->src.u.all);
	session.dst_l4_port = ntohs(tuple->dst.u.all);

	ret = ca_nat_entry_get(device_id, &session);
	if (ret == CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: session.ip_proto=%u\n", __func__, session.ip_proto);
			printk("%s: session.src_ip_addr.afi=%d\n", __func__, session.src_ip_addr.afi);
			printk("%s: session.src_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session.src_ip_addr.ip_addr.ipv4_addr);
			printk("%s: session.src_ip_addr.addr_len=%d\n", __func__, session.src_ip_addr.addr_len);
			printk("%s: session.src_l4_port=%d\n", __func__, session.src_l4_port);
			printk("%s: session.dst_ip_addr.afi=%d\n", __func__, session.dst_ip_addr.afi);
			printk("%s: session.dst_ip_addr.ip_addr.ipv4_add=%pI4\n", __func__, &session.dst_ip_addr.ip_addr.ipv4_addr);
			printk("%s: session.dst_ip_addr.addr_len=%d\n", __func__, session.dst_ip_addr.addr_len);
			printk("%s: session.dst_l4_port=%d\n", __func__, session.dst_l4_port);
			printk("%s: session.aging_timer=%d\n", __func__, session.aging_timer);
		}
		ret = ca_nat_entry_delete(device_id, &session);
		if (ret != CA_E_OK) {
			printk("%s: fail to delete entry!!, ret=%d\n", __func__, ret);
		}
	} else {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: Fail to get NAT entry. ret=%d\n", __func__, ret);
		}
	}

	return CA_E_OK;
}

static int ca_kh_nat_entry_traverse_cb1(ca_device_id_t device_id, ca_nat_entry_t *session, void *user_data)
{
        int ret;
	ca_uint16_t port;

	port = *(ca_uint16_t *)user_data;

        if (ca_kh_debug & CA_KH_DEBUG_NAT) {
                printk("%s: session->src_ip_addr.afi=%d\n", __func__, session->src_ip_addr.afi);
		printk("%s: session->src_l4_port=%d, session->dst_l4_port=%d, session->new_src_l4_port=%d, session->new_dst_l4_port=%d\n",
			__func__, session->src_l4_port, session->dst_l4_port, session->new_src_l4_port, session->new_dst_l4_port);
        }

        if ((port == session->src_l4_port) || (port == session->dst_l4_port) || (port == session->new_src_l4_port) || (port == session->new_dst_l4_port)) {

                if (ca_kh_debug & CA_KH_DEBUG_NAT) {
                        printk("%s: This session should be deleted!!\n", __func__);
                }
                ret = ca_nat_entry_delete(device_id, session);

                if (ret != CA_E_OK) {
                        printk("%s: ca_nat_entry_delete(), return failed!!, ret=%d\n", __func__, ret);
                }
                return ret;
        }
        return CA_E_OK;
}

ca_status_t ca_kh_nat_entry_delete_by_port(ca_device_id_t device_id, ca_uint16_t port)
{
        if (ca_kh_debug & CA_KH_DEBUG_NAT) {
                printk("%s: port=%d\n", __func__, port);
        }

        ca_nat_entry_traverse(device_id, ca_kh_nat_entry_traverse_cb1, &port);
        return CA_E_OK;
}

static ca_status_t ca_nat_entry_traverse(ca_device_id_t device_id, ca_nat_entry_traverse_cb fn, void *user_data)
{
	ca_iterator_t   iterator;
	ca_nat_entry_t  nat_entry;
	ca_uint32_t     count;
	ca_status_t     ret;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &nat_entry;
	iterator.user_buffer_size = sizeof(ca_nat_entry_t);

	printk("\n================= NAT table =====================\n");
	while (1) {
		ret = ca_nat_entry_iterate(device_id, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;
		fn(device_id, &nat_entry, user_data);
	}

	return ret;
}

/* tuple0, tuple1: Conntrack tuples; one is original direction, another one is replied direction.
 * aging_time: Returned aging time of tuple0 and tuple1.
 *             If both aging times are different, return the large one.
 *             Only meaningful when function returns CA_E_OK.
 *
 * Return value:
 *   CA_E_OK: Aging time is synchronized and refreshed, NAT sessions are still working.
 *   others: NAT sessions can be deleted.
 */
ca_status_t ca_kh_nat_entry_timer_refresh(ca_device_id_t device_id, struct nf_conntrack_tuple *tuple0, struct nf_conntrack_tuple *tuple1, ca_uint32_t *aging_time)
{
	ca_status_t ret;
	ca_nat_entry_t session0, session1;

	if ((tuple0 == NULL) || (tuple1 == NULL) || (aging_time == NULL))
		return CA_E_PARAM;

	if (tuple0->dst.protonum != tuple1->dst.protonum) {
		return CA_E_PARAM;
	}

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		if (tuple0->src.l3num == AF_INET) {
			printk("%s: entry0, ip_proto=%u, old_src_ip=%pI4, old_dst_ip=%pI4, old_sport=%d, old_dport=%d\n",
				__func__, tuple0->dst.protonum, &tuple0->src.u3.ip, &tuple0->dst.u3.ip, ntohs(tuple0->src.u.all), ntohs(tuple0->dst.u.all));
		} else if (tuple0->src.l3num == AF_INET6) {
			printk("%s: entry0, ip_proto=%u, old_src_ip=%pI6c, old_dst_ip=%pI6c, old_sport=%d, old_dport=%d\n",
				__func__, tuple0->dst.protonum, &tuple0->src.u3.ip6, &tuple0->dst.u3.ip6, ntohs(tuple0->src.u.all), ntohs(tuple0->dst.u.all));
		} else {
			printk("%s: entry0, unknown l3num %u\n", __func__, tuple0->src.l3num);
			return CA_E_NOT_SUPPORT;
		}
		if (tuple1->src.l3num == AF_INET) {
			printk("%s: entry1, ip_proto=%u, old_src_ip=%pI4, old_dst_ip=%pI4, old_sport=%d, old_dport=%d\n",
				__func__, tuple1->dst.protonum, &tuple1->src.u3.ip, &tuple1->dst.u3.ip, ntohs(tuple1->src.u.all), ntohs(tuple1->dst.u.all));
		} else if (tuple1->src.l3num == AF_INET6) {
			printk("%s: entry1, ip_proto=%u, old_src_ip=%pI6c, old_dst_ip=%pI6c, old_sport=%d, old_dport=%d\n",
				__func__, tuple1->dst.protonum, &tuple1->src.u3.ip6, &tuple1->dst.u3.ip6, ntohs(tuple1->src.u.all), ntohs(tuple1->dst.u.all));
		} else {
			printk("%s: entry1, unknown l3num %u\n", __func__, tuple1->src.l3num);
			return CA_E_NOT_SUPPORT;
		}
	}

	memset(&session0, 0, sizeof(session0));
	session0.ip_proto = tuple0->dst.protonum;
	if (tuple0->src.l3num == AF_INET) {
		session0.src_ip_addr.afi = CA_IPV4;
		session0.src_ip_addr.ip_addr.ipv4_addr = ntohl(tuple0->src.u3.ip);
		session0.src_ip_addr.addr_len = 32;
		session0.dst_ip_addr.afi = CA_IPV4;
		session0.dst_ip_addr.ip_addr.ipv4_addr = ntohl(tuple0->dst.u3.ip);
		session0.dst_ip_addr.addr_len = 32;
	} else {
		session0.src_ip_addr.afi = CA_IPV6;
		session0.src_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple0->src.u3.ip6[0]);
		session0.src_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple0->src.u3.ip6[1]);
		session0.src_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple0->src.u3.ip6[2]);
		session0.src_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple0->src.u3.ip6[3]);
		session0.src_ip_addr.addr_len = 128;
		session0.dst_ip_addr.afi = CA_IPV6;
		session0.dst_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple0->dst.u3.ip6[0]);
		session0.dst_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple0->dst.u3.ip6[1]);
		session0.dst_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple0->dst.u3.ip6[2]);
		session0.dst_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple0->dst.u3.ip6[3]);
		session0.dst_ip_addr.addr_len = 128;
	}
	session0.src_l4_port = ntohs(tuple0->src.u.all);
	session0.dst_l4_port = ntohs(tuple0->dst.u.all);
	ret = ca_nat_entry_get(device_id, &session0);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: Fail to get NAT entry#0. ret=%u\n", __func__, ret);
		}
		session0.aging_timer = 0;
		/* Maybe only one direction of NAT entry is added, so don't treat it as error. */
		//return ret;
	}

	memset(&session1, 0, sizeof(session1));
	session1.ip_proto = tuple1->dst.protonum;
	if (tuple1->src.l3num == AF_INET) {
		session1.src_ip_addr.afi = CA_IPV4;
		session1.src_ip_addr.ip_addr.ipv4_addr = ntohl(tuple1->src.u3.ip);
		session1.src_ip_addr.addr_len = 32;
		session1.dst_ip_addr.afi = CA_IPV4;
		session1.dst_ip_addr.ip_addr.ipv4_addr = ntohl(tuple1->dst.u3.ip);
		session1.dst_ip_addr.addr_len = 32;
	} else {
		session1.src_ip_addr.afi = CA_IPV6;
		session1.src_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple1->src.u3.ip6[0]);
		session1.src_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple1->src.u3.ip6[1]);
		session1.src_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple1->src.u3.ip6[2]);
		session1.src_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple1->src.u3.ip6[3]);
		session1.src_ip_addr.addr_len = 128;
		session1.dst_ip_addr.afi = CA_IPV6;
		session1.dst_ip_addr.ip_addr.ipv6_addr[0] = ntohl(tuple1->dst.u3.ip6[0]);
		session1.dst_ip_addr.ip_addr.ipv6_addr[1] = ntohl(tuple1->dst.u3.ip6[1]);
		session1.dst_ip_addr.ip_addr.ipv6_addr[2] = ntohl(tuple1->dst.u3.ip6[2]);
		session1.dst_ip_addr.ip_addr.ipv6_addr[3] = ntohl(tuple1->dst.u3.ip6[3]);
		session1.dst_ip_addr.addr_len = 128;
	}
	session1.src_l4_port = ntohs(tuple1->src.u.all);
	session1.dst_l4_port = ntohs(tuple1->dst.u.all);
	ret = ca_nat_entry_get(device_id, &session1);
	if (ret != CA_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_NAT) {
			printk("%s: Fail to get NAT entry#1. ret=%u\n", __func__, ret);
		}
		session1.aging_timer = 0;
		/* Maybe only one direction of NAT entry is added, so don't treat it as error. */
		// return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_NAT) {
		printk("%s: entry#0 aging_timer=%u, entry#1 aging_timer=%u\n", __func__, session0.aging_timer, session1.aging_timer);
	}

	if ((session0.aging_timer == 0) && (session1.aging_timer == 0)) {
		/* both directions are expired, so this NAT entry is expired */
		return CA_E_TIMEOUT;
	}

	*aging_time = (session0.aging_timer > session1.aging_timer) ? session0.aging_timer : session1.aging_timer;

	return CA_E_OK;
}

