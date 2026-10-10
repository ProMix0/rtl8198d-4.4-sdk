#ifndef __TUNNEL_IPC_INTERNAL_H__
#define __TUNNEL_IPC_INTERNAL_H__

#include "ca_types.h"
#include "tunnel.h"

/***************************************************************
 * CA_IPC_PE_MTU
 ***************************************************************/

typedef struct {
	ca_tunnel_type_t	tunnel_type;
	ca_uint32_t		sa_id;
	ca_uint16_t		mtu;
} __attribute__((packed)) ca_ipc_pe_mtu_set_t;

typedef struct {
	ca_tunnel_type_t	tunnel_type;
	ca_uint32_t		sa_id;
} __attribute__((packed)) ca_ipc_pe_mtu_get_t;

typedef struct {
	ca_uint32_t		ret_value;
	ca_tunnel_type_t	tunnel_type;
	ca_uint32_t		sa_id;
	ca_uint16_t		mtu;
} __attribute__((packed)) ca_ipc_pe_mtu_get_ack_t;

/***************************************************************
 * CA_IPSEC_IPC_SADB
 ***************************************************************/

/* set SADB */

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_uint8_t		replay_window;
	ca_uint32_t		spi;			/* NOTE: network order in memory */
	ca_uint32_t		seq_num;
	ca_uint32_t		ip_ver:1;		/* 0=IPv4, 1=IPv6         */
	ca_uint32_t		proto:1;		/* 0=ESP; 1=AH            */
	ca_uint32_t		tunnel:1;		/* 0=Transport; 1=Tunnel  */

	/* encryption mode based on elliptic */
	ca_uint32_t		ealg:3;			/* ca_ipsec_cipher_alg_t  */
	ca_uint32_t		ealg_mode:4;		/* ca_ipsec_cipher_mode_t */
	ca_uint8_t		enc_keylen;
	ca_uint32_t		iv_len:4;		/* (n * 4) bytes for CBC  */

	/* authentication mode based on elliptic */
	ca_uint32_t		aalg:5;			/* ca_ipsec_hash_alg_t    */
	ca_uint8_t		auth_keylen;
	ca_uint32_t		icv_trunclen:4;		/* (n * 4) bytes          */
	ca_uint32_t		etherIP:1;
	ca_uint32_t		reserved:3;

	ca_uint32_t		is_natt;		/* NAT-Traversal          */
	ca_uint16_t		src_l4_port;		/* UDP port for IPSec NAT-Traversal */
	ca_uint16_t		dest_l4_port;		/* UDP port for IPSec NAT-Traversal (default 4500) */
	ca_tunnel_direction_t	dir;			/* SA direction, see definition of ca_tunnel_direction_t */

	ca_l3_ip_addr_t		tunnel_saddr;		/* NOTE: network order */
	ca_l3_ip_addr_t		tunnel_daddr;		/* NOTE: network order */

	ca_uint32_t		lifetime_bytes;
	ca_uint32_t		bytes_count;
	ca_uint32_t		lifetime_packets;
	ca_uint32_t		packets_count;

	ca_uint16_t		ip_checksum;
	ca_uint8_t		copy_ip_id;
	ca_uint8_t		copy_tos;
	ca_uint8_t		default_tos;	/* Set the default dscp value */
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_set_t;

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_set_ack_t;

/* set SADB key */

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint8_t		akey[MAX_AUTH_KEY_LEN];		/* auth key */
	ca_uint8_t		ekey[MAX_ENCRYPTION_KEY_LEN];	/* encryption key */
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_key_set_t;

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_key_set_ack_t;

/* del SADB */

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_del_t;

typedef struct {
	ca_uint32_t		ipsec_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		offload_seq;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_ipsec_ipc_sadb_del_ack_t;

/***************************************************************
 * CA_L2TP_IPC_ENTRY
 ***************************************************************/

typedef struct {
	ca_uint32_t		l2tp_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_l2tp_tunnel_cfg_t	l2tp;

	/* For L2TP over IPSec */
	ca_uint16_t		ip_ver;
	ca_uint16_t		ip_checksum;
	ca_l3_ip_addr_t		tunnel_saddr;		/* NOTE: network order */
	ca_l3_ip_addr_t		tunnel_daddr;		/* NOTE: network order */
} __attribute__((__packed__)) ca_l2tp_ipc_entry_set_t;

typedef struct {
	ca_uint32_t		l2tp_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_l2tp_ipc_entry_set_ack_t;

typedef struct {
	ca_uint32_t		l2tp_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_l2tp_ipc_entry_del_t;

typedef struct {
	ca_uint32_t		l2tp_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		offload_seq;
} __attribute__((__packed__)) ca_l2tp_ipc_entry_del_ack_t;

/***************************************************************
 * CA_MAP_E_IPC_ENTRY
 ***************************************************************/

typedef struct {
	ca_uint32_t		map_e_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint8_t		remote_mac[6];
	ca_uint8_t		local_mac[6];
	ca_ip_address_t		remote_ipv6;	/* BR IPv6 address */
	ca_ip_address_t		local_ipv6;	/* CE IPv6 address */
	ca_uint8_t		copy_tos;
	ca_uint8_t		default_tos;
	ca_uint8_t		hop_limit;

	ca_uint16_t		v4id_base;
	ca_uint16_t		v4id_mask;
	ca_uint16_t		port_set_id;
	ca_uint16_t		port_set_id_length;
	ca_uint16_t		port_set_id_offset;
} __attribute__((__packed__)) ca_map_e_ipc_entry_set_t;

typedef struct {
	ca_uint32_t		map_e_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_map_e_ipc_entry_set_ack_t;

typedef struct {
	ca_uint32_t		map_e_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_map_e_ipc_entry_del_t;

typedef struct {
	ca_uint32_t		map_e_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_map_e_ipc_entry_del_ack_t;

/***************************************************************
 * CA_MAP_T_IPC_ENTRY
 ***************************************************************/

typedef struct {
	ca_uint32_t		map_t_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint8_t		remote_mac[6];
	ca_uint8_t		local_mac[6];
	ca_ip_address_t		remote_ip;	/* It's BR IPv6 prefix for upstream and downstream. */
	ca_ip_address_t		local_ip;	/* It's CE IPv6 addr for upstream, and CE IPv4 addr for downstream. */
	ca_uint8_t		copy_tos;
	ca_uint8_t		default_tos;

	ca_uint8_t		hop_limit;
	ca_uint16_t		v4id_base;
	ca_uint16_t		v4id_mask;
	ca_uint16_t		port_set_id;
	ca_uint16_t		port_set_id_length;
	ca_uint16_t		port_set_id_offset;
} __attribute__((__packed__)) ca_map_t_ipc_entry_set_t;

typedef struct {
	ca_uint32_t		map_t_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_map_t_ipc_entry_set_ack_t;

typedef struct {
	ca_uint32_t		map_t_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_map_t_ipc_entry_del_t;

typedef struct {
	ca_uint32_t		map_t_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_map_t_ipc_entry_del_ack_t;

/***************************************************************
 * CA_MACSEC_IPC_ENTRY
 ***************************************************************/

typedef struct {
	ca_uint32_t		macsec_idx4pe;
	ca_tunnel_direction_t	dir;

	ca_uint32_t		ealg:3;			/* ca_ipsec_cipher_alg_t  */
	ca_uint32_t		ealg_mode:4;		/* ca_ipsec_cipher_mode_t */
	ca_uint8_t		enc_keylen;
	ca_uint32_t		iv_len:4;		/* (n * 4) bytes for CBC  */
	ca_uint8_t		ekey[MAX_ENCRYPTION_KEY_LEN];	/* encryption key */

	/* SecTAG */
	ca_uint8_t		tci_an;
	ca_uint32_t		packet_number;
	ca_uint8_t		secure_channel_id[8]; /* optional */
} __attribute__((__packed__)) ca_macsec_ipc_sadb_set_t;

typedef struct {
	ca_uint32_t		macsec_idx4pe;
	ca_tunnel_direction_t	dir;
	ca_uint32_t		ret_value;
} __attribute__((__packed__)) ca_macsec_ipc_sadb_set_ack_t;

typedef struct {
	ca_uint32_t		macsec_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_macsec_ipc_sadb_del_t;

typedef struct {
	ca_uint32_t		macsec_idx4pe;
	ca_tunnel_direction_t	dir;
} __attribute__((__packed__)) ca_macsec_ipc_sadb_del_ack_t;

#endif /* __TUNNEL_IPC_INTERNAL_H__ */

