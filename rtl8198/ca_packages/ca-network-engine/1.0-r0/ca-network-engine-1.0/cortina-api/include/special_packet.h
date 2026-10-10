#ifndef __SPECIAL_PACKET_H__
#define __SPECIAL_PACKET_H__

#include "ca_types.h"
#include "rx.h"
#include "classifier.h"



/* special packet option mask definition */
typedef struct ca_special_packet_option_mask_s {
    /* user defined special packet type, used when ca_pkt_type_t is CA_PKT_TYPE_USER_DEFINED */
    ca_uint32_t    user_defined_type:1;
    ca_uint32_t    forward_original:1;      /* do not modify packet (bypass all packet editing in L2FE) */
    ca_uint32_t    enable_sa_learning:1;    /* enable MAC learning for special packet */
    ca_uint32_t    do_not_drop:1;           /* use reserve receive buffer. Do not drop packet when shared buffer is full */
    ca_uint32_t    action_handle:1;         /* forward specail packet to flow/GEM port/LLID port */
} ca_special_packet_option_mask_t;


/* user defined ether type + MAC DA + 48 bit MAC DA mask to be treated as special packet */
typedef struct ca_user_defined_special_packet_mask_s {
    ca_uint32_t    ether_type:1;        /* user defined ether type to be treated as special packet */
    ca_uint32_t    mac_da:1;            /* user defined MAC DA to be treated as special packet */
    ca_uint32_t    mac_da_mask:1;       /* 48 bit MAC DA mask */
} ca_user_defined_special_packet_mask_t;

/* special packet option definition */
typedef struct ca_user_defined_special_packet_s {
    ca_user_defined_special_packet_mask_t   mask;
    ca_uint16_t                             ether_type;
    ca_mac_addr_t                           mac_da;
    ca_mac_addr_t                           mac_da_mask;
} ca_user_defined_special_packet_t;

/* special packet option definition */
typedef struct ca_special_packet_option_s {
    ca_special_packet_option_mask_t     mask;
    ca_uint8_t      forward_original;           /* 1=forward original packet, bypass packet editing logic */
    ca_uint8_t      enable_sa_learning;         /* 1=enable MAC SA learning, 0=bypass MAC SA learning */
    ca_uint8_t      do_not_drop;                /* 0=drop if buffer full. 1 = use reserved buffer, never drop */
    ca_user_defined_special_packet_t    user_defined_special_packet;    /* user defined special packet type  */
    ca_classifier_handle_t              action_handle;    /* valid when mask. action_hanlde = 1 */
} ca_special_packet_option_t;


ca_status_t ca_special_packet_set (
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_port_id_t     source_port,
    CA_IN ca_pkt_type_t    special_packet_type,
    CA_IN ca_boolean_t     enable,
    CA_IN ca_port_id_t     dest_port,
    CA_IN ca_uint8_t       priority,
    CA_IN ca_special_packet_option_t   *options
);

ca_status_t ca_special_packet_get (
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t     source_port,
    CA_IN  ca_pkt_type_t    special_packet_type,
    CA_OUT ca_boolean_t     *enable,
    CA_OUT ca_port_id_t     *dest_port,
    CA_OUT ca_uint8_t       *priority,
    CA_OUT ca_special_packet_option_t   *options
);



#endif
