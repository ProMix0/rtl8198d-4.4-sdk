#ifndef __MIRROR_H__
#define __MIRROR_H__

#include "ca_types.h"

//port mirroring configurations
typedef struct {
    ca_uint32_t            mirror_source_port    :1;
    ca_uint32_t            mirror_dest_port      :1;
} ca_port_mirror_mask_t;


typedef struct {
    ca_port_mirror_mask_t   mask;        /* see ca_port_mirror_mask_t */
    ca_boolean_t            enable;        /* (0= disable, 1=enable) */
    ca_port_id_t            source_port;
    ca_port_id_t            destination_port;
    ca_port_id_t            mirror_port;
} ca_port_mirror_config_t;

typedef struct mirror_config_s{
    ca_port_mirror_config_t mirror_cfg[2];
    ca_uint32_t l3_cls_idx;
    ca_uint32_t l2_cls_idx;        
    ca_uint32_t vlan;
    ca_boolean_t wan_drop_unknow_vlan;
    ca_uint8_t   wan_rxsel;
    ca_uint32_t l3fe_dpq_demux;
    ca_uint32_t l3fe_demux;
    ca_uint32_t stp_mode;
    ca_uint32_t wan_to_l3fe_cls_idx;
    ca_uint32_t rx_vlan_member;
    ca_uint32_t tx_vlan_member;
    ca_uint32_t l3fe_stg0_ctrl;
    ca_uint32_t l3fe_stg0_ldpid_map;
    ca_uint32_t g3_l3_lan_2_wan;
}mirror_config_t;

ca_status_t ca_port_mirror_enable_set (
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_port_mirror_config_t        *mirror_config);

ca_status_t ca_port_mirror_enable_get (
    CA_IN   ca_device_id_t                 device_id,
    CA_OUT  ca_port_mirror_config_t        *mirror_config);


ca_status_t ca_mirror_control_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t src_port,
    CA_IN ca_port_id_t dst_port,
    CA_IN ca_boolean_t mir_ena);

#endif
