#ifndef __CA_KERNEL_HOOK_UTIL_H__
#define __CA_KERNEL_HOOK_UTIL_H__

#include "ca_types.h"
#include <soc/cortina/ca_kernel_hook.h>

ca_boolean_t ca_kh_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
void ca_kh_ip_address_htonl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src);
void ca_kh_ip_address_ntohl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src);

ca_status_t ca_kh_decide_nat_enable(ca_l3_intf_t *intf);

#endif /* __CA_KERNEL_HOOK_UTIL_H__ */

