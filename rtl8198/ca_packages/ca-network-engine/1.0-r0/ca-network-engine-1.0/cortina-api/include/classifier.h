#ifndef __CLASSIFIER_H__
#define __CLASSIFIER_H__

#include "ca_types.h"

#define CA_MAX_DATA_EXTRACTION_LEN 4

typedef struct {
    ca_uint32_t min;   /* inclusive small boundary                          */
    ca_uint32_t max;   /* inclusive big boundary, end = start for one value */
} ca_classifier_l4_port_range_t;

typedef struct {
    ca_uint16_t tpid; /* TPID        */
    ca_uint8_t  pri;  /* 802.1p, PCP */
    ca_uint8_t  dei;  /* CFI or DEI  */
    ca_uint16_t vid;  /* VLAN ID     */
} ca_classifier_vlan_t;

typedef struct{
    ca_uint32_t tpid : 1;
    ca_uint32_t vid  : 1;
    ca_uint32_t dei  : 1;
    ca_uint32_t pri  : 1;
} ca_classifier_vlan_mask_t;

typedef struct {
    ca_mac_addr_t   mac_min;    /* inclusive lower bound */
    ca_mac_addr_t   mac_max;    /* inclusive upper bound, min = max for one value, and mac_min[0] & mac_max[0] are MSB */
}ca_classifier_mac_addr_range_t;

typedef struct {
    ca_classifier_vlan_t vlan_min;  /* inclusive lower bound */
    ca_classifier_vlan_t vlan_max;  /* inclusive upper bound, min = max for one value */
}ca_classifier_vlan_range_t;

typedef struct {
    ca_uint16_t                     ethertype;      /* ethertype value */
    ca_uint16_t                     subtype;        /* ethertype subtype or opcode value, OAM/MPCP */
    ca_classifier_mac_addr_range_t  mac_sa;         /* source MAC address */
    ca_classifier_mac_addr_range_t  mac_da;         /* destination MAC address */
    ca_classifier_vlan_range_t      vlan_otag;      /* the outer VLAN tag */
    ca_classifier_vlan_range_t      vlan_itag;      /* the inner VLAN tag */
    ca_uint8_t                      vlan_count;     /* tag count */
    ca_boolean_t                    is_multicast;   /* true - multicast, false - non-multicast */
    ca_boolean_t                    is_length;      /* true - length, false - ethertype */
    ca_uint16_t                     cfm_opcode;     /* CFM IPcode, work with 802.1AG ethertype */
    ca_uint16_t                     ppp_proto;	     /* LCP (0xC021), IPCP(0x8021), IPv4(0x0021/0x21), IPv6(0x0057/0x57) */
} ca_classifier_l2_t;

typedef struct {
    ca_uint32_t         ethertype       :1;
    ca_uint32_t         subtype         :1;
    ca_uint32_t         mac_sa          :1; /* src MAC addr */
    ca_uint32_t         mac_da          :1; /* dest MAC addr */
    ca_uint32_t         vlan_otag       :1;  /* outer tag */
    ca_uint32_t         vlan_itag       :1;  /* inner tag */
    ca_uint32_t         vlan_count      :1;
    ca_uint32_t         is_multicast    :1;
    ca_uint32_t         is_length       :1;
    ca_uint32_t         cfm_opcode      :1;
    ca_uint32_t         ppp_proto         :1;
    /* when vlan_count = 1, top tag is the otag */
    ca_classifier_vlan_mask_t   vlan_otag_mask;
    ca_classifier_vlan_mask_t   vlan_itag_mask;
} ca_classifier_l2_mask_t;



typedef enum {
    CA_IPV6_EXT_HEADER_HOP_BY_HOP,       /* hop-by-hop options header             */
    CA_IPV6_EXT_HEADER_ROUTING,          /* routing header                        */
    CA_IPV6_EXT_HEADER_FRAGMENT,         /* fragment header                       */
    CA_IPV6_EXT_HEADER_DESTINATION,      /* destination options header            */
    CA_IPV6_EXT_HEADER_AUTHENTICATION,   /* authentication header                 */
    CA_IPV6_EXT_HEADER_ESP               /* encapsulating security payload header */
} ca_ipv6_ext_header_t;


typedef struct {
    ca_boolean_t                    l4_valid;      /* layer 4 or not                              */
    ca_classifier_l4_port_range_t   src_port;      /* source port range, start = end for one      */
    ca_classifier_l4_port_range_t   dst_port;      /* destination port range, start = end for one */
    ca_uint16_t                     tcp_flags;     /* TCP control bits                            */
} ca_classifier_l4_t;

typedef struct {
    ca_uint32_t l4_valid  : 1;
    ca_uint32_t src_port  : 1;
    ca_uint32_t dst_port  : 1;
    ca_uint32_t tcp_flags : 9;
} ca_classifier_l4_mask_t;

typedef enum {
    CA_CLASSIFIER_OFFSET_START_L2,   			/* offset value relative to Ethernet header */
    CA_CLASSIFIER_OFFSET_START_L3,   			/* offset value relative to IP header */
    CA_CLASSIFIER_OFFSET_START_L4   			/* offset value relative to TCP/UDP header */
} ca_classifier_key_offset_start_t; 			/* offset based key starting location */

typedef struct {
    ca_classifier_key_offset_start_t	starting_location;	/* starting location */
    ca_uint16_t	    offset; 		/* offset from start of packet */
    ca_uint16_t	    length; 		/* length of the field, unit is byte*/
    ca_uint8_t	    data[CA_MAX_DATA_EXTRACTION_LEN]; /*data */
    ca_uint8_t      mask[CA_MAX_DATA_EXTRACTION_LEN]; /*bit mask for data */
} ca_classifier_ext_t;

typedef struct {
    ca_boolean_t         ip_valid;       /* valid IP check                                   */
    ca_uint32_t          ip_version;     /* IP version, V4 or V6                             */
    ca_uint8_t           ip_protocol;    /* IP protocol, IPv6 in next-header                 */
    ca_uint8_t           dscp;           /* DSCP                                             */
    ca_uint8_t           dscp_mask;    /* DSCP mask used for key compare 
                                                  set to 0x38 to get the IP precendence bits*/
    ca_uint8_t           ecn;            /* ECN                                              */
    ca_ip_address_t      ip_sa;          /* ip_sa source IP address                          */
    ca_ip_address_t      ip_sa_max;      /* range max source IP addr when range is specified */
    ca_ip_address_t      ip_da;          /* ip_da destination IP address                     */
    ca_ip_address_t      ip_da_max;      /* range max dest IP addr when range is specified   */
    ca_uint8_t           fragment ;      /* fragment status : 0-no, 1-first, 2-others        */
    ca_uint32_t          have_options;   /* whether IP-options is inclused or not, IPv4 only */
    ca_uint32_t          flow_label;     /* flow Lable IPv6only                              */
    ca_ipv6_ext_header_t ext_header;     /* IPv6 Extension Headers Type                      */
    ca_uint8_t           icmp_type;      /*ICMP message type                                 */
    ca_uint8_t           igmp_type;      /*IGMP message type                                 */
    ca_boolean_t         is_multicast;   /* valid if packet is multicast */
} ca_classifier_ip_t;

typedef struct {
    ca_uint32_t ip_valid     :1;
    ca_uint32_t ip_version   :1;
    ca_uint32_t ip_protocol  :1;
    ca_uint32_t dscp         :1;
    ca_uint32_t ecn          :1;

    ca_uint32_t ip_sa        :1;  /* enable/disable ip_sa field */
    ca_uint32_t ip_sa_max    :1;  /* 1: range max of ip_sa      */
    ca_uint32_t ip_da        :1;  /* enable/disable ip_da field */
    ca_uint32_t ip_da_max    :1;  /* 1: range max of ip_da      */

    ca_uint32_t fragment     :1;
    ca_uint32_t have_options :1;
    ca_uint32_t flow_label   :1;
    ca_uint32_t ext_header   :1;
    ca_uint32_t icmp_type    :1;
    ca_uint32_t igmp_type    :1;
    ca_uint32_t is_multicast :1;
} ca_classifier_ip_mask_t;


typedef enum {
    CA_KEY_HANDLE_TYPE_FLOW_ID,         /* flow id */
    CA_KEY_HANDLE_TYPE_GEM_INDEX,       /* gem index */
    CA_KEY_HANDLE_TYPE_LLID_COS         /* LLID_+ CoS */
} ca_classifier_handle_type_t;       /* classifier handle type */


typedef union {
    ca_uint16_t                 flow_id;        /* flow ID for following flow based control */
    ca_uint16_t                 gem_index;      /* gem_index = TCONT * 8 + CoS */
    ca_uint16_t                 llid_cos_index; /* llid_cos_index = llid_index*/
} ca_classifier_handle_t;

typedef struct {
    ca_port_id_t            orig_src_port; /* for traffic forwarded to L2RP ports */
    ca_port_id_t            src_port;
    ca_port_id_t            dest_port;     /* ACL: dest port based */
    ca_classifier_handle_t  key_handle;
    ca_classifier_handle_type_t handle_type;    /* valid when key_handle bit is set */
    ca_intf_id_t            src_intf;
    ca_intf_id_t            dst_intf;
    ca_classifier_l2_t      l2;       /* layer 2 fields    */
    ca_classifier_ip_t      ip;       /* IP related fields */
    ca_classifier_l4_t      l4;       /* layer 4 fields    */
    ca_classifier_ext_t     ext;      /* extended fields   */
    ca_uint8_t                  merge_prio;    /* in case of multiple hits with same priority value,
                                                                           FIBs from multiple hits will be merged in the following priorities:
                                                                           merge_prio (3)> merge_prio (2)> merge_prio (1)>prio_ merge_prio (0) */

    /*rule match if
         packet_length_high > ingress packet length > packet_length_low
     */
    ca_uint16_t packet_length_low;
    ca_uint16_t packet_length_high;

    /*rule match if ingress packet is from a LAN port*/
    ca_uint8_t ingress_lan;	/* TRUE=ingress packet from LAN, 0=from WAN */
} ca_classifier_key_t;

typedef struct {
    ca_uint32_t orig_src_port   : 1;
    ca_uint32_t src_port        : 1;
    ca_uint32_t dest_port       : 1;
    ca_uint32_t key_handle      : 1;
    ca_uint32_t src_intf        : 1;
    ca_uint32_t dst_intf        : 1;
    ca_uint32_t       l2        : 1;
    ca_uint32_t       ip        : 1;
    ca_uint32_t       l4        : 1;
    ca_uint32_t       ext       : 1;
    ca_uint32_t merge_prio     :1;
    ca_uint32_t packet_length:1;
    ca_uint32_t ingress_lan     :1;
    ca_classifier_l2_mask_t l2_mask;            /* valid only when l2 is set */
    ca_classifier_ip_mask_t ip_mask;            /* valid only when ip is set */
    ca_classifier_l4_mask_t l4_mask;            /* valid only when l4 is set */
} ca_classifier_key_mask_t;

typedef enum {
    CA_CLASSIFIER_FORWARD_DENY,      /* bypass L2/L3 Forwarding Engine, drop packet */
    CA_CLASSIFIER_FORWARD_FE,        /* forward to L2/L3 Forwarding Engine, do standard switching/routing */
    CA_CLASSIFIER_FORWARD_INTERFACE, /* bypass L2/L3 Forwarding Engine, forward to destination interface ID */
    CA_CLASSIFIER_FORWARD_PORT,      /* bypass L2/L3 Forwarding Engine, forward to dpid */
    CA_CLASSIFIER_FORWARD_TUNNEL,    /* forward to specific tunnel interface */
} ca_classifier_forward_flag_t;

typedef enum {
    CA_CLASSIFIER_VLAN_ACTION_NOP,	/* bypass VLAN action */
    CA_CLASSIFIER_VLAN_ACTION_PUSH,   	/* push a VLAN tag */
    CA_CLASSIFIER_VLAN_ACTION_POP,		/* pop the VLAN tag */
    CA_CLASSIFIER_VLAN_ACTION_SWAP,	/* swap the VLAN ID with given */
    CA_CLASSIFIER_VLAN_ACTION_SET		/* set to specified value */
} ca_classifier_vlan_action_t;

typedef struct {
    ca_classifier_vlan_action_t action;    /* VLAN swap is implemented by vid/tpid */

    ca_uint32_t                 tpid; /* TPID        */
    ca_uint32_t                 pri;  /* 802.1p, PCP */
    ca_uint32_t                 dei;  /* CFI or DEI  */
    ca_uint32_t                 vid;  /* VLAN ID     */
} ca_classifier_action_vlan_t;

typedef struct {
    ca_uint32_t action : 1;           /* VLAN swap ignore this bit */
    ca_uint32_t tpid   : 1;
    ca_uint32_t pri    : 1;
    ca_uint32_t dei    : 1;
    ca_uint32_t vid    : 1;
} ca_classifier_action_vlan_mask_t;

typedef struct {
    ca_uint32_t			action_handle :1;
    ca_uint32_t			priority	:1;
    ca_uint32_t			dscp		:1;
    ca_uint32_t			inner_dot1p	:1;
    ca_uint32_t			inner_tpid	:1;
    ca_uint32_t			inner_dei	:1;
    ca_uint32_t			inner_vlan_act	:1; /* Validate for vlan_act and vid */
    ca_uint32_t			outer_dot1p	:1;
    ca_uint32_t			outer_tpid	:1;
    ca_uint32_t			outer_dei	:1;
    ca_uint32_t			outer_vlan_act	:1; /* Validate for vlan_act and vid */
    ca_uint32_t			mac_da	:1;
    ca_uint32_t			sw_id	:1;         /* store marker for SW usage */
    ca_uint32_t			sw_shaper_id	:1; /* SW shaper id */
    ca_uint32_t 		mcg_id	:1; 		/* multicast group id returned from ca_l2/l3_mcast_entry_t */
} ca_classifier_action_option_mask_t;


typedef struct {
    ca_classifier_handle_t		action_handle;
    ca_classifier_handle_type_t handle_type;    /* valid when key_handle bit is set */
    ca_uint8_t					priority;  		    /* internal priority */
    ca_uint8_t					dscp; 			    /* new dscp value for remark */
    ca_uint16_t					inner_dot1p;		/* inner 802.1P for remark or new VLAN tag */
    ca_uint16_t					inner_tpid;		    /* inner TPID for remark or new VLAN tag */
    ca_uint16_t					inner_dei;  		/* inner DEI for Push/Swap VLAN */
    ca_uint16_t					inner_vid;  		/* inner VLAN ID for Push/Swap VLAN */
    ca_classifier_vlan_action_t 	inner_vlan_act;	/* inner VLAN action */
    ca_uint16_t					outer_dot1p;		/* outer 802.1P for remark or new VLAN tag */
    ca_uint16_t					outer_tpid;		    /* outer TPID for remark or new VLAN tag */
    ca_uint16_t					outer_dei;  		/* outer DEI for Push/Swap VLAN */
    ca_uint16_t					outer_vid;  		/* outer VLAN ID for Push/Swap VLAN */
    ca_classifier_vlan_action_t 	outer_vlan_act;	/* outer VLAN action */
    ca_mac_addr_t					mac_da;			/* destination MAC for intf only */
    ca_uint16_t					sw_id[4];		    /* store marker value for SW usage */
    ca_uint16_t					sw_shaper_id;  	    /* SW shaper ID */
    ca_uint32_t 				mcg_id; 			/* multicast group id returned from ca_l2/l3_mcast_entry_t */

    ca_classifier_action_option_mask_t masks;
} ca_classifier_action_option_t;

typedef enum {
    CA_CLASSIFIER_FORWARD_FE_NONE = 0,   		/* don't forward to layer 2/3 forward engine */
    CA_CLASSIFIER_FORWARD_FE_L3FE = 1,   		/* forward to layer 3 forward engine */
    CA_CLASSIFIER_FORWARD_FE_L2FE = 2			/* forward to layer 2 forward engine */
} ca_classifier_action_dest_fe_t;

typedef struct {
    ca_uint32_t	 drop; 	/* not use, reserved for forward = xxx_DENY */
    ca_classifier_action_dest_fe_t fe;    	/* forward to L2/3 forward engine*/
    ca_intf_id_t intf; 	/* destination for forward = xxx_INTERFACE */
    ca_port_id_t port; 	/* destination for forward = xxx_PORT */
    ca_port_id_t tunnel_id; 	/* destination for forward = xxx_TUNNEL */
    ca_uint8_t no_drop; /* use reserved buffer to ensure no drop */
} ca_classifier_action_dest_t;

typedef struct {
    ca_classifier_forward_flag_t    forward;
    ca_classifier_action_dest_t dest;
    ca_classifier_action_option_t options; /* not valid for drop */
} ca_classifier_action_t;


typedef struct {
    ca_classifier_forward_flag_t    action_type;
    ca_classifier_action_dest_t dest;
} ca_classifier_default_action_t;

typedef struct {
    ca_uint32_t index;
    ca_uint32_t priority;
    ca_classifier_key_t key;
    ca_classifier_key_mask_t key_mask;
    ca_classifier_action_t action;
} ca_classifier_rule_t;

ca_status_t ca_classifier_rule_add (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              priority,
    CA_IN  ca_classifier_key_t      *key,
    CA_IN  ca_classifier_key_mask_t *key_mask,
    CA_IN  ca_classifier_action_t   *action,
    CA_OUT ca_uint32_t              *index
);

ca_status_t ca_classifier_rule_get (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              index,
    CA_OUT ca_uint32_t              *priority,
    CA_OUT ca_classifier_key_t      *key,
    CA_OUT ca_classifier_key_mask_t *key_mask,
    CA_OUT ca_classifier_action_t   *action
);

ca_status_t ca_classifier_rule_iterate (
	CA_IN     ca_device_id_t   device_id,
	CA_OUT ca_iterator_t *return_entry
);

ca_status_t ca_classifier_rule_delete (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t index
);

ca_status_t ca_classifier_rule_delete_all (
    CA_IN ca_device_id_t device_id
);

ca_status_t ca_classifier_port_default_action_get (
	CA_IN     ca_device_id_t                    device_id,
	CA_IN     ca_port_id_t                   port_id,
	CA_OUT ca_classifier_default_action_t *p_action
);

ca_status_t ca_classifier_port_default_action_set (
	CA_IN ca_device_id_t                    device_id,
	CA_IN ca_port_id_t                   port_id,
	CA_IN ca_classifier_default_action_t *p_action
);

#if defined(CONFIG_LUNA_G3_SERIES)
typedef enum {
    CA_CLASSIFIER_AAL_NA = 0,
    CA_CLASSIFIER_AAL_L3_PPP_LCP = (1<<1),
    CA_CLASSIFIER_AAL_L3_PPP_IPCP    = (1<<2),
	CA_CLASSIFIER_AAL_L3_PPP_IP6CP = (1<<3),
	CA_CLASSIFIER_AAL_L3_IGNORE_SRC_PORT = (1<<4),
	CA_CLASSIFIER_AAL_L3_NOT_BROADCAST = (1<<5),
	CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX0 = (1<<6),
	CA_CLASSIFIER_AAL_L3_PPP_IPV4 = (1<<7),
	CA_CLASSIFIER_AAL_L3_PPP_IPV6 = (1<<8),
	CA_CLASSIFIER_AAL_L3_KEEP_ORIG = (1<<9),
	CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX1 = (1<<10),
	CA_CLASSIFIER_AAL_L3_SPEC_PKT_L4_L2TP_PORT = (1<<11),
	CA_CLASSIFIER_AAL_L3_MCAST_FORWARD = (1<<12),
	CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX2 = (1<<13),
	CA_CLASSIFIER_AAL_L3_RSLT_TYPE_0 = (1<<14),
	CA_CLASSIFIER_AAL_L3_MAC_DA_IP_MC = (1<<15),
	CA_CLASSIFIER_AAL_L3_MAC_DA_RSVD = (1<<16),
	CA_CLASSIFIER_AAL_L3_PPPOE_SESID_0 = (1<<17),
	CA_CLASSIFIER_AAL_L3_MAC_DA_AN_HIT = (1<<18),
	CA_CLASSIFIER_AAL_L3_POL_REMAP_GRP = (1<<19),
	CA_CLASSIFIER_AAL_END,
} ca_classifier_aal_customize_t;

ca_status_t ca_classifier_l3_cls_rule_add (
	CA_IN ca_device_id_t			device_id,
	CA_IN ca_uint32_t               priority,
	CA_IN ca_classifier_key_t       *key,
	CA_IN ca_classifier_key_mask_t  *key_mask,
	CA_IN ca_classifier_action_t    *action,
	CA_IN ca_uint32_t               *index,
	CA_IN unsigned int              aal_customize
);

ca_status_t ca_classifier_l3_cls_rule_idx_get (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              index,
    CA_OUT  ca_uint32_t 	*rule_idx
);

ca_status_t ca_classifier_l3_cls_rule_dump(
	CA_IN  ca_uint32_t 				hw_index,
	CA_IN  ca_uint32_t 				priority,
	CA_IN  ca_int32_t 				index
);

ca_status_t ca_used_aal_entry_print (
	CA_IN  ca_device_id_t              device_id
);

ca_status_t ca_used_aal_entry_print_by_fib_idx(
	CA_IN  ca_device_id_t              device_id,
	CA_IN  unsigned int                fib_idx
);
#endif

#endif /* __CLASSIFIER_H__ */

