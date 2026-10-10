#ifndef __TUNNEL_H__
#define __TUNNEL_H__

#include "ca_types.h"
#include "port.h"

/***************************************************************************
 * Common
 ***************************************************************************/

#define CA_TUNNEL_MAX_LIMIT	64

typedef enum {
	CA_TUNNEL_DIR_OUTBOUND	= 0,
	CA_TUNNEL_DIR_INBOUND	= 1,

	CA_IPSEC_SPD_ENCRYPT	= CA_TUNNEL_DIR_OUTBOUND,
	CA_IPSEC_SPD_DECRYPT	= CA_TUNNEL_DIR_INBOUND,

	CA_PPTP_DIR_UPSTREAM	= CA_TUNNEL_DIR_OUTBOUND,
	CA_PPTP_DIR_DOWNSTREAM	= CA_TUNNEL_DIR_INBOUND,
} ca_tunnel_direction_t;

typedef enum {
	CA_FUNC_ID_INVALID = 0,

	CA_FUNC_ID_L2TP = 1,		/* L2TPv2 */
	CA_FUNC_ID_L2TPV3 = 2,
	CA_FUNC_ID_IPSEC = 3,
	CA_FUNC_ID_L2TP_IPSEC = 4,
	CA_FUNC_ID_L2TPV3_IPSEC = 5,
	CA_FUNC_ID_MACSEC = 6,
	CA_FUNC_ID_PPTP = 7,
	CA_FUNC_ID_RTP = 8,
	CA_FUNC_ID_MAP_E = 9,
	CA_FUNC_ID_MAP_T = 10,

	CA_FUNC_ID_MAX,
} ca_vpn_offload_function_id;

typedef enum {
	CA_PKT_TYPE_FROM_NE = 0,	/* LAN -> PE, WAN -> PE */
	CA_PKT_TYPE_FROM_CPU = 1,	/* CPU -> PE */
} ca_vpn_offload_pkt_type;

typedef struct {
	union {
		struct {
			ca_vpn_offload_function_id  func_id    :4;
			ca_vpn_offload_pkt_type     pkt_type   :1;
			ca_tunnel_direction_t       direction  :1;
			ca_uint8_t                  rsv        :2;
		};
		ca_uint8_t value;
	};
} __attribute__((packed)) tunnel_info_t;

/***************************************************************************
 * IPSec
 ***************************************************************************/

#define MAX_ENCRYPTION_KEY_LEN	32
#define MAX_AUTH_KEY_LEN	64

typedef enum {
	CA_IPSEC_SA_PROTO_ESP	= 0,
	CA_IPSEC_SA_PROTO_AH	= 1,
	CA_IPSEC_SA_PROTO_MAX	= 2,
} ca_ipsec_sa_proto_t;

typedef enum {
	CA_IPSEC_POLICY_ACTION_IPSEC	= 1, /* Apply IPSec    */
	CA_IPSEC_POLICY_ACTION_DISCARD	= 2, /* Discard / Drop */
	CA_IPSEC_POLICY_ACTION_BYPASS	= 3, /* Allow to pass  */
} ca_ipsec_policy_action_t;

typedef enum ca_ipsec_ciph_alg {
	CA_IPSEC_CIPHER_NULL	= 0,
	CA_IPSEC_CIPHER_DES	= 1,
	CA_IPSEC_CIPHER_3DES	= 2, /* triple key length of DES's */
	CA_IPSEC_CIPHER_AES	= 3,
} ca_ipsec_cipher_alg_t;

typedef enum ca_ipsec_cipher_mode {
	CA_IPSEC_CIPHER_MODE_ECB	= 0,
	CA_IPSEC_CIPHER_MODE_CBC	= 1,
	CA_IPSEC_CIPHER_MODE_CTR	= 2,
	CA_IPSEC_CIPHER_MODE_CCM	= 3,
	CA_IPSEC_CIPHER_MODE_GCM	= 5,
	CA_IPSEC_CIPHER_MODE_OFB	= 7,
	CA_IPSEC_CIPHER_MODE_CFB	= 8,
} ca_ipsec_cipher_mode_t;

typedef enum ca_ipsec_hash_alg {
	CA_IPSEC_HASH_NULL	= 0,
	CA_IPSEC_HASH_MD5	= 1,
	CA_IPSEC_HASH_SHA1	= 2,
	CA_IPSEC_HASH_SHA256	= 3,
	CA_IPSEC_HASH_SHA384	= 4,
	CA_IPSEC_HASH_SHA512	= 5,
} ca_ipsec_hash_alg_t;

typedef struct ca_ipsec_sa {
	ca_uint8_t	replay_window;
	ca_uint32_t	spi;			/* same as spi captured in Wireshark which is network order */
	ca_uint32_t	sequence_number;
	ca_uint8_t	ekey[MAX_ENCRYPTION_KEY_LEN];
	ca_uint8_t	akey[MAX_AUTH_KEY_LEN];
	ca_uint32_t	ip_version;		/* 0=IPv4, 1=IPv6         */
	ca_uint32_t	protocol;		/* 0=ESP; 1=AH            */
	ca_uint32_t	tunnel;			/* 0=Transport; 1=Tunnel  */

	/* encryption mode based on elliptic                              */
	ca_uint32_t	ealg;			/* ca_ipsec_cipher_alg_t  */
	ca_uint32_t	ealg_mode;		/* ca_ipsec_cipher_mode_t */
	ca_uint32_t	encryption_keylen;	/* (n * 4) bytes          */
	ca_uint32_t	iv_len;			/* (n * 4) bytes for CBC  */

	/* authentication mode based on elliptic                          */
	ca_uint32_t	aalg;			/* ca_ipsec_hash_alg_t    */
	ca_uint32_t	auth_keylen;
	ca_uint32_t	icv_trunclen;		/* (n * 4) bytes          */
	ca_uint32_t	etherIP;
	ca_uint32_t	reserved;

	ca_uint32_t	is_natt;		/* NAT-Traversal          */
	ca_uint16_t	src_l4_port;		/* UDP port for IPSec NAT-Traversal */
	ca_uint16_t	dest_l4_port;		/* UDP port for IPSec NAT-Traversal (default 4500) */
	ca_tunnel_direction_t	sa_dir;		/* SA direction, see definition of ca_tunnel_direction_t */
} ca_ipsec_sa_t;

typedef struct ca_ipsec_tunnel_cfg_s {
	ca_ipsec_sa_id_t	sa_id_encrypt;	/* Ingress IPSEC policy for tunnel */
	ca_ipsec_sa_id_t	sa_id_decrypt;	/* Egress IPSEC policy for tunnel  */
} ca_ipsec_tunnel_cfg_t;

typedef struct ca_ipsec_sa_iterate_entry_s {
	ca_ipsec_sa_t		ipsec_sa;
	ca_ipsec_sa_id_t	sa_id;
} ca_ipsec_sa_iterate_entry_t;

/***************************************************************************
 * MACSec
 ***************************************************************************/

typedef struct ca_macsec_tunnel_cfg_s {
	ca_macsec_sa_id_t	tx_sa;			/* Tx MACSec SA for tunnel */
	ca_macsec_sa_id_t	rx_sa;			/* Rx MACSec SA for tunnel  */
} ca_macsec_tunnel_cfg_t;

/***************************************************************************
 * PPPoE
 ***************************************************************************/

#include "rx.h"

/***************************************************************************
 * L2TP
 ***************************************************************************/

#define L2TPv3_COOKIE_SIZE	8

typedef struct ca_l2tp_tunnel_cfg_s {
	ca_uint16_t		version;		/* Version and flags */
	ca_uint16_t		len;			/* Optional Length */
	ca_uint16_t		tunnel_id;		/* Tunnel ID */
	ca_uint16_t		dest_l4_port;		/* UDP port of L2TP hdr */
	ca_uint16_t		src_l4_port;		/* UDP port of L2TP hdr */
	ca_uint32_t		session_id;		/* Session ID */

	/* For L2TPv3 */
	ca_uint16_t		encap_type;		/* 1: IP, 0: UDP */
	ca_uint16_t		l2_specific_len;	/* default is 4 */
	ca_uint16_t		l2_specific_type;	/* 0: NONE, 1: DEFAULT */
	ca_uint8_t		send_seq;		/* 0: Not send sequence */
	ca_uint8_t		calc_udp_csum;		/* For IPv6 outer IP, calculate UDP checksum of L2TP header.
							     0: UDP checksum is zero
							     1: calculate UDP checksum
							 */
	ca_uint32_t		sequence_number;	/* If send_seq, set the start of Session NR state */

	ca_uint16_t		cookie_len;
	ca_uint8_t		cookie[L2TPv3_COOKIE_SIZE];
	ca_uint16_t		offset;			/* offset from the end of L2TP header to beginning of data */
	ca_mac_addr_t		l2tp_src_mac;
	ca_mac_addr_t		peer_l2tp_src_mac;
} ca_l2tp_tunnel_cfg_t;

/***************************************************************************
 * PPTP
 ***************************************************************************/

#define CA_PPTP_DEFAULT_KEY_LENGTH	16

typedef enum {
	CA_CRYPTO_INVALID	= -1,
	CA_CRYPTO_NONE		= 0,
	CA_CRYPTO_MPPE40,
	CA_CRYPTO_MPPE128,
} ca_pptp_crypto_type_t;

typedef struct ca_pptp_sa_s {
	ca_tunnel_direction_t	direction;	/* 0=outbound, 1=inbound.		*/
	ca_uint16_t		call_id;	/* local call ID			*/

	ca_boolean_t		state_less;	/* TRUE/FALSE				*/
	ca_pptp_crypto_type_t	crypto_type;

	ca_uint8_t		key[CA_PPTP_DEFAULT_KEY_LENGTH];/* Start Key		*/
	ca_uint8_t		keylen;
	ca_uint32_t		sequence_number;
} ca_pptp_sa_t;

typedef struct ca_pptp_tunnel_cfg_s {
	ca_pptp_sa_id_t		sa_id_outbound;	/* Egress PPTP SA for tunnel		*/
	ca_pptp_sa_id_t		sa_id_inbound;	/* Ingress PPTP SA for tunnel		*/
} ca_pptp_tunnel_cfg_t;

typedef struct ca_pptp_sa_iterate_entry_s {
	ca_pptp_sa_t		pptp_sa;
	ca_pptp_sa_id_t		sa_id;
} ca_pptp_sa_iterate_entry_t;

/***************************************************************************
 * DS-Lite
 ***************************************************************************/

typedef struct ca_dslite_config_s {
	/* For Unicast,
	   ca_tunnel_cfg_t.src_addr = B4 addr
	   ca_tunnel_cfg_t.dest_addr = AFTR addr

	   For Multicast,
	     ca_tunnel_cfg_t.src_addr = Multicast group addr
	       ca_tunnel_cfg_t.src_addr.ipv6_addr[0] and ipv6_addr[1] is MPrefix64.
	       ca_tunnel_cfg_t.src_addr.ipv6_addr[2] and ipv6_add3[2] is MPrefix64_mask.
	     ca_tunnel_cfg_t.dest_addr = Multicast source addr
	       ca_tunnel_cfg_t.dest_addr.ipv6_addr[0] and ipv6_addr[1] is UPrefix64.
	       ca_tunnel_cfg_t.dest_addr.ipv6_addr[2] and ipv6_add3[2] is UPrefix64_mask.
	 */
	/* ingress tunnel hdr validation check includes:
	   1. Check packet IPv6 prefix matches the configured tunnel end point prefix
	   2. Check packet IPv6 ecn bits matches the IPv4 ECN bits
	   3. Outer packet IPv6 DA, SA /IPv4 part = inner IPv4 DA, SA in case of MC packet
	 */

	/* When '1', filter outer IP is MC, and inner IP is UC packet */
	ca_boolean_t		dsl_ipmc_illegal_check_en;

	/* When '1', the outer IPv6 is UC, inner IPv4 is MC packet, also treated as DSLite IP MC.
	   Change MAC DA to multcast MAC DA and forward packet to L2 forward engine.
	 */
	ca_boolean_t		dsl_ipmc_to_l2fe;

	/* When '1', support the DSLite IPMC packet outer address MPrefix/UPrefix check */
	ca_boolean_t		dsl_ipmc_addr_prefix_check_en;

	/* When '1', support the DSLite IPMC packet outer address and inner address consistency check */
	ca_boolean_t		dsl_ipmc_addr_consistency_check_en;

	/* When '1', incoming packet outer IPv6DA and IPv6SA match with B4 and AFTR address for non-outer IPMC packet.
	 * If doesn't match, check failure. Packet will be dropped or redirected.
	 */
	ca_boolean_t		dsl_reverse_path_check_en;

	/* When '1', the outer TTL field will be kept while performing the DSLite outer IP pop */
	ca_boolean_t		dsl_ttl_keep_outer;

	/* When '1', the outer DSCP field will be kept while performing the DSLite outer IP pop */
	ca_boolean_t		dsl_dscp_keep_outer;

	/* When '1', the outer ECN field will be kept while performing the DSLite outer IP pop */
	ca_boolean_t		dsl_ecn_keep_outer;

	ca_boolean_t		dsl_ecn_check_enable;		/* check ECN of outer and inner header match */
	ca_boolean_t 		rebuild_mc_mac;			/* rebuild L2 MC hdr based on inner IP version */
	ca_boolean_t		mc_flag;			/* 1: multicast tunnel (only decapsulation is supported) 0: unicast */
	ca_mac_addr_t		mc_mac;				/* lower 25 bits (MSB) specified here */
	ca_boolean_t		mc_drop;			/* drop received MC packets */
	ca_port_id_t		validation_check_failed_dest_port;

	/* encap parameters */
	ca_uint8_t		egress_tc_value;
	ca_uint8_t		egress_hoplimit_value;
	ca_uint32_t		egress_flow_label_value;
} ca_dslite_config_t;

/***************************************************************************
 * 6RD
 ***************************************************************************/

typedef struct ca_6rd_config_s {
	/* For Unicast,
	     ca_tunnel_cfg_t.src_addr = CE: Customer Edge IPv4 addr
	     ca_tunnel_cfg_t.dest_addr = BR: Border Relay IPv4 addr

	   For Multicast,
	     ca_tunnel_cfg_t.src_addr = Multicast group addr
	     ca_tunnel_cfg_t.dest_addr = Multicast source addr
	 */

	/* 6rd delegated IPv6 prefix and length */
	ca_ip_address_t		ipv6_6rd_prefix;

	/* Used to extract top bits for IP_DA_v4,and calculate the length of rest of IPv4 address bits in IPv4 address */
	ca_uint8_t		six_rd_v4_mask_len;

	/* 1: Use v6 IP_DA's IPv4 part as outer IPv4 DA.
	 * 0: use ca_tunnel_cfg_t.dest_addr as outer IPv4 DA.
	 */
	ca_boolean_t		six_rd_ipda_from_v6;

	/* ingress tunnel hdr validation check includes:
	   1. Check packet IPv6 DA delegated prefix matches the configured tunnel local end point prefix.
	      This will be the CE to CE case. Else it is the BR/CE case
	   2. Packet IPv4 DA/SA match configured tunnel IPv4 DA/SA (tunnel end point check)
	   3. Tunnel IPv4 header suffix matches configured delegated IPv6 prefix/IPv4 suffix part
	   4. 6RD domain is defined by IPv4 subnet and delegated ipv6 prefix. The address format is
	      <outer ipv4> <inner ipv6 == (delegated ipv6 prefix | ipv4 suffix )>
	 */

	ca_uint8_t		six_rd_v6_prefix_len;		/* for 6rd delegated addr calculation & check when decap */
	ca_boolean_t		six_rd_ingress_check_en;		/* enable ingress validation check */
	ca_boolean_t		six_rd_ipsa_match;		/* Check for IPSA_v4v6 match when traffic from BR */

	/* When '1', filter outer IP is MC, and inner IP is UC packet */
	ca_boolean_t		six_rd_ipmc_illegal_check_en;

	/* When '1', the outer TTL field will be kept while performing the 6RD outer IP pop */
	ca_boolean_t		six_rd_ttl_keep_outer;

	/* When '1', the outer DSCP field will be kept while performing the 6RD outer IP pop */
	ca_boolean_t		six_rd_dscp_keep_outer;

	/* When '1', the outer ECN field will be kept while performing the 6RD outer IP pop */
	ca_boolean_t		six_rd_ecn_keep_outer;

	ca_boolean_t		six_rd_ecn_check_enable;	/* check ECN of outer match that of inner hdr */

	/* Check packet IPv6 DA delegated prefix matches the configured tunnel local end point prefix.
	 * This will be the CE to CE case. Else it is the BR/CE case.
	 */
	ca_boolean_t		ce_ce_allowed;			/* allow CE/CE traffic */
	ca_boolean_t		mc_drop;			/* drop received MC packets */
	ca_port_id_t		validation_check_failed_dest_port;

	ca_boolean_t 		rebuild_mc_mac;			/* rebuild L2 MC hdr based on inner IP version */
	ca_mac_addr_t		mc_mac;				/* lower 16 bits (MSB) specified here */

	/* encap parameters */
	ca_uint8_t		egress_tos_value;
	ca_uint8_t		egress_ttl_value;
	ca_uint16_t		egress_identification_start;
	ca_uint16_t		egress_identification_end;
} ca_6rd_config_t;

/***************************************************************************
 * MAP-E / MAP-T
 *
 * BMR needs the following parameters:
 * - Rule IPv6 prefix (n bits), ca_map_config_t::ipv6_prefix
 *   It is embedded with IPv4 suffix (p bits), PSID.
 * - Rule IPv4 prefix (r bits), ca_map_config_t::ipv4_prefix
 * - Rule EA-bit length (o bits), ca_map_config_t::ea_bit_length
 *
 * Other required parameters:
 * - End-user IPv6 prefix, ca_tunnel_cfg_t::src_addr
 * - BR IPv6 addr for MAP-E, or BR IPv6 prefix for MAP-T, ca_tunnel_cfg_t::dest_addr
 * - PSID offset (a bits), ca_map_config_t::psid_offset (default is 6)
 * - PSID (assigned? calculated from End-user IPv6 addr?)
 *
 * Calculated from above parameters:
 * - IPv4 suffix length (p bits), 32 - r bits
 * - PSID length (q bits, k bits), o bits - p bits
 * - CE IPv6 addr
 * - CE IPv4 addr
 ***************************************************************************/

typedef enum {
	MAP_E = 0,		/* RFC7597: MAP-E */
	MAP_T = 1,		/* RFC7599: MAP-T */
	MAP_E_DRAFT_03 = 2,	/* draft-ietf-softwire-map-03: MAP-E draft03 */

	MAP_TUNNEL_TYPE_MAX,
} ca_map_tunnel_type;

typedef struct ca_map_config_s {
	ca_map_tunnel_type	map_type;

	/* Rule IPv6 prefix (RFC7597/7599: n bits), tunnel interface identified by IPv6 prefix */
	ca_ip_address_t		ipv6_prefix;

	/* Rule IPv4 prefix (RFC7597/7599: size of p bits can be derived from this) */
	ca_ip_address_t		ipv4_prefix;

	/* Rule EA-bit length (o bits) */
	ca_uint8_t		ea_bit_length;

	/* PSID offset (RFC7597/7599: a bits, this will determine number of SPORT that can be used by SNAT) */
	ca_uint8_t		psid_offset;

	/* PSID length (RFC7597/7599: k bits), EA bit size = IPv4 suffix length + PSID length */
	ca_uint8_t		psid_length;

	/* PSID value (RFC7597/7599 port set id), source ports used in SNAT must fall within this range */
	ca_uint16_t		psid_id;

	/* destination port to forward the packet if packet validation check failed
	 * (IPv4 address mismatch RFC7597 section 8.1, RFC7599 section 8.2, default is blackhole)
	 */
	ca_port_id_t		validation_check_failed_dest_port;

	/* encap parameters */
	ca_uint8_t		egress_tc_copied_from_ipv4_tos; /* 1: TC is IPv4 ToS, 0: static value */
	ca_uint8_t		egress_tc_value;
	ca_uint8_t		egress_hoplimit_value;		/* 0: use packet's TTL */
	ca_uint32_t		egress_flow_label_value;
} ca_map_config_t;

/***************************************************************************
 * Tunnel API
 ***************************************************************************/

typedef enum {
	CA_TUNNEL_MIN		= 0,

	CA_TUNNELINVALID	= CA_TUNNEL_MIN,
	CA_TUNNEL_PPPOE		= 1,
	CA_TUNNEL_L2TP		= 2,
	CA_TUNNEL_IPSEC		= 3,
	CA_TUNNEL_PPTP		= 4,
	CA_TUNNEL_4_IN_4	= 5,
	CA_TUNNEL_DSLITE	= 6,
	CA_TUNNEL_6RD		= 7,
	CA_TUNNEL_6_IN_6	= 8,
	CA_TUNNEL_MAP_E		= 9,
	CA_TUNNEL_MAP_T		= 10,
	CA_TUNNEL_MACSEC	= 11,

	CA_TUNNEL_MAX,
} ca_tunnel_type_t;

typedef struct ca_tunnel_cfg_union_s {
	ca_pppoe_tunnel_cfg_t	pppoe;
	ca_l2tp_tunnel_cfg_t	l2tp;
	ca_ipsec_tunnel_cfg_t	ipsec;
	ca_pptp_tunnel_cfg_t	pptp;
	ca_dslite_config_t	dslite;
	ca_6rd_config_t		six_rd;
	ca_map_config_t		map;			/* MAP-E (RFC7597) or MAP-T (RFC7599) */
	ca_macsec_tunnel_cfg_t	macsec;
} ca_tunnel_cfg_union_t;

typedef struct ca_tunnel_cfg_s {
	ca_tunnel_type_t	type;			/* tunnel type                       */
	ca_ip_address_t		dest_addr;		/* tunnel destination IP             */
	ca_ip_address_t		src_addr;		/* tunnel src IP                     */
	ca_intf_id_t		parent_l3_intf_id;	/* Parent L3 interface of the tunnel */
	ca_tunnel_cfg_union_t	tunnel;
} ca_tunnel_cfg_t;

typedef struct ca_tunnel_iterate_entry_s {
	ca_tunnel_id_t		tunnel_id;
	ca_tunnel_cfg_t		tunnel;
} ca_tunnel_iterate_entry_t;

ca_status_t ca_ipsec_init (
	CA_IN		ca_device_id_t		device_id );

ca_status_t ca_ipsec_shut (
	CA_IN		ca_device_id_t		device_id );

ca_status_t ca_ipsec_sa_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_t		*ipsec_sa,
	CA_OUT		ca_ipsec_sa_id_t	*ipsec_sa_id );

ca_status_t ca_ipsec_sa_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_id_t	ipsec_sa_id );

ca_status_t ca_ipsec_sa_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_id_t	ipsec_sa_id,
	CA_OUT		ca_ipsec_sa_t		*ipsec_sa );

ca_status_t ca_ipsec_sa_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry);

ca_status_t ca_pptp_sa_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_t		*pptp_sa,
	CA_OUT		ca_pptp_sa_id_t		*pptp_sa_id );

ca_status_t ca_pptp_sa_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_id_t		pptp_sa_id );

ca_status_t ca_pptp_sa_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_id_t		pptp_sa_id,
	CA_OUT		ca_pptp_sa_t		*pptp_sa );

ca_status_t ca_pptp_sa_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry );

ca_status_t ca_pptp_key_change (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id );

ca_status_t ca_tunnel_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_cfg_t		*tunnel_cfg,
	CA_OUT		ca_tunnel_id_t		*tunnel_id );

ca_status_t ca_tunnel_update (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_cfg_t		*tunnel_cfg,
	CA_IN		ca_tunnel_id_t		tunnel_id );

ca_status_t ca_tunnel_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id );

ca_status_t ca_tunnel_delete_all (
	CA_IN		ca_device_id_t		device_id );

ca_status_t ca_tunnel_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id,
	CA_OUT		ca_tunnel_cfg_t		*tunnel_cfg );

ca_status_t ca_tunnel_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry );

/***************************************************************************
 * Internal Use
 ***************************************************************************/

typedef struct {/*CODEGEN_IGNORE_TAG*/
	int hash_idx_upstream;			/* upstream ciphertext */
	int hash_idx_downstream;		/* downstream ciphertext */

	unsigned int idx4pe;			/* only valid when offloading to PE */
} ipsec_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	unsigned int idx4pe;			/* only valid when offloading to PE */
} macsec_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	ca_uint32_t sa_id_outbound;		/* used by any plaintext tunnel */
	ca_uint32_t sa_id_inbound;		/* used by any plaintext tunnel */

	unsigned int idx4pe;			/* only valid when offloading to PE */
} l2tp_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	ca_uint32_t sa_id_outbound;		/* used by any plaintext tunnel */
	ca_uint32_t sa_id_inbound;		/* used by any plaintext tunnel */
} pptp_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	int aal_idx;				/* AAL index in DSLite Table */
} dslite_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	int aal_idx;				/* AAL index in 6RD Table */
} sixrd_private_data_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
	int aal_idx;				/* AAL index in DSLite Table */
	ca_ip_address_t ce_ipv4;		/* CE IPv4 address */

	ca_uint8_t offload_in_ne;		/* used by MAP-E only */
	unsigned int idx4pe;			/* only valid when offloading to PE */
} map_private_data_t;

typedef struct ca_tunnel_s {/*CODEGEN_IGNORE_TAG*/
	ca_boolean_t used;
	ca_tunnel_cfg_t tcfg;
	void* private_data;
} ca_tunnel_t;

#endif /* __TUNNEL_H__ */

