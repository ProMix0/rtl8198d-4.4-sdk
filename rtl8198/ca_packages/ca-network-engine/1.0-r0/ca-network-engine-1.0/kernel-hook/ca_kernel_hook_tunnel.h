#ifndef __CA_KERNEL_HOOK_TUNNEL_H__
#define __CA_KERNEL_HOOK_TUNNEL_H__

#include <soc/cortina/ca_kernel_hook.h>

ca_status_t ca_kh_l3_tunnel_add(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel);
ca_status_t ca_kh_l3_tunnel_get(ca_device_id_t device_id, ca_kh_tunnel_entry_t *kh_tunnel);
ca_status_t ca_kh_l3_tunnel_delete(ca_device_id_t device_id, ca_tunnel_id_t tunnel_id);

#endif /* __CA_KERNEL_HOOK_TUNNEL_H__ */

