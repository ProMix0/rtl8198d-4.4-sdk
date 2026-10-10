/* Kernel Hook APIs for iros */
#ifndef __KHOOK_H__
#define __KHOOK_H__

#ifdef __cplusplus
extern "C"{
#endif
#include "ca_types.h"
#include "route.h"

ca_status_t ca_kh_l3_get_intf_id_by_ifindex(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_ip_afi_t afi,
    CA_IN ca_int16_t ifindex,
    CA_IN ca_l3_intf_type_t intf_type,
    CA_OUT ca_intf_id_t *intf_id);

ca_status_t ca_kh_mcast_get_multicast_snooping(
        CA_IN ca_device_id_t device_id,
        CA_OUT ca_uint8_t *mcast_snooping);

#ifdef __cplusplus
}
#endif
#endif

