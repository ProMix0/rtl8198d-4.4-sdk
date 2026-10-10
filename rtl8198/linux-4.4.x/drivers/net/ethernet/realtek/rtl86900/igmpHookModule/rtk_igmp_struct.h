#ifndef _RTK_IGMP_STRUCT_H
#define _RTK_IGMP_STRUCT_H

#include <common/rt_type.h>
#include <common/type.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
//#include <linux/if_vlan.h>
//#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
//#include <linux/tcp.h>
#include <linux/udp.h>
#include <uapi/linux/netfilter_bridge.h>
//#include <uapi/linux/netfilter_arp.h>
#include <linux/netfilter_bridge.h>
//#include <linux/inet.h>
#include <rt_igmpHook_ext.h>


#define IGMP_MAX_BR_MODULE_NUM 8
#define IGMP_MAX_DEV_NUM (IGMP_MAX_WAN_DEV_NUM+IGMP_MAX_LAN_DEV_NUM)
#define IGMP_MAX_WAN_DEV_NUM 32
#define IGMP_MAX_LAN_DEV_NUM 64

/* common */
#ifndef NULL
#define NULL  ((void *) 0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILED
#define FAILED	-1
#endif

#ifndef FAIL
#define FAIL -1
#endif

#if 0
#ifndef ENABLE
#define ENABLE 1
#endif

#ifndef DISABLE
#define DISABLE 0
#endif
#endif


#define rtlglue_printf printk
#ifndef bzero
#define bzero(p,s) memset(p,0,s)
#endif

int igmp_assert_eq(int func_return,int expect_return,const char* func,int line);

#ifndef ASSERT_EQ
#define ASSERT_EQ(value1, value2) \
do { \
	int func_return=(value1); \
	if(igmp_assert_eq((func_return), (value2), __FUNCTION__,__LINE__)!=0) \
		return func_return; \
}while(0)
#endif


typedef enum rtk_igmp_debug_level_e
{
	RTK_IGMP_DEBUG_LEVEL_WARNING=			0x001,
	RTK_IGMP_DEBUG_LEVEL_HOOK_POINT=		0x002,
	RTK_IGMP_DEBUG_LEVEL_PARSER=			0x004,
	RTK_IGMP_DEBUG_LEVEL_IGMPCTRL=			0x008,
	RTK_IGMP_DEBUG_LEVEL_FLOWDATA=			0x010,
	RTK_IGMP_DEBUG_LEVEL_HWCB=				0x020,
	RTK_IGMP_DEBUG_LEVEL_DUMP=				0x040,
	RTK_IGMP_DEBUG_LEVEL_RATE_LIMIT_DROP=	0x080,
	RTK_IGMP_DEBUG_LEVEL_API=				0x100
} rtk_igmp_debug_level_t;


#define NF_RETURN	 0xfd
#define NF_CONTINUE  0xfe

typedef enum rtk_igmp_nf_ret_e
{
	RTK_IGMP_NF_DROP	=	NF_DROP,
	RTK_IGMP_NF_ACCEPT	=	NF_ACCEPT,
	RTK_IGMP_NF_CONTINUE=	NF_CONTINUE,	//continue to learning igmp packet
	RTK_IGMP_NF_RETURN	=	NF_RETURN,		//break this record and continue to next (for IGMPv3 multi-record)
}rtk_igmp_nf_ret_e_t;



#define IGMP_ERR_START 0x10000
/* ERROR NO =============================================================== */
typedef enum rtk_igmp_enum_err_e
{
	RTK_IGMP_ERR_OK=0,
	RTK_IGMP_ERR_FAILED = (IGMP_ERR_START+1), 									//0x10001
	RTK_IGMP_TABLE_FULL,
	RTK_IGMP_ENTRY_NOT_FOUND,
}rtk_fc_enum_err_t;

#define RATE_LIMIT_CYCLE_DEFINE_SECONDS 1
typedef struct rate_limit_s
{
	uint32  rate; /*rate limit: Accept "rate" packets per time cycle*/
	uint32  counter; /*How many packes has been recevied in current time cycle.*/
	unsigned long cycle_start_time;
}rate_limit_t;

typedef enum rtk_rate_limit_ret_e
{
	RTK_RATE_LIMIT_ACCEPT_PACKET=0,
	RTK_RATE_LIMIT_DROP_PACKET
}rtk_rate_limit_ret_t;

typedef struct rtk_igmpv4_control_s
{
	/* should add mapping rt_igmpHook_control_list_t*/
	uint8 	igmpVerIgr_v1:1;
	uint8 	igmpVerIgr_v2:1;
	uint8 	igmpVerIgr_v3:1;
	uint8 	igmpVerDrp_v1:1;
	uint8 	igmpVerDrp_v2:1;
	uint8 	igmpVerDrp_v3:1;
	uint8	igmpReportDrop:1;
	uint8 	igmpSnoopingDisable:1;
	uint8 	igmpfastLeave:1;			//for igmpv2
	uint8 	igmpv3CareSource:1;
	uint8	igmpOverWeightDrop:1;	
	uint16 	igmpMaxGroupSize;		//0:disable
	uint16	igmpMcMemberAgingInterval;		//groupAgingTime linux -> multicast_membership_interval
	uint16	igmpMcLastMemberAgingInterval;	//group short timeout linux -> multicast_last_member_interval
	uint32	igmpMaxWeight;

	struct  list_head white;
	struct  list_head block;
	rate_limit_t  ratelimit;
	//whiteList* ,blockList* per-port linkList
}rtk_igmpv4_control_t;


typedef struct rtk_igmpv6_control_s
{
	/* should add mapping rt_igmpHook_control_list_t*/
	uint8	igmp6VerIgr_v1:1;
	uint8	igmp6VerIgr_v2:1;
	uint8 	igmp6VerDrp_v1:1;
	uint8 	igmp6VerDrp_v2:1;
	uint8	igmp6ReportDrop:1;
	uint8 	igmp6SnoopingDisable:1;
	uint8 	igmp6fastLeave:1;			//for mldv1
	uint8 	mldv2CareSource:1;
	uint32	igmp6OverWeightDrop:1;
	uint16 	igmp6MaxGroupSize;		//0:disable
	uint16	igmp6McMemberAgingInterval;		//groupAgingTime linux -> multicast_membership_interval(26000ms)
	uint16	igmp6McLastMemberAgingInterval;	//group short timeout linux -> multicast_last_member_interval
	uint32	igmp6MaxWeight;
	struct  list_head white;
	struct  list_head block;
	rate_limit_t  ratelimit;
	//whiteList* ,blockList* per-port linkList
}rtk_igmpv6_control_t;



typedef struct rtk_igmp_groupCbEvt_s
{
	int32 isIPv6;
	int32 vlan;
	uint32 group[4];
	uint8 minusGroupRefCntOnly:1;
}rtk_igmp_groupCbEvt_t;

typedef struct rtk_igmp_flowToDevInfo_s
{
	uint8  	valid:1;
	// uint8  	egressCtagif:1;
	// uint8	egressCpri:3;
	uint16  devIfIdx;
	//int16  	egressVlan;

}rtk_igmp_flowToDevInfo_t;

typedef struct rtk_igmp_flowCbEvt_s
{
	int32 isIPv6;
	int32 ingressCvlan;		//-1 untag
	int32 isGwMac;
	uint32 group[4];
	uint32 sourceAddr[4];
	int32 l4proto;
	int32 dport;
	int32 sport;
	int32 floodflowdelete;
	rtk_igmp_flowToDevInfo_t egressDevInfo[IGMP_MAX_DEV_NUM];
}rtk_igmp_flowCbEvt_t;

typedef enum rtk_igmp_unknownCtrl_s
{
	UNKNOWN_MC_DROP=0,
	UNKNOWN_MC_TRAP,
}rtk_igmp_unknownMcCtrl_t;


typedef struct rtk_igmp_unknownMcCbEvt_s
{
	uint32 isIpv6;
	rtk_igmp_unknownMcCtrl_t unknownAct;
}rtk_igmp_unknownMcCbEvt_t;


typedef struct rtk_igmp_hwCbEvt_s
{
	int (*hwRegInit)(void);
	int (*hwUnRegClear)(void);
	int (*hwUnknownIpMcAction) (rtk_igmp_unknownMcCbEvt_t *);
	int (*groupAddCbEvt)(rtk_igmp_groupCbEvt_t *groupCb);
	int (*groupDelCbEvt)(rtk_igmp_groupCbEvt_t *groupCb);
	int (*flowSetCbEvt)(rtk_igmp_flowCbEvt_t *flowCb);
	int (*flowDelCbEvt)(rtk_igmp_flowCbEvt_t *flowCb);

}rtk_igmp_hwCbEvt_t;

typedef struct rtk_igmp_brConfdb_s
{
	//uint16 igmp6GenQuerySec;
	//uint16 igmpGenQuerySec;
	uint16 sysMaxGroupSize;			//0:disable
	uint16 sysIp4MaxGroupSize;		//0:disable
	uint16 sysIp6MaxGroupSize;		//0:disable

	uint16 sysMaxClientSize;		//0:disable
	uint16 sysIp4MaxClientSize;		//0:disable
	uint16 sysIp6MaxClientSize;		//0:disable

	uint16 perIp4ClientMaxJoinGroupSize;//0:disable
	uint16 perIp6ClientMaxJoinGroupSize;//0:disable

	uint16 sysIp4MaxWeight;		//0:disable
	uint16 sysIp6MaxWeight;		//0:disable

	uint32	sysIp4OverWeightDrop:1;
	uint32	sysIp6OverWeightDrop:1;

	//whiteList* ,blockList* System linkList

/*
	TODO List: kernel
	cat	sys/devices/virtual/net/br0/bridge/
	multicast_last_member_count (2)
	multicast_query_response_interval (1000ms)
	multicast_last_member_interval (100ms)
	multicast_query_use_ifaddr
	(v)multicast_membership_interval(26000ms)
	multicast_router(1 ?)
	multicast_querier(0 ?)
	(v)multicast_snooping (enable/disable)
	multicast_querier_interval(25500ms)
	multicast_startup_query_count(2)
	multicast_query_interval(12500ms)
	multicast_startup_query_interval(3125)

# cat sys/devices/virtual/net/nas0_0/brport/
	(v)multicast_fast_leave
	multicast_router

*/

}rtk_igmp_brConfdb_t;

typedef struct rtk_igmp_statistic_s
{

	uint32 igmp4JoinCnt;
	uint32 igmp6JoinCnt;

	uint32 igmp4WeightExceedCnt;
	uint32 igmp6WeightExceedCnt;

	
}rtk_igmp_statistic_t;


typedef struct rtk_igmp_globalConfdb_s
{
	uint32 igmp_sys_timer_sec;

}rtk_igmp_globalConfdb_t;

typedef struct rtk_igmp_DevConfdb_s
{
	rtk_igmpv4_control_t igmp;
	rtk_igmpv6_control_t igmp6;
}rtk_igmp_DevConfdb_t;

typedef struct rtk_igmp_multicastDeviceInfo_s
{
	char devName[32];
	struct net_device *dev;
	rtk_igmp_DevConfdb_t devConf;
	rtk_igmp_statistic_t devStatistic;
	// uint8 forceDevCTag:1;
	// uint8 forceDevCTagif:1;
	// int16 forceDevCvid;
	uint16 ifindex;
	/*gateway infomation*/
	uint8 rtk_igmp_gatewayMac[6];
	//uint32 rtk_igmp_gatewayIpv4Addr;
	//uint32 rtk_igmp_gatewayIpv6Addr[4];
	int32 bind2BrModuleIdx; //for wan dev only

}rtk_igmp_multicastDeviceInfo_t;


typedef struct rtk_igmp_pktHdr_s
{
	struct ethhdr *ethHdr;
	//struct vlan_hdr *cvh;
	struct ipv6hdr *ip6h;
	struct iphdr * iph;
	struct igmphdr * igmph;
	struct icmp6hdr *icmp6h;
	struct udphdr *udph;
	struct sk_buff *skb;
	uint8 isDipMc:1;
	uint8 ingressCtagif:1;
	int16  ingressCvid;
	uint16 l4proto;
	int32 devIfindex;
	uint32 priflag;
	rtk_igmp_multicastDeviceInfo_t *devInfo;
	int32 moduleIndex;

} rtk_igmp_pktHdr_t;


typedef struct rtk_igmp_Sysdb_s
{
	rtk_igmp_debug_level_t igmp_debug_level;
	spinlock_t lock_igmp;
	int32 disHook;
	int32 hwctrl;
	int32 unknownFlood;
}rtk_igmp_Sysdb_t;


typedef struct rtk_igmp_whiteListEntry_s
{
	uint8  valid:1;
	rt_igmpHook_whiteList_t patten;
	uint32 whiteListIdx;
	struct list_head entry;
}rtk_igmp_whiteListEntry_t;



typedef struct rtk_igmp_userResvGroup_s
{
	uint8  valid:1;
	uint32 resvGrpListIdx;
	rt_igmpHook_ignoreGroup_t patten;
	struct list_head entry;
}rtk_igmp_userResvGroup_t;

typedef struct rtk_igmp_groupWeight_s
{
	uint8  valid:1;
	uint32 groupWeightListIdx;
	uint32 devIdx;
	rt_igmpHook_groupWeight_t patten;
	struct list_head entry;
}rtk_igmp_groupWeight_t;



#endif
