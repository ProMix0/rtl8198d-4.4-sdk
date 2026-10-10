#ifndef __AAL_L3_DDOS_H__
#define __AAL_L3_DDOS_H__

#include "ca_types.h"


typedef enum {
    AAL_DOS_ID_TYPE_ICMP_FLOOD		= 0,
    AAL_DOS_ID_TYPE_FIN_FLOOD		= 1,
    AAL_DOS_ID_TYPE_SYN_FLOOD		= 2,
    AAL_DOS_ID_TYPE_SYN_WITH_DATA	= 3,
    AAL_DOS_ID_TYPE_UDP_BOMB		= 4,
    AAL_DOS_ID_TYPE_PING_OF_DEATH	= 5,
    AAL_DOS_ID_TYPE_ICMP_FRAGMENT	= 6,
    AAL_DOS_ID_TYPE_TCP_FRAG_ERROR	= 7,
    AAL_DOS_ID_TYPE_TCP_SHORT_HDR	= 8,
    AAL_DOS_ID_TYPE_SYN1024			= 9,
    AAL_DOS_ID_TYPE_NULL_SCAN		= 10,
    AAL_DOS_ID_TYPE_XMA_SCAN		= 11,
    AAL_DOS_ID_TYPE_SYN_FIN_SCAN	= 12,
    AAL_DOS_ID_TYPE_BLAT_ATTACKS	= 13,
    AAL_DOS_ID_TYPE_LAND_ATTACKS	= 14,
    AAL_DOS_ID_TYPE_SMAC_EQ_DMAC	= 15,
} aal_dos_type_id_t;

/***************************************************************************
 * L3FE_PP_DOS_ATK_xx
 ***************************************************************************/

typedef union {
	struct {
		ca_uint32_t	icmp_flood_pol_id	: 1;
		ca_uint32_t	icmp_flood_th		: 1;
		ca_uint32_t	icmp_flood		: 1;

		ca_uint32_t	fin_flood_pol_id	: 1;
		ca_uint32_t	fin_flood_th		: 1;
		ca_uint32_t	fin_flood		: 1;

		ca_uint32_t	syn_flood_pol_id	: 1;
		ca_uint32_t	syn_flood_th		: 1;
		ca_uint32_t	syn_flood		: 1;

		ca_uint32_t	syn_with_data_pol_id	: 1;
		ca_uint32_t	syn_with_data		: 1;

		ca_uint32_t	udp_bomb_pol_id		: 1;
		ca_uint32_t	udp_bomb		: 1;

		ca_uint32_t	ping_of_death_pol_id	: 1;
		ca_uint32_t	ping_of_death		: 1;

		ca_uint32_t	icmp_fragment_pol_id	: 1;
		ca_uint32_t	icmp_fragment		: 1;

		ca_uint32_t	tcp_frag_error_pol_id	: 1;
		ca_uint32_t	tcp_frag_error		: 1;

		ca_uint32_t	tcp_short_hdr_pol_id	: 1;
		ca_uint32_t	tcp_short_hdr		: 1;

		ca_uint32_t	syn1024_pol_id		: 1;
		ca_uint32_t	syn1024		: 1;

		ca_uint32_t	null_scan_pol_id	: 1;
		ca_uint32_t	null_scan		: 1;

		ca_uint32_t	xma_scan_pol_id		: 1;
		ca_uint32_t	xma_scan		: 1;

		ca_uint32_t	syn_fin_scan_pol_id	: 1;
		ca_uint32_t	syn_fin_scan		: 1;

		ca_uint32_t	blat_attacks_pol_id	: 1;
		ca_uint32_t	blat_attacks		: 1;

		ca_uint32_t	land_attacks_pol_id	: 1;
		ca_uint32_t	land_attacks		: 1;

		ca_uint32_t	smac_eq_dmac_pol_id	: 1;
		ca_uint32_t	smac_eq_dmac		: 1;

		ca_uint32_t	dos_type_en		: 1;
		ca_uint32_t	dos_port_mask		: 1;
	} s;
	ca_uint32_t	u32;
} aal_dos_fib_mask_t;

typedef struct {
	/* ICMP flood */
	ca_uint32_t	icmp_flood_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	icmp_flood_th		: 16;	/* System-based ICMP flood threshold, time unit 1ms */
	ca_uint32_t	icmp_flood		: 1;	/* Receiving ICMP packet number is over threshold. 1'b0: drop, 1'b1: trap. */

	/* TCP FIN flood */
	ca_uint32_t	fin_flood_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	fin_flood_th		: 16;	/* threshold, time unit 1ms */
	ca_uint32_t	fin_flood		: 1;	/* Receiving TCP FIN packet number is over threshold. 1'b0: drop, 1'b1: trap. */

	/* TCP SYN flood */
	ca_uint32_t	syn_flood_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	syn_flood_th		: 16;	/* threshold, time unit 1ms */
	ca_uint32_t	syn_flood		: 1;	/* Receiving TCP SYN packet number is over threshold. 1'b0: drop, 1'b1: trap. */

	/* 1. IP length > IP header + TCP header length while SYN flag is set 1
	   2. IP More Fragment or Offset > 0 while SYN is set to 1
	 */
	ca_uint32_t	syn_with_data_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	syn_with_data		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* UDP length > IP payload length(only works while More Fragment = 0 and Offset = 0) (support single IP4/IP6 or inner IP4/IP6) */
	ca_uint32_t	udp_bomb_pol_id		: 9;	/* policer_id when trapped */
	ca_uint32_t	udp_bomb		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* IP packet size > 65535 bytes, (support single IP4/IP6 or inner IP4/IP6)
		1) ((Ipv4 offset *8) + (IP length))> 65535
		2) IP6 fragment offset*8 + L4 length >  65535
	*/
	ca_uint32_t	ping_of_death_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	ping_of_death		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* ICMPv4/ICMPv6 data unit carried in a fragmented IP datagram */
	ca_uint32_t	icmp_fragment_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	icmp_fragment		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* the Fragment_Offset=1 in any fragment of  a fragmented IP datagram carrying part of TCP data */
	ca_uint32_t	tcp_frag_error_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	tcp_frag_error		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* the length of a TCP header carried in an unfragmented IP(Ipv4 and Ipv6) datagram
	   or the first fragment of a fragmented IP(Ipv4 and Ipv6) datagram is less than MIN_TCP_Header_Size(20 bytes)
	 */
	ca_uint32_t	tcp_short_hdr_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	tcp_short_hdr		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* TCP SYN packets with source port less than 1024 */
	ca_uint32_t	syn1024_pol_id		: 9;	/* policer_id when trapped */
	ca_uint32_t	syn1024		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* TCP packets while sequence number is zero and all contorl bits are zeros.
	   p.s. NULL is for TCP connection code defined in RFC793, at that time TCP flag is defined for U/A/P/R/S/F only.
	 */
	ca_uint32_t	null_scan_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	null_scan		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* TCP packets with zero sequence number and FIN,URG,PSH bits set */
	ca_uint32_t	xma_scan_pol_id		: 9;	/* policer_id when trapped */
	ca_uint32_t	xma_scan		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/*  TCP packets with both SYN and FIN flags set */
	ca_uint32_t	syn_fin_scan_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	syn_fin_scan		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/*  TCP/UDP SPORT is the same as DPORT destination TCP/UDP port(support single or (L3FE) inner L4 only) */
	ca_uint32_t	blat_attacks_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	blat_attacks		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* packets with same SIP and DIP (L3FE support single IP4/IP6 or inner IP4/IP6, L2FE supports single IP4/IP6) */
	ca_uint32_t	land_attacks_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	land_attacks		: 1;	/* 1'b0: drop, 1'b1: trap. */

	/* packets with same SMAC and DMAC */
	ca_uint32_t	smac_eq_dmac_pol_id	: 9;	/* policer_id when trapped */
	ca_uint32_t	smac_eq_dmac		: 1;	/* 1'b0: drop, 1'b1: trap. */

	ca_uint32_t	dos_type_en		: 16;	/*  per DOS ATTACK type filtering enable */
	ca_uint64_t	dos_port_mask		: 64;	/* port mask for DOS filters (define for physical ports). 1'b0: disabled, 1'b1: enabled */
} __attribute__((packed)) aal_dos_fib_t;

/***************************************************************************
 * L3FE_PP_DOS_ATK_FLOOD_CNT
 ***************************************************************************/

typedef union {
	struct {
		ca_uint32_t	cnt		: 1;
	} s;
	ca_uint32_t	u32;
} aal_dos_atk_flood_cnt_mask_t;

typedef struct {
	ca_uint32_t		cnt		:10;
	ca_uint32_t		reserved:22;
} __attribute__((packed)) aal_dos_atk_flood_cnt_t;

/***************************************************************************
 * L3FE_PP_DOS_ATK_EVENT_TRAP
 ***************************************************************************/

typedef union {
	struct {
		ca_uint32_t	rdir_ldpid		: 1;
		ca_uint32_t	rdir_cos		: 1;
		ca_uint32_t keep_orig_pkt	: 1;
		ca_uint32_t	reserved		: 1;
		ca_uint32_t	t0_ctrl			: 1;
		ca_uint32_t	t1_ctrl			: 1;
		ca_uint32_t	t2_ctrl			: 1;
		ca_uint32_t	t3_ctrl			: 1;
		ca_uint32_t	t4_ctrl			: 1;
		ca_uint32_t	t5_ctrl			: 1;
	} s;
	ca_uint32_t	u32;
} aal_dos_atk_event_trap_mask_t;

typedef struct {
	ca_uint32_t		rdir_ldpid		: 6;
	ca_uint32_t		rdir_cos		: 3;
	ca_uint32_t		keep_orig_pkt	: 1;
	ca_uint32_t		reserved		: 1;
	ca_uint32_t		t0_ctrl			: 1;
	ca_uint32_t		t1_ctrl			: 4;
	ca_uint32_t		t2_ctrl			: 4;
	ca_uint32_t		t3_ctrl			: 4;
	ca_uint32_t		t4_ctrl			: 4;
	ca_uint32_t		t5_ctrl			: 4;
} __attribute__((packed)) aal_dos_atk_event_trap_t;

/***************************************************************************
 * L3FE_PP_DOS_ATK_EVENT_DROP
 ***************************************************************************/

typedef union {
	struct {
		ca_uint32_t	rdir_ldpid		: 1;
		ca_uint32_t	rdir_cos		: 1;
		ca_uint32_t keep_orig_pkt	: 1;
		ca_uint32_t	reserved		: 1;
		ca_uint32_t	t0_ctrl			: 1;
		ca_uint32_t	t1_ctrl			: 1;
		ca_uint32_t	t2_ctrl			: 1;
		ca_uint32_t	t3_ctrl			: 1;
		ca_uint32_t	t4_ctrl			: 1;
		ca_uint32_t	t5_ctrl			: 1;
	} s;
	ca_uint32_t	u32;
} aal_dos_atk_event_drop_mask_t;

typedef struct {
	ca_uint32_t		rdir_ldpid		: 6;
	ca_uint32_t		rdir_cos		: 3;
	ca_uint32_t		keep_orig_pkt	: 1;
	ca_uint32_t		reserved		: 1;
	ca_uint32_t		t0_ctrl			: 1;
	ca_uint32_t		t1_ctrl			: 4;
	ca_uint32_t		t2_ctrl			: 4;
	ca_uint32_t		t3_ctrl			: 4;
	ca_uint32_t		t4_ctrl			: 4;
	ca_uint32_t		t5_ctrl			: 4;
} __attribute__((packed)) aal_dos_atk_event_drop_t;

/***************************************************************************
 * AAL functions
 ***************************************************************************/

ca_status_t aal_dos_atk_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_fib_mask_t		mask,
	CA_IN	aal_dos_fib_t			*fib
);

ca_status_t aal_dos_atk_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_fib_t			*fib
);

ca_status_t aal_dos_atk_flood_cnt_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_flood_cnt_mask_t	mask,
	CA_IN	aal_dos_atk_flood_cnt_t		*cnt
);

ca_status_t aal_dos_atk_flood_cnt_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_flood_cnt_t		*cnt
);

ca_status_t aal_dos_atk_event_trap_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_event_trap_mask_t	mask,
	CA_IN	aal_dos_atk_event_trap_t	*trap_cfg
);

ca_status_t aal_dos_atk_event_trap_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_event_trap_t	*trap_cfg
);

ca_status_t aal_dos_atk_event_drop_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_event_drop_mask_t	mask,
	CA_IN	aal_dos_atk_event_drop_t	*drop_cfg
);

ca_status_t aal_dos_atk_event_drop_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_event_drop_t	*drop_cfg
);

ca_status_t aal_dos_init(/*CODEGEN_IGNORE_TAG*/
	CA_IN ca_device_id_t dev
);

#endif /* __AAL_L3_DDOS_H__ */
