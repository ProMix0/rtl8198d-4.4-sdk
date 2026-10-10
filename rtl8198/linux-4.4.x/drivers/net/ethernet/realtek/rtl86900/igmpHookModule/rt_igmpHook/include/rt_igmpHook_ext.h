#ifndef _RT_IGMPHOOK_EXT_H
#define _RT_IGMPHOOK_EXT_H

#define ETHER_ADDR_LEN 6

typedef enum rt_igmp_control_list_e
{
/* WARNING: don't move  start for diag using*/
	RT_DEV_INVAILD=0,
	RT_SNOOPING_DISABLE=1,
	RT_FAST_LEAVE=2,
	RT_MAX_GROUP_SIZE=3,
	RT_MC_MEMBER_AGING_INTERVAL=4,
	RT_CARE_SOURCE=5,
	RT_RATE_LIMIT=6,
/* WARNING: don't move	End for diag using*/
	RT_MC_LAST_MEMBER_AGING_INTERVAL,
	RT_FLUSH_MDB,
	RT_FLUSH_CONF,
	RT_REPORT_DROP,
	RT_DROP_IGMPV1,
	RT_DROP_IGMPV2,
	RT_DROP_IGMPV3,
	RT_IGNORE_IGMPV1,
	RT_IGNORE_IGMPV2,
	RT_IGNORE_IGMPV3,
	RT_DROP_MLDV1,
	RT_DROP_MLDV2,
	RT_IGNORE_MLDV1,
	RT_IGNORE_MLDV2,
	RT_MAX_WEIGHT,
	RT_DROP_OVER_WEIGHT,
//  RT_CONTROL_CONFIG_MAX should be last
	RT_IGMP_CONTROL_CONFIG_MAX,
}rt_igmpHook_control_list_t;

typedef enum rt_igmpHook_br_control_list_e
{	
/* WARNING: don't move	start for diag using*/
	RT_BR_INVAILD=0,
	RT_BR_MAX_GROUP_SIZE=1,
	RT_BR_MAX_IP4_GROUP_SIZE=2,
	RT_BR_MAX_IP6_GROUP_SIZE=3,
	RT_BR_MAX_CLIENT_SIZE=4,
	RT_BR_MAX_IP4_CLIENT_SIZE=5,
	RT_BR_MAX_IP6_CLIENT_SIZE=6,
/* WARNING: don't move	End for diag using*/	
	RT_BR_PER_IP4_CLIENT_MAX_JOIN_GROUP,
	RT_BR_PER_IP6_CLIENT_MAX_JOIN_GROUP,
	RT_BR_SNOOPING_DISABLE,
	RT_BR_FLUSH_CONF,
	RT_BR_FLUSH_MDB,
	RT_BR_RATE_LIMIT,
	RT_BR_FAST_LEAVE,
	RT_BR_DROP_IGMPV1,
	RT_BR_DROP_IGMPV2,
	RT_BR_DROP_IGMPV3,
	RT_BR_DROP_MLDV1,
	RT_BR_DROP_MLDV2,
	RT_BR_MAX_IP4_WEIGHT,
	RT_BR_MAX_IP6_WEIGHT,
	RT_BR_DROP_IP4_OVER_WEIGHT,
	RT_BR_DROP_IP6_OVER_WEIGHT,
//	RT_BR_CONTROL_CONFIG_MAX should be last
	RT_BR_CONTROL_CONFIG_MAX,
}rt_igmpHook_br_control_list_t;


typedef struct rtk_igmpHook_whiteList_s
{
	unsigned char  smacChk:1;
	unsigned char  sipChk:1;		//lan host sip
	unsigned char  gipChk:1;
	unsigned char  isIpv6:1;
	unsigned char  mcSipChk:1;		//multicast sipfilter
	unsigned char  smac[ETHER_ADDR_LEN];
	unsigned char  smacMask[ETHER_ADDR_LEN];

	unsigned int  wlSipStart[4];			//if v4 using host endina store in arry[0],if v6 using net endian store in array
	unsigned int  wlSipEnd[4];

	unsigned int  wlMcSip[4];
	unsigned int  wlMcSipEnd[4];

	unsigned int  wlGroupIpStart[4];
	unsigned int  wlGroupIpEnd[4];
}rt_igmpHook_whiteList_t;


typedef struct rt_igmpHook_devInfo_s
{
	char devIfname[32];
	int devIfidx;
}rt_igmpHook_devInfo_t;


typedef struct rt_igmpHook_ignoreGroup_s
{
	unsigned char   isIpv6:1;
	unsigned int	ignGroupIpStart[4];
	unsigned int	ignGroupIpEnd[4];
}rt_igmpHook_ignoreGroup_t;


typedef struct rt_igmpHook_groupWeight_s
{
    unsigned char	isIpv6:1;
    unsigned int	weightGroupIpStart[4];
    unsigned int	weightGroupIpEnd[4];
    unsigned int	weight;
}rt_igmpHook_groupWeight_t;

typedef struct rt_igmpHook_devStatistic_s
{
    unsigned int    curWeight;			//total weight of this device group 
    unsigned int    joinCnt;			//total of IGMP join receive
    unsigned int    bandwidthExceedCnt;	//exceed bandwidth and count IGMP join
    unsigned int    activeGroupCnt;		//total of active group counter
}rt_igmpHook_devStatistic_t;


typedef struct rt_igmpHook_groupStatistic_s
{
	unsigned int	groupAddr[4];
	unsigned int	sourceAddr[4];
	short			vlanId;
	unsigned int	curClient[4];
	unsigned int	curClientExistTimesSec;
}rt_igmpHook_groupStatistic_t;


extern int rt_igmpHook_whiteList_add( rt_igmpHook_whiteList_t *patten,rt_igmpHook_devInfo_t ifidx , int *index);
extern int rt_igmpHook_whiteList_del(rt_igmpHook_whiteList_t *patten,rt_igmpHook_devInfo_t ifidx,int index);

extern int rt_igmpHook_devConfig_set(rt_igmpHook_devInfo_t devInfo , unsigned int isIpv6 ,rt_igmpHook_control_list_t type , unsigned int value);
extern int rt_igmpHook_devConfig_get(rt_igmpHook_devInfo_t devInfo , unsigned int isIpv6 ,rt_igmpHook_control_list_t type , unsigned int* value);

extern int rt_igmpHook_brDevConfig_set(rt_igmpHook_devInfo_t devInfo ,rt_igmpHook_br_control_list_t type , unsigned int value);
extern int rt_igmpHook_brDevConfig_get(rt_igmpHook_devInfo_t devInfo ,rt_igmpHook_br_control_list_t type , unsigned int* value);


extern int rt_igmpHook_groupToPsTbl_add(rt_igmpHook_ignoreGroup_t* patten,unsigned int *index);
extern int rt_igmpHook_groupToPsTbl_del(rt_igmpHook_ignoreGroup_t* patten,unsigned int index);

extern int rt_igmpHook_nextValidGroupStatistic_get(rt_igmpHook_devInfo_t devInfo,int *getIdx,unsigned int isIpv6,rt_igmpHook_groupStatistic_t *statisticInfo);

extern int rt_igmpHook_devStatistic_get(rt_igmpHook_devInfo_t devInfo,unsigned int isIpv6,rt_igmpHook_devStatistic_t *statisticInfo);
 
extern int rt_igmpHook_groupWeight_add(rt_igmpHook_devInfo_t devInfo,rt_igmpHook_groupWeight_t* groupWeightCfg, int *index);
extern int rt_igmpHook_groupWeight_del(rt_igmpHook_devInfo_t devInfo,rt_igmpHook_groupWeight_t* groupWeightCfg, int  index);



#endif

