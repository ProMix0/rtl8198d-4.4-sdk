
#include <linux/ip.h>

#include <soc/cortina/ca_kernel_hook.h>

#include "ca_kernel_hook_util.h"

typedef struct ca_kh_tr069_intf_s {
	ca_uint8_t valid;
	int ifindex;
	ca_uint8_t cos;
} ca_kh_tr069_intf_t;

static ca_kh_tr069_intf_t tr069_intf;

static ca_uint32_t ca_kh_gen_ip_mask_v4(ca_uint8_t mask)
{
        ca_uint8_t i;
        ca_uint32_t mask_pattern=0;

        for (i = 0; i < 32; i++, mask--) {
                if (mask == 0)
                        break;
                mask_pattern |= 1 << (31 - i);
        }

        return mask_pattern;
}

ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2)
{
        ca_uint32_t ip1, ip2;
        ca_uint32_t  mask1, mask2;

	/* the input ip address should be in host order */

	if (ip_addr1->afi != ip_addr2->afi) return FALSE;

        if (ip_addr1->afi == CA_IPV6) {
                mask1 = ip_addr1->addr_len;
                mask2 = ip_addr2->addr_len;

		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        printk("%s: mask1=0x%x, mask2=0x%x\n", __func__, mask1, mask2);
                        printk("%s: ip_addr1=%pI6, ip_addr2=%pI6\n", __func__, &ip_addr1->ip_addr.ipv6_addr, ip_addr2->ip_addr.ipv6_addr);
                }
                if (mask1 == mask2) {

			if (mask1 == 0)
				return TRUE;

                        if (mask1 > 96) {        /* Just compare addr32[0~3] */
                                mask1 = ca_kh_gen_ip_mask_v4(mask1 - 96);
                                if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                    (ip_addr1->ip_addr.ipv6_addr[1] == ip_addr2->ip_addr.ipv6_addr[1]) &&
                                    (ip_addr1->ip_addr.ipv6_addr[2] == ip_addr2->ip_addr.ipv6_addr[2]) &&
                                    ((ip_addr1->ip_addr.ipv6_addr[3] & mask1) == (ip_addr2->ip_addr.ipv6_addr[3] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 64) { /* Compare addr32[1~3] */
                               mask1 = ca_kh_gen_ip_mask_v4(mask1 - 64);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                   (ip_addr1->ip_addr.ipv6_addr[1] == ip_addr2->ip_addr.ipv6_addr[1]) &&
                                   ((ip_addr1->ip_addr.ipv6_addr[2] & mask1) == (ip_addr2->ip_addr.ipv6_addr[2] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 32) { /* Compare addr32[2~3] */
                               mask1 = ca_kh_gen_ip_mask_v4(mask1 - 32);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                   ((ip_addr1->ip_addr.ipv6_addr[1] & mask1) == (ip_addr2->ip_addr.ipv6_addr[1] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 0) {  /* Compare addr32[3] */
                               mask1 = ca_kh_gen_ip_mask_v4(mask1);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] & mask1) == (ip_addr2->ip_addr.ipv6_addr[0] & mask1))
                                        return TRUE;
                        }
            }
        }
        else {
                ip1 = ip_addr1->ip_addr.ipv4_addr;
                ip2 = ip_addr2->ip_addr.ipv4_addr;
                mask1 = ca_kh_gen_ip_mask_v4(ip_addr1->addr_len);
                mask2 = ca_kh_gen_ip_mask_v4(ip_addr2->addr_len);

		if (ca_kh_debug & CA_KH_DEBUG_ROUTE) {
                        printk("%s: ip1=%pI4, ip2=%pI4, mask1=0x%x, mask2=0x%x\n", __func__, &ip1, &ip2, mask1, mask2);
                }
                if ((mask1 == mask2) && ((ip1 & mask1) == (ip2 & mask2))) {
                        return TRUE;
                }
        }
        return FALSE;
}

void ca_kh_ip_address_ntohl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src)
{
	p_dest->afi = p_src->afi;
	p_dest->addr_len = p_src->addr_len;

	if (p_src->afi == CA_IPV6) {
		p_dest->ip_addr.ipv6_addr[0] = ntohl(p_src->ip_addr.ipv6_addr[0]);
		p_dest->ip_addr.ipv6_addr[1] = ntohl(p_src->ip_addr.ipv6_addr[1]);
		p_dest->ip_addr.ipv6_addr[2] = ntohl(p_src->ip_addr.ipv6_addr[2]);
		p_dest->ip_addr.ipv6_addr[3] = ntohl(p_src->ip_addr.ipv6_addr[3]);
	} else {
		p_dest->ip_addr.ipv4_addr = ntohl(p_src->ip_addr.ipv4_addr);
	}
}

void ca_kh_ip_address_htonl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src)
{
	p_dest->afi = p_src->afi;
	p_dest->addr_len = p_src->addr_len;

	if (p_src->afi == CA_IPV6) {
		p_dest->ip_addr.ipv6_addr[0] = htonl(p_src->ip_addr.ipv6_addr[0]);
		p_dest->ip_addr.ipv6_addr[1] = htonl(p_src->ip_addr.ipv6_addr[1]);
		p_dest->ip_addr.ipv6_addr[2] = htonl(p_src->ip_addr.ipv6_addr[2]);
		p_dest->ip_addr.ipv6_addr[3] = htonl(p_src->ip_addr.ipv6_addr[3]);
	} else {
		p_dest->ip_addr.ipv4_addr = htonl(p_src->ip_addr.ipv4_addr);
	}
}

ca_status_t ca_kh_tr069_intf_set(ca_int32_t ifindex, ca_uint8_t cos)
{
	if (cos > 7)
		return CA_E_PARAM;

	tr069_intf.cos = cos;
	tr069_intf.ifindex = ifindex;
	tr069_intf.valid = 1;
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_tr069_intf_set);

ca_status_t ca_kh_tr069_intf_get(ca_int32_t *ifindex, ca_uint8_t *cos)
{
	if (ifindex == NULL || cos == NULL)
		return CA_E_PARAM;

	if (!tr069_intf.valid)
		return CA_E_UNAVAIL;

	*cos = tr069_intf.cos;
	*ifindex = tr069_intf.ifindex;
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_tr069_intf_get);

ca_status_t ca_kh_tr069_intf_clean(void)
{
	memset(&tr069_intf, 0, sizeof(ca_kh_tr069_intf_t));
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_tr069_intf_clean);

ca_status_t ca_kh_8021q_pcp_handler(struct sk_buff *skb)
{
	ca_uint8_t cos;
	int ret;
	int ifindex;

	ret = ca_kh_tr069_intf_get(&ifindex, &cos);

	if (ret == CA_E_OK) {
		if (skb->dev->ifindex == ifindex) {
				skb->priority = cos;
		}
	}
	return CA_E_OK;
}

/* WORKAROUND:
 *   Use port_id to decide WAN port or LAN port.
 *   WAN port is NAT enabled, and LAN port is not.
 */
ca_status_t ca_kh_decide_nat_enable(ca_l3_intf_t *intf)
{
	if (!intf) return CA_E_PARAM;

	switch (intf->port_id & CA_PORT_ID_MASK) {
	case 0x19:
		intf->nat_enable = 1; // Don't modify this because it breaks current planned flow & design.
		break;
	case 0x7:
	case 0x18:
	default:
		intf->nat_enable = 1;
		break;
	}

	intf->mask.nat_enable = 1;

	return CA_E_OK;
}

