#ifndef CA_KERNEL_HOOK_VLAN_H
#define CA_KERNEL_HOOK_VLAN_H

#define CA_KH_WAN_PORT_VID_MAX  16

typedef struct wan_port_vid_s {
	ca_vlan_id_t	vid;
	ca_uint32_t	cls_rule_index;	
} wan_port_vid_t;

extern wan_port_vid_t wan_port_vid_list[];

ca_status_t ca_kh_add_wan_port_to_unknown_vlan_flooding_domain(ca_uint8_t enable);
ca_status_t ca_kh_add_classifier_for_wan_port_bridge_mode(ca_vlan_id_t vid, ca_uint32_t *cls_rule_index);
ca_status_t ca_kh_del_classifier_for_wan_port_bridge_mode(ca_uint32_t cls_rule_index);
#endif
