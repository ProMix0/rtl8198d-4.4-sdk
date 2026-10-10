
#ifndef __CA_MULTICAST_H__
#define __CA_MULTICAST_H__


#include "ca_types.h"
#include "vlan.h"

#ifdef __cplusplus
extern "C"{
#endif


#define CA_MAX_L2_MC_MEMBER 	32

#define CA_MAX_L3_MC_MEMBER 64
#define CA_MC_EGRESS_ACTION_RESERVED 4

typedef enum {
	CA_MC_MODE_MAC 	= 1,	/* filer, replicate based on MAC DA */
	CA_MC_MODE_IP 	= 2,    /* filter, replicate based on IP DA */
	CA_MC_MODE_LLID = 3,    /* filter, replicate based on MC LLID */
	CA_MC_MODE_CLS  = 3
} ca_mcast_addressing_mode_t;

typedef struct ca_mcast_config_s {
	/* 1 - flood unknown multicast traffic to all ports. 0 - drop unknown multicast traffic */
	ca_boolean_t					unknown_multicast_flooding_enable;
	ca_boolean_t					igmp_use_mc_vlan;
	ca_boolean_t					mld_use_mc_vlan;
	ca_mcast_addressing_mode_t	mode;
} ca_l2_mcast_config_t;

typedef struct {
/* #if defined(CONFIG_LUNA_G3_SERIES)	*/
	ca_uint32_t					mac_sa_trans	:1;
/* #endif */
	ca_uint32_t					mac_da_overwrite	:1;
	ca_uint32_t					vlan_action		:1;
} ca_mcast_egress_action_mask_t;

typedef struct ca_l2_mcast_entry_s {
	ca_uint32_t					mcg_id;
	ca_uint16_t					mcast_vlan;
	/* specify group MC LLID, used when CA_MC_MODE_LLID is configured */
	ca_uint16_t					replication_only; /* for EPON LLID based MC */
	/* specify group mac addr, used when CA_MC_MODE_L2 is configured */
	ca_mac_addr_t					group_mac_addr;
	/* specify group ip addr, used when CA_MC_MODE_L3 is configured*/
	ca_ip_address_t 				group_ip_addr;
	/* src_ip_address.addr_len = 0 for (*,G)*/
	ca_ip_address_t					src_ip_address;
} ca_l2_mcast_entry_t;

typedef struct ca_l2_mcast_member_s {
	ca_mcast_egress_action_mask_t			action_mask;
	ca_mac_addr_t 					new_mac_da;
	ca_vlan_tag_action_t				vlan_action;
	ca_uint16_t					vid;
/* #if defined(CONFIG_LUNA_G3_SERIES) */
	ca_uint16_t					vpri:3;
	ca_uint16_t					cos:3;
	ca_uint16_t 					wifi_flowid:10;
/* #endif */
	ca_uint32_t 					reserved[CA_MC_EGRESS_ACTION_RESERVED];
	ca_port_id_t					member_port;
/* #if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH) */
	ca_mac_addr_t 					new_mac_sa;
/* #endif */
} ca_l2_mcast_member_t;

typedef struct ca_l2_mcast_group_members_s {
	ca_uint32_t					mcg_id;
	ca_uint32_t					member_count;
	ca_l2_mcast_member_t				member[CA_MAX_L2_MC_MEMBER];
} ca_l2_mcast_group_members_t;

typedef struct ca_l2_mcast_iterator_s {
	ca_l2_mcast_entry_t 				mc_entry;
	ca_l2_mcast_group_members_t			members;
} ca_l2_mcast_iterator_entry_t;

typedef struct ca_l3_mcast_entry_s {
	/*unique ID for multicast traffic replication returned by driver
	Caller must use this id for subsequent group_delete/member_add/member_delete
	And group_get
	*/
	ca_uint32_t			mcg_id;
	/* specify group ip addr */
/* #ifdef CONFIG_RTK_DEV_AP */
	ca_uint16_t					mcast_vlan;
/* #endif */
	ca_ip_address_t			group_ip_addr;
	/* src_ip_address.addr_len = 0 for (*,G)*/
	ca_ip_address_t			src_ip_address;
} ca_l3_mcast_entry_t;

typedef struct ca_l3_mcast_member_s {
	ca_mcast_egress_action_mask_t 			action_mask;
	ca_mac_addr_t 					new_mac_da;
	ca_uint32_t 					reserved[CA_MC_EGRESS_ACTION_RESERVED];
	ca_intf_id_t					member_intf;
} ca_l3_mcast_member_t;

typedef struct ca_l3_mcast_group_members_s {
	ca_uint32_t					mcg_id;
	ca_uint32_t					member_count;
	ca_l3_mcast_member_t				member[CA_MAX_L3_MC_MEMBER];
} ca_l3_mcast_group_members_t;

typedef struct ca_l3_mcast_iterator_s {
	ca_l3_mcast_entry_t 				mc_entry;
	ca_l3_mcast_group_members_t			members;
} ca_l3_mcast_iterator_entry_t;

ca_status_t ca_l2_mcast_config_set (
	CA_IN ca_device_id_t		device_id,
	CA_IN ca_l2_mcast_config_t	*mcast_config
);
ca_status_t ca_l2_mcast_config_get (
	CA_IN  ca_device_id_t		device_id,
	CA_OUT ca_l2_mcast_config_t	*mcast_config
);

ca_status_t ca_l2_mcast_group_add (
	CA_IN ca_device_id_t		device_id,
	CA_OUT ca_l2_mcast_entry_t	*entry
);

ca_status_t ca_l2_mcast_group_delete (
	CA_IN ca_device_id_t		device_id,
	CA_IN ca_uint32_t		mcg_id
);

ca_status_t ca_l2_mcast_group_delete_all (
	CA_IN ca_device_id_t		device_id
);

ca_status_t ca_l2_mcast_group_get (
	CA_IN ca_device_id_t		device_id,
	CA_IN ca_uint32_t		mcg_id,
	CA_OUT ca_l2_mcast_iterator_entry_t	*mcast_group
);

ca_status_t ca_l2_mcast_group_iterate (
	CA_IN ca_device_id_t 		device_id,
	CA_OUT ca_iterator_t		*return_entry
);

ca_status_t ca_l2_mcast_member_add (
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_l2_mcast_group_members_t	*members
);

ca_status_t ca_l2_mcast_member_delete (
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_l2_mcast_group_members_t	*members
);

ca_status_t ca_l3_mcast_group_add (
	CA_IN ca_device_id_t         device_id,
	CA_OUT ca_l3_mcast_entry_t *entry
);

/* #ifdef CONFIG_RTK_DEV_AP */
ca_uint16_t ca_l3_mcast_fake_vid_get(
	CA_IN ca_uint16_t wan_vid
);
/* #endif */

ca_status_t ca_l3_mcast_group_delete (
	CA_IN ca_device_id_t		device_id,
	CA_IN ca_uint32_t		mcg_id
);

ca_status_t ca_l3_mcast_group_delete_all (
	CA_IN ca_device_id_t		device_id
);

ca_status_t ca_l3_mcast_group_get (
	CA_IN ca_device_id_t		device_id,
	CA_IN ca_uint32_t		mcg_id,
	CA_OUT ca_l3_mcast_iterator_entry_t	*mcast_group
);

ca_status_t ca_l3_mcast_group_iterate (
	CA_IN ca_device_id_t 		device_id,
	CA_OUT ca_iterator_t		*return_entry
);

ca_status_t ca_l3_mcast_member_add (
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_l3_mcast_group_members_t	*member
);

ca_status_t ca_l3_mcast_member_delete (
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_l3_mcast_group_members_t	*member
);
 
ca_status_t ca_l3_mcast_update_IPDA_entry (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32 mcg_id,
	CA_IN ca_uint32 v3_ind
);

ca_status_t ca_l2_mcast_update_IPDA_entry (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32 mcg_id,
	CA_IN ca_uint32 v3_ind
);



#ifdef __cplusplus
}
#endif

#endif /* __CA_MULTICAST_H__ */
