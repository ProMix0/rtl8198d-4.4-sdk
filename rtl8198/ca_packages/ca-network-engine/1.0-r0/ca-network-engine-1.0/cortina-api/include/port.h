#ifndef __PORT_H__
#define __PORT_H__

#include "ca_types.h"
#include "classifier.h"


#ifdef CONFIG_ARCH_CORTINA_G3LITE/*CODEGEN_IGNORE_TAG*/
#define CA_PORT_MAX_NUM_ETHERNET         5
#endif

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU/*CODEGEN_IGNORE_TAG*/
#define CA_PORT_MAX_NUM_ETHERNET         4
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)/*CODEGEN_IGNORE_TAG*/
#define CA_PORT_MAX_NUM_ETHERNET         7
#elif defined(CONFIG_ARCH_CORTINA_G3)/*CODEGEN_IGNORE_TAG*/
#ifdef CONFIG_CORTINA_BOARD_FPGA/*CODEGEN_IGNORE_TAG*/
#define CA_PORT_MAX_NUM_ETHERNET         4
#else /*CODEGEN_IGNORE_TAG*/
#define CA_PORT_MAX_NUM_ETHERNET         8
#endif/*CODEGEN_IGNORE_TAG*/

#endif




#define CA_PORT_MAX_NUM_L2RP            2    /* L3_LAN, L3_WAN */
#define CA_PORT_MAX_NUM_CPU            43
#define CA_PORT_MAX_NUM_TRUNK            2
#define CA_PORT_MAX_NUM_MC            1

#define CA_PORT_MAX_NUM                64

#define CA_PORT_MAX_NUM_TRUNK_MEMBER    16

/******************************************************************************************************
* Port type check macros
*
******************************************************************************************************/
#define CA_PORT_VALID_TEST(/*CODEGEN_IGNORE_TAG*/_port, _port_type, min, max)     ((PORT_TYPE(_port)==_port_type) && (PORT_ID(_port) >= min) && (PORT_ID(_port) <= max))

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#define CA_PORT_IS_PON(/*CODEGEN_IGNORE_TAG*/_port)             ((PORT_TYPE(_port) == CA_PORT_TYPE_EPON || PORT_TYPE(_port) == CA_PORT_TYPE_GPON)&&(PORT_ID(_port) == CA_PORT_ID_NI7))
#else //8198F
#define CA_PORT_IS_PON(/*CODEGEN_IGNORE_TAG*/_port) false
#endif

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define CA_PORT_IS_ETHERNET(/*CODEGEN_IGNORE_TAG*/_port)    CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI3, CA_PORT_ID_NI7)
#define CA_PORT_IS_GE(/*CODEGEN_IGNORE_TAG*/_port)          CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI3, CA_PORT_ID_NI4)
#define CA_PORT_IS_XGE(/*CODEGEN_IGNORE_TAG*/_port)         CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI5, CA_PORT_ID_NI6)
#elif defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#define CA_PORT_IS_ETHERNET(/*CODEGEN_IGNORE_TAG*/_port)    CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI0, CA_PORT_ID_NI7)
#define CA_PORT_IS_GE(/*CODEGEN_IGNORE_TAG*/_port)          CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI0, CA_PORT_ID_NI4)
#define CA_PORT_IS_XGE(/*CODEGEN_IGNORE_TAG*/_port)         CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI5, CA_PORT_ID_NI6)
#else
#define CA_PORT_IS_ETHERNET(/*CODEGEN_IGNORE_TAG*/_port)    CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI0, CA_PORT_ID_NI7)
#define CA_PORT_IS_GE(/*CODEGEN_IGNORE_TAG*/_port)          CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI0, CA_PORT_ID_NI4)
#define CA_PORT_IS_XGE(/*CODEGEN_IGNORE_TAG*/_port)         CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI5, CA_PORT_ID_NI7)
#endif

#define CA_PORT_IS_CPU(/*CODEGEN_IGNORE_TAG*/_port)               CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_CPU, CA_PORT_ID_CPU0, CA_PORT_ID_CPU7)
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)
#define CA_PORT_IS_CPU_OR_VIRTUAL_PORT(/*CODEGEN_IGNORE_TAG*/_port)       CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_CPU, CA_PORT_ID_CPU0, CA_PORT_ID_L3_LAN)
#endif

#define CA_PORT_IS_L2RP(/*CODEGEN_IGNORE_TAG*/_port)                CA_PORT_VALID_TEST(_port, CA_PORT_TYPE_L2RP, CA_PORT_ID_L3_WAN, CA_PORT_ID_L3_LAN)

#define CA_PORT_ETHERNET_CHECK(/*CODEGEN_IGNORE_TAG*/port)        if (!CA_PORT_IS_ETHERNET(port)) { return CA_E_PARAM;}
#define CA_PORT_CPU_CHECK(/*CODEGEN_IGNORE_TAG*/port)           if (!CA_PORT_IS_CPU(port)) { return CA_E_PARAM;}
#define CA_PORT_L2RP_CHECK(/*CODEGEN_IGNORE_TAG*/port)            if (!CA_PORT_IS_L2RP(port)) { return CA_E_PARAM;}

#if defined(CONFIG_NE_PON)
#define CA_PORT_PHYSICAL_CHECK(/*CODEGEN_IGNORE_TAG*/port)        if (!CA_PORT_IS_PON(port) &&!CA_PORT_IS_ETHERNET(port)) { return CA_E_PARAM;}
#else
#define CA_PORT_PHYSICAL_CHECK(/*CODEGEN_IGNORE_TAG*/port)           if (!CA_PORT_IS_ETHERNET(port)) { return CA_E_PARAM;}
#endif

#if defined(CONFIG_NE_PON)
#define CA_PORT_ID_CHECK(/*CODEGEN_IGNORE_TAG*/port)        if (!CA_PORT_IS_PON(port) &&!CA_PORT_IS_ETHERNET(port) && !CA_PORT_IS_CPU(port) && !CA_PORT_IS_L2RP(port)) { return CA_E_PARAM;}
#else
#define CA_PORT_ID_CHECK(/*CODEGEN_IGNORE_TAG*/port)           if (!CA_PORT_IS_ETHERNET(port) && !CA_PORT_IS_CPU(port) && !CA_PORT_IS_L2RP(port)) { /*return CA_E_PARAM*/;}
#endif

#define CA_PORT_GPON_PORT                    CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7)
#define CA_PORT_EPON_PORT                    CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7)

#define CA_PORT_LAG_ID_CHECK(/*CODEGEN_IGNORE_TAG*/ gid)          if((gid) > CA_PORT_MAX_NUM_TRUNK-1) {return CA_E_PARAM;}

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define CA_WAN_TYPE_CPU_PORT_DEFAULT_ID     CA_PORT_ID_CPU0
#define CA_LAN_TYPE_CPU_PORT_DEFAULT_ID     CA_PORT_ID_CPU0 /*By default, only one interface exists on SFU */
#else
#define CA_WAN_TYPE_CPU_PORT_DEFAULT_ID     CA_PORT_ID_CPU0
#define CA_LAN_TYPE_CPU_PORT_DEFAULT_ID     CA_PORT_ID_CPU1
#endif

#define CA_PORT_IPG_GE_VAL_MIN             8
#define CA_PORT_IPG_GE_VAL_MAX             15
#define CA_PORT_IPG_XAUI_VAL_MIN           5
#define CA_PORT_IPG_XAUI_VAL_MAX           0xffffffff
#define CA_PORT_IPG_XGE_VAL_MIN            5
#define CA_PORT_IPG_XGE_VAL_MAX            0xffff

#define CA_PORT_VLAN_TAG_MAX 2

#if defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)
#define G3_LOOPBACK_DOWNSTREAM_VID_P0			0x10
#define	G3_LOOPBACK_DOWNSTREAM_VID_P1			0x20
#define G3_LOOPBACK_DOWNSTREAM_VID_P2			0x30
#define G3_LOOPBACK_DOWNSTREAM_VID_P3			0x40
#define G3_LOOPBACK_DOWNSTREAM_VID_BASE			G3_LOOPBACK_DOWNSTREAM_VID_P0
#define G3_LOOPBACK_DOWNSTREAM_VID(port, cos)	(G3_LOOPBACK_DOWNSTREAM_VID_BASE * (1+port) + cos)
#define G3_LOOPBACK_DOWNSTREAM_VID_MIN			G3_LOOPBACK_DOWNSTREAM_VID(AAL_LPORT_ETH_NI0,0)
#define G3_LOOPBACK_DOWNSTREAM_VID_MAX			G3_LOOPBACK_DOWNSTREAM_VID(AAL_LPORT_ETH_NI3,7)
#define G3_LOOPBACK_UPSTREAM_VID_P0				4000
#define G3_LOOPBACK_UPSTREAM_VID_P1				4001
#define G3_LOOPBACK_UPSTREAM_VID_P2				4002
#define G3_LOOPBACK_UPSTREAM_VID_P3				4003
#define G3_LOOPBACK_UPSTREAM_VID_BASE			G3_LOOPBACK_UPSTREAM_VID_P0
#define G3_LOOPBACK_UPSTREAM_VID(port)			(G3_LOOPBACK_UPSTREAM_VID_BASE + port)
#define G3_LOOPBACK_UPSTREAM_LSPID(VID)			(VID - G3_LOOPBACK_UPSTREAM_VID_BASE)
#define G3_LOOPBACK_UPSTREAM_VID_MIN			G3_LOOPBACK_UPSTREAM_VID(AAL_LPORT_ETH_NI0)
#define G3_LOOPBACK_UPSTREAM_VID_MAX			G3_LOOPBACK_UPSTREAM_VID(AAL_LPORT_ETH_NI3)
#define G3_LOOPBACK_P_START						AAL_LPORT_ETH_NI0
#define G3_LOOPBACK_P_END						AAL_LPORT_ETH_NI3
#define G3_LOOPBACK_P_MASK						((1 << AAL_LPORT_ETH_NI0) | (1 << AAL_LPORT_ETH_NI1) | (1 << AAL_LPORT_ETH_NI2) | (1 << AAL_LPORT_ETH_NI3))
#define G3_LOOPBACK_P_RSVSPA					AAL_LPORT_ETH_NI5		// DO NOT configure cls rule, merge p5/p6 entries for host policing
#define G3_LOOPBACK_P_NEWSPA					AAL_LPORT_ETH_NI6		// loopback from p6, igrcls(p6) for dmac host pol, egrcls(p25) for smac host pol.
#endif
#endif

typedef enum {
    CA_PORT_ENCAP_INTERNAL = 0,
    CA_PORT_ENCAP_ETH_E = 1,
    CA_PORT_ENCAP_ETH_1Q_E = 2,
    CA_PORT_ENCAP_ETH_QinQ_E = 3,
} ca_port_encap_type_t;

typedef struct {
    ca_port_encap_type_t type;
    ca_mac_addr_t src_mac;
    ca_uint32_t tag[CA_PORT_VLAN_TAG_MAX];
} ca_port_encap_t;

typedef enum {
    CA_PORT_STP_DISABLED = 0,
    CA_PORT_STP_BLOCKING = 1,
    CA_PORT_STP_LEARNING = 2,
    CA_PORT_STP_FORWARDING = 3,
    CA_PORT_STP_FWD_NO_LEARN = 4,
} ca_port_stp_state_t;

typedef enum {
    CA_PORT_LOOPBACK_NONE = 0,
    CA_PORT_LOOPBACK_MAC_LOCAL = 1,
    CA_PORT_LOOPBACK_MAC_REMOTE = 2,
    CA_PORT_LOOPBACK_PHY_LOCAL = 3,
    CA_PORT_LOOPBACK_PHY_REMOTE = 4
} ca_port_loopback_mode_t;

typedef struct {
    ca_boolean_t drop_untag;        /* drop or permit the untagged packets coming from this port_id. {FALSE, TRUE}         */
    ca_boolean_t drop_priority_tag; /* drop or permit the priority tagged packets coming from this port_id.{FALSE, TRUE}   */
    ca_boolean_t drop_multiple_tag; /* drop or permit multiple(>=2) tagged packets coming from this port_id. {FALSE, TRUE} */
    ca_boolean_t drop_single_tag;   /* drop or permit the tagged packets(one tag) coming from this port_id. {FALSE, TRUE}  */
} ca_port_discard_control_t;


#define CA_PORT_LOGIC_INDEX_SHIFT 0
#define CA_PORT_LOGIC_INDEX_MASK  0xFF

typedef enum {
    CA_DIRECTION_ENC_TX             = 0,        /* encrypt TX */
    CA_DIRECTION_ENC_RX             = 1,        /* decryt RX */
    CA_DIRECTION_ENC_BI             = 2        /* Bidirection */
} ca_port_direction_encryption_t;

typedef enum {
    CA_PORT_ENCRYPTION_MODE_DISABLE        = 0, /* disabled */
    CA_PORT_ENCRYPTION_MODE_AES_CTR        = 1, /* supported only on LLID */
    CA_PORT_ENCRYPTION_MODE_AES_CFB        = 2, /* supported only on LLID (DPoE) */
    CA_PORT_ENCRYPTION_MODE_MACSEC         = 3, /* supported on UNI and LLID */
    CA_PORT_ENCRYPTION_MODE_SW_CTC_CHURNING= 4, /* supported only on LLID */
    CA_PORT_ENCRYPTION_MODE_HW_CTC_CHURNING= 5  /* supported only on LLID */
} ca_port_encryption_mode_t;

ca_status_t ca_port_enable_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_boolean_t enable);

ca_status_t ca_port_enable_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_boolean_t * enable);

ca_status_t ca_port_max_frame_size_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t size,
                CA_IN ca_boolean_t include_tag);

ca_status_t ca_port_max_frame_size_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t *size,
                CA_OUT ca_boolean_t *include_tag);

ca_status_t ca_port_ipg_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id, CA_IN ca_uint32_t ipg);

ca_status_t ca_port_ipg_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * ipg);

ca_status_t ca_port_untagged_priority_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t priority);

ca_status_t ca_port_untagged_priority_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * priority);

ca_status_t ca_port_tpid_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t tpid);

ca_status_t ca_port_tpid_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * tpid);

ca_status_t ca_port_stp_state_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_port_stp_state_t stp_state);

ca_status_t ca_port_stp_state_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_port_stp_state_t * stp_state);

ca_status_t ca_port_isolation_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint8_t port_count,
                CA_IN ca_port_id_t *dst_ports /*[port_count:8] */);

ca_status_t ca_port_isolation_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint32_t port_id,
                CA_OUT ca_uint8_t * port_count,
                CA_OUT ca_port_id_t * dst_ports  /*[*port_count:8] */);

ca_status_t ca_port_discard_control_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_port_discard_control_t *control);

ca_status_t ca_port_discard_control_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_port_discard_control_t * control);

ca_status_t ca_port_station_move_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_boolean_t enable);

ca_status_t ca_port_station_move_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_boolean_t *enable);

ca_status_t ca_port_loopback_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_port_loopback_mode_t loopback);

ca_status_t ca_port_loopback_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_port_loopback_mode_t * loopback);

ca_status_t ca_port_pvid_vlan_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t vid);

ca_status_t ca_port_pvid_vlan_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * vid);

ca_status_t ca_port_pvid_tpid_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t tpid_index);

ca_status_t ca_port_pvid_tpid_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * tpid_index);

ca_status_t ca_port_pvid_dot1p_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint32_t priority);

ca_status_t ca_port_pvid_dot1p_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_OUT ca_uint32_t * priority);

ca_status_t ca_port_link_aggregation_group_create(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t lag_group_id,
                CA_IN ca_uint8_t member_port_count,
                CA_IN ca_port_id_t *member_ports,/*[member_port_count:8] */
                CA_IN ca_classifier_key_mask_t lag_hash_mask);

ca_status_t ca_port_link_aggregation_group_update(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t lag_group_id,
                CA_IN ca_uint8_t member_port_count,
                CA_IN ca_port_id_t *member_ports,/*[member_port_count:8] */
                CA_IN ca_classifier_key_mask_t lag_hash_mask);

ca_status_t ca_port_link_aggregation_group_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t lag_group_id,
                CA_OUT ca_uint8_t * member_port_count,
                CA_OUT ca_port_id_t *member_ports,/*[*member_port_count:8] */
                CA_OUT ca_classifier_key_mask_t *lag_hash_mask);

ca_status_t ca_port_link_aggregation_group_delete(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t lag_group_id);

ca_status_t ca_port_encryption_enable_set (
        CA_IN       ca_device_id_t                      device_id,
        CA_IN       ca_port_id_t                        port_id,
        CA_IN       ca_port_direction_encryption_t      direction,
        CA_IN       ca_boolean_t                        enable);

ca_status_t ca_port_encryption_enable_get (
        CA_IN       ca_device_id_t                      device_id,
        CA_IN       ca_port_id_t                        port_id,
        CA_OUT      ca_port_direction_encryption_t      *direction,
        CA_OUT      ca_boolean_t                        *enable);

ca_status_t ca_port_encryption_mode_set (
             CA_IN        ca_device_id_t                    device_id,
             CA_IN        ca_port_id_t                    port_id,
             CA_IN        ca_port_direction_encryption_t    direction,
             CA_IN        ca_port_encryption_mode_t        mode);

ca_status_t ca_port_encryption_mode_get (
             CA_IN        ca_device_id_t                    device_id,
             CA_IN        ca_port_id_t                    port_id,
             CA_OUT        ca_port_direction_encryption_t    *direction,
             CA_OUT        ca_port_encryption_mode_t        *mode);

ca_status_t ca_port_tx_traffic_block_get(
        CA_IN   ca_device_id_t  device_id,
        CA_IN   ca_port_id_t    port_id,
        CA_OUT  ca_boolean_t    *enable);
        
ca_status_t ca_port_tx_traffic_block_set(
        CA_IN   ca_device_id_t  device_id,
        CA_IN   ca_port_id_t    port_id,
        CA_IN   ca_boolean_t    enable);

ca_status_t ca_port_egr_qm_block_set(
        CA_IN   ca_device_id_t  device_id,
        CA_IN   ca_port_id_t    port_id,
        CA_IN   ca_boolean_t    enable);
        
ca_status_t ca_port_egr_qm_block_get(
        CA_IN   ca_device_id_t  device_id,
        CA_IN   ca_port_id_t    port_id,
        CA_OUT  ca_boolean_t    *enable);

ca_status_t ca_port_serdes_rx_equalization_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_port_id_t   port_id,
	CA_IN ca_uint8_t      equalization_db);

ca_status_t ca_port_serdes_rx_equalization_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_port_id_t   port_id,
	CA_OUT ca_uint8_t*    equalization_db);

ca_status_t ca_port_serdes_tx_preemphasis_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_port_id_t   port_id,
	CA_IN ca_uint16_t    main_cursor_amp,
	CA_IN ca_uint8_t     pre_cursor_amp,
	CA_IN ca_uint8_t     post_cursor_amp);

ca_status_t ca_port_serdes_tx_preemphasis_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_port_id_t   port_id,
	CA_OUT ca_uint16_t*  main_cursor_amp,
	CA_OUT ca_uint8_t*   pre_cursor_amp,
	CA_OUT ca_uint8_t*   post_cursor_amp);
#endif
