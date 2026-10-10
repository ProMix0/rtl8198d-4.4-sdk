
#include "ca_types.h"
#include <soc/cortina/ca_kernel_hook.h>

ca_status_t ca_kh_l3_tunnel_add(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel)
{
        int i;

        if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: kh_tunnel->type=%d\n", __func__, kh_tunnel->type);
                if (kh_tunnel->dest_addr.afi == CA_IPV4) {
                        printk("%s: kh_tunnel->dest_addr.afi=%d, kh_tunnel->dest_addr.ip_addr.ipv4_addr=%pI4, kh_tunnel->dest_addr.addr_len=%d\n",
                                __func__, kh_tunnel->dest_addr.afi, &kh_tunnel->dest_addr.ip_addr.ipv4_addr, kh_tunnel->dest_addr.addr_len);
			printk("%s: kh_tunnel->src_addr.afi=%d, kh_tunnel->src_addr.ip_addr.ipv4_addr=%pI4, kh_tunnel->src_addr.addr_len=%d\n",
                                __func__, kh_tunnel->src_addr.afi, &kh_tunnel->src_addr.ip_addr.ipv4_addr, kh_tunnel->src_addr.addr_len);
                }
                else {
			printk("%s: kh_tunnel->dest_addr.afi=%d, kh_tunnel->dest_addr.ip_addr.ipv6_addr=%pI6, kh_tunnel->dest_addr.addr_len=%d\n",
                                __func__, kh_tunnel->dest_addr.afi, &kh_tunnel->dest_addr.ip_addr.ipv6_addr, kh_tunnel->dest_addr.addr_len);
                        printk("%s: kh_tunnel->src_addr.afi=%d, kh_tunnel->src_addr.ip_addr.ipv6_addr=%pI6, kh_tunnel->src_addr.addr_len=%d\n",
                                __func__, kh_tunnel->src_addr.afi, &kh_tunnel->src_addr.ip_addr.ipv6_addr, kh_tunnel->src_addr.addr_len);
                }
        }

	spin_lock_bh(&ca_kh_tnl_lock);
        for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i ++) {
        	if (ca_kh_tunnel_table[i].is_used == 0)
                                break;
	}
	spin_unlock_bh(&ca_kh_tnl_lock);

        if (i == CA_TUNNEL_MAX_LIMIT) {
        	printk("%s: No more  ca_kh_tunnel_entry_t is available!!\n", __func__);
                return CA_E_ERROR;
        }

	spin_lock_bh(&ca_kh_tnl_lock);
        ca_kh_tunnel_table[i].is_used = 1;
        ca_kh_tunnel_table[i].type = kh_tunnel->type;
        ca_kh_tunnel_table[i].dest_addr = kh_tunnel->dest_addr;
        ca_kh_tunnel_table[i].src_addr = kh_tunnel->src_addr;
        ca_kh_tunnel_table[i].tunnel_id = kh_tunnel->tunnel_id;
	spin_unlock_bh(&ca_kh_tnl_lock);

	return CA_E_OK;
}

ca_status_t ca_kh_l3_tunnel_get(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel)
{
        int i;

        if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
                printk("%s: kh_tunnel->type=%d\n", __func__, kh_tunnel->type);
		if (kh_tunnel->dest_addr.afi == CA_IPV4) {
                        printk("%s: kh_tunnel->dest_addr.afi=%d, kh_tunnel->dest_addr.ip_addr.ipv4_addr=%pI4, kh_tunnel->dest_addr.addr_len=%d\n",
                                __func__, kh_tunnel->dest_addr.afi, &kh_tunnel->dest_addr.ip_addr.ipv4_addr, kh_tunnel->dest_addr.addr_len);
                        printk("%s: kh_tunnel->src_addr.afi=%d, kh_tunnel->src_addr.ip_addr.ipv4_addr=%pI4, kh_tunnel->src_addr.addr_len=%d\n",
                                __func__, kh_tunnel->src_addr.afi, &kh_tunnel->src_addr.ip_addr.ipv4_addr, kh_tunnel->src_addr.addr_len);
                }
                else {
                        printk("%s: kh_tunnel->dest_addr.afi=%d, kh_tunnel->dest_addr.ip_addr.ipv6_addr=%pI6, kh_tunnel->dest_addr.addr_len=%d\n",
                                __func__, kh_tunnel->dest_addr.afi, &kh_tunnel->dest_addr.ip_addr.ipv6_addr, kh_tunnel->dest_addr.addr_len);
                        printk("%s: kh_tunnel->src_addr.afi=%d, kh_tunnel->src_addr.ip_addr.ipv6_addr=%pI6, kh_tunnel->src_addr.addr_len=%d\n",
                                __func__, kh_tunnel->src_addr.afi, &kh_tunnel->src_addr.ip_addr.ipv6_addr, kh_tunnel->src_addr.addr_len);
                }
        }

	spin_lock_bh(&ca_kh_tnl_lock);
        for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i ++) {
		if (ca_kh_tunnel_table[i].is_used == 0)
                	continue;

        	if ((ca_kh_tunnel_table[i].type == kh_tunnel->type) &&
                	!memcmp(&(ca_kh_tunnel_table[i].dest_addr), &(kh_tunnel->dest_addr), sizeof(ca_ip_address_t)) &&
                	!memcmp(&(ca_kh_tunnel_table[i].src_addr), &(kh_tunnel->src_addr), sizeof(ca_ip_address_t))) {

				kh_tunnel->tunnel_id = ca_kh_tunnel_table[i].tunnel_id;
                                if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
                                        printk("%s: get the tunnel at tunnel id = %d\n", __func__, kh_tunnel->tunnel_id);
                                }
                                break;
                        }
        }
	spin_unlock_bh(&ca_kh_tnl_lock);

        if (i == CA_TUNNEL_MAX_LIMIT) {
        	printk("%s: No matched ca_kh_tunnel_entry_t is found!!\n", __func__);
                return CA_E_ERROR;
        }
	return CA_E_OK;
}

ca_status_t ca_kh_l3_tunnel_delete(ca_device_id_t device_id, ca_tunnel_id_t tunnel_id)
{
	int i;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
	printk("%s: tunnel_id=%d\n", __func__, tunnel_id);
	}

	spin_lock_bh(&ca_kh_tnl_lock);
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i ++) {
                if (ca_kh_tunnel_table[i].is_used == 0)
                        continue;
		if (ca_kh_tunnel_table[i].tunnel_id == tunnel_id) {
			if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
                        	printk("%s: found the tunnel at tunnel table index=%d\n", __func__, i);
                        }
			ca_kh_tunnel_table[i].is_used = 0;
			break;
		}
	}
	spin_unlock_bh(&ca_kh_tnl_lock);

	if (i == CA_TUNNEL_MAX_LIMIT) {
                printk("%s: No matched ca_kh_tunnel_entry_t is found!!\n", __func__);
                return CA_E_ERROR;
        }
        return CA_E_OK;
}

