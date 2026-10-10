/*
 *	rtk_rnr_ext.h
 */

#ifndef _RTK_RNR_EXT_H_
#define _RTK_RNR_EXT_H_

#ifndef SIGNED_INVALID
#define SIGNED_INVALID -1
#endif

#define RNR_MR_PORT   			6

#define RNR_SEARCH_WHITE_LIST	1
#define RNR_SEARCH_BLACK_LIST	2
#define RNR_SEARCH_BOTH			(RNR_SEARCH_WHITE_LIST | RNR_SEARCH_BLACK_LIST)

#define RNR_IS_EF 				1
#define RNR_NOT_EF 				0

#define RNR_DEV_RATE_NUM		20
#define RNR_FLOW_CHG_NUM		63
#define FC_TABLESIZE_HW_FLOW 	32768
#define RNR_CLIENT_NUM			200

#define NEXT_IDX(idx, _MAX) \
	do { \
		idx++; \
		if (idx >= _MAX) \
			idx = 0; \
	} while(0);

typedef enum rnr_type_e
{
    RNR_TYPE_OTHER = 0,
    RNR_TYPE_IN_WHITE_LIST,
    RNR_TYPE_IN_BLACK_LIST
} rnr_type_t;

typedef enum rnr_set_e
{
    RNR_SET_DUMP = 0,
    RNR_SET_PRI,
    RNR_SET_DSCP,
    RNR_SET_ROLE,
    RNR_SET_HOLD_TIME
} rnr_set_t;

typedef enum rnr_traverse_flow_e
{
    RNR_TRAVERSE_FLOW_DUMP_RATE = 0,
    RNR_TRAVERSE_FLOW_DUMP_5TUPLE,
    RNR_TRAVERSE_FLOW_DEV_RATE,
    RNR_TRAVERSE_FLOW_LIST_CHG,
    RNR_TRAVERSE_FLOW_CLEAN,
    RNR_TRAVERSE_FLOW_GET_STATS
} rnr_traverse_flow_t;

typedef enum rnr_msg_e
{
    RNR_MSG_ENABLE = 0,
    RNR_MSG_DISABLE,
    RNR_MSG_WHITE,
    RNR_MSG_BLACK,
    RNR_MSG_FLOW
} rnr_msg_t;

typedef struct rtk_rnr_dev_rate_s
{
	uint32 out_rx_bytes;
	uint32 in_rx_bytes;
	uint16 out_ef;
	uint16 in_ef;
	uint16 out_non_ef;
	uint16 in_non_ef;
	uint8 mac[6];
} rtk_rnr_dev_rate_t;

struct flow_subtype {
	uint16 sport;
	uint16 dport;
	uint32 sip;
	uint32 dip;
	char protocol;
	char direction;
	char is_ef;
	char dummy3;
};

typedef struct rnr_wan_stats_s
{
	uint32 wan_num;
	uint32 wan1_out_rate; // unit: Kbps
	uint32 wan1_in_rate; // unit: Kbps
	uint32 wan2_out_rate; // unit: Kbps
	uint32 wan2_in_rate; // unit: Kbps
} rnr_wan_stats;

typedef struct rnr_client_stats_s
{
	uint8 mac[6];
	uint16 dummy;
	uint32 out_rate; // unit: Kbps
	uint32 in_rate; // unit: Kbps
	uint16 out_ef_num;
	uint16 in_ef_num;
	uint16 out_non_ef_num;
	uint16 in_non_ef_num;
} rnr_client_stats;

typedef struct rnr_ef_stats_s
{
	uint8 mac[6];
	uint8 protocol;
	uint8 direction;
	uint16 sport;
	uint16 dport;
	uint32 sip;
	uint32 dip;
	uint32 rate; // unit: Kbps
} rnr_ef_stats;

typedef struct rnr_result_s
{
	uint32 seq_id;
	rnr_wan_stats	wan_stats;

	uint32 client_num;
	rnr_client_stats	client[RNR_CLIENT_NUM];

	uint32 ef_num;
	rnr_ef_stats		ef[FC_TABLESIZE_HW_FLOW];
} rnr_result_t;

typedef int (*rtk_rnr_pkt_inspect_hook_t)(struct sk_buff *, unsigned int);
typedef unsigned int (*rtk_rnr_list_check_hook_t)(unsigned char *, unsigned int);
typedef void (*rtk_rnr_sendto_daemon_hook_t)(int , unsigned char *, int);

extern uint8 rtk_fc_rnr_enabled(int state);
extern uint8 rnr_wan_mac[];

#endif /*_RTK_RNR_EXT_H_*/

